#include <arch/thread.h>

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
}
