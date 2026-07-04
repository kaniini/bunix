#include <bunix/libbunix.h>
#include <bunix/id_table.h>

enum {
	VFS_HANDLE_NAMES = 3,
	ROOTFS_MAGIC = 0x30534652,
	ROOTFS_MAX_PATH = 32,
	VFS_REMOTE_PROCFS = 1ull << 63,
};

struct rootfs_header {
	unsigned int magic;
	unsigned int entries;
};

struct rootfs_entry {
	char path[ROOTFS_MAX_PATH];
	u64 offset;
	u64 size;
	unsigned int uid;
	unsigned int gid;
	unsigned int mode;
	unsigned int type;
};

static struct rootfs_entry *root_entries;
static unsigned int root_entry_count;
static struct bunix_id_table open_files;
static u64 user_service;
static u64 procfs_service;

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

static u64 service_user(void)
{
	if (user_service == 0) {
		user_service = resolve_service(BUNIX_SERVICE_USER,
					       BUNIX_RIGHT_SEND);
	}

	return user_service;
}

static u64 service_procfs(void)
{
	if (procfs_service == 0) {
		procfs_service = resolve_service(BUNIX_SERVICE_PROCFS,
						  BUNIX_RIGHT_SEND);
	}

	return procfs_service;
}

static int path_is_procfs(const char *path)
{
	if (path[0] != '/' || path[1] != 'p' || path[2] != 'r' ||
	    path[3] != 'o' || path[4] != 'c') {
		return 0;
	}
	return path[5] == '\0' || path[5] == '/';
}

static int handle_is_procfs(u64 handle)
{
	return (handle & VFS_REMOTE_PROCFS) != 0;
}

static u64 procfs_handle(u64 handle)
{
	return handle & ~VFS_REMOTE_PROCFS;
}

static int forward_procfs(struct bunix_msg *message, struct bunix_msg *reply)
{
	const u64 procfs = service_procfs();

	if (procfs == 0) {
		reply->words[0] = BUNIX_VFS_ERR_NOENT;
		return -1;
	}
	if (handle_is_procfs(message->words[0])) {
		message->words[0] = procfs_handle(message->words[0]);
	}
	if (bunix_ipc_call(procfs, message, reply) != 0) {
		reply->words[0] = (u64)-1;
		return -1;
	}
	if (message->type == BUNIX_VFS_OPEN && reply->words[0] == 0) {
		reply->words[1] |= VFS_REMOTE_PROCFS;
	}
	return 0;
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

static u64 str_len(const char *text)
{
	u64 len = 0;

	while (len < ROOTFS_MAX_PATH && text[len] != '\0') {
		len++;
	}

	return len;
}

static int entry_is_child_of(const struct rootfs_entry *entry,
			     const char *directory)
{
	const u64 dir_len = str_len(directory);
	u64 pos;

	if (entry == 0 || directory == 0 || path_eq(entry->path, directory)) {
		return 0;
	}
	if (path_eq(directory, "/")) {
		if (entry->path[0] != '/' || entry->path[1] == '\0') {
			return 0;
		}
		pos = 1;
	} else {
		for (u64 i = 0; i < dir_len; i++) {
			if (entry->path[i] != directory[i]) {
				return 0;
			}
		}
		if (entry->path[dir_len] != '/') {
			return 0;
		}
		pos = dir_len + 1;
	}

	while (entry->path[pos] != '\0') {
		if (entry->path[pos] == '/') {
			return 0;
		}
		pos++;
	}
	return 1;
}

static const char *entry_name_in_directory(const struct rootfs_entry *entry,
					   const char *directory)
{
	const u64 dir_len = str_len(directory);

	return path_eq(directory, "/") ? entry->path + 1 :
	       entry->path + dir_len + 1;
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
	struct rootfs_header header;

	if (bunix_ipc_call(block, &request, &reply) != 0 ||
	    reply.words[0] != 0 ||
	    reply.words[1] < sizeof(header) ||
	    block_read_bytes(block, 0, (unsigned char *)&header,
			     sizeof(header)) != 0 ||
	    header.magic != ROOTFS_MAGIC ||
	    header.entries == 0 ||
	    reply.words[1] < sizeof(header) +
	    (u64)header.entries * sizeof(struct rootfs_entry)) {
		return -1;
	}

	root_entries = (struct rootfs_entry *)
		bunix_calloc(header.entries, sizeof(root_entries[0]));
	if (root_entries == 0) {
		return -1;
	}

	if (block_read_bytes(block, sizeof(header),
			     (unsigned char *)root_entries,
			     (u64)header.entries * sizeof(root_entries[0])) != 0) {
		return -1;
	}

	for (u64 i = 0; i < header.entries; i++) {
		if (root_entries[i].type != BUNIX_VFS_TYPE_REGULAR &&
		    root_entries[i].type != BUNIX_VFS_TYPE_DIRECTORY) {
			return -1;
		}
		if (root_entries[i].type == BUNIX_VFS_TYPE_REGULAR &&
		    (root_entries[i].offset > reply.words[1] ||
		     root_entries[i].size > reply.words[1] - root_entries[i].offset)) {
			return -1;
		}
	}

	root_entry_count = header.entries;
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

static const struct rootfs_entry *rootfs_find_ref(const char *path)
{
	for (u64 i = 0; i < root_entry_count; i++) {
		if (path_eq(root_entries[i].path, path)) {
			return &root_entries[i];
		}
	}

	return 0;
}

static int user_can_access(u64 task, const struct rootfs_entry *entry, u64 mask)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_USER,
		.type = BUNIX_USER_CAN_ACCESS,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { task, 0, 0, 0 },
	};
	struct bunix_msg reply;
	const u64 user = service_user();

	if (entry == 0) {
		return 0;
	}
	if (task == 0) {
		return 1;
	}

	request.words[1] = entry->uid;
	request.words[2] = entry->gid;
	request.words[3] = ((u64)entry->mode << 32) | mask;
	if (user == 0 ||
	    bunix_ipc_call(user, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return 0;
	}

	return reply.words[1] != 0;
}

static int can_read_entry(const struct rootfs_entry *entry, u64 task)
{
	return user_can_access(task, entry, 04);
}

static int can_search_entry(const struct rootfs_entry *entry, u64 task)
{
	return user_can_access(task, entry, 01);
}

static void stat_meta_reply(struct bunix_msg *reply,
			    const struct rootfs_entry *entry)
{
	reply->words[0] = 0;
	reply->words[1] = entry->size;
	reply->words[2] = entry->mode | ((u64)entry->type << 32);
	reply->words[3] = ((u64)entry->uid) | ((u64)entry->gid << 32);
}

static const struct rootfs_entry *directory_entry_at(
	const struct rootfs_entry *directory, u64 index)
{
	u64 current = 0;

	if (directory == 0 || directory->type != BUNIX_VFS_TYPE_DIRECTORY) {
		return 0;
	}

	for (u64 i = 0; i < root_entry_count; i++) {
		if (!entry_is_child_of(&root_entries[i], directory->path)) {
			continue;
		}
		if (current == index) {
			return &root_entries[i];
		}
		current++;
	}

	return 0;
}

static u64 open_file(const struct rootfs_entry *entry)
{
	if (entry == 0) {
		return 0;
	}

	return bunix_id_alloc(&open_files, (u64)entry);
}

static const struct rootfs_entry *file_from_handle(u64 handle)
{
	return (const struct rootfs_entry *)bunix_id_get(&open_files, handle);
}

static void close_file(u64 handle)
{
	(void)bunix_id_remove(&open_files, handle);
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
	bunix_id_table_init(&open_files);
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
		case BUNIX_VFS_OPEN: {
			char path[ROOTFS_MAX_PATH];
			const struct rootfs_entry *entry;
			u64 file;

			for (u64 i = 0; i < sizeof(path); i++) {
				path[i] = '\0';
			}
			unpack_bytes((unsigned char *)path, &message.words[0], 16);
			if (path_is_procfs(path)) {
				(void)forward_procfs(&message, &reply);
				break;
			}
			entry = rootfs_find_ref(path);
			if (entry == 0) {
				reply.words[0] = BUNIX_VFS_ERR_NOENT;
				break;
			}
			if (entry->type == BUNIX_VFS_TYPE_DIRECTORY) {
				if (!can_search_entry(entry, message.words[3]) ||
				    !can_read_entry(entry, message.words[3])) {
					reply.words[0] = BUNIX_VFS_ERR_ACCESS;
					break;
				}
			} else if (!can_read_entry(entry, message.words[3])) {
				reply.words[0] = BUNIX_VFS_ERR_ACCESS;
				break;
			}
			file = open_file(entry);
			if (file == 0) {
				reply.words[0] = (u64)-1;
			} else {
				reply.words[0] = 0;
				reply.words[1] = file;
				reply.words[2] = entry->size;
				reply.words[3] = entry->type;
				bunix_console_write("vfs: open\n", 10);
			}
			break;
		}
		case BUNIX_VFS_STAT_PATH_META: {
			char path[ROOTFS_MAX_PATH];
			const struct rootfs_entry *entry;

			for (u64 i = 0; i < sizeof(path); i++) {
				path[i] = '\0';
			}
			unpack_bytes((unsigned char *)path, &message.words[0], 16);
			if (path_is_procfs(path)) {
				(void)forward_procfs(&message, &reply);
				break;
			}
			entry = rootfs_find_ref(path);
			if (entry == 0) {
				reply.words[0] = BUNIX_VFS_ERR_NOENT;
			} else {
				stat_meta_reply(&reply, entry);
			}
			break;
		}
		case BUNIX_VFS_STAT: {
			const struct rootfs_entry *entry =
				file_from_handle(message.words[0]);

			if (handle_is_procfs(message.words[0])) {
				(void)forward_procfs(&message, &reply);
				break;
			}
			if (entry == 0) {
				reply.words[0] = (u64)-1;
			} else {
				reply.words[0] = 0;
				reply.words[1] = entry->size;
				reply.words[2] = entry->type;
			}
			break;
		}
		case BUNIX_VFS_STAT_META: {
			const struct rootfs_entry *entry =
				file_from_handle(message.words[0]);

			if (handle_is_procfs(message.words[0])) {
				(void)forward_procfs(&message, &reply);
				break;
			}
			if (entry == 0) {
				reply.words[0] = (u64)-1;
			} else {
				stat_meta_reply(&reply, entry);
			}
			break;
		}
		case BUNIX_VFS_READ_FILE_BUFFER: {
			const struct rootfs_entry *entry =
				file_from_handle(message.words[0]);
			const u64 offset = message.words[1];
			u64 len = message.words[2];
			int read_len;

			if (handle_is_procfs(message.words[0])) {
				(void)forward_procfs(&message, &reply);
				break;
			}
			if (entry == 0 ||
			    entry->type != BUNIX_VFS_TYPE_REGULAR ||
			    message.cap == 0 ||
			    (message.cap_rights & (BUNIX_RIGHT_SEND |
						   BUNIX_RIGHT_DUP)) !=
			    (BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP)) {
				reply.words[0] = (u64)-1;
				break;
			}

			if (offset >= entry->size) {
				len = 0;
			} else if (len > entry->size - offset) {
				len = entry->size - offset;
			}

			if (len == 0) {
				reply.words[0] = 0;
				reply.words[1] = 0;
				break;
			}

			read_len = block_read_buffer(block, entry->offset + offset,
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
		case BUNIX_VFS_READDIR: {
			const struct rootfs_entry *directory =
				file_from_handle(message.words[0]);
			const struct rootfs_entry *child;
			const char *name;

			if (handle_is_procfs(message.words[0])) {
				(void)forward_procfs(&message, &reply);
				break;
			}
			if (directory == 0 ||
			    directory->type != BUNIX_VFS_TYPE_DIRECTORY) {
				reply.words[0] = BUNIX_VFS_ERR_NOTDIR;
				break;
			}
			child = directory_entry_at(directory, message.words[1]);
			if (child == 0) {
				reply.words[0] = BUNIX_VFS_ERR_NOENT;
				break;
			}

			name = entry_name_in_directory(child, directory->path);
			reply.words[0] = 0;
			reply.words[1] = (message.words[1] + 1) |
					 ((u64)child->type << 32);
			pack_bytes(&reply.words[2],
				   (const unsigned char *)name,
				   str_len(name) + 1);
			break;
		}
		case BUNIX_VFS_CLOSE:
			if (handle_is_procfs(message.words[0])) {
				(void)forward_procfs(&message, &reply);
				break;
			}
			if (file_from_handle(message.words[0]) == 0) {
				reply.words[0] = (u64)-1;
			} else {
				close_file(message.words[0]);
				reply.words[0] = 0;
				bunix_console_write("vfs: close\n", 11);
			}
			break;
		default:
			reply.words[0] = (u64)-1;
			break;
		}

		if (message.cap != 0) {
			bunix_handle_close(message.cap);
		}
		if (message.reply != 0) {
			bunix_ipc_send(message.reply, &reply);
		}
	}
}
