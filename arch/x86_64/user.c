#include <arch/user.h>
#include <arch/interrupts.h>
#include <arch/io.h>
#include <arch/power.h>
#include <arch/smp.h>
#include <arch/thread.h>
#include "buffer.h"
#include "cmdline.h"
#include "console.h"
#include "ipc.h"
#include "linux_abi.h"
#include "sched.h"
#include "slab.h"
#include "spinlock.h"
#include "server.h"
#include "task_lifecycle.h"
#include "timer.h"
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
	RFLAGS_IF = 1u << 9,
	RFLAGS_AC = 1u << 18,
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
	SYSCALL_MACHINE_POWER = -64,
	SYSCALL_TASK_CLEAR = -66,
	SYSCALL_HW_PORT_IN8 = -68,
	SYSCALL_HW_PORT_OUT8 = -70,
	SYSCALL_IPC_TRY_RECV = -72,
	SYSCALL_HW_PORT_IN16 = -74,
	SYSCALL_HW_PORT_OUT16 = -76,
	SYSCALL_HW_PORT_IN32 = -78,
	SYSCALL_HW_PORT_OUT32 = -80,
	SYSCALL_HW_PCI_BAR_GRANT = -82,
	SYSCALL_HW_MMIO_READ8 = -84,
	SYSCALL_HW_MMIO_WRITE8 = -86,
	SYSCALL_HW_MMIO_READ16 = -88,
	SYSCALL_HW_MMIO_WRITE16 = -90,
	SYSCALL_HW_MMIO_READ32 = -92,
	SYSCALL_HW_MMIO_WRITE32 = -94,
	SYSCALL_BUFFER_PHYS = -96,
	SYSCALL_CMDLINE_HAS = -98,
	SYSCALL_SCHED_STATS = -100,
	SYSCALL_HW_PCI_IRQ_GRANT = -102,
	SYSCALL_HW_IRQ_BIND = -104,
	SYSCALL_HW_IRQ_ACK = -106,
	SYSCALL_HW_IRQ_MASK = -108,
	SYSCALL_SCHED_THREAD_INFO = -110,
	SYSCALL_HANDLE_FIND = -112,
	SYSCALL_TASK_GRANT_TAGGED = -114,
	SYSCALL_TASK_HANDLE_FIND = -116,
	SYSCALL_SCHED_POLICY_GRANT = -118,
	SYSCALL_SCHED_POLICY_GET = -120,
	SYSCALL_SCHED_POLICY_SET = -122,
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
	LINUX_SYSCALL_RT_SIGRETURN = 15,
	LINUX_SYSCALL_RT_SIGTIMEDWAIT = 128,
	LINUX_SYSCALL_IOCTL = 16,
	LINUX_SYSCALL_WRITEV = 20,
	LINUX_SYSCALL_ACCESS = 21,
	LINUX_SYSCALL_PIPE = 22,
	LINUX_SYSCALL_SELECT = 23,
	LINUX_SYSCALL_SCHED_YIELD = 24,
	LINUX_SYSCALL_NANOSLEEP = 35,
	LINUX_SYSCALL_DUP = 32,
	LINUX_SYSCALL_DUP2 = 33,
	LINUX_SYSCALL_ALARM = 37,
	LINUX_SYSCALL_SETITIMER = 38,
	LINUX_SYSCALL_TRUNCATE = 76,
	LINUX_SYSCALL_FTRUNCATE = 77,
	LINUX_SYSCALL_RMDIR = 84,
	LINUX_SYSCALL_LINK = 86,
	LINUX_SYSCALL_UNLINK = 87,
	LINUX_SYSCALL_SENDFILE = 40,
	LINUX_SYSCALL_SOCKET = 41,
	LINUX_SYSCALL_CONNECT = 42,
	LINUX_SYSCALL_ACCEPT = 43,
	LINUX_SYSCALL_SENDTO = 44,
	LINUX_SYSCALL_RECVFROM = 45,
	LINUX_SYSCALL_SENDMSG = 46,
	LINUX_SYSCALL_RECVMSG = 47,
	LINUX_SYSCALL_SHUTDOWN = 48,
	LINUX_SYSCALL_BIND = 49,
	LINUX_SYSCALL_LISTEN = 50,
	LINUX_SYSCALL_GETSOCKNAME = 51,
	LINUX_SYSCALL_GETPEERNAME = 52,
	LINUX_SYSCALL_SETSOCKOPT = 54,
	LINUX_SYSCALL_GETSOCKOPT = 55,
	LINUX_SYSCALL_FCNTL = 72,
	LINUX_SYSCALL_FLOCK = 73,
	LINUX_SYSCALL_GETCWD = 79,
	LINUX_SYSCALL_CHDIR = 80,
	LINUX_SYSCALL_RENAME = 82,
	LINUX_SYSCALL_MKDIR = 83,
	LINUX_SYSCALL_SYMLINK = 88,
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
	LINUX_SYSCALL_MKNOD = 133,
	LINUX_SYSCALL_STATFS = 137,
	LINUX_SYSCALL_FSTATFS = 138,
	LINUX_SYSCALL_GETPRIORITY = 140,
	LINUX_SYSCALL_SETPRIORITY = 141,
	LINUX_SYSCALL_SCHED_SETPARAM = 142,
	LINUX_SYSCALL_SCHED_GETPARAM = 143,
	LINUX_SYSCALL_SCHED_SETSCHEDULER = 144,
	LINUX_SYSCALL_SCHED_GETSCHEDULER = 145,
	LINUX_SYSCALL_SCHED_GET_PRIORITY_MAX = 146,
	LINUX_SYSCALL_SCHED_GET_PRIORITY_MIN = 147,
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
	LINUX_SYSCALL_SCHED_SETAFFINITY = 203,
	LINUX_SYSCALL_SCHED_GETAFFINITY = 204,
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
	LINUX_AT_EMPTY_PATH = 0x1000,
	LINUX_SYSCALL_PSELECT6 = 270,
	LINUX_SYSCALL_PPOLL = 271,
	LINUX_SYSCALL_ACCEPT4 = 288,
	LINUX_SYSCALL_SET_ROBUST_LIST = 273,
	LINUX_SYSCALL_UTIMENSAT = 280,
	LINUX_SYSCALL_DUP3 = 292,
	LINUX_SYSCALL_PIPE2 = 293,
	LINUX_SYSCALL_CLOSE_RANGE = 436,
	LINUX_SYSCALL_OPENAT = 257,
	LINUX_SYSCALL_MKDIRAT = 258,
	LINUX_SYSCALL_MKNODAT = 259,
	LINUX_SYSCALL_FCHOWNAT = 260,
	LINUX_SYSCALL_LINKAT = 265,
	LINUX_SYSCALL_RENAMEAT = 264,
	LINUX_SYSCALL_UNLINKAT = 263,
	LINUX_SYSCALL_SYMLINKAT = 266,
	LINUX_SYSCALL_PRLIMIT64 = 302,
	LINUX_SYSCALL_RENAMEAT2 = 316,
	LINUX_SYSCALL_GETRANDOM = 318,
	LINUX_SYSCALL_STATX = 332,
	LINUX_SYSCALL_EXIT_GROUP = 231,
	LINUX_SYSCALL_FACCESSAT2 = 439,
	LINUX_RPC_SIGNAL_PENDING = 1007,
	LINUX_RPC_SIGNAL_DEQUEUE = 1008,
	LINUX_CLONE_VFORK = 0x00004000,
	LINUX_EPERM = 1,
	LINUX_ENOENT = 2,
	LINUX_ESRCH = 3,
	LINUX_EINTR = 4,
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
	LINUX_UTIME_NOW = 0x3fffffff,
	LINUX_UTIME_OMIT = 0x3ffffffe,
	LINUX_POLLIN = 0x0001,
	LINUX_POLLOUT = 0x0004,
	LINUX_ARCH_SET_FS = 0x1002,
	LINUX_FUTEX_WAIT = 0,
	LINUX_FUTEX_WAKE = 1,
	LINUX_F_GETLK = 5,
	LINUX_F_UNLCK = 2,
	LINUX_TCGETS = 0x5401,
	LINUX_TCSETS = 0x5402,
	LINUX_TCSETSW = 0x5403,
	LINUX_TCSETSF = 0x5404,
	LINUX_TIOCGPGRP = 0x540f,
	LINUX_TIOCSPGRP = 0x5410,
	LINUX_TIOCGWINSZ = 0x5413,
	LINUX_TERMIOS_SIZE = 60,
	LINUX_SIOCGIFFLAGS = 0x8913,
	LINUX_SIOCGIFHWADDR = 0x8927,
	LINUX_SIOCGIFMTU = 0x8921,
	LINUX_SIOCGIFINDEX = 0x8933,
	LINUX_IFREQ_SIZE = 40,
	ARCH_USER_MAX_CPUS = 8,
	LINUX_MAX_SYSCALL_BUFFER = 65536,
	LINUX_MAX_SHARED_BUFFER = 1024 * 1024,
	LINUX_MAX_SOCKADDR = 128,
	LINUX_IOV_MAX = 1024,
	LINUX_FDSET_BITS = 1024,
	LINUX_FDSET_BYTES = LINUX_FDSET_BITS / 8,
	LINUX_SIGNAL_FRAME_MAGIC = 0x534947424e555858ull,
	LINUX_MSGHDR_SIZE = 56,
	LINUX_MSGHDR_NAME_OFF = 0,
	LINUX_MSGHDR_NAMELEN_OFF = 8,
	LINUX_MSGHDR_IOV_OFF = 16,
	LINUX_MSGHDR_IOVLEN_OFF = 24,
	LINUX_MSGHDR_CONTROL_OFF = 32,
	LINUX_MSGHDR_CONTROLLEN_OFF = 40,
	LINUX_MSGHDR_FLAGS_OFF = 48,
	LINUX_EXEC_MAX_PATH = 4096,
	LINUX_EXEC_MAX_STRING = 128 * 1024,
	LINUX_EXEC_MAX_STRING_BYTES = 384 * 1024,
	LINUX_MAX_GROUPS = 65536,
	LINUX_PROC_CMDLINE_MAX = 4096,
	LINUX_SHEBANG_MAX = 256,
	LINUX_SHEBANG_MAX_DEPTH = 4,
	LINUX_EXEC_PREPARE_PATH_OFF = 0,
	LINUX_EXEC_PREPARE_IMAGE_OFF = LINUX_EXEC_MAX_PATH,
	LINUX_EXEC_PREPARE_VECTOR_OFF = LINUX_EXEC_MAX_PATH + LINUX_SHEBANG_MAX,
	LINUX_EXEC_PREPARE_BUFFER_SIZE =
		LINUX_EXEC_PREPARE_VECTOR_OFF + LINUX_EXEC_MAX_STRING_BYTES +
		((LINUX_EXEC_MAX_STRING_BYTES / 8) * sizeof(u64)) +
		2 * sizeof(u64),
	LINUX_EXEC_DYN_LOAD_BIAS = 0x400000,
	LINUX_EXEC_INTERP_LOAD_BIAS = 0x600000,
	LINUX_EXEC_STACK_TOP = 0x800000,
	LINUX_EXEC_STACK_PAGES = 128,
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
	LINUX_S_IFCHR = 0020000,
	LINUX_S_IFIFO = 0010000,
	LINUX_S_IFLNK = 0120000,
	LINUX_S_IFREG = 0100000,
	LINUX_INITIAL_BRK = 0x900000,
	LINUX_MAX_BRK = 0x10000000,
	LINUX_MAP_FIXED_MIN = 0x10000,
	LINUX_MMAP_BASE = 0x10000000,
	LINUX_MMAP_LIMIT = 0x20000000,
	LINUX_EXEC_MAX_POINTERS = LINUX_EXEC_MAX_STRING_BYTES / 8,
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
	USER_VFS_TYPE_FIFO = 4,
	USER_VFS_TYPE_CHARACTER = 5,
	USER_VFS_ERR_NOENT = 1,
	USER_VFS_ERR_ACCESS = 2,
	USER_VFS_ERR_NOTDIR = 3,
	USER_VFS_ERR_ISDIR = 4,
	USER_VFS_MMAP_PAGE_DATA = 1,
	USER_VFS_MMAP_PAGE_ZERO = 2,
	USER_VFS_MMAP_PAGE_BUS = 3,
	LINUX_RPC_REGISTER_PROCESS = 1000,
	LINUX_RPC_FORK_PROCESS = 1001,
	LINUX_RPC_EXEC_PROCESS = 1002,
	LINUX_RPC_EXEC_PREPARE = 1005,
	USER_PROC_SET_CMDLINE_BUFFER = 14,
	USER_PROC_INFO_BY_LINUX_PID = 18,
	USER_PROC_EXEC_REPLACE_TASK = 16,
	USER_PROC_EXEC_REPLACE_BUFFER = 17,
};

static int linux_forward_message(struct ipc_port *linux,
				 struct ipc_port *reply_port,
				 struct ipc_message *request,
				 struct ipc_message *reply);

static int linux_einval_int(const char *function, u64 line)
{
	console_printf("linux-einval: kernel %s:%u\n", function, (u32)line);
	return -LINUX_EINVAL;
}

static u64 linux_einval_u64(const char *function, u64 line)
{
	console_printf("linux-einval: kernel %s:%u\n", function, (u32)line);
	return (u64)-LINUX_EINVAL;
}

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
static void linux_vfork_complete_task(u32 child_task);
static int str_eq(const char *left, const char *right);

struct linux_signal_frame {
	u64 magic;
	u64 saved_rax;
	u64 old_mask;
	struct arch_syscall_frame frame;
};

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

		message->cap_type = type == TASK_CAP_PORT ? IPC_CAP_PORT :
			(type == TASK_CAP_BUFFER ? IPC_CAP_BUFFER :
			 type == TASK_CAP_TASK ? IPC_CAP_TASK :
			 type == TASK_CAP_HW_RESOURCE ? IPC_CAP_HW_RESOURCE :
			 IPC_CAP_SCHED_POLICY);
		message->cap_object = object;
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
	} else if (message->cap_type == IPC_CAP_TASK) {
		user_message->cap =
			task_grant_task(task_current(),
					(struct task *)message->cap_object,
					message->cap_rights);
	} else if (message->cap_type == IPC_CAP_HW_RESOURCE) {
		user_message->cap =
			task_grant_hw_resource(
				task_current(),
				(const struct task_hw_resource *)
					message->cap_object,
				message->cap_rights);
	} else if (message->cap_type == IPC_CAP_SCHED_POLICY) {
		user_message->cap =
			task_grant_sched_policy(
				task_current(),
				(const struct sched_policy_cap *)
					message->cap_object,
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
	struct task *task = task_current();
	const u64 start = align_down(vaddr, VM_PAGE_SIZE);
	const u64 end = align_up(vaddr + len, VM_PAGE_SIZE);

	if (vm_write_user(task_vm_space(task), vaddr, src, len) == 0) {
		return 0;
	}
	if (len == 0 || vaddr + len < vaddr || end <= start) {
		return -1;
	}
	for (u64 page = start; page < end; page += VM_PAGE_SIZE) {
		(void)task_handle_cow_fault(task, page, 1);
	}
	return vm_write_user(task_vm_space(task), vaddr, src, len);
}

static int linux_timespec_to_ns(u64 vaddr, u64 *ns)
{
	u64 timespec[2];
	const u64 nsec_per_sec = 1000000000ull;

	if (ns == 0) {
		return linux_einval_int(__func__, __LINE__);
	}
	if (vaddr == 0 ||
	    read_current_user(vaddr, timespec, sizeof(timespec)) != 0) {
		return -LINUX_EFAULT;
	}
	if (timespec[1] >= nsec_per_sec ||
	    timespec[0] > (~0ull - timespec[1]) / nsec_per_sec) {
		return linux_einval_int(__func__, __LINE__);
	}

	*ns = timespec[0] * nsec_per_sec + timespec[1];
	return 0;
}

static int linux_validate_utimens(u64 vaddr)
{
	u64 timespec[4];
	const u64 nsec_per_sec = 1000000000ull;

	if (vaddr == 0) {
		return 0;
	}
	if (read_current_user(vaddr, timespec, sizeof(timespec)) != 0) {
		return -LINUX_EFAULT;
	}
	for (u64 i = 1; i < 4; i += 2) {
		if (timespec[i] >= nsec_per_sec &&
		    timespec[i] != LINUX_UTIME_NOW &&
		    timespec[i] != LINUX_UTIME_OMIT) {
			return linux_einval_int(__func__, __LINE__);
		}
	}
	return 0;
}

static void linux_store_u64_le(unsigned char *buffer, u64 offset, u64 value)
{
	for (u64 i = 0; i < 8; i++) {
		buffer[offset + i] = (unsigned char)((value >> (i * 8)) & 0xff);
	}
}

static u64 linux_forward_utimensat(struct ipc_port *linux,
				   struct ipc_port *reply_port,
				   struct ipc_message *request,
				   struct ipc_message *reply,
				   const char *path, u64 times_addr,
				   u64 path_flags)
{
	enum {
		LINUX_UTIMENS_ATIME_OMIT = 1,
		LINUX_UTIMENS_MTIME_OMIT = 2,
		LINUX_UTIMENS_ATIME_NOW = 4,
		LINUX_UTIMENS_MTIME_NOW = 8,
	};
	struct shared_buffer *buffer;
	char *copy;
	u64 len = 0;
	u64 timespec[4];
	u64 time_flags = 0;
	u64 atime = 0;
	u64 mtime = 0;
	unsigned char payload[16];
	int rc;

	if (times_addr == 0) {
		time_flags = LINUX_UTIMENS_ATIME_NOW |
			     LINUX_UTIMENS_MTIME_NOW;
	} else if (read_current_user(times_addr, timespec,
				     sizeof(timespec)) != 0) {
		return (u64)-LINUX_EFAULT;
	} else {
		if (timespec[1] == LINUX_UTIME_OMIT) {
			time_flags |= LINUX_UTIMENS_ATIME_OMIT;
		} else if (timespec[1] == LINUX_UTIME_NOW) {
			time_flags |= LINUX_UTIMENS_ATIME_NOW;
		} else {
			atime = timespec[0];
		}
		if (timespec[3] == LINUX_UTIME_OMIT) {
			time_flags |= LINUX_UTIMENS_MTIME_OMIT;
		} else if (timespec[3] == LINUX_UTIME_NOW) {
			time_flags |= LINUX_UTIMENS_MTIME_NOW;
		} else {
			mtime = timespec[2];
		}
	}

	if (path == 0) {
		return (u64)-LINUX_EFAULT;
	}
	copy = (char *)slab_alloc(LINUX_MAX_SYSCALL_BUFFER);
	if (copy == 0) {
		return (u64)-LINUX_ENOMEM;
	}
	rc = copy_cstr_from_user_errno(copy, path, LINUX_MAX_SYSCALL_BUFFER);
	if (rc != 0) {
		slab_free(copy);
		return (u64)rc;
	}
	len = str_len(copy) + 1;
	buffer = buffer_create(len + sizeof(payload));
	if (buffer == 0 ||
	    buffer_write(buffer, 0, copy, len) != 0) {
		buffer_release(buffer);
		slab_free(copy);
		return (u64)-LINUX_ENOMEM;
	}
	slab_free(copy);
	linux_store_u64_le(payload, 0, atime);
	linux_store_u64_le(payload, 8, mtime);
	if (buffer_write(buffer, len, payload, sizeof(payload)) != 0) {
		buffer_release(buffer);
		return (u64)-LINUX_ENOMEM;
	}
	request->words[1] = len;
	request->words[2] = path_flags | (time_flags << 32);
	request->cap_type = IPC_CAP_BUFFER;
	request->cap_rights = TASK_RIGHT_RECV;
	request->cap_object = buffer;
	if (linux_forward_message(linux, reply_port, request, reply) != 0) {
		buffer_release(buffer);
		return (u64)-LINUX_ENOSYS;
	}
	buffer_release(buffer);
	return reply->words[0];
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
	return status == 0 ? 0 : linux_einval_int(__func__, __LINE__);
}

static int linux_vfs_read_prefix(struct task *task, const char *path,
				 u8 *prefix, u64 prefix_cap,
				 u64 *prefix_size)
{
	struct ipc_port *vfs = ipc_port_find("vfs");
	struct ipc_port *reply_port = task_reply_port(task);
	struct ipc_message request;
	struct ipc_message reply;
	u64 file;
	u64 size;
	u64 read_len;
	struct shared_buffer *buffer;
	const u64 path_len = str_len(path) + 1;
	const char cwd[] = "/";
	const u64 cwd_len = sizeof(cwd);
	struct shared_buffer *path_buffer;

	if (vfs == 0 || reply_port == 0 || path == 0 || prefix == 0 ||
	    prefix_cap == 0 || prefix_size == 0) {
		return -1;
	}
	*prefix_size = 0;

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
	if (size == 0) {
		(void)linux_vfs_close(task, file);
		return -1;
	}

	read_len = min_u64(size, prefix_cap);
	buffer = buffer_create(read_len);
	if (buffer == 0) {
		(void)linux_vfs_close(task, file);
		return -2;
	}

	request = (struct ipc_message){
		.protocol = USER_FOURCC_VFS,
		.type = USER_VFS_READ_FILE_BUFFER,
		.sender = 0,
		.cap_rights = TASK_RIGHT_SEND | TASK_RIGHT_DUP,
		.reply_port = reply_port,
		.cap_type = IPC_CAP_BUFFER,
		.cap_object = buffer,
		.words = { file, 0, read_len, 0 },
	};
	if (ipc_send(vfs, &request) != 0 ||
	    ipc_recv(reply_port, &reply) != 0 ||
	    reply.words[0] != 0 ||
	    reply.words[1] == 0 ||
	    reply.words[1] > read_len ||
	    buffer_read(buffer, 0, prefix, reply.words[1]) != 0) {
		buffer_release(buffer);
		(void)linux_vfs_close(task, file);
		return -1;
	}

	*prefix_size = reply.words[1];
	buffer_release(buffer);
	return linux_vfs_close(task, file);
}

static int linux_mmap_file_into_task(struct task *task, struct ipc_port *linux,
				     struct ipc_port *reply_port, u64 base,
				     u64 len, u64 fd, u64 offset,
				     u64 *populated_len)
{
	struct shared_buffer *buffer;
	u64 populated = 0;

	if (linux == 0 || reply_port == 0 || len == 0 ||
	    populated_len == 0 || (offset & (VM_PAGE_SIZE - 1)) != 0) {
		return -1;
	}
	*populated_len = 0;

	buffer = buffer_create(LINUX_MAX_SHARED_BUFFER);
	if (buffer == 0) {
		return -1;
	}

	for (u64 done = 0; done < len;) {
		const u64 chunk = min_u64(len - done, LINUX_MAX_SHARED_BUFFER);
		struct ipc_message request = {
			.protocol = USER_FOURCC_LINX,
			.type = LINUX_SYSCALL_MMAP,
			.sender = 0,
			.cap_rights = TASK_RIGHT_SEND | TASK_RIGHT_RECV |
				      TASK_RIGHT_DUP,
			.reply_port = reply_port,
			.cap_type = IPC_CAP_BUFFER,
			.cap_object = buffer,
			.words = { fd, offset + done, chunk, 0 },
		};
		struct ipc_message reply;

		if (ipc_send(linux, &request) != 0 ||
		    ipc_recv(reply_port, &reply) != 0 ||
		    (i64)reply.words[0] < 0 ||
		    reply.words[0] > chunk ||
		    (reply.words[1] != USER_VFS_MMAP_PAGE_DATA &&
		     reply.words[1] != USER_VFS_MMAP_PAGE_ZERO &&
		     reply.words[1] != USER_VFS_MMAP_PAGE_BUS)) {
			buffer_release(buffer);
			return -1;
		}
		if (reply.words[1] == USER_VFS_MMAP_PAGE_BUS ||
		    reply.words[0] == 0) {
			break;
		}
		if (reply.words[1] == USER_VFS_MMAP_PAGE_ZERO) {
			done += reply.words[0];
			populated = done;
			continue;
		}
		for (u64 copied = 0; copied < reply.words[0];) {
			const u64 copy = min_u64(reply.words[0] - copied,
						 LINUX_MAX_SYSCALL_BUFFER);
			const u64 flags = spin_lock_irqsave(&syscall_copy_lock);

			if (buffer_read(buffer, copied, syscall_copy_buffer,
					copy) != 0 ||
			    vm_write_user(task_vm_space(task), base + done +
					  copied, syscall_copy_buffer, copy) != 0) {
				spin_unlock_irqrestore(&syscall_copy_lock, flags);
				buffer_release(buffer);
				return -1;
			}
			spin_unlock_irqrestore(&syscall_copy_lock, flags);
			copied += copy;
		}
		done += reply.words[0];
		populated = done;
	}

	buffer_release(buffer);
	*populated_len = populated;
	if (populated < len) {
		const u64 keep = align_up(populated, VM_PAGE_SIZE);

		if (keep < len &&
		    vm_unmap_user_range(task_vm_space(task), base + keep,
					len - keep) != 0) {
			return -1;
		}
	}
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

static int linux_send_message(struct ipc_port *linux,
			      struct ipc_message *request)
{
	if (linux == 0 || request == 0) {
		return -1;
	}

	request->protocol = USER_FOURCC_LINX;
	request->reply_port = 0;
	return ipc_send(linux, request);
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
		for (u64 done = 0; done < len;) {
			const u64 chunk = min_u64(len - done,
						  LINUX_MAX_SYSCALL_BUFFER);

			if (read_current_user(user_buffer + done,
					      syscall_copy_buffer, chunk) != 0 ||
			    buffer_write(buffer, done, syscall_copy_buffer,
					 chunk) != 0) {
				spin_unlock_irqrestore(&syscall_copy_lock,
						       flags);
				buffer_release(buffer);
				return (u64)-LINUX_EFAULT;
			}
			done += chunk;
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
	case LINUX_SYSCALL_CLOSE_RANGE:
	case LINUX_SYSCALL_FLOCK:
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
	case LINUX_SYSCALL_ACCEPT:
	case LINUX_SYSCALL_KILL:
	case LINUX_SYSCALL_REBOOT:
	case LINUX_SYSCALL_FTRUNCATE:
	case LINUX_SYSCALL_FCHMOD:
	case LINUX_SYSCALL_FCHOWN:
	case LINUX_SYSCALL_LISTEN:
	case LINUX_SYSCALL_SHUTDOWN:
		return 1;
	default:
		return 0;
	}
}

static int linux_fdset_get(const u8 *set, u64 fd)
{
	return (set[fd / 8] & (1u << (fd % 8))) != 0;
}

static void linux_fdset_set(u8 *set, u64 fd)
{
	set[fd / 8] |= (u8)(1u << (fd % 8));
}

static u64 linux_forward_select(struct ipc_port *linux,
				struct ipc_port *reply_port, u64 nfds,
				u64 readfds, u64 writefds, u64 exceptfds)
{
	enum {
		LINUX_POLLIN = 0x0001,
		LINUX_POLLOUT = 0x0004,
		LINUX_POLLERR = 0x0008,
	};
	u8 in_read[LINUX_FDSET_BYTES];
	u8 in_write[LINUX_FDSET_BYTES];
	u8 in_except[LINUX_FDSET_BYTES];
	u8 out_read[LINUX_FDSET_BYTES];
	u8 out_write[LINUX_FDSET_BYTES];
	u8 out_except[LINUX_FDSET_BYTES];
	u64 bytes;
	u64 ready = 0;

	if (nfds > LINUX_FDSET_BITS) {
		return linux_einval_u64(__func__, __LINE__);
	}
	bytes = (nfds + 7) / 8;
	for (u64 i = 0; i < LINUX_FDSET_BYTES; i++) {
		in_read[i] = 0;
		in_write[i] = 0;
		in_except[i] = 0;
		out_read[i] = 0;
		out_write[i] = 0;
		out_except[i] = 0;
	}
	if (readfds != 0 &&
	    read_current_user(readfds, in_read, bytes) != 0) {
		return (u64)-LINUX_EFAULT;
	}
	if (writefds != 0 &&
	    read_current_user(writefds, in_write, bytes) != 0) {
		return (u64)-LINUX_EFAULT;
	}
	if (exceptfds != 0 &&
	    read_current_user(exceptfds, in_except, bytes) != 0) {
		return (u64)-LINUX_EFAULT;
	}
	for (u64 fd = 0; fd < nfds; fd++) {
		u64 events = 0;
		u64 revents;

		if (linux_fdset_get(in_read, fd)) {
			events |= LINUX_POLLIN;
		}
		if (linux_fdset_get(in_write, fd)) {
			events |= LINUX_POLLOUT;
		}
		if (linux_fdset_get(in_except, fd)) {
			events |= LINUX_POLLERR;
		}
		if (events == 0) {
			continue;
		}
		revents = linux_forward_words(linux, reply_port,
					      LINUX_SYSCALL_POLL, fd,
					      events, 0);
		if ((i64)revents < 0) {
			return revents;
		}
		if (readfds != 0 && (revents & LINUX_POLLIN) != 0 &&
		    linux_fdset_get(in_read, fd)) {
			linux_fdset_set(out_read, fd);
			ready++;
		}
		if (writefds != 0 && (revents & LINUX_POLLOUT) != 0 &&
		    linux_fdset_get(in_write, fd)) {
			linux_fdset_set(out_write, fd);
			ready++;
		}
		if (exceptfds != 0 && (revents & LINUX_POLLERR) != 0 &&
		    linux_fdset_get(in_except, fd)) {
			linux_fdset_set(out_except, fd);
			ready++;
		}
	}
	if (readfds != 0 &&
	    write_current_user(readfds, out_read, bytes) != 0) {
		return (u64)-LINUX_EFAULT;
	}
	if (writefds != 0 &&
	    write_current_user(writefds, out_write, bytes) != 0) {
		return (u64)-LINUX_EFAULT;
	}
	if (exceptfds != 0 &&
	    write_current_user(exceptfds, out_except, bytes) != 0) {
		return (u64)-LINUX_EFAULT;
	}
	return ready;
}

struct linux_msghdr_copy {
	u64 name;
	u64 namelen;
	u64 iov;
	u64 iovlen;
	u64 control;
	u64 controllen;
	u64 flags;
};

static int linux_read_msghdr(u64 user_msghdr, struct linux_msghdr_copy *out)
{
	u32 namelen;
	u32 flags;

	if (user_msghdr == 0 || out == 0) {
		return -LINUX_EFAULT;
	}
	if (read_current_user(user_msghdr + LINUX_MSGHDR_NAME_OFF,
			      &out->name, sizeof(out->name)) != 0 ||
	    read_current_user(user_msghdr + LINUX_MSGHDR_NAMELEN_OFF,
			      &namelen, sizeof(namelen)) != 0 ||
	    read_current_user(user_msghdr + LINUX_MSGHDR_IOV_OFF,
			      &out->iov, sizeof(out->iov)) != 0 ||
	    read_current_user(user_msghdr + LINUX_MSGHDR_IOVLEN_OFF,
			      &out->iovlen, sizeof(out->iovlen)) != 0 ||
	    read_current_user(user_msghdr + LINUX_MSGHDR_CONTROL_OFF,
			      &out->control, sizeof(out->control)) != 0 ||
	    read_current_user(user_msghdr + LINUX_MSGHDR_CONTROLLEN_OFF,
			      &out->controllen, sizeof(out->controllen)) != 0 ||
	    read_current_user(user_msghdr + LINUX_MSGHDR_FLAGS_OFF,
			      &flags, sizeof(flags)) != 0) {
		return -LINUX_EFAULT;
	}
	out->namelen = namelen;
	out->flags = flags;
	if (out->iovlen > LINUX_IOV_MAX ||
	    out->namelen > LINUX_MAX_SOCKADDR) {
		return -LINUX_EINVAL;
	}
	if (out->iovlen != 0 && out->iov == 0) {
		return -LINUX_EFAULT;
	}
	if (out->namelen != 0 && out->name == 0) {
		return -LINUX_EFAULT;
	}
	if (out->controllen != 0 && out->control == 0) {
		return -LINUX_EFAULT;
	}
	return 0;
}

static u64 linux_forward_sendmsg(struct ipc_port *linux,
				 struct ipc_port *reply_port,
				 struct ipc_message *request, u64 fd,
				 u64 user_msghdr, u64 flags)
{
	struct linux_msghdr_copy msg;
	struct {
		u64 base;
		u64 len;
	} iov;
	struct shared_buffer *buffer;
	struct ipc_message reply;
	u64 total = 0;
	u64 payload_offset;
	u64 written = 0;
	u64 irq_flags;
	int rc;

	rc = linux_read_msghdr(user_msghdr, &msg);
	if (rc != 0) {
		return (u64)rc;
	}
	if (msg.controllen != 0) {
		return linux_einval_u64(__func__, __LINE__);
	}
	for (u64 i = 0; i < msg.iovlen; i++) {
		if (read_current_user(msg.iov + i * sizeof(iov), &iov,
				      sizeof(iov)) != 0) {
			return (u64)-LINUX_EFAULT;
		}
		if (iov.base == 0 && iov.len != 0) {
			return (u64)-LINUX_EFAULT;
		}
		if (iov.len > LINUX_MAX_SYSCALL_BUFFER - total) {
			return linux_einval_u64(__func__, __LINE__);
		}
		total += iov.len;
	}
	payload_offset = msg.namelen;
	buffer = buffer_create(payload_offset + (total == 0 ? 1 : total));
	if (buffer == 0) {
		return (u64)-LINUX_ENOMEM;
	}
	irq_flags = spin_lock_irqsave(&syscall_copy_lock);
	if (msg.namelen != 0 &&
	    (read_current_user(msg.name, syscall_copy_buffer,
			       msg.namelen) != 0 ||
	     buffer_write(buffer, 0, syscall_copy_buffer,
			  msg.namelen) != 0)) {
		spin_unlock_irqrestore(&syscall_copy_lock, irq_flags);
		buffer_release(buffer);
		return (u64)-LINUX_EFAULT;
	}
	for (u64 i = 0; i < msg.iovlen; i++) {
		if (read_current_user(msg.iov + i * sizeof(iov), &iov,
				      sizeof(iov)) != 0) {
			spin_unlock_irqrestore(&syscall_copy_lock, irq_flags);
			buffer_release(buffer);
			return (u64)-LINUX_EFAULT;
		}
		for (u64 done = 0; done < iov.len;) {
			const u64 chunk = min_u64(iov.len - done,
						  LINUX_MAX_SYSCALL_BUFFER);

			if (read_current_user(iov.base + done,
					      syscall_copy_buffer, chunk) != 0 ||
			    buffer_write(buffer, payload_offset + written,
					 syscall_copy_buffer, chunk) != 0) {
				spin_unlock_irqrestore(&syscall_copy_lock,
						       irq_flags);
				buffer_release(buffer);
				return (u64)-LINUX_EFAULT;
			}
			done += chunk;
			written += chunk;
		}
	}
	spin_unlock_irqrestore(&syscall_copy_lock, irq_flags);

	request->type = LINUX_SYSCALL_SENDTO;
	request->words[0] = fd;
	request->words[1] = total;
	request->words[2] = flags;
	request->words[3] = msg.namelen;
	request->cap_type = IPC_CAP_BUFFER;
	request->cap_rights = TASK_RIGHT_RECV | TASK_RIGHT_DUP;
	request->cap_object = buffer;
	if (linux_forward_message(linux, reply_port, request, &reply) != 0) {
		buffer_release(buffer);
		return (u64)-LINUX_ENOSYS;
	}
	buffer_release(buffer);
	return reply.words[0];
}

static u64 linux_forward_recvmsg(struct ipc_port *linux,
				 struct ipc_port *reply_port,
				 struct ipc_message *request, u64 fd,
				 u64 user_msghdr, u64 flags)
{
	struct linux_msghdr_copy msg;
	struct {
		u64 base;
		u64 len;
	} iov;
	struct shared_buffer *buffer;
	struct ipc_message reply;
	u64 total = 0;
	u64 copied = 0;
	u64 irq_flags;
	int rc;

	rc = linux_read_msghdr(user_msghdr, &msg);
	if (rc != 0) {
		return (u64)rc;
	}
	for (u64 i = 0; i < msg.iovlen; i++) {
		if (read_current_user(msg.iov + i * sizeof(iov), &iov,
				      sizeof(iov)) != 0) {
			return (u64)-LINUX_EFAULT;
		}
		if (iov.base == 0 && iov.len != 0) {
			return (u64)-LINUX_EFAULT;
		}
		if (iov.len > LINUX_MAX_SYSCALL_BUFFER - total) {
			return linux_einval_u64(__func__, __LINE__);
		}
		total += iov.len;
	}
	buffer = buffer_create(total + msg.namelen == 0 ? 1 :
			       total + msg.namelen);
	if (buffer == 0) {
		return (u64)-LINUX_ENOMEM;
	}
	request->type = LINUX_SYSCALL_RECVMSG;
	request->words[0] = fd;
	request->words[1] = total;
	request->words[2] = flags;
	request->words[3] = msg.namelen;
	request->cap_type = IPC_CAP_BUFFER;
	request->cap_rights = TASK_RIGHT_SEND | TASK_RIGHT_DUP;
	request->cap_object = buffer;
	if (linux_forward_message(linux, reply_port, request, &reply) != 0) {
		buffer_release(buffer);
		return (u64)-LINUX_ENOSYS;
	}
	if ((i64)reply.words[0] <= 0) {
		buffer_release(buffer);
		return reply.words[0];
	}
	irq_flags = spin_lock_irqsave(&syscall_copy_lock);
	for (u64 i = 0; i < msg.iovlen && copied < reply.words[0]; i++) {
		u64 todo;

		if (read_current_user(msg.iov + i * sizeof(iov), &iov,
				      sizeof(iov)) != 0) {
			spin_unlock_irqrestore(&syscall_copy_lock, irq_flags);
			buffer_release(buffer);
			return (u64)-LINUX_EFAULT;
		}
		todo = min_u64(iov.len, reply.words[0] - copied);
		for (u64 done = 0; done < todo;) {
			const u64 chunk = min_u64(todo - done,
						  LINUX_MAX_SYSCALL_BUFFER);

			if (buffer_read(buffer, copied, syscall_copy_buffer,
					chunk) != 0 ||
			    write_current_user(iov.base + done,
					       syscall_copy_buffer, chunk) != 0) {
				spin_unlock_irqrestore(&syscall_copy_lock,
						       irq_flags);
				buffer_release(buffer);
				return (u64)-LINUX_EFAULT;
			}
			done += chunk;
			copied += chunk;
		}
	}
	spin_unlock_irqrestore(&syscall_copy_lock, irq_flags);
	if (msg.namelen != 0 && reply.words[1] != 0) {
		u32 actual = (u32)reply.words[1];
		u64 copy = msg.namelen < reply.words[1] ?
			   msg.namelen : reply.words[1];

		irq_flags = spin_lock_irqsave(&syscall_copy_lock);
		if ((copy != 0 &&
		     (buffer_read(buffer, total, syscall_copy_buffer, copy) != 0 ||
		      write_current_user(msg.name, syscall_copy_buffer,
					 copy) != 0)) ||
		    write_current_user(user_msghdr + LINUX_MSGHDR_NAMELEN_OFF,
				       &actual, sizeof(actual)) != 0) {
			spin_unlock_irqrestore(&syscall_copy_lock, irq_flags);
			buffer_release(buffer);
			return (u64)-LINUX_EFAULT;
		}
		spin_unlock_irqrestore(&syscall_copy_lock, irq_flags);
	}
	if (msg.controllen != 0) {
		u64 zero = 0;

		if (write_current_user(user_msghdr +
				       LINUX_MSGHDR_CONTROLLEN_OFF,
				       &zero, sizeof(zero)) != 0) {
			buffer_release(buffer);
			return (u64)-LINUX_EFAULT;
		}
	}
	{
		u32 out_flags = 0;

		if (write_current_user(user_msghdr + LINUX_MSGHDR_FLAGS_OFF,
				       &out_flags, sizeof(out_flags)) != 0) {
			buffer_release(buffer);
			return (u64)-LINUX_EFAULT;
		}
	}
	buffer_release(buffer);
	return reply.words[0];
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

static void linux_task_died(u32 task_id, const char *task_name)
{
	(void)task_name;
	linux_vfork_complete_task(task_id);
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
static u64 linux_copy_buffer_to_user(struct shared_buffer *buffer,
				     u64 offset, u64 user_out, u64 size);

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
		return linux_einval_u64(__func__, __LINE__);
	}
	return linux_forward_input_buffer(linux, reply_port, &request,
					  user_buffer, len,
					  TASK_RIGHT_RECV | TASK_RIGHT_DUP);
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
		return linux_einval_u64(__func__, __LINE__);
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
		       linux_einval_u64(__func__, __LINE__);
	}
	while (total < len) {
		const u64 chunk = min_u64(len - total,
					  LINUX_MAX_SHARED_BUFFER);
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
			return linux_einval_u64(__func__, __LINE__);
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
	char *copy;
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
	copy = (char *)slab_alloc(LINUX_MAX_SYSCALL_BUFFER);
	if (copy == 0) {
		*error = (u64)-LINUX_ENOMEM;
		return 0;
	}
	rc = copy_cstr_from_user_errno(copy, path, LINUX_MAX_SYSCALL_BUFFER);
	if (rc != 0) {
		slab_free(copy);
		*error = (u64)rc;
		return 0;
	}
	len = str_len(copy) + 1;
	size = min_size > len ? min_size : len;
	buffer = buffer_create(size == 0 ? 1 : size);
	if (buffer == 0 ||
	    buffer_write(buffer, 0, copy, len) != 0) {
		buffer_release(buffer);
		slab_free(copy);
		*error = (u64)-LINUX_ENOMEM;
		return 0;
	}
	slab_free(copy);
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

static u64 linux_forward_symlinkat(struct ipc_port *linux,
				   struct ipc_port *reply_port,
				   struct ipc_message *request,
				   const char *target, u64 dirfd,
				   const char *linkpath)
{
	struct shared_buffer *buffer;
	struct ipc_message reply;
	char *copy;
	u64 target_len;
	u64 link_len;
	u64 total_len;
	int rc;

	if (target == 0 || linkpath == 0) {
		return (u64)-LINUX_EFAULT;
	}
	copy = (char *)slab_alloc(LINUX_MAX_SYSCALL_BUFFER);
	if (copy == 0) {
		return (u64)-LINUX_ENOMEM;
	}
	rc = copy_cstr_from_user_errno(copy, target, LINUX_MAX_SYSCALL_BUFFER);
	if (rc != 0) {
		slab_free(copy);
		return (u64)rc;
	}
	target_len = str_len(copy) + 1;
	if (target_len == 1) {
		slab_free(copy);
		return (u64)-LINUX_ENOENT;
	}
	if (target_len >= LINUX_MAX_SYSCALL_BUFFER) {
		slab_free(copy);
		return (u64)-LINUX_ENAMETOOLONG;
	}
	rc = copy_cstr_from_user_errno(copy + target_len, linkpath,
				       LINUX_MAX_SYSCALL_BUFFER -
				       target_len);
	if (rc != 0) {
		slab_free(copy);
		return (u64)rc;
	}
	link_len = str_len(copy + target_len) + 1;
	if (link_len == 1) {
		slab_free(copy);
		return (u64)-LINUX_ENOENT;
	}
	total_len = target_len + link_len;
	buffer = buffer_create(total_len);
	if (buffer == 0 ||
	    buffer_write(buffer, 0, copy, total_len) != 0) {
		slab_free(copy);
		buffer_release(buffer);
		return (u64)-LINUX_ENOMEM;
	}
	slab_free(copy);
	request->type = LINUX_SYSCALL_SYMLINKAT;
	request->words[0] = dirfd;
	request->words[1] = target_len;
	request->words[2] = link_len;
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

static u64 linux_forward_two_path_at(struct ipc_port *linux,
				     struct ipc_port *reply_port,
				     struct ipc_message *request,
				     u32 type, u64 olddirfd,
				     const char *oldpath, u64 newdirfd,
				     const char *newpath, u64 flags)
{
	struct shared_buffer *buffer;
	struct ipc_message reply;
	char *copy;
	u64 old_len;
	u64 new_len;
	u64 total_len;
	int rc;

	if (oldpath == 0 || newpath == 0) {
		return (u64)-LINUX_EFAULT;
	}
	copy = (char *)slab_alloc(LINUX_MAX_SYSCALL_BUFFER);
	if (copy == 0) {
		return (u64)-LINUX_ENOMEM;
	}
	rc = copy_cstr_from_user_errno(copy, oldpath, LINUX_MAX_SYSCALL_BUFFER);
	if (rc != 0) {
		slab_free(copy);
		return (u64)rc;
	}
	old_len = str_len(copy) + 1;
	if (old_len == 1) {
		slab_free(copy);
		return (u64)-LINUX_ENOENT;
	}
	if (old_len >= LINUX_MAX_SYSCALL_BUFFER) {
		slab_free(copy);
		return (u64)-LINUX_ENAMETOOLONG;
	}
	rc = copy_cstr_from_user_errno(copy + old_len, newpath,
				       LINUX_MAX_SYSCALL_BUFFER - old_len);
	if (rc != 0) {
		slab_free(copy);
		return (u64)rc;
	}
	new_len = str_len(copy + old_len) + 1;
	if (new_len == 1) {
		slab_free(copy);
		return (u64)-LINUX_ENOENT;
	}
	total_len = old_len + new_len;
	if (total_len > LINUX_MAX_SYSCALL_BUFFER ||
	    old_len > 0xffffffff || new_len > 0xffffffff ||
	    flags > 0xffffffff) {
		slab_free(copy);
		return (u64)-LINUX_ENAMETOOLONG;
	}
	buffer = buffer_create(total_len);
	if (buffer == 0 ||
	    buffer_write(buffer, 0, copy, total_len) != 0) {
		slab_free(copy);
		buffer_release(buffer);
		return (u64)-LINUX_ENOMEM;
	}
	slab_free(copy);
	request->type = type;
	request->words[0] = olddirfd;
	request->words[1] = newdirfd;
	request->words[2] = old_len;
	request->words[3] = new_len | (flags << 32);
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

static u64 linux_forward_renameat2(struct ipc_port *linux,
				   struct ipc_port *reply_port,
				   struct ipc_message *request,
				   u64 olddirfd, const char *oldpath,
				   u64 newdirfd, const char *newpath,
				   u64 flags)
{
	return linux_forward_two_path_at(linux, reply_port, request,
					 LINUX_SYSCALL_RENAMEAT2, olddirfd,
					 oldpath, newdirfd, newpath, flags);
}

static u64 linux_forward_mount(struct ipc_port *linux,
			       struct ipc_port *reply_port,
			       struct ipc_message *request,
			       const char *target, const char *fstype,
			       u64 flags)
{
	struct shared_buffer *buffer;
	struct ipc_message reply;
	char *copy;
	u64 target_len;
	u64 fstype_len;
	u64 total_len;
	int rc;

	if (target == 0 || fstype == 0) {
		return (u64)-LINUX_EFAULT;
	}
	copy = (char *)slab_alloc(LINUX_MAX_SYSCALL_BUFFER);
	if (copy == 0) {
		return (u64)-LINUX_ENOMEM;
	}
	rc = copy_cstr_from_user_errno(copy, target, LINUX_MAX_SYSCALL_BUFFER);
	if (rc != 0) {
		slab_free(copy);
		return (u64)rc;
	}
	target_len = str_len(copy) + 1;
	if (target_len == 1 || target_len >= LINUX_MAX_SYSCALL_BUFFER) {
		slab_free(copy);
		return linux_einval_u64(__func__, __LINE__);
	}
	rc = copy_cstr_from_user_errno(copy + target_len, fstype,
				       LINUX_MAX_SYSCALL_BUFFER -
				       target_len);
	if (rc != 0) {
		slab_free(copy);
		return (u64)rc;
	}
	fstype_len = str_len(copy + target_len) + 1;
	total_len = target_len + fstype_len;
	if (fstype_len == 1 || total_len > LINUX_MAX_SYSCALL_BUFFER) {
		slab_free(copy);
		return linux_einval_u64(__func__, __LINE__);
	}
	buffer = buffer_create(total_len);
	if (buffer == 0 ||
	    buffer_write(buffer, 0, copy, total_len) != 0) {
		slab_free(copy);
		buffer_release(buffer);
		return (u64)-LINUX_ENOMEM;
	}
	slab_free(copy);
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
		if (reply.words[0] > size ||
		    linux_copy_buffer_to_user(buffer, 0, (u64)user_out,
					      reply.words[0]) != 0) {
			buffer_release(buffer);
			return (u64)-LINUX_EFAULT;
		}
	}
	buffer_release(buffer);
	return reply.words[0];
}

static u64 linux_copy_buffer_to_user(struct shared_buffer *buffer,
				     u64 offset, u64 user_out, u64 size)
{
	u64 flags;

	flags = spin_lock_irqsave(&syscall_copy_lock);
	for (u64 done = 0; done < size;) {
		const u64 chunk = min_u64(size - done,
					  LINUX_MAX_SYSCALL_BUFFER);

		if (buffer_read(buffer, offset + done, syscall_copy_buffer,
				chunk) != 0 ||
		    write_current_user(user_out + done, syscall_copy_buffer,
				       chunk) != 0) {
			spin_unlock_irqrestore(&syscall_copy_lock, flags);
			return (u64)-LINUX_EFAULT;
		}
		done += chunk;
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

static u64 linux_forward_recvfrom(struct ipc_port *linux,
				  struct ipc_port *reply_port,
				  struct ipc_message *request,
				  u64 fd, u64 user_payload, u64 payload_len,
				  u64 flags, u64 user_addr,
				  u64 user_addr_lenp)
{
	struct shared_buffer *buffer;
	struct ipc_message reply;
	u32 in_addr_len = 0;
	u64 addr_len = 0;
	u64 buffer_size;

	if (user_payload == 0 && payload_len != 0) {
		return (u64)-LINUX_EFAULT;
	}
	if (user_addr != 0) {
		if (user_addr_lenp == 0 ||
		    read_current_user(user_addr_lenp, &in_addr_len,
				      sizeof(in_addr_len)) != 0) {
			return (u64)-LINUX_EFAULT;
		}
		addr_len = in_addr_len > LINUX_MAX_SOCKADDR ?
			   LINUX_MAX_SOCKADDR : in_addr_len;
	}
	buffer_size = payload_len + addr_len;
	buffer = buffer_create(buffer_size == 0 ? 1 : buffer_size);
	if (buffer == 0) {
		return (u64)-LINUX_ENOMEM;
	}
	request->type = LINUX_SYSCALL_RECVFROM;
	request->words[0] = fd;
	request->words[1] = payload_len;
	request->words[2] = flags;
	request->words[3] = addr_len;
	request->cap_type = IPC_CAP_BUFFER;
	request->cap_rights = TASK_RIGHT_SEND | TASK_RIGHT_DUP;
	request->cap_object = buffer;
	if (linux_forward_message(linux, reply_port, request, &reply) != 0) {
		buffer_release(buffer);
		return (u64)-LINUX_ENOSYS;
	}
	if ((i64)reply.words[0] > 0) {
		if (reply.words[0] > payload_len ||
		    linux_copy_buffer_to_user(buffer, 0, user_payload,
					      reply.words[0]) != 0) {
			buffer_release(buffer);
			return (u64)-LINUX_EFAULT;
		}
	}
	if ((i64)reply.words[0] >= 0 && user_addr != 0) {
		const u32 out_addr_len = (u32)reply.words[1];
		const u64 copy = reply.words[1] < addr_len ?
				 reply.words[1] : addr_len;

		if (write_current_user(user_addr_lenp, &out_addr_len,
				       sizeof(out_addr_len)) != 0 ||
		    (copy != 0 &&
		     (reply.words[0] + copy > buffer_size ||
		      linux_copy_buffer_to_user(buffer, reply.words[0],
						user_addr, copy) != 0))) {
			buffer_release(buffer);
			return (u64)-LINUX_EFAULT;
		}
	}
	buffer_release(buffer);
	return reply.words[0];
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
		if (linux_copy_buffer_to_user(buffer, 0, (u64)user_out,
					      size) != 0) {
			buffer_release(buffer);
			return (u64)-LINUX_EFAULT;
		}
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

static u64 linux_forward_socklen_output(struct ipc_port *linux,
					struct ipc_port *reply_port,
					struct ipc_message *request,
					u32 type, u64 fd, u64 user_out,
					u64 user_lenp)
{
	struct shared_buffer *buffer;
	struct ipc_message reply;
	u32 in_len;
	u32 out_len;
	u64 copy;

	if (user_out == 0 || user_lenp == 0) {
		return (u64)-LINUX_EFAULT;
	}
	if (read_current_user(user_lenp, &in_len, sizeof(in_len)) != 0) {
		return (u64)-LINUX_EFAULT;
	}
	if (in_len > LINUX_MAX_SOCKADDR) {
		in_len = LINUX_MAX_SOCKADDR;
	}
	buffer = buffer_create(in_len == 0 ? 1 : in_len);
	if (buffer == 0) {
		return (u64)-LINUX_ENOMEM;
	}
	request->type = type;
	request->words[0] = fd;
	request->words[1] = in_len;
	request->words[2] = 0;
	request->words[3] = 0;
	request->cap_type = IPC_CAP_BUFFER;
	request->cap_rights = TASK_RIGHT_SEND;
	request->cap_object = buffer;
	if (linux_forward_message(linux, reply_port, request, &reply) != 0) {
		buffer_release(buffer);
		return (u64)-LINUX_ENOSYS;
	}
	if ((i64)reply.words[0] < 0) {
		buffer_release(buffer);
		return reply.words[0];
	}
	out_len = (u32)reply.words[1];
	if (write_current_user(user_lenp, &out_len, sizeof(out_len)) != 0) {
		buffer_release(buffer);
		return (u64)-LINUX_EFAULT;
	}
	copy = out_len < in_len ? out_len : in_len;
	if (copy != 0 &&
	    linux_copy_buffer_to_user(buffer, 0, user_out, copy) != 0) {
		buffer_release(buffer);
		return (u64)-LINUX_EFAULT;
	}
	buffer_release(buffer);
	return reply.words[0];
}

static u64 linux_forward_getsockopt(struct ipc_port *linux,
				    struct ipc_port *reply_port,
				    struct ipc_message *request,
				    u64 fd, u64 level, u64 optname,
				    u64 user_out, u64 user_lenp)
{
	struct shared_buffer *buffer;
	struct ipc_message reply;
	u32 in_len;
	u32 out_len;
	u64 copy;

	if (user_out == 0 || user_lenp == 0) {
		return (u64)-LINUX_EFAULT;
	}
	if (read_current_user(user_lenp, &in_len, sizeof(in_len)) != 0) {
		return (u64)-LINUX_EFAULT;
	}
	if (in_len > LINUX_MAX_SOCKADDR) {
		in_len = LINUX_MAX_SOCKADDR;
	}
	buffer = buffer_create(in_len == 0 ? 1 : in_len);
	if (buffer == 0) {
		return (u64)-LINUX_ENOMEM;
	}
	request->type = LINUX_SYSCALL_GETSOCKOPT;
	request->words[0] = fd;
	request->words[1] = level;
	request->words[2] = optname;
	request->words[3] = in_len;
	request->cap_type = IPC_CAP_BUFFER;
	request->cap_rights = TASK_RIGHT_SEND;
	request->cap_object = buffer;
	if (linux_forward_message(linux, reply_port, request, &reply) != 0) {
		buffer_release(buffer);
		return (u64)-LINUX_ENOSYS;
	}
	if ((i64)reply.words[0] < 0) {
		buffer_release(buffer);
		return reply.words[0];
	}
	out_len = (u32)reply.words[1];
	if (write_current_user(user_lenp, &out_len, sizeof(out_len)) != 0) {
		buffer_release(buffer);
		return (u64)-LINUX_EFAULT;
	}
	copy = out_len < in_len ? out_len : in_len;
	if (copy != 0 &&
	    linux_copy_buffer_to_user(buffer, 0, user_out, copy) != 0) {
		buffer_release(buffer);
		return (u64)-LINUX_EFAULT;
	}
	buffer_release(buffer);
	return reply.words[0];
}

static u64 linux_forward_groups_out(struct ipc_port *linux,
				    struct ipc_port *reply_port,
				    struct ipc_message *request,
				    u64 count, u64 user_out)
{
	struct shared_buffer *buffer = 0;
	struct ipc_message reply;
	u64 size;

	if (count > LINUX_MAX_GROUPS) {
		return linux_einval_u64(__func__, __LINE__);
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
			return linux_einval_u64(__func__, __LINE__);
		}
		if (linux_copy_buffer_to_user(buffer, 0, user_out,
					      reply.words[0] *
					      sizeof(u32)) != 0) {
			buffer_release(buffer);
			return (u64)-LINUX_EFAULT;
		}
	}
	if (buffer != 0) {
		buffer_release(buffer);
	}
	return reply.words[0];
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

static int linux_exec_buffer_write_u64(struct shared_buffer *buffer,
				       u64 offset, u64 value)
{
	return buffer_write(buffer, offset, &value, sizeof(value));
}

static int linux_exec_buffer_read_u64(struct shared_buffer *buffer,
				      u64 offset, u64 *value)
{
	return buffer_read(buffer, offset, value, sizeof(*value));
}

static int linux_exec_serialize_one(struct shared_buffer *buffer, u64 *offset,
				    const char *value, u64 *bytes)
{
	const u64 len = value == 0 ? 0 : str_len(value) + 1;

	if (len == 0 || len > LINUX_EXEC_MAX_STRING ||
	    *bytes + len < *bytes ||
	    *bytes + len > LINUX_EXEC_MAX_STRING_BYTES ||
	    *offset + sizeof(len) < *offset ||
	    *offset + sizeof(len) + len < *offset ||
	    *offset + sizeof(len) + len > LINUX_EXEC_PREPARE_BUFFER_SIZE) {
		return -LINUX_E2BIG;
	}
	if (linux_exec_buffer_write_u64(buffer, *offset, len) != 0 ||
	    buffer_write(buffer, *offset + sizeof(len), value, len) != 0) {
		return -LINUX_EFAULT;
	}
	*offset += sizeof(len) + len;
	*bytes += len;
	return 0;
}

static int linux_exec_serialize_args(struct shared_buffer *buffer,
				     u64 offset,
				     const struct linux_exec_args *args,
				     u64 *len_out)
{
	const u64 start = offset;
	u64 bytes = 0;

	if (buffer == 0 || args == 0 || len_out == 0 ||
	    args->argc == 0 ||
	    args->argc > LINUX_EXEC_MAX_POINTERS ||
	    args->envc > LINUX_EXEC_MAX_POINTERS ||
	    offset + 2 * sizeof(u64) < offset ||
	    offset + 2 * sizeof(u64) > LINUX_EXEC_PREPARE_BUFFER_SIZE) {
		return -LINUX_EINVAL;
	}
	if (linux_exec_buffer_write_u64(buffer, offset, args->argc) != 0 ||
	    linux_exec_buffer_write_u64(buffer, offset + sizeof(u64),
				       args->envc) != 0) {
		return -LINUX_EFAULT;
	}
	offset += 2 * sizeof(u64);
	for (u64 i = 0; i < args->argc; i++) {
		const int result = linux_exec_serialize_one(buffer, &offset,
							   args->argv[i],
							   &bytes);

		if (result != 0) {
			return result;
		}
	}
	for (u64 i = 0; i < args->envc; i++) {
		const int result = linux_exec_serialize_one(buffer, &offset,
							   args->envp[i],
							   &bytes);

		if (result != 0) {
			return result;
		}
	}
	*len_out = offset - start;
	return 0;
}

static int linux_exec_deserialize_one(struct shared_buffer *buffer,
				      u64 limit, u64 *offset, char **out)
{
	u64 len = 0;
	char *copy;

	if (buffer == 0 || offset == 0 || out == 0 ||
	    *offset + sizeof(len) < *offset ||
	    *offset + sizeof(len) > limit ||
	    linux_exec_buffer_read_u64(buffer, *offset, &len) != 0 ||
	    len == 0 || len > LINUX_EXEC_MAX_STRING ||
	    *offset + sizeof(len) + len < *offset ||
	    *offset + sizeof(len) + len > limit) {
		return -LINUX_EINVAL;
	}
	copy = (char *)slab_alloc(len);
	if (copy == 0) {
		return -LINUX_ENOMEM;
	}
	if (buffer_read(buffer, *offset + sizeof(len), copy, len) != 0 ||
	    copy[len - 1] != '\0') {
		slab_free(copy);
		return -LINUX_EINVAL;
	}
	*offset += sizeof(len) + len;
	*out = copy;
	return 0;
}

static int linux_exec_deserialize_args(struct shared_buffer *buffer,
				       u64 offset, u64 len,
				       struct linux_exec_args *args)
{
	const u64 limit = offset + len;
	struct linux_exec_args next;
	u64 argv_capacity = 0;
	u64 env_capacity = 0;
	u64 argc = 0;
	u64 envc = 0;
	int result = 0;

	if (buffer == 0 || args == 0 || len < 2 * sizeof(u64) ||
	    limit < offset || limit > LINUX_EXEC_PREPARE_BUFFER_SIZE ||
	    linux_exec_buffer_read_u64(buffer, offset, &argc) != 0 ||
	    linux_exec_buffer_read_u64(buffer, offset + sizeof(u64),
				       &envc) != 0 ||
	    argc == 0 || argc > LINUX_EXEC_MAX_POINTERS ||
	    envc > LINUX_EXEC_MAX_POINTERS) {
		return -LINUX_EINVAL;
	}
	mem_zero((u8 *)&next, sizeof(next));
	offset += 2 * sizeof(u64);
	for (u64 i = 0; i < argc; i++) {
		char *copy = 0;
		int push_result;

		result = linux_exec_deserialize_one(buffer, limit, &offset,
						    &copy);
		push_result = result == 0 ?
			      linux_exec_vector_push(&next.argv, &next.argc,
						     &argv_capacity, copy) :
			      result;
		if (push_result != 0) {
			slab_free(copy);
			result = push_result;
			goto fail;
		}
		next.bytes += str_len(copy) + 1;
	}
	for (u64 i = 0; i < envc; i++) {
		char *copy = 0;
		int push_result;

		result = linux_exec_deserialize_one(buffer, limit, &offset,
						    &copy);
		push_result = result == 0 ?
			      linux_exec_vector_push(&next.envp, &next.envc,
						     &env_capacity, copy) :
			      result;
		if (push_result != 0) {
			slab_free(copy);
			result = push_result;
			goto fail;
		}
		next.bytes += str_len(copy) + 1;
	}
	if (offset != limit || next.bytes > LINUX_EXEC_MAX_STRING_BYTES) {
		result = -LINUX_EINVAL;
		goto fail;
	}

	linux_exec_args_free(args);
	*args = next;
	return 0;

fail:
	linux_exec_args_free(&next);
	return result;
}

static int linux_exec_prepare_shebang(struct task *task, char *load_path,
				      struct linux_exec_args *args,
				      const u8 *image, u64 image_size)
{
	struct ipc_port *linux = ipc_port_find("linux");
	struct ipc_port *reply_port = task_reply_port(task);
	struct shared_buffer *buffer;
	struct ipc_message request = {
		.protocol = USER_FOURCC_LINX,
		.type = LINUX_RPC_EXEC_PREPARE,
		.sender = 0,
		.cap_rights = TASK_RIGHT_SEND | TASK_RIGHT_RECV |
			      TASK_RIGHT_DUP,
		.reply_port = reply_port,
		.cap_type = IPC_CAP_BUFFER,
		.cap_object = 0,
		.words = { 0, 0, 0, 0 },
	};
	struct ipc_message reply;
	u64 path_len;
	u64 image_len;
	u64 vector_len = 0;
	int result;

	if (linux == 0 || reply_port == 0 || load_path == 0 || args == 0 ||
	    image == 0) {
		return -LINUX_ENOSYS;
	}
	path_len = str_len(load_path) + 1;
	image_len = min_u64(image_size, LINUX_SHEBANG_MAX);
	if (path_len == 1 || path_len > LINUX_EXEC_MAX_PATH ||
	    image_len == 0) {
		return -LINUX_EINVAL;
	}

	buffer = buffer_create(LINUX_EXEC_PREPARE_BUFFER_SIZE);
	if (buffer == 0) {
		return -LINUX_ENOMEM;
	}
	if (buffer_write(buffer, LINUX_EXEC_PREPARE_PATH_OFF,
			 load_path, path_len) != 0 ||
	    buffer_write(buffer, LINUX_EXEC_PREPARE_IMAGE_OFF,
			 image, image_len) != 0 ||
	    linux_exec_serialize_args(buffer, LINUX_EXEC_PREPARE_VECTOR_OFF,
				      args, &vector_len) != 0) {
		buffer_release(buffer);
		return -LINUX_EFAULT;
	}

	request.cap_object = buffer;
	request.words[0] = path_len;
	request.words[1] = image_len;
	request.words[2] = vector_len;
	if (linux_forward_message(linux, reply_port, &request, &reply) != 0) {
		buffer_release(buffer);
		return -LINUX_ENOSYS;
	}
	result = (int)(i64)reply.words[0];
	if (result == 0) {
		if (reply.words[1] == 0 ||
		    reply.words[1] > LINUX_EXEC_MAX_PATH ||
		    reply.words[2] == 0 ||
		    buffer_read(buffer, LINUX_EXEC_PREPARE_PATH_OFF,
				load_path, reply.words[1]) != 0 ||
		    load_path[reply.words[1] - 1] != '\0') {
			result = -LINUX_EINVAL;
		} else {
			result = linux_exec_deserialize_args(
				buffer, LINUX_EXEC_PREPARE_VECTOR_OFF,
				reply.words[2], args);
		}
	}
	buffer_release(buffer);
	return result;
}

static void linux_debug_exec_log(struct task *task, const char *phase,
				 const char *path, int result)
{
	if (!kernel_cmdline_has("debug-linux-exec")) {
		return;
	}

	console_write("linux-exec-raw: task=");
	console_write_hex64(task != 0 ? task_id(task) : 0);
	console_write(" phase=");
	console_write(phase != 0 ? phase : "");
	console_write(" result=");
	console_write_hex64((u64)(i64)result);
	console_write(" path=");
	console_write(path != 0 ? path : "");
	console_write("\n");
	console_printf("linux-exec: task=%u phase=%s result=%d path=%s\n",
		       task != 0 ? task_id(task) : 0, phase, result,
		       path != 0 ? path : "");
}

static int linux_exec_replace_write_string(struct shared_buffer *buffer,
					   u64 *offset, u64 limit,
					   const char *text)
{
	const u64 len = str_len(text) + 1;

	if (buffer == 0 || offset == 0 || text == 0 || len == 1 ||
	    len > LINUX_EXEC_MAX_STRING ||
	    *offset + len < *offset ||
	    *offset + len > limit ||
	    buffer_write(buffer, *offset, text, len) != 0) {
		return -1;
	}

	*offset += len;
	return 0;
}

static void linux_exec_credentials_load(struct task *task, u64 creds[5])
{
	struct ipc_port *linux = ipc_port_find("linux");
	struct ipc_port *reply_port = task_reply_port(task);
	u64 value;

	creds[0] = 0;
	creds[1] = 0;
	creds[2] = 0;
	creds[3] = 0;
	creds[4] = 0;
	if (linux == 0 || reply_port == 0) {
		return;
	}

	value = linux_forward_words(linux, reply_port, LINUX_SYSCALL_GETUID,
				    0, 0, 0);
	if ((i64)value >= 0) {
		creds[0] = value;
	}
	value = linux_forward_words(linux, reply_port, LINUX_SYSCALL_GETGID,
				    0, 0, 0);
	if ((i64)value >= 0) {
		creds[1] = value;
	}
	value = linux_forward_words(linux, reply_port, LINUX_SYSCALL_GETEUID,
				    0, 0, 0);
	if ((i64)value >= 0) {
		creds[2] = value;
	}
	value = linux_forward_words(linux, reply_port, LINUX_SYSCALL_GETEGID,
				    0, 0, 0);
	if ((i64)value >= 0) {
		creds[3] = value;
	}
}

static int linux_exec_replace_buffer_create(const char *load_path,
					    const char *execfn_path,
					    const struct linux_exec_args *args,
					    const u64 creds[5],
					    struct shared_buffer **out,
					    u64 *total_out)
{
	u64 total;
	u64 offset = 0;
	struct shared_buffer *buffer;

	if (load_path == 0 || execfn_path == 0 || args == 0 || creds == 0 ||
	    out == 0 || total_out == 0 || args->argc == 0 ||
	    args->argc > LINUX_EXEC_MAX_POINTERS ||
	    args->envc > LINUX_EXEC_MAX_POINTERS) {
		return -LINUX_EINVAL;
	}

	total = 5 * sizeof(u64) + str_len(load_path) + 1 +
		str_len(execfn_path) + 1;
	for (u64 i = 0; i < args->argc; i++) {
		total += str_len(args->argv[i]) + 1;
	}
	for (u64 i = 0; i < args->envc; i++) {
		total += str_len(args->envp[i]) + 1;
	}
	if (total == 0 || total > LINUX_EXEC_MAX_STRING_BYTES ||
	    total < str_len(load_path) + 1) {
		return -LINUX_E2BIG;
	}

	buffer = buffer_create(total);
	if (buffer == 0) {
		return -LINUX_ENOMEM;
	}
	if (buffer_write(buffer, 0, creds, 5 * sizeof(u64)) != 0) {
		buffer_release(buffer);
		return -LINUX_EFAULT;
	}
	offset = 5 * sizeof(u64);
	if (linux_exec_replace_write_string(buffer, &offset, total,
					    load_path) != 0 ||
	    linux_exec_replace_write_string(buffer, &offset, total,
					    execfn_path) != 0) {
		buffer_release(buffer);
		return -LINUX_EFAULT;
	}
	for (u64 i = 0; i < args->argc; i++) {
		if (linux_exec_replace_write_string(buffer, &offset, total,
						    args->argv[i]) != 0) {
			buffer_release(buffer);
			return -LINUX_EFAULT;
		}
	}
	for (u64 i = 0; i < args->envc; i++) {
		if (linux_exec_replace_write_string(buffer, &offset, total,
						    args->envp[i]) != 0) {
			buffer_release(buffer);
			return -LINUX_EFAULT;
		}
	}
	if (offset != total) {
		buffer_release(buffer);
		return -LINUX_EFAULT;
	}

	*out = buffer;
	*total_out = total;
	return 0;
}

static int linux_proc_exec_replace(struct task *task, const char *load_path,
				   const char *execfn_path,
				   const struct linux_exec_args *args,
				   u64 *entry, u64 *stack)
{
	struct ipc_port *proc = ipc_port_find("proc");
	struct ipc_port *reply_port = task_reply_port(task);
	struct ipc_message request = {
		.protocol = USER_FOURCC_PROC,
		.type = USER_PROC_EXEC_REPLACE_TASK,
		.sender = 0,
		.cap_rights = TASK_RIGHT_SEND | TASK_RIGHT_DUP,
		.reply_port = reply_port,
		.cap_type = IPC_CAP_TASK,
		.cap_object = task,
		.words = { 0, 0, 0, 0 },
	};
	struct ipc_message reply;
	struct shared_buffer *buffer = 0;
	u64 creds[5];
	u64 total = 0;
	u64 token;
	int result;

	if (proc == 0 || reply_port == 0 || load_path == 0 ||
	    execfn_path == 0 || args == 0 || entry == 0 || stack == 0) {
		return -LINUX_ENOSYS;
	}

	linux_debug_exec_log(task, "replace-creds-start", load_path, 0);
	linux_exec_credentials_load(task, creds);
	linux_debug_exec_log(task, "replace-creds-done", load_path, 0);
	linux_debug_exec_log(task, "replace-buffer-start", load_path, 0);
	result = linux_exec_replace_buffer_create(load_path, execfn_path,
						  args, creds, &buffer, &total);
	if (result != 0) {
		linux_debug_exec_log(task, "replace-buffer", load_path,
				     result);
		return result;
	}
	linux_debug_exec_log(task, "replace-buffer-done", load_path, 0);

	linux_debug_exec_log(task, "replace-token-start", load_path, 0);
	if (ipc_send(proc, &request) != 0 ||
	    ipc_recv(reply_port, &reply) != 0 ||
	    reply.words[0] != 0 ||
	    reply.words[1] == 0) {
		buffer_release(buffer);
		linux_debug_exec_log(task, "replace-token", load_path,
				     -LINUX_ENOSYS);
		return -LINUX_ENOSYS;
	}
	token = reply.words[1];
	linux_debug_exec_log(task, "replace-token-done", load_path, 0);

	request = (struct ipc_message){
		.protocol = USER_FOURCC_PROC,
		.type = USER_PROC_EXEC_REPLACE_BUFFER,
		.sender = 0,
		.cap_rights = TASK_RIGHT_RECV,
		.reply_port = reply_port,
		.cap_type = IPC_CAP_BUFFER,
		.cap_object = buffer,
		.words = { token, total, args->argc, args->envc },
	};
	linux_debug_exec_log(task, "replace-rpc-start", load_path, 0);
	if (ipc_send(proc, &request) != 0 ||
	    ipc_recv(reply_port, &reply) != 0 ||
	    reply.words[0] != 0 ||
	    reply.words[1] == 0 ||
	    reply.words[2] == 0) {
		buffer_release(buffer);
		linux_debug_exec_log(task, "replace-rpc", load_path,
				     -LINUX_EINVAL);
		return -LINUX_EINVAL;
	}
	linux_debug_exec_log(task, "replace-rpc-done", load_path, 0);

	buffer_release(buffer);
	*entry = reply.words[1];
	*stack = reply.words[2];
	return 0;
}

static u64 linux_execve(struct task *task, struct arch_syscall_frame *frame,
			const char *user_path) __attribute__((noinline));
static u64 linux_exec_process(struct task *task);
static void linux_proc_set_cmdline(struct task *task, u64 linux_pid,
				   const struct linux_exec_args *args);

static u64 linux_execve(struct task *task, struct arch_syscall_frame *frame,
			const char *user_path)
{
	char path[LINUX_EXEC_MAX_PATH];
	char load_path[LINUX_EXEC_MAX_PATH];
	struct linux_exec_args args;
	u8 prefix[LINUX_SHEBANG_MAX];
	u64 prefix_size = 0;
	u64 new_sp = 0;
	u64 entry = 0;
	int read_result;
	int path_result;
	int arg_result;
	int replace_result;

	path_result = copy_cstr_from_user_errno(path, user_path, sizeof(path));
	if (path_result != 0) {
		linux_debug_exec_log(task, "copy-path", "", path_result);
		return (u64)path_result;
	}
	linux_debug_exec_log(task, "copy-path", path, 0);
	mem_copy((u8 *)load_path, (const u8 *)path, str_len(path) + 1);
	arg_result = linux_exec_collect_args(path, frame->arg1, &args);
	if (arg_result == 0) {
		arg_result = linux_exec_collect_env(frame->arg2, &args);
	}
	if (arg_result != 0) {
		linux_debug_exec_log(task, "collect-args", path, arg_result);
		linux_exec_args_free(&args);
		return (u64)arg_result;
	}
	linux_debug_exec_log(task, "collect-args", path, 0);

	for (u64 depth = 0; depth < LINUX_SHEBANG_MAX_DEPTH; depth++) {
		read_result = linux_vfs_read_prefix(task, load_path, prefix,
						    sizeof(prefix),
						    &prefix_size);
		if (read_result != 0) {
			linux_debug_exec_log(task, "read-prefix", load_path,
					     read_result);
			linux_exec_args_free(&args);
			return read_result == -2 ? (u64)-LINUX_ENOMEM :
			       (read_result == -1 ? (u64)-LINUX_EIO :
				(u64)read_result);
		}
		linux_debug_exec_log(task, "read-prefix", load_path, 0);
		if (prefix_size < 2 || prefix[0] != '#' || prefix[1] != '!') {
			break;
		}
		if (depth + 1 == LINUX_SHEBANG_MAX_DEPTH) {
			linux_debug_exec_log(task, "shebang-depth", load_path,
					     -LINUX_EINVAL);
			linux_exec_args_free(&args);
			return linux_einval_u64(__func__, __LINE__);
		}
		const int script_result =
			linux_exec_prepare_shebang(task, load_path, &args,
						   prefix, prefix_size);
		if (script_result != 0) {
			linux_debug_exec_log(task, "prepare-shebang",
					     load_path, script_result);
			linux_exec_args_free(&args);
			return (u64)script_result;
		}
		linux_debug_exec_log(task, "prepare-shebang", load_path, 0);
	}

	replace_result = linux_proc_exec_replace(task, load_path, path, &args,
						 &entry, &new_sp);
	if (replace_result != 0) {
		linux_debug_exec_log(task, "replace", load_path,
				     replace_result);
		linux_exec_args_free(&args);
		return (u64)replace_result;
	}
	linux_debug_exec_log(task, "replace", load_path, 0);

	task_set_linux_brk(task, LINUX_INITIAL_BRK);
	task_set_linux_mmap_next(task, LINUX_MMAP_BASE);
	frame->user_rip = entry;
	frame->user_rsp = new_sp;
	const u64 exec_result = linux_exec_process(task);
	if ((i64)exec_result < 0) {
		linux_debug_exec_log(task, "linux-exec-process", load_path,
				     (int)(i64)exec_result);
		linux_exec_args_free(&args);
		return exec_result;
	}
	linux_debug_exec_log(task, "linux-exec-process", load_path, 0);
	linux_proc_set_cmdline(task, exec_result, &args);
	linux_vfork_complete_task(task_id(task));
	console_printf("linux: execve task=%u path=%s entry=%p stack=%p\n",
		       task_id(task), path, (const void *)entry,
		       (const void *)new_sp);
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
				   const struct linux_exec_args *args)
{
	struct ipc_port *proc = ipc_port_find("proc");
	struct ipc_port *reply_port = task_reply_port(task);
	struct shared_buffer *buffer;
	u64 len = 0;
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

	if (proc == 0 || reply_port == 0 || linux_pid == 0 ||
	    args == 0 || args->argc == 0) {
		return;
	}
	for (u64 i = 0; i < args->argc; i++) {
		const u64 arg_len = str_len(args->argv[i]) + 1;

		if (arg_len == 1 || len + arg_len < len ||
		    len + arg_len > LINUX_PROC_CMDLINE_MAX) {
			return;
		}
		len += arg_len;
	}
	if (len == 0) {
		return;
	}
	buffer = buffer_create(len);
	if (buffer == 0) {
		buffer_release(buffer);
		return;
	}
	for (u64 i = 0, offset = 0; i < args->argc; i++) {
		const u64 arg_len = str_len(args->argv[i]) + 1;

		if (buffer_write(buffer, offset, args->argv[i], arg_len) != 0) {
			buffer_release(buffer);
			return;
		}
		offset += arg_len;
	}
	request.cap_object = buffer;
	request.words[2] = len;
	(void)(ipc_send(proc, &request) == 0 &&
	       ipc_recv(reply_port, &reply) == 0);
	buffer_release(buffer);
}

static int linux_task_id_from_pid(u64 linux_pid, u64 *task_id_out)
{
	struct ipc_port *proc = ipc_port_find("proc");
	struct ipc_port *reply_port = task_reply_port(task_current());
	struct ipc_message request = {
		.protocol = USER_FOURCC_PROC,
		.type = USER_PROC_INFO_BY_LINUX_PID,
		.sender = 0,
		.cap_rights = 0,
		.reply_port = reply_port,
		.cap_type = IPC_CAP_NONE,
		.cap_object = 0,
		.words = { linux_pid, 0, 0, 0 },
	};
	struct ipc_message reply;

	if (task_id_out == 0) {
		return -LINUX_EINVAL;
	}
	if (linux_pid == 0) {
		*task_id_out = task_id(task_current());
		return 0;
	}
	if (proc == 0 || reply_port == 0 ||
	    ipc_send(proc, &request) != 0 ||
	    ipc_recv(reply_port, &reply) != 0 ||
	    reply.words[0] != 0 || reply.words[2] == 0) {
		return -LINUX_ESRCH;
	}
	*task_id_out = reply.words[2];
	return 0;
}

static u64 linux_sched_authority_handle(void)
{
	struct sched_policy_cap *authority = sched_policy_cap_create_system();
	u64 handle;

	if (authority == 0) {
		return 0;
	}
	handle = task_grant_sched_policy(task_current(), authority,
					 TASK_RIGHT_SEND);
	sched_policy_cap_release(authority);
	return handle;
}

static int linux_sched_get_task_state(u64 linux_pid,
				      struct sched_policy_state *state,
				      u64 *authority_out)
{
	u64 task_id_value;
	u64 authority;
	int result;

	if (state == 0 || authority_out == 0) {
		return -LINUX_EINVAL;
	}
	result = linux_task_id_from_pid(linux_pid, &task_id_value);
	if (result != 0) {
		return result;
	}
	authority = linux_sched_authority_handle();
	if (authority == 0) {
		return -LINUX_EPERM;
	}
	*state = (struct sched_policy_state){
		.target_kind = SCHED_POLICY_TARGET_TASK,
		.target_id = task_id_value,
	};
	if (sched_policy_get(task_current(), authority, state) != 0) {
		(void)task_close_handle(task_current(), authority);
		return -LINUX_ESRCH;
	}
	*authority_out = authority;
	return 0;
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
	case LINUX_SYSCALL_SELECT:
		return "select";
	case LINUX_SYSCALL_SCHED_YIELD:
		return "sched_yield";
	case LINUX_SYSCALL_SCHED_SETAFFINITY:
		return "sched_setaffinity";
	case LINUX_SYSCALL_SCHED_GETAFFINITY:
		return "sched_getaffinity";
	case LINUX_SYSCALL_GETPRIORITY:
		return "getpriority";
	case LINUX_SYSCALL_SETPRIORITY:
		return "setpriority";
	case LINUX_SYSCALL_SCHED_SETPARAM:
		return "sched_setparam";
	case LINUX_SYSCALL_SCHED_GETPARAM:
		return "sched_getparam";
	case LINUX_SYSCALL_SCHED_SETSCHEDULER:
		return "sched_setscheduler";
	case LINUX_SYSCALL_SCHED_GETSCHEDULER:
		return "sched_getscheduler";
	case LINUX_SYSCALL_SCHED_GET_PRIORITY_MAX:
		return "sched_get_priority_max";
	case LINUX_SYSCALL_SCHED_GET_PRIORITY_MIN:
		return "sched_get_priority_min";
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
	case LINUX_SYSCALL_RT_SIGRETURN:
		return "rt_sigreturn";
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
	case LINUX_SYSCALL_SYMLINK:
		return "symlink";
	case LINUX_SYSCALL_NANOSLEEP:
		return "nanosleep";
	case LINUX_SYSCALL_DUP:
		return "dup";
	case LINUX_SYSCALL_DUP2:
		return "dup2";
	case LINUX_SYSCALL_ALARM:
		return "alarm";
	case LINUX_SYSCALL_SETITIMER:
		return "setitimer";
	case LINUX_SYSCALL_SENDFILE:
		return "sendfile";
	case LINUX_SYSCALL_SOCKET:
		return "socket";
	case LINUX_SYSCALL_CONNECT:
		return "connect";
	case LINUX_SYSCALL_GETSOCKNAME:
		return "getsockname";
	case LINUX_SYSCALL_GETPEERNAME:
		return "getpeername";
	case LINUX_SYSCALL_SETSOCKOPT:
		return "setsockopt";
	case LINUX_SYSCALL_GETSOCKOPT:
		return "getsockopt";
	case LINUX_SYSCALL_SENDTO:
		return "sendto";
	case LINUX_SYSCALL_RECVFROM:
		return "recvfrom";
	case LINUX_SYSCALL_SENDMSG:
		return "sendmsg";
	case LINUX_SYSCALL_RECVMSG:
		return "recvmsg";
	case LINUX_SYSCALL_GETCWD:
		return "getcwd";
	case LINUX_SYSCALL_CHDIR:
		return "chdir";
	case LINUX_SYSCALL_RENAME:
		return "rename";
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
	case LINUX_SYSCALL_MKNOD:
		return "mknod";
	case LINUX_SYSCALL_LINK:
		return "link";
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
	case LINUX_SYSCALL_READLINK:
		return "readlink";
	case LINUX_SYSCALL_FCHMODAT:
		return "fchmodat";
	case LINUX_SYSCALL_FACCESSAT:
		return "faccessat";
	case LINUX_SYSCALL_FACCESSAT2:
		return "faccessat2";
	case LINUX_SYSCALL_PPOLL:
		return "ppoll";
	case LINUX_SYSCALL_PSELECT6:
		return "pselect6";
	case LINUX_SYSCALL_SET_ROBUST_LIST:
		return "set_robust_list";
	case LINUX_SYSCALL_DUP3:
		return "dup3";
	case LINUX_SYSCALL_PIPE2:
		return "pipe2";
	case LINUX_SYSCALL_CLOSE_RANGE:
		return "close_range";
	case LINUX_SYSCALL_OPENAT:
		return "openat";
	case LINUX_SYSCALL_MKDIRAT:
		return "mkdirat";
	case LINUX_SYSCALL_MKNODAT:
		return "mknodat";
	case LINUX_SYSCALL_FCHOWNAT:
		return "fchownat";
	case LINUX_SYSCALL_LINKAT:
		return "linkat";
	case LINUX_SYSCALL_RENAMEAT:
		return "renameat";
	case LINUX_SYSCALL_UNLINKAT:
		return "unlinkat";
	case LINUX_SYSCALL_SYMLINKAT:
		return "symlinkat";
	case LINUX_SYSCALL_PRLIMIT64:
		return "prlimit64";
	case LINUX_SYSCALL_UTIMENSAT:
		return "utimensat";
	case LINUX_SYSCALL_RENAMEAT2:
		return "renameat2";
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

#define LINUX_SYSCALL_COUNT_MAX 512
#define LINUX_SYSCALL_COUNT_PERIOD 1000
#define LINUX_SYSCALL_COUNT_TOP 10

static u64 linux_syscall_counts[LINUX_SYSCALL_COUNT_MAX];
static u64 linux_syscall_count_total;
static u64 linux_syscall_count_next = LINUX_SYSCALL_COUNT_PERIOD;

static void linux_syscall_count_log(const struct arch_syscall_frame *frame)
{
	static int enabled = -1;
	u64 used[LINUX_SYSCALL_COUNT_TOP];

	if (enabled < 0) {
		enabled = kernel_cmdline_has("debug-linux-syscall-counts") != 0;
	}
	if (!enabled) {
		return;
	}

	linux_syscall_count_total++;
	if (frame->number < LINUX_SYSCALL_COUNT_MAX) {
		linux_syscall_counts[frame->number]++;
	}
	if (linux_syscall_count_total < linux_syscall_count_next) {
		return;
	}
	linux_syscall_count_next += LINUX_SYSCALL_COUNT_PERIOD;

	for (u64 i = 0; i < LINUX_SYSCALL_COUNT_TOP; i++) {
		used[i] = (u64)-1;
	}

	for (u64 rank = 0; rank < LINUX_SYSCALL_COUNT_TOP; rank++) {
		u64 best_nr = (u64)-1;
		u64 best_count = 0;

		for (u64 nr = 0; nr < LINUX_SYSCALL_COUNT_MAX; nr++) {
			int already_used = 0;

			for (u64 j = 0; j < rank; j++) {
				if (used[j] == nr) {
					already_used = 1;
					break;
				}
			}
			if (!already_used && linux_syscall_counts[nr] > best_count) {
				best_nr = nr;
				best_count = linux_syscall_counts[nr];
			}
		}
		if (best_count == 0) {
			break;
		}
		used[rank] = best_nr;
		console_printf("linux-syscall-counts: total=%u rank=%u nr=%u syscall=%s count=%u\n",
			       (u32)linux_syscall_count_total, (u32)(rank + 1),
			       (u32)best_nr, linux_syscall_name(best_nr),
			       (u32)best_count);
	}
}

static void linux_sigprocmask_shape_log(u64 how, u64 set, int has_set,
					int has_old)
{
	enum {
		SHAPE_GET_ONLY,
		SHAPE_BLOCK,
		SHAPE_UNBLOCK,
		SHAPE_SETMASK,
		SHAPE_OTHER,
		SHAPE_COUNT,
	};
	static int enabled = -1;
	static u64 total;
	static u64 counts[SHAPE_COUNT];
	u64 shape;

	if (enabled < 0) {
		enabled = kernel_cmdline_has("debug-linux-syscall-counts") != 0;
	}
	if (!enabled) {
		return;
	}

	if (!has_set) {
		shape = SHAPE_GET_ONLY;
	} else if (how == 0) {
		shape = SHAPE_BLOCK;
	} else if (how == 1) {
		shape = SHAPE_UNBLOCK;
	} else if (how == 2) {
		shape = SHAPE_SETMASK;
	} else {
		shape = SHAPE_OTHER;
	}

	total++;
	counts[shape]++;
	if ((total & 255) != 0) {
		return;
	}

	console_printf("linux-sigprocmask-counts: total=%u get=%u block=%u unblock=%u setmask=%u other=%u last_how=%u last_has_set=%u last_has_old=%u last_set=%p\n",
		       (u32)total, (u32)counts[SHAPE_GET_ONLY],
		       (u32)counts[SHAPE_BLOCK], (u32)counts[SHAPE_UNBLOCK],
		       (u32)counts[SHAPE_SETMASK], (u32)counts[SHAPE_OTHER],
		       (u32)how, (u32)has_set, (u32)has_old,
		       (const void *)set);
}

static void linux_read_shape_log(u64 fd, u64 len)
{
	enum {
		READ_LEN_ZERO,
		READ_LEN_ONE,
		READ_LEN_TINY,
		READ_LEN_SMALL,
		READ_LEN_MEDIUM,
		READ_LEN_LARGE,
		READ_FD_BUCKETS = 8,
	};
	static int enabled = -1;
	static u64 total;
	static u64 len_counts[6];
	static u64 fd_counts[READ_FD_BUCKETS];
	u64 len_bucket;
	u64 fd_bucket = fd < READ_FD_BUCKETS - 1 ? fd : READ_FD_BUCKETS - 1;

	if (enabled < 0) {
		enabled = kernel_cmdline_has("debug-linux-syscall-counts") != 0;
	}
	if (!enabled) {
		return;
	}

	if (len == 0) {
		len_bucket = READ_LEN_ZERO;
	} else if (len == 1) {
		len_bucket = READ_LEN_ONE;
	} else if (len <= 16) {
		len_bucket = READ_LEN_TINY;
	} else if (len <= 256) {
		len_bucket = READ_LEN_SMALL;
	} else if (len <= 4096) {
		len_bucket = READ_LEN_MEDIUM;
	} else {
		len_bucket = READ_LEN_LARGE;
	}

	total++;
	len_counts[len_bucket]++;
	fd_counts[fd_bucket]++;
	if ((total & 255) != 0) {
		return;
	}

	console_printf("linux-read-counts: total=%u len0=%u len1=%u len2_16=%u len17_256=%u len257_4096=%u len4097p=%u fd0=%u fd1=%u fd2=%u fd3=%u fd4=%u fd5=%u fd6=%u fd7p=%u last_fd=%u last_len=%u\n",
		       (u32)total, (u32)len_counts[READ_LEN_ZERO],
		       (u32)len_counts[READ_LEN_ONE],
		       (u32)len_counts[READ_LEN_TINY],
		       (u32)len_counts[READ_LEN_SMALL],
		       (u32)len_counts[READ_LEN_MEDIUM],
		       (u32)len_counts[READ_LEN_LARGE],
		       (u32)fd_counts[0], (u32)fd_counts[1],
		       (u32)fd_counts[2], (u32)fd_counts[3],
		       (u32)fd_counts[4], (u32)fd_counts[5],
		       (u32)fd_counts[6], (u32)fd_counts[7],
		       (u32)fd, (u32)len);
}

static void linux_mmap_shape_log(u64 len, u64 prot, u64 flags, u32 task_prot)
{
	enum {
		MMAP_LEN_4K,
		MMAP_LEN_16K,
		MMAP_LEN_64K,
		MMAP_LEN_1M,
		MMAP_LEN_LARGE,
		MMAP_LEN_COUNT,
	};
	static int enabled = -1;
	static u64 total;
	static u64 len_counts[MMAP_LEN_COUNT];
	static u64 anonymous;
	static u64 file;
	static u64 writable;
	static u64 fixed;
	u64 bucket;

	if (enabled < 0) {
		enabled = kernel_cmdline_has("debug-linux-syscall-counts") != 0;
	}
	if (!enabled) {
		return;
	}

	if (len <= 4096) {
		bucket = MMAP_LEN_4K;
	} else if (len <= 16384) {
		bucket = MMAP_LEN_16K;
	} else if (len <= 65536) {
		bucket = MMAP_LEN_64K;
	} else if (len <= 1024 * 1024) {
		bucket = MMAP_LEN_1M;
	} else {
		bucket = MMAP_LEN_LARGE;
	}

	total++;
	len_counts[bucket]++;
	if ((flags & LINUX_MAP_ANONYMOUS) != 0) {
		anonymous++;
	} else {
		file++;
	}
	if ((task_prot & TASK_VM_PROT_WRITE) != 0) {
		writable++;
	}
	if ((flags & LINUX_MAP_FIXED) != 0) {
		fixed++;
	}
	if ((total & 255) != 0) {
		return;
	}

	console_printf("linux-mmap-counts: total=%u anon=%u file=%u writable=%u fixed=%u len4k=%u len16k=%u len64k=%u len1m=%u lenlarge=%u last_len=%u last_prot=0x%x last_flags=0x%x\n",
		       (u32)total, (u32)anonymous, (u32)file,
		       (u32)writable, (u32)fixed,
		       (u32)len_counts[MMAP_LEN_4K],
		       (u32)len_counts[MMAP_LEN_16K],
		       (u32)len_counts[MMAP_LEN_64K],
		       (u32)len_counts[MMAP_LEN_1M],
		       (u32)len_counts[MMAP_LEN_LARGE],
		       (u32)len, (u32)prot, (u32)flags);
}

static void linux_munmap_shape_log(u64 len)
{
	static int enabled = -1;
	static u64 total;
	static u64 len4k;
	static u64 len16k;
	static u64 len64k;
	static u64 len1m;
	static u64 lenlarge;

	if (enabled < 0) {
		enabled = kernel_cmdline_has("debug-linux-syscall-counts") != 0;
	}
	if (!enabled) {
		return;
	}
	total++;
	if (len <= 4096) {
		len4k++;
	} else if (len <= 16384) {
		len16k++;
	} else if (len <= 65536) {
		len64k++;
	} else if (len <= 1024 * 1024) {
		len1m++;
	} else {
		lenlarge++;
	}
	if ((total & 255) != 0) {
		return;
	}
	console_printf("linux-munmap-counts: total=%u len4k=%u len16k=%u len64k=%u len1m=%u lenlarge=%u last_len=%u\n",
		       (u32)total, (u32)len4k, (u32)len16k, (u32)len64k,
		       (u32)len1m, (u32)lenlarge, (u32)len);
}

static void linux_poll_shape_log(u64 nfds, u64 timeout_ns,
				 int infinite_timeout, u64 ready, int slept)
{
	static int enabled = -1;
	static u64 total;
	static u64 nfds0;
	static u64 nfds1;
	static u64 nfds_many;
	static u64 timeout_zero;
	static u64 timeout_finite;
	static u64 timeout_infinite;
	static u64 immediate_ready;
	static u64 slept_count;
	static u64 empty;

	if (enabled < 0) {
		enabled = kernel_cmdline_has("debug-linux-syscall-counts") != 0;
	}
	if (!enabled) {
		return;
	}
	total++;
	if (nfds == 0) {
		nfds0++;
	} else if (nfds == 1) {
		nfds1++;
	} else {
		nfds_many++;
	}
	if (timeout_ns == 0) {
		timeout_zero++;
	} else if (infinite_timeout) {
		timeout_infinite++;
	} else {
		timeout_finite++;
	}
	if (ready != 0 && !slept) {
		immediate_ready++;
	}
	if (slept) {
		slept_count++;
	}
	if (ready == 0) {
		empty++;
	}
	if ((total & 255) != 0) {
		return;
	}
	console_printf("linux-poll-counts: total=%u nfds0=%u nfds1=%u nfdsmany=%u timeout0=%u timeoutfinite=%u timeoutinf=%u immediate=%u slept=%u empty=%u last_nfds=%u last_ready=%u last_slept=%u\n",
		       (u32)total, (u32)nfds0, (u32)nfds1, (u32)nfds_many,
		       (u32)timeout_zero, (u32)timeout_finite,
		       (u32)timeout_infinite, (u32)immediate_ready,
		       (u32)slept_count, (u32)empty, (u32)nfds,
		       (u32)ready, (u32)slept);
}

enum linux_strace_mode {
	LINUX_STRACE_LEGACY = 0,
	LINUX_STRACE_STRUCTURED,
	LINUX_STRACE_OFF,
};

static u32 linux_strace_mode = LINUX_STRACE_LEGACY;

static int str_token_eq(const char *left, const char *right)
{
	while (*left != '\0' && *left != ' ' && *right != '\0') {
		if (*left++ != *right++) {
			return 0;
		}
	}

	return (*left == '\0' || *left == ' ') && *right == '\0';
}

void arch_user_set_strace_mode(const char *mode)
{
	if (mode == 0 ||
	    str_token_eq(mode, "legacy") ||
	    str_token_eq(mode, "human")) {
		linux_strace_mode = LINUX_STRACE_LEGACY;
		return;
	}
	if (str_token_eq(mode, "structured") || str_token_eq(mode, "kv")) {
		linux_strace_mode = LINUX_STRACE_STRUCTURED;
		return;
	}
	if (str_token_eq(mode, "off") || str_token_eq(mode, "none")) {
		linux_strace_mode = LINUX_STRACE_OFF;
		return;
	}

	linux_strace_mode = LINUX_STRACE_LEGACY;
}

static void linux_strace_log(const struct arch_syscall_frame *frame, u64 result)
{
	const struct task *task = task_current();
	const struct thread *thread = thread_current();
	const u64 number = frame->number;

	if (linux_strace_mode == LINUX_STRACE_OFF) {
		return;
	}
	if (linux_strace_mode == LINUX_STRACE_STRUCTURED) {
		console_printf("linux-strace phase=exit bunix_task=%u bunix_tid=%u task_name=%s thread_name=%s nr=%u syscall=%s rip=%p ret=%p arg0=%p arg1=%p arg2=%p arg3=%p\n",
			       task_id(task), thread_id(thread), task_name(task),
			       thread_name(thread), (u32)number,
			       linux_syscall_name(number),
			       (const void *)frame->user_rip,
			       (const void *)result,
			       (const void *)frame->arg0,
			       (const void *)frame->arg1,
			       (const void *)frame->arg2,
			       (const void *)frame->arg3);
		return;
	}

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
	const struct thread *thread = thread_current();
	const u64 number = frame->number;

	if (linux_strace_mode == LINUX_STRACE_OFF) {
		return;
	}
	if (linux_strace_mode == LINUX_STRACE_STRUCTURED) {
		console_printf("linux-strace phase=enter bunix_task=%u bunix_tid=%u task_name=%s thread_name=%s nr=%u syscall=%s rip=%p arg0=%p arg1=%p arg2=%p arg3=%p\n",
			       task_id(task), thread_id(thread), task_name(task),
			       thread_name(thread), (u32)number,
			       linux_syscall_name(number),
			       (const void *)frame->user_rip,
			       (const void *)frame->arg0,
			       (const void *)frame->arg1,
			       (const void *)frame->arg2,
			       (const void *)frame->arg3);
		return;
	}

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

static u64 linux_restore_signal_frame(struct arch_syscall_frame *frame,
				      struct ipc_port *linux,
				      struct ipc_port *reply_port)
{
	struct linux_signal_frame signal_frame;
	const u64 frame_addr = frame->user_rsp;

	if (read_current_user(frame_addr, &signal_frame,
			      sizeof(signal_frame)) != 0 ||
	    signal_frame.magic != LINUX_SIGNAL_FRAME_MAGIC) {
		return (u64)-LINUX_EFAULT;
	}

	(void)linux_forward_words(linux, reply_port,
				  LINUX_SYSCALL_RT_SIGPROCMASK,
				  2, signal_frame.old_mask, 0);
	*frame = signal_frame.frame;
	return signal_frame.saved_rax;
}

static u64 linux_signal_pending(struct ipc_port *linux,
				struct ipc_port *reply_port)
{
	return linux_forward_words(linux, reply_port, LINUX_RPC_SIGNAL_PENDING,
				   0, 0, 0);
}

static int linux_signal_dequeue(struct ipc_port *linux,
				struct ipc_port *reply_port,
				u64 *signal, u64 *handler,
				u64 *restorer, u64 *old_mask)
{
	struct ipc_message request = {
		.protocol = USER_FOURCC_LINX,
		.type = LINUX_RPC_SIGNAL_DEQUEUE,
		.sender = 0,
		.cap_rights = 0,
		.reply_port = reply_port,
		.cap_type = IPC_CAP_NONE,
		.cap_object = 0,
		.words = { 0, 0, 0, 0 },
	};
	struct ipc_message reply;

	if (linux_forward_message(linux, reply_port, &request, &reply) != 0 ||
	    (i64)reply.words[0] < 0) {
		return -1;
	}
	*signal = reply.words[0];
	*handler = reply.words[1];
	*restorer = reply.words[2];
	*old_mask = reply.words[3];
	return 0;
}

static int linux_deliver_signal(struct arch_syscall_frame *frame,
				struct ipc_port *linux,
				struct ipc_port *reply_port,
				u64 saved_rax)
{
	struct linux_signal_frame signal_frame;
	u64 signal;
	u64 handler;
	u64 restorer;
	u64 old_mask;
	u64 frame_addr;

	if (linux_signal_dequeue(linux, reply_port, &signal, &handler,
				 &restorer, &old_mask) != 0 ||
	    signal == 0 || handler == 0 || restorer == 0) {
		return 0;
	}

	signal_frame.magic = LINUX_SIGNAL_FRAME_MAGIC;
	signal_frame.saved_rax = saved_rax;
	signal_frame.old_mask = old_mask;
	signal_frame.frame = *frame;

	frame_addr = align_down(frame->user_rsp - sizeof(signal_frame) - 8, 16) - 8;
	if (write_current_user(frame_addr, &restorer, sizeof(restorer)) != 0 ||
	    write_current_user(frame_addr + 8, &signal_frame,
			       sizeof(signal_frame)) != 0) {
		return -1;
	}

	frame->arg0 = signal;
	frame->arg1 = 0;
	frame->arg2 = 0;
	frame->arg3 = 0;
	frame->r8 = 0;
	frame->r9 = 0;
	frame->user_rip = handler;
	frame->user_rsp = frame_addr;
	return 1;
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
	case LINUX_SYSCALL_RT_SIGRETURN:
		return linux_restore_signal_frame(frame, linux, reply_port);
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
		u64 populated_len;

		linux_mmap_shape_log(len, prot, flags, task_prot);

		if (len == 0 || (flags & LINUX_MAP_PRIVATE) == 0 ||
		    len + VM_PAGE_SIZE - 1 < len ||
		    (!anonymous && (offset & (VM_PAGE_SIZE - 1)) != 0)) {
			return linux_einval_u64(__func__, __LINE__);
		}

		len = align_up(len, VM_PAGE_SIZE);
		populated_len = len;
		if (base == 0) {
			base = task_linux_mmap_next(task);
		} else if ((flags & LINUX_MAP_FIXED) == 0) {
			base = align_up(base, VM_PAGE_SIZE);
		} else if ((base & (VM_PAGE_SIZE - 1)) != 0) {
			return linux_einval_u64(__func__, __LINE__);
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
			return linux_einval_u64(__func__, __LINE__);
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
					anonymous ? TASK_VM_OBJECT_ANON :
						    TASK_VM_OBJECT_FILE,
					anonymous ? 0 : (u32)fd,
					anonymous ? 0 : offset) != 0) {
			(void)vm_unmap_user_range(task_vm_space(task), base, len);
			return (u64)-LINUX_ENOMEM;
		}
		if (!anonymous &&
		    linux_mmap_file_into_task(task, linux, reply_port, base, len,
					     fd, offset, &populated_len) != 0) {
			(void)linux_unmap_task_range(task, base, len);
			return linux_einval_u64(__func__, __LINE__);
		}
		if (!anonymous && !writable && populated_len != 0 &&
		    vm_protect_user_range(task_vm_space(task), base,
					  align_up(populated_len, VM_PAGE_SIZE),
					  0) != 0) {
			(void)linux_unmap_task_range(task, base, len);
			return linux_einval_u64(__func__, __LINE__);
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
			return linux_einval_u64(__func__, __LINE__);
		}

		len = align_up(len, VM_PAGE_SIZE);
		linux_munmap_shape_log(len);
		if (linux_unmap_task_range(task, base, len) != 0) {
			return linux_einval_u64(__func__, __LINE__);
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
		child = is_vfork ?
			server_task_vfork_current_stopped(&child_frame) :
			server_task_fork_current_stopped(&child_frame);
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
			return linux_einval_u64(__func__, __LINE__);
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
				return linux_einval_u64(__func__, __LINE__);
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
			return linux_einval_u64(__func__, __LINE__);
		}
	}
	case LINUX_SYSCALL_SET_ROBUST_LIST:
		return 0;
	case LINUX_SYSCALL_RT_SIGACTION: {
		u64 handler = ~0ull;
		u64 flags = 0;
		u64 restorer = 0;

		if (arg3 != 8) {
			return linux_einval_u64(__func__, __LINE__);
		}
		if (arg1 != 0) {
			if (read_current_user(arg1, &handler,
					      sizeof(handler)) != 0 ||
			    read_current_user(arg1 + 8, &flags,
					      sizeof(flags)) != 0 ||
			    read_current_user(arg1 + 16, &restorer,
					      sizeof(restorer)) != 0) {
				return (u64)-LINUX_EFAULT;
			}
		}
		request.type = LINUX_SYSCALL_RT_SIGACTION;
		request.words[0] = arg0;
		request.words[1] = handler;
		request.words[2] = flags;
		request.words[3] = restorer;
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
			mem_copy(action + 8, (const u8 *)&reply.words[2],
				 sizeof(reply.words[2]));
			mem_copy(action + 16, (const u8 *)&reply.words[3],
				 sizeof(reply.words[3]));
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
			return linux_einval_u64(__func__, __LINE__);
		}
		if (arg1 != 0 &&
		    read_current_user(arg1, &set, sizeof(set)) != 0) {
			return (u64)-LINUX_EFAULT;
		}
		if (how != ~0ull && how != 0 && how != 1 && how != 2) {
			return linux_einval_u64(__func__, __LINE__);
		}
		linux_sigprocmask_shape_log(how, set, arg1 != 0, arg2 != 0);
		request.type = LINUX_SYSCALL_RT_SIGPROCMASK;
		request.words[0] = how;
		request.words[1] = set;
		request.words[2] = 0;
		if (arg2 == 0) {
			return linux_send_message(linux, &request) == 0 ?
			       0 : (u64)-LINUX_ENOSYS;
		}
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
			return linux_einval_u64(__func__, __LINE__);
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
			return linux_einval_u64(__func__, __LINE__);
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
				return linux_einval_u64(__func__, __LINE__);
			}
			console_printf("linux: mprotect task=%u addr=%p len=%u prot=0x%x\n",
				       task_id(task), (const void *)arg0,
				       (u32)len, (u32)arg2);
		}
		return 0;
	case LINUX_SYSCALL_IOCTL:
		if (arg1 == LINUX_TCGETS || arg1 == LINUX_TCSETS ||
		    arg1 == LINUX_TIOCGPGRP || arg1 == LINUX_TIOCSPGRP ||
		    arg1 == LINUX_TIOCGWINSZ ||
		    arg1 == LINUX_SIOCGIFFLAGS ||
		    arg1 == LINUX_SIOCGIFHWADDR ||
		    arg1 == LINUX_SIOCGIFMTU ||
		    arg1 == LINUX_SIOCGIFINDEX) {
			break;
		}
		return (u64)-LINUX_ENOTTY;
	case LINUX_SYSCALL_FCNTL:
		break;
	case LINUX_SYSCALL_SCHED_YIELD:
		thread_yield();
		return 0;
	case LINUX_SYSCALL_SCHED_SETAFFINITY: {
		struct sched_policy_state state;
		u64 authority;
		u64 cpu_mask;
		int result;

		if (arg1 < sizeof(cpu_mask) || arg2 == 0 ||
		    read_current_user(arg2, &cpu_mask, sizeof(cpu_mask)) != 0) {
			return (u64)-LINUX_EINVAL;
		}
		result = linux_sched_get_task_state(arg0, &state, &authority);
		if (result != 0) {
			return (u64)(i64)result;
		}
		state.cpu_mask = cpu_mask;
		result = sched_policy_set(task_current(), authority, &state) == 0 ?
				 0 :
				 -LINUX_EINVAL;
		(void)task_close_handle(task_current(), authority);
		return (u64)(i64)result;
	}
	case LINUX_SYSCALL_SCHED_GETAFFINITY: {
		struct sched_policy_state state;
		u64 authority;
		int result;

		if (arg1 < sizeof(state.cpu_mask) || arg2 == 0) {
			return (u64)-LINUX_EINVAL;
		}
		result = linux_sched_get_task_state(arg0, &state, &authority);
		if (result != 0) {
			return (u64)(i64)result;
		}
		result = write_current_user(arg2, &state.cpu_mask,
					    sizeof(state.cpu_mask)) == 0 ?
				 (int)sizeof(state.cpu_mask) :
				 -LINUX_EFAULT;
		(void)task_close_handle(task_current(), authority);
		return (u64)(i64)result;
	}
	case LINUX_SYSCALL_SCHED_GET_PRIORITY_MAX:
		return arg0 == 0 || arg0 == 1 || arg0 == 2 ? 127 :
		       (u64)-LINUX_EINVAL;
	case LINUX_SYSCALL_SCHED_GET_PRIORITY_MIN:
		return arg0 == 0 || arg0 == 1 || arg0 == 2 ? 0 :
		       (u64)-LINUX_EINVAL;
	case LINUX_SYSCALL_SCHED_GETPARAM: {
		struct sched_policy_state state;
		u64 authority;
		u32 priority;
		int result;

		if (arg1 == 0) {
			return (u64)-LINUX_EFAULT;
		}
		result = linux_sched_get_task_state(arg0, &state, &authority);
		if (result != 0) {
			return (u64)(i64)result;
		}
		priority = (u32)state.priority;
		result = write_current_user(arg1, &priority, sizeof(priority)) == 0 ?
				 0 :
				 -LINUX_EFAULT;
		(void)task_close_handle(task_current(), authority);
		return (u64)(i64)result;
	}
	case LINUX_SYSCALL_SCHED_SETPARAM: {
		struct sched_policy_state state;
		u64 authority;
		u32 priority;
		int result;

		if (arg1 == 0 ||
		    read_current_user(arg1, &priority, sizeof(priority)) != 0) {
			return (u64)-LINUX_EFAULT;
		}
		result = linux_sched_get_task_state(arg0, &state, &authority);
		if (result != 0) {
			return (u64)(i64)result;
		}
		state.priority = priority;
		result = sched_policy_set(task_current(), authority, &state) == 0 ?
				 0 :
				 -LINUX_EINVAL;
		(void)task_close_handle(task_current(), authority);
		return (u64)(i64)result;
	}
	case LINUX_SYSCALL_SCHED_GETSCHEDULER: {
		struct sched_policy_state state;
		u64 authority;
		u64 policy;
		int result = linux_sched_get_task_state(arg0, &state, &authority);

		if (result != 0) {
			return (u64)(i64)result;
		}
		policy = state.sched_class == SCHED_CLASS_BATCH ? 3 :
			 state.sched_class == SCHED_CLASS_IDLE ? 5 : 0;
		(void)task_close_handle(task_current(), authority);
		return policy;
	}
	case LINUX_SYSCALL_SCHED_SETSCHEDULER: {
		struct sched_policy_state state;
		u64 authority;
		u32 priority = 0;
		int result;

		if (arg2 != 0 &&
		    read_current_user(arg2, &priority, sizeof(priority)) != 0) {
			return (u64)-LINUX_EFAULT;
		}
		result = linux_sched_get_task_state(arg0, &state, &authority);
		if (result != 0) {
			return (u64)(i64)result;
		}
		state.sched_class = arg1 == 3 ? SCHED_CLASS_BATCH :
				    arg1 == 5 ? SCHED_CLASS_IDLE :
				    SCHED_CLASS_USER;
		state.priority = priority;
		result = (arg1 == 0 || arg1 == 1 || arg1 == 2 ||
			  arg1 == 3 || arg1 == 5) &&
					 sched_policy_set(task_current(),
							  authority,
							  &state) == 0 ?
				 0 :
				 -LINUX_EINVAL;
		(void)task_close_handle(task_current(), authority);
		return (u64)(i64)result;
	}
	case LINUX_SYSCALL_GETPRIORITY: {
		struct sched_policy_state state;
		u64 authority;
		int result;

		if (arg0 != 0) {
			return (u64)-LINUX_EINVAL;
		}
		result = linux_sched_get_task_state(arg1, &state, &authority);
		if (result != 0) {
			return (u64)(i64)result;
		}
		(void)task_close_handle(task_current(), authority);
		return state.priority > 40 ? 0 : 20 - state.priority;
	}
	case LINUX_SYSCALL_SETPRIORITY: {
		struct sched_policy_state state;
		u64 authority;
		i64 nice = (i64)arg2;
		int result;

		if (arg0 != 0 || nice < -20 || nice > 19) {
			return (u64)-LINUX_EINVAL;
		}
		result = linux_sched_get_task_state(arg1, &state, &authority);
		if (result != 0) {
			return (u64)(i64)result;
		}
		state.priority = (u64)(20 - nice);
		result = sched_policy_set(task_current(), authority, &state) == 0 ?
				 0 :
				 -LINUX_EINVAL;
		(void)task_close_handle(task_current(), authority);
		return (u64)(i64)result;
	}
	case LINUX_SYSCALL_NANOSLEEP:
		return linux_sleep_relative(arg0, arg1);
	case LINUX_SYSCALL_ALARM:
		break;
	case LINUX_SYSCALL_SETITIMER: {
		u64 timer[4];
		u64 old_timer[4] = { 0, 0, 0, 0 };

		if (arg0 != 0) {
			return linux_einval_u64(__func__, __LINE__);
		}
		if (arg2 != 0 &&
		    write_current_user(arg2, old_timer, sizeof(old_timer)) != 0) {
			return (u64)-LINUX_EFAULT;
		}
		if (arg1 == 0 ||
		    read_current_user(arg1, timer, sizeof(timer)) != 0) {
			return (u64)-LINUX_EFAULT;
		}
		return linux_forward_words(linux, reply_port,
					   LINUX_SYSCALL_SETITIMER,
					   arg0, timer[2], timer[3]);
	}
	case LINUX_SYSCALL_CLOCK_NANOSLEEP:
		if (arg0 != LINUX_CLOCK_REALTIME &&
		    arg0 != LINUX_CLOCK_MONOTONIC) {
			return linux_einval_u64(__func__, __LINE__);
		}
		if ((arg1 & ~LINUX_TIMER_ABSTIME) != 0) {
			return linux_einval_u64(__func__, __LINE__);
		}
		if ((arg1 & LINUX_TIMER_ABSTIME) != 0) {
			return linux_sleep_absolute(arg2);
		}
		return linux_sleep_relative(arg2, arg3);
	case LINUX_SYSCALL_PRLIMIT64:
		if (arg3 != 0) {
			u64 limit[2] = { 0x800000, 0x800000 };

			if (write_current_user(arg3, limit, sizeof(limit)) != 0) {
				return linux_einval_u64(__func__, __LINE__);
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

		if (arg1 > 64 || (arg1 != 0 && arg0 == 0)) {
			return linux_einval_u64(__func__, __LINE__);
		}
		if (linux == 0 || reply_port == 0) {
			return (u64)-LINUX_ENOSYS;
		}
		timeout_ns = linux_poll_timeout_ns(number, arg2);
		infinite_timeout = linux_poll_timeout_is_infinite(number, arg2);
poll_again:
		ready = 0;
		for (u64 i = 0; i < arg1; i++) {
			const u64 addr = arg0 + i * sizeof(pollfd);
			u64 revents_result;
			short revents;

			if (read_current_user(addr, &pollfd, sizeof(pollfd)) != 0) {
				return (u64)-LINUX_EFAULT;
			}
			revents_result =
				linux_forward_words(linux, reply_port, number,
						    (u64)(long)pollfd.fd,
						    (u64)(unsigned short)pollfd.events,
						    0);
			if ((i64)revents_result < 0) {
				return revents_result;
			}
			revents = (short)revents_result;
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
			const u64 pending = linux_signal_pending(linux,
								 reply_port);

			if ((i64)pending < 0) {
				return pending;
			}
			if (pending != 0) {
				return (u64)-LINUX_EINTR;
			}
			slept = 1;
			thread_sleep_ns(infinite_timeout ? 10000000ull : timeout_ns);
			const u64 woke_pending = linux_signal_pending(linux,
								      reply_port);

			if ((i64)woke_pending < 0) {
				return woke_pending;
			}
			if (woke_pending != 0) {
				return (u64)-LINUX_EINTR;
			}
			goto poll_again;
		}
		linux_poll_shape_log(arg1, timeout_ns, infinite_timeout, ready,
				     slept);
		return ready;
	}
	case LINUX_SYSCALL_WRITEV: {
		struct {
			u64 base;
			u64 len;
		} iov;
		u64 total = 0;

		if (arg2 == 0) {
			return 0;
		}
		if (linux == 0 || reply_port == 0) {
			return (u64)-LINUX_ENOSYS;
		}
		if (arg1 == 0) {
			return (u64)-LINUX_EFAULT;
		}
		if (arg2 > LINUX_IOV_MAX) {
			return linux_einval_u64(__func__, __LINE__);
		}
		for (u64 i = 0; i < arg2; i++) {
			if (read_current_user(arg1 + i * sizeof(iov), &iov,
					      sizeof(iov)) != 0) {
				return (u64)-LINUX_EFAULT;
			}
			if (iov.base == 0 && iov.len != 0) {
				return (u64)-LINUX_EFAULT;
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
	if (number == LINUX_SYSCALL_FCNTL && arg1 == LINUX_F_GETLK) {
		u16 lock_type = LINUX_F_UNLCK;
		const u64 result = linux_forward_words(linux, reply_port, number,
						       arg0, arg1, arg2);

		if ((i64)result < 0) {
			return result;
		}
		if (arg2 == 0 ||
		    write_current_user(arg2, &lock_type, sizeof(lock_type)) != 0) {
			return (u64)-LINUX_EFAULT;
		}
		return result;
	}
	if (linux_syscall_forwards_scalar(number)) {
		return linux_forward_words(linux, reply_port, number,
					   arg0, arg1, arg2);
	}

	switch (number) {
	case LINUX_SYSCALL_READ: {
		return linux_read_chunked(linux, reply_port, arg0, arg1,
					  arg2, TASK_RIGHT_SEND |
					  TASK_RIGHT_RECV | TASK_RIGHT_DUP);
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
			       linux_einval_u64(__func__, __LINE__);
		}
		request.type = LINUX_SYSCALL_CONNECT;
		request.words[0] = arg0;
		request.words[1] = len;
		request.words[2] = 0;
		request.words[3] = 0;
		return linux_forward_input_buffer(linux, reply_port, &request,
						  arg1, len, TASK_RIGHT_RECV);
	}
	case LINUX_SYSCALL_BIND: {
		const u64 len = arg2 > LINUX_MAX_SOCKADDR ?
				LINUX_MAX_SOCKADDR : arg2;

		if (arg1 == 0 || len == 0) {
			return arg1 == 0 ? (u64)-LINUX_EFAULT :
			       linux_einval_u64(__func__, __LINE__);
		}
		request.type = LINUX_SYSCALL_BIND;
		request.words[0] = arg0;
		request.words[1] = len;
		request.words[2] = 0;
		request.words[3] = 0;
		return linux_forward_input_buffer(linux, reply_port, &request,
						  arg1, len, TASK_RIGHT_RECV);
	}
	case LINUX_SYSCALL_ACCEPT4:
		return linux_forward_words(linux, reply_port,
					   LINUX_SYSCALL_ACCEPT4,
					   arg0, arg3, 0);
	case LINUX_SYSCALL_SENDTO: {
		const u64 len = arg2 > LINUX_MAX_SYSCALL_BUFFER ?
				LINUX_MAX_SYSCALL_BUFFER : arg2;
		const u64 dest_addr = frame->r8;
		const u64 addr_len = frame->r9 > LINUX_MAX_SOCKADDR ?
				     LINUX_MAX_SOCKADDR : frame->r9;

		if (arg1 == 0 && len != 0) {
			return (u64)-LINUX_EFAULT;
		}
		if (dest_addr != 0 && addr_len != 0) {
			struct shared_buffer *buffer;
			struct ipc_message reply;
			u64 flags;

			buffer = buffer_create(addr_len + (len == 0 ? 1 : len));
			if (buffer == 0) {
				return (u64)-LINUX_ENOMEM;
			}
			flags = spin_lock_irqsave(&syscall_copy_lock);
			if (read_current_user(dest_addr, syscall_copy_buffer,
					      addr_len) != 0 ||
			    buffer_write(buffer, 0, syscall_copy_buffer,
					 addr_len) != 0) {
				spin_unlock_irqrestore(&syscall_copy_lock,
						       flags);
				buffer_release(buffer);
				return (u64)-LINUX_EFAULT;
			}
			for (u64 done = 0; done < len;) {
				const u64 chunk = min_u64(len - done,
							  LINUX_MAX_SYSCALL_BUFFER);

				if (read_current_user(arg1 + done,
						      syscall_copy_buffer,
						      chunk) != 0 ||
				    buffer_write(buffer, addr_len + done,
						 syscall_copy_buffer,
						 chunk) != 0) {
					spin_unlock_irqrestore(&syscall_copy_lock,
							       flags);
					buffer_release(buffer);
					return (u64)-LINUX_EFAULT;
				}
				done += chunk;
			}
			spin_unlock_irqrestore(&syscall_copy_lock, flags);
			request.type = LINUX_SYSCALL_SENDTO;
			request.words[0] = arg0;
			request.words[1] = len;
			request.words[2] = arg3;
			request.words[3] = addr_len;
			request.cap_type = IPC_CAP_BUFFER;
			request.cap_rights = TASK_RIGHT_RECV | TASK_RIGHT_DUP;
			request.cap_object = buffer;
			if (linux_forward_message(linux, reply_port, &request,
						  &reply) != 0) {
				buffer_release(buffer);
				return (u64)-LINUX_ENOSYS;
			}
			buffer_release(buffer);
			return reply.words[0];
		}
		request.type = LINUX_SYSCALL_SENDTO;
		request.words[0] = arg0;
		request.words[1] = len;
		request.words[2] = arg3;
		request.words[3] = 0;
		return linux_forward_input_buffer(linux, reply_port, &request,
						  arg1, len,
						  TASK_RIGHT_RECV |
						  TASK_RIGHT_DUP);
	}
	case LINUX_SYSCALL_RECVFROM: {
		const u64 len = arg2 > LINUX_MAX_SYSCALL_BUFFER ?
				LINUX_MAX_SYSCALL_BUFFER : arg2;

		return linux_forward_recvfrom(linux, reply_port, &request,
					      arg0, arg1, len, arg3,
					      frame->r8, frame->r9);
	}
	case LINUX_SYSCALL_SENDMSG:
		return linux_forward_sendmsg(linux, reply_port, &request,
					     arg0, arg1, arg2);
	case LINUX_SYSCALL_RECVMSG:
		return linux_forward_recvmsg(linux, reply_port, &request,
					     arg0, arg1, arg2);
	case LINUX_SYSCALL_SELECT:
		return linux_forward_select(linux, reply_port, arg0, arg1,
					    arg2, arg3);
	case LINUX_SYSCALL_PSELECT6:
		return linux_forward_select(linux, reply_port, arg0, arg1,
					    arg2, arg3);
	case LINUX_SYSCALL_GETSOCKNAME:
	case LINUX_SYSCALL_GETPEERNAME:
		return linux_forward_socklen_output(linux, reply_port, &request,
						    (u32)number, arg0,
						    arg1, arg2);
	case LINUX_SYSCALL_SETSOCKOPT: {
		const u64 len = frame->r8 > LINUX_MAX_SOCKADDR ?
				LINUX_MAX_SOCKADDR : frame->r8;

		if (arg3 == 0 && len != 0) {
			return (u64)-LINUX_EFAULT;
		}
		request.type = LINUX_SYSCALL_SETSOCKOPT;
		request.words[0] = arg0;
		request.words[1] = arg1;
		request.words[2] = arg2;
		request.words[3] = len;
		return linux_forward_input_buffer(linux, reply_port, &request,
						  arg3, len, TASK_RIGHT_RECV);
	}
	case LINUX_SYSCALL_GETSOCKOPT:
		return linux_forward_getsockopt(linux, reply_port, &request,
						arg0, arg1, arg2, arg3,
						frame->r8);
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
			       linux_einval_u64(__func__, __LINE__);
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
		return linux_forward_groups_out(linux, reply_port, &request,
						arg0, arg1);
	case LINUX_SYSCALL_SETGROUPS: {
		const u64 size = arg0 * sizeof(u32);

		if (arg0 > LINUX_MAX_GROUPS ||
		    (arg0 != 0 && size / sizeof(u32) != arg0)) {
			return linux_einval_u64(__func__, __LINE__);
		}
		if (arg0 != 0 && arg1 == 0) {
			return (u64)-LINUX_EFAULT;
		}

		request.type = LINUX_SYSCALL_SETGROUPS;
		request.words[0] = arg0;
		request.words[1] = 0;
		request.words[2] = 0;
		request.words[3] = 0;
		return linux_forward_input_buffer(linux, reply_port, &request,
						  arg1, size,
						  TASK_RIGHT_RECV |
						  TASK_RIGHT_DUP);
	}
	case LINUX_SYSCALL_IOCTL: {
		struct shared_buffer *buffer = 0;
		u64 output_size = 0;
		u32 value = 0;

		if (arg1 != LINUX_TCGETS && arg1 != LINUX_TCSETS &&
		    arg1 != LINUX_TCSETSW && arg1 != LINUX_TCSETSF &&
		    arg1 != LINUX_TIOCGPGRP && arg1 != LINUX_TIOCSPGRP &&
		    arg1 != LINUX_TIOCGWINSZ &&
		    arg1 != LINUX_SIOCGIFFLAGS &&
		    arg1 != LINUX_SIOCGIFHWADDR &&
		    arg1 != LINUX_SIOCGIFMTU &&
		    arg1 != LINUX_SIOCGIFINDEX) {
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
		} else if (arg1 == LINUX_SIOCGIFFLAGS ||
			   arg1 == LINUX_SIOCGIFHWADDR ||
			   arg1 == LINUX_SIOCGIFMTU ||
			   arg1 == LINUX_SIOCGIFINDEX) {
			output_size = LINUX_IFREQ_SIZE;
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
			if ((arg1 == LINUX_SIOCGIFFLAGS ||
			     arg1 == LINUX_SIOCGIFHWADDR ||
			     arg1 == LINUX_SIOCGIFMTU ||
			     arg1 == LINUX_SIOCGIFINDEX) &&
			    read_current_user(arg2, syscall_copy_buffer,
					      output_size) != 0) {
				buffer_release(buffer);
				return (u64)-LINUX_EFAULT;
			}
			if ((arg1 == LINUX_SIOCGIFFLAGS ||
			     arg1 == LINUX_SIOCGIFHWADDR ||
			     arg1 == LINUX_SIOCGIFMTU ||
			     arg1 == LINUX_SIOCGIFINDEX) &&
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
		if (arg1 == LINUX_SIOCGIFFLAGS ||
		    arg1 == LINUX_SIOCGIFHWADDR ||
		    arg1 == LINUX_SIOCGIFMTU ||
		    arg1 == LINUX_SIOCGIFINDEX) {
			request.cap_rights = TASK_RIGHT_SEND | TASK_RIGHT_RECV;
		}
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
	case LINUX_SYSCALL_MKNOD:
	case LINUX_SYSCALL_MKNODAT:
	case LINUX_SYSCALL_CHMOD:
	case LINUX_SYSCALL_FCHMODAT:
	case LINUX_SYSCALL_CHOWN:
	case LINUX_SYSCALL_LCHOWN:
	case LINUX_SYSCALL_FCHOWNAT: {
		const int is_mkdir = number == LINUX_SYSCALL_MKDIR ||
				     number == LINUX_SYSCALL_MKDIRAT;
		const int is_mknod = number == LINUX_SYSCALL_MKNOD ||
				     number == LINUX_SYSCALL_MKNODAT;
		const int is_chown = number == LINUX_SYSCALL_CHOWN ||
				     number == LINUX_SYSCALL_LCHOWN ||
				     number == LINUX_SYSCALL_FCHOWNAT;
		const int is_at = number == LINUX_SYSCALL_MKDIRAT ||
				  number == LINUX_SYSCALL_MKNODAT ||
				  number == LINUX_SYSCALL_FCHMODAT ||
				  number == LINUX_SYSCALL_FCHOWNAT;
		const char *path = is_at ? (const char *)arg1 :
				   (const char *)arg0;
		const u64 dirfd = is_at ? arg0 : (u64)-100;
		const u64 mode = is_at ? arg2 : arg1;
		const u64 owner = is_at ? arg2 : arg1;
		const u64 group = is_at ? arg3 : arg2;
		const u64 flags = number == LINUX_SYSCALL_FCHMODAT ? 0 :
				  (number == LINUX_SYSCALL_FCHOWNAT ?
				   frame->r8 :
				   (number == LINUX_SYSCALL_LCHOWN ?
				    LINUX_AT_SYMLINK_NOFOLLOW : 0));
		const u32 type = is_mkdir ? LINUX_SYSCALL_MKDIRAT :
				 (is_mknod ? LINUX_SYSCALL_MKNODAT :
				 (is_chown ? LINUX_SYSCALL_FCHOWNAT :
				  LINUX_SYSCALL_FCHMODAT));
		const u64 word2 = is_chown ? owner : mode;
		const u64 word3 = is_mknod ? (is_at ? arg3 : arg2) :
				  (is_chown ?
				  ((flags & 0xffffffff) << 32) |
				  (group & 0xffffffff) : flags);

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
			       linux_einval_u64(__func__, __LINE__);
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
	case LINUX_SYSCALL_SYMLINK:
	case LINUX_SYSCALL_SYMLINKAT: {
		return linux_forward_symlinkat(linux, reply_port, &request,
					       (const char *)arg0,
					       number == LINUX_SYSCALL_SYMLINK ?
					       (u64)-100 : arg1,
					       number == LINUX_SYSCALL_SYMLINK ?
					       (const char *)arg1 :
					       (const char *)arg2);
	}
	case LINUX_SYSCALL_LINK:
	case LINUX_SYSCALL_LINKAT: {
		const u64 olddirfd = number == LINUX_SYSCALL_LINK ?
				     (u64)-100 : arg0;
		const char *oldpath = number == LINUX_SYSCALL_LINK ?
				      (const char *)arg0 :
				      (const char *)arg1;
		const u64 newdirfd = number == LINUX_SYSCALL_LINK ?
				     (u64)-100 : arg2;
		const char *newpath = number == LINUX_SYSCALL_LINK ?
				      (const char *)arg1 :
				      (const char *)arg3;
		const u64 flags = number == LINUX_SYSCALL_LINKAT ?
				  frame->r8 : 0;

		return linux_forward_two_path_at(linux, reply_port, &request,
						 LINUX_SYSCALL_LINKAT,
						 olddirfd, oldpath, newdirfd,
						 newpath, flags);
	}
	case LINUX_SYSCALL_RENAME:
	case LINUX_SYSCALL_RENAMEAT:
	case LINUX_SYSCALL_RENAMEAT2: {
		const u64 olddirfd = number == LINUX_SYSCALL_RENAME ?
				     (u64)-100 : arg0;
		const char *oldpath = number == LINUX_SYSCALL_RENAME ?
				      (const char *)arg0 :
				      (const char *)arg1;
		const u64 newdirfd = number == LINUX_SYSCALL_RENAME ?
				     (u64)-100 : arg2;
		const char *newpath = number == LINUX_SYSCALL_RENAME ?
				      (const char *)arg1 :
				      (const char *)arg3;
		const u64 flags = number == LINUX_SYSCALL_RENAMEAT2 ?
				  frame->r8 : 0;

		return linux_forward_renameat2(linux, reply_port, &request,
					       olddirfd, oldpath, newdirfd,
					       newpath, flags);
	}
	case LINUX_SYSCALL_UTIMENSAT: {
		const int time_result = linux_validate_utimens(arg2);

		if (time_result != 0) {
			return (u64)time_result;
		}
		request.type = LINUX_SYSCALL_UTIMENSAT;
		request.words[0] = arg0;
		request.words[3] = 0;
		return linux_forward_utimensat(linux, reply_port, &request,
					       &reply, (const char *)arg1,
					       arg2, arg3);
	}
	case LINUX_SYSCALL_STAT:
	case LINUX_SYSCALL_LSTAT:
	case LINUX_SYSCALL_NEWFSTATAT: {
		struct shared_buffer *buffer;
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
		request.words[2] = flags;
		request.words[3] = 0;
		buffer = linux_forward_path_buffer(linux, reply_port, &request,
						   &reply, path,
						   LINUX_STAT_SIZE, 1,
						   TASK_RIGHT_RECV |
						   TASK_RIGHT_SEND,
						   &result);
		if (buffer == 0) {
			return result;
		}
		if (result == 0 &&
		    linux_copy_buffer_to_user(buffer, 0, stat_addr,
					      LINUX_STAT_SIZE) != 0) {
			buffer_release(buffer);
			return (u64)-LINUX_EFAULT;
		}
		buffer_release(buffer);
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
	struct task *task = task_current();
	struct ipc_port *linux = ipc_port_find("linux");
	struct ipc_port *reply_port = task_reply_port(task);
	u64 result;
	u64 debug_task = 0;
	const int debug_late =
		(kernel_cmdline_has("debug-linux-syscalls") &&
		 task_id(task) >= 140) ||
		(kernel_cmdline_get_u64("debug-linux-syscall-task=",
					&debug_task) &&
		 debug_task == task_id(task));

	if (debug_late) {
		console_printf("linux-debug: enter task=%u name=%s syscall=%s nr=%u rip=%p arg0=%p arg1=%p arg2=%p arg3=%p\n",
			       task_id(task), task_name(task),
			       linux_syscall_name(frame->number),
			       (u32)frame->number,
			       (const void *)frame->user_rip,
			       (const void *)frame->arg0,
			       (const void *)frame->arg1,
			       (const void *)frame->arg2,
			       (const void *)frame->arg3);
	}
	linux_syscall_count_log(frame);
	if (frame->number == LINUX_SYSCALL_READ) {
		linux_read_shape_log(frame->arg0, frame->arg2);
	}
	linux_strace_enter(frame);
	result = linux_syscall_handle(frame);
	linux_strace_log(frame, result);
	if (debug_late) {
		console_printf("linux-debug: exit task=%u name=%s syscall=%s nr=%u ret=%p\n",
			       task_id(task), task_name(task),
			       linux_syscall_name(frame->number),
			       (u32)frame->number, (const void *)result);
	}
	if (frame->number != LINUX_SYSCALL_RT_SIGRETURN &&
	    linux != 0 && reply_port != 0 &&
	    (i64)linux_signal_pending(linux, reply_port) > 0 &&
	    linux_deliver_signal(frame, linux, reply_port, result) < 0) {
		return (u64)-LINUX_EFAULT;
	}
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
		CR0_AM = 1u << 18,
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
	cr0 |= CR0_AM;
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
	arch_wrmsr(MSR_FMASK, RFLAGS_IF | RFLAGS_AC);
	arch_wrmsr(MSR_EFER, arch_rdmsr(MSR_EFER) | EFER_SCE);

	if (cpu_id == 0) {
		console_printf("user: gdt/tss/syscall ready\n");
	}
}

void arch_user_init(void)
{
	arch_user_init_cpu(arch_smp_current_cpu_id());
	(void)task_lifecycle_register_death_hook(linux_task_died);
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

u16 arch_user_elf_machine(void)
{
	return 62;
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
	u64 arg4;
	u64 arg5;
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

static u64 native_sys_cmdline_has(const struct native_syscall_args *args)
{
	char token[128];

	if (copy_cstr_from_user(token, (const char *)args->arg0,
				sizeof(token)) != 0) {
		return 0;
	}
	return kernel_cmdline_has(token) != 0 ? 1 : 0;
}

static u64 native_sys_launch_module(const struct native_syscall_args *args)
{
	char name[LINUX_EXEC_MAX_PATH];
	struct task_launch_cap caps[16];

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
		args->arg0 == 0 ? task_current() :
				  task_from_handle(task_current(), args->arg0,
						   TASK_RIGHT_SEND);
	u64 id;

	if (task == 0) {
		return (u64)-1;
	}
	id = task_id(task);
	if (args->arg0 != 0) {
		task_release(task);
	}
	return id;
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

static u64 native_sys_task_grant_tagged(const struct native_syscall_args *args)
{
	return server_task_grant_tagged(task_current(), args->arg0, args->arg1,
					(u32)args->arg2, (u32)args->arg3);
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

static u64 native_sys_task_clear(const struct native_syscall_args *args)
{
	return (u64)server_task_clear(task_current(), args->arg0);
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
			buffer_release(buffer);
			return (u64)-1;
		}
		done += chunk;
	}
	spin_unlock_irqrestore(&syscall_copy_lock, flags);
	buffer_release(buffer);
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
	buffer_release(buffer);
	return result;
}

static u64 native_sys_buffer_phys(const struct native_syscall_args *args)
{
	struct shared_buffer *buffer =
		task_buffer_from_handle(task_current(), args->arg0,
					TASK_RIGHT_SEND);

	if (buffer == 0) {
		return (u64)-1;
	}
	const u64 phys = buffer_phys(buffer);
	buffer_release(buffer);
	return phys;
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

static u64 native_sys_handle_find(const struct native_syscall_args *args)
{
	const u64 handle = task_handle_find(task_current(), (u32)args->arg0);

	return handle != 0 ? handle : (u64)-1;
}

static u64 native_sys_task_handle_find(const struct native_syscall_args *args)
{
	const u64 handle =
		server_task_handle_find(task_current(), args->arg0,
					(u32)args->arg1);

	return handle != 0 ? handle : (u64)-1;
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

static u64 native_sys_sched_stats(const struct native_syscall_args *args)
{
	struct sched_stats stats;

	if (args->arg0 == 0) {
		return (u64)-1;
	}
	sched_stats_snapshot(&stats);
	return write_current_user(args->arg0, &stats, sizeof(stats)) == 0 ?
	       0 : (u64)-1;
}

static u64 native_sys_sched_thread_info(const struct native_syscall_args *args)
{
	struct sched_thread_info info;

	if (args->arg1 == 0 ||
	    sched_thread_info_at(args->arg0, &info) != 0) {
		return (u64)-1;
	}
	return write_current_user(args->arg1, &info, sizeof(info)) == 0 ?
	       0 : (u64)-1;
}

struct user_sched_policy {
	u64 target_kind;
	u64 target_id;
	u64 rights;
	u64 class_mask;
	u64 min_priority;
	u64 max_priority;
	u64 min_weight;
	u64 max_weight;
	u64 cpu_mask;
	u64 delegation_depth;
};

static u64 native_sys_sched_policy_grant(
	const struct native_syscall_args *args)
{
	struct user_sched_policy user_policy;
	struct sched_policy_cap policy;
	struct task *target;
	u64 granted;

	if (args->arg2 == 0 ||
	    read_current_user(args->arg2, &user_policy,
			      sizeof(user_policy)) != 0) {
		return (u64)-1;
	}

	policy = (struct sched_policy_cap){
		.ref_count = 1,
		.target_kind = (u32)user_policy.target_kind,
		.rights = (u32)user_policy.rights,
		.delegation_depth = (u32)user_policy.delegation_depth,
		.target_id = user_policy.target_id,
		.class_mask = user_policy.class_mask,
		.min_priority = (u32)user_policy.min_priority,
		.max_priority = (u32)user_policy.max_priority,
		.min_weight = (u32)user_policy.min_weight,
		.max_weight = (u32)user_policy.max_weight,
		.cpu_mask = user_policy.cpu_mask,
	};
	target = task_from_handle(task_current(), args->arg1, TASK_RIGHT_SEND);
	if (target == 0) {
		return (u64)-1;
	}
	granted = sched_policy_grant(task_current(), args->arg0, target,
				     &policy);
	task_release(target);
	return granted != 0 ? granted : (u64)-1;
}

static u64 native_sys_sched_policy_get(const struct native_syscall_args *args)
{
	struct sched_policy_state state;

	if (args->arg1 == 0 ||
	    read_current_user(args->arg1, &state, sizeof(state)) != 0 ||
	    sched_policy_get(task_current(), args->arg0, &state) != 0) {
		return (u64)-1;
	}
	return write_current_user(args->arg1, &state, sizeof(state)) == 0 ?
	       0 : (u64)-1;
}

static u64 native_sys_sched_policy_set(const struct native_syscall_args *args)
{
	struct sched_policy_state state;

	if (args->arg1 == 0 ||
	    read_current_user(args->arg1, &state, sizeof(state)) != 0 ||
	    sched_policy_set(task_current(), args->arg0, &state) != 0) {
		return (u64)-1;
	}
	return 0;
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

static u64 native_sys_machine_power(const struct native_syscall_args *args)
{
	const struct task_hw_resource *authority =
		task_hw_resource_from_handle(task_current(), args->arg0,
					     TASK_RIGHT_SEND);

	if (authority == 0 ||
	    authority->type != TASK_HW_RESOURCE_POWER_AUTH ||
	    (authority->ops & TASK_HW_OP_POWER) == 0) {
		task_hw_resource_release(authority);
		return (u64)-1;
	}

	task_hw_resource_release(authority);
	arch_poweroff();
	return 0;
}

enum {
	PCI_VENDOR_VIRTIO = 0x1af4,
	PCI_VENDOR_NONE = 0xffff,
	PCI_BAR0 = 0x10,
	PCI_INTERRUPT_LINE = 0x3c,
	PCI_INTERRUPT_PIN = 0x3d,
	PCI_BAR_IO = 1,
	PCI_BAR_MEM_TYPE_MASK = 0x6,
	PCI_BAR_MEM_TYPE_64 = 0x4,
	PCI_CLASS_SERIAL_USB_XHCI = 0x0c0330,
};

static u32 pci_config_address(u64 bus, u64 slot, u64 function, u64 offset)
{
	return 0x80000000u |
	       ((u32)(bus & 0xff) << 16) |
	       ((u32)(slot & 0x1f) << 11) |
	       ((u32)(function & 0x7) << 8) |
	       (u32)(offset & 0xfc);
}

static u32 pci_config_read32(u64 bus, u64 slot, u64 function, u64 offset)
{
	arch_outl(0xcf8, pci_config_address(bus, slot, function, offset));
	return arch_inl(0xcfc);
}

static u8 pci_config_read8(u64 bus, u64 slot, u64 function, u64 offset)
{
	const u32 value = pci_config_read32(bus, slot, function, offset);

	return (value >> ((offset & 3) * 8)) & 0xff;
}

static void pci_config_write32(u64 bus, u64 slot, u64 function, u64 offset,
			       u32 value)
{
	arch_outl(0xcf8, pci_config_address(bus, slot, function, offset));
	arch_outl(0xcfc, value);
}

static int pci_device_is_virtio(u64 bus, u64 slot, u64 function)
{
	const u32 id = pci_config_read32(bus, slot, function, 0);
	const u32 vendor = id & 0xffff;
	const u32 device = id >> 16;

	if (vendor == PCI_VENDOR_NONE || vendor != PCI_VENDOR_VIRTIO) {
		return 0;
	}
	return (device >= 0x1000 && device <= 0x107f) ||
	       (device >= 0x1040 && device <= 0x107f);
}

static int pci_device_resource_allowed(u64 bus, u64 slot, u64 function)
{
	const u32 id = pci_config_read32(bus, slot, function, 0);
	const u32 vendor = id & 0xffff;
	const u32 class_revision = pci_config_read32(bus, slot, function, 0x08);
	const u32 class_code = (class_revision >> 8) & 0xffffffu;

	if (vendor == PCI_VENDOR_NONE) {
		return 0;
	}
	return pci_device_is_virtio(bus, slot, function) ||
	       class_code == PCI_CLASS_SERIAL_USB_XHCI;
}

static int pci_bar_info(u64 bus, u64 slot, u64 function, u64 bar,
			u32 *type, u64 *base, u64 *size)
{
	const u64 offset = PCI_BAR0 + bar * 4;
	const u32 original = pci_config_read32(bus, slot, function, offset);
	u32 original_high = 0;
	u32 mask;
	u32 mask_high = 0;

	if (bar >= 6 || original == 0 || type == 0 || base == 0 || size == 0) {
		return -1;
	}

	if ((original & PCI_BAR_IO) == 0 &&
	    (original & PCI_BAR_MEM_TYPE_MASK) == PCI_BAR_MEM_TYPE_64) {
		if (bar + 1 >= 6) {
			return -1;
		}
		original_high = pci_config_read32(bus, slot, function,
						  offset + 4);
		pci_config_write32(bus, slot, function, offset + 4,
				   0xffffffffu);
	}
	pci_config_write32(bus, slot, function, offset, 0xffffffffu);
	mask = pci_config_read32(bus, slot, function, offset);
	if ((original & PCI_BAR_IO) == 0 &&
	    (original & PCI_BAR_MEM_TYPE_MASK) == PCI_BAR_MEM_TYPE_64) {
		mask_high = pci_config_read32(bus, slot, function, offset + 4);
	}
	pci_config_write32(bus, slot, function, offset, original);
	if ((original & PCI_BAR_IO) == 0 &&
	    (original & PCI_BAR_MEM_TYPE_MASK) == PCI_BAR_MEM_TYPE_64) {
		pci_config_write32(bus, slot, function, offset + 4,
				   original_high);
	}
	if (mask == 0 || mask == 0xffffffffu) {
		return -1;
	}

	if ((original & PCI_BAR_IO) != 0) {
		*type = TASK_HW_RESOURCE_PORT;
		*base = original & ~0x3ull;
		*size = (~(mask & ~0x3u) + 1u) & 0xffffffffu;
		return *size != 0 ? 0 : -1;
	}

	*type = TASK_HW_RESOURCE_MMIO;
	if ((original & PCI_BAR_MEM_TYPE_MASK) == PCI_BAR_MEM_TYPE_64) {
		const u64 raw = ((u64)original_high << 32) |
				((u64)original & 0xffffffffu);
		const u64 mask64 = ((u64)mask_high << 32) |
				   ((u64)mask & 0xffffffffu);

		*base = raw & ~0xfull;
		*size = (~(mask64 & ~0xfull) + 1ull);
		return *size != 0 ? 0 : -1;
	}
	*base = original & ~0xfull;
	*size = (~(mask & ~0xfu) + 1u) & 0xffffffffu;
	return *size != 0 ? 0 : -1;
}

static u64 native_sys_hw_pci_bar_grant(const struct native_syscall_args *args)
{
	const u64 packed = args->arg0;
	const u64 bus = packed & 0xff;
	const u64 slot = (packed >> 8) & 0x1f;
	const u64 function = (packed >> 16) & 0x7;
	const u64 bar = (packed >> 24) & 0x7;
	const u64 offset = args->arg1;
	const u64 len = args->arg2;
	const u32 ops = (u32)args->arg3;
	const u64 authority_handle = args->arg4;
	const struct task_hw_resource *authority =
		task_hw_resource_from_handle(task_current(), authority_handle,
					     TASK_RIGHT_SEND);
	u32 type;
	u64 base;
	u64 size;

	if (authority == 0 ||
	    authority->type != TASK_HW_RESOURCE_PCI_AUTH ||
	    (authority->ops & TASK_HW_OP_GRANT) == 0 ||
	    len == 0 || (ops & ~(TASK_HW_OP_READ | TASK_HW_OP_WRITE)) != 0 ||
	    !pci_device_resource_allowed(bus, slot, function) ||
	    pci_bar_info(bus, slot, function, bar, &type, &base, &size) != 0 ||
	    offset > size || len > size - offset) {
		task_hw_resource_release(authority);
		return (u64)-1;
	}
	task_hw_resource_release(authority);

	struct task_hw_resource *resource =
		(struct task_hw_resource *)slab_alloc(sizeof(*resource));
	if (resource == 0) {
		return (u64)-1;
	}
	resource->type = type;
	resource->ops = ops;
	resource->flags = TASK_HW_RESOURCE_OWNED;
	resource->ref_count = 0;
	resource->base = base + offset;
	resource->len = len;
	const u64 handle =
		task_grant_hw_resource(task_current(), resource,
				       TASK_RIGHT_SEND | TASK_RIGHT_DUP);
	if (handle == 0) {
		slab_free(resource);
	}
	return handle;
}

static u64 native_sys_hw_pci_irq_grant(const struct native_syscall_args *args)
{
	const u64 packed = args->arg0;
	const u64 bus = packed & 0xff;
	const u64 slot = (packed >> 8) & 0x1f;
	const u64 function = (packed >> 16) & 0x7;
	const u64 requested_line = args->arg1;
	const u64 authority_handle = args->arg2;
	const struct task_hw_resource *authority =
		task_hw_resource_from_handle(task_current(), authority_handle,
					     TASK_RIGHT_SEND);
	const u64 line = pci_config_read8(bus, slot, function, PCI_INTERRUPT_LINE);
	const u64 pin = pci_config_read8(bus, slot, function, PCI_INTERRUPT_PIN);
	u32 gsi;

	if (authority == 0 ||
	    authority->type != TASK_HW_RESOURCE_PCI_AUTH ||
	    (authority->ops & TASK_HW_OP_GRANT) == 0 ||
	    !pci_device_resource_allowed(bus, slot, function) || pin == 0 ||
	    line == 0 || line == 0xff ||
	    (requested_line != 0 && requested_line != line)) {
		task_hw_resource_release(authority);
		return (u64)-1;
	}
	task_hw_resource_release(authority);

	gsi = (u32)line;
	(void)arch_smp_irq_override((u32)line, &gsi, 0);

	struct task_hw_resource *resource =
		(struct task_hw_resource *)slab_alloc(sizeof(*resource));
	if (resource == 0) {
		return (u64)-1;
	}
	resource->type = TASK_HW_RESOURCE_IRQ;
	resource->ops = TASK_HW_OP_BIND_IRQ | TASK_HW_OP_ACK_IRQ |
			TASK_HW_OP_MASK_IRQ;
	resource->flags = TASK_HW_RESOURCE_OWNED |
			  TASK_HW_RESOURCE_IRQ_LEVEL_LOW;
	resource->ref_count = 0;
	resource->base = gsi;
	resource->len = 1;
	const u64 handle =
		task_grant_hw_resource(task_current(), resource,
				       TASK_RIGHT_SEND | TASK_RIGHT_DUP);
	if (handle == 0) {
		slab_free(resource);
	}
	return handle;
}

static int hw_irq_validate(u64 handle, u64 index, u32 op,
			   const struct task_hw_resource **out, u32 *gsi)
{
	const struct task_hw_resource *resource =
		task_hw_resource_from_handle(task_current(), handle,
					     TASK_RIGHT_SEND);

	if (resource == 0 || out == 0 || gsi == 0 ||
	    resource->type != TASK_HW_RESOURCE_IRQ ||
	    (resource->ops & op) == 0 ||
	    index >= resource->len ||
	    resource->base + index < resource->base ||
	    resource->base + index > 0xffffffffull) {
		task_hw_resource_release(resource);
		return -1;
	}

	*out = resource;
	*gsi = (u32)(resource->base + index);
	return 0;
}

static u64 native_sys_hw_irq_bind(const struct native_syscall_args *args)
{
	const struct task_hw_resource *resource;
	u32 gsi;
	struct ipc_port *port =
		task_port_from_handle(task_current(), args->arg2,
				      TASK_RIGHT_RECV);

	if (port == 0 ||
	    hw_irq_validate(args->arg0, args->arg1, TASK_HW_OP_BIND_IRQ,
			    &resource, &gsi) != 0) {
		ipc_port_release(port);
		return (u64)-1;
	}
	const u64 result =
		arch_irq_bind(gsi, resource->flags, port) == 0 ? 0 : (u64)-1;
	task_hw_resource_release(resource);
	ipc_port_release(port);
	return result;
}

static u64 native_sys_hw_irq_ack(const struct native_syscall_args *args)
{
	const struct task_hw_resource *resource;
	u32 gsi;

	if (hw_irq_validate(args->arg0, args->arg1, TASK_HW_OP_ACK_IRQ,
			    &resource, &gsi) != 0) {
		return (u64)-1;
	}
	(void)resource;
	task_hw_resource_release(resource);
	return arch_irq_ack(gsi) == 0 ? 0 : (u64)-1;
}

static u64 native_sys_hw_irq_mask(const struct native_syscall_args *args)
{
	const struct task_hw_resource *resource;
	u32 gsi;

	if (hw_irq_validate(args->arg0, args->arg1, TASK_HW_OP_MASK_IRQ,
			    &resource, &gsi) != 0) {
		return (u64)-1;
	}
	(void)resource;
	task_hw_resource_release(resource);
	return arch_irq_mask(gsi, args->arg2 != 0) == 0 ? 0 : (u64)-1;
}

static int hw_port_validate(u64 handle, u64 offset, u64 width, u32 op,
			    u16 *port)
{
	const struct task_hw_resource *resource =
		task_hw_resource_from_handle(task_current(), handle,
					     TASK_RIGHT_SEND);

	if (resource == 0 || port == 0 ||
	    resource->type != TASK_HW_RESOURCE_PORT ||
	    (resource->ops & op) == 0 ||
	    width == 0 ||
	    offset + width < offset ||
	    offset + width > resource->len ||
	    resource->base + offset < resource->base ||
	    resource->base + offset > 0xffff) {
		task_hw_resource_release(resource);
		return -1;
	}

	*port = (u16)(resource->base + offset);
	task_hw_resource_release(resource);
	return 0;
}

static u64 native_sys_hw_port_in8(const struct native_syscall_args *args)
{
	u16 port;

	if (hw_port_validate(args->arg0, args->arg1, 1, TASK_HW_OP_READ,
			     &port) != 0) {
		return (u64)-1;
	}
	return arch_inb(port);
}

static u64 native_sys_hw_port_out8(const struct native_syscall_args *args)
{
	u16 port;

	if (hw_port_validate(args->arg0, args->arg1, 1, TASK_HW_OP_WRITE,
			     &port) != 0) {
		return (u64)-1;
	}
	arch_outb(port, (u8)args->arg2);
	return 0;
}

static u64 native_sys_hw_port_in16(const struct native_syscall_args *args)
{
	u16 port;

	if (hw_port_validate(args->arg0, args->arg1, 2, TASK_HW_OP_READ,
			     &port) != 0) {
		return (u64)-1;
	}
	return arch_inw(port);
}

static u64 native_sys_hw_port_out16(const struct native_syscall_args *args)
{
	u16 port;

	if (hw_port_validate(args->arg0, args->arg1, 2, TASK_HW_OP_WRITE,
			     &port) != 0) {
		return (u64)-1;
	}
	arch_outw(port, (u16)args->arg2);
	return 0;
}

static u64 native_sys_hw_port_in32(const struct native_syscall_args *args)
{
	u16 port;

	if (hw_port_validate(args->arg0, args->arg1, 4, TASK_HW_OP_READ,
			     &port) != 0) {
		return (u64)-1;
	}
	return arch_inl(port);
}

static u64 native_sys_hw_port_out32(const struct native_syscall_args *args)
{
	u16 port;

	if (hw_port_validate(args->arg0, args->arg1, 4, TASK_HW_OP_WRITE,
			     &port) != 0) {
		return (u64)-1;
	}
	arch_outl(port, (u32)args->arg2);
	return 0;
}

static int hw_mmio_validate(u64 handle, u64 offset, u64 width, u32 op,
			    volatile void **addr)
{
	const struct task_hw_resource *resource =
		task_hw_resource_from_handle(task_current(), handle,
					     TASK_RIGHT_SEND);

	if (resource == 0 || addr == 0 ||
	    resource->type != TASK_HW_RESOURCE_MMIO ||
	    (resource->ops & op) == 0 ||
	    width == 0 ||
	    offset + width < offset ||
	    offset + width > resource->len ||
	    resource->base + offset < resource->base) {
		task_hw_resource_release(resource);
		return -1;
	}

	const u64 phys = resource->base + offset;
	const u64 first_page = align_down(phys, VM_PAGE_SIZE);
	const u64 last_page = align_down(phys + width - 1, VM_PAGE_SIZE);

	for (u64 page = first_page; page <= last_page; page += VM_PAGE_SIZE) {
		if (vm_map_kernel_page(page, page,
				       op == TASK_HW_OP_WRITE ? 1 : 0) != 0) {
			task_hw_resource_release(resource);
			return -1;
		}
	}

	*addr = (volatile void *)phys;
	task_hw_resource_release(resource);
	return 0;
}

static u64 native_sys_hw_mmio_read8(const struct native_syscall_args *args)
{
	volatile void *addr;
	struct vm_space *space;
	u8 value;

	if (hw_mmio_validate(args->arg0, args->arg1, 1, TASK_HW_OP_READ,
			     &addr) != 0) {
		return (u64)-1;
	}
	space = task_vm_space(task_current());
	vm_rpc_activate_space(vm_kernel_space());
	value = *(volatile const u8 *)addr;
	vm_rpc_activate_space(space);
	return value;
}

static u64 native_sys_hw_mmio_write8(const struct native_syscall_args *args)
{
	volatile void *addr;
	struct vm_space *space;

	if (hw_mmio_validate(args->arg0, args->arg1, 1, TASK_HW_OP_WRITE,
			     &addr) != 0) {
		return (u64)-1;
	}
	space = task_vm_space(task_current());
	vm_rpc_activate_space(vm_kernel_space());
	*(volatile u8 *)addr = (u8)args->arg2;
	vm_rpc_activate_space(space);
	return 0;
}

static u64 native_sys_hw_mmio_read16(const struct native_syscall_args *args)
{
	volatile void *addr;
	struct vm_space *space;
	u16 value;

	if (hw_mmio_validate(args->arg0, args->arg1, 2, TASK_HW_OP_READ,
			     &addr) != 0) {
		return (u64)-1;
	}
	space = task_vm_space(task_current());
	vm_rpc_activate_space(vm_kernel_space());
	value = *(volatile const u16 *)addr;
	vm_rpc_activate_space(space);
	return value;
}

static u64 native_sys_hw_mmio_write16(const struct native_syscall_args *args)
{
	volatile void *addr;
	struct vm_space *space;

	if (hw_mmio_validate(args->arg0, args->arg1, 2, TASK_HW_OP_WRITE,
			     &addr) != 0) {
		return (u64)-1;
	}
	space = task_vm_space(task_current());
	vm_rpc_activate_space(vm_kernel_space());
	*(volatile u16 *)addr = (u16)args->arg2;
	vm_rpc_activate_space(space);
	return 0;
}

static u64 native_sys_hw_mmio_read32(const struct native_syscall_args *args)
{
	volatile void *addr;
	struct vm_space *space;
	u32 value;

	if (hw_mmio_validate(args->arg0, args->arg1, 4, TASK_HW_OP_READ,
			     &addr) != 0) {
		return (u64)-1;
	}
	space = task_vm_space(task_current());
	vm_rpc_activate_space(vm_kernel_space());
	value = *(volatile const u32 *)addr;
	vm_rpc_activate_space(space);
	return value;
}

static u64 native_sys_hw_mmio_write32(const struct native_syscall_args *args)
{
	volatile void *addr;
	struct vm_space *space;

	if (hw_mmio_validate(args->arg0, args->arg1, 4, TASK_HW_OP_WRITE,
			     &addr) != 0) {
		return (u64)-1;
	}
	space = task_vm_space(task_current());
	vm_rpc_activate_space(vm_kernel_space());
	*(volatile u32 *)addr = (u32)args->arg2;
	vm_rpc_activate_space(space);
	return 0;
}

static u64 native_sys_ipc_send(const struct native_syscall_args *args)
{
	struct user_ipc_message user_message;
	struct ipc_port *port =
		task_port_from_handle(task_current(), args->arg0,
				      TASK_RIGHT_SEND);
	u64 result;

	if (port == 0 || args->arg1 == 0 ||
	    read_current_user(args->arg1, &user_message,
			      sizeof(user_message)) != 0) {
		ipc_port_release(port);
		return (u64)-1;
	}

	struct ipc_message message = { .protocol = 0, .type = 0 };

	if (user_message_to_ipc(&user_message, &message) != 0) {
		ipc_message_release(&message);
		ipc_port_release(port);
		return (u64)-1;
	}

	result = (u64)ipc_send(port, &message);
	ipc_message_release(&message);
	ipc_port_release(port);
	return result;
}

static u64 native_sys_ipc_recv(const struct native_syscall_args *args)
{
	struct user_ipc_message user_message;
	struct ipc_port *port =
		task_port_from_handle(task_current(), args->arg0,
				      TASK_RIGHT_RECV);
	struct ipc_message message;
	u64 result;

	if (port == 0 || args->arg1 == 0 || ipc_recv(port, &message) != 0) {
		ipc_port_release(port);
		return (u64)-1;
	}

	ipc_message_to_user(&message, &user_message);
	if (write_current_user(args->arg1, &user_message,
			       sizeof(user_message)) != 0) {
		ipc_message_release(&message);
		ipc_port_release(port);
		return (u64)-1;
	}
	result = 0;
	ipc_message_release(&message);
	ipc_port_release(port);
	return result;
}

static u64 native_sys_ipc_try_recv(const struct native_syscall_args *args)
{
	struct user_ipc_message user_message;
	struct ipc_port *port =
		task_port_from_handle(task_current(), args->arg0,
				      TASK_RIGHT_RECV);
	struct ipc_message message;
	int result;

	if (port == 0 || args->arg1 == 0) {
		ipc_port_release(port);
		return (u64)-1;
	}
	result = ipc_try_recv(port, &message);
	if (result < 0) {
		ipc_port_release(port);
		return (u64)-1;
	}
	if (result > 0) {
		ipc_port_release(port);
		return 1;
	}

	ipc_message_to_user(&message, &user_message);
	if (write_current_user(args->arg1, &user_message,
			       sizeof(user_message)) != 0) {
		ipc_message_release(&message);
		ipc_port_release(port);
		return (u64)-1;
	}
	ipc_message_release(&message);
	ipc_port_release(port);
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
		ipc_port_release(port);
		return (u64)-1;
	}

	if (user_message_to_ipc(&user_request, &message) != 0) {
		ipc_message_release(&message);
		ipc_port_release(port);
		return (u64)-1;
	}

	ipc_port_release(message.reply_port);
	message.reply_port = reply_port;
	ipc_port_retain(message.reply_port);
	if (ipc_send(port, &message) != 0) {
		ipc_message_release(&message);
		ipc_port_release(port);
		return (u64)-1;
	}
	ipc_message_release(&message);
	if (ipc_recv(reply_port, &reply) != 0) {
		ipc_port_release(port);
		return (u64)-1;
	}

	ipc_message_to_user(&reply, &user_reply);
	if (write_current_user(args->arg2, &user_reply,
			       sizeof(user_reply)) != 0) {
		ipc_message_release(&reply);
		ipc_port_release(port);
		return (u64)-1;
	}
	ipc_message_release(&reply);
	ipc_port_release(port);
	return 0;
}

static const struct native_syscall_entry native_syscalls[] = {
	{ SYSCALL_EXIT, "exit", native_sys_exit },
	{ SYSCALL_TIMER_TICKS, "timer_ticks", native_sys_timer_ticks },
	{ SYSCALL_LAUNCH_MODULE, "launch_module", native_sys_launch_module },
	{ SYSCALL_PORT_CREATE, "port_create", native_sys_port_create },
	{ SYSCALL_IPC_SEND, "ipc_send", native_sys_ipc_send },
	{ SYSCALL_IPC_RECV, "ipc_recv", native_sys_ipc_recv },
	{ SYSCALL_IPC_TRY_RECV, "ipc_try_recv", native_sys_ipc_try_recv },
	{ SYSCALL_IPC_CALL, "ipc_call", native_sys_ipc_call },
	{ SYSCALL_HANDLE_CLOSE, "handle_close", native_sys_handle_close },
	{ SYSCALL_HANDLE_FIND, "handle_find", native_sys_handle_find },
	{ SYSCALL_TASK_HANDLE_FIND, "task_handle_find",
	  native_sys_task_handle_find },
	{ SYSCALL_BOOT_MODULE_READ, "boot_module_read",
	  native_sys_boot_module_read },
	{ SYSCALL_CLOCK_MONOTONIC_NS, "clock_monotonic_ns",
	  native_sys_clock_monotonic_ns },
	{ SYSCALL_SLEEP_NS, "sleep_ns", native_sys_sleep_ns },
	{ SYSCALL_CMDLINE_HAS, "cmdline_has", native_sys_cmdline_has },
	{ SYSCALL_TASK_CREATE, "task_create", native_sys_task_create },
	{ SYSCALL_TASK_MAP, "task_map", native_sys_task_map },
	{ SYSCALL_TASK_GRANT, "task_grant", native_sys_task_grant },
	{ SYSCALL_TASK_GRANT_TAGGED, "task_grant_tagged",
	  native_sys_task_grant_tagged },
	{ SYSCALL_TASK_START, "task_start", native_sys_task_start },
	{ SYSCALL_BUFFER_CREATE, "buffer_create", native_sys_buffer_create },
	{ SYSCALL_BUFFER_READ, "buffer_read", native_sys_buffer_read },
	{ SYSCALL_BUFFER_WRITE, "buffer_write", native_sys_buffer_write },
	{ SYSCALL_BUFFER_PHYS, "buffer_phys", native_sys_buffer_phys },
	{ SYSCALL_TASK_WRITE, "task_write", native_sys_task_write },
	{ SYSCALL_TASK_START_AT, "task_start_at", native_sys_task_start_at },
	{ SYSCALL_TASK_ID, "task_id", native_sys_task_id },
	{ SYSCALL_TASK_INFO, "task_info", native_sys_task_info },
	{ SYSCALL_TASK_ALLOC, "task_alloc", native_sys_task_alloc },
	{ SYSCALL_TASK_CLONE_RANGE, "task_clone_range",
	  native_sys_task_clone_range },
	{ SYSCALL_CONSOLE_READ, "console_read", native_sys_console_read },
	{ SYSCALL_TASK_KILL, "task_kill", native_sys_task_kill },
	{ SYSCALL_TASK_CLEAR, "task_clear", native_sys_task_clear },
	{ SYSCALL_EARLY_CONSOLE_WRITE, "early_console_write",
	  native_sys_early_console_write },
	{ SYSCALL_EARLY_CONSOLE_LOG, "early_console_log",
	  native_sys_early_console_log },
	{ SYSCALL_EARLY_CONSOLE_LOGS_TO_RING, "early_console_logs_to_ring",
	  native_sys_early_console_logs_to_ring },
	{ SYSCALL_IPC_STATS, "ipc_stats", native_sys_ipc_stats },
	{ SYSCALL_SCHED_STATS, "sched_stats", native_sys_sched_stats },
	{ SYSCALL_SCHED_THREAD_INFO, "sched_thread_info",
	  native_sys_sched_thread_info },
	{ SYSCALL_SCHED_POLICY_GRANT, "sched_policy_grant",
	  native_sys_sched_policy_grant },
	{ SYSCALL_SCHED_POLICY_GET, "sched_policy_get",
	  native_sys_sched_policy_get },
	{ SYSCALL_SCHED_POLICY_SET, "sched_policy_set",
	  native_sys_sched_policy_set },
	{ SYSCALL_VM_STATS, "vm_stats", native_sys_vm_stats },
	{ SYSCALL_MACHINE_POWER, "machine_power", native_sys_machine_power },
	{ SYSCALL_HW_PORT_IN8, "hw_port_in8", native_sys_hw_port_in8 },
	{ SYSCALL_HW_PORT_OUT8, "hw_port_out8", native_sys_hw_port_out8 },
	{ SYSCALL_HW_PORT_IN16, "hw_port_in16", native_sys_hw_port_in16 },
	{ SYSCALL_HW_PORT_OUT16, "hw_port_out16", native_sys_hw_port_out16 },
	{ SYSCALL_HW_PORT_IN32, "hw_port_in32", native_sys_hw_port_in32 },
	{ SYSCALL_HW_PORT_OUT32, "hw_port_out32", native_sys_hw_port_out32 },
	{ SYSCALL_HW_PCI_BAR_GRANT, "hw_pci_bar_grant",
	  native_sys_hw_pci_bar_grant },
	{ SYSCALL_HW_PCI_IRQ_GRANT, "hw_pci_irq_grant",
	  native_sys_hw_pci_irq_grant },
	{ SYSCALL_HW_IRQ_BIND, "hw_irq_bind", native_sys_hw_irq_bind },
	{ SYSCALL_HW_IRQ_ACK, "hw_irq_ack", native_sys_hw_irq_ack },
	{ SYSCALL_HW_IRQ_MASK, "hw_irq_mask", native_sys_hw_irq_mask },
	{ SYSCALL_HW_MMIO_READ8, "hw_mmio_read8", native_sys_hw_mmio_read8 },
	{ SYSCALL_HW_MMIO_WRITE8, "hw_mmio_write8", native_sys_hw_mmio_write8 },
	{ SYSCALL_HW_MMIO_READ16, "hw_mmio_read16", native_sys_hw_mmio_read16 },
	{ SYSCALL_HW_MMIO_WRITE16, "hw_mmio_write16",
	  native_sys_hw_mmio_write16 },
	{ SYSCALL_HW_MMIO_READ32, "hw_mmio_read32", native_sys_hw_mmio_read32 },
	{ SYSCALL_HW_MMIO_WRITE32, "hw_mmio_write32",
	  native_sys_hw_mmio_write32 },
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
		.arg4 = frame->r8,
		.arg5 = frame->r9,
	};

	return entry->handler(&args);
}
