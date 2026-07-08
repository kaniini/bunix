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
	u64 ra;
	u64 gp;
	u64 tp;
	u64 t0;
	u64 t1;
	u64 t2;
	u64 s0;
	u64 s1;
	u64 a0;
	u64 a1;
	u64 a2;
	u64 a3;
	u64 a4;
	u64 a5;
	u64 a6;
	u64 a7;
	u64 s2;
	u64 s3;
	u64 s4;
	u64 s5;
	u64 s6;
	u64 s7;
	u64 s8;
	u64 s9;
	u64 s10;
	u64 s11;
	u64 t3;
	u64 t4;
	u64 t5;
	u64 t6;
};

void arch_interrupts_init(void);
void arch_interrupts_load(void);
void arch_interrupts_enable(void);
void arch_interrupts_disable(void);
u64 arch_timer_ticks(void);
u64 arch_timer_hz(void);
void arch_interrupt_dispatch(struct arch_interrupt_frame *frame);
int riscv64_syscall_entry_self_test(void);
void riscv64_timer_set_relative(u64 cycles);
int arch_irq_bind(u32 gsi, u32 resource_flags, struct ipc_port *port);
int arch_irq_mask(u32 gsi, int masked);
int arch_irq_ack(u32 gsi);

#endif
