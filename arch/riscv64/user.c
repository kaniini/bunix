#include <arch/user.h>
#include <arch/interrupts.h>
#include <arch/layout.h>
#include <arch/sbi.h>
#include "sched.h"

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

u16 arch_user_elf_machine(void)
{
	return 243;
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

static void mem_copy(u8 *dst, const u8 *src, u64 len)
{
	for (u64 i = 0; i < len; i++) {
		dst[i] = src[i];
	}
}

static int user_copy_args_valid(const void *kernel, u64 user, u64 len)
{
	if (len == 0) {
		return 0;
	}
	return kernel != 0 && riscv64_user_addr_valid(user, len);
}

int arch_user_copy_from(void *dst, u64 user_src, u64 len)
{
	const u64 saved_status = riscv64_sstatus_read();

	if (!user_copy_args_valid(dst, user_src, len)) {
		return -1;
	}

	riscv64_sstatus_write(saved_status | RISCV64_SSTATUS_SUM);
	mem_copy((u8 *)dst, (const u8 *)user_src, len);
	riscv64_sstatus_write(saved_status);
	return 0;
}

int arch_user_copy_to(u64 user_dst, const void *src, u64 len)
{
	const u64 saved_status = riscv64_sstatus_read();

	if (!user_copy_args_valid(src, user_dst, len)) {
		return -1;
	}

	riscv64_sstatus_write(saved_status | RISCV64_SSTATUS_SUM);
	mem_copy((u8 *)user_dst, (const u8 *)src, len);
	riscv64_sstatus_write(saved_status);
	return 0;
}

static void early_console_write_user(u64 text, u64 len)
{
	for (u64 i = 0; i < len; i++) {
		char ch;

		if (arch_user_copy_from(&ch, text + i, sizeof(ch)) != 0) {
			break;
		}
		if (ch == '\n') {
			riscv64_sbi_putchar('\r');
		}
		riscv64_sbi_putchar(ch);
	}
}

u64 arch_syscall_dispatch(struct arch_syscall_frame *frame)
{
	(void)strace_mode;
	if (frame == 0) {
		return (u64)-1;
	}
	switch ((long)frame->number) {
	case RISCV64_SYSCALL_EXIT:
		thread_exit();
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
