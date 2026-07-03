#include <bunix/syscall.h>

enum {
	VFS_HANDLE_NAMES = 3,
	ROOTFS_MAX_PATH = 32,
	ROOTFS_MAX_ENTRIES = 8,
	ROOTFS_DATA_OFFSET = 104,
	ROOTFS_HELLO_SIZE = 15,
};

struct rootfs_entry {
	char path[ROOTFS_MAX_PATH];
	u64 offset;
	u64 size;
};

static struct rootfs_entry root_entries[ROOTFS_MAX_ENTRIES];
static unsigned int root_entry_count;

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

	if (bunix_ipc_call(VFS_HANDLE_NAMES, &request, &reply) != 0) {
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

	if (bunix_ipc_call(VFS_HANDLE_NAMES, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return 0;
	}

	return reply.cap;
}

static void pack_bytes(u64 *words, const unsigned char *data, u64 len)
{
	words[0] = 0;
	words[1] = 0;

	for (u64 i = 0; i < len && i < BUNIX_IPC_DATA_BYTES; i++) {
		const u64 slot = i / 8;
		const u64 shift = (i % 8) * 8;

		words[slot] |= ((u64)data[i]) << shift;
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

static int path_eq(const char *left, const char *right)
{
	for (u64 i = 0; i < ROOTFS_MAX_PATH; i++) {
		if (left[i] != right[i]) {
			return 0;
		}
		if (left[i] == '\0') {
			return 1;
		}
	}

	return 1;
}

static int block_read_bytes(u64 block, u64 offset, unsigned char *buffer,
			    u64 len)
{
	u64 done = 0;

	while (done < len) {
		u64 chunk = len - done;
		struct bunix_msg request = {
			.protocol = BUNIX_PROTO_BLOCK,
			.type = BUNIX_BLOCK_READ,
			.sender = 0,
			.cap_rights = 0,
			.reply = 0,
			.cap = 0,
			.words = { offset + done, chunk, 0, 0 },
		};
		struct bunix_msg reply;

		if (chunk > BUNIX_IPC_DATA_BYTES) {
			chunk = BUNIX_IPC_DATA_BYTES;
		}
		request.words[1] = chunk;

		if (bunix_ipc_call(block, &request, &reply) != 0 ||
		    reply.words[0] != 0 ||
		    reply.words[1] == 0 ||
		    reply.words[1] > chunk) {
			return -1;
		}

		unpack_bytes(buffer + done, &reply.words[2], reply.words[1]);
		done += reply.words[1];
	}

	return 0;
}

static int block_read_buffer(u64 block, u64 offset, u64 buffer, u64 len)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_BLOCK,
		.type = BUNIX_BLOCK_READ_BUFFER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_SEND,
		.reply = 0,
		.cap = buffer,
		.words = { offset, len, 0, 0 },
	};
	struct bunix_msg reply;

	if (bunix_ipc_call(block, &request, &reply) != 0 ||
	    reply.words[0] != 0 ||
	    reply.words[1] > len) {
		return -1;
	}

	return (int)reply.words[1];
}

static int rootfs_mount(u64 block)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_BLOCK,
		.type = BUNIX_BLOCK_GET_INFO,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { 0, 0, 0, 0 },
	};
	struct bunix_msg reply;
	const char hello[] = "/hello.txt";
	const char first[] = "/bin/first";

	if (bunix_ipc_call(block, &request, &reply) != 0 ||
	    reply.words[0] != 0 ||
	    reply.words[1] <= ROOTFS_DATA_OFFSET + ROOTFS_HELLO_SIZE) {
		return -1;
	}

	for (u64 i = 0; i < ROOTFS_MAX_ENTRIES; i++) {
		for (u64 j = 0; j < ROOTFS_MAX_PATH; j++) {
			root_entries[i].path[j] = '\0';
		}
	}

	for (u64 i = 0; hello[i] != '\0'; i++) {
		root_entries[0].path[i] = hello[i];
	}
	root_entries[0].offset = ROOTFS_DATA_OFFSET;
	root_entries[0].size = ROOTFS_HELLO_SIZE;

	for (u64 i = 0; first[i] != '\0'; i++) {
		root_entries[1].path[i] = first[i];
	}
	root_entries[1].offset = ROOTFS_DATA_OFFSET + ROOTFS_HELLO_SIZE;
	root_entries[1].size = reply.words[1] - root_entries[1].offset;
	root_entry_count = 2;
	return 0;
}

static int rootfs_find(const char *path, struct rootfs_entry *entry)
{
	for (u64 i = 0; i < root_entry_count; i++) {
		if (path_eq(root_entries[i].path, path)) {
			*entry = root_entries[i];
			return 0;
		}
	}

	return -1;
}

int main(void)
{
	const char online[] = "vfs: online\n";
	const char ready[] = "vfs: mounted block\n";
	struct bunix_msg message;
	u64 block = 0;

	bunix_console_write(online, sizeof(online) - 1);
	register_service(BUNIX_SERVICE_VFS, BUNIX_HANDLE_SELF);
	block = resolve_service(BUNIX_SERVICE_BLOCK, BUNIX_RIGHT_SEND);
	if (block == 0 || rootfs_mount(block) != 0) {
		return 1;
	}
	bunix_console_write(ready, sizeof(ready) - 1);

	for (;;) {
		struct bunix_msg reply = {
			.protocol = BUNIX_PROTO_VFS,
			.type = 0,
			.sender = 0,
			.cap_rights = 0,
			.reply = 0,
			.cap = 0,
			.words = { 0, 0, 0, 0 },
		};

		if (bunix_ipc_recv(BUNIX_HANDLE_SELF, &message) != 0 ||
		    message.protocol != BUNIX_PROTO_VFS) {
			continue;
		}

		reply.type = message.type;
		switch (message.type) {
		case BUNIX_VFS_READ: {
			struct bunix_msg request = {
				.protocol = BUNIX_PROTO_BLOCK,
				.type = BUNIX_BLOCK_READ,
				.sender = 0,
				.cap_rights = 0,
				.reply = 0,
				.cap = 0,
				.words = { message.words[0], message.words[1], 0, 0 },
			};
			struct bunix_msg block_reply;

			if (bunix_ipc_call(block, &request, &block_reply) == 0 &&
			    block_reply.words[0] == 0) {
				reply.words[0] = 0;
				reply.words[1] = block_reply.words[1];
				reply.words[2] = block_reply.words[2];
				reply.words[3] = block_reply.words[3];
			} else {
				reply.words[0] = (u64)-1;
			}
			break;
		}
		case BUNIX_VFS_READ_PATH: {
			char path[ROOTFS_MAX_PATH];
			struct rootfs_entry entry;
			const u64 offset = message.words[2];
			u64 len = message.words[3];
			unsigned char buffer[BUNIX_IPC_DATA_BYTES];

			for (u64 i = 0; i < sizeof(path); i++) {
				path[i] = '\0';
			}
			unpack_bytes((unsigned char *)path, &message.words[0], 16);

			if (rootfs_find(path, &entry) != 0) {
				reply.words[0] = (u64)-1;
				break;
			}

			if (offset >= entry.size) {
				len = 0;
			} else if (len > entry.size - offset) {
				len = entry.size - offset;
			}
			if (len > BUNIX_IPC_DATA_BYTES) {
				len = BUNIX_IPC_DATA_BYTES;
			}

			reply.words[0] = 0;
			reply.words[1] = len;
			if (len != 0 &&
			    block_read_bytes(block, entry.offset + offset, buffer,
					     len) != 0) {
				reply.words[0] = (u64)-1;
				reply.words[1] = 0;
			} else {
				pack_bytes(&reply.words[2], buffer, len);
			}
			break;
		}
		case BUNIX_VFS_READ_PATH_BUFFER: {
			char path[ROOTFS_MAX_PATH];
			struct rootfs_entry entry;
			const u64 offset = message.words[2];
			u64 len = message.words[3];
			int read_len;

			for (u64 i = 0; i < sizeof(path); i++) {
				path[i] = '\0';
			}
			unpack_bytes((unsigned char *)path, &message.words[0], 16);

			if (message.cap == 0 ||
			    (message.cap_rights & (BUNIX_RIGHT_SEND |
						   BUNIX_RIGHT_DUP)) !=
			    (BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP) ||
			    rootfs_find(path, &entry) != 0) {
				reply.words[0] = (u64)-1;
				break;
			}

			if (offset >= entry.size) {
				len = 0;
			} else if (len > entry.size - offset) {
				len = entry.size - offset;
			}

			read_len = block_read_buffer(block, entry.offset + offset,
						     message.cap, len);
			if (read_len < 0) {
				reply.words[0] = (u64)-1;
				reply.words[1] = 0;
			} else {
				reply.words[0] = 0;
				reply.words[1] = (u64)read_len;
			}
			break;
		}
		default:
			reply.words[0] = (u64)-1;
			break;
		}

		if (message.reply != 0) {
			bunix_ipc_send(message.reply, &reply);
		}
	}
}
