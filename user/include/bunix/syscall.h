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
	BUNIX_SYSCALL_HANDLE_CLOSE = -16,
	BUNIX_SYSCALL_BOOT_MODULE_READ = -18,
	BUNIX_SYSCALL_SLEEP_TICKS = -20,
	BUNIX_IPC_WORDS = 4,
	BUNIX_RIGHT_SEND = 1 << 0,
	BUNIX_RIGHT_RECV = 1 << 1,
	BUNIX_RIGHT_DUP = 1 << 2,
	BUNIX_PROTO_CONSOLE = ('C') | ('O' << 8) | ('N' << 16) | ('S' << 24),
	BUNIX_PROTO_VM = ('V') | ('M' << 8) | ('E' << 16) | ('M' << 24),
	BUNIX_PROTO_PING = ('P') | ('I' << 8) | ('N' << 16) | ('G' << 24),
	BUNIX_PROTO_NAMES = ('N') | ('A' << 8) | ('M' << 16) | ('E' << 24),
	BUNIX_PROTO_BLOCK = ('B') | ('L' << 8) | ('K' << 16) | ('0' << 24),
	BUNIX_PROTO_VFS = ('V') | ('F' << 8) | ('S' << 16) | ('0' << 24),
	BUNIX_NAMES_REGISTER = 1,
	BUNIX_NAMES_RESOLVE = 2,
	BUNIX_NAMES_CREATE_NS = 3,
	BUNIX_NAMES_WAIT = 4,
	BUNIX_NAMES_ROOT = 1,
	BUNIX_BLOCK_GET_INFO = 1,
	BUNIX_BLOCK_READ = 2,
	BUNIX_VFS_READ = 1,
	BUNIX_SERVICE_CONSOLE = BUNIX_PROTO_CONSOLE,
	BUNIX_SERVICE_VM = BUNIX_PROTO_VM,
	BUNIX_SERVICE_BLOCK = BUNIX_PROTO_BLOCK,
	BUNIX_SERVICE_VFS = BUNIX_PROTO_VFS,
	BUNIX_CONSOLE_WRITE = 1,
	BUNIX_HANDLE_SELF = 1,
	BUNIX_HANDLE_CONSOLE = 2,
	BUNIX_HANDLE_VM = 3,
	BUNIX_HANDLE_NAMES = 4,
};

struct bunix_msg {
	unsigned int protocol;
	unsigned int type;
	unsigned int sender;
	unsigned int cap_rights;
	u64 reply;
	u64 cap;
	u64 words[BUNIX_IPC_WORDS];
};

struct bunix_launch_cap {
	u64 handle;
	unsigned int rights;
	unsigned int reserved;
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
						 const struct bunix_launch_cap *caps,
						 u64 cap_count)
{
	return bunix_syscall3(BUNIX_SYSCALL_LAUNCH_MODULE, (u64)name,
			      (u64)caps, cap_count);
}

static inline u64 bunix_timer_ticks(void)
{
	return (u64)bunix_syscall0(BUNIX_SYSCALL_TIMER_TICKS);
}

static inline long bunix_sleep_ticks(u64 ticks)
{
	return bunix_syscall1(BUNIX_SYSCALL_SLEEP_TICKS, ticks);
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

static inline long bunix_handle_close(u64 handle)
{
	return bunix_syscall1(BUNIX_SYSCALL_HANDLE_CLOSE, handle);
}

static inline long bunix_boot_module_read(u64 offset, void *buffer, u64 len)
{
	return bunix_syscall3(BUNIX_SYSCALL_BOOT_MODULE_READ, offset,
			      (u64)buffer, len);
}

static inline u64 bunix_boot_module_size(void)
{
	return (u64)bunix_syscall3(BUNIX_SYSCALL_BOOT_MODULE_READ, 0, 0, 0);
}

static inline long bunix_console_write(const char *text, usize len)
{
	const struct bunix_msg message = {
		.protocol = BUNIX_PROTO_CONSOLE,
		.type = BUNIX_CONSOLE_WRITE,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { (u64)text, len, 0, 0 },
	};

	return bunix_ipc_send(BUNIX_HANDLE_CONSOLE, &message);
}

#endif
