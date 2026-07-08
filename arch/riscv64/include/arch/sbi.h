#ifndef BUNIXOS_ARCH_RISCV64_SBI_H
#define BUNIXOS_ARCH_RISCV64_SBI_H

#include "types.h"

enum {
	RISCV64_SBI_LEGACY_CONSOLE_PUTCHAR = 1,
	RISCV64_SBI_LEGACY_CONSOLE_GETCHAR = 2,
	RISCV64_SBI_LEGACY_SET_TIMER = 0,
	RISCV64_SBI_LEGACY_SHUTDOWN = 8,
};

static inline long riscv64_sbi_call1(u64 extension, u64 arg0)
{
	register u64 a0 __asm__("a0") = arg0;
	register u64 a7 __asm__("a7") = extension;

	__asm__ volatile ("ecall" : "+r"(a0) : "r"(a7) : "memory");
	return (long)a0;
}

static inline long riscv64_sbi_call0(u64 extension)
{
	register u64 a0 __asm__("a0");
	register u64 a7 __asm__("a7") = extension;

	__asm__ volatile ("ecall" : "=r"(a0) : "r"(a7) : "memory");
	return (long)a0;
}

static inline void riscv64_sbi_putchar(char ch)
{
	(void)riscv64_sbi_call1(RISCV64_SBI_LEGACY_CONSOLE_PUTCHAR, (u64)ch);
}

#endif
