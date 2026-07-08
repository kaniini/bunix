#ifndef BUNIXOS_ARCH_THREAD_H
#define BUNIXOS_ARCH_THREAD_H

#include "types.h"

struct arch_thread_context {
	u64 sp;
	u64 ra;
	u64 s[12];
};

void arch_thread_context_init(struct arch_thread_context *ctx, void *stack_top,
			      void (*entry)(void));
void arch_thread_context_init_current(struct arch_thread_context *ctx);
void arch_thread_fpu_init_cpu(u64 state_mask, int enabled);
void arch_thread_fpu_reset_current(void);
void arch_thread_switch(struct arch_thread_context *prev,
			struct arch_thread_context *next);

#endif
