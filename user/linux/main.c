#include <bunix/libbunix.h>

enum {
	LINUX_HANDLE_VFS = 3,
	LINUX_HANDLE_NAMES = 4,
	LINUX_EBADF = 9,
	LINUX_ENOENT = 2,
	LINUX_EINVAL = 22,
	LINUX_EMFILE = 24,
	LINUX_ESRCH = 3,
	LINUX_ECHILD = 10,
	LINUX_MAX_WRITE = 4096,
	LINUX_MAX_PATH = 32,
	LINUX_STAT_SIZE = 144,
	LINUX_MAX_PROCESSES = 16,
	LINUX_MAX_FDS = 16,
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
	u64 ppid;
	u64 bunix_task;
	u64 bunix_thread;
	u64 exited;
	u64 exit_status;
	u64 waited;
	u64 waiter;
	u64 wait_buffer;
	u64 wait_pid;
	struct linux_fd fds[LINUX_MAX_FDS];
};

static struct linux_process processes[LINUX_MAX_PROCESSES];
static char write_buffer[LINUX_MAX_WRITE];
static u64 next_pid = 1;

static void linux_process_reset(struct linux_process *process);

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

static void notify_proc_exit(u64 linux_pid, u64 status)
{
	const u64 proc = resolve_service(BUNIX_SERVICE_PROC, BUNIX_RIGHT_SEND);
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_PROC,
		.type = BUNIX_PROC_EXIT,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { linux_pid, status, 0, 0 },
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

static void linux_process_init_fds(struct linux_process *process)
{
	for (u64 fd = 0; fd < LINUX_MAX_FDS; fd++) {
		process->fds[fd].handle = 0;
		process->fds[fd].kind = 0;
		process->fds[fd].offset = 0;
		process->fds[fd].size = 0;
	}

	process->fds[1].handle = BUNIX_HANDLE_CONSOLE;
	process->fds[1].kind = LINUX_FD_CONSOLE;
	process->fds[2].handle = BUNIX_HANDLE_CONSOLE;
	process->fds[2].kind = LINUX_FD_CONSOLE;
}

static long alloc_fd(struct linux_process *process, u64 kind, u64 handle,
		     u64 size)
{
	for (u64 fd = 3; fd < LINUX_MAX_FDS; fd++) {
		if (process->fds[fd].kind == 0) {
			process->fds[fd].kind = kind;
			process->fds[fd].handle = handle;
			process->fds[fd].offset = 0;
			process->fds[fd].size = size;
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

	return 0;
}

static long linux_register_process(u64 bunix_task, u64 ppid)
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
			processes[i].bunix_task = bunix_task;
			processes[i].bunix_thread = 0;
			processes[i].exited = 0;
			processes[i].exit_status = 0;
			processes[i].waited = 0;
			processes[i].waiter = 0;
			processes[i].wait_buffer = 0;
			processes[i].wait_pid = 0;
			linux_process_init_fds(&processes[i]);
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
			processes[i].bunix_task = child_task;
			processes[i].bunix_thread = 0;
			processes[i].exited = 0;
			processes[i].exit_status = 0;
			processes[i].waited = 0;
			processes[i].waiter = 0;
			processes[i].wait_buffer = 0;
			processes[i].wait_pid = 0;
			for (u64 fd = 0; fd < LINUX_MAX_FDS; fd++) {
				processes[i].fds[fd] = parent->fds[fd];
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

static int linux_child_matches(const struct linux_process *parent,
			       const struct linux_process *child, long pid)
{
	if (parent == 0 || child == 0 ||
	    child->ppid != parent->pid ||
	    child->waited) {
		return 0;
	}

	return pid == -1 || child->pid == (u64)pid;
}

static int linux_store_wait_status(u64 buffer, u64 exit_status)
{
	char status[4];
	const unsigned int value = (unsigned int)(exit_status << 8);

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

	if (pid != -1 || options != 0) {
		return -LINUX_EINVAL;
	}

	for (u64 i = 0; i < LINUX_MAX_PROCESSES; i++) {
		if (!linux_child_matches(parent, &processes[i], pid)) {
			continue;
		}

		candidate = &processes[i];
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

	if (reply == 0) {
		return -LINUX_EINVAL;
	}

	parent->waiter = reply;
	parent->wait_buffer = status_buffer;
	parent->wait_pid = (u64)pid;
	return 0;
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

static long linux_openat(struct linux_process *process, u64 dirfd,
			 u64 path_len, u64 flags, u64 path_buffer)
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

	return alloc_fd(process, LINUX_FD_FILE, reply.words[1], reply.words[2]);
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

static long linux_fstat(struct linux_process *process, u64 fd, u64 stat_buffer)
{
	if (fd >= LINUX_MAX_FDS || process->fds[fd].kind == 0) {
		return -LINUX_EBADF;
	}

	if (process->fds[fd].kind == LINUX_FD_CONSOLE) {
		return linux_stat_write(stat_buffer, LINUX_S_IFCHR | 0600, 0);
	}

	return linux_stat_write(stat_buffer, LINUX_S_IFREG | 0444,
				process->fds[fd].size);
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

static long linux_read(struct linux_process *process, u64 fd, u64 len,
		       u64 buffer)
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

	if (fd >= LINUX_MAX_FDS ||
	    process->fds[fd].kind != LINUX_FD_FILE ||
	    buffer == 0) {
		return -LINUX_EBADF;
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
	    process->fds[fd].kind == 0 ||
	    fd < 3) {
		return -LINUX_EBADF;
	}

	if (process->fds[fd].kind == LINUX_FD_FILE) {
		request.words[0] = process->fds[fd].handle;
		if (bunix_ipc_call(LINUX_HANDLE_VFS, &request, &reply) != 0 ||
		    reply.words[0] != 0) {
			return -LINUX_EINVAL;
		}
	}

	process->fds[fd].kind = 0;
	process->fds[fd].handle = 0;
	process->fds[fd].offset = 0;
	process->fds[fd].size = 0;
	return 0;
}

static void linux_close_process_fds(struct linux_process *process)
{
	if (process == 0) {
		return;
	}

	for (u64 fd = 3; fd < LINUX_MAX_FDS; fd++) {
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

	linux_close_process_fds(process);
	process->pid = 0;
	process->tid = 0;
	process->ppid = 0;
	process->bunix_task = 0;
	process->bunix_thread = 0;
	process->exited = 0;
	process->exit_status = 0;
	process->waited = 0;
	process->waiter = 0;
	process->wait_buffer = 0;
	process->wait_pid = 0;
	for (u64 fd = 0; fd < LINUX_MAX_FDS; fd++) {
		process->fds[fd].handle = 0;
		process->fds[fd].kind = 0;
		process->fds[fd].offset = 0;
		process->fds[fd].size = 0;
	}
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
		case BUNIX_LINUX_WAIT4: {
			const long waited = linux_wait4(process,
						       (long)message.words[0],
						       message.words[1],
						       message.cap,
						       message.reply);

			if (waited == 0) {
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
			reply.words[0] = (u64)linux_read(process,
							 message.words[0],
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

			if (fd >= LINUX_MAX_FDS ||
			    process->fds[fd].kind != LINUX_FD_CONSOLE) {
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

				bunix_ipc_send(process->fds[fd].handle,
					       &console_message);
				bunix_console_write(write_ok, sizeof(write_ok) - 1);
				reply.words[0] = len;
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
			linux_close_process_fds(process);
			process->exited = 1;
			process->exit_status = message.words[0];
			bunix_console_write(exit_group, sizeof(exit_group) - 1);
			bunix_console_write(exited_ok, sizeof(exited_ok) - 1);
			notify_proc_exit(process->pid, process->exit_status);
			linux_wake_parent(process);
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
