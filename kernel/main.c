#include "buffer.h"
#include "console.h"
#include "ipc.h"
#include "multiboot2.h"
#include "name.h"
#include "sched.h"
#include "server.h"
#include "slab.h"
#include "types.h"
#include "vm.h"
#include <arch/interrupts.h>
#include <arch/smp.h>
#include <arch/user.h>
#include "../servers/vm/vm_server.h"

static int starts_with(const char *text, const char *prefix)
{
	while (*prefix != '\0') {
		if (*text++ != *prefix++) {
			return 0;
		}
	}

	return 1;
}

static void configure_console(u64 multiboot_info)
{
	const char *cmdline = multiboot2_cmdline(multiboot_info);

	while (*cmdline != '\0') {
		while (*cmdline == ' ') {
			cmdline++;
		}

		if (starts_with(cmdline, "log=")) {
			console_set_verbosity(cmdline + 4);
			return;
		}

		while (*cmdline != '\0' && *cmdline != ' ') {
			cmdline++;
		}
	}

	console_set_verbosity("info");
}

void kernel_main(u32 magic, u64 multiboot_info)
{
	console_init();

	console_printf("bunixos: x86_64 microkernel bootstrap\n");
	console_printf("bunixos: multiboot2 magic=0x%x info=%p\n",
		       magic, (const void *)multiboot_info);

	if (magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
		console_printf("bunixos: invalid boot protocol\n");
		for (;;) {
			__asm__ volatile ("hlt");
		}
	}

	configure_console(multiboot_info);
	multiboot2_dump(multiboot_info);
	vm_init(multiboot_info);
	slab_init();
	buffer_init();
	vm_self_test();
	arch_interrupts_init();
	arch_user_init();
	arch_smp_init(multiboot_info);
	ipc_init();
	name_service_init();
	vm_server_init();
	sched_init();
	arch_smp_release_aps();
	server_start_boot_modules(multiboot_info);
	sched_enable_preemption();

	console_printf("kernel: idle\n");
	sched_idle_loop();
}
