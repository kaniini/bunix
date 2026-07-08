#include <arch/power.h>
#include <arch/sbi.h>

void arch_power_init(u64 boot_info)
{
	(void)boot_info;
}

void arch_reboot(void)
{
	(void)riscv64_sbi_call1(RISCV64_SBI_LEGACY_SHUTDOWN, 0);
	for (;;) {
		__asm__ volatile ("wfi");
	}
}

void arch_poweroff(void)
{
	for (const char *p = "machine: poweroff\n"; *p != '\0'; p++) {
		if (*p == '\n') {
			riscv64_sbi_putchar('\r');
		}
		riscv64_sbi_putchar(*p);
	}
	(void)riscv64_sbi_call1(RISCV64_SBI_LEGACY_SHUTDOWN, 0);
	for (;;) {
		__asm__ volatile ("wfi");
	}
}
