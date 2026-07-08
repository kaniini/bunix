#include <arch/interrupts.h>
#include <arch/sbi.h>
#include <arch/user.h>

enum {
	RISCV64_SCAUSE_INTERRUPT = 1ULL << 63,
	RISCV64_SCAUSE_SUPERVISOR_TIMER = RISCV64_SCAUSE_INTERRUPT | 5,
	RISCV64_SCAUSE_USER_ECALL = 8,
	RISCV64_SCAUSE_SUPERVISOR_ECALL = 9,
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

static void riscv64_handle_ecall(struct arch_interrupt_frame *frame)
{
	struct arch_syscall_frame syscall_frame = {
		.number = frame->a7,
		.arg0 = frame->a0,
		.arg1 = frame->a1,
		.arg2 = frame->a2,
		.arg3 = frame->a3,
		.user_pc = frame->sepc,
		.user_status = frame->sstatus,
		.user_sp = frame->sp,
		.ra = frame->ra,
		.gp = frame->gp,
		.tp = frame->tp,
		.s = {
			frame->s0, frame->s1, frame->s2, frame->s3,
			frame->s4, frame->s5, frame->s6, frame->s7,
			frame->s8, frame->s9, frame->s10, frame->s11,
		},
		.a = {
			frame->a0, frame->a1, frame->a2, frame->a3,
			frame->a4, frame->a5, frame->a6, frame->a7,
		},
	};

	frame->a0 = arch_syscall_dispatch(&syscall_frame);
	frame->sepc += 4;
}

void arch_interrupt_dispatch(struct arch_interrupt_frame *frame)
{
	if (frame != 0 && frame->scause == RISCV64_SCAUSE_SUPERVISOR_TIMER) {
		ticks++;
		(void)riscv64_sbi_call1(RISCV64_SBI_LEGACY_SET_TIMER,
					~0ULL);
		return;
	}
	if (frame != 0 &&
	    (frame->scause == RISCV64_SCAUSE_USER_ECALL ||
	     frame->scause == RISCV64_SCAUSE_SUPERVISOR_ECALL)) {
		riscv64_handle_ecall(frame);
	}
}

int riscv64_syscall_entry_self_test(void)
{
	struct arch_interrupt_frame frame = {
		.scause = RISCV64_SCAUSE_USER_ECALL,
		.sepc = 0x1000,
		.a0 = 0x1111,
		.a1 = 0x2222,
		.a2 = 0x3333,
		.a3 = 0x4444,
		.a7 = (u64)-999,
	};

	arch_interrupt_dispatch(&frame);
	return frame.sepc == 0x1004 && frame.a0 == (u64)-1 ? 0 : -1;
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
