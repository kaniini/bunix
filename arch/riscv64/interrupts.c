#include <arch/interrupts.h>
#include <arch/sbi.h>

static u64 ticks;

static u64 csr_read_time(void)
{
	u64 value;

	__asm__ volatile ("rdtime %0" : "=r"(value));
	return value;
}

void arch_interrupts_init(void)
{
	ticks = 0;
}

void arch_interrupts_load(void)
{
}

void arch_interrupts_enable(void)
{
	__asm__ volatile ("csrsi sstatus, 2" : : : "memory");
}

void arch_interrupts_disable(void)
{
	__asm__ volatile ("csrci sstatus, 2" : : : "memory");
}

u64 arch_timer_ticks(void)
{
	return ticks;
}

u64 arch_timer_hz(void)
{
	return 10000000;
}

void arch_interrupt_dispatch(struct arch_interrupt_frame *frame)
{
	(void)frame;
	ticks++;
}

int arch_irq_bind(u32 gsi, u32 resource_flags, struct ipc_port *port)
{
	(void)gsi;
	(void)resource_flags;
	(void)port;
	return -1;
}

int arch_irq_mask(u32 gsi, int masked)
{
	(void)gsi;
	(void)masked;
	return -1;
}

int arch_irq_ack(u32 gsi)
{
	(void)gsi;
	return -1;
}

void riscv64_timer_set_relative(u64 cycles)
{
	(void)riscv64_sbi_call1(RISCV64_SBI_LEGACY_SET_TIMER,
				csr_read_time() + cycles);
}
