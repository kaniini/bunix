#ifndef BUNIXOS_ARCH_INTERRUPTS_H
#define BUNIXOS_ARCH_INTERRUPTS_H

#include "types.h"

struct ipc_port;

struct arch_interrupt_frame {
	u64 scause;
	u64 stval;
	u64 sepc;
	u64 sstatus;
	u64 sp;
};

void arch_interrupts_init(void);
void arch_interrupts_load(void);
void arch_interrupts_enable(void);
void arch_interrupts_disable(void);
u64 arch_timer_ticks(void);
u64 arch_timer_hz(void);
void arch_interrupt_dispatch(struct arch_interrupt_frame *frame);
int arch_irq_bind(u32 gsi, u32 resource_flags, struct ipc_port *port);
int arch_irq_mask(u32 gsi, int masked);
int arch_irq_ack(u32 gsi);

#endif
