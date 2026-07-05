#include <arch/user.h>
#include <arch/io.h>
#include <arch/smp.h>
#include <arch/thread.h>
#include "buffer.h"
#include "console.h"
#include "ipc.h"
#include "sched.h"
#include "slab.h"
#include "spinlock.h"
#include "timer.h"
#include "server.h"
#include "vm.h"
#include "../servers/vm/vm_server.h"

enum {
	GDT_KERNEL_CODE = 0x08,
	GDT_KERNEL_DATA = 0x10,
	GDT_USER_DATA = 0x1b,
	GDT_USER_CODE = 0x23,
	GDT_TSS = 0x28,
	MSR_EFER = 0xc0000080,
	MSR_STAR = 0xc0000081,
	MSR_LSTAR = 0xc0000082,
	MSR_FMASK = 0xc0000084,
	MSR_FS_BASE = 0xc0000100,
	EFER_SCE = 1,
	SYSCALL_EXIT = -2,
	SYSCALL_TIMER_TICKS = -4,
	SYSCALL_LAUNCH_MODULE = -8,
	SYSCALL_PORT_CREATE = -10,
	SYSCALL_IPC_SEND = -12,
	SYSCALL_IPC_RECV = -13,
	SYSCALL_IPC_CALL = -14,
	SYSCALL_HANDLE_CLOSE = -16,
	SYSCALL_BOOT_MODULE_READ = -18,
	SYSCALL_CLOCK_MONOTONIC_NS = -20,
	SYSCALL_SLEEP_NS = -22,
	SYSCALL_TASK_CREATE = -24,
	SYSCALL_TASK_MAP = -26,
	SYSCALL_TASK_GRANT = -28,
	SYSCALL_TASK_START = -30,
	SYSCALL_BUFFER_CREATE = -32,
	SYSCALL_BUFFER_READ = -34,
	SYSCALL_BUFFER_WRITE = -36,
	SYSCALL_TASK_WRITE = -38,
	SYSCALL_TASK_START_AT = -40,
	SYSCALL_TASK_ID = -42,
	SYSCALL_TASK_ALLOC = -44,
	SYSCALL_TASK_CLONE_RANGE = -46,
	SYSCALL_CONSOLE_READ = -48,
	SYSCALL_TASK_KILL = -50,
	SYSCALL_TASK_INFO = -52,
	SYSCALL_EARLY_CONSOLE_WRITE = -54,
	SYSCALL_EARLY_CONSOLE_LOG = -56,
	SYSCALL_EARLY_CONSOLE_LOGS_TO_RING = -58,
	SYSCALL_IPC_STATS = -60,
	SYSCALL_VM_STATS = -62,
	LINUX_SYSCALL_READ = 0,
	LINUX_SYSCALL_WRITE = 1,
	LINUX_SYSCALL_OPEN = 2,
	LINUX_SYSCALL_CLOSE = 3,
	LINUX_SYSCALL_STAT = 4,
	LINUX_SYSCALL_FSTAT = 5,
	LINUX_SYSCALL_LSTAT = 6,
	LINUX_SYSCALL_POLL = 7,
	LINUX_SYSCALL_LSEEK = 8,
	LINUX_SYSCALL_MMAP = 9,
	LINUX_SYSCALL_MPROTECT = 10,
	LINUX_SYSCALL_MUNMAP = 11,
	LINUX_SYSCALL_BRK = 12,
	LINUX_SYSCALL_RT_SIGACTION = 13,
	LINUX_SYSCALL_RT_SIGPROCMASK = 14,
	LINUX_SYSCALL_RT_SIGTIMEDWAIT = 128,
	LINUX_SYSCALL_IOCTL = 16,
	LINUX_SYSCALL_WRITEV = 20,
	LINUX_SYSCALL_ACCESS = 21,
	LINUX_SYSCALL_PIPE = 22,
	LINUX_SYSCALL_NANOSLEEP = 35,
	LINUX_SYSCALL_DUP = 32,
	LINUX_SYSCALL_DUP2 = 33,
	LINUX_SYSCALL_TRUNCATE = 76,
	LINUX_SYSCALL_FTRUNCATE = 77,
	LINUX_SYSCALL_RMDIR = 84,
	LINUX_SYSCALL_UNLINK = 87,
	LINUX_SYSCALL_SENDFILE = 40,
	LINUX_SYSCALL_SOCKET = 41,
	LINUX_SYSCALL_CONNECT = 42,
	LINUX_SYSCALL_SENDTO = 44,
	LINUX_SYSCALL_RECVFROM = 45,
	LINUX_SYSCALL_FCNTL = 72,
	LINUX_SYSCALL_GETCWD = 79,
	LINUX_SYSCALL_CHDIR = 80,
	LINUX_SYSCALL_MKDIR = 83,
	LINUX_SYSCALL_CHMOD = 90,
	LINUX_SYSCALL_FCHMOD = 91,
	LINUX_SYSCALL_CHOWN = 92,
	LINUX_SYSCALL_FCHOWN = 93,
	LINUX_SYSCALL_LCHOWN = 94,
	LINUX_SYSCALL_READLINK = 89,
	LINUX_SYSCALL_GETTIMEOFDAY = 96,
	LINUX_SYSCALL_UMASK = 95,
	LINUX_SYSCALL_SYSINFO = 99,
	LINUX_SYSCALL_GETUID = 102,
	LINUX_SYSCALL_SYSLOG = 103,
	LINUX_SYSCALL_GETGID = 104,
	LINUX_SYSCALL_SETUID = 105,
	LINUX_SYSCALL_SETGID = 106,
	LINUX_SYSCALL_GETEUID = 107,
	LINUX_SYSCALL_GETEGID = 108,
	LINUX_SYSCALL_GETPPID = 110,
	LINUX_SYSCALL_GETPGRP = 111,
	LINUX_SYSCALL_CLONE = 56,
	LINUX_SYSCALL_FORK = 57,
	LINUX_SYSCALL_VFORK = 58,
	LINUX_SYSCALL_KILL = 62,
	LINUX_SYSCALL_EXIT = 60,
	LINUX_SYSCALL_UNAME = 63,
	LINUX_SYSCALL_SETPGID = 109,
	LINUX_SYSCALL_SETSID = 112,
	LINUX_SYSCALL_GETSID = 124,
	LINUX_SYSCALL_STATFS = 137,
	LINUX_SYSCALL_FSTATFS = 138,
	LINUX_SYSCALL_GETGROUPS = 115,
	LINUX_SYSCALL_SETGROUPS = 116,
	LINUX_SYSCALL_SETRESUID = 117,
	LINUX_SYSCALL_SETRESGID = 119,
	LINUX_SYSCALL_GETPGID = 121,
	LINUX_SYSCALL_ARCH_PRCTL = 158,
	LINUX_SYSCALL_MOUNT = 165,
	LINUX_SYSCALL_UMOUNT2 = 166,
	LINUX_SYSCALL_REBOOT = 169,
	LINUX_SYSCALL_TIME = 201,
	LINUX_SYSCALL_FUTEX = 202,
	LINUX_SYSCALL_SET_TID_ADDRESS = 218,
	LINUX_SYSCALL_CLOCK_GETTIME = 228,
	LINUX_SYSCALL_CLOCK_NANOSLEEP = 230,
	LINUX_SYSCALL_EXECVE = 59,
	LINUX_SYSCALL_GETPID = 39,
	LINUX_SYSCALL_WAIT4 = 61,
	LINUX_SYSCALL_GETTID = 186,
	LINUX_SYSCALL_GETDENTS64 = 217,
	LINUX_SYSCALL_NEWFSTATAT = 262,
	LINUX_SYSCALL_READLINKAT = 267,
	LINUX_SYSCALL_FCHMODAT = 268,
	LINUX_SYSCALL_FACCESSAT = 269,
	LINUX_AT_SYMLINK_NOFOLLOW = 0x100,
	LINUX_SYSCALL_PPOLL = 271,
	LINUX_SYSCALL_SET_ROBUST_LIST = 273,
	LINUX_SYSCALL_DUP3 = 292,
	LINUX_SYSCALL_PIPE2 = 293,
	LINUX_SYSCALL_OPENAT = 257,
	LINUX_SYSCALL_MKDIRAT = 258,
	LINUX_SYSCALL_FCHOWNAT = 260,
	LINUX_SYSCALL_UNLINKAT = 263,
	LINUX_SYSCALL_PRLIMIT64 = 302,
	LINUX_SYSCALL_GETRANDOM = 318,
	LINUX_SYSCALL_STATX = 332,
	LINUX_SYSCALL_EXIT_GROUP = 231,
	LINUX_SYSCALL_FACCESSAT2 = 439,
	LINUX_CLONE_VFORK = 0x00004000,
	LINUX_ENOENT = 2,
	LINUX_EIO = 5,
	LINUX_E2BIG = 7,
	LINUX_EBADF = 9,
	LINUX_EAGAIN = 11,
	LINUX_ENOMEM = 12,
	LINUX_EFAULT = 14,
	LINUX_EINVAL = 22,
	LINUX_EACCES = 13,
	LINUX_ENOTDIR = 20,
	LINUX_EISDIR = 21,
	LINUX_ENAMETOOLONG = 36,
	LINUX_ENOSYS = 38,
	LINUX_ENOTTY = 25,
	LINUX_CLOCK_REALTIME = 0,
	LINUX_CLOCK_MONOTONIC = 1,
	LINUX_TIMER_ABSTIME = 1,
	LINUX_POLLIN = 0x0001,
	LINUX_POLLOUT = 0x0004,
	LINUX_ARCH_SET_FS = 0x1002,
	LINUX_FUTEX_WAIT = 0,
	LINUX_FUTEX_WAKE = 1,
	LINUX_TCGETS = 0x5401,
	LINUX_TCSETS = 0x5402,
	LINUX_TCSETSW = 0x5403,
	LINUX_TCSETSF = 0x5404,
	LINUX_TIOCGPGRP = 0x540f,
	LINUX_TIOCSPGRP = 0x5410,
	LINUX_TIOCGWINSZ = 0x5413,
	LINUX_TERMIOS_SIZE = 60,
	ARCH_USER_MAX_CPUS = 8,
	LINUX_MAX_SYSCALL_BUFFER = 65536,
	LINUX_MAX_SOCKADDR = 128,
	LINUX_EXEC_MAX_PATH = 4096,
	LINUX_EXEC_MAX_STRING = 4096,
	LINUX_EXEC_MAX_STRING_BYTES = 128 * 1024,
	LINUX_MAX_GROUPS = 64,
	LINUX_EXEC_DYN_LOAD_BIAS = 0x400000,
	LINUX_EXEC_INTERP_LOAD_BIAS = 0x600000,
	LINUX_EXEC_STACK_TOP = 0x800000,
	LINUX_EXEC_STACK_PAGES = 64,
	LINUX_STAT_SIZE = 144,
	LINUX_STATX_SIZE = 256,
	LINUX_STATFS_SIZE = 120,
	LINUX_WAIT_STATUS_SIZE = 4,
	LINUX_TIMEVAL_SIZE = 16,
	LINUX_TIMESPEC_SIZE = 16,
	LINUX_TIME_T_SIZE = 8,
	LINUX_SYSINFO_SIZE = 112,
	LINUX_UTSNAME_SIZE = 65 * 6,
	LINUX_S_IFDIR = 0040000,
	LINUX_S_IFLNK = 0120000,
	LINUX_S_IFREG = 0100000,
	LINUX_INITIAL_BRK = 0x900000,
	LINUX_MAX_BRK = 0x10000000,
	LINUX_MAP_FIXED_MIN = 0x10000,
	LINUX_MMAP_BASE = 0x10000000,
	LINUX_MMAP_LIMIT = 0x20000000,
	LINUX_MAX_MMAP_SIZE = 0x1000000,
	LINUX_EXEC_MAX_POINTERS = LINUX_EXEC_MAX_STRING_BYTES / 8,
	LINUX_PROT_READ = 0x1,
	LINUX_PROT_WRITE = 0x2,
	LINUX_PROT_EXEC = 0x4,
	LINUX_MAP_PRIVATE = 0x2,
	LINUX_MAP_FIXED = 0x10,
	LINUX_MAP_ANONYMOUS = 0x20,
	USER_IPC_WORDS = 4,
	USER_FOURCC_CONS = ('C') | ('O' << 8) | ('N' << 16) | ('S' << 24),
	USER_FOURCC_LINX = ('L') | ('I' << 8) | ('N' << 16) | ('X' << 24),
	USER_FOURCC_PROC = ('P') | ('R' << 8) | ('O' << 16) | ('C' << 24),
	USER_FOURCC_VFS = ('V') | ('F' << 8) | ('S' << 16) | ('0' << 24),
	USER_CONSOLE_WRITE = 1,
	USER_CONSOLE_LOG = 2,
	USER_CONSOLE_LOGS_TO_RING = 3,
	USER_VFS_READ_FILE_BUFFER = 6,
	USER_VFS_CLOSE = 7,
	USER_VFS_OPEN_BUFFER = 12,
	USER_VFS_TYPE_REGULAR = 1,
	USER_VFS_TYPE_DIRECTORY = 2,
	USER_VFS_TYPE_SYMLINK = 3,
	USER_VFS_ERR_NOENT = 1,
	USER_VFS_ERR_ACCESS = 2,
	USER_VFS_ERR_NOTDIR = 3,
	USER_VFS_ERR_ISDIR = 4,
	LINUX_RPC_REGISTER_PROCESS = 1000,
	LINUX_RPC_FORK_PROCESS = 1001,
	LINUX_RPC_EXEC_PROCESS = 1002,
	USER_PROC_SET_CMDLINE_BUFFER = 14,
};

enum {
	ELF_MAGIC0 = 0x7f,
	ELF_MAGIC1 = 'E',
	ELF_MAGIC2 = 'L',
	ELF_MAGIC3 = 'F',
	ELF_CLASS_64 = 2,
	ELF_DATA_LSB = 1,
	ELF_TYPE_EXEC = 2,
	ELF_TYPE_DYN = 3,
	ELF_MACHINE_X86_64 = 62,
	ELF_PH_LOAD = 1,
	ELF_PH_DYNAMIC = 2,
	ELF_PH_INTERP = 3,
	ELF_PF_X = 1,
	ELF_PF_W = 2,
	ELF_EHDR_SIZE = 64,
	ELF_PHDR_SIZE = 56,
	ELF_DT_NULL = 0,
	ELF_DT_RELR = 0x24,
	ELF_DT_RELRSZ = 0x23,
	ELF_DT_RELRENT = 0x25,
	LINUX_AT_NULL = 0,
	LINUX_AT_PHDR = 3,
	LINUX_AT_PHENT = 4,
	LINUX_AT_PHNUM = 5,
	LINUX_AT_PAGESZ = 6,
	LINUX_AT_BASE = 7,
	LINUX_AT_ENTRY = 9,
	LINUX_AT_CLKTCK = 17,
	LINUX_AT_RANDOM = 25,
	LINUX_AT_EXECFN = 31,
	BUNIX_AT_STDOUT = 0x62780101,
	BUNIX_AT_STDERR = 0x62780102,
	BUNIX_AT_TIME = 0x62780103,
	BUNIX_AT_PROC = 0x62780104,
	BUNIX_AT_NAMES = 0x62780106,
	LINUX_EXEC_HANDLE_STDOUT = 2,
	LINUX_EXEC_HANDLE_STDERR = LINUX_EXEC_HANDLE_STDOUT,
	LINUX_EXEC_HANDLE_TIME = 3,
	LINUX_EXEC_HANDLE_PROC = 4,
	LINUX_EXEC_HANDLE_NAMES = 5,
};

struct elf64_ehdr {
	u8 ident[16];
	u16 type;
	u16 machine;
	u32 version;
	u64 entry;
	u64 phoff;
	u64 shoff;
	u32 flags;
	u16 ehsize;
	u16 phentsize;
	u16 phnum;
	u16 shentsize;
	u16 shnum;
	u16 shstrndx;
} __attribute__((packed));

struct elf64_phdr {
	u32 type;
	u32 flags;
	u64 offset;
	u64 vaddr;
	u64 paddr;
	u64 filesz;
	u64 memsz;
	u64 align;
} __attribute__((packed));

struct elf64_dyn {
	i64 tag;
	u64 value;
} __attribute__((packed));

struct linux_exec_args {
	u64 argc;
	u64 envc;
	u64 bytes;
	char **argv;
	char **envp;
};

struct linux_vfork_wait {
	u32 child_task;
	u32 done;
	struct linux_vfork_wait *next;
};

static u8 syscall_copy_buffer[LINUX_MAX_SYSCALL_BUFFER];
static u8 console_input_buffer[LINUX_MAX_SYSCALL_BUFFER];
static struct spinlock syscall_copy_lock = SPINLOCK_INIT("syscall-copy");
static struct spinlock linux_vfork_lock = SPINLOCK_INIT("linux-vfork");
static struct linux_vfork_wait *linux_vfork_waiters;
static int str_eq(const char *left, const char *right);
struct user_ipc_message {
	u32 protocol;
	u32 type;
	u32 sender;
	u32 cap_rights;
	u64 reply;
	u64 cap;
	u64 words[USER_IPC_WORDS];
};

static int user_message_to_ipc(const struct user_ipc_message *user_message,
			       struct ipc_message *message)
{
	message->protocol = user_message->protocol;
	message->type = user_message->type;
	message->sender = 0;
	message->cap_rights = user_message->cap_rights;
	message->reply_port = task_port_from_handle(task_current(),
						    user_message->reply,
						    TASK_RIGHT_SEND);
	ipc_port_retain(message->reply_port);
	message->cap_type = IPC_CAP_NONE;
	message->cap_object = 0;
	if (user_message->cap == 0 && user_message->cap_rights != 0) {
		return -1;
	}

	if (user_message->cap != 0) {
		const u32 valid_rights = TASK_RIGHT_SEND | TASK_RIGHT_RECV |
					 TASK_RIGHT_DUP;
		const u32 rights = user_message->cap_rights | TASK_RIGHT_DUP;

		if ((user_message->cap_rights & ~valid_rights) != 0) {
			return -1;
		}

		enum task_cap_type type = TASK_CAP_NONE;
		void *object = 0;

		if (task_export_cap(task_current(), user_message->cap, rights,
				    &type, &object) != 0 ||
		    user_message->cap_rights == 0) {
			return -1;
		}

		message->cap_type = type == TASK_CAP_PORT ?
			IPC_CAP_PORT : IPC_CAP_BUFFER;
		message->cap_object = object;
		if (message->cap_type == IPC_CAP_PORT) {
			ipc_port_retain((struct ipc_port *)message->cap_object);
		} else {
			buffer_retain((struct shared_buffer *)message->cap_object);
		}
	}

	for (u64 i = 0; i < USER_IPC_WORDS; i++) {
		message->words[i] = user_message->words[i];
	}

	return 0;
}

static void ipc_message_to_user(const struct ipc_message *message,
				struct user_ipc_message *user_message)
{
	user_message->protocol = message->protocol;
	user_message->type = message->type;
	user_message->sender = message->sender;
	user_message->cap_rights = message->cap_rights;
	user_message->reply = task_grant_port(task_current(), message->reply_port,
					      TASK_RIGHT_SEND);
	user_message->cap = 0;
	if (message->cap_type == IPC_CAP_PORT) {
		user_message->cap =
			task_grant_port(task_current(),
					(struct ipc_port *)message->cap_object,
					message->cap_rights);
	} else if (message->cap_type == IPC_CAP_BUFFER) {
		user_message->cap =
			task_grant_buffer(task_current(),
					  (struct shared_buffer *)message->cap_object,
					  message->cap_rights);
	}
	for (u64 i = 0; i < USER_IPC_WORDS; i++) {
		user_message->words[i] = message->words[i];
	}
}

static u64 align_down(u64 value, u64 align)
{
	return value & ~(align - 1);
}

static u64 align_up(u64 value, u64 align)
{
	return align_down(value + align - 1, align);
}

static u32 linux_prot_to_task(u64 prot)
{
	u32 task_prot = 0;

	if ((prot & LINUX_PROT_READ) != 0) {
		task_prot |= TASK_VM_PROT_READ;
	}
	if ((prot & LINUX_PROT_WRITE) != 0) {
		task_prot |= TASK_VM_PROT_WRITE;
	}
	if ((prot & LINUX_PROT_EXEC) != 0) {
		task_prot |= TASK_VM_PROT_EXEC;
	}
	return task_prot;
}

static u32 linux_map_flags_to_task(u64 flags)
{
	u32 task_flags = 0;

	if ((flags & LINUX_MAP_PRIVATE) != 0) {
		task_flags |= TASK_VM_MAP_PRIVATE;
	}
	if ((flags & LINUX_MAP_ANONYMOUS) != 0) {
		task_flags |= TASK_VM_MAP_ANONYMOUS;
	}
	if ((flags & LINUX_MAP_FIXED) != 0) {
		task_flags |= TASK_VM_MAP_FIXED;
	}
	return task_flags;
}

static u64 min_u64(u64 left, u64 right)
{
	return left < right ? left : right;
}

static u64 max_u64(u64 left, u64 right)
{
	return left > right ? left : right;
}

static u64 read_u64_le(const u8 *data)
{
	u64 value = 0;

	for (u64 i = 0; i < sizeof(value); i++) {
		value |= ((u64)data[i]) << (i * 8);
	}

	return value;
}

static void write_u64_le(u8 *data, u64 value)
{
	for (u64 i = 0; i < sizeof(value); i++) {
		data[i] = (u8)((value >> (i * 8)) & 0xff);
	}
}

static void write_u32_le(u8 *data, u32 value)
{
	for (u64 i = 0; i < sizeof(value); i++) {
		data[i] = (u8)((value >> (i * 8)) & 0xff);
	}
}

static void mem_copy(u8 *dst, const u8 *src, u64 len)
{
	for (u64 i = 0; i < len; i++) {
		dst[i] = src[i];
	}
}

static void mem_zero(u8 *dst, u64 len)
{
	for (u64 i = 0; i < len; i++) {
		dst[i] = 0;
	}
}

static int copy_cstr_from_user_errno(char *dst, const char *src, u64 max_len)
{
	if (dst == 0 || src == 0 || max_len == 0) {
		return -LINUX_EFAULT;
	}

	for (u64 i = 0; i < max_len; i++) {
		if (vm_read_user(task_vm_space(task_current()), (u64)src + i,
				 &dst[i], 1) != 0) {
			dst[i] = '\0';
			return -LINUX_EFAULT;
		}
		if (dst[i] == '\0') {
			return 0;
		}
	}

	dst[max_len - 1] = '\0';
	return -LINUX_ENAMETOOLONG;
}

static int copy_cstr_from_user(char *dst, const char *src, u64 max_len)
{
	return copy_cstr_from_user_errno(dst, src, max_len) == 0 ? 0 : -1;
}

static u64 str_len(const char *value)
{
	u64 len = 0;

	while (value[len] != '\0') {
		len++;
	}

	return len;
}

static int read_current_user(u64 vaddr, void *dst, u64 len)
{
	return vm_read_user(task_vm_space(task_current()), vaddr, dst, len);
}

static int write_current_user(u64 vaddr, const void *src, u64 len)
{
	return vm_write_user(task_vm_space(task_current()), vaddr, src, len);
}

static int linux_write_stat_user(u64 addr, u64 size, u64 type_mode,
				 u64 owner)
{
	u8 stat[LINUX_STAT_SIZE];
	const u64 mode = type_mode & 0xffffffff;
	const u64 type = type_mode >> 32;
	const u64 linux_type = type == USER_VFS_TYPE_DIRECTORY ?
			       LINUX_S_IFDIR :
			       (type == USER_VFS_TYPE_SYMLINK ?
				LINUX_S_IFLNK : LINUX_S_IFREG);

	mem_zero(stat, sizeof(stat));
	write_u64_le(stat + 0, 1);
	write_u64_le(stat + 8, 1);
	write_u64_le(stat + 16, 1);
	write_u32_le(stat + 24, (u32)(linux_type | mode));
	write_u32_le(stat + 28, (u32)(owner & 0xffffffff));
	write_u32_le(stat + 32, (u32)(owner >> 32));
	write_u64_le(stat + 48, size);
	write_u64_le(stat + 56, 4096);
	write_u64_le(stat + 64, (size + 511) / 512);
	return write_current_user(addr, stat, sizeof(stat));
}

static int linux_timespec_to_ns(u64 vaddr, u64 *ns)
{
	u64 timespec[2];
	const u64 nsec_per_sec = 1000000000ull;

	if (ns == 0) {
		return -LINUX_EINVAL;
	}
	if (vaddr == 0 ||
	    read_current_user(vaddr, timespec, sizeof(timespec)) != 0) {
		return -LINUX_EFAULT;
	}
	if (timespec[1] >= nsec_per_sec ||
	    timespec[0] > (~0ull - timespec[1]) / nsec_per_sec) {
		return -LINUX_EINVAL;
	}

	*ns = timespec[0] * nsec_per_sec + timespec[1];
	return 0;
}

static u64 linux_sleep_relative(u64 request, u64 remainder)
{
	u64 ns;
	const int rc = linux_timespec_to_ns(request, &ns);

	if (rc != 0) {
		return (u64)rc;
	}
	if (remainder != 0) {
		u64 zero[2] = { 0, 0 };

		if (write_current_user(remainder, zero, sizeof(zero)) != 0) {
			return (u64)-LINUX_EFAULT;
		}
	}
	thread_sleep_ns(ns);
	return 0;
}

static u64 linux_sleep_absolute(u64 request)
{
	u64 target;
	const u64 now = timer_monotonic_ns();
	const int rc = linux_timespec_to_ns(request, &target);

	if (rc != 0) {
		return (u64)rc;
	}
	if (target > now) {
		thread_sleep_ns(target - now);
	}
	return 0;
}

static u64 linux_poll_timeout_ns(u64 number, u64 timeout)
{
	if (number == LINUX_SYSCALL_PPOLL) {
		u64 ns;

		if (timeout == 0) {
			return 1000000000ull;
		}
		return linux_timespec_to_ns(timeout, &ns) == 0 ? ns : 0;
	}

	{
		const u32 raw = (u32)timeout;

		if ((raw & 0x80000000u) != 0) {
			return 1000000000ull;
		}
		return (u64)raw * 1000000ull;
	}
}

static int linux_poll_timeout_is_infinite(u64 number, u64 timeout)
{
	if (number == LINUX_SYSCALL_PPOLL) {
		return timeout == 0;
	}
	return ((u32)timeout & 0x80000000u) != 0;
}

static short linux_poll_revents(int fd, short events, int suppress_pollin)
{
	short revents = 0;

	if (fd < 0) {
		return 0;
	}
	if ((events & LINUX_POLLOUT) != 0) {
		revents |= events & LINUX_POLLOUT;
	}
	if ((events & LINUX_POLLIN) != 0) {
		if (fd == 0) {
			if (console_can_read()) {
				revents |= events & LINUX_POLLIN;
			}
		} else if (!suppress_pollin) {
			revents |= events & LINUX_POLLIN;
		}
	}

	return revents;
}

static int console_write_user(u64 vaddr, u64 len)
{
	enum {
		CONSOLE_COPY_CHUNK = 128,
	};

	u8 buffer[CONSOLE_COPY_CHUNK];
	u64 done = 0;

	if (vaddr == 0) {
		return -1;
	}

	while (done < len) {
		const u64 chunk = min_u64(len - done, CONSOLE_COPY_CHUNK);

		if (read_current_user(vaddr + done, buffer, chunk) != 0) {
			return -1;
		}
		console_write_len((const char *)buffer, chunk);
		done += chunk;
	}

	return 0;
}

static int console_log_user(u64 vaddr, u64 len)
{
	enum {
		CONSOLE_COPY_CHUNK = 128,
	};

	u8 buffer[CONSOLE_COPY_CHUNK];
	u64 done = 0;

	if (vaddr == 0) {
		return -1;
	}

	while (done < len) {
		const u64 chunk = min_u64(len - done, CONSOLE_COPY_CHUNK);

		if (read_current_user(vaddr + done, buffer, chunk) != 0) {
			return -1;
		}
		console_log_write_len((const char *)buffer, chunk);
		done += chunk;
	}

	return 0;
}

static int linux_vfs_close(struct task *task, u64 file)
{
	struct ipc_port *vfs = ipc_port_find("vfs");
	struct ipc_port *reply_port = task_reply_port(task);
	struct ipc_message request = {
		.protocol = USER_FOURCC_VFS,
		.type = USER_VFS_CLOSE,
		.sender = 0,
		.cap_rights = 0,
		.reply_port = reply_port,
		.cap_type = IPC_CAP_NONE,
		.cap_object = 0,
		.words = { file, 0, 0, 0 },
	};
	struct ipc_message reply;

	if (vfs == 0 || reply_port == 0 ||
	    ipc_send(vfs, &request) != 0 ||
	    ipc_recv(reply_port, &reply) != 0 ||
	    reply.words[0] != 0) {
		return -1;
	}

	return 0;
}

static int linux_vfs_status_to_errno(u64 status)
{
	if (status == USER_VFS_ERR_NOENT) {
		return -LINUX_ENOENT;
	}
	if (status == USER_VFS_ERR_ACCESS) {
		return -LINUX_EACCES;
	}
	if (status == USER_VFS_ERR_NOTDIR) {
		return -LINUX_ENOTDIR;
	}
	if (status == USER_VFS_ERR_ISDIR) {
		return -LINUX_EISDIR;
	}
	return status == 0 ? 0 : -LINUX_EINVAL;
}

static int linux_vfs_read_file(struct task *task, const char *path,
			       u8 **image_out, u64 image_cap,
			       u64 *image_size)
{
	struct ipc_port *vfs = ipc_port_find("vfs");
	struct ipc_port *reply_port = task_reply_port(task);
	struct ipc_message request;
	struct ipc_message reply;
	u64 file;
	u64 size;
	struct shared_buffer *buffer;
	const u64 path_len = str_len(path) + 1;
	const char cwd[] = "/";
	const u64 cwd_len = sizeof(cwd);
	struct shared_buffer *path_buffer;
	u8 *image;

	if (vfs == 0 || reply_port == 0 || path == 0 || image_out == 0 ||
	    image_size == 0) {
		return -1;
	}
	*image_out = 0;

	path_buffer = buffer_create(cwd_len + path_len);
	if (path_buffer == 0 ||
	    buffer_write(path_buffer, 0, cwd, cwd_len) != 0 ||
	    buffer_write(path_buffer, cwd_len, path, path_len) != 0) {
		buffer_release(path_buffer);
		return -1;
	}
	request = (struct ipc_message){
		.protocol = USER_FOURCC_VFS,
		.type = USER_VFS_OPEN_BUFFER,
		.sender = 0,
		.cap_rights = TASK_RIGHT_RECV,
		.reply_port = reply_port,
		.cap_type = IPC_CAP_BUFFER,
		.cap_object = path_buffer,
		.words = { cwd_len, path_len, 0, 0 },
	};
	if (ipc_send(vfs, &request) != 0 ||
	    ipc_recv(reply_port, &reply) != 0) {
		buffer_release(path_buffer);
		return -1;
	}
	if (reply.words[0] != 0) {
		const int err = linux_vfs_status_to_errno(reply.words[0]);

		buffer_release(path_buffer);
		return err;
	}
	if (reply.words[3] != USER_VFS_TYPE_REGULAR) {
		buffer_release(path_buffer);
		return -LINUX_EACCES;
	}
	buffer_release(path_buffer);

	file = reply.words[1];
	size = reply.words[2];
	if (size == 0 || (image_cap != 0 && size > image_cap)) {
		(void)linux_vfs_close(task, file);
		return -1;
	}
	image = (u8 *)slab_alloc(size);
	if (image == 0) {
		(void)linux_vfs_close(task, file);
		return -2;
	}

	buffer = buffer_create(LINUX_MAX_SYSCALL_BUFFER);
	if (buffer == 0) {
		slab_free(image);
		(void)linux_vfs_close(task, file);
		return -2;
	}

	for (u64 offset = 0; offset < size;) {
		const u64 chunk = min_u64(size - offset,
					  LINUX_MAX_SYSCALL_BUFFER);

		request = (struct ipc_message){
			.protocol = USER_FOURCC_VFS,
			.type = USER_VFS_READ_FILE_BUFFER,
			.sender = 0,
			.cap_rights = TASK_RIGHT_SEND | TASK_RIGHT_DUP,
			.reply_port = reply_port,
			.cap_type = IPC_CAP_BUFFER,
			.cap_object = buffer,
			.words = { file, offset, chunk, 0 },
		};
		if (ipc_send(vfs, &request) != 0 ||
		    ipc_recv(reply_port, &reply) != 0 ||
		    reply.words[0] != 0 ||
		    reply.words[1] != chunk ||
		    buffer_read(buffer, 0, image + offset, chunk) != 0) {
			buffer_release(buffer);
			slab_free(image);
			(void)linux_vfs_close(task, file);
			return -1;
		}
		offset += chunk;
	}

	buffer_release(buffer);
	if (linux_vfs_close(task, file) != 0) {
		slab_free(image);
		return -1;
	}

	*image_out = image;
	*image_size = size;
	return 0;
}

static int linux_mmap_file_into_task(struct task *task, struct ipc_port *linux,
				     struct ipc_port *reply_port, u64 base,
				     u64 len, u64 fd, u64 offset)
{
	struct shared_buffer *buffer;
	u64 flags;

	if (linux == 0 || reply_port == 0 || len == 0 ||
	    (offset & (VM_PAGE_SIZE - 1)) != 0) {
		return -1;
	}

	buffer = buffer_create(LINUX_MAX_SYSCALL_BUFFER);
	if (buffer == 0) {
		return -1;
	}

	flags = spin_lock_irqsave(&syscall_copy_lock);
	for (u64 done = 0; done < len;) {
		const u64 chunk = min_u64(len - done, LINUX_MAX_SYSCALL_BUFFER);
		struct ipc_message request = {
			.protocol = USER_FOURCC_LINX,
			.type = LINUX_SYSCALL_MMAP,
			.sender = 0,
			.cap_rights = TASK_RIGHT_SEND | TASK_RIGHT_DUP,
			.reply_port = reply_port,
			.cap_type = IPC_CAP_BUFFER,
			.cap_object = buffer,
			.words = { fd, offset + done, chunk, 0 },
		};
		struct ipc_message reply;

		if (ipc_send(linux, &request) != 0 ||
		    ipc_recv(reply_port, &reply) != 0 ||
		    (i64)reply.words[0] < 0 ||
		    reply.words[0] > chunk) {
			spin_unlock_irqrestore(&syscall_copy_lock, flags);
			buffer_release(buffer);
			return -1;
		}
		if (reply.words[0] == 0) {
			break;
		}
		if (buffer_read(buffer, 0, syscall_copy_buffer,
				reply.words[0]) != 0 ||
		    vm_write_user(task_vm_space(task), base + done,
				  syscall_copy_buffer, reply.words[0]) != 0) {
			spin_unlock_irqrestore(&syscall_copy_lock, flags);
			buffer_release(buffer);
			return -1;
		}
		done += reply.words[0];
	}
	spin_unlock_irqrestore(&syscall_copy_lock, flags);

	buffer_release(buffer);
	return 0;
}

static int linux_exec_validate(const u8 *image, u64 image_size,
			       const struct elf64_ehdr **ehdr_out)
{
	const struct elf64_ehdr *ehdr;
	u64 phdr_bytes;

	if (image == 0 || ehdr_out == 0 || image_size < ELF_EHDR_SIZE) {
		return -1;
	}

	ehdr = (const struct elf64_ehdr *)image;
	phdr_bytes = ehdr->phoff + (u64)ehdr->phnum * ehdr->phentsize;
	if (ehdr->phoff > phdr_bytes ||
	    ehdr->ident[0] != ELF_MAGIC0 ||
	    ehdr->ident[1] != ELF_MAGIC1 ||
	    ehdr->ident[2] != ELF_MAGIC2 ||
	    ehdr->ident[3] != ELF_MAGIC3 ||
	    ehdr->ident[4] != ELF_CLASS_64 ||
	    ehdr->ident[5] != ELF_DATA_LSB ||
	    (ehdr->type != ELF_TYPE_EXEC && ehdr->type != ELF_TYPE_DYN) ||
	    ehdr->machine != ELF_MACHINE_X86_64 ||
	    ehdr->ehsize != ELF_EHDR_SIZE ||
	    ehdr->phentsize != ELF_PHDR_SIZE ||
	    ehdr->phnum == 0 ||
	    phdr_bytes > image_size) {
		return -1;
	}

	for (u64 i = 0; i < ehdr->phnum; i++) {
		const struct elf64_phdr *phdr =
			(const struct elf64_phdr *)(image + ehdr->phoff +
						    i * ehdr->phentsize);

		if (phdr->type != ELF_PH_LOAD) {
			continue;
		}
		if (phdr->filesz > phdr->memsz ||
		    phdr->offset + phdr->filesz < phdr->offset ||
		    phdr->offset + phdr->filesz > image_size ||
		    phdr->vaddr + phdr->memsz < phdr->vaddr ||
		    phdr->memsz == 0) {
			return -1;
		}
	}

	*ehdr_out = ehdr;
	return 0;
}

static int linux_exec_vaddr_to_offset(const struct elf64_ehdr *ehdr,
				      u64 vaddr, u64 len, u64 *offset)
{
	if (ehdr == 0 || offset == 0 || vaddr + len < vaddr) {
		return -1;
	}

	for (u64 i = 0; i < ehdr->phnum; i++) {
		const struct elf64_phdr *phdr =
			(const struct elf64_phdr *)((const u8 *)ehdr +
						    ehdr->phoff +
						    i * ehdr->phentsize);

		if (phdr->type == ELF_PH_LOAD &&
		    vaddr >= phdr->vaddr &&
		    vaddr + len <= phdr->vaddr + phdr->filesz) {
			*offset = phdr->offset + (vaddr - phdr->vaddr);
			return 0;
		}
	}

	return -1;
}

static int linux_exec_apply_relr_one(u8 *image,
				     const struct elf64_ehdr *ehdr,
				     u64 load_bias, u64 reloc_vaddr)
{
	u64 offset = 0;

	if (linux_exec_vaddr_to_offset(ehdr, reloc_vaddr, sizeof(u64),
				       &offset) != 0) {
		return -1;
	}

	write_u64_le(image + offset, read_u64_le(image + offset) + load_bias);
	return 0;
}

static int linux_exec_apply_relative_relocations(u8 *image,
						 const struct elf64_ehdr *ehdr,
						 u64 image_size, u64 load_bias)
{
	u64 relr = 0;
	u64 relrsz = 0;
	u64 relrent = 8;

	if (load_bias == 0) {
		return 0;
	}

	for (u64 i = 0; i < ehdr->phnum; i++) {
		const struct elf64_phdr *phdr =
			(const struct elf64_phdr *)(image + ehdr->phoff +
						    i * ehdr->phentsize);

		if (phdr->type != ELF_PH_DYNAMIC) {
			continue;
		}
		if (phdr->filesz % sizeof(struct elf64_dyn) != 0 ||
		    phdr->offset + phdr->filesz < phdr->offset ||
		    phdr->offset + phdr->filesz > image_size) {
			return -1;
		}

		const u64 entries = phdr->filesz / sizeof(struct elf64_dyn);
		const struct elf64_dyn *dyn =
			(const struct elf64_dyn *)(image + phdr->offset);

		for (u64 entry = 0; entry < entries; entry++) {
			if (dyn[entry].tag == ELF_DT_NULL) {
				break;
			}
			if (dyn[entry].tag == ELF_DT_RELR) {
				relr = dyn[entry].value;
			} else if (dyn[entry].tag == ELF_DT_RELRSZ) {
				relrsz = dyn[entry].value;
			} else if (dyn[entry].tag == ELF_DT_RELRENT) {
				relrent = dyn[entry].value;
			}
		}
		break;
	}

	if (relr == 0 || relrsz == 0) {
		return 0;
	}
	if (relrent != sizeof(u64) || relrsz % relrent != 0) {
		return -1;
	}

	u64 relr_offset = 0;
	if (linux_exec_vaddr_to_offset(ehdr, relr, relrsz, &relr_offset) != 0 ||
	    relr_offset + relrsz < relr_offset ||
	    relr_offset + relrsz > image_size) {
		return -1;
	}

	u64 reloc_vaddr = 0;
	const u64 entries = relrsz / relrent;

	for (u64 i = 0; i < entries; i++) {
		const u64 entry = read_u64_le(image + relr_offset + i * relrent);

		if ((entry & 1) == 0) {
			reloc_vaddr = entry;
			if (linux_exec_apply_relr_one(image, ehdr, load_bias,
						      reloc_vaddr) != 0) {
				return -1;
			}
			reloc_vaddr += sizeof(u64);
			continue;
		}

		for (u64 bit = 1; bit < 64; bit++) {
			if ((entry & (1ull << bit)) != 0 &&
			    linux_exec_apply_relr_one(image, ehdr, load_bias,
						      reloc_vaddr) != 0) {
				return -1;
			}
			reloc_vaddr += sizeof(u64);
		}
	}

	return 0;
}

static int linux_exec_interp_path(const u8 *image,
				  const struct elf64_ehdr *ehdr,
				  u64 image_size, char *path, u64 path_size)
{
	if (image == 0 || ehdr == 0 || path == 0 || path_size == 0) {
		return -1;
	}
	path[0] = '\0';
	for (u64 i = 0; i < ehdr->phnum; i++) {
		const struct elf64_phdr *phdr =
			(const struct elf64_phdr *)(image + ehdr->phoff +
						    i * ehdr->phentsize);

		if (phdr->type != ELF_PH_INTERP) {
			continue;
		}
		if (phdr->filesz == 0 || phdr->filesz >= path_size ||
		    phdr->offset + phdr->filesz < phdr->offset ||
		    phdr->offset + phdr->filesz > image_size) {
			return -1;
		}
		mem_copy((u8 *)path, image + phdr->offset, phdr->filesz);
		path[phdr->filesz] = '\0';
		return 1;
	}
	return 0;
}

static int linux_exec_unmap_current(struct task *task)
{
	struct vm_space *space = task_vm_space(task);

	while (task_vm_region_count(task) != 0) {
		const struct task_vm_region *region =
			task_vm_region_at(task, 0);
		const u64 base = region->base;
		const u64 len = region->len;

		if (vm_unmap_user_range(space, base, len) != 0) {
			return -1;
		}
		if (task_remove_vm_region(task, base, len) != 0) {
			return -1;
		}
	}

	task_clear_vm_regions(task);
	task_set_linux_brk(task, LINUX_INITIAL_BRK);
	task_set_linux_mmap_next(task, LINUX_MMAP_BASE);
	return 0;
}

static int linux_unmap_task_range(struct task *task, u64 base, u64 len)
{
	if (vm_unmap_user_range(task_vm_space(task), base, len) != 0) {
		return -1;
	}
	return task_remove_vm_region(task, base, len);
}

static int linux_forward_message(struct ipc_port *linux,
				 struct ipc_port *reply_port,
				 struct ipc_message *request,
				 struct ipc_message *reply)
{
	if (linux == 0 || reply_port == 0 || request == 0 || reply == 0) {
		return -1;
	}

	request->protocol = USER_FOURCC_LINX;
	request->reply_port = reply_port;
	return ipc_send(linux, request) == 0 &&
	       ipc_recv(reply_port, reply) == 0 ? 0 : -1;
}

static u64 linux_forward_words(struct ipc_port *linux,
			       struct ipc_port *reply_port, u64 number,
			       u64 word0, u64 word1, u64 word2)
{
	struct ipc_message request = {
		.protocol = USER_FOURCC_LINX,
		.type = (u32)number,
		.sender = 0,
		.cap_rights = 0,
		.reply_port = reply_port,
		.cap_type = IPC_CAP_NONE,
		.cap_object = 0,
		.words = { word0, word1, word2, 0 },
	};
	struct ipc_message reply;

	return linux_forward_message(linux, reply_port, &request, &reply) == 0 ?
	       reply.words[0] : (u64)-LINUX_ENOSYS;
}

static u64 linux_forward_input_buffer(struct ipc_port *linux,
				      struct ipc_port *reply_port,
				      struct ipc_message *request,
				      u64 user_buffer, u64 len,
				      u32 cap_rights)
{
	struct ipc_message reply;
	struct shared_buffer *buffer;
	u64 flags;

	buffer = buffer_create(len == 0 ? 1 : len);
	if (buffer == 0) {
		return (u64)-LINUX_ENOMEM;
	}
	if (len != 0) {
		flags = spin_lock_irqsave(&syscall_copy_lock);
		if (read_current_user(user_buffer, syscall_copy_buffer, len) != 0 ||
		    buffer_write(buffer, 0, syscall_copy_buffer, len) != 0) {
			spin_unlock_irqrestore(&syscall_copy_lock, flags);
			buffer_release(buffer);
			return (u64)-LINUX_EFAULT;
		}
		spin_unlock_irqrestore(&syscall_copy_lock, flags);
	}
	request->cap_type = IPC_CAP_BUFFER;
	request->cap_rights = cap_rights;
	request->cap_object = buffer;
	if (linux_forward_message(linux, reply_port, request, &reply) != 0) {
		buffer_release(buffer);
		return (u64)-LINUX_ENOSYS;
	}
	buffer_release(buffer);
	return reply.words[0];
}

static int linux_syscall_forwards_scalar(u64 number)
{
	switch (number) {
	case LINUX_SYSCALL_CLOSE:
	case LINUX_SYSCALL_LSEEK:
	case LINUX_SYSCALL_DUP:
	case LINUX_SYSCALL_DUP2:
	case LINUX_SYSCALL_DUP3:
	case LINUX_SYSCALL_FCNTL:
	case LINUX_SYSCALL_GETUID:
	case LINUX_SYSCALL_GETGID:
	case LINUX_SYSCALL_GETEUID:
	case LINUX_SYSCALL_GETEGID:
	case LINUX_SYSCALL_GETPPID:
	case LINUX_SYSCALL_GETPGRP:
	case LINUX_SYSCALL_UMASK:
	case LINUX_SYSCALL_SETSID:
	case LINUX_SYSCALL_SETUID:
	case LINUX_SYSCALL_SETGID:
	case LINUX_SYSCALL_SETRESUID:
	case LINUX_SYSCALL_SETRESGID:
	case LINUX_SYSCALL_GETPGID:
	case LINUX_SYSCALL_GETSID:
	case LINUX_SYSCALL_SETPGID:
	case LINUX_SYSCALL_SOCKET:
	case LINUX_SYSCALL_KILL:
	case LINUX_SYSCALL_REBOOT:
	case LINUX_SYSCALL_FTRUNCATE:
	case LINUX_SYSCALL_FCHMOD:
	case LINUX_SYSCALL_FCHOWN:
		return 1;
	default:
		return 0;
	}
}

static struct linux_vfork_wait *linux_vfork_begin(u32 child_task)
{
	struct linux_vfork_wait *waiter =
		(struct linux_vfork_wait *)slab_zalloc(sizeof(*waiter));

	if (waiter == 0) {
		return 0;
	}
	waiter->child_task = child_task;
	waiter->done = 0;

	const u64 flags = spin_lock_irqsave(&linux_vfork_lock);
	waiter->next = linux_vfork_waiters;
	linux_vfork_waiters = waiter;

	spin_unlock_irqrestore(&linux_vfork_lock, flags);
	return waiter;
}

static void linux_vfork_cancel(struct linux_vfork_wait *waiter)
{
	if (waiter == 0) {
		return;
	}

	const u64 flags = spin_lock_irqsave(&linux_vfork_lock);
	struct linux_vfork_wait **slot = &linux_vfork_waiters;

	while (*slot != 0) {
		if (*slot == waiter) {
			*slot = waiter->next;
			break;
		}
		slot = &(*slot)->next;
	}
	spin_unlock_irqrestore(&linux_vfork_lock, flags);
	slab_free(waiter);
}

static int linux_vfork_is_done(struct linux_vfork_wait *waiter)
{
	int done;
	const u64 flags = spin_lock_irqsave(&linux_vfork_lock);

	done = waiter == 0 || waiter->done != 0;
	spin_unlock_irqrestore(&linux_vfork_lock, flags);
	return done;
}

static void linux_vfork_wait(struct linux_vfork_wait *waiter)
{
	while (!linux_vfork_is_done(waiter)) {
		thread_sleep_ns(1000000ull);
	}
	linux_vfork_cancel(waiter);
}

static void linux_vfork_complete_task(u32 child_task)
{
	const u64 flags = spin_lock_irqsave(&linux_vfork_lock);

	for (struct linux_vfork_wait *waiter = linux_vfork_waiters;
	     waiter != 0; waiter = waiter->next) {
		if (waiter->child_task == child_task) {
			waiter->done = 1;
			break;
		}
	}

	spin_unlock_irqrestore(&linux_vfork_lock, flags);
}

static u64 linux_forward_output_words(struct ipc_port *linux,
				      struct ipc_port *reply_port,
				      struct ipc_message *request,
				      u32 type,
				      void *user_out,
				      u64 size,
				      u32 cap_rights,
				      u64 word0,
				      u64 word2,
				      u64 word3);

static u64 linux_exit_current(u64 status)
{
	struct ipc_port *linux = ipc_port_find("linux");
	struct ipc_port *reply_port = task_reply_port(task_current());
	struct ipc_message request = {
		.protocol = USER_FOURCC_LINX,
		.type = LINUX_SYSCALL_EXIT_GROUP,
		.sender = 0,
		.cap_rights = 0,
		.reply_port = reply_port,
		.cap_type = IPC_CAP_NONE,
		.cap_object = 0,
		.words = { status, 0, 0, 0 },
	};
	struct ipc_message reply;

	(void)linux_forward_message(linux, reply_port, &request, &reply);
	console_printf("linux: exit_group status=%u\n", (u32)status);
	linux_vfork_complete_task(task_id(task_current()));
	__asm__ volatile ("sti");
	thread_exit();
}

static u64 linux_write_one(struct ipc_port *linux, struct ipc_port *reply_port,
			   u64 fd, u64 user_buffer, u64 len)
{
	struct ipc_message request = {
		.protocol = USER_FOURCC_LINX,
		.type = LINUX_SYSCALL_WRITE,
		.sender = 0,
		.cap_rights = 0,
		.reply_port = reply_port,
		.cap_type = IPC_CAP_NONE,
		.cap_object = 0,
		.words = { fd, len, 0, 0 },
	};

	if (len > LINUX_MAX_SYSCALL_BUFFER) {
		return (u64)-LINUX_EINVAL;
	}
	return linux_forward_input_buffer(linux, reply_port, &request,
					  user_buffer, len, TASK_RIGHT_RECV);
}

static u64 linux_write_chunked(struct ipc_port *linux,
			       struct ipc_port *reply_port,
			       u64 fd, u64 user_buffer, u64 len)
{
	u64 total = 0;

	if (user_buffer == 0 && len != 0) {
		return (u64)-LINUX_EFAULT;
	}
	while (total < len) {
		const u64 chunk = min_u64(len - total,
					  LINUX_MAX_SYSCALL_BUFFER);
		const u64 wrote = linux_write_one(linux, reply_port, fd,
						  user_buffer + total,
						  chunk);

		if ((i64)wrote < 0) {
			return total != 0 ? total : wrote;
		}
		if (wrote == 0) {
			return total;
		}
		total += wrote;
		if (wrote != chunk) {
			return total;
		}
	}
	return total;
}

static u64 linux_read_one(struct ipc_port *linux, struct ipc_port *reply_port,
			  u64 fd, u64 user_buffer, u64 len, u32 cap_rights)
{
	struct ipc_message request = {
		.protocol = USER_FOURCC_LINX,
		.type = LINUX_SYSCALL_READ,
		.sender = 0,
		.cap_rights = 0,
		.reply_port = 0,
		.cap_type = IPC_CAP_NONE,
		.cap_object = 0,
		.words = { 0, 0, 0, 0 },
	};

	if (len > LINUX_MAX_SYSCALL_BUFFER) {
		return (u64)-LINUX_EINVAL;
	}
	return linux_forward_output_words(linux, reply_port, &request,
					  LINUX_SYSCALL_READ,
					  (void *)user_buffer, len,
					  cap_rights, fd, 0, 0);
}

static u64 linux_read_chunked(struct ipc_port *linux,
			      struct ipc_port *reply_port,
			      u64 fd, u64 user_buffer, u64 len,
			      u32 cap_rights)
{
	u64 total = 0;

	if (user_buffer == 0 && len != 0) {
		return (u64)-LINUX_EFAULT;
	}
	while (total < len) {
		const u64 chunk = min_u64(len - total,
					  LINUX_MAX_SYSCALL_BUFFER);
		const u64 nread = linux_read_one(linux, reply_port, fd,
						 user_buffer + total, chunk,
						 cap_rights);

		if ((i64)nread < 0) {
			return total != 0 ? total : nread;
		}
		if (nread == 0) {
			return total;
		}
		total += nread;
		if (nread != chunk) {
			return total;
		}
	}
	return total;
}

static u64 linux_getdents64_chunked(struct ipc_port *linux,
				    struct ipc_port *reply_port,
				    struct ipc_message *request, u64 fd,
				    u64 user_buffer, u64 len)
{
	u64 total = 0;

	if (user_buffer == 0 || len == 0) {
		return user_buffer == 0 ? (u64)-LINUX_EFAULT :
		       (u64)-LINUX_EINVAL;
	}
	while (total < len) {
		const u64 chunk = min_u64(len - total,
					  LINUX_MAX_SYSCALL_BUFFER);
		const u64 nread = linux_forward_output_words(
			linux, reply_port, request, LINUX_SYSCALL_GETDENTS64,
			(void *)(user_buffer + total), chunk, TASK_RIGHT_SEND,
			fd, 0, 0);

		if ((i64)nread < 0) {
			return total != 0 ? total : nread;
		}
		if (nread == 0) {
			break;
		}
		total += nread;
	}
	return total;
}

static u64 linux_getrandom_chunked(struct ipc_port *linux,
				   struct ipc_port *reply_port,
				   u64 user_buffer, u64 len)
{
	struct ipc_message request = {
		.protocol = USER_FOURCC_LINX,
		.type = LINUX_SYSCALL_GETRANDOM,
		.sender = 0,
		.cap_rights = 0,
		.reply_port = 0,
		.cap_type = IPC_CAP_NONE,
		.cap_object = 0,
		.words = { 0, 0, 0, 0 },
	};
	u64 total = 0;

	if (user_buffer == 0 && len != 0) {
		return (u64)-LINUX_EFAULT;
	}
	while (total < len) {
		const u64 chunk = min_u64(len - total,
					  LINUX_MAX_SYSCALL_BUFFER);
		const u64 nread = linux_forward_output_words(
			linux, reply_port, &request, LINUX_SYSCALL_GETRANDOM,
			(void *)(user_buffer + total), chunk,
			TASK_RIGHT_SEND, 0, 0, 0);

		if ((i64)nread < 0) {
			return total != 0 ? total : nread;
		}
		if (nread == 0) {
			return total;
		}
		total += nread;
		if (nread != chunk) {
			return total;
		}
	}
	return total;
}

static u64 linux_sendfile_chunked(struct ipc_port *linux,
				  struct ipc_port *reply_port, u64 out_fd,
				  u64 in_fd, u64 offset_addr, u64 count)
{
	u64 total = 0;
	u64 offset = (u64)-1;
	const int positioned = offset_addr != 0;

	if (positioned) {
		if (read_current_user(offset_addr, &offset,
				      sizeof(offset)) != 0) {
			return (u64)-LINUX_EFAULT;
		}
		if ((offset >> 63) != 0) {
			return (u64)-LINUX_EINVAL;
		}
	}
	while (total < count) {
		const u64 chunk = min_u64(count - total,
					  LINUX_MAX_SYSCALL_BUFFER);
		struct shared_buffer *buffer =
			buffer_create(chunk == 0 ? 1 : chunk);
		struct ipc_message request = {
			.protocol = USER_FOURCC_LINX,
			.type = LINUX_SYSCALL_SENDFILE,
			.sender = 0,
			.cap_rights = TASK_RIGHT_SEND | TASK_RIGHT_RECV |
				      TASK_RIGHT_DUP,
			.reply_port = reply_port,
			.cap_type = IPC_CAP_BUFFER,
			.cap_object = buffer,
			.words = { out_fd, in_fd, chunk, offset },
		};
		struct ipc_message reply;
		u64 nwritten;

		if (buffer == 0) {
			return total != 0 ? total : (u64)-LINUX_ENOMEM;
		}
		if (linux_forward_message(linux, reply_port, &request,
					  &reply) != 0) {
			buffer_release(buffer);
			return total != 0 ? total : (u64)-LINUX_ENOSYS;
		}
		buffer_release(buffer);
		if ((i64)reply.words[0] < 0) {
			return total != 0 ? total : reply.words[0];
		}
		nwritten = reply.words[0];
		if (nwritten == 0) {
			break;
		}
		total += nwritten;
		if (positioned) {
			offset = reply.words[1];
		}
		if (nwritten != chunk) {
			break;
		}
	}
	if (positioned &&
	    write_current_user(offset_addr, &offset, sizeof(offset)) != 0) {
		return total != 0 ? total : (u64)-LINUX_EFAULT;
	}
	return total;
}

static struct shared_buffer *linux_path_buffer_from_user(const char *path,
							 u64 min_size,
							 u64 *path_len,
							 u64 *error)
{
	struct shared_buffer *buffer;
	u64 len;
	u64 size;
	int rc;

	if (path_len == 0 || error == 0) {
		return 0;
	}
	if (path == 0) {
		*error = (u64)-LINUX_EFAULT;
		return 0;
	}
	rc = copy_cstr_from_user_errno((char *)syscall_copy_buffer, path,
				       LINUX_MAX_SYSCALL_BUFFER);
	if (rc != 0) {
		*error = (u64)rc;
		return 0;
	}
	len = str_len((const char *)syscall_copy_buffer) + 1;
	size = min_size > len ? min_size : len;
	buffer = buffer_create(size == 0 ? 1 : size);
	if (buffer == 0 ||
	    buffer_write(buffer, 0, syscall_copy_buffer, len) != 0) {
		buffer_release(buffer);
		*error = (u64)-LINUX_ENOMEM;
		return 0;
	}
	*path_len = len;
	return buffer;
}

static u64 linux_forward_path(struct ipc_port *linux,
			      struct ipc_port *reply_port,
			      struct ipc_message *request,
			      struct ipc_message *reply,
			      const char *path,
			      u64 min_buffer_size,
			      u64 path_len_word,
			      u32 cap_rights)
{
	struct shared_buffer *buffer;
	u64 len = 0;
	u64 error = 0;

	buffer = linux_path_buffer_from_user(path, min_buffer_size, &len,
					     &error);
	if (buffer == 0) {
		return error;
	}
	if (path_len_word < 4) {
		request->words[path_len_word] = len;
	}
	request->cap_type = IPC_CAP_BUFFER;
	request->cap_rights = cap_rights;
	request->cap_object = buffer;
	if (linux_forward_message(linux, reply_port, request, reply) != 0) {
		buffer_release(buffer);
		return (u64)-LINUX_ENOSYS;
	}
	buffer_release(buffer);
	return reply->words[0];
}

static u64 linux_forward_path_words(struct ipc_port *linux,
				    struct ipc_port *reply_port,
				    struct ipc_message *request,
				    struct ipc_message *reply,
				    u32 type,
				    const char *path,
				    u64 word0,
				    u64 word2,
				    u64 word3)
{
	request->type = type;
	request->words[0] = word0;
	request->words[1] = 0;
	request->words[2] = word2;
	request->words[3] = word3;
	return linux_forward_path(linux, reply_port, request, reply, path, 0, 1,
				  TASK_RIGHT_RECV);
}

static struct shared_buffer *linux_forward_path_buffer(
	struct ipc_port *linux, struct ipc_port *reply_port,
	struct ipc_message *request, struct ipc_message *reply,
	const char *path, u64 min_buffer_size, u64 path_len_word,
	u32 cap_rights, u64 *result)
{
	struct shared_buffer *buffer;
	u64 len = 0;
	u64 error = 0;

	if (result == 0) {
		return 0;
	}
	buffer = linux_path_buffer_from_user(path, min_buffer_size, &len,
					     &error);
	if (buffer == 0) {
		*result = error;
		return 0;
	}
	if (path_len_word < 4) {
		request->words[path_len_word] = len;
	}
	request->cap_type = IPC_CAP_BUFFER;
	request->cap_rights = cap_rights;
	request->cap_object = buffer;
	if (linux_forward_message(linux, reply_port, request, reply) != 0) {
		buffer_release(buffer);
		*result = (u64)-LINUX_ENOSYS;
		return 0;
	}
	*result = reply->words[0];
	return buffer;
}

static u64 linux_forward_mount(struct ipc_port *linux,
			       struct ipc_port *reply_port,
			       struct ipc_message *request,
			       const char *target, const char *fstype,
			       u64 flags)
{
	struct shared_buffer *buffer;
	struct ipc_message reply;
	u64 target_len;
	u64 fstype_len;
	u64 total_len;
	int rc;

	if (target == 0 || fstype == 0) {
		return (u64)-LINUX_EFAULT;
	}
	rc = copy_cstr_from_user_errno((char *)syscall_copy_buffer, target,
				       LINUX_MAX_SYSCALL_BUFFER);
	if (rc != 0) {
		return (u64)rc;
	}
	target_len = str_len((const char *)syscall_copy_buffer) + 1;
	if (target_len == 1 || target_len >= LINUX_MAX_SYSCALL_BUFFER) {
		return (u64)-LINUX_EINVAL;
	}
	rc = copy_cstr_from_user_errno(
		(char *)syscall_copy_buffer + target_len, fstype,
		LINUX_MAX_SYSCALL_BUFFER - target_len);
	if (rc != 0) {
		return (u64)rc;
	}
	fstype_len = str_len((const char *)syscall_copy_buffer + target_len) + 1;
	total_len = target_len + fstype_len;
	if (fstype_len == 1 || total_len > LINUX_MAX_SYSCALL_BUFFER) {
		return (u64)-LINUX_EINVAL;
	}
	buffer = buffer_create(total_len);
	if (buffer == 0 ||
	    buffer_write(buffer, 0, syscall_copy_buffer, total_len) != 0) {
		buffer_release(buffer);
		return (u64)-LINUX_ENOMEM;
	}
	request->type = LINUX_SYSCALL_MOUNT;
	request->words[0] = target_len;
	request->words[1] = fstype_len;
	request->words[2] = flags;
	request->words[3] = 0;
	request->cap_type = IPC_CAP_BUFFER;
	request->cap_rights = TASK_RIGHT_RECV;
	request->cap_object = buffer;
	if (linux_forward_message(linux, reply_port, request, &reply) != 0) {
		buffer_release(buffer);
		return (u64)-LINUX_ENOSYS;
	}
	buffer_release(buffer);
	return reply.words[0];
}

static u64 linux_forward_output_buffer(struct ipc_port *linux,
				       struct ipc_port *reply_port,
				       struct ipc_message *request,
				       void *user_out, u64 size,
				       u32 cap_rights)
{
	struct shared_buffer *buffer;
	struct ipc_message reply;

	buffer = buffer_create(size == 0 ? 1 : size);
	if (buffer == 0) {
		return (u64)-LINUX_ENOMEM;
	}
	request->cap_type = IPC_CAP_BUFFER;
	request->cap_rights = cap_rights;
	request->cap_object = buffer;
	if (linux_forward_message(linux, reply_port, request, &reply) != 0) {
		buffer_release(buffer);
		return (u64)-LINUX_ENOSYS;
	}
	if ((i64)reply.words[0] > 0) {
		u64 flags;

		if (reply.words[0] > size ||
		    reply.words[0] > LINUX_MAX_SYSCALL_BUFFER) {
			buffer_release(buffer);
			return (u64)-LINUX_EFAULT;
		}
		flags = spin_lock_irqsave(&syscall_copy_lock);
		if (buffer_read(buffer, 0, syscall_copy_buffer,
				reply.words[0]) != 0 ||
		    write_current_user((u64)user_out, syscall_copy_buffer,
				       reply.words[0]) != 0) {
			spin_unlock_irqrestore(&syscall_copy_lock, flags);
			buffer_release(buffer);
			return (u64)-LINUX_EFAULT;
		}
		spin_unlock_irqrestore(&syscall_copy_lock, flags);
	}
	buffer_release(buffer);
	return reply.words[0];
}

static u64 linux_copy_buffer_to_user(struct shared_buffer *buffer,
				     u64 offset, u64 user_out, u64 size)
{
	u64 flags;

	if (size > LINUX_MAX_SYSCALL_BUFFER) {
		return (u64)-LINUX_EFAULT;
	}
	flags = spin_lock_irqsave(&syscall_copy_lock);
	if (buffer_read(buffer, offset, syscall_copy_buffer, size) != 0 ||
	    write_current_user(user_out, syscall_copy_buffer, size) != 0) {
		spin_unlock_irqrestore(&syscall_copy_lock, flags);
		return (u64)-LINUX_EFAULT;
	}
	spin_unlock_irqrestore(&syscall_copy_lock, flags);
	return 0;
}

static u64 linux_forward_output_words(struct ipc_port *linux,
				      struct ipc_port *reply_port,
				      struct ipc_message *request,
				      u32 type,
				      void *user_out,
				      u64 size,
				      u32 cap_rights,
				      u64 word0,
				      u64 word2,
				      u64 word3)
{
	request->type = type;
	request->words[0] = word0;
	request->words[1] = size;
	request->words[2] = word2;
	request->words[3] = word3;
	return linux_forward_output_buffer(linux, reply_port, request, user_out,
					   size, cap_rights);
}

static u64 linux_forward_fixed_output_buffer(struct ipc_port *linux,
					     struct ipc_port *reply_port,
					     struct ipc_message *request,
					     void *user_out, u64 size,
					     int copy_on_zero,
					     int copy_on_positive)
{
	struct shared_buffer *buffer;
	struct ipc_message reply;
	u64 result;

	buffer = buffer_create(size == 0 ? 1 : size);
	if (buffer == 0) {
		return (u64)-LINUX_ENOMEM;
	}
	request->cap_type = IPC_CAP_BUFFER;
	request->cap_rights = TASK_RIGHT_SEND;
	request->cap_object = buffer;
	if (linux_forward_message(linux, reply_port, request, &reply) != 0) {
		buffer_release(buffer);
		return (u64)-LINUX_ENOSYS;
	}
	result = reply.words[0];
	if ((copy_on_zero && result == 0) ||
	    (copy_on_positive && (i64)result > 0)) {
		u64 flags;

		if (size > LINUX_MAX_SYSCALL_BUFFER) {
			buffer_release(buffer);
			return (u64)-LINUX_EFAULT;
		}
		flags = spin_lock_irqsave(&syscall_copy_lock);
		if (buffer_read(buffer, 0, syscall_copy_buffer, size) != 0 ||
		    write_current_user((u64)user_out, syscall_copy_buffer,
				       size) != 0) {
			spin_unlock_irqrestore(&syscall_copy_lock, flags);
			buffer_release(buffer);
			return (u64)-LINUX_EFAULT;
		}
		spin_unlock_irqrestore(&syscall_copy_lock, flags);
	}
	buffer_release(buffer);
	return result;
}

static u64 linux_forward_fixed_output_words(struct ipc_port *linux,
					    struct ipc_port *reply_port,
					    struct ipc_message *request,
					    u32 type, void *user_out,
					    u64 size, int copy_on_zero,
					    int copy_on_positive, u64 word0,
					    u64 word1, u64 word2, u64 word3)
{
	request->type = type;
	request->words[0] = word0;
	request->words[1] = word1;
	request->words[2] = word2;
	request->words[3] = word3;
	return linux_forward_fixed_output_buffer(linux, reply_port, request,
						 user_out, size, copy_on_zero,
						 copy_on_positive);
}

static u64 linux_forward_two_u32_out(struct ipc_port *linux,
				     struct ipc_port *reply_port,
				     struct ipc_message *request,
				     u64 user_out)
{
	struct ipc_message reply;
	u32 values[2];

	if (linux_forward_message(linux, reply_port, request, &reply) != 0) {
		return (u64)-LINUX_ENOSYS;
	}
	if ((i64)reply.words[0] < 0) {
		return reply.words[0];
	}
	values[0] = (u32)reply.words[1];
	values[1] = (u32)reply.words[2];
	return write_current_user(user_out, values, sizeof(values)) == 0 ?
	       reply.words[0] : (u64)-LINUX_EFAULT;
}

static u64 linux_forward_groups_out(struct task *task, struct ipc_port *linux,
				    struct ipc_port *reply_port,
				    struct ipc_message *request,
				    u64 count, u64 user_out)
{
	struct shared_buffer *buffer = 0;
	struct ipc_message reply;
	u64 size;

	if (count > LINUX_MAX_GROUPS) {
		return (u64)-LINUX_EINVAL;
	}
	if (count != 0) {
		size = count * sizeof(u32);
		buffer = buffer_create(size);
		if (buffer == 0) {
			return (u64)-LINUX_ENOMEM;
		}
		request->cap_type = IPC_CAP_BUFFER;
		request->cap_rights = TASK_RIGHT_SEND | TASK_RIGHT_DUP;
		request->cap_object = buffer;
	}
	if (linux_forward_message(linux, reply_port, request, &reply) != 0) {
		if (buffer != 0) {
			buffer_release(buffer);
		}
		return (u64)-LINUX_ENOSYS;
	}
	if ((i64)reply.words[0] > 0 && count != 0) {
		if (reply.words[0] > count ||
		    reply.words[0] > LINUX_MAX_GROUPS) {
			buffer_release(buffer);
			return (u64)-LINUX_EINVAL;
		}
		if (buffer_read(buffer, 0, syscall_copy_buffer,
				reply.words[0] * sizeof(u32)) != 0 ||
		    vm_write_user(task_vm_space(task), user_out,
				  syscall_copy_buffer,
				  reply.words[0] * sizeof(u32)) != 0) {
			buffer_release(buffer);
			return (u64)-LINUX_EFAULT;
		}
	}
	if (buffer != 0) {
		buffer_release(buffer);
	}
	return reply.words[0];
}

static int linux_exec_map_segment(struct task *task, const u8 *image,
				  const struct elf64_phdr *phdr,
				  u64 load_bias)
{
	struct vm_space *space = task_vm_space(task);
	const u32 writable = (phdr->flags & ELF_PF_W) != 0;
	const u64 segment_start = phdr->vaddr + load_bias;
	const u64 segment_end = segment_start + phdr->memsz;
	const u64 page_start = align_down(segment_start, VM_PAGE_SIZE);
	const u64 page_end = align_up(segment_end, VM_PAGE_SIZE);

	for (u64 page = page_start; page < page_end; page += VM_PAGE_SIZE) {
		struct vm_frame frame = vm_alloc_user_page(space, page,
							   writable);
		const u64 page_end_addr = page + VM_PAGE_SIZE;
		const u64 copy_start = max_u64(page, segment_start);
		const u64 copy_end = min_u64(page_end_addr,
					     segment_start + phdr->filesz);

		if (frame.addr == 0) {
			(void)vm_unmap_user_range(space, page_start,
						  page - page_start);
			return -1;
		}
		mem_zero((u8 *)frame.addr, VM_PAGE_SIZE);
		if (copy_start < copy_end) {
			const u64 dst_offset = copy_start - page;
			const u64 src_offset = copy_start - segment_start;

			mem_copy((u8 *)(frame.addr + dst_offset),
				 image + phdr->offset + src_offset,
				 copy_end - copy_start);
		}
	}

	const u32 prot = TASK_VM_PROT_READ |
			 (writable != 0 ? TASK_VM_PROT_WRITE : 0) |
			 ((phdr->flags & ELF_PF_X) != 0 ?
			  TASK_VM_PROT_EXEC : 0);

	if (task_add_vm_mapping(task, page_start, page_end - page_start,
				prot, TASK_VM_MAP_PRIVATE, TASK_VM_REGION_ELF,
				TASK_VM_OBJECT_FILE, 0, phdr->offset) != 0) {
		(void)vm_unmap_user_range(space, page_start,
					  page_end - page_start);
		return -1;
	}

	return 0;
}

static char *linux_copy_cstr_dynamic(const char *src, u64 max_len, u64 *len_out,
				     int *error)
{
	u64 len = 0;
	char *copy;

	if (src == 0 || max_len == 0 || len_out == 0 || error == 0) {
		if (error != 0) {
			*error = -LINUX_EFAULT;
		}
		return 0;
	}
	*error = 0;

	for (;;) {
		char c = '\0';

		if (len >= max_len) {
			*error = -LINUX_E2BIG;
			return 0;
		}
		if (vm_read_user(task_vm_space(task_current()), (u64)src + len,
				 &c, 1) != 0) {
			*error = -LINUX_EFAULT;
			return 0;
		}
		len++;
		if (c == '\0') {
			break;
		}
	}

	copy = (char *)slab_alloc(len);
	if (copy == 0) {
		*error = -LINUX_ENOMEM;
		return 0;
	}
	*error = copy_cstr_from_user_errno(copy, src, len);
	if (*error != 0) {
		slab_free(copy);
		return 0;
	}
	*len_out = len;
	return copy;
}

static void linux_exec_args_free(struct linux_exec_args *args)
{
	if (args == 0) {
		return;
	}
	if (args->argv != 0) {
		for (u64 i = 0; i < args->argc; i++) {
			if (args->argv[i] != 0) {
				slab_free(args->argv[i]);
			}
		}
		slab_free(args->argv);
	}
	if (args->envp != 0) {
		for (u64 i = 0; i < args->envc; i++) {
			if (args->envp[i] != 0) {
				slab_free(args->envp[i]);
			}
		}
		slab_free(args->envp);
	}
	args->argc = 0;
	args->envc = 0;
	args->bytes = 0;
	args->argv = 0;
	args->envp = 0;
}

static int linux_exec_vector_push(char ***values, u64 *count, u64 *capacity,
				  char *value)
{
	if (values == 0 || count == 0 || capacity == 0 || value == 0 ||
	    *count >= LINUX_EXEC_MAX_POINTERS) {
		return -LINUX_E2BIG;
	}

	if (*count == *capacity) {
		u64 new_capacity = *capacity == 0 ? 8 : *capacity * 2;
		char **next;

		if (new_capacity < *capacity ||
		    new_capacity > LINUX_EXEC_MAX_POINTERS) {
			new_capacity = LINUX_EXEC_MAX_POINTERS;
		}
		if (new_capacity == *capacity) {
			return -LINUX_E2BIG;
		}
		next = (char **)slab_zalloc(new_capacity * sizeof(next[0]));
		if (next == 0) {
			return -LINUX_ENOMEM;
		}
		if (*values != 0) {
			mem_copy((u8 *)next, (const u8 *)*values,
				 *count * sizeof(next[0]));
			slab_free(*values);
		}
		*values = next;
		*capacity = new_capacity;
	}

	(*values)[*count] = value;
	(*count)++;
	return 0;
}

static int linux_exec_collect_args(const char *path, u64 user_argv,
				   struct linux_exec_args *args)
{
	u64 argv_capacity = 0;

	if (path == 0 || args == 0) {
		return -LINUX_EFAULT;
	}

	args->argc = 0;
	args->envc = 0;
	args->bytes = 0;
	args->argv = 0;
	args->envp = 0;

	if (user_argv == 0) {
		const u64 len = str_len(path) + 1;

		if (len > LINUX_EXEC_MAX_STRING ||
		    len > LINUX_EXEC_MAX_STRING_BYTES) {
			return -LINUX_E2BIG;
		}
		char *copy = (char *)slab_alloc(len);
		if (copy == 0) {
			return -LINUX_ENOMEM;
		}
		mem_copy((u8 *)copy, (const u8 *)path, len);
		const int push_result =
			linux_exec_vector_push(&args->argv, &args->argc,
					       &argv_capacity, copy);
		if (push_result != 0) {
			slab_free(copy);
			return push_result;
		}
		args->bytes = len;
		return 0;
	}

	for (u64 i = 0; i < LINUX_EXEC_MAX_POINTERS; i++) {
		u64 arg_ptr = 0;
		char *copy;
		u64 len = 0;
		int copy_result;

		if (read_current_user(user_argv + i * sizeof(arg_ptr),
				      &arg_ptr, sizeof(arg_ptr)) != 0) {
			return -LINUX_EFAULT;
		}
		if (arg_ptr == 0) {
			break;
		}
		copy = linux_copy_cstr_dynamic((const char *)arg_ptr,
					       LINUX_EXEC_MAX_STRING, &len,
					       &copy_result);
		if (copy == 0) {
			return copy_result != 0 ? copy_result : -LINUX_ENOMEM;
		}
		if (args->bytes + len > LINUX_EXEC_MAX_STRING_BYTES) {
			if (copy != 0) {
				slab_free(copy);
			}
			return -LINUX_E2BIG;
		}
		const int push_result =
			linux_exec_vector_push(&args->argv, &args->argc,
					       &argv_capacity, copy);
		if (push_result != 0) {
			slab_free(copy);
			return push_result;
		}
		args->bytes += len;
	}
	{
		u64 next_arg = 0;

		if (read_current_user(user_argv + args->argc * sizeof(next_arg),
				      &next_arg, sizeof(next_arg)) != 0 ||
		    next_arg != 0) {
			return next_arg != 0 ? -LINUX_E2BIG : -LINUX_EFAULT;
		}
	}

	if (args->argc == 0) {
		const u64 len = str_len(path) + 1;

		if (len > LINUX_EXEC_MAX_STRING ||
		    args->bytes + len > LINUX_EXEC_MAX_STRING_BYTES) {
			return -LINUX_E2BIG;
		}
		char *copy = (char *)slab_alloc(len);
		if (copy == 0) {
			return -LINUX_ENOMEM;
		}
		mem_copy((u8 *)copy, (const u8 *)path, len);
		const int push_result =
			linux_exec_vector_push(&args->argv, &args->argc,
					       &argv_capacity, copy);
		if (push_result != 0) {
			slab_free(copy);
			return push_result;
		}
		args->bytes += len;
	}

	return 0;
}

static int linux_exec_collect_env(u64 user_envp, struct linux_exec_args *args)
{
	u64 env_capacity = 0;

	if (args == 0) {
		return -LINUX_EFAULT;
	}
	args->envc = 0;
	if (user_envp == 0) {
		return 0;
	}
	for (u64 i = 0; i < LINUX_EXEC_MAX_POINTERS; i++) {
		u64 env_ptr = 0;
		char *copy;
		u64 len = 0;
		int copy_result;

		if (read_current_user(user_envp + i * sizeof(env_ptr),
				      &env_ptr, sizeof(env_ptr)) != 0) {
			return -LINUX_EFAULT;
		}
		if (env_ptr == 0) {
			return 0;
		}
		copy = linux_copy_cstr_dynamic((const char *)env_ptr,
					       LINUX_EXEC_MAX_STRING, &len,
					       &copy_result);
		if (copy == 0) {
			return copy_result != 0 ? copy_result : -LINUX_ENOMEM;
		}
		if (args->bytes + len > LINUX_EXEC_MAX_STRING_BYTES) {
			if (copy != 0) {
				slab_free(copy);
			}
			return -LINUX_E2BIG;
		}
		const int push_result =
			linux_exec_vector_push(&args->envp, &args->envc,
					       &env_capacity, copy);
		if (push_result != 0) {
			slab_free(copy);
			return push_result;
		}
		args->bytes += len;
	}
	{
		u64 next_env = 0;

		if (read_current_user(user_envp + args->envc * sizeof(next_env),
				      &next_env, sizeof(next_env)) != 0 ||
		    next_env != 0) {
			return next_env != 0 ? -LINUX_E2BIG : -LINUX_EFAULT;
		}
	}
	return 0;
}

static int linux_exec_build_stack(const char *path,
				  const struct linux_exec_args *args,
				  const struct elf64_ehdr *ehdr,
				  u64 load_bias, u64 program_entry,
				  u64 interpreter_base,
				  u8 *stack_image, u64 stack_size,
				  u64 *new_sp)
{
	const u64 aux_pairs = interpreter_base == 0 ? 15 : 16;
	const u64 stack_base = LINUX_EXEC_STACK_TOP - stack_size;
	u64 *argv_addrs;
	u64 *env_addrs = 0;
	u64 sp = stack_size;
	int result = -1;

	if (path == 0 || args == 0 || ehdr == 0 || stack_image == 0 ||
	    new_sp == 0 || stack_size == 0 || args->argc == 0) {
		return -1;
	}

	argv_addrs = (u64 *)slab_alloc(args->argc * sizeof(u64));
	env_addrs = args->envc == 0 ? 0 :
		(u64 *)slab_alloc(args->envc * sizeof(u64));
	if (argv_addrs == 0 || (args->envc != 0 && env_addrs == 0)) {
		goto out;
	}
	mem_zero(stack_image, stack_size);

	for (u64 i = args->argc; i > 0; i--) {
		const u64 index = i - 1;
		const u64 len = str_len(args->argv[index]) + 1;

		if (len > LINUX_EXEC_MAX_STRING || sp < len) {
			goto out;
		}
		sp -= len;
		mem_copy(stack_image + sp, (const u8 *)args->argv[index], len);
		argv_addrs[index] = stack_base + sp;
	}
	for (u64 i = args->envc; i > 0; i--) {
		const u64 index = i - 1;
		const u64 len = str_len(args->envp[index]) + 1;

		if (len > LINUX_EXEC_MAX_STRING || sp < len) {
			goto out;
		}
		sp -= len;
		mem_copy(stack_image + sp, (const u8 *)args->envp[index], len);
		env_addrs[index] = stack_base + sp;
	}

	const u64 execfn = argv_addrs[0];
	const u8 random_bytes[16] = {
		0x62, 0x75, 0x6e, 0x69, 0x78, 0x2d, 0x72, 0x61,
		0x6e, 0x64, 0x6f, 0x6d, 0x2d, 0x65, 0x78, 0x00,
	};

	sp = align_down(sp, 16);
	if (sp < sizeof(random_bytes)) {
		goto out;
	}
	sp -= sizeof(random_bytes);
	mem_copy(stack_image + sp, random_bytes, sizeof(random_bytes));
	const u64 random_addr = stack_base + sp;

	const u64 phdr_bytes = ehdr->phnum * sizeof(struct elf64_phdr);
	if (sp < phdr_bytes) {
		goto out;
	}
	sp = align_down(sp - phdr_bytes, 8);
	for (u64 i = 0; i < ehdr->phnum; i++) {
		const struct elf64_phdr *phdr =
			(const struct elf64_phdr *)((const u8 *)ehdr +
						    ehdr->phoff +
						    i * ehdr->phentsize);
		struct elf64_phdr aux_phdr = *phdr;

		aux_phdr.vaddr += load_bias;
		aux_phdr.paddr += load_bias;
		mem_copy(stack_image + sp + i * sizeof(aux_phdr),
			 (const u8 *)&aux_phdr, sizeof(aux_phdr));
	}
	const u64 copied_phdr_addr = stack_base + sp;
	const u64 phdr_addr = interpreter_base != 0 ?
			       load_bias + ehdr->phoff : copied_phdr_addr;

	sp = align_down(sp, 16);
	const u64 words = 1 + args->argc + 1 + args->envc + 1 +
			  aux_pairs * 2;
	if (sp < words * sizeof(u64)) {
		goto out;
	}
	sp -= words * sizeof(u64);
	u64 *stack_words = (u64 *)(stack_image + sp);
	u64 word = 0;

	stack_words[word++] = args->argc;
	for (u64 i = 0; i < args->argc; i++) {
		stack_words[word++] = argv_addrs[i];
	}
	stack_words[word++] = 0;
	for (u64 i = 0; i < args->envc; i++) {
		stack_words[word++] = env_addrs[i];
	}
	stack_words[word++] = 0;
	stack_words[word++] = LINUX_AT_PAGESZ;
	stack_words[word++] = VM_PAGE_SIZE;
	stack_words[word++] = LINUX_AT_ENTRY;
	stack_words[word++] = program_entry;
	if (interpreter_base != 0) {
		stack_words[word++] = LINUX_AT_BASE;
		stack_words[word++] = interpreter_base;
	}
	stack_words[word++] = LINUX_AT_PHDR;
	stack_words[word++] = phdr_addr;
	stack_words[word++] = LINUX_AT_PHENT;
	stack_words[word++] = ehdr->phentsize;
	stack_words[word++] = LINUX_AT_PHNUM;
	stack_words[word++] = ehdr->phnum;
	stack_words[word++] = LINUX_AT_EXECFN;
	stack_words[word++] = execfn;
	stack_words[word++] = LINUX_AT_RANDOM;
	stack_words[word++] = random_addr;
	stack_words[word++] = LINUX_AT_CLKTCK;
	stack_words[word++] = 100;
	stack_words[word++] = BUNIX_AT_STDOUT;
	stack_words[word++] = LINUX_EXEC_HANDLE_STDOUT;
	stack_words[word++] = BUNIX_AT_STDERR;
	stack_words[word++] = LINUX_EXEC_HANDLE_STDERR;
	stack_words[word++] = BUNIX_AT_TIME;
	stack_words[word++] = LINUX_EXEC_HANDLE_TIME;
	stack_words[word++] = BUNIX_AT_PROC;
	stack_words[word++] = LINUX_EXEC_HANDLE_PROC;
	stack_words[word++] = BUNIX_AT_NAMES;
	stack_words[word++] = LINUX_EXEC_HANDLE_NAMES;
	stack_words[word++] = LINUX_AT_NULL;
	stack_words[word++] = 0;

	*new_sp = stack_base + sp;
	result = 0;

out:
	slab_free(argv_addrs);
	slab_free(env_addrs);
	return result;
}

static u64 linux_execve(struct task *task, struct arch_syscall_frame *frame,
			const char *user_path) __attribute__((noinline));
static u64 linux_exec_process(struct task *task);
static void linux_proc_set_cmdline(struct task *task, u64 linux_pid,
				   const char *path);

static u64 linux_execve(struct task *task, struct arch_syscall_frame *frame,
			const char *user_path)
{
	char path[LINUX_EXEC_MAX_PATH];
	struct linux_exec_args args;
	u64 image_size = 0;
	u64 interp_size = 0;
	const struct elf64_ehdr *ehdr;
	const struct elf64_ehdr *interp_ehdr = 0;
	u8 *stack_image = 0;
	u8 *image = 0;
	u8 *interp_image = 0;
	char interp_path[LINUX_EXEC_MAX_PATH];
	u64 new_sp = 0;
	u64 load_bias = 0;
	u64 interp_bias = 0;
	u64 entry = 0;
	u64 program_entry = 0;
	const u64 stack_base =
		LINUX_EXEC_STACK_TOP - LINUX_EXEC_STACK_PAGES * VM_PAGE_SIZE;
	int read_result;
	int path_result;
	int arg_result;
	const u64 stack_size = LINUX_EXEC_STACK_PAGES * VM_PAGE_SIZE;

	path_result = copy_cstr_from_user_errno(path, user_path, sizeof(path));
	if (path_result != 0) {
		return (u64)path_result;
	}
	arg_result = linux_exec_collect_args(path, frame->arg1, &args);
	if (arg_result == 0) {
		arg_result = linux_exec_collect_env(frame->arg2, &args);
	}
	if (arg_result != 0) {
		linux_exec_args_free(&args);
		return (u64)arg_result;
	}

	read_result = linux_vfs_read_file(task, path, &image, 0, &image_size);
	if (read_result != 0) {
		linux_exec_args_free(&args);
		return read_result == -2 ? (u64)-LINUX_ENOMEM :
		       (read_result == -1 ? (u64)-LINUX_EIO :
			(u64)read_result);
	}
	if (linux_exec_validate(image, image_size, &ehdr) != 0) {
		slab_free(image);
		linux_exec_args_free(&args);
		return (u64)-LINUX_EINVAL;
	}

	load_bias = ehdr->type == ELF_TYPE_DYN ? LINUX_EXEC_DYN_LOAD_BIAS : 0;
	program_entry = ehdr->entry + load_bias;
	entry = program_entry;

	const int interp = linux_exec_interp_path(image, ehdr, image_size,
						 interp_path,
						 sizeof(interp_path));
	if (interp < 0) {
		slab_free(image);
		linux_exec_args_free(&args);
		return (u64)-LINUX_EINVAL;
	}
	if (interp > 0) {
		read_result = linux_vfs_read_file(task, interp_path,
						  &interp_image, 0,
						  &interp_size);
		if (read_result != 0) {
			slab_free(image);
			linux_exec_args_free(&args);
			return read_result == -2 ? (u64)-LINUX_ENOMEM :
			       (read_result == -1 ? (u64)-LINUX_EIO :
				(u64)read_result);
		}
		if (linux_exec_validate(interp_image, interp_size,
					&interp_ehdr) != 0) {
			slab_free(interp_image);
			slab_free(image);
			linux_exec_args_free(&args);
			return (u64)-LINUX_EINVAL;
		}
		interp_bias = interp_ehdr->type == ELF_TYPE_DYN ?
			      LINUX_EXEC_INTERP_LOAD_BIAS : 0;
		entry = interp_ehdr->entry + interp_bias;
	}
	stack_image = (u8 *)slab_alloc(stack_size);
	if (stack_image == 0) {
		if (interp_image != 0) {
			slab_free(interp_image);
		}
		slab_free(image);
		linux_exec_args_free(&args);
		return (u64)-LINUX_ENOMEM;
	}
	if ((interp_image == 0 &&
	     linux_exec_apply_relative_relocations(image, ehdr, image_size,
						  load_bias) != 0) ||
	    linux_exec_build_stack(path, &args, ehdr, load_bias, program_entry,
				   interp_bias, stack_image, stack_size,
				   &new_sp) != 0) {
		slab_free(stack_image);
		if (interp_image != 0) {
			slab_free(interp_image);
		}
		slab_free(image);
		linux_exec_args_free(&args);
		return (u64)-LINUX_EINVAL;
	}

	if (linux_exec_unmap_current(task) != 0) {
		slab_free(stack_image);
		if (interp_image != 0) {
			slab_free(interp_image);
		}
		slab_free(image);
		linux_exec_args_free(&args);
		return (u64)-LINUX_EINVAL;
	}

	for (u64 i = 0; i < ehdr->phnum; i++) {
		const struct elf64_phdr *phdr =
			(const struct elf64_phdr *)(image +
						    ehdr->phoff +
						    i * ehdr->phentsize);

		if (phdr->type == ELF_PH_LOAD &&
		    linux_exec_map_segment(task, image, phdr, load_bias) != 0) {
			slab_free(stack_image);
			if (interp_image != 0) {
				slab_free(interp_image);
			}
			slab_free(image);
			linux_exec_args_free(&args);
			return (u64)-LINUX_ENOMEM;
		}
	}
	if (interp_image != 0) {
		for (u64 i = 0; i < interp_ehdr->phnum; i++) {
			const struct elf64_phdr *phdr =
				(const struct elf64_phdr *)(interp_image +
							    interp_ehdr->phoff +
							    i * interp_ehdr->phentsize);

			if (phdr->type == ELF_PH_LOAD &&
			    linux_exec_map_segment(task, interp_image, phdr,
						   interp_bias) != 0) {
				slab_free(stack_image);
				slab_free(interp_image);
				slab_free(image);
				linux_exec_args_free(&args);
				return (u64)-LINUX_ENOMEM;
			}
		}
	}

	if (vm_alloc_user_range(task_vm_space(task), stack_base,
				stack_size, 1) != 0 ||
	    task_add_vm_mapping(task, stack_base,
				stack_size,
				TASK_VM_PROT_READ | TASK_VM_PROT_WRITE,
				TASK_VM_MAP_PRIVATE | TASK_VM_MAP_ANONYMOUS,
				TASK_VM_REGION_STACK, TASK_VM_OBJECT_ANON,
				0, 0) != 0 ||
	    vm_write_user(task_vm_space(task), stack_base, stack_image,
			  stack_size) != 0) {
		slab_free(stack_image);
		if (interp_image != 0) {
			slab_free(interp_image);
		}
		slab_free(image);
		linux_exec_args_free(&args);
		return (u64)-LINUX_ENOMEM;
	}
	slab_free(stack_image);
	stack_image = 0;

	frame->user_rip = entry;
	frame->user_rsp = new_sp;
	const u64 exec_result = linux_exec_process(task);
	if ((i64)exec_result < 0) {
		if (interp_image != 0) {
			slab_free(interp_image);
		}
		slab_free(image);
		linux_exec_args_free(&args);
		return exec_result;
	}
	linux_proc_set_cmdline(task, exec_result, path);
	linux_vfork_complete_task(task_id(task));
	console_printf("linux: execve task=%u path=%s entry=%p stack=%p\n",
		       task_id(task), path, (const void *)entry,
		       (const void *)new_sp);
	if (interp_image != 0) {
		slab_free(interp_image);
	}
	slab_free(image);
	linux_exec_args_free(&args);
	return 0;
}

static u64 linux_fork_process(struct task *parent, struct task *child)
{
	struct ipc_port *linux = ipc_port_find("linux");
	struct ipc_port *reply_port = task_reply_port(parent);
	struct ipc_message request = {
		.protocol = USER_FOURCC_LINX,
		.type = LINUX_RPC_FORK_PROCESS,
		.sender = 0,
		.cap_rights = 0,
		.reply_port = reply_port,
		.cap_type = IPC_CAP_NONE,
		.cap_object = 0,
		.words = { task_id(parent), task_id(child), 0, 0 },
	};
	struct ipc_message reply;

	if (linux_forward_message(linux, reply_port, &request, &reply) != 0) {
		return (u64)-LINUX_ENOSYS;
	}

	return reply.words[0];
}

static u64 linux_exec_process(struct task *task)
{
	struct ipc_port *linux = ipc_port_find("linux");
	struct ipc_port *reply_port = task_reply_port(task);
	struct ipc_message request = {
		.protocol = USER_FOURCC_LINX,
		.type = LINUX_RPC_EXEC_PROCESS,
		.sender = 0,
		.cap_rights = 0,
		.reply_port = reply_port,
		.cap_type = IPC_CAP_NONE,
		.cap_object = 0,
		.words = { 0, 0, 0, 0 },
	};
	struct ipc_message reply;

	if (linux_forward_message(linux, reply_port, &request, &reply) != 0) {
		return (u64)-LINUX_ENOSYS;
	}

	return reply.words[0];
}

static void linux_proc_set_cmdline(struct task *task, u64 linux_pid,
				   const char *path)
{
	struct ipc_port *proc = ipc_port_find("proc");
	struct ipc_port *reply_port = task_reply_port(task);
	struct shared_buffer *buffer;
	u64 len;
	struct ipc_message request = {
		.protocol = USER_FOURCC_PROC,
		.type = USER_PROC_SET_CMDLINE_BUFFER,
		.sender = 0,
		.cap_rights = TASK_RIGHT_RECV,
		.reply_port = reply_port,
		.cap_type = IPC_CAP_BUFFER,
		.cap_object = 0,
		.words = { linux_pid, task_id(task), 0, 0 },
	};
	struct ipc_message reply;

	if (proc == 0 || reply_port == 0 || linux_pid == 0 || path == 0) {
		return;
	}
	len = str_len(path) + 1;
	if (len == 0 || len > LINUX_EXEC_MAX_PATH) {
		return;
	}
	buffer = buffer_create(len);
	if (buffer == 0 || buffer_write(buffer, 0, path, len) != 0) {
		buffer_release(buffer);
		return;
	}
	request.cap_object = buffer;
	request.words[2] = len;
	(void)(ipc_send(proc, &request) == 0 &&
	       ipc_recv(reply_port, &reply) == 0);
	buffer_release(buffer);
}

static const char *linux_syscall_name(u64 number)
{
	switch (number) {
	case LINUX_SYSCALL_READ:
		return "read";
	case LINUX_SYSCALL_WRITE:
		return "write";
	case LINUX_SYSCALL_OPEN:
		return "open";
	case LINUX_SYSCALL_CLOSE:
		return "close";
	case LINUX_SYSCALL_STAT:
		return "stat";
	case LINUX_SYSCALL_FSTAT:
		return "fstat";
	case LINUX_SYSCALL_LSTAT:
		return "lstat";
	case LINUX_SYSCALL_POLL:
		return "poll";
	case LINUX_SYSCALL_MMAP:
		return "mmap";
	case LINUX_SYSCALL_MPROTECT:
		return "mprotect";
	case LINUX_SYSCALL_MUNMAP:
		return "munmap";
	case LINUX_SYSCALL_BRK:
		return "brk";
	case LINUX_SYSCALL_RT_SIGACTION:
		return "rt_sigaction";
	case LINUX_SYSCALL_RT_SIGPROCMASK:
		return "rt_sigprocmask";
	case LINUX_SYSCALL_RT_SIGTIMEDWAIT:
		return "rt_sigtimedwait";
	case LINUX_SYSCALL_IOCTL:
		return "ioctl";
	case LINUX_SYSCALL_WRITEV:
		return "writev";
	case LINUX_SYSCALL_ACCESS:
		return "access";
	case LINUX_SYSCALL_PIPE:
		return "pipe";
	case LINUX_SYSCALL_TRUNCATE:
		return "truncate";
	case LINUX_SYSCALL_FTRUNCATE:
		return "ftruncate";
	case LINUX_SYSCALL_RMDIR:
		return "rmdir";
	case LINUX_SYSCALL_CHMOD:
		return "chmod";
	case LINUX_SYSCALL_FCHMOD:
		return "fchmod";
	case LINUX_SYSCALL_CHOWN:
		return "chown";
	case LINUX_SYSCALL_FCHOWN:
		return "fchown";
	case LINUX_SYSCALL_LCHOWN:
		return "lchown";
	case LINUX_SYSCALL_UNLINK:
		return "unlink";
	case LINUX_SYSCALL_NANOSLEEP:
		return "nanosleep";
	case LINUX_SYSCALL_DUP:
		return "dup";
	case LINUX_SYSCALL_DUP2:
		return "dup2";
	case LINUX_SYSCALL_SENDFILE:
		return "sendfile";
	case LINUX_SYSCALL_SOCKET:
		return "socket";
	case LINUX_SYSCALL_CONNECT:
		return "connect";
	case LINUX_SYSCALL_SENDTO:
		return "sendto";
	case LINUX_SYSCALL_RECVFROM:
		return "recvfrom";
	case LINUX_SYSCALL_GETCWD:
		return "getcwd";
	case LINUX_SYSCALL_CHDIR:
		return "chdir";
	case LINUX_SYSCALL_MKDIR:
		return "mkdir";
	case LINUX_SYSCALL_GETTIMEOFDAY:
		return "gettimeofday";
	case LINUX_SYSCALL_UMASK:
		return "umask";
	case LINUX_SYSCALL_SYSINFO:
		return "sysinfo";
	case LINUX_SYSCALL_GETUID:
		return "getuid";
	case LINUX_SYSCALL_SYSLOG:
		return "syslog";
	case LINUX_SYSCALL_GETGID:
		return "getgid";
	case LINUX_SYSCALL_SETUID:
		return "setuid";
	case LINUX_SYSCALL_SETGID:
		return "setgid";
	case LINUX_SYSCALL_GETEUID:
		return "geteuid";
	case LINUX_SYSCALL_GETEGID:
		return "getegid";
	case LINUX_SYSCALL_GETPPID:
		return "getppid";
	case LINUX_SYSCALL_GETPGRP:
		return "getpgrp";
	case LINUX_SYSCALL_GETGROUPS:
		return "getgroups";
	case LINUX_SYSCALL_SETGROUPS:
		return "setgroups";
	case LINUX_SYSCALL_SETRESUID:
		return "setresuid";
	case LINUX_SYSCALL_SETRESGID:
		return "setresgid";
	case LINUX_SYSCALL_GETPGID:
		return "getpgid";
	case LINUX_SYSCALL_GETSID:
		return "getsid";
	case LINUX_SYSCALL_STATFS:
		return "statfs";
	case LINUX_SYSCALL_FSTATFS:
		return "fstatfs";
	case LINUX_SYSCALL_MOUNT:
		return "mount";
	case LINUX_SYSCALL_UMOUNT2:
		return "umount2";
	case LINUX_SYSCALL_SETPGID:
		return "setpgid";
	case LINUX_SYSCALL_SETSID:
		return "setsid";
	case LINUX_SYSCALL_CLONE:
		return "clone";
	case LINUX_SYSCALL_FORK:
		return "fork";
	case LINUX_SYSCALL_VFORK:
		return "vfork";
	case LINUX_SYSCALL_KILL:
		return "kill";
	case LINUX_SYSCALL_EXIT:
		return "exit";
	case LINUX_SYSCALL_UNAME:
		return "uname";
	case LINUX_SYSCALL_FCNTL:
		return "fcntl";
	case LINUX_SYSCALL_ARCH_PRCTL:
		return "arch_prctl";
	case LINUX_SYSCALL_REBOOT:
		return "reboot";
	case LINUX_SYSCALL_TIME:
		return "time";
	case LINUX_SYSCALL_FUTEX:
		return "futex";
	case LINUX_SYSCALL_SET_TID_ADDRESS:
		return "set_tid_address";
	case LINUX_SYSCALL_CLOCK_GETTIME:
		return "clock_gettime";
	case LINUX_SYSCALL_CLOCK_NANOSLEEP:
		return "clock_nanosleep";
	case LINUX_SYSCALL_EXECVE:
		return "execve";
	case LINUX_SYSCALL_GETPID:
		return "getpid";
	case LINUX_SYSCALL_WAIT4:
		return "wait4";
	case LINUX_SYSCALL_GETTID:
		return "gettid";
	case LINUX_SYSCALL_GETDENTS64:
		return "getdents64";
	case LINUX_SYSCALL_NEWFSTATAT:
		return "newfstatat";
	case LINUX_SYSCALL_READLINKAT:
		return "readlinkat";
	case LINUX_SYSCALL_FCHMODAT:
		return "fchmodat";
	case LINUX_SYSCALL_FACCESSAT:
		return "faccessat";
	case LINUX_SYSCALL_FACCESSAT2:
		return "faccessat2";
	case LINUX_SYSCALL_PPOLL:
		return "ppoll";
	case LINUX_SYSCALL_SET_ROBUST_LIST:
		return "set_robust_list";
	case LINUX_SYSCALL_DUP3:
		return "dup3";
	case LINUX_SYSCALL_PIPE2:
		return "pipe2";
	case LINUX_SYSCALL_OPENAT:
		return "openat";
	case LINUX_SYSCALL_MKDIRAT:
		return "mkdirat";
	case LINUX_SYSCALL_FCHOWNAT:
		return "fchownat";
	case LINUX_SYSCALL_UNLINKAT:
		return "unlinkat";
	case LINUX_SYSCALL_PRLIMIT64:
		return "prlimit64";
	case LINUX_SYSCALL_GETRANDOM:
		return "getrandom";
	case LINUX_SYSCALL_STATX:
		return "statx";
	case LINUX_SYSCALL_EXIT_GROUP:
		return "exit_group";
	default:
		return "unknown";
	}
}

static void linux_strace_log(const struct arch_syscall_frame *frame, u64 result)
{
	const struct task *task = task_current();
	const u64 number = frame->number;

	console_printf("linux-strace: task=%u name=%s rip=%p %s(%p,%p,%p,%p) = %p\n",
		       task_id(task), task_name(task),
		       (const void *)frame->user_rip,
		       linux_syscall_name(number),
		       (const void *)frame->arg0, (const void *)frame->arg1,
		       (const void *)frame->arg2, (const void *)frame->arg3,
		       (const void *)result);
}

static void linux_strace_enter(const struct arch_syscall_frame *frame)
{
	const struct task *task = task_current();
	const u64 number = frame->number;

	console_printf("linux-strace-enter: task=%u name=%s rip=%p %s(%p,%p,%p,%p)\n",
		       task_id(task), task_name(task),
		       (const void *)frame->user_rip,
		       linux_syscall_name(number),
		       (const void *)frame->arg0, (const void *)frame->arg1,
		       (const void *)frame->arg2, (const void *)frame->arg3);
}

static int task_uses_linux_personality(const struct task *task)
{
	const char *name = task_name(task);

	return str_eq(name, "busybox") ||
	       str_eq(name, "login") ||
	       str_eq(name, "musl-hello") ||
	       str_eq(name, "lxtest") ||
	       str_eq(name, "execok");
}

static void linux_negative_syscall_dump_bytes(struct task *task, u64 vaddr)
{
	u8 bytes[16];

	if (vaddr == 0 ||
	    vm_read_user(task_vm_space(task), vaddr, bytes, sizeof(bytes)) != 0) {
		console_printf("user-syscall: bytes addr=%p unreadable\n",
			       (const void *)vaddr);
		return;
	}

	console_printf("user-syscall: bytes addr=%p %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n",
		       (const void *)vaddr, bytes[0], bytes[1], bytes[2],
		       bytes[3], bytes[4], bytes[5], bytes[6], bytes[7],
		       bytes[8], bytes[9], bytes[10], bytes[11], bytes[12],
		       bytes[13], bytes[14], bytes[15]);
}

static void linux_negative_syscall_dump_stack(struct task *task, u64 rsp)
{
	u64 words[8];

	if (rsp == 0 ||
	    vm_read_user(task_vm_space(task), rsp, words, sizeof(words)) != 0) {
		console_printf("user-syscall: stack rsp=%p unreadable\n",
			       (const void *)rsp);
		return;
	}

	for (u64 i = 0; i < sizeof(words) / sizeof(words[0]); i++) {
		console_printf("user-syscall: stack[%u] %p\n", (u32)i,
			       (const void *)words[i]);
	}
}

static void linux_negative_syscall_dump_rbp(struct task *task, u64 rbp)
{
	for (u64 depth = 0; depth < 8 && rbp != 0; depth++) {
		u64 frame[2];

		if ((rbp & 7) != 0 ||
		    vm_read_user(task_vm_space(task), rbp, frame,
				 sizeof(frame)) != 0) {
			console_printf("user-syscall: rbp[%u] frame=%p unreadable\n",
				       (u32)depth, (const void *)rbp);
			return;
		}

		console_printf("user-syscall: rbp[%u] frame=%p return=%p\n",
			       (u32)depth, (const void *)rbp,
			       (const void *)frame[1]);
		if (frame[0] <= rbp) {
			return;
		}
		rbp = frame[0];
	}
}

static void linux_negative_syscall_dump(struct arch_syscall_frame *frame)
{
	static u32 dumps;
	struct task *task = task_current();

	if (dumps >= 4) {
		return;
	}
	dumps++;

	console_printf("user-syscall: negative linux task=%u name=%s number=%d rip=%p rsp=%p rbp=%p rflags=%p\n",
		       task_id(task), task_name(task), (i32)frame->number,
		       (const void *)frame->user_rip,
		       (const void *)frame->user_rsp,
		       (const void *)frame->rbp,
		       (const void *)frame->user_rflags);
	console_printf("user-syscall: args arg0=%p arg1=%p arg2=%p arg3=%p\n",
		       (const void *)frame->arg0, (const void *)frame->arg1,
		       (const void *)frame->arg2, (const void *)frame->arg3);
	linux_negative_syscall_dump_bytes(task, frame->user_rip - 8);
	linux_negative_syscall_dump_bytes(task, frame->arg0);
	linux_negative_syscall_dump_stack(task, frame->user_rsp);
	linux_negative_syscall_dump_rbp(task, frame->rbp);
}

static u64 linux_syscall_handle(struct arch_syscall_frame *frame)
	__attribute__((noinline));

static u64 linux_syscall_handle(struct arch_syscall_frame *frame)
{
	struct task *task = task_current();
	const u64 number = frame->number;
	const u64 arg0 = frame->arg0;
	const u64 arg1 = frame->arg1;
	const u64 arg2 = frame->arg2;
	const u64 arg3 = frame->arg3;
	struct ipc_port *linux = ipc_port_find("linux");
	struct ipc_port *reply_port = task_reply_port(task);
	struct ipc_message request = {
		.protocol = USER_FOURCC_LINX,
		.type = (u32)number,
		.sender = 0,
		.cap_rights = 0,
		.reply_port = 0,
		.cap_type = IPC_CAP_NONE,
		.cap_object = 0,
		.words = { arg0, arg1, arg2, 0 },
	};
	struct ipc_message reply;

	switch (number) {
	case LINUX_SYSCALL_EXECVE:
		return linux_execve(task, frame, (const char *)arg0);
	case LINUX_SYSCALL_MMAP: {
		const u64 prot = arg2;
		const u64 flags = arg3;
		const u64 fd = frame->r8;
		const u64 offset = frame->r9;
		const int anonymous = (flags & LINUX_MAP_ANONYMOUS) != 0;
		const u32 task_prot = linux_prot_to_task(prot);
		const u32 task_flags = linux_map_flags_to_task(flags);
		const u32 writable = (task_prot & TASK_VM_PROT_WRITE) != 0;
		const u32 alloc_writable = writable || !anonymous;
		u64 base = arg0;
		u64 len = arg1;

		if (len == 0 || len > LINUX_MAX_MMAP_SIZE ||
		    (flags & LINUX_MAP_PRIVATE) == 0 ||
		    len + VM_PAGE_SIZE - 1 < len ||
		    (!anonymous && (offset & (VM_PAGE_SIZE - 1)) != 0)) {
			return (u64)-LINUX_EINVAL;
		}

		len = align_up(len, VM_PAGE_SIZE);
		if (base == 0) {
			base = task_linux_mmap_next(task);
		} else if ((flags & LINUX_MAP_FIXED) == 0) {
			base = align_up(base, VM_PAGE_SIZE);
		} else if ((base & (VM_PAGE_SIZE - 1)) != 0) {
			return (u64)-LINUX_EINVAL;
		}

		const u64 min_base = (flags & LINUX_MAP_FIXED) != 0 ?
				     LINUX_MAP_FIXED_MIN : LINUX_MMAP_BASE;
		if (base < min_base || base + len < base ||
		    base + len > LINUX_MMAP_LIMIT) {
			return (u64)-LINUX_ENOMEM;
		}

		if ((flags & LINUX_MAP_FIXED) != 0 &&
		    !task_vm_range_is_free(task, base, len) &&
		    linux_unmap_task_range(task, base, len) != 0) {
			return (u64)-LINUX_EINVAL;
		}
		if ((flags & LINUX_MAP_FIXED) == 0 &&
		    !task_vm_range_is_free(task, base, len)) {
			return (u64)-LINUX_ENOMEM;
		}

		if (vm_alloc_user_range(task_vm_space(task), base, len,
					alloc_writable) != 0) {
			return (u64)-LINUX_ENOMEM;
		}
		if (task_add_vm_mapping(task, base, len, task_prot,
					task_flags, TASK_VM_REGION_MMAP,
					TASK_VM_OBJECT_ANON, 0, 0) != 0) {
			(void)vm_unmap_user_range(task_vm_space(task), base, len);
			return (u64)-LINUX_ENOMEM;
		}
		if (!anonymous &&
		    linux_mmap_file_into_task(task, linux, reply_port, base, len,
					     fd, offset) != 0) {
			(void)linux_unmap_task_range(task, base, len);
			return (u64)-LINUX_EINVAL;
		}
		if (!anonymous && !writable &&
		    vm_protect_user_range(task_vm_space(task), base, len, 0) != 0) {
			(void)linux_unmap_task_range(task, base, len);
			return (u64)-LINUX_EINVAL;
		}

		if (base + len > task_linux_mmap_next(task)) {
			task_set_linux_mmap_next(task, base + len);
		}
		console_printf("linux: mmap task=%u addr=%p len=%u flags=0x%x\n",
			       task_id(task), (const void *)base, (u32)len,
			       (u32)flags);
		return base;
	}
	case LINUX_SYSCALL_MUNMAP: {
		u64 base = arg0;
		u64 len = arg1;

		if (base == 0 || len == 0 ||
		    (base & (VM_PAGE_SIZE - 1)) != 0 ||
		    len + VM_PAGE_SIZE - 1 < len) {
			return (u64)-LINUX_EINVAL;
		}

		len = align_up(len, VM_PAGE_SIZE);
		if (linux_unmap_task_range(task, base, len) != 0) {
			return (u64)-LINUX_EINVAL;
		}
		console_printf("linux: munmap task=%u addr=%p len=%u\n",
			       task_id(task), (const void *)base, (u32)len);
		return 0;
	}
	case LINUX_SYSCALL_CLONE:
	case LINUX_SYSCALL_FORK:
	case LINUX_SYSCALL_VFORK: {
		struct arch_syscall_frame child_frame = *frame;
		struct task *child;
		struct linux_vfork_wait *vfork_waiter = 0;
		const int is_vfork = number == LINUX_SYSCALL_VFORK ||
				     (number == LINUX_SYSCALL_CLONE &&
				      (arg0 & LINUX_CLONE_VFORK) != 0);
		u64 pid;

		if (number == LINUX_SYSCALL_CLONE && arg1 != 0) {
			child_frame.user_rsp = arg1;
		}
		child = server_task_fork_current_stopped(&child_frame);
		if (child == 0) {
			return (u64)-LINUX_ENOMEM;
		}
		if (is_vfork) {
			vfork_waiter = linux_vfork_begin(task_id(child));
			if (vfork_waiter == 0) {
				(void)task_kill(child);
				return (u64)-LINUX_ENOMEM;
			}
		}

		pid = linux_fork_process(task, child);
		if ((i64)pid < 0) {
			linux_vfork_cancel(vfork_waiter);
			(void)task_kill(child);
			return pid;
		}
		if (server_task_start_fork(child, &child_frame) != 0) {
			linux_vfork_cancel(vfork_waiter);
			(void)task_kill(child);
			return (u64)-LINUX_ENOMEM;
		}
		if (is_vfork) {
			linux_vfork_wait(vfork_waiter);
		}
		console_printf("linux: fork parent=%u child=%u linux_pid=%u\n",
			       task_id(task), task_id(child), (u32)pid);
		return pid;
	}
	case LINUX_SYSCALL_BRK:
		if (arg0 == 0) {
			return task_linux_brk(task);
		}
		if (arg0 >= LINUX_INITIAL_BRK && arg0 < LINUX_MAX_BRK) {
			const u64 old_brk = task_linux_brk(task);
			const u64 old_page = align_up(old_brk, VM_PAGE_SIZE);
			const u64 new_page = align_up(arg0, VM_PAGE_SIZE);

			if (new_page > old_page &&
			    (vm_alloc_user_range(task_vm_space(task), old_page,
						 new_page - old_page, 1) != 0 ||
			     task_add_or_extend_vm_mapping(task, old_page,
							   new_page - old_page,
							   TASK_VM_PROT_READ |
							   TASK_VM_PROT_WRITE,
							   TASK_VM_MAP_PRIVATE |
							   TASK_VM_MAP_ANONYMOUS,
							   TASK_VM_REGION_BRK,
							   TASK_VM_OBJECT_ANON,
							   0, 0) != 0)) {
				return task_linux_brk(task);
			}
			if (new_page < old_page &&
			    linux_unmap_task_range(task, new_page,
						   old_page - new_page) != 0) {
				return task_linux_brk(task);
			}
			task_set_linux_brk(task, arg0);
		}
		return task_linux_brk(task);
	case LINUX_SYSCALL_ARCH_PRCTL:
		if (arg0 != LINUX_ARCH_SET_FS) {
			return (u64)-LINUX_EINVAL;
		}
		task_set_linux_fs_base(task, arg1);
		arch_user_set_fs_base(arg1);
		console_printf("linux: arch_prctl set_fs=%p\n",
			       (const void *)arg1);
		return 0;
	case LINUX_SYSCALL_SET_TID_ADDRESS:
		return thread_id(thread_current());
	case LINUX_SYSCALL_SYSLOG: {
		enum {
			LINUX_SYSLOG_ACTION_READ = 2,
			LINUX_SYSLOG_ACTION_READ_ALL = 3,
			LINUX_SYSLOG_ACTION_READ_CLEAR = 4,
			LINUX_SYSLOG_ACTION_CLEAR = 5,
			LINUX_SYSLOG_ACTION_CONSOLE_OFF = 6,
			LINUX_SYSLOG_ACTION_CONSOLE_ON = 7,
			LINUX_SYSLOG_ACTION_CONSOLE_LEVEL = 8,
			LINUX_SYSLOG_ACTION_SIZE_BUFFER = 10,
		};
		const u64 size = min_u64(console_log_size(),
					 LINUX_MAX_SYSCALL_BUFFER);
		u64 nread;
		u64 flags;

		switch (arg0) {
		case LINUX_SYSLOG_ACTION_SIZE_BUFFER:
			return size;
		case LINUX_SYSLOG_ACTION_CLEAR:
		case LINUX_SYSLOG_ACTION_CONSOLE_OFF:
		case LINUX_SYSLOG_ACTION_CONSOLE_ON:
		case LINUX_SYSLOG_ACTION_CONSOLE_LEVEL:
			return 0;
		case LINUX_SYSLOG_ACTION_READ:
		case LINUX_SYSLOG_ACTION_READ_ALL:
		case LINUX_SYSLOG_ACTION_READ_CLEAR:
			if (arg1 == 0) {
				return (u64)-LINUX_EINVAL;
			}
			nread = min_u64(arg2, size);
			flags = spin_lock_irqsave(&syscall_copy_lock);
			nread = console_log_read((char *)syscall_copy_buffer,
						 nread);
			if (write_current_user(arg1, syscall_copy_buffer,
					       nread) != 0) {
				spin_unlock_irqrestore(&syscall_copy_lock, flags);
				return (u64)-LINUX_EFAULT;
			}
			spin_unlock_irqrestore(&syscall_copy_lock, flags);
			return nread;
		default:
			return (u64)-LINUX_EINVAL;
		}
	}
	case LINUX_SYSCALL_SET_ROBUST_LIST:
		return 0;
	case LINUX_SYSCALL_RT_SIGACTION: {
		u64 handler = ~0ull;

		if (arg3 != 8) {
			return (u64)-LINUX_EINVAL;
		}
		if (arg1 != 0 &&
		    read_current_user(arg1, &handler, sizeof(handler)) != 0) {
			return (u64)-LINUX_EFAULT;
		}
		request.type = LINUX_SYSCALL_RT_SIGACTION;
		request.words[0] = arg0;
		request.words[1] = handler;
		request.words[2] = 0;
		request.reply_port = reply_port;
		if (linux_forward_message(linux, reply_port, &request,
					  &reply) != 0) {
			return (u64)-LINUX_ENOSYS;
		}
		if ((i64)reply.words[0] < 0) {
			return reply.words[0];
		}
		if (arg2 != 0) {
			u8 action[32];

			mem_zero(action, sizeof(action));
			mem_copy(action, (const u8 *)&reply.words[1],
				 sizeof(reply.words[1]));
			if (write_current_user(arg2, action, sizeof(action)) != 0) {
				return (u64)-LINUX_EFAULT;
			}
		}
		return 0;
	}
	case LINUX_SYSCALL_RT_SIGPROCMASK: {
		u64 set = 0;
		u64 how = arg1 == 0 ? ~0ull : arg0;

		if (arg3 != 8) {
			return (u64)-LINUX_EINVAL;
		}
		if (arg1 != 0 &&
		    read_current_user(arg1, &set, sizeof(set)) != 0) {
			return (u64)-LINUX_EFAULT;
		}
		request.type = LINUX_SYSCALL_RT_SIGPROCMASK;
		request.words[0] = how;
		request.words[1] = set;
		request.words[2] = 0;
		request.reply_port = reply_port;
		if (linux_forward_message(linux, reply_port, &request,
					  &reply) != 0) {
			return (u64)-LINUX_ENOSYS;
		}
		if ((i64)reply.words[0] < 0) {
			return reply.words[0];
		}
		if (arg2 != 0 &&
		    write_current_user(arg2, &reply.words[1],
				       sizeof(reply.words[1])) != 0) {
			return (u64)-LINUX_EFAULT;
		}
		return 0;
	}
	case LINUX_SYSCALL_RT_SIGTIMEDWAIT: {
		u64 set = 0;

		if (arg3 != 8 || arg0 == 0) {
			return (u64)-LINUX_EINVAL;
		}
		if (read_current_user(arg0, &set, sizeof(set)) != 0) {
			return (u64)-LINUX_EFAULT;
		}
		request.type = LINUX_SYSCALL_RT_SIGTIMEDWAIT;
		request.words[0] = set;
		request.words[1] = arg1 != 0;
		request.words[2] = arg2 != 0;
		request.reply_port = reply_port;
		if (linux_forward_message(linux, reply_port, &request,
					  &reply) != 0) {
			return (u64)-LINUX_ENOSYS;
		}
		return reply.words[0];
	}
	case LINUX_SYSCALL_MPROTECT:
		if (arg0 == 0 || (arg0 & (VM_PAGE_SIZE - 1)) != 0 ||
		    arg1 == 0 || arg1 + VM_PAGE_SIZE - 1 < arg1) {
			return (u64)-LINUX_EINVAL;
		}
		{
			const u64 len = align_up(arg1, VM_PAGE_SIZE);
			const u32 prot = linux_prot_to_task(arg2);
			const u32 writable =
				(prot & TASK_VM_PROT_WRITE) != 0;

			const int vm_rc = arg0 + len < arg0 ? -1 :
					  vm_protect_user_range(task_vm_space(task),
								arg0, len,
								writable);
			const int task_rc = vm_rc != 0 ? -1 :
					    task_protect_vm_region(task, arg0,
								   len, prot);

			if (vm_rc != 0 || task_rc != 0) {
				return (u64)-LINUX_EINVAL;
			}
			console_printf("linux: mprotect task=%u addr=%p len=%u prot=0x%x\n",
				       task_id(task), (const void *)arg0,
				       (u32)len, (u32)arg2);
		}
		return 0;
	case LINUX_SYSCALL_IOCTL:
		if (arg1 == LINUX_TCGETS || arg1 == LINUX_TCSETS ||
		    arg1 == LINUX_TIOCGPGRP || arg1 == LINUX_TIOCSPGRP ||
		    arg1 == LINUX_TIOCGWINSZ) {
			break;
		}
		return (u64)-LINUX_ENOTTY;
	case LINUX_SYSCALL_FCNTL:
		break;
	case LINUX_SYSCALL_NANOSLEEP:
		return linux_sleep_relative(arg0, arg1);
	case LINUX_SYSCALL_CLOCK_NANOSLEEP:
		if (arg0 != LINUX_CLOCK_REALTIME &&
		    arg0 != LINUX_CLOCK_MONOTONIC) {
			return (u64)-LINUX_EINVAL;
		}
		if ((arg1 & ~LINUX_TIMER_ABSTIME) != 0) {
			return (u64)-LINUX_EINVAL;
		}
		if ((arg1 & LINUX_TIMER_ABSTIME) != 0) {
			return linux_sleep_absolute(arg2);
		}
		return linux_sleep_relative(arg2, arg3);
	case LINUX_SYSCALL_PRLIMIT64:
		if (arg3 != 0) {
			u64 limit[2] = { 0x800000, 0x800000 };

			if (write_current_user(arg3, limit, sizeof(limit)) != 0) {
				return (u64)-LINUX_EINVAL;
			}
		}
		return 0;
	case LINUX_SYSCALL_FUTEX: {
		const u64 op = arg1 & 0x7f;

		if (op == LINUX_FUTEX_WAKE) {
			return 0;
		}
		if (op == LINUX_FUTEX_WAIT) {
			u32 value = 0;

			if (arg0 == 0 ||
			    read_current_user(arg0, &value, sizeof(value)) != 0) {
				return (u64)-LINUX_EFAULT;
			}
			return value != (u32)arg2 ? (u64)-LINUX_EAGAIN : 0;
		}
		return (u64)-LINUX_ENOSYS;
	}
	case LINUX_SYSCALL_POLL:
	case LINUX_SYSCALL_PPOLL: {
		struct {
			int fd;
			short events;
			short revents;
		} pollfd;
		u64 ready = 0;
		u64 timeout_ns;
		int infinite_timeout;
		int slept = 0;
		int suppress_pollin = 0;

		if (arg1 > 64 || (arg1 != 0 && arg0 == 0)) {
			return (u64)-LINUX_EINVAL;
		}
		timeout_ns = linux_poll_timeout_ns(number, arg2);
		infinite_timeout = linux_poll_timeout_is_infinite(number, arg2);
		if (number == LINUX_SYSCALL_PPOLL) {
			suppress_pollin = arg2 != 0 && timeout_ns != 0;
		} else {
			const u32 raw = (u32)arg2;

			suppress_pollin = raw != 0 &&
					   (raw & 0x80000000u) == 0;
		}
poll_again:
		ready = 0;
		for (u64 i = 0; i < arg1; i++) {
			const u64 addr = arg0 + i * sizeof(pollfd);
			short revents = 0;

			if (read_current_user(addr, &pollfd, sizeof(pollfd)) != 0) {
				return (u64)-LINUX_EFAULT;
			}
			revents = linux_poll_revents(pollfd.fd, pollfd.events,
						     suppress_pollin);
			if (revents != 0) {
				ready++;
			}
			if (write_current_user(addr + 6, &revents,
					       sizeof(revents)) != 0) {
				return (u64)-LINUX_EFAULT;
			}
		}
		if (ready == 0 && timeout_ns != 0 &&
		    (infinite_timeout || !slept)) {
			slept = 1;
			thread_sleep_ns(infinite_timeout ? 10000000ull : timeout_ns);
			goto poll_again;
		}
		return ready;
	}
	case LINUX_SYSCALL_WRITEV: {
		struct {
			u64 base;
			u64 len;
		} iov;
		u64 total = 0;

		if (linux == 0 || reply_port == 0 || arg1 == 0 || arg2 > 16) {
			return (u64)-LINUX_EINVAL;
		}
		for (u64 i = 0; i < arg2; i++) {
			if (read_current_user(arg1 + i * sizeof(iov), &iov,
					      sizeof(iov)) != 0) {
				return (u64)-LINUX_EFAULT;
			}
			if (iov.base == 0 && iov.len != 0) {
				return (u64)-LINUX_EINVAL;
			}
			for (u64 offset = 0; offset < iov.len;) {
				const u64 chunk =
					min_u64(iov.len - offset,
						LINUX_MAX_SYSCALL_BUFFER);
				const u64 wrote =
					linux_write_one(linux, reply_port, arg0,
							iov.base + offset,
							chunk);

				if ((i64)wrote < 0) {
					return total != 0 ? total : wrote;
				}
				if (wrote == 0) {
					return total;
				}
				total += wrote;
				offset += wrote;
				if (wrote != chunk) {
					return total;
				}
			}
		}
		return total;
	}
	case LINUX_SYSCALL_EXIT:
		return linux_exit_current(arg0);
	default:
		break;
	}

	if (linux == 0 || reply_port == 0) {
		return (u64)-LINUX_ENOSYS;
	}
	request.reply_port = reply_port;
	if (linux_syscall_forwards_scalar(number)) {
		return linux_forward_words(linux, reply_port, number,
					   arg0, arg1, arg2);
	}

	switch (number) {
	case LINUX_SYSCALL_READ: {
		return linux_read_chunked(linux, reply_port, arg0, arg1,
					  arg2, TASK_RIGHT_SEND |
					  TASK_RIGHT_DUP);
	}
	case LINUX_SYSCALL_GETDENTS64: {
		return linux_getdents64_chunked(linux, reply_port, &request,
						arg0, arg1, arg2);
	}
	case LINUX_SYSCALL_WRITE: {
		return linux_write_chunked(linux, reply_port, arg0, arg1,
					   arg2);
	}
	case LINUX_SYSCALL_PIPE:
	case LINUX_SYSCALL_PIPE2: {
		if (arg0 == 0) {
			return (u64)-LINUX_EFAULT;
		}
		request.type = number == LINUX_SYSCALL_PIPE ?
			       LINUX_SYSCALL_PIPE : LINUX_SYSCALL_PIPE2;
		request.words[0] = number == LINUX_SYSCALL_PIPE ? 0 : arg1;
		request.words[1] = 0;
		request.words[2] = 0;
		request.words[3] = 0;
		return linux_forward_two_u32_out(linux, reply_port, &request,
						 arg0);
	}
	case LINUX_SYSCALL_UNAME: {
		if (arg0 == 0) {
			return (u64)-LINUX_EFAULT;
		}
		return linux_forward_fixed_output_words(
			linux, reply_port, &request, LINUX_SYSCALL_UNAME,
			(void *)arg0, LINUX_UTSNAME_SIZE, 1, 0,
			0, 0, 0, 0);
	}
	case LINUX_SYSCALL_SYSINFO: {
		if (arg0 == 0) {
			return (u64)-LINUX_EFAULT;
		}
		return linux_forward_fixed_output_words(
			linux, reply_port, &request, LINUX_SYSCALL_SYSINFO,
			(void *)arg0, LINUX_SYSINFO_SIZE, 1, 0,
			0, 0, 0, 0);
	}
	case LINUX_SYSCALL_FSTATFS: {
		if (arg1 == 0) {
			return (u64)-LINUX_EFAULT;
		}
		return linux_forward_fixed_output_words(
			linux, reply_port, &request, LINUX_SYSCALL_FSTATFS,
			(void *)arg1, LINUX_STATFS_SIZE, 1, 0,
			arg0, 0, 0, 0);
	}
	case LINUX_SYSCALL_CLOCK_GETTIME: {
		if (arg1 == 0) {
			return (u64)-LINUX_EFAULT;
		}
		return linux_forward_fixed_output_words(
			linux, reply_port, &request, LINUX_SYSCALL_CLOCK_GETTIME,
			(void *)arg1, LINUX_TIMESPEC_SIZE, 1, 0,
			arg0, 0, 0, 0);
	}
	case LINUX_SYSCALL_GETTIMEOFDAY: {
		if (arg0 == 0) {
			return linux_forward_words(linux, reply_port,
						   LINUX_SYSCALL_GETTIMEOFDAY,
						   0, arg1, 0);
		}
		return linux_forward_fixed_output_words(
			linux, reply_port, &request, LINUX_SYSCALL_GETTIMEOFDAY,
			(void *)arg0, LINUX_TIMEVAL_SIZE, 1, 0,
			0, arg1, 0, 0);
	}
	case LINUX_SYSCALL_TIME: {
		if (arg0 == 0) {
			return linux_forward_words(linux, reply_port,
						   LINUX_SYSCALL_TIME, 0, 0, 0);
		}
		return linux_forward_fixed_output_words(
			linux, reply_port, &request, LINUX_SYSCALL_TIME,
			(void *)arg0, LINUX_TIME_T_SIZE, 1, 1,
			0, 0, 0, 0);
	}
	case LINUX_SYSCALL_CONNECT: {
		const u64 len = arg2 > LINUX_MAX_SOCKADDR ?
				LINUX_MAX_SOCKADDR : arg2;

		if (arg1 == 0 || len == 0) {
			return arg1 == 0 ? (u64)-LINUX_EFAULT :
			       (u64)-LINUX_EINVAL;
		}
		request.type = LINUX_SYSCALL_CONNECT;
		request.words[0] = arg0;
		request.words[1] = len;
		request.words[2] = 0;
		request.words[3] = 0;
		return linux_forward_input_buffer(linux, reply_port, &request,
						  arg1, len, TASK_RIGHT_RECV);
	}
	case LINUX_SYSCALL_SENDTO: {
		const u64 len = arg2 > LINUX_MAX_SYSCALL_BUFFER ?
				LINUX_MAX_SYSCALL_BUFFER : arg2;

		if (arg1 == 0 && len != 0) {
			return (u64)-LINUX_EFAULT;
		}
		request.type = LINUX_SYSCALL_SENDTO;
		request.words[0] = arg0;
		request.words[1] = len;
		request.words[2] = arg3;
		request.words[3] = 0;
		return linux_forward_input_buffer(linux, reply_port, &request,
						  arg1, len, TASK_RIGHT_RECV);
	}
	case LINUX_SYSCALL_RECVFROM: {
		const u64 len = arg2 > LINUX_MAX_SYSCALL_BUFFER ?
				LINUX_MAX_SYSCALL_BUFFER : arg2;

		if (arg1 == 0 && len != 0) {
			return (u64)-LINUX_EFAULT;
		}

		return linux_forward_output_words(linux, reply_port, &request,
						  LINUX_SYSCALL_RECVFROM,
						  (void *)arg1, len,
						  TASK_RIGHT_SEND,
						  arg0, arg3, 0);
	}
	case LINUX_SYSCALL_GETRANDOM: {
		return linux_getrandom_chunked(linux, reply_port, arg0, arg1);
	}
	case LINUX_SYSCALL_SENDFILE:
		return linux_sendfile_chunked(linux, reply_port, arg0, arg1,
					      arg2, arg3);
	case LINUX_SYSCALL_GETCWD: {
		u64 size;

		if (arg0 == 0 || arg1 == 0) {
			return arg0 == 0 ? (u64)-LINUX_EFAULT :
			       (u64)-LINUX_EINVAL;
		}
		size = min_u64(arg1, LINUX_EXEC_MAX_PATH);
		return linux_forward_output_words(linux, reply_port, &request,
						  LINUX_SYSCALL_GETCWD,
						  (void *)arg0, size,
						  TASK_RIGHT_SEND,
						  arg1, 0, 0);
	}
	case LINUX_SYSCALL_CHDIR: {
		request.type = LINUX_SYSCALL_CHDIR;
		request.words[0] = 0;
		request.words[1] = 0;
		request.words[2] = 0;
		request.words[3] = 0;
		return linux_forward_path(linux, reply_port, &request, &reply,
					  (const char *)arg0, 0,
					  0,
					  TASK_RIGHT_RECV);
	}
	case LINUX_SYSCALL_GETPID:
	case LINUX_SYSCALL_GETTID:
		return linux_forward_words(linux, reply_port, number,
					   task_id(task),
					   thread_id(thread_current()), 0);
	case LINUX_SYSCALL_GETGROUPS:
		request.type = LINUX_SYSCALL_GETGROUPS;
		request.words[0] = arg0;
		request.words[1] = 0;
		request.words[2] = 0;
		request.words[3] = 0;
		return linux_forward_groups_out(task, linux, reply_port,
						&request, arg0, arg1);
	case LINUX_SYSCALL_SETGROUPS: {
		struct shared_buffer *buffer;
		struct ipc_message reply;
		const u64 size = arg0 * sizeof(u32);

		if (arg0 > LINUX_MAX_GROUPS ||
		    size > LINUX_MAX_SYSCALL_BUFFER) {
			return (u64)-LINUX_EINVAL;
		}
		if (arg0 != 0 &&
		    vm_read_user(task_vm_space(task), arg1, syscall_copy_buffer,
				 size) != 0) {
			return (u64)-LINUX_EFAULT;
		}
		buffer = buffer_create(size == 0 ? 1 : size);
		if (buffer == 0 ||
		    (size != 0 &&
		     buffer_write(buffer, 0, syscall_copy_buffer, size) != 0)) {
			if (buffer != 0) {
				buffer_release(buffer);
			}
			return (u64)-LINUX_ENOMEM;
		}

		request.type = LINUX_SYSCALL_SETGROUPS;
		request.words[0] = arg0;
		request.words[1] = 0;
		request.words[2] = 0;
		request.words[3] = 0;
		request.cap_type = IPC_CAP_BUFFER;
		request.cap_rights = TASK_RIGHT_RECV | TASK_RIGHT_DUP;
		request.cap_object = buffer;
		if (linux_forward_message(linux, reply_port, &request, &reply) != 0) {
			buffer_release(buffer);
			return (u64)-LINUX_ENOSYS;
		}
		buffer_release(buffer);
		return reply.words[0];
	}
	case LINUX_SYSCALL_IOCTL: {
		struct shared_buffer *buffer = 0;
		u64 output_size = 0;
		u32 value = 0;

		if (arg1 != LINUX_TCGETS && arg1 != LINUX_TCSETS &&
		    arg1 != LINUX_TCSETSW && arg1 != LINUX_TCSETSF &&
		    arg1 != LINUX_TIOCGPGRP && arg1 != LINUX_TIOCSPGRP &&
		    arg1 != LINUX_TIOCGWINSZ) {
			return (u64)-LINUX_ENOTTY;
		}
		if (arg2 == 0) {
			return (u64)-LINUX_EFAULT;
		}
		if (arg1 == LINUX_TIOCSPGRP &&
		    read_current_user(arg2, &value, sizeof(value)) != 0) {
			return (u64)-LINUX_EFAULT;
		}
		if (arg1 == LINUX_TCGETS) {
			output_size = LINUX_TERMIOS_SIZE;
		} else if (arg1 == LINUX_TCSETS ||
			   arg1 == LINUX_TCSETSW ||
			   arg1 == LINUX_TCSETSF) {
			output_size = LINUX_TERMIOS_SIZE;
		} else if (arg1 == LINUX_TIOCGPGRP) {
			output_size = sizeof(value);
		} else if (arg1 == LINUX_TIOCGWINSZ) {
			output_size = 8;
		}
		if (output_size != 0) {
			buffer = buffer_create(output_size);
			if (buffer == 0) {
				return (u64)-LINUX_ENOMEM;
			}
			if ((arg1 == LINUX_TCSETS ||
			     arg1 == LINUX_TCSETSW ||
			     arg1 == LINUX_TCSETSF) &&
			    read_current_user(arg2, syscall_copy_buffer,
					      output_size) != 0) {
				buffer_release(buffer);
				return (u64)-LINUX_EFAULT;
			}
			if ((arg1 == LINUX_TCSETS ||
			     arg1 == LINUX_TCSETSW ||
			     arg1 == LINUX_TCSETSF) &&
			    buffer_write(buffer, 0, syscall_copy_buffer,
					 output_size) != 0) {
				buffer_release(buffer);
				return (u64)-LINUX_ENOMEM;
			}
		}

		request.type = LINUX_SYSCALL_IOCTL;
		request.words[0] = arg0;
		request.words[1] = arg1;
		request.words[2] = value;
		request.words[3] = 0;
		request.cap_type = buffer != 0 ? IPC_CAP_BUFFER : IPC_CAP_NONE;
		request.cap_rights = buffer != 0 ?
				     ((arg1 == LINUX_TCSETS ||
				       arg1 == LINUX_TCSETSW ||
				       arg1 == LINUX_TCSETSF) ?
				      TASK_RIGHT_RECV : TASK_RIGHT_SEND) : 0;
		request.cap_object = buffer;
		if (linux_forward_message(linux, reply_port, &request, &reply) != 0) {
			if (buffer != 0) {
				buffer_release(buffer);
			}
			return (u64)-LINUX_ENOSYS;
		}
		if (reply.words[0] == 0 && buffer != 0 &&
		    arg1 != LINUX_TCSETS &&
		    arg1 != LINUX_TCSETSW &&
		    arg1 != LINUX_TCSETSF &&
		    linux_copy_buffer_to_user(buffer, 0, arg2, output_size) != 0) {
			buffer_release(buffer);
			return (u64)-LINUX_EFAULT;
		}
		if (buffer != 0) {
			buffer_release(buffer);
		}
		return reply.words[0];
	}
	case LINUX_SYSCALL_WAIT4: {
		if (arg1 != 0) {
			return linux_forward_fixed_output_words(
				linux, reply_port, &request, LINUX_SYSCALL_WAIT4,
				(void *)arg1, LINUX_WAIT_STATUS_SIZE, 0, 1,
				arg0, arg2, 0, 0);
		}
		request.type = LINUX_SYSCALL_WAIT4;
		request.words[0] = arg0;
		request.words[1] = arg2;
		request.words[2] = 0;
		request.words[3] = 0;
		if (linux_forward_message(linux, reply_port, &request, &reply) != 0) {
			return (u64)-LINUX_ENOSYS;
		}
		return reply.words[0];
	}
	case LINUX_SYSCALL_FSTAT: {
		if (arg1 == 0) {
			return (u64)-LINUX_EFAULT;
		}
		return linux_forward_fixed_output_words(
			linux, reply_port, &request, LINUX_SYSCALL_FSTAT,
			(void *)arg1, LINUX_STAT_SIZE, 1, 0, arg0,
			LINUX_STAT_SIZE, 0, 0);
	}
	case LINUX_SYSCALL_OPEN:
	case LINUX_SYSCALL_OPENAT: {
		const char *path = number == LINUX_SYSCALL_OPEN ?
				    (const char *)arg0 : (const char *)arg1;
		const u64 dirfd = number == LINUX_SYSCALL_OPEN ?
				  (u64)-100 : arg0;
		const u64 flags = number == LINUX_SYSCALL_OPEN ? arg1 : arg2;
		const u64 mode = number == LINUX_SYSCALL_OPEN ? arg2 : arg3;

		return linux_forward_path_words(linux, reply_port, &request,
						&reply, LINUX_SYSCALL_OPENAT,
						path, dirfd, flags, mode);
	}
	case LINUX_SYSCALL_UNLINK:
	case LINUX_SYSCALL_UNLINKAT:
	case LINUX_SYSCALL_TRUNCATE:
	case LINUX_SYSCALL_RMDIR: {
		const char *path = number == LINUX_SYSCALL_UNLINKAT ?
				    (const char *)arg1 : (const char *)arg0;
		const u64 dirfd = number == LINUX_SYSCALL_UNLINKAT ?
				  arg0 : (u64)-100;
		const u64 flags = number == LINUX_SYSCALL_UNLINKAT ? arg2 : 0;
		const u64 length = number == LINUX_SYSCALL_TRUNCATE ? arg1 : 0;
		const u32 type = number == LINUX_SYSCALL_TRUNCATE ?
				 LINUX_SYSCALL_TRUNCATE :
				 (number == LINUX_SYSCALL_RMDIR ?
				  LINUX_SYSCALL_RMDIR : LINUX_SYSCALL_UNLINKAT);
		const u64 word2 = number == LINUX_SYSCALL_TRUNCATE ?
				  length : flags;

		return linux_forward_path_words(linux, reply_port, &request,
						&reply, type, path, dirfd,
						word2, 0);
	}
	case LINUX_SYSCALL_MKDIR:
	case LINUX_SYSCALL_MKDIRAT:
	case LINUX_SYSCALL_CHMOD:
	case LINUX_SYSCALL_FCHMODAT:
	case LINUX_SYSCALL_CHOWN:
	case LINUX_SYSCALL_LCHOWN:
	case LINUX_SYSCALL_FCHOWNAT: {
		const int is_mkdir = number == LINUX_SYSCALL_MKDIR ||
				     number == LINUX_SYSCALL_MKDIRAT;
		const int is_chown = number == LINUX_SYSCALL_CHOWN ||
				     number == LINUX_SYSCALL_LCHOWN ||
				     number == LINUX_SYSCALL_FCHOWNAT;
		const int is_at = number == LINUX_SYSCALL_MKDIRAT ||
				  number == LINUX_SYSCALL_FCHMODAT ||
				  number == LINUX_SYSCALL_FCHOWNAT;
		const char *path = is_at ? (const char *)arg1 :
				   (const char *)arg0;
		const u64 dirfd = is_at ? arg0 : (u64)-100;
		const u64 mode = is_at ? arg2 : arg1;
		const u64 owner = is_at ? arg2 : arg1;
		const u64 group = is_at ? arg3 : arg2;
		const u64 flags = number == LINUX_SYSCALL_FCHMODAT ? arg3 :
				  (number == LINUX_SYSCALL_FCHOWNAT ?
				   frame->r8 :
				   (number == LINUX_SYSCALL_LCHOWN ?
				    LINUX_AT_SYMLINK_NOFOLLOW : 0));
		const u32 type = is_mkdir ? LINUX_SYSCALL_MKDIRAT :
				 (is_chown ? LINUX_SYSCALL_FCHOWNAT :
				  LINUX_SYSCALL_FCHMODAT);
		const u64 word2 = is_chown ? owner : mode;
		const u64 word3 = is_chown ?
				  ((flags & 0xffffffff) << 32) |
				  (group & 0xffffffff) : flags;

		return linux_forward_path_words(linux, reply_port, &request,
						&reply, type, path, dirfd,
						word2, word3);
	}
	case LINUX_SYSCALL_ACCESS:
	case LINUX_SYSCALL_FACCESSAT: {
		const char *path = number == LINUX_SYSCALL_ACCESS ?
				    (const char *)arg0 : (const char *)arg1;
		const u64 dirfd = number == LINUX_SYSCALL_ACCESS ?
				  (u64)-100 : arg0;
		const u64 mode = number == LINUX_SYSCALL_ACCESS ? arg1 : arg2;

		return linux_forward_path_words(linux, reply_port, &request,
						&reply, LINUX_SYSCALL_FACCESSAT,
						path, dirfd, mode, 0);
	}
	case LINUX_SYSCALL_FACCESSAT2: {
		const char *path = (const char *)arg1;
		const u64 dirfd = arg0;
		const u64 mode = arg2;
		const u64 flags = arg3;

		return linux_forward_path_words(linux, reply_port, &request,
						&reply, LINUX_SYSCALL_FACCESSAT2,
						path, dirfd, mode, flags);
	}
	case LINUX_SYSCALL_MOUNT:
		return linux_forward_mount(linux, reply_port, &request,
					   (const char *)arg1,
					   (const char *)arg2, arg3);
	case LINUX_SYSCALL_UMOUNT2:
		return linux_forward_path_words(linux, reply_port, &request,
						&reply, LINUX_SYSCALL_UMOUNT2,
						(const char *)arg0, arg1, 0, 0);
	case LINUX_SYSCALL_READLINK:
	case LINUX_SYSCALL_READLINKAT: {
		struct shared_buffer *buffer;
		const char *path = number == LINUX_SYSCALL_READLINK ?
				    (const char *)arg0 : (const char *)arg1;
		void *out = number == LINUX_SYSCALL_READLINK ?
			    (void *)arg1 : (void *)arg2;
		const u64 out_size = number == LINUX_SYSCALL_READLINK ?
				     arg2 : arg3;
		const u64 dirfd = number == LINUX_SYSCALL_READLINK ?
				  (u64)-100 : arg0;
		const u64 out_buffer_size = min_u64(out_size,
						    LINUX_EXEC_MAX_PATH);
		u64 result;

		if (path == 0 || out == 0 || out_size == 0) {
			return path == 0 || out == 0 ? (u64)-LINUX_EFAULT :
			       (u64)-LINUX_EINVAL;
		}
		request.type = LINUX_SYSCALL_READLINKAT;
		request.words[0] = dirfd;
		request.words[1] = 0;
		request.words[2] = out_size;
		request.words[3] = 0;
		buffer = linux_forward_path_buffer(linux, reply_port, &request,
						   &reply, path,
						   out_buffer_size, 1,
						   TASK_RIGHT_RECV |
						   TASK_RIGHT_SEND,
						   &result);
		if (buffer == 0) {
			return result;
		}
		if ((long)result >= 0 &&
		    linux_copy_buffer_to_user(buffer, 0, (u64)out, result) != 0) {
			buffer_release(buffer);
			return (u64)-LINUX_EFAULT;
		}
		buffer_release(buffer);
		return result;
	}
	case LINUX_SYSCALL_STAT:
	case LINUX_SYSCALL_LSTAT:
	case LINUX_SYSCALL_NEWFSTATAT: {
		const char *path = number == LINUX_SYSCALL_NEWFSTATAT ?
				    (const char *)arg1 : (const char *)arg0;
		const u64 dirfd = number == LINUX_SYSCALL_NEWFSTATAT ?
				  arg0 : (u64)-100;
		const u64 stat_addr = number == LINUX_SYSCALL_NEWFSTATAT ?
				      arg2 : arg1;
		const u64 flags = number == LINUX_SYSCALL_LSTAT ?
				  LINUX_AT_SYMLINK_NOFOLLOW :
				  (number == LINUX_SYSCALL_NEWFSTATAT ?
				   arg3 : 0);
		u64 result;

		if (path == 0 || stat_addr == 0) {
			return (u64)-LINUX_EFAULT;
		}
		request.type = LINUX_SYSCALL_NEWFSTATAT;
		request.words[0] = dirfd;
		request.words[1] = 0;
		request.words[2] = flags & LINUX_AT_SYMLINK_NOFOLLOW ? 1 : 0;
		request.words[3] = 0;
		result = linux_forward_path(linux, reply_port, &request, &reply,
					    path, 0, 1, TASK_RIGHT_RECV);
		if (result == 0 &&
		    linux_write_stat_user(stat_addr, reply.words[1],
					  reply.words[2],
					  reply.words[3]) != 0) {
			return (u64)-LINUX_EFAULT;
		}
		return result;
	}
	case LINUX_SYSCALL_STATFS: {
		struct shared_buffer *buffer;
		const char *path = (const char *)arg0;
		u64 result;

		if (path == 0 || arg1 == 0) {
			return (u64)-LINUX_EFAULT;
		}
		request.type = LINUX_SYSCALL_STATFS;
		request.words[0] = 0;
		request.words[1] = 0;
		request.words[2] = 0;
		request.words[3] = 0;
		buffer = linux_forward_path_buffer(linux, reply_port, &request,
						   &reply, path,
						   LINUX_STATFS_SIZE, 1,
						   TASK_RIGHT_RECV |
						   TASK_RIGHT_SEND,
						   &result);
		if (buffer == 0) {
			return result;
		}
		if (result == 0 &&
		    linux_copy_buffer_to_user(buffer, 0, arg1,
					      LINUX_STATFS_SIZE) != 0) {
			buffer_release(buffer);
			return (u64)-LINUX_EFAULT;
		}
		buffer_release(buffer);
		return result;
	}
	case LINUX_SYSCALL_STATX: {
		struct shared_buffer *buffer;
		const char *path = (const char *)arg1;
		const u64 statx_addr = frame->r8;
		u64 result;

		if (path == 0 || statx_addr == 0) {
			return (u64)-LINUX_EFAULT;
		}
		request.type = LINUX_SYSCALL_STATX;
		request.words[0] = arg0;
		request.words[1] = 0;
		request.words[2] = arg2;
		request.words[3] = arg3;
		buffer = linux_forward_path_buffer(linux, reply_port, &request,
						   &reply, path,
						   LINUX_STATX_SIZE, 1,
						   TASK_RIGHT_RECV |
						   TASK_RIGHT_SEND,
						   &result);
		if (buffer == 0) {
			return result;
		}
		if (result == 0 &&
		    linux_copy_buffer_to_user(buffer, 0, statx_addr,
					      LINUX_STATX_SIZE) != 0) {
			buffer_release(buffer);
			return (u64)-LINUX_EFAULT;
		}
		buffer_release(buffer);
		return result;
	}
	case LINUX_SYSCALL_EXIT_GROUP:
		return linux_exit_current(arg0);
	default:
		console_printf("linux: unknown syscall=%u rip=%p arg0=%p arg1=%p arg2=%p arg3=%p\n",
			       (u32)number, (const void *)frame->user_rip,
			       (const void *)arg0, (const void *)arg1,
			       (const void *)arg2, (const void *)arg3);
		return (u64)-LINUX_ENOSYS;
	}
}

static u64 linux_syscall_dispatch(struct arch_syscall_frame *frame)
{
	u64 result;

	linux_strace_enter(frame);
	result = linux_syscall_handle(frame);
	linux_strace_log(frame, result);
	return result;
}

struct gdt_ptr {
	u16 limit;
	u64 base;
} __attribute__((packed));

struct tss {
	u32 reserved0;
	u64 rsp0;
	u64 rsp1;
	u64 rsp2;
	u64 reserved1;
	u64 ist[7];
	u64 reserved2;
	u16 reserved3;
	u16 iomap_base;
} __attribute__((packed));

struct arch_user_cpu {
	u64 gdt[7];
	struct tss tss;
	u8 interrupt_stack[16384] __attribute__((aligned(16)));
};

static struct arch_user_cpu user_cpus[ARCH_USER_MAX_CPUS];
u64 arch_current_syscall_stack[ARCH_USER_MAX_CPUS];

extern void arch_syscall_entry(void);

static int str_eq(const char *left, const char *right)
{
	while (*left != '\0' && *right != '\0') {
		if (*left++ != *right++) {
			return 0;
		}
	}

	return *left == *right;
}

static void gdt_set_tss(struct arch_user_cpu *cpu, u32 index, u64 base,
			u32 limit)
{
	cpu->gdt[index] = ((u64)(limit & 0xffff)) |
			  ((base & 0xffffff) << 16) |
			  ((u64)0x89 << 40) |
			  ((u64)((limit >> 16) & 0x0f) << 48) |
			  (((base >> 24) & 0xff) << 56);
	cpu->gdt[index + 1] = base >> 32;
}

static void arch_enable_sse(void)
{
	enum {
		CR4_OSFXSR = 1u << 9,
		CR4_OSXMMEXCPT = 1u << 10,
		CR4_OSXSAVE = 1u << 18,
		CPUID1_ECX_XSAVE = 1u << 26,
		CPUID1_ECX_AVX = 1u << 28,
		XCR0_X87 = 1u << 0,
		XCR0_SSE = 1u << 1,
		XCR0_AVX = 1u << 2,
	};
	u64 cr0;
	u64 cr4;
	u32 eax;
	u32 ebx;
	u32 ecx;
	u32 edx;
	u64 xstate_mask = XCR0_X87 | XCR0_SSE;
	int use_xsave = 0;

	__asm__ volatile ("movq %%cr0, %0" : "=r"(cr0));
	cr0 |= 1u << 1;
	cr0 |= 1u << 5;
	cr0 &= ~(1u << 2);
	cr0 &= ~(1u << 3);
	__asm__ volatile ("movq %0, %%cr0" : : "r"(cr0) : "memory");

	__asm__ volatile ("movq %%cr4, %0" : "=r"(cr4));
	cr4 |= CR4_OSFXSR | CR4_OSXMMEXCPT;

	__asm__ volatile ("cpuid"
			  : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
			  : "a"(1), "c"(0)
			  : "memory");
	if ((ecx & (CPUID1_ECX_XSAVE | CPUID1_ECX_AVX)) ==
	    (CPUID1_ECX_XSAVE | CPUID1_ECX_AVX)) {
		u32 xcr_low;
		u32 xcr_high;

		cr4 |= CR4_OSXSAVE;
		__asm__ volatile ("movq %0, %%cr4" : : "r"(cr4) : "memory");
		xstate_mask |= XCR0_AVX;
		xcr_low = (u32)xstate_mask;
		xcr_high = (u32)(xstate_mask >> 32);
		__asm__ volatile ("xsetbv"
				  :
				  : "a"(xcr_low), "d"(xcr_high), "c"(0)
				  : "memory");
		use_xsave = 1;
	} else {
		cr4 &= ~((u64)CR4_OSXSAVE);
		__asm__ volatile ("movq %0, %%cr4" : : "r"(cr4) : "memory");
	}

	arch_thread_fpu_init_cpu(xstate_mask, use_xsave);
}

void arch_user_init_cpu(u32 cpu_id)
{
	struct arch_user_cpu *cpu = &user_cpus[cpu_id];
	const struct gdt_ptr ptr = {
		.limit = sizeof(cpu->gdt) - 1,
		.base = (u64)cpu->gdt,
	};

	cpu->gdt[0] = 0;
	cpu->gdt[1] = 0x00af9a000000ffff;
	cpu->gdt[2] = 0x00af92000000ffff;
	cpu->gdt[3] = 0x00aff2000000ffff;
	cpu->gdt[4] = 0x00affa000000ffff;

	cpu->tss.rsp0 = (u64)(cpu->interrupt_stack + sizeof(cpu->interrupt_stack));
	cpu->tss.iomap_base = sizeof(cpu->tss);
	gdt_set_tss(cpu, 5, (u64)&cpu->tss, sizeof(cpu->tss) - 1);

	__asm__ volatile ("lgdt %0" : : "m"(ptr));
	/* APs arrive on the trampoline GDT; reload CS before interrupts return. */
	__asm__ volatile (
		"pushq %[code]\n"
		"leaq 1f(%%rip), %%rax\n"
		"pushq %%rax\n"
		"lretq\n"
		"1:\n"
		:
		: [code] "i"(GDT_KERNEL_CODE)
		: "rax", "memory");
	__asm__ volatile (
		"movw %0, %%ds\n"
		"movw %0, %%es\n"
		"movw %0, %%ss\n"
		:
		: "r"((u16)GDT_KERNEL_DATA)
		: "memory");
	__asm__ volatile ("ltr %0" : : "r"((u16)GDT_TSS));
	arch_enable_sse();

	arch_wrmsr(MSR_STAR, ((u64)0x13 << 48) | ((u64)GDT_KERNEL_CODE << 32));
	arch_wrmsr(MSR_LSTAR, (u64)arch_syscall_entry);
	arch_wrmsr(MSR_FMASK, 0x200);
	arch_wrmsr(MSR_EFER, arch_rdmsr(MSR_EFER) | EFER_SCE);

	if (cpu_id == 0) {
		console_printf("user: gdt/tss/syscall ready\n");
	}
}

void arch_user_init(void)
{
	arch_user_init_cpu(arch_smp_current_cpu_id());
}

void arch_user_set_kernel_stack(u64 stack)
{
	const u32 cpu_id = arch_smp_current_cpu_id();

	user_cpus[cpu_id].tss.rsp0 = stack;
	arch_current_syscall_stack[cpu_id] = stack;
	arch_smp_set_syscall_stack(stack);
}

void arch_user_set_fs_base(u64 fs_base)
{
	arch_wrmsr(MSR_FS_BASE, fs_base);
}

void arch_user_enter(u64 entry, u64 stack)
{
	console_printf("user: enter rip=%p rsp=%p\n",
		       (const void *)entry, (const void *)stack);
	arch_thread_fpu_reset_current();

	__asm__ volatile (
		"cli\n"
		"movw $0x1b, %%ax\n"
		"movw %%ax, %%ds\n"
		"movw %%ax, %%es\n"
		"pushq $0x1b\n"
		"pushq %[stack]\n"
		"pushfq\n"
		"popq %%rax\n"
		"orq $0x200, %%rax\n"
		"pushq %%rax\n"
		"pushq $0x23\n"
		"pushq %[entry]\n"
		"iretq\n"
		:
		: [entry] "r"(entry),
		  [stack] "r"(stack)
		: "rax", "memory");

	for (;;) {
		__asm__ volatile ("hlt");
	}
}

struct native_syscall_args {
	struct arch_syscall_frame *frame;
	u64 arg0;
	u64 arg1;
	u64 arg2;
	u64 arg3;
};

typedef u64 (*native_syscall_fn)(const struct native_syscall_args *args);

struct native_syscall_entry {
	i64 number;
	const char *name;
	native_syscall_fn handler;
};

struct user_vm_stats {
	u64 total_frames;
	u64 free_frames;
};

static u64 native_sys_exit(const struct native_syscall_args *args)
{
	console_printf("syscall: exit status=%u\n", (u32)args->arg0);
	__asm__ volatile ("sti");
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
	char name[LINUX_EXEC_MAX_PATH];
	struct task_launch_cap caps[8];

	if (copy_cstr_from_user(name, (const char *)args->arg0,
				sizeof(name)) != 0 ||
	    args->arg2 > sizeof(caps) / sizeof(caps[0])) {
		return (u64)-1;
	}
	if (args->arg2 != 0 &&
	    (args->arg1 == 0 ||
	     read_current_user(args->arg1, caps,
			       args->arg2 * sizeof(caps[0])) != 0)) {
		return (u64)-1;
	}
	return server_launch_module_with_caps(name, task_current(),
					      args->arg2 != 0 ? caps : 0,
					      args->arg2);
}

static u64 native_sys_task_create(const struct native_syscall_args *args)
{
	char name[LINUX_EXEC_MAX_PATH];

	if (copy_cstr_from_user(name, (const char *)args->arg0,
				sizeof(name)) != 0) {
		return (u64)-1;
	}
	return server_task_create(task_current(), name);
}

static u64 native_sys_task_id(const struct native_syscall_args *args)
{
	struct task *task =
		task_from_handle(task_current(), args->arg0, TASK_RIGHT_SEND);

	return task != 0 ? task_id(task) : (u64)-1;
}

static u64 native_sys_task_info(const struct native_syscall_args *args)
{
	u64 pid_threads_flags = 0;
	u64 name_words[2] = { 0, 0 };

	if (args->arg1 == 0 || args->arg2 == 0 ||
	    task_info_at(args->arg0, &pid_threads_flags, name_words) != 0 ||
	    write_current_user(args->arg1, &pid_threads_flags,
			       sizeof(pid_threads_flags)) != 0 ||
	    write_current_user(args->arg2, name_words,
			       sizeof(name_words)) != 0) {
		return (u64)-1;
	}

	return 0;
}

static u64 native_sys_task_map(const struct native_syscall_args *sys_args)
{
	u64 args[6];
	u64 flags;
	u64 result;

	if (sys_args->arg0 == 0 ||
	    read_current_user(sys_args->arg0, args, sizeof(args)) != 0 ||
	    args[3] > LINUX_MAX_SYSCALL_BUFFER) {
		return (u64)-1;
	}

	flags = spin_lock_irqsave(&syscall_copy_lock);
	if (read_current_user(args[2], syscall_copy_buffer, args[3]) != 0) {
		spin_unlock_irqrestore(&syscall_copy_lock, flags);
		return (u64)-1;
	}
	result = (u64)server_task_map(task_current(), args[0], args[1],
				      syscall_copy_buffer, args[3],
				      args[4], (u32)args[5]);
	spin_unlock_irqrestore(&syscall_copy_lock, flags);
	return result;
}

static u64 native_sys_task_grant(const struct native_syscall_args *args)
{
	return (u64)server_task_grant(task_current(), args->arg0, args->arg1,
				      (u32)args->arg2);
}

static u64 native_sys_task_start(const struct native_syscall_args *args)
{
	return (u64)server_task_start(task_current(), args->arg0, args->arg1);
}

static u64 native_sys_task_write(const struct native_syscall_args *sys_args)
{
	u64 args[4];
	u64 flags;
	u64 result;

	if (sys_args->arg0 == 0 ||
	    read_current_user(sys_args->arg0, args, sizeof(args)) != 0 ||
	    args[3] > LINUX_MAX_SYSCALL_BUFFER) {
		return (u64)-1;
	}

	flags = spin_lock_irqsave(&syscall_copy_lock);
	if (read_current_user(args[2], syscall_copy_buffer, args[3]) != 0) {
		spin_unlock_irqrestore(&syscall_copy_lock, flags);
		return (u64)-1;
	}
	result = (u64)server_task_write(task_current(), args[0], args[1],
					syscall_copy_buffer, args[3]);
	spin_unlock_irqrestore(&syscall_copy_lock, flags);
	return result;
}

static u64 native_sys_task_alloc(const struct native_syscall_args *sys_args)
{
	u64 args[4];

	if (sys_args->arg0 == 0 ||
	    read_current_user(sys_args->arg0, args, sizeof(args)) != 0) {
		return (u64)-1;
	}

	return (u64)server_task_alloc(task_current(), args[0], args[1],
				      args[2], (u32)args[3]);
}

static u64 native_sys_task_clone_range(const struct native_syscall_args *sys_args)
{
	u64 args[5];

	if (sys_args->arg0 == 0 ||
	    read_current_user(sys_args->arg0, args, sizeof(args)) != 0) {
		return (u64)-1;
	}

	return (u64)server_task_clone_range(task_current(), args[0],
					    args[1], args[2], args[3],
					    (u32)args[4]);
}

static u64 native_sys_task_start_at(const struct native_syscall_args *args)
{
	return (u64)server_task_start_at(task_current(), args->arg0, args->arg1,
					 args->arg2);
}

static u64 native_sys_task_kill(const struct native_syscall_args *args)
{
	return (u64)server_task_kill(task_current(), args->arg0);
}

static u64 native_sys_buffer_create(const struct native_syscall_args *args)
{
	struct shared_buffer *buffer = buffer_create(args->arg0);
	if (buffer == 0) {
		return (u64)-1;
	}
	const u64 handle =
		task_grant_buffer(task_current(), buffer,
				  TASK_RIGHT_SEND | TASK_RIGHT_RECV |
				  TASK_RIGHT_DUP);

	buffer_release(buffer);
	return handle != 0 ? handle : (u64)-1;
}

static u64 native_sys_buffer_read(const struct native_syscall_args *sys_args)
{
	u64 args[4];
	u64 flags;

	if (sys_args->arg0 == 0 ||
	    read_current_user(sys_args->arg0, args, sizeof(args)) != 0) {
		return (u64)-1;
	}

	struct shared_buffer *buffer =
		task_buffer_from_handle(task_current(), args[0],
					TASK_RIGHT_RECV);
	if (buffer == 0) {
		return (u64)-1;
	}
	flags = spin_lock_irqsave(&syscall_copy_lock);
	for (u64 done = 0; done < args[3];) {
		const u64 chunk =
			min_u64(args[3] - done, LINUX_MAX_SYSCALL_BUFFER);

		if (buffer_read(buffer, args[1] + done, syscall_copy_buffer,
				chunk) != 0 ||
		    write_current_user(args[2] + done, syscall_copy_buffer,
				       chunk) != 0) {
			spin_unlock_irqrestore(&syscall_copy_lock, flags);
			return (u64)-1;
		}
		done += chunk;
	}
	spin_unlock_irqrestore(&syscall_copy_lock, flags);
	return 0;
}

static u64 native_sys_buffer_write(const struct native_syscall_args *sys_args)
{
	u64 args[4];
	u64 flags;
	u64 result;

	if (sys_args->arg0 == 0 ||
	    read_current_user(sys_args->arg0, args, sizeof(args)) != 0) {
		return (u64)-1;
	}
	flags = spin_lock_irqsave(&syscall_copy_lock);
	struct shared_buffer *buffer =
		task_buffer_from_handle(task_current(), args[0],
					TASK_RIGHT_SEND);
	if (buffer == 0) {
		spin_unlock_irqrestore(&syscall_copy_lock, flags);
		return (u64)-1;
	}
	result = 0;
	for (u64 done = 0; done < args[3];) {
		const u64 chunk =
			min_u64(args[3] - done, LINUX_MAX_SYSCALL_BUFFER);

		if (read_current_user(args[2] + done, syscall_copy_buffer,
				      chunk) != 0 ||
		    buffer_write(buffer, args[1] + done, syscall_copy_buffer,
				 chunk) != 0) {
			result = (u64)-1;
			break;
		}
		done += chunk;
	}
	spin_unlock_irqrestore(&syscall_copy_lock, flags);
	return result;
}

static u64 native_sys_port_create(const struct native_syscall_args *args)
{
	char name[LINUX_EXEC_MAX_PATH];

	if (copy_cstr_from_user(name, (const char *)args->arg0,
				sizeof(name)) != 0) {
		return (u64)-1;
	}
	return task_grant_port(task_current(), ipc_port_create_private(name),
			       TASK_RIGHT_SEND | TASK_RIGHT_RECV |
			       TASK_RIGHT_DUP);
}

static u64 native_sys_handle_close(const struct native_syscall_args *args)
{
	return (u64)task_close_handle(task_current(), args->arg0);
}

static u64 native_sys_boot_module_read(const struct native_syscall_args *args)
{
	u64 done = 0;
	u64 flags;

	if (args->arg1 == 0) {
		return server_boot_module_size();
	}

	flags = spin_lock_irqsave(&syscall_copy_lock);
	while (done < args->arg2) {
		const u64 chunk = min_u64(args->arg2 - done,
					  LINUX_MAX_SYSCALL_BUFFER);

		if (server_boot_module_read(args->arg0 + done,
					    syscall_copy_buffer, chunk) != 0 ||
		    write_current_user(args->arg1 + done, syscall_copy_buffer,
				       chunk) != 0) {
			spin_unlock_irqrestore(&syscall_copy_lock, flags);
			return (u64)-1;
		}
		done += chunk;
	}
	spin_unlock_irqrestore(&syscall_copy_lock, flags);
	return 0;
}

static u64 native_sys_console_read(const struct native_syscall_args *args)
{
	u64 nread;
	u64 flags;

	if (args->arg0 == 0 || args->arg1 > LINUX_MAX_SYSCALL_BUFFER) {
		return (u64)-1;
	}

	nread = console_read((char *)console_input_buffer, args->arg1);
	flags = spin_lock_irqsave(&syscall_copy_lock);
	if (write_current_user(args->arg0, console_input_buffer, nread) != 0) {
		spin_unlock_irqrestore(&syscall_copy_lock, flags);
		return (u64)-1;
	}
	spin_unlock_irqrestore(&syscall_copy_lock, flags);
	return nread;
}

static u64 native_sys_early_console_write(const struct native_syscall_args *args)
{
	return console_write_user(args->arg0, args->arg1) == 0 ? 0 : (u64)-1;
}

static u64 native_sys_early_console_log(const struct native_syscall_args *args)
{
	return console_log_user(args->arg0, args->arg1) == 0 ? 0 : (u64)-1;
}

static u64 native_sys_early_console_logs_to_ring(
	const struct native_syscall_args *args)
{
	(void)args;
	console_logs_to_ring();
	return 0;
}

static u64 native_sys_ipc_stats(const struct native_syscall_args *args)
{
	struct ipc_stats stats;

	if (args->arg0 == 0) {
		return (u64)-1;
	}
	ipc_stats_snapshot(&stats);
	return write_current_user(args->arg0, &stats, sizeof(stats)) == 0 ?
	       0 : (u64)-1;
}

static u64 native_sys_vm_stats(const struct native_syscall_args *args)
{
	struct user_vm_stats stats;

	if (args->arg0 == 0) {
		return (u64)-1;
	}
	stats.total_frames = vm_rpc_total_frames();
	stats.free_frames = vm_rpc_free_frames();
	return write_current_user(args->arg0, &stats, sizeof(stats)) == 0 ?
	       0 : (u64)-1;
}

static u64 native_sys_ipc_send(const struct native_syscall_args *args)
{
	struct user_ipc_message user_message;
	struct ipc_port *port =
		task_port_from_handle(task_current(), args->arg0,
				      TASK_RIGHT_SEND);

	if (args->arg1 == 0 ||
	    read_current_user(args->arg1, &user_message,
			      sizeof(user_message)) != 0) {
		return (u64)-1;
	}

	struct ipc_message message = { .protocol = 0, .type = 0 };

	if (user_message_to_ipc(&user_message, &message) != 0) {
		ipc_message_release(&message);
		return (u64)-1;
	}

	const int result = ipc_send(port, &message);
	ipc_message_release(&message);
	return (u64)result;
}

static u64 native_sys_ipc_recv(const struct native_syscall_args *args)
{
	struct user_ipc_message user_message;
	struct ipc_port *port =
		task_port_from_handle(task_current(), args->arg0,
				      TASK_RIGHT_RECV);
	struct ipc_message message;

	if (args->arg1 == 0 || ipc_recv(port, &message) != 0) {
		return (u64)-1;
	}

	ipc_message_to_user(&message, &user_message);
	if (write_current_user(args->arg1, &user_message,
			       sizeof(user_message)) != 0) {
		ipc_message_release(&message);
		return (u64)-1;
	}
	ipc_message_release(&message);
	return 0;
}

static u64 native_sys_ipc_call(const struct native_syscall_args *args)
{
	struct user_ipc_message user_request;
	struct user_ipc_message user_reply;
	struct ipc_port *port =
		task_port_from_handle(task_current(), args->arg0,
				      TASK_RIGHT_SEND);
	struct ipc_port *reply_port = task_reply_port(task_current());
	struct ipc_message message = { .protocol = 0, .type = 0 };
	struct ipc_message reply;

	if (args->arg1 == 0 || args->arg2 == 0 || port == 0 ||
	    reply_port == 0 ||
	    read_current_user(args->arg1, &user_request,
			      sizeof(user_request)) != 0) {
		return (u64)-1;
	}

	if (user_message_to_ipc(&user_request, &message) != 0) {
		ipc_message_release(&message);
		return (u64)-1;
	}

	ipc_port_release(message.reply_port);
	message.reply_port = reply_port;
	ipc_port_retain(message.reply_port);
	if (ipc_send(port, &message) != 0) {
		ipc_message_release(&message);
		return (u64)-1;
	}
	ipc_message_release(&message);
	if (ipc_recv(reply_port, &reply) != 0) {
		return (u64)-1;
	}

	ipc_message_to_user(&reply, &user_reply);
	if (write_current_user(args->arg2, &user_reply,
			       sizeof(user_reply)) != 0) {
		ipc_message_release(&reply);
		return (u64)-1;
	}
	ipc_message_release(&reply);
	return 0;
}

static const struct native_syscall_entry native_syscalls[] = {
	{ SYSCALL_EXIT, "exit", native_sys_exit },
	{ SYSCALL_TIMER_TICKS, "timer_ticks", native_sys_timer_ticks },
	{ SYSCALL_LAUNCH_MODULE, "launch_module", native_sys_launch_module },
	{ SYSCALL_PORT_CREATE, "port_create", native_sys_port_create },
	{ SYSCALL_IPC_SEND, "ipc_send", native_sys_ipc_send },
	{ SYSCALL_IPC_RECV, "ipc_recv", native_sys_ipc_recv },
	{ SYSCALL_IPC_CALL, "ipc_call", native_sys_ipc_call },
	{ SYSCALL_HANDLE_CLOSE, "handle_close", native_sys_handle_close },
	{ SYSCALL_BOOT_MODULE_READ, "boot_module_read",
	  native_sys_boot_module_read },
	{ SYSCALL_CLOCK_MONOTONIC_NS, "clock_monotonic_ns",
	  native_sys_clock_monotonic_ns },
	{ SYSCALL_SLEEP_NS, "sleep_ns", native_sys_sleep_ns },
	{ SYSCALL_TASK_CREATE, "task_create", native_sys_task_create },
	{ SYSCALL_TASK_MAP, "task_map", native_sys_task_map },
	{ SYSCALL_TASK_GRANT, "task_grant", native_sys_task_grant },
	{ SYSCALL_TASK_START, "task_start", native_sys_task_start },
	{ SYSCALL_BUFFER_CREATE, "buffer_create", native_sys_buffer_create },
	{ SYSCALL_BUFFER_READ, "buffer_read", native_sys_buffer_read },
	{ SYSCALL_BUFFER_WRITE, "buffer_write", native_sys_buffer_write },
	{ SYSCALL_TASK_WRITE, "task_write", native_sys_task_write },
	{ SYSCALL_TASK_START_AT, "task_start_at", native_sys_task_start_at },
	{ SYSCALL_TASK_ID, "task_id", native_sys_task_id },
	{ SYSCALL_TASK_INFO, "task_info", native_sys_task_info },
	{ SYSCALL_TASK_ALLOC, "task_alloc", native_sys_task_alloc },
	{ SYSCALL_TASK_CLONE_RANGE, "task_clone_range",
	  native_sys_task_clone_range },
	{ SYSCALL_CONSOLE_READ, "console_read", native_sys_console_read },
	{ SYSCALL_TASK_KILL, "task_kill", native_sys_task_kill },
	{ SYSCALL_EARLY_CONSOLE_WRITE, "early_console_write",
	  native_sys_early_console_write },
	{ SYSCALL_EARLY_CONSOLE_LOG, "early_console_log",
	  native_sys_early_console_log },
	{ SYSCALL_EARLY_CONSOLE_LOGS_TO_RING, "early_console_logs_to_ring",
	  native_sys_early_console_logs_to_ring },
	{ SYSCALL_IPC_STATS, "ipc_stats", native_sys_ipc_stats },
	{ SYSCALL_VM_STATS, "vm_stats", native_sys_vm_stats },
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

u64 arch_syscall_dispatch(struct arch_syscall_frame *frame)
{
	struct task *current = task_current();
	const i64 number = (i64)frame->number;

	if (task_is_killing(current) && number != SYSCALL_EXIT) {
		if (task_uses_linux_personality(current)) {
			linux_vfork_complete_task(task_id(current));
		}
		__asm__ volatile ("sti");
		thread_exit();
	}
	if (number >= 0) {
		return linux_syscall_dispatch(frame);
	}
	if (task_uses_linux_personality(current)) {
		linux_negative_syscall_dump(frame);
	}

	const struct native_syscall_entry *entry = native_syscall_lookup(number);
	if (entry == 0) {
		console_printf("syscall: unknown native number=%d\n",
			       (i32)number);
		return (u64)-1;
	}

	const struct native_syscall_args args = {
		.frame = frame,
		.arg0 = frame->arg0,
		.arg1 = frame->arg1,
		.arg2 = frame->arg2,
		.arg3 = frame->arg3,
	};

	return entry->handler(&args);
}
