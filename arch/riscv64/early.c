#include <arch/sbi.h>

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

void riscv64_early_main(void)
{
	early_puts("bunixos: riscv64 early bootstrap\n");
	early_puts("machine: poweroff\n");
	(void)riscv64_sbi_call1(RISCV64_SBI_LEGACY_SHUTDOWN, 0);

	for (;;) {
		__asm__ volatile ("wfi");
	}
}
