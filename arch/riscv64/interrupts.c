#include <arch/interrupts.h>
#include <arch/sbi.h>
#include <arch/user.h>

#define RISCV64_SCAUSE_INTERRUPT (1ULL << 63)
#define RISCV64_SCAUSE_SUPERVISOR_TIMER (RISCV64_SCAUSE_INTERRUPT | 5ULL)
#define RISCV64_SCAUSE_USER_ECALL 8ULL
#define RISCV64_SCAUSE_SUPERVISOR_ECALL 9ULL
#define RISCV64_SYSCALL_EXIT ((u64)-2)
#define RISCV64_TEST_ECALL_RETURN ((u64)-998)
#define RISCV64_SIE_STIE (1ULL << 5)
#define RISCV64_SSTATUS_SIE (1ULL << 1)
#define RISCV64_SSTATUS_SPP (1ULL << 8)
#define RISCV64_TIMER_HZ 10000000ULL

extern void riscv64_trap_entry(void);
extern u64 riscv64_user_test_return_pc;
extern volatile u64 riscv64_user_test_status;

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
	__asm__ volatile ("csrw sscratch, zero" ::: "memory");
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
	struct arch_syscall_frame syscall_frame;

	if ((frame->a7 == RISCV64_TEST_ECALL_RETURN ||
	     frame->a7 == RISCV64_SYSCALL_EXIT) &&
	    riscv64_user_test_return_pc != 0) {
		riscv64_user_test_status = frame->a0;
		frame->a0 = 0;
		frame->sepc = riscv64_user_test_return_pc;
		frame->sstatus |= RISCV64_SSTATUS_SPP;
		return;
	}

	syscall_frame.number = frame->a7;
	syscall_frame.arg0 = frame->a0;
	syscall_frame.arg1 = frame->a1;
	syscall_frame.arg2 = frame->a2;
	syscall_frame.arg3 = frame->a3;
	syscall_frame.user_pc = frame->sepc;
	syscall_frame.user_status = frame->sstatus;
	syscall_frame.user_sp = frame->sp;
	syscall_frame.ra = frame->ra;
	syscall_frame.gp = frame->gp;
	syscall_frame.tp = frame->tp;
	syscall_frame.s[0] = frame->s0;
	syscall_frame.s[1] = frame->s1;
	syscall_frame.s[2] = frame->s2;
	syscall_frame.s[3] = frame->s3;
	syscall_frame.s[4] = frame->s4;
	syscall_frame.s[5] = frame->s5;
	syscall_frame.s[6] = frame->s6;
	syscall_frame.s[7] = frame->s7;
	syscall_frame.s[8] = frame->s8;
	syscall_frame.s[9] = frame->s9;
	syscall_frame.s[10] = frame->s10;
	syscall_frame.s[11] = frame->s11;
	syscall_frame.a[0] = frame->a0;
	syscall_frame.a[1] = frame->a1;
	syscall_frame.a[2] = frame->a2;
	syscall_frame.a[3] = frame->a3;
	syscall_frame.a[4] = frame->a4;
	syscall_frame.a[5] = frame->a5;
	syscall_frame.a[6] = frame->a6;
	syscall_frame.a[7] = frame->a7;

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
	struct arch_interrupt_frame frame;

	frame.scause = RISCV64_SCAUSE_USER_ECALL;
	frame.stval = 0;
	frame.sepc = 0x1000;
	frame.sstatus = 0;
	frame.sp = 0;
	frame.ra = 0;
	frame.gp = 0;
	frame.tp = 0;
	frame.t0 = 0;
	frame.t1 = 0;
	frame.t2 = 0;
	frame.s0 = 0;
	frame.s1 = 0;
	frame.a0 = 0x1111;
	frame.a1 = 0x2222;
	frame.a2 = 0x3333;
	frame.a3 = 0x4444;
	frame.a4 = 0;
	frame.a5 = 0;
	frame.a6 = 0;
	frame.a7 = (u64)-999;
	frame.s2 = 0;
	frame.s3 = 0;
	frame.s4 = 0;
	frame.s5 = 0;
	frame.s6 = 0;
	frame.s7 = 0;
	frame.s8 = 0;
	frame.s9 = 0;
	frame.s10 = 0;
	frame.s11 = 0;
	frame.t3 = 0;
	frame.t4 = 0;
	frame.t5 = 0;
	frame.t6 = 0;

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
