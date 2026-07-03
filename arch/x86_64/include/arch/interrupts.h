#ifndef BUNIXOS_ARCH_INTERRUPTS_H
#define BUNIXOS_ARCH_INTERRUPTS_H

#include "types.h"

struct arch_interrupt_frame {
	u64 vector;
	u64 error_code;
	u64 rip;
	u64 cs;
	u64 rflags;
	u64 rsp;
	u64 ss;
};

void arch_interrupts_init(void);
void arch_interrupts_load(void);
u64 arch_timer_ticks(void);
u64 arch_timer_hz(void);
void arch_interrupt_dispatch(struct arch_interrupt_frame *frame);

#endif
