#include "console.h"
#include "elf.h"
#include "ipc.h"
#include "multiboot2.h"
#include "name.h"
#include "sched.h"
#include "server.h"
#include "types.h"
#include "vm.h"
#include <arch/smp.h>
#include <arch/user.h>
#include "../servers/vm/vm_server.h"

static const struct server boot_servers[] = {
	{ "init", 0 },
	{ "hello", 0 },
	{ "ping", 0 },
	{ "vm", vm_server_start },
};

struct module_server_start {
	const struct server *server;
	u64 image_start;
	u64 image_end;
	struct vm_space *space;
	u64 stack;
	u32 launched;
};

static struct module_server_start module_starts[16];
static u32 module_start_count;

static int str_eq(const char *left, const char *right);
static const struct server *find_boot_server(const char *name);

static u32 module_cpu_id(const struct server *server)
{
	if (arch_smp_cpu_count() < 2) {
		return 0;
	}

	if (server->start != 0 || str_eq(server->name, "hello") ||
	    str_eq(server->name, "ping")) {
		return 1;
	}

	return 0;
}

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

int server_launch_module(const char *name)
{
	return server_launch_module_with_caps(name, 0, 0, 0) == (u64)-1 ? -1 : 0;
}

static void grant_bootstrap_caps(struct task *task, const char *server_name)
{
	if (!str_eq(server_name, "init")) {
		return;
	}

	task_grant_port(task, ipc_port_find("console"));
	task_grant_port(task, ipc_port_find("vm"));
}

u64 server_launch_module_with_caps(const char *name, struct task *parent,
				   const u64 *handles, u64 handle_count)
{
	enum {
		USER_STACK_TOP = 0x800000,
		USER_STACK_PAGES = 4,
		MAX_INHERITED_HANDLES = 8,
	};
	struct ipc_port *inherited_ports[MAX_INHERITED_HANDLES];

	if (handle_count > MAX_INHERITED_HANDLES) {
		console_printf("kernel: too many inherited caps for %s count=%u\n",
			       name, (u32)handle_count);
		return (u64)-1;
	}

	for (u64 handle = 0; handle < handle_count; handle++) {
		inherited_ports[handle] = task_port_from_handle(parent,
								handles[handle]);
		if (inherited_ports[handle] == 0) {
			console_printf("kernel: invalid inherited cap %u for %s\n",
				       (u32)handles[handle], name);
			return (u64)-1;
		}
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

		start->launched = 1;
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

		struct ipc_port *service_port = ipc_port_create(server_name);
		const u64 child_self = task_grant_port(task, service_port);
		if (child_self == 0) {
			return (u64)-1;
		}

		for (u64 handle = 0; handle < handle_count; handle++) {
			if (task_grant_port(task, inherited_ports[handle]) == 0) {
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

		const u32 cpu_id = module_cpu_id(start->server);
		if (thread_create_on_cpu(task, server_name, module_server_thread,
					 start, cpu_id) == 0) {
			console_printf("kernel: failed to create server thread %s\n",
				       server_name);
			return (u64)-1;
		}

		if (!str_eq(server_name, "vm")) {
			name_service_register(server_name, NAME_SERVICE_TASK,
					      task_id(task));
		}

		return parent != 0 ? task_grant_port(parent, service_port) : 0;
	}

	console_printf("kernel: no module server named %s\n", name);
	return (u64)-1;
}

static void record_boot_module(const struct multiboot2_module *module, void *ctx)
{
	(void)ctx;

	const struct server *server = find_boot_server(module->cmdline);
	if (server == 0) {
		console_printf("kernel: ignoring unknown module %s\n",
			       module->cmdline);
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
	server_launch_module("init");
}
