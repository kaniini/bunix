#include <bunix/libbunix.h>

enum {
	LINUX_HANDLE_VFS = 3,
	LINUX_HANDLE_NAMES = 4,
	LINUX_EPERM = 1,
	LINUX_ENOENT = 2,
	LINUX_EBADF = 9,
	LINUX_EAGAIN = 11,
	LINUX_EACCES = 13,
	LINUX_ENOTDIR = 20,
	LINUX_EISDIR = 21,
	LINUX_EINVAL = 22,
	LINUX_EMFILE = 24,
	LINUX_EPIPE = 32,
	LINUX_EAFNOSUPPORT = 97,
	LINUX_EPROTONOSUPPORT = 93,
	LINUX_ENOTSOCK = 88,
	LINUX_ESRCH = 3,
	LINUX_ECHILD = 10,
	LINUX_WAIT_BLOCK = 0x7fffffff,
	LINUX_NO_PROCESS = 0xffffffff,
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
	LINUX_TERM_SIZE = 64,
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
	LINUX_MAX_WRITE = 4096,
	LINUX_MAX_PATH = 32,
	LINUX_STAT_SIZE = 144,
	LINUX_MAX_PROCESSES = 64,
	LINUX_MAX_FDS = 16,
	LINUX_MAX_FILE_REFS = 32,
	LINUX_MAX_PIPES = 8,
	LINUX_PIPE_CAPACITY = 4096,
	LINUX_S_IFCHR = 0020000,
	LINUX_S_IFDIR = 0040000,
	LINUX_S_IFREG = 0100000,
	LINUX_S_IFSOCK = 0140000,
	LINUX_S_IFIFO = 0010000,
	LINUX_O_ACCMODE = 3,
	LINUX_O_DIRECTORY = 00200000,
	LINUX_O_CLOEXEC = 02000000,
	LINUX_DUP_CLOEXEC = 02000000,
	LINUX_FD_CLOEXEC = 1,
	LINUX_FD_CONSOLE = 1,
	LINUX_FD_FILE = 2,
	LINUX_FD_DIR = 3,
	LINUX_FD_UTMP = 4,
	LINUX_FD_SOCKET = 5,
	LINUX_FD_PIPE_READ = 6,
	LINUX_FD_PIPE_WRITE = 7,
	LINUX_AF_UNIX = 1,
	LINUX_SOCK_STREAM = 1,
	LINUX_SOCK_NONBLOCK = 00004000,
	LINUX_SOCK_CLOEXEC = 02000000,
	LINUX_SOCKET_UNIX_STREAM = 1,
	LINUX_SOCKET_UTMPD = 2,
	LINUX_UTMPS_NONE = 0,
	LINUX_UTMPS_REWIND = 'r',
	LINUX_UTMPS_GETENT = 'e',
	LINUX_AT_FDCWD = (u64)-100,
	LINUX_F_DUPFD = 0,
	LINUX_F_GETFD = 1,
	LINUX_F_SETFD = 2,
	LINUX_F_GETFL = 3,
	LINUX_F_SETFL = 4,
	LINUX_F_DUPFD_CLOEXEC = 1030,
	LINUX_DT_DIR = 4,
	LINUX_DT_REG = 8,
	LINUX_UTMP_RECORD_SIZE = 400,
	LINUX_UTMP_USER_PROCESS = 7,
};

struct linux_fd {
	u64 handle;
	u64 kind;
	u64 offset;
	u64 size;
	u64 flags;
};

struct linux_process {
	u64 pid;
	u64 tid;
	u64 ppid;
	u64 pgid;
	u64 sid;
	u64 parent;
	u64 first_child;
	u64 next_sibling;
	u64 bunix_task;
	u64 bunix_thread;
	u64 exited;
	u64 exit_status;
	u64 waited;
	u64 waiter;
	u64 wait_buffer;
	u64 wait_pid;
	u64 pending_signals;
	u64 session_id;
	char cwd[LINUX_MAX_PATH];
	struct linux_fd fds[LINUX_MAX_FDS];
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

static struct linux_process processes[LINUX_MAX_PROCESSES];
static struct linux_pipe pipes[LINUX_MAX_PIPES];
static char write_buffer[LINUX_MAX_WRITE];
static char utmp_buffer[LINUX_MAX_WRITE];
static char tty_termios[LINUX_TERM_SIZE];
static char tty_line[256];
static u64 tty_line_offset;
static u64 tty_line_len;
static u64 file_ref_handles[LINUX_MAX_FILE_REFS];
static u64 file_ref_counts[LINUX_MAX_FILE_REFS];
static u64 next_pid = 1;
static u64 next_pipe_id = 1;
static u64 foreground_pgid = 1;
static u64 user_service;

static u64 resolve_service(u64 service, unsigned int rights);
static void linux_process_reset(struct linux_process *process);
static void linux_close_process_fds(struct linux_process *process);
static long linux_user_process_exit(u64 pid);
static void linux_wake_parent(struct linux_process *child);

static u64 linux_user_service(void)
{
	if (user_service == 0) {
		user_service = resolve_service(BUNIX_SERVICE_USER,
					       BUNIX_RIGHT_SEND);
	}

	return user_service;
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

static u64 linux_user_session_count(void)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_USER,
		.type = BUNIX_USER_SESSION_COUNT,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { 0, 0, 0, 0 },
	};
	struct bunix_msg reply;
	const u64 user = linux_user_service();

	if (user == 0 ||
	    bunix_ipc_call(user, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return 0;
	}
	return reply.words[1];
}

static long linux_user_session_at(u64 index, u64 *session_id, u64 *uid,
				  u64 *tty, u64 *foreground)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_USER,
		.type = BUNIX_USER_SESSION_AT,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { index, 0, 0, 0 },
	};
	struct bunix_msg reply;
	const u64 user = linux_user_service();

	if (user == 0 ||
	    bunix_ipc_call(user, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return -LINUX_ESRCH;
	}
	if (session_id != 0) {
		*session_id = reply.words[1];
	}
	if (uid != 0) {
		*uid = reply.words[2];
	}
	if (tty != 0) {
		*tty = reply.words[3] >> 32;
	}
	if (foreground != 0) {
		*foreground = reply.words[3] & 0xffffffff;
	}
	return 0;
}

static u64 linux_process_index(const struct linux_process *process)
{
	if (process == 0) {
		return LINUX_NO_PROCESS;
	}

	for (u64 i = 0; i < LINUX_MAX_PROCESSES; i++) {
		if (&processes[i] == process) {
			return i;
		}
	}

	return LINUX_NO_PROCESS;
}

static struct linux_process *linux_process_at(u64 index)
{
	return index < LINUX_MAX_PROCESSES ? &processes[index] : 0;
}

static void linux_process_init_links(struct linux_process *process)
{
	process->parent = LINUX_NO_PROCESS;
	process->first_child = LINUX_NO_PROCESS;
	process->next_sibling = LINUX_NO_PROCESS;
}

static void linux_child_link(struct linux_process *parent,
			     struct linux_process *child)
{
	if (parent == 0 || child == 0) {
		return;
	}

	child->parent = linux_process_index(parent);
	child->next_sibling = parent->first_child;
	parent->first_child = linux_process_index(child);
	child->ppid = parent->pid;
}

static void linux_child_unlink(struct linux_process *child)
{
	struct linux_process *parent;
	u64 *link;
	const u64 child_index = linux_process_index(child);

	if (child == 0 || child->parent == LINUX_NO_PROCESS ||
	    child_index == LINUX_NO_PROCESS) {
		return;
	}

	parent = linux_process_at(child->parent);
	if (parent == 0) {
		return;
	}

	link = &parent->first_child;
	while (*link != LINUX_NO_PROCESS) {
		struct linux_process *candidate = linux_process_at(*link);

		if (*link == child_index) {
			*link = child->next_sibling;
			child->parent = LINUX_NO_PROCESS;
			child->next_sibling = LINUX_NO_PROCESS;
			return;
		}
		if (candidate == 0) {
			break;
		}
		link = &candidate->next_sibling;
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

static void zero_bytes(char *buffer, u64 len)
{
	for (u64 i = 0; i < len; i++) {
		buffer[i] = 0;
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

static void pack_path(u64 *words, const char *path)
{
	words[0] = 0;
	words[1] = 0;

	for (u64 i = 0; i < 16 && path[i] != '\0'; i++) {
		const u64 slot = i / 8;
		const u64 shift = (i % 8) * 8;

		words[slot] |= ((u64)(unsigned char)path[i]) << shift;
	}
}

static void unpack_path(char *path, u64 word0, u64 word1)
{
	const u64 words[] = { word0, word1 };

	for (u64 i = 0; i < 16; i++) {
		path[i] = (char)((words[i / 8] >> ((i % 8) * 8)) & 0xff);
		if (path[i] == '\0') {
			return;
		}
	}
	path[15] = '\0';
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

static int is_utmp_path(const char *path)
{
	return string_equal(path, "/run/utmp") ||
	       string_equal(path, "/var/run/utmp");
}

static int is_utmps_socket_path(const char *path)
{
	return string_equal(path, "/run/utmps/.utmpd-socket");
}

static void copy_literal(char *dst, u64 dst_len, const char *src)
{
	for (u64 i = 0; i < dst_len; i++) {
		dst[i] = src[i];
		if (src[i] == '\0') {
			return;
		}
	}
}

static const char *user_name_for_uid(u64 uid)
{
	if (uid == 0) {
		return "root";
	}
	if (uid == 1000) {
		return "kaniini";
	}
	return "user";
}

static void build_utmp_record(char *record, u64 index)
{
	u64 session_id = 0;
	u64 uid = 0;
	u64 tty = 0;
	u64 foreground = 0;

	zero_bytes(record, LINUX_UTMP_RECORD_SIZE);
	if (linux_user_session_at(index, &session_id, &uid,
				  &tty, &foreground) != 0) {
		return;
	}

	store_u16(record, 0, LINUX_UTMP_USER_PROCESS);
	store_u32(record, 4, (unsigned int)foreground);
	copy_literal(record + 8, 32, tty == 1 ? "ttyS0" : "tty");
	copy_literal(record + 40, 4, tty == 1 ? "S0" : "tt");
	copy_literal(record + 44, 32, user_name_for_uid(uid));
	store_u32(record, 336, (unsigned int)session_id);
}

static long read_utmp(u64 offset, u64 len, u64 buffer)
{
	const u64 size = linux_user_session_count() * LINUX_UTMP_RECORD_SIZE;
	u64 done = 0;

	if (buffer == 0) {
		return -LINUX_EBADF;
	}
	if (offset >= size) {
		return 0;
	}
	if (len > LINUX_MAX_WRITE) {
		len = LINUX_MAX_WRITE;
	}
	if (len > size - offset) {
		len = size - offset;
	}

	while (done < len) {
		const u64 absolute = offset + done;
		const u64 record_index = absolute / LINUX_UTMP_RECORD_SIZE;
		const u64 record_offset = absolute % LINUX_UTMP_RECORD_SIZE;
		u64 chunk = LINUX_UTMP_RECORD_SIZE - record_offset;
		char record[LINUX_UTMP_RECORD_SIZE];

		if (chunk > len - done) {
			chunk = len - done;
		}
		build_utmp_record(record, record_index);
		for (u64 i = 0; i < chunk; i++) {
			utmp_buffer[done + i] = record[record_offset + i];
		}
		done += chunk;
	}

	if (bunix_buffer_write(buffer, 0, utmp_buffer, done) != 0) {
		return -LINUX_EINVAL;
	}
	return (long)done;
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

	zero_bytes(utmp_buffer, sizeof(utmp_buffer));
	if (fd->size == LINUX_UTMPS_REWIND) {
		fd->offset = 0;
		utmp_buffer[0] = 0;
	} else if (fd->size == LINUX_UTMPS_GETENT) {
		if (fd->offset >= linux_user_session_count()) {
			utmp_buffer[0] = LINUX_ENOENT;
		} else {
			utmp_buffer[0] = 0;
			build_utmp_record(utmp_buffer + 1, fd->offset);
			fd->offset++;
			response_len = LINUX_UTMP_RECORD_SIZE + 1;
		}
	} else {
		utmp_buffer[0] = LINUX_EINVAL;
	}

	fd->size = LINUX_UTMPS_NONE;
	if (response_len > len) {
		response_len = len;
	}
	if (bunix_buffer_write(buffer, 0, utmp_buffer, response_len) != 0) {
		return -LINUX_EINVAL;
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

static int path_normalize(const char *cwd, const char *input, char *out)
{
	char temp[LINUX_MAX_PATH];
	u64 pos = 0;
	u64 in = 0;

	if (input == 0 || input[0] == '\0') {
		return -1;
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
		if (pos > 1 && pos < sizeof(temp) - 1) {
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
			if (comp_len + 1 >= sizeof(component)) {
				return -1;
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
			}
			temp[pos] = '\0';
			continue;
		}
		if (pos > 1) {
			if (pos + 1 >= sizeof(temp)) {
				return -1;
			}
			temp[pos++] = '/';
		}
		if (pos + comp_len >= sizeof(temp)) {
			return -1;
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

static void linux_process_init_fds(struct linux_process *process)
{
	for (u64 fd = 0; fd < LINUX_MAX_FDS; fd++) {
		process->fds[fd].handle = 0;
		process->fds[fd].kind = 0;
		process->fds[fd].offset = 0;
		process->fds[fd].size = 0;
		process->fds[fd].flags = 0;
	}

	process->fds[0].handle = BUNIX_HANDLE_CONSOLE;
	process->fds[0].kind = LINUX_FD_CONSOLE;
	process->fds[1].handle = BUNIX_HANDLE_CONSOLE;
	process->fds[1].kind = LINUX_FD_CONSOLE;
	process->fds[2].handle = BUNIX_HANDLE_CONSOLE;
	process->fds[2].kind = LINUX_FD_CONSOLE;
	process->cwd[0] = '/';
	process->cwd[1] = '\0';
}

static long alloc_fd(struct linux_process *process, u64 kind, u64 handle,
		     u64 size)
{
	for (u64 fd = 0; fd < LINUX_MAX_FDS; fd++) {
		if (process->fds[fd].kind == 0) {
			process->fds[fd].kind = kind;
			process->fds[fd].handle = handle;
			process->fds[fd].offset = 0;
			process->fds[fd].size = size;
			process->fds[fd].flags = 0;
			return (long)fd;
		}
	}

	return -LINUX_EMFILE;
}

static struct linux_pipe *linux_pipe_find(u64 id)
{
	if (id == 0) {
		return 0;
	}
	for (u64 i = 0; i < LINUX_MAX_PIPES; i++) {
		if (pipes[i].id == id) {
			return &pipes[i];
		}
	}
	return 0;
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
		pipe->id = 0;
		pipe->start = 0;
		pipe->len = 0;
		pipe->reader_reply = 0;
		pipe->reader_buffer = 0;
		pipe->reader_len = 0;
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
	for (u64 i = 0; i < LINUX_MAX_PIPES; i++) {
		if (pipes[i].id == 0) {
			pipe = &pipes[i];
			break;
		}
	}
	if (pipe == 0) {
		return -LINUX_EMFILE;
	}

	pipe->id = next_pipe_id++;
	if (next_pipe_id == 0) {
		next_pipe_id = 1;
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
		pipe->id = 0;
		return left;
	}
	right = alloc_fd(process, LINUX_FD_PIPE_WRITE, pipe->id, 0);
	if (right < 0) {
		process->fds[left].kind = 0;
		pipe->id = 0;
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
		return -LINUX_EINVAL;
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
	if (handle == 0) {
		return;
	}

	for (u64 i = 0; i < LINUX_MAX_FILE_REFS; i++) {
		if (file_ref_handles[i] == handle) {
			file_ref_counts[i]++;
			return;
		}
	}

	for (u64 i = 0; i < LINUX_MAX_FILE_REFS; i++) {
		if (file_ref_handles[i] == 0) {
			file_ref_handles[i] = handle;
			file_ref_counts[i] = 1;
			return;
		}
	}
}

static long linux_file_ref_drop(u64 handle)
{
	if (handle == 0) {
		return 0;
	}

	for (u64 i = 0; i < LINUX_MAX_FILE_REFS; i++) {
		if (file_ref_handles[i] != handle) {
			continue;
		}
		if (file_ref_counts[i] > 1) {
			file_ref_counts[i]--;
			return 1;
		}
		file_ref_handles[i] = 0;
		file_ref_counts[i] = 0;
		return 0;
	}

	return 0;
}

static struct linux_process *linux_process_find(u64 bunix_task)
{
	for (u64 i = 0; i < LINUX_MAX_PROCESSES; i++) {
		if (processes[i].bunix_task == bunix_task) {
			return &processes[i];
		}
	}

	return 0;
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

static long linux_register_process(u64 bunix_task, u64 ppid, u64 session_id)
{
	if (bunix_task == 0 || linux_process_find(bunix_task) != 0) {
		return -LINUX_EINVAL;
	}

	for (u64 i = 0; i < LINUX_MAX_PROCESSES; i++) {
		if (processes[i].pid == 0) {
			const u64 pid = next_pid++;

			processes[i].pid = pid;
			processes[i].tid = pid;
			processes[i].ppid = ppid;
			processes[i].pgid = pid;
			processes[i].sid = pid;
			linux_process_init_links(&processes[i]);
			processes[i].bunix_task = bunix_task;
			processes[i].bunix_thread = 0;
			processes[i].exited = 0;
			processes[i].exit_status = 0;
			processes[i].waited = 0;
			processes[i].waiter = 0;
			processes[i].wait_buffer = 0;
			processes[i].wait_pid = 0;
			processes[i].pending_signals = 0;
			processes[i].session_id = session_id;
			linux_process_init_fds(&processes[i]);
			if (foreground_pgid == 0 || pid == 1) {
				foreground_pgid = pid;
			}
			if (session_id != 0) {
				(void)linux_user_session_set_foreground(session_id,
									processes[i].pgid);
			}
			if (linux_user_process_register(bunix_task) != 0) {
				linux_process_reset(&processes[i]);
				return -LINUX_ESRCH;
			}
			return (long)pid;
		}
	}

	return -LINUX_ESRCH;
}

static long linux_fork_process(u64 parent_task, u64 child_task)
{
	struct linux_process *parent = linux_process_find(parent_task);

	if (parent == 0 || child_task == 0 ||
	    linux_process_find(child_task) != 0) {
		return -LINUX_EINVAL;
	}

	for (u64 i = 0; i < LINUX_MAX_PROCESSES; i++) {
		if (processes[i].pid == 0) {
			const u64 pid = next_pid++;

			processes[i].pid = pid;
			processes[i].tid = pid;
			processes[i].ppid = parent->pid;
			processes[i].pgid = parent->pgid;
			processes[i].sid = parent->sid;
			linux_process_init_links(&processes[i]);
			linux_child_link(parent, &processes[i]);
			processes[i].bunix_task = child_task;
			processes[i].bunix_thread = 0;
			processes[i].exited = 0;
			processes[i].exit_status = 0;
			processes[i].waited = 0;
			processes[i].waiter = 0;
			processes[i].wait_buffer = 0;
			processes[i].wait_pid = 0;
			processes[i].pending_signals = 0;
			processes[i].session_id = parent->session_id;
			for (u64 fd = 0; fd < LINUX_MAX_FDS; fd++) {
				processes[i].fds[fd] = parent->fds[fd];
				if (processes[i].fds[fd].kind == LINUX_FD_FILE) {
					linux_file_ref_add(processes[i].fds[fd].handle);
				} else if (processes[i].fds[fd].kind == LINUX_FD_DIR) {
					linux_file_ref_add(processes[i].fds[fd].handle);
				} else if (processes[i].fds[fd].kind == LINUX_FD_PIPE_READ ||
					   processes[i].fds[fd].kind == LINUX_FD_PIPE_WRITE) {
					linux_pipe_ref_add(&processes[i].fds[fd]);
				}
			}
			string_copy(processes[i].cwd, parent->cwd);
			if (linux_user_process_fork(parent_task, child_task) != 0) {
				linux_process_reset(&processes[i]);
				return -LINUX_ESRCH;
			}
			return (long)pid;
		}
	}

	return -LINUX_ESRCH;
}

static struct linux_process *linux_process_find_pid(u64 pid)
{
	for (u64 i = 0; i < LINUX_MAX_PROCESSES; i++) {
		if (processes[i].pid == pid) {
			return &processes[i];
		}
	}

	return 0;
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
	for (u64 i = 0; i < LINUX_MAX_PROCESSES; i++) {
		if (processes[i].pid != 0 &&
		    !processes[i].exited &&
		    processes[i].pgid == pgid) {
			return 1;
		}
	}

	return 0;
}

static int linux_child_matches(const struct linux_process *parent,
			       const struct linux_process *child, long pid)
{
	if (parent == 0 || child == 0 ||
	    child->parent != linux_process_index(parent) ||
	    child->waited) {
		return 0;
	}

	return pid == -1 || child->pid == (u64)pid;
}

static void linux_reparent_children(struct linux_process *process)
{
	struct linux_process *init = linux_process_find_pid(1);
	u64 child_index;

	if (process == 0 || process->first_child == LINUX_NO_PROCESS) {
		return;
	}

	child_index = process->first_child;
	process->first_child = LINUX_NO_PROCESS;

	while (child_index != LINUX_NO_PROCESS) {
		struct linux_process *child = linux_process_at(child_index);
		u64 next;

		if (child == 0) {
			break;
		}
		next = child->next_sibling;
		child->parent = LINUX_NO_PROCESS;
		child->next_sibling = LINUX_NO_PROCESS;
		if (init != 0 && init != process) {
			linux_child_link(init, child);
		} else {
			child->ppid = 0;
		}
		child_index = next;
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

	if ((pid != -1 && pid <= 0) ||
	    (options & ~(LINUX_WNOHANG | LINUX_WUNTRACED |
			 LINUX_WCONTINUED)) != 0) {
		return -LINUX_EINVAL;
	}

	for (u64 child_index = parent->first_child;
	     child_index != LINUX_NO_PROCESS;) {
		struct linux_process *child = linux_process_at(child_index);

		if (child == 0) {
			break;
		}
		child_index = child->next_sibling;

		if (!linux_child_matches(parent, child, pid)) {
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
		bunix_console_write(wait4_ok, sizeof(wait4_ok) - 1);
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

static int linux_signal_process(struct linux_process *process, u64 signal)
{
	if (process == 0 || process->exited || signal >= 64) {
		return 0;
	}
	if (signal == 0) {
		return 1;
	}

	process->pending_signals |= 1ull << signal;
	if (signal == LINUX_SIGINT || signal == LINUX_SIGTERM) {
		linux_process_exit_status(process, signal, 1);
		return 1;
	}

	return 0;
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
	if (process != 0 && process->session_id != 0) {
		return linux_user_session_set_foreground(process->session_id,
							 pgid);
	}
	foreground_pgid = pgid;
	return 0;
}

static long linux_kill(struct linux_process *source, long pid, u64 signal)
{
	int delivered = 0;

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
		for (u64 i = 0; i < LINUX_MAX_PROCESSES; i++) {
			if (processes[i].pid != 0 &&
			    !processes[i].exited &&
			    processes[i].pgid == source->pgid &&
			    linux_same_session(source, &processes[i])) {
				delivered |= linux_signal_process(&processes[i],
								  signal);
			}
		}
		return delivered ? 0 : -LINUX_ESRCH;
	}
	if (pid < -1) {
		const u64 pgid = (u64)(-pid);

		for (u64 i = 0; i < LINUX_MAX_PROCESSES; i++) {
			if (processes[i].pid != 0 &&
			    !processes[i].exited &&
			    processes[i].pgid == pgid &&
			    linux_same_session(source, &processes[i])) {
				delivered |= linux_signal_process(&processes[i],
								  signal);
			}
		}
		return delivered ? 0 : -LINUX_ESRCH;
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
	if (bunix_ipc_call(user_service, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return -LINUX_EINVAL;
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
	if (bunix_ipc_call(user_service, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return -LINUX_EINVAL;
	}

	if (group0 != 0) {
		*group0 = reply.words[2];
	}
	if (group1 != 0) {
		*group1 = reply.words[3];
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
		return -LINUX_EINVAL;
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
		return -LINUX_EINVAL;
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

static long linux_setpgid(struct linux_process *process, u64 pid, u64 pgid)
{
	struct linux_process *target = pid == 0 ? process :
				       linux_process_find_pid(pid);
	struct linux_process *leader;

	if (target == 0) {
		return -LINUX_ESRCH;
	}
	if (target != process &&
	    target->parent != linux_process_index(process)) {
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
	const struct bunix_msg console_message = {
		.protocol = BUNIX_PROTO_CONSOLE,
		.type = BUNIX_CONSOLE_WRITE,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { (u64)text, len, 0, 0 },
	};

	bunix_ipc_send(BUNIX_HANDLE_CONSOLE, &console_message);
}

static long tty_termios_get(u64 buffer)
{
	tty_init();
	return bunix_buffer_write(buffer, 0, tty_termios,
				  sizeof(tty_termios)) == 0 ?
	       0 : -LINUX_EINVAL;
}

static long tty_termios_set(u64 buffer)
{
	if (buffer == 0 ||
	    bunix_buffer_read(buffer, 0, tty_termios,
			      sizeof(tty_termios)) != 0) {
		return -LINUX_EINVAL;
	}
	tty_line_offset = 0;
	tty_line_len = 0;
	if (tty_termios[LINUX_TERM_CC + LINUX_VMIN] == 0) {
		tty_termios[LINUX_TERM_CC + LINUX_VMIN] = 1;
	}
	return 0;
}

static long tty_read_raw(struct linux_process *process, u64 len, u64 buffer)
{
	u64 done = 0;
	const unsigned int lflag = tty_lflag();
	const char intr = tty_termios[LINUX_TERM_CC + LINUX_VINTR];

	while (done < len) {
		char c;
		const long nread = bunix_console_read(&c, 1);

		if (nread != 1) {
			if (done != 0) {
				return (long)done;
			}
			return -LINUX_EINVAL;
		}
		if ((lflag & LINUX_ISIG) != 0 && c == intr) {
			if ((lflag & LINUX_ECHO) != 0) {
				tty_echo("^C\n", 3);
			}
			(void)process;
			return done != 0 ? (long)done : 0;
		}
		write_buffer[done++] = c;
		if ((lflag & LINUX_ECHO) != 0) {
			tty_echo(&c, 1);
		}
		if (done >= (u64)(unsigned char)tty_termios[LINUX_TERM_CC + LINUX_VMIN]) {
			break;
		}
	}

	if (bunix_buffer_write(buffer, 0, write_buffer, done) != 0) {
		return -LINUX_EINVAL;
	}
	return (long)done;
}

static long tty_fill_canonical_line(struct linux_process *process)
{
	const unsigned int lflag = tty_lflag();
	const unsigned int iflag = tty_iflag();
	const char erase = tty_termios[LINUX_TERM_CC + LINUX_VERASE];
	const char intr = tty_termios[LINUX_TERM_CC + LINUX_VINTR];
	u64 used = 0;

	for (;;) {
		char c;
		const long nread = bunix_console_read(&c, 1);

		if (nread != 1) {
			return -LINUX_EINVAL;
		}
		if (c == '\r' && (iflag & LINUX_ICRNL) != 0) {
			c = '\n';
		}
		if ((lflag & LINUX_ISIG) != 0 && c == intr) {
			if ((lflag & LINUX_ECHO) != 0) {
				tty_echo("^C\n", 3);
			}
			tty_line_offset = 0;
			tty_line_len = 0;
			(void)process;
			return 0;
		}
		if ((c == erase || c == '\b') && used != 0) {
			used--;
			if ((lflag & LINUX_ECHO) != 0) {
				tty_echo("\b \b", 3);
			}
			continue;
		}
		if ((unsigned char)c < 0x20 && c != '\n' &&
		    c != tty_termios[LINUX_TERM_CC + LINUX_VEOF]) {
			continue;
		}
		if (c == tty_termios[LINUX_TERM_CC + LINUX_VEOF]) {
			break;
		}
		if (used < sizeof(tty_line)) {
			tty_line[used++] = c;
		}
		if ((lflag & LINUX_ECHO) != 0) {
			tty_echo(&c, 1);
		}
		if (c == '\n' || used == sizeof(tty_line)) {
			break;
		}
	}

	tty_line_offset = 0;
	tty_line_len = used;
	return 0;
}

static long tty_read_canonical(struct linux_process *process, u64 len,
			       u64 buffer)
{
	long filled;

	if (tty_line_offset >= tty_line_len &&
	    (filled = tty_fill_canonical_line(process)) != 0) {
		return filled;
	}

	const u64 remaining = tty_line_len - tty_line_offset;
	const u64 nread = len < remaining ? len : remaining;

	for (u64 i = 0; i < nread; i++) {
		write_buffer[i] = tty_line[tty_line_offset + i];
	}
	tty_line_offset += nread;
	if (tty_line_offset >= tty_line_len) {
		tty_line_offset = 0;
		tty_line_len = 0;
	}

	if (bunix_buffer_write(buffer, 0, write_buffer, nread) != 0) {
		return -LINUX_EINVAL;
	}
	return (long)nread;
}

static long tty_read(struct linux_process *process, u64 len, u64 buffer)
{
	if (len == 0) {
		return 0;
	}
	if ((tty_lflag() & LINUX_ICANON) != 0) {
		return tty_read_canonical(process, len, buffer);
	}
	return tty_read_raw(process, len, buffer);
}

static long linux_ioctl(struct linux_process *process, u64 fd, u64 request,
			u64 value, u64 buffer)
{
	char data[8];
	u64 pgid;

	if (fd >= LINUX_MAX_FDS ||
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
			return -LINUX_EINVAL;
		}
		zero_bytes(data, sizeof(data));
		store_u32(data, 0,
			  (unsigned int)linux_foreground_pgid(process));
		return bunix_buffer_write(buffer, 0, data, 4) == 0 ?
		       0 : -LINUX_EINVAL;
	case LINUX_TIOCSPGRP:
		pgid = value;
		return linux_set_foreground_pgid(process, pgid);
	case LINUX_TIOCGWINSZ:
		if (buffer == 0) {
			return -LINUX_EINVAL;
		}
		zero_bytes(data, sizeof(data));
		store_u16(data, 0, 25);
		store_u16(data, 2, 80);
		return bunix_buffer_write(buffer, 0, data, 8) == 0 ?
		       0 : -LINUX_EINVAL;
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
	return -LINUX_ENOENT;
}

static u64 linux_mode_for_type(u64 type, u64 mode)
{
	if (type == BUNIX_VFS_TYPE_DIRECTORY) {
		return LINUX_S_IFDIR | mode;
	}
	return LINUX_S_IFREG | mode;
}

static long linux_openat(struct linux_process *process, u64 dirfd,
			 u64 path_len, u64 flags, u64 path_buffer)
{
	char path[LINUX_MAX_PATH];
	char full_path[LINUX_MAX_PATH];
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_OPEN,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { 0, 0, 0, 0 },
	};
	struct bunix_msg reply;

	if (dirfd != LINUX_AT_FDCWD ||
	    path_buffer == 0 || path_len == 0 || path_len > sizeof(path) ||
	    bunix_buffer_read(path_buffer, 0, path, path_len) != 0 ||
	    path[path_len - 1] != '\0' ||
	    path_normalize(process->cwd, path, full_path) != 0) {
		return -LINUX_EINVAL;
	}

	if (string_equal(full_path, "/dev/tty")) {
		return alloc_fd(process, LINUX_FD_CONSOLE,
				BUNIX_HANDLE_CONSOLE, 0);
	}
	if (is_utmp_path(full_path)) {
		return alloc_fd(process, LINUX_FD_UTMP, 0,
				linux_user_session_count() *
				LINUX_UTMP_RECORD_SIZE);
	}
	if ((flags & LINUX_O_ACCMODE) != 0) {
		return -LINUX_EINVAL;
	}

	pack_path(&request.words[0], full_path);
	request.words[3] = process->bunix_task;
	if (bunix_ipc_call(LINUX_HANDLE_VFS, &request, &reply) != 0) {
		return -LINUX_ENOENT;
	}
	if (reply.words[0] != 0) {
		return linux_vfs_error(reply.words[0]);
	}
	if ((flags & LINUX_O_DIRECTORY) != 0 &&
	    reply.words[3] != BUNIX_VFS_TYPE_DIRECTORY) {
		request.type = BUNIX_VFS_CLOSE;
		request.words[0] = reply.words[1];
		(void)bunix_ipc_call(LINUX_HANDLE_VFS, &request, &reply);
		return -LINUX_ENOTDIR;
	}

	const long fd = alloc_fd(process,
				 reply.words[3] == BUNIX_VFS_TYPE_DIRECTORY ?
				 LINUX_FD_DIR : LINUX_FD_FILE,
				 reply.words[1], reply.words[2]);
	if (fd < 0) {
		request.type = BUNIX_VFS_CLOSE;
		request.words[0] = reply.words[1];
		(void)bunix_ipc_call(LINUX_HANDLE_VFS, &request, &reply);
		return fd;
	}
	linux_file_ref_add(reply.words[1]);
	if ((flags & LINUX_O_CLOEXEC) != 0) {
		process->fds[fd].flags |= LINUX_FD_CLOEXEC;
	}
	return fd;
}

static long linux_stat_write(u64 stat_buffer, u64 mode, u64 uid, u64 gid,
			     u64 size)
{
	char stat[LINUX_STAT_SIZE];

	if (stat_buffer == 0) {
		return -LINUX_EINVAL;
	}

	zero_bytes(stat, sizeof(stat));
	store_u64(stat, 0, 1);
	store_u64(stat, 8, 1);
	store_u64(stat, 16, 1);
	store_u32(stat, 24, (unsigned int)mode);
	store_u32(stat, 28, (unsigned int)uid);
	store_u32(stat, 32, (unsigned int)gid);
	store_u64(stat, 48, size);
	store_u64(stat, 56, 4096);
	store_u64(stat, 64, (size + 511) / 512);

	return bunix_buffer_write(stat_buffer, 0, stat, sizeof(stat)) == 0 ?
		0 : -LINUX_EINVAL;
}

static long linux_stat_from_vfs_meta(u64 stat_buffer,
				     const struct bunix_msg *reply)
{
	const u64 mode = reply->words[2] & 0xffffffff;
	const u64 type = reply->words[2] >> 32;

	return linux_stat_write(stat_buffer, linux_mode_for_type(type, mode),
				reply->words[3] & 0xffffffff,
				reply->words[3] >> 32,
				reply->words[1]);
}

static long linux_fstat(struct linux_process *process, u64 fd, u64 stat_buffer)
{
	if (fd >= LINUX_MAX_FDS || process->fds[fd].kind == 0) {
		return -LINUX_EBADF;
	}

	if (process->fds[fd].kind == LINUX_FD_CONSOLE) {
		return linux_stat_write(stat_buffer, LINUX_S_IFCHR | 0600,
					0, 0, 0);
	}
	if (process->fds[fd].kind == LINUX_FD_UTMP) {
		return linux_stat_write(stat_buffer, LINUX_S_IFREG | 0444,
					0, 0,
					linux_user_session_count() *
					LINUX_UTMP_RECORD_SIZE);
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

	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_STAT_META,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { process->fds[fd].handle, 0, 0, 0 },
	};
	struct bunix_msg reply;

	if (bunix_ipc_call(LINUX_HANDLE_VFS, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return -LINUX_EINVAL;
	}

	return linux_stat_from_vfs_meta(stat_buffer, &reply);
}

static long linux_newfstatat(struct linux_process *process, u64 dirfd,
			     u64 word0, u64 word1, u64 stat_buffer)
{
	char path[16];
	char full_path[LINUX_MAX_PATH];
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_STAT_PATH_META,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { 0, 0, 0, 0 },
	};
	struct bunix_msg reply;

	if (dirfd != LINUX_AT_FDCWD || stat_buffer == 0) {
		return -LINUX_EINVAL;
	}

	unpack_path(path, word0, word1);
	if (path_normalize(process->cwd, path, full_path) != 0) {
		return -LINUX_EINVAL;
	}
	if (is_utmp_path(full_path)) {
		return linux_stat_write(stat_buffer, LINUX_S_IFREG | 0444,
					0, 0,
					linux_user_session_count() *
					LINUX_UTMP_RECORD_SIZE);
	}
	pack_path(&request.words[0], full_path);
	if (bunix_ipc_call(LINUX_HANDLE_VFS, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return -LINUX_ENOENT;
	}

	return linux_stat_from_vfs_meta(stat_buffer, &reply);
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

static long linux_getdents64(struct linux_process *process, u64 fd,
			     u64 buffer, u64 len)
{
	char out[LINUX_MAX_WRITE];
	u64 written = 0;

	if (fd >= LINUX_MAX_FDS || process->fds[fd].kind == 0) {
		return -LINUX_EBADF;
	}
	if (process->fds[fd].kind != LINUX_FD_DIR) {
		return -LINUX_ENOTDIR;
	}
	if (buffer == 0 || len > sizeof(out)) {
		return -LINUX_EINVAL;
	}

	while (written < len) {
		char name[16];
		u64 type;
		u64 next_offset;
		struct bunix_msg request = {
			.protocol = BUNIX_PROTO_VFS,
			.type = BUNIX_VFS_READDIR,
			.sender = 0,
			.cap_rights = 0,
			.reply = 0,
			.cap = 0,
			.words = { process->fds[fd].handle,
				   process->fds[fd].offset >= 2 ?
				   process->fds[fd].offset - 2 : 0,
				   0, 0 },
		};
		struct bunix_msg reply;
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
			if (bunix_ipc_call(LINUX_HANDLE_VFS, &request, &reply) != 0 ||
			    reply.words[0] != 0) {
				break;
			}
			unpack_path(name, reply.words[2], reply.words[3]);
			type = reply.words[1] >> 32;
			next_offset = (reply.words[1] & 0xffffffff) + 2;
		}

		reclen = linux_dirent_reclen(name);
		if (written + reclen > len) {
			break;
		}
		for (u64 i = 0; i < reclen; i++) {
			out[written + i] = 0;
		}
		linux_store_dirent(out, written, process->fds[fd].offset + 1,
				   next_offset, reclen,
				   type == BUNIX_VFS_TYPE_DIRECTORY ?
				   LINUX_DT_DIR : LINUX_DT_REG, name);
		written += reclen;
		process->fds[fd].offset = next_offset;
	}

	if (bunix_buffer_write(buffer, 0, out, written) != 0) {
		return -LINUX_EINVAL;
	}
	return (long)written;
}

static long linux_getcwd(struct linux_process *process, u64 size,
			 u64 *word0, u64 *word1)
{
	const u64 len = string_len(process->cwd) + 1;
	u64 words[2];

	if (size < len || len > BUNIX_IPC_DATA_BYTES ||
	    word0 == 0 || word1 == 0) {
		return -LINUX_EINVAL;
	}
	pack_path(words, process->cwd);
	*word0 = words[0];
	*word1 = words[1];
	return (long)len;
}

static long linux_chdir(struct linux_process *process, u64 word0, u64 word1)
{
	char path[LINUX_MAX_PATH];
	char full_path[LINUX_MAX_PATH];
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_OPEN,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { 0, 0, 0, process->bunix_task },
	};
	struct bunix_msg reply;

	for (u64 i = 0; i < sizeof(path); i++) {
		path[i] = '\0';
	}
	unpack_path(path, word0, word1);
	if (path[0] == '\0' ||
	    path_normalize(process->cwd, path, full_path) != 0) {
		return -LINUX_EINVAL;
	}

	pack_path(&request.words[0], full_path);
	if (bunix_ipc_call(LINUX_HANDLE_VFS, &request, &reply) != 0) {
		return -LINUX_ENOENT;
	}
	if (reply.words[0] != 0) {
		return linux_vfs_error(reply.words[0]);
	}
	if (reply.words[3] != BUNIX_VFS_TYPE_DIRECTORY) {
		request.type = BUNIX_VFS_CLOSE;
		request.words[0] = reply.words[1];
		(void)bunix_ipc_call(LINUX_HANDLE_VFS, &request, &reply);
		return -LINUX_ENOTDIR;
	}

	request.type = BUNIX_VFS_CLOSE;
	request.words[0] = reply.words[1];
	(void)bunix_ipc_call(LINUX_HANDLE_VFS, &request, &reply);
	string_copy(process->cwd, full_path);
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

	if (fd >= LINUX_MAX_FDS ||
	    process->fds[fd].kind != LINUX_FD_SOCKET) {
		return -LINUX_ENOTSOCK;
	}
	if (addr_buffer == 0 || addr_len < 3 || addr_len > sizeof(addr) ||
	    bunix_buffer_read(addr_buffer, 0, addr, addr_len) != 0) {
		return -LINUX_EINVAL;
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

	if (fd >= LINUX_MAX_FDS ||
	    process->fds[fd].kind != LINUX_FD_SOCKET) {
		return -LINUX_ENOTSOCK;
	}
	if (process->fds[fd].handle != LINUX_SOCKET_UTMPD) {
		return -LINUX_EINVAL;
	}
	if (len != 1 || buffer == 0 ||
	    bunix_buffer_read(buffer, 0, &command, 1) != 0) {
		return -LINUX_EINVAL;
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
	if (fd >= LINUX_MAX_FDS ||
	    process->fds[fd].kind != LINUX_FD_SOCKET) {
		return -LINUX_ENOTSOCK;
	}
	return utmps_recv_response(&process->fds[fd], len, buffer);
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
	if (fd >= LINUX_MAX_FDS ||
	    process->fds[fd].kind == 0 ||
	    buffer == 0) {
		return -LINUX_EBADF;
	}

	if (process->fds[fd].kind == LINUX_FD_CONSOLE) {
		return tty_read(process, len, buffer);
	}
	if (process->fds[fd].kind == LINUX_FD_UTMP) {
		const long nread = read_utmp(process->fds[fd].offset, len,
					     buffer);

		if (nread > 0) {
			process->fds[fd].offset += (u64)nread;
		}
		return nread;
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
	if (bunix_ipc_call(LINUX_HANDLE_VFS, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return -LINUX_EINVAL;
	}

	process->fds[fd].offset += reply.words[1];
	return (long)reply.words[1];
}

static long linux_write_buffer(struct linux_process *process, u64 fd, u64 len,
			       u64 buffer)
{
	if (fd >= LINUX_MAX_FDS || process->fds[fd].kind == 0) {
		return -LINUX_EBADF;
	}
	if (buffer == 0 || len > sizeof(write_buffer) ||
	    bunix_buffer_read(buffer, 0, write_buffer, len) != 0) {
		return -LINUX_EINVAL;
	}

	if (process->fds[fd].kind == LINUX_FD_PIPE_WRITE) {
		struct linux_pipe *pipe = linux_pipe_find(process->fds[fd].handle);
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
		nwritten = len < space ? len : space;
		for (u64 i = 0; i < nwritten; i++) {
			pipe->data[(pipe->start + pipe->len + i) %
				   LINUX_PIPE_CAPACITY] = write_buffer[i];
		}
		pipe->len += nwritten;
		linux_pipe_wake_reader(pipe);
		return (long)nwritten;
	}
	if (process->fds[fd].kind != LINUX_FD_CONSOLE) {
		return -LINUX_EBADF;
	}

	const struct bunix_msg console_message = {
		.protocol = BUNIX_PROTO_CONSOLE,
		.type = BUNIX_CONSOLE_WRITE,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { (u64)write_buffer, len, 0, 0 },
	};

	bunix_ipc_send(process->fds[fd].handle, &console_message);
	return (long)len;
}

static long linux_sendfile(struct linux_process *process, u64 out_fd,
			   u64 in_fd, u64 len, u64 offset, u64 buffer,
			   u64 *next_offset)
{
	u64 saved_offset = 0;
	const int positioned = offset != (u64)-1;
	long nread;

	if (in_fd >= LINUX_MAX_FDS || process->fds[in_fd].kind == 0) {
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

static long linux_close(struct linux_process *process, u64 fd)
{
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

	if (fd >= LINUX_MAX_FDS ||
	    process->fds[fd].kind == 0) {
		return -LINUX_EBADF;
	}

	if (process->fds[fd].kind == LINUX_FD_FILE ||
	    process->fds[fd].kind == LINUX_FD_DIR) {
		if (linux_file_ref_drop(process->fds[fd].handle) == 0) {
			request.words[0] = process->fds[fd].handle;
			if (bunix_ipc_call(LINUX_HANDLE_VFS, &request, &reply) != 0 ||
			    reply.words[0] != 0) {
				return -LINUX_EINVAL;
			}
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
	if (old_fd >= LINUX_MAX_FDS || process->fds[old_fd].kind == 0) {
		return -LINUX_EBADF;
	}

	if (!fixed) {
		for (new_fd = 0; new_fd < LINUX_MAX_FDS; new_fd++) {
			if (process->fds[new_fd].kind == 0) {
				break;
			}
		}
	}
	if (new_fd >= LINUX_MAX_FDS) {
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
	if (fd >= LINUX_MAX_FDS || process->fds[fd].kind == 0) {
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
	if (cmd == LINUX_F_GETFL || cmd == LINUX_F_SETFL) {
		return 0;
	}
	if (cmd == LINUX_F_DUPFD || cmd == LINUX_F_DUPFD_CLOEXEC) {
		if (arg >= LINUX_MAX_FDS) {
			return -LINUX_EINVAL;
		}
		for (u64 new_fd = arg; new_fd < LINUX_MAX_FDS; new_fd++) {
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
		return -LINUX_EMFILE;
	}

	return -LINUX_EINVAL;
}

static long linux_exec_process(struct linux_process *process)
{
	if (process == 0) {
		return -LINUX_ESRCH;
	}

	for (u64 fd = 0; fd < LINUX_MAX_FDS; fd++) {
		if (process->fds[fd].kind != 0 &&
		    (process->fds[fd].flags & LINUX_FD_CLOEXEC) != 0) {
			const long closed = linux_close(process, fd);

			if (closed != 0) {
				return closed;
			}
		}
	}

	return 0;
}

static void linux_close_process_fds(struct linux_process *process)
{
	if (process == 0) {
		return;
	}

	for (u64 fd = 0; fd < LINUX_MAX_FDS; fd++) {
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
	process->session_id = 0;
	process->cwd[0] = '\0';
	for (u64 fd = 0; fd < LINUX_MAX_FDS; fd++) {
		process->fds[fd].handle = 0;
		process->fds[fd].kind = 0;
		process->fds[fd].offset = 0;
		process->fds[fd].size = 0;
		process->fds[fd].flags = 0;
	}
}

int main(void)
{
	const char online[] = "linux-server: online\n";
	const char bad_fd[] = "linux-server: ebadf\n";
	const char open_ok[] = "linux-server: openat\n";
	const char fstat_ok[] = "linux-server: fstat\n";
	const char newfstatat_ok[] = "linux-server: newfstatat\n";
	const char process_ok[] = "linux-server: process\n";
	const char registered_ok[] = "linux-server: registered\n";
	const char wait4_ok[] = "linux-server: wait4\n";
	const char exited_ok[] = "linux-server: exited\n";
	const char close_ok[] = "linux-server: close\n";
	const char exit_group[] = "linux-server: exit_group\n";
	struct bunix_msg message;

	if (register_service(BUNIX_SERVICE_LINUX) != 0) {
		return 1;
	}
	bunix_console_write(online, sizeof(online) - 1);
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
		if (message.type == BUNIX_LINUX_REGISTER_PROCESS) {
			reply.words[0] = (u64)linux_register_process(message.words[0],
								     message.words[1],
								     message.words[2]);
			if ((long)reply.words[0] > 0) {
				bunix_console_write(registered_ok,
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
				bunix_console_write(registered_ok,
						    sizeof(registered_ok) - 1);
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
			bunix_console_write(process_ok, sizeof(process_ok) - 1);
			break;
		case BUNIX_LINUX_GETTID:
			reply.words[0] = process->tid;
			break;
		case BUNIX_LINUX_GETCWD:
			reply.words[0] = (u64)linux_getcwd(process,
							   message.words[0],
							   &reply.words[1],
							   &reply.words[2]);
			break;
		case BUNIX_LINUX_CHDIR:
			reply.words[0] = (u64)linux_chdir(process,
							  message.words[0],
							  message.words[1]);
			break;
		case BUNIX_LINUX_GETUID:
		case BUNIX_LINUX_GETGID:
		case BUNIX_LINUX_GETEUID:
		case BUNIX_LINUX_GETEGID:
			reply.words[0] = (u64)linux_user_credential(process,
								    message.type);
			break;
		case BUNIX_LINUX_GETGROUPS:
			reply.words[0] = (u64)linux_user_groups(process,
								message.words[0],
								&reply.words[1],
								&reply.words[2]);
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
			reply.words[0] = (u64)linux_user_setgroups(process,
								   message.words[0],
								   message.words[1],
								   message.words[2]);
			break;
		case BUNIX_LINUX_GETPPID:
			reply.words[0] = process->ppid;
			break;
		case BUNIX_LINUX_GETPGRP:
			reply.words[0] = process->pgid;
			break;
		case BUNIX_LINUX_GETPGID:
			reply.words[0] = (u64)linux_getpgid(process,
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
					bunix_console_write(wait4_ok,
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
							   message.cap);
			if ((long)reply.words[0] >= 0) {
				bunix_console_write(open_ok, sizeof(open_ok) - 1);
			}
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_FSTAT:
			reply.words[0] = (u64)linux_fstat(process,
							  message.words[0],
							  message.cap);
			if (reply.words[0] == 0) {
				bunix_console_write(fstat_ok,
						    sizeof(fstat_ok) - 1);
			}
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_FCNTL:
			reply.words[0] = (u64)linux_fcntl(process,
							  message.words[0],
							  message.words[1],
							  message.words[2]);
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
			reply.words[0] = (u64)linux_newfstatat(process,
							       message.words[0],
							       message.words[2],
							       message.words[3],
							       message.cap);
			if (reply.words[0] == 0) {
				bunix_console_write(newfstatat_ok,
						    sizeof(newfstatat_ok) - 1);
			}
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
				bunix_console_write(bad_fd, sizeof(bad_fd) - 1);
			}
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		}
		case BUNIX_LINUX_CLOSE:
			reply.words[0] = (u64)linux_close(process,
							  message.words[0]);
			if (reply.words[0] == 0) {
				bunix_console_write(close_ok, sizeof(close_ok) - 1);
			}
			break;
		case BUNIX_LINUX_EXIT_GROUP:
			bunix_console_write(exit_group, sizeof(exit_group) - 1);
			linux_process_exit_code(process, message.words[0], 0);
			bunix_console_write(exited_ok, sizeof(exited_ok) - 1);
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
