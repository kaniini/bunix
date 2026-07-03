#include <bunix/syscall.h>

enum {
	PROC_HANDLE_CONSOLE = 2,
	PROC_HANDLE_NAMES = 3,
	PROC_HANDLE_TIME = 4,
	PROC_SEGMENT_MAX = 4096,
	PROC_MAX_PHDRS = 8,
	ELF_MAGIC = 0x464c457f,
	ELFCLASS64 = 2,
	ELFDATA2LSB = 1,
	ET_EXEC = 2,
	EM_X86_64 = 62,
	PT_LOAD = 1,
	PF_W = 1 << 1,
};

struct process {
	u64 pid;
	u64 status;
	u64 exited;
	u64 waiter;
};

struct elf64_ehdr {
	unsigned char ident[16];
	unsigned short type;
	unsigned short machine;
	unsigned int version;
	u64 entry;
	u64 phoff;
	u64 shoff;
	unsigned int flags;
	unsigned short ehsize;
	unsigned short phentsize;
	unsigned short phnum;
	unsigned short shentsize;
	unsigned short shnum;
	unsigned short shstrndx;
} __attribute__((packed));

struct elf64_phdr {
	unsigned int type;
	unsigned int flags;
	u64 offset;
	u64 vaddr;
	u64 paddr;
	u64 filesz;
	u64 memsz;
	u64 align;
} __attribute__((packed));

struct vfs_file {
	u64 handle;
	u64 size;
	u64 type;
};

static struct process first_process;
static u64 next_pid = 1;
static unsigned char segment_buffer[PROC_SEGMENT_MAX];

static long register_service(u64 service, u64 handle)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_NAMES,
		.type = BUNIX_NAMES_REGISTER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP,
		.reply = 0,
		.cap = handle,
		.words = { BUNIX_NAMES_ROOT, service, 0, 0 },
	};
	struct bunix_msg reply;

	if (bunix_ipc_call(PROC_HANDLE_NAMES, &request, &reply) != 0) {
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

	if (bunix_ipc_call(PROC_HANDLE_NAMES, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return 0;
	}

	return reply.cap;
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

static void unpack_bytes(unsigned char *out, const u64 *words, u64 len)
{
	for (u64 i = 0; i < len; i++) {
		const u64 slot = i / 8;
		const u64 shift = (i % 8) * 8;

		out[i] = (unsigned char)((words[slot] >> shift) & 0xff);
	}
}

static unsigned int read_magic(const unsigned char *ident)
{
	return ((unsigned int)ident[0]) |
	       ((unsigned int)ident[1] << 8) |
	       ((unsigned int)ident[2] << 16) |
	       ((unsigned int)ident[3] << 24);
}

static long vfs_open(u64 vfs, const char *path, struct vfs_file *file)
{
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

	if (file == 0) {
		return -1;
	}

	pack_path(&request.words[0], path);
	if (bunix_ipc_call(vfs, &request, &reply) != 0 ||
	    reply.words[0] != 0 ||
	    reply.words[1] == 0 ||
	    reply.words[3] != BUNIX_VFS_TYPE_REGULAR) {
		return -1;
	}

	file->handle = reply.words[1];
	file->size = reply.words[2];
	file->type = reply.words[3];
	return 0;
}

static long vfs_stat(u64 vfs, struct vfs_file *file)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_STAT,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { file != 0 ? file->handle : 0, 0, 0, 0 },
	};
	struct bunix_msg reply;

	if (file == 0 ||
	    bunix_ipc_call(vfs, &request, &reply) != 0 ||
	    reply.words[0] != 0 ||
	    reply.words[2] != BUNIX_VFS_TYPE_REGULAR) {
		return -1;
	}

	file->size = reply.words[1];
	file->type = reply.words[2];
	return 0;
}

static void vfs_close(u64 vfs, u64 file)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_CLOSE,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { file, 0, 0, 0 },
	};
	struct bunix_msg reply;

	bunix_ipc_call(vfs, &request, &reply);
}

static long vfs_read_file(u64 vfs, u64 file, u64 file_size, u64 offset,
			  unsigned char *buffer, u64 len, u64 io_buffer)
{
	u64 done = 0;

	if (offset > file_size || len > file_size - offset) {
		return -1;
	}

	while (done < len) {
		u64 chunk = len - done;
		struct bunix_msg request = {
			.protocol = BUNIX_PROTO_VFS,
			.type = BUNIX_VFS_READ_FILE_BUFFER,
			.sender = 0,
			.cap_rights = BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP,
			.reply = 0,
			.cap = io_buffer,
			.words = { file, offset + done, chunk, 0 },
		};
		struct bunix_msg reply;

		if (chunk > PROC_SEGMENT_MAX) {
			chunk = PROC_SEGMENT_MAX;
		}
		request.words[2] = chunk;
		if (bunix_ipc_call(vfs, &request, &reply) != 0 ||
		    reply.words[0] != 0 ||
		    reply.words[1] > chunk) {
			return -1;
		}

		if (reply.words[1] == 0) {
			return -1;
		}

		if (bunix_buffer_read(io_buffer, 0, buffer + done,
				      reply.words[1]) != 0) {
			return -1;
		}
		done += reply.words[1];
	}

	return 0;
}

static long exec_first_path(u64 vfs, const char *path)
{
	const struct bunix_launch_cap caps[] = {
		{ PROC_HANDLE_CONSOLE, BUNIX_RIGHT_SEND, 0 },
		{ PROC_HANDLE_TIME, BUNIX_RIGHT_SEND, 0 },
		{ BUNIX_HANDLE_SELF, BUNIX_RIGHT_SEND, 0 },
	};
	struct elf64_ehdr ehdr;
	struct elf64_phdr phdrs[PROC_MAX_PHDRS];
	struct vfs_file file = { 0, 0, 0 };
	long io_buffer;
	long task;

	io_buffer = bunix_buffer_create(PROC_SEGMENT_MAX);
	if (io_buffer <= 0 ||
	    vfs_open(vfs, path, &file) != 0 ||
	    vfs_stat(vfs, &file) != 0 ||
	    vfs_read_file(vfs, file.handle, file.size, 0,
			  (unsigned char *)&ehdr, sizeof(ehdr),
			  (u64)io_buffer) != 0 ||
	    read_magic(ehdr.ident) != ELF_MAGIC ||
	    ehdr.ident[4] != ELFCLASS64 ||
	    ehdr.ident[5] != ELFDATA2LSB ||
	    ehdr.type != ET_EXEC ||
	    ehdr.machine != EM_X86_64 ||
	    ehdr.phnum > PROC_MAX_PHDRS ||
	    ehdr.phentsize != sizeof(phdrs[0]) ||
	    vfs_read_file(vfs, file.handle, file.size, ehdr.phoff,
			  (unsigned char *)phdrs,
			  (u64)ehdr.phnum * sizeof(phdrs[0]),
			  (u64)io_buffer) != 0) {
		if (file.handle != 0) {
			vfs_close(vfs, file.handle);
		}
		return -1;
	}

	task = bunix_task_create("first");
	if (task < 0) {
		vfs_close(vfs, file.handle);
		return -1;
	}

	for (u64 i = 0; i < sizeof(caps) / sizeof(caps[0]); i++) {
		if (bunix_task_grant((u64)task, caps[i].handle,
				     caps[i].rights) != 0) {
			vfs_close(vfs, file.handle);
			return -1;
		}
	}

	for (u64 i = 0; i < ehdr.phnum; i++) {
		const struct elf64_phdr *phdr = &phdrs[i];

		if (phdr->type != PT_LOAD) {
			continue;
		}

		if (phdr->filesz > phdr->memsz ||
		    phdr->filesz > sizeof(segment_buffer) ||
		    phdr->vaddr + phdr->memsz < phdr->vaddr ||
		    vfs_read_file(vfs, file.handle, file.size, phdr->offset,
				  segment_buffer, phdr->filesz,
				  (u64)io_buffer) != 0 ||
		    bunix_task_map((u64)task, phdr->vaddr,
				   segment_buffer, phdr->filesz, phdr->memsz,
				   (phdr->flags & PF_W) != 0) != 0) {
			vfs_close(vfs, file.handle);
			return -1;
		}
	}

	vfs_close(vfs, file.handle);
	return bunix_task_start((u64)task, ehdr.entry);
}

static void reply_status(u64 reply_handle, u64 pid, u64 status)
{
	struct bunix_msg reply = {
		.protocol = BUNIX_PROTO_PROC,
		.type = BUNIX_PROC_WAIT,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { 0, pid, status, 0 },
	};

	bunix_ipc_send(reply_handle, &reply);
}

static long spawn_first(const char *path)
{
	u64 vfs;

	if (first_process.pid != 0) {
		return -1;
	}

	vfs = resolve_service(BUNIX_SERVICE_VFS, BUNIX_RIGHT_SEND);
	if (vfs == 0) {
		return -1;
	}

	first_process.pid = next_pid++;
	first_process.status = 0;
	first_process.exited = 0;
	first_process.waiter = 0;

	if (exec_first_path(vfs, path) != 0) {
		first_process.pid = 0;
		return -1;
	}

	return 0;
}

int main(void)
{
	const char online[] = "proc: online\n";
	const char ready[] = "proc: ready\n";
	const char exec[] = "proc: exec /bin/first\n";
	const char spawned[] = "proc: spawned pid=1\n";
	const char exited[] = "proc: exited pid=1 status=0\n";
	const char waited[] = "proc: wait pid=1 status=0\n";
	struct bunix_msg message;

	bunix_console_write(online, sizeof(online) - 1);
	if (register_service(BUNIX_SERVICE_PROC, BUNIX_HANDLE_SELF) != 0) {
		return 1;
	}
	bunix_console_write(ready, sizeof(ready) - 1);

	for (;;) {
		struct bunix_msg reply = {
			.protocol = BUNIX_PROTO_PROC,
			.type = 0,
			.sender = 0,
			.cap_rights = 0,
			.reply = 0,
			.cap = 0,
			.words = { 0, 0, 0, 0 },
		};
		int should_reply = 1;

		if (bunix_ipc_recv(BUNIX_HANDLE_SELF, &message) != 0 ||
		    message.protocol != BUNIX_PROTO_PROC) {
			continue;
		}

		reply.type = message.type;
		switch (message.type) {
		case BUNIX_PROC_SPAWN: {
			char path[16];

			for (u64 i = 0; i < sizeof(path); i++) {
				path[i] = '\0';
			}
			unpack_bytes((unsigned char *)path, &message.words[0],
				     sizeof(path));
			if (spawn_first(path) == 0) {
				reply.words[0] = 0;
				reply.words[1] = first_process.pid;
				bunix_console_write(exec, sizeof(exec) - 1);
				bunix_console_write(spawned, sizeof(spawned) - 1);
			} else {
				reply.words[0] = (u64)-1;
			}
			break;
		}
		case BUNIX_PROC_WAIT:
			if (first_process.pid == 0) {
				reply.words[0] = (u64)-1;
			} else if (first_process.exited) {
				reply.words[0] = 0;
				reply.words[1] = first_process.pid;
				reply.words[2] = first_process.status;
				bunix_console_write(waited, sizeof(waited) - 1);
			} else if (message.reply != 0) {
				first_process.waiter = message.reply;
				should_reply = 0;
			} else {
				reply.words[0] = (u64)-1;
			}
			break;
		case BUNIX_PROC_EXIT:
			if (first_process.pid == message.words[0]) {
				first_process.status = message.words[1];
				first_process.exited = 1;
				reply.words[0] = 0;
				bunix_console_write(exited, sizeof(exited) - 1);
				if (first_process.waiter != 0) {
					reply_status(first_process.waiter,
						     first_process.pid,
						     first_process.status);
					first_process.waiter = 0;
					bunix_console_write(waited,
							    sizeof(waited) - 1);
				}
			} else {
				reply.words[0] = (u64)-1;
			}
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
