#include "buffer.h"
#include "boot_timing.h"
#include "cmdline.h"
#include "console.h"
#include "ipc.h"
#include "multiboot2.h"
#include "name.h"
#include "pmm.h"
#include "runtime.h"
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

void kernel_main(u32 magic, u64 multiboot_info)
{
	console_init();
	boot_timing_init();

	console_printf("bunixos: x86_64 microkernel bootstrap\n");
	console_printf("bunixos: multiboot2 magic=0x%x info=%p\n",
		       magic, (const void *)multiboot_info);

	if (magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
		console_printf("bunixos: invalid boot protocol\n");
		for (;;) {
			__asm__ volatile ("hlt");
		}
	}

	kernel_cmdline_configure(multiboot2_cmdline(multiboot_info));
	multiboot2_dump(multiboot_info);
	pmm_init(multiboot_info);
	kernel_runtime_memory_init();
	vm_self_test();
	arch_power_init(multiboot_info);
	arch_interrupts_init();
	boot_timing_record("kernel-interrupts-ready");
	arch_smp_init(multiboot_info);
	boot_timing_record("kernel-smp-ready");
	kernel_runtime_services_init();
	boot_timing_record("kernel-services-ready");
	kernel_runtime_boot_modules_init();
	kernel_runtime_scheduler_init();
	boot_timing_record("kernel-scheduler-ready");
	if (kernel_cmdline_has("handle-race-selftest") &&
	    task_handle_lifetime_selftest() != 0) {
		console_printf("sched: handle lifetime selftest failed\n");
		for (;;) {
			__asm__ volatile ("hlt");
		}
	}
	arch_smp_release_aps();
	server_start_boot_modules(multiboot_info);
	boot_timing_record("kernel-boot-modules-started");
	sched_enable_preemption();

	console_printf("kernel: idle\n");
	sched_idle_loop();
}
