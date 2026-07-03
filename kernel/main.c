#include "console.h"
#include "ipc.h"
#include "multiboot2.h"
#include "name.h"
#include "sched.h"
#include "server.h"
#include "types.h"
#include "vm.h"
#include <arch/interrupts.h>
#include <arch/smp.h>
#include <arch/user.h>
#include "../servers/vm/vm_server.h"

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

	multiboot2_dump(multiboot_info);
	vm_init(multiboot_info);
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
