#include <arch/boot.h>
#include <arch/fdt.h>
#include <arch/interrupts.h>
#include <arch/sbi.h>
#include <arch/thread.h>
#include <arch/user.h>

static struct riscv64_boot_info boot_info;
static struct arch_thread_context boot_context;
static struct arch_thread_context worker_context;
static volatile u32 worker_switched;
static u8 worker_stack[4096] __attribute__((aligned(16)));
static u8 user_probe_stack[4096] __attribute__((aligned(16)));
static u8 user_probe_kernel_stack[4096] __attribute__((aligned(16)));

static const char riscv64_bootpkg_magic[] = "BUNIX-RV64-BOOTPKG\n";

const struct riscv64_boot_info *riscv64_boot_info(void)
{
	return &boot_info;
}

static void sbi_putchar(char ch)
{
	riscv64_sbi_putchar(ch);
}

static void early_puts(const char *text)
{
	while (*text != '\0') {
		if (*text == '\n') {
			sbi_putchar('\r');
		}
		sbi_putchar(*text++);
	}
}

static void worker_thread_main(void)
{
	worker_switched = 1;
	arch_thread_switch(&worker_context, &boot_context);
	for (;;) {
		__asm__ volatile ("wfi");
	}
}

static int context_switch_self_test(void)
{
	worker_switched = 0;
	arch_thread_context_init_current(&boot_context);
	arch_thread_context_init(&worker_context,
				 worker_stack + sizeof(worker_stack),
				 worker_thread_main);
	arch_thread_switch(&boot_context, &worker_context);
	return worker_switched == 1 ? 0 : -1;
}

static int bootpkg_magic_ok(u64 start, u64 end)
{
	const char *image = (const char *)start;
	const u64 magic_len = sizeof(riscv64_bootpkg_magic) - 1;

	if (start == 0 || end <= start || end - start < magic_len) {
		return 0;
	}
	for (u64 i = 0; i < magic_len; i++) {
		if (image[i] != riscv64_bootpkg_magic[i]) {
			return 0;
		}
	}
	return 1;
}

void riscv64_early_main(u64 hart_id, u64 fdt)
{
	struct riscv64_fdt_memory_range memory;
	struct riscv64_fdt_initrd initrd;

	boot_info.hart_id = hart_id;
	boot_info.fdt = fdt;
	boot_info.phys_base = RISCV64_PHYS_MEM_BASE;
	boot_info.phys_size = 0;
	boot_info.kernel_load_base = RISCV64_KERNEL_LOAD_BASE;
	boot_info.direct_map_base = RISCV64_DIRECT_MAP_BASE;
	boot_info.direct_map_size = RISCV64_DIRECT_MAP_SIZE;
	boot_info.user_base = RISCV64_USER_BASE;
	boot_info.user_limit = RISCV64_USER_LIMIT;
	boot_info.initrd_start = 0;
	boot_info.initrd_end = 0;
	if (riscv64_fdt_scan_memory((const void *)fdt, &memory, 1) == 1) {
		boot_info.phys_base = memory.base;
		boot_info.phys_size = memory.size;
	}
	if (riscv64_fdt_scan_initrd((const void *)fdt, &initrd) == 0) {
		boot_info.initrd_start = initrd.start;
		boot_info.initrd_end = initrd.end;
	}

	early_puts("bunixos: riscv64 early bootstrap\n");
	arch_interrupts_init();
	riscv64_timer_set_relative(arch_timer_hz() / 100);
	arch_interrupts_enable();
	while (arch_timer_ticks() == 0) {
		__asm__ volatile ("wfi");
	}
	arch_interrupts_disable();
	early_puts("timer: riscv64 tick\n");
	if (context_switch_self_test() == 0) {
		early_puts("thread: riscv64 switch\n");
	}
	if (riscv64_syscall_entry_self_test() == 0) {
		early_puts("syscall: riscv64 ecall\n");
	}
	if (riscv64_user_mode_self_test((u64)riscv64_user_ecall_probe,
					(u64)(user_probe_stack +
					      sizeof(user_probe_stack)),
					(u64)(user_probe_kernel_stack +
					      sizeof(user_probe_kernel_stack))) == 0) {
		early_puts("user: riscv64 mode\n");
	}
	if (bootpkg_magic_ok(boot_info.initrd_start, boot_info.initrd_end)) {
		early_puts("bootpkg: riscv64 initrd\n");
	}
	early_puts("machine: poweroff\n");
	(void)riscv64_sbi_call1(RISCV64_SBI_LEGACY_SHUTDOWN, 0);

	for (;;) {
		__asm__ volatile ("wfi");
	}
}
