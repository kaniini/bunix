#include "console.h"
#include "ipc.h"
#include "multiboot2.h"
#include "sched.h"
#include "server.h"
#include "types.h"
#include "vm.h"
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
	ipc_init();
	vm_server_init();
	sched_init();
	server_start_boot_modules(multiboot_info);
	sched_run();

	console_printf("kernel: idle\n");
	for (;;) {
		__asm__ volatile ("hlt");
	}
}
