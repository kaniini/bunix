#include <arch/user.h>

static const char *strace_mode;

void arch_user_init(void)
{
	strace_mode = "off";
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
	(void)stack;
}

void arch_user_set_fs_base(u64 fs_base)
{
	(void)fs_base;
}

void arch_user_enter(u64 entry, u64 stack)
{
	(void)entry;
	(void)stack;
	for (;;) {
		__asm__ volatile ("wfi");
	}
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
