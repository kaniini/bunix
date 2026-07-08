#include "multiboot2.h"
#include "server.h"

static void record_multiboot_module(const struct multiboot2_module *module,
				    void *ctx)
{
	(void)ctx;

	server_record_boot_module(module->cmdline, module->start, module->end);
}

void server_start_boot_modules(u64 multiboot_info)
{
	server_boot_modules_init();
	multiboot2_for_each_module(multiboot_info, record_multiboot_module, 0);
	server_start_initial_boot_modules();
}
