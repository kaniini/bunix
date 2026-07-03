#ifndef BUNIX_USER_SYSCALL_H
#define BUNIX_USER_SYSCALL_H

typedef unsigned long usize;
typedef signed long isize;
typedef unsigned long u64;

enum {
	BUNIX_SYSCALL_WRITE = -1,
	BUNIX_SYSCALL_EXIT = -2,
	BUNIX_SYSCALL_VM_PING = -3,
	BUNIX_SYSCALL_TIMER_TICKS = -4,
	BUNIX_SYSCALL_NAME_LOOKUP = -5,
	BUNIX_SYSCALL_SERVICE_WRITE = -6,
	BUNIX_SYSCALL_SERVICE_VM_PING = -7,
	BUNIX_SYSCALL_LAUNCH_MODULE = -8,
	BUNIX_SYSCALL_NAME_REGISTER = -9,
};

static inline long bunix_syscall0(long number)
{
	long rax = number;

	__asm__ volatile ("syscall"
			  : "+a"(rax)
			  :
			  : "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10",
			    "r11", "memory");
	return rax;
}

static inline long bunix_syscall1(long number, u64 arg0)
{
	long rax = number;
	register u64 rdi __asm__("rdi") = arg0;

	__asm__ volatile ("syscall"
			  : "+a"(rax), "+D"(rdi)
			  :
			  : "rcx", "rdx", "rsi", "r8", "r9", "r10", "r11",
			    "memory");
	return rax;
}

static inline long bunix_syscall2(long number, u64 arg0, u64 arg1)
{
	long rax = number;
	register u64 rdi __asm__("rdi") = arg0;
	register u64 rsi __asm__("rsi") = arg1;

	__asm__ volatile ("syscall"
			  : "+a"(rax), "+D"(rdi), "+S"(rsi)
			  :
			  : "rcx", "rdx", "r8", "r9", "r10", "r11", "memory");
	return rax;
}

static inline long bunix_syscall3(long number, u64 arg0, u64 arg1, u64 arg2)
{
	long rax = number;
	register u64 rdi __asm__("rdi") = arg0;
	register u64 rsi __asm__("rsi") = arg1;
	register u64 rdx __asm__("rdx") = arg2;

	__asm__ volatile ("syscall"
			  : "+a"(rax), "+D"(rdi), "+S"(rsi), "+d"(rdx)
			  :
			  : "rcx", "r8", "r9", "r10", "r11", "memory");
	return rax;
}

static inline long bunix_name_register(const char *name)
{
	return bunix_syscall1(BUNIX_SYSCALL_NAME_REGISTER, (u64)name);
}

static inline long bunix_name_lookup(const char *name)
{
	return bunix_syscall1(BUNIX_SYSCALL_NAME_LOOKUP, (u64)name);
}

static inline long bunix_service_write(u64 service, const char *text, usize len)
{
	return bunix_syscall3(BUNIX_SYSCALL_SERVICE_WRITE, service, (u64)text, len);
}

static inline long bunix_launch_module(const char *name)
{
	return bunix_syscall1(BUNIX_SYSCALL_LAUNCH_MODULE, (u64)name);
}

static inline u64 bunix_timer_ticks(void)
{
	return (u64)bunix_syscall0(BUNIX_SYSCALL_TIMER_TICKS);
}

static inline long bunix_service_vm_ping(u64 service, u64 word)
{
	return bunix_syscall2(BUNIX_SYSCALL_SERVICE_VM_PING, service, word);
}

#endif
