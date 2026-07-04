#ifndef BUNIXOS_ARCH_USER_H
#define BUNIXOS_ARCH_USER_H

#include "types.h"

struct arch_syscall_frame {
	u64 number;
	u64 arg0;
	u64 arg1;
	u64 arg2;
	u64 arg3;
	u64 user_rip;
	u64 user_rflags;
	u64 user_rsp;
	u64 rbx;
	u64 rbp;
	u64 r12;
	u64 r13;
	u64 r14;
	u64 r15;
	u64 r8;
	u64 r9;
};

void arch_user_init(void);
void arch_user_init_cpu(u32 cpu_id);
void arch_user_set_kernel_stack(u64 stack);
void arch_user_set_fs_base(u64 fs_base);
void arch_user_enter(u64 entry, u64 stack) __attribute__((noreturn));
void arch_user_resume(const struct arch_syscall_frame *frame)
	__attribute__((noreturn));
u64 arch_syscall_dispatch(struct arch_syscall_frame *frame);

#endif
