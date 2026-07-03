#ifndef BUNIX_USER_SYSCALL_H
#define BUNIX_USER_SYSCALL_H

typedef unsigned long usize;
typedef signed long isize;
typedef unsigned long u64;

enum {
	BUNIX_SYSCALL_EXIT = -2,
	BUNIX_SYSCALL_TIMER_TICKS = -4,
	BUNIX_SYSCALL_LAUNCH_MODULE = -8,
	BUNIX_SYSCALL_PORT_CREATE = -10,
	BUNIX_SYSCALL_IPC_SEND = -12,
	BUNIX_SYSCALL_IPC_RECV = -13,
	BUNIX_SYSCALL_IPC_CALL = -14,
	BUNIX_IPC_WORDS = 4,
	BUNIX_CONSOLE_WRITE = 1,
	BUNIX_HANDLE_SELF = 1,
	BUNIX_HANDLE_CONSOLE = 2,
	BUNIX_HANDLE_VM = 3,
};

struct bunix_msg {
	unsigned int type;
	unsigned int sender;
	u64 reply;
	u64 words[BUNIX_IPC_WORDS];
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

static inline long bunix_launch_module(const char *name)
{
	return bunix_syscall3(BUNIX_SYSCALL_LAUNCH_MODULE, (u64)name, 0, 0);
}

static inline long bunix_launch_module_with_caps(const char *name,
						 const u64 *handles,
						 u64 handle_count)
{
	return bunix_syscall3(BUNIX_SYSCALL_LAUNCH_MODULE, (u64)name,
			      (u64)handles, handle_count);
}

static inline u64 bunix_timer_ticks(void)
{
	return (u64)bunix_syscall0(BUNIX_SYSCALL_TIMER_TICKS);
}

static inline long bunix_port_create(const char *name)
{
	return bunix_syscall1(BUNIX_SYSCALL_PORT_CREATE, (u64)name);
}

static inline long bunix_ipc_send(u64 port, const struct bunix_msg *message)
{
	return bunix_syscall2(BUNIX_SYSCALL_IPC_SEND, port, (u64)message);
}

static inline long bunix_ipc_recv(u64 port, struct bunix_msg *message)
{
	return bunix_syscall2(BUNIX_SYSCALL_IPC_RECV, port, (u64)message);
}

static inline long bunix_ipc_call(u64 port, const struct bunix_msg *request,
				  struct bunix_msg *reply)
{
	return bunix_syscall3(BUNIX_SYSCALL_IPC_CALL, port, (u64)request,
			      (u64)reply);
}

static inline long bunix_console_write(const char *text, usize len)
{
	const struct bunix_msg message = {
		.type = BUNIX_CONSOLE_WRITE,
		.sender = 0,
		.reply = 0,
		.words = { (u64)text, len, 0, 0 },
	};

	return bunix_ipc_send(BUNIX_HANDLE_CONSOLE, &message);
}

#endif
