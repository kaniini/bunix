#include <arch/user.h>

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

u64 arch_syscall_dispatch(struct arch_syscall_frame *frame)
{
	(void)frame;
	(void)strace_mode;
	return (u64)-1;
}
