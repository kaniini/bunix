#include <arch/interrupts.h>
#include <arch/sbi.h>

enum {
	RISCV64_SCAUSE_INTERRUPT = 1ULL << 63,
	RISCV64_SCAUSE_SUPERVISOR_TIMER = RISCV64_SCAUSE_INTERRUPT | 5,
	RISCV64_SIE_STIE = 1 << 5,
	RISCV64_SSTATUS_SIE = 1 << 1,
	RISCV64_TIMER_HZ = 10000000,
};

extern void riscv64_trap_entry(void);

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
	__asm__ volatile ("csrw stvec, %0" : : "r"(riscv64_trap_entry) :
			  "memory");
	__asm__ volatile ("csrs sie, %0" : : "r"(RISCV64_SIE_STIE) :
			  "memory");
}

void arch_interrupts_load(void)
{
}

void arch_interrupts_enable(void)
{
	__asm__ volatile ("csrs sstatus, %0" : : "r"(RISCV64_SSTATUS_SIE) :
			  "memory");
}

void arch_interrupts_disable(void)
{
	__asm__ volatile ("csrc sstatus, %0" : : "r"(RISCV64_SSTATUS_SIE) :
			  "memory");
}

u64 arch_timer_ticks(void)
{
	return ticks;
}

u64 arch_timer_hz(void)
{
	return RISCV64_TIMER_HZ;
}

void arch_interrupt_dispatch(struct arch_interrupt_frame *frame)
{
	if (frame != 0 && frame->scause == RISCV64_SCAUSE_SUPERVISOR_TIMER) {
		ticks++;
		(void)riscv64_sbi_call1(RISCV64_SBI_LEGACY_SET_TIMER,
					~0ULL);
	}
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
