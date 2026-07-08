#ifndef BUNIXOS_ARCH_INTERRUPTS_H
#define BUNIXOS_ARCH_INTERRUPTS_H

#include "types.h"

struct ipc_port;

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
u64 arch_interrupts_save(void);
void arch_interrupts_restore(u64 flags);
void arch_interrupts_enable(void);
void arch_interrupts_disable(void);
void arch_cpu_wait_for_interrupt(void);
u64 arch_timer_ticks(void);
u64 arch_timer_hz(void);
void arch_interrupt_dispatch(struct arch_interrupt_frame *frame);
int arch_irq_bind(u32 gsi, u32 resource_flags, struct ipc_port *port);
int arch_irq_mask(u32 gsi, int masked);
int arch_irq_ack(u32 gsi);

#endif
