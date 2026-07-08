#include <arch/power.h>
#include <arch/sbi.h>

static void power_puts(const char *text)
{
	for (const char *p = text; *p != '\0'; p++) {
		if (*p == '\n') {
			riscv64_sbi_putchar('\r');
		}
		riscv64_sbi_putchar(*p);
	}
}

void arch_power_init(u64 boot_info)
{
	(void)boot_info;
}

static void system_reset_or_legacy(u64 type)
{
	if (riscv64_sbi_probe_extension(RISCV64_SBI_EXT_SYSTEM_RESET)) {
		(void)riscv64_sbi_ecall(RISCV64_SBI_EXT_SYSTEM_RESET,
					RISCV64_SBI_SYSTEM_RESET, type,
					RISCV64_SBI_RESET_REASON_NONE);
	}
	power_puts("sbi: legacy shutdown fallback\n");
	(void)riscv64_sbi_call1(RISCV64_SBI_LEGACY_SHUTDOWN, 0);
}

void arch_reboot(void)
{
	power_puts("machine: reboot\n");
	power_puts("sbi: system reset reboot\n");
	system_reset_or_legacy(RISCV64_SBI_RESET_COLD_REBOOT);
	for (;;) {
		__asm__ volatile ("wfi");
	}
}

void arch_poweroff(void)
{
	power_puts("machine: poweroff\n");
	power_puts("sbi: system reset poweroff\n");
	system_reset_or_legacy(RISCV64_SBI_RESET_SHUTDOWN);
	for (;;) {
		__asm__ volatile ("wfi");
	}
}
