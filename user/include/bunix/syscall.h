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
	BUNIX_SYSCALL_MACHINE_POWER = -64,
	BUNIX_SYSCALL_TASK_CLEAR = -66,
	BUNIX_SYSCALL_HW_PORT_IN8 = -68,
	BUNIX_SYSCALL_HW_PORT_OUT8 = -70,
	BUNIX_SYSCALL_IPC_TRY_RECV = -72,
	BUNIX_SYSCALL_HW_PORT_IN16 = -74,
	BUNIX_SYSCALL_HW_PORT_OUT16 = -76,
	BUNIX_SYSCALL_HW_PORT_IN32 = -78,
	BUNIX_SYSCALL_HW_PORT_OUT32 = -80,
	BUNIX_SYSCALL_HW_PCI_BAR_GRANT = -82,
	BUNIX_SYSCALL_HW_MMIO_READ8 = -84,
	BUNIX_SYSCALL_HW_MMIO_WRITE8 = -86,
	BUNIX_SYSCALL_HW_MMIO_READ16 = -88,
	BUNIX_SYSCALL_HW_MMIO_WRITE16 = -90,
	BUNIX_SYSCALL_HW_MMIO_READ32 = -92,
	BUNIX_SYSCALL_HW_MMIO_WRITE32 = -94,
	BUNIX_SYSCALL_BUFFER_PHYS = -96,
	BUNIX_SYSCALL_CMDLINE_HAS = -98,
	BUNIX_SYSCALL_SCHED_STATS = -100,
	BUNIX_SYSCALL_HW_PCI_IRQ_GRANT = -102,
	BUNIX_SYSCALL_HW_IRQ_BIND = -104,
	BUNIX_SYSCALL_HW_IRQ_ACK = -106,
	BUNIX_SYSCALL_HW_IRQ_MASK = -108,
	BUNIX_SYSCALL_SCHED_THREAD_INFO = -110,
	BUNIX_SYSCALL_HANDLE_FIND = -112,
	BUNIX_SYSCALL_TASK_GRANT_TAGGED = -114,
	BUNIX_SYSCALL_TASK_HANDLE_FIND = -116,
	BUNIX_SYSCALL_SCHED_POLICY_GRANT = -118,
	BUNIX_SYSCALL_SCHED_POLICY_GET = -120,
	BUNIX_SYSCALL_SCHED_POLICY_SET = -122,
	BUNIX_SYSCALL_BOOT_EVENT = -124,
	BUNIX_SYSCALL_IPC_RECV_ANY = -126,
	BUNIX_IPC_WORDS = 4,
	BUNIX_IPC_STATS_CPUS = 8,
	BUNIX_SCHED_STATS_CPUS = 8,
	BUNIX_BOOT_EVENT_NAME_BYTES = 64,
	BUNIX_IPC_DATA_BYTES = (BUNIX_IPC_WORDS - 2) * 8,
	BUNIX_RIGHT_SEND = 1 << 0,
	BUNIX_RIGHT_RECV = 1 << 1,
	BUNIX_RIGHT_DUP = 1 << 2,
	BUNIX_VM_PROT_READ = 1 << 0,
	BUNIX_VM_PROT_WRITE = 1 << 1,
	BUNIX_VM_PROT_EXEC = 1 << 2,
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
	BUNIX_PROTO_UTMPFS = ('U') | ('T' << 8) | ('M' << 16) | ('P' << 24),
	BUNIX_PROTO_SYSFS = ('S') | ('Y' << 8) | ('S' << 16) | ('F' << 24),
	BUNIX_PROTO_HW = ('H') | ('W' << 8) | ('R' << 16) | ('0' << 24),
	BUNIX_PROTO_PCI = ('P') | ('C' << 8) | ('I' << 16) | ('0' << 24),
	BUNIX_PROTO_USB = ('U') | ('S' << 8) | ('B' << 16) | ('0' << 24),
	BUNIX_PROTO_USBHC = ('U') | ('H' << 8) | ('C' << 16) | ('0' << 24),
	BUNIX_PROTO_INPUT = ('I') | ('N' << 8) | ('P' << 16) | ('0' << 24),
	BUNIX_PROTO_DEVICE = ('D') | ('E' << 8) | ('V' << 16) | ('0' << 24),
	BUNIX_PROTO_NET = ('N') | ('E' << 8) | ('T' << 16) | ('0' << 24),
	BUNIX_PROTO_NETCFG = ('N') | ('E' << 8) | ('T' << 16) | ('C' << 24),
	BUNIX_PROTO_EXT2 = ('E') | ('X' << 8) | ('T' << 16) | ('2' << 24),
	BUNIX_PROTO_SQUASHFS = ('S') | ('Q' << 8) | ('F' << 16) | ('S' << 24),
	BUNIX_PROTO_SCHED = ('S') | ('C' << 8) | ('H' << 16) | ('D' << 24),
	BUNIX_PROCFS_MOUNT_NOTIFY = 1,
	BUNIX_PROCFS_MOUNT_PATH = 2,
	BUNIX_PROCFS_UNMOUNT_NOTIFY = 3,
	BUNIX_NAMES_REGISTER = 1,
	BUNIX_NAMES_RESOLVE = 2,
	BUNIX_NAMES_CREATE_NS = 3,
	BUNIX_NAMES_WAIT = 4,
	BUNIX_NAMES_CLAIM_ADMIN = 5,
	BUNIX_NAMES_CREATE_CLAIM = 6,
	BUNIX_NAMES_CLAIM_READY = 7,
	BUNIX_NAMES_ROOT = 1,
	BUNIX_BLOCK_GET_INFO = 1,
	BUNIX_BLOCK_READ = 2,
	BUNIX_BLOCK_READ_BUFFER = 3,
	BUNIX_BLOCK_WRITE_BUFFER = 4,
	BUNIX_BLOCK_FLUSH = 5,
	BUNIX_VFS_READ_FILE_BUFFER = 6,
	BUNIX_VFS_CLOSE = 7,
	BUNIX_VFS_STAT_META = 8,
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
	BUNIX_VFS_READDIR_BUFFER = 26,
	BUNIX_VFS_MOUNT_BUFFER = 27,
	BUNIX_VFS_UNMOUNT_BUFFER = 28,
	BUNIX_VFS_RESOLVE_MOUNT_BUFFER = 29,
	BUNIX_VFS_SYMLINK_BUFFER = 30,
	BUNIX_VFS_RENAME_BUFFER = 31,
	BUNIX_VFS_MKNOD_BUFFER = 32,
	BUNIX_VFS_LINK_BUFFER = 33,
	BUNIX_VFS_PIN_ROUTE_BUFFER = 34,
	BUNIX_VFS_UNPIN_ROUTE = 35,
	BUNIX_VFS_GRANT_ADMIN_TASK = 36,
	BUNIX_VFS_GRANT_SUBJECT_TASK = 37,
	BUNIX_VFS_UTIMENS_BUFFER = 38,
	BUNIX_VFS_MMAP_PAGE_BUFFER = 39,
	BUNIX_VFS_ROUTE_FLAG_RECURSIVE = 1,
	BUNIX_VFS_TYPE_REGULAR = 1,
	BUNIX_VFS_TYPE_DIRECTORY = 2,
	BUNIX_VFS_TYPE_SYMLINK = 3,
	BUNIX_VFS_TYPE_FIFO = 4,
	BUNIX_VFS_TYPE_CHARACTER = 5,
	BUNIX_VFS_ERR_NOENT = 1,
	BUNIX_VFS_ERR_ACCESS = 2,
	BUNIX_VFS_ERR_NOTDIR = 3,
	BUNIX_VFS_ERR_ISDIR = 4,
	BUNIX_VFS_ERR_EXIST = 5,
	BUNIX_VFS_ERR_NOTEMPTY = 6,
	BUNIX_VFS_ERR_XDEV = 7,
	BUNIX_VFS_ERR_INVAL = 8,
	BUNIX_VFS_ERR_BUSY = 9,
	BUNIX_VFS_ERR_LOOP = 10,
	BUNIX_VFS_MMAP_PAGE_DATA = 1,
	BUNIX_VFS_MMAP_PAGE_ZERO = 2,
	BUNIX_VFS_MMAP_PAGE_BUS = 3,
	BUNIX_VFS_STAT_SIZE = 0,
	BUNIX_VFS_STAT_MODE_TYPE = 8,
	BUNIX_VFS_STAT_OWNER = 16,
	BUNIX_VFS_STAT_DEV = 24,
	BUNIX_VFS_STAT_INO = 32,
	BUNIX_VFS_STAT_NLINK = 40,
	BUNIX_VFS_STAT_RDEV = 48,
	BUNIX_VFS_STAT_BLKSIZE = 56,
	BUNIX_VFS_STAT_BLOCKS = 64,
	BUNIX_VFS_STAT_ATIME = 72,
	BUNIX_VFS_STAT_MTIME = 80,
	BUNIX_VFS_STAT_CTIME = 88,
	BUNIX_VFS_STAT_BYTES = 96,
	BUNIX_VFS_DEV_TMPFS = 2,
	BUNIX_VFS_DEV_PROCFS = 3,
	BUNIX_VFS_DEV_DEVFS = 4,
	BUNIX_VFS_DEV_SYSFS = 5,
	BUNIX_VFS_DEV_UTMPFS = 6,
	BUNIX_VFS_DEV_UNIONFS = 7,
	BUNIX_VFS_DEV_EXT2 = 8,
	BUNIX_VFS_DEV_SQUASHFS = 9,
	BUNIX_LINUX_READ = 0,
	BUNIX_LINUX_WRITE = 1,
	BUNIX_LINUX_OPEN = 2,
	BUNIX_LINUX_CLOSE = 3,
	BUNIX_LINUX_FSTAT = 5,
	BUNIX_LINUX_POLL = 7,
	BUNIX_LINUX_LSEEK = 8,
	BUNIX_LINUX_MMAP = 9,
	BUNIX_LINUX_RT_SIGACTION = 13,
	BUNIX_LINUX_RT_SIGPROCMASK = 14,
	BUNIX_LINUX_RT_SIGRETURN = 15,
	BUNIX_LINUX_IOCTL = 16,
	BUNIX_LINUX_PIPE = 22,
	BUNIX_LINUX_DUP = 32,
	BUNIX_LINUX_DUP2 = 33,
	BUNIX_LINUX_ALARM = 37,
	BUNIX_LINUX_SETITIMER = 38,
	BUNIX_LINUX_SENDFILE = 40,
	BUNIX_LINUX_TRUNCATE = 76,
	BUNIX_LINUX_FTRUNCATE = 77,
	BUNIX_LINUX_RMDIR = 84,
	BUNIX_LINUX_LINK = 86,
	BUNIX_LINUX_UNLINK = 87,
	BUNIX_LINUX_MKNOD = 133,
	BUNIX_LINUX_SOCKET = 41,
	BUNIX_LINUX_CONNECT = 42,
	BUNIX_LINUX_ACCEPT = 43,
	BUNIX_LINUX_SENDTO = 44,
	BUNIX_LINUX_RECVFROM = 45,
	BUNIX_LINUX_RECVMSG = 47,
	BUNIX_LINUX_GETSOCKNAME = 51,
	BUNIX_LINUX_GETPEERNAME = 52,
	BUNIX_LINUX_SETSOCKOPT = 54,
	BUNIX_LINUX_GETSOCKOPT = 55,
	BUNIX_LINUX_SHUTDOWN = 48,
	BUNIX_LINUX_BIND = 49,
	BUNIX_LINUX_LISTEN = 50,
	BUNIX_LINUX_GETCWD = 79,
	BUNIX_LINUX_CHDIR = 80,
	BUNIX_LINUX_MKDIR = 83,
	BUNIX_LINUX_RENAME = 82,
	BUNIX_LINUX_CHMOD = 90,
	BUNIX_LINUX_FCHMOD = 91,
	BUNIX_LINUX_CHOWN = 92,
	BUNIX_LINUX_FCHOWN = 93,
	BUNIX_LINUX_LCHOWN = 94,
	BUNIX_LINUX_ACCESS = 21,
	BUNIX_LINUX_READLINK = 89,
	BUNIX_LINUX_UMASK = 95,
	BUNIX_LINUX_GETTIMEOFDAY = 96,
	BUNIX_LINUX_SYSINFO = 99,
	BUNIX_LINUX_GETPID = 39,
	BUNIX_LINUX_SETPGID = 109,
	BUNIX_LINUX_GETPPID = 110,
	BUNIX_LINUX_GETPGRP = 111,
	BUNIX_LINUX_SETSID = 112,
	BUNIX_LINUX_GETGROUPS = 115,
	BUNIX_LINUX_KILL = 62,
	BUNIX_LINUX_GETPGID = 121,
	BUNIX_LINUX_GETSID = 124,
	BUNIX_LINUX_STATFS = 137,
	BUNIX_LINUX_FSTATFS = 138,
	BUNIX_LINUX_FCNTL = 72,
	BUNIX_LINUX_FLOCK = 73,
	BUNIX_LINUX_GETDENTS64 = 217,
	BUNIX_LINUX_GETUID = 102,
	BUNIX_LINUX_GETGID = 104,
	BUNIX_LINUX_SETUID = 105,
	BUNIX_LINUX_SETGID = 106,
	BUNIX_LINUX_GETEUID = 107,
	BUNIX_LINUX_GETEGID = 108,
	BUNIX_LINUX_WAIT4 = 61,
	BUNIX_LINUX_UNAME = 63,
	BUNIX_LINUX_SETGROUPS = 116,
	BUNIX_LINUX_SETRESUID = 117,
	BUNIX_LINUX_SETRESGID = 119,
	BUNIX_LINUX_RT_SIGTIMEDWAIT = 128,
	BUNIX_LINUX_MOUNT = 165,
	BUNIX_LINUX_UMOUNT2 = 166,
	BUNIX_LINUX_REBOOT = 169,
	BUNIX_LINUX_GETTID = 186,
	BUNIX_LINUX_TIME = 201,
	BUNIX_LINUX_CLOCK_GETTIME = 228,
	BUNIX_LINUX_OPENAT = 257,
	BUNIX_LINUX_PPOLL = 271,
	BUNIX_LINUX_ACCEPT4 = 288,
	BUNIX_LINUX_MKDIRAT = 258,
	BUNIX_LINUX_MKNODAT = 259,
	BUNIX_LINUX_FCHOWNAT = 260,
	BUNIX_LINUX_LINKAT = 265,
	BUNIX_LINUX_RENAMEAT = 264,
	BUNIX_LINUX_UNLINKAT = 263,
	BUNIX_LINUX_SYMLINKAT = 266,
	BUNIX_LINUX_NEWFSTATAT = 262,
	BUNIX_LINUX_READLINKAT = 267,
	BUNIX_LINUX_FCHMODAT = 268,
	BUNIX_LINUX_FACCESSAT = 269,
	BUNIX_LINUX_UTIMENSAT = 280,
	BUNIX_LINUX_DUP3 = 292,
	BUNIX_LINUX_PIPE2 = 293,
	BUNIX_LINUX_CLOSE_RANGE = 436,
	BUNIX_LINUX_GETRANDOM = 318,
	BUNIX_LINUX_RENAMEAT2 = 316,
	BUNIX_LINUX_STATX = 332,
	BUNIX_LINUX_FACCESSAT2 = 439,
	BUNIX_LINUX_REGISTER_PROCESS = 1000,
	BUNIX_LINUX_FORK_PROCESS = 1001,
	BUNIX_LINUX_EXEC_PROCESS = 1002,
	BUNIX_LINUX_TTY_INPUT = 1003,
	BUNIX_LINUX_EXEC_INIT = 1004,
	BUNIX_LINUX_EXEC_PREPARE = 1005,
	BUNIX_LINUX_ATTACH_SESSION = 1006,
	BUNIX_LINUX_SIGNAL_PENDING = 1007,
	BUNIX_LINUX_SIGNAL_DEQUEUE = 1008,
	BUNIX_LINUX_TASK_FAULT = 1009,
	BUNIX_LINUX_TTY_INPUT_BUFFER = 1010,
	BUNIX_LINUX_GRANT_TTY_INPUT_TASK = 1011,
	BUNIX_LINUX_EXIT_GROUP = 231,
	BUNIX_TIME_NOW_MONOTONIC = 1,
	BUNIX_TIME_SLEEP_NS = 2,
	BUNIX_BOOT_EVENT_RECORD = 1,
	BUNIX_BOOT_EVENT_READ = 2,
	BUNIX_PROC_WAIT = 2,
	BUNIX_PROC_EXIT = 3,
	BUNIX_PROC_SELF = 4,
	BUNIX_PROC_INFO = 5,
	BUNIX_PROC_AT = 6,
	BUNIX_PROC_REGISTER_LINUX = 9,
	BUNIX_PROC_DETAILS = 10,
	BUNIX_PROC_REGISTER_EXEC = 11,
	BUNIX_PROC_SPAWN_BUFFER = 12,
	BUNIX_PROC_CMDLINE_BUFFER = 13,
	BUNIX_PROC_SET_CMDLINE_BUFFER = 14,
	BUNIX_PROC_INFO_BY_TASK = 15,
	BUNIX_PROC_EXEC_REPLACE_TASK = 16,
	BUNIX_PROC_EXEC_REPLACE_BUFFER = 17,
	BUNIX_PROC_INFO_BY_LINUX_PID = 18,
	BUNIX_SCHED_INFO = 1,
	BUNIX_SCHED_GRANT = 2,
	BUNIX_SCHED_GETP = 3,
	BUNIX_SCHED_SETP = 4,
	BUNIX_SCHED_STAT = 5,
	BUNIX_SCHED_TARGET_TASK = 1,
	BUNIX_SCHED_TARGET_THREAD = 2,
	BUNIX_SCHED_TARGET_SYSTEM = 5,
	BUNIX_SCHED_RIGHT_OBSERVE = 1 << 0,
	BUNIX_SCHED_RIGHT_CLASS = 1 << 1,
	BUNIX_SCHED_RIGHT_PRIORITY = 1 << 2,
	BUNIX_SCHED_RIGHT_WEIGHT = 1 << 3,
	BUNIX_SCHED_RIGHT_AFFINITY = 1 << 4,
	BUNIX_SCHED_RIGHT_DELEGATE = 1 << 5,
	BUNIX_SCHED_CLASS_KERNEL = 1,
	BUNIX_SCHED_CLASS_SERVER = 2,
	BUNIX_SCHED_CLASS_USER = 3,
	BUNIX_SCHED_CLASS_BATCH = 4,
	BUNIX_SCHED_CLASS_IDLE = 5,
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
	BUNIX_SERVICE_UTMPFS = BUNIX_PROTO_UTMPFS,
	BUNIX_SERVICE_SYSFS = BUNIX_PROTO_SYSFS,
	BUNIX_SERVICE_HW = BUNIX_PROTO_HW,
	BUNIX_SERVICE_PCI = BUNIX_PROTO_PCI,
	BUNIX_SERVICE_USB = BUNIX_PROTO_USB,
	BUNIX_SERVICE_INPUT = BUNIX_PROTO_INPUT,
	BUNIX_SERVICE_DEVICE = BUNIX_PROTO_DEVICE,
	BUNIX_SERVICE_NET = BUNIX_PROTO_NET,
	BUNIX_SERVICE_NETCFG = BUNIX_PROTO_NETCFG,
	BUNIX_SERVICE_EXT2 = BUNIX_PROTO_EXT2,
	BUNIX_SERVICE_SQUASHFS = BUNIX_PROTO_SQUASHFS,
	BUNIX_SERVICE_SCHED = BUNIX_PROTO_SCHED,
	BUNIX_NET_INTERFACE_COUNT = 1,
	BUNIX_NET_INTERFACE_INFO = 2,
	BUNIX_NET_LOOPBACK_SEND = 3,
	BUNIX_NET_LOOPBACK_RECV = 4,
	BUNIX_NET_INTERFACE_STATS = 5,
	BUNIX_NET_UDP_OPEN = 6,
	BUNIX_NET_UDP_BIND = 7,
	BUNIX_NET_UDP_CONNECT = 8,
	BUNIX_NET_UDP_SEND = 9,
	BUNIX_NET_UDP_RECV = 10,
	BUNIX_NET_UDP_POLL = 11,
	BUNIX_NET_UDP_CLOSE = 12,
	BUNIX_NET_TCP_OPEN = 13,
	BUNIX_NET_TCP_BIND = 14,
	BUNIX_NET_TCP_LISTEN = 15,
	BUNIX_NET_TCP_CONNECT = 16,
	BUNIX_NET_TCP_ACCEPT = 17,
	BUNIX_NET_TCP_WRITE = 18,
	BUNIX_NET_TCP_READ = 19,
	BUNIX_NET_TCP_SHUTDOWN = 20,
	BUNIX_NET_TCP_CLOSE = 21,
	BUNIX_NET_TCP_POLL = 22,
	BUNIX_NET_UDP_SENDTO = 23,
	BUNIX_NET_UDP_LOCAL = 24,
	BUNIX_NET_UDP_PEER = 25,
	BUNIX_NET_TCP_LOCAL = 26,
	BUNIX_NET_TCP_PEER = 27,
	BUNIX_NET_OBSERVE_SOCKET_COUNT = 28,
	BUNIX_NET_OBSERVE_SOCKET_AT = 29,
	BUNIX_NET_PACKET_INTERFACE_ATTACH = 30,
	BUNIX_NET_PACKET_INTERFACE_LINK = 31,
	BUNIX_NET_PACKET_RX_SUBMIT = 32,
	BUNIX_NET_PACKET_TX_DEQUEUE = 33,
	BUNIX_NET_PACKET_TX_COMPLETE = 34,
	BUNIX_NET_INTERFACE_AT = 35,
	BUNIX_NET_ROUTE_ADD = 36,
	BUNIX_NET_ROUTE_COUNT = 37,
	BUNIX_NET_ROUTE_AT = 38,
	BUNIX_NET_PACKET_TX_ENQUEUE = 39,
	BUNIX_NET_INTERFACE_SET_FLAGS = 40,
	BUNIX_NET_ADDR_ADD = 41,
	BUNIX_NET_ADDR_DELETE = 42,
	BUNIX_NET_ADDR_COUNT = 43,
	BUNIX_NET_ADDR_AT = 44,
	BUNIX_NET_ROUTE_DELETE = 45,
	BUNIX_NET_CONFIG_STATUS = 46,
	BUNIX_NET_DHCP4_LEASE_INSTALL = 47,
	BUNIX_NET_NEIGHBOR_COUNT = 48,
	BUNIX_NET_NEIGHBOR_AT = 49,
	BUNIX_NET_NEIGHBOR_ADD = 50,
	BUNIX_NET_NEIGHBOR_DELETE = 51,
	BUNIX_NET_ICMP_OPEN = 52,
	BUNIX_NET_ICMP_SENDTO = 53,
	BUNIX_NET_ICMP_RECV = 54,
	BUNIX_NET_ICMP_POLL = 55,
	BUNIX_NET_ICMP_CLOSE = 56,
	BUNIX_NET_INTERFACE_DETAILS = 57,
	BUNIX_NET_PACKET_RX_DEQUEUE = 58,
	BUNIX_NET_PACKET_RX_POLL = 59,
	BUNIX_NET_IFACE_FLAG_UP = 1 << 0,
	BUNIX_NET_IFACE_FLAG_LOOPBACK = 1 << 1,
	BUNIX_NET_IFACE_FLAG_BROADCAST = 1 << 2,
	BUNIX_NET_IFACE_FLAG_RUNNING = 1 << 3,
	BUNIX_NET_ROUTE_FLAG_UP = 1 << 0,
	BUNIX_NET_ROUTE_FLAG_GATEWAY = 1 << 1,
	BUNIX_NET_CONFIG_LOOPBACK = 1 << 0,
	BUNIX_NET_CONFIG_DEFAULT_IPV4 = 1 << 1,
	BUNIX_NET_CONFIG_DEFAULT_IPV6 = 1 << 2,
	BUNIX_NET_ADDR_FAMILY_IPV4 = 4,
	BUNIX_NET_ADDR_FAMILY_IPV6 = 6,
	BUNIX_UNIONFS_SET_UPPER = 1,
	BUNIX_UNIONFS_MOUNT_PATH = 2,
	BUNIX_UNIONFS_SET_LOWER = 3,
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
	BUNIX_USER_NAME_FOR_UID = 23,
	BUNIX_USER_GETGROUPS_BUFFER = 24,
	BUNIX_USER_SETGROUPS_BUFFER = 25,
	BUNIX_USER_LOGIN_GROUPS_BUFFER = 26,
	BUNIX_USER_AUTH_LOGIN_BUFFER = 27,
	BUNIX_USER_LOGIN_GROUPS_NAME_BUFFER = 28,
	BUNIX_TMPFS_MOUNT_ROOT = 1,
	BUNIX_TMPFS_MKDIR_P_BUFFER = 2,
	BUNIX_UTMPFS_GETENT = 1,
	BUNIX_UTMPFS_MOUNT_PATH = 2,
	BUNIX_DEVFS_MOUNT_PATH = 1,
	BUNIX_SYSFS_MOUNT_PATH = 1,
	BUNIX_CONSOLE_WRITE = 1,
	BUNIX_CONSOLE_LOG = 2,
	BUNIX_CONSOLE_LOGS_TO_RING = 3,
	BUNIX_HW_RESOURCE_PORT = 1,
	BUNIX_HW_RESOURCE_MMIO = 2,
	BUNIX_HW_RESOURCE_IRQ = 3,
	BUNIX_HW_OP_READ = 1 << 0,
	BUNIX_HW_OP_WRITE = 1 << 1,
	BUNIX_HW_OP_BIND_IRQ = 1 << 2,
	BUNIX_HW_OP_ACK_IRQ = 1 << 3,
	BUNIX_HW_OP_MASK_IRQ = 1 << 4,
	BUNIX_HW_PORT_IN8 = 1,
	BUNIX_HW_PORT_OUT8 = 2,
	BUNIX_HW_PORT_IN16 = 3,
	BUNIX_HW_PORT_OUT16 = 4,
	BUNIX_HW_MMIO_READ8 = 5,
	BUNIX_HW_MMIO_WRITE8 = 6,
	BUNIX_HW_MMIO_READ16 = 7,
	BUNIX_HW_MMIO_WRITE16 = 8,
	BUNIX_HW_MMIO_READ32 = 9,
	BUNIX_HW_MMIO_WRITE32 = 10,
	BUNIX_HW_IRQ_BIND = 11,
	BUNIX_HW_IRQ_ACK = 12,
	BUNIX_HW_IRQ_MASK = 13,
	BUNIX_HW_EVENT_IRQ = 14,
	BUNIX_PCI_ENUMERATE = 1,
	BUNIX_PCI_GET_INFO = 2,
	BUNIX_PCI_ENABLE = 3,
	BUNIX_PCI_READ_CONFIG = 4,
	BUNIX_PCI_GET_BAR = 5,
	BUNIX_PCI_GRANT_BAR = 6,
	BUNIX_PCI_GRANT_IRQ = 7,
	BUNIX_PCI_OK = 0,
	BUNIX_PCI_ERR_NOENT = 1,
	BUNIX_PCI_ERR_INVAL = 2,
	BUNIX_PCI_ERR_UNSUPPORTED = 3,
	BUNIX_USB_CONTROLLER_REGISTER = 1,
	BUNIX_USB_DEVICE_COUNT = 2,
	BUNIX_USB_DEVICE_INFO = 3,
	BUNIX_USB_DESCRIPTOR_READ = 4,
	BUNIX_USB_CLAIM_INTERFACE = 5,
	BUNIX_USB_RELEASE_INTERFACE = 6,
	BUNIX_USB_OPEN_ENDPOINT = 7,
	BUNIX_USB_SUBMIT_TRANSFER = 8,
	BUNIX_USB_CANCEL_TRANSFER = 9,
	BUNIX_USB_WAIT_COMPLETION = 10,
	BUNIX_USB_CONTROLLER_TRANSFER_POLL = 11,
	BUNIX_USB_CONTROLLER_TRANSFER_COMPLETE = 12,
	BUNIX_USB_CONTROL_NO_DATA = 13,
	BUNIX_USB_CONTROLLER_CONTROL_POLL = 14,
	BUNIX_USB_CONTROLLER_CONTROL_COMPLETE = 15,
	BUNIX_USB_OK = 0,
	BUNIX_USB_ERR_NOENT = 1,
	BUNIX_USB_ERR_INVAL = 2,
	BUNIX_USB_ERR_BUSY = 3,
	BUNIX_USB_ERR_UNSUPPORTED = 4,
	BUNIX_USB_DESCRIPTOR_DEVICE = 1,
	BUNIX_USB_DESCRIPTOR_CONFIGURATION = 2,
	BUNIX_USB_DESCRIPTOR_STRING = 3,
	BUNIX_USB_DESCRIPTOR_INTERFACE = 4,
	BUNIX_USB_DESCRIPTOR_ENDPOINT = 5,
	BUNIX_USB_TRANSFER_CONTROL = 1,
	BUNIX_USB_TRANSFER_ISOCHRONOUS = 2,
	BUNIX_USB_TRANSFER_BULK = 3,
	BUNIX_USB_TRANSFER_INTERRUPT = 4,
	BUNIX_USB_TRANSFER_MAX = 128 * 1024,
	BUNIX_USB_DIR_OUT = 0,
	BUNIX_USB_DIR_IN = 1,
	BUNIX_INPUT_KEY_EVENT = 1,
	BUNIX_INPUT_STATS = 2,
	BUNIX_INPUT_OK = 0,
	BUNIX_INPUT_ERR_INVAL = 1,
	BUNIX_USBHC_REGISTER_CONTROLLER = 1,
	BUNIX_USBHC_ROOT_PORT_COUNT = 2,
	BUNIX_USBHC_RESET_PORT = 3,
	BUNIX_USBHC_ALLOCATE_SLOT = 4,
	BUNIX_USBHC_ADDRESS_DEVICE = 5,
	BUNIX_USBHC_SUBMIT_TRANSFER = 6,
	BUNIX_USBHC_CANCEL_TRANSFER = 7,
	BUNIX_USBHC_COMPLETION_EVENT = 8,
	BUNIX_DEV_ENUMERATE = 1,
	BUNIX_DEV_GET_INFO = 2,
	BUNIX_DEV_GET_RESOURCE = 3,
	BUNIX_DEV_BIND_DRIVER = 4,
	BUNIX_DEV_RESET = 5,
	BUNIX_DEV_READ_FEATURES = 6,
	BUNIX_DEV_NEGOTIATE_FEATURES = 7,
	BUNIX_DEV_SETUP_QUEUE = 8,
	BUNIX_DEV_NOTIFY_QUEUE = 9,
	BUNIX_DEV_ACK_INTERRUPT = 10,
	BUNIX_DEV_READ_CONFIG64 = 11,
	BUNIX_DEV_OK = 0,
	BUNIX_DEV_ERR_NOENT = 1,
	BUNIX_DEV_ERR_INVAL = 2,
	BUNIX_DEV_ERR_BUSY = 3,
	BUNIX_DEV_ERR_UNSUPPORTED = 4,
	BUNIX_DEV_RESOURCE_PIO = 1,
	BUNIX_DEV_RESOURCE_MMIO = 2,
	BUNIX_DEV_RESOURCE_IRQ = 3,
	BUNIX_DEV_RESOURCE_DMA_WINDOW = 4,
	BUNIX_DEV_RESOURCE_QUEUE_MEMORY = 5,
	BUNIX_DEV_RESOURCE_FEATURE_BITS = 6,
	BUNIX_DEV_RESOURCE_RESET = 7,
	BUNIX_DEV_RESOURCE_FLAG_PCI_IO = 1 << 0,
	BUNIX_DEV_RESOURCE_FLAG_PCI_MEM64 = 1 << 1,
	BUNIX_DEV_RESOURCE_FLAG_PCI_PREFETCH = 1 << 2,
	BUNIX_DEV_RESOURCE_FLAG_VIRTIO_COMMON_CFG = 1 << 8,
	BUNIX_DEV_RESOURCE_FLAG_VIRTIO_NOTIFY_CFG = 1 << 9,
	BUNIX_DEV_RESOURCE_FLAG_VIRTIO_ISR_CFG = 1 << 10,
	BUNIX_DEV_RESOURCE_FLAG_VIRTIO_DEVICE_CFG = 1 << 11,
	BUNIX_DEV_TRANSPORT_PLATFORM = 1,
	BUNIX_DEV_TRANSPORT_PCI = 2,
	BUNIX_DEV_TRANSPORT_MMIO = 3,
	BUNIX_DEV_OP_READ = 1 << 0,
	BUNIX_DEV_OP_WRITE = 1 << 1,
	BUNIX_DEV_OP_BIND_IRQ = 1 << 2,
	BUNIX_DEV_OP_ACK_IRQ = 1 << 3,
	BUNIX_DEV_OP_MASK_IRQ = 1 << 4,
	BUNIX_DEV_OP_DMA_READ = 1 << 5,
	BUNIX_DEV_OP_DMA_WRITE = 1 << 6,
	BUNIX_DEV_OP_QUEUE_SETUP = 1 << 7,
	BUNIX_DEV_OP_QUEUE_NOTIFY = 1 << 8,
	BUNIX_DEV_OP_RESET = 1 << 9,
	BUNIX_DEV_OP_FEATURE_NEGOTIATE = 1 << 10,
	BUNIX_SQUASHFS_MOUNT_PATH = 1,
	BUNIX_HANDLE_SELF = 1,
	BUNIX_HANDLE_CONSOLE = 2,
	BUNIX_HANDLE_VM = 3,
	BUNIX_HANDLE_NAMES = 4,
	BUNIX_HANDLE_POWER_AUTH = 5,
	BUNIX_HANDLE_PCI_AUTH = 5,
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

enum bunix_task_fault_access {
	BUNIX_TASK_FAULT_ACCESS_READ = 1,
	BUNIX_TASK_FAULT_ACCESS_WRITE = 2,
	BUNIX_TASK_FAULT_ACCESS_EXEC = 3,
};

enum bunix_task_fault_class {
	BUNIX_TASK_FAULT_CLASS_UNKNOWN = 0,
	BUNIX_TASK_FAULT_CLASS_MAPPING = 1,
	BUNIX_TASK_FAULT_CLASS_PROTECTION = 2,
	BUNIX_TASK_FAULT_CLASS_BUS = 3,
};

struct bunix_task_fault_event {
	u64 task;
	u64 thread;
	u64 trap;
	u64 error;
	u64 fault_addr;
	u64 ip;
	u64 sp;
	u64 flags;
	u64 arch0;
	u64 arch1;
};

struct bunix_launch_cap {
	u64 handle;
	unsigned int rights;
	unsigned int tag;
};

#define BUNIX_FOURCC(a, b, c, d) \
	((unsigned int)(a) | ((unsigned int)(b) << 8) | \
	 ((unsigned int)(c) << 16) | ((unsigned int)(d) << 24))

enum {
	BUNIX_CAP_CONS = BUNIX_FOURCC('C', 'O', 'N', 'S'),
	BUNIX_CAP_VM = BUNIX_FOURCC('V', 'M', ' ', ' '),
	BUNIX_CAP_NAME = BUNIX_FOURCC('N', 'A', 'M', 'E'),
	BUNIX_CAP_CLAM = BUNIX_FOURCC('C', 'L', 'A', 'M'),
	BUNIX_CAP_TIME = BUNIX_FOURCC('T', 'I', 'M', 'E'),
	BUNIX_CAP_PROC = BUNIX_FOURCC('P', 'R', 'O', 'C'),
	BUNIX_CAP_VFS = BUNIX_FOURCC('V', 'F', 'S', ' '),
	BUNIX_CAP_POWR = BUNIX_FOURCC('P', 'O', 'W', 'R'),
	BUNIX_CAP_PCI = BUNIX_FOURCC('P', 'C', 'I', ' '),
	BUNIX_CAP_PCFG = BUNIX_FOURCC('P', 'C', 'F', 'G'),
	BUNIX_CAP_PAUT = BUNIX_FOURCC('P', 'A', 'U', 'T'),
	BUNIX_CAP_USB = BUNIX_FOURCC('U', 'S', 'B', ' '),
	BUNIX_CAP_INP0 = BUNIX_FOURCC('I', 'N', 'P', '0'),
	BUNIX_CAP_COM1 = BUNIX_FOURCC('C', 'O', 'M', '1'),
	BUNIX_CAP_USRM = BUNIX_FOURCC('U', 'S', 'R', 'M'),
	BUNIX_CAP_PRMG = BUNIX_FOURCC('P', 'R', 'M', 'G'),
	BUNIX_CAP_LNXM = BUNIX_FOURCC('L', 'N', 'X', 'M'),
	BUNIX_CAP_VADM = BUNIX_FOURCC('V', 'A', 'D', 'M'),
	BUNIX_CAP_VSUB = BUNIX_FOURCC('V', 'S', 'U', 'B'),
	BUNIX_CAP_SCHD = BUNIX_FOURCC('S', 'C', 'H', 'D'),
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

struct bunix_sched_stats {
	u64 enqueues;
	u64 switches;
	u64 wakeups;
	u64 preemptions;
	u64 migrations;
	u64 idle_pulls;
	u64 idle_migrations;
	u64 runtime_ticks;
	u64 wait_ticks;
	u64 max_wait_ticks;
	u64 wake_to_run_ticks;
	u64 max_wake_to_run_ticks;
	u64 cpu_enqueues[BUNIX_SCHED_STATS_CPUS];
	u64 cpu_switches[BUNIX_SCHED_STATS_CPUS];
	u64 cpu_wakeups[BUNIX_SCHED_STATS_CPUS];
	u64 cpu_preemptions[BUNIX_SCHED_STATS_CPUS];
	u64 cpu_migrations[BUNIX_SCHED_STATS_CPUS];
	u64 cpu_idle_pulls[BUNIX_SCHED_STATS_CPUS];
	u64 cpu_idle_migrations[BUNIX_SCHED_STATS_CPUS];
	u64 cpu_runtime_ticks[BUNIX_SCHED_STATS_CPUS];
	u64 cpu_wait_ticks[BUNIX_SCHED_STATS_CPUS];
	u64 cpu_max_wait_ticks[BUNIX_SCHED_STATS_CPUS];
	u64 cpu_wake_to_run_ticks[BUNIX_SCHED_STATS_CPUS];
	u64 cpu_max_wake_to_run_ticks[BUNIX_SCHED_STATS_CPUS];
	u64 cpu_runq_load[BUNIX_SCHED_STATS_CPUS];
	u64 cpu_min_vruntime[BUNIX_SCHED_STATS_CPUS];
};

struct bunix_sched_thread_info {
	u64 task_id;
	u64 thread_id;
	u64 state;
	u64 cpu_id;
	u64 sched_class;
	u64 sched_priority;
	u64 weight;
	u64 runtime_ticks;
	u64 wakeups;
	u64 migrations;
	u64 preemptions;
	u64 vruntime;
	u64 virtual_deadline;
	u64 runnable_wait_ticks;
	u64 wake_to_run_pending_ticks;
	u64 affinity_mask;
	u64 task_name_words[2];
	u64 thread_name_words[2];
};

struct bunix_sched_policy {
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

struct bunix_sched_policy_state {
	u64 target_kind;
	u64 target_id;
	u64 sched_class;
	u64 priority;
	u64 weight;
	u64 cpu_mask;
	u64 online_cpu_mask;
};

struct bunix_vm_stats {
	u64 total_frames;
	u64 free_frames;
};

struct bunix_boot_event {
	u64 index;
	u64 time_ns;
	char name[BUNIX_BOOT_EVENT_NAME_BYTES];
};

struct bunix_net_socket_info {
	u64 id;
	u64 protocol;
	u64 family;
	u64 state;
	u64 local_hi;
	u64 local_lo;
	u64 local_port;
	u64 peer_hi;
	u64 peer_lo;
	u64 peer_port;
	u64 rx_len;
	u64 tx_len;
};

struct bunix_net_packet_interface_info {
	u64 id;
	u64 flags;
	u64 mtu;
	u64 mac_hi;
	u64 mac_lo;
	u64 rx_packets;
	u64 tx_packets;
	u64 rx_drops;
	u64 tx_drops;
	u64 rx_pending;
	u64 tx_pending;
};

struct bunix_net_packet_info {
	u64 iface;
	u64 len;
	u64 flags;
	u64 reserved;
};

struct bunix_net_route_info {
	u64 family;
	u64 prefix_hi;
	u64 prefix_lo;
	u64 prefix_len;
	u64 iface;
	u64 gateway_hi;
	u64 gateway_lo;
	u64 flags;
	u64 metric;
};

struct bunix_net_addr_info {
	u64 family;
	u64 addr_hi;
	u64 addr_lo;
	u64 prefix_len;
	u64 iface;
	u64 flags;
	u64 preferred_lifetime;
	u64 valid_lifetime;
};

struct bunix_net_neighbor_info {
	u64 family;
	u64 addr_hi;
	u64 addr_lo;
	u64 iface;
	u64 mac_hi;
	u64 mac_lo;
	u64 flags;
	u64 state;
};

struct bunix_net_dhcp4_lease {
	u64 iface;
	u64 address;
	u64 prefix_len;
	u64 gateway;
	u64 dns0;
	u64 dns1;
	u64 lease_lifetime;
	u64 renewal_time;
	u64 server;
};

struct bunix_net_config_status {
	u64 flags;
	u64 interface_count;
	u64 address_count;
	u64 route_count;
	u64 default_ipv4_iface;
	u64 default_ipv6_iface;
	u64 last_error;
	u64 reserved;
};

static inline void bunix_store_u64_le(unsigned char *buffer, u64 offset,
				      u64 value)
{
	for (u64 i = 0; i < 8; i++) {
		buffer[offset + i] = (unsigned char)((value >> (i * 8)) & 0xff);
	}
}

static inline u64 bunix_load_u64_le(const unsigned char *buffer, u64 offset)
{
	u64 value = 0;

	for (u64 i = 0; i < 8; i++) {
		value |= ((u64)buffer[offset + i]) << (i * 8);
	}
	return value;
}

static inline void bunix_vfs_stat_pack_times(unsigned char *buffer, u64 size,
					     u64 mode_type, u64 owner, u64 dev,
					     u64 ino, u64 nlink, u64 rdev,
					     u64 blksize, u64 blocks,
					     u64 atime, u64 mtime, u64 ctime)
{
	for (u64 i = 0; i < BUNIX_VFS_STAT_BYTES; i++) {
		buffer[i] = 0;
	}
	bunix_store_u64_le(buffer, BUNIX_VFS_STAT_SIZE, size);
	bunix_store_u64_le(buffer, BUNIX_VFS_STAT_MODE_TYPE, mode_type);
	bunix_store_u64_le(buffer, BUNIX_VFS_STAT_OWNER, owner);
	bunix_store_u64_le(buffer, BUNIX_VFS_STAT_DEV, dev);
	bunix_store_u64_le(buffer, BUNIX_VFS_STAT_INO, ino);
	bunix_store_u64_le(buffer, BUNIX_VFS_STAT_NLINK,
			   nlink == 0 ? 1 : nlink);
	bunix_store_u64_le(buffer, BUNIX_VFS_STAT_RDEV, rdev);
	bunix_store_u64_le(buffer, BUNIX_VFS_STAT_BLKSIZE,
			   blksize == 0 ? 4096 : blksize);
	bunix_store_u64_le(buffer, BUNIX_VFS_STAT_BLOCKS, blocks);
	bunix_store_u64_le(buffer, BUNIX_VFS_STAT_ATIME, atime);
	bunix_store_u64_le(buffer, BUNIX_VFS_STAT_MTIME, mtime);
	bunix_store_u64_le(buffer, BUNIX_VFS_STAT_CTIME, ctime);
}

static inline void bunix_vfs_stat_pack(unsigned char *buffer, u64 size,
				       u64 mode_type, u64 owner, u64 dev,
				       u64 ino, u64 nlink, u64 rdev,
				       u64 blksize, u64 blocks)
{
	bunix_vfs_stat_pack_times(buffer, size, mode_type, owner, dev, ino,
				  nlink, rdev, blksize, blocks, 0, 0, 0);
}

static inline long bunix_syscall0(long number)
{
#if defined(__riscv) && __riscv_xlen == 64
	register long a0 __asm__("a0");
	register long a7 __asm__("a7") = number;

	__asm__ volatile ("ecall"
			  : "=r"(a0)
			  : "r"(a7)
			  : "memory");
	return a0;
#else
	long rax = number;

	__asm__ volatile ("syscall"
			  : "+a"(rax)
			  :
			  : "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10",
			    "r11", "memory");
	return rax;
#endif
}

static inline long bunix_syscall1(long number, u64 arg0)
{
#if defined(__riscv) && __riscv_xlen == 64
	register long a0 __asm__("a0") = (long)arg0;
	register long a7 __asm__("a7") = number;

	__asm__ volatile ("ecall"
			  : "+r"(a0)
			  : "r"(a7)
			  : "memory");
	return a0;
#else
	long rax = number;
	register u64 rdi __asm__("rdi") = arg0;

	__asm__ volatile ("syscall"
			  : "+a"(rax), "+D"(rdi)
			  :
			  : "rcx", "rdx", "rsi", "r8", "r9", "r10", "r11",
			    "memory");
	return rax;
#endif
}

static inline long bunix_syscall2(long number, u64 arg0, u64 arg1)
{
#if defined(__riscv) && __riscv_xlen == 64
	register long a0 __asm__("a0") = (long)arg0;
	register long a1 __asm__("a1") = (long)arg1;
	register long a7 __asm__("a7") = number;

	__asm__ volatile ("ecall"
			  : "+r"(a0)
			  : "r"(a1), "r"(a7)
			  : "memory");
	return a0;
#else
	long rax = number;
	register u64 rdi __asm__("rdi") = arg0;
	register u64 rsi __asm__("rsi") = arg1;

	__asm__ volatile ("syscall"
			  : "+a"(rax), "+D"(rdi), "+S"(rsi)
			  :
			  : "rcx", "rdx", "r8", "r9", "r10", "r11", "memory");
	return rax;
#endif
}

static inline long bunix_syscall3(long number, u64 arg0, u64 arg1, u64 arg2)
{
#if defined(__riscv) && __riscv_xlen == 64
	register long a0 __asm__("a0") = (long)arg0;
	register long a1 __asm__("a1") = (long)arg1;
	register long a2 __asm__("a2") = (long)arg2;
	register long a7 __asm__("a7") = number;

	__asm__ volatile ("ecall"
			  : "+r"(a0)
			  : "r"(a1), "r"(a2), "r"(a7)
			  : "memory");
	return a0;
#else
	long rax = number;
	register u64 rdi __asm__("rdi") = arg0;
	register u64 rsi __asm__("rsi") = arg1;
	register u64 rdx __asm__("rdx") = arg2;

	__asm__ volatile ("syscall"
			  : "+a"(rax), "+D"(rdi), "+S"(rsi), "+d"(rdx)
			  :
			  : "rcx", "r8", "r9", "r10", "r11", "memory");
	return rax;
#endif
}

static inline long bunix_syscall4(long number, u64 arg0, u64 arg1, u64 arg2,
				  u64 arg3)
{
#if defined(__riscv) && __riscv_xlen == 64
	register long a0 __asm__("a0") = (long)arg0;
	register long a1 __asm__("a1") = (long)arg1;
	register long a2 __asm__("a2") = (long)arg2;
	register long a3 __asm__("a3") = (long)arg3;
	register long a7 __asm__("a7") = number;

	__asm__ volatile ("ecall"
			  : "+r"(a0)
			  : "r"(a1), "r"(a2), "r"(a3), "r"(a7)
			  : "memory");
	return a0;
#else
	long rax = number;
	register u64 rdi __asm__("rdi") = arg0;
	register u64 rsi __asm__("rsi") = arg1;
	register u64 rdx __asm__("rdx") = arg2;
	register u64 r10 __asm__("r10") = arg3;

	__asm__ volatile ("syscall"
			  : "+a"(rax), "+D"(rdi), "+S"(rsi), "+d"(rdx),
			    "+r"(r10)
			  :
			  : "rcx", "r8", "r9", "r11", "memory");
	return rax;
#endif
}

static inline long bunix_syscall5(long number, u64 arg0, u64 arg1, u64 arg2,
				  u64 arg3, u64 arg4)
{
#if defined(__riscv) && __riscv_xlen == 64
	register long a0 __asm__("a0") = (long)arg0;
	register long a1 __asm__("a1") = (long)arg1;
	register long a2 __asm__("a2") = (long)arg2;
	register long a3 __asm__("a3") = (long)arg3;
	register long a4 __asm__("a4") = (long)arg4;
	register long a7 __asm__("a7") = number;

	__asm__ volatile ("ecall"
			  : "+r"(a0)
			  : "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a7)
			  : "memory");
	return a0;
#else
	long rax = number;
	register u64 rdi __asm__("rdi") = arg0;
	register u64 rsi __asm__("rsi") = arg1;
	register u64 rdx __asm__("rdx") = arg2;
	register u64 r10 __asm__("r10") = arg3;
	register u64 r8 __asm__("r8") = arg4;

	__asm__ volatile ("syscall"
			  : "+a"(rax), "+D"(rdi), "+S"(rsi), "+d"(rdx),
			    "+r"(r10), "+r"(r8)
			  :
			  : "rcx", "r9", "r11", "memory");
	return rax;
#endif
}

static inline long bunix_syscall6(long number, u64 arg0, u64 arg1, u64 arg2,
				  u64 arg3, u64 arg4, u64 arg5)
{
#if defined(__riscv) && __riscv_xlen == 64
	register long a0 __asm__("a0") = (long)arg0;
	register long a1 __asm__("a1") = (long)arg1;
	register long a2 __asm__("a2") = (long)arg2;
	register long a3 __asm__("a3") = (long)arg3;
	register long a4 __asm__("a4") = (long)arg4;
	register long a5 __asm__("a5") = (long)arg5;
	register long a7 __asm__("a7") = number;

	__asm__ volatile ("ecall"
			  : "+r"(a0)
			  : "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5),
			    "r"(a7)
			  : "memory");
	return a0;
#else
	long rax = number;
	register u64 rdi __asm__("rdi") = arg0;
	register u64 rsi __asm__("rsi") = arg1;
	register u64 rdx __asm__("rdx") = arg2;
	register u64 r10 __asm__("r10") = arg3;
	register u64 r8 __asm__("r8") = arg4;
	register u64 r9 __asm__("r9") = arg5;

	__asm__ volatile ("syscall"
			  : "+a"(rax), "+D"(rdi), "+S"(rsi), "+d"(rdx),
			    "+r"(r10), "+r"(r8), "+r"(r9)
			  :
			  : "rcx", "r11", "memory");
	return rax;
#endif
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

static inline long bunix_cmdline_has(const char *token)
{
	return bunix_syscall1(BUNIX_SYSCALL_CMDLINE_HAS, (u64)token);
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

static inline long bunix_ipc_try_recv(u64 port, struct bunix_msg *message)
{
	return bunix_syscall2(BUNIX_SYSCALL_IPC_TRY_RECV, port, (u64)message);
}

static inline long bunix_ipc_recv_any(const u64 *ports, u64 count,
				      struct bunix_msg *message, u64 *index)
{
	return bunix_syscall4(BUNIX_SYSCALL_IPC_RECV_ANY, (u64)ports, count,
			      (u64)message, (u64)index);
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

static inline u64 bunix_handle_find(unsigned int tag)
{
	const long handle = bunix_syscall1(BUNIX_SYSCALL_HANDLE_FIND, tag);

	return handle > 0 ? (u64)handle : 0;
}

static inline long bunix_names_register_claim(u64 claim_handle,
					      u64 service_handle)
{
	long reply_handle;
	u64 names_handle;
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_NAMES,
		.type = BUNIX_NAMES_REGISTER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP,
		.reply = 0,
		.cap = service_handle,
		.words = { 0, 0, 0, 0 },
	};
	struct bunix_msg nudge = {
		.protocol = BUNIX_PROTO_NAMES,
		.type = BUNIX_NAMES_CLAIM_READY,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { 0, 0, 0, 0 },
	};
	struct bunix_msg reply;

	reply_handle = bunix_port_create("names-claim-reply");
	if (reply_handle <= 0) {
		return -1;
	}

	request.reply = (u64)reply_handle;
	if (bunix_ipc_send(claim_handle, &request) != 0) {
		bunix_handle_close((u64)reply_handle);
		return -1;
	}

	names_handle = bunix_handle_find(BUNIX_CAP_NAME);
	if (names_handle == 0 || bunix_ipc_send(names_handle, &nudge) != 0) {
		bunix_handle_close((u64)reply_handle);
		return -1;
	}

	if (bunix_ipc_recv((u64)reply_handle, &reply) != 0) {
		bunix_handle_close((u64)reply_handle);
		return -1;
	}
	bunix_handle_close((u64)reply_handle);
	return reply.words[0] == 0 ? 0 : -1;
}

static inline u64 bunix_task_handle_find(u64 task, unsigned int tag)
{
	const long handle =
		bunix_syscall2(BUNIX_SYSCALL_TASK_HANDLE_FIND, task, tag);

	return handle > 0 ? (u64)handle : 0;
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

static inline long bunix_task_map_prot(u64 task, u64 vaddr, const void *src,
				       u64 filesz, u64 memsz, u64 prot)
{
	const u64 args[] = { task, vaddr, (u64)src, filesz, memsz, prot };

	return bunix_syscall3(BUNIX_SYSCALL_TASK_MAP, (u64)args, 0, 0);
}

static inline long bunix_task_map(u64 task, u64 vaddr, const void *src,
				  u64 filesz, u64 memsz, u64 writable)
{
	const u64 prot = writable != 0 ?
		BUNIX_VM_PROT_READ | BUNIX_VM_PROT_WRITE :
		BUNIX_VM_PROT_READ | BUNIX_VM_PROT_EXEC;

	return bunix_task_map_prot(task, vaddr, src, filesz, memsz, prot);
}

static inline long bunix_task_grant(u64 task, u64 handle, unsigned int rights)
{
	return bunix_syscall3(BUNIX_SYSCALL_TASK_GRANT, task, handle, rights);
}

static inline long bunix_task_grant_tagged(u64 task, u64 handle,
					   unsigned int rights,
					   unsigned int tag)
{
	return bunix_syscall4(BUNIX_SYSCALL_TASK_GRANT_TAGGED, task, handle,
			      rights, tag);
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

static inline long bunix_task_alloc_prot(u64 task, u64 vaddr, u64 len,
					 u64 prot)
{
	const u64 args[] = { task, vaddr, len, prot };

	return bunix_syscall3(BUNIX_SYSCALL_TASK_ALLOC, (u64)args, 0, 0);
}

static inline long bunix_task_alloc(u64 task, u64 vaddr, u64 len,
				    u64 writable)
{
	const u64 prot = writable != 0 ?
		BUNIX_VM_PROT_READ | BUNIX_VM_PROT_WRITE :
		BUNIX_VM_PROT_READ;

	return bunix_task_alloc_prot(task, vaddr, len, prot);
}

static inline long bunix_task_clone_range(u64 dst_task, u64 src_task,
					  u64 vaddr, u64 len, u64 writable);

static inline long bunix_task_clone_range_prot(u64 dst_task, u64 src_task,
					       u64 vaddr, u64 len, u64 prot)
{
	const u64 args[] = { dst_task, src_task, vaddr, len, prot };

	return bunix_syscall3(BUNIX_SYSCALL_TASK_CLONE_RANGE, (u64)args, 0, 0);
}

static inline long bunix_task_clone_range(u64 dst_task, u64 src_task,
					  u64 vaddr, u64 len, u64 writable)
{
	const u64 prot = writable != 0 ?
		BUNIX_VM_PROT_READ | BUNIX_VM_PROT_WRITE :
		BUNIX_VM_PROT_READ | BUNIX_VM_PROT_EXEC;

	return bunix_task_clone_range_prot(dst_task, src_task, vaddr, len,
					   prot);
}

static inline long bunix_task_start_at(u64 task, u64 entry, u64 stack)
{
	return bunix_syscall3(BUNIX_SYSCALL_TASK_START_AT, task, entry, stack);
}

static inline long bunix_task_kill(u64 task)
{
	return bunix_syscall1(BUNIX_SYSCALL_TASK_KILL, task);
}

static inline long bunix_task_clear(u64 task)
{
	return bunix_syscall1(BUNIX_SYSCALL_TASK_CLEAR, task);
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

static inline long bunix_buffer_phys(u64 handle)
{
	return bunix_syscall1(BUNIX_SYSCALL_BUFFER_PHYS, handle);
}

static inline long bunix_vfs_stat_write(u64 buffer, u64 offset, u64 size,
					u64 mode_type, u64 owner, u64 dev,
					u64 ino, u64 nlink, u64 rdev,
					u64 blksize, u64 blocks)
{
	unsigned char stat[BUNIX_VFS_STAT_BYTES];

	if (buffer == 0) {
		return -1;
	}
	bunix_vfs_stat_pack(stat, size, mode_type, owner, dev, ino, nlink,
			    rdev, blksize, blocks);
	return bunix_buffer_write(buffer, offset, stat, sizeof(stat));
}

static inline long bunix_vfs_stat_write_times(u64 buffer, u64 offset, u64 size,
					      u64 mode_type, u64 owner,
					      u64 dev, u64 ino, u64 nlink,
					      u64 rdev, u64 blksize,
					      u64 blocks, u64 atime,
					      u64 mtime, u64 ctime)
{
	unsigned char stat[BUNIX_VFS_STAT_BYTES];

	if (buffer == 0) {
		return -1;
	}
	bunix_vfs_stat_pack_times(stat, size, mode_type, owner, dev, ino,
				  nlink, rdev, blksize, blocks, atime, mtime,
				  ctime);
	return bunix_buffer_write(buffer, offset, stat, sizeof(stat));
}

static inline u64 bunix_cstring_len(const char *text)
{
	u64 len = 0;

	while (text != 0 && text[len] != '\0') {
		len++;
	}
	return len;
}

static inline long bunix_read_path_cap(const struct bunix_msg *message,
				       char *path, u64 path_cap)
{
	const u64 len = message != 0 ? message->words[0] : 0;

	if (message == 0 || path == 0 || len == 0 || len > path_cap ||
	    (message->cap_rights & BUNIX_RIGHT_RECV) == 0 ||
	    bunix_buffer_read(message->cap, 0, path, len) != 0 ||
	    path[len - 1] != '\0') {
		return -1;
	}
	return 0;
}

static inline long bunix_ipc_call_path(u64 port, unsigned int protocol,
				       unsigned int type, const char *path,
				       u64 word1, u64 word2, u64 word3,
				       struct bunix_msg *reply)
{
	const u64 len = bunix_cstring_len(path) + 1;
	long buffer;
	long result;
	struct bunix_msg request = {
		.protocol = protocol,
		.type = type,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_RECV,
		.reply = 0,
		.cap = 0,
		.words = { len, word1, word2, word3 },
	};

	if (path == 0 || len == 1) {
		return -1;
	}
	buffer = bunix_buffer_create(len);
	if (buffer < 0 || bunix_buffer_write((u64)buffer, 0, path, len) != 0) {
		if (buffer >= 0) {
			bunix_handle_close((u64)buffer);
		}
		return -1;
	}
	request.cap = (u64)buffer;
	result = bunix_ipc_call(port, &request, reply);
	bunix_handle_close((u64)buffer);
	return result;
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
	const u64 console = bunix_handle_find(BUNIX_CAP_CONS);
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
	result = bunix_ipc_send(console != 0 ? console : BUNIX_HANDLE_CONSOLE,
				&message);
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
	const u64 console = bunix_handle_find(BUNIX_CAP_CONS);
	const struct bunix_msg message = {
		.protocol = BUNIX_PROTO_CONSOLE,
		.type = BUNIX_CONSOLE_LOGS_TO_RING,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { 0, 0, 0, 0 },
	};

	return bunix_ipc_send(console != 0 ? console : BUNIX_HANDLE_CONSOLE,
			      &message);
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

static inline long bunix_sched_stats(struct bunix_sched_stats *stats)
{
	return bunix_syscall1(BUNIX_SYSCALL_SCHED_STATS, (u64)stats);
}

static inline long bunix_sched_thread_info(u64 index,
					   struct bunix_sched_thread_info *info)
{
	return bunix_syscall2(BUNIX_SYSCALL_SCHED_THREAD_INFO, index,
			      (u64)info);
}

static inline long bunix_sched_policy_grant(
	u64 authority, u64 task, const struct bunix_sched_policy *policy)
{
	return bunix_syscall3(BUNIX_SYSCALL_SCHED_POLICY_GRANT,
			      authority, task, (u64)policy);
}

static inline long bunix_sched_policy_get(
	u64 authority, struct bunix_sched_policy_state *state)
{
	return bunix_syscall2(BUNIX_SYSCALL_SCHED_POLICY_GET, authority,
			      (u64)state);
}

static inline long bunix_sched_policy_set(
	u64 authority, const struct bunix_sched_policy_state *state)
{
	return bunix_syscall2(BUNIX_SYSCALL_SCHED_POLICY_SET, authority,
			      (u64)state);
}

static inline long bunix_vm_stats(struct bunix_vm_stats *stats)
{
	return bunix_syscall1(BUNIX_SYSCALL_VM_STATS, (u64)stats);
}

static inline long bunix_boot_event_record(const char *name)
{
	return bunix_syscall2(BUNIX_SYSCALL_BOOT_EVENT,
			      BUNIX_BOOT_EVENT_RECORD, (u64)name);
}

static inline long bunix_boot_event_read(u64 index,
					 struct bunix_boot_event *event)
{
	return bunix_syscall3(BUNIX_SYSCALL_BOOT_EVENT,
			      BUNIX_BOOT_EVENT_READ, index, (u64)event);
}

static inline long bunix_machine_poweroff(u64 authority)
{
	return bunix_syscall1(BUNIX_SYSCALL_MACHINE_POWER, authority);
}

static inline long bunix_hw_port_in8(u64 handle, u64 offset)
{
	return bunix_syscall2(BUNIX_SYSCALL_HW_PORT_IN8, handle, offset);
}

static inline long bunix_hw_port_out8(u64 handle, u64 offset, u64 value)
{
	return bunix_syscall3(BUNIX_SYSCALL_HW_PORT_OUT8, handle, offset, value);
}

static inline long bunix_hw_port_in16(u64 handle, u64 offset)
{
	return bunix_syscall2(BUNIX_SYSCALL_HW_PORT_IN16, handle, offset);
}

static inline long bunix_hw_port_out16(u64 handle, u64 offset, u64 value)
{
	return bunix_syscall3(BUNIX_SYSCALL_HW_PORT_OUT16, handle, offset, value);
}

static inline long bunix_hw_port_in32(u64 handle, u64 offset)
{
	return bunix_syscall2(BUNIX_SYSCALL_HW_PORT_IN32, handle, offset);
}

static inline long bunix_hw_port_out32(u64 handle, u64 offset, u64 value)
{
	return bunix_syscall3(BUNIX_SYSCALL_HW_PORT_OUT32, handle, offset, value);
}

static inline long bunix_hw_pci_bar_grant(u64 authority, u64 device,
					  u64 offset, u64 len, u64 ops)
{
	return bunix_syscall5(BUNIX_SYSCALL_HW_PCI_BAR_GRANT, device, offset,
			      len, ops, authority);
}

static inline long bunix_hw_pci_irq_grant(u64 authority, u64 device, u64 line)
{
	return bunix_syscall3(BUNIX_SYSCALL_HW_PCI_IRQ_GRANT, device, line,
			      authority);
}

static inline long bunix_hw_irq_bind(u64 handle, u64 index, u64 event_port)
{
	return bunix_syscall3(BUNIX_SYSCALL_HW_IRQ_BIND, handle, index,
			      event_port);
}

static inline long bunix_hw_irq_ack(u64 handle, u64 index)
{
	return bunix_syscall2(BUNIX_SYSCALL_HW_IRQ_ACK, handle, index);
}

static inline long bunix_hw_irq_mask(u64 handle, u64 index, u64 masked)
{
	return bunix_syscall3(BUNIX_SYSCALL_HW_IRQ_MASK, handle, index, masked);
}

static inline long bunix_hw_mmio_read8(u64 handle, u64 offset)
{
	return bunix_syscall2(BUNIX_SYSCALL_HW_MMIO_READ8, handle, offset);
}

static inline long bunix_hw_mmio_write8(u64 handle, u64 offset, u64 value)
{
	return bunix_syscall3(BUNIX_SYSCALL_HW_MMIO_WRITE8, handle, offset,
			      value);
}

static inline long bunix_hw_mmio_read16(u64 handle, u64 offset)
{
	return bunix_syscall2(BUNIX_SYSCALL_HW_MMIO_READ16, handle, offset);
}

static inline long bunix_hw_mmio_write16(u64 handle, u64 offset, u64 value)
{
	return bunix_syscall3(BUNIX_SYSCALL_HW_MMIO_WRITE16, handle, offset,
			      value);
}

static inline long bunix_hw_mmio_read32(u64 handle, u64 offset)
{
	return bunix_syscall2(BUNIX_SYSCALL_HW_MMIO_READ32, handle, offset);
}

static inline long bunix_hw_mmio_write32(u64 handle, u64 offset, u64 value)
{
	return bunix_syscall3(BUNIX_SYSCALL_HW_MMIO_WRITE32, handle, offset,
			      value);
}

#endif
