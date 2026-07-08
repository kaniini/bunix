#ifndef BUNIXOS_ARCH_USER_H
#define BUNIXOS_ARCH_USER_H

#include "types.h"

/*
 * Native Bunix syscall ABI for rv64:
 * - a7 carries the signed syscall number.  Bunix-native syscall IDs remain
 *   negative; non-negative numbers are reserved for the Linux personality.
 * - a0-a3 carry the first four 64-bit arguments.
 * - a0 carries the return value after ecall.
 * - ecall advances sepc by 4 before returning to userspace.
 */
struct arch_syscall_frame {
	u64 number;
	u64 arg0;
	u64 arg1;
	u64 arg2;
	u64 arg3;
	u64 user_pc;
	u64 user_status;
	u64 user_sp;
	u64 ra;
	u64 gp;
	u64 tp;
	u64 s[12];
	u64 a[8];
};

void arch_user_init(void);
void arch_user_set_strace_mode(const char *mode);
void arch_user_init_cpu(u32 cpu_id);
void arch_user_set_kernel_stack(u64 stack);
void arch_user_set_fs_base(u64 fs_base);
void arch_user_enter(u64 entry, u64 stack) __attribute__((noreturn));
void arch_user_resume(const struct arch_syscall_frame *frame)
	__attribute__((noreturn));
u64 arch_syscall_dispatch(struct arch_syscall_frame *frame);

#endif
