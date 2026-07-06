#include <bunix/libbunix.h>
#include <bunix/id_table.h>

enum {
	LINUX_HANDLE_VFS = 3,
	LINUX_HANDLE_NAMES = 4,
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
	LINUX_ESPIPE = 29,
	LINUX_EPIPE = 32,
	LINUX_ERANGE = 34,
	LINUX_ENAMETOOLONG = 36,
	LINUX_ENOSYS = 38,
	LINUX_ENOTEMPTY = 39,
	LINUX_ELOOP = 40,
	LINUX_EAFNOSUPPORT = 97,
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
	LINUX_SIGINT = 2,
	LINUX_SIGTERM = 15,
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
	LINUX_MAX_WRITE = 65536,
	LINUX_MAX_MMAP_BUFFER = 1024 * 1024,
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
	LINUX_PIPE_CAPACITY = 4096,
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
	LINUX_O_NOFOLLOW = 00400000,
	LINUX_O_DIRECTORY = 00200000,
	LINUX_O_CLOEXEC = 02000000,
	LINUX_DUP_CLOEXEC = 02000000,
	LINUX_FD_CLOEXEC = 1,
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
	LINUX_SOCK_STREAM = 1,
	LINUX_SOCK_NONBLOCK = 00004000,
	LINUX_SOCK_CLOEXEC = 02000000,
	LINUX_SOCKET_UNIX_STREAM = 1,
	LINUX_SOCKET_UTMPD = 2,
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

struct linux_fd {
	u64 handle;
	u64 kind;
	u64 offset;
	u64 size;
	u64 flags;
	u64 status_flags;
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
	u64 umask;
	u64 session_id;
	u64 cwd_handle;
	char *cwd;
	struct linux_fd *fds;
	u64 fd_capacity;
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
	char data[LINUX_PIPE_CAPACITY];
};

static struct bunix_map process_by_task;
static struct bunix_map process_by_pid;
static struct bunix_id_table pipe_ids;
static char write_buffer[LINUX_MAX_WRITE];
static char tty_termios[LINUX_TERM_SIZE];
static char tty_line[256];
static u64 tty_line_len;
static char tty_read_queue[512];
static u64 tty_read_queue_start;
static u64 tty_read_queue_len;
static u64 tty_reader_pid;
static u64 tty_reader_reply;
static u64 tty_reader_buffer;
static u64 tty_reader_len;
static struct bunix_map file_refs;
static u64 next_pid = 1;
static u64 foreground_pgid = 1;
static u64 user_service;
static u64 utmpfs_service;
static u64 procfs_service;
static u64 tmpfs_service;
static u64 devfs_service;
static u64 unionfs_service;

static u64 resolve_service(u64 service, unsigned int rights);
static void linux_process_reset(struct linux_process *process);
static void linux_close_process_fds(struct linux_process *process);
static long linux_user_process_exit(u64 pid);
static void linux_wake_parent(struct linux_process *child);
static void tty_cancel_reader(struct linux_process *process);
static u64 string_len(const char *text);
static long linux_check_name_max(const char *path);
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
	const u64 user = linux_user_service();

	if (user == 0 ||
	    bunix_ipc_call(user, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
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
	const u64 user = linux_user_service();

	if (user == 0 ||
	    bunix_ipc_call(user, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
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
	const u64 user = linux_user_service();

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
	const u64 user = linux_user_service();

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
	const u64 user = linux_user_service();

	if (user == 0 ||
	    bunix_ipc_call(user, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return -LINUX_ESRCH;
	}
	return 0;
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
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_NAMES,
		.type = BUNIX_NAMES_REGISTER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP,
		.reply = 0,
		.cap = BUNIX_HANDLE_SELF,
		.words = { BUNIX_NAMES_ROOT, service, 0, 0 },
	};
	struct bunix_msg reply;

	if (bunix_ipc_call(LINUX_HANDLE_NAMES, &request, &reply) != 0) {
		return -1;
	}

	return reply.words[0] == 0 ? 0 : -1;
}

static u64 resolve_service(u64 service, unsigned int rights)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_NAMES,
		.type = BUNIX_NAMES_WAIT,
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

static void notify_proc_exit(u64 linux_pid, u64 status, u64 kill_task)
{
	const u64 proc = resolve_service(BUNIX_SERVICE_PROC, BUNIX_RIGHT_SEND);
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

static void notify_proc_register_linux(u64 linux_pid, u64 bunix_task,
				       u64 parent_linux_pid)
{
	const u64 proc = resolve_service(BUNIX_SERVICE_PROC, BUNIX_RIGHT_SEND);
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_PROC,
		.type = BUNIX_PROC_REGISTER_LINUX,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { linux_pid, bunix_task, parent_linux_pid, 0 },
	};

	if (proc != 0) {
		(void)bunix_ipc_send(proc, &request);
	}
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
	const u64 proc = resolve_service(BUNIX_SERVICE_PROC, BUNIX_RIGHT_SEND);
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

static void string_copy(char *dst, const char *src)
{
	u64 i = 0;

	while (i + 1 < LINUX_MAX_PATH && src[i] != '\0') {
		dst[i] = src[i];
		i++;
	}
	dst[i] = '\0';
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

struct linux_exec_args {
	u64 argc;
	u64 envc;
	u64 bytes;
	char **argv;
	char **envp;
};

static void linux_exec_args_free(struct linux_exec_args *args)
{
	if (args == 0) {
		return;
	}
	if (args->argv != 0) {
		for (u64 i = 0; i < args->argc; i++) {
			bunix_free(args->argv[i]);
		}
		bunix_free(args->argv);
	}
	if (args->envp != 0) {
		for (u64 i = 0; i < args->envc; i++) {
			bunix_free(args->envp[i]);
		}
		bunix_free(args->envp);
	}
	args->argc = 0;
	args->envc = 0;
	args->bytes = 0;
	args->argv = 0;
	args->envp = 0;
}

static long linux_exec_vector_push(char ***values, u64 *count, u64 *capacity,
				   char *value)
{
	char **next;
	u64 new_capacity;

	if (values == 0 || count == 0 || capacity == 0 || value == 0 ||
	    *count >= LINUX_EXEC_MAX_POINTERS) {
		return -LINUX_E2BIG;
	}
	if (*count == *capacity) {
		new_capacity = *capacity == 0 ? 8 : *capacity * 2;
		if (new_capacity < *capacity ||
		    new_capacity > LINUX_EXEC_MAX_POINTERS) {
			new_capacity = LINUX_EXEC_MAX_POINTERS;
		}
		if (new_capacity == *capacity) {
			return -LINUX_E2BIG;
		}
		next = (char **)bunix_calloc(new_capacity, sizeof(next[0]));
		if (next == 0) {
			return -LINUX_ENOMEM;
		}
		if (*values != 0) {
			for (u64 i = 0; i < *count; i++) {
				next[i] = (*values)[i];
			}
			bunix_free(*values);
		}
		*values = next;
		*capacity = new_capacity;
	}
	(*values)[*count] = value;
	(*count)++;
	return 0;
}

static long linux_exec_buffer_read_u64(u64 buffer, u64 offset, u64 *value)
{
	return bunix_buffer_read(buffer, offset, value, sizeof(*value));
}

static long linux_exec_buffer_write_u64(u64 buffer, u64 offset, u64 value)
{
	return bunix_buffer_write(buffer, offset, &value, sizeof(value));
}

static long linux_exec_deserialize_one(u64 buffer, u64 limit, u64 *offset,
				       char **out)
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
	copy = (char *)bunix_alloc(len);
	if (copy == 0) {
		return -LINUX_ENOMEM;
	}
	if (bunix_buffer_read(buffer, *offset + sizeof(len), copy, len) != 0 ||
	    copy[len - 1] != '\0') {
		bunix_free(copy);
		return -LINUX_EINVAL;
	}
	*offset += sizeof(len) + len;
	*out = copy;
	return 0;
}

static long linux_exec_deserialize_args(u64 buffer, u64 offset, u64 len,
					struct linux_exec_args *args)
{
	const u64 limit = offset + len;
	u64 argc = 0;
	u64 envc = 0;
	u64 argv_capacity = 0;
	u64 env_capacity = 0;
	long result = 0;

	if (args == 0 || len < 2 * sizeof(u64) ||
	    limit < offset || limit > LINUX_EXEC_PREPARE_BUFFER_SIZE ||
	    linux_exec_buffer_read_u64(buffer, offset, &argc) != 0 ||
	    linux_exec_buffer_read_u64(buffer, offset + sizeof(u64),
				       &envc) != 0 ||
	    argc == 0 || argc > LINUX_EXEC_MAX_POINTERS ||
	    envc > LINUX_EXEC_MAX_POINTERS) {
		return -LINUX_EINVAL;
	}
	offset += 2 * sizeof(u64);
	for (u64 i = 0; i < argc; i++) {
		char *copy = 0;
		long push;

		result = linux_exec_deserialize_one(buffer, limit, &offset,
						    &copy);
		if (result != 0) {
			goto fail;
		}
		push = linux_exec_vector_push(&args->argv, &args->argc,
					      &argv_capacity, copy);
		if (push != 0) {
			bunix_free(copy);
			result = push;
			goto fail;
		}
		args->bytes += string_len(copy) + 1;
	}
	for (u64 i = 0; i < envc; i++) {
		char *copy = 0;
		long push;

		result = linux_exec_deserialize_one(buffer, limit, &offset,
						    &copy);
		if (result != 0) {
			goto fail;
		}
		push = linux_exec_vector_push(&args->envp, &args->envc,
					      &env_capacity, copy);
		if (push != 0) {
			bunix_free(copy);
			result = push;
			goto fail;
		}
		args->bytes += string_len(copy) + 1;
	}
	if (offset != limit || args->bytes > LINUX_EXEC_MAX_STRING_BYTES) {
		result = -LINUX_EINVAL;
		goto fail;
	}
	return 0;

fail:
	linux_exec_args_free(args);
	return result;
}

static long linux_exec_serialize_one(u64 buffer, u64 *offset,
				     const char *value, u64 *bytes)
{
	const u64 len = value == 0 ? 0 : string_len(value) + 1;

	if (len == 0 || len > LINUX_EXEC_MAX_STRING ||
	    *bytes + len < *bytes ||
	    *bytes + len > LINUX_EXEC_MAX_STRING_BYTES ||
	    *offset + sizeof(len) < *offset ||
	    *offset + sizeof(len) + len < *offset ||
	    *offset + sizeof(len) + len > LINUX_EXEC_PREPARE_BUFFER_SIZE) {
		return -LINUX_E2BIG;
	}
	if (linux_exec_buffer_write_u64(buffer, *offset, len) != 0 ||
	    bunix_buffer_write(buffer, *offset + sizeof(len), value,
			       len) != 0) {
		return -LINUX_EFAULT;
	}
	*offset += sizeof(len) + len;
	*bytes += len;
	return 0;
}

static long linux_exec_serialize_args(u64 buffer, u64 offset,
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
		const long result = linux_exec_serialize_one(buffer, &offset,
							    args->argv[i],
							    &bytes);

		if (result != 0) {
			return result;
		}
	}
	for (u64 i = 0; i < args->envc; i++) {
		const long result = linux_exec_serialize_one(buffer, &offset,
							    args->envp[i],
							    &bytes);

		if (result != 0) {
			return result;
		}
	}
	*len_out = offset - start;
	return 0;
}

static char *linux_exec_dup_text(const char *text)
{
	const u64 len = text == 0 ? 0 : string_len(text) + 1;
	char *copy;

	if (len == 0 || len > LINUX_EXEC_MAX_STRING) {
		return 0;
	}
	copy = (char *)bunix_alloc(len);
	if (copy == 0) {
		return 0;
	}
	for (u64 i = 0; i < len; i++) {
		copy[i] = text[i];
	}
	return copy;
}

static long linux_exec_parse_shebang(const unsigned char *image,
				     u64 image_size, char **interp_out,
				     char **arg_out)
{
	u64 end = 0;
	u64 pos = 2;
	u64 interp_start;
	u64 interp_end;
	u64 arg_start;
	u64 arg_end;
	char *interp;
	char *arg = 0;

	if (interp_out == 0 || arg_out == 0) {
		return -LINUX_EFAULT;
	}
	*interp_out = 0;
	*arg_out = 0;
	if (image == 0 || image_size < 2 || image[0] != '#' ||
	    image[1] != '!') {
		return -LINUX_EINVAL;
	}
	while (end < image_size && end < LINUX_SHEBANG_MAX &&
	       image[end] != '\n' && image[end] != '\r' &&
	       image[end] != '\0') {
		end++;
	}
	while (pos < end && (image[pos] == ' ' || image[pos] == '\t')) {
		pos++;
	}
	interp_start = pos;
	while (pos < end && image[pos] != ' ' && image[pos] != '\t') {
		pos++;
	}
	interp_end = pos;
	if (interp_start == interp_end ||
	    image[interp_start] != '/' ||
	    interp_end - interp_start >= LINUX_MAX_PATH) {
		return -LINUX_EINVAL;
	}
	interp = (char *)bunix_alloc(interp_end - interp_start + 1);
	if (interp == 0) {
		return -LINUX_ENOMEM;
	}
	for (u64 i = 0; i < interp_end - interp_start; i++) {
		interp[i] = (char)image[interp_start + i];
	}
	interp[interp_end - interp_start] = '\0';

	while (pos < end && (image[pos] == ' ' || image[pos] == '\t')) {
		pos++;
	}
	arg_start = pos;
	arg_end = end;
	while (arg_end > arg_start &&
	       (image[arg_end - 1] == ' ' || image[arg_end - 1] == '\t')) {
		arg_end--;
	}
	if (arg_end > arg_start) {
		arg = (char *)bunix_alloc(arg_end - arg_start + 1);
		if (arg == 0) {
			bunix_free(interp);
			return -LINUX_ENOMEM;
		}
		for (u64 i = 0; i < arg_end - arg_start; i++) {
			arg[i] = (char)image[arg_start + i];
		}
		arg[arg_end - arg_start] = '\0';
	}

	*interp_out = interp;
	*arg_out = arg;
	return 0;
}

static long linux_exec_rewrite_shebang_args(struct linux_exec_args *args,
					    const char *script_path,
					    char *interp, char *interp_arg)
{
	const u64 tail = args->argc > 1 ? args->argc - 1 : 0;
	const u64 prefix = interp_arg == 0 ? 2 : 3;
	const u64 new_argc = prefix + tail;
	char **next;
	char *script_copy;
	u64 new_bytes = 0;
	u64 index = 0;

	if (args == 0 || script_path == 0 || interp == 0 ||
	    new_argc > LINUX_EXEC_MAX_POINTERS) {
		return -LINUX_EINVAL;
	}
	next = (char **)bunix_calloc(new_argc, sizeof(next[0]));
	if (next == 0) {
		return -LINUX_ENOMEM;
	}
	script_copy = linux_exec_dup_text(script_path);
	if (script_copy == 0) {
		bunix_free(next);
		return -LINUX_ENOMEM;
	}
	new_bytes += string_len(interp) + 1;
	if (interp_arg != 0) {
		new_bytes += string_len(interp_arg) + 1;
	}
	new_bytes += string_len(script_copy) + 1;
	for (u64 i = 1; i < args->argc; i++) {
		new_bytes += string_len(args->argv[i]) + 1;
	}
	for (u64 i = 0; i < args->envc; i++) {
		new_bytes += string_len(args->envp[i]) + 1;
	}
	if (new_bytes > LINUX_EXEC_MAX_STRING_BYTES) {
		bunix_free(script_copy);
		bunix_free(next);
		return -LINUX_E2BIG;
	}

	next[index++] = interp;
	if (interp_arg != 0) {
		next[index++] = interp_arg;
	}
	next[index++] = script_copy;
	for (u64 i = 1; i < args->argc; i++) {
		next[index++] = args->argv[i];
		args->argv[i] = 0;
	}
	if (args->argc != 0) {
		bunix_free(args->argv[0]);
	}
	bunix_free(args->argv);
	args->argv = next;
	args->argc = new_argc;
	args->bytes = new_bytes;
	return 0;
}

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
	    path[0] != '/' ||
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
	return 0;
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
	}

	process->fds[0].handle = BUNIX_HANDLE_CONSOLE;
	process->fds[0].kind = LINUX_FD_CONSOLE;
	process->fds[1].handle = BUNIX_HANDLE_CONSOLE;
	process->fds[1].kind = LINUX_FD_CONSOLE;
	process->fds[2].handle = BUNIX_HANDLE_CONSOLE;
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
	} else if (fd->kind == LINUX_FD_PIPE_WRITE) {
		pipe->write_refs++;
	}
}

static void linux_pipe_ref_drop(const struct linux_fd *fd)
{
	struct linux_pipe *pipe = linux_pipe_find(fd->handle);

	if (pipe == 0) {
		return;
	}
	if (fd->kind == LINUX_FD_PIPE_READ && pipe->read_refs != 0) {
		pipe->read_refs--;
	} else if (fd->kind == LINUX_FD_PIPE_WRITE && pipe->write_refs != 0) {
		pipe->write_refs--;
	}
	if (pipe->read_refs == 0 && pipe->write_refs == 0) {
		if (pipe->reader_buffer != 0) {
			bunix_handle_close(pipe->reader_buffer);
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

	if ((flags & ~LINUX_O_CLOEXEC) != 0 ||
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

	left = alloc_fd(process, LINUX_FD_PIPE_READ, pipe->id, 0);
	if (left < 0) {
		(void)bunix_id_remove(&pipe_ids, pipe->id);
		bunix_free(pipe);
		return left;
	}
	right = alloc_fd(process, LINUX_FD_PIPE_WRITE, pipe->id, 0);
	if (right < 0) {
		process->fds[left].kind = 0;
		(void)bunix_id_remove(&pipe_ids, pipe->id);
		bunix_free(pipe);
		return right;
	}
	if ((flags & LINUX_O_CLOEXEC) != 0) {
		process->fds[left].flags |= LINUX_FD_CLOEXEC;
		process->fds[right].flags |= LINUX_FD_CLOEXEC;
	}
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
	return (long)nread;
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
	reply.words[0] = (u64)nread;
	(void)bunix_ipc_send(reply_handle, &reply);
	if (buffer != 0) {
		bunix_handle_close(buffer);
	}
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
		return -LINUX_EINVAL;
	}
	if (requested_pid != 0 && linux_process_find_pid(requested_pid) != 0) {
		return -LINUX_EINVAL;
	}

	struct linux_process *process = (struct linux_process *)
		bunix_calloc(1, sizeof(*process));
	if (process == 0) {
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
	process->session_id = session_id;
	if (linux_process_init_fds(process) != 0) {
		bunix_free(process);
		return -LINUX_ESRCH;
	}
	if (linux_process_set_cwd(process, "/") != 0) {
		bunix_free(process->fds);
		bunix_free(process);
		return -LINUX_ESRCH;
	}
	if (foreground_pgid == 0 || pid == 1) {
		foreground_pgid = pid;
	}
	if (session_id != 0) {
		(void)linux_user_session_set_foreground(session_id,
							process->pgid);
	}
	if (linux_user_process_register(bunix_task) != 0) {
		linux_process_reset(process);
		return -LINUX_ESRCH;
	}
	if (bunix_map_set(&process_by_task, bunix_task, (u64)process) != 0 ||
	    bunix_map_set(&process_by_pid, pid, (u64)process) != 0) {
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
		return -LINUX_EINVAL;
	}

	struct linux_process *process = (struct linux_process *)
		bunix_calloc(1, sizeof(*process));
	if (process == 0) {
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
		linux_process_reset(process);
		return -LINUX_ESRCH;
	}
	for (u64 fd = 0; fd < process->fd_capacity; fd++) {
		process->fds[fd] = parent->fds[fd];
		if (process->fds[fd].kind == LINUX_FD_FILE) {
			linux_file_ref_add(process->fds[fd].handle);
		} else if (process->fds[fd].kind == LINUX_FD_DIR) {
			linux_file_ref_add(process->fds[fd].handle);
		} else if (process->fds[fd].kind == LINUX_FD_PIPE_READ ||
			   process->fds[fd].kind == LINUX_FD_PIPE_WRITE) {
			linux_pipe_ref_add(&process->fds[fd]);
		}
	}
	if (linux_process_set_cwd(process, parent->cwd) != 0) {
		linux_process_reset(process);
		return -LINUX_ESRCH;
	}
	if (linux_user_process_fork(parent_task, child_task) != 0) {
		linux_process_reset(process);
		return -LINUX_ESRCH;
	}
	if (bunix_map_set(&process_by_task, child_task, (u64)process) != 0 ||
	    bunix_map_set(&process_by_pid, pid, (u64)process) != 0) {
		linux_process_reset(process);
		return -LINUX_ESRCH;
	}
	notify_proc_register_linux(pid, child_task, parent->pid);
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
				return -LINUX_EINVAL;
			}
			candidate->waited = 1;
			linux_process_reset(candidate);
			return (long)waited_pid;
		}
		child = next;
	}

	if (candidate == 0) {
		return -LINUX_ECHILD;
	}

	if ((options & LINUX_WNOHANG) != 0) {
		return 0;
	}

	if (reply == 0) {
		return -LINUX_EINVAL;
	}

	parent->waiter = reply;
	parent->wait_buffer = status_buffer;
	parent->wait_pid = (u64)pid;
	return LINUX_WAIT_BLOCK;
}

static void linux_wake_parent(struct linux_process *child)
{
	struct linux_process *parent = linux_process_find_pid(child->ppid);
	const char wait4_ok[] = "linux-server: wait4\n";

	if (parent == 0 || parent->waiter == 0 ||
	    !linux_child_matches(parent, child, (long)parent->wait_pid)) {
		return;
	}

	struct bunix_msg reply = {
		.protocol = BUNIX_PROTO_LINUX,
		.type = BUNIX_LINUX_WAIT4,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { child->pid, 0, 0, 0 },
	};

	if (linux_store_wait_status(parent->wait_buffer,
				    child->exit_status) == 0) {
		child->waited = 1;
		bunix_console_log(wait4_ok, sizeof(wait4_ok) - 1);
		bunix_ipc_send(parent->waiter, &reply);
		linux_process_reset(child);
	}
	if (parent->wait_buffer != 0) {
		bunix_handle_close(parent->wait_buffer);
	}
	parent->waiter = 0;
	parent->wait_buffer = 0;
	parent->wait_pid = 0;
}

static void linux_process_exit_status(struct linux_process *process, u64 status,
				      u64 kill_task)
{
	if (process == 0 || process->exited) {
		return;
	}

	tty_cancel_reader(process);
	linux_close_process_fds(process);
	linux_reparent_children(process);
	process->exited = 1;
	process->exit_status = status;
	notify_proc_exit(process->pid, process->exit_status, kill_task);
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
	if ((process->signal_ignored & bit) != 0) {
		return 1;
	}
	process->pending_signals |= bit;
	if ((process->signal_mask & bit) != 0) {
		return 1;
	}
	if (signal == LINUX_SIGINT || signal == LINUX_SIGTERM) {
		linux_process_exit_status(process, signal, 1);
		return 1;
	}

	return 0;
}

static long linux_rt_sigaction(struct linux_process *process, u64 signal,
			       u64 handler, u64 *old_handler)
{
	const u64 bit = linux_signal_bit(signal);

	if (process == 0 || bit == 0 || signal == 9 || signal == 19) {
		return -LINUX_EINVAL;
	}
	if (old_handler != 0) {
		*old_handler = (process->signal_ignored & bit) != 0 ? 1 : 0;
	}
	if (handler != ~0ull) {
		if (handler == 1) {
			process->signal_ignored |= bit;
		} else {
			process->signal_ignored &= ~bit;
		}
	}
	return 0;
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
	u64 foreground = 0;

	if (process != 0 &&
	    process->session_id != 0 &&
	    linux_user_session_get(process->session_id, 0, 0, 0,
				   &foreground) == 0 &&
	    foreground != 0) {
		return foreground;
	}
	if (process != 0) {
		return process->pgid;
	}
	return foreground_pgid;
}

static long linux_set_foreground_pgid(struct linux_process *process, u64 pgid)
{
	if (pgid == 0) {
		return -LINUX_EINVAL;
	}
	foreground_pgid = pgid;
	if (process != 0 && process->session_id != 0) {
		return linux_user_session_set_foreground(process->session_id,
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

	if (process == 0 || linux_user_service() == 0) {
		return -LINUX_ESRCH;
	}

	request.words[0] = process->bunix_task;
	if (bunix_ipc_call(user_service, &request, &reply) != 0) {
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

	if (process == 0 || linux_user_service() == 0) {
		return -LINUX_ESRCH;
	}

	request.words[0] = process->bunix_task;
	if (bunix_ipc_call(user_service, &request, &reply) != 0) {
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

	if (process == 0 || linux_user_service() == 0) {
		return -LINUX_ESRCH;
	}

	request.words[0] = process->bunix_task;
	if (bunix_ipc_call(user_service, &request, &reply) != 0) {
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
	if (process == 0 || linux_user_service() == 0) {
		return -LINUX_ESRCH;
	}

	request.words[0] = process->bunix_task;
	if (bunix_ipc_call(user_service, &request, &reply) != 0) {
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

	if (process == 0 || linux_user_service() == 0) {
		return -LINUX_ESRCH;
	}

	request.words[0] = process->bunix_task;
	if (bunix_ipc_call(user_service, &request, &reply) != 0) {
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
	(void)linux_set_foreground_pgid(process, process->pgid);
	return (long)process->sid;
}

static void tty_init(void)
{
	if (tty_termios[LINUX_TERM_CC + LINUX_VMIN] != 0) {
		return;
	}

	zero_bytes(tty_termios, sizeof(tty_termios));
	store_u32(tty_termios, LINUX_TERM_IFLAG, LINUX_ICRNL);
	store_u32(tty_termios, LINUX_TERM_OFLAG, LINUX_OPOST | LINUX_ONLCR);
	store_u32(tty_termios, LINUX_TERM_CFLAG, LINUX_CS8 | LINUX_CREAD);
	store_u32(tty_termios, LINUX_TERM_LFLAG,
		  LINUX_ISIG | LINUX_ICANON | LINUX_ECHO |
		  LINUX_ECHOE | LINUX_ECHOK);
	tty_termios[LINUX_TERM_LINE] = 0;
	tty_termios[LINUX_TERM_CC + LINUX_VINTR] = 3;
	tty_termios[LINUX_TERM_CC + LINUX_VERASE] = 0x7f;
	tty_termios[LINUX_TERM_CC + LINUX_VEOF] = 4;
	tty_termios[LINUX_TERM_CC + LINUX_VMIN] = 1;
	tty_termios[LINUX_TERM_CC + LINUX_VTIME] = 0;
}

static unsigned int tty_lflag(void)
{
	tty_init();
	return load_u32(tty_termios, LINUX_TERM_LFLAG);
}

static unsigned int tty_iflag(void)
{
	tty_init();
	return load_u32(tty_termios, LINUX_TERM_IFLAG);
}

static void tty_echo(const char *text, u64 len)
{
	(void)bunix_console_write(text, len);
}

static int tty_read_queue_push(char c)
{
	if (tty_read_queue_len >= sizeof(tty_read_queue)) {
		return 0;
	}
	tty_read_queue[(tty_read_queue_start + tty_read_queue_len) %
		       sizeof(tty_read_queue)] = c;
	tty_read_queue_len++;
	return 1;
}

static long tty_read_available(u64 len, u64 buffer)
{
	u64 nread;

	if (buffer == 0) {
		return -LINUX_EBADF;
	}
	if (tty_read_queue_len == 0) {
		return -LINUX_EAGAIN;
	}
	nread = len < tty_read_queue_len ? len : tty_read_queue_len;
	for (u64 i = 0; i < nread; i++) {
		write_buffer[i] = tty_read_queue[(tty_read_queue_start + i) %
						 sizeof(tty_read_queue)];
	}
	tty_read_queue_start =
		(tty_read_queue_start + nread) % sizeof(tty_read_queue);
	tty_read_queue_len -= nread;
	if (bunix_buffer_write(buffer, 0, write_buffer, nread) != 0) {
		return -LINUX_EFAULT;
	}
	return (long)nread;
}

static void tty_wake_reader(void)
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

	if (tty_reader_reply == 0 || tty_read_queue_len == 0) {
		return;
	}

	reply_handle = tty_reader_reply;
	buffer = tty_reader_buffer;
	tty_reader_pid = 0;
	tty_reader_reply = 0;
	tty_reader_buffer = 0;
	nread = tty_read_available(tty_reader_len, buffer);
	tty_reader_len = 0;
	reply.words[0] = (u64)nread;
	(void)bunix_ipc_send(reply_handle, &reply);
	if (buffer != 0) {
		bunix_handle_close(buffer);
	}
}

static void tty_cancel_reader(struct linux_process *process)
{
	if (process == 0 || tty_reader_pid != process->pid) {
		return;
	}
	if (tty_reader_buffer != 0) {
		bunix_handle_close(tty_reader_buffer);
	}
	tty_reader_pid = 0;
	tty_reader_reply = 0;
	tty_reader_buffer = 0;
	tty_reader_len = 0;
}

static long tty_termios_get(u64 buffer)
{
	tty_init();
	return bunix_buffer_write(buffer, 0, tty_termios,
				  sizeof(tty_termios)) == 0 ?
	       0 : -LINUX_EFAULT;
}

static long tty_termios_set(u64 buffer)
{
	if (buffer == 0 ||
	    bunix_buffer_read(buffer, 0, tty_termios,
			      sizeof(tty_termios)) != 0) {
		return -LINUX_EFAULT;
	}
	tty_line_len = 0;
	if (tty_termios[LINUX_TERM_CC + LINUX_VMIN] == 0) {
		tty_termios[LINUX_TERM_CC + LINUX_VMIN] = 1;
	}
	return 0;
}

static void tty_interrupt_foreground(void)
{
	struct linux_process *reader =
		tty_reader_pid != 0 ? linux_process_find_pid(tty_reader_pid) : 0;

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

	if (foreground_pgid != 0) {
		(void)linux_signal_pgrp(0, foreground_pgid, LINUX_SIGINT);
	}
}

static void tty_input_event(char c)
{
	const unsigned int lflag = tty_lflag();
	const unsigned int iflag = tty_iflag();
	const char intr = tty_termios[LINUX_TERM_CC + LINUX_VINTR];

	if (c == '\r' && (iflag & LINUX_ICRNL) != 0) {
		c = '\n';
	}

	if ((lflag & LINUX_ISIG) != 0 && c == intr) {
		if ((lflag & LINUX_ECHO) != 0) {
			tty_echo("^C\n", 3);
		}
		tty_line_len = 0;
		tty_interrupt_foreground();
		return;
	}

	if ((lflag & LINUX_ICANON) == 0) {
		if ((lflag & LINUX_ECHO) != 0) {
			tty_echo(&c, 1);
		}
		(void)tty_read_queue_push(c);
		tty_wake_reader();
		return;
	}

	if ((c == tty_termios[LINUX_TERM_CC + LINUX_VERASE] || c == '\b') &&
	    tty_line_len != 0) {
		tty_line_len--;
		if ((lflag & LINUX_ECHO) != 0) {
			tty_echo("\b \b", 3);
		}
		return;
	}
	if ((unsigned char)c < 0x20 && c != '\n' &&
	    c != tty_termios[LINUX_TERM_CC + LINUX_VEOF]) {
		return;
	}
	if (c != tty_termios[LINUX_TERM_CC + LINUX_VEOF]) {
		if (tty_line_len < sizeof(tty_line)) {
			tty_line[tty_line_len++] = c;
		}
		if ((lflag & LINUX_ECHO) != 0) {
			tty_echo(&c, 1);
		}
	}
	if (c != '\n' && c != tty_termios[LINUX_TERM_CC + LINUX_VEOF] &&
	    tty_line_len < sizeof(tty_line)) {
		return;
	}

	for (u64 i = 0; i < tty_line_len; i++) {
		if (!tty_read_queue_push(tty_line[i])) {
			break;
		}
	}
	tty_line_len = 0;
	tty_wake_reader();
}

static long tty_read(struct linux_process *process, u64 len, u64 buffer,
		     u64 reply_handle, int *blocked)
{
	long nread;

	if (len == 0) {
		return 0;
	}
	nread = tty_read_available(len, buffer);
	if (nread != -(long)LINUX_EAGAIN) {
		return nread;
	}
	if (reply_handle == 0 || tty_reader_reply != 0) {
		return -LINUX_EAGAIN;
	}
	tty_reader_pid = process->pid;
	tty_reader_reply = reply_handle;
	tty_reader_buffer = buffer;
	tty_reader_len = len;
	if (blocked != 0) {
		*blocked = 1;
	}
	return 0;
}

static long linux_ioctl(struct linux_process *process, u64 fd, u64 request,
			u64 value, u64 buffer)
{
	char data[8];
	u64 pgid;

	if (fd >= process->fd_capacity ||
	    process->fds[fd].kind != LINUX_FD_CONSOLE) {
		return -LINUX_EBADF;
	}

	switch (request) {
	case LINUX_TCGETS:
		return tty_termios_get(buffer);
	case LINUX_TCSETS:
	case LINUX_TCSETSW:
	case LINUX_TCSETSF:
		return tty_termios_set(buffer);
	case LINUX_TIOCGPGRP:
		if (buffer == 0) {
			return -LINUX_EFAULT;
		}
		zero_bytes(data, sizeof(data));
		store_u32(data, 0,
			  (unsigned int)linux_foreground_pgid(process));
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
	return linux_vfs_path_call_flags(process, type, base_handle, path, 0,
					 reply);
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

static long linux_openat(struct linux_process *process, u64 dirfd,
			 u64 path_len, u64 flags, u64 mode, u64 path_buffer)
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
		return normalize_result;
	}
	base_result = linux_dirfd_base_handle(process, dirfd, path,
					      &base_handle);
	if (base_result != 0) {
		return base_result;
	}
	if ((flags & LINUX_O_NOFOLLOW) != 0) {
		struct bunix_msg meta;

		if (linux_vfs_path_call_flags(
			    process, BUNIX_VFS_STAT_PATH_META_BUFFER,
			    base_handle, path, 1, &meta) == 0 &&
		    meta.words[0] == 0 &&
		    (meta.words[2] >> 32) == BUNIX_VFS_TYPE_SYMLINK) {
			return -LINUX_ELOOP;
		}
	}

	if (linux_vfs_path_call(process, BUNIX_VFS_OPEN_BUFFER, base_handle,
				path, &reply) != 0) {
		return -LINUX_ENOENT;
	}
	if (reply.words[0] == 0 &&
	    (flags & (LINUX_O_CREAT | LINUX_O_EXCL)) ==
		    (LINUX_O_CREAT | LINUX_O_EXCL)) {
		struct bunix_msg close = {
			.protocol = BUNIX_PROTO_VFS,
			.type = BUNIX_VFS_CLOSE,
			.sender = 0,
			.cap_rights = 0,
			.reply = 0,
			.cap = 0,
			.words = { reply.words[1], 0, 0, 0 },
		};
		struct bunix_msg ignored;

		(void)bunix_ipc_call(LINUX_HANDLE_VFS, &close, &ignored);
		return -LINUX_EEXIST;
	}
	if (reply.words[0] == BUNIX_VFS_ERR_NOENT &&
	    (flags & LINUX_O_CREAT) != 0) {
		if (linux_vfs_path_call_word3(process, BUNIX_VFS_CREATE_BUFFER,
					      base_handle, path,
					      process->bunix_task |
					      (((mode & ~process->umask) & 0777) << 32),
					      &reply) != 0) {
			return -LINUX_EIO;
		}
		if (reply.words[0] != 0) {
			return linux_vfs_error(reply.words[0]);
		}
		if (linux_vfs_path_call(process, BUNIX_VFS_OPEN_BUFFER,
					base_handle, path, &reply) != 0) {
			return -LINUX_EIO;
		}
	}
	if (reply.words[0] != 0) {
		return linux_vfs_error(reply.words[0]);
	}
	if ((flags & LINUX_O_DIRECTORY) != 0 &&
	    reply.words[3] != BUNIX_VFS_TYPE_DIRECTORY) {
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
	if (linux_console_path(full_path)) {
		const u64 remote_handle = reply.words[1];
		const long fd = alloc_fd(process, LINUX_FD_CONSOLE,
					 BUNIX_HANDLE_CONSOLE, 0);

		(void)linux_close_vfs_handle(remote_handle);
		return fd;
	}

	const long fd = alloc_fd(process,
				 reply.words[3] == BUNIX_VFS_TYPE_DIRECTORY ?
				 LINUX_FD_DIR : LINUX_FD_FILE,
				 reply.words[1], reply.words[2]);
	if (fd < 0) {
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
		return fd;
	}
	linux_file_ref_add(reply.words[1]);
	if ((flags & LINUX_O_CLOEXEC) != 0) {
		process->fds[fd].flags |= LINUX_FD_CLOEXEC;
	}
	process->fds[fd].status_flags = flags &
		(LINUX_O_ACCMODE | LINUX_O_APPEND);
	if ((flags & LINUX_O_TRUNC) != 0 &&
	    (flags & LINUX_O_ACCMODE) != 0 &&
	    process->fds[fd].kind == LINUX_FD_FILE &&
	    (base_result = linux_ftruncate(process, (u64)fd, 0)) != 0) {
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
	struct bunix_msg reply;
	u64 base_handle;
	long base_result;
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
	base_result = linux_dirfd_base_handle(process, dirfd, path,
					      &base_handle);
	if (base_result != 0) {
		return base_result;
	}

	if (linux_vfs_path_call_flags(process, BUNIX_VFS_ACCESS_BUFFER,
				      base_handle, path, mode,
				      &reply) != 0) {
		return -LINUX_EIO;
	}
	if (reply.words[0] != 0) {
		return linux_vfs_error(reply.words[0]);
	}
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
	normalize_result = path_normalize(process->cwd, path, full_path);
	if (normalize_result != 0) {
		return normalize_result;
	}
	base_result = linux_dirfd_base_handle(process, dirfd, path,
					      &base_handle);
	if (base_result != 0) {
		return base_result;
	}
	if (linux_vfs_path_call_word3(process, type, base_handle, path,
				      process->bunix_task | (flags << 32),
				      &reply) != 0) {
		return -LINUX_EIO;
	}
	if (reply.words[0] != 0) {
		return linux_vfs_error(reply.words[0]);
	}
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
		return normalize_result;
	}
	base_result = linux_dirfd_base_handle(process, LINUX_AT_FDCWD, path,
					      &base_handle);
	if (base_result != 0) {
		return base_result;
	}
	if (linux_vfs_path_call_word3(process, BUNIX_VFS_RMDIR_BUFFER,
				      base_handle, path,
				      process->bunix_task, &reply) != 0) {
		return -LINUX_EIO;
	}
	if (reply.words[0] != 0) {
		return linux_vfs_error(reply.words[0]);
	}
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
		return normalize_result;
	}
	base_result = linux_dirfd_base_handle(process, dirfd, path,
					      &base_handle);
	if (base_result != 0) {
		return base_result;
	}
	if (linux_vfs_path_call_word3(process, BUNIX_VFS_MKDIR_BUFFER,
				      base_handle, path,
				      process->bunix_task |
				      (((mode & ~process->umask) & 0777) << 32),
				      &reply) != 0) {
		return -LINUX_EIO;
	}
	if (reply.words[0] != 0) {
		return linux_vfs_error(reply.words[0]);
	}
	return 0;
}

static long linux_mknodat(struct linux_process *process, u64 dirfd,
			  u64 path_len, u64 mode, u64 dev, u64 path_buffer)
{
	char path[LINUX_MAX_PATH];
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
	return 0;
}

static long linux_symlinkat(struct linux_process *process, u64 dirfd,
			    u64 target_len, u64 path_len, u64 buffer)
{
	char target[LINUX_MAX_PATH];
	char path[LINUX_MAX_PATH];
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
	name_result = linux_check_name_max(path);
	if (name_result != 0) {
		return name_result;
	}
	base_result = linux_dirfd_base_handle(process, dirfd, path,
					      &base_handle);
	if (base_result != 0) {
		return base_result;
	}
	if (linux_vfs_symlink_call(process, base_handle, target, path,
				   &reply) != 0) {
		return -LINUX_EIO;
	}
	if (reply.words[0] != 0) {
		return linux_vfs_error(reply.words[0]);
	}
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
	long base_result;
	long name_result;

	if (flags != 0) {
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
	if (linux_vfs_two_path_call(process, BUNIX_VFS_LINK_BUFFER, old_base,
				    old_path, new_base, new_path, flags,
				    &reply) != 0) {
		return -LINUX_EIO;
	}
	if (reply.words[0] != 0) {
		return linux_vfs_error(reply.words[0]);
	}
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
				      u64 blocks)
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

	return bunix_buffer_write(stat_buffer, 0, stat, sizeof(stat)) == 0 ?
		0 : -LINUX_EFAULT;
}

static long linux_stat_write(u64 stat_buffer, u64 mode, u64 uid, u64 gid,
			     u64 size)
{
	return linux_stat_write_identity(stat_buffer, mode, uid, gid, size,
					 1, 1, 1, 0, 4096,
					 (size + 511) / 512);
}

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
};

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
					 stat->blksize, stat->blocks);
}

static long linux_statx_write(u64 statx_buffer, u64 mode, u64 uid, u64 gid,
			      u64 size, u64 dev, u64 ino, u64 nlink,
			      u64 rdev, u64 blksize, u64 blocks)
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
	store_u32(statx, 64, (unsigned int)(rdev >> 32));
	store_u32(statx, 68, (unsigned int)rdev);
	store_u32(statx, 72, (unsigned int)(dev >> 32));
	store_u32(statx, 76, (unsigned int)dev);

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
				 stat->blksize, stat->blocks);
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
	return reply.words[0] == 0 ? 0 : -LINUX_EBUSY;
}

static long linux_vfs_unmount(const char *target)
{
	struct bunix_msg reply;

	if (bunix_ipc_call_path(LINUX_HANDLE_VFS, BUNIX_PROTO_VFS,
				BUNIX_VFS_UNMOUNT_BUFFER, target, 0, 0, 0,
				&reply) != 0) {
		return -LINUX_ENOSYS;
	}
	return reply.words[0] == 0 ? 0 : linux_vfs_error(reply.words[0]);
}

static long linux_mount(struct linux_process *process, u64 target_len,
			u64 fstype_len, u64 flags, u64 buffer)
{
	char target[LINUX_MAX_PATH];
	char full_target[LINUX_MAX_PATH];
	char fstype[64];
	long normalize_result;

	(void)flags;
	if (target_len == 0 || target_len > sizeof(target) ||
	    fstype_len == 0 || fstype_len > sizeof(fstype)) {
		return -LINUX_EINVAL;
	}
	if (buffer == 0 ||
	    bunix_buffer_read(buffer, 0, target, target_len) != 0 ||
	    bunix_buffer_read(buffer, target_len, fstype, fstype_len) != 0) {
		return -LINUX_EFAULT;
	}
	if (target[target_len - 1] != '\0' ||
	    fstype[fstype_len - 1] != '\0') {
		return -LINUX_EINVAL;
	}
	if (target[0] == '\0' || fstype[0] == '\0') {
		return -LINUX_EINVAL;
	}
	normalize_result = path_normalize(process->cwd, target, full_target);
	if (normalize_result != 0) {
		return normalize_result;
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

	(void)flags;
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
	process->fds[fd].size = length;
	if (process->fds[fd].offset > length) {
		process->fds[fd].offset = length;
	}
	return 0;
}

static long linux_newfstatat(struct linux_process *process, u64 dirfd,
			     u64 path_len, u64 path_buffer,
			     u64 flags, struct linux_vfs_stat *out)
{
	char path[LINUX_MAX_PATH];
	u64 base_handle;
	long base_result;
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
	base_result = linux_dirfd_base_handle(process, dirfd, path,
					      &base_handle);
	if (base_result != 0) {
		return base_result;
	}

	return linux_vfs_path_stat_call(
		process, base_handle, path,
		(flags & LINUX_AT_SYMLINK_NOFOLLOW) != 0 ? 1 : 0, out);
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
	struct linux_vfs_stat meta;

	if ((flags & ~LINUX_AT_SYMLINK_NOFOLLOW) != 0) {
		return -LINUX_EINVAL;
	}
	return linux_newfstatat(process, dirfd, path_len, path_buffer,
				flags & LINUX_AT_SYMLINK_NOFOLLOW ? 1 : 0,
				&meta);
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
	base_result = linux_dirfd_base_handle(process, dirfd, path,
					      &base_handle);
	if (base_result != 0) {
		return base_result;
	}
	if (linux_vfs_readlink_call(process, base_handle, path, out_size,
				    path_buffer, &copy_len) != 0) {
		return -LINUX_ENOENT;
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

	while (written < out_len) {
		char name[LINUX_MAX_PATH];
		u64 type = BUNIX_VFS_TYPE_DIRECTORY;
		u64 next_offset = process->fds[fd].offset;
		u64 reclen;

		for (u64 i = 0; i < sizeof(name); i++) {
			name[i] = '\0';
		}

		if (process->fds[fd].offset == 0) {
			name[0] = '.';
			name[1] = '\0';
			type = BUNIX_VFS_TYPE_DIRECTORY;
			next_offset = 1;
		} else if (process->fds[fd].offset == 1) {
			name[0] = '.';
			name[1] = '.';
			name[2] = '\0';
			type = BUNIX_VFS_TYPE_DIRECTORY;
			next_offset = 2;
		} else {
			const long result =
				linux_readdir_name(process->fds[fd].handle,
						   process->fds[fd].offset - 2,
						   (u64)name_buffer,
						   name, sizeof(name),
						   &type, &next_offset);

			if (result == -(long)LINUX_ENOENT) {
				break;
			}
			if (result != 0) {
				bunix_handle_close((u64)name_buffer);
				bunix_free(out);
				return result;
			}
		}

		reclen = linux_dirent_reclen(name);
		if (written + reclen > out_len) {
			break;
		}
		for (u64 i = 0; i < reclen; i++) {
			out[written + i] = 0;
		}
		linux_store_dirent(out, written, process->fds[fd].offset + 1,
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
		process->fds[fd].offset = next_offset;
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

static long linux_socket(struct linux_process *process, u64 domain, u64 type,
			 u64 protocol)
{
	const u64 base_type = type & ~(LINUX_SOCK_NONBLOCK |
				       LINUX_SOCK_CLOEXEC);
	long fd;

	if (domain != LINUX_AF_UNIX) {
		return -LINUX_EAFNOSUPPORT;
	}
	if (base_type != LINUX_SOCK_STREAM || protocol != 0) {
		return -LINUX_EPROTONOSUPPORT;
	}

	fd = alloc_fd(process, LINUX_FD_SOCKET, LINUX_SOCKET_UNIX_STREAM, 0);
	if (fd >= 0 && (type & LINUX_SOCK_CLOEXEC) != 0) {
		process->fds[fd].flags |= LINUX_FD_CLOEXEC;
	}
	return fd;
}

static long linux_connect(struct linux_process *process, u64 fd, u64 addr_len,
			  u64 addr_buffer)
{
	char addr[128];
	char path[LINUX_MAX_PATH];
	u64 path_len = 0;
	unsigned int family;

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
	process->fds[fd].handle = LINUX_SOCKET_UTMPD;
	process->fds[fd].offset = 0;
	process->fds[fd].size = LINUX_UTMPS_NONE;
	return 0;
}

static long linux_sendto(struct linux_process *process, u64 fd, u64 len,
			 u64 buffer)
{
	char command;

	if (fd >= process->fd_capacity ||
	    process->fds[fd].kind != LINUX_FD_SOCKET) {
		return -LINUX_ENOTSOCK;
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
			   u64 buffer)
{
	if (fd >= process->fd_capacity ||
	    process->fds[fd].kind != LINUX_FD_SOCKET) {
		return -LINUX_ENOTSOCK;
	}
	return utmps_recv_response(&process->fds[fd], len, buffer);
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
		if ((events & LINUX_POLLIN) != 0 && tty_read_queue_len != 0) {
			revents |= LINUX_POLLIN;
		}
		if ((events & LINUX_POLLOUT) != 0) {
			revents |= LINUX_POLLOUT;
		}
		break;
	case LINUX_FD_FILE:
	case LINUX_FD_DIR:
	case LINUX_FD_SOCKET:
		if ((events & LINUX_POLLIN) != 0) {
			revents |= LINUX_POLLIN;
		}
		if ((events & LINUX_POLLOUT) != 0) {
			revents |= LINUX_POLLOUT;
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

	if (process->fds[fd].kind == LINUX_FD_CONSOLE) {
		return tty_read(process, len, buffer, reply_handle, blocked);
	}
	if (process->fds[fd].kind == LINUX_FD_PIPE_READ) {
		struct linux_pipe *pipe = linux_pipe_find(process->fds[fd].handle);

		if (pipe == 0) {
			return -LINUX_EBADF;
		}
		if (pipe->len == 0) {
			if (pipe->write_refs == 0) {
				return 0;
			}
			if (reply_handle == 0 || pipe->reader_reply != 0) {
				return -LINUX_EAGAIN;
			}
			pipe->reader_reply = reply_handle;
			pipe->reader_buffer = buffer;
			pipe->reader_len = len;
			if (blocked != 0) {
				*blocked = 1;
			}
			return 0;
		}
		return linux_pipe_read_available(pipe, len, buffer);
	}
	if (process->fds[fd].kind == LINUX_FD_DIR) {
		return -LINUX_EISDIR;
	}
	if (process->fds[fd].kind == LINUX_FD_FILE &&
	    process->fds[fd].offset >= process->fds[fd].size) {
		return 0;
	}

	request.words[0] = process->fds[fd].handle;
	request.words[1] = process->fds[fd].offset;
	if (bunix_ipc_call(LINUX_HANDLE_VFS, &request, &reply) != 0) {
		return -LINUX_EIO;
	}
	if (reply.words[0] != 0) {
		return linux_vfs_error(reply.words[0]);
	}

	process->fds[fd].offset += reply.words[1];
	return (long)reply.words[1];
}

static long linux_mmap_read(struct linux_process *process, u64 fd, u64 offset,
			    u64 len, u64 buffer)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_READ_FILE_BUFFER,
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
	if (fd >= process->fd_capacity ||
	    process->fds[fd].kind != LINUX_FD_FILE) {
		return -LINUX_EBADF;
	}
	if (offset >= process->fds[fd].size) {
		return 0;
	}
	if (len > process->fds[fd].size - offset) {
		len = process->fds[fd].size - offset;
	}

	request.words[0] = process->fds[fd].handle;
	request.words[2] = len;
	if (bunix_ipc_call(LINUX_HANDLE_VFS, &request, &reply) != 0) {
		return -LINUX_EIO;
	}
	if (reply.words[0] != 0) {
		return linux_vfs_error(reply.words[0]);
	}
	return (long)reply.words[1];
}

static long linux_write_buffer(struct linux_process *process, u64 fd, u64 len,
			       u64 buffer)
{
	if (fd >= process->fd_capacity || process->fds[fd].kind == 0) {
		return -LINUX_EBADF;
	}
	if (buffer == 0) {
		return -LINUX_EFAULT;
	}

	if (process->fds[fd].kind == LINUX_FD_PIPE_WRITE) {
		struct linux_pipe *pipe = linux_pipe_find(process->fds[fd].handle);
		u64 copy_len;
		u64 space;
		u64 nwritten;

		if (pipe == 0) {
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
		linux_pipe_wake_reader(pipe);
		return (long)nwritten;
	}
	if (process->fds[fd].kind == LINUX_FD_FILE) {
		u64 done = 0;
		u64 offset = (process->fds[fd].status_flags & LINUX_O_APPEND) != 0 ?
			     process->fds[fd].size : process->fds[fd].offset;

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
				return done != 0 ? (long)done : -(long)LINUX_EBADF;
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
		process->fds[fd].offset = offset + done;
		if (process->fds[fd].offset > process->fds[fd].size) {
			process->fds[fd].size = process->fds[fd].offset;
		}
		return (long)done;
	}
	if (process->fds[fd].kind != LINUX_FD_CONSOLE) {
		return -LINUX_EBADF;
	}

	(void)process;
	for (u64 done = 0; done < len;) {
		const u64 chunk = len - done > sizeof(write_buffer) ?
				  sizeof(write_buffer) : len - done;

		if (bunix_buffer_read(buffer, done, write_buffer, chunk) != 0) {
			return done != 0 ? (long)done : -(long)LINUX_EFAULT;
		}
		bunix_console_write((const char *)write_buffer, chunk);
		done += chunk;
	}
	return (long)len;
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
	if (positioned) {
		saved_offset = process->fds[in_fd].offset;
		process->fds[in_fd].offset = offset;
	}

	nread = linux_read(process, in_fd, len, buffer, 0, 0);
	if (positioned) {
		if (next_offset != 0 && nread > 0) {
			*next_offset = process->fds[in_fd].offset;
		}
		process->fds[in_fd].offset = saved_offset;
	}
	if (nread <= 0) {
		return nread;
	}
	return linux_write_buffer(process, out_fd, (u64)nread, buffer);
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
		base = process->fds[fd].offset;
	} else if (whence == LINUX_SEEK_END) {
		base = process->fds[fd].size;
	} else {
		return -LINUX_EINVAL;
	}
	if ((offset >> 63) != 0 || base + offset < base) {
		return -LINUX_EINVAL;
	}
	next = base + offset;
	process->fds[fd].offset = next;
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
		if (linux_close_vfs_handle(process->fds[fd].handle) != 0) {
			return -LINUX_EIO;
		}
	}
	if (process->fds[fd].kind == LINUX_FD_PIPE_READ ||
	    process->fds[fd].kind == LINUX_FD_PIPE_WRITE) {
		const u64 pipe_id = process->fds[fd].handle;

		linux_pipe_ref_drop(&process->fds[fd]);
		linux_pipe_wake_reader(linux_pipe_find(pipe_id));
	}
	process->fds[fd].kind = 0;
	process->fds[fd].handle = 0;
	process->fds[fd].offset = 0;
	process->fds[fd].size = 0;
	process->fds[fd].flags = 0;
	process->fds[fd].status_flags = 0;
	return 0;
}

static void linux_fd_ref_add(const struct linux_fd *fd)
{
	if (fd->kind == LINUX_FD_FILE || fd->kind == LINUX_FD_DIR) {
		linux_file_ref_add(fd->handle);
	} else if (fd->kind == LINUX_FD_PIPE_READ ||
		   fd->kind == LINUX_FD_PIPE_WRITE) {
		linux_pipe_ref_add(fd);
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
	process->fds[new_fd].flags = (flags & LINUX_DUP_CLOEXEC) != 0 ?
				     LINUX_FD_CLOEXEC : 0;
	linux_fd_ref_add(&process->fds[new_fd]);
	return (long)new_fd;
}

static long linux_fcntl(struct linux_process *process, u64 fd, u64 cmd, u64 arg)
{
	if (fd >= process->fd_capacity || process->fds[fd].kind == 0) {
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
			(arg & LINUX_O_APPEND);
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
			return -LINUX_EFAULT;
		}
		return 0;
	}

	return -LINUX_EINVAL;
}

static long linux_exec_process(struct linux_process *process)
{
	if (process == 0) {
		return -LINUX_ESRCH;
	}

	for (u64 fd = 0; fd < process->fd_capacity; fd++) {
		if (process->fds[fd].kind != 0 &&
		    (process->fds[fd].flags & LINUX_FD_CLOEXEC) != 0) {
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
	copy_field(uts, 1 * 65, 65, "bunix");
	copy_field(uts, 2 * 65, 65, "0.1");
	copy_field(uts, 3 * 65, 65, "#1");
	copy_field(uts, 4 * 65, 65, "x86_64");
	copy_field(uts, 5 * 65, 65, "local");
	return bunix_buffer_write(buffer, 0, uts, sizeof(uts)) == 0 ? 0 :
	       -LINUX_EFAULT;
}

static long linux_sysinfo(u64 buffer)
{
	char info[LINUX_SYSINFO_SIZE];

	if (buffer == 0) {
		return -LINUX_EFAULT;
	}
	zero_bytes(info, sizeof(info));
	store_u64(info, 0, bunix_timer_ticks() / 1000000000ull);
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

static long linux_clock_gettime(u64 buffer)
{
	char timespec[LINUX_TIMESPEC_SIZE];
	const u64 ns = bunix_timer_ticks();

	if (buffer == 0) {
		return -LINUX_EFAULT;
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
	const u64 ns = bunix_timer_ticks();

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
	const u64 seconds = bunix_timer_ticks() / 1000000000ull;

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
	process->umask = 0;
	process->session_id = 0;
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

		if (bunix_ipc_recv(BUNIX_HANDLE_SELF, &message) != 0 ||
		    message.protocol != BUNIX_PROTO_LINUX) {
			continue;
		}

		reply.type = message.type;
		if (message.type == BUNIX_LINUX_TTY_INPUT) {
			tty_input_event((char)(unsigned char)message.words[0]);
			continue;
		}
		if (message.type == BUNIX_LINUX_REGISTER_PROCESS) {
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
					(u64)bunix_machine_poweroff(0);
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
								 &reply.words[1]);
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
					bunix_console_log(wait4_ok,
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
			if ((long)reply.words[0] >= 0) {
				bunix_console_log(open_ok, sizeof(open_ok) - 1);
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
				bunix_console_log(fstat_ok,
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
			reply.words[0] = (u64)linux_clock_gettime(message.cap);
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
		case BUNIX_LINUX_SENDTO:
			reply.words[0] = (u64)linux_sendto(process,
							   message.words[0],
							   message.words[1],
							   message.cap);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_RECVFROM:
			reply.words[0] = (u64)linux_recvfrom(process,
							     message.words[0],
							     message.words[1],
							     message.cap);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_EXEC_PROCESS:
			reply.words[0] = (u64)linux_exec_process(process);
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
				bunix_console_log(newfstatat_ok,
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

			reply.words[0] = (u64)linux_write_buffer(process, fd, len,
								 message.cap);
			if (reply.words[0] == (u64)-LINUX_EBADF) {
				bunix_console_log(bad_fd, sizeof(bad_fd) - 1);
			}
			if (message.cap != 0) {
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
				bunix_console_log(close_ok, sizeof(close_ok) - 1);
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
