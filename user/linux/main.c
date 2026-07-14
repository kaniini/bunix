#include <bunix/libbunix.h>
#include <bunix/id_table.h>

#define LINUX_HANDLE_CONSOLE (bunix_handle_find(BUNIX_CAP_CONS))
#define LINUX_HANDLE_VFS (bunix_handle_find(BUNIX_CAP_VFS))
#define LINUX_HANDLE_NAMES (bunix_handle_find(BUNIX_CAP_NAME))
#define LINUX_HANDLE_POWER_AUTH (bunix_handle_find(BUNIX_CAP_POWR))
#define LINUX_HANDLE_USER_MGMT (bunix_handle_find(BUNIX_CAP_USRM))
#define LINUX_HANDLE_PROC_MGMT (bunix_handle_find(BUNIX_CAP_PRMG))
#define LINUX_HANDLE_MGMT (bunix_handle_find(BUNIX_CAP_LNXM))

enum {
	LINUX_EPERM = 1,
	LINUX_ENOENT = 2,
	LINUX_E2BIG = 7,
	LINUX_EIO = 5,
	LINUX_EBADF = 9,
	LINUX_EAGAIN = 11,
	LINUX_EACCES = 13,
	LINUX_EFAULT = 14,
	LINUX_EBUSY = 16,
	LINUX_EEXIST = 17,
	LINUX_EXDEV = 18,
	LINUX_ENODEV = 19,
	LINUX_ENOTDIR = 20,
	LINUX_EISDIR = 21,
	LINUX_EINVAL = 22,
	LINUX_EMFILE = 24,
	LINUX_ENOTTY = 25,
	LINUX_ESPIPE = 29,
	LINUX_EPIPE = 32,
	LINUX_EADDRINUSE = 98,
	LINUX_EISCONN = 106,
	LINUX_ENOTCONN = 107,
	LINUX_ECONNREFUSED = 111,
	LINUX_ERANGE = 34,
	LINUX_ENAMETOOLONG = 36,
	LINUX_ENOSYS = 38,
	LINUX_ENOTEMPTY = 39,
	LINUX_ELOOP = 40,
	LINUX_EAFNOSUPPORT = 97,
	LINUX_EDESTADDRREQ = 89,
	LINUX_EPROTONOSUPPORT = 93,
	LINUX_ENOTSOCK = 88,
	LINUX_ESRCH = 3,
	LINUX_ECHILD = 10,
	LINUX_ENOMEM = 12,
	LINUX_WAIT_BLOCK = 0x7fffffff,
	LINUX_EINTR = 4,
	LINUX_GETUID = 102,
	LINUX_GETGID = 104,
	LINUX_SETUID = 105,
	LINUX_SETGID = 106,
	LINUX_GETEUID = 107,
	LINUX_GETEGID = 108,
	LINUX_GETGROUPS = 115,
	LINUX_SETGROUPS = 116,
	LINUX_SETRESUID = 117,
	LINUX_SETRESGID = 119,
	LINUX_WNOHANG = 1,
	LINUX_WUNTRACED = 2,
	LINUX_WCONTINUED = 8,
	LINUX_TCGETS = 0x5401,
	LINUX_TCSETS = 0x5402,
	LINUX_TCSETSW = 0x5403,
	LINUX_TCSETSF = 0x5404,
	LINUX_TIOCGPGRP = 0x540f,
	LINUX_TIOCSPGRP = 0x5410,
	LINUX_TIOCGWINSZ = 0x5413,
	LINUX_SIOCGIFFLAGS = 0x8913,
	LINUX_SIOCSIFFLAGS = 0x8914,
	LINUX_SIOCGIFHWADDR = 0x8927,
	LINUX_SIOCGIFMTU = 0x8921,
	LINUX_SIOCGIFINDEX = 0x8933,
	LINUX_SIOCGIFTXQLEN = 0x8942,
	LINUX_IFNAMSIZ = 16,
	LINUX_IFREQ_SIZE = 40,
	LINUX_ARPHRD_ETHER = 1,
	LINUX_ARPHRD_LOOPBACK = 772,
	LINUX_IFF_UP = 0x1,
	LINUX_IFF_BROADCAST = 0x2,
	LINUX_IFF_LOOPBACK = 0x8,
	LINUX_IFF_RUNNING = 0x40,
	LINUX_TERM_SIZE = 60,
	LINUX_TERM_IFLAG = 0,
	LINUX_TERM_OFLAG = 4,
	LINUX_TERM_CFLAG = 8,
	LINUX_TERM_LFLAG = 12,
	LINUX_TERM_LINE = 16,
	LINUX_TERM_CC = 17,
	LINUX_VINTR = 0,
	LINUX_VERASE = 2,
	LINUX_VEOF = 4,
	LINUX_VTIME = 5,
	LINUX_VMIN = 6,
	LINUX_SIGHUP = 1,
	LINUX_SIGINT = 2,
	LINUX_SIGBUS = 7,
	LINUX_SIGKILL = 9,
	LINUX_SIGSEGV = 11,
	LINUX_SIGPIPE = 13,
	LINUX_SIGALRM = 14,
	LINUX_SIGTERM = 15,
	LINUX_SIGCHLD = 17,
	LINUX_SIGSTOP = 19,
	LINUX_ICRNL = 0000400,
	LINUX_OPOST = 0000001,
	LINUX_ONLCR = 0000004,
	LINUX_CS8 = 0000060,
	LINUX_CREAD = 0000200,
	LINUX_ISIG = 0000001,
	LINUX_ICANON = 0000002,
	LINUX_ECHO = 0000010,
	LINUX_ECHOE = 0000020,
	LINUX_ECHOK = 0000040,
	LINUX_TTY_CANON_MAX = 4096,
	LINUX_TTY_READ_QUEUE_MAX = 8192,
	LINUX_MAX_WRITE = 65536,
	LINUX_MAX_MMAP_BUFFER = 1024 * 1024,
	LINUX_MMAP_PAGE_SIZE = 4096,
	LINUX_MMAP_PAGE_DATA = BUNIX_VFS_MMAP_PAGE_DATA,
	LINUX_MMAP_PAGE_ZERO = BUNIX_VFS_MMAP_PAGE_ZERO,
	LINUX_MMAP_PAGE_BUS = BUNIX_VFS_MMAP_PAGE_BUS,
	LINUX_MAX_DIRENT_BUFFER = 512 * 1024,
	LINUX_MAX_PATH = 4096,
	LINUX_EXEC_MAX_STRING = 128 * 1024,
	LINUX_EXEC_MAX_STRING_BYTES = 384 * 1024,
	LINUX_EXEC_MAX_POINTERS = LINUX_EXEC_MAX_STRING_BYTES / 8,
	LINUX_SHEBANG_MAX = 256,
	LINUX_SHEBANG_MAX_DEPTH = 4,
	LINUX_EXEC_PREPARE_PATH_OFF = 0,
	LINUX_EXEC_PREPARE_IMAGE_OFF = LINUX_MAX_PATH,
	LINUX_EXEC_PREPARE_VECTOR_OFF = LINUX_MAX_PATH + LINUX_SHEBANG_MAX,
	LINUX_EXEC_PREPARE_BUFFER_SIZE =
		LINUX_EXEC_PREPARE_VECTOR_OFF + LINUX_EXEC_MAX_STRING_BYTES +
		(LINUX_EXEC_MAX_POINTERS * sizeof(u64)) + 2 * sizeof(u64),
	LINUX_NAME_MAX = 255,
	LINUX_TIMEVAL_SIZE = 16,
	LINUX_TIMESPEC_SIZE = 16,
	LINUX_TIME_T_SIZE = 8,
	LINUX_SYSINFO_SIZE = 112,
	LINUX_UTSNAME_SIZE = 65 * 6,
	LINUX_STAT_SIZE = 144,
	LINUX_STATX_SIZE = 256,
	LINUX_STATFS_SIZE = 120,
	LINUX_STATX_BASIC_STATS = 0x7ff,
	LINUX_STATFS_MAGIC = 0x42554e58,
	LINUX_STATFS_BLOCK_SIZE = 4096,
	LINUX_STATFS_BLOCKS = 32768,
	LINUX_STATFS_FREE_BLOCKS = 16384,
	LINUX_INITIAL_FDS = 16,
	LINUX_PIPE_CAPACITY = 65536,
	LINUX_ACCESS_CACHE_SIZE = 8,
	LINUX_GLOBAL_PATH_CACHE_SIZE = 512,
	LINUX_FILE_READ_CACHE_SIZE = 128,
	LINUX_FILE_READ_CACHE_MAX = 4096,
	LINUX_FILE_READAHEAD_SIZE = 4096,
	LINUX_FILE_READAHEAD_TRIGGER_MAX = 256,
	LINUX_DIR_CACHE_SIZE = 16,
	LINUX_DIR_CACHE_MAX_ENTRIES = 128,
	LINUX_CLOCK_REALTIME = 0,
	LINUX_CLOCK_MONOTONIC = 1,
	/* 2026-01-01 UTC until the time service grows an RTC-backed wall clock. */
	LINUX_REALTIME_BOOT_SECONDS = 1767225600ull,
	LINUX_CACHE_DOMAIN_OTHER = 0,
	LINUX_CACHE_DOMAIN_ETC = 1,
	LINUX_CACHE_DOMAIN_RUN = 2,
	LINUX_CACHE_DOMAIN_COUNT = 3,
	LINUX_UTIME_NOW = 0x3fffffff,
	LINUX_UTIME_OMIT = 0x3ffffffe,
	LINUX_PROC_SPAWN_LINUX = 2,
	LINUX_S_IFCHR = 0020000,
	LINUX_S_IFBLK = 0060000,
	LINUX_S_IFDIR = 0040000,
	LINUX_S_IFLNK = 0120000,
	LINUX_S_IFREG = 0100000,
	LINUX_S_IFSOCK = 0140000,
	LINUX_S_IFIFO = 0010000,
	LINUX_S_IFMT = 0170000,
	LINUX_O_ACCMODE = 3,
	LINUX_O_WRONLY = 1,
	LINUX_O_RDWR = 2,
	LINUX_O_CREAT = 0100,
	LINUX_O_EXCL = 0200,
	LINUX_O_TRUNC = 01000,
	LINUX_O_APPEND = 02000,
	LINUX_O_NONBLOCK = 04000,
	LINUX_O_NOFOLLOW = 00400000,
	LINUX_O_DIRECTORY = 00200000,
	LINUX_O_CLOEXEC = 02000000,
	LINUX___O_TMPFILE = 020000000,
	LINUX_O_TMPFILE = 020200000,
	LINUX_O_TMPFILE_MASK = 020300000,
	LINUX_DUP_CLOEXEC = 02000000,
	LINUX_FD_CLOEXEC = 1,
	LINUX_FD_TMPFILE = 1 << 1,
	LINUX_FD_CONSOLE = 1,
	LINUX_FD_FILE = 2,
	LINUX_FD_DIR = 3,
	LINUX_FD_SOCKET = 5,
	LINUX_FD_PIPE_READ = 6,
	LINUX_FD_PIPE_WRITE = 7,
	LINUX_SEEK_SET = 0,
	LINUX_SEEK_CUR = 1,
	LINUX_SEEK_END = 2,
	LINUX_AF_UNIX = 1,
	LINUX_AF_INET = 2,
	LINUX_AF_INET6 = 10,
	LINUX_AF_NETLINK = 16,
	LINUX_AF_PACKET = 17,
	LINUX_SOCK_STREAM = 1,
	LINUX_SOCK_DGRAM = 2,
	LINUX_SOCK_RAW = 3,
	LINUX_SOL_SOCKET = 1,
	LINUX_SO_DEBUG = 1,
	LINUX_SO_REUSEADDR = 2,
	LINUX_SO_KEEPALIVE = 9,
	LINUX_SO_ERROR = 4,
	LINUX_SO_TYPE = 3,
	LINUX_SOCK_NONBLOCK = 00004000,
	LINUX_SOCK_CLOEXEC = 02000000,
	LINUX_SOCKET_UNIX_STREAM = 1,
	LINUX_SOCKET_UTMPD = 2,
	LINUX_SOCKET_NET_UDP = 3,
	LINUX_SOCKET_NET_TCP = 4,
	LINUX_SOCKET_NET_ICMP = 5,
	LINUX_SOCKET_NET_RAW = 6,
	LINUX_SOCKET_NET_PACKET = 7,
	LINUX_SOCKET_NETLINK_ROUTE = 8,
	LINUX_SOCKET_UNIX_DGRAM = 9,
	LINUX_FD_NETLINK_ACK_PENDING = 2,
	LINUX_FD_NETLINK_DONE_PENDING = 4,
	LINUX_NETLINK_ROUTE = 0,
	LINUX_NLM_F_REQUEST = 0x1,
	LINUX_NLM_F_MULTI = 0x2,
	LINUX_NLM_F_ACK = 0x4,
	LINUX_NLM_F_DUMP = 0x300,
	LINUX_NLMSG_ERROR = 2,
	LINUX_NLMSG_DONE = 3,
	LINUX_RTM_NEWLINK = 16,
	LINUX_RTM_DELLINK = 17,
	LINUX_RTM_GETLINK = 18,
	LINUX_RTM_SETLINK = 19,
	LINUX_RTM_NEWADDR = 20,
	LINUX_RTM_DELADDR = 21,
	LINUX_RTM_GETADDR = 22,
	LINUX_RTM_NEWROUTE = 24,
	LINUX_RTM_DELROUTE = 25,
	LINUX_RTM_GETROUTE = 26,
	LINUX_IFLA_ADDRESS = 1,
	LINUX_IFLA_IFNAME = 3,
	LINUX_IFLA_MTU = 4,
	LINUX_IFA_ADDRESS = 1,
	LINUX_IFA_LOCAL = 2,
	LINUX_IFA_LABEL = 3,
	LINUX_RTA_DST = 1,
	LINUX_RTA_OIF = 4,
	LINUX_RTA_GATEWAY = 5,
	LINUX_RTA_PRIORITY = 6,
	LINUX_RT_TABLE_MAIN = 254,
	LINUX_RTPROT_BOOT = 3,
	LINUX_RTPROT_DHCP = 16,
	LINUX_RT_SCOPE_UNIVERSE = 0,
	LINUX_RT_SCOPE_LINK = 253,
	LINUX_RTN_UNICAST = 1,
	LINUX_IPPROTO_ICMP = 1,
	LINUX_IPPROTO_TCP = 6,
	LINUX_IPPROTO_UDP = 17,
	LINUX_IPPROTO_ICMPV6 = 58,
	LINUX_IPPROTO_RAW = 255,
	LINUX_MSG_DONTWAIT = 0x40,
	LINUX_ETH_P_IP_NET = 0x0008,
	LINUX_ETH_P_IP = 0x0800,
	LINUX_ETHERNET_HEADER_SIZE = 14,
	LINUX_RECV_BLOCK_RETRIES = 5000,
	LINUX_RECV_BLOCK_SLEEP_NS = 1000000,
	LINUX_DT_FIFO = 1,
	LINUX_DT_CHR = 2,
	LINUX_DT_REG = 8,
	LINUX_DT_DIR = 4,
	LINUX_DT_LNK = 10,
	LINUX_UTMPS_NONE = 0,
	LINUX_UTMPS_REWIND = 'r',
	LINUX_UTMPS_GETENT = 'e',
	LINUX_AT_FDCWD = (u64)-100,
	LINUX_AT_SYMLINK_NOFOLLOW = 0x100,
	LINUX_AT_REMOVEDIR = 0x200,
	LINUX_AT_EACCESS = 0x200,
	LINUX_AT_SYMLINK_FOLLOW = 0x400,
	LINUX_AT_NO_AUTOMOUNT = 0x800,
	LINUX_AT_EMPTY_PATH = 0x1000,
	LINUX_AT_STATX_SYNC_TYPE = 0x6000,
	LINUX_RENAME_NOREPLACE = 1,
	LINUX_R_OK = 4,
	LINUX_W_OK = 2,
	LINUX_X_OK = 1,
	LINUX_F_DUPFD = 0,
	LINUX_F_GETFD = 1,
	LINUX_F_SETFD = 2,
	LINUX_F_GETFL = 3,
	LINUX_F_SETFL = 4,
	LINUX_F_GETLK = 5,
	LINUX_F_SETLK = 6,
	LINUX_F_SETLKW = 7,
	LINUX_F_DUPFD_CLOEXEC = 1030,
	LINUX_CLOSE_RANGE_UNSHARE = 1 << 1,
	LINUX_CLOSE_RANGE_CLOEXEC = 1 << 2,
	LINUX_UTMP_RECORD_SIZE = 400,
	LINUX_REBOOT_MAGIC1 = 0xfee1dead,
	LINUX_REBOOT_MAGIC2 = 672274793,
	LINUX_REBOOT_MAGIC2A = 85072278,
	LINUX_REBOOT_MAGIC2B = 369367448,
	LINUX_REBOOT_MAGIC2C = 537993216,
	LINUX_REBOOT_CMD_RESTART = 0x01234567,
	LINUX_REBOOT_CMD_HALT = 0xcdef0123,
	LINUX_REBOOT_CMD_POWER_OFF = 0x4321fedc,
};

struct linux_open_file {
	u64 handle;
	u64 offset;
	u64 size;
	u64 refs;
	u64 read_cache_offset;
	u64 read_cache_len;
	u64 read_cache_domain;
	u64 read_cache_epoch;
	char *read_cache;
};

struct linux_fd {
	u64 handle;
	u64 kind;
	u64 offset;
	u64 size;
	u64 flags;
	u64 status_flags;
	u64 socket_type;
	struct linux_open_file *open_file;
	char path[LINUX_MAX_PATH];
};

static u64 linux_tmpfile_counter;

struct linux_vfs_stat {
	u64 size;
	u64 mode_type;
	u64 owner;
	u64 dev;
	u64 ino;
	u64 nlink;
	u64 rdev;
	u64 blksize;
	u64 blocks;
	u64 atime;
	u64 mtime;
	u64 ctime;
};

struct linux_access_cache_entry {
	u64 valid;
	u64 stat_valid;
	u64 base_handle;
	u64 mode;
	u64 flags;
	u64 nofollow;
	u64 cache_domain;
	u64 cache_epoch;
	long result;
	long stat_result;
	struct linux_vfs_stat stat;
	char path[LINUX_MAX_PATH];
};

struct linux_file_read_cache_entry {
	u64 valid;
	u64 offset;
	u64 len;
	u64 cache_domain;
	u64 cache_epoch;
	char path[LINUX_MAX_PATH];
	char data[LINUX_FILE_READ_CACHE_MAX];
};

struct linux_dir_cache_entry {
	u64 valid;
	u64 cache_domain;
	u64 cache_epoch;
	u64 count;
	char path[LINUX_MAX_PATH];
	struct {
		u64 type;
		char name[LINUX_NAME_MAX + 1];
	} entries[LINUX_DIR_CACHE_MAX_ENTRIES];
};

struct linux_process {
	u64 pid;
	u64 tid;
	u64 ppid;
	u64 pgid;
	u64 sid;
	struct linux_process *parent;
	struct linux_process *first_child;
	struct linux_process *next_sibling;
	u64 bunix_task;
	u64 bunix_proc_pid;
	u64 bunix_thread;
	u64 exited;
	u64 exit_status;
	u64 waited;
	u64 waiter;
	u64 wait_buffer;
	u64 wait_pid;
	u64 pending_signals;
	u64 signal_mask;
	u64 signal_ignored;
	u64 signal_handlers[64];
	u64 signal_restorers[64];
	u64 signal_flags[64];
	u64 alarm_deadline_ns;
	u64 alarm_active;
	u64 umask;
	u64 session_id;
	u64 session_owner;
	u64 cwd_handle;
	char *cwd;
	struct linux_fd *fds;
	u64 fd_capacity;
	u64 access_cache_epoch;
	u64 access_cache_next;
	u64 last_mmap_page_class;
	struct linux_access_cache_entry access_cache[LINUX_ACCESS_CACHE_SIZE];
};

struct linux_pipe {
	u64 id;
	u64 read_refs;
	u64 write_refs;
	u64 start;
	u64 len;
	u64 reader_reply;
	u64 reader_buffer;
	u64 reader_len;
	u64 writer_reply;
	u64 writer_buffer;
	u64 writer_len;
	char data[LINUX_PIPE_CAPACITY];
};

struct linux_tty {
	u64 session_id;
	u64 owner_pid;
	u64 foreground_pgid;
	char termios[LINUX_TERM_SIZE];
	char line[LINUX_TTY_CANON_MAX];
	u64 line_len;
	char read_queue[LINUX_TTY_READ_QUEUE_MAX];
	u64 read_queue_start;
	u64 read_queue_len;
	u64 reader_pid;
	u64 reader_reply;
	u64 reader_buffer;
	u64 reader_len;
	u64 cpr_state;
};

static struct bunix_map process_by_task;
static struct bunix_map process_by_pid;
static struct bunix_id_table pipe_ids;
static char write_buffer[LINUX_MAX_WRITE];
static struct linux_tty console_tty;
static u64 linux_vfs_mutation_epochs[LINUX_CACHE_DOMAIN_COUNT];
static struct linux_access_cache_entry
	linux_global_path_cache[LINUX_GLOBAL_PATH_CACHE_SIZE];
static u64 linux_global_path_cache_next;
static struct linux_file_read_cache_entry
	linux_file_read_cache[LINUX_FILE_READ_CACHE_SIZE];
static u64 linux_file_read_cache_next;
static struct linux_dir_cache_entry linux_dir_cache[LINUX_DIR_CACHE_SIZE];
static u64 linux_dir_cache_next;
static struct bunix_map file_refs;
static struct bunix_map net_socket_refs;
static struct bunix_map tty_input_tasks;
static u64 next_pid = 1;
static u64 user_service;
static u64 utmpfs_service;
static u64 procfs_service;
static u64 tmpfs_service;
static u64 devfs_service;
static u64 unionfs_service;
static u64 net_service;
static char linux_hostname[65] = "bunix";

static u64 resolve_service(u64 service, unsigned int rights);
static void linux_process_reset(struct linux_process *process);
static void linux_access_cache_clear(struct linux_process *process);
static void linux_close_process_fds(struct linux_process *process);
static long linux_user_process_exit(u64 pid);
static void linux_wake_parent(struct linux_process *child);
static int linux_signal_process(struct linux_process *process, u64 signal);
static u64 linux_fd_offset(const struct linux_fd *fd);
static u64 linux_fd_size(const struct linux_fd *fd);
static int linux_tty_read_queue_push(struct linux_tty *tty, char c);
static int linux_tty_read_queue_push_front(struct linux_tty *tty, char c);
static void linux_tty_wake_reader(struct linux_tty *tty);
static void linux_tty_cancel_reader(struct linux_tty *tty,
				    struct linux_process *process);
static int linux_tty_session_matches(const struct linux_tty *tty,
				     const struct linux_process *process);
static void linux_tty_bind_session(struct linux_tty *tty,
				   const struct linux_process *process);
static void linux_tty_note_process(struct linux_tty *tty,
				   const struct linux_process *process);
static u64 string_len(const char *text);
static u64 linux_realtime_seconds(void);
static int linux_debug_paths_enabled_for_task(const struct linux_process *process);
static long linux_check_name_max(const char *path);
static int linux_path_prefix(const char *path, const char *prefix);
static long linux_read_path_arg(u64 path_buffer, u64 path_len, char *path,
				u64 path_cap);
static long alloc_fd(struct linux_process *process, u64 kind, u64 handle,
		     u64 size);

static u64 linux_user_service(void)
{
	if (user_service == 0) {
		user_service = resolve_service(BUNIX_SERVICE_USER,
					       BUNIX_RIGHT_SEND);
	}

	return user_service;
}

static u64 linux_utmpfs_service(void)
{
	if (utmpfs_service == 0) {
		utmpfs_service = resolve_service(BUNIX_SERVICE_UTMPFS,
						 BUNIX_RIGHT_SEND);
	}

	return utmpfs_service;
}

static u64 linux_cached_service(u64 service, u64 *cache)
{
	if (*cache == 0) {
		*cache = resolve_service(service, BUNIX_RIGHT_SEND);
	}
	return *cache;
}

static long linux_user_process_register(u64 bunix_task)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_USER,
		.type = BUNIX_USER_REGISTER_PROCESS,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { bunix_task, 0, 0, 0 },
	};
	struct bunix_msg reply;
	const u64 user = LINUX_HANDLE_USER_MGMT;

	if (user == 0) {
		bunix_console_log("linux-server: user mgmt missing\n",
				  sizeof("linux-server: user mgmt missing\n") - 1);
		return -LINUX_ESRCH;
	}
	if (bunix_ipc_call(user, &request, &reply) != 0) {
		bunix_console_log("linux-server: user register call failed\n",
				  sizeof("linux-server: user register call failed\n") - 1);
		return -LINUX_ESRCH;
	}
	if (reply.words[0] != 0) {
		bunix_console_log("linux-server: user register denied\n",
				  sizeof("linux-server: user register denied\n") - 1);
		return -LINUX_ESRCH;
	}

	return 0;
}

static long linux_user_process_fork(u64 parent_task, u64 child_task)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_USER,
		.type = BUNIX_USER_FORK_PROCESS,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { parent_task, child_task, 0, 0 },
	};
	struct bunix_msg reply;
	const u64 user = LINUX_HANDLE_USER_MGMT;

	if (user == 0) {
		bunix_console_log("linux-server: user mgmt missing\n",
				  sizeof("linux-server: user mgmt missing\n") - 1);
		return -LINUX_ESRCH;
	}
	if (bunix_ipc_call(user, &request, &reply) != 0) {
		bunix_console_log("linux-server: user fork call failed\n",
				  sizeof("linux-server: user fork call failed\n") - 1);
		return -LINUX_ESRCH;
	}
	if (reply.words[0] != 0) {
		bunix_console_log("linux-server: user fork denied\n",
				  sizeof("linux-server: user fork denied\n") - 1);
		return -LINUX_ESRCH;
	}

	return 0;
}

static long linux_user_process_exit(u64 bunix_task)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_USER,
		.type = BUNIX_USER_EXIT_PROCESS,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { bunix_task, 0, 0, 0 },
	};
	struct bunix_msg reply;
	const u64 user = LINUX_HANDLE_USER_MGMT;

	if (bunix_task == 0 || user == 0) {
		return 0;
	}

	if (bunix_ipc_call(user, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return -LINUX_ESRCH;
	}

	return 0;
}

static long linux_user_session_get(u64 session_id, u64 *uid, u64 *gid,
				   u64 *tty, u64 *foreground)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_USER,
		.type = BUNIX_USER_SESSION_GET,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { session_id, 0, 0, 0 },
	};
	struct bunix_msg reply;
	const u64 user = LINUX_HANDLE_USER_MGMT;

	if (user == 0 ||
	    bunix_ipc_call(user, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return -LINUX_ESRCH;
	}
	if (uid != 0) {
		*uid = reply.words[1];
	}
	if (gid != 0) {
		*gid = reply.words[2];
	}
	if (tty != 0) {
		*tty = reply.words[3] >> 32;
	}
	if (foreground != 0) {
		*foreground = reply.words[3] & 0xffffffff;
	}
	return 0;
}

static long linux_user_session_set_foreground(u64 session_id, u64 foreground)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_USER,
		.type = BUNIX_USER_SESSION_SET_FOREGROUND,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { session_id, foreground, 0, 0 },
	};
	struct bunix_msg reply;
	const u64 user = LINUX_HANDLE_USER_MGMT;

	if (user == 0 ||
	    bunix_ipc_call(user, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return -LINUX_ESRCH;
	}
	return 0;
}

static long linux_user_session_end(u64 session_id)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_USER,
		.type = BUNIX_USER_SESSION_END,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { session_id, 0, 0, 0 },
	};
	struct bunix_msg reply;
	const u64 user = LINUX_HANDLE_USER_MGMT;

	if (session_id == 0 || user == 0) {
		return 0;
	}
	return bunix_ipc_call(user, &request, &reply) == 0 &&
		       reply.words[0] == 0 ?
	       0 : -LINUX_ESRCH;
}

static long linux_attach_session(struct linux_process *process, u64 session_id)
{
	if (process == 0 || session_id == 0 ||
	    linux_user_session_get(session_id, 0, 0, 0, 0) != 0) {
		return -LINUX_ESRCH;
	}
	process->session_id = session_id;
	process->session_owner = 1;
	linux_tty_bind_session(&console_tty, process);
	console_tty.foreground_pgid = process->pgid;
	return linux_user_session_set_foreground(session_id, process->pgid);
}

static void linux_process_init_links(struct linux_process *process)
{
	process->parent = 0;
	process->first_child = 0;
	process->next_sibling = 0;
}

static void linux_child_link(struct linux_process *parent,
			     struct linux_process *child)
{
	if (parent == 0 || child == 0) {
		return;
	}

	child->parent = parent;
	child->next_sibling = parent->first_child;
	parent->first_child = child;
	child->ppid = parent->pid;
}

static void linux_child_unlink(struct linux_process *child)
{
	struct linux_process *parent;
	struct linux_process **link;

	if (child == 0 || child->parent == 0) {
		return;
	}

	parent = child->parent;
	link = &parent->first_child;
	while (*link != 0) {
		if (*link == child) {
			*link = child->next_sibling;
			child->parent = 0;
			child->next_sibling = 0;
			return;
		}
		link = &(*link)->next_sibling;
	}
}

static long register_service(u64 service)
{
	(void)service;
	return bunix_names_register_claim(bunix_handle_find(BUNIX_CAP_CLAM),
					  BUNIX_HANDLE_SELF);
}

static int linux_tty_input_authorized(u64 sender)
{
	return sender != 0 && bunix_map_get(&tty_input_tasks, sender) != 0;
}

static long linux_grant_tty_input_task(u64 task)
{
	if (task == 0) {
		return -LINUX_EINVAL;
	}
	return bunix_map_set(&tty_input_tasks, task, 1) == 0 ? 0 :
	       -LINUX_ENOMEM;
}

static int recv_linux_message(struct bunix_msg *message, int *management)
{
	const u64 mgmt = LINUX_HANDLE_MGMT;
	u64 ports[2];
	u64 count = 0;
	u64 index = 0;
	u64 mgmt_index = (u64)-1;

	if (message == 0 || management == 0) {
		return -1;
	}
	if (mgmt != 0) {
		mgmt_index = count;
		ports[count++] = mgmt;
	}
	ports[count++] = BUNIX_HANDLE_SELF;
	if (bunix_ipc_recv_any(ports, count, message, &index) != 0) {
		return -1;
	}
	*management = (index == mgmt_index) ? 1 : 0;
	return 0;
}

static u64 resolve_service_type(u64 service, unsigned int rights,
				unsigned int type)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_NAMES,
		.type = type,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { BUNIX_NAMES_ROOT, service, rights, 0 },
	};
	struct bunix_msg reply;

	if (bunix_ipc_call(LINUX_HANDLE_NAMES, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return 0;
	}

	return reply.cap;
}

static u64 resolve_service(u64 service, unsigned int rights)
{
	return resolve_service_type(service, rights, BUNIX_NAMES_WAIT);
}

static void notify_proc_exit(u64 linux_pid, u64 status, u64 kill_task)
{
	const u64 proc = LINUX_HANDLE_PROC_MGMT;
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_PROC,
		.type = BUNIX_PROC_EXIT,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { linux_pid, status, 1, kill_task },
	};
	if (proc != 0) {
		(void)bunix_ipc_send(proc, &request);
	}
}

static long notify_proc_register_linux(u64 linux_pid, u64 bunix_task,
				       u64 parent_linux_pid,
				       u64 *bunix_proc_pid)
{
	const u64 proc = LINUX_HANDLE_PROC_MGMT;
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_PROC,
		.type = BUNIX_PROC_REGISTER_LINUX,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { linux_pid, bunix_task, parent_linux_pid, 0 },
	};
	struct bunix_msg reply;
	long call_result;

	if (proc == 0) {
		bunix_console_log("linux-server: fork proc missing\n",
				  sizeof("linux-server: fork proc missing\n") - 1);
		return -LINUX_ESRCH;
	}
	call_result = bunix_ipc_call(proc, &request, &reply);
	if (call_result != 0) {
		bunix_console_log("linux-server: fork proc call failed\n",
				  sizeof("linux-server: fork proc call failed\n") - 1);
		return -LINUX_ESRCH;
	}
	if (reply.words[0] != 0) {
		bunix_console_log("linux-server: fork proc denied\n",
				  sizeof("linux-server: fork proc denied\n") - 1);
		return -LINUX_ESRCH;
	}
	if (reply.words[1] == 0) {
		bunix_console_log("linux-server: fork proc pid zero\n",
				  sizeof("linux-server: fork proc pid zero\n") - 1);
		return -LINUX_ESRCH;
	}
	if (bunix_proc_pid != 0) {
		*bunix_proc_pid = reply.words[1];
	}
	return 0;
}

static long linux_proc_spawn_path(u64 proc, const char *path)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_PROC,
		.type = BUNIX_PROC_SPAWN_BUFFER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_RECV | BUNIX_RIGHT_DUP,
		.reply = 0,
		.cap = 0,
		.words = { 0, 1, 0, LINUX_PROC_SPAWN_LINUX },
	};
	u64 len;
	u64 total;
	long buffer;

	if (proc == 0 || path == 0 || path[0] != '/') {
		return -LINUX_ESRCH;
	}
	len = string_len(path) + 1;
	total = len * 2;
	buffer = bunix_buffer_create(total);
	if (buffer <= 0) {
		return -LINUX_EAGAIN;
	}
	request.cap = (u64)buffer;
	request.words[0] = total;
	if (bunix_buffer_write((u64)buffer, 0, path, len) != 0 ||
	    bunix_buffer_write((u64)buffer, len, path, len) != 0 ||
	    bunix_ipc_send(proc, &request) != 0) {
		bunix_handle_close((u64)buffer);
		return -LINUX_ESRCH;
	}
	bunix_handle_close((u64)buffer);
	return 0;
}

static long linux_exec_init(u64 path_len, u64 path_buffer)
{
	const u64 proc = LINUX_HANDLE_PROC_MGMT;
	char path[LINUX_MAX_PATH];
	long result;

	if (proc == 0) {
		return -LINUX_ESRCH;
	}
	result = linux_read_path_arg(path_buffer, path_len, path, sizeof(path));
	if (result != 0) {
		return result;
	}
	return linux_proc_spawn_path(proc, path);
}

static void zero_bytes(char *buffer, u64 len)
{
	for (u64 i = 0; i < len; i++) {
		buffer[i] = 0;
	}
}

static void copy_field(char *buffer, u64 offset, u64 field_size,
		       const char *text)
{
	u64 i = 0;

	while (i + 1 < field_size && text[i] != '\0') {
		buffer[offset + i] = text[i];
		i++;
	}
}

static void store_u32(char *buffer, u64 offset, unsigned int value)
{
	for (u64 i = 0; i < 4; i++) {
		buffer[offset + i] = (char)((value >> (i * 8)) & 0xff);
	}
}

static void store_u16(char *buffer, u64 offset, unsigned int value)
{
	for (u64 i = 0; i < 2; i++) {
		buffer[offset + i] = (char)((value >> (i * 8)) & 0xff);
	}
}

static unsigned int load_u16(const char *buffer, u64 offset)
{
	unsigned int value = 0;

	for (u64 i = 0; i < 2; i++) {
		value |= ((unsigned int)(unsigned char)buffer[offset + i]) <<
			 (i * 8);
	}

	return value;
}

static unsigned int load_u32(const char *buffer, u64 offset)
{
	unsigned int value = 0;

	for (u64 i = 0; i < 4; i++) {
		value |= ((unsigned int)(unsigned char)buffer[offset + i]) <<
			 (i * 8);
	}

	return value;
}

static void store_u64(char *buffer, u64 offset, u64 value)
{
	for (u64 i = 0; i < 8; i++) {
		buffer[offset + i] = (char)((value >> (i * 8)) & 0xff);
	}
}

static int string_equal(const char *left, const char *right)
{
	u64 i = 0;

	while (left[i] != '\0' && right[i] != '\0') {
		if (left[i] != right[i]) {
			return 0;
		}
		i++;
	}

	return left[i] == right[i];
}

static int is_utmps_socket_path(const char *path)
{
	return string_equal(path, "/run/utmps/.utmpd-socket");
}

static int linux_console_path(const char *path)
{
	return string_equal(path, "/dev/tty") ||
	       string_equal(path, "/dev/ttyS0") ||
	       string_equal(path, "/dev/console");
}

static long linux_utmpfs_getent(u64 index, char *record)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_UTMPFS,
		.type = BUNIX_UTMPFS_GETENT,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP,
		.reply = 0,
		.cap = 0,
		.words = { index, 0, 0, 0 },
	};
	struct bunix_msg reply;
	const u64 utmpfs = linux_utmpfs_service();
	const long tmp = bunix_buffer_create(LINUX_UTMP_RECORD_SIZE);

	if (record == 0 || utmpfs == 0 || tmp <= 0) {
		if (tmp > 0) {
			bunix_handle_close((u64)tmp);
		}
		return -LINUX_ENOENT;
	}

	request.cap = (u64)tmp;
	if (bunix_ipc_call(utmpfs, &request, &reply) != 0 ||
	    reply.words[0] != 0 ||
	    bunix_buffer_read((u64)tmp, 0, record,
			      LINUX_UTMP_RECORD_SIZE) != 0) {
		bunix_handle_close((u64)tmp);
		return -LINUX_ENOENT;
	}
	bunix_handle_close((u64)tmp);
	return 0;
}

static long utmps_recv_response(struct linux_fd *fd, u64 len, u64 buffer)
{
	u64 response_len = 1;

	if (fd == 0 || buffer == 0 || fd->handle != LINUX_SOCKET_UTMPD) {
		return -LINUX_ENOTSOCK;
	}
	if (len == 0) {
		return 0;
	}

	zero_bytes(write_buffer, sizeof(write_buffer));
	if (fd->size == LINUX_UTMPS_REWIND) {
		fd->offset = 0;
		write_buffer[0] = 0;
	} else if (fd->size == LINUX_UTMPS_GETENT) {
		if (linux_utmpfs_getent(fd->offset, write_buffer + 1) == 0) {
			write_buffer[0] = 0;
			fd->offset++;
			response_len = LINUX_UTMP_RECORD_SIZE + 1;
		} else {
			write_buffer[0] = LINUX_ENOENT;
		}
	} else {
		write_buffer[0] = LINUX_EINVAL;
	}

	fd->size = LINUX_UTMPS_NONE;
	if (response_len > len) {
		response_len = len;
	}
	if (bunix_buffer_write(buffer, 0, write_buffer, response_len) != 0) {
		return -LINUX_EFAULT;
	}
	return (long)response_len;
}

static u64 string_len(const char *text)
{
	u64 len = 0;

	while (len < LINUX_MAX_PATH && text[len] != '\0') {
		len++;
	}

	return len;
}

static int path_has_prefix(const char *path, const char *prefix)
{
	u64 i = 0;

	if (path == 0 || prefix == 0) {
		return 0;
	}
	while (prefix[i] != '\0') {
		if (path[i] != prefix[i]) {
			return 0;
		}
		i++;
	}
	return 1;
}

static int path_contains(const char *path, const char *needle)
{
	if (path == 0 || needle == 0 || needle[0] == '\0') {
		return 0;
	}
	for (u64 i = 0; path[i] != '\0' && i < LINUX_MAX_PATH; i++) {
		u64 j = 0;

		while (i + j < LINUX_MAX_PATH &&
		       path[i + j] != '\0' && needle[j] != '\0' &&
		       path[i + j] == needle[j]) {
			j++;
		}
		if (needle[j] == '\0') {
			return 1;
		}
	}
	return 0;
}

static int linux_parse_proc_self_fd(const char *path, u64 *fd_out)
{
	const char prefix[] = "/proc/self/fd/";
	u64 i = sizeof(prefix) - 1;
	u64 value = 0;

	if (path == 0 || fd_out == 0 || !path_has_prefix(path, prefix) ||
	    path[i] == '\0') {
		return -1;
	}
	while (path[i] != '\0') {
		const char c = path[i++];

		if (c < '0' || c > '9') {
			return -1;
		}
		if (value > (((u64)-1) / 10)) {
			return -1;
		}
		value = value * 10 + (u64)(c - '0');
	}
	*fd_out = value;
	return 0;
}

static int linux_fd_allows_zero_size_read(const struct linux_fd *fd)
{
	return fd != 0 && path_has_prefix(fd->path, "/proc/");
}

static void append_char(char *line, u64 line_size, u64 *cursor, char value)
{
	if (*cursor + 1 < line_size) {
		line[(*cursor)++] = value;
	}
}

static void append_text(char *line, u64 line_size, u64 *cursor,
			const char *text)
{
	for (u64 i = 0; text[i] != '\0'; i++) {
		append_char(line, line_size, cursor, text[i]);
	}
}

static void append_dec(char *line, u64 line_size, u64 *cursor, u64 value)
{
	char digits[20];
	u64 count = 0;

	if (value == 0) {
		append_char(line, line_size, cursor, '0');
		return;
	}
	while (value != 0 && count < sizeof(digits)) {
		digits[count++] = (char)('0' + (value % 10));
		value /= 10;
	}
	while (count != 0) {
		append_char(line, line_size, cursor, digits[--count]);
	}
}

static void append_hex64(char *line, u64 line_size, u64 *cursor, u64 value)
{
	static const char digits[] = "0123456789abcdef";

	append_text(line, line_size, cursor, "0x");
	for (int shift = 60; shift >= 0; shift -= 4) {
		append_char(line, line_size, cursor,
			    digits[(value >> (u64)shift) & 0xf]);
	}
}

static void append_long(char *line, u64 line_size, u64 *cursor, long value)
{
	if (value < 0) {
		append_char(line, line_size, cursor, '-');
		append_dec(line, line_size, cursor, (u64)(-value));
		return;
	}
	append_dec(line, line_size, cursor, (u64)value);
}

static int linux_debug_apk_enabled(void)
{
	static int enabled = -1;

	if (enabled < 0) {
		enabled = bunix_cmdline_has("debug-linux-apk") > 0;
	}
	return enabled;
}

static int linux_debug_apk_path_applies(const char *path)
{
	if (!linux_debug_apk_enabled() || path == 0) {
		return 0;
	}
	return path_contains(path, ".apk") ||
	       string_equal(path, "bin") ||
	       string_equal(path, "/bin") ||
	       path_contains(path, "busybox-extras") ||
	       path_contains(path, "busybox-paths") ||
	       path_contains(path, "/etc/apk") ||
	       path_contains(path, "etc/apk") ||
	       path_contains(path, "/lib/apk") ||
	       path_contains(path, "lib/apk") ||
	       path_contains(path, "installed.tmp") ||
	       path_contains(path, "world.tmp");
}

static void linux_debug_path_log(const struct linux_process *process,
				 const char *syscall, const char *path)
{
	char line[320];
	char task_token[40];
	u64 cursor = 0;
	u64 token_cursor = 0;

	const int all_paths = bunix_cmdline_has("debug-linux-paths") > 0;
	const int late_paths = bunix_cmdline_has("debug-linux-paths-late") > 0;
	const int apk_paths = linux_debug_apk_path_applies(path);
	int task_paths = 0;

	append_text(task_token, sizeof(task_token), &token_cursor,
		    "debug-linux-path-task");
	append_dec(task_token, sizeof(task_token), &token_cursor,
		   process != 0 ? process->bunix_task : 0);
	if (token_cursor < sizeof(task_token)) {
		task_token[token_cursor] = '\0';
		task_paths = bunix_cmdline_has(task_token) > 0;
	}

	if (!all_paths && !late_paths && !task_paths && !apk_paths) {
		return;
	}
	if (!apk_paths && !all_paths && !task_paths && process != 0 &&
	    process->bunix_task < 140) {
		return;
	}

	append_text(line, sizeof(line), &cursor, "linux-server: ");
	append_text(line, sizeof(line), &cursor, syscall);
	append_text(line, sizeof(line), &cursor, " pid=");
	append_dec(line, sizeof(line), &cursor,
		   process != 0 ? process->pid : 0);
	append_text(line, sizeof(line), &cursor, " task=");
	append_dec(line, sizeof(line), &cursor,
		   process != 0 ? process->bunix_task : 0);
	append_text(line, sizeof(line), &cursor, " path=");
	if (path != 0) {
		append_text(line, sizeof(line), &cursor, path);
	}
	append_char(line, sizeof(line), &cursor, '\n');
	(void)bunix_early_console_write(line, cursor);
}

static void linux_debug_path_result_log(const struct linux_process *process,
					const char *syscall, const char *path,
					long result)
{
	char line[352];
	u64 cursor = 0;

	if (!linux_debug_paths_enabled_for_task(process) &&
	    !linux_debug_apk_path_applies(path)) {
		return;
	}
	append_text(line, sizeof(line), &cursor, "linux-server: ");
	append_text(line, sizeof(line), &cursor, syscall);
	append_text(line, sizeof(line), &cursor, " result pid=");
	append_dec(line, sizeof(line), &cursor,
		   process != 0 ? process->pid : 0);
	append_text(line, sizeof(line), &cursor, " task=");
	append_dec(line, sizeof(line), &cursor,
		   process != 0 ? process->bunix_task : 0);
	append_text(line, sizeof(line), &cursor, " result=");
	append_long(line, sizeof(line), &cursor, result);
	append_text(line, sizeof(line), &cursor, " path=");
	if (path != 0) {
		append_text(line, sizeof(line), &cursor, path);
	}
	append_char(line, sizeof(line), &cursor, '\n');
	(void)bunix_early_console_write(line, cursor);
}

static void linux_debug_apk_openat_state_log(
	const struct linux_process *process, u64 dirfd, const char *phase,
	const char *path, const char *opened_path, long result, u64 flags)
{
	const struct linux_fd *base_fd = 0;
	char line[896];
	u64 cursor = 0;

	if (!linux_debug_apk_path_applies(path) &&
	    !linux_debug_apk_path_applies(opened_path)) {
		return;
	}
	if (process != 0 && dirfd < process->fd_capacity) {
		base_fd = &process->fds[dirfd];
	}
	append_text(line, sizeof(line), &cursor,
		    "linux-server: apk-openat-state ");
	append_text(line, sizeof(line), &cursor, phase);
	append_text(line, sizeof(line), &cursor, " pid=");
	append_dec(line, sizeof(line), &cursor,
		   process != 0 ? process->pid : 0);
	append_text(line, sizeof(line), &cursor, " task=");
	append_dec(line, sizeof(line), &cursor,
		   process != 0 ? process->bunix_task : 0);
	append_text(line, sizeof(line), &cursor, " dirfd=");
	append_dec(line, sizeof(line), &cursor, dirfd);
	append_text(line, sizeof(line), &cursor, " result=");
	append_long(line, sizeof(line), &cursor, result);
	append_text(line, sizeof(line), &cursor, " flags=");
	append_dec(line, sizeof(line), &cursor, flags);
	append_text(line, sizeof(line), &cursor, " base_kind=");
	append_dec(line, sizeof(line), &cursor,
		   base_fd != 0 ? base_fd->kind : 0);
	append_text(line, sizeof(line), &cursor, " base_handle=");
	append_dec(line, sizeof(line), &cursor,
		   base_fd != 0 ? base_fd->handle : 0);
	append_text(line, sizeof(line), &cursor, " base_flags=");
	append_dec(line, sizeof(line), &cursor,
		   base_fd != 0 ? base_fd->flags : 0);
	append_text(line, sizeof(line), &cursor, " base_status=");
	append_dec(line, sizeof(line), &cursor,
		   base_fd != 0 ? base_fd->status_flags : 0);
	append_text(line, sizeof(line), &cursor, " base_path=");
	if (base_fd != 0 && base_fd->path[0] != '\0') {
		append_text(line, sizeof(line), &cursor, base_fd->path);
	}
	append_text(line, sizeof(line), &cursor, " path=");
	if (path != 0) {
		append_text(line, sizeof(line), &cursor, path);
	}
	append_text(line, sizeof(line), &cursor, " opened=");
	if (opened_path != 0) {
		append_text(line, sizeof(line), &cursor, opened_path);
	}
	append_char(line, sizeof(line), &cursor, '\n');
	(void)bunix_early_console_write(line, cursor);
}

static void linux_debug_path_arg_offset_result_log(
	const struct linux_process *process, const char *syscall,
	u64 path_len, u64 path_buffer, u64 path_offset, long result);

static void linux_debug_path_arg_result_log(const struct linux_process *process,
					    const char *syscall,
					    u64 path_len, u64 path_buffer,
					    long result)
{
	linux_debug_path_arg_offset_result_log(process, syscall, path_len,
					       path_buffer, 0, result);
}

static void linux_debug_path_arg_offset_result_log(
	const struct linux_process *process, const char *syscall,
	u64 path_len, u64 path_buffer, u64 path_offset, long result)
{
	char path[LINUX_MAX_PATH];

	if (!linux_debug_paths_enabled_for_task(process) &&
	    !linux_debug_apk_enabled()) {
		return;
	}
	if (path_buffer == 0 ||
	    path_len == 0 || path_len > sizeof(path) ||
	    bunix_buffer_read(path_buffer, path_offset, path, path_len) != 0 ||
	    path[path_len - 1] != '\0') {
		path[0] = '\0';
	}
	linux_debug_path_result_log(process, syscall, path, result);
}

static int linux_debug_paths_enabled_for_task(const struct linux_process *process)
{
	char task_token[40];
	u64 token_cursor = 0;

	if (bunix_cmdline_has("debug-linux-paths") > 0) {
		return 1;
	}
	if (bunix_cmdline_has("debug-linux-paths-late") > 0 &&
	    process != 0 && process->bunix_task >= 140) {
		return 1;
	}
	append_text(task_token, sizeof(task_token), &token_cursor,
		    "debug-linux-path-task");
	append_dec(task_token, sizeof(task_token), &token_cursor,
		   process != 0 ? process->bunix_task : 0);
	if (token_cursor >= sizeof(task_token)) {
		return 0;
	}
	task_token[token_cursor] = '\0';
	return bunix_cmdline_has(task_token) > 0;
}

static void linux_debug_getdents_log(const struct linux_process *process,
				     u64 fd, const char *phase, u64 index,
				     long result)
{
	char line[384];
	u64 cursor = 0;
	const struct linux_fd *linux_fd = 0;

	if (!linux_debug_paths_enabled_for_task(process)) {
		return;
	}
	if (process != 0 && fd < process->fd_capacity) {
		linux_fd = &process->fds[fd];
	}
	append_text(line, sizeof(line), &cursor, "linux-server: getdents ");
	append_text(line, sizeof(line), &cursor, phase);
	append_text(line, sizeof(line), &cursor, " pid=");
	append_dec(line, sizeof(line), &cursor, process != 0 ? process->pid : 0);
	append_text(line, sizeof(line), &cursor, " task=");
	append_dec(line, sizeof(line), &cursor,
		   process != 0 ? process->bunix_task : 0);
	append_text(line, sizeof(line), &cursor, " fd=");
	append_dec(line, sizeof(line), &cursor, fd);
	append_text(line, sizeof(line), &cursor, " kind=");
	append_dec(line, sizeof(line), &cursor, linux_fd != 0 ? linux_fd->kind : 0);
	append_text(line, sizeof(line), &cursor, " handle=");
	append_dec(line, sizeof(line), &cursor,
		   linux_fd != 0 ? linux_fd->handle : 0);
	append_text(line, sizeof(line), &cursor, " offset=");
	append_dec(line, sizeof(line), &cursor,
		   linux_fd != 0 ? linux_fd_offset(linux_fd) : 0);
	append_text(line, sizeof(line), &cursor, " index=");
	append_dec(line, sizeof(line), &cursor, index);
	append_text(line, sizeof(line), &cursor, " result=");
	append_long(line, sizeof(line), &cursor, result);
	append_text(line, sizeof(line), &cursor, " path=");
	if (linux_fd != 0 && linux_fd->path[0] != '\0') {
		append_text(line, sizeof(line), &cursor, linux_fd->path);
	}
	append_char(line, sizeof(line), &cursor, '\n');
	(void)bunix_early_console_write(line, cursor);
}

static void linux_debug_rpc_log(const char *line, u64 len)
{
	if (bunix_cmdline_has("debug-linux-rpc") <= 0) {
		return;
	}
	bunix_console_log(line, len);
}

static void linux_debug_wait_log(const struct linux_process *parent,
				 const struct linux_process *child,
				 const char *action, long pid, long result)
{
	char line[192];
	u64 cursor = 0;

	if (bunix_cmdline_has("debug-linux-wait") <= 0) {
		return;
	}

	append_text(line, sizeof(line), &cursor, "linux-wait: ");
	append_text(line, sizeof(line), &cursor, action);
	append_text(line, sizeof(line), &cursor, " parent=");
	append_dec(line, sizeof(line), &cursor, parent != 0 ? parent->pid : 0);
	append_text(line, sizeof(line), &cursor, " task=");
	append_dec(line, sizeof(line), &cursor,
		   parent != 0 ? parent->bunix_task : 0);
	append_text(line, sizeof(line), &cursor, " waitpid=");
	append_long(line, sizeof(line), &cursor, pid);
	append_text(line, sizeof(line), &cursor, " child=");
	append_dec(line, sizeof(line), &cursor, child != 0 ? child->pid : 0);
	append_text(line, sizeof(line), &cursor, " child_task=");
	append_dec(line, sizeof(line), &cursor,
		   child != 0 ? child->bunix_task : 0);
	append_text(line, sizeof(line), &cursor, " status=");
	append_dec(line, sizeof(line), &cursor,
		   child != 0 ? child->exit_status : 0);
	append_text(line, sizeof(line), &cursor, " result=");
	append_long(line, sizeof(line), &cursor, result);
	append_char(line, sizeof(line), &cursor, '\n');
	bunix_console_log(line, cursor);
}

static int linux_debug_pipe_enabled(void)
{
	static int enabled = -1;

	if (enabled < 0) {
		enabled = bunix_cmdline_has("debug-linux-pipe") > 0;
	}
	return enabled;
}

static void linux_debug_pipe_log(const char *action,
				 const struct linux_process *process,
				 u64 fd, const struct linux_pipe *pipe,
				 long result)
{
	char line[240];
	u64 cursor = 0;

	if (!linux_debug_pipe_enabled()) {
		return;
	}

	append_text(line, sizeof(line), &cursor, "linux-pipe: ");
	append_text(line, sizeof(line), &cursor, action);
	append_text(line, sizeof(line), &cursor, " pid=");
	append_dec(line, sizeof(line), &cursor, process != 0 ? process->pid : 0);
	append_text(line, sizeof(line), &cursor, " task=");
	append_dec(line, sizeof(line), &cursor,
		   process != 0 ? process->bunix_task : 0);
	append_text(line, sizeof(line), &cursor, " fd=");
	append_dec(line, sizeof(line), &cursor, fd);
	append_text(line, sizeof(line), &cursor, " pipe=");
	append_dec(line, sizeof(line), &cursor, pipe != 0 ? pipe->id : 0);
	append_text(line, sizeof(line), &cursor, " rrefs=");
	append_dec(line, sizeof(line), &cursor, pipe != 0 ? pipe->read_refs : 0);
	append_text(line, sizeof(line), &cursor, " wrefs=");
	append_dec(line, sizeof(line), &cursor, pipe != 0 ? pipe->write_refs : 0);
	append_text(line, sizeof(line), &cursor, " len=");
	append_dec(line, sizeof(line), &cursor, pipe != 0 ? pipe->len : 0);
	append_text(line, sizeof(line), &cursor, " rr=");
	append_dec(line, sizeof(line), &cursor,
		   pipe != 0 ? pipe->reader_reply : 0);
	append_text(line, sizeof(line), &cursor, " wr=");
	append_dec(line, sizeof(line), &cursor,
		   pipe != 0 ? pipe->writer_reply : 0);
	append_text(line, sizeof(line), &cursor, " result=");
	append_long(line, sizeof(line), &cursor, result);
	append_char(line, sizeof(line), &cursor, '\n');
	bunix_console_log(line, cursor);
}

static int linux_debug_netlink_enabled(void)
{
	static int enabled = -1;

	if (enabled < 0) {
		enabled = bunix_cmdline_has("debug-linux-netlink") > 0;
	}
	return enabled;
}

static void linux_debug_netlink_log(const char *action,
				    const struct linux_process *process,
				    u64 fd, u64 type, u64 flags, u64 seq,
				    u64 len, long result, u64 queued)
{
	char line[220];
	u64 cursor = 0;

	if (!linux_debug_netlink_enabled()) {
		return;
	}
	append_text(line, sizeof(line), &cursor, "linux-netlink: ");
	append_text(line, sizeof(line), &cursor, action);
	append_text(line, sizeof(line), &cursor, " pid=");
	append_dec(line, sizeof(line), &cursor, process != 0 ? process->pid : 0);
	append_text(line, sizeof(line), &cursor, " task=");
	append_dec(line, sizeof(line), &cursor,
		   process != 0 ? process->bunix_task : 0);
	append_text(line, sizeof(line), &cursor, " fd=");
	append_dec(line, sizeof(line), &cursor, fd);
	append_text(line, sizeof(line), &cursor, " type=");
	append_dec(line, sizeof(line), &cursor, type);
	append_text(line, sizeof(line), &cursor, " flags=");
	append_dec(line, sizeof(line), &cursor, flags);
	append_text(line, sizeof(line), &cursor, " seq=");
	append_dec(line, sizeof(line), &cursor, seq);
	append_text(line, sizeof(line), &cursor, " len=");
	append_dec(line, sizeof(line), &cursor, len);
	append_text(line, sizeof(line), &cursor, " result=");
	append_long(line, sizeof(line), &cursor, result);
	append_text(line, sizeof(line), &cursor, " queued=");
	append_dec(line, sizeof(line), &cursor, queued);
	append_char(line, sizeof(line), &cursor, '\n');
	bunix_console_log(line, cursor);
}

#define LINUX_DEBUG_COUNT_MAX 1024
#define LINUX_DEBUG_COUNT_PERIOD 100
#define LINUX_DEBUG_COUNT_TOP 12

static u64 linux_debug_message_counts[LINUX_DEBUG_COUNT_MAX];
static u64 linux_debug_message_total;
static u64 linux_debug_message_next = LINUX_DEBUG_COUNT_PERIOD;

static const char *linux_debug_message_name(u64 type)
{
	switch (type) {
	case BUNIX_LINUX_READ: return "read";
	case BUNIX_LINUX_WRITE: return "write";
	case BUNIX_LINUX_OPEN: return "open";
	case BUNIX_LINUX_CLOSE: return "close";
	case BUNIX_LINUX_FSTAT: return "fstat";
	case BUNIX_LINUX_POLL: return "poll";
	case BUNIX_LINUX_LSEEK: return "lseek";
	case BUNIX_LINUX_MMAP: return "mmap";
	case BUNIX_LINUX_RT_SIGACTION: return "rt_sigaction";
	case BUNIX_LINUX_RT_SIGPROCMASK: return "rt_sigprocmask";
	case BUNIX_LINUX_IOCTL: return "ioctl";
	case BUNIX_LINUX_PIPE: return "pipe";
	case BUNIX_LINUX_DUP: return "dup";
	case BUNIX_LINUX_DUP2: return "dup2";
	case BUNIX_LINUX_ALARM: return "alarm";
	case BUNIX_LINUX_SETITIMER: return "setitimer";
	case BUNIX_LINUX_SENDFILE: return "sendfile";
	case BUNIX_LINUX_SOCKET: return "socket";
	case BUNIX_LINUX_CONNECT: return "connect";
	case BUNIX_LINUX_SENDTO: return "sendto";
	case BUNIX_LINUX_RECVFROM: return "recvfrom";
	case BUNIX_LINUX_RECVMSG: return "recvmsg";
	case BUNIX_LINUX_BIND: return "bind";
	case BUNIX_LINUX_GETSOCKNAME: return "getsockname";
	case BUNIX_LINUX_GETPEERNAME: return "getpeername";
	case BUNIX_LINUX_SETSOCKOPT: return "setsockopt";
	case BUNIX_LINUX_GETSOCKOPT: return "getsockopt";
	case BUNIX_LINUX_WAIT4: return "wait4";
	case BUNIX_LINUX_UNAME: return "uname";
	case BUNIX_LINUX_SETHOSTNAME: return "sethostname";
	case BUNIX_LINUX_GETCWD: return "getcwd";
	case BUNIX_LINUX_CHDIR: return "chdir";
	case BUNIX_LINUX_FCHDIR: return "fchdir";
	case BUNIX_LINUX_MKDIR: return "mkdir";
	case BUNIX_LINUX_RMDIR: return "rmdir";
	case BUNIX_LINUX_RENAME: return "rename";
	case BUNIX_LINUX_UNLINK: return "unlink";
	case BUNIX_LINUX_ACCESS: return "access";
	case BUNIX_LINUX_READLINK: return "readlink";
	case BUNIX_LINUX_GETPID: return "getpid";
	case BUNIX_LINUX_GETPPID: return "getppid";
	case BUNIX_LINUX_GETGROUPS: return "getgroups";
	case BUNIX_LINUX_STATFS: return "statfs";
	case BUNIX_LINUX_FSTATFS: return "fstatfs";
	case BUNIX_LINUX_FCNTL: return "fcntl";
	case BUNIX_LINUX_GETDENTS64: return "getdents64";
	case BUNIX_LINUX_CLOCK_GETTIME: return "clock_gettime";
	case BUNIX_LINUX_OPENAT: return "openat";
	case BUNIX_LINUX_NEWFSTATAT: return "newfstatat";
	case BUNIX_LINUX_FACCESSAT: return "faccessat";
	case BUNIX_LINUX_FACCESSAT2: return "faccessat2";
	case BUNIX_LINUX_READLINKAT: return "readlinkat";
	case BUNIX_LINUX_UTIMENSAT: return "utimensat";
	case BUNIX_LINUX_PPOLL: return "ppoll";
	case BUNIX_LINUX_DUP3: return "dup3";
	case BUNIX_LINUX_PIPE2: return "pipe2";
	case BUNIX_LINUX_STATX: return "statx";
	case BUNIX_LINUX_GETRANDOM: return "getrandom";
	case BUNIX_LINUX_CLOSE_RANGE: return "close_range";
	case BUNIX_LINUX_EXIT_GROUP: return "exit_group";
	case BUNIX_LINUX_REGISTER_PROCESS: return "register_process";
	case BUNIX_LINUX_FORK_PROCESS: return "fork_process";
	case BUNIX_LINUX_EXEC_PROCESS: return "exec_process";
	case BUNIX_LINUX_EXEC_INIT: return "exec_init";
	case BUNIX_LINUX_EXEC_PREPARE: return "exec_prepare";
	case BUNIX_LINUX_ATTACH_SESSION: return "attach_session";
	case BUNIX_LINUX_SIGNAL_PENDING: return "signal_pending";
	case BUNIX_LINUX_SIGNAL_DEQUEUE: return "signal_dequeue";
	case BUNIX_LINUX_TASK_FAULT: return "task_fault";
	case BUNIX_LINUX_TTY_INPUT_BUFFER: return "tty_input_buffer";
	case BUNIX_LINUX_GRANT_TTY_INPUT_TASK: return "grant_tty_input_task";
	case BUNIX_LINUX_RESOLVE_FD_PATH: return "resolve_fd_path";
	default: return 0;
	}
}

static void linux_debug_append_message_name(char *line, u64 line_size,
					    u64 *cursor, u64 type)
{
	const char *name = linux_debug_message_name(type);

	if (name != 0) {
		append_text(line, line_size, cursor, name);
		return;
	}
	append_text(line, line_size, cursor, "type");
	append_dec(line, line_size, cursor, type);
}

static void linux_debug_count_message(u64 type)
{
	static int enabled = -1;
	u64 used[LINUX_DEBUG_COUNT_TOP];
	char line[480];
	u64 cursor = 0;

	if (enabled < 0) {
		enabled = bunix_cmdline_has("debug-linux-counts") > 0;
	}
	if (!enabled) {
		return;
	}

	linux_debug_message_total++;
	if (type < LINUX_DEBUG_COUNT_MAX) {
		linux_debug_message_counts[type]++;
	}
	if (linux_debug_message_total < linux_debug_message_next) {
		return;
	}
	linux_debug_message_next += LINUX_DEBUG_COUNT_PERIOD;

	for (u64 i = 0; i < LINUX_DEBUG_COUNT_TOP; i++) {
		used[i] = (u64)-1;
	}

	append_text(line, sizeof(line), &cursor, "linux-counts: total=");
	append_dec(line, sizeof(line), &cursor, linux_debug_message_total);
	for (u64 rank = 0; rank < LINUX_DEBUG_COUNT_TOP; rank++) {
		u64 best_type = (u64)-1;
		u64 best_count = 0;

		for (u64 i = 0; i < LINUX_DEBUG_COUNT_MAX; i++) {
			int already_used = 0;

			for (u64 j = 0; j < rank; j++) {
				if (used[j] == i) {
					already_used = 1;
					break;
				}
			}
			if (!already_used &&
			    linux_debug_message_counts[i] > best_count) {
				best_type = i;
				best_count = linux_debug_message_counts[i];
			}
		}
		if (best_count == 0) {
			break;
		}
		used[rank] = best_type;
		append_char(line, sizeof(line), &cursor, ' ');
		linux_debug_append_message_name(line, sizeof(line), &cursor,
						best_type);
		append_char(line, sizeof(line), &cursor, '=');
		append_dec(line, sizeof(line), &cursor, best_count);
	}
	append_char(line, sizeof(line), &cursor, '\n');
	bunix_console_log(line, cursor);
}

static void linux_debug_read_kind_log(const struct linux_process *process,
				      u64 fd, u64 len)
{
	enum {
		READ_KIND_CONSOLE,
		READ_KIND_FILE,
		READ_KIND_DIR,
		READ_KIND_SOCKET,
		READ_KIND_PIPE,
		READ_KIND_OTHER,
		READ_KIND_COUNT,
	};
	static int enabled = -1;
	static u64 total;
	static u64 kind_counts[READ_KIND_COUNT];
	static u64 len1_counts[READ_KIND_COUNT];
	u64 kind;
	char line[520];
	u64 cursor = 0;

	if (enabled < 0) {
		enabled = bunix_cmdline_has("debug-linux-syscall-counts") > 0;
	}
	if (!enabled || process == 0 || fd >= process->fd_capacity ||
	    process->fds[fd].kind == 0) {
		return;
	}

	switch (process->fds[fd].kind) {
	case LINUX_FD_CONSOLE:
		kind = READ_KIND_CONSOLE;
		break;
	case LINUX_FD_FILE:
		kind = READ_KIND_FILE;
		break;
	case LINUX_FD_DIR:
		kind = READ_KIND_DIR;
		break;
	case LINUX_FD_SOCKET:
		kind = READ_KIND_SOCKET;
		break;
	case LINUX_FD_PIPE_READ:
	case LINUX_FD_PIPE_WRITE:
		kind = READ_KIND_PIPE;
		break;
	default:
		kind = READ_KIND_OTHER;
		break;
	}

	total++;
	kind_counts[kind]++;
	if (len == 1) {
		len1_counts[kind]++;
	}
	if ((total & 255) != 0) {
		return;
	}

	append_text(line, sizeof(line), &cursor, "linux-server: read-kinds total=");
	append_dec(line, sizeof(line), &cursor, total);
	append_text(line, sizeof(line), &cursor, " console=");
	append_dec(line, sizeof(line), &cursor, kind_counts[READ_KIND_CONSOLE]);
	append_text(line, sizeof(line), &cursor, " file=");
	append_dec(line, sizeof(line), &cursor, kind_counts[READ_KIND_FILE]);
	append_text(line, sizeof(line), &cursor, " dir=");
	append_dec(line, sizeof(line), &cursor, kind_counts[READ_KIND_DIR]);
	append_text(line, sizeof(line), &cursor, " socket=");
	append_dec(line, sizeof(line), &cursor, kind_counts[READ_KIND_SOCKET]);
	append_text(line, sizeof(line), &cursor, " pipe=");
	append_dec(line, sizeof(line), &cursor, kind_counts[READ_KIND_PIPE]);
	append_text(line, sizeof(line), &cursor, " other=");
	append_dec(line, sizeof(line), &cursor, kind_counts[READ_KIND_OTHER]);
	append_text(line, sizeof(line), &cursor, " len1_console=");
	append_dec(line, sizeof(line), &cursor, len1_counts[READ_KIND_CONSOLE]);
	append_text(line, sizeof(line), &cursor, " len1_file=");
	append_dec(line, sizeof(line), &cursor, len1_counts[READ_KIND_FILE]);
	append_text(line, sizeof(line), &cursor, " len1_socket=");
	append_dec(line, sizeof(line), &cursor, len1_counts[READ_KIND_SOCKET]);
	append_text(line, sizeof(line), &cursor, " len1_pipe=");
	append_dec(line, sizeof(line), &cursor, len1_counts[READ_KIND_PIPE]);
	append_text(line, sizeof(line), &cursor, " last_fd=");
	append_dec(line, sizeof(line), &cursor, fd);
	append_text(line, sizeof(line), &cursor, " last_kind=");
	append_dec(line, sizeof(line), &cursor, process->fds[fd].kind);
	append_text(line, sizeof(line), &cursor, " last_len=");
	append_dec(line, sizeof(line), &cursor, len);
	append_char(line, sizeof(line), &cursor, '\n');
	bunix_console_log(line, cursor);
}

static void string_copy(char *dst, const char *src)
{
	u64 i = 0;

	while (i + 1 < LINUX_MAX_PATH && src[i] != '\0') {
		dst[i] = src[i];
		i++;
	}
	dst[i] = '\0';
}

static void string_copy_n(char *dst, u64 dst_size, const char *src)
{
	u64 i = 0;

	if (dst == 0 || dst_size == 0) {
		return;
	}
	while (i + 1 < dst_size && src != 0 && src[i] != '\0') {
		dst[i] = src[i];
		i++;
	}
	dst[i] = '\0';
}

static int path_eq(const char *left, const char *right)
{
	u64 i = 0;

	if (left == 0 || right == 0) {
		return left == right;
	}
	while (left[i] != '\0' && right[i] != '\0') {
		if (left[i] != right[i]) {
			return 0;
		}
		i++;
	}
	return left[i] == right[i];
}

static char *path_dup(const char *src)
{
	u64 len;
	char *dst;

	if (src == 0) {
		return 0;
	}
	len = string_len(src);
	if (len >= LINUX_MAX_PATH) {
		return 0;
	}
	dst = (char *)bunix_alloc(len + 1);
	if (dst == 0) {
		return 0;
	}
	for (u64 i = 0; i <= len; i++) {
		dst[i] = src[i];
	}
	return dst;
}

#include "exec_abi.c"

static long linux_exec_prepare(u64 path_len, u64 image_len, u64 vector_len,
			       u64 buffer, u64 *reply_path_len,
			       u64 *reply_vector_len)
{
	char path[LINUX_MAX_PATH];
	unsigned char image[LINUX_SHEBANG_MAX];
	struct linux_exec_args args = { 0, 0, 0, 0, 0 };
	char *interp = 0;
	char *interp_arg = 0;
	long result;

	if (reply_path_len == 0 || reply_vector_len == 0 ||
	    buffer == 0 || path_len == 0 || path_len > sizeof(path) ||
	    image_len == 0 || image_len > sizeof(image) ||
	    vector_len == 0 ||
	    vector_len > LINUX_EXEC_PREPARE_BUFFER_SIZE -
			 LINUX_EXEC_PREPARE_VECTOR_OFF ||
	    bunix_buffer_read(buffer, LINUX_EXEC_PREPARE_PATH_OFF,
			      path, path_len) != 0 ||
	    path[path_len - 1] != '\0' ||
	    path[0] == '\0' ||
	    bunix_buffer_read(buffer, LINUX_EXEC_PREPARE_IMAGE_OFF,
			      image, image_len) != 0) {
		return -LINUX_EINVAL;
	}
	result = linux_exec_deserialize_args(buffer,
					     LINUX_EXEC_PREPARE_VECTOR_OFF,
					     vector_len, &args);
	if (result != 0) {
		return result;
	}
	result = linux_exec_parse_shebang(image, image_len, &interp,
					  &interp_arg);
	if (result == 0) {
		result = linux_exec_rewrite_shebang_args(&args, path, interp,
							 interp_arg);
	}
	if (result == 0) {
		const u64 new_path_len = string_len(args.argv[0]) + 1;
		u64 new_vector_len = 0;

		if (new_path_len == 1 || new_path_len > LINUX_MAX_PATH ||
		    bunix_buffer_write(buffer, LINUX_EXEC_PREPARE_PATH_OFF,
				       args.argv[0], new_path_len) != 0) {
			result = -LINUX_EINVAL;
		} else {
			result = linux_exec_serialize_args(
				buffer, LINUX_EXEC_PREPARE_VECTOR_OFF, &args,
				&new_vector_len);
			if (result == 0) {
				*reply_path_len = new_path_len;
				*reply_vector_len = new_vector_len;
			}
		}
	}
	if (result != 0) {
		bunix_free(interp);
		bunix_free(interp_arg);
	}
	linux_exec_args_free(&args);
	return result;
}

static long linux_resolve_fd_path(struct linux_process *process, u64 path_len,
				  u64 out_cap, u64 buffer,
				  u64 *reply_path_len)
{
	char path[LINUX_MAX_PATH];
	u64 fd;
	u64 resolved_len;

	if (process == 0 || reply_path_len == 0 || buffer == 0 ||
	    path_len == 0 || path_len > sizeof(path) ||
	    out_cap == 0 || out_cap > LINUX_MAX_PATH ||
	    bunix_buffer_read(buffer, 0, path, path_len) != 0 ||
	    path[path_len - 1] != '\0') {
		return -LINUX_EINVAL;
	}
	if (linux_parse_proc_self_fd(path, &fd) != 0) {
		return -LINUX_EINVAL;
	}
	if (fd >= process->fd_capacity || process->fds[fd].kind == 0) {
		return -LINUX_EBADF;
	}
	if (process->fds[fd].kind != LINUX_FD_FILE &&
	    process->fds[fd].kind != LINUX_FD_DIR) {
		return -LINUX_EBADF;
	}
	if (process->fds[fd].path[0] == '\0') {
		return -LINUX_ENOENT;
	}

	resolved_len = string_len(process->fds[fd].path) + 1;
	if (resolved_len > out_cap) {
		return -LINUX_ENAMETOOLONG;
	}
	if (bunix_buffer_write(buffer, 0, process->fds[fd].path,
			       resolved_len) != 0) {
		return -LINUX_EFAULT;
	}
	*reply_path_len = resolved_len;
	return 0;
}

static int linux_process_set_cwd(struct linux_process *process,
				 const char *cwd)
{
	char *copy;

	if (process == 0 || cwd == 0 || cwd[0] != '/') {
		return -1;
	}
	copy = path_dup(cwd);
	if (copy == 0) {
		return -1;
	}
	bunix_free(process->cwd);
	process->cwd = copy;
	linux_access_cache_clear(process);
	return 0;
}

static void linux_access_cache_clear(struct linux_process *process)
{
	if (process == 0) {
		return;
	}
	for (u64 i = 0; i < LINUX_ACCESS_CACHE_SIZE; i++) {
		process->access_cache[i].valid = 0;
		process->access_cache[i].stat_valid = 0;
	}
	process->access_cache_epoch = 0;
	process->access_cache_next = 0;
}

static void linux_access_cache_sync(struct linux_process *process)
{
	(void)process;
}

static int linux_path_prefix(const char *path, const char *prefix)
{
	u64 i = 0;

	if (path == 0 || prefix == 0) {
		return 0;
	}
	while (prefix[i] != '\0') {
		if (path[i] != prefix[i]) {
			return 0;
		}
		i++;
	}
	return path[i] == '\0' || path[i] == '/';
}

static int linux_path_final_dot_component(const char *path)
{
	u64 end;
	u64 start;

	if (path == 0 || path[0] == '\0') {
		return 0;
	}
	end = string_len(path);
	while (end > 0 && path[end - 1] == '/') {
		end--;
	}
	if (end == 0) {
		return 0;
	}
	start = end;
	while (start > 0 && path[start - 1] != '/') {
		start--;
	}
	if (end - start == 1 && path[start] == '.') {
		return 1;
	}
	return end - start == 2 && path[start] == '.' && path[start + 1] == '.';
}

static int linux_debug_openrc_state_enabled(void)
{
	static int enabled = -1;

	if (enabled < 0) {
		enabled = bunix_cmdline_has("debug-linux-openrc-state") > 0;
	}
	return enabled;
}

static int linux_debug_openrc_state_applies(const struct linux_process *process,
					    u64 dirfd, const char *path,
					    const char *opened_path)
{
	if (!linux_debug_openrc_state_enabled()) {
		return 0;
	}
	if (linux_path_prefix(path, "/run/openrc") ||
	    linux_path_prefix(opened_path, "/run/openrc")) {
		return 1;
	}
	if (process != 0 && dirfd < process->fd_capacity &&
	    linux_path_prefix(process->fds[dirfd].path, "/run/openrc")) {
		return 1;
	}
	return 0;
}

static void linux_debug_openrc_state_log_fd(const struct linux_process *process,
					    const struct linux_fd *fd,
					    u64 fd_number,
					    const char *reason,
					    long result, u64 arg0, u64 arg1)
{
	char line[640];
	u64 cursor = 0;

	if (!linux_debug_openrc_state_enabled()) {
		return;
	}
	append_text(line, sizeof(line), &cursor, "linux-server: openrc-state ");
	append_text(line, sizeof(line), &cursor, reason);
	append_text(line, sizeof(line), &cursor, " pid=");
	append_dec(line, sizeof(line), &cursor, process != 0 ? process->pid : 0);
	append_text(line, sizeof(line), &cursor, " task=");
	append_dec(line, sizeof(line), &cursor,
		   process != 0 ? process->bunix_task : 0);
	append_text(line, sizeof(line), &cursor, " fd=");
	append_dec(line, sizeof(line), &cursor, fd_number);
	append_text(line, sizeof(line), &cursor, " result=");
	append_long(line, sizeof(line), &cursor, result);
	append_text(line, sizeof(line), &cursor, " arg0=");
	append_dec(line, sizeof(line), &cursor, arg0);
	append_text(line, sizeof(line), &cursor, " arg1=");
	append_dec(line, sizeof(line), &cursor, arg1);
	append_text(line, sizeof(line), &cursor, " kind=");
	append_dec(line, sizeof(line), &cursor, fd != 0 ? fd->kind : 0);
	append_text(line, sizeof(line), &cursor, " handle=");
	append_dec(line, sizeof(line), &cursor, fd != 0 ? fd->handle : 0);
	append_text(line, sizeof(line), &cursor, " flags=");
	append_dec(line, sizeof(line), &cursor, fd != 0 ? fd->flags : 0);
	append_text(line, sizeof(line), &cursor, " status=");
	append_dec(line, sizeof(line), &cursor, fd != 0 ? fd->status_flags : 0);
	append_text(line, sizeof(line), &cursor, " offset=");
	append_dec(line, sizeof(line), &cursor, fd != 0 ? linux_fd_offset(fd) : 0);
	append_text(line, sizeof(line), &cursor, " size=");
	append_dec(line, sizeof(line), &cursor, fd != 0 ? linux_fd_size(fd) : 0);
	append_text(line, sizeof(line), &cursor, " refs=");
	append_dec(line, sizeof(line), &cursor,
		   fd != 0 && fd->open_file != 0 ? fd->open_file->refs : 0);
	append_text(line, sizeof(line), &cursor, " path=");
	if (fd != 0 && fd->path[0] != '\0') {
		append_text(line, sizeof(line), &cursor, fd->path);
	}
	append_char(line, sizeof(line), &cursor, '\n');
	(void)bunix_early_console_write(line, cursor);
}

static void linux_debug_openrc_state_log_path(const struct linux_process *process,
					      u64 dirfd, const char *phase,
					      const char *path,
					      const char *opened_path,
					      long result, u64 flags)
{
	const struct linux_fd *base_fd = 0;
	char line[720];
	u64 cursor = 0;

	if (!linux_debug_openrc_state_applies(process, dirfd, path, opened_path)) {
		return;
	}
	if (process != 0 && dirfd < process->fd_capacity) {
		base_fd = &process->fds[dirfd];
	}
	append_text(line, sizeof(line), &cursor, "linux-server: openrc-state ");
	append_text(line, sizeof(line), &cursor, phase);
	append_text(line, sizeof(line), &cursor, " pid=");
	append_dec(line, sizeof(line), &cursor, process != 0 ? process->pid : 0);
	append_text(line, sizeof(line), &cursor, " task=");
	append_dec(line, sizeof(line), &cursor,
		   process != 0 ? process->bunix_task : 0);
	append_text(line, sizeof(line), &cursor, " dirfd=");
	append_dec(line, sizeof(line), &cursor, dirfd);
	append_text(line, sizeof(line), &cursor, " result=");
	append_long(line, sizeof(line), &cursor, result);
	append_text(line, sizeof(line), &cursor, " flags=");
	append_dec(line, sizeof(line), &cursor, flags);
	append_text(line, sizeof(line), &cursor, " base_kind=");
	append_dec(line, sizeof(line), &cursor, base_fd != 0 ? base_fd->kind : 0);
	append_text(line, sizeof(line), &cursor, " base_handle=");
	append_dec(line, sizeof(line), &cursor,
		   base_fd != 0 ? base_fd->handle : 0);
	append_text(line, sizeof(line), &cursor, " base_path=");
	if (base_fd != 0 && base_fd->path[0] != '\0') {
		append_text(line, sizeof(line), &cursor, base_fd->path);
	}
	append_text(line, sizeof(line), &cursor, " path=");
	if (path != 0) {
		append_text(line, sizeof(line), &cursor, path);
	}
	append_text(line, sizeof(line), &cursor, " opened=");
	if (opened_path != 0) {
		append_text(line, sizeof(line), &cursor, opened_path);
	}
	append_char(line, sizeof(line), &cursor, '\n');
	bunix_console_log(line, cursor);
}

static u64 linux_cache_domain_for_path(const struct linux_process *process,
				       const char *path)
{
	if (path == 0) {
		return LINUX_CACHE_DOMAIN_OTHER;
	}
	if (path[0] == '/') {
		if (linux_path_prefix(path, "/etc")) {
			return LINUX_CACHE_DOMAIN_ETC;
		}
		if (linux_path_prefix(path, "/run")) {
			return LINUX_CACHE_DOMAIN_RUN;
		}
		return LINUX_CACHE_DOMAIN_OTHER;
	}
	if (process != 0 && linux_path_prefix(process->cwd, "/etc")) {
		return LINUX_CACHE_DOMAIN_ETC;
	}
	if (process != 0 && linux_path_prefix(process->cwd, "/run")) {
		return LINUX_CACHE_DOMAIN_RUN;
	}
	return LINUX_CACHE_DOMAIN_OTHER;
}

static void linux_vfs_note_mutation(const char *path)
{
	u64 domain;

	if (path == 0) {
		for (domain = 0; domain < LINUX_CACHE_DOMAIN_COUNT; domain++) {
			linux_vfs_mutation_epochs[domain]++;
			if (linux_vfs_mutation_epochs[domain] == 0) {
				linux_vfs_mutation_epochs[domain] = 1;
			}
		}
		return;
	}
	domain = linux_cache_domain_for_path(0, path);
	linux_vfs_mutation_epochs[domain]++;
	if (linux_vfs_mutation_epochs[domain] == 0) {
		linux_vfs_mutation_epochs[domain] = 1;
	}
}

static int linux_global_path_cache_allowed(const char *cache_path)
{
	const u64 domain = linux_cache_domain_for_path(0, cache_path);

	return domain == LINUX_CACHE_DOMAIN_ETC ||
	       domain == LINUX_CACHE_DOMAIN_RUN;
}

static int linux_file_read_cache_allowed(const char *cache_path)
{
	if (cache_path == 0 || cache_path[0] != '/') {
		return 0;
	}
	return linux_path_prefix(cache_path, "/etc") ||
	       linux_path_prefix(cache_path, "/usr") ||
	       linux_path_prefix(cache_path, "/bin") ||
	       linux_path_prefix(cache_path, "/sbin") ||
	       linux_path_prefix(cache_path, "/lib");
}

static int linux_file_read_cache_get(const char *cache_path, u64 offset,
				     u64 len, u64 buffer)
{
	const u64 domain = linux_cache_domain_for_path(0, cache_path);

	if (cache_path == 0 || buffer == 0 || len == 0 ||
	    len > LINUX_FILE_READ_CACHE_MAX ||
	    !linux_file_read_cache_allowed(cache_path)) {
		return 0;
	}
	for (u64 i = 0; i < LINUX_FILE_READ_CACHE_SIZE; i++) {
		const struct linux_file_read_cache_entry *entry =
			&linux_file_read_cache[i];

		if (entry->valid &&
		    entry->offset == offset &&
		    entry->len == len &&
		    entry->cache_domain == domain &&
		    entry->cache_epoch == linux_vfs_mutation_epochs[domain] &&
		    path_eq(entry->path, cache_path)) {
			return bunix_buffer_write(buffer, 0, entry->data,
						  len) == 0;
		}
	}
	return 0;
}

static void linux_file_read_cache_put(const char *cache_path, u64 offset,
				      u64 len, u64 buffer)
{
	struct linux_file_read_cache_entry *entry;
	const u64 domain = linux_cache_domain_for_path(0, cache_path);

	if (cache_path == 0 || buffer == 0 || len == 0 ||
	    len > LINUX_FILE_READ_CACHE_MAX ||
	    !linux_file_read_cache_allowed(cache_path)) {
		return;
	}
	entry = &linux_file_read_cache[linux_file_read_cache_next %
				       LINUX_FILE_READ_CACHE_SIZE];
	if (bunix_buffer_read(buffer, 0, entry->data, len) != 0) {
		entry->valid = 0;
		return;
	}
	entry->valid = 1;
	entry->offset = offset;
	entry->len = len;
	entry->cache_domain = domain;
	entry->cache_epoch = linux_vfs_mutation_epochs[domain];
	string_copy(entry->path, cache_path);
	linux_file_read_cache_next++;
}

static void linux_open_file_readahead_clear(struct linux_open_file *open_file)
{
	if (open_file == 0) {
		return;
	}
	open_file->read_cache_len = 0;
	open_file->read_cache_offset = 0;
	open_file->read_cache_epoch = 0;
	open_file->read_cache_domain = 0;
}

static void linux_debug_readahead_log(u64 hit, u64 bytes)
{
	static int enabled = -1;
	static u64 hits;
	static u64 fills;
	static u64 hit_bytes;
	static u64 fill_bytes;
	char line[220];
	u64 cursor = 0;

	if (enabled < 0) {
		enabled = bunix_cmdline_has("debug-linux-syscall-counts") > 0;
	}
	if (!enabled) {
		return;
	}
	if (hit) {
		hits++;
		hit_bytes += bytes;
	} else {
		fills++;
		fill_bytes += bytes;
	}
	if (((hits + fills) & 255) != 0) {
		return;
	}
	append_text(line, sizeof(line), &cursor,
		    "linux-server: readahead hits=");
	append_dec(line, sizeof(line), &cursor, hits);
	append_text(line, sizeof(line), &cursor, " fills=");
	append_dec(line, sizeof(line), &cursor, fills);
	append_text(line, sizeof(line), &cursor, " hit_bytes=");
	append_dec(line, sizeof(line), &cursor, hit_bytes);
	append_text(line, sizeof(line), &cursor, " fill_bytes=");
	append_dec(line, sizeof(line), &cursor, fill_bytes);
	append_char(line, sizeof(line), &cursor, '\n');
	bunix_console_log(line, cursor);
}

static void linux_debug_fcntl_log(u64 cmd)
{
	enum {
		FCNTL_DUPFD,
		FCNTL_GETFD,
		FCNTL_SETFD,
		FCNTL_GETFL,
		FCNTL_SETFL,
		FCNTL_LOCK,
		FCNTL_DUPFD_CLOEXEC,
		FCNTL_OTHER,
		FCNTL_COUNT,
	};
	static int enabled = -1;
	static u64 total;
	static u64 counts[FCNTL_COUNT];
	u64 bucket = FCNTL_OTHER;
	char line[360];
	u64 cursor = 0;

	if (enabled < 0) {
		enabled = bunix_cmdline_has("debug-linux-syscall-counts") > 0;
	}
	if (!enabled) {
		return;
	}
	switch (cmd) {
	case LINUX_F_DUPFD:
		bucket = FCNTL_DUPFD;
		break;
	case LINUX_F_GETFD:
		bucket = FCNTL_GETFD;
		break;
	case LINUX_F_SETFD:
		bucket = FCNTL_SETFD;
		break;
	case LINUX_F_GETFL:
		bucket = FCNTL_GETFL;
		break;
	case LINUX_F_SETFL:
		bucket = FCNTL_SETFL;
		break;
	case LINUX_F_SETLK:
	case LINUX_F_SETLKW:
	case LINUX_F_GETLK:
		bucket = FCNTL_LOCK;
		break;
	case LINUX_F_DUPFD_CLOEXEC:
		bucket = FCNTL_DUPFD_CLOEXEC;
		break;
	default:
		break;
	}
	total++;
	counts[bucket]++;
	if ((total & 255) != 0) {
		return;
	}
	append_text(line, sizeof(line), &cursor,
		    "linux-server: fcntl-counts total=");
	append_dec(line, sizeof(line), &cursor, total);
	append_text(line, sizeof(line), &cursor, " dupfd=");
	append_dec(line, sizeof(line), &cursor, counts[FCNTL_DUPFD]);
	append_text(line, sizeof(line), &cursor, " getfd=");
	append_dec(line, sizeof(line), &cursor, counts[FCNTL_GETFD]);
	append_text(line, sizeof(line), &cursor, " setfd=");
	append_dec(line, sizeof(line), &cursor, counts[FCNTL_SETFD]);
	append_text(line, sizeof(line), &cursor, " getfl=");
	append_dec(line, sizeof(line), &cursor, counts[FCNTL_GETFL]);
	append_text(line, sizeof(line), &cursor, " setfl=");
	append_dec(line, sizeof(line), &cursor, counts[FCNTL_SETFL]);
	append_text(line, sizeof(line), &cursor, " lock=");
	append_dec(line, sizeof(line), &cursor, counts[FCNTL_LOCK]);
	append_text(line, sizeof(line), &cursor, " dupfd_cloexec=");
	append_dec(line, sizeof(line), &cursor, counts[FCNTL_DUPFD_CLOEXEC]);
	append_text(line, sizeof(line), &cursor, " other=");
	append_dec(line, sizeof(line), &cursor, counts[FCNTL_OTHER]);
	append_text(line, sizeof(line), &cursor, " last_cmd=");
	append_dec(line, sizeof(line), &cursor, cmd);
	append_char(line, sizeof(line), &cursor, '\n');
	bunix_console_log(line, cursor);
}

static int linux_open_file_readahead_get(struct linux_fd *fd, u64 offset,
					 u64 len, u64 buffer)
{
	struct linux_open_file *open_file;
	u64 available;
	u64 cache_delta;

	if (fd == 0 || fd->open_file == 0 || fd->path[0] != '/' ||
	    buffer == 0 || len == 0 ||
	    !linux_file_read_cache_allowed(fd->path)) {
		return 0;
	}
	open_file = fd->open_file;
	if (open_file->read_cache == 0 || open_file->read_cache_len == 0 ||
	    open_file->read_cache_domain != linux_cache_domain_for_path(0, fd->path) ||
	    open_file->read_cache_epoch !=
		    linux_vfs_mutation_epochs[open_file->read_cache_domain] ||
	    offset < open_file->read_cache_offset) {
		return 0;
	}
	cache_delta = offset - open_file->read_cache_offset;
	if (cache_delta >= open_file->read_cache_len) {
		return 0;
	}
	available = open_file->read_cache_len - cache_delta;
	if (len > available) {
		return 0;
	}
	if (bunix_buffer_write(buffer, 0,
			       open_file->read_cache + cache_delta,
			       len) != 0) {
		return 0;
	}
	linux_debug_readahead_log(1, len);
	return 1;
}

static int linux_open_file_readahead_fill(struct linux_fd *fd, u64 offset,
					  u64 len, u64 buffer)
{
	struct linux_open_file *open_file;
	u64 read_len;
	long tmp;
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_READ_FILE_BUFFER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP,
		.reply = 0,
		.cap = 0,
		.words = { 0, offset, 0, 0 },
	};
	struct bunix_msg reply;

	if (fd == 0 || fd->open_file == 0 || fd->path[0] != '/' ||
	    buffer == 0 || len == 0 ||
	    len > LINUX_FILE_READAHEAD_TRIGGER_MAX ||
	    offset >= linux_fd_size(fd) ||
	    !linux_file_read_cache_allowed(fd->path)) {
		return 0;
	}
	open_file = fd->open_file;
	if (open_file->read_cache == 0) {
		open_file->read_cache = (char *)bunix_alloc(LINUX_FILE_READAHEAD_SIZE);
		if (open_file->read_cache == 0) {
			return 0;
		}
	}
	read_len = linux_fd_size(fd) - offset;
	if (read_len > LINUX_FILE_READAHEAD_SIZE) {
		read_len = LINUX_FILE_READAHEAD_SIZE;
	}
	if (read_len < len) {
		return 0;
	}
	tmp = bunix_buffer_create(read_len);
	if (tmp < 0) {
		return 0;
	}
	request.cap = (u64)tmp;
	request.words[0] = fd->handle;
	request.words[2] = read_len;
	if (bunix_ipc_call(LINUX_HANDLE_VFS, &request, &reply) != 0 ||
	    reply.words[0] != 0 || reply.words[1] < len ||
	    bunix_buffer_read((u64)tmp, 0, open_file->read_cache,
			      reply.words[1]) != 0) {
		bunix_handle_close((u64)tmp);
		linux_open_file_readahead_clear(open_file);
		return 0;
	}
	bunix_handle_close((u64)tmp);
	open_file->read_cache_offset = offset;
	open_file->read_cache_len = reply.words[1];
	open_file->read_cache_domain = linux_cache_domain_for_path(0, fd->path);
	open_file->read_cache_epoch =
		linux_vfs_mutation_epochs[open_file->read_cache_domain];
	linux_debug_readahead_log(0, reply.words[1]);
	return linux_open_file_readahead_get(fd, offset, len, buffer);
}

static int linux_global_access_cache_get(const char *cache_path, u64 mode,
					 u64 flags, long *result)
{
	const u64 domain = linux_cache_domain_for_path(0, cache_path);

	if (cache_path == 0 || result == 0 ||
	    !linux_global_path_cache_allowed(cache_path)) {
		return 0;
	}
	for (u64 i = 0; i < LINUX_GLOBAL_PATH_CACHE_SIZE; i++) {
		const struct linux_access_cache_entry *entry =
			&linux_global_path_cache[i];

		if (entry->valid &&
		    entry->mode == mode &&
		    entry->flags == flags &&
		    entry->cache_domain == domain &&
		    entry->cache_epoch == linux_vfs_mutation_epochs[domain] &&
		    path_eq(entry->path, cache_path)) {
			*result = entry->result;
			return 1;
		}
	}
	return 0;
}

static void linux_global_access_cache_put(const char *cache_path, u64 mode,
					  u64 flags, long result)
{
	struct linux_access_cache_entry *entry;
	const u64 domain = linux_cache_domain_for_path(0, cache_path);

	if (cache_path == 0 || !linux_global_path_cache_allowed(cache_path)) {
		return;
	}
	entry = &linux_global_path_cache[linux_global_path_cache_next %
					 LINUX_GLOBAL_PATH_CACHE_SIZE];
	entry->valid = 1;
	entry->stat_valid = 0;
	entry->base_handle = 0;
	entry->mode = mode;
	entry->flags = flags;
	entry->nofollow = 0;
	entry->cache_domain = domain;
	entry->cache_epoch = linux_vfs_mutation_epochs[domain];
	entry->result = result;
	string_copy(entry->path, cache_path);
	linux_global_path_cache_next++;
}

static int linux_global_stat_cache_get(const char *cache_path, u64 nofollow,
				       long *result,
				       struct linux_vfs_stat *stat)
{
	const u64 domain = linux_cache_domain_for_path(0, cache_path);

	if (cache_path == 0 || result == 0 || stat == 0 ||
	    !linux_global_path_cache_allowed(cache_path)) {
		return 0;
	}
	for (u64 i = 0; i < LINUX_GLOBAL_PATH_CACHE_SIZE; i++) {
		const struct linux_access_cache_entry *entry =
			&linux_global_path_cache[i];

		if (entry->stat_valid &&
		    entry->nofollow == nofollow &&
		    entry->cache_domain == domain &&
		    entry->cache_epoch == linux_vfs_mutation_epochs[domain] &&
		    path_eq(entry->path, cache_path)) {
			*result = entry->stat_result;
			if (entry->stat_result == 0) {
				*stat = entry->stat;
			}
			return 1;
		}
	}
	return 0;
}

static void linux_global_stat_cache_put(const char *cache_path, u64 nofollow,
					long result,
					const struct linux_vfs_stat *stat)
{
	struct linux_access_cache_entry *entry;
	const u64 domain = linux_cache_domain_for_path(0, cache_path);

	if (cache_path == 0 || !linux_global_path_cache_allowed(cache_path)) {
		return;
	}
	entry = &linux_global_path_cache[linux_global_path_cache_next %
					 LINUX_GLOBAL_PATH_CACHE_SIZE];
	entry->valid = 0;
	entry->stat_valid = 1;
	entry->base_handle = 0;
	entry->mode = 0;
	entry->flags = 0;
	entry->nofollow = nofollow;
	entry->cache_domain = domain;
	entry->cache_epoch = linux_vfs_mutation_epochs[domain];
	entry->result = 0;
	entry->stat_result = result;
	if (result == 0 && stat != 0) {
		entry->stat = *stat;
	}
	string_copy(entry->path, cache_path);
	linux_global_path_cache_next++;
}

static int linux_access_cache_get(struct linux_process *process,
				  u64 base_handle, const char *path,
				  const char *cache_path,
				  u64 mode, u64 flags, long *result)
{
	const u64 domain = linux_cache_domain_for_path(process, path);

	if (process == 0 || path == 0 || result == 0) {
		return 0;
	}
	if (linux_global_access_cache_get(cache_path, mode, flags, result)) {
		return 1;
	}
	linux_access_cache_sync(process);
	for (u64 i = 0; i < LINUX_ACCESS_CACHE_SIZE; i++) {
		const struct linux_access_cache_entry *entry =
			&process->access_cache[i];

		if (entry->valid &&
		    entry->base_handle == base_handle &&
		    entry->mode == mode &&
		    entry->flags == flags &&
		    entry->cache_domain == domain &&
		    entry->cache_epoch == linux_vfs_mutation_epochs[domain] &&
		    path_eq(entry->path, path)) {
			*result = entry->result;
			return 1;
		}
	}
	return 0;
}

static void linux_access_cache_put(struct linux_process *process,
				   u64 base_handle, const char *path,
				   const char *cache_path,
				   u64 mode, u64 flags, long result)
{
	struct linux_access_cache_entry *entry;
	const u64 domain = linux_cache_domain_for_path(process, path);

	if (process == 0 || path == 0) {
		return;
	}
	linux_global_access_cache_put(cache_path, mode, flags, result);
	linux_access_cache_sync(process);
	entry = &process->access_cache[process->access_cache_next %
				      LINUX_ACCESS_CACHE_SIZE];
	entry->valid = 1;
	entry->stat_valid = 0;
	entry->base_handle = base_handle;
	entry->mode = mode;
	entry->flags = flags;
	entry->nofollow = 0;
	entry->cache_domain = domain;
	entry->cache_epoch = linux_vfs_mutation_epochs[domain];
	entry->result = result;
	string_copy(entry->path, path);
	process->access_cache_next++;
}

static int linux_stat_cache_get(struct linux_process *process,
				u64 base_handle, const char *path,
				const char *cache_path,
				u64 nofollow, long *result,
				struct linux_vfs_stat *stat)
{
	const u64 domain = linux_cache_domain_for_path(process, path);

	if (process == 0 || path == 0 || result == 0 || stat == 0) {
		return 0;
	}
	if (linux_global_stat_cache_get(cache_path, nofollow, result, stat)) {
		return 1;
	}
	linux_access_cache_sync(process);
	for (u64 i = 0; i < LINUX_ACCESS_CACHE_SIZE; i++) {
		const struct linux_access_cache_entry *entry =
			&process->access_cache[i];

		if (entry->stat_valid &&
		    entry->base_handle == base_handle &&
		    entry->nofollow == nofollow &&
		    entry->cache_domain == domain &&
		    entry->cache_epoch == linux_vfs_mutation_epochs[domain] &&
		    path_eq(entry->path, path)) {
			*result = entry->stat_result;
			if (entry->stat_result == 0) {
				*stat = entry->stat;
			}
			return 1;
		}
	}
	return 0;
}

static void linux_stat_cache_put(struct linux_process *process,
				 u64 base_handle, const char *path,
				 const char *cache_path,
				 u64 nofollow, long result,
				 const struct linux_vfs_stat *stat)
{
	struct linux_access_cache_entry *entry;
	const u64 domain = linux_cache_domain_for_path(process, path);

	if (process == 0 || path == 0) {
		return;
	}
	linux_global_stat_cache_put(cache_path, nofollow, result, stat);
	linux_access_cache_sync(process);
	entry = &process->access_cache[process->access_cache_next %
				      LINUX_ACCESS_CACHE_SIZE];
	entry->valid = 0;
	entry->stat_valid = 1;
	entry->base_handle = base_handle;
	entry->mode = 0;
	entry->flags = 0;
	entry->nofollow = nofollow;
	entry->cache_domain = domain;
	entry->cache_epoch = linux_vfs_mutation_epochs[domain];
	entry->result = 0;
	entry->stat_result = result;
	if (result == 0 && stat != 0) {
		entry->stat = *stat;
	}
	string_copy(entry->path, path);
	process->access_cache_next++;
}

static long linux_check_name_max(const char *path)
{
	u64 component_len = 0;

	if (path == 0) {
		return -LINUX_EFAULT;
	}
	for (u64 i = 0; path[i] != '\0'; i++) {
		if (path[i] == '/') {
			component_len = 0;
			continue;
		}
		component_len++;
		if (component_len > LINUX_NAME_MAX) {
			return -LINUX_ENAMETOOLONG;
		}
	}
	return 0;
}

static long linux_read_path_arg(u64 path_buffer, u64 path_len, char *path,
				u64 path_cap)
{
	if (path_buffer == 0) {
		return -LINUX_EFAULT;
	}
	if (path_len == 0) {
		return -LINUX_EINVAL;
	}
	if (path_len > path_cap) {
		return -LINUX_ENAMETOOLONG;
	}
	if (bunix_buffer_read(path_buffer, 0, path, path_len) != 0) {
		return -LINUX_EFAULT;
	}
	if (path[path_len - 1] != '\0') {
		return -LINUX_ENAMETOOLONG;
	}
	if (path[0] == '\0') {
		return -LINUX_ENOENT;
	}
	return linux_check_name_max(path);
}

static long path_normalize(const char *cwd, const char *input, char *out)
{
	char temp[LINUX_MAX_PATH];
	u64 pos = 0;
	u64 in = 0;

	if (input == 0 || input[0] == '\0') {
		return -LINUX_EINVAL;
	}
	if (input[0] == '/') {
		temp[pos++] = '/';
	} else {
		const u64 cwd_len = string_len(cwd);

		for (u64 i = 0; i < cwd_len && pos < sizeof(temp) - 1; i++) {
			temp[pos++] = cwd[i];
		}
		if (pos == 0) {
			temp[pos++] = '/';
		}
	}

	while (input[in] != '\0') {
		char component[LINUX_MAX_PATH];
		u64 comp_len = 0;

		while (input[in] == '/') {
			in++;
		}
		while (input[in] != '\0' && input[in] != '/') {
			if (comp_len >= LINUX_NAME_MAX) {
				return -LINUX_ENAMETOOLONG;
			}
			if (comp_len + 1 >= sizeof(component)) {
				return -LINUX_ENAMETOOLONG;
			}
			component[comp_len++] = input[in++];
		}
		component[comp_len] = '\0';
		if (comp_len == 0 || string_equal(component, ".")) {
			continue;
		}
		if (string_equal(component, "..")) {
			if (pos > 1) {
				if (temp[pos - 1] == '/') {
					pos--;
				}
				while (pos > 1 && temp[pos - 1] != '/') {
					pos--;
				}
				if (pos > 1 && temp[pos - 1] == '/') {
					pos--;
				}
			}
			temp[pos] = '\0';
			continue;
		}
		if (pos > 1) {
			if (pos + 1 >= sizeof(temp)) {
				return -LINUX_ENAMETOOLONG;
			}
			temp[pos++] = '/';
		}
		if (pos + comp_len >= sizeof(temp)) {
			return -LINUX_ENAMETOOLONG;
		}
		for (u64 i = 0; i < comp_len; i++) {
			temp[pos++] = component[i];
		}
		temp[pos] = '\0';
	}

	if (pos == 0) {
		temp[pos++] = '/';
	}
	temp[pos] = '\0';
	string_copy(out, temp);
	return 0;
}

static long linux_path_join_child(const char *dir, const char *name, char *out)
{
	u64 pos = 0;
	u64 i = 0;

	if (dir == 0 || name == 0 || out == 0 ||
	    dir[0] == '\0' || name[0] == '\0') {
		return -LINUX_EINVAL;
	}
	if (path_eq(dir, ".")) {
		string_copy(out, name);
		return linux_check_name_max(out);
	}
	while (dir[i] != '\0') {
		if (pos + 1 >= LINUX_MAX_PATH) {
			return -LINUX_ENAMETOOLONG;
		}
		out[pos++] = dir[i++];
	}
	if (pos == 0) {
		return -LINUX_EINVAL;
	}
	if (out[pos - 1] != '/') {
		if (pos + 1 >= LINUX_MAX_PATH) {
			return -LINUX_ENAMETOOLONG;
		}
		out[pos++] = '/';
	}
	i = 0;
	while (name[i] != '\0') {
		if (pos + 1 >= LINUX_MAX_PATH) {
			return -LINUX_ENAMETOOLONG;
		}
		out[pos++] = name[i++];
	}
	out[pos] = '\0';
	return linux_check_name_max(out);
}

static void linux_tmpfile_name(struct linux_process *process, char *name,
			       u64 name_size)
{
	u64 cursor = 0;
	const u64 serial = ++linux_tmpfile_counter;

	append_text(name, name_size, &cursor, ".bunix-tmpfile.");
	append_dec(name, name_size, &cursor, process != 0 ? process->pid : 0);
	append_char(name, name_size, &cursor, '.');
	append_dec(name, name_size, &cursor, serial);
	if (cursor < name_size) {
		name[cursor] = '\0';
	} else if (name_size != 0) {
		name[name_size - 1] = '\0';
	}
}

static int linux_cache_path_for_dirfd(const struct linux_process *process,
				      u64 dirfd, const char *path, char *out)
{
	if (process == 0 || path == 0 || out == 0) {
		return -1;
	}
	if (path[0] == '/' || dirfd == LINUX_AT_FDCWD) {
		return path_normalize(process->cwd, path, out) == 0 ? 0 : -1;
	}
	if (dirfd >= process->fd_capacity ||
	    process->fds[dirfd].kind != LINUX_FD_DIR ||
	    process->fds[dirfd].path[0] != '/') {
		return -1;
	}
	return path_normalize(process->fds[dirfd].path, path, out) == 0 ?
	       0 : -1;
}

static int linux_process_init_fds(struct linux_process *process)
{
	process->fd_capacity = LINUX_INITIAL_FDS;
	process->fds = (struct linux_fd *)
		bunix_calloc(process->fd_capacity, sizeof(process->fds[0]));
	if (process->fds == 0) {
		process->fd_capacity = 0;
		return -1;
	}

	for (u64 fd = 0; fd < process->fd_capacity; fd++) {
		process->fds[fd].handle = 0;
		process->fds[fd].kind = 0;
		process->fds[fd].offset = 0;
		process->fds[fd].size = 0;
		process->fds[fd].flags = 0;
		process->fds[fd].status_flags = 0;
		process->fds[fd].socket_type = 0;
		process->fds[fd].open_file = 0;
		process->fds[fd].path[0] = '\0';
	}

	process->fds[0].handle = LINUX_HANDLE_CONSOLE;
	process->fds[0].kind = LINUX_FD_CONSOLE;
	process->fds[1].handle = LINUX_HANDLE_CONSOLE;
	process->fds[1].kind = LINUX_FD_CONSOLE;
	process->fds[2].handle = LINUX_HANDLE_CONSOLE;
	process->fds[2].kind = LINUX_FD_CONSOLE;
	return 0;
}

static int linux_process_ensure_fds(struct linux_process *process, u64 count)
{
	u64 old_capacity;
	u64 new_capacity;
	struct linux_fd *fds;

	if (process == 0 || count == 0) {
		return -1;
	}
	if (process->fd_capacity >= count) {
		return 0;
	}

	old_capacity = process->fd_capacity;
	new_capacity = old_capacity == 0 ? LINUX_INITIAL_FDS : old_capacity;
	while (new_capacity < count) {
		new_capacity *= 2;
	}

	fds = (struct linux_fd *)
		bunix_realloc(process->fds,
			      old_capacity * sizeof(process->fds[0]),
			      new_capacity * sizeof(process->fds[0]));
	if (fds == 0) {
		return -1;
	}
	for (u64 fd = old_capacity; fd < new_capacity; fd++) {
		fds[fd].handle = 0;
		fds[fd].kind = 0;
		fds[fd].offset = 0;
		fds[fd].size = 0;
		fds[fd].flags = 0;
		fds[fd].status_flags = 0;
		fds[fd].socket_type = 0;
		fds[fd].open_file = 0;
		fds[fd].path[0] = '\0';
	}
	process->fds = fds;
	process->fd_capacity = new_capacity;
	return 0;
}

static long alloc_fd(struct linux_process *process, u64 kind, u64 handle,
		     u64 size)
{
	for (;;) {
		for (u64 fd = 0; fd < process->fd_capacity; fd++) {
			if (process->fds[fd].kind == 0) {
				process->fds[fd].kind = kind;
				process->fds[fd].handle = handle;
				process->fds[fd].offset = 0;
				process->fds[fd].size = size;
				process->fds[fd].flags = 0;
				process->fds[fd].status_flags = 0;
				process->fds[fd].socket_type = 0;
				process->fds[fd].open_file = 0;
				process->fds[fd].path[0] = '\0';
				return (long)fd;
			}
		}
		if (linux_process_ensure_fds(process,
					     process->fd_capacity + 1) != 0) {
			return -LINUX_EMFILE;
		}
	}
}

static struct linux_pipe *linux_pipe_find(u64 id)
{
	const u64 index = bunix_id_get(&pipe_ids, id);

	return (struct linux_pipe *)index;
}

static void linux_pipe_ref_add(const struct linux_fd *fd)
{
	struct linux_pipe *pipe = linux_pipe_find(fd->handle);

	if (pipe == 0) {
		return;
	}
	if (fd->kind == LINUX_FD_PIPE_READ) {
		pipe->read_refs++;
		linux_debug_pipe_log("ref-add-read", 0, 0, pipe, 0);
	} else if (fd->kind == LINUX_FD_PIPE_WRITE) {
		pipe->write_refs++;
		linux_debug_pipe_log("ref-add-write", 0, 0, pipe, 0);
	}
}

static long linux_pipe_write_available(struct linux_pipe *pipe, u64 len,
				       u64 buffer);
static void linux_pipe_wake_writer(struct linux_pipe *pipe);

static void linux_pipe_ref_drop(const struct linux_fd *fd)
{
	struct linux_pipe *pipe = linux_pipe_find(fd->handle);

	if (pipe == 0) {
		return;
	}
	if (fd->kind == LINUX_FD_PIPE_READ && pipe->read_refs != 0) {
		pipe->read_refs--;
		linux_debug_pipe_log("ref-drop-read", 0, 0, pipe, 0);
	} else if (fd->kind == LINUX_FD_PIPE_WRITE && pipe->write_refs != 0) {
		pipe->write_refs--;
		linux_debug_pipe_log("ref-drop-write", 0, 0, pipe, 0);
	}
}

static void linux_pipe_destroy_if_unused(struct linux_pipe *pipe)
{
	if (pipe == 0) {
		return;
	}
	if (pipe->read_refs == 0 && pipe->write_refs == 0) {
		if (pipe->reader_buffer != 0) {
			bunix_handle_close(pipe->reader_buffer);
		}
		if (pipe->writer_buffer != 0) {
			bunix_handle_close(pipe->writer_buffer);
		}
		(void)bunix_id_remove(&pipe_ids, pipe->id);
		pipe->id = 0;
		pipe->start = 0;
		pipe->len = 0;
		pipe->reader_reply = 0;
		pipe->reader_buffer = 0;
		pipe->reader_len = 0;
		bunix_free(pipe);
	}
}

static long linux_pipe_create(struct linux_process *process, u64 flags,
			      u64 *read_fd, u64 *write_fd)
{
	struct linux_pipe *pipe = 0;
	long left;
	long right;

	if ((flags & ~(LINUX_O_CLOEXEC | LINUX_O_NONBLOCK)) != 0 ||
	    read_fd == 0 || write_fd == 0) {
		return -LINUX_EINVAL;
	}
	pipe = (struct linux_pipe *)bunix_calloc(1, sizeof(*pipe));
	if (pipe == 0) {
		return -LINUX_EMFILE;
	}

	pipe->id = bunix_id_alloc(&pipe_ids, (u64)pipe);
	if (pipe->id == 0) {
		bunix_free(pipe);
		return -LINUX_EMFILE;
	}
	pipe->read_refs = 1;
	pipe->write_refs = 1;
	pipe->start = 0;
	pipe->len = 0;
	pipe->reader_reply = 0;
	pipe->reader_buffer = 0;
	pipe->reader_len = 0;
	pipe->writer_reply = 0;
	pipe->writer_buffer = 0;
	pipe->writer_len = 0;

	left = alloc_fd(process, LINUX_FD_PIPE_READ, pipe->id, 0);
	if (left < 0) {
		(void)bunix_id_remove(&pipe_ids, pipe->id);
		bunix_free(pipe);
		return left;
	}
	right = alloc_fd(process, LINUX_FD_PIPE_WRITE, pipe->id, 0);
	if (right < 0) {
		process->fds[left].kind = 0;
		process->fds[left].socket_type = 0;
		(void)bunix_id_remove(&pipe_ids, pipe->id);
		bunix_free(pipe);
		return right;
	}
	if ((flags & LINUX_O_CLOEXEC) != 0) {
		process->fds[left].flags |= LINUX_FD_CLOEXEC;
		process->fds[right].flags |= LINUX_FD_CLOEXEC;
	}
	if ((flags & LINUX_O_NONBLOCK) != 0) {
		process->fds[left].status_flags |= LINUX_O_NONBLOCK;
		process->fds[right].status_flags |= LINUX_O_NONBLOCK;
	}
	linux_debug_pipe_log("create-read", process, (u64)left, pipe,
			     (long)right);
	*read_fd = (u64)left;
	*write_fd = (u64)right;
	return 0;
}

static long linux_pipe_read_available(struct linux_pipe *pipe, u64 len,
				      u64 buffer)
{
	u64 nread;

	if (pipe == 0 || buffer == 0) {
		return -LINUX_EBADF;
	}
	if (pipe->len == 0) {
		return pipe->write_refs == 0 ? 0 : -LINUX_EAGAIN;
	}
	nread = len < pipe->len ? len : pipe->len;
	for (u64 i = 0; i < nread; i++) {
		write_buffer[i] = pipe->data[(pipe->start + i) %
					     LINUX_PIPE_CAPACITY];
	}
	pipe->start = (pipe->start + nread) % LINUX_PIPE_CAPACITY;
	pipe->len -= nread;
	if (bunix_buffer_write(buffer, 0, write_buffer, nread) != 0) {
		return -LINUX_EFAULT;
	}
	linux_pipe_wake_writer(pipe);
	return (long)nread;
}

static long linux_pipe_write_available(struct linux_pipe *pipe, u64 len,
				       u64 buffer)
{
	u64 copy_len;
	u64 space;
	u64 nwritten;

	if (pipe == 0 || buffer == 0) {
		return -LINUX_EBADF;
	}
	if (pipe->read_refs == 0) {
		return -LINUX_EPIPE;
	}
	space = LINUX_PIPE_CAPACITY - pipe->len;
	if (space == 0) {
		return -LINUX_EAGAIN;
	}
	copy_len = len < sizeof(write_buffer) ? len : sizeof(write_buffer);
	nwritten = copy_len < space ? copy_len : space;
	if (nwritten != 0 &&
	    bunix_buffer_read(buffer, 0, write_buffer, nwritten) != 0) {
		return -LINUX_EFAULT;
	}
	for (u64 i = 0; i < nwritten; i++) {
		pipe->data[(pipe->start + pipe->len + i) %
			   LINUX_PIPE_CAPACITY] = write_buffer[i];
	}
	pipe->len += nwritten;
	return (long)nwritten;
}

static void linux_pipe_wake_reader(struct linux_pipe *pipe)
{
	struct bunix_msg reply = {
		.protocol = BUNIX_PROTO_LINUX,
		.type = BUNIX_LINUX_READ,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { 0, 0, 0, 0 },
	};
	u64 reply_handle;
	u64 buffer;
	long nread;

	if (pipe == 0 || pipe->reader_reply == 0 ||
	    (pipe->len == 0 && pipe->write_refs != 0)) {
		return;
	}

	reply_handle = pipe->reader_reply;
	buffer = pipe->reader_buffer;
	pipe->reader_reply = 0;
	pipe->reader_buffer = 0;
	nread = linux_pipe_read_available(pipe, pipe->reader_len, buffer);
	pipe->reader_len = 0;
	linux_debug_pipe_log("wake-reader", 0, 0, pipe, nread);
	reply.words[0] = (u64)nread;
	(void)bunix_ipc_send(reply_handle, &reply);
	if (buffer != 0) {
		bunix_handle_close(buffer);
	}
}

static void linux_pipe_wake_writer(struct linux_pipe *pipe)
{
	struct bunix_msg reply = {
		.protocol = BUNIX_PROTO_LINUX,
		.type = BUNIX_LINUX_WRITE,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { 0, 0, 0, 0 },
	};
	u64 reply_handle;
	u64 buffer;
	long nwritten;

	if (pipe == 0 || pipe->writer_reply == 0 ||
	    (pipe->read_refs != 0 && pipe->len == LINUX_PIPE_CAPACITY)) {
		return;
	}

	reply_handle = pipe->writer_reply;
	buffer = pipe->writer_buffer;
	pipe->writer_reply = 0;
	pipe->writer_buffer = 0;
	nwritten = linux_pipe_write_available(pipe, pipe->writer_len, buffer);
	pipe->writer_len = 0;
	linux_debug_pipe_log("wake-writer", 0, 0, pipe, nwritten);
	reply.words[0] = (u64)nwritten;
	(void)bunix_ipc_send(reply_handle, &reply);
	if (buffer != 0) {
		bunix_handle_close(buffer);
	}
	linux_pipe_wake_reader(pipe);
}

static void linux_file_ref_add(u64 handle)
{
	u64 refs;

	if (handle == 0) {
		return;
	}

	refs = bunix_map_get(&file_refs, handle);
	if (refs == 0) {
		(void)bunix_map_set(&file_refs, handle, 1);
	} else {
		(void)bunix_map_set(&file_refs, handle, refs + 1);
	}
}

static long linux_file_ref_drop(u64 handle)
{
	u64 refs;

	if (handle == 0) {
		return 0;
	}

	refs = bunix_map_get(&file_refs, handle);
	if (refs > 1) {
		(void)bunix_map_set(&file_refs, handle, refs - 1);
		return 1;
	}
	if (refs == 1) {
		(void)bunix_map_remove(&file_refs, handle);
	}
	return 0;
}

static void linux_fd_set_offset(struct linux_process *process, u64 fd,
				u64 offset)
{
	if (process == 0 || fd >= process->fd_capacity) {
		return;
	}
	process->fds[fd].offset = offset;
	if (process->fds[fd].open_file != 0) {
		process->fds[fd].open_file->offset = offset;
	}
}

static void linux_fd_set_size(struct linux_process *process, u64 fd, u64 size)
{
	if (process == 0 || fd >= process->fd_capacity) {
		return;
	}
	process->fds[fd].size = size;
	if (process->fds[fd].offset > size) {
		process->fds[fd].offset = size;
	}
	if (process->fds[fd].open_file != 0) {
		process->fds[fd].open_file->size = size;
		if (process->fds[fd].open_file->offset > size) {
			process->fds[fd].open_file->offset = size;
		}
		process->fds[fd].offset = process->fds[fd].open_file->offset;
	}
}

static u64 linux_fd_offset(const struct linux_fd *fd)
{
	return fd != 0 && fd->open_file != 0 ? fd->open_file->offset :
	       (fd != 0 ? fd->offset : 0);
}

static u64 linux_fd_size(const struct linux_fd *fd)
{
	return fd != 0 && fd->open_file != 0 ? fd->open_file->size :
	       (fd != 0 ? fd->size : 0);
}

static int linux_fd_open_file_create(struct linux_fd *fd, u64 handle,
				     u64 size)
{
	struct linux_open_file *open_file;

	if (fd == 0) {
		return -1;
	}
	open_file = (struct linux_open_file *)bunix_calloc(1,
							   sizeof(*open_file));
	if (open_file == 0) {
		return -1;
	}
	open_file->handle = handle;
	open_file->offset = 0;
	open_file->size = size;
	open_file->refs = 1;
	fd->handle = handle;
	fd->offset = 0;
	fd->size = size;
	fd->open_file = open_file;
	return 0;
}

static void linux_open_file_ref_add(struct linux_fd *fd)
{
	if (fd != 0 && fd->open_file != 0) {
		fd->open_file->refs++;
		fd->handle = fd->open_file->handle;
		fd->offset = fd->open_file->offset;
		fd->size = fd->open_file->size;
	}
}

static long linux_open_file_ref_drop(struct linux_fd *fd)
{
	struct linux_open_file *open_file;
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_CLOSE,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { 0, 0, 0, 0 },
	};
	struct bunix_msg reply;

	if (fd == 0 || fd->open_file == 0) {
		return 0;
	}
	open_file = fd->open_file;
	if (open_file->refs > 1) {
		open_file->refs--;
		fd->open_file = 0;
		return 0;
	}
	request.words[0] = open_file->handle;
	fd->open_file = 0;
	if (bunix_ipc_call(LINUX_HANDLE_VFS, &request, &reply) != 0) {
		if (open_file->read_cache != 0) {
			bunix_free(open_file->read_cache);
		}
		bunix_free(open_file);
		return -LINUX_EIO;
	}
	if (open_file->read_cache != 0) {
		bunix_free(open_file->read_cache);
	}
	bunix_free(open_file);
	return reply.words[0] == 0 ? 0 : -LINUX_EIO;
}

static int linux_fd_is_refcounted_net_socket(const struct linux_fd *fd)
{
	return fd != 0 && fd->kind == LINUX_FD_SOCKET &&
	       (fd->handle == LINUX_SOCKET_NET_UDP ||
		fd->handle == LINUX_SOCKET_NET_TCP ||
		fd->handle == LINUX_SOCKET_NET_ICMP);
}

static u64 linux_net_socket_ref_key(const struct linux_fd *fd)
{
	return (fd->handle << 56) ^ fd->offset;
}

static void linux_net_socket_ref_add(const struct linux_fd *fd)
{
	const u64 key = linux_net_socket_ref_key(fd);
	u64 refs;

	if (!linux_fd_is_refcounted_net_socket(fd) || fd->offset == 0) {
		return;
	}
	refs = bunix_map_get(&net_socket_refs, key);
	if (refs == 0) {
		(void)bunix_map_set(&net_socket_refs, key, 1);
	} else {
		(void)bunix_map_set(&net_socket_refs, key, refs + 1);
	}
}

static long linux_net_socket_ref_drop(const struct linux_fd *fd)
{
	const u64 key = linux_net_socket_ref_key(fd);
	u64 refs;

	if (!linux_fd_is_refcounted_net_socket(fd) || fd->offset == 0) {
		return 0;
	}
	refs = bunix_map_get(&net_socket_refs, key);
	if (refs > 1) {
		(void)bunix_map_set(&net_socket_refs, key, refs - 1);
		return 1;
	}
	if (refs == 1) {
		(void)bunix_map_remove(&net_socket_refs, key);
	}
	return 0;
}

static struct linux_process *linux_process_find(u64 bunix_task)
{
	const u64 index = bunix_map_get(&process_by_task, bunix_task);

	return (struct linux_process *)index;
}

static struct linux_process *linux_process_find_pid(u64 pid)
{
	const u64 index = bunix_map_get(&process_by_pid, pid);

	return (struct linux_process *)index;
}

static struct linux_process *linux_process_for(const struct bunix_msg *message)
{
	struct linux_process *process = linux_process_find(message->sender);

	if (process != 0) {
		if (message->words[1] != 0) {
			process->bunix_thread = message->words[1];
		}
		return process;
	}

	return 0;
}

static long linux_register_process(u64 bunix_task, u64 ppid, u64 session_id,
				   u64 requested_pid)
{
	if (bunix_task == 0 || linux_process_find(bunix_task) != 0) {
		bunix_console_log("linux-server: register duplicate task\n",
				  sizeof("linux-server: register duplicate task\n") - 1);
		return -LINUX_EINVAL;
	}
	if (requested_pid != 0 && linux_process_find_pid(requested_pid) != 0) {
		bunix_console_log("linux-server: register duplicate pid\n",
				  sizeof("linux-server: register duplicate pid\n") - 1);
		return -LINUX_EINVAL;
	}

	struct linux_process *process = (struct linux_process *)
		bunix_calloc(1, sizeof(*process));
	if (process == 0) {
		bunix_console_log("linux-server: register alloc failed\n",
				  sizeof("linux-server: register alloc failed\n") - 1);
		return -LINUX_ESRCH;
	}

	const u64 pid = requested_pid != 0 ? requested_pid : next_pid++;

	if (pid >= next_pid) {
		next_pid = pid + 1;
	}

	process->pid = pid;
	process->tid = pid;
	process->ppid = ppid;
	process->pgid = pid;
	process->sid = pid;
	process->umask = 022;
	linux_process_init_links(process);
	process->bunix_task = bunix_task;
	process->bunix_proc_pid = requested_pid != 0 ? requested_pid : pid;
	process->session_id = session_id;
	if (linux_process_init_fds(process) != 0) {
		bunix_console_log("linux-server: register fds failed\n",
				  sizeof("linux-server: register fds failed\n") - 1);
		bunix_free(process);
		return -LINUX_ESRCH;
	}
	if (linux_process_set_cwd(process, "/") != 0) {
		bunix_console_log("linux-server: register cwd failed\n",
				  sizeof("linux-server: register cwd failed\n") - 1);
		bunix_free(process->fds);
		bunix_free(process);
		return -LINUX_ESRCH;
	}
	linux_tty_note_process(&console_tty, process);
	if (session_id != 0) {
		(void)linux_user_session_set_foreground(session_id,
							process->pgid);
	}
	if (linux_user_process_register(bunix_task) != 0) {
		bunix_console_log("linux-server: register user failed\n",
				  sizeof("linux-server: register user failed\n") - 1);
		linux_process_reset(process);
		return -LINUX_ESRCH;
	}
	if (bunix_map_set(&process_by_task, bunix_task, (u64)process) != 0 ||
	    bunix_map_set(&process_by_pid, pid, (u64)process) != 0) {
		bunix_console_log("linux-server: register map failed\n",
				  sizeof("linux-server: register map failed\n") - 1);
		linux_process_reset(process);
		return -LINUX_ESRCH;
	}
	return (long)pid;
}

static long linux_fork_process(u64 parent_task, u64 child_task)
{
	struct linux_process *parent = linux_process_find(parent_task);

	if (parent == 0 || child_task == 0 ||
	    linux_process_find(child_task) != 0) {
		bunix_console_log("linux-server: fork invalid task\n",
				  sizeof("linux-server: fork invalid task\n") - 1);
		return -LINUX_EINVAL;
	}

	struct linux_process *process = (struct linux_process *)
		bunix_calloc(1, sizeof(*process));
	u64 bunix_proc_pid = 0;
	if (process == 0) {
		bunix_console_log("linux-server: fork alloc failed\n",
				  sizeof("linux-server: fork alloc failed\n") - 1);
		return -LINUX_ESRCH;
	}

	const u64 pid = next_pid++;

	process->pid = pid;
	process->tid = pid;
	process->ppid = parent->pid;
	process->pgid = parent->pgid;
	process->sid = parent->sid;
	process->signal_mask = parent->signal_mask;
	process->signal_ignored = parent->signal_ignored;
	for (u64 signal = 0; signal < 64; signal++) {
		process->signal_handlers[signal] = parent->signal_handlers[signal];
		process->signal_restorers[signal] = parent->signal_restorers[signal];
		process->signal_flags[signal] = parent->signal_flags[signal];
	}
	process->umask = parent->umask;
	linux_process_init_links(process);
	linux_child_link(parent, process);
	process->bunix_task = child_task;
	process->session_id = parent->session_id;
	process->cwd_handle = parent->cwd_handle;
	if (process->cwd_handle != 0) {
		linux_file_ref_add(process->cwd_handle);
	}
	process->fd_capacity = parent->fd_capacity;
	process->fds = (struct linux_fd *)
		bunix_alloc(parent->fd_capacity * sizeof(process->fds[0]));
	if (process->fds == 0) {
		bunix_console_log("linux-server: fork fds failed\n",
				  sizeof("linux-server: fork fds failed\n") - 1);
		linux_process_reset(process);
		return -LINUX_ESRCH;
	}
	for (u64 fd = 0; fd < process->fd_capacity; fd++) {
		process->fds[fd] = parent->fds[fd];
		if (process->fds[fd].kind == LINUX_FD_FILE ||
		    process->fds[fd].kind == LINUX_FD_DIR) {
			linux_open_file_ref_add(&process->fds[fd]);
		} else if (process->fds[fd].kind == LINUX_FD_PIPE_READ ||
			   process->fds[fd].kind == LINUX_FD_PIPE_WRITE) {
			linux_pipe_ref_add(&process->fds[fd]);
		} else if (linux_fd_is_refcounted_net_socket(&process->fds[fd])) {
			linux_net_socket_ref_add(&process->fds[fd]);
		}
	}
	if (linux_process_set_cwd(process, parent->cwd) != 0) {
		bunix_console_log("linux-server: fork cwd failed\n",
				  sizeof("linux-server: fork cwd failed\n") - 1);
		linux_process_reset(process);
		return -LINUX_ESRCH;
	}
	if (linux_user_process_fork(parent_task, child_task) != 0) {
		bunix_console_log("linux-server: fork user failed\n",
				  sizeof("linux-server: fork user failed\n") - 1);
		linux_process_reset(process);
		return -LINUX_ESRCH;
	}
	if (bunix_map_set(&process_by_task, child_task, (u64)process) != 0 ||
	    bunix_map_set(&process_by_pid, pid, (u64)process) != 0) {
		bunix_console_log("linux-server: fork map failed\n",
				  sizeof("linux-server: fork map failed\n") - 1);
		linux_process_reset(process);
		return -LINUX_ESRCH;
	}
	if (notify_proc_register_linux(pid, child_task, parent->pid,
				       &bunix_proc_pid) != 0) {
		bunix_console_log("linux-server: fork proc failed\n",
				  sizeof("linux-server: fork proc failed\n") - 1);
		linux_process_reset(process);
		return -LINUX_ESRCH;
	}
	process->bunix_proc_pid = bunix_proc_pid;
	return (long)pid;
}

static int linux_same_session(const struct linux_process *left,
			      const struct linux_process *right)
{
	if (left == 0 || right == 0) {
		return 0;
	}
	if (left->session_id != 0 || right->session_id != 0) {
		return left->session_id != 0 &&
		       left->session_id == right->session_id;
	}
	return left->sid == right->sid;
}

static u64 linux_process_tty_session(const struct linux_process *process)
{
	if (process == 0) {
		return 0;
	}
	return process->sid;
}

static int linux_tty_session_matches(const struct linux_tty *tty,
				     const struct linux_process *process)
{
	const u64 session = linux_process_tty_session(process);

	return tty != 0 && process != 0 && session != 0 &&
	       (tty->session_id == 0 || tty->session_id == session);
}

static void linux_tty_bind_session(struct linux_tty *tty,
				   const struct linux_process *process)
{
	if (tty == 0 || process == 0) {
		return;
	}
	tty->session_id = linux_process_tty_session(process);
	tty->owner_pid = process->pid;
	if (tty->foreground_pgid == 0) {
		tty->foreground_pgid = process->pgid;
	}
}

static int linux_pgrp_exists(u64 pgid)
{
	for (u64 i = 0;; i++) {
		struct linux_process *process =
			(struct linux_process *)bunix_map_at(&process_by_pid,
							     i);

		if (process == 0) {
			break;
		}
		if (!process->exited && process->pgid == pgid) {
			return 1;
		}
	}

	return 0;
}

static int linux_child_matches(const struct linux_process *parent,
			       const struct linux_process *child, long pid)
{
	if (parent == 0 || child == 0 ||
	    child->parent != parent ||
	    child->waited) {
		return 0;
	}

	if (pid == -1) {
		return 1;
	}
	if (pid == 0) {
		return child->pgid == parent->pgid;
	}
	if (pid < -1) {
		return child->pgid == (u64)(-(pid + 1)) + 1;
	}
	return pid > 0 && child->pid == (u64)pid;
}

static void linux_reparent_children(struct linux_process *process)
{
	struct linux_process *init = linux_process_find_pid(1);
	struct linux_process *child;

	if (process == 0 || process->first_child == 0) {
		return;
	}

	child = process->first_child;
	process->first_child = 0;

	while (child != 0) {
		struct linux_process *next = child->next_sibling;

		child->parent = 0;
		child->next_sibling = 0;
		if (child->exited) {
			child->waited = 1;
			linux_process_reset(child);
			child = next;
			continue;
		}
		if (init != 0 && init != process) {
			linux_child_link(init, child);
		} else {
			child->ppid = 0;
		}
		child = next;
	}
}

static int linux_store_wait_status(u64 buffer, u64 exit_status)
{
	char status[4];
	const unsigned int value = (unsigned int)exit_status;

	for (u64 i = 0; i < sizeof(status); i++) {
		status[i] = (char)((value >> (i * 8)) & 0xff);
	}

	return buffer == 0 ||
		bunix_buffer_write(buffer, 0, status, sizeof(status)) == 0 ?
		0 : -1;
}

static long linux_wait4(struct linux_process *parent, long pid, u64 options,
			u64 status_buffer, u64 reply)
{
	struct linux_process *candidate = 0;

	if ((options & ~(LINUX_WNOHANG | LINUX_WUNTRACED |
			 LINUX_WCONTINUED)) != 0) {
		return -LINUX_EINVAL;
	}

	for (struct linux_process *child = parent->first_child;
	     child != 0;) {
		struct linux_process *next = child->next_sibling;

		if (!linux_child_matches(parent, child, pid)) {
			child = next;
			continue;
		}

		candidate = child;
		if (candidate->exited) {
			const u64 waited_pid = candidate->pid;
			if (linux_store_wait_status(status_buffer,
						    candidate->exit_status) != 0) {
				linux_debug_wait_log(parent, candidate,
						     "ready-fault", pid,
						     -LINUX_EFAULT);
				return -LINUX_EFAULT;
			}
			linux_debug_wait_log(parent, candidate, "ready", pid,
					     (long)waited_pid);
			candidate->waited = 1;
			linux_process_reset(candidate);
			return (long)waited_pid;
		}
		child = next;
	}

	if (candidate == 0) {
		linux_debug_wait_log(parent, 0, "no-child", pid,
				     -LINUX_ECHILD);
		return -LINUX_ECHILD;
	}

	if ((options & LINUX_WNOHANG) != 0) {
		linux_debug_wait_log(parent, candidate, "nohang", pid, 0);
		return 0;
	}

	if (reply == 0) {
		linux_debug_wait_log(parent, candidate, "no-reply", pid,
				     -LINUX_EINVAL);
		return -LINUX_EINVAL;
	}

	parent->waiter = reply;
	parent->wait_buffer = status_buffer;
	parent->wait_pid = (u64)pid;
	linux_debug_wait_log(parent, candidate, "block", pid,
			     LINUX_WAIT_BLOCK);
	return LINUX_WAIT_BLOCK;
}

static void linux_wake_parent(struct linux_process *child)
{
	struct linux_process *parent = linux_process_find_pid(child->ppid);
	const char wait4_ok[] = "linux-server: wait4\n";
	u64 waiter;
	u64 wait_buffer;

	if (parent == 0) {
		linux_debug_wait_log(0, child, "wake-no-parent", 0, 0);
		return;
	}
	if (parent->waiter == 0) {
		linux_debug_wait_log(parent, child, "wake-no-waiter",
				     (long)parent->wait_pid, 0);
		return;
	}
	if (!linux_child_matches(parent, child, (long)parent->wait_pid)) {
		linux_debug_wait_log(parent, child, "wake-mismatch",
				     (long)parent->wait_pid, 0);
		return;
	}
	waiter = parent->waiter;
	wait_buffer = parent->wait_buffer;

	struct bunix_msg reply = {
		.protocol = BUNIX_PROTO_LINUX,
		.type = BUNIX_LINUX_WAIT4,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { child->pid, 0, 0, 0 },
	};

	if (linux_store_wait_status(wait_buffer, child->exit_status) == 0) {
		child->waited = 1;
		linux_debug_wait_log(parent, child, "wake",
				     (long)parent->wait_pid,
				     (long)child->pid);
		linux_debug_rpc_log(wait4_ok, sizeof(wait4_ok) - 1);
		parent->waiter = 0;
		parent->wait_buffer = 0;
		parent->wait_pid = 0;
		if (wait_buffer != 0) {
			bunix_handle_close(wait_buffer);
		}
		linux_process_reset(child);
		bunix_ipc_send(waiter, &reply);
	} else {
		reply.words[0] = (u64)-LINUX_EFAULT;
		linux_debug_wait_log(parent, child, "wake-fault",
				     (long)parent->wait_pid,
				     -LINUX_EFAULT);
		parent->waiter = 0;
		parent->wait_buffer = 0;
		parent->wait_pid = 0;
		if (wait_buffer != 0) {
			bunix_handle_close(wait_buffer);
		}
		bunix_ipc_send(waiter, &reply);
	}
}

static void linux_process_exit_status(struct linux_process *process, u64 status,
				      u64 kill_task)
{
	if (process == 0 || process->exited) {
		return;
	}

	linux_tty_cancel_reader(&console_tty, process);
	linux_close_process_fds(process);
	linux_reparent_children(process);
	process->exited = 1;
	process->exit_status = status;
	if (process->session_owner && process->session_id != 0) {
		(void)linux_user_session_end(process->session_id);
		process->session_owner = 0;
	}
	notify_proc_exit(process->pid, process->exit_status, kill_task);
	if (process->ppid != 0) {
		(void)linux_signal_process(linux_process_find_pid(process->ppid),
					   LINUX_SIGCHLD);
	}
	linux_wake_parent(process);
}

static void linux_process_exit_code(struct linux_process *process, u64 status,
				    u64 kill_task)
{
	linux_process_exit_status(process, (status & 0xff) << 8, kill_task);
}

static u64 linux_signal_bit(u64 signal)
{
	return signal > 0 && signal < 64 ? 1ull << (signal - 1) : 0;
}

static int linux_signal_default_terminates(u64 signal)
{
	switch (signal) {
	case LINUX_SIGHUP:
	case LINUX_SIGINT:
	case LINUX_SIGBUS:
	case LINUX_SIGKILL:
	case LINUX_SIGSEGV:
	case LINUX_SIGPIPE:
	case LINUX_SIGALRM:
	case LINUX_SIGTERM:
		return 1;
	default:
		return 0;
	}
}

static int linux_signal_has_handler(const struct linux_process *process,
				    u64 signal)
{
	return process != 0 &&
	       process->signal_handlers[signal] != 0 &&
	       process->signal_restorers[signal] != 0;
}

static void linux_apply_pending_default_signals(struct linux_process *process)
{
	if (process == 0 || process->exited) {
		return;
	}
	for (u64 signal = 1; signal < 64; signal++) {
		const u64 bit = linux_signal_bit(signal);

		if ((process->pending_signals & bit) == 0 ||
		    (process->signal_mask & bit) != 0 ||
		    (process->signal_ignored & bit) != 0 ||
		    linux_signal_has_handler(process, signal) ||
		    !linux_signal_default_terminates(signal)) {
			continue;
		}
		linux_process_exit_status(process, signal, 1);
		return;
	}
}

static int linux_signal_process(struct linux_process *process, u64 signal)
{
	const u64 bit = linux_signal_bit(signal);

	if (process == 0 || process->exited || signal >= 64) {
		return 0;
	}
	if (signal == 0) {
		return 1;
	}

	if (bit == 0) {
		return 0;
	}
	if (signal == LINUX_SIGKILL) {
		linux_process_exit_status(process, signal, 1);
		return 1;
	}
	if ((process->signal_ignored & bit) != 0) {
		return 1;
	}
	process->pending_signals |= bit;
	if ((process->signal_mask & bit) != 0) {
		return 1;
	}
	if (!linux_signal_has_handler(process, signal) &&
	    linux_signal_default_terminates(signal)) {
		linux_process_exit_status(process, signal, 1);
	}

	return 1;
}

static long linux_alarm(struct linux_process *process, u64 seconds)
{
	const u64 now = bunix_clock_monotonic_ns();
	u64 previous = 0;

	if (process == 0) {
		return -LINUX_EINVAL;
	}
	if (process->alarm_active != 0 &&
	    process->alarm_deadline_ns > now) {
		previous = (process->alarm_deadline_ns - now +
			    999999999ull) / 1000000000ull;
	}
	if (seconds == 0) {
		process->alarm_active = 0;
		process->alarm_deadline_ns = 0;
		return (long)previous;
	}
	process->alarm_active = 1;
	process->alarm_deadline_ns = now + seconds * 1000000000ull;
	return (long)previous;
}

static long linux_setitimer_real(struct linux_process *process, u64 seconds,
				 u64 useconds)
{
	if (process == 0 || useconds >= 1000000ull) {
		return -LINUX_EINVAL;
	}
	if (seconds == 0 && useconds == 0) {
		process->alarm_active = 0;
		process->alarm_deadline_ns = 0;
		return 0;
	}
	process->alarm_active = 1;
	process->alarm_deadline_ns = bunix_clock_monotonic_ns() +
				     seconds * 1000000000ull +
				     useconds * 1000ull;
	return 0;
}

static int linux_alarm_expire_if_ready(struct linux_process *process)
{
	if (process == 0 || process->alarm_active == 0 ||
	    bunix_clock_monotonic_ns() < process->alarm_deadline_ns) {
		return 0;
	}
	process->alarm_active = 0;
	process->alarm_deadline_ns = 0;
	(void)linux_signal_process(process, LINUX_SIGALRM);
	return 1;
}

static u64 linux_task_fault_signal(const struct bunix_task_fault_event *event)
{
	const u64 fault_class = event->flags >> 32;

	if (fault_class == BUNIX_TASK_FAULT_CLASS_BUS || event->trap == 17) {
		return LINUX_SIGBUS;
	}
	return LINUX_SIGSEGV;
}

static long linux_task_fault(u64 task, u64 thread, u64 trap, u64 flags,
			     u64 cap, u64 cap_rights, u64 *handler,
			     u64 *restorer, u64 *old_mask)
{
	struct bunix_task_fault_event event = {
		.task = task,
		.thread = thread,
		.trap = trap,
		.flags = flags,
	};

	if (cap != 0 && (cap_rights & BUNIX_RIGHT_RECV) != 0) {
		if (bunix_buffer_read(cap, 0, &event, sizeof(event)) != 0) {
			return -LINUX_EINVAL;
		}
	} else if (cap != 0) {
		return -LINUX_EINVAL;
	}
	if (event.task == 0) {
		event.task = task;
	}

	struct linux_process *process = linux_process_find(event.task);
	if (process == 0) {
		return -LINUX_ESRCH;
	}

	const u64 signal = linux_task_fault_signal(&event);
	const u64 bit = linux_signal_bit(signal);
	if (bit != 0 && (process->signal_ignored & bit) == 0 &&
	    process->signal_handlers[signal] != 0 &&
	    process->signal_restorers[signal] != 0) {
		if (handler != 0) {
			*handler = process->signal_handlers[signal];
		}
		if (restorer != 0) {
			*restorer = process->signal_restorers[signal];
		}
		if (old_mask != 0) {
			*old_mask = process->signal_mask;
		}
		process->signal_mask |= bit;
		return (long)signal;
	}

	const u64 wait_status = signal | 0x80;
	char fault_log[256];
	u64 cursor = 0;

	append_text(fault_log, sizeof(fault_log), &cursor,
		    "linux-server: task fault pid=");
	append_dec(fault_log, sizeof(fault_log), &cursor, process->pid);
	append_text(fault_log, sizeof(fault_log), &cursor, " task=");
	append_dec(fault_log, sizeof(fault_log), &cursor, event.task);
	append_text(fault_log, sizeof(fault_log), &cursor, " signal=");
	append_dec(fault_log, sizeof(fault_log), &cursor, signal);
	append_text(fault_log, sizeof(fault_log), &cursor, " trap=");
	append_dec(fault_log, sizeof(fault_log), &cursor, event.trap);
	append_text(fault_log, sizeof(fault_log), &cursor, " error=");
	append_hex64(fault_log, sizeof(fault_log), &cursor, event.error);
	append_text(fault_log, sizeof(fault_log), &cursor, " ip=");
	append_hex64(fault_log, sizeof(fault_log), &cursor, event.ip);
	append_text(fault_log, sizeof(fault_log), &cursor, " addr=");
	append_hex64(fault_log, sizeof(fault_log), &cursor, event.fault_addr);
	append_text(fault_log, sizeof(fault_log), &cursor, "\n");
	bunix_console_log(fault_log, cursor);
	linux_process_exit_status(process, wait_status, 0);
	return 0;
}

static long linux_rt_sigaction(struct linux_process *process, u64 signal,
			       u64 handler, u64 flags, u64 restorer,
			       u64 *old_handler, u64 *old_flags,
			       u64 *old_restorer)
{
	const u64 bit = linux_signal_bit(signal);

	if (process == 0 || bit == 0 || signal == 9 || signal == 19) {
		return -LINUX_EINVAL;
	}
	if (old_handler != 0) {
		*old_handler = (process->signal_ignored & bit) != 0 ?
			       1 : process->signal_handlers[signal];
	}
	if (old_flags != 0) {
		*old_flags = process->signal_flags[signal];
	}
	if (old_restorer != 0) {
		*old_restorer = process->signal_restorers[signal];
	}
	if (handler != ~0ull) {
		if (handler == 1) {
			process->signal_ignored |= bit;
			process->signal_handlers[signal] = 0;
			process->signal_restorers[signal] = 0;
			process->signal_flags[signal] = flags;
		} else {
			process->signal_ignored &= ~bit;
			process->signal_handlers[signal] = handler;
			process->signal_restorers[signal] = restorer;
			process->signal_flags[signal] = flags;
		}
	}
	return 0;
}

static int linux_pgrp_exists_in_session(u64 pgid,
					const struct linux_process *source)
{
	for (u64 i = 0;; i++) {
		struct linux_process *process =
			(struct linux_process *)bunix_map_at(&process_by_pid,
							     i);

		if (process == 0) {
			break;
		}
		if (!process->exited &&
		    process->pgid == pgid &&
		    linux_same_session(source, process)) {
			return 1;
		}
	}
	return 0;
}

static void linux_tty_note_process(struct linux_tty *tty,
				   const struct linux_process *process)
{
	if (tty == 0 || process == 0) {
		return;
	}
	if (tty->session_id == 0) {
		linux_tty_bind_session(tty, process);
	}
	if (tty->foreground_pgid == 0 || process->pid == 1) {
		tty->foreground_pgid = process->pgid;
	}
}

static u64 linux_signal_deliverable(const struct linux_process *process)
{
	if (process == 0) {
		return 0;
	}
	for (u64 signal = 1; signal < 64; signal++) {
		const u64 bit = linux_signal_bit(signal);

		if ((process->pending_signals & bit) == 0 ||
		    (process->signal_mask & bit) != 0 ||
		    (process->signal_ignored & bit) != 0 ||
		    process->signal_handlers[signal] == 0 ||
		    process->signal_restorers[signal] == 0) {
			continue;
		}
		return signal;
	}
	return 0;
}

static long linux_signal_dequeue(struct linux_process *process,
				 u64 *handler, u64 *restorer,
				 u64 *old_mask)
{
	const u64 signal = linux_signal_deliverable(process);
	const u64 bit = linux_signal_bit(signal);

	if (signal == 0 || bit == 0) {
		return 0;
	}
	process->pending_signals &= ~bit;
	if (handler != 0) {
		*handler = process->signal_handlers[signal];
	}
	if (restorer != 0) {
		*restorer = process->signal_restorers[signal];
	}
	if (old_mask != 0) {
		*old_mask = process->signal_mask;
	}
	process->signal_mask |= bit;
	return (long)signal;
}

static long linux_rt_sigprocmask(struct linux_process *process, u64 how,
				 u64 set, u64 *old_set)
{
	if (process == 0) {
		return -LINUX_EINVAL;
	}
	if (old_set != 0) {
		*old_set = process->signal_mask;
	}

	if (how == ~0ull) {
		return 0;
	} else if (how == 0) {
		process->signal_mask |= set;
	} else if (how == 1) {
		process->signal_mask &= ~set;
	} else if (how == 2) {
		process->signal_mask = set;
	} else {
		return -LINUX_EINVAL;
	}
	process->signal_mask &= ~(linux_signal_bit(LINUX_SIGKILL) |
				  linux_signal_bit(LINUX_SIGSTOP));
	linux_apply_pending_default_signals(process);
	return 0;
}

static long linux_rt_sigtimedwait(struct linux_process *process, u64 set,
				  u64 has_timeout)
{
	u64 pending;

	if (process == 0) {
		return -LINUX_EINVAL;
	}
	pending = process->pending_signals & set;
	if (pending == 0) {
		return has_timeout ? -LINUX_EAGAIN : -LINUX_ENOSYS;
	}
	for (u64 signal = 1; signal < 64; signal++) {
		const u64 bit = linux_signal_bit(signal);

		if ((pending & bit) != 0) {
			process->pending_signals &= ~bit;
			return (long)signal;
		}
	}
	return -LINUX_EAGAIN;
}

static u64 linux_foreground_pgid(const struct linux_process *process)
{
	if (!linux_tty_session_matches(&console_tty, process)) {
		return 0;
	}
	return console_tty.foreground_pgid;
}

static long linux_set_foreground_pgid(struct linux_process *process, u64 pgid)
{
	if (process == 0 || pgid == 0) {
		return -LINUX_EINVAL;
	}
	if (!linux_tty_session_matches(&console_tty, process)) {
		return -LINUX_ENOTTY;
	}
	if (!linux_pgrp_exists_in_session(pgid, process)) {
		return -LINUX_EPERM;
	}
	linux_tty_bind_session(&console_tty, process);
	console_tty.foreground_pgid = pgid;
	if (process->session_id != 0) {
		(void)linux_user_session_set_foreground(process->session_id,
							pgid);
	}
	return 0;
}

static int linux_signal_pgrp(struct linux_process *source, u64 pgid, u64 signal)
{
	int delivered = 0;

	for (u64 i = 0;; i++) {
		struct linux_process *process =
			(struct linux_process *)bunix_map_at(&process_by_pid,
							     i);

		if (process == 0) {
			break;
		}
		if (!process->exited &&
		    process->pgid == pgid &&
		    (source == 0 || linux_same_session(source, process))) {
			delivered |= linux_signal_process(process, signal);
		}
	}

	return delivered;
}

static long linux_kill(struct linux_process *source, long pid, u64 signal)
{
	if (signal >= 64) {
		return -LINUX_EINVAL;
	}
	if (pid > 0) {
		struct linux_process *target = linux_process_find_pid((u64)pid);

		if (target == 0 || target->exited) {
			return -LINUX_ESRCH;
		}
		return linux_signal_process(target, signal) ? 0 : -LINUX_ESRCH;
	}
	if (pid == 0) {
		return linux_signal_pgrp(source, source->pgid, signal) ?
		       0 : -LINUX_ESRCH;
	}
	if (pid < -1) {
		return linux_signal_pgrp(source, (u64)(-pid), signal) ?
		       0 : -LINUX_ESRCH;
	}
	return -LINUX_EINVAL;
}

static long linux_user_credential(struct linux_process *process, u64 type)
{
	u64 request_type;
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_USER,
		.type = 0,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { 0, 0, 0, 0 },
	};
	struct bunix_msg reply;

	if (type == BUNIX_LINUX_GETUID) {
		request_type = BUNIX_USER_GETUID;
	} else if (type == BUNIX_LINUX_GETGID) {
		request_type = BUNIX_USER_GETGID;
	} else if (type == BUNIX_LINUX_GETEUID) {
		request_type = BUNIX_USER_GETEUID;
	} else if (type == BUNIX_LINUX_GETEGID) {
		request_type = BUNIX_USER_GETEGID;
	} else {
		return -LINUX_EINVAL;
	}

	if (process == 0 || linux_user_service() == 0) {
		return -LINUX_ESRCH;
	}

	request.type = request_type;
	request.words[0] = process->bunix_task;
	if (bunix_ipc_call(user_service, &request, &reply) != 0) {
		return -LINUX_EIO;
	}
	if (reply.words[0] != 0) {
		return -LINUX_EIO;
	}

	return (long)reply.words[1];
}

static long linux_user_groups(struct linux_process *process, u64 max_groups,
			      u64 *group0, u64 *group1)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_USER,
		.type = BUNIX_USER_GETGROUPS,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { 0, max_groups, 0, 0 },
	};
	struct bunix_msg reply;

	if (process == 0 || LINUX_HANDLE_USER_MGMT == 0) {
		return -LINUX_ESRCH;
	}

	request.words[0] = process->bunix_task;
	if (bunix_ipc_call(LINUX_HANDLE_USER_MGMT, &request, &reply) != 0) {
		return -LINUX_EIO;
	}
	if (reply.words[0] != 0) {
		return -LINUX_EIO;
	}

	if (group0 != 0) {
		*group0 = reply.words[2];
	}
	if (group1 != 0) {
		*group1 = reply.words[3];
	}
	return (long)reply.words[1];
}

static long linux_user_groups_buffer(struct linux_process *process,
				     u64 max_groups, u64 buffer)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_USER,
		.type = BUNIX_USER_GETGROUPS_BUFFER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP,
		.reply = 0,
		.cap = buffer,
		.words = { 0, max_groups, 0, 0 },
	};
	struct bunix_msg reply;

	if (process == 0 || LINUX_HANDLE_USER_MGMT == 0) {
		return -LINUX_ESRCH;
	}

	request.words[0] = process->bunix_task;
	if (bunix_ipc_call(LINUX_HANDLE_USER_MGMT, &request, &reply) != 0) {
		return -LINUX_EIO;
	}
	if (reply.words[0] != 0) {
		return -LINUX_EIO;
	}
	return (long)reply.words[1];
}

static long linux_user_setres(struct linux_process *process, u64 type,
			      u64 real, u64 effective, u64 saved)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_USER,
		.type = type,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { 0, real, effective, saved },
	};
	struct bunix_msg reply;

	if (process == 0 || LINUX_HANDLE_USER_MGMT == 0) {
		return -LINUX_ESRCH;
	}

	request.words[0] = process->bunix_task;
	if (bunix_ipc_call(LINUX_HANDLE_USER_MGMT, &request, &reply) != 0) {
		return -LINUX_EIO;
	}
	return reply.words[0] == 0 ? 0 : -LINUX_EPERM;
}

static long linux_user_setgroups(struct linux_process *process, u64 count,
				 u64 group0, u64 group1)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_USER,
		.type = BUNIX_USER_SETGROUPS,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { 0, count, group0, group1 },
	};
	struct bunix_msg reply;

	if (count > 2) {
		return -LINUX_EINVAL;
	}
	if (process == 0 || LINUX_HANDLE_USER_MGMT == 0) {
		return -LINUX_ESRCH;
	}

	request.words[0] = process->bunix_task;
	if (bunix_ipc_call(LINUX_HANDLE_USER_MGMT, &request, &reply) != 0) {
		return -LINUX_EIO;
	}
	return reply.words[0] == 0 ? 0 : -LINUX_EPERM;
}

static long linux_user_setgroups_buffer(struct linux_process *process,
					u64 count, u64 buffer)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_USER,
		.type = BUNIX_USER_SETGROUPS_BUFFER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_RECV | BUNIX_RIGHT_DUP,
		.reply = 0,
		.cap = buffer,
		.words = { 0, count, 0, 0 },
	};
	struct bunix_msg reply;

	if (process == 0 || LINUX_HANDLE_USER_MGMT == 0) {
		return -LINUX_ESRCH;
	}

	request.words[0] = process->bunix_task;
	if (bunix_ipc_call(LINUX_HANDLE_USER_MGMT, &request, &reply) != 0) {
		return -LINUX_EIO;
	}
	return reply.words[0] == 0 ? 0 : -LINUX_EPERM;
}

static long linux_getpgid(struct linux_process *process, u64 pid)
{
	struct linux_process *target = pid == 0 ? process :
				       linux_process_find_pid(pid);

	if (target == 0) {
		return -LINUX_ESRCH;
	}
	return (long)target->pgid;
}

static long linux_getsid(struct linux_process *process, u64 pid)
{
	struct linux_process *target = pid == 0 ? process :
				       linux_process_find_pid(pid);

	if (target == 0) {
		return -LINUX_ESRCH;
	}
	return (long)target->sid;
}

static long linux_umask(struct linux_process *process, u64 mask)
{
	const u64 old = process->umask;

	process->umask = mask & 0777;
	return (long)old;
}

static long linux_setpgid(struct linux_process *process, u64 pid, u64 pgid)
{
	struct linux_process *target = pid == 0 ? process :
				       linux_process_find_pid(pid);
	struct linux_process *leader;

	if (target == 0) {
		return -LINUX_ESRCH;
	}
	if (target != process &&
	    target->parent != process) {
		return -LINUX_ESRCH;
	}
	if (!linux_same_session(process, target)) {
		return -LINUX_ESRCH;
	}
	if (target->sid == target->pid) {
		return -LINUX_EPERM;
	}
	if (pgid == 0) {
		pgid = target->pid;
	}
	leader = linux_process_find_pid(pgid);
	if (leader != 0 &&
	    leader != target &&
	    (leader->pgid != pgid || !linux_same_session(target, leader))) {
		return -LINUX_EPERM;
	}
	target->pgid = pgid;
	return 0;
}

static long linux_setsid(struct linux_process *process)
{
	if (process == 0) {
		return -LINUX_ESRCH;
	}
	if (linux_pgrp_exists(process->pid)) {
		return -LINUX_EPERM;
	}

	process->sid = process->pid;
	process->pgid = process->pid;
	if (linux_tty_session_matches(&console_tty, process)) {
		console_tty.foreground_pgid = process->pgid;
	}
	return (long)process->sid;
}

static void linux_tty_init(struct linux_tty *tty)
{
	if (tty == 0 || tty->termios[LINUX_TERM_CC + LINUX_VMIN] != 0) {
		return;
	}

	zero_bytes(tty->termios, sizeof(tty->termios));
	store_u32(tty->termios, LINUX_TERM_IFLAG, LINUX_ICRNL);
	store_u32(tty->termios, LINUX_TERM_OFLAG,
		  LINUX_OPOST | LINUX_ONLCR);
	store_u32(tty->termios, LINUX_TERM_CFLAG,
		  LINUX_CS8 | LINUX_CREAD);
	store_u32(tty->termios, LINUX_TERM_LFLAG,
		  LINUX_ISIG | LINUX_ICANON | LINUX_ECHO |
		  LINUX_ECHOE | LINUX_ECHOK);
	tty->termios[LINUX_TERM_LINE] = 0;
	tty->termios[LINUX_TERM_CC + LINUX_VINTR] = 3;
	tty->termios[LINUX_TERM_CC + LINUX_VERASE] = 0x7f;
	tty->termios[LINUX_TERM_CC + LINUX_VEOF] = 4;
	tty->termios[LINUX_TERM_CC + LINUX_VMIN] = 1;
	tty->termios[LINUX_TERM_CC + LINUX_VTIME] = 0;
}

static unsigned int linux_tty_lflag(struct linux_tty *tty)
{
	linux_tty_init(tty);
	return tty != 0 ? load_u32(tty->termios, LINUX_TERM_LFLAG) : 0;
}

static unsigned int linux_tty_iflag(struct linux_tty *tty)
{
	linux_tty_init(tty);
	return tty != 0 ? load_u32(tty->termios, LINUX_TERM_IFLAG) : 0;
}

static void tty_echo(const char *text, u64 len)
{
	(void)bunix_console_write(text, len);
}

static void linux_tty_queue_cursor_position_report(struct linux_tty *tty)
{
	static const char report[] = "\033[1;1R";

	for (u64 i = sizeof(report) - 1; i != 0; i--) {
		(void)linux_tty_read_queue_push_front(tty, report[i - 1]);
	}
	linux_tty_wake_reader(tty);
}

static void linux_tty_output_event(struct linux_tty *tty, char c)
{
	static const char query[] = "\033[6n";

	if (tty == 0) {
		return;
	}

	if (c == query[tty->cpr_state]) {
		tty->cpr_state++;
		if (tty->cpr_state == sizeof(query) - 1) {
			tty->cpr_state = 0;
			linux_tty_queue_cursor_position_report(tty);
		}
		return;
	}
	tty->cpr_state = c == query[0] ? 1 : 0;
}

static int linux_tty_read_queue_push(struct linux_tty *tty, char c)
{
	if (tty == 0 || tty->read_queue_len >= sizeof(tty->read_queue)) {
		return 0;
	}
	tty->read_queue[(tty->read_queue_start + tty->read_queue_len) %
			sizeof(tty->read_queue)] = c;
	tty->read_queue_len++;
	return 1;
}

static int linux_tty_read_queue_push_front(struct linux_tty *tty, char c)
{
	if (tty == 0 || tty->read_queue_len >= sizeof(tty->read_queue)) {
		return 0;
	}
	tty->read_queue_start =
		(tty->read_queue_start + sizeof(tty->read_queue) - 1) %
		sizeof(tty->read_queue);
	tty->read_queue[tty->read_queue_start] = c;
	tty->read_queue_len++;
	return 1;
}

static int linux_tty_can_read(struct linux_tty *tty)
{
	return tty != 0 && tty->read_queue_len != 0;
}

static long linux_tty_read_available(struct linux_tty *tty, u64 len,
				     u64 buffer)
{
	u64 nread;

	if (tty == 0 || buffer == 0) {
		return -LINUX_EBADF;
	}
	if (tty->read_queue_len == 0) {
		return -LINUX_EAGAIN;
	}
	nread = len < tty->read_queue_len ? len : tty->read_queue_len;
	for (u64 i = 0; i < nread; i++) {
		write_buffer[i] = tty->read_queue[(tty->read_queue_start + i) %
						  sizeof(tty->read_queue)];
	}
	tty->read_queue_start =
		(tty->read_queue_start + nread) % sizeof(tty->read_queue);
	tty->read_queue_len -= nread;
	if (bunix_buffer_write(buffer, 0, write_buffer, nread) != 0) {
		return -LINUX_EFAULT;
	}
	return (long)nread;
}

static void linux_tty_wake_reader(struct linux_tty *tty)
{
	struct bunix_msg reply = {
		.protocol = BUNIX_PROTO_LINUX,
		.type = BUNIX_LINUX_READ,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { 0, 0, 0, 0 },
	};
	u64 reply_handle;
	u64 buffer;
	long nread;

	if (tty == 0 || tty->reader_reply == 0 || tty->read_queue_len == 0) {
		return;
	}

	reply_handle = tty->reader_reply;
	buffer = tty->reader_buffer;
	tty->reader_pid = 0;
	tty->reader_reply = 0;
	tty->reader_buffer = 0;
	nread = linux_tty_read_available(tty, tty->reader_len, buffer);
	tty->reader_len = 0;
	reply.words[0] = (u64)nread;
	(void)bunix_ipc_send(reply_handle, &reply);
	if (buffer != 0) {
		bunix_handle_close(buffer);
	}
}

static void linux_tty_cancel_reader(struct linux_tty *tty,
				    struct linux_process *process)
{
	if (tty == 0 || process == 0 || tty->reader_pid != process->pid) {
		return;
	}
	if (tty->reader_buffer != 0) {
		bunix_handle_close(tty->reader_buffer);
	}
	tty->reader_pid = 0;
	tty->reader_reply = 0;
	tty->reader_buffer = 0;
	tty->reader_len = 0;
}

static long linux_tty_termios_get(struct linux_tty *tty, u64 buffer)
{
	linux_tty_init(tty);
	if (tty == 0) {
		return -LINUX_EBADF;
	}
	return bunix_buffer_write(buffer, 0, tty->termios,
				  sizeof(tty->termios)) == 0 ?
	       0 : -LINUX_EFAULT;
}

static long linux_tty_termios_set(struct linux_tty *tty, u64 buffer)
{
	if (tty == 0 || buffer == 0 ||
	    bunix_buffer_read(buffer, 0, tty->termios,
			      sizeof(tty->termios)) != 0) {
		return -LINUX_EFAULT;
	}
	tty->line_len = 0;
	if (tty->termios[LINUX_TERM_CC + LINUX_VMIN] == 0) {
		tty->termios[LINUX_TERM_CC + LINUX_VMIN] = 1;
	}
	return 0;
}

static void linux_tty_interrupt_foreground(struct linux_tty *tty)
{
	struct linux_process *reader =
		tty != 0 && tty->reader_pid != 0 ?
		linux_process_find_pid(tty->reader_pid) : 0;

	if (reader != 0) {
		(void)linux_signal_process(reader, LINUX_SIGINT);
		return;
	}

	for (u64 i = 0;; i++) {
		struct linux_process *process =
			(struct linux_process *)bunix_map_at(&process_by_pid,
							     i);

		if (process == 0) {
			break;
		}
		if (process->exited || process->waiter == 0) {
			continue;
		}
		for (struct linux_process *child = process->first_child;
		     child != 0; child = child->next_sibling) {
			if (!child->exited &&
			    linux_child_matches(process, child,
						(long)process->wait_pid)) {
				(void)linux_signal_process(child, LINUX_SIGINT);
				return;
			}
		}
	}

	if (tty != 0 && tty->foreground_pgid != 0) {
		(void)linux_signal_pgrp(0, tty->foreground_pgid, LINUX_SIGINT);
	}
}

static void linux_tty_input_event(struct linux_tty *tty, char c)
{
	const unsigned int lflag = linux_tty_lflag(tty);
	const unsigned int iflag = linux_tty_iflag(tty);
	const char intr = tty != 0 ? tty->termios[LINUX_TERM_CC + LINUX_VINTR] :
			    0;

	if (tty == 0) {
		return;
	}

	if (c == '\r' && (iflag & LINUX_ICRNL) != 0) {
		c = '\n';
	}

	if ((lflag & LINUX_ISIG) != 0 && c == intr) {
		if ((lflag & LINUX_ECHO) != 0) {
			tty_echo("^C\n", 3);
		}
		tty->line_len = 0;
		linux_tty_interrupt_foreground(tty);
		return;
	}

	if ((lflag & LINUX_ICANON) == 0) {
		if ((lflag & LINUX_ECHO) != 0) {
			tty_echo(&c, 1);
		}
		(void)linux_tty_read_queue_push(tty, c);
		linux_tty_wake_reader(tty);
		return;
	}

	if ((c == tty->termios[LINUX_TERM_CC + LINUX_VERASE] || c == '\b') &&
	    tty->line_len != 0) {
		tty->line_len--;
		if ((lflag & LINUX_ECHO) != 0) {
			tty_echo("\b \b", 3);
		}
		return;
	}
	if ((unsigned char)c < 0x20 && c != '\n' &&
	    c != tty->termios[LINUX_TERM_CC + LINUX_VEOF]) {
		return;
	}
	if (c != tty->termios[LINUX_TERM_CC + LINUX_VEOF]) {
		if (c == '\n' && tty->line_len < sizeof(tty->line)) {
			tty->line[tty->line_len++] = c;
			if ((lflag & LINUX_ECHO) != 0) {
				tty_echo(&c, 1);
			}
		} else if (c != '\n' &&
			   tty->line_len + 1 < sizeof(tty->line)) {
			tty->line[tty->line_len++] = c;
			if ((lflag & LINUX_ECHO) != 0) {
				tty_echo(&c, 1);
			}
		}
	}
	if (c != '\n' && c != tty->termios[LINUX_TERM_CC + LINUX_VEOF]) {
		return;
	}

	for (u64 i = 0; i < tty->line_len; i++) {
		if (!linux_tty_read_queue_push(tty, tty->line[i])) {
			break;
		}
	}
	tty->line_len = 0;
	linux_tty_wake_reader(tty);
}

static void linux_tty_input_buffer_event(struct linux_tty *tty, u64 buffer,
					 u64 len)
{
	enum {
		TTY_INPUT_CHUNK = 128,
	};
	char input[TTY_INPUT_CHUNK];
	u64 done = 0;

	if (tty == 0 || buffer == 0) {
		return;
	}
	while (done < len) {
		u64 chunk = len - done;

		if (chunk > sizeof(input)) {
			chunk = sizeof(input);
		}
		if (bunix_buffer_read(buffer, done, input, chunk) != 0) {
			break;
		}
		for (u64 i = 0; i < chunk; i++) {
			linux_tty_input_event(tty, input[i]);
		}
		done += chunk;
	}
}

static long linux_tty_read(struct linux_tty *tty, struct linux_process *process,
			   u64 len, u64 buffer, u64 reply_handle,
			   int *blocked)
{
	long nread;

	if (tty == 0 || process == 0) {
		return -LINUX_EBADF;
	}
	if (len == 0) {
		return 0;
	}
	if (linux_tty_session_matches(tty, process)) {
		linux_tty_bind_session(tty, process);
	}
	nread = linux_tty_read_available(tty, len, buffer);
	if (nread != -(long)LINUX_EAGAIN) {
		return nread;
	}
	if (reply_handle == 0 || tty->reader_reply != 0) {
		return -LINUX_EAGAIN;
	}
	tty->reader_pid = process->pid;
	tty->reader_reply = reply_handle;
	tty->reader_buffer = buffer;
	tty->reader_len = len;
	if (blocked != 0) {
		*blocked = 1;
	}
	return 0;
}

static long linux_tty_write_buffer(struct linux_tty *tty, u64 len, u64 buffer)
{
	char tty_write_buffer[256];

	if (tty == 0) {
		return -LINUX_EBADF;
	}
	if (buffer == 0) {
		return -LINUX_EFAULT;
	}

	for (u64 done = 0; done < len;) {
		const u64 chunk = len - done > sizeof(tty_write_buffer) ?
				  sizeof(tty_write_buffer) : len - done;

		if (bunix_buffer_read(buffer, done, tty_write_buffer,
				      chunk) != 0) {
			return done != 0 ? (long)done : -(long)LINUX_EFAULT;
		}
		for (u64 i = 0; i < chunk; i++) {
			linux_tty_output_event(tty, tty_write_buffer[i]);
		}
		bunix_console_write((const char *)tty_write_buffer, chunk);
		done += chunk;
	}
	return (long)len;
}

static int ifreq_name_eq(const char *ifreq, const char *name)
{
	for (u64 i = 0; i < LINUX_IFNAMSIZ; i++) {
		if (name[i] == '\0') {
			return ifreq[i] == '\0';
		}
		if (ifreq[i] != name[i]) {
			return 0;
		}
	}
	return 1;
}

static int ifreq_eth_index(const char *ifreq, u64 *index)
{
	u64 value = 0;
	u64 pos = 3;

	if (ifreq == 0 || index == 0 ||
	    ifreq[0] != 'e' || ifreq[1] != 't' || ifreq[2] != 'h' ||
	    pos >= LINUX_IFNAMSIZ ||
	    ifreq[pos] < '0' || ifreq[pos] > '9') {
		return 0;
	}
	while (pos < LINUX_IFNAMSIZ &&
	       ifreq[pos] >= '0' && ifreq[pos] <= '9') {
		value = value * 10 + (u64)(ifreq[pos] - '0');
		pos++;
	}
	if (pos < LINUX_IFNAMSIZ && ifreq[pos] != '\0') {
		return 0;
	}
	*index = value;
	return 1;
}

static int linux_net_request(u64 type, u64 cap, unsigned int cap_rights,
			     u64 word0, u64 word1, u64 word2, u64 word3,
			     struct bunix_msg *reply)
{
	const u64 net = linux_cached_service(BUNIX_SERVICE_NET, &net_service);
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_NET,
		.type = (unsigned int)type,
		.sender = 0,
		.cap_rights = cap_rights,
		.reply = 0,
		.cap = cap,
		.words = { word0, word1, word2, word3 },
	};

	return net != 0 && reply != 0 &&
		       bunix_ipc_call(net, &request, reply) == 0 &&
		       reply->words[0] == 0 ?
	       0 :
	       -1;
}

static long linux_net_interface_id_by_name(const char *ifreq)
{
	struct bunix_msg reply;
	u64 count;
	u64 target;
	u64 eth_index = 0;

	if (ifreq_name_eq(ifreq, "lo")) {
		return 1;
	}
	if (!ifreq_eth_index(ifreq, &target)) {
		return -LINUX_ENODEV;
	}
	if (linux_net_request(BUNIX_NET_INTERFACE_COUNT, 0, 0, 0, 0, 0, 0,
			      &reply) != 0) {
		return -LINUX_ENODEV;
	}
	count = reply.words[1];
	for (u64 i = 0; i < count; i++) {
		if (linux_net_request(BUNIX_NET_INTERFACE_AT, 0, 0, i, 0, 0, 0,
				      &reply) != 0) {
			return -LINUX_ENODEV;
		}
		if (reply.words[1] != 1) {
			if (eth_index == target) {
				return (long)reply.words[1];
			}
			eth_index++;
		}
	}
	return -LINUX_ENODEV;
}

static long linux_net_interface_details(u64 iface,
					struct bunix_net_packet_interface_info *info)
{
	struct bunix_msg reply;
	long buffer;
	int result;

	if (info == 0) {
		return -LINUX_EFAULT;
	}
	buffer = bunix_buffer_create(sizeof(*info));
	if (buffer <= 0) {
		return -LINUX_ENOMEM;
	}
	result = linux_net_request(BUNIX_NET_INTERFACE_DETAILS, (u64)buffer,
				   BUNIX_RIGHT_SEND, iface, 0, 0, 0,
				   &reply) == 0 &&
			 bunix_buffer_read((u64)buffer, 0, info,
					   sizeof(*info)) == 0 ?
		 0 :
		 -((int)LINUX_ENODEV);
	bunix_handle_close((u64)buffer);
	return result;
}

static long linux_net_default_packet_interface(void)
{
	struct bunix_msg reply;
	u64 count;

	if (linux_net_request(BUNIX_NET_INTERFACE_COUNT, 0, 0, 0, 0, 0, 0,
			      &reply) != 0) {
		return -LINUX_ENODEV;
	}
	count = reply.words[1];
	for (u64 i = 0; i < count; i++) {
		struct bunix_net_packet_interface_info info;

		if (linux_net_request(BUNIX_NET_INTERFACE_AT, 0, 0, i, 0, 0, 0,
				      &reply) != 0) {
			return -LINUX_ENODEV;
		}
		if (reply.words[1] == 1) {
			continue;
		}
		if (linux_net_interface_details(reply.words[1], &info) == 0 &&
		    (info.flags & (BUNIX_NET_IFACE_FLAG_UP |
				   BUNIX_NET_IFACE_FLAG_RUNNING)) ==
			    (BUNIX_NET_IFACE_FLAG_UP |
			     BUNIX_NET_IFACE_FLAG_RUNNING)) {
			return (long)info.id;
		}
	}
	return -LINUX_ENODEV;
}

static unsigned int linux_ifflags_from_net(u64 flags)
{
	unsigned int out = 0;

	if ((flags & BUNIX_NET_IFACE_FLAG_UP) != 0) {
		out |= LINUX_IFF_UP;
	}
	if ((flags & BUNIX_NET_IFACE_FLAG_BROADCAST) != 0) {
		out |= LINUX_IFF_BROADCAST;
	}
	if ((flags & BUNIX_NET_IFACE_FLAG_LOOPBACK) != 0) {
		out |= LINUX_IFF_LOOPBACK;
	}
	if ((flags & BUNIX_NET_IFACE_FLAG_RUNNING) != 0) {
		out |= LINUX_IFF_RUNNING;
	}
	return out;
}

static long linux_set_ifflags(u64 iface, unsigned int flags)
{
	struct bunix_msg reply;
	u64 set = 0;
	u64 clear = 0;

	if ((flags & LINUX_IFF_UP) != 0) {
		set |= BUNIX_NET_IFACE_FLAG_UP;
	} else {
		clear |= BUNIX_NET_IFACE_FLAG_UP;
	}
	if ((flags & LINUX_IFF_BROADCAST) != 0) {
		set |= BUNIX_NET_IFACE_FLAG_BROADCAST;
	} else {
		clear |= BUNIX_NET_IFACE_FLAG_BROADCAST;
	}
	return iface == 1 ||
		       linux_net_request(BUNIX_NET_INTERFACE_SET_FLAGS, 0, 0,
					 iface, set, clear, 0, &reply) == 0 ?
	       0 : -LINUX_EINVAL;
}

static void linux_store_mac(char *ifreq, u64 mac_hi)
{
	for (u64 i = 0; i < 6; i++) {
		ifreq[18 + i] = (char)((mac_hi >> ((5 - i) * 8)) & 0xff);
	}
}

static void linux_store_ether_mac(unsigned char *out, u64 mac_hi)
{
	for (u64 i = 0; i < 6; i++) {
		out[i] = (unsigned char)((mac_hi >> ((5 - i) * 8)) & 0xff);
	}
}

static void linux_net_interface_name(u64 iface, char *name, u64 size)
{
	u64 eth_index = 0;
	struct bunix_msg reply;
	u64 count;

	if (name == 0 || size == 0) {
		return;
	}
	zero_bytes(name, size);
	if (iface == 1) {
		copy_field(name, 0, size, "lo");
		return;
	}
	if (linux_net_request(BUNIX_NET_INTERFACE_COUNT, 0, 0, 0, 0, 0, 0,
			      &reply) != 0) {
		return;
	}
	count = reply.words[1];
	for (u64 i = 0; i < count; i++) {
		if (linux_net_request(BUNIX_NET_INTERFACE_AT, 0, 0, i, 0, 0, 0,
				      &reply) != 0) {
			return;
		}
		if (reply.words[1] == 1) {
			continue;
		}
		if (reply.words[1] == iface) {
			u64 divisor = 1;
			u64 cursor = 3;

			copy_field(name, 0, size, "eth");
			while (eth_index / divisor >= 10) {
				divisor *= 10;
			}
			while (divisor != 0 && cursor + 1 < size) {
				name[cursor++] =
					(char)('0' + (eth_index / divisor) % 10);
				divisor /= 10;
			}
			return;
		}
		eth_index++;
	}
}

static long linux_net_ioctl(u64 request, u64 buffer)
{
	char ifreq[LINUX_IFREQ_SIZE];
	struct bunix_net_packet_interface_info info;
	long iface;
	long result;

	if (buffer == 0 ||
	    bunix_buffer_read(buffer, 0, ifreq, sizeof(ifreq)) != 0) {
		return -LINUX_EFAULT;
	}
	ifreq[LINUX_IFNAMSIZ - 1] = '\0';
	iface = linux_net_interface_id_by_name(ifreq);
	if (iface < 0) {
		return iface;
	}
	result = linux_net_interface_details((u64)iface, &info);
	if (result != 0) {
		return result;
	}
	switch (request) {
	case LINUX_SIOCGIFINDEX:
		store_u32(ifreq, 16, (unsigned int)info.id);
		break;
	case LINUX_SIOCGIFFLAGS:
		store_u16(ifreq, 16, linux_ifflags_from_net(info.flags));
		break;
	case LINUX_SIOCSIFFLAGS:
		return linux_set_ifflags((u64)iface, load_u16(ifreq, 16));
	case LINUX_SIOCGIFMTU:
		store_u32(ifreq, 16, (unsigned int)info.mtu);
		break;
	case LINUX_SIOCGIFHWADDR:
		zero_bytes(ifreq + 16, 16);
		store_u16(ifreq, 16, info.id == 1 ? LINUX_ARPHRD_LOOPBACK :
			  LINUX_ARPHRD_ETHER);
		linux_store_mac(ifreq, info.mac_hi);
		break;
	case LINUX_SIOCGIFTXQLEN:
		store_u32(ifreq, 16, 1000);
		break;
	default:
		return -LINUX_EINVAL;
	}
	return bunix_buffer_write(buffer, 0, ifreq, sizeof(ifreq)) == 0 ?
	       0 : -LINUX_EFAULT;
}

static long linux_ioctl(struct linux_process *process, u64 fd, u64 request,
			u64 value, u64 buffer)
{
	char data[8];
	u64 pgid;

	if (fd >= process->fd_capacity || process->fds[fd].kind == 0) {
		linux_debug_openrc_state_log_fd(process, 0, fd, "ioctl-badfd",
						-LINUX_EBADF, request, value);
		return -LINUX_EBADF;
	}
	if (process->fds[fd].kind == LINUX_FD_SOCKET) {
		const long result = linux_net_ioctl(request, buffer);

		linux_debug_openrc_state_log_fd(process, &process->fds[fd],
						fd, "ioctl-socket", result,
						request, value);
		return result;
	}
	if (process->fds[fd].kind != LINUX_FD_CONSOLE) {
		linux_debug_openrc_state_log_fd(process, &process->fds[fd],
						fd, "ioctl-nontty",
						-LINUX_ENOTTY, request, value);
		return -LINUX_ENOTTY;
	}

	switch (request) {
	case LINUX_TCGETS:
		return linux_tty_termios_get(&console_tty, buffer);
	case LINUX_TCSETS:
	case LINUX_TCSETSW:
	case LINUX_TCSETSF:
		return linux_tty_termios_set(&console_tty, buffer);
	case LINUX_TIOCGPGRP:
		if (buffer == 0) {
			return -LINUX_EFAULT;
		}
		pgid = linux_foreground_pgid(process);
		if (pgid == 0) {
			return -LINUX_ENOTTY;
		}
		zero_bytes(data, sizeof(data));
		store_u32(data, 0, (unsigned int)pgid);
		return bunix_buffer_write(buffer, 0, data, 4) == 0 ?
		       0 : -LINUX_EFAULT;
	case LINUX_TIOCSPGRP:
		pgid = value;
		return linux_set_foreground_pgid(process, pgid);
	case LINUX_TIOCGWINSZ:
		if (buffer == 0) {
			return -LINUX_EFAULT;
		}
		zero_bytes(data, sizeof(data));
		store_u16(data, 0, 25);
		store_u16(data, 2, 80);
		return bunix_buffer_write(buffer, 0, data, 8) == 0 ?
		       0 : -LINUX_EFAULT;
	default:
		linux_debug_openrc_state_log_fd(process, &process->fds[fd],
						fd, "ioctl-console",
						-LINUX_EINVAL, request, value);
		return -LINUX_EINVAL;
	}
}

static long linux_vfs_error(u64 status)
{
	if (status == BUNIX_VFS_ERR_ACCESS) {
		return -LINUX_EACCES;
	}
	if (status == BUNIX_VFS_ERR_NOTDIR) {
		return -LINUX_ENOTDIR;
	}
	if (status == BUNIX_VFS_ERR_ISDIR) {
		return -LINUX_EISDIR;
	}
	if (status == BUNIX_VFS_ERR_EXIST) {
		return -LINUX_EEXIST;
	}
	if (status == BUNIX_VFS_ERR_NOTEMPTY) {
		return -LINUX_ENOTEMPTY;
	}
	if (status == BUNIX_VFS_ERR_XDEV) {
		return -LINUX_EXDEV;
	}
	if (status == BUNIX_VFS_ERR_INVAL) {
		return -LINUX_EINVAL;
	}
	if (status == BUNIX_VFS_ERR_BUSY) {
		return -LINUX_EBUSY;
	}
	if (status == BUNIX_VFS_ERR_LOOP) {
		return -LINUX_ELOOP;
	}
	return -LINUX_ENOENT;
}

static u64 linux_mode_for_type(u64 type, u64 mode)
{
	if ((mode & 0170000) != 0) {
		return mode;
	}
	if (type == BUNIX_VFS_TYPE_DIRECTORY) {
		return LINUX_S_IFDIR | mode;
	}
	if (type == BUNIX_VFS_TYPE_SYMLINK) {
		return LINUX_S_IFLNK | mode;
	}
	if (type == BUNIX_VFS_TYPE_FIFO) {
		return LINUX_S_IFIFO | mode;
	}
	if (type == BUNIX_VFS_TYPE_CHARACTER) {
		return LINUX_S_IFCHR | mode;
	}
	return LINUX_S_IFREG | mode;
}

static long linux_vfs_path_call_word3(struct linux_process *process, u64 type,
				      u64 base_handle, const char *path,
				      u64 word3, struct bunix_msg *reply);

static long linux_vfs_path_call_flags(struct linux_process *process, u64 type,
				      u64 base_handle, const char *path,
				      u64 flags, struct bunix_msg *reply)
{
	return linux_vfs_path_call_word3(process, type, base_handle, path,
					 process->bunix_task | (flags << 32),
					 reply);
}

static long linux_vfs_path_call_word3(struct linux_process *process, u64 type,
				      u64 base_handle, const char *path,
				      u64 word3, struct bunix_msg *reply)
{
	const u64 cwd_len = string_len(process->cwd) + 1;
	const u64 path_len = string_len(path) + 1;
	const u64 len = cwd_len + path_len;
	const long path_buffer = bunix_buffer_create(len);
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = (unsigned int)type,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_RECV,
		.reply = 0,
		.cap = (u64)path_buffer,
		.words = { cwd_len, path_len, base_handle, word3 },
	};
	long result;

	if (path_buffer < 0 || cwd_len == 0 || path_len == 0 ||
	    cwd_len > LINUX_MAX_PATH || path_len > LINUX_MAX_PATH ||
	    len > LINUX_MAX_PATH * 2 ||
	    bunix_buffer_write((u64)path_buffer, 0, process->cwd,
			       cwd_len) != 0 ||
	    bunix_buffer_write((u64)path_buffer, cwd_len, path,
			       path_len) != 0) {
		if (path_buffer >= 0) {
			bunix_handle_close((u64)path_buffer);
		}
		return -1;
	}

	result = bunix_ipc_call(LINUX_HANDLE_VFS, &request, reply);
	bunix_handle_close((u64)path_buffer);
	return result;
}

static long linux_vfs_path_call(struct linux_process *process, u64 type,
				u64 base_handle, const char *path,
				struct bunix_msg *reply)
{
	u64 word3 = process->bunix_task;

	if (type == BUNIX_VFS_OPEN_BUFFER) {
		word3 |= (process->bunix_proc_pid & 0xffffffff) << 32;
	}
	return linux_vfs_path_call_word3(process, type, base_handle, path,
					 word3, reply);
}

static long linux_vfs_symlink_call(struct linux_process *process,
				   u64 base_handle, const char *target,
				   const char *path, struct bunix_msg *reply)
{
	const u64 target_len = string_len(target) + 1;
	const u64 cwd_len = string_len(process->cwd) + 1;
	const u64 path_len = string_len(path) + 1;
	const u64 len = target_len + cwd_len + path_len;
	const long path_buffer = bunix_buffer_create(len);
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_SYMLINK_BUFFER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_RECV,
		.reply = 0,
		.cap = (u64)path_buffer,
		.words = { target_len, cwd_len, path_len,
			   process->bunix_task | (base_handle << 32) },
	};
	long result;

	if (path_buffer < 0 || target_len == 0 || cwd_len == 0 ||
	    path_len == 0 || target_len > LINUX_MAX_PATH ||
	    cwd_len > LINUX_MAX_PATH || path_len > LINUX_MAX_PATH ||
	    len > LINUX_MAX_PATH * 3 || base_handle > 0xffffffff ||
	    bunix_buffer_write((u64)path_buffer, 0, target,
			       target_len) != 0 ||
	    bunix_buffer_write((u64)path_buffer, target_len,
			       process->cwd, cwd_len) != 0 ||
	    bunix_buffer_write((u64)path_buffer, target_len + cwd_len,
			       path, path_len) != 0) {
		if (path_buffer >= 0) {
			bunix_handle_close((u64)path_buffer);
		}
		return -1;
	}

	result = bunix_ipc_call(LINUX_HANDLE_VFS, &request, reply);
	bunix_handle_close((u64)path_buffer);
	return result;
}

static long linux_vfs_two_path_call(struct linux_process *process, u64 type,
				    u64 old_base, const char *old_path,
				    u64 new_base, const char *new_path,
				    u64 flags, struct bunix_msg *reply)
{
	const u64 old_cwd_len = string_len(process->cwd) + 1;
	const u64 old_path_len = string_len(old_path) + 1;
	const u64 new_cwd_len = old_cwd_len;
	const u64 new_path_len = string_len(new_path) + 1;
	const u64 len = old_cwd_len + old_path_len + new_cwd_len +
			new_path_len;
	const long path_buffer = bunix_buffer_create(len);
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = (unsigned int)type,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_RECV,
		.reply = 0,
		.cap = (u64)path_buffer,
		.words = {
			old_base | (new_base << 32),
			old_cwd_len | (old_path_len << 32),
			new_cwd_len | (new_path_len << 32),
			(process->bunix_task & 0xffffffff) | (flags << 32),
		},
	};
	long result;

	if (path_buffer < 0 || old_cwd_len == 0 || old_path_len == 0 ||
	    new_cwd_len == 0 || new_path_len == 0 ||
	    old_cwd_len > LINUX_MAX_PATH || old_path_len > LINUX_MAX_PATH ||
	    new_cwd_len > LINUX_MAX_PATH || new_path_len > LINUX_MAX_PATH ||
	    len > LINUX_MAX_PATH * 4 || old_base > 0xffffffff ||
	    new_base > 0xffffffff || flags > 0xffffffff ||
	    bunix_buffer_write((u64)path_buffer, 0, process->cwd,
			       old_cwd_len) != 0 ||
	    bunix_buffer_write((u64)path_buffer, old_cwd_len, old_path,
			       old_path_len) != 0 ||
	    bunix_buffer_write((u64)path_buffer, old_cwd_len + old_path_len,
			       process->cwd, new_cwd_len) != 0 ||
	    bunix_buffer_write((u64)path_buffer,
			       old_cwd_len + old_path_len + new_cwd_len,
			       new_path, new_path_len) != 0) {
		if (path_buffer >= 0) {
			bunix_handle_close((u64)path_buffer);
		}
		return -1;
	}

	result = bunix_ipc_call(LINUX_HANDLE_VFS, &request, reply);
	bunix_handle_close((u64)path_buffer);
	return result;
}

static long linux_vfs_rename_call(struct linux_process *process,
				  u64 old_base, const char *old_path,
				  u64 new_base, const char *new_path, u64 flags,
				  struct bunix_msg *reply)
{
	return linux_vfs_two_path_call(process, BUNIX_VFS_RENAME_BUFFER,
				       old_base, old_path, new_base, new_path,
				       flags, reply);
}

static long linux_vfs_readlink_call(struct linux_process *process,
				    u64 base_handle, const char *path,
				    u64 out_size, u64 syscall_buffer,
				    u64 *target_len)
{
	char target[LINUX_MAX_PATH];
	const u64 cwd_len = string_len(process->cwd) + 1;
	const u64 path_len = string_len(path) + 1;
	const u64 out_cap = out_size > LINUX_MAX_PATH ? LINUX_MAX_PATH :
			    out_size;
	const u64 out_offset = cwd_len + path_len;
	const u64 len = out_offset + out_cap;
	const long path_buffer = bunix_buffer_create(len);
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_READLINK_BUFFER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_RECV | BUNIX_RIGHT_SEND,
		.reply = 0,
		.cap = (u64)path_buffer,
		.words = { cwd_len, path_len, base_handle,
			   process->bunix_task | (out_cap << 32) },
	};
	struct bunix_msg reply;
	u64 copy_len;

	if (syscall_buffer == 0) {
		if (path_buffer > 0) {
			bunix_handle_close((u64)path_buffer);
		}
		return -LINUX_EFAULT;
	}
	if (path_buffer < 0) {
		return -LINUX_ENOMEM;
	}
	if (target_len == 0 || out_cap == 0 || cwd_len == 0 ||
	    path_len == 0 || cwd_len > LINUX_MAX_PATH ||
	    path_len > LINUX_MAX_PATH || len > LINUX_MAX_PATH * 3) {
		bunix_handle_close((u64)path_buffer);
		return -LINUX_EINVAL;
	}
	if (bunix_buffer_write((u64)path_buffer, 0, process->cwd,
			       cwd_len) != 0 ||
	    bunix_buffer_write((u64)path_buffer, cwd_len, path,
			       path_len) != 0) {
		bunix_handle_close((u64)path_buffer);
		return -LINUX_EIO;
	}
	if (bunix_ipc_call(LINUX_HANDLE_VFS, &request, &reply) != 0) {
		bunix_handle_close((u64)path_buffer);
		return -LINUX_EIO;
	}
	if (reply.words[0] != 0) {
		const long result = linux_vfs_error(reply.words[0]);
		bunix_handle_close((u64)path_buffer);
		return result;
	}
	copy_len = reply.words[1];
	if (copy_len > out_size) {
		copy_len = out_size;
	}
	if (reply.words[3] < copy_len ||
	    copy_len > sizeof(target) ||
	    (copy_len != 0 &&
	     (bunix_buffer_read((u64)path_buffer, out_offset, target,
				copy_len) != 0 ||
	      bunix_buffer_write(syscall_buffer, 0, target, copy_len) != 0))) {
		bunix_handle_close((u64)path_buffer);
		return -LINUX_EFAULT;
	}
	bunix_handle_close((u64)path_buffer);
	*target_len = copy_len;
	return 0;
}

static long linux_close_vfs_handle(u64 handle)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_CLOSE,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { handle, 0, 0, 0 },
	};
	struct bunix_msg reply;

	if (handle == 0 || linux_file_ref_drop(handle) != 0) {
		return 0;
	}
	if (bunix_ipc_call(LINUX_HANDLE_VFS, &request, &reply) != 0) {
		return -LINUX_EIO;
	}
	return reply.words[0] == 0 ? 0 : -LINUX_EIO;
}

static long linux_close_raw_vfs_handle(u64 handle)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_CLOSE,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { handle, 0, 0, 0 },
	};
	struct bunix_msg reply;

	if (handle == 0) {
		return 0;
	}
	if (bunix_ipc_call(LINUX_HANDLE_VFS, &request, &reply) != 0) {
		return -LINUX_EIO;
	}
	return reply.words[0] == 0 ? 0 : -LINUX_EIO;
}

static long linux_ensure_cwd_handle(struct linux_process *process)
{
	struct bunix_msg reply;

	if (process->cwd_handle != 0) {
		return 0;
	}
	if (linux_vfs_path_call(process, BUNIX_VFS_OPEN_BUFFER, 0,
				"/", &reply) != 0 ||
	    reply.words[0] != 0 ||
	    reply.words[3] != BUNIX_VFS_TYPE_DIRECTORY) {
		return -LINUX_ENOENT;
	}
	process->cwd_handle = reply.words[1];
	linux_file_ref_add(process->cwd_handle);
	return 0;
}

static long linux_dirfd_base_handle(struct linux_process *process, u64 dirfd,
				    const char *path, u64 *base_handle)
{
	if (base_handle == 0 || path == 0) {
		return -LINUX_EINVAL;
	}
	*base_handle = 0;
	if (path[0] == '/') {
		return 0;
	}
	if (dirfd == LINUX_AT_FDCWD) {
		if (linux_ensure_cwd_handle(process) != 0) {
			return -LINUX_ENOENT;
		}
		*base_handle = process->cwd_handle;
		return 0;
	}
	if (dirfd >= process->fd_capacity || process->fds[dirfd].kind == 0) {
		return -LINUX_EBADF;
	}
	if (process->fds[dirfd].kind != LINUX_FD_DIR) {
		return -LINUX_ENOTDIR;
	}
	*base_handle = process->fds[dirfd].handle;
	return 0;
}

static long linux_ftruncate(struct linux_process *process, u64 fd, u64 length);
static long linux_close(struct linux_process *process, u64 fd);

static void linux_unlink_hidden_path(struct linux_process *process,
				     const char *path)
{
	struct bunix_msg reply;

	if (process == 0 || path == 0 || path[0] == '\0') {
		return;
	}
	if (linux_vfs_path_call_word3(process, BUNIX_VFS_UNLINK_BUFFER, 0,
				      path, process->bunix_task,
				      &reply) == 0 &&
	    reply.words[0] == 0) {
		linux_vfs_note_mutation(path[0] == '/' ? path : 0);
	}
}

static long linux_open_tmpfile(struct linux_process *process, u64 dirfd,
			       const char *dir_path, u64 base_handle,
			       u64 flags, u64 mode, const char *dir_key)
{
	char name[64];
	char tmp_path[LINUX_MAX_PATH];
	char tmp_key[LINUX_MAX_PATH];
	struct bunix_msg reply;
	long result;

	if ((flags & LINUX_O_ACCMODE) == 0) {
		return -LINUX_EINVAL;
	}
	if (dir_key == 0 || dir_key[0] == '\0') {
		linux_debug_apk_openat_state_log(process, dirfd,
						 "tmpfile-no-key", dir_path,
						 dir_key, -LINUX_ENOENT,
						 flags);
		return -LINUX_ENOENT;
	}

	if (linux_vfs_path_call(process, BUNIX_VFS_OPEN_BUFFER, base_handle,
				dir_path, &reply) != 0) {
		linux_debug_apk_openat_state_log(process, dirfd,
						 "tmpfile-open-call", dir_path,
						 dir_key, -LINUX_ENOENT,
						 flags);
		return -LINUX_ENOENT;
	}
	if (reply.words[0] != 0) {
		result = linux_vfs_error(reply.words[0]);
		linux_debug_apk_openat_state_log(process, dirfd,
						 "tmpfile-open", dir_path,
						 dir_key, result, flags);
		return result;
	}
	if (reply.words[3] != BUNIX_VFS_TYPE_DIRECTORY) {
		(void)linux_close_raw_vfs_handle(reply.words[1]);
		linux_debug_apk_openat_state_log(process, dirfd,
						 "tmpfile-notdir", dir_path,
						 dir_key, -LINUX_ENOTDIR,
						 flags);
		return -LINUX_ENOTDIR;
	}
	(void)linux_close_raw_vfs_handle(reply.words[1]);

	for (u64 attempt = 0; attempt < 32; attempt++) {
		linux_tmpfile_name(process, name, sizeof(name));
		result = linux_path_join_child(dir_path, name, tmp_path);
		if (result != 0) {
			linux_debug_apk_openat_state_log(process, dirfd,
							 "tmpfile-path",
							 dir_path, dir_key,
							 result, flags);
			return result;
		}
		result = linux_path_join_child(dir_key, name, tmp_key);
		if (result != 0) {
			linux_debug_apk_openat_state_log(process, dirfd,
							 "tmpfile-key",
							 dir_path, dir_key,
							 result, flags);
			return result;
		}

		if (linux_vfs_path_call_word3(
			    process, BUNIX_VFS_CREATE_BUFFER, base_handle,
			    tmp_path,
			    process->bunix_task |
			    (((mode & ~process->umask) & 0777) << 32),
			    &reply) != 0) {
			linux_debug_apk_openat_state_log(process, dirfd,
							 "tmpfile-create-call",
							 dir_path, tmp_key,
							 -LINUX_EIO, flags);
			return -LINUX_EIO;
		}
		if (reply.words[0] == BUNIX_VFS_ERR_EXIST) {
			continue;
		}
		if (reply.words[0] != 0) {
			result = linux_vfs_error(reply.words[0]);
			linux_debug_apk_openat_state_log(process, dirfd,
							 "tmpfile-create",
							 dir_path, tmp_key,
							 result, flags);
			return result;
		}
		linux_vfs_note_mutation(tmp_key);

		if (linux_vfs_path_call(process, BUNIX_VFS_OPEN_BUFFER,
					base_handle, tmp_path, &reply) != 0) {
			linux_unlink_hidden_path(process, tmp_key);
			linux_debug_apk_openat_state_log(process, dirfd,
							 "tmpfile-reopen-call",
							 tmp_path, tmp_key,
							 -LINUX_EIO, flags);
			return -LINUX_EIO;
		}
		if (reply.words[0] != 0) {
			result = linux_vfs_error(reply.words[0]);
			linux_unlink_hidden_path(process, tmp_key);
			linux_debug_apk_openat_state_log(process, dirfd,
							 "tmpfile-reopen",
							 tmp_path, tmp_key,
							 result, flags);
			return result;
		}
		if (reply.words[3] != BUNIX_VFS_TYPE_REGULAR) {
			(void)linux_close_raw_vfs_handle(reply.words[1]);
			linux_unlink_hidden_path(process, tmp_key);
			linux_debug_apk_openat_state_log(process, dirfd,
							 "tmpfile-reopen-type",
							 tmp_path, tmp_key,
							 -LINUX_EIO, flags);
			return -LINUX_EIO;
		}

		const long fd = alloc_fd(process, LINUX_FD_FILE,
					 reply.words[1], reply.words[2]);

		if (fd < 0) {
			(void)linux_close_raw_vfs_handle(reply.words[1]);
			linux_unlink_hidden_path(process, tmp_key);
			linux_debug_apk_openat_state_log(process, dirfd,
							 "tmpfile-alloc-fd",
							 tmp_path, tmp_key,
							 fd, flags);
			return fd;
		}
		if (linux_fd_open_file_create(&process->fds[fd],
					      reply.words[1],
					      reply.words[2]) != 0) {
			const u64 remote_handle = reply.words[1];

			(void)linux_close_raw_vfs_handle(remote_handle);
			(void)linux_close(process, (u64)fd);
			linux_unlink_hidden_path(process, tmp_key);
			linux_debug_apk_openat_state_log(process, dirfd,
							 "tmpfile-open-file",
							 tmp_path, tmp_key,
							 -LINUX_EMFILE,
							 flags);
			return -LINUX_EMFILE;
		}
		string_copy(process->fds[fd].path, tmp_key);
		process->fds[fd].flags |= LINUX_FD_TMPFILE;
		if ((flags & LINUX_O_CLOEXEC) != 0) {
			process->fds[fd].flags |= LINUX_FD_CLOEXEC;
		}
		process->fds[fd].status_flags = flags &
			(LINUX_O_ACCMODE | LINUX_O_APPEND);
		(void)dirfd;
		return fd;
	}

	linux_debug_apk_openat_state_log(process, dirfd, "tmpfile-exist",
					 dir_path, dir_key, -LINUX_EEXIST,
					 flags);
	return -LINUX_EEXIST;
}

static long linux_openat(struct linux_process *process, u64 dirfd,
			 u64 path_len, u64 flags, u64 mode, u64 path_buffer)
{
	char path[LINUX_MAX_PATH];
	char full_path[LINUX_MAX_PATH];
	char opened_path[LINUX_MAX_PATH];
	const char *opened_key = 0;
	struct bunix_msg reply;
	u64 base_handle;
	long base_result;
	long path_result;
	long normalize_result;

	path_result = linux_read_path_arg(path_buffer, path_len, path,
					  sizeof(path));
	if (path_result != 0) {
		return path_result;
	}
	linux_debug_path_log(process, "openat", path);
	normalize_result = path_normalize(process->cwd, path, full_path);
	if (normalize_result != 0) {
		linux_debug_openrc_state_log_path(process, dirfd,
						  "openat-normalize", path,
						  0, normalize_result, flags);
		linux_debug_apk_openat_state_log(process, dirfd,
						 "openat-normalize", path,
						 0, normalize_result, flags);
		return normalize_result;
	}
	base_result = linux_dirfd_base_handle(process, dirfd, path,
					      &base_handle);
	if (base_result != 0) {
		linux_debug_openrc_state_log_path(process, dirfd,
						  "openat-dirfd", path,
						  0, base_result, flags);
		linux_debug_apk_openat_state_log(process, dirfd,
						 "openat-dirfd", path, 0,
						 base_result, flags);
		return base_result;
	}
	if (linux_cache_path_for_dirfd(process, dirfd, path, opened_path) == 0) {
		opened_key = opened_path;
	} else if (path[0] == '/' || dirfd == LINUX_AT_FDCWD) {
		opened_key = full_path;
	}
	if ((flags & LINUX___O_TMPFILE) != 0 &&
	    (flags & LINUX_O_TMPFILE_MASK) != LINUX_O_TMPFILE) {
		linux_debug_apk_openat_state_log(process, dirfd,
						 "openat-bad-tmpfile",
						 path, opened_key,
						 -LINUX_EINVAL, flags);
		return -LINUX_EINVAL;
	}
	if ((flags & LINUX_O_TMPFILE_MASK) == LINUX_O_TMPFILE) {
		return linux_open_tmpfile(process, dirfd, path, base_handle,
					  flags, mode, opened_key);
	}
	if ((flags & LINUX_O_NOFOLLOW) != 0) {
		struct bunix_msg meta;

		if (linux_vfs_path_call_flags(
			    process, BUNIX_VFS_STAT_PATH_META_BUFFER,
			    base_handle, path, 1, &meta) == 0 &&
		    meta.words[0] == 0 &&
		    (meta.words[2] >> 32) == BUNIX_VFS_TYPE_SYMLINK) {
			linux_debug_openrc_state_log_path(process, dirfd,
							  "openat-nofollow",
							  path, opened_key,
							  -LINUX_ELOOP,
							  flags);
			linux_debug_apk_openat_state_log(process, dirfd,
							 "openat-nofollow",
							 path, opened_key,
							 -LINUX_ELOOP,
							 flags);
			return -LINUX_ELOOP;
		}
	}

	if (linux_vfs_path_call(process, BUNIX_VFS_OPEN_BUFFER, base_handle,
				path, &reply) != 0) {
		linux_debug_openrc_state_log_path(process, dirfd,
						  "openat-vfs-call", path,
						  opened_key, -LINUX_ENOENT,
						  flags);
		linux_debug_apk_openat_state_log(process, dirfd,
						 "openat-vfs-call", path,
						 opened_key, -LINUX_ENOENT,
						 flags);
		return -LINUX_ENOENT;
	}
	if (reply.words[0] == 0 &&
	    (flags & (LINUX_O_CREAT | LINUX_O_EXCL)) ==
		    (LINUX_O_CREAT | LINUX_O_EXCL)) {
		(void)linux_close_raw_vfs_handle(reply.words[1]);
		linux_debug_openrc_state_log_path(process, dirfd,
						  "openat-exist", path,
						  opened_key, -LINUX_EEXIST,
						  flags);
		linux_debug_apk_openat_state_log(process, dirfd,
						 "openat-exist", path,
						 opened_key, -LINUX_EEXIST,
						 flags);
		return -LINUX_EEXIST;
	}
	if (reply.words[0] == BUNIX_VFS_ERR_NOENT &&
	    (flags & LINUX_O_CREAT) != 0) {
		if (linux_vfs_path_call_word3(process, BUNIX_VFS_CREATE_BUFFER,
					      base_handle, path,
					      process->bunix_task |
					      (((mode & ~process->umask) & 0777) << 32),
					      &reply) != 0) {
			linux_debug_openrc_state_log_path(process, dirfd,
							  "openat-create-call",
							  path, opened_key,
							  -LINUX_EIO, flags);
			linux_debug_apk_openat_state_log(process, dirfd,
							 "openat-create-call",
							 path, opened_key,
							 -LINUX_EIO, flags);
			return -LINUX_EIO;
		}
		if (reply.words[0] != 0) {
			const long result = linux_vfs_error(reply.words[0]);

			linux_debug_openrc_state_log_path(process, dirfd,
							  "openat-create",
							  path, opened_key,
							  result, flags);
			linux_debug_apk_openat_state_log(process, dirfd,
							 "openat-create",
							 path, opened_key,
							 result, flags);
			return result;
		}
		linux_vfs_note_mutation(opened_key);
		if (linux_vfs_path_call(process, BUNIX_VFS_OPEN_BUFFER,
					base_handle, path, &reply) != 0) {
			linux_debug_openrc_state_log_path(process, dirfd,
							  "openat-reopen",
							  path, opened_key,
							  -LINUX_EIO, flags);
			linux_debug_apk_openat_state_log(process, dirfd,
							 "openat-reopen",
							 path, opened_key,
							 -LINUX_EIO, flags);
			return -LINUX_EIO;
		}
	}
	if (reply.words[0] != 0) {
		const long result = linux_vfs_error(reply.words[0]);

		linux_debug_openrc_state_log_path(process, dirfd,
						  "openat-open", path,
						  opened_key, result, flags);
		linux_debug_apk_openat_state_log(process, dirfd,
						 "openat-open", path,
						 opened_key, result, flags);
		return result;
	}
	if ((flags & LINUX_O_DIRECTORY) != 0 &&
	    reply.words[3] != BUNIX_VFS_TYPE_DIRECTORY) {
		(void)linux_close_raw_vfs_handle(reply.words[1]);
		linux_debug_openrc_state_log_path(process, dirfd,
						  "openat-notdir", path,
						  opened_key, -LINUX_ENOTDIR,
						  flags);
		linux_debug_apk_openat_state_log(process, dirfd,
						 "openat-notdir", path,
						 opened_key, -LINUX_ENOTDIR,
						 flags);
		return -LINUX_ENOTDIR;
	}
	if (reply.words[3] == BUNIX_VFS_TYPE_DIRECTORY &&
	    ((flags & LINUX_O_ACCMODE) != 0 ||
	     (flags & LINUX_O_TRUNC) != 0)) {
		(void)linux_close_raw_vfs_handle(reply.words[1]);
		linux_debug_openrc_state_log_path(process, dirfd,
						  "openat-isdir", path,
						  opened_key, -LINUX_EISDIR,
						  flags);
		linux_debug_apk_openat_state_log(process, dirfd,
						 "openat-isdir", path,
						 opened_key, -LINUX_EISDIR,
						 flags);
		return -LINUX_EISDIR;
	}
	if (opened_key != 0 && linux_console_path(opened_key)) {
		const u64 remote_handle = reply.words[1];
		const long fd = alloc_fd(process, LINUX_FD_CONSOLE,
					 LINUX_HANDLE_CONSOLE, 0);

		(void)linux_close_vfs_handle(remote_handle);
		return fd;
	}

	const long fd = alloc_fd(process,
				 reply.words[3] == BUNIX_VFS_TYPE_DIRECTORY ?
				 LINUX_FD_DIR : LINUX_FD_FILE,
				 reply.words[1], reply.words[2]);
	if (fd < 0) {
		(void)linux_close_raw_vfs_handle(reply.words[1]);
		linux_debug_openrc_state_log_path(process, dirfd,
						  "openat-alloc-fd", path,
						  opened_key, fd, flags);
		linux_debug_apk_openat_state_log(process, dirfd,
						 "openat-alloc-fd", path,
						 opened_key, fd, flags);
		return fd;
	}
	if (linux_fd_open_file_create(&process->fds[fd], reply.words[1],
				      reply.words[2]) != 0) {
		const u64 remote_handle = reply.words[1];
		(void)linux_close_raw_vfs_handle(remote_handle);
		(void)linux_close(process, (u64)fd);
		linux_debug_openrc_state_log_path(process, dirfd,
						  "openat-open-file",
						  path, opened_key,
						  -LINUX_EMFILE, flags);
		linux_debug_apk_openat_state_log(process, dirfd,
						 "openat-open-file", path,
						 opened_key, -LINUX_EMFILE,
						 flags);
		return -LINUX_EMFILE;
	}
	if (opened_key != 0) {
		string_copy(process->fds[fd].path, opened_key);
	}
	if ((flags & LINUX_O_CLOEXEC) != 0) {
		process->fds[fd].flags |= LINUX_FD_CLOEXEC;
	}
	process->fds[fd].status_flags = flags &
		(LINUX_O_ACCMODE | LINUX_O_APPEND);
	if ((flags & LINUX_O_TRUNC) != 0 &&
	    (flags & LINUX_O_ACCMODE) != 0 &&
	    process->fds[fd].kind == LINUX_FD_FILE &&
	    (base_result = linux_ftruncate(process, (u64)fd, 0)) != 0) {
		linux_debug_path_result_log(process, "openat-truncate",
					    opened_key != 0 ? opened_key : path,
					    base_result);
		linux_debug_openrc_state_log_fd(process, &process->fds[fd],
						(u64)fd, "openat-truncate",
						base_result, flags, 0);
		(void)linux_close(process, (u64)fd);
		return base_result;
	}
	return fd;
}

static long linux_faccessat(struct linux_process *process, u64 dirfd,
			    u64 path_len, u64 mode, u64 flags,
			    u64 path_buffer)
{
	char path[LINUX_MAX_PATH];
	char cache_path[LINUX_MAX_PATH];
	const char *cache_key = 0;
	struct bunix_msg reply;
	u64 base_handle;
	long base_result;
	long cached_result;
	long path_result;

	if ((mode & ~(LINUX_R_OK | LINUX_W_OK | LINUX_X_OK)) != 0 ||
	    (flags & ~(LINUX_AT_EACCESS | LINUX_AT_SYMLINK_NOFOLLOW)) != 0) {
		return -LINUX_EINVAL;
	}
	path_result = linux_read_path_arg(path_buffer, path_len, path,
					  sizeof(path));
	if (path_result != 0) {
		return path_result;
	}
	linux_debug_path_log(process, "faccessat", path);
	base_result = linux_dirfd_base_handle(process, dirfd, path,
					      &base_handle);
	if (base_result != 0) {
		return base_result;
	}
	if (linux_cache_path_for_dirfd(process, dirfd, path, cache_path) == 0) {
		cache_key = cache_path;
	}
	if (linux_access_cache_get(process, base_handle, path, cache_key, mode,
				   flags, &cached_result)) {
		return cached_result;
	}

	if (linux_vfs_path_call_flags(process, BUNIX_VFS_ACCESS_BUFFER,
				      base_handle, path, mode,
				      &reply) != 0) {
		return -LINUX_EIO;
	}
	if (reply.words[0] != 0) {
		const long result = linux_vfs_error(reply.words[0]);

		linux_access_cache_put(process, base_handle, path, cache_key,
				       mode, flags, result);
		return result;
	}
	linux_access_cache_put(process, base_handle, path, cache_key, mode,
			       flags, 0);
	return 0;
}

static long linux_unlinkat(struct linux_process *process, u64 dirfd,
			   u64 path_len, u64 flags, u64 path_buffer)
{
	char path[LINUX_MAX_PATH];
	char full_path[LINUX_MAX_PATH];
	struct bunix_msg reply;
	const u64 type = (flags & LINUX_AT_REMOVEDIR) != 0 ?
			 BUNIX_VFS_RMDIR_BUFFER : BUNIX_VFS_UNLINK_BUFFER;
	u64 base_handle;
	long base_result;
	long path_result;
	long normalize_result;

	if ((flags & ~LINUX_AT_REMOVEDIR) != 0) {
		return -LINUX_EINVAL;
	}
	path_result = linux_read_path_arg(path_buffer, path_len, path,
					  sizeof(path));
	if (path_result != 0) {
		return path_result;
	}
	if (linux_path_final_dot_component(path)) {
		linux_debug_openrc_state_log_path(process, dirfd,
						  "unlinkat-dot", path, 0,
						  -LINUX_EINVAL, flags);
		return -LINUX_EINVAL;
	}
	normalize_result = path_normalize(process->cwd, path, full_path);
	if (normalize_result != 0) {
		linux_debug_openrc_state_log_path(process, dirfd,
						  "unlinkat-normalize", path,
						  0, normalize_result, flags);
		return normalize_result;
	}
	base_result = linux_dirfd_base_handle(process, dirfd, path,
					      &base_handle);
	if (base_result != 0) {
		linux_debug_openrc_state_log_path(process, dirfd,
						  "unlinkat-dirfd", path,
						  0, base_result, flags);
		return base_result;
	}
	if (linux_cache_path_for_dirfd(process, dirfd, path, full_path) != 0) {
		full_path[0] = '\0';
	}
	linux_debug_openrc_state_log_path(process, dirfd, "unlinkat-call",
					  path, full_path, 0, flags);
	if (linux_vfs_path_call_word3(process, type, base_handle, path,
				      process->bunix_task | (flags << 32),
				      &reply) != 0) {
		linux_debug_openrc_state_log_path(process, dirfd,
						  "unlinkat-vfs-call", path,
						  full_path, -LINUX_EIO,
						  flags);
		return -LINUX_EIO;
	}
	if (reply.words[0] != 0) {
		const long result = linux_vfs_error(reply.words[0]);

		linux_debug_openrc_state_log_path(process, dirfd,
						  "unlinkat-result", path,
						  full_path, result, flags);
		return result;
	}
	linux_vfs_note_mutation(full_path[0] != '\0' ? full_path : 0);
	return 0;
}

static long linux_rmdir(struct linux_process *process, u64 path_len,
			u64 path_buffer)
{
	char path[LINUX_MAX_PATH];
	char full_path[LINUX_MAX_PATH];
	struct bunix_msg reply;
	u64 base_handle;
	long base_result;
	long path_result;
	long normalize_result;

	path_result = linux_read_path_arg(path_buffer, path_len, path,
					  sizeof(path));
	if (path_result != 0) {
		return path_result;
	}
	normalize_result = path_normalize(process->cwd, path, full_path);
	if (normalize_result != 0) {
		linux_debug_openrc_state_log_path(process, LINUX_AT_FDCWD,
						  "rmdir-normalize", path,
						  0, normalize_result, 0);
		return normalize_result;
	}
	base_result = linux_dirfd_base_handle(process, LINUX_AT_FDCWD, path,
					      &base_handle);
	if (base_result != 0) {
		linux_debug_openrc_state_log_path(process, LINUX_AT_FDCWD,
						  "rmdir-dirfd", path, 0,
						  base_result, 0);
		return base_result;
	}
	linux_debug_openrc_state_log_path(process, LINUX_AT_FDCWD,
					  "rmdir-call", path, full_path, 0, 0);
	if (linux_vfs_path_call_word3(process, BUNIX_VFS_RMDIR_BUFFER,
				      base_handle, path,
				      process->bunix_task, &reply) != 0) {
		linux_debug_openrc_state_log_path(process, LINUX_AT_FDCWD,
						  "rmdir-vfs-call", path,
						  full_path, -LINUX_EIO, 0);
		return -LINUX_EIO;
	}
	if (reply.words[0] != 0) {
		const long result = linux_vfs_error(reply.words[0]);

		linux_debug_openrc_state_log_path(process, LINUX_AT_FDCWD,
						  "rmdir-result", path,
						  full_path, result, 0);
		return result;
	}
	linux_vfs_note_mutation(full_path);
	return 0;
}

static long linux_truncate(struct linux_process *process, u64 dirfd,
			   u64 path_len, u64 length, u64 path_buffer)
{
	char path[LINUX_MAX_PATH];
	char full_path[LINUX_MAX_PATH];
	u64 base_handle;
	long base_result;
	long fd;
	long result;
	long path_result;
	long normalize_result;

	if ((length >> 63) != 0) {
		return -LINUX_EINVAL;
	}
	path_result = linux_read_path_arg(path_buffer, path_len, path,
					  sizeof(path));
	if (path_result != 0) {
		return path_result;
	}
	normalize_result = path_normalize(process->cwd, path, full_path);
	if (normalize_result != 0) {
		return normalize_result;
	}
	base_result = linux_dirfd_base_handle(process, dirfd, path,
					      &base_handle);
	if (base_result != 0) {
		return base_result;
	}
	(void)base_handle;
	fd = linux_openat(process, dirfd, path_len, LINUX_O_RDWR, 0,
			  path_buffer);
	if (fd < 0) {
		return fd;
	}
	result = linux_ftruncate(process, (u64)fd, length);
	(void)linux_close(process, (u64)fd);
	return result;
}

static long linux_mkdirat(struct linux_process *process, u64 dirfd,
			  u64 path_len, u64 mode, u64 path_buffer)
{
	char path[LINUX_MAX_PATH];
	char full_path[LINUX_MAX_PATH];
	struct bunix_msg reply;
	u64 base_handle;
	long base_result;
	long path_result;
	long normalize_result;

	path_result = linux_read_path_arg(path_buffer, path_len, path,
					  sizeof(path));
	if (path_result != 0) {
		return path_result;
	}
	normalize_result = path_normalize(process->cwd, path, full_path);
	if (normalize_result != 0) {
		linux_debug_openrc_state_log_path(process, dirfd,
						  "mkdirat-normalize", path,
						  0, normalize_result, mode);
		return normalize_result;
	}
	base_result = linux_dirfd_base_handle(process, dirfd, path,
					      &base_handle);
	if (base_result != 0) {
		linux_debug_openrc_state_log_path(process, dirfd,
						  "mkdirat-dirfd", path,
						  0, base_result, mode);
		return base_result;
	}
	if (linux_cache_path_for_dirfd(process, dirfd, path, full_path) != 0) {
		full_path[0] = '\0';
	}
	linux_debug_openrc_state_log_path(process, dirfd, "mkdirat-call",
					  path, full_path, 0, mode);
	if (linux_vfs_path_call_word3(process, BUNIX_VFS_MKDIR_BUFFER,
				      base_handle, path,
				      process->bunix_task |
				      (((mode & ~process->umask) & 0777) << 32),
				      &reply) != 0) {
		linux_debug_openrc_state_log_path(process, dirfd,
						  "mkdirat-vfs-call", path,
						  full_path, -LINUX_EIO,
						  mode);
		return -LINUX_EIO;
	}
	if (reply.words[0] != 0) {
		const long result = linux_vfs_error(reply.words[0]);

		linux_debug_openrc_state_log_path(process, dirfd,
						  "mkdirat-result", path,
						  full_path, result, mode);
		return result;
	}
	linux_vfs_note_mutation(full_path[0] != '\0' ? full_path : 0);
	return 0;
}

static long linux_mknodat(struct linux_process *process, u64 dirfd,
			  u64 path_len, u64 mode, u64 dev, u64 path_buffer)
{
	char path[LINUX_MAX_PATH];
	char mutation_path[LINUX_MAX_PATH];
	struct bunix_msg reply;
	const u64 file_type = mode & LINUX_S_IFMT;
	u64 base_handle;
	u64 vfs_type;
	long base_result;
	long path_result;
	(void)dev;

	path_result = linux_read_path_arg(path_buffer, path_len, path,
					  sizeof(path));
	if (path_result != 0) {
		return path_result;
	}
	base_result = linux_dirfd_base_handle(process, dirfd, path,
					      &base_handle);
	if (base_result != 0) {
		return base_result;
	}
	if (linux_cache_path_for_dirfd(process, dirfd, path,
				       mutation_path) != 0) {
		mutation_path[0] = '\0';
	}
	if (file_type == LINUX_S_IFIFO) {
		vfs_type = BUNIX_VFS_TYPE_FIFO;
	} else if (file_type == 0 || file_type == LINUX_S_IFREG) {
		vfs_type = BUNIX_VFS_TYPE_REGULAR;
	} else if (file_type == LINUX_S_IFCHR || file_type == LINUX_S_IFBLK) {
		if (linux_vfs_path_call(process, BUNIX_VFS_STAT_PATH_META_BUFFER,
					base_handle, path, &reply) == 0 &&
		    reply.words[0] == 0) {
			return -LINUX_EEXIST;
		}
		return -LINUX_EPERM;
	} else {
		return -LINUX_EINVAL;
	}
	if (linux_vfs_path_call_word3(process, BUNIX_VFS_MKNOD_BUFFER,
				      base_handle, path,
				      process->bunix_task |
				      (((mode & 0777) & ~process->umask) << 32) |
				      (vfs_type << 48),
				      &reply) != 0) {
		return -LINUX_EIO;
	}
	if (reply.words[0] != 0) {
		return linux_vfs_error(reply.words[0]);
	}
	linux_vfs_note_mutation(mutation_path[0] != '\0' ? mutation_path : 0);
	return 0;
}

static long linux_symlinkat(struct linux_process *process, u64 dirfd,
			    u64 target_len, u64 path_len, u64 buffer)
{
	char target[LINUX_MAX_PATH];
	char path[LINUX_MAX_PATH];
	char mutation_path[LINUX_MAX_PATH];
	struct bunix_msg reply = {
		.words = { 0, 0, 0, 0 },
	};
	u64 base_handle;
	long base_result;
	long name_result;

	if (target_len == 0 || path_len == 0 ||
	    target_len > sizeof(target) || path_len > sizeof(path) ||
	    buffer == 0 ||
	    bunix_buffer_read(buffer, 0, target, target_len) != 0 ||
	    bunix_buffer_read(buffer, target_len, path, path_len) != 0) {
		return -LINUX_EFAULT;
	}
	if (target[target_len - 1] != '\0' ||
	    path[path_len - 1] != '\0' ||
	    target[0] == '\0' || path[0] == '\0') {
		return -LINUX_ENOENT;
	}
	linux_debug_path_log(process, "symlinkat", path);
	name_result = linux_check_name_max(path);
	if (name_result != 0) {
		linux_debug_path_result_log(process, "symlinkat", path,
					    name_result);
		return name_result;
	}
	base_result = linux_dirfd_base_handle(process, dirfd, path,
					      &base_handle);
	if (base_result != 0) {
		linux_debug_path_result_log(process, "symlinkat", path,
					    base_result);
		return base_result;
	}
	if (linux_cache_path_for_dirfd(process, dirfd, path,
				       mutation_path) != 0) {
		mutation_path[0] = '\0';
	}
	if (linux_vfs_symlink_call(process, base_handle, target, path,
				   &reply) != 0) {
		linux_debug_path_result_log(process, "symlinkat", path,
					    -LINUX_EIO);
		return -LINUX_EIO;
	}
	if (reply.words[0] != 0) {
		const long result = linux_vfs_error(reply.words[0]);

		linux_debug_path_result_log(process, "symlinkat", path, result);
		return result;
	}
	linux_vfs_note_mutation(mutation_path[0] != '\0' ? mutation_path : 0);
	linux_debug_path_result_log(process, "symlinkat", path, 0);
	return 0;
}

static long linux_renameat2(struct linux_process *process, u64 olddirfd,
			    u64 newdirfd, u64 old_len, u64 new_len,
			    u64 flags, u64 buffer)
{
	char old_path[LINUX_MAX_PATH];
	char new_path[LINUX_MAX_PATH];
	struct bunix_msg reply = {
		.words = { 0, 0, 0, 0 },
	};
	u64 old_base;
	u64 new_base;
	long base_result;
	long name_result;

	if ((flags & ~LINUX_RENAME_NOREPLACE) != 0) {
		return -LINUX_EINVAL;
	}
	if (old_len == 0 || new_len == 0 ||
	    old_len > sizeof(old_path) || new_len > sizeof(new_path) ||
	    buffer == 0 ||
	    bunix_buffer_read(buffer, 0, old_path, old_len) != 0 ||
	    bunix_buffer_read(buffer, old_len, new_path, new_len) != 0) {
		return -LINUX_EFAULT;
	}
	if (old_path[old_len - 1] != '\0' ||
	    new_path[new_len - 1] != '\0' ||
	    old_path[0] == '\0' || new_path[0] == '\0') {
		return -LINUX_ENOENT;
	}
	name_result = linux_check_name_max(old_path);
	if (name_result != 0) {
		return name_result;
	}
	name_result = linux_check_name_max(new_path);
	if (name_result != 0) {
		return name_result;
	}
	base_result = linux_dirfd_base_handle(process, olddirfd, old_path,
					      &old_base);
	if (base_result != 0) {
		return base_result;
	}
	base_result = linux_dirfd_base_handle(process, newdirfd, new_path,
					      &new_base);
	if (base_result != 0) {
		return base_result;
	}
	if (linux_vfs_rename_call(process, old_base, old_path, new_base,
				  new_path, flags, &reply) != 0) {
		return -LINUX_EIO;
	}
	if (reply.words[0] != 0) {
		return linux_vfs_error(reply.words[0]);
	}
	linux_vfs_note_mutation(0);
	return 0;
}

static long linux_linkat(struct linux_process *process, u64 olddirfd,
			 u64 newdirfd, u64 old_len, u64 new_len, u64 flags,
			 u64 buffer)
{
	char old_path[LINUX_MAX_PATH];
	char new_path[LINUX_MAX_PATH];
	struct bunix_msg reply = {
		.words = { 0, 0, 0, 0 },
	};
	u64 old_base;
	u64 new_base;
	u64 old_fd = 0;
	u64 vfs_flags = 0;
	long base_result;
	long name_result;

	if ((flags & ~LINUX_AT_SYMLINK_FOLLOW) != 0) {
		return -LINUX_EINVAL;
	}
	if (old_len == 0 || new_len == 0 ||
	    old_len > sizeof(old_path) || new_len > sizeof(new_path) ||
	    buffer == 0 ||
	    bunix_buffer_read(buffer, 0, old_path, old_len) != 0 ||
	    bunix_buffer_read(buffer, old_len, new_path, new_len) != 0) {
		return -LINUX_EFAULT;
	}
	if (old_path[old_len - 1] != '\0' ||
	    new_path[new_len - 1] != '\0' ||
	    old_path[0] == '\0' || new_path[0] == '\0') {
		return -LINUX_ENOENT;
	}
	name_result = linux_check_name_max(old_path);
	if (name_result != 0) {
		return name_result;
	}
	name_result = linux_check_name_max(new_path);
	if (name_result != 0) {
		return name_result;
	}
	if ((flags & LINUX_AT_SYMLINK_FOLLOW) != 0 &&
	    linux_parse_proc_self_fd(old_path, &old_fd) == 0) {
		if (old_fd >= process->fd_capacity ||
		    process->fds[old_fd].kind == 0) {
			return -LINUX_EBADF;
		}
		if (process->fds[old_fd].kind != LINUX_FD_FILE) {
			return -LINUX_EPERM;
		}
		if (process->fds[old_fd].path[0] == '\0') {
			return -LINUX_ENOENT;
		}
		string_copy(old_path, process->fds[old_fd].path);
		old_base = 0;
	} else {
		base_result = linux_dirfd_base_handle(process, olddirfd,
						      old_path, &old_base);
		if (base_result != 0) {
			return base_result;
		}
	}
	base_result = linux_dirfd_base_handle(process, newdirfd, new_path,
					      &new_base);
	if (base_result != 0) {
		return base_result;
	}
	if (linux_vfs_two_path_call(process, BUNIX_VFS_LINK_BUFFER, old_base,
				    old_path, new_base, new_path, vfs_flags,
				    &reply) != 0) {
		return -LINUX_EIO;
	}
	if (reply.words[0] != 0) {
		return linux_vfs_error(reply.words[0]);
	}
	linux_vfs_note_mutation(0);
	return 0;
}

static long linux_chmodat(struct linux_process *process, u64 dirfd,
			  u64 path_len, u64 mode, u64 flags,
			  u64 path_buffer)
{
	char path[LINUX_MAX_PATH];
	char full_path[LINUX_MAX_PATH];
	struct bunix_msg reply;
	u64 base_handle;
	long base_result;
	long path_result;
	long normalize_result;

	if ((flags & ~LINUX_AT_SYMLINK_NOFOLLOW) != 0) {
		return -LINUX_EINVAL;
	}
	path_result = linux_read_path_arg(path_buffer, path_len, path,
					  sizeof(path));
	if (path_result != 0) {
		return path_result;
	}
	normalize_result = path_normalize(process->cwd, path, full_path);
	if (normalize_result != 0) {
		return normalize_result;
	}
	base_result = linux_dirfd_base_handle(process, dirfd, path,
					      &base_handle);
	if (base_result != 0) {
		return base_result;
	}
	if (linux_cache_path_for_dirfd(process, dirfd, path, full_path) != 0) {
		full_path[0] = '\0';
	}
	if (linux_vfs_path_call_word3(process, BUNIX_VFS_CHMOD_BUFFER,
				      base_handle, path,
				      process->bunix_task |
				      ((mode & 0777) << 32),
				      &reply) != 0) {
		return -LINUX_EIO;
	}
	if (reply.words[0] != 0) {
		return linux_vfs_error(reply.words[0]);
	}
	linux_vfs_note_mutation(full_path[0] != '\0' ? full_path : 0);
	return 0;
}

static long linux_fchmod(struct linux_process *process, u64 fd, u64 mode)
{
	struct bunix_msg request;
	struct bunix_msg reply;

	if (fd >= process->fd_capacity ||
	    (process->fds[fd].kind != LINUX_FD_FILE &&
	     process->fds[fd].kind != LINUX_FD_DIR)) {
		return -LINUX_EBADF;
	}
	request.protocol = BUNIX_PROTO_VFS;
	request.type = BUNIX_VFS_CHMOD;
	request.sender = 0;
	request.cap_rights = 0;
	request.reply = 0;
	request.cap = 0;
	request.words[0] = process->fds[fd].handle;
	request.words[1] = mode & 0777;
	request.words[2] = 0;
	request.words[3] = process->bunix_task;
	if (bunix_ipc_call(LINUX_HANDLE_VFS, &request, &reply) != 0) {
		return -LINUX_EIO;
	}
	if (reply.words[0] != 0) {
		return linux_vfs_error(reply.words[0]);
	}
	linux_vfs_note_mutation(process->fds[fd].path[0] != '\0' ?
				process->fds[fd].path : 0);
	return 0;
}

static long linux_fchown(struct linux_process *process, u64 fd, u64 uid,
			 u64 gid)
{
	struct bunix_msg request;
	struct bunix_msg reply;
	const u64 owner = (uid & 0xffffffff) == 0xffffffff ?
			  (u64)-1 : uid & 0xffffffff;
	const u64 group = (gid & 0xffffffff) == 0xffffffff ?
			  (u64)-1 : gid & 0xffffffff;

	if (fd >= process->fd_capacity ||
	    (process->fds[fd].kind != LINUX_FD_FILE &&
	     process->fds[fd].kind != LINUX_FD_DIR)) {
		return -LINUX_EBADF;
	}
	request.protocol = BUNIX_PROTO_VFS;
	request.type = BUNIX_VFS_CHOWN;
	request.sender = 0;
	request.cap_rights = 0;
	request.reply = 0;
	request.cap = 0;
	request.words[0] = process->fds[fd].handle;
	request.words[1] = owner;
	request.words[2] = group;
	request.words[3] = process->bunix_task;
	if (bunix_ipc_call(LINUX_HANDLE_VFS, &request, &reply) != 0) {
		return -LINUX_EIO;
	}
	if (reply.words[0] != 0) {
		return linux_vfs_error(reply.words[0]);
	}
	linux_vfs_note_mutation(process->fds[fd].path[0] != '\0' ?
				process->fds[fd].path : 0);
	return 0;
}

static long linux_chownat(struct linux_process *process, u64 dirfd,
			  u64 path_len, u64 uid, u64 gid, u64 flags,
			  u64 path_buffer)
{
	char path[LINUX_MAX_PATH];
	char full_path[LINUX_MAX_PATH];
	u64 base_handle;
	long base_result;
	long fd;
	long result;
	long path_result;
	long normalize_result;

	if ((flags & ~LINUX_AT_SYMLINK_NOFOLLOW) != 0) {
		return -LINUX_EINVAL;
	}
	path_result = linux_read_path_arg(path_buffer, path_len, path,
					  sizeof(path));
	if (path_result != 0) {
		return path_result;
	}
	normalize_result = path_normalize(process->cwd, path, full_path);
	if (normalize_result != 0) {
		return normalize_result;
	}
	base_result = linux_dirfd_base_handle(process, dirfd, path,
					      &base_handle);
	if (base_result != 0) {
		return base_result;
	}
	(void)base_handle;
	fd = linux_openat(process, dirfd, path_len, LINUX_O_RDWR, 0,
			  path_buffer);
	if (fd < 0) {
		fd = linux_openat(process, dirfd, path_len, LINUX_O_DIRECTORY,
				  0, path_buffer);
	}
	if (fd < 0) {
		return fd;
	}
	result = linux_fchown(process, (u64)fd, uid, gid);
	(void)linux_close(process, (u64)fd);
	return result;
}

static long linux_stat_write_identity(u64 stat_buffer, u64 mode, u64 uid,
				      u64 gid, u64 size, u64 dev, u64 ino,
				      u64 nlink, u64 rdev, u64 blksize,
				      u64 blocks, u64 atime, u64 mtime,
				      u64 ctime)
{
	char stat[LINUX_STAT_SIZE];

	if (stat_buffer == 0) {
		return -LINUX_EFAULT;
	}

	zero_bytes(stat, sizeof(stat));
	store_u64(stat, 0, dev != 0 ? dev : 1);
	store_u64(stat, 8, ino != 0 ? ino : 1);
	store_u64(stat, 16, nlink != 0 ? nlink : 1);
	store_u32(stat, 24, (unsigned int)mode);
	store_u32(stat, 28, (unsigned int)uid);
	store_u32(stat, 32, (unsigned int)gid);
	store_u64(stat, 40, rdev);
	store_u64(stat, 48, size);
	store_u64(stat, 56, blksize != 0 ? blksize : 4096);
	store_u64(stat, 64, blocks);
	store_u64(stat, 72, atime);
	store_u64(stat, 88, mtime);
	store_u64(stat, 104, ctime);

	return bunix_buffer_write(stat_buffer, 0, stat, sizeof(stat)) == 0 ?
		0 : -LINUX_EFAULT;
}

static long linux_stat_write(u64 stat_buffer, u64 mode, u64 uid, u64 gid,
			     u64 size)
{
	return linux_stat_write_identity(stat_buffer, mode, uid, gid, size,
					 1, 1, 1, 0, 4096,
					 (size + 511) / 512, 0, 0, 0);
}

static void linux_vfs_stat_from_reply(struct linux_vfs_stat *stat,
				      const struct bunix_msg *reply,
				      u64 fallback_ino)
{
	stat->size = reply->words[1];
	stat->mode_type = reply->words[2];
	stat->owner = reply->words[3];
	stat->dev = 1;
	stat->ino = fallback_ino == 0 ? 1 : fallback_ino;
	stat->nlink = 1;
	stat->rdev = 0;
	stat->blksize = 4096;
	stat->blocks = (stat->size + 511) / 512;
	stat->atime = 0;
	stat->mtime = 0;
	stat->ctime = 0;
}

static void linux_vfs_stat_from_buffer(struct linux_vfs_stat *stat,
				       const unsigned char *buffer)
{
	stat->size = bunix_load_u64_le(buffer, BUNIX_VFS_STAT_SIZE);
	stat->mode_type = bunix_load_u64_le(buffer, BUNIX_VFS_STAT_MODE_TYPE);
	stat->owner = bunix_load_u64_le(buffer, BUNIX_VFS_STAT_OWNER);
	stat->dev = bunix_load_u64_le(buffer, BUNIX_VFS_STAT_DEV);
	stat->ino = bunix_load_u64_le(buffer, BUNIX_VFS_STAT_INO);
	stat->nlink = bunix_load_u64_le(buffer, BUNIX_VFS_STAT_NLINK);
	stat->rdev = bunix_load_u64_le(buffer, BUNIX_VFS_STAT_RDEV);
	stat->blksize = bunix_load_u64_le(buffer, BUNIX_VFS_STAT_BLKSIZE);
	stat->blocks = bunix_load_u64_le(buffer, BUNIX_VFS_STAT_BLOCKS);
	stat->atime = bunix_load_u64_le(buffer, BUNIX_VFS_STAT_ATIME);
	stat->mtime = bunix_load_u64_le(buffer, BUNIX_VFS_STAT_MTIME);
	stat->ctime = bunix_load_u64_le(buffer, BUNIX_VFS_STAT_CTIME);
	if (stat->dev == 0) {
		stat->dev = 1;
	}
	if (stat->ino == 0) {
		stat->ino = 1;
	}
	if (stat->nlink == 0) {
		stat->nlink = 1;
	}
	if (stat->blksize == 0) {
		stat->blksize = 4096;
	}
}

static long linux_stat_from_vfs_stat(u64 stat_buffer,
				     const struct linux_vfs_stat *stat)
{
	const u64 mode = stat->mode_type & 0xffffffff;
	const u64 type = stat->mode_type >> 32;

	return linux_stat_write_identity(stat_buffer,
					 linux_mode_for_type(type, mode),
					 stat->owner & 0xffffffff,
					 stat->owner >> 32,
					 stat->size, stat->dev, stat->ino,
					 stat->nlink, stat->rdev,
					 stat->blksize, stat->blocks,
					 stat->atime, stat->mtime,
					 stat->ctime);
}

static long linux_statx_write(u64 statx_buffer, u64 mode, u64 uid, u64 gid,
			      u64 size, u64 dev, u64 ino, u64 nlink,
			      u64 rdev, u64 blksize, u64 blocks,
			      u64 atime, u64 mtime, u64 ctime)
{
	char statx[LINUX_STATX_SIZE];

	if (statx_buffer == 0) {
		return -LINUX_EFAULT;
	}

	zero_bytes(statx, sizeof(statx));
	store_u32(statx, 0, LINUX_STATX_BASIC_STATS);
	store_u32(statx, 4, (unsigned int)(blksize != 0 ? blksize : 4096));
	store_u32(statx, 16, (unsigned int)(nlink != 0 ? nlink : 1));
	store_u32(statx, 20, (unsigned int)uid);
	store_u32(statx, 24, (unsigned int)gid);
	store_u16(statx, 28, (unsigned int)mode);
	store_u64(statx, 32, ino != 0 ? ino : 1);
	store_u64(statx, 40, size);
	store_u64(statx, 48, blocks);
	store_u64(statx, 64, atime);
	store_u64(statx, 96, ctime);
	store_u64(statx, 112, mtime);
	store_u32(statx, 128, (unsigned int)(rdev >> 32));
	store_u32(statx, 132, (unsigned int)rdev);
	store_u32(statx, 136, (unsigned int)(dev >> 32));
	store_u32(statx, 140, (unsigned int)dev);

	return bunix_buffer_write(statx_buffer, 0, statx, sizeof(statx)) == 0 ?
		0 : -LINUX_EFAULT;
}

static long linux_statx_from_vfs_stat(u64 statx_buffer,
				      const struct linux_vfs_stat *stat)
{
	const u64 mode = stat->mode_type & 0xffffffff;
	const u64 type = stat->mode_type >> 32;

	return linux_statx_write(statx_buffer, linux_mode_for_type(type, mode),
				 stat->owner & 0xffffffff,
				 stat->owner >> 32, stat->size, stat->dev,
				 stat->ino, stat->nlink, stat->rdev,
				 stat->blksize, stat->blocks, stat->atime,
				 stat->mtime, stat->ctime);
}

static int linux_stat_buffer_is_zero(const unsigned char *buffer)
{
	for (u64 i = 0; i < BUNIX_VFS_STAT_BYTES; i++) {
		if (buffer[i] != 0) {
			return 0;
		}
	}
	return 1;
}

static long linux_vfs_fstat_call(u64 handle, struct linux_vfs_stat *stat)
{
	const long buffer = bunix_buffer_create(BUNIX_VFS_STAT_BYTES);
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_STAT_META,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP,
		.reply = 0,
		.cap = buffer < 0 ? 0 : (u64)buffer,
		.words = { handle, 0, 0, 0 },
	};
	struct bunix_msg reply;
	unsigned char raw[BUNIX_VFS_STAT_BYTES];

	if (buffer < 0) {
		return -LINUX_ENOMEM;
	}
	if (bunix_ipc_call(LINUX_HANDLE_VFS, &request, &reply) != 0) {
		bunix_handle_close((u64)buffer);
		return -LINUX_EIO;
	}
	if (reply.words[0] != 0) {
		bunix_handle_close((u64)buffer);
		return linux_vfs_error(reply.words[0]);
	}
	if (bunix_buffer_read((u64)buffer, 0, raw, sizeof(raw)) == 0 &&
	    !linux_stat_buffer_is_zero(raw)) {
		linux_vfs_stat_from_buffer(stat, raw);
	} else {
		linux_vfs_stat_from_reply(stat, &reply, handle);
	}
	bunix_handle_close((u64)buffer);
	return 0;
}

static long linux_vfs_path_stat_call(struct linux_process *process,
				     u64 base_handle, const char *path,
				     u64 nofollow,
				     struct linux_vfs_stat *stat)
{
	const u64 cwd_len = string_len(process->cwd) + 1;
	const u64 path_len = string_len(path) + 1;
	const u64 stat_offset = cwd_len + path_len;
	const u64 len = stat_offset + BUNIX_VFS_STAT_BYTES;
	const long path_buffer = bunix_buffer_create(len);
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_STAT_PATH_META_BUFFER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_RECV | BUNIX_RIGHT_SEND |
			      BUNIX_RIGHT_DUP,
		.reply = 0,
		.cap = path_buffer < 0 ? 0 : (u64)path_buffer,
		.words = { cwd_len, path_len, base_handle,
			   process->bunix_task | (nofollow << 32) },
	};
	struct bunix_msg reply;
	unsigned char raw[BUNIX_VFS_STAT_BYTES];

	if (path_buffer < 0) {
		return -LINUX_ENOMEM;
	}
	if (cwd_len == 0 || path_len == 0 ||
	    cwd_len > LINUX_MAX_PATH || path_len > LINUX_MAX_PATH ||
	    len > LINUX_MAX_PATH * 2 + BUNIX_VFS_STAT_BYTES ||
	    bunix_buffer_write((u64)path_buffer, 0, process->cwd,
			       cwd_len) != 0 ||
	    bunix_buffer_write((u64)path_buffer, cwd_len, path,
			       path_len) != 0) {
		bunix_handle_close((u64)path_buffer);
		return -LINUX_EIO;
	}
	if (bunix_ipc_call(LINUX_HANDLE_VFS, &request, &reply) != 0) {
		bunix_handle_close((u64)path_buffer);
		return -LINUX_EIO;
	}
	if (reply.words[0] != 0) {
		bunix_handle_close((u64)path_buffer);
		return linux_vfs_error(reply.words[0]);
	}
	if (bunix_buffer_read((u64)path_buffer, stat_offset, raw,
			      sizeof(raw)) == 0 &&
	    !linux_stat_buffer_is_zero(raw)) {
		linux_vfs_stat_from_buffer(stat, raw);
	} else {
		linux_vfs_stat_from_reply(stat, &reply, 1);
	}
	bunix_handle_close((u64)path_buffer);
	return 0;
}

static long linux_vfs_utimens_call(struct linux_process *process,
				   u64 base_handle, const char *path,
				   u64 atime, u64 mtime, u64 flags)
{
	const u64 cwd_len = string_len(process->cwd) + 1;
	const u64 path_len = string_len(path) + 1;
	const u64 payload_offset = cwd_len + path_len;
	const u64 len = payload_offset + 16;
	const long path_buffer = bunix_buffer_create(len);
	unsigned char payload[16];
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_UTIMENS_BUFFER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_RECV,
		.reply = 0,
		.cap = path_buffer < 0 ? 0 : (u64)path_buffer,
		.words = { cwd_len, path_len, base_handle,
			   process->bunix_task | (flags << 32) },
	};
	struct bunix_msg reply;

	if (path_buffer < 0) {
		return -LINUX_ENOMEM;
	}
	if (cwd_len == 0 || path_len == 0 ||
	    cwd_len > LINUX_MAX_PATH || path_len > LINUX_MAX_PATH ||
	    len > LINUX_MAX_PATH * 2 + sizeof(payload) ||
	    bunix_buffer_write((u64)path_buffer, 0, process->cwd,
			       cwd_len) != 0 ||
	    bunix_buffer_write((u64)path_buffer, cwd_len, path,
			       path_len) != 0) {
		bunix_handle_close((u64)path_buffer);
		return -LINUX_EIO;
	}
	bunix_store_u64_le(payload, 0, atime);
	bunix_store_u64_le(payload, 8, mtime);
	if (bunix_buffer_write((u64)path_buffer, payload_offset, payload,
			       sizeof(payload)) != 0) {
		bunix_handle_close((u64)path_buffer);
		return -LINUX_EIO;
	}
	if (bunix_ipc_call(LINUX_HANDLE_VFS, &request, &reply) != 0) {
		bunix_handle_close((u64)path_buffer);
		return -LINUX_EIO;
	}
	bunix_handle_close((u64)path_buffer);
	if (reply.words[0] != 0) {
		return linux_vfs_error(reply.words[0]);
	}
	linux_vfs_note_mutation(path[0] == '/' ? path : 0);
	return 0;
}

static long linux_statfs_write(u64 statfs_buffer)
{
	char statfs[LINUX_STATFS_SIZE];

	if (statfs_buffer == 0) {
		return -LINUX_EFAULT;
	}

	zero_bytes(statfs, sizeof(statfs));
	store_u64(statfs, 0, LINUX_STATFS_MAGIC);
	store_u64(statfs, 8, LINUX_STATFS_BLOCK_SIZE);
	store_u64(statfs, 16, LINUX_STATFS_BLOCKS);
	store_u64(statfs, 24, LINUX_STATFS_FREE_BLOCKS);
	store_u64(statfs, 32, LINUX_STATFS_FREE_BLOCKS);
	store_u64(statfs, 40, 65536);
	store_u64(statfs, 48, 32768);
	store_u32(statfs, 64, LINUX_NAME_MAX);
	store_u64(statfs, 72, LINUX_STATFS_BLOCK_SIZE);
	return bunix_buffer_write(statfs_buffer, 0, statfs, sizeof(statfs)) == 0 ?
	       0 : -LINUX_EFAULT;
}

static long linux_fstatfs(struct linux_process *process, u64 fd,
			  u64 statfs_buffer)
{
	if (fd >= process->fd_capacity || process->fds[fd].kind == 0) {
		return -LINUX_EBADF;
	}
	return linux_statfs_write(statfs_buffer);
}

static long linux_statfs(struct linux_process *process, u64 path_len,
			 u64 path_buffer)
{
	char path[LINUX_MAX_PATH];
	struct bunix_msg reply;
	u64 base_handle;
	long path_result;
	long base_result;

	path_result = linux_read_path_arg(path_buffer, path_len, path,
					  sizeof(path));
	if (path_result != 0) {
		return path_result;
	}
	linux_debug_path_log(process, "statfs", path);
	base_result = linux_dirfd_base_handle(process, LINUX_AT_FDCWD, path,
					      &base_handle);
	if (base_result != 0) {
		return base_result;
	}
	if (linux_vfs_path_call_flags(process, BUNIX_VFS_STAT_PATH_META_BUFFER,
				      base_handle, path, 0, &reply) != 0) {
		return -LINUX_EIO;
	}
	if (reply.words[0] != 0) {
		return linux_vfs_error(reply.words[0]);
	}
	return linux_statfs_write(path_buffer);
}

static long linux_mount_path_command(u64 service, unsigned int protocol,
				     unsigned int type, const char *target)
{
	struct bunix_msg reply;

	if (service == 0 ||
	    bunix_ipc_call_path(service, protocol, type, target, 0, 0, 0,
				&reply) != 0) {
		return -LINUX_ENODEV;
	}
	if (reply.words[0] != 0) {
		return -LINUX_EBUSY;
	}
	linux_vfs_note_mutation(target);
	return 0;
}

static long linux_vfs_unmount(const char *target)
{
	struct bunix_msg reply;

	if (bunix_ipc_call_path(LINUX_HANDLE_VFS, BUNIX_PROTO_VFS,
				BUNIX_VFS_UNMOUNT_BUFFER, target, 0, 0, 0,
				&reply) != 0) {
		return -LINUX_ENOSYS;
	}
	if (reply.words[0] != 0) {
		return linux_vfs_error(reply.words[0]);
	}
	linux_vfs_note_mutation(target);
	return 0;
}

static long linux_mount(struct linux_process *process, u64 target_len,
			u64 fstype_len, u64 flags, u64 buffer)
{
	enum {
		LINUX_MS_REMOUNT = 32,
	};
	char target[LINUX_MAX_PATH];
	char full_target[LINUX_MAX_PATH];
	char fstype[64];
	long normalize_result;
	const long euid = linux_user_credential(process, BUNIX_LINUX_GETEUID);

	if (euid < 0) {
		return euid;
	}
	if (euid != 0) {
		return -LINUX_EPERM;
	}
	if (target_len == 0 || target_len > sizeof(target) ||
	    fstype_len > sizeof(fstype)) {
		return -LINUX_EINVAL;
	}
	if (buffer == 0 ||
	    bunix_buffer_read(buffer, 0, target, target_len) != 0 ||
	    (fstype_len != 0 &&
	     bunix_buffer_read(buffer, target_len, fstype, fstype_len) != 0)) {
		return -LINUX_EFAULT;
	}
	if (target[target_len - 1] != '\0' ||
	    (fstype_len != 0 && fstype[fstype_len - 1] != '\0')) {
		return -LINUX_EINVAL;
	}
	if (target[0] == '\0' ||
	    (fstype_len != 0 && fstype[0] == '\0')) {
		return -LINUX_EINVAL;
	}
	if (fstype_len == 0) {
		fstype[0] = '\0';
	}
	linux_debug_path_log(process, "mount", target);
	normalize_result = path_normalize(process->cwd, target, full_target);
	if (normalize_result != 0) {
		return normalize_result;
	}
	if ((flags & LINUX_MS_REMOUNT) != 0) {
		linux_vfs_note_mutation(full_target);
		return 0;
	}
	if (fstype[0] == '\0') {
		return -LINUX_EINVAL;
	}
	if (string_equal(fstype, "proc") || string_equal(fstype, "procfs")) {
		return linux_mount_path_command(
			linux_cached_service(BUNIX_SERVICE_PROCFS,
					     &procfs_service),
			BUNIX_PROTO_PROCFS, BUNIX_PROCFS_MOUNT_PATH,
			full_target);
	}
	if (string_equal(fstype, "tmpfs")) {
		return linux_mount_path_command(
			linux_cached_service(BUNIX_SERVICE_TMPFS,
					     &tmpfs_service),
			BUNIX_PROTO_TMPFS, BUNIX_TMPFS_MOUNT_ROOT,
			full_target);
	}
	if (string_equal(fstype, "devtmpfs") || string_equal(fstype, "devfs")) {
		return linux_mount_path_command(
			linux_cached_service(BUNIX_SERVICE_DEVFS,
					     &devfs_service),
			BUNIX_PROTO_DEVFS, BUNIX_DEVFS_MOUNT_PATH,
			full_target);
	}
	if (string_equal(fstype, "unionfs")) {
		return linux_mount_path_command(
			linux_cached_service(BUNIX_SERVICE_UNIONFS,
					     &unionfs_service),
			BUNIX_PROTO_UNIONFS, BUNIX_UNIONFS_MOUNT_PATH,
			full_target);
	}
	return -LINUX_ENODEV;
}

static long linux_umount2(struct linux_process *process, u64 flags,
			  u64 path_len, u64 path_buffer)
{
	char path[LINUX_MAX_PATH];
	char full_path[LINUX_MAX_PATH];
	long path_result;
	long normalize_result;
	const long euid = linux_user_credential(process, BUNIX_LINUX_GETEUID);

	(void)flags;
	if (euid < 0) {
		return euid;
	}
	if (euid != 0) {
		return -LINUX_EPERM;
	}
	path_result = linux_read_path_arg(path_buffer, path_len, path,
					  sizeof(path));
	if (path_result != 0) {
		return path_result;
	}
	normalize_result = path_normalize(process->cwd, path, full_path);
	if (normalize_result != 0) {
		return normalize_result;
	}
	return linux_vfs_unmount(full_path);
}

static long linux_fstat(struct linux_process *process, u64 fd, u64 stat_buffer)
{
	if (fd >= process->fd_capacity || process->fds[fd].kind == 0) {
		return -LINUX_EBADF;
	}

	if (process->fds[fd].kind == LINUX_FD_CONSOLE) {
		return linux_stat_write(stat_buffer, LINUX_S_IFCHR | 0600,
					0, 0, 0);
	}
	if (process->fds[fd].kind == LINUX_FD_SOCKET) {
		return linux_stat_write(stat_buffer, LINUX_S_IFSOCK | 0700,
					0, 0, 0);
	}
	if (process->fds[fd].kind == LINUX_FD_PIPE_READ ||
	    process->fds[fd].kind == LINUX_FD_PIPE_WRITE) {
		return linux_stat_write(stat_buffer, LINUX_S_IFIFO | 0600,
					0, 0, 0);
	}
	struct linux_vfs_stat stat;
	const long result = linux_vfs_fstat_call(process->fds[fd].handle,
						 &stat);

	if (result != 0) {
		return result;
	}

	return linux_stat_from_vfs_stat(stat_buffer, &stat);
}

static long linux_ftruncate(struct linux_process *process, u64 fd, u64 length)
{
	if ((length >> 63) != 0) {
		return -LINUX_EINVAL;
	}
	if (fd >= process->fd_capacity || process->fds[fd].kind == 0) {
		return -LINUX_EBADF;
	}
	if (process->fds[fd].kind != LINUX_FD_FILE) {
		return -LINUX_EINVAL;
	}
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_TRUNCATE,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { process->fds[fd].handle, length, 0,
			   process->bunix_task },
	};
	struct bunix_msg reply;

	if (bunix_ipc_call(LINUX_HANDLE_VFS, &request, &reply) != 0) {
		return -LINUX_EIO;
	}
	if (reply.words[0] != 0) {
		return linux_vfs_error(reply.words[0]);
	}
	if (process->fds[fd].open_file != 0) {
		linux_open_file_readahead_clear(process->fds[fd].open_file);
	}
	linux_fd_set_size(process, fd, length);
	linux_vfs_note_mutation(process->fds[fd].path[0] != '\0' ?
				process->fds[fd].path : 0);
	return 0;
}

static long linux_newfstatat(struct linux_process *process, u64 dirfd,
			     u64 path_len, u64 path_buffer,
			     u64 flags, struct linux_vfs_stat *out)
{
	char path[LINUX_MAX_PATH];
	char cache_path[LINUX_MAX_PATH];
	const char *cache_key = 0;
	u64 base_handle;
	u64 nofollow;
	long base_result;
	long cached_result;
	long result;
	long path_result;
	const u64 allowed_flags = LINUX_AT_SYMLINK_NOFOLLOW |
				  LINUX_AT_NO_AUTOMOUNT |
				  LINUX_AT_EMPTY_PATH;

	if ((flags & ~allowed_flags) != 0) {
		return -LINUX_EINVAL;
	}
	if ((flags & LINUX_AT_EMPTY_PATH) != 0 && path_len == 1) {
		if (dirfd >= process->fd_capacity ||
		    process->fds[dirfd].kind == 0) {
			return -LINUX_EBADF;
		}
		return linux_vfs_fstat_call(process->fds[dirfd].handle, out);
	}

	path_result = linux_read_path_arg(path_buffer, path_len, path,
					  sizeof(path));
	if (path_result != 0) {
		return path_result;
	}
	linux_debug_path_log(process, "newfstatat", path);
	base_result = linux_dirfd_base_handle(process, dirfd, path,
					      &base_handle);
	if (base_result != 0) {
		linux_debug_path_result_log(process, "newfstatat", path,
					    base_result);
		return base_result;
	}
	if (linux_cache_path_for_dirfd(process, dirfd, path, cache_path) == 0) {
		cache_key = cache_path;
	}

	nofollow = (flags & LINUX_AT_SYMLINK_NOFOLLOW) != 0 ? 1 : 0;
	if (linux_stat_cache_get(process, base_handle, path, cache_key, nofollow,
				 &cached_result, out)) {
		linux_debug_path_result_log(process, "newfstatat", path,
					    cached_result);
		return cached_result;
	}

	result = linux_vfs_path_stat_call(process, base_handle, path,
					  nofollow, out);
	linux_stat_cache_put(process, base_handle, path, cache_key, nofollow,
			     result, out);
	linux_debug_path_result_log(process, "newfstatat", path, result);
	return result;
}

static long linux_statx(struct linux_process *process, u64 dirfd,
			u64 path_len, u64 flags, u64 mask, u64 path_buffer)
{
	struct linux_vfs_stat meta;
	const u64 allowed_flags = LINUX_AT_SYMLINK_NOFOLLOW |
				  LINUX_AT_NO_AUTOMOUNT |
				  LINUX_AT_EMPTY_PATH |
				  LINUX_AT_STATX_SYNC_TYPE;
	(void)mask;

	if ((flags & ~allowed_flags) != 0) {
		return -LINUX_EINVAL;
	}
	const long result = linux_newfstatat(
		process, dirfd, path_len, path_buffer,
		flags & (LINUX_AT_SYMLINK_NOFOLLOW |
			 LINUX_AT_NO_AUTOMOUNT |
			 LINUX_AT_EMPTY_PATH),
		&meta);
	if (result != 0) {
		return result;
	}
	return linux_statx_from_vfs_stat(path_buffer, &meta);
}

static long linux_utimensat(struct linux_process *process, u64 dirfd,
			    u64 path_len, u64 flags, u64 path_buffer)
{
	char path[LINUX_MAX_PATH];
	unsigned char payload[16];
	u64 base_handle;
	u64 atime;
	u64 mtime;
	const u64 path_flags = flags & 0xffffffff;
	const u64 time_flags = flags >> 32;
	long base_result;
	long path_result;

	if ((path_flags & ~LINUX_AT_SYMLINK_NOFOLLOW) != 0 ||
	    (time_flags & ~15UL) != 0) {
		return -LINUX_EINVAL;
	}
	path_result = linux_read_path_arg(path_buffer, path_len, path,
					  sizeof(path));
	if (path_result != 0) {
		return path_result;
	}
	linux_debug_path_log(process, "utimensat", path);
	if (bunix_buffer_read(path_buffer, path_len, payload,
			      sizeof(payload)) != 0) {
		return -LINUX_EFAULT;
	}
	atime = bunix_load_u64_le(payload, 0);
	mtime = bunix_load_u64_le(payload, 8);
	if ((time_flags & 4) != 0) {
		atime = linux_realtime_seconds();
	}
	if ((time_flags & 8) != 0) {
		mtime = linux_realtime_seconds();
	}
	base_result = linux_dirfd_base_handle(process, dirfd, path,
					      &base_handle);
	if (base_result != 0) {
		return base_result;
	}
	return linux_vfs_utimens_call(process, base_handle, path, atime,
				      mtime, time_flags & ~12UL);
}

static long linux_readlinkat(struct linux_process *process, u64 dirfd,
			     u64 path_len, u64 out_size, u64 path_buffer)
{
	char path[LINUX_MAX_PATH];
	u64 base_handle;
	u64 copy_len;
	long base_result;
	long path_result;

	if (out_size == 0) {
		return -LINUX_EINVAL;
	}
	path_result = linux_read_path_arg(path_buffer, path_len, path,
					  sizeof(path));
	if (path_result != 0) {
		return path_result;
	}
	linux_debug_path_log(process, "readlinkat", path);
	base_result = linux_dirfd_base_handle(process, dirfd, path,
					      &base_handle);
	if (base_result != 0) {
		return base_result;
	}
	path_result = linux_vfs_readlink_call(process, base_handle, path,
					      out_size, path_buffer, &copy_len);
	if (path_result != 0) {
		return path_result;
	}
	return (long)copy_len;
}

static void linux_store_dirent(char *buffer, u64 offset, u64 ino, u64 next,
			       u64 reclen, u64 type, const char *name)
{
	store_u64(buffer, offset + 0, ino);
	store_u64(buffer, offset + 8, next);
	store_u16(buffer, offset + 16, (unsigned int)reclen);
	buffer[offset + 18] = (char)type;
	for (u64 i = 0; i + 19 < reclen; i++) {
		buffer[offset + 19 + i] = name[i];
		if (name[i] == '\0') {
			break;
		}
	}
}

static u64 linux_dirent_reclen(const char *name)
{
	const u64 len = string_len(name) + 1;
	const u64 raw = 19 + len;

	return (raw + 7) & ~7ull;
}

static long linux_readdir_name(u64 handle, u64 index, u64 name_buffer,
			       char *name, u64 name_cap, u64 *type,
			       u64 *next_offset)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_READDIR_BUFFER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_SEND,
		.reply = 0,
		.cap = name_buffer,
		.words = { handle, index, 0, name_cap - 1 },
	};
	struct bunix_msg reply;

	if (name == 0 || name_cap == 0 || type == 0 || next_offset == 0) {
		return -LINUX_EINVAL;
	}
	for (u64 i = 0; i < name_cap; i++) {
		name[i] = '\0';
	}
	if (bunix_ipc_call(LINUX_HANDLE_VFS, &request, &reply) != 0) {
		return -LINUX_EIO;
	}
	if (reply.words[0] != 0) {
		if (reply.words[0] == BUNIX_VFS_ERR_NOENT) {
			return -LINUX_ENOENT;
		}
		return linux_vfs_error(reply.words[0]);
	}
	const u64 name_len = reply.words[2];
	const u64 written = reply.words[3];

	if (name_len >= name_cap || written < name_len ||
	    (name_len != 0 &&
	     bunix_buffer_read(name_buffer, 0, name, name_len) != 0)) {
		return -LINUX_EIO;
	}
	name[name_len] = '\0';
	*type = reply.words[1] >> 32;
	*next_offset = (reply.words[1] & 0xffffffff) + 2;
	return 0;
}

static int linux_dir_cache_allowed(const char *path)
{
	return path != 0 &&
	       path[0] == '/' &&
	       linux_cache_domain_for_path(0, path) == LINUX_CACHE_DOMAIN_ETC;
}

static struct linux_dir_cache_entry *linux_dir_cache_lookup(const char *path)
{
	const u64 domain = linux_cache_domain_for_path(0, path);

	if (!linux_dir_cache_allowed(path)) {
		return 0;
	}
	for (u64 i = 0; i < LINUX_DIR_CACHE_SIZE; i++) {
		struct linux_dir_cache_entry *entry = &linux_dir_cache[i];

		if (entry->valid != 0 &&
		    entry->cache_domain == domain &&
		    entry->cache_epoch == linux_vfs_mutation_epochs[domain] &&
		    string_equal(entry->path, path)) {
			return entry;
		}
	}
	return 0;
}

static struct linux_dir_cache_entry *linux_dir_cache_claim(const char *path)
{
	struct linux_dir_cache_entry *entry = 0;

	for (u64 i = 0; i < LINUX_DIR_CACHE_SIZE; i++) {
		if (linux_dir_cache[i].valid == 0) {
			entry = &linux_dir_cache[i];
			break;
		}
	}
	if (entry == 0) {
		entry = &linux_dir_cache[linux_dir_cache_next %
					 LINUX_DIR_CACHE_SIZE];
		linux_dir_cache_next++;
	}
	entry->valid = 0;
	entry->count = 0;
	string_copy(entry->path, path);
	entry->cache_domain = linux_cache_domain_for_path(0, path);
	entry->cache_epoch = linux_vfs_mutation_epochs[entry->cache_domain];
	return entry;
}

static long linux_dir_cache_fill(u64 handle, const char *path,
				 struct linux_dir_cache_entry **out)
{
	struct linux_dir_cache_entry *entry;
	long name_buffer;

	if (out == 0) {
		return -LINUX_EINVAL;
	}
	*out = 0;
	entry = linux_dir_cache_lookup(path);
	if (entry != 0) {
		*out = entry;
		return 1;
	}
	if (!linux_dir_cache_allowed(path)) {
		return 0;
	}
	name_buffer = bunix_buffer_create(LINUX_MAX_PATH);
	if (name_buffer <= 0) {
		return -LINUX_ENOMEM;
	}
	entry = linux_dir_cache_claim(path);
	for (u64 index = 0;; index++) {
		char name[LINUX_MAX_PATH];
		u64 type = BUNIX_VFS_TYPE_REGULAR;
		u64 next_offset = 0;
		const long result = linux_readdir_name(handle, index,
						       (u64)name_buffer,
						       name, sizeof(name),
						       &type, &next_offset);

		(void)next_offset;
		if (result == -(long)LINUX_ENOENT) {
			entry->valid = 1;
			*out = entry;
			bunix_handle_close((u64)name_buffer);
			return 1;
		}
		if (result != 0) {
			bunix_handle_close((u64)name_buffer);
			return result;
		}
		if (entry->count >= LINUX_DIR_CACHE_MAX_ENTRIES ||
		    string_len(name) > LINUX_NAME_MAX) {
			entry->valid = 0;
			entry->count = 0;
			bunix_handle_close((u64)name_buffer);
			return 0;
		}
		entry->entries[entry->count].type = type;
		string_copy_n(entry->entries[entry->count].name,
			      sizeof(entry->entries[entry->count].name),
			      name);
		entry->count++;
	}
}

static long linux_dir_cache_read(u64 handle, const char *path, u64 index,
				 char *name, u64 name_cap, u64 *type,
				 u64 *next_offset)
{
	struct linux_dir_cache_entry *entry = 0;
	const long cached = linux_dir_cache_fill(handle, path, &entry);

	if (cached <= 0) {
		return cached;
	}
	if (entry == 0) {
		return 0;
	}
	if (index >= entry->count) {
		return -LINUX_ENOENT;
	}
	if (name == 0 || name_cap == 0 ||
	    string_len(entry->entries[index].name) >= name_cap) {
		return -LINUX_EIO;
	}
	if (type != 0) {
		*type = entry->entries[index].type;
	}
	if (next_offset != 0) {
		*next_offset = index + 3;
	}
	string_copy_n(name, name_cap, entry->entries[index].name);
	return 1;
}

static long linux_getdents64(struct linux_process *process, u64 fd,
			     u64 buffer, u64 len)
{
	char *out;
	u64 written = 0;
	const u64 out_len = len > LINUX_MAX_DIRENT_BUFFER ?
			    LINUX_MAX_DIRENT_BUFFER : len;
	const long name_buffer = bunix_buffer_create(LINUX_MAX_PATH);

	if (fd >= process->fd_capacity || process->fds[fd].kind == 0) {
		if (name_buffer > 0) {
			bunix_handle_close((u64)name_buffer);
		}
		return -LINUX_EBADF;
	}
	if (process->fds[fd].kind != LINUX_FD_DIR) {
		if (name_buffer > 0) {
			bunix_handle_close((u64)name_buffer);
		}
		return -LINUX_ENOTDIR;
	}
	if (buffer == 0 || len == 0 || name_buffer <= 0) {
		if (name_buffer > 0) {
			bunix_handle_close((u64)name_buffer);
		}
		if (buffer == 0) {
			return -LINUX_EFAULT;
		}
		return -LINUX_EINVAL;
	}
	out = (char *)bunix_alloc(out_len);
	if (out == 0) {
		bunix_handle_close((u64)name_buffer);
		return -LINUX_ENOMEM;
	}
	linux_debug_getdents_log(process, fd, "start", 0, 0);

	while (written < out_len) {
		char name[LINUX_MAX_PATH];
		u64 type = BUNIX_VFS_TYPE_DIRECTORY;
		u64 current_offset = linux_fd_offset(&process->fds[fd]);
		u64 next_offset = current_offset;
		u64 reclen;

		for (u64 i = 0; i < sizeof(name); i++) {
			name[i] = '\0';
		}

		if (current_offset == 0) {
			name[0] = '.';
			name[1] = '\0';
			type = BUNIX_VFS_TYPE_DIRECTORY;
			next_offset = 1;
		} else if (current_offset == 1) {
			name[0] = '.';
			name[1] = '.';
			name[2] = '\0';
			type = BUNIX_VFS_TYPE_DIRECTORY;
			next_offset = 2;
		} else {
			const u64 read_index = current_offset - 2;
			const long cached_result =
				linux_dir_cache_read(process->fds[fd].handle,
						     process->fds[fd].path,
						     read_index, name,
						     sizeof(name), &type,
						     &next_offset);

			if (cached_result == 0) {
				linux_debug_getdents_log(process, fd,
							 "vfs-enter",
							 read_index, 0);
				const long result =
					linux_readdir_name(process->fds[fd].handle,
							   read_index,
							   (u64)name_buffer,
							   name, sizeof(name),
							   &type, &next_offset);
				linux_debug_getdents_log(process, fd,
							 "vfs-exit",
							 read_index, result);

				if (result == -(long)LINUX_ENOENT) {
					break;
				}
				if (result != 0) {
					bunix_handle_close((u64)name_buffer);
					bunix_free(out);
					return result;
				}
			} else if (cached_result == -(long)LINUX_ENOENT) {
				break;
			} else if (cached_result < 0) {
				bunix_handle_close((u64)name_buffer);
				bunix_free(out);
				return cached_result;
			}
			if (next_offset <= current_offset) {
				bunix_handle_close((u64)name_buffer);
				bunix_free(out);
				return -LINUX_EIO;
			}
		}

		reclen = linux_dirent_reclen(name);
		if (written + reclen > out_len) {
			break;
		}
		for (u64 i = 0; i < reclen; i++) {
			out[written + i] = 0;
		}
		linux_store_dirent(out, written, current_offset + 1,
				   next_offset, reclen,
				   type == BUNIX_VFS_TYPE_DIRECTORY ?
				   LINUX_DT_DIR :
				   (type == BUNIX_VFS_TYPE_SYMLINK ?
				    LINUX_DT_LNK :
				    (type == BUNIX_VFS_TYPE_FIFO ?
				     LINUX_DT_FIFO :
				     (type == BUNIX_VFS_TYPE_CHARACTER ?
				      LINUX_DT_CHR : LINUX_DT_REG))), name);
		written += reclen;
		linux_fd_set_offset(process, fd, next_offset);
		current_offset = next_offset;
	}

	if (bunix_buffer_write(buffer, 0, out, written) != 0) {
		bunix_handle_close((u64)name_buffer);
		bunix_free(out);
		return -LINUX_EFAULT;
	}
	bunix_handle_close((u64)name_buffer);
	bunix_free(out);
	return (long)written;
}

static long linux_getcwd(struct linux_process *process, u64 size,
			 u64 buffer, u64 *word0, u64 *word1)
{
	const u64 len = process->cwd != 0 ? string_len(process->cwd) + 1 : 0;

	if (len == 0) {
		return -LINUX_ENOENT;
	}
	if (size < len) {
		return -LINUX_ERANGE;
	}
	if (buffer != 0) {
		if (bunix_buffer_write(buffer, 0, process->cwd, len) != 0) {
			return -LINUX_EFAULT;
		}
		return (long)len;
	}
	(void)word0;
	(void)word1;
	return -LINUX_EFAULT;
}

static long linux_chdir(struct linux_process *process, u64 path_len,
			u64 path_buffer)
{
	char path[LINUX_MAX_PATH];
	char full_path[LINUX_MAX_PATH];
	struct bunix_msg reply;
	u64 base_handle;
	u64 old_handle;
	long base_result;
	long path_result;
	long normalize_result;

	path_result = linux_read_path_arg(path_buffer, path_len, path,
					  sizeof(path));
	if (path_result != 0) {
		return path_result;
	}
	if (path[0] == '\0') {
		return -LINUX_EINVAL;
	}
	normalize_result = path_normalize(process->cwd, path, full_path);
	if (normalize_result != 0) {
		return normalize_result;
	}
	base_result = linux_dirfd_base_handle(process, LINUX_AT_FDCWD, path,
					      &base_handle);
	if (base_result != 0) {
		return base_result;
	}

	if (linux_vfs_path_call(process, BUNIX_VFS_OPEN_BUFFER, base_handle,
				path, &reply) != 0) {
		return -LINUX_ENOENT;
	}
	if (reply.words[0] != 0) {
		return linux_vfs_error(reply.words[0]);
	}
	if (reply.words[3] != BUNIX_VFS_TYPE_DIRECTORY) {
		struct bunix_msg request = {
			.protocol = BUNIX_PROTO_VFS,
			.type = BUNIX_VFS_CLOSE,
			.sender = 0,
			.cap_rights = 0,
			.reply = 0,
			.cap = 0,
			.words = { reply.words[1], 0, 0, 0 },
		};

		(void)bunix_ipc_call(LINUX_HANDLE_VFS, &request, &reply);
		return -LINUX_ENOTDIR;
	}

	if (linux_process_set_cwd(process, full_path) != 0) {
		const u64 close_handle = reply.words[1];
		struct bunix_msg close_reply;
		struct bunix_msg close_request = {
			.protocol = BUNIX_PROTO_VFS,
			.type = BUNIX_VFS_CLOSE,
			.sender = 0,
			.cap_rights = 0,
			.reply = 0,
			.cap = 0,
			.words = { close_handle, 0, 0, 0 },
		};

		(void)bunix_ipc_call(LINUX_HANDLE_VFS, &close_request,
				     &close_reply);
		return -LINUX_ENOMEM;
	}
	old_handle = process->cwd_handle;
	process->cwd_handle = reply.words[1];
	linux_file_ref_add(process->cwd_handle);
	(void)linux_close_vfs_handle(old_handle);
	return 0;
}

static long linux_fchdir(struct linux_process *process, u64 fd)
{
	struct bunix_msg reply;
	u64 old_handle;

	if (fd >= process->fd_capacity || process->fds[fd].kind == 0) {
		return -LINUX_EBADF;
	}
	if (process->fds[fd].kind != LINUX_FD_DIR) {
		return -LINUX_ENOTDIR;
	}
	if (process->fds[fd].path[0] != '/') {
		return -LINUX_ENOENT;
	}
	if (linux_vfs_path_call(process, BUNIX_VFS_OPEN_BUFFER, 0,
				process->fds[fd].path, &reply) != 0) {
		return -LINUX_ENOENT;
	}
	if (reply.words[0] != 0) {
		return linux_vfs_error(reply.words[0]);
	}
	if (reply.words[3] != BUNIX_VFS_TYPE_DIRECTORY) {
		(void)linux_close_raw_vfs_handle(reply.words[1]);
		return -LINUX_ENOTDIR;
	}
	if (linux_process_set_cwd(process, process->fds[fd].path) != 0) {
		(void)linux_close_raw_vfs_handle(reply.words[1]);
		return -LINUX_ENOMEM;
	}

	old_handle = process->cwd_handle;
	process->cwd_handle = reply.words[1];
	linux_file_ref_add(process->cwd_handle);
	(void)linux_close_vfs_handle(old_handle);
	return 0;
}

struct linux_sockaddr {
	u64 family;
	u64 hi;
	u64 lo;
	u64 port;
};

static long linux_net_call(u64 type, u64 cap, u64 cap_rights,
			   u64 word0, u64 word1, u64 word2, u64 word3,
			   struct bunix_msg *reply)
{
	const u64 net = linux_cached_service(BUNIX_SERVICE_NET, &net_service);
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_NET,
		.type = (unsigned int)type,
		.sender = 0,
		.cap_rights = cap_rights,
		.reply = 0,
		.cap = cap,
		.words = { word0, word1, word2, word3 },
	};

	if (net == 0 || bunix_ipc_call(net, &request, reply) != 0 ||
	    reply->words[0] != 0) {
		return -1;
	}
	return 0;
}

static long linux_parse_sockaddr(u64 buffer, u64 len,
				 struct linux_sockaddr *out)
{
	unsigned char raw[28];
	u64 family;

	if (buffer == 0 || out == 0) {
		return -LINUX_EFAULT;
	}
	if (len > sizeof(raw)) {
		len = sizeof(raw);
	}
	if (len < 2 || bunix_buffer_read(buffer, 0, raw, len) != 0) {
		return -LINUX_EFAULT;
	}
	family = (u64)raw[0] | ((u64)raw[1] << 8);
	out->family = family;
	out->hi = 0;
	out->lo = 0;
	out->port = 0;
	if (family == LINUX_AF_INET) {
		if (len < 8) {
			return -LINUX_EINVAL;
		}
		out->port = ((u64)raw[2] << 8) | raw[3];
		out->lo = ((u64)raw[4] << 24) | ((u64)raw[5] << 16) |
			  ((u64)raw[6] << 8) | raw[7];
		return 0;
	}
	if (family == LINUX_AF_INET6) {
		if (len < 28) {
			return -LINUX_EINVAL;
		}
		out->port = ((u64)raw[2] << 8) | raw[3];
		for (u64 i = 8; i < 16; i++) {
			out->hi = (out->hi << 8) | raw[i];
		}
		for (u64 i = 16; i < 24; i++) {
			out->lo = (out->lo << 8) | raw[i];
		}
		return 0;
	}
	return -LINUX_EAFNOSUPPORT;
}

static long linux_parse_sockaddr_at(u64 buffer, u64 offset, u64 len,
				    struct linux_sockaddr *out)
{
	unsigned char raw[28];
	u64 family;

	if (buffer == 0 || out == 0) {
		return -LINUX_EFAULT;
	}
	if (len > sizeof(raw)) {
		len = sizeof(raw);
	}
	if (len < 2 || bunix_buffer_read(buffer, offset, raw, len) != 0) {
		return -LINUX_EFAULT;
	}
	family = (u64)raw[0] | ((u64)raw[1] << 8);
	out->family = family;
	out->hi = 0;
	out->lo = 0;
	out->port = 0;
	if (family == LINUX_AF_INET) {
		if (len < 8) {
			return -LINUX_EINVAL;
		}
		out->port = ((u64)raw[2] << 8) | raw[3];
		out->lo = ((u64)raw[4] << 24) | ((u64)raw[5] << 16) |
			  ((u64)raw[6] << 8) | raw[7];
		return 0;
	}
	if (family == LINUX_AF_INET6) {
		if (len < 28) {
			return -LINUX_EINVAL;
		}
		out->port = ((u64)raw[2] << 8) | raw[3];
		for (u64 i = 8; i < 16; i++) {
			out->hi = (out->hi << 8) | raw[i];
		}
		for (u64 i = 16; i < 24; i++) {
			out->lo = (out->lo << 8) | raw[i];
		}
		return 0;
	}
	return -LINUX_EAFNOSUPPORT;
}

static long linux_parse_sockaddr_ll(u64 buffer, u64 len, u64 *protocol,
				    u64 *ifindex)
{
	unsigned char raw[20];
	u64 family;

	if (buffer == 0 || protocol == 0 || ifindex == 0) {
		return -LINUX_EFAULT;
	}
	if (len > sizeof(raw)) {
		len = sizeof(raw);
	}
	if (len < 8 || bunix_buffer_read(buffer, 0, raw, len) != 0) {
		return -LINUX_EFAULT;
	}
	family = (u64)raw[0] | ((u64)raw[1] << 8);
	if (family != LINUX_AF_PACKET) {
		return -LINUX_EAFNOSUPPORT;
	}
	*protocol = ((u64)raw[2] << 8) | raw[3];
	*ifindex = (u64)raw[4] | ((u64)raw[5] << 8) |
		   ((u64)raw[6] << 16) | ((u64)raw[7] << 24);
	return 0;
}

static long linux_parse_sockaddr_ll_at(u64 buffer, u64 offset, u64 len,
				       u64 *protocol, u64 *ifindex,
				       unsigned char *addr, u64 *addr_len)
{
	unsigned char raw[20];
	u64 family;

	for (u64 i = 0; i < sizeof(raw); i++) {
		raw[i] = 0;
	}
	if (len > sizeof(raw)) {
		len = sizeof(raw);
	}
	if (buffer == 0 || len < 8 ||
	    bunix_buffer_read(buffer, offset, raw, len) != 0) {
		return -LINUX_EFAULT;
	}
	family = (u64)raw[0] | ((u64)raw[1] << 8);
	if (family != LINUX_AF_PACKET) {
		return -LINUX_EAFNOSUPPORT;
	}
	*protocol = ((u64)raw[2] << 8) | raw[3];
	*ifindex = (u64)raw[4] | ((u64)raw[5] << 8) |
		   ((u64)raw[6] << 16) | ((u64)raw[7] << 24);
	if (addr != 0 && addr_len != 0) {
		*addr_len = len >= 12 ? raw[11] : 0;
		if (*addr_len > 8) {
			*addr_len = 8;
		}
		for (u64 i = 0; i < *addr_len; i++) {
			addr[i] = raw[12 + i];
		}
	}
	return 0;
}

static long linux_net_datagram_sendto(u64 op, u64 socket, u64 payload_buffer,
				      u64 payload_offset, u64 payload_len,
				      const struct linux_sockaddr *dest)
{
	const u64 header_len = 4 * sizeof(u64);
	const long net_buffer = bunix_buffer_create(header_len + payload_len);
	struct bunix_msg reply;
	u64 header[4];

	if (net_buffer < 0) {
		return -LINUX_ENOMEM;
	}
	header[0] = dest->family == LINUX_AF_INET ?
		    BUNIX_NET_ADDR_FAMILY_IPV4 : BUNIX_NET_ADDR_FAMILY_IPV6;
	header[1] = dest->hi;
	header[2] = dest->lo;
	header[3] = dest->port;
	if (bunix_buffer_write(net_buffer, 0, header, sizeof(header)) != 0) {
		bunix_handle_close((u64)net_buffer);
		return -LINUX_EFAULT;
	}
	for (u64 done = 0; done < payload_len;) {
		unsigned char chunk[256];
		u64 copy = payload_len - done;

		if (copy > sizeof(chunk)) {
			copy = sizeof(chunk);
		}
		if (bunix_buffer_read(payload_buffer, payload_offset + done,
				      chunk, copy) != 0 ||
		    bunix_buffer_write(net_buffer, header_len + done,
				       chunk, copy) != 0) {
			bunix_handle_close((u64)net_buffer);
			return -LINUX_EFAULT;
		}
		done += copy;
	}
	if (linux_net_call(op, (u64)net_buffer, BUNIX_RIGHT_RECV, socket,
			   payload_len, 0, 0, &reply) != 0) {
		bunix_handle_close((u64)net_buffer);
		return -LINUX_EPIPE;
	}
	bunix_handle_close((u64)net_buffer);
	return (long)reply.words[1];
}

static long linux_net_raw_ipv4_sendto(u64 payload_buffer, u64 payload_offset,
				      u64 payload_len,
				      const struct linux_sockaddr *dest)
{
	const u64 frame_len = LINUX_ETHERNET_HEADER_SIZE + payload_len;
	const long iface = linux_net_default_packet_interface();
	struct bunix_net_packet_interface_info iface_info;
	struct bunix_net_packet_info packet_info;
	struct bunix_msg reply;
	long net_buffer;
	unsigned char header[LINUX_ETHERNET_HEADER_SIZE];

	if (dest == 0 || dest->family != LINUX_AF_INET ||
	    payload_len == 0 || payload_len > 1500 ||
	    frame_len > 2048 || iface < 0) {
		return -LINUX_EINVAL;
	}
	if (linux_net_interface_details((u64)iface, &iface_info) != 0) {
		return -LINUX_ENODEV;
	}
	net_buffer = bunix_buffer_create(sizeof(packet_info) + frame_len);
	if (net_buffer < 0) {
		return -LINUX_ENOMEM;
	}
	packet_info.iface = (u64)iface;
	packet_info.len = frame_len;
	packet_info.flags = 0;
	packet_info.reserved = 0;
	for (u64 i = 0; i < 6; i++) {
		header[i] = 0xff;
	}
	linux_store_ether_mac(header + 6, iface_info.mac_hi);
	header[12] = 0x08;
	header[13] = 0x00;
	if (bunix_buffer_write((u64)net_buffer, 0, &packet_info,
			       sizeof(packet_info)) != 0 ||
	    bunix_buffer_write((u64)net_buffer, sizeof(packet_info), header,
			       sizeof(header)) != 0) {
		bunix_handle_close((u64)net_buffer);
		return -LINUX_EFAULT;
	}
	for (u64 done = 0; done < payload_len;) {
		unsigned char chunk[256];
		u64 copy = payload_len - done;

		if (copy > sizeof(chunk)) {
			copy = sizeof(chunk);
		}
		if (bunix_buffer_read(payload_buffer, payload_offset + done,
				      chunk, copy) != 0 ||
		    bunix_buffer_write((u64)net_buffer,
				       sizeof(packet_info) +
					       LINUX_ETHERNET_HEADER_SIZE +
					       done,
				       chunk, copy) != 0) {
			bunix_handle_close((u64)net_buffer);
			return -LINUX_EFAULT;
		}
		done += copy;
	}
	if (linux_net_request(BUNIX_NET_PACKET_TX_ENQUEUE, (u64)net_buffer,
			      BUNIX_RIGHT_RECV, 0, 0, 0, 0, &reply) != 0) {
		bunix_handle_close((u64)net_buffer);
		return -LINUX_EPIPE;
	}
	bunix_handle_close((u64)net_buffer);
	return (long)payload_len;
}

static long linux_net_raw_ipv4_recv(u64 buffer, u64 len)
{
	const long iface = linux_net_default_packet_interface();
	struct bunix_msg reply;

	if (iface < 0 || buffer == 0 || len == 0) {
		return -LINUX_EAGAIN;
	}
	if (linux_net_request(BUNIX_NET_PACKET_RX_DEQUEUE, buffer,
			      BUNIX_RIGHT_SEND, (u64)iface, len,
			      LINUX_ETH_P_IP, LINUX_ETHERNET_HEADER_SIZE,
			      &reply) != 0) {
		return -LINUX_EAGAIN;
	}
	return (long)reply.words[2];
}

static long linux_net_raw_ipv4_poll(void)
{
	const long iface = linux_net_default_packet_interface();
	struct bunix_msg reply;

	if (iface < 0) {
		return -1;
	}
	if (linux_net_request(BUNIX_NET_PACKET_RX_POLL, 0, 0, (u64)iface,
			      LINUX_ETH_P_IP, 0, 0, &reply) != 0) {
		return -1;
	}
	return (long)reply.words[1];
}

static long linux_net_packet_sendto(u64 payload_buffer, u64 payload_offset,
				    u64 payload_len, u64 ifindex,
				    const unsigned char *dest_mac,
				    u64 dest_mac_len)
{
	const u64 frame_len = LINUX_ETHERNET_HEADER_SIZE + payload_len;
	struct bunix_net_packet_interface_info iface_info;
	struct bunix_net_packet_info packet_info;
	struct bunix_msg reply;
	long net_buffer;
	unsigned char header[LINUX_ETHERNET_HEADER_SIZE];

	if (ifindex == 0 || payload_len == 0 || payload_len > 1500 ||
	    frame_len > 2048 ||
	    linux_net_interface_details(ifindex, &iface_info) != 0) {
		return -LINUX_EINVAL;
	}
	net_buffer = bunix_buffer_create(sizeof(packet_info) + frame_len);
	if (net_buffer < 0) {
		return -LINUX_ENOMEM;
	}
	for (u64 i = 0; i < 6; i++) {
		header[i] = dest_mac != 0 && dest_mac_len >= 6 ?
			    dest_mac[i] : 0xff;
	}
	linux_store_ether_mac(header + 6, iface_info.mac_hi);
	header[12] = 0x08;
	header[13] = 0x00;
	packet_info.iface = ifindex;
	packet_info.len = frame_len;
	packet_info.flags = 0;
	packet_info.reserved = 0;
	if (bunix_buffer_write((u64)net_buffer, 0, &packet_info,
			       sizeof(packet_info)) != 0 ||
	    bunix_buffer_write((u64)net_buffer, sizeof(packet_info), header,
			       sizeof(header)) != 0) {
		bunix_handle_close((u64)net_buffer);
		return -LINUX_EFAULT;
	}
	for (u64 done = 0; done < payload_len;) {
		unsigned char chunk[256];
		u64 copy = payload_len - done;

		if (copy > sizeof(chunk)) {
			copy = sizeof(chunk);
		}
		if (bunix_buffer_read(payload_buffer, payload_offset + done,
				      chunk, copy) != 0 ||
		    bunix_buffer_write((u64)net_buffer,
				       sizeof(packet_info) +
					       LINUX_ETHERNET_HEADER_SIZE +
					       done,
				       chunk, copy) != 0) {
			bunix_handle_close((u64)net_buffer);
			return -LINUX_EFAULT;
		}
		done += copy;
	}
	if (linux_net_request(BUNIX_NET_PACKET_TX_ENQUEUE, (u64)net_buffer,
			      BUNIX_RIGHT_RECV, 0, 0, 0, 0, &reply) != 0) {
		bunix_handle_close((u64)net_buffer);
		return -LINUX_EPIPE;
	}
	bunix_handle_close((u64)net_buffer);
	return (long)payload_len;
}

static u64 linux_sockaddr_len(u64 family)
{
	return family == LINUX_AF_INET6 ? 28 : 16;
}

static long linux_write_sockaddr(u64 buffer, u64 max_len,
				 const struct linux_sockaddr *addr)
{
	char raw[28];
	u64 actual;
	u64 copy;

	if (buffer == 0 || addr == 0) {
		return -LINUX_EFAULT;
	}
	zero_bytes(raw, sizeof(raw));
	actual = linux_sockaddr_len(addr->family);
	raw[0] = (unsigned char)(addr->family & 0xff);
	raw[1] = (unsigned char)((addr->family >> 8) & 0xff);
	raw[2] = (unsigned char)((addr->port >> 8) & 0xff);
	raw[3] = (unsigned char)(addr->port & 0xff);
	if (addr->family == LINUX_AF_INET) {
		raw[4] = (unsigned char)((addr->lo >> 24) & 0xff);
		raw[5] = (unsigned char)((addr->lo >> 16) & 0xff);
		raw[6] = (unsigned char)((addr->lo >> 8) & 0xff);
		raw[7] = (unsigned char)(addr->lo & 0xff);
	} else if (addr->family == LINUX_AF_INET6) {
		u64 hi = addr->hi;
		u64 lo = addr->lo;

		for (u64 i = 0; i < 8; i++) {
			raw[15 - i] = (unsigned char)(hi & 0xff);
			hi >>= 8;
		}
		for (u64 i = 0; i < 8; i++) {
			raw[23 - i] = (unsigned char)(lo & 0xff);
			lo >>= 8;
		}
	} else {
		return -LINUX_EAFNOSUPPORT;
	}
	copy = actual < max_len ? actual : max_len;
	if (copy != 0 && bunix_buffer_write(buffer, 0, raw, copy) != 0) {
		return -LINUX_EFAULT;
	}
	return (long)actual;
}

static long linux_write_sockaddr_at(u64 buffer, u64 offset, u64 max_len,
				    const struct linux_sockaddr *addr)
{
	char raw[28];
	u64 actual;
	u64 copy;

	if (buffer == 0 || addr == 0) {
		return -LINUX_EFAULT;
	}
	zero_bytes(raw, sizeof(raw));
	actual = linux_sockaddr_len(addr->family);
	raw[0] = (unsigned char)(addr->family & 0xff);
	raw[1] = (unsigned char)((addr->family >> 8) & 0xff);
	raw[2] = (unsigned char)((addr->port >> 8) & 0xff);
	raw[3] = (unsigned char)(addr->port & 0xff);
	if (addr->family == LINUX_AF_INET) {
		raw[4] = (unsigned char)((addr->lo >> 24) & 0xff);
		raw[5] = (unsigned char)((addr->lo >> 16) & 0xff);
		raw[6] = (unsigned char)((addr->lo >> 8) & 0xff);
		raw[7] = (unsigned char)(addr->lo & 0xff);
	} else if (addr->family == LINUX_AF_INET6) {
		u64 hi = addr->hi;
		u64 lo = addr->lo;

		for (u64 i = 0; i < 8; i++) {
			raw[15 - i] = (unsigned char)(hi & 0xff);
			hi >>= 8;
		}
		for (u64 i = 0; i < 8; i++) {
			raw[23 - i] = (unsigned char)(lo & 0xff);
			lo >>= 8;
		}
	} else {
		return -LINUX_EAFNOSUPPORT;
	}
	copy = actual < max_len ? actual : max_len;
	if (copy != 0 && bunix_buffer_write(buffer, offset, raw, copy) != 0) {
		return -LINUX_EFAULT;
	}
	return (long)actual;
}

static long linux_sockaddr_from_bunix_net(
	const struct bunix_net_endpoint_addr *source,
	struct linux_sockaddr *addr)
{
	if (source == 0 || addr == 0) {
		return -LINUX_EFAULT;
	}
	if (source->family == BUNIX_NET_ADDR_FAMILY_IPV4) {
		addr->family = LINUX_AF_INET;
		addr->hi = 0;
		addr->lo = source->addr_lo;
		addr->port = source->port;
		return 0;
	}
	if (source->family == BUNIX_NET_ADDR_FAMILY_IPV6) {
		addr->family = LINUX_AF_INET6;
		addr->hi = source->addr_hi;
		addr->lo = source->addr_lo;
		addr->port = source->port;
		return 0;
	}
	return -LINUX_EAFNOSUPPORT;
}

static void linux_net_write_be16(unsigned char *data, u64 value)
{
	data[0] = (unsigned char)((value >> 8) & 0xff);
	data[1] = (unsigned char)(value & 0xff);
}

static void linux_net_write_be32(unsigned char *data, u64 value)
{
	data[0] = (unsigned char)((value >> 24) & 0xff);
	data[1] = (unsigned char)((value >> 16) & 0xff);
	data[2] = (unsigned char)((value >> 8) & 0xff);
	data[3] = (unsigned char)(value & 0xff);
}

static u64 linux_net_checksum_sum(const unsigned char *data, u64 len, u64 sum)
{
	for (u64 i = 0; i < len; i++) {
		sum += (i & 1) == 0 ? ((u64)data[i] << 8) : data[i];
		while (sum >> 16) {
			sum = (sum & 0xffff) + (sum >> 16);
		}
	}
	return sum;
}

static unsigned short linux_net_checksum_finish(u64 sum)
{
	while (sum >> 16) {
		sum = (sum & 0xffff) + (sum >> 16);
	}
	return (unsigned short)(~sum & 0xffff);
}

static u64 linux_net_default_ipv4_addr(void)
{
	const long default_iface = linux_net_default_packet_interface();
	struct bunix_msg reply;
	u64 count;

	if (linux_net_request(BUNIX_NET_ADDR_COUNT, 0, 0, 0, 0, 0, 0,
			      &reply) != 0) {
		return 0;
	}
	count = reply.words[1];
	for (u64 i = 0; i < count; i++) {
		struct bunix_net_addr_info addr;
		long buffer = bunix_buffer_create(sizeof(addr));
		int ok;

		if (buffer <= 0) {
			return 0;
		}
		ok = linux_net_request(BUNIX_NET_ADDR_AT, (u64)buffer,
				       BUNIX_RIGHT_SEND, i, 0, 0, 0,
				       &reply) == 0 &&
		     bunix_buffer_read((u64)buffer, 0, &addr, sizeof(addr)) == 0;
		bunix_handle_close((u64)buffer);
		if (!ok || addr.family != BUNIX_NET_ADDR_FAMILY_IPV4) {
			continue;
		}
		if (default_iface >= 0 && addr.iface == (u64)default_iface) {
			return addr.addr_lo;
		}
		if (default_iface < 0 && addr.addr_lo != 0x7f000001) {
			return addr.addr_lo;
		}
	}
	return 0;
}

static long linux_icmp_raw_ipv4_recv(struct linux_process *process, u64 fd,
				     u64 len, u64 buffer, u64 addr_len,
				     u64 *actual_addr_len)
{
	struct bunix_msg reply;
	unsigned char header[20];
	u64 icmp_len;
	u64 packet_len;
	u64 source_ip;
	u64 dest_ip;
	u64 ttl;
	unsigned short checksum;

	if (process == 0 || fd >= process->fd_capacity || len <= 20 ||
	    buffer == 0) {
		return -LINUX_EAGAIN;
	}
	if (linux_net_call(BUNIX_NET_ICMP_RECV, buffer, BUNIX_RIGHT_SEND,
			   process->fds[fd].offset, len - 20, 20, 0,
			   &reply) != 0) {
		return -LINUX_EAGAIN;
	}
	icmp_len = reply.words[1];
	packet_len = 20 + icmp_len;
	if (packet_len > len) {
		return -LINUX_EFAULT;
	}
	source_ip = reply.words[3];
	ttl = reply.words[2] & 0xff;
	if (ttl == 0) {
		ttl = 64;
	}
	dest_ip = linux_net_default_ipv4_addr();
	zero_bytes((char *)header, sizeof(header));
	header[0] = 0x45;
	linux_net_write_be16(header + 2, packet_len);
	header[8] = (unsigned char)ttl;
	header[9] = LINUX_IPPROTO_ICMP;
	linux_net_write_be32(header + 12, source_ip);
	linux_net_write_be32(header + 16, dest_ip);
	checksum = linux_net_checksum_finish(
		linux_net_checksum_sum(header, sizeof(header), 0));
	header[10] = (unsigned char)((checksum >> 8) & 0xff);
	header[11] = (unsigned char)(checksum & 0xff);
	if (bunix_buffer_write(buffer, 0, header, sizeof(header)) != 0) {
		return -LINUX_EFAULT;
	}
	if (addr_len != 0) {
		const struct linux_sockaddr addr = {
			.family = LINUX_AF_INET,
			.hi = 0,
			.lo = source_ip,
			.port = 0,
		};
		const long actual = linux_write_sockaddr_at(buffer, packet_len,
							    addr_len, &addr);

		if (actual < 0) {
			return actual;
		}
		if (actual_addr_len != 0) {
			*actual_addr_len = (u64)actual;
		}
	}
	return (long)packet_len;
}

static long linux_socket_addr(struct linux_process *process, u64 fd, u64 max_len,
			      u64 buffer, int peer, u64 *actual_len)
{
	struct linux_sockaddr addr;
	struct bunix_msg reply;
	u64 op;
	long written;

	if (actual_len != 0) {
		*actual_len = 0;
	}
	if (fd >= process->fd_capacity ||
	    process->fds[fd].kind != LINUX_FD_SOCKET) {
		return -LINUX_ENOTSOCK;
	}
	if (process->fds[fd].handle == LINUX_SOCKET_NETLINK_ROUTE) {
		unsigned char raw[12];
		u64 copy;

		(void)peer;
		zero_bytes((char *)raw, sizeof(raw));
		if (actual_len != 0) {
			*actual_len = sizeof(raw);
		}
		raw[0] = (unsigned char)(LINUX_AF_NETLINK & 0xff);
		raw[1] = (unsigned char)((LINUX_AF_NETLINK >> 8) & 0xff);
		raw[4] = (unsigned char)(process->pid & 0xff);
		raw[5] = (unsigned char)((process->pid >> 8) & 0xff);
		raw[6] = (unsigned char)((process->pid >> 16) & 0xff);
		raw[7] = (unsigned char)((process->pid >> 24) & 0xff);
		copy = sizeof(raw) < max_len ? sizeof(raw) : max_len;
		if (copy != 0 && bunix_buffer_write(buffer, 0, raw, copy) != 0) {
			return -LINUX_EFAULT;
		}
		return 0;
	}
	if (process->fds[fd].handle != LINUX_SOCKET_NET_UDP &&
	    process->fds[fd].handle != LINUX_SOCKET_NET_TCP) {
		return -LINUX_EINVAL;
	}
	if (process->fds[fd].handle == LINUX_SOCKET_NET_UDP) {
		op = peer ? BUNIX_NET_UDP_PEER : BUNIX_NET_UDP_LOCAL;
	} else {
		op = peer ? BUNIX_NET_TCP_PEER : BUNIX_NET_TCP_LOCAL;
	}
	if (linux_net_call(op, 0, 0, process->fds[fd].offset, 0, 0, 0,
			   &reply) != 0) {
		return peer ? -LINUX_ENOTCONN : -LINUX_EINVAL;
	}
	addr.family = process->fds[fd].size;
	addr.hi = reply.words[1];
	addr.lo = reply.words[2];
	addr.port = reply.words[3];
	written = linux_write_sockaddr(buffer, max_len, &addr);
	if (written < 0) {
		return written;
	}
	if (actual_len != 0) {
		*actual_len = (u64)written;
	}
	return 0;
}

static u64 linux_nl_align(u64 len)
{
	return (len + 3) & ~3ull;
}

static int linux_netlink_queue_bytes(struct linux_fd *fd, const char *data,
				     u64 len)
{
	if (fd == 0 || data == 0 || len > sizeof(fd->path) - fd->size) {
		return -1;
	}
	for (u64 i = 0; i < len; i++) {
		fd->path[fd->size + i] = data[i];
	}
	fd->size += len;
	return 0;
}

static int linux_netlink_queue_msg(struct linux_fd *fd, u64 type, u64 flags,
				   u64 seq, const char *payload,
				   u64 payload_len)
{
	char header[16];
	char pad[4] = { 0, 0, 0, 0 };
	const u64 msg_len = 16 + payload_len;
	const u64 aligned = linux_nl_align(msg_len);

	if (aligned > sizeof(fd->path) - fd->size) {
		return -1;
	}
	zero_bytes(header, sizeof(header));
	store_u32(header, 0, (unsigned int)msg_len);
	store_u16(header, 4, (unsigned int)type);
	store_u16(header, 6, (unsigned int)flags);
	store_u32(header, 8, (unsigned int)seq);
	if (linux_netlink_queue_bytes(fd, header, sizeof(header)) != 0) {
		return -1;
	}
	if (payload_len != 0 &&
	    linux_netlink_queue_bytes(fd, payload, payload_len) != 0) {
		return -1;
	}
	if (aligned != msg_len &&
	    linux_netlink_queue_bytes(fd, pad, aligned - msg_len) != 0) {
		return -1;
	}
	return 0;
}

static void linux_netlink_queue_stamp_pid(struct linux_fd *fd, u64 pid)
{
	u64 off = 0;

	if (fd == 0) {
		return;
	}
	while (off + 16 <= fd->size) {
		const u64 msg_len = load_u32(fd->path, off);
		const u64 aligned = linux_nl_align(msg_len);

		if (msg_len < 16 || aligned == 0 || off + aligned > fd->size) {
			return;
		}
		store_u32(fd->path, off + 12, (unsigned int)pid);
		off += aligned;
	}
}

static int linux_netlink_queue_error(struct linux_fd *fd, u64 seq, long error,
				     const char *request)
{
	char payload[20];

	zero_bytes(payload, sizeof(payload));
	store_u32(payload, 0, (unsigned int)error);
	if (request != 0) {
		for (u64 i = 0; i < 16; i++) {
			payload[4 + i] = request[i];
		}
	}
	return linux_netlink_queue_msg(fd, LINUX_NLMSG_ERROR, 0, seq, payload,
				       sizeof(payload));
}

static int linux_netlink_queue_done(struct linux_fd *fd, u64 seq)
{
	char payload[4] = { 0, 0, 0, 0 };

	return linux_netlink_queue_msg(fd, LINUX_NLMSG_DONE, LINUX_NLM_F_MULTI,
				       seq, payload, sizeof(payload));
}

static int linux_nl_attr_put(char *payload, u64 *len, u64 max, u64 type,
			     const void *data, u64 data_len)
{
	char pad[4] = { 0, 0, 0, 0 };
	const u64 attr_len = 4 + data_len;
	const u64 aligned = linux_nl_align(attr_len);

	if (payload == 0 || len == 0 || data == 0 || aligned > max - *len) {
		return -1;
	}
	store_u16(payload, *len, (unsigned int)attr_len);
	store_u16(payload, *len + 2, (unsigned int)type);
	for (u64 i = 0; i < data_len; i++) {
		payload[*len + 4 + i] = ((const char *)data)[i];
	}
	for (u64 i = 0; i < aligned - attr_len; i++) {
		payload[*len + attr_len + i] = pad[i];
	}
	*len += aligned;
	return 0;
}

static int linux_nl_attr_put_u32(char *payload, u64 *len, u64 max, u64 type,
				 u64 value)
{
	char data[4];

	store_u32(data, 0, (unsigned int)value);
	return linux_nl_attr_put(payload, len, max, type, data, sizeof(data));
}

static int linux_nl_attr_put_ipv4(char *payload, u64 *len, u64 max, u64 type,
				  u64 value)
{
	unsigned char data[4];

	data[0] = (unsigned char)((value >> 24) & 0xff);
	data[1] = (unsigned char)((value >> 16) & 0xff);
	data[2] = (unsigned char)((value >> 8) & 0xff);
	data[3] = (unsigned char)(value & 0xff);
	return linux_nl_attr_put(payload, len, max, type, data, sizeof(data));
}

static int linux_nl_attr_find(const char *attrs, u64 len, u64 type,
			      const char **data, u64 *data_len)
{
	u64 off = 0;

	while (off + 4 <= len) {
		const u64 attr_len = load_u16(attrs, off);
		const u64 attr_type = load_u16(attrs, off + 2);

		if (attr_len < 4 || off + attr_len > len) {
			return 0;
		}
		if (attr_type == type) {
			if (data != 0) {
				*data = attrs + off + 4;
			}
			if (data_len != 0) {
				*data_len = attr_len - 4;
			}
			return 1;
		}
		off += linux_nl_align(attr_len);
	}
	return 0;
}

static int linux_nl_attr_read_u32(const char *attrs, u64 len, u64 type,
				  u64 *out)
{
	const char *data;
	u64 data_len;

	if (!linux_nl_attr_find(attrs, len, type, &data, &data_len) ||
	    data_len < 4 || out == 0) {
		return 0;
	}
	*out = load_u32(data, 0);
	return 1;
}

static int linux_nl_attr_read_ipv4(const char *attrs, u64 len, u64 type,
				   u64 *out)
{
	const char *data;
	u64 data_len;

	if (!linux_nl_attr_find(attrs, len, type, &data, &data_len) ||
	    data_len < 4 || out == 0) {
		return 0;
	}
	*out = ((u64)(unsigned char)data[0] << 24) |
	       ((u64)(unsigned char)data[1] << 16) |
	       ((u64)(unsigned char)data[2] << 8) |
	       (u64)(unsigned char)data[3];
	return 1;
}

static int linux_nl_attr_read_name(const char *attrs, u64 len, u64 type,
				   char *out, u64 out_len)
{
	const char *data;
	u64 data_len;
	u64 copy;

	if (!linux_nl_attr_find(attrs, len, type, &data, &data_len) ||
	    out == 0 || out_len == 0) {
		return 0;
	}
	zero_bytes(out, out_len);
	copy = data_len < out_len - 1 ? data_len : out_len - 1;
	for (u64 i = 0; i < copy && data[i] != '\0'; i++) {
		out[i] = data[i];
	}
	return out[0] != '\0';
}

static long linux_netlink_iface_from_name_or_index(const char *name, u64 index)
{
	char ifreq[LINUX_IFNAMSIZ];

	if (index != 0) {
		struct bunix_net_packet_interface_info info;

		if (linux_net_interface_details(index, &info) == 0) {
			return (long)index;
		}
		return -LINUX_ENODEV;
	}
	if (name == 0 || name[0] == '\0') {
		return -LINUX_ENODEV;
	}
	zero_bytes(ifreq, sizeof(ifreq));
	copy_field(ifreq, 0, sizeof(ifreq), name);
	return linux_net_interface_id_by_name(ifreq);
}

static int linux_netlink_addr_rpc(u64 op,
				  const struct bunix_net_addr_info *addr)
{
	long buffer;
	struct bunix_msg reply;
	int ok;

	if (addr == 0) {
		return -1;
	}
	buffer = bunix_buffer_create(sizeof(*addr));
	if (buffer <= 0) {
		return -1;
	}
	ok = bunix_buffer_write((u64)buffer, 0, addr, sizeof(*addr)) == 0 &&
	     linux_net_request(op, (u64)buffer, BUNIX_RIGHT_RECV, 0, 0, 0,
			       0, &reply) == 0;
	bunix_handle_close((u64)buffer);
	return ok ? 0 : -1;
}

static int linux_netlink_route_rpc(u64 op,
				   const struct bunix_net_route_info *route)
{
	long buffer;
	struct bunix_msg reply;
	int ok;

	if (route == 0) {
		return -1;
	}
	buffer = bunix_buffer_create(sizeof(*route));
	if (buffer <= 0) {
		return -1;
	}
	ok = bunix_buffer_write((u64)buffer, 0, route, sizeof(*route)) == 0 &&
	     linux_net_request(op, (u64)buffer, BUNIX_RIGHT_RECV, 0, 0, 0,
			       0, &reply) == 0;
	bunix_handle_close((u64)buffer);
	return ok ? 0 : -1;
}

static int linux_netlink_queue_link(struct linux_fd *fd, u64 seq, u64 iface)
{
	struct bunix_net_packet_interface_info info;
	char payload[160];
	char name[LINUX_IFNAMSIZ];
	unsigned char mac[6];
	u64 len = 16;

	if (linux_net_interface_details(iface, &info) != 0) {
		return 0;
	}
	zero_bytes(payload, sizeof(payload));
	payload[0] = 0;
	store_u16(payload, 2, info.id == 1 ? LINUX_ARPHRD_LOOPBACK :
		  LINUX_ARPHRD_ETHER);
	store_u32(payload, 4, (unsigned int)info.id);
	store_u32(payload, 8, linux_ifflags_from_net(info.flags));
	store_u32(payload, 12, 0xffffffffu);
	linux_net_interface_name(info.id, name, sizeof(name));
	linux_store_ether_mac(mac, info.mac_hi);
	if (linux_nl_attr_put(payload, &len, sizeof(payload),
			      LINUX_IFLA_IFNAME, name,
			      string_len(name) + 1) != 0 ||
	    linux_nl_attr_put(payload, &len, sizeof(payload),
			      LINUX_IFLA_ADDRESS, mac, sizeof(mac)) != 0 ||
	    linux_nl_attr_put_u32(payload, &len, sizeof(payload),
				  LINUX_IFLA_MTU, info.mtu) != 0) {
		return -1;
	}
	return linux_netlink_queue_msg(fd, LINUX_RTM_NEWLINK,
				       LINUX_NLM_F_MULTI, seq, payload, len);
}

static int linux_netlink_queue_links(struct linux_fd *fd, u64 seq,
				     const char *filter_name, u64 filter_index)
{
	struct bunix_msg reply;
	u64 count;

	if (linux_net_request(BUNIX_NET_INTERFACE_COUNT, 0, 0, 0, 0, 0, 0,
			      &reply) != 0) {
		return -1;
	}
	count = reply.words[1];
	for (u64 i = 0; i < count; i++) {
		char name[LINUX_IFNAMSIZ];
		u64 iface;

		if (linux_net_request(BUNIX_NET_INTERFACE_AT, 0, 0, i, 0, 0,
				      0, &reply) != 0) {
			return -1;
		}
		iface = reply.words[1];
		linux_net_interface_name(iface, name, sizeof(name));
		if (filter_index != 0 && iface != filter_index) {
			continue;
		}
		if (filter_name != 0 && filter_name[0] != '\0' &&
		    !string_equal(name, filter_name)) {
			continue;
		}
		if (linux_netlink_queue_link(fd, seq, iface) != 0) {
			return -1;
		}
	}
	return linux_netlink_queue_done(fd, seq);
}

static int linux_netlink_queue_addr(struct linux_fd *fd, u64 seq,
				    const struct bunix_net_addr_info *addr)
{
	char payload[128];
	char name[LINUX_IFNAMSIZ];
	u64 len = 8;

	if (addr == 0 || addr->family != BUNIX_NET_ADDR_FAMILY_IPV4) {
		return 0;
	}
	zero_bytes(payload, sizeof(payload));
	payload[0] = LINUX_AF_INET;
	payload[1] = (char)addr->prefix_len;
	payload[2] = 0x80;
	payload[3] = 0;
	store_u32(payload, 4, (unsigned int)addr->iface);
	linux_net_interface_name(addr->iface, name, sizeof(name));
	if (linux_nl_attr_put_ipv4(payload, &len, sizeof(payload),
				   LINUX_IFA_ADDRESS, addr->addr_lo) != 0 ||
	    linux_nl_attr_put_ipv4(payload, &len, sizeof(payload),
				   LINUX_IFA_LOCAL, addr->addr_lo) != 0 ||
	    linux_nl_attr_put(payload, &len, sizeof(payload),
			      LINUX_IFA_LABEL, name,
			      string_len(name) + 1) != 0) {
		return -1;
	}
	return linux_netlink_queue_msg(fd, LINUX_RTM_NEWADDR,
				       LINUX_NLM_F_MULTI, seq, payload, len);
}

static int linux_netlink_queue_addrs(struct linux_fd *fd, u64 seq,
				     u64 filter_index)
{
	struct bunix_msg reply;
	u64 count;

	if (linux_net_request(BUNIX_NET_ADDR_COUNT, 0, 0, 0, 0, 0, 0,
			      &reply) != 0) {
		return -1;
	}
	count = reply.words[1];
	for (u64 i = 0; i < count; i++) {
		struct bunix_net_addr_info addr;
		long buffer = bunix_buffer_create(sizeof(addr));
		int ok;

		if (buffer <= 0) {
			return -1;
		}
		ok = linux_net_request(BUNIX_NET_ADDR_AT, (u64)buffer,
				       BUNIX_RIGHT_SEND, i, 0, 0, 0,
				       &reply) == 0 &&
		     bunix_buffer_read((u64)buffer, 0, &addr, sizeof(addr)) == 0;
		bunix_handle_close((u64)buffer);
		if (!ok || addr.family != BUNIX_NET_ADDR_FAMILY_IPV4 ||
		    (filter_index != 0 && addr.iface != filter_index)) {
			continue;
		}
		if (linux_netlink_queue_addr(fd, seq, &addr) != 0) {
			return -1;
		}
	}
	return linux_netlink_queue_done(fd, seq);
}

static int linux_netlink_queue_route(struct linux_fd *fd, u64 seq,
				     const struct bunix_net_route_info *route)
{
	char payload[128];
	u64 len = 12;

	if (route == 0 || route->family != BUNIX_NET_ADDR_FAMILY_IPV4) {
		return 0;
	}
	zero_bytes(payload, sizeof(payload));
	payload[0] = LINUX_AF_INET;
	payload[1] = (char)route->prefix_len;
	payload[2] = 0;
	payload[3] = 0;
	payload[4] = LINUX_RT_TABLE_MAIN;
	payload[5] = LINUX_RTPROT_DHCP;
	payload[6] = route->gateway_lo != 0 ? LINUX_RT_SCOPE_UNIVERSE :
		     LINUX_RT_SCOPE_LINK;
	payload[7] = LINUX_RTN_UNICAST;
	store_u32(payload, 8, 0);
	if (route->prefix_len != 0 &&
	    linux_nl_attr_put_ipv4(payload, &len, sizeof(payload),
				   LINUX_RTA_DST, route->prefix_lo) != 0) {
		return -1;
	}
	if (route->gateway_lo != 0 &&
	    linux_nl_attr_put_ipv4(payload, &len, sizeof(payload),
				   LINUX_RTA_GATEWAY,
				   route->gateway_lo) != 0) {
		return -1;
	}
	if (linux_nl_attr_put_u32(payload, &len, sizeof(payload),
				  LINUX_RTA_OIF, route->iface) != 0 ||
	    linux_nl_attr_put_u32(payload, &len, sizeof(payload),
				  LINUX_RTA_PRIORITY, route->metric) != 0) {
		return -1;
	}
	return linux_netlink_queue_msg(fd, LINUX_RTM_NEWROUTE,
				       LINUX_NLM_F_MULTI, seq, payload, len);
}

static int linux_netlink_queue_routes(struct linux_fd *fd, u64 seq)
{
	struct bunix_msg reply;
	u64 count;

	if (linux_net_request(BUNIX_NET_ROUTE_COUNT, 0, 0, 0, 0, 0, 0,
			      &reply) != 0) {
		return -1;
	}
	count = reply.words[1];
	for (u64 i = 0; i < count; i++) {
		struct bunix_net_route_info route;
		long buffer = bunix_buffer_create(sizeof(route));
		int ok;

		if (buffer <= 0) {
			return -1;
		}
		ok = linux_net_request(BUNIX_NET_ROUTE_AT, (u64)buffer,
				       BUNIX_RIGHT_SEND, i, 0, 0, 0,
				       &reply) == 0 &&
		     bunix_buffer_read((u64)buffer, 0, &route,
				       sizeof(route)) == 0;
		bunix_handle_close((u64)buffer);
		if (!ok || route.family != BUNIX_NET_ADDR_FAMILY_IPV4) {
			continue;
		}
		if (linux_netlink_queue_route(fd, seq, &route) != 0) {
			return -1;
		}
	}
	return linux_netlink_queue_done(fd, seq);
}

static long linux_netlink_route_send(struct linux_process *process, u64 fd,
				     u64 len, u64 addr_len, u64 buffer)
{
	unsigned char header[16];
	char body[512];
	struct linux_fd *linux_fd = &process->fds[fd];
	u64 off = 0;

	(void)process;
	if (buffer == 0) {
		return -LINUX_EFAULT;
	}
	linux_fd->offset = 0;
	linux_fd->size = 0;
	while (off + sizeof(header) <= len) {
		u64 nlmsg_len;
		u64 nlmsg_type;
		u64 nlmsg_flags;
		u64 nlmsg_seq;
		u64 body_len;
		u64 copy_len;
		long result = 0;

		if (bunix_buffer_read(buffer, addr_len + off, header,
				      sizeof(header)) != 0) {
			return -LINUX_EFAULT;
		}
		nlmsg_len = load_u32((const char *)header, 0);
		nlmsg_type = load_u16((const char *)header, 4);
		nlmsg_flags = load_u16((const char *)header, 6);
		nlmsg_seq = load_u32((const char *)header, 8);
		if (nlmsg_len < sizeof(header) || off + nlmsg_len > len) {
			return -LINUX_EINVAL;
		}
		body_len = nlmsg_len - sizeof(header);
		if (body_len > sizeof(body)) {
			return -LINUX_E2BIG;
		}
		copy_len = body_len;
		zero_bytes(body, sizeof(body));
		if (copy_len != 0 &&
		    bunix_buffer_read(buffer, addr_len + off + sizeof(header),
				      body, copy_len) != 0) {
			return -LINUX_EFAULT;
		}

		switch (nlmsg_type) {
		case LINUX_RTM_GETLINK: {
			char name[LINUX_IFNAMSIZ];
			u64 index = body_len >= 8 ? load_u32(body, 4) : 0;
			const char *attrs = body_len > 16 ? body + 16 : 0;
			const u64 attrs_len = body_len > 16 ? body_len - 16 : 0;

			zero_bytes(name, sizeof(name));
			if (attrs != 0) {
				(void)linux_nl_attr_read_name(
					attrs, attrs_len, LINUX_IFLA_IFNAME,
					name, sizeof(name));
			}
			result = linux_netlink_queue_links(linux_fd, nlmsg_seq,
							   name, index) == 0 ?
				 0 : -LINUX_ENOMEM;
			break;
		}
		case LINUX_RTM_GETADDR: {
			u64 index = body_len >= 8 ? load_u32(body, 4) : 0;

			result = linux_netlink_queue_addrs(linux_fd, nlmsg_seq,
							   index) == 0 ?
				 0 : -LINUX_ENOMEM;
			break;
		}
		case LINUX_RTM_GETROUTE:
			result = linux_netlink_queue_routes(linux_fd,
							    nlmsg_seq) == 0 ?
				 0 : -LINUX_ENOMEM;
			break;
		case LINUX_RTM_NEWLINK:
		case LINUX_RTM_SETLINK: {
			char name[LINUX_IFNAMSIZ];
			u64 index = body_len >= 8 ? load_u32(body, 4) : 0;
			u64 flags = body_len >= 12 ? load_u32(body, 8) : 0;
			u64 change = body_len >= 16 ? load_u32(body, 12) : 0;
			const char *attrs = body_len > 16 ? body + 16 : 0;
			const u64 attrs_len = body_len > 16 ? body_len - 16 : 0;
			long iface;
			u64 set = 0;
			u64 clear = 0;
			struct bunix_msg reply;

			zero_bytes(name, sizeof(name));
			if (attrs != 0) {
				(void)linux_nl_attr_read_name(
					attrs, attrs_len, LINUX_IFLA_IFNAME,
					name, sizeof(name));
			}
			iface = linux_netlink_iface_from_name_or_index(name,
								       index);
			if (iface < 0) {
				result = iface;
				break;
			}
			if ((change == 0 || (change & LINUX_IFF_UP) != 0) &&
			    (flags & LINUX_IFF_UP) != 0) {
				set |= BUNIX_NET_IFACE_FLAG_UP;
			} else if ((change & LINUX_IFF_UP) != 0) {
				clear |= BUNIX_NET_IFACE_FLAG_UP;
			}
			result = iface == 1 ||
					 linux_net_request(
						 BUNIX_NET_INTERFACE_SET_FLAGS,
						 0, 0, (u64)iface, set, clear,
						 0, &reply) == 0 ?
				 0 : -LINUX_EINVAL;
			break;
		}
		case LINUX_RTM_NEWADDR:
		case LINUX_RTM_DELADDR: {
			const char *attrs = body_len > 8 ? body + 8 : 0;
			const u64 attrs_len = body_len > 8 ? body_len - 8 : 0;
			struct bunix_net_addr_info addr = { 0 };
			u64 ipv4 = 0;

			if (body_len < 8 || body[0] != LINUX_AF_INET ||
			    attrs == 0 ||
			    (!linux_nl_attr_read_ipv4(attrs, attrs_len,
						      LINUX_IFA_LOCAL,
						      &ipv4) &&
			     !linux_nl_attr_read_ipv4(attrs, attrs_len,
						      LINUX_IFA_ADDRESS,
						      &ipv4))) {
				result = -LINUX_EINVAL;
				break;
			}
			addr.family = BUNIX_NET_ADDR_FAMILY_IPV4;
			addr.addr_hi = 0;
			addr.addr_lo = ipv4;
			addr.prefix_len = (u64)(unsigned char)body[1];
			addr.iface = load_u32(body, 4);
			addr.flags = 1;
			addr.preferred_lifetime = 0xffffffffull;
			addr.valid_lifetime = 0xffffffffull;
			result = linux_netlink_addr_rpc(
					 nlmsg_type == LINUX_RTM_NEWADDR ?
					 BUNIX_NET_ADDR_ADD :
					 BUNIX_NET_ADDR_DELETE,
					 &addr) == 0 ?
				 0 : -LINUX_EINVAL;
			break;
		}
		case LINUX_RTM_NEWROUTE:
		case LINUX_RTM_DELROUTE: {
			const char *attrs = body_len > 12 ? body + 12 : 0;
			const u64 attrs_len = body_len > 12 ? body_len - 12 : 0;
			struct bunix_net_route_info route = { 0 };

			if (body_len < 12 || body[0] != LINUX_AF_INET ||
			    attrs == 0) {
				result = -LINUX_EINVAL;
				break;
			}
			route.family = BUNIX_NET_ADDR_FAMILY_IPV4;
			route.prefix_hi = 0;
			route.prefix_lo = 0;
			route.prefix_len = (u64)(unsigned char)body[1];
			(void)linux_nl_attr_read_ipv4(attrs, attrs_len,
						      LINUX_RTA_DST,
						      &route.prefix_lo);
			(void)linux_nl_attr_read_ipv4(attrs, attrs_len,
						      LINUX_RTA_GATEWAY,
						      &route.gateway_lo);
			(void)linux_nl_attr_read_u32(attrs, attrs_len,
						     LINUX_RTA_OIF,
						     &route.iface);
			(void)linux_nl_attr_read_u32(attrs, attrs_len,
						     LINUX_RTA_PRIORITY,
						     &route.metric);
			if (route.iface == 0) {
				const long iface =
					linux_net_default_packet_interface();

				if (iface > 0) {
					route.iface = (u64)iface;
				}
			}
			if (route.metric == 0) {
				route.metric = 100;
			}
			route.flags = BUNIX_NET_ROUTE_FLAG_UP |
				      (route.gateway_lo != 0 ?
				       BUNIX_NET_ROUTE_FLAG_GATEWAY : 0);
			result = linux_netlink_route_rpc(
					 nlmsg_type == LINUX_RTM_NEWROUTE ?
					 BUNIX_NET_ROUTE_ADD :
					 BUNIX_NET_ROUTE_DELETE,
					 &route) == 0 ?
				 0 : -LINUX_ENOENT;
			break;
		}
		default:
			result = -LINUX_EINVAL;
			break;
		}

		if (result != 0 ||
		    (nlmsg_flags & LINUX_NLM_F_ACK) != 0 ||
		    (nlmsg_type != LINUX_RTM_GETLINK &&
		     nlmsg_type != LINUX_RTM_GETADDR &&
		     nlmsg_type != LINUX_RTM_GETROUTE)) {
			if (linux_netlink_queue_error(linux_fd, nlmsg_seq,
						      result, (const char *)header) != 0) {
				return -LINUX_ENOMEM;
			}
		}
		linux_debug_netlink_log("send", process, fd, nlmsg_type,
					nlmsg_flags, nlmsg_seq, nlmsg_len,
					result, linux_fd->size);
		off += linux_nl_align(nlmsg_len);
	}
	if (off == 0) {
		linux_debug_netlink_log("send-empty", process, fd, 0, 0, 0,
					len, -LINUX_EINVAL, linux_fd->size);
	}
	linux_netlink_queue_stamp_pid(linux_fd, process->pid);
	return (long)len;
}

static int linux_netlink_write_sockaddr(u64 buffer, u64 payload_len,
					u64 result_len, u64 addr_len,
					u64 *actual_addr_len)
{
	char raw[12];
	u64 copy;

	if (actual_addr_len != 0) {
		*actual_addr_len = 0;
	}
	if (addr_len == 0) {
		return 0;
	}
	zero_bytes(raw, sizeof(raw));
	store_u16(raw, 0, LINUX_AF_NETLINK);
	copy = sizeof(raw) < addr_len ? sizeof(raw) : addr_len;
	if ((copy != 0 &&
	     bunix_buffer_write(buffer, payload_len, raw, copy) != 0) ||
	    (result_len != payload_len && copy != 0 &&
	     bunix_buffer_write(buffer, result_len, raw, copy) != 0)) {
		return -1;
	}
	if (actual_addr_len != 0) {
		*actual_addr_len = sizeof(raw);
	}
	return 0;
}

static long linux_netlink_route_recv(struct linux_process *process, u64 fd,
				     u64 len, u64 buffer, u64 addr_len,
				     u64 *actual_addr_len)
{
	struct linux_fd *linux_fd = &process->fds[fd];
	u64 remaining;
	u64 copy;

	if (linux_fd->offset >= linux_fd->size) {
		linux_debug_netlink_log("recv-empty", process, fd, 0, 0, 0,
					len, 0, linux_fd->size);
		linux_fd->offset = 0;
		linux_fd->size = 0;
		return 0;
	}
	remaining = linux_fd->size - linux_fd->offset;
	if (remaining >= 16) {
		const u64 msg_len = load_u32(linux_fd->path, linux_fd->offset);
		const u64 aligned = linux_nl_align(msg_len);
		const u64 msg_type = load_u16(linux_fd->path,
					      linux_fd->offset + 4);
		const u64 msg_flags = load_u16(linux_fd->path,
					       linux_fd->offset + 6);
		const u64 msg_seq = load_u32(linux_fd->path,
					     linux_fd->offset + 8);

		if (msg_len >= 16 && aligned <= remaining) {
			remaining = aligned;
		}
		linux_debug_netlink_log("recv-head", process, fd, msg_type,
					msg_flags, msg_seq, msg_len, 0,
					linux_fd->size);
	}
	copy = len < remaining ? len : remaining;
	if (buffer == 0 && copy != 0) {
		return -LINUX_EFAULT;
	}
	if (copy != 0 &&
	    bunix_buffer_write(buffer, 0, linux_fd->path + linux_fd->offset,
			       copy) != 0) {
		return -LINUX_EFAULT;
	}
	if (linux_netlink_write_sockaddr(buffer, len, copy, addr_len,
					 actual_addr_len) != 0) {
		return -LINUX_EFAULT;
	}
	linux_fd->offset += remaining;
	linux_debug_netlink_log("recv", process, fd, 0, 0, 0, len,
				(long)copy, linux_fd->size);
	if (linux_fd->offset >= linux_fd->size) {
		linux_fd->offset = 0;
		linux_fd->size = 0;
	}
	return (long)copy;
}

static long linux_setsockopt(struct linux_process *process, u64 fd, u64 level,
			     u64 optname, u64 opt_len, u64 opt_buffer)
{
	(void)level;
	(void)optname;
	(void)opt_len;
	(void)opt_buffer;
	if (fd >= process->fd_capacity ||
	    process->fds[fd].kind != LINUX_FD_SOCKET) {
		return -LINUX_ENOTSOCK;
	}
	if (process->fds[fd].handle != LINUX_SOCKET_NET_UDP &&
	    process->fds[fd].handle != LINUX_SOCKET_NET_TCP &&
	    process->fds[fd].handle != LINUX_SOCKET_NET_ICMP &&
	    process->fds[fd].handle != LINUX_SOCKET_NET_RAW &&
	    process->fds[fd].handle != LINUX_SOCKET_NET_PACKET &&
	    process->fds[fd].handle != LINUX_SOCKET_NETLINK_ROUTE &&
	    process->fds[fd].handle != LINUX_SOCKET_UNIX_STREAM &&
	    process->fds[fd].handle != LINUX_SOCKET_UNIX_DGRAM &&
	    process->fds[fd].handle != LINUX_SOCKET_UTMPD) {
		return -LINUX_EINVAL;
	}
	return 0;
}

static long linux_getsockopt(struct linux_process *process, u64 fd, u64 level,
			     u64 optname, u64 max_len, u64 buffer,
			     u64 *actual_len)
{
	int value = 0;
	u64 copy;

	if (actual_len != 0) {
		*actual_len = sizeof(value);
	}
	if (fd >= process->fd_capacity ||
	    process->fds[fd].kind != LINUX_FD_SOCKET) {
		return -LINUX_ENOTSOCK;
	}
	if (level == LINUX_SOL_SOCKET && optname == LINUX_SO_TYPE) {
		if (process->fds[fd].socket_type != 0) {
			value = (int)process->fds[fd].socket_type;
		} else if (process->fds[fd].handle == LINUX_SOCKET_NET_UDP ||
		    process->fds[fd].handle == LINUX_SOCKET_UNIX_DGRAM) {
			value = LINUX_SOCK_DGRAM;
		} else if (process->fds[fd].handle == LINUX_SOCKET_NET_PACKET) {
			value = LINUX_SOCK_DGRAM;
		} else if (process->fds[fd].handle ==
			   LINUX_SOCKET_NETLINK_ROUTE) {
			value = LINUX_SOCK_RAW;
		} else if (process->fds[fd].handle == LINUX_SOCKET_NET_ICMP ||
			   process->fds[fd].handle == LINUX_SOCKET_NET_RAW) {
			value = LINUX_SOCK_RAW;
		} else {
			value = LINUX_SOCK_STREAM;
		}
	} else if (level == LINUX_SOL_SOCKET &&
		   (optname == LINUX_SO_ERROR ||
		    optname == LINUX_SO_REUSEADDR ||
		    optname == LINUX_SO_KEEPALIVE ||
		    optname == LINUX_SO_DEBUG)) {
		value = 0;
	} else {
		value = 0;
	}
	copy = sizeof(value) < max_len ? sizeof(value) : max_len;
	if (copy != 0 && bunix_buffer_write(buffer, 0, &value, copy) != 0) {
		return -LINUX_EFAULT;
	}
	return 0;
}

static long linux_socket(struct linux_process *process, u64 domain, u64 type,
			 u64 protocol)
{
	const u64 base_type = type & ~(LINUX_SOCK_NONBLOCK |
				       LINUX_SOCK_CLOEXEC);
	struct bunix_msg reply;
	u64 net_type;
	u64 net_family;
	long fd;

	if (domain != LINUX_AF_UNIX &&
	    domain != LINUX_AF_INET &&
	    domain != LINUX_AF_INET6 &&
	    domain != LINUX_AF_NETLINK &&
	    domain != LINUX_AF_PACKET) {
		return -LINUX_EAFNOSUPPORT;
	}
	if (domain == LINUX_AF_UNIX &&
	    ((base_type != LINUX_SOCK_STREAM &&
	      base_type != LINUX_SOCK_DGRAM) ||
	     protocol != 0)) {
		return -LINUX_EPROTONOSUPPORT;
	}
	if (domain == LINUX_AF_UNIX) {
		fd = alloc_fd(process, LINUX_FD_SOCKET,
			      base_type == LINUX_SOCK_DGRAM ?
			      LINUX_SOCKET_UNIX_DGRAM :
			      LINUX_SOCKET_UNIX_STREAM, 0);
		if (fd >= 0) {
			process->fds[fd].socket_type = base_type;
		}
		if (fd >= 0 && (type & LINUX_SOCK_CLOEXEC) != 0) {
			process->fds[fd].flags |= LINUX_FD_CLOEXEC;
		}
		return fd;
	}
	if (domain == LINUX_AF_NETLINK) {
		if (base_type != LINUX_SOCK_RAW ||
		    protocol != LINUX_NETLINK_ROUTE) {
			return -LINUX_EPROTONOSUPPORT;
		}
		fd = alloc_fd(process, LINUX_FD_SOCKET,
			      LINUX_SOCKET_NETLINK_ROUTE, protocol);
		if (fd >= 0) {
			process->fds[fd].socket_type = base_type;
		}
		if (fd >= 0 && (type & LINUX_SOCK_CLOEXEC) != 0) {
			process->fds[fd].flags |= LINUX_FD_CLOEXEC;
		}
		return fd;
	}
	if (domain == LINUX_AF_PACKET) {
		if (base_type != LINUX_SOCK_DGRAM ||
		    protocol != LINUX_ETH_P_IP_NET) {
			return -LINUX_EPROTONOSUPPORT;
		}
		fd = alloc_fd(process, LINUX_FD_SOCKET,
			      LINUX_SOCKET_NET_PACKET, protocol);
		if (fd >= 0) {
			process->fds[fd].socket_type = base_type;
		}
		if (fd >= 0 && (type & LINUX_SOCK_CLOEXEC) != 0) {
			process->fds[fd].flags |= LINUX_FD_CLOEXEC;
		}
		return fd;
	}

	net_family = domain == LINUX_AF_INET ? BUNIX_NET_ADDR_FAMILY_IPV4 :
		     BUNIX_NET_ADDR_FAMILY_IPV6;
	if (base_type == LINUX_SOCK_DGRAM &&
	    (protocol == 0 || protocol == LINUX_IPPROTO_UDP)) {
		net_type = BUNIX_NET_UDP_OPEN;
	} else if (base_type == LINUX_SOCK_STREAM &&
		   (protocol == 0 || protocol == LINUX_IPPROTO_TCP)) {
		net_type = BUNIX_NET_TCP_OPEN;
	} else if ((base_type == LINUX_SOCK_RAW ||
		    base_type == LINUX_SOCK_DGRAM) &&
		   ((domain == LINUX_AF_INET &&
		     protocol == LINUX_IPPROTO_ICMP) ||
		    (domain == LINUX_AF_INET6 &&
		     protocol == LINUX_IPPROTO_ICMPV6))) {
		net_type = BUNIX_NET_ICMP_OPEN;
	} else if (base_type == LINUX_SOCK_RAW &&
		   domain == LINUX_AF_INET &&
		   (protocol == LINUX_IPPROTO_RAW ||
		    protocol == LINUX_IPPROTO_UDP)) {
		fd = alloc_fd(process, LINUX_FD_SOCKET, LINUX_SOCKET_NET_RAW,
			      domain);
		if (fd >= 0) {
			process->fds[fd].offset = protocol;
			process->fds[fd].socket_type = base_type;
		}
		if (fd >= 0 && (type & LINUX_SOCK_CLOEXEC) != 0) {
			process->fds[fd].flags |= LINUX_FD_CLOEXEC;
		}
		return fd;
	} else {
		return -LINUX_EPROTONOSUPPORT;
	}
	if (linux_net_call(net_type, 0, 0, net_family, protocol, 0, 0,
			   &reply) != 0) {
		return -LINUX_EIO;
	}
	fd = alloc_fd(process, LINUX_FD_SOCKET,
		      net_type == BUNIX_NET_UDP_OPEN ? LINUX_SOCKET_NET_UDP :
		      net_type == BUNIX_NET_TCP_OPEN ? LINUX_SOCKET_NET_TCP :
		      LINUX_SOCKET_NET_ICMP,
		      domain);
	if (fd >= 0) {
		process->fds[fd].offset = reply.words[1];
		process->fds[fd].socket_type = base_type;
		linux_net_socket_ref_add(&process->fds[fd]);
	}
	if (fd >= 0 && (type & LINUX_SOCK_CLOEXEC) != 0) {
		process->fds[fd].flags |= LINUX_FD_CLOEXEC;
	}
	return fd;
}

static long linux_bind(struct linux_process *process, u64 fd, u64 addr_len,
		       u64 addr_buffer)
{
	struct linux_sockaddr addr;
	struct bunix_msg reply;
	long parsed;
	u64 op;

	if (fd >= process->fd_capacity ||
	    process->fds[fd].kind != LINUX_FD_SOCKET) {
		return -LINUX_ENOTSOCK;
	}
	if (process->fds[fd].handle == LINUX_SOCKET_NETLINK_ROUTE) {
		(void)addr_len;
		(void)addr_buffer;
		return 0;
	}
	if (process->fds[fd].handle == LINUX_SOCKET_NET_PACKET) {
		u64 protocol;
		u64 ifindex;

		parsed = linux_parse_sockaddr_ll(addr_buffer, addr_len,
						 &protocol, &ifindex);
		if (parsed != 0) {
			return parsed;
		}
		if ((protocol != process->fds[fd].size &&
		     !(protocol == LINUX_ETH_P_IP &&
		       process->fds[fd].size == LINUX_ETH_P_IP_NET)) ||
		    ifindex == 0) {
			return -LINUX_EINVAL;
		}
		process->fds[fd].offset = ifindex;
		return 0;
	}
	if (process->fds[fd].handle != LINUX_SOCKET_NET_UDP &&
	    process->fds[fd].handle != LINUX_SOCKET_NET_TCP) {
		return -LINUX_EINVAL;
	}
	parsed = linux_parse_sockaddr(addr_buffer, addr_len, &addr);
	if (parsed != 0) {
		return parsed;
	}
	if (addr.family != process->fds[fd].size) {
		return -LINUX_EAFNOSUPPORT;
	}
	if (addr.port != 0 && addr.port < 1024) {
		const long euid =
			linux_user_credential(process, BUNIX_LINUX_GETEUID);

		if (euid < 0) {
			return euid;
		}
		if (euid != 0) {
			return -LINUX_EACCES;
		}
	}
	op = process->fds[fd].handle == LINUX_SOCKET_NET_UDP ?
	     BUNIX_NET_UDP_BIND : BUNIX_NET_TCP_BIND;
	return linux_net_call(op, 0, 0, process->fds[fd].offset,
			      addr.hi, addr.lo, addr.port, &reply) == 0 ?
	       0 : -LINUX_EADDRINUSE;
}

static long linux_listen(struct linux_process *process, u64 fd, u64 backlog)
{
	struct bunix_msg reply;

	if (fd >= process->fd_capacity ||
	    process->fds[fd].kind != LINUX_FD_SOCKET) {
		return -LINUX_ENOTSOCK;
	}
	if (process->fds[fd].handle != LINUX_SOCKET_NET_TCP) {
		return -LINUX_EPROTONOSUPPORT;
	}
	return linux_net_call(BUNIX_NET_TCP_LISTEN, 0, 0,
			      process->fds[fd].offset, backlog, 0, 0,
			      &reply) == 0 ? 0 : -LINUX_EINVAL;
}

static long linux_accept(struct linux_process *process, u64 fd, u64 flags)
{
	struct bunix_msg reply;
	long new_fd;

	if (fd >= process->fd_capacity ||
	    process->fds[fd].kind != LINUX_FD_SOCKET) {
		return -LINUX_ENOTSOCK;
	}
	if (process->fds[fd].handle != LINUX_SOCKET_NET_TCP) {
		return -LINUX_EPROTONOSUPPORT;
	}
	if (linux_net_call(BUNIX_NET_TCP_ACCEPT, 0, 0,
			   process->fds[fd].offset, 0, 0, 0, &reply) != 0) {
		return -LINUX_EAGAIN;
	}
	new_fd = alloc_fd(process, LINUX_FD_SOCKET, LINUX_SOCKET_NET_TCP,
			  process->fds[fd].size);
	if (new_fd < 0) {
		return new_fd;
	}
	process->fds[new_fd].offset = reply.words[1];
	if ((flags & LINUX_SOCK_CLOEXEC) != 0) {
		process->fds[new_fd].flags |= LINUX_FD_CLOEXEC;
	}
	return new_fd;
}

static long linux_shutdown(struct linux_process *process, u64 fd, u64 how)
{
	struct bunix_msg reply;
	u64 net_how = 0;

	if (fd >= process->fd_capacity ||
	    process->fds[fd].kind != LINUX_FD_SOCKET) {
		return -LINUX_ENOTSOCK;
	}
	if (process->fds[fd].handle != LINUX_SOCKET_NET_TCP) {
		return -LINUX_EPROTONOSUPPORT;
	}
	if (how == 0 || how == 2) {
		net_how |= 1;
	}
	if (how == 1 || how == 2) {
		net_how |= 2;
	}
	return linux_net_call(BUNIX_NET_TCP_SHUTDOWN, 0, 0,
			      process->fds[fd].offset, net_how, 0, 0,
			      &reply) == 0 ? 0 : -LINUX_ENOTCONN;
}

static long linux_connect(struct linux_process *process, u64 fd, u64 addr_len,
			  u64 addr_buffer)
{
	char addr[128];
	char path[LINUX_MAX_PATH];
	u64 path_len = 0;
	unsigned int family;
	struct linux_sockaddr net_addr;
	struct bunix_msg reply;
	long parsed;
	u64 op;

	if (fd >= process->fd_capacity ||
	    process->fds[fd].kind != LINUX_FD_SOCKET) {
		return -LINUX_ENOTSOCK;
	}
	if (addr_len < 3 || addr_len > sizeof(addr)) {
		return -LINUX_EINVAL;
	}
	if (addr_buffer == 0 ||
	    bunix_buffer_read(addr_buffer, 0, addr, addr_len) != 0) {
		return -LINUX_EFAULT;
	}

	family = ((unsigned int)(unsigned char)addr[0]) |
		 ((unsigned int)(unsigned char)addr[1] << 8);
	if (family != LINUX_AF_UNIX) {
		if (process->fds[fd].handle != LINUX_SOCKET_NET_UDP &&
		    process->fds[fd].handle != LINUX_SOCKET_NET_TCP) {
			return -LINUX_EAFNOSUPPORT;
		}
		parsed = linux_parse_sockaddr(addr_buffer, addr_len, &net_addr);
		if (parsed != 0) {
			return parsed;
		}
		if (net_addr.family != process->fds[fd].size) {
			return -LINUX_EAFNOSUPPORT;
		}
		op = process->fds[fd].handle == LINUX_SOCKET_NET_UDP ?
		     BUNIX_NET_UDP_CONNECT : BUNIX_NET_TCP_CONNECT;
		return linux_net_call(op, 0, 0, process->fds[fd].offset,
				      net_addr.hi, net_addr.lo, net_addr.port,
				      &reply) == 0 ? 0 : -LINUX_ECONNREFUSED;
	}
	if (process->fds[fd].handle != LINUX_SOCKET_UNIX_STREAM &&
	    process->fds[fd].handle != LINUX_SOCKET_UNIX_DGRAM &&
	    process->fds[fd].handle != LINUX_SOCKET_UTMPD) {
		return -LINUX_EAFNOSUPPORT;
	}
	for (u64 i = 2; i < addr_len && path_len + 1 < sizeof(path); i++) {
		path[path_len++] = addr[i];
		if (addr[i] == '\0') {
			break;
		}
	}
	if (path_len == 0 || path[path_len - 1] != '\0') {
		return -LINUX_EINVAL;
	}

	if (!is_utmps_socket_path(path)) {
		return -LINUX_ENOENT;
	}
	if (process->fds[fd].handle == LINUX_SOCKET_UNIX_DGRAM) {
		return -LINUX_EPROTONOSUPPORT;
	}
	process->fds[fd].handle = LINUX_SOCKET_UTMPD;
	process->fds[fd].offset = 0;
	process->fds[fd].size = LINUX_UTMPS_NONE;
	return 0;
}

static long linux_sendto(struct linux_process *process, u64 fd, u64 len,
			 u64 flags, u64 addr_len, u64 buffer)
{
	char addr[128];
	char command;
	char path[LINUX_MAX_PATH];
	struct bunix_msg reply;
	u64 path_len = 0;
	u64 op;
	unsigned int family;

	if (fd >= process->fd_capacity ||
	    process->fds[fd].kind != LINUX_FD_SOCKET) {
		return -LINUX_ENOTSOCK;
	}
	if (process->fds[fd].handle == LINUX_SOCKET_UNIX_DGRAM) {
		(void)flags;
		(void)len;
		if (addr_len == 0) {
			return -LINUX_ENOTCONN;
		}
		if (addr_len < 3 || addr_len > sizeof(addr)) {
			return -LINUX_EINVAL;
		}
		if (buffer == 0 ||
		    bunix_buffer_read(buffer, 0, addr, addr_len) != 0) {
			return -LINUX_EFAULT;
		}
		family = ((unsigned int)(unsigned char)addr[0]) |
			 ((unsigned int)(unsigned char)addr[1] << 8);
		if (family != LINUX_AF_UNIX) {
			return -LINUX_EAFNOSUPPORT;
		}
		for (u64 i = 2; i < addr_len && path_len + 1 < sizeof(path);
		     i++) {
			path[path_len++] = addr[i];
			if (addr[i] == '\0') {
				break;
			}
		}
		if (path_len == 0 || path[path_len - 1] != '\0') {
			return -LINUX_EINVAL;
		}
		return is_utmps_socket_path(path) ? -LINUX_EPROTONOSUPPORT :
		       -LINUX_ENOENT;
	}
	if (process->fds[fd].handle == LINUX_SOCKET_NETLINK_ROUTE) {
		(void)flags;
		return linux_netlink_route_send(process, fd, len, addr_len,
						buffer);
	}
	if (process->fds[fd].handle == LINUX_SOCKET_NET_PACKET) {
		unsigned char dest_mac[8];
		u64 dest_mac_len = 0;
		u64 protocol = process->fds[fd].size;
		u64 ifindex = process->fds[fd].offset;
		long parsed;

		if (addr_len != 0) {
			parsed = linux_parse_sockaddr_ll_at(buffer, 0,
							   addr_len,
							   &protocol,
							   &ifindex,
							   dest_mac,
							   &dest_mac_len);
			if (parsed != 0) {
				return parsed;
			}
		}
		if ((protocol != process->fds[fd].size &&
		     !(protocol == LINUX_ETH_P_IP &&
		       process->fds[fd].size == LINUX_ETH_P_IP_NET)) ||
		    ifindex == 0) {
			return -LINUX_EDESTADDRREQ;
		}
		return linux_net_packet_sendto(buffer, addr_len, len, ifindex,
					       dest_mac_len >= 6 ?
					       dest_mac : 0, dest_mac_len);
	}
	if (process->fds[fd].handle == LINUX_SOCKET_NET_UDP ||
	    process->fds[fd].handle == LINUX_SOCKET_NET_TCP ||
	    process->fds[fd].handle == LINUX_SOCKET_NET_ICMP ||
	    process->fds[fd].handle == LINUX_SOCKET_NET_RAW) {
		if (addr_len != 0) {
			struct linux_sockaddr dest;
			long parsed;

			if (process->fds[fd].handle == LINUX_SOCKET_NET_TCP) {
				return -LINUX_EISCONN;
			}
			parsed = linux_parse_sockaddr_at(buffer, 0, addr_len,
							 &dest);
			if (parsed != 0) {
				return parsed;
			}
			if (dest.family != process->fds[fd].size) {
				return -LINUX_EAFNOSUPPORT;
			}
			if (process->fds[fd].handle == LINUX_SOCKET_NET_RAW) {
				return linux_net_raw_ipv4_sendto(buffer,
								addr_len, len,
								&dest);
			}
			return linux_net_datagram_sendto(
				process->fds[fd].handle == LINUX_SOCKET_NET_UDP ?
				BUNIX_NET_UDP_SENDTO : BUNIX_NET_ICMP_SENDTO,
				process->fds[fd].offset, buffer, addr_len, len,
				&dest);
		}
		if (process->fds[fd].handle == LINUX_SOCKET_NET_ICMP ||
		    process->fds[fd].handle == LINUX_SOCKET_NET_RAW) {
			return -LINUX_EDESTADDRREQ;
		}
		(void)flags;
		op = process->fds[fd].handle == LINUX_SOCKET_NET_UDP ?
		     BUNIX_NET_UDP_SEND : BUNIX_NET_TCP_WRITE;
		if (linux_net_call(op, buffer, BUNIX_RIGHT_RECV,
				   process->fds[fd].offset, len, 0, 0,
				   &reply) != 0) {
			return -LINUX_EPIPE;
		}
		return (long)reply.words[1];
	}
	if (process->fds[fd].handle != LINUX_SOCKET_UTMPD) {
		return -LINUX_EINVAL;
	}
	if (len != 1) {
		return -LINUX_EINVAL;
	}
	if (buffer == 0 || bunix_buffer_read(buffer, 0, &command, 1) != 0) {
		return -LINUX_EFAULT;
	}
	if (command != LINUX_UTMPS_REWIND &&
	    command != LINUX_UTMPS_GETENT) {
		return -LINUX_EINVAL;
	}
	if (command == LINUX_UTMPS_REWIND) {
		process->fds[fd].offset = 0;
	}
	process->fds[fd].size = (u64)(unsigned char)command;
	return 1;
}

static long linux_recvfrom(struct linux_process *process, u64 fd, u64 len,
			   u64 flags, u64 buffer, u64 addr_len,
			   u64 *actual_addr_len)
{
	struct bunix_msg reply;
	u64 op;
	const int nonblock =
		(flags & LINUX_MSG_DONTWAIT) != 0 ||
		(fd < process->fd_capacity &&
		 (process->fds[fd].status_flags & LINUX_O_NONBLOCK) != 0);

	if (actual_addr_len != 0) {
		*actual_addr_len = 0;
	}
	if (fd >= process->fd_capacity ||
	    process->fds[fd].kind != LINUX_FD_SOCKET) {
		return -LINUX_ENOTSOCK;
	}
	if (process->fds[fd].handle == LINUX_SOCKET_NET_ICMP &&
	    process->fds[fd].socket_type == LINUX_SOCK_RAW &&
	    process->fds[fd].size == LINUX_AF_INET) {
		for (u64 retry = 0;; retry++) {
			const long result = linux_icmp_raw_ipv4_recv(process, fd,
								    len, buffer,
								    addr_len,
								    actual_addr_len);

			if (result != -((long)LINUX_EAGAIN)) {
				return result;
			}
			if (!nonblock && linux_alarm_expire_if_ready(process)) {
				return -LINUX_EINTR;
			}
			if (nonblock || retry >= LINUX_RECV_BLOCK_RETRIES) {
				return -LINUX_EAGAIN;
			}
			(void)bunix_sleep_ns(LINUX_RECV_BLOCK_SLEEP_NS);
		}
	}
	if (process->fds[fd].handle == LINUX_SOCKET_NET_UDP ||
	    process->fds[fd].handle == LINUX_SOCKET_NET_TCP ||
	    process->fds[fd].handle == LINUX_SOCKET_NET_ICMP) {
		op = process->fds[fd].handle == LINUX_SOCKET_NET_UDP ?
		     BUNIX_NET_UDP_RECV :
		     process->fds[fd].handle == LINUX_SOCKET_NET_TCP ?
		     BUNIX_NET_TCP_READ : BUNIX_NET_ICMP_RECV;
		for (u64 retry = 0;; retry++) {
			const u64 native_addr_len =
				process->fds[fd].handle == LINUX_SOCKET_NET_UDP &&
				addr_len != 0 ?
				sizeof(struct bunix_net_endpoint_addr) : 0;
			const u64 native_addr_offset = native_addr_len != 0 ?
						       len + addr_len : 0;

			if (linux_net_call(op, buffer, BUNIX_RIGHT_SEND,
					   process->fds[fd].offset, len,
					   native_addr_offset, native_addr_len,
					   &reply) == 0) {
				const long received = (long)reply.words[1];

				if (addr_len != 0 &&
				    process->fds[fd].handle ==
				    LINUX_SOCKET_NET_UDP) {
					struct bunix_net_endpoint_addr source;
					struct linux_sockaddr addr;
					long actual;

					if (received < 0 ||
					    bunix_buffer_read(buffer,
							      native_addr_offset,
							      &source,
							      sizeof(source)) != 0) {
						return -LINUX_EFAULT;
					}
					actual = linux_sockaddr_from_bunix_net(
						&source, &addr);
					if (actual != 0) {
						return actual;
					}
					actual = linux_write_sockaddr_at(
						buffer, (u64)received,
						addr_len, &addr);
					if (actual < 0) {
						return actual;
					}
					if (actual_addr_len != 0) {
						*actual_addr_len = (u64)actual;
					}
				}
				if (addr_len != 0 &&
				    process->fds[fd].handle ==
				    LINUX_SOCKET_NET_ICMP) {
					const struct linux_sockaddr addr = {
						.family = process->fds[fd].size,
						.hi = reply.words[2],
						.lo = reply.words[3],
						.port = 0,
					};
					const long actual =
						linux_write_sockaddr_at(
							buffer, reply.words[1],
							addr_len, &addr);

					if (actual < 0) {
						return actual;
					}
					if (actual_addr_len != 0) {
						*actual_addr_len = (u64)actual;
					}
				}
				return received;
			}
			if (!nonblock && linux_alarm_expire_if_ready(process)) {
				return -LINUX_EINTR;
			}
			if (nonblock || retry >= LINUX_RECV_BLOCK_RETRIES) {
				return -LINUX_EAGAIN;
			}
			(void)bunix_sleep_ns(LINUX_RECV_BLOCK_SLEEP_NS);
		}
	}
	if (process->fds[fd].handle == LINUX_SOCKET_NET_PACKET) {
		if (process->fds[fd].offset == 0) {
			return -LINUX_EDESTADDRREQ;
		}
		if (linux_net_request(BUNIX_NET_PACKET_RX_DEQUEUE, buffer,
				      BUNIX_RIGHT_SEND,
				      process->fds[fd].offset, len,
				      LINUX_ETH_P_IP,
				      LINUX_ETHERNET_HEADER_SIZE,
				      &reply) != 0) {
			return -LINUX_EAGAIN;
		}
		return (long)reply.words[2];
	}
	if (process->fds[fd].handle == LINUX_SOCKET_NET_RAW) {
		return linux_net_raw_ipv4_recv(buffer, len);
	}
	if (process->fds[fd].handle == LINUX_SOCKET_NETLINK_ROUTE) {
		return linux_netlink_route_recv(process, fd, len, buffer,
						addr_len, actual_addr_len);
	}
	return utmps_recv_response(&process->fds[fd], len, buffer);
}

static long linux_recvmsg(struct linux_process *process, u64 fd, u64 len,
			  u64 flags, u64 name_len, u64 buffer,
			  u64 *actual_name_len)
{
	struct bunix_msg reply;
	const u64 sockaddr_ll_len = 20;
	const u64 encoded = LINUX_ETHERNET_HEADER_SIZE |
			    ((name_len != 0 ? name_len : 0) << 32);

	if (actual_name_len != 0) {
		*actual_name_len = 0;
	}
	if (fd >= process->fd_capacity ||
	    process->fds[fd].kind != LINUX_FD_SOCKET) {
		return -LINUX_ENOTSOCK;
	}
	if (process->fds[fd].handle != LINUX_SOCKET_NET_PACKET) {
		return linux_recvfrom(process, fd, len, flags, buffer, name_len,
				      actual_name_len);
	}
	if (process->fds[fd].offset == 0) {
		return -LINUX_EDESTADDRREQ;
	}
	if (linux_net_request(BUNIX_NET_PACKET_RX_DEQUEUE, buffer,
			      BUNIX_RIGHT_SEND, process->fds[fd].offset, len,
			      LINUX_ETH_P_IP, encoded, &reply) != 0) {
		return -LINUX_EAGAIN;
	}
	if (actual_name_len != 0 && name_len != 0) {
		*actual_name_len = sockaddr_ll_len;
	}
	return (long)reply.words[2];
}

static long linux_pollfd(struct linux_process *process, long fd, u64 events)
{
	enum {
		LINUX_POLLIN = 0x0001,
		LINUX_POLLOUT = 0x0004,
		LINUX_POLLERR = 0x0008,
		LINUX_POLLHUP = 0x0010,
		LINUX_POLLNVAL = 0x0020,
	};
	struct linux_pipe *pipe;
	u64 revents = 0;

	if (fd < 0) {
		return 0;
	}
	if ((u64)fd >= process->fd_capacity ||
	    process->fds[fd].kind == 0) {
		return LINUX_POLLNVAL;
	}
	switch (process->fds[fd].kind) {
	case LINUX_FD_CONSOLE:
		if ((events & LINUX_POLLIN) != 0 &&
		    linux_tty_can_read(&console_tty)) {
			revents |= LINUX_POLLIN;
		}
		if ((events & LINUX_POLLOUT) != 0) {
			revents |= LINUX_POLLOUT;
		}
		break;
	case LINUX_FD_FILE:
	case LINUX_FD_DIR:
	case LINUX_FD_SOCKET:
		if (process->fds[fd].kind == LINUX_FD_SOCKET &&
		    (process->fds[fd].handle == LINUX_SOCKET_NET_UDP ||
		     process->fds[fd].handle == LINUX_SOCKET_NET_TCP ||
		     process->fds[fd].handle == LINUX_SOCKET_NET_ICMP)) {
			struct bunix_msg reply;
			u64 net_events = 0;
			const u64 op =
				process->fds[fd].handle == LINUX_SOCKET_NET_UDP ?
				BUNIX_NET_UDP_POLL :
				process->fds[fd].handle == LINUX_SOCKET_NET_TCP ?
				BUNIX_NET_TCP_POLL : BUNIX_NET_ICMP_POLL;

			if (linux_net_call(op, 0, 0, process->fds[fd].offset,
					   0, 0, 0, &reply) != 0) {
				revents |= LINUX_POLLERR;
				break;
			}
			net_events = reply.words[1];
			if ((events & LINUX_POLLIN) != 0 &&
			    (net_events & 1) != 0) {
				revents |= LINUX_POLLIN;
			}
			if ((events & LINUX_POLLOUT) != 0 &&
			    (net_events & 2) != 0) {
				revents |= LINUX_POLLOUT;
			}
			if ((net_events & 4) != 0) {
				revents |= LINUX_POLLHUP;
			}
		} else if (process->fds[fd].kind == LINUX_FD_SOCKET &&
			   process->fds[fd].handle == LINUX_SOCKET_NET_PACKET) {
			struct bunix_msg reply;
			u64 net_events = 2;

			if (process->fds[fd].offset == 0 ||
			    linux_net_request(BUNIX_NET_PACKET_RX_POLL, 0, 0,
					      process->fds[fd].offset,
					      LINUX_ETH_P_IP, 0, 0,
					      &reply) != 0) {
				revents |= LINUX_POLLERR;
				break;
			}
			net_events = reply.words[1];
			if ((events & LINUX_POLLIN) != 0 &&
			    (net_events & 1) != 0) {
				revents |= LINUX_POLLIN;
			}
			if ((events & LINUX_POLLOUT) != 0 &&
			    (net_events & 2) != 0) {
				revents |= LINUX_POLLOUT;
			}
		} else if (process->fds[fd].kind == LINUX_FD_SOCKET &&
			   process->fds[fd].handle == LINUX_SOCKET_NET_RAW) {
			const long net_events = linux_net_raw_ipv4_poll();

			if (net_events < 0) {
				revents |= LINUX_POLLERR;
				break;
			}
			if ((events & LINUX_POLLIN) != 0 &&
			    (((u64)net_events) & 1) != 0) {
				revents |= LINUX_POLLIN;
			}
			if ((events & LINUX_POLLOUT) != 0) {
				revents |= LINUX_POLLOUT;
			}
		} else if (process->fds[fd].kind == LINUX_FD_SOCKET &&
			   process->fds[fd].handle ==
			   LINUX_SOCKET_NETLINK_ROUTE) {
			if ((events & LINUX_POLLIN) != 0 &&
			    (process->fds[fd].flags &
			     LINUX_FD_NETLINK_ACK_PENDING) != 0) {
				revents |= LINUX_POLLIN;
			}
			if ((events & LINUX_POLLOUT) != 0) {
				revents |= LINUX_POLLOUT;
			}
		} else {
			if ((events & LINUX_POLLIN) != 0) {
				revents |= LINUX_POLLIN;
			}
			if ((events & LINUX_POLLOUT) != 0) {
				revents |= LINUX_POLLOUT;
			}
		}
		break;
	case LINUX_FD_PIPE_READ:
		pipe = linux_pipe_find(process->fds[fd].handle);
		if (pipe == 0) {
			revents |= LINUX_POLLNVAL;
			break;
		}
		if ((events & LINUX_POLLIN) != 0 &&
		    (pipe->len != 0 || pipe->write_refs == 0)) {
			revents |= LINUX_POLLIN;
		}
		if (pipe->write_refs == 0) {
			revents |= LINUX_POLLHUP;
		}
		break;
	case LINUX_FD_PIPE_WRITE:
		pipe = linux_pipe_find(process->fds[fd].handle);
		if (pipe == 0) {
			revents |= LINUX_POLLNVAL;
			break;
		}
		if (pipe->read_refs == 0) {
			revents |= LINUX_POLLERR;
		} else if ((events & LINUX_POLLOUT) != 0 &&
			   pipe->len < LINUX_PIPE_CAPACITY) {
			revents |= LINUX_POLLOUT;
		}
		break;
	default:
		revents |= LINUX_POLLNVAL;
		break;
	}
	return (long)revents;
}

static long linux_read(struct linux_process *process, u64 fd, u64 len,
		       u64 buffer, u64 reply_handle, int *blocked)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_READ_FILE_BUFFER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP,
		.reply = 0,
		.cap = buffer,
		.words = { 0, 0, len, 0 },
	};
	struct bunix_msg reply;

	if (len > LINUX_MAX_WRITE) {
		len = LINUX_MAX_WRITE;
	}
	if (fd >= process->fd_capacity ||
	    process->fds[fd].kind == 0 ||
	    buffer == 0) {
		return -LINUX_EBADF;
	}
	if (len == 0) {
		return 0;
	}
	linux_debug_read_kind_log(process, fd, len);

	if (process->fds[fd].kind == LINUX_FD_CONSOLE) {
		return linux_tty_read(&console_tty, process, len, buffer,
				      reply_handle, blocked);
	}
	if (process->fds[fd].kind == LINUX_FD_PIPE_READ) {
		struct linux_pipe *pipe = linux_pipe_find(process->fds[fd].handle);

		if (pipe == 0) {
			return -LINUX_EBADF;
		}
		if (pipe->len == 0) {
			if (pipe->write_refs == 0) {
				linux_debug_pipe_log("read-eof", process, fd,
						     pipe, 0);
				return 0;
			}
			if ((process->fds[fd].status_flags &
			     LINUX_O_NONBLOCK) != 0) {
				linux_debug_pipe_log("read-eagain", process,
						     fd, pipe,
						     -LINUX_EAGAIN);
				return -LINUX_EAGAIN;
			}
			if (reply_handle == 0 || pipe->reader_reply != 0) {
				linux_debug_pipe_log("read-busy", process, fd,
						     pipe, -LINUX_EAGAIN);
				return -LINUX_EAGAIN;
			}
			pipe->reader_reply = reply_handle;
			pipe->reader_buffer = buffer;
			pipe->reader_len = len;
			if (blocked != 0) {
				*blocked = 1;
			}
			linux_debug_pipe_log("read-block", process, fd, pipe,
					     0);
			return 0;
		}
		const long nread = linux_pipe_read_available(pipe, len, buffer);
		linux_debug_pipe_log("read-data", process, fd, pipe, nread);
		return nread;
	}
	if (process->fds[fd].kind == LINUX_FD_SOCKET &&
	    (process->fds[fd].handle == LINUX_SOCKET_NET_UDP ||
	     process->fds[fd].handle == LINUX_SOCKET_NET_TCP ||
	     process->fds[fd].handle == LINUX_SOCKET_NET_ICMP ||
	     process->fds[fd].handle == LINUX_SOCKET_NET_RAW ||
	     process->fds[fd].handle == LINUX_SOCKET_NET_PACKET ||
	     process->fds[fd].handle == LINUX_SOCKET_NETLINK_ROUTE)) {
		return linux_recvfrom(process, fd, len, 0, buffer, 0, 0);
	}
	if (process->fds[fd].kind == LINUX_FD_DIR) {
		return -LINUX_EISDIR;
	}
	if (process->fds[fd].kind == LINUX_FD_FILE) {
		const u64 offset = linux_fd_offset(&process->fds[fd]);
		const u64 size = linux_fd_size(&process->fds[fd]);

		if (offset >= size &&
		    !(size == 0 &&
		      linux_fd_allows_zero_size_read(&process->fds[fd]))) {
			return 0;
		}
		if (size != 0 && len > size - offset) {
			len = size - offset;
		}
	}

	request.words[0] = process->fds[fd].handle;
	request.words[1] = linux_fd_offset(&process->fds[fd]);
	if (process->fds[fd].kind == LINUX_FD_FILE &&
	    process->fds[fd].path[0] == '/' &&
	    len <= LINUX_FILE_READAHEAD_TRIGGER_MAX &&
	    request.words[1] <= linux_fd_size(&process->fds[fd]) &&
	    len <= linux_fd_size(&process->fds[fd]) - request.words[1] &&
	    (linux_open_file_readahead_get(&process->fds[fd],
					   request.words[1], len, buffer) ||
	     linux_open_file_readahead_fill(&process->fds[fd],
					    request.words[1], len, buffer))) {
		linux_fd_set_offset(process, fd,
				    linux_fd_offset(&process->fds[fd]) + len);
		return (long)len;
	}
	if (process->fds[fd].kind == LINUX_FD_FILE &&
	    process->fds[fd].path[0] == '/' &&
	    len <= LINUX_FILE_READ_CACHE_MAX &&
	    request.words[1] <= linux_fd_size(&process->fds[fd]) &&
	    len <= linux_fd_size(&process->fds[fd]) - request.words[1] &&
	    linux_file_read_cache_get(process->fds[fd].path, request.words[1],
				      len, buffer)) {
		linux_fd_set_offset(process, fd,
				    linux_fd_offset(&process->fds[fd]) + len);
		return (long)len;
	}
	if (bunix_ipc_call(LINUX_HANDLE_VFS, &request, &reply) != 0) {
		return -LINUX_EIO;
	}
	if (reply.words[0] != 0) {
		return linux_vfs_error(reply.words[0]);
	}
	if (process->fds[fd].kind == LINUX_FD_FILE &&
	    process->fds[fd].path[0] == '/' &&
	    reply.words[1] == len) {
		linux_file_read_cache_put(process->fds[fd].path,
					  request.words[1], len, buffer);
	}

	linux_fd_set_offset(process, fd,
			    linux_fd_offset(&process->fds[fd]) +
			    reply.words[1]);
	return (long)reply.words[1];
}

static long linux_mmap_read(struct linux_process *process, u64 fd, u64 offset,
			    u64 len, u64 buffer)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_MMAP_PAGE_BUFFER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP,
		.reply = 0,
		.cap = buffer,
		.words = { 0, offset, len, 0 },
	};
	struct bunix_msg reply;

	if (buffer == 0 || len > LINUX_MAX_MMAP_BUFFER) {
		if (buffer == 0) {
			return -LINUX_EFAULT;
		}
		return -LINUX_EINVAL;
	}
	if (len > LINUX_MMAP_PAGE_SIZE) {
		len = LINUX_MMAP_PAGE_SIZE;
	}
	if (fd >= process->fd_capacity ||
	    process->fds[fd].kind != LINUX_FD_FILE) {
		return -LINUX_EBADF;
	}
	if (offset >= linux_fd_size(&process->fds[fd])) {
		process->last_mmap_page_class = LINUX_MMAP_PAGE_BUS;
		return 0;
	}
	if (len > linux_fd_size(&process->fds[fd]) - offset) {
		len = linux_fd_size(&process->fds[fd]) - offset;
	}
	process->last_mmap_page_class = LINUX_MMAP_PAGE_DATA;
	if (process->fds[fd].path[0] == '/' &&
	    len <= LINUX_FILE_READ_CACHE_MAX &&
	    linux_file_read_cache_get(process->fds[fd].path, offset, len,
				      buffer)) {
		process->last_mmap_page_class = LINUX_MMAP_PAGE_DATA;
		return (long)len;
	}

	request.words[0] = process->fds[fd].handle;
	request.words[2] = len;
	if (bunix_ipc_call(LINUX_HANDLE_VFS, &request, &reply) != 0) {
		return -LINUX_EIO;
	}
	if (reply.words[0] != 0) {
		return linux_vfs_error(reply.words[0]);
	}
	process->last_mmap_page_class = reply.words[1];
	if (reply.words[1] == BUNIX_VFS_MMAP_PAGE_BUS) {
		return 0;
	}
	if (reply.words[1] == BUNIX_VFS_MMAP_PAGE_ZERO) {
		return (long)reply.words[2];
	}
	if (reply.words[1] != BUNIX_VFS_MMAP_PAGE_DATA) {
		return -LINUX_EIO;
	}
	if (process->fds[fd].path[0] == '/' && reply.words[2] == len) {
		linux_file_read_cache_put(process->fds[fd].path, offset, len,
					  buffer);
	}
	return (long)reply.words[2];
}

static long linux_write_buffer(struct linux_process *process, u64 fd, u64 len,
			       u64 buffer, u64 reply_handle, int *blocked)
{
	if (fd >= process->fd_capacity || process->fds[fd].kind == 0) {
		return -LINUX_EBADF;
	}
	if (buffer == 0) {
		return -LINUX_EFAULT;
	}

	if (process->fds[fd].kind == LINUX_FD_PIPE_WRITE) {
		struct linux_pipe *pipe = linux_pipe_find(process->fds[fd].handle);
		long nwritten;

		if (pipe == 0) {
			return -LINUX_EBADF;
		}
		if (pipe->read_refs == 0) {
			linux_debug_pipe_log("write-epipe", process, fd, pipe,
					     -LINUX_EPIPE);
			return -LINUX_EPIPE;
		}
		nwritten = linux_pipe_write_available(pipe, len, buffer);
		if (nwritten == (long)-LINUX_EAGAIN &&
		    (process->fds[fd].status_flags & LINUX_O_NONBLOCK) == 0) {
			if (reply_handle == 0 || pipe->writer_reply != 0) {
				linux_debug_pipe_log("write-busy", process, fd,
						     pipe, -LINUX_EAGAIN);
				return -LINUX_EAGAIN;
			}
			pipe->writer_reply = reply_handle;
			pipe->writer_buffer = buffer;
			pipe->writer_len = len;
			if (blocked != 0) {
				*blocked = 1;
			}
			linux_debug_pipe_log("write-block", process, fd, pipe,
					     0);
			return 0;
		}
		if (nwritten < 0) {
			linux_debug_pipe_log("write-error", process, fd, pipe,
					     nwritten);
			return nwritten;
		}
		linux_pipe_wake_reader(pipe);
		linux_debug_pipe_log("write-data", process, fd, pipe,
				     nwritten);
		return nwritten;
	}
	if (process->fds[fd].kind == LINUX_FD_SOCKET &&
	    (process->fds[fd].handle == LINUX_SOCKET_NET_UDP ||
	     process->fds[fd].handle == LINUX_SOCKET_NET_TCP)) {
		return linux_sendto(process, fd, len, 0, 0, buffer);
	}
	if (process->fds[fd].kind == LINUX_FD_SOCKET &&
	    process->fds[fd].handle == LINUX_SOCKET_NETLINK_ROUTE) {
		return linux_sendto(process, fd, len, 0, 0, buffer);
	}
	if (process->fds[fd].kind == LINUX_FD_FILE) {
		u64 done = 0;
		u64 offset = (process->fds[fd].status_flags & LINUX_O_APPEND) != 0 ?
			     linux_fd_size(&process->fds[fd]) :
			     linux_fd_offset(&process->fds[fd]);

		while (done < len || (len == 0 && done == 0)) {
			const u64 remaining = len - done;
			const u64 chunk = remaining > sizeof(write_buffer) ?
					  sizeof(write_buffer) : remaining;
			const long tmp = bunix_buffer_create(chunk == 0 ? 1 : chunk);
			struct bunix_msg request = {
				.protocol = BUNIX_PROTO_VFS,
				.type = BUNIX_VFS_WRITE_FILE_BUFFER,
				.sender = 0,
				.cap_rights = BUNIX_RIGHT_RECV | BUNIX_RIGHT_DUP,
				.reply = 0,
				.cap = (u64)tmp,
				.words = { process->fds[fd].handle,
					   offset + done, chunk,
					   process->bunix_task },
			};
			struct bunix_msg reply;

			if (tmp < 0) {
				return done != 0 ? (long)done :
				       -(long)LINUX_ENOMEM;
			}
			if (chunk != 0 &&
			    (bunix_buffer_read(buffer, done, write_buffer, chunk) != 0 ||
			     bunix_buffer_write((u64)tmp, 0, write_buffer,
						chunk) != 0)) {
				bunix_handle_close((u64)tmp);
				return done != 0 ? (long)done : -(long)LINUX_EFAULT;
			}
			if (bunix_ipc_call(LINUX_HANDLE_VFS,
					   &request, &reply) != 0) {
				bunix_handle_close((u64)tmp);
				return done != 0 ? (long)done :
				       -(long)LINUX_EIO;
			}
			if (reply.words[0] != 0) {
				const long error = linux_vfs_error(reply.words[0]);

				bunix_handle_close((u64)tmp);
				return done != 0 ? (long)done : error;
			}
			bunix_handle_close((u64)tmp);
			done += reply.words[1];
			if (chunk == 0 || reply.words[1] != chunk) {
				break;
			}
		}
		linux_fd_set_offset(process, fd, offset + done);
		if (linux_fd_offset(&process->fds[fd]) >
		    linux_fd_size(&process->fds[fd])) {
			linux_fd_set_size(process, fd,
					  linux_fd_offset(&process->fds[fd]));
		}
		if (done != 0) {
			if (process->fds[fd].open_file != 0) {
				linux_open_file_readahead_clear(
					process->fds[fd].open_file);
			}
			linux_vfs_note_mutation(process->fds[fd].path[0] != '\0' ?
						process->fds[fd].path : 0);
		}
		return (long)done;
	}
	if (process->fds[fd].kind != LINUX_FD_CONSOLE) {
		return -LINUX_EBADF;
	}

	(void)process;
	return linux_tty_write_buffer(&console_tty, len, buffer);
}

static long linux_sendfile(struct linux_process *process, u64 out_fd,
			   u64 in_fd, u64 len, u64 offset, u64 buffer,
			   u64 *next_offset)
{
	u64 saved_offset = 0;
	const int positioned = offset != (u64)-1;
	long nread;

	if (in_fd >= process->fd_capacity || process->fds[in_fd].kind == 0) {
		return -LINUX_EBADF;
	}
	if (process->fds[in_fd].kind != LINUX_FD_FILE) {
		return -LINUX_EINVAL;
	}
	if (positioned) {
		saved_offset = linux_fd_offset(&process->fds[in_fd]);
		linux_fd_set_offset(process, in_fd, offset);
	}

	nread = linux_read(process, in_fd, len, buffer, 0, 0);
	if (positioned) {
		if (next_offset != 0 && nread > 0) {
			*next_offset = linux_fd_offset(&process->fds[in_fd]);
		}
		linux_fd_set_offset(process, in_fd, saved_offset);
	}
	if (nread <= 0) {
		return nread;
	}
	return linux_write_buffer(process, out_fd, (u64)nread, buffer, 0, 0);
}

static long linux_lseek(struct linux_process *process, u64 fd, u64 offset,
			u64 whence)
{
	u64 base;
	u64 next;

	if (fd >= process->fd_capacity || process->fds[fd].kind == 0) {
		return -LINUX_EBADF;
	}
	if (process->fds[fd].kind != LINUX_FD_FILE &&
	    process->fds[fd].kind != LINUX_FD_DIR) {
		return -LINUX_ESPIPE;
	}
	if (whence == LINUX_SEEK_SET) {
		base = 0;
	} else if (whence == LINUX_SEEK_CUR) {
		base = linux_fd_offset(&process->fds[fd]);
	} else if (whence == LINUX_SEEK_END) {
		base = linux_fd_size(&process->fds[fd]);
	} else {
		return -LINUX_EINVAL;
	}
	if ((offset >> 63) != 0 || base + offset < base) {
		return -LINUX_EINVAL;
	}
	next = base + offset;
	linux_fd_set_offset(process, fd, next);
	return (long)next;
}

static long linux_close(struct linux_process *process, u64 fd)
{
	if (fd >= process->fd_capacity ||
	    process->fds[fd].kind == 0) {
		return -LINUX_EBADF;
	}

	if (process->fds[fd].kind == LINUX_FD_FILE ||
	    process->fds[fd].kind == LINUX_FD_DIR) {
		if ((process->fds[fd].flags & LINUX_FD_TMPFILE) != 0 &&
		    process->fds[fd].open_file != 0 &&
		    process->fds[fd].open_file->refs <= 1) {
			linux_unlink_hidden_path(process, process->fds[fd].path);
		}
		if (linux_open_file_ref_drop(&process->fds[fd]) != 0) {
			return -LINUX_EIO;
		}
	}
	if (process->fds[fd].kind == LINUX_FD_PIPE_READ ||
	    process->fds[fd].kind == LINUX_FD_PIPE_WRITE) {
		const u64 pipe_id = process->fds[fd].handle;
		struct linux_pipe *pipe = linux_pipe_find(pipe_id);
		const char *action =
			process->fds[fd].kind == LINUX_FD_PIPE_READ ?
			"close-read" : "close-write";

		linux_pipe_ref_drop(&process->fds[fd]);
		linux_debug_pipe_log(action, process, fd, pipe, 0);
		linux_pipe_wake_reader(pipe);
		linux_pipe_wake_writer(pipe);
		linux_pipe_destroy_if_unused(pipe);
	}
	if (linux_fd_is_refcounted_net_socket(&process->fds[fd]) &&
	    linux_net_socket_ref_drop(&process->fds[fd]) == 0) {
		struct bunix_msg reply;
		const u64 op = process->fds[fd].handle == LINUX_SOCKET_NET_UDP ?
			       BUNIX_NET_UDP_CLOSE :
			       process->fds[fd].handle == LINUX_SOCKET_NET_TCP ?
			       BUNIX_NET_TCP_CLOSE : BUNIX_NET_ICMP_CLOSE;

		(void)linux_net_call(op, 0, 0, process->fds[fd].offset,
				     0, 0, 0, &reply);
	}
	process->fds[fd].kind = 0;
	process->fds[fd].handle = 0;
	process->fds[fd].offset = 0;
	process->fds[fd].size = 0;
	process->fds[fd].flags = 0;
	process->fds[fd].status_flags = 0;
	process->fds[fd].socket_type = 0;
	process->fds[fd].open_file = 0;
	process->fds[fd].path[0] = '\0';
	return 0;
}

static long linux_close_range(struct linux_process *process, u64 first,
			      u64 last, u64 flags)
{
	const u64 supported = LINUX_CLOSE_RANGE_UNSHARE |
			      LINUX_CLOSE_RANGE_CLOEXEC;
	u64 end;

	if ((flags & ~supported) != 0 || first > last) {
		return -LINUX_EINVAL;
	}
	if (first >= process->fd_capacity) {
		return 0;
	}
	end = last;
	if (end >= process->fd_capacity) {
		end = process->fd_capacity - 1;
	}

	for (u64 fd = first; fd <= end; fd++) {
		if (process->fds[fd].kind == 0) {
			continue;
		}
		if ((flags & LINUX_CLOSE_RANGE_CLOEXEC) != 0) {
			process->fds[fd].flags |= LINUX_FD_CLOEXEC;
			continue;
		}
		(void)linux_close(process, fd);
	}
	return 0;
}

static void linux_fd_ref_add(struct linux_fd *fd)
{
	if (fd->kind == LINUX_FD_FILE || fd->kind == LINUX_FD_DIR) {
		linux_open_file_ref_add(fd);
	} else if (fd->kind == LINUX_FD_PIPE_READ ||
		   fd->kind == LINUX_FD_PIPE_WRITE) {
		linux_pipe_ref_add(fd);
	} else if (linux_fd_is_refcounted_net_socket(fd)) {
		linux_net_socket_ref_add(fd);
	}
}

static long linux_dup_to(struct linux_process *process, u64 old_fd,
			 u64 new_fd, u64 flags, int fixed, int allow_same)
{
	if ((flags & ~LINUX_DUP_CLOEXEC) != 0) {
		return -LINUX_EINVAL;
	}
	if (old_fd >= process->fd_capacity || process->fds[old_fd].kind == 0) {
		return -LINUX_EBADF;
	}

	if (!fixed) {
		for (;;) {
			for (new_fd = 0; new_fd < process->fd_capacity; new_fd++) {
				if (process->fds[new_fd].kind == 0) {
					break;
				}
			}
			if (new_fd < process->fd_capacity) {
				break;
			}
			if (linux_process_ensure_fds(process,
						     process->fd_capacity + 1) != 0) {
				return -LINUX_EMFILE;
			}
		}
	}
	if (fixed &&
	    linux_process_ensure_fds(process, new_fd + 1) != 0) {
		return -LINUX_EMFILE;
	}
	if (old_fd == new_fd) {
		if (fixed && allow_same) {
			return (long)new_fd;
		}
		return -LINUX_EINVAL;
	}
	if (process->fds[new_fd].kind != 0) {
		const long closed = linux_close(process, new_fd);

		if (closed != 0) {
			return closed;
		}
	}

	process->fds[new_fd] = process->fds[old_fd];
	process->fds[new_fd].flags =
		(process->fds[old_fd].flags & ~LINUX_FD_CLOEXEC) |
		((flags & LINUX_DUP_CLOEXEC) != 0 ? LINUX_FD_CLOEXEC : 0);
	linux_fd_ref_add(&process->fds[new_fd]);
	if (process->fds[new_fd].kind == LINUX_FD_PIPE_READ ||
	    process->fds[new_fd].kind == LINUX_FD_PIPE_WRITE) {
		linux_debug_pipe_log("dup", process, new_fd,
				     linux_pipe_find(process->fds[new_fd].handle),
				     (long)old_fd);
	}
	return (long)new_fd;
}

static long linux_fcntl(struct linux_process *process, u64 fd, u64 cmd, u64 arg)
{
	linux_debug_fcntl_log(cmd);

	if (fd >= process->fd_capacity || process->fds[fd].kind == 0) {
		linux_debug_openrc_state_log_fd(process, 0, fd, "fcntl-badfd",
						-LINUX_EBADF, cmd, arg);
		return -LINUX_EBADF;
	}

	if (cmd == LINUX_F_GETFD) {
		return (process->fds[fd].flags & LINUX_FD_CLOEXEC) != 0 ?
		       LINUX_FD_CLOEXEC : 0;
	}
	if (cmd == LINUX_F_SETFD) {
		if ((arg & LINUX_FD_CLOEXEC) != 0) {
			process->fds[fd].flags |= LINUX_FD_CLOEXEC;
		} else {
			process->fds[fd].flags &= ~LINUX_FD_CLOEXEC;
		}
		return 0;
	}
	if (cmd == LINUX_F_GETFL) {
		return (long)process->fds[fd].status_flags;
	}
	if (cmd == LINUX_F_SETFL) {
		process->fds[fd].status_flags =
			(process->fds[fd].status_flags & LINUX_O_ACCMODE) |
			(arg & (LINUX_O_APPEND | LINUX_O_NONBLOCK));
		return 0;
	}
	if (cmd == LINUX_F_DUPFD || cmd == LINUX_F_DUPFD_CLOEXEC) {
		for (;;) {
			for (u64 new_fd = arg; new_fd < process->fd_capacity; new_fd++) {
				if (process->fds[new_fd].kind == 0) {
					process->fds[new_fd] = process->fds[fd];
					if (cmd == LINUX_F_DUPFD_CLOEXEC) {
						process->fds[new_fd].flags |=
							LINUX_FD_CLOEXEC;
					} else {
						process->fds[new_fd].flags &=
							~LINUX_FD_CLOEXEC;
					}
					linux_fd_ref_add(&process->fds[new_fd]);
					if (process->fds[new_fd].kind ==
					    LINUX_FD_PIPE_READ ||
					    process->fds[new_fd].kind ==
					    LINUX_FD_PIPE_WRITE) {
						linux_debug_pipe_log(
							"fcntl-dup", process,
							new_fd,
							linux_pipe_find(
								process->fds[new_fd].handle),
							(long)fd);
					}
					return (long)new_fd;
				}
			}
			if (linux_process_ensure_fds(process,
						     process->fd_capacity + 1) != 0) {
				return -LINUX_EMFILE;
			}
		}
	}
	if (cmd == LINUX_F_SETLK || cmd == LINUX_F_SETLKW ||
	    cmd == LINUX_F_GETLK) {
		if (arg == 0) {
			linux_debug_openrc_state_log_fd(process,
							&process->fds[fd], fd,
							"fcntl-lock-efault",
							-LINUX_EFAULT,
							cmd, arg);
			return -LINUX_EFAULT;
		}
		return 0;
	}

	linux_debug_openrc_state_log_fd(process, &process->fds[fd], fd,
					"fcntl-unsupported",
					-LINUX_EINVAL, cmd, arg);
	return -LINUX_EINVAL;
}

static long linux_exec_process(struct linux_process *process)
{
	if (process == 0) {
		return -LINUX_ESRCH;
	}

	for (u64 signal = 1; signal < 64; signal++) {
		if (process->signal_handlers[signal] != 0) {
			process->signal_handlers[signal] = 0;
			process->signal_restorers[signal] = 0;
			process->signal_flags[signal] = 0;
		}
	}

	for (u64 fd = 0; fd < process->fd_capacity; fd++) {
		if (process->fds[fd].kind != 0 &&
		    (process->fds[fd].flags & LINUX_FD_CLOEXEC) != 0) {
			if (process->fds[fd].kind == LINUX_FD_PIPE_READ ||
			    process->fds[fd].kind == LINUX_FD_PIPE_WRITE) {
				linux_debug_pipe_log(
					"exec-close", process, fd,
					linux_pipe_find(process->fds[fd].handle),
					0);
			}
			const long closed = linux_close(process, fd);

			if (closed != 0) {
				return closed;
			}
		}
	}

	return (long)process->pid;
}

static long linux_uname(u64 buffer)
{
	char uts[LINUX_UTSNAME_SIZE];

	if (buffer == 0) {
		return -LINUX_EFAULT;
	}
	zero_bytes(uts, sizeof(uts));
	copy_field(uts, 0 * 65, 65, "Bunix");
	copy_field(uts, 1 * 65, 65, linux_hostname);
	copy_field(uts, 2 * 65, 65, "0.1");
	copy_field(uts, 3 * 65, 65, "#1 Bunix");
	copy_field(uts, 4 * 65, 65, "x86_64");
	copy_field(uts, 5 * 65, 65, "local");
	return bunix_buffer_write(buffer, 0, uts, sizeof(uts)) == 0 ? 0 :
	       -LINUX_EFAULT;
}

static long linux_sethostname(struct linux_process *process, u64 len,
			      u64 buffer, u64 cap_rights)
{
	const long euid = linux_user_credential(process, BUNIX_LINUX_GETEUID);

	if (euid < 0) {
		return euid;
	}
	if (euid != 0) {
		return -LINUX_EPERM;
	}
	if (len >= sizeof(linux_hostname)) {
		return -LINUX_EINVAL;
	}
	if (len != 0 &&
	    (buffer == 0 || (cap_rights & BUNIX_RIGHT_RECV) == 0 ||
	     bunix_buffer_read(buffer, 0, linux_hostname, len) != 0)) {
		return -LINUX_EFAULT;
	}
	linux_hostname[len] = '\0';
	return 0;
}

static u64 linux_monotonic_ns(void)
{
	return bunix_clock_monotonic_ns();
}

static u64 linux_realtime_ns(void)
{
	return (LINUX_REALTIME_BOOT_SECONDS * 1000000000ull) +
	       linux_monotonic_ns();
}

static u64 linux_realtime_seconds(void)
{
	return linux_realtime_ns() / 1000000000ull;
}

static long linux_sysinfo(u64 buffer)
{
	char info[LINUX_SYSINFO_SIZE];

	if (buffer == 0) {
		return -LINUX_EFAULT;
	}
	zero_bytes(info, sizeof(info));
	store_u64(info, 0, bunix_clock_monotonic_ns() / 1000000000ull);
	store_u64(info, 32, 128ull * 1024ull * 1024ull);
	store_u64(info, 40, 64ull * 1024ull * 1024ull);
	store_u16(info, 80, 1);
	store_u32(info, 104, 1);
	return bunix_buffer_write(buffer, 0, info, sizeof(info)) == 0 ? 0 :
	       -LINUX_EFAULT;
}

static long linux_getrandom(u64 len, u64 buffer)
{
	u64 done = 0;

	if (buffer == 0 && len != 0) {
		return -LINUX_EFAULT;
	}
	while (done < len) {
		const u64 chunk = len - done > sizeof(write_buffer) ?
				  sizeof(write_buffer) : len - done;

		for (u64 i = 0; i < chunk; i++) {
			write_buffer[i] = (char)(0xa5u ^
						 (unsigned char)(done + i));
		}
		if (bunix_buffer_write(buffer, done, write_buffer, chunk) != 0) {
			return done != 0 ? (long)done : (long)-LINUX_EFAULT;
		}
		done += chunk;
	}
	return (long)len;
}

static long linux_clock_gettime(u64 clock_id, u64 buffer)
{
	char timespec[LINUX_TIMESPEC_SIZE];
	u64 ns;

	if (buffer == 0) {
		return -LINUX_EFAULT;
	}
	if (clock_id == LINUX_CLOCK_REALTIME) {
		ns = linux_realtime_ns();
	} else if (clock_id == LINUX_CLOCK_MONOTONIC) {
		ns = linux_monotonic_ns();
	} else {
		return -LINUX_EINVAL;
	}
	zero_bytes(timespec, sizeof(timespec));
	store_u64(timespec, 0, ns / 1000000000ull);
	store_u64(timespec, 8, ns % 1000000000ull);
	return bunix_buffer_write(buffer, 0, timespec, sizeof(timespec)) == 0 ?
	       0 : -LINUX_EFAULT;
}

static long linux_gettimeofday(u64 buffer)
{
	char timeval[LINUX_TIMEVAL_SIZE];
	const u64 ns = linux_realtime_ns();

	if (buffer == 0) {
		return 0;
	}
	zero_bytes(timeval, sizeof(timeval));
	store_u64(timeval, 0, ns / 1000000000ull);
	store_u64(timeval, 8, (ns % 1000000000ull) / 1000ull);
	return bunix_buffer_write(buffer, 0, timeval, sizeof(timeval)) == 0 ?
	       0 : -LINUX_EFAULT;
}

static long linux_time(u64 buffer, u64 *seconds_out)
{
	char time_value[LINUX_TIME_T_SIZE];
	const u64 seconds = linux_realtime_seconds();

	if (seconds_out != 0) {
		*seconds_out = seconds;
	}
	if (buffer == 0) {
		return (long)seconds;
	}
	zero_bytes(time_value, sizeof(time_value));
	store_u64(time_value, 0, seconds);
	return bunix_buffer_write(buffer, 0, time_value, sizeof(time_value)) == 0 ?
	       (long)seconds : (long)-LINUX_EFAULT;
}

static void linux_close_process_fds(struct linux_process *process)
{
	if (process == 0) {
		return;
	}

	for (u64 fd = 0; fd < process->fd_capacity; fd++) {
		if (process->fds[fd].kind != 0) {
			(void)linux_close(process, fd);
		}
	}
}

static void linux_process_reset(struct linux_process *process)
{
	if (process == 0) {
		return;
	}

	(void)linux_user_process_exit(process->bunix_task);
	linux_child_unlink(process);
	linux_close_process_fds(process);
	(void)bunix_map_remove(&process_by_task, process->bunix_task);
	(void)bunix_map_remove(&process_by_pid, process->pid);
	process->pid = 0;
	process->tid = 0;
	process->ppid = 0;
	process->pgid = 0;
	process->sid = 0;
	linux_process_init_links(process);
	process->bunix_task = 0;
	process->bunix_proc_pid = 0;
	process->bunix_thread = 0;
	process->exited = 0;
	process->exit_status = 0;
	process->waited = 0;
	process->waiter = 0;
	process->wait_buffer = 0;
	process->wait_pid = 0;
	process->pending_signals = 0;
	process->signal_mask = 0;
	process->signal_ignored = 0;
	for (u64 signal = 0; signal < 64; signal++) {
		process->signal_handlers[signal] = 0;
	process->signal_restorers[signal] = 0;
	process->signal_flags[signal] = 0;
	}
	process->alarm_deadline_ns = 0;
	process->alarm_active = 0;
	process->umask = 0;
	process->session_id = 0;
	process->session_owner = 0;
	(void)linux_close_vfs_handle(process->cwd_handle);
	process->cwd_handle = 0;
	bunix_free(process->cwd);
	process->cwd = 0;
	bunix_free(process->fds);
	process->fds = 0;
	process->fd_capacity = 0;
	bunix_free(process);
}

int main(void)
{
	const char online[] = "linux-server: online\n";
	const char bad_fd[] = "linux-server: ebadf\n";
	const char open_ok[] = "linux-server: openat\n";
	const char fstat_ok[] = "linux-server: fstat\n";
	const char newfstatat_ok[] = "linux-server: newfstatat\n";
	const char registered_ok[] = "linux-server: registered\n";
	const char wait4_ok[] = "linux-server: wait4\n";
	const char exited_ok[] = "linux-server: exited\n";
	const char close_ok[] = "linux-server: close\n";
	const char exit_group[] = "linux-server: exit_group\n";
	struct bunix_msg message;

	if (register_service(BUNIX_SERVICE_LINUX) != 0) {
		return 1;
	}
	bunix_map_init(&process_by_task);
	bunix_map_init(&process_by_pid);
	bunix_map_init(&file_refs);
	bunix_map_init(&net_socket_refs);
	bunix_map_init(&tty_input_tasks);
	bunix_id_table_init(&pipe_ids);
	bunix_console_log(online, sizeof(online) - 1);
	for (;;) {
		struct bunix_msg reply = {
			.protocol = BUNIX_PROTO_LINUX,
			.type = 0,
			.sender = 0,
			.cap_rights = 0,
			.reply = 0,
			.cap = 0,
			.words = { 0, 0, 0, 0 },
		};
		int should_reply = 1;
		int management = 0;

		if (recv_linux_message(&message, &management) != 0 ||
		    message.protocol != BUNIX_PROTO_LINUX) {
			continue;
		}

		linux_debug_count_message(message.type);
		reply.type = message.type;
		if (message.type == BUNIX_LINUX_GRANT_TTY_INPUT_TASK) {
			if (!management) {
				reply.words[0] = (u64)-LINUX_EPERM;
			} else {
				reply.words[0] =
					(u64)linux_grant_tty_input_task(
						message.words[0]);
			}
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			if (message.reply != 0) {
				bunix_ipc_send(message.reply, &reply);
			}
			continue;
		}
		if (message.type == BUNIX_LINUX_TTY_INPUT) {
			if (!linux_tty_input_authorized(message.sender)) {
				reply.words[0] = (u64)-LINUX_EPERM;
			} else {
				linux_tty_input_event(&console_tty,
						      (char)(unsigned char)
						      message.words[0]);
				reply.words[0] = 0;
			}
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			if (message.reply != 0) {
				bunix_ipc_send(message.reply, &reply);
			}
			continue;
		}
		if (message.type == BUNIX_LINUX_TTY_INPUT_BUFFER) {
			if (!linux_tty_input_authorized(message.sender)) {
				reply.words[0] = (u64)-LINUX_EPERM;
			} else if ((message.cap_rights & BUNIX_RIGHT_RECV) != 0) {
				linux_tty_input_buffer_event(&console_tty,
							     message.cap,
							     message.words[0]);
				reply.words[0] = 0;
			} else {
				reply.words[0] = (u64)-LINUX_EINVAL;
			}
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			if (message.reply != 0) {
				bunix_ipc_send(message.reply, &reply);
			}
			continue;
		}
		if (message.type == BUNIX_LINUX_REGISTER_PROCESS) {
			if (!management && message.sender != 0) {
				reply.words[0] = (u64)-LINUX_EPERM;
				if (message.reply != 0) {
					bunix_ipc_send(message.reply, &reply);
				}
				continue;
			}
			reply.words[0] = (u64)linux_register_process(message.words[0],
								     message.words[1],
								     message.words[2],
								     message.words[3]);
			if ((long)reply.words[0] > 0) {
				bunix_console_log(registered_ok,
						    sizeof(registered_ok) - 1);
			}
			if (message.reply != 0) {
				bunix_ipc_send(message.reply, &reply);
			}
			continue;
		}
		if (message.type == BUNIX_LINUX_FORK_PROCESS) {
			if (!management &&
			    (message.sender == 0 ||
			     message.words[0] != message.sender ||
			     linux_process_find(message.sender) == 0)) {
				reply.words[0] = (u64)-LINUX_EPERM;
				if (message.reply != 0) {
					bunix_ipc_send(message.reply, &reply);
				}
				continue;
			}
			reply.words[0] = (u64)linux_fork_process(message.words[0],
								 message.words[1]);
			if ((long)reply.words[0] > 0) {
				bunix_console_log(registered_ok,
						    sizeof(registered_ok) - 1);
			}
			if (message.reply != 0) {
				bunix_ipc_send(message.reply, &reply);
			}
			continue;
		}
		if (message.type == BUNIX_LINUX_EXEC_INIT) {
			if (!management && message.sender != 0) {
				reply.words[0] = (u64)-LINUX_EPERM;
				if (message.cap != 0) {
					bunix_handle_close(message.cap);
				}
				if (message.reply != 0) {
					bunix_ipc_send(message.reply, &reply);
				}
				continue;
			}
			reply.words[0] = (u64)linux_exec_init(message.words[0],
							     message.cap);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			if (message.reply != 0) {
				bunix_ipc_send(message.reply, &reply);
			}
			continue;
		}
		if (message.type == BUNIX_LINUX_EXEC_PREPARE) {
			if (!management &&
			    (message.sender == 0 ||
			     linux_process_find(message.sender) == 0)) {
				reply.words[0] = (u64)-LINUX_EPERM;
				if (message.cap != 0) {
					bunix_handle_close(message.cap);
				}
				if (message.reply != 0) {
					bunix_ipc_send(message.reply, &reply);
				}
				continue;
			}
			reply.words[0] = (u64)linux_exec_prepare(message.words[0],
								 message.words[1],
								 message.words[2],
								 message.cap,
								 &reply.words[1],
								 &reply.words[2]);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			if (message.reply != 0) {
				bunix_ipc_send(message.reply, &reply);
			}
			continue;
		}
		if (message.type == BUNIX_LINUX_TASK_FAULT) {
			reply.words[0] = (u64)linux_task_fault(message.words[0],
							       message.words[1],
							       message.words[2],
							       message.words[3],
							       message.cap,
							       message.cap_rights,
							       &reply.words[1],
							       &reply.words[2],
							       &reply.words[3]);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			if (message.reply != 0) {
				bunix_ipc_send(message.reply, &reply);
			}
			continue;
		}

		struct linux_process *process = linux_process_for(&message);
		if (process == 0) {
			reply.words[0] = (u64)-LINUX_ESRCH;
			if (message.reply != 0) {
				bunix_ipc_send(message.reply, &reply);
			}
			continue;
		}

		switch (message.type) {
		case BUNIX_LINUX_GETPID:
			reply.words[0] = process->pid;
			break;
		case BUNIX_LINUX_GETTID:
			reply.words[0] = process->tid;
			break;
		case BUNIX_LINUX_GETCWD:
			reply.words[0] = (u64)linux_getcwd(process,
							   message.words[0],
							   message.cap,
							   &reply.words[1],
							   &reply.words[2]);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_CHDIR:
			reply.words[0] = (u64)linux_chdir(process,
							  message.words[0],
							  message.cap);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_FCHDIR:
			reply.words[0] = (u64)linux_fchdir(process,
							   message.words[0]);
			break;
		case BUNIX_LINUX_GETUID:
		case BUNIX_LINUX_GETGID:
		case BUNIX_LINUX_GETEUID:
		case BUNIX_LINUX_GETEGID:
			reply.words[0] = (u64)linux_user_credential(process,
								    message.type);
			break;
		case BUNIX_LINUX_GETGROUPS:
			if (message.cap != 0) {
				reply.words[0] =
					(u64)linux_user_groups_buffer(process,
								      message.words[0],
								      message.cap);
			} else {
				reply.words[0] =
					(u64)linux_user_groups(process,
							       message.words[0],
							       &reply.words[1],
							       &reply.words[2]);
			}
			break;
		case BUNIX_LINUX_SETUID:
			reply.words[0] = (u64)linux_user_setres(process,
								BUNIX_USER_SETRESUID,
								message.words[0],
								message.words[0],
								message.words[0]);
			break;
		case BUNIX_LINUX_SETGID:
			reply.words[0] = (u64)linux_user_setres(process,
								BUNIX_USER_SETRESGID,
								message.words[0],
								message.words[0],
								message.words[0]);
			break;
		case BUNIX_LINUX_SETRESUID:
			reply.words[0] = (u64)linux_user_setres(process,
								BUNIX_USER_SETRESUID,
								message.words[0],
								message.words[1],
								message.words[2]);
			break;
		case BUNIX_LINUX_SETRESGID:
			reply.words[0] = (u64)linux_user_setres(process,
								BUNIX_USER_SETRESGID,
								message.words[0],
								message.words[1],
								message.words[2]);
			break;
		case BUNIX_LINUX_SETGROUPS:
			if (message.cap != 0) {
				reply.words[0] =
					(u64)linux_user_setgroups_buffer(process,
									 message.words[0],
									 message.cap);
			} else {
				reply.words[0] =
					(u64)linux_user_setgroups(process,
								  message.words[0],
								  message.words[1],
								  message.words[2]);
			}
			break;
		case BUNIX_LINUX_REBOOT:
		{
			const long euid =
				linux_user_credential(process, BUNIX_LINUX_GETEUID);

			if (message.words[0] != LINUX_REBOOT_MAGIC1 ||
			    (message.words[1] != LINUX_REBOOT_MAGIC2 &&
			     message.words[1] != LINUX_REBOOT_MAGIC2A &&
			     message.words[1] != LINUX_REBOOT_MAGIC2B &&
			     message.words[1] != LINUX_REBOOT_MAGIC2C)) {
				reply.words[0] = (u64)-LINUX_EINVAL;
				break;
			}
			if (euid < 0) {
				reply.words[0] = (u64)euid;
				break;
			}
			if (euid != 0) {
				reply.words[0] = (u64)-LINUX_EPERM;
				break;
			}
			if (message.words[2] == LINUX_REBOOT_CMD_POWER_OFF ||
			     message.words[2] == LINUX_REBOOT_CMD_HALT ||
			     message.words[2] == LINUX_REBOOT_CMD_RESTART) {
				reply.words[0] =
					(u64)bunix_machine_poweroff(
						LINUX_HANDLE_POWER_AUTH);
				break;
			}
			reply.words[0] = (u64)-LINUX_EINVAL;
			break;
		}
		case BUNIX_LINUX_GETPPID:
			reply.words[0] = process->ppid;
			break;
		case BUNIX_LINUX_GETPGRP:
			reply.words[0] = process->pgid;
			break;
		case BUNIX_LINUX_UMASK:
			reply.words[0] = (u64)linux_umask(process,
							  message.words[0]);
			break;
		case BUNIX_LINUX_GETPGID:
			reply.words[0] = (u64)linux_getpgid(process,
							    message.words[0]);
			break;
		case BUNIX_LINUX_GETSID:
			reply.words[0] = (u64)linux_getsid(process,
							   message.words[0]);
			break;
		case BUNIX_LINUX_ATTACH_SESSION:
			reply.words[0] = (u64)linux_attach_session(process,
								   message.words[0]);
			break;
		case BUNIX_LINUX_SETPGID:
			reply.words[0] = (u64)linux_setpgid(process,
							    message.words[0],
							    message.words[1]);
			break;
		case BUNIX_LINUX_SETSID:
			reply.words[0] = (u64)linux_setsid(process);
			break;
		case BUNIX_LINUX_KILL:
			reply.words[0] = (u64)linux_kill(process,
							 (long)message.words[0],
							 message.words[1]);
			break;
		case BUNIX_LINUX_RT_SIGACTION:
			reply.words[0] = (u64)linux_rt_sigaction(process,
								 message.words[0],
								 message.words[1],
								 message.words[2],
								 message.words[3],
								 &reply.words[1],
								 &reply.words[2],
								 &reply.words[3]);
			break;
		case BUNIX_LINUX_RT_SIGPROCMASK:
			reply.words[0] = (u64)linux_rt_sigprocmask(process,
								   message.words[0],
								   message.words[1],
								   &reply.words[1]);
			break;
		case BUNIX_LINUX_RT_SIGTIMEDWAIT:
			reply.words[0] = (u64)linux_rt_sigtimedwait(process,
								    message.words[0],
								    message.words[2]);
			break;
		case BUNIX_LINUX_ALARM:
			reply.words[0] = (u64)linux_alarm(process,
							  message.words[0]);
			break;
		case BUNIX_LINUX_SETITIMER:
			reply.words[0] = (u64)linux_setitimer_real(
				process, message.words[1], message.words[2]);
			break;
		case BUNIX_LINUX_SIGNAL_PENDING:
			reply.words[0] = linux_signal_deliverable(process) != 0;
			break;
		case BUNIX_LINUX_SIGNAL_DEQUEUE:
			reply.words[0] = (u64)linux_signal_dequeue(process,
								   &reply.words[1],
								   &reply.words[2],
								   &reply.words[3]);
			break;
		case BUNIX_LINUX_IOCTL:
			reply.words[0] = (u64)linux_ioctl(process,
							  message.words[0],
							  message.words[1],
							  message.words[2],
							  message.cap);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_WAIT4: {
			const long waited = linux_wait4(process,
						       (long)message.words[0],
						       message.words[1],
						       message.cap,
						       message.reply);

			if (waited == LINUX_WAIT_BLOCK) {
				should_reply = 0;
			} else {
				reply.words[0] = (u64)waited;
				if (waited > 0) {
					linux_debug_rpc_log(wait4_ok,
							    sizeof(wait4_ok) - 1);
				}
				if (message.cap != 0) {
					bunix_handle_close(message.cap);
				}
			}
			break;
		}
		case BUNIX_LINUX_OPEN:
		case BUNIX_LINUX_OPENAT:
			reply.words[0] = (u64)linux_openat(process,
							   message.words[0],
							   message.words[1],
							   message.words[2],
							   message.words[3],
							   message.cap);
			linux_debug_path_arg_result_log(process, "openat",
							message.words[1],
							message.cap,
							(long)reply.words[0]);
			if ((long)reply.words[0] >= 0) {
				linux_debug_rpc_log(open_ok, sizeof(open_ok) - 1);
			}
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_ACCESS:
		case BUNIX_LINUX_FACCESSAT:
		case BUNIX_LINUX_FACCESSAT2:
			reply.words[0] = (u64)linux_faccessat(process,
							      message.words[0],
							      message.words[1],
							      message.words[2],
							      message.words[3],
							      message.cap);
			linux_debug_path_arg_result_log(process, "faccessat",
							message.words[1],
							message.cap,
							(long)reply.words[0]);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_UNLINK:
		case BUNIX_LINUX_UNLINKAT:
			reply.words[0] = (u64)linux_unlinkat(process,
							     message.words[0],
							     message.words[1],
							     message.words[2],
							     message.cap);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_RMDIR:
			reply.words[0] = (u64)linux_rmdir(process,
							  message.words[1],
							  message.cap);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_MOUNT:
			reply.words[0] = (u64)linux_mount(process,
							  message.words[0],
							  message.words[1],
							  message.words[2],
							  message.cap);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_UMOUNT2:
			reply.words[0] = (u64)linux_umount2(process,
							    message.words[0],
							    message.words[1],
							    message.cap);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_TRUNCATE:
			reply.words[0] = (u64)linux_truncate(process,
							     message.words[0],
							     message.words[1],
							     message.words[2],
							     message.cap);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_MKDIR:
		case BUNIX_LINUX_MKDIRAT:
			reply.words[0] = (u64)linux_mkdirat(process,
							    message.words[0],
							    message.words[1],
							    message.words[2],
							    message.cap);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_MKNOD:
		case BUNIX_LINUX_MKNODAT:
			reply.words[0] = (u64)linux_mknodat(process,
							    message.words[0],
							    message.words[1],
							    message.words[2],
							    message.words[3],
							    message.cap);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_SYMLINKAT:
			reply.words[0] = (u64)linux_symlinkat(process,
							      message.words[0],
							      message.words[1],
							      message.words[2],
							      message.cap);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_LINK:
		case BUNIX_LINUX_LINKAT:
			reply.words[0] = (u64)linux_linkat(
				process, message.words[0], message.words[1],
				message.words[2], message.words[3] & 0xffffffff,
				message.words[3] >> 32, message.cap);
			linux_debug_path_arg_result_log(process, "linkat-old",
							message.words[2],
							message.cap,
							(long)reply.words[0]);
			linux_debug_path_arg_offset_result_log(
				process, "linkat-new",
				message.words[3] & 0xffffffff, message.cap,
				message.words[2], (long)reply.words[0]);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_RENAME:
		case BUNIX_LINUX_RENAMEAT:
		case BUNIX_LINUX_RENAMEAT2:
			reply.words[0] = (u64)linux_renameat2(
				process, message.words[0], message.words[1],
				message.words[2], message.words[3] & 0xffffffff,
				message.words[3] >> 32, message.cap);
			linux_debug_path_arg_result_log(process, "renameat-old",
							message.words[2],
							message.cap,
							(long)reply.words[0]);
			linux_debug_path_arg_offset_result_log(
				process, "renameat-new",
				message.words[3] & 0xffffffff, message.cap,
				message.words[2], (long)reply.words[0]);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_CHMOD:
		case BUNIX_LINUX_FCHMODAT:
			reply.words[0] = (u64)linux_chmodat(process,
							    message.words[0],
							    message.words[1],
							    message.words[2],
							    message.words[3],
							    message.cap);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_CHOWN:
		case BUNIX_LINUX_LCHOWN:
		case BUNIX_LINUX_FCHOWNAT:
			reply.words[0] = (u64)linux_chownat(process,
							    message.words[0],
							    message.words[1],
							    (message.words[2] &
							     0xffffffff) ==
							    0xffffffff ?
							    (u64)-1 :
							    message.words[2] &
							    0xffffffff,
							    (message.words[3] &
							     0xffffffff) ==
							    0xffffffff ?
							    (u64)-1 :
							    message.words[3] &
							    0xffffffff,
							    message.words[3] >> 32,
							    message.cap);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_FSTAT:
			reply.words[0] = (u64)linux_fstat(process,
							  message.words[0],
							  message.cap);
			if (reply.words[0] == 0) {
				linux_debug_rpc_log(fstat_ok,
						    sizeof(fstat_ok) - 1);
			}
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_FTRUNCATE:
			reply.words[0] = (u64)linux_ftruncate(process,
							      message.words[0],
							      message.words[1]);
			break;
		case BUNIX_LINUX_FCHMOD:
			reply.words[0] = (u64)linux_fchmod(process,
							   message.words[0],
							   message.words[1]);
			break;
		case BUNIX_LINUX_FCHOWN:
			reply.words[0] = (u64)linux_fchown(process,
							   message.words[0],
							   message.words[1],
							   message.words[2]);
			break;
		case BUNIX_LINUX_FCNTL:
			reply.words[0] = (u64)linux_fcntl(process,
							  message.words[0],
							  message.words[1],
							  message.words[2]);
			break;
		case BUNIX_LINUX_CLOSE_RANGE:
			reply.words[0] = (u64)linux_close_range(process,
								message.words[0],
								message.words[1],
								message.words[2]);
			break;
		case BUNIX_LINUX_FLOCK:
			reply.words[0] = message.words[0] < process->fd_capacity &&
						 process->fds[message.words[0]].kind != 0 ?
					 0 : (u64)-LINUX_EBADF;
			break;
		case BUNIX_LINUX_POLL:
		case BUNIX_LINUX_PPOLL:
			reply.words[0] = (u64)linux_pollfd(process,
							   (long)message.words[0],
							   message.words[1]);
			break;
		case BUNIX_LINUX_DUP:
			reply.words[0] = (u64)linux_dup_to(process,
							   message.words[0],
							   0, 0, 0, 0);
			break;
		case BUNIX_LINUX_DUP2:
			reply.words[0] = (u64)linux_dup_to(process,
							   message.words[0],
							   message.words[1],
							   0, 1, 1);
			break;
		case BUNIX_LINUX_DUP3:
			reply.words[0] = (u64)linux_dup_to(process,
							   message.words[0],
							   message.words[1],
							   message.words[2],
							   1, 0);
			break;
		case BUNIX_LINUX_PIPE:
		case BUNIX_LINUX_PIPE2:
			reply.words[0] = (u64)linux_pipe_create(process,
								message.words[0],
								&reply.words[1],
								&reply.words[2]);
			break;
		case BUNIX_LINUX_UNAME:
			reply.words[0] = (u64)linux_uname(message.cap);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_SETHOSTNAME:
			reply.words[0] = (u64)linux_sethostname(
				process, message.words[0], message.cap,
				message.cap_rights);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_SYSINFO:
			reply.words[0] = (u64)linux_sysinfo(message.cap);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_STATFS:
			reply.words[0] = (u64)linux_statfs(process,
							   message.words[1],
							   message.cap);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_FSTATFS:
			reply.words[0] = (u64)linux_fstatfs(process,
							    message.words[0],
							    message.cap);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_GETRANDOM:
			reply.words[0] = (u64)linux_getrandom(message.words[1],
							      message.cap);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_CLOCK_GETTIME:
			reply.words[0] = (u64)linux_clock_gettime(message.words[0],
								  message.cap);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_GETTIMEOFDAY:
			reply.words[0] = (u64)linux_gettimeofday(message.cap);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_TIME:
			reply.words[0] = (u64)linux_time(message.cap,
							 &reply.words[1]);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_SOCKET:
			reply.words[0] = (u64)linux_socket(process,
							   message.words[0],
							   message.words[1],
							   message.words[2]);
			break;
		case BUNIX_LINUX_CONNECT:
			reply.words[0] = (u64)linux_connect(process,
							    message.words[0],
							    message.words[1],
							    message.cap);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_BIND:
			reply.words[0] = (u64)linux_bind(process,
							 message.words[0],
							 message.words[1],
							 message.cap);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_LISTEN:
			reply.words[0] = (u64)linux_listen(process,
							   message.words[0],
							   message.words[1]);
			break;
		case BUNIX_LINUX_ACCEPT:
			reply.words[0] = (u64)linux_accept(process,
							   message.words[0],
							   0);
			break;
		case BUNIX_LINUX_ACCEPT4:
			reply.words[0] = (u64)linux_accept(process,
							   message.words[0],
							   message.words[1]);
			break;
		case BUNIX_LINUX_SHUTDOWN:
			reply.words[0] = (u64)linux_shutdown(process,
							     message.words[0],
							     message.words[1]);
			break;
		case BUNIX_LINUX_SENDTO:
			reply.words[0] = (u64)linux_sendto(process,
							   message.words[0],
							   message.words[1],
							   message.words[2],
							   message.words[3],
							   message.cap);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_RECVFROM:
			reply.words[0] = (u64)linux_recvfrom(process,
							     message.words[0],
							     message.words[1],
							     message.words[2],
							     message.cap,
							     message.words[3],
							     &reply.words[1]);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_RECVMSG:
			reply.words[0] = (u64)linux_recvmsg(process,
							    message.words[0],
							    message.words[1],
							    message.words[2],
							    message.words[3],
							    message.cap,
							    &reply.words[1]);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_GETSOCKNAME:
			reply.words[0] = (u64)linux_socket_addr(process,
								message.words[0],
								message.words[1],
								message.cap, 0,
								&reply.words[1]);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_GETPEERNAME:
			reply.words[0] = (u64)linux_socket_addr(process,
								message.words[0],
								message.words[1],
								message.cap, 1,
								&reply.words[1]);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_SETSOCKOPT:
			reply.words[0] = (u64)linux_setsockopt(process,
							       message.words[0],
							       message.words[1],
							       message.words[2],
							       message.words[3],
							       message.cap);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_GETSOCKOPT:
			reply.words[0] = (u64)linux_getsockopt(process,
							       message.words[0],
							       message.words[1],
							       message.words[2],
							       message.words[3],
							       message.cap,
							       &reply.words[1]);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_EXEC_PROCESS:
			reply.words[0] = (u64)linux_exec_process(process);
			break;
		case BUNIX_LINUX_RESOLVE_FD_PATH:
			reply.words[0] = (u64)linux_resolve_fd_path(
				process, message.words[0], message.words[1],
				message.cap, &reply.words[1]);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_NEWFSTATAT:
		{
			struct linux_vfs_stat stat;
			const long result = linux_newfstatat(process,
							    message.words[0],
							    message.words[1],
							    message.cap,
							    message.words[2],
							    &stat);

			reply.words[0] = (u64)result;
			if (result == 0) {
				reply.words[0] =
					(u64)linux_stat_from_vfs_stat(message.cap,
								      &stat);
			}
			if (reply.words[0] == 0) {
				linux_debug_rpc_log(newfstatat_ok,
						    sizeof(newfstatat_ok) - 1);
			}
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		}
		case BUNIX_LINUX_STATX:
			reply.words[0] = (u64)linux_statx(process,
							  message.words[0],
							  message.words[1],
							  message.words[2],
							  message.words[3],
							  message.cap);
			linux_debug_path_arg_result_log(process, "statx",
							message.words[1],
							message.cap,
							(long)reply.words[0]);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_UTIMENSAT:
			reply.words[0] = (u64)linux_utimensat(process,
							      message.words[0],
							      message.words[1],
							      message.words[2],
							      message.cap);
			linux_debug_path_arg_result_log(process, "utimensat",
							message.words[1],
							message.cap,
							(long)reply.words[0]);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_READLINK:
		case BUNIX_LINUX_READLINKAT:
			reply.words[0] = (u64)linux_readlinkat(process,
							       message.words[0],
							       message.words[1],
							       message.words[2],
							       message.cap);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_READ:
		{
			int blocked = 0;

			reply.words[0] = (u64)linux_read(process,
							 message.words[0],
							 message.words[1],
							 message.cap,
							 message.reply,
							 &blocked);
			if (blocked) {
				should_reply = 0;
			} else if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		}
		case BUNIX_LINUX_MMAP:
			reply.words[0] = (u64)linux_mmap_read(process,
							      message.words[0],
							      message.words[1],
							      message.words[2],
							      message.cap);
			reply.words[1] = process->last_mmap_page_class;
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_SENDFILE:
			reply.words[1] = message.words[3];
			reply.words[0] = (u64)linux_sendfile(process,
							     message.words[0],
							     message.words[1],
							     message.words[2],
							     message.words[3],
							     message.cap,
							     &reply.words[1]);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_GETDENTS64:
			reply.words[0] = (u64)linux_getdents64(process,
							       message.words[0],
							       message.cap,
							       message.words[1]);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_WRITE: {
			const u64 fd = message.words[0];
			const u64 len = message.words[1];
			int blocked = 0;

			reply.words[0] = (u64)linux_write_buffer(process, fd, len,
								 message.cap,
								 message.reply,
								 &blocked);
			if (reply.words[0] == (u64)-LINUX_EBADF) {
				bunix_console_log(bad_fd, sizeof(bad_fd) - 1);
			}
			if (blocked) {
				should_reply = 0;
			} else if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		}
		case BUNIX_LINUX_LSEEK:
			reply.words[0] = (u64)linux_lseek(process,
							  message.words[0],
							  message.words[1],
							  message.words[2]);
			break;
		case BUNIX_LINUX_CLOSE:
			reply.words[0] = (u64)linux_close(process,
							  message.words[0]);
			if (reply.words[0] == 0) {
				linux_debug_rpc_log(close_ok,
						    sizeof(close_ok) - 1);
			}
			break;
		case BUNIX_LINUX_EXIT_GROUP:
			bunix_console_log(exit_group, sizeof(exit_group) - 1);
			linux_process_exit_code(process, message.words[0], 0);
			bunix_console_log(exited_ok, sizeof(exited_ok) - 1);
			reply.words[0] = 0;
			break;
		default:
			reply.words[0] = (u64)-1;
			break;
		}

		if (should_reply && message.reply != 0) {
			bunix_ipc_send(message.reply, &reply);
		}
	}
}
