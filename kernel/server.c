#include "console.h"
#include "elf.h"
#include "ipc.h"
#include "multiboot2.h"
#include "name.h"
#include "sched.h"
#include "server.h"
#include "types.h"
#include "vm.h"
#include <arch/user.h>
#include "../servers/vm/vm_server.h"

static const struct server boot_servers[] = {
	{ "names", 0 },
	{ "init", 0 },
	{ "block", 0 },
	{ "vfs", 0 },
	{ "ping", 0 },
	{ "vm", vm_server_start },
};

struct module_server_start {
	const struct server *server;
	u64 image_start;
	u64 image_end;
	u64 data_start;
	u64 data_end;
	struct vm_space *space;
	u64 stack;
	u32 launched;
};

struct boot_data_module {
	const char *name;
	u64 start;
	u64 end;
};

static struct module_server_start module_starts[16];
static u32 module_start_count;
static struct boot_data_module disk0_module;

static int str_eq(const char *left, const char *right);
static const struct server *find_boot_server(const char *name);

void server_start_all(void)
{
	const u32 count = sizeof(boot_servers) / sizeof(boot_servers[0]);

	for (u32 i = 0; i < count; i++) {
		console_printf("kernel: starting server %s\n", boot_servers[i].name);
		boot_servers[i].start();
	}
}

static void module_server_thread(void *arg)
{
	const struct module_server_start *start =
		(const struct module_server_start *)arg;

	console_printf("kernel: starting module server %s image=%p-%p\n",
		       start->server->name,
		       (const void *)start->image_start,
		       (const void *)start->image_end);

	if (start->server->start == 0) {
		u64 entry = 0;
		if (elf_load_user_image(start->space, start->image_start,
					start->image_end, &entry) == 0) {
			arch_user_enter(entry, start->stack);
		}
		return;
	}

	if (start->server->start != 0) {
		start->server->start();
	}
}

static const struct server *find_boot_server(const char *name)
{
	const u32 count = sizeof(boot_servers) / sizeof(boot_servers[0]);

	for (u32 i = 0; i < count; i++) {
		if (str_eq(name, boot_servers[i].name)) {
			return &boot_servers[i];
		}
	}

	return 0;
}

static int str_eq(const char *left, const char *right)
{
	while (*left != '\0' && *right != '\0') {
		if (*left++ != *right++) {
			return 0;
		}
	}

	return *left == *right;
}

static void mem_copy(u8 *dst, const u8 *src, u64 len)
{
	for (u64 i = 0; i < len; i++) {
		dst[i] = src[i];
	}
}

int server_launch_module(const char *name)
{
	return server_launch_module_with_caps(name, 0, 0, 0) == (u64)-1 ? -1 : 0;
}

static void grant_bootstrap_caps(struct task *task, const char *server_name)
{
	if (str_eq(server_name, "names")) {
		task_grant_port(task, ipc_port_find("console"),
				TASK_RIGHT_SEND | TASK_RIGHT_DUP);
		return;
	}

	if (!str_eq(server_name, "init")) {
		return;
	}

	task_grant_port(task, ipc_port_find("console"),
			TASK_RIGHT_SEND | TASK_RIGHT_DUP);
	task_grant_port(task, ipc_port_find("vm"),
			TASK_RIGHT_SEND | TASK_RIGHT_DUP);
	task_grant_port(task, ipc_port_find("names"),
			TASK_RIGHT_SEND | TASK_RIGHT_DUP);
}

u64 server_launch_module_with_caps(const char *name, struct task *parent,
				   const struct task_launch_cap *caps,
				   u64 cap_count)
{
	enum {
		USER_STACK_TOP = 0x800000,
		USER_STACK_PAGES = 4,
		MAX_INHERITED_HANDLES = 8,
	};
	if (cap_count > MAX_INHERITED_HANDLES) {
		console_printf("kernel: too many inherited caps for %s count=%u\n",
			       name, (u32)cap_count);
		return (u64)-1;
	}

	for (u32 i = 0; i < module_start_count; i++) {
		struct module_server_start *start = &module_starts[i];

		if (!str_eq(start->server->name, name)) {
			continue;
		}
		const char *server_name = start->server->name;

		if (start->launched) {
			console_printf("kernel: module server %s already launched\n",
				       server_name);
			return -1;
		}

		for (u64 cap = 0; cap < cap_count; cap++) {
			if (caps == 0 ||
			    caps[cap].reserved != 0 ||
			    task_can_inherit_handle(parent, caps[cap].handle,
						    caps[cap].rights) != 0) {
				console_printf("kernel: invalid inherited cap handle=%u rights=0x%x for %s\n",
					       caps != 0 ? (u32)caps[cap].handle : 0,
					       caps != 0 ? caps[cap].rights : 0,
					       name);
				return (u64)-1;
			}
		}

		console_printf("kernel: launching module server %s\n",
			       server_name);

		struct vm_space *space = vm_server_bootstrap_space(server_name);
		if (space == 0) {
			return -1;
		}

		struct task *task = task_create(server_name, space);
		if (task == 0) {
			return (u64)-1;
		}

		start->launched = 1;

		struct ipc_port *service_port = ipc_port_create(server_name);
		const u64 child_self =
			task_grant_port(task, service_port,
					TASK_RIGHT_SEND | TASK_RIGHT_RECV |
					TASK_RIGHT_DUP);
		if (child_self == 0) {
			return (u64)-1;
		}

		for (u64 cap = 0; cap < cap_count; cap++) {
			if (task_grant_inherited_handle(task, parent,
							caps[cap].handle,
							caps[cap].rights) == 0) {
				console_printf("kernel: invalid inherited cap handle=%u rights=0x%x for %s\n",
					       (u32)caps[cap].handle,
					       caps[cap].rights, name);
				return (u64)-1;
			}
		}

		if (parent == 0) {
			grant_bootstrap_caps(task, server_name);
		}

		for (u64 page = 0; page < USER_STACK_PAGES; page++) {
			const u64 vaddr = USER_STACK_TOP -
					  (page + 1) * VM_PAGE_SIZE;
			if (vm_alloc_user_page(space, vaddr, 1).addr == 0) {
				console_printf("kernel: failed to map stack for %s\n",
					       server_name);
				return (u64)-1;
			}
		}

		start->space = space;
		start->stack = USER_STACK_TOP;

		if (thread_create(task, server_name, module_server_thread,
				  start) == 0) {
			console_printf("kernel: failed to create server thread %s\n",
				       server_name);
			return (u64)-1;
		}

		if (!str_eq(server_name, "vm")) {
			name_service_register(server_name, NAME_SERVICE_TASK,
					      task_id(task));
		}

		return parent != 0 ?
		       task_grant_port(parent, service_port,
				       TASK_RIGHT_SEND | TASK_RIGHT_DUP) : 0;
	}

	console_printf("kernel: no module server named %s\n", name);
	return (u64)-1;
}

static struct module_server_start *current_module_start(void)
{
	const char *name = task_name(task_current());

	for (u32 i = 0; i < module_start_count; i++) {
		if (str_eq(module_starts[i].server->name, name)) {
			return &module_starts[i];
		}
	}

	return 0;
}

u64 server_boot_module_size(void)
{
	const struct module_server_start *start = current_module_start();

	return start != 0 && start->data_end > start->data_start ?
	       start->data_end - start->data_start : 0;
}

int server_boot_module_read(u64 offset, void *buffer, u64 len)
{
	const struct module_server_start *start = current_module_start();
	const u64 size = server_boot_module_size();

	if (start == 0 || start->data_start == 0 || buffer == 0 ||
	    offset > size || len > size - offset) {
		return -1;
	}

	mem_copy((u8 *)buffer, (const u8 *)(start->data_start + offset), len);
	return 0;
}

static void record_boot_module(const struct multiboot2_module *module, void *ctx)
{
	(void)ctx;

	const struct server *server = find_boot_server(module->cmdline);
	if (server == 0) {
		if (str_eq(module->cmdline, "disk0")) {
			disk0_module.name = module->cmdline;
			disk0_module.start = module->start;
			disk0_module.end = module->end;
			console_printf("kernel: recorded data module disk0 image=%p-%p size=%u\n",
				       (const void *)module->start,
				       (const void *)module->end,
				       (u32)(module->end - module->start));
			for (u32 i = 0; i < module_start_count; i++) {
				if (str_eq(module_starts[i].server->name, "block")) {
					module_starts[i].data_start = module->start;
					module_starts[i].data_end = module->end;
				}
			}
			return;
		}

		console_printf("kernel: ignoring unknown module %s\n", module->cmdline);
		return;
	}

	if (module_start_count >=
	    sizeof(module_starts) / sizeof(module_starts[0])) {
		console_printf("kernel: module server table full\n");
		return;
	}

	struct module_server_start *start = &module_starts[module_start_count++];
	start->server = server;
	start->image_start = module->start;
	start->image_end = module->end;
	start->data_start = 0;
	start->data_end = 0;
	if (str_eq(server->name, "block")) {
		start->data_start = disk0_module.start;
		start->data_end = disk0_module.end;
	}
	start->space = 0;
	start->stack = 0;
	start->launched = 0;

	console_printf("kernel: recorded module server %s image=%p-%p\n",
		       server->name, (const void *)module->start,
		       (const void *)module->end);
}

void server_start_boot_modules(u64 multiboot_info)
{
	struct ipc_port *console_port = ipc_port_create("console");
	name_service_register("console", NAME_SERVICE_IPC_PORT,
			      (u64)console_port);
	multiboot2_for_each_module(multiboot_info, record_boot_module, 0);
	server_launch_module("vm");
	sched_run();
	server_launch_module("names");
	server_launch_module("init");
}
