typedef unsigned long u64;

enum {
	SBI_LEGACY_CONSOLE_PUTCHAR = 1,
	SBI_LEGACY_SHUTDOWN = 8,
};

static long sbi_call(u64 extension, u64 arg0)
{
	register u64 a0 __asm__("a0") = arg0;
	register u64 a7 __asm__("a7") = extension;

	__asm__ volatile ("ecall" : "+r"(a0) : "r"(a7) : "memory");
	return (long)a0;
}

static void sbi_putchar(char ch)
{
	(void)sbi_call(SBI_LEGACY_CONSOLE_PUTCHAR, (u64)ch);
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
	(void)sbi_call(SBI_LEGACY_SHUTDOWN, 0);

	for (;;) {
		__asm__ volatile ("wfi");
	}
}
