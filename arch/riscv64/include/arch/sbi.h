#ifndef BUNIXOS_ARCH_RISCV64_SBI_H
#define BUNIXOS_ARCH_RISCV64_SBI_H

#include "types.h"

enum {
	RISCV64_SBI_EXT_BASE = 0x10,
	RISCV64_SBI_EXT_SYSTEM_RESET = 0x53525354,
	RISCV64_SBI_BASE_PROBE_EXTENSION = 3,
	RISCV64_SBI_SYSTEM_RESET = 0,
	RISCV64_SBI_LEGACY_CONSOLE_PUTCHAR = 1,
	RISCV64_SBI_LEGACY_CONSOLE_GETCHAR = 2,
	RISCV64_SBI_LEGACY_SET_TIMER = 0,
	RISCV64_SBI_LEGACY_SHUTDOWN = 8,
	RISCV64_SBI_RESET_SHUTDOWN = 0,
	RISCV64_SBI_RESET_COLD_REBOOT = 1,
	RISCV64_SBI_RESET_WARM_REBOOT = 2,
	RISCV64_SBI_RESET_REASON_NONE = 0,
};

struct riscv64_sbi_ret {
	long error;
	long value;
};

static inline struct riscv64_sbi_ret riscv64_sbi_ecall(u64 extension,
						       u64 function,
						       u64 arg0,
						       u64 arg1)
{
	register u64 a0 __asm__("a0") = arg0;
	register u64 a1 __asm__("a1") = arg1;
	register u64 a6 __asm__("a6") = function;
	register u64 a7 __asm__("a7") = extension;

	__asm__ volatile ("ecall"
			  : "+r"(a0), "+r"(a1)
			  : "r"(a6), "r"(a7)
			  : "memory");
	return (struct riscv64_sbi_ret){ (long)a0, (long)a1 };
}

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

static inline int riscv64_sbi_probe_extension(u64 extension)
{
	struct riscv64_sbi_ret ret =
		riscv64_sbi_ecall(RISCV64_SBI_EXT_BASE,
				  RISCV64_SBI_BASE_PROBE_EXTENSION,
				  extension, 0);

	return ret.error == 0 && ret.value != 0;
}

static inline void riscv64_sbi_putchar(char ch)
{
	(void)riscv64_sbi_call1(RISCV64_SBI_LEGACY_CONSOLE_PUTCHAR, (u64)ch);
}

#endif
