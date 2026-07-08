#include <arch/user.h>
#include <arch/interrupts.h>
#include <arch/layout.h>
#include <arch/power.h>
#include <arch/sbi.h>
#include "console.h"
#include "sched.h"
#include "server.h"
#include "timer.h"

enum {
	SYSCALL_EXIT = -2,
	SYSCALL_TIMER_TICKS = -4,
	SYSCALL_LAUNCH_MODULE = -8,
	SYSCALL_BOOT_MODULE_READ = -18,
	SYSCALL_CLOCK_MONOTONIC_NS = -20,
	SYSCALL_SLEEP_NS = -22,
	SYSCALL_TASK_ID = -42,
	SYSCALL_EARLY_CONSOLE_WRITE = -54,
	SYSCALL_EARLY_CONSOLE_LOG = -56,
	SYSCALL_MACHINE_POWER = -64,
	LINUX_RISCV64_WRITE = 64,
	LINUX_RISCV64_EXIT = 93,
	LINUX_RISCV64_EXIT_GROUP = 94,
	LINUX_RISCV64_SET_TID_ADDRESS = 96,
	RISCV64_SSTATUS_SUM = 1ULL << 18,
	RISCV64_USER_COPY_CHUNK = 256,
	RISCV64_MAX_CSTR = 256,
	MAX_INHERITED_CAPS = 8,
};

struct native_syscall_args {
	u64 arg0;
	u64 arg1;
	u64 arg2;
	u64 arg3;
};

struct native_syscall_entry {
	i64 number;
	const char *name;
	u64 (*fn)(const struct native_syscall_args *args);
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

static u64 min_u64(u64 left, u64 right)
{
	return left < right ? left : right;
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
	u8 buffer[RISCV64_USER_COPY_CHUNK];
	u64 done = 0;

	if (text == 0) {
		return;
	}
	while (done < len) {
		const u64 chunk = min_u64(len - done, sizeof(buffer));

		if (arch_user_copy_from(buffer, text + done, chunk) != 0) {
			return;
		}
		console_write_len((const char *)buffer, chunk);
		done += chunk;
	}
}

static int copy_cstr_from_user(char *dst, u64 user_src, u64 capacity)
{
	if (dst == 0 || user_src == 0 || capacity == 0) {
		return -1;
	}
	for (u64 i = 0; i < capacity; i++) {
		if (arch_user_copy_from(&dst[i], user_src + i, 1) != 0) {
			return -1;
		}
		if (dst[i] == '\0') {
			return 0;
		}
	}
	dst[capacity - 1] = '\0';
	return -1;
}

static u64 native_sys_exit(const struct native_syscall_args *args)
{
	console_printf("syscall: exit status=%u\n", (u32)args->arg0);
	thread_exit();
}

static u64 native_sys_timer_ticks(const struct native_syscall_args *args)
{
	(void)args;
	return timer_ticks();
}

static u64 native_sys_clock_monotonic_ns(const struct native_syscall_args *args)
{
	(void)args;
	return timer_monotonic_ns();
}

static u64 native_sys_sleep_ns(const struct native_syscall_args *args)
{
	thread_sleep_ns(args->arg0);
	return 0;
}

static u64 native_sys_launch_module(const struct native_syscall_args *args)
{
	char name[RISCV64_MAX_CSTR];
	struct task_launch_cap caps[MAX_INHERITED_CAPS];

	if (copy_cstr_from_user(name, args->arg0, sizeof(name)) != 0 ||
	    args->arg2 > MAX_INHERITED_CAPS) {
		return (u64)-1;
	}
	if (args->arg2 != 0 &&
	    (args->arg1 == 0 ||
	     arch_user_copy_from(caps, args->arg1,
				 args->arg2 * sizeof(caps[0])) != 0)) {
		return (u64)-1;
	}
	return server_launch_module_with_caps(name, task_current(),
					      args->arg2 != 0 ? caps : 0,
					      args->arg2);
}

static u64 native_sys_boot_module_read(const struct native_syscall_args *args)
{
	u8 buffer[RISCV64_USER_COPY_CHUNK];
	u64 done = 0;

	if (args->arg1 == 0) {
		return server_boot_module_size();
	}
	while (done < args->arg2) {
		const u64 chunk = min_u64(args->arg2 - done, sizeof(buffer));

		if (server_boot_module_read(args->arg0 + done, buffer, chunk) != 0 ||
		    arch_user_copy_to(args->arg1 + done, buffer, chunk) != 0) {
			return (u64)-1;
		}
		done += chunk;
	}
	return 0;
}

static u64 native_sys_task_id(const struct native_syscall_args *args)
{
	struct task *task;

	if (args->arg0 == 0) {
		return task_id(task_current());
	}
	task = task_from_handle(task_current(), args->arg0, TASK_RIGHT_SEND);
	return task != 0 ? task_id(task) : (u64)-1;
}

static u64 native_sys_early_console_write(const struct native_syscall_args *args)
{
	early_console_write_user(args->arg0, args->arg1);
	return 0;
}

static u64 native_sys_early_console_log(const struct native_syscall_args *args)
{
	u8 buffer[RISCV64_USER_COPY_CHUNK];
	u64 done = 0;

	if (args->arg0 == 0) {
		return (u64)-1;
	}
	while (done < args->arg1) {
		const u64 chunk = min_u64(args->arg1 - done, sizeof(buffer));

		if (arch_user_copy_from(buffer, args->arg0 + done, chunk) != 0) {
			return (u64)-1;
		}
		console_log_write_len((const char *)buffer, chunk);
		done += chunk;
	}
	return 0;
}

static u64 native_sys_machine_power(const struct native_syscall_args *args)
{
	(void)args;
	arch_poweroff();
}

static const struct native_syscall_entry native_syscalls[] = {
	{ SYSCALL_EXIT, "exit", native_sys_exit },
	{ SYSCALL_TIMER_TICKS, "timer_ticks", native_sys_timer_ticks },
	{ SYSCALL_LAUNCH_MODULE, "launch_module", native_sys_launch_module },
	{ SYSCALL_BOOT_MODULE_READ, "boot_module_read",
	  native_sys_boot_module_read },
	{ SYSCALL_CLOCK_MONOTONIC_NS, "clock_monotonic_ns",
	  native_sys_clock_monotonic_ns },
	{ SYSCALL_SLEEP_NS, "sleep_ns", native_sys_sleep_ns },
	{ SYSCALL_TASK_ID, "task_id", native_sys_task_id },
	{ SYSCALL_EARLY_CONSOLE_WRITE, "early_console_write",
	  native_sys_early_console_write },
	{ SYSCALL_EARLY_CONSOLE_LOG, "early_console_log",
	  native_sys_early_console_log },
	{ SYSCALL_MACHINE_POWER, "machine_power", native_sys_machine_power },
};

static const struct native_syscall_entry *native_syscall_lookup(i64 number)
{
	for (u64 i = 0; i < sizeof(native_syscalls) / sizeof(native_syscalls[0]);
	     i++) {
		if (native_syscalls[i].number == number) {
			return &native_syscalls[i];
		}
	}
	return 0;
}

static u64 linux_riscv64_write(u64 fd, u64 user_buffer, u64 len)
{
	if (fd != 1 && fd != 2) {
		return (u64)-1;
	}
	early_console_write_user(user_buffer, len);
	return len;
}

static u64 linux_riscv64_syscall_dispatch(struct arch_syscall_frame *frame)
{
	switch (frame->number) {
	case LINUX_RISCV64_WRITE:
		return linux_riscv64_write(frame->a[0], frame->a[1],
					   frame->a[2]);
	case LINUX_RISCV64_EXIT:
	case LINUX_RISCV64_EXIT_GROUP:
		console_printf("linux-riscv64: exit status=%u\n",
			       (u32)frame->a[0]);
		thread_exit();
	case LINUX_RISCV64_SET_TID_ADDRESS:
		return task_id(task_current());
	default:
		console_printf("linux-riscv64: unknown syscall=%u pc=%p arg0=%p arg1=%p arg2=%p arg3=%p\n",
			       (u32)frame->number,
			       (const void *)frame->user_pc,
			       (const void *)frame->a[0],
			       (const void *)frame->a[1],
			       (const void *)frame->a[2],
			       (const void *)frame->a[3]);
		return (u64)-1;
	}
}

u64 arch_syscall_dispatch(struct arch_syscall_frame *frame)
{
	const struct native_syscall_entry *entry;
	struct native_syscall_args args;

	(void)strace_mode;
	if (frame == 0) {
		return (u64)-1;
	}
	if ((i64)frame->number >= 0) {
		return linux_riscv64_syscall_dispatch(frame);
	}
	entry = native_syscall_lookup((i64)frame->number);
	if (entry == 0) {
		console_printf("riscv64: unknown native syscall=%d pc=%p\n",
			       (int)frame->number,
			       (const void *)frame->user_pc);
		return (u64)-1;
	}
	args.arg0 = frame->arg0;
	args.arg1 = frame->arg1;
	args.arg2 = frame->arg2;
	args.arg3 = frame->arg3;
	return entry->fn(&args);
}
