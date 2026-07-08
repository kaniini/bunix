#include "buffer.h"
#include "cmdline.h"
#include "console.h"
#include "ipc.h"
#include "multiboot2.h"
#include "name.h"
#include "pmm.h"
#include "sched.h"
#include "server.h"
#include "slab.h"
#include "types.h"
#include "vm.h"
#include <arch/interrupts.h>
#include <arch/power.h>
#include <arch/smp.h>
#include <arch/user.h>
#include "../servers/vm/vm_server.h"

static const char *kernel_cmdline;

static int starts_with(const char *text, const char *prefix)
{
	while (*prefix != '\0') {
		if (*text++ != *prefix++) {
			return 0;
		}
	}

	return 1;
}

static u64 str_len(const char *s)
{
	u64 len = 0;

	while (s[len] != '\0') {
		len++;
	}
	return len;
}

int kernel_cmdline_has(const char *token)
{
	const char *cmdline = kernel_cmdline;
	const u64 token_len = token == 0 ? 0 : str_len(token);

	if (cmdline == 0 || token_len == 0) {
		return 0;
	}

	while (*cmdline != '\0') {
		while (*cmdline == ' ') {
			cmdline++;
		}

		u64 len = 0;
		while (cmdline[len] != '\0' && cmdline[len] != ' ') {
			len++;
		}
		if (len == token_len) {
			u64 i;

			for (i = 0; i < len; i++) {
				if (cmdline[i] != token[i]) {
					break;
				}
			}
			if (i == len) {
				return 1;
			}
		}
		cmdline += len;
	}

	return 0;
}

static void configure_boot_options(u64 multiboot_info)
{
	const char *cmdline = multiboot2_cmdline(multiboot_info);
	int have_log = 0;

	kernel_cmdline = cmdline;
	while (*cmdline != '\0') {
		while (*cmdline == ' ') {
			cmdline++;
		}

		if (starts_with(cmdline, "log=")) {
			console_set_verbosity(cmdline + 4);
			have_log = 1;
		}
		if (starts_with(cmdline, "strace=")) {
			arch_user_set_strace_mode(cmdline + 7);
		}

		while (*cmdline != '\0' && *cmdline != ' ') {
			cmdline++;
		}
	}

	if (!have_log) {
		console_set_verbosity("info");
	}
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

	configure_boot_options(multiboot_info);
	multiboot2_dump(multiboot_info);
	pmm_init(multiboot_info);
	vm_init();
	slab_init();
	buffer_init();
	vm_self_test();
	arch_power_init(multiboot_info);
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
