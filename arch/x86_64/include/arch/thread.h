#ifndef BUNIXOS_ARCH_THREAD_H
#define BUNIXOS_ARCH_THREAD_H

#include "types.h"

struct arch_thread_context {
	u64 rsp;
};

void arch_thread_context_init(struct arch_thread_context *ctx, void *stack_top,
			      void (*entry)(void));
void arch_thread_switch(struct arch_thread_context *prev,
			struct arch_thread_context *next);

#endif
