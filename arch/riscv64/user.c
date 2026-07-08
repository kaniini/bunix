#include <arch/user.h>
#include <arch/interrupts.h>
#include <arch/sbi.h>

enum {
	RISCV64_SYSCALL_EXIT = -2,
	RISCV64_SYSCALL_TIMER_TICKS = -4,
	RISCV64_SYSCALL_EARLY_CONSOLE_WRITE = -54,
	RISCV64_SYSCALL_EARLY_CONSOLE_LOG = -56,
	RISCV64_SSTATUS_SUM = 1ULL << 18,
};

static const char *strace_mode;
static u64 current_kernel_stack;

void riscv64_user_enter(u64 entry, u64 stack, u64 kernel_stack)
	__attribute__((noreturn));

void arch_user_init(void)
{
	strace_mode = "off";
	current_kernel_stack = 0;
}

void arch_user_set_strace_mode(const char *mode)
{
	strace_mode = mode;
}

void arch_user_init_cpu(u32 cpu_id)
{
	(void)cpu_id;
}

void arch_user_set_kernel_stack(u64 stack)
{
	current_kernel_stack = stack;
}

void arch_user_set_fs_base(u64 fs_base)
{
	(void)fs_base;
}

void arch_user_enter(u64 entry, u64 stack)
{
	riscv64_user_enter(entry, stack, current_kernel_stack);
}

void arch_user_resume(const struct arch_syscall_frame *frame)
{
	(void)frame;
	for (;;) {
		__asm__ volatile ("wfi");
	}
}

static u64 riscv64_sstatus_read(void)
{
	u64 value;

	__asm__ volatile ("csrr %0, sstatus" : "=r"(value));
	return value;
}

static void riscv64_sstatus_write(u64 value)
{
	__asm__ volatile ("csrw sstatus, %0" : : "r"(value) : "memory");
}

static void early_console_write_user(u64 text, u64 len)
{
	const u64 saved_status = riscv64_sstatus_read();
	const char *user_text = (const char *)text;

	riscv64_sstatus_write(saved_status | RISCV64_SSTATUS_SUM);
	for (u64 i = 0; i < len; i++) {
		if (user_text[i] == '\n') {
			riscv64_sbi_putchar('\r');
		}
		riscv64_sbi_putchar(user_text[i]);
	}
	riscv64_sstatus_write(saved_status);
}

u64 arch_syscall_dispatch(struct arch_syscall_frame *frame)
{
	(void)strace_mode;
	if (frame == 0) {
		return (u64)-1;
	}
	switch ((long)frame->number) {
	case RISCV64_SYSCALL_EXIT:
		return frame->arg0;
	case RISCV64_SYSCALL_TIMER_TICKS:
		return arch_timer_ticks();
	case RISCV64_SYSCALL_EARLY_CONSOLE_WRITE:
	case RISCV64_SYSCALL_EARLY_CONSOLE_LOG:
		early_console_write_user(frame->arg0, frame->arg1);
		return 0;
	default:
		break;
	}
	return (u64)-1;
}
