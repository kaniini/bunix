#ifndef BUNIXOS_ARCH_USER_H
#define BUNIXOS_ARCH_USER_H

#include "types.h"

void arch_user_init(void);
void arch_user_set_kernel_stack(u64 stack);
void arch_user_enter(u64 entry, u64 stack) __attribute__((noreturn));
u64 arch_syscall_dispatch(u64 number, u64 arg0, u64 arg1, u64 arg2);

#endif
