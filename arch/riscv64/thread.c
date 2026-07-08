#include <arch/thread.h>

extern void arch_thread_trampoline(void);

void arch_thread_context_init(struct arch_thread_context *ctx, void *stack_top,
			      void (*entry)(void))
{
	ctx->sp = (u64)stack_top;
	ctx->ra = (u64)arch_thread_trampoline;
	ctx->s[0] = (u64)entry;
	for (u32 i = 1; i < 12; i++) {
		ctx->s[i] = 0;
	}
}

void arch_thread_context_init_current(struct arch_thread_context *ctx)
{
	ctx->sp = 0;
	ctx->ra = 0;
	for (u32 i = 0; i < 12; i++) {
		ctx->s[i] = 0;
	}
}

void arch_thread_fpu_init_cpu(u64 state_mask, int enabled)
{
	(void)state_mask;
	(void)enabled;
}

void arch_thread_fpu_reset_current(void)
{
}
