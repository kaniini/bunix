#ifndef BUNIXOS_ARCH_THREAD_H
#define BUNIXOS_ARCH_THREAD_H

#include "types.h"

struct arch_thread_context {
	u64 rsp;
	u8 xstate[4096] __attribute__((aligned(64)));
};

extern u32 arch_thread_use_xsave;
extern u32 arch_thread_xstate_mask_low;
extern u32 arch_thread_xstate_mask_high;

void arch_thread_context_init(struct arch_thread_context *ctx, void *stack_top,
			      void (*entry)(void));
void arch_thread_context_init_current(struct arch_thread_context *ctx);
void arch_thread_fpu_init_cpu(u64 xstate_mask, int use_xsave);
void arch_thread_fpu_reset_current(void);
void arch_thread_switch(struct arch_thread_context *prev,
			struct arch_thread_context *next);

#endif
