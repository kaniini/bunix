#include <arch/thread.h>

u32 arch_thread_use_xsave;
u32 arch_thread_xstate_mask_low = 0x3;
u32 arch_thread_xstate_mask_high;

static u8 initial_xstate[4096] __attribute__((aligned(64)));

static void mem_copy(void *dst, const void *src, u64 len)
{
	u8 *out = (u8 *)dst;
	const u8 *in = (const u8 *)src;

	for (u64 i = 0; i < len; i++) {
		out[i] = in[i];
	}
}

void arch_thread_context_init(struct arch_thread_context *ctx, void *stack_top,
			      void (*entry)(void))
{
	u64 *stack = (u64 *)stack_top;

	*--stack = 0; /* Alignment slot for the synthetic call frame. */
	*--stack = (u64)entry;
	*--stack = 0; /* rbp */
	*--stack = 0; /* rbx */
	*--stack = 0; /* r12 */
	*--stack = 0; /* r13 */
	*--stack = 0; /* r14 */
	*--stack = 0; /* r15 */

	ctx->rsp = (u64)stack;
	mem_copy(ctx->xstate, initial_xstate, sizeof(ctx->xstate));
}

void arch_thread_context_init_current(struct arch_thread_context *ctx)
{
	if (arch_thread_use_xsave) {
		const u32 low = arch_thread_xstate_mask_low;
		const u32 high = arch_thread_xstate_mask_high;

		__asm__ volatile ("xsave64 %0"
				  : "=m"(ctx->xstate)
				  : "a"(low), "d"(high)
				  : "memory");
	} else {
		__asm__ volatile ("fxsave64 %0"
				  : "=m"(ctx->xstate)
				  :
				  : "memory");
	}
}

void arch_thread_fpu_init_cpu(u64 xstate_mask, int use_xsave)
{
	const u32 mxcsr = 0x1f80;

	arch_thread_use_xsave = use_xsave ? 1 : 0;
	arch_thread_xstate_mask_low = (u32)xstate_mask;
	arch_thread_xstate_mask_high = (u32)(xstate_mask >> 32);

	__asm__ volatile ("fninit");
	__asm__ volatile ("ldmxcsr %0" : : "m"(mxcsr));
	if (arch_thread_use_xsave) {
		const u32 low = arch_thread_xstate_mask_low;
		const u32 high = arch_thread_xstate_mask_high;

		__asm__ volatile ("xsave64 %0"
				  : "=m"(initial_xstate)
				  : "a"(low), "d"(high)
				  : "memory");
	} else {
		__asm__ volatile ("fxsave64 %0"
				  : "=m"(initial_xstate)
				  :
				  : "memory");
	}
}

void arch_thread_fpu_reset_current(void)
{
	const u32 mxcsr = 0x1f80;

	if (arch_thread_use_xsave) {
		const u32 low = arch_thread_xstate_mask_low;
		const u32 high = arch_thread_xstate_mask_high;

		__asm__ volatile ("xrstor64 %0"
				  :
				  : "m"(initial_xstate), "a"(low), "d"(high)
				  : "memory");
	} else {
		__asm__ volatile ("fxrstor64 %0"
				  :
				  : "m"(initial_xstate)
				  : "memory");
	}
	__asm__ volatile ("fninit");
	__asm__ volatile ("ldmxcsr %0" : : "m"(mxcsr));
}
