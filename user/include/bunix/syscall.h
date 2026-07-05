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
	BUNIX_SYSCALL_CLOCK_MONOTONIC_NS = -20,
	BUNIX_SYSCALL_SLEEP_NS = -22,
	BUNIX_SYSCALL_TASK_CREATE = -24,
	BUNIX_SYSCALL_TASK_MAP = -26,
	BUNIX_SYSCALL_TASK_GRANT = -28,
	BUNIX_SYSCALL_TASK_START = -30,
	BUNIX_SYSCALL_BUFFER_CREATE = -32,
	BUNIX_SYSCALL_BUFFER_READ = -34,
	BUNIX_SYSCALL_BUFFER_WRITE = -36,
	BUNIX_SYSCALL_TASK_WRITE = -38,
	BUNIX_SYSCALL_TASK_START_AT = -40,
	BUNIX_SYSCALL_TASK_ID = -42,
	BUNIX_SYSCALL_TASK_ALLOC = -44,
	BUNIX_SYSCALL_TASK_CLONE_RANGE = -46,
	BUNIX_SYSCALL_CONSOLE_READ = -48,
	BUNIX_SYSCALL_TASK_KILL = -50,
	BUNIX_SYSCALL_TASK_INFO = -52,
	BUNIX_SYSCALL_EARLY_CONSOLE_WRITE = -54,
	BUNIX_SYSCALL_EARLY_CONSOLE_LOG = -56,
	BUNIX_SYSCALL_EARLY_CONSOLE_LOGS_TO_RING = -58,
	BUNIX_SYSCALL_IPC_STATS = -60,
	BUNIX_SYSCALL_VM_STATS = -62,
	BUNIX_IPC_WORDS = 4,
	BUNIX_IPC_STATS_CPUS = 8,
	BUNIX_IPC_DATA_BYTES = (BUNIX_IPC_WORDS - 2) * 8,
	BUNIX_RIGHT_SEND = 1 << 0,
	BUNIX_RIGHT_RECV = 1 << 1,
	BUNIX_RIGHT_DUP = 1 << 2,
	BUNIX_PROTO_CONSOLE = ('C') | ('O' << 8) | ('N' << 16) | ('S' << 24),
	BUNIX_PROTO_VM = ('V') | ('M' << 8) | ('E' << 16) | ('M' << 24),
	BUNIX_PROTO_PING = ('P') | ('I' << 8) | ('N' << 16) | ('G' << 24),
	BUNIX_PROTO_NAMES = ('N') | ('A' << 8) | ('M' << 16) | ('E' << 24),
	BUNIX_PROTO_TIME = ('T') | ('I' << 8) | ('M' << 16) | ('E' << 24),
	BUNIX_PROTO_PROC = ('P') | ('R' << 8) | ('O' << 16) | ('C' << 24),
	BUNIX_PROTO_BLOCK = ('B') | ('L' << 8) | ('K' << 16) | ('0' << 24),
	BUNIX_PROTO_VFS = ('V') | ('F' << 8) | ('S' << 16) | ('0' << 24),
	BUNIX_PROTO_PROCFS = ('P') | ('F' << 8) | ('S' << 16) | ('0' << 24),
	BUNIX_PROTO_LINUX = ('L') | ('I' << 8) | ('N' << 16) | ('X' << 24),
	BUNIX_PROTO_USER = ('U') | ('S' << 8) | ('E' << 16) | ('R' << 24),
	BUNIX_PROTO_TMPFS = ('T') | ('M' << 8) | ('P' << 16) | ('F' << 24),
	BUNIX_PROTO_UNIONFS = ('U') | ('N' << 8) | ('I' << 16) | ('O' << 24),
	BUNIX_PROTO_DEVFS = ('D') | ('E' << 8) | ('V' << 16) | ('F' << 24),
	BUNIX_PROCFS_MOUNT_NOTIFY = 1,
	BUNIX_NAMES_REGISTER = 1,
	BUNIX_NAMES_RESOLVE = 2,
	BUNIX_NAMES_CREATE_NS = 3,
	BUNIX_NAMES_WAIT = 4,
	BUNIX_NAMES_ROOT = 1,
	BUNIX_BLOCK_GET_INFO = 1,
	BUNIX_BLOCK_READ = 2,
	BUNIX_BLOCK_READ_BUFFER = 3,
	BUNIX_VFS_READ = 1,
	BUNIX_VFS_READ_PATH = 2,
	BUNIX_VFS_READ_PATH_BUFFER = 3,
	BUNIX_VFS_OPEN = 4,
	BUNIX_VFS_STAT = 5,
	BUNIX_VFS_READ_FILE_BUFFER = 6,
	BUNIX_VFS_CLOSE = 7,
	BUNIX_VFS_STAT_META = 8,
	BUNIX_VFS_STAT_PATH_META = 9,
	BUNIX_VFS_READDIR = 10,
	BUNIX_VFS_MOUNT = 11,
	BUNIX_VFS_OPEN_BUFFER = 12,
	BUNIX_VFS_STAT_PATH_META_BUFFER = 13,
	BUNIX_VFS_READLINK_BUFFER = 14,
	BUNIX_VFS_ACCESS_BUFFER = 15,
	BUNIX_VFS_CREATE_BUFFER = 16,
	BUNIX_VFS_WRITE_FILE_BUFFER = 17,
	BUNIX_VFS_TRUNCATE = 18,
	BUNIX_VFS_UNLINK_BUFFER = 19,
	BUNIX_VFS_CHMOD_BUFFER = 20,
	BUNIX_VFS_CHMOD = 21,
	BUNIX_VFS_MKDIR_BUFFER = 22,
	BUNIX_VFS_CHOWN_BUFFER = 23,
	BUNIX_VFS_CHOWN = 24,
	BUNIX_VFS_RMDIR_BUFFER = 25,
	BUNIX_VFS_TYPE_REGULAR = 1,
	BUNIX_VFS_TYPE_DIRECTORY = 2,
	BUNIX_VFS_TYPE_SYMLINK = 3,
	BUNIX_VFS_ERR_NOENT = 1,
	BUNIX_VFS_ERR_ACCESS = 2,
	BUNIX_VFS_ERR_NOTDIR = 3,
	BUNIX_VFS_ERR_ISDIR = 4,
	BUNIX_LINUX_READ = 0,
	BUNIX_LINUX_WRITE = 1,
	BUNIX_LINUX_OPEN = 2,
	BUNIX_LINUX_CLOSE = 3,
	BUNIX_LINUX_FSTAT = 5,
	BUNIX_LINUX_MMAP = 9,
	BUNIX_LINUX_RT_SIGACTION = 13,
	BUNIX_LINUX_RT_SIGPROCMASK = 14,
	BUNIX_LINUX_IOCTL = 16,
	BUNIX_LINUX_PIPE = 22,
	BUNIX_LINUX_DUP = 32,
	BUNIX_LINUX_DUP2 = 33,
	BUNIX_LINUX_SENDFILE = 40,
	BUNIX_LINUX_TRUNCATE = 76,
	BUNIX_LINUX_FTRUNCATE = 77,
	BUNIX_LINUX_RMDIR = 84,
	BUNIX_LINUX_UNLINK = 87,
	BUNIX_LINUX_SOCKET = 41,
	BUNIX_LINUX_CONNECT = 42,
	BUNIX_LINUX_SENDTO = 44,
	BUNIX_LINUX_RECVFROM = 45,
	BUNIX_LINUX_GETCWD = 79,
	BUNIX_LINUX_CHDIR = 80,
	BUNIX_LINUX_MKDIR = 83,
	BUNIX_LINUX_CHMOD = 90,
	BUNIX_LINUX_FCHMOD = 91,
	BUNIX_LINUX_CHOWN = 92,
	BUNIX_LINUX_FCHOWN = 93,
	BUNIX_LINUX_LCHOWN = 94,
	BUNIX_LINUX_ACCESS = 21,
	BUNIX_LINUX_READLINK = 89,
	BUNIX_LINUX_UMASK = 95,
	BUNIX_LINUX_GETPID = 39,
	BUNIX_LINUX_SETPGID = 109,
	BUNIX_LINUX_GETPPID = 110,
	BUNIX_LINUX_GETPGRP = 111,
	BUNIX_LINUX_SETSID = 112,
	BUNIX_LINUX_GETGROUPS = 115,
	BUNIX_LINUX_KILL = 62,
	BUNIX_LINUX_GETPGID = 121,
	BUNIX_LINUX_GETSID = 124,
	BUNIX_LINUX_FCNTL = 72,
	BUNIX_LINUX_GETDENTS64 = 217,
	BUNIX_LINUX_GETUID = 102,
	BUNIX_LINUX_GETGID = 104,
	BUNIX_LINUX_SETUID = 105,
	BUNIX_LINUX_SETGID = 106,
	BUNIX_LINUX_GETEUID = 107,
	BUNIX_LINUX_GETEGID = 108,
	BUNIX_LINUX_WAIT4 = 61,
	BUNIX_LINUX_SETGROUPS = 116,
	BUNIX_LINUX_SETRESUID = 117,
	BUNIX_LINUX_SETRESGID = 119,
	BUNIX_LINUX_RT_SIGTIMEDWAIT = 128,
	BUNIX_LINUX_REBOOT = 169,
	BUNIX_LINUX_GETTID = 186,
	BUNIX_LINUX_OPENAT = 257,
	BUNIX_LINUX_MKDIRAT = 258,
	BUNIX_LINUX_FCHOWNAT = 260,
	BUNIX_LINUX_UNLINKAT = 263,
	BUNIX_LINUX_NEWFSTATAT = 262,
	BUNIX_LINUX_READLINKAT = 267,
	BUNIX_LINUX_FCHMODAT = 268,
	BUNIX_LINUX_FACCESSAT = 269,
	BUNIX_LINUX_DUP3 = 292,
	BUNIX_LINUX_PIPE2 = 293,
	BUNIX_LINUX_STATX = 332,
	BUNIX_LINUX_FACCESSAT2 = 439,
	BUNIX_LINUX_REGISTER_PROCESS = 1000,
	BUNIX_LINUX_FORK_PROCESS = 1001,
	BUNIX_LINUX_EXEC_PROCESS = 1002,
	BUNIX_LINUX_TTY_INPUT = 1003,
	BUNIX_LINUX_EXEC_INIT = 1004,
	BUNIX_LINUX_EXIT_GROUP = 231,
	BUNIX_TIME_NOW_MONOTONIC = 1,
	BUNIX_TIME_SLEEP_NS = 2,
	BUNIX_PROC_SPAWN = 1,
	BUNIX_PROC_SPAWN_FIRST = BUNIX_PROC_SPAWN,
	BUNIX_PROC_WAIT = 2,
	BUNIX_PROC_EXIT = 3,
	BUNIX_PROC_SELF = 4,
	BUNIX_PROC_INFO = 5,
	BUNIX_PROC_AT = 6,
	BUNIX_PROC_CMDLINE = 7,
	BUNIX_PROC_SET_CMDLINE = 8,
	BUNIX_PROC_REGISTER_LINUX = 9,
	BUNIX_PROC_DETAILS = 10,
	BUNIX_SERVICE_CONSOLE = BUNIX_PROTO_CONSOLE,
	BUNIX_SERVICE_VM = BUNIX_PROTO_VM,
	BUNIX_SERVICE_TIME = BUNIX_PROTO_TIME,
	BUNIX_SERVICE_PROC = BUNIX_PROTO_PROC,
	BUNIX_SERVICE_LINUX = BUNIX_PROTO_LINUX,
	BUNIX_SERVICE_BLOCK = BUNIX_PROTO_BLOCK,
	BUNIX_SERVICE_VFS = BUNIX_PROTO_VFS,
	BUNIX_SERVICE_PROCFS = BUNIX_PROTO_PROCFS,
	BUNIX_SERVICE_USER = BUNIX_PROTO_USER,
	BUNIX_SERVICE_TMPFS = BUNIX_PROTO_TMPFS,
	BUNIX_SERVICE_UNIONFS = BUNIX_PROTO_UNIONFS,
	BUNIX_SERVICE_DEVFS = BUNIX_PROTO_DEVFS,
	BUNIX_USER_GETUID = 1,
	BUNIX_USER_GETGID = 2,
	BUNIX_USER_GETEUID = 3,
	BUNIX_USER_GETEGID = 4,
	BUNIX_USER_REGISTER_PROCESS = 5,
	BUNIX_USER_FORK_PROCESS = 6,
	BUNIX_USER_EXIT_PROCESS = 7,
	BUNIX_USER_GETGROUPS = 8,
	BUNIX_USER_SETRESUID = 9,
	BUNIX_USER_SETRESGID = 10,
	BUNIX_USER_SETGROUPS = 11,
	BUNIX_USER_HAS_GROUP = 12,
	BUNIX_USER_CAN_ACCESS = 13,
	BUNIX_USER_APPLY_LOGIN = 14,
	BUNIX_USER_AUTH_LOGIN = 15,
	BUNIX_USER_SESSION_BEGIN = 16,
	BUNIX_USER_SESSION_END = 17,
	BUNIX_USER_SESSION_GET = 18,
	BUNIX_USER_SESSION_SET_FOREGROUND = 19,
	BUNIX_USER_SESSION_COUNT = 20,
	BUNIX_USER_SESSION_AT = 21,
	BUNIX_USER_LOGIN_GROUPS = 22,
	BUNIX_CONSOLE_WRITE = 1,
	BUNIX_CONSOLE_LOG = 2,
	BUNIX_CONSOLE_LOGS_TO_RING = 3,
	BUNIX_HANDLE_SELF = 1,
	BUNIX_HANDLE_CONSOLE = 2,
	BUNIX_HANDLE_VM = 3,
	BUNIX_HANDLE_NAMES = 4,
	BUNIX_AT_STDOUT = 0x62780101,
	BUNIX_AT_STDERR = 0x62780102,
	BUNIX_AT_TIME = 0x62780103,
	BUNIX_AT_PROC = 0x62780104,
	BUNIX_AT_NAMES = 0x62780106,
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

struct bunix_ipc_stats {
	u64 sends;
	u64 queued;
	u64 direct_delivered;
	u64 direct_handoff;
	u64 fallback_cross_cpu;
	u64 fallback_nested;
	u64 fallback_scheduler;
	u64 fallback_invalid;
	u64 cpu_sends[BUNIX_IPC_STATS_CPUS];
	u64 cpu_queued[BUNIX_IPC_STATS_CPUS];
	u64 cpu_direct_delivered[BUNIX_IPC_STATS_CPUS];
	u64 cpu_direct_handoff[BUNIX_IPC_STATS_CPUS];
	u64 cpu_fallback_cross_cpu[BUNIX_IPC_STATS_CPUS];
	u64 cpu_fallback_nested[BUNIX_IPC_STATS_CPUS];
	u64 cpu_fallback_scheduler[BUNIX_IPC_STATS_CPUS];
	u64 cpu_fallback_invalid[BUNIX_IPC_STATS_CPUS];
};

struct bunix_vm_stats {
	u64 total_frames;
	u64 free_frames;
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

static inline u64 bunix_clock_monotonic_ns(void)
{
	return (u64)bunix_syscall0(BUNIX_SYSCALL_CLOCK_MONOTONIC_NS);
}

static inline long bunix_sleep_ns(u64 ns)
{
	return bunix_syscall1(BUNIX_SYSCALL_SLEEP_NS, ns);
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

static inline long bunix_task_create(const char *name)
{
	return bunix_syscall1(BUNIX_SYSCALL_TASK_CREATE, (u64)name);
}

static inline long bunix_task_id(u64 task)
{
	return bunix_syscall1(BUNIX_SYSCALL_TASK_ID, task);
}

static inline long bunix_task_info(u64 index, u64 *pid_threads_flags,
				   u64 *name_words)
{
	return bunix_syscall3(BUNIX_SYSCALL_TASK_INFO, index,
			      (u64)pid_threads_flags, (u64)name_words);
}

static inline long bunix_task_map(u64 task, u64 vaddr, const void *src,
				  u64 filesz, u64 memsz, u64 writable)
{
	const u64 args[] = { task, vaddr, (u64)src, filesz, memsz, writable };

	return bunix_syscall3(BUNIX_SYSCALL_TASK_MAP, (u64)args, 0, 0);
}

static inline long bunix_task_grant(u64 task, u64 handle, unsigned int rights)
{
	return bunix_syscall3(BUNIX_SYSCALL_TASK_GRANT, task, handle, rights);
}

static inline long bunix_task_start(u64 task, u64 entry)
{
	return bunix_syscall2(BUNIX_SYSCALL_TASK_START, task, entry);
}

static inline long bunix_task_write(u64 task, u64 vaddr, const void *src,
				    u64 len)
{
	const u64 args[] = { task, vaddr, (u64)src, len };

	return bunix_syscall3(BUNIX_SYSCALL_TASK_WRITE, (u64)args, 0, 0);
}

static inline long bunix_task_alloc(u64 task, u64 vaddr, u64 len,
				    u64 writable)
{
	const u64 args[] = { task, vaddr, len, writable };

	return bunix_syscall3(BUNIX_SYSCALL_TASK_ALLOC, (u64)args, 0, 0);
}

static inline long bunix_task_clone_range(u64 dst_task, u64 src_task,
					  u64 vaddr, u64 len, u64 writable)
{
	const u64 args[] = { dst_task, src_task, vaddr, len, writable };

	return bunix_syscall3(BUNIX_SYSCALL_TASK_CLONE_RANGE, (u64)args, 0, 0);
}

static inline long bunix_task_start_at(u64 task, u64 entry, u64 stack)
{
	return bunix_syscall3(BUNIX_SYSCALL_TASK_START_AT, task, entry, stack);
}

static inline long bunix_task_kill(u64 task)
{
	return bunix_syscall1(BUNIX_SYSCALL_TASK_KILL, task);
}

static inline long bunix_buffer_create(u64 size)
{
	return bunix_syscall1(BUNIX_SYSCALL_BUFFER_CREATE, size);
}

static inline long bunix_buffer_read(u64 handle, u64 offset, void *dst, u64 len)
{
	const u64 args[] = { handle, offset, (u64)dst, len };

	return bunix_syscall3(BUNIX_SYSCALL_BUFFER_READ, (u64)args, 0, 0);
}

static inline long bunix_buffer_write(u64 handle, u64 offset, const void *src,
				      u64 len)
{
	const u64 args[] = { handle, offset, (u64)src, len };

	return bunix_syscall3(BUNIX_SYSCALL_BUFFER_WRITE, (u64)args, 0, 0);
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

static inline long bunix_console_send(unsigned int type, const char *text,
				      usize len)
{
	u64 buffer;
	long result;
	struct bunix_msg message = {
		.protocol = BUNIX_PROTO_CONSOLE,
		.type = type,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_RECV,
		.reply = 0,
		.cap = 0,
		.words = { len, 0, 0, 0 },
	};

	if (len == 0) {
		return 0;
	}

	buffer = bunix_buffer_create(len);
	if (buffer == (u64)-1 ||
	    bunix_buffer_write(buffer, 0, text, len) != 0) {
		if (buffer != (u64)-1) {
			bunix_handle_close(buffer);
		}
		return -1;
	}

	message.cap = buffer;
	result = bunix_ipc_send(BUNIX_HANDLE_CONSOLE, &message);
	bunix_handle_close(buffer);
	return result;
}

static inline long bunix_console_write(const char *text, usize len)
{
	return bunix_console_send(BUNIX_CONSOLE_WRITE, text, len);
}

static inline long bunix_console_log(const char *text, usize len)
{
	return bunix_console_send(BUNIX_CONSOLE_LOG, text, len);
}

static inline long bunix_console_logs_to_ring(void)
{
	const struct bunix_msg message = {
		.protocol = BUNIX_PROTO_CONSOLE,
		.type = BUNIX_CONSOLE_LOGS_TO_RING,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { 0, 0, 0, 0 },
	};

	return bunix_ipc_send(BUNIX_HANDLE_CONSOLE, &message);
}

static inline long bunix_early_console_write(const char *text, usize len)
{
	return bunix_syscall2(BUNIX_SYSCALL_EARLY_CONSOLE_WRITE,
			      (u64)text, len);
}

static inline long bunix_early_console_log(const char *text, usize len)
{
	return bunix_syscall2(BUNIX_SYSCALL_EARLY_CONSOLE_LOG,
			      (u64)text, len);
}

static inline long bunix_early_console_logs_to_ring(void)
{
	return bunix_syscall0(BUNIX_SYSCALL_EARLY_CONSOLE_LOGS_TO_RING);
}

static inline long bunix_console_read(char *buffer, usize len)
{
	return bunix_syscall2(BUNIX_SYSCALL_CONSOLE_READ, (u64)buffer, len);
}

static inline long bunix_ipc_stats(struct bunix_ipc_stats *stats)
{
	return bunix_syscall1(BUNIX_SYSCALL_IPC_STATS, (u64)stats);
}

static inline long bunix_vm_stats(struct bunix_vm_stats *stats)
{
	return bunix_syscall1(BUNIX_SYSCALL_VM_STATS, (u64)stats);
}

#endif
