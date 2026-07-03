#include "console.h"
#include "elf.h"
#include "multiboot2.h"
#include "sched.h"
#include "server.h"
#include "types.h"
#include <arch/user.h>
#include "../servers/vm/vm_server.h"

static const struct server boot_servers[] = {
	{ "hello", 0 },
	{ "ping", ping_server_start },
	{ "vm", vm_server_start },
};

struct module_server_start {
	const struct server *server;
	u64 image_start;
	u64 image_end;
};

static struct module_server_start module_starts[16];
static u32 module_start_count;

static int str_eq(const char *left, const char *right);

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

	if (str_eq(start->server->name, "hello")) {
		u64 entry = 0;
		if (elf_load_user_image(start->image_start, start->image_end, &entry) == 0) {
			arch_user_enter(entry, 0x600000);
		}
		return;
	}

	if (start->server->start != 0) {
		start->server->start();
	}
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

static void start_boot_module(const struct multiboot2_module *module, void *ctx)
{
	(void)ctx;

	const u32 count = sizeof(boot_servers) / sizeof(boot_servers[0]);

	for (u32 i = 0; i < count; i++) {
		if (!str_eq(module->cmdline, boot_servers[i].name)) {
			continue;
		}

		if (module_start_count >=
		    sizeof(module_starts) / sizeof(module_starts[0])) {
			console_printf("kernel: module server table full\n");
			return;
		}

		struct module_server_start *start =
			&module_starts[module_start_count++];
		start->server = &boot_servers[i];
		start->image_start = module->start;
		start->image_end = module->end;

		struct vm_space *space;

		if (str_eq(boot_servers[i].name, "vm")) {
			space = vm_server_bootstrap_space(boot_servers[i].name);
		} else {
			space = vm_server_rpc_create_space(boot_servers[i].name);
		}
		if (space == 0) {
			return;
		}

		struct task *task = task_create(boot_servers[i].name, space);
		if (task == 0) {
			return;
		}

		if (thread_create(task, boot_servers[i].name,
				  module_server_thread, start) == 0) {
			console_printf("kernel: failed to create server thread %s\n",
				       boot_servers[i].name);
		}
		return;
	}

	console_printf("kernel: ignoring unknown module %s\n", module->cmdline);
}

void server_start_boot_modules(u64 multiboot_info)
{
	multiboot2_for_each_module(multiboot_info, start_boot_module, 0);
}
