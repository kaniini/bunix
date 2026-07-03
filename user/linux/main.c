#include <bunix/libbunix.h>

enum {
	LINUX_HANDLE_VFS = 3,
	LINUX_EBADF = 9,
	LINUX_ENOENT = 2,
	LINUX_EINVAL = 22,
	LINUX_EMFILE = 24,
	LINUX_ESRCH = 3,
	LINUX_MAX_WRITE = 4096,
	LINUX_MAX_PATH = 32,
	LINUX_STAT_SIZE = 144,
	LINUX_MAX_PROCESSES = 16,
	LINUX_S_IFCHR = 0020000,
	LINUX_S_IFREG = 0100000,
	LINUX_FD_CONSOLE = 1,
	LINUX_FD_FILE = 2,
	LINUX_AT_FDCWD = (u64)-100,
};

struct linux_fd {
	u64 handle;
	u64 kind;
	u64 offset;
	u64 size;
};

struct linux_process {
	u64 pid;
	u64 tid;
	u64 bunix_task;
	u64 bunix_thread;
};

static struct linux_fd fds[16];
static struct linux_process processes[LINUX_MAX_PROCESSES];
static char write_buffer[LINUX_MAX_WRITE];
static u64 next_pid = 1;

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

static long alloc_fd(u64 kind, u64 handle, u64 size)
{
	for (u64 fd = 3; fd < sizeof(fds) / sizeof(fds[0]); fd++) {
		if (fds[fd].kind == 0) {
			fds[fd].kind = kind;
			fds[fd].handle = handle;
			fds[fd].offset = 0;
			fds[fd].size = size;
			return (long)fd;
		}
	}

	return -LINUX_EMFILE;
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

	for (u64 i = 0; i < LINUX_MAX_PROCESSES; i++) {
		if (processes[i].pid == 0) {
			const u64 pid = next_pid++;

			processes[i].pid = pid;
			processes[i].tid = pid;
			processes[i].bunix_task = message->sender;
			processes[i].bunix_thread = message->words[1];
			return &processes[i];
		}
	}

	return 0;
}

static long linux_openat(u64 dirfd, u64 path_len, u64 flags, u64 path_buffer)
{
	char path[LINUX_MAX_PATH];
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

	if (dirfd != LINUX_AT_FDCWD || flags != 0 ||
	    path_buffer == 0 || path_len == 0 || path_len > sizeof(path) ||
	    bunix_buffer_read(path_buffer, 0, path, path_len) != 0 ||
	    path[path_len - 1] != '\0') {
		return -LINUX_EINVAL;
	}

	pack_path(&request.words[0], path);
	if (bunix_ipc_call(LINUX_HANDLE_VFS, &request, &reply) != 0 ||
	    reply.words[0] != 0 ||
	    reply.words[3] != BUNIX_VFS_TYPE_REGULAR) {
		return -LINUX_ENOENT;
	}

	return alloc_fd(LINUX_FD_FILE, reply.words[1], reply.words[2]);
}

static long linux_stat_write(u64 stat_buffer, u64 mode, u64 size)
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
	store_u64(stat, 48, size);
	store_u64(stat, 56, 4096);
	store_u64(stat, 64, (size + 511) / 512);

	return bunix_buffer_write(stat_buffer, 0, stat, sizeof(stat)) == 0 ?
		0 : -LINUX_EINVAL;
}

static long linux_fstat(u64 fd, u64 stat_buffer)
{
	if (fd >= sizeof(fds) / sizeof(fds[0]) || fds[fd].kind == 0) {
		return -LINUX_EBADF;
	}

	if (fds[fd].kind == LINUX_FD_CONSOLE) {
		return linux_stat_write(stat_buffer, LINUX_S_IFCHR | 0600, 0);
	}

	return linux_stat_write(stat_buffer, LINUX_S_IFREG | 0444,
				fds[fd].size);
}

static long linux_newfstatat(u64 dirfd, u64 word0, u64 word1, u64 stat_buffer)
{
	char path[16];
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

	if (dirfd != LINUX_AT_FDCWD || stat_buffer == 0) {
		return -LINUX_EINVAL;
	}

	unpack_path(path, word0, word1);
	pack_path(&request.words[0], path);
	if (bunix_ipc_call(LINUX_HANDLE_VFS, &request, &reply) != 0 ||
	    reply.words[0] != 0 ||
	    reply.words[3] != BUNIX_VFS_TYPE_REGULAR) {
		return -LINUX_ENOENT;
	}

	const long result = linux_stat_write(stat_buffer, LINUX_S_IFREG | 0444,
					     reply.words[2]);
	request.type = BUNIX_VFS_CLOSE;
	request.words[0] = reply.words[1];
	(void)bunix_ipc_call(LINUX_HANDLE_VFS, &request, &reply);
	return result;
}

static long linux_read(u64 fd, u64 len, u64 buffer)
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

	if (fd >= sizeof(fds) / sizeof(fds[0]) ||
	    fds[fd].kind != LINUX_FD_FILE ||
	    buffer == 0) {
		return -LINUX_EBADF;
	}

	request.words[0] = fds[fd].handle;
	request.words[1] = fds[fd].offset;
	if (bunix_ipc_call(LINUX_HANDLE_VFS, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return -LINUX_EINVAL;
	}

	fds[fd].offset += reply.words[1];
	return (long)reply.words[1];
}

static long linux_close(u64 fd)
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

	if (fd >= sizeof(fds) / sizeof(fds[0]) ||
	    fds[fd].kind == 0 ||
	    fd < 3) {
		return -LINUX_EBADF;
	}

	if (fds[fd].kind == LINUX_FD_FILE) {
		request.words[0] = fds[fd].handle;
		if (bunix_ipc_call(LINUX_HANDLE_VFS, &request, &reply) != 0 ||
		    reply.words[0] != 0) {
			return -LINUX_EINVAL;
		}
	}

	fds[fd].kind = 0;
	fds[fd].handle = 0;
	fds[fd].offset = 0;
	fds[fd].size = 0;
	return 0;
}

int main(void)
{
	const char online[] = "linux-server: online\n";
	const char write_ok[] = "linux-server: write\n";
	const char bad_fd[] = "linux-server: ebadf\n";
	const char open_ok[] = "linux-server: openat\n";
	const char read_ok[] = "linux-server: read\n";
	const char fstat_ok[] = "linux-server: fstat\n";
	const char newfstatat_ok[] = "linux-server: newfstatat\n";
	const char process_ok[] = "linux-server: process\n";
	const char close_ok[] = "linux-server: close\n";
	const char exit_group[] = "linux-server: exit_group\n";
	struct bunix_msg message;

	fds[1].handle = BUNIX_HANDLE_CONSOLE;
	fds[1].kind = LINUX_FD_CONSOLE;
	fds[2].handle = BUNIX_HANDLE_CONSOLE;
	fds[2].kind = LINUX_FD_CONSOLE;

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

		if (bunix_ipc_recv(BUNIX_HANDLE_SELF, &message) != 0 ||
		    message.protocol != BUNIX_PROTO_LINUX) {
			continue;
		}

		reply.type = message.type;
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
		case BUNIX_LINUX_OPENAT:
			reply.words[0] = (u64)linux_openat(message.words[0],
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
			reply.words[0] = (u64)linux_fstat(message.words[0],
							  message.cap);
			if (reply.words[0] == 0) {
				bunix_console_write(fstat_ok,
						    sizeof(fstat_ok) - 1);
			}
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_NEWFSTATAT:
			reply.words[0] = (u64)linux_newfstatat(message.words[0],
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
			reply.words[0] = (u64)linux_read(message.words[0],
							 message.words[1],
							 message.cap);
			if ((long)reply.words[0] >= 0) {
				bunix_console_write(read_ok, sizeof(read_ok) - 1);
			}
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_LINUX_WRITE: {
			const u64 fd = message.words[0];
			const u64 len = message.words[1];

			if (fd >= sizeof(fds) / sizeof(fds[0]) ||
			    fds[fd].kind != LINUX_FD_CONSOLE) {
				bunix_console_write(bad_fd, sizeof(bad_fd) - 1);
				reply.words[0] = (u64)-LINUX_EBADF;
			} else if (message.cap == 0 || len > sizeof(write_buffer) ||
				   bunix_buffer_read(message.cap, 0,
						     write_buffer, len) != 0) {
				reply.words[0] = (u64)-LINUX_EINVAL;
			} else {
				const struct bunix_msg console_message = {
					.protocol = BUNIX_PROTO_CONSOLE,
					.type = BUNIX_CONSOLE_WRITE,
					.sender = 0,
					.cap_rights = 0,
					.reply = 0,
					.cap = 0,
					.words = { (u64)write_buffer, len, 0, 0 },
				};

				bunix_ipc_send(fds[fd].handle, &console_message);
				bunix_console_write(write_ok, sizeof(write_ok) - 1);
				reply.words[0] = len;
			}
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		}
		case BUNIX_LINUX_CLOSE:
			reply.words[0] = (u64)linux_close(message.words[0]);
			if (reply.words[0] == 0) {
				bunix_console_write(close_ok, sizeof(close_ok) - 1);
			}
			break;
		case BUNIX_LINUX_EXIT_GROUP:
			bunix_console_write(exit_group, sizeof(exit_group) - 1);
			reply.words[0] = 0;
			break;
		default:
			reply.words[0] = (u64)-1;
			break;
		}

		if (message.reply != 0) {
			bunix_ipc_send(message.reply, &reply);
		}
	}
}
