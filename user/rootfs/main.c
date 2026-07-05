#include <bunix/alloc.h>
#include <bunix/libbunix.h>
#include <bunix/tree.h>

enum {
	ROOTFS_HANDLE_NAMES = 3,
	ROOTFS_MAGIC = 0x30534652,
	ROOTFS_MAX_PATH = 4096,
	ROOTFS_MAX_SYMLINKS = 8,
	ROOTFS_OPEN_ROOT = 1,
	ROOTFS_OPEN_ENTRY = 2,
};

struct rootfs_header {
	unsigned int magic;
	unsigned int entries;
};

struct rootfs_disk_entry {
	char path[ROOTFS_MAX_PATH];
	u64 offset;
	u64 size;
	unsigned int uid;
	unsigned int gid;
	unsigned int mode;
	unsigned int type;
};

struct rootfs_entry {
	struct bunix_tree_node node;
	char path[ROOTFS_MAX_PATH];
	u64 offset;
	u64 size;
	unsigned int uid;
	unsigned int gid;
	unsigned int mode;
	unsigned int type;
};

struct rootfs_open {
	struct bunix_u64_tree_node node;
	u64 id;
	u64 kind;
	const struct rootfs_entry *entry;
};

static struct bunix_tree rootfs_by_path;
static struct bunix_u64_tree open_files;
static struct rootfs_entry *root_entries;
static u64 root_entry_count;
static u64 next_open_id = 1;
static u64 block_service;
static u64 user_service;

static u64 str_len(const char *text)
{
	u64 len = 0;

	while (len < ROOTFS_MAX_PATH && text[len] != '\0') {
		len++;
	}
	return len;
}

static int str_eq(const char *left, const char *right)
{
	while (*left != '\0' && *right != '\0') {
		if (*left++ != *right++) {
			return 0;
		}
	}
	return *left == '\0' && *right == '\0';
}

static void pack_bytes(u64 *words, const unsigned char *data, u64 len)
{
	words[0] = 0;
	words[1] = 0;
	for (u64 i = 0; i < len && i < BUNIX_IPC_DATA_BYTES; i++) {
		words[i / 8] |= ((u64)data[i]) << ((i % 8) * 8);
	}
}

static int name_is_valid(const char *name)
{
	if (name == 0 || name[0] == '\0') {
		return 0;
	}
	for (u64 i = 0; i < BUNIX_IPC_DATA_BYTES && name[i] != '\0'; i++) {
		if (name[i] == '/') {
			return 0;
		}
	}
	return 1;
}

static int set_path(char *target, const char *path)
{
	u64 i;

	if (target == 0 || path == 0 || path[0] != '/') {
		return -1;
	}
	for (i = 0; i + 1 < ROOTFS_MAX_PATH && path[i] != '\0'; i++) {
		target[i] = path[i];
	}
	if (path[i] != '\0') {
		return -1;
	}
	target[i] = '\0';
	return 0;
}

static int read_path_buffer_at(u64 buffer, u64 offset, u64 len, char *path)
{
	if (buffer == 0 || len == 0 || len > ROOTFS_MAX_PATH) {
		return -1;
	}
	for (u64 i = 0; i < ROOTFS_MAX_PATH; i++) {
		path[i] = '\0';
	}
	if (bunix_buffer_read(buffer, offset, path, len) != 0 ||
	    path[len - 1] != '\0') {
		return -1;
	}
	return str_len(path) < ROOTFS_MAX_PATH ? 0 : -1;
}

static int read_resolved_path(const struct bunix_msg *message, char *path)
{
	char cwd[ROOTFS_MAX_PATH];

	if ((message->cap_rights & BUNIX_RIGHT_RECV) == 0 ||
	    read_path_buffer_at(message->cap, 0, message->words[0],
				cwd) != 0 ||
	    read_path_buffer_at(message->cap, message->words[0],
				message->words[1], path) != 0 ||
	    path[0] != '/') {
		return -1;
	}
	(void)cwd;
	return 0;
}

static u64 resolve_service(u64 service, unsigned int rights)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_NAMES,
		.type = BUNIX_NAMES_WAIT,
		.words = { BUNIX_NAMES_ROOT, service, rights, 0 },
	};
	struct bunix_msg reply;

	if (bunix_ipc_call(ROOTFS_HANDLE_NAMES, &request, &reply) != 0 ||
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

static int task_can_access(u64 task, const struct rootfs_entry *entry, u64 mask)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_USER,
		.type = BUNIX_USER_CAN_ACCESS,
		.words = { task, 0, 0, 0 },
	};
	struct bunix_msg reply;
	const u64 user = service_user();

	if (entry == 0) {
		return mask == 0 || task == 0;
	}
	if (task == 0 || mask == 0) {
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

static long register_service(u64 service, u64 handle)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_NAMES,
		.type = BUNIX_NAMES_REGISTER,
		.cap_rights = BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP,
		.cap = handle,
		.words = { BUNIX_NAMES_ROOT, service, 0, 0 },
	};
	struct bunix_msg reply;

	if (bunix_ipc_call(ROOTFS_HANDLE_NAMES, &request, &reply) != 0) {
		return -1;
	}
	return reply.words[0] == 0 ? 0 : -1;
}

static int block_read_bytes(u64 offset, unsigned char *buffer, u64 len)
{
	u64 done = 0;

	while (done < len) {
		u64 chunk = len - done;
		struct bunix_msg request = {
			.protocol = BUNIX_PROTO_BLOCK,
			.type = BUNIX_BLOCK_READ,
			.words = { offset + done, chunk, 0, 0 },
		};
		struct bunix_msg reply;

		if (chunk > BUNIX_IPC_DATA_BYTES) {
			chunk = BUNIX_IPC_DATA_BYTES;
		}
		request.words[1] = chunk;
		if (bunix_ipc_call(block_service, &request, &reply) != 0 ||
		    reply.words[0] != 0 ||
		    reply.words[1] == 0 ||
		    reply.words[1] > chunk) {
			return -1;
		}
		for (u64 i = 0; i < reply.words[1]; i++) {
			buffer[done + i] =
				(unsigned char)((reply.words[2 + i / 8] >>
				((i % 8) * 8)) & 0xff);
		}
		done += reply.words[1];
	}
	return 0;
}

static int block_read_buffer(u64 offset, u64 buffer, u64 len)
{
	u64 done = 0;

	while (done < len) {
		struct bunix_msg request = {
			.protocol = BUNIX_PROTO_BLOCK,
			.type = BUNIX_BLOCK_READ_BUFFER,
			.cap_rights = BUNIX_RIGHT_SEND,
			.cap = buffer,
			.words = { offset + done, len - done, done, 0 },
		};
		struct bunix_msg reply;

		if (bunix_ipc_call(block_service, &request, &reply) != 0 ||
		    reply.words[0] != 0 ||
		    reply.words[1] == 0 ||
		    reply.words[1] > len - done) {
			return -1;
		}
		done += reply.words[1];
	}
	return (int)done;
}

static int rootfs_mount(void)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_BLOCK,
		.type = BUNIX_BLOCK_GET_INFO,
	};
	struct bunix_msg reply;
	struct rootfs_header header;
	struct rootfs_disk_entry *disk_entries;

	if (bunix_ipc_call(block_service, &request, &reply) != 0 ||
	    reply.words[0] != 0 ||
	    reply.words[1] < sizeof(header) ||
	    block_read_bytes(0, (unsigned char *)&header, sizeof(header)) != 0 ||
	    header.magic != ROOTFS_MAGIC ||
	    header.entries == 0 ||
	    reply.words[1] < sizeof(header) +
	    (u64)header.entries * sizeof(struct rootfs_disk_entry)) {
		return -1;
	}
	root_entries = (struct rootfs_entry *)
		bunix_calloc(header.entries, sizeof(root_entries[0]));
	disk_entries = (struct rootfs_disk_entry *)
		bunix_calloc(header.entries, sizeof(disk_entries[0]));
	if (root_entries == 0 || disk_entries == 0 ||
	    block_read_bytes(sizeof(header), (unsigned char *)disk_entries,
			     (u64)header.entries *
			     sizeof(disk_entries[0])) != 0) {
		return -1;
	}
	for (u64 i = 0; i < header.entries; i++) {
		for (u64 j = 0; j < ROOTFS_MAX_PATH; j++) {
			root_entries[i].path[j] = disk_entries[i].path[j];
		}
		root_entries[i].offset = disk_entries[i].offset;
		root_entries[i].size = disk_entries[i].size;
		root_entries[i].uid = disk_entries[i].uid;
		root_entries[i].gid = disk_entries[i].gid;
		root_entries[i].mode = disk_entries[i].mode;
		root_entries[i].type = disk_entries[i].type;
		if (bunix_tree_insert_node(&rootfs_by_path,
					   &root_entries[i].node,
					   root_entries[i].path,
					   (u64)&root_entries[i]) != 0) {
			bunix_free(disk_entries);
			return -1;
		}
	}
	bunix_free(disk_entries);
	root_entry_count = header.entries;
	return 0;
}

static const struct rootfs_entry *rootfs_find(const char *path)
{
	return (const struct rootfs_entry *)bunix_tree_get(&rootfs_by_path,
							   path);
}

static int rootfs_read_text(const struct rootfs_entry *entry, char *out, u64 max)
{
	if (entry == 0 || out == 0 || max == 0 ||
	    entry->size + 1 > max ||
	    block_read_bytes(entry->offset, (unsigned char *)out,
			     entry->size) != 0) {
		return -1;
	}
	out[entry->size] = '\0';
	return 0;
}

static const struct rootfs_entry *rootfs_resolve(const char *path)
{
	char current[ROOTFS_MAX_PATH];

	if (path == 0 || path[0] != '/' || set_path(current, path) != 0) {
		return 0;
	}
	for (u64 depth = 0; depth < ROOTFS_MAX_SYMLINKS; depth++) {
		const struct rootfs_entry *entry = rootfs_find(current);
		char target[ROOTFS_MAX_PATH];

		if (entry == 0 || entry->type != BUNIX_VFS_TYPE_SYMLINK) {
			return entry;
		}
		if (rootfs_read_text(entry, target, sizeof(target)) != 0 ||
		    target[0] != '/' ||
		    set_path(current, target) != 0) {
			return 0;
		}
	}
	return 0;
}

static int path_is_child_of(const char *path, const char *directory)
{
	const u64 dir_len = str_len(directory);
	u64 pos;

	if (path == 0 || directory == 0 || str_eq(path, directory)) {
		return 0;
	}
	if (str_eq(directory, "/")) {
		if (path[0] != '/' || path[1] == '\0') {
			return 0;
		}
		pos = 1;
	} else {
		for (u64 i = 0; i < dir_len; i++) {
			if (path[i] != directory[i]) {
				return 0;
			}
		}
		if (path[dir_len] != '/') {
			return 0;
		}
		pos = dir_len + 1;
	}
	while (path[pos] != '\0') {
		if (path[pos] == '/') {
			return 0;
		}
		pos++;
	}
	return 1;
}

static const char *path_name_in_dir(const char *path, const char *directory)
{
	const u64 dir_len = str_len(directory);

	return str_eq(directory, "/") ? path + 1 : path + dir_len + 1;
}

static void stat_entry(struct bunix_msg *reply, const struct rootfs_entry *entry)
{
	reply->words[0] = 0;
	reply->words[1] = entry->size;
	reply->words[2] = entry->mode | ((u64)entry->type << 32);
	reply->words[3] = ((u64)entry->uid) | ((u64)entry->gid << 32);
}

static void stat_root_dir(struct bunix_msg *reply)
{
	reply->words[0] = 0;
	reply->words[1] = 0;
	reply->words[2] = 0555 | ((u64)BUNIX_VFS_TYPE_DIRECTORY << 32);
	reply->words[3] = 0;
}

static u64 remember_open(u64 kind, const struct rootfs_entry *entry)
{
	struct rootfs_open *open;
	u64 id;

	open = (struct rootfs_open *)bunix_calloc(1, sizeof(*open));
	if (open == 0) {
		return 0;
	}
	id = next_open_id++;
	while (id == 0 || bunix_u64_tree_get(&open_files, id) != 0) {
		id = next_open_id++;
	}
	open->id = id;
	open->kind = kind;
	open->entry = entry;
	if (bunix_u64_tree_insert_node(&open_files, &open->node, id,
				       (u64)open) != 0) {
		bunix_free(open);
		return 0;
	}
	return id;
}

static struct rootfs_open *open_from_handle(u64 handle)
{
	return (struct rootfs_open *)bunix_u64_tree_get(&open_files, handle);
}

static void forget_open(struct rootfs_open *open)
{
	if (open == 0) {
		return;
	}
	bunix_u64_tree_remove_node(&open_files, &open->node);
	bunix_free(open);
}

static void reply_path_meta(struct bunix_msg *message, struct bunix_msg *reply,
			    const char *path)
{
	const int nofollow = message->type == BUNIX_VFS_READLINK_BUFFER ||
			     (((message->words[3] >> 32) & 1) != 0 &&
			      message->type == BUNIX_VFS_STAT_PATH_META_BUFFER);
	const struct rootfs_entry *entry = nofollow ? rootfs_find(path) :
					   rootfs_resolve(path);

	if (entry == 0) {
		if (str_eq(path, "/")) {
			stat_root_dir(reply);
		} else {
			reply->words[0] = BUNIX_VFS_ERR_NOENT;
		}
	} else if (message->type == BUNIX_VFS_READLINK_BUFFER) {
		char target[ROOTFS_MAX_PATH];

		if (entry->type != BUNIX_VFS_TYPE_SYMLINK ||
		    rootfs_read_text(entry, target, sizeof(target)) != 0) {
			reply->words[0] = (u64)-1;
			return;
		}
		reply->words[0] = 0;
		reply->words[1] = str_len(target);
		pack_bytes(&reply->words[2], (const unsigned char *)target,
			   str_len(target) + 1);
	} else if (message->type == BUNIX_VFS_ACCESS_BUFFER) {
		reply->words[0] = task_can_access(message->words[3] &
						  0xffffffff, entry,
						  message->words[3] >> 32) ?
				  0 : BUNIX_VFS_ERR_ACCESS;
	} else {
		stat_entry(reply, entry);
	}
}

static void reply_open(struct bunix_msg *message, struct bunix_msg *reply,
		       const char *path)
{
	const struct rootfs_entry *entry = rootfs_resolve(path);
	u64 handle;

	if (entry == 0 && !str_eq(path, "/")) {
		reply->words[0] = BUNIX_VFS_ERR_NOENT;
		return;
	}
	if (entry != 0 &&
	    !task_can_access(message->words[3], entry,
			     entry->type == BUNIX_VFS_TYPE_DIRECTORY ?
			     05 : 04)) {
		reply->words[0] = BUNIX_VFS_ERR_ACCESS;
		return;
	}
	handle = entry == 0 ? remember_open(ROOTFS_OPEN_ROOT, 0) :
			      remember_open(ROOTFS_OPEN_ENTRY, entry);
	if (handle == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	reply->words[0] = 0;
	reply->words[1] = handle;
	reply->words[2] = entry == 0 ? 0 : entry->size;
	reply->words[3] = entry == 0 ? BUNIX_VFS_TYPE_DIRECTORY : entry->type;
}

static void reply_readdir(struct rootfs_open *open, u64 index,
			  struct bunix_msg *reply)
{
	u64 current = 0;
	const char *directory;

	if (open == 0 ||
	    (open->kind == ROOTFS_OPEN_ENTRY &&
	     open->entry->type != BUNIX_VFS_TYPE_DIRECTORY)) {
		reply->words[0] = BUNIX_VFS_ERR_NOTDIR;
		return;
	}
	directory = open->kind == ROOTFS_OPEN_ROOT ? "/" : open->entry->path;
	for (u64 i = 0; i < root_entry_count; i++) {
		const struct rootfs_entry *entry = &root_entries[i];
		const char *name;

		if (!path_is_child_of(entry->path, directory)) {
			continue;
		}
		name = path_name_in_dir(entry->path, directory);
		if (!name_is_valid(name)) {
			continue;
		}
		if (current == index) {
			reply->words[0] = 0;
			reply->words[1] = (index + 1) |
					  ((u64)entry->type << 32);
			pack_bytes(&reply->words[2],
				   (const unsigned char *)name,
				   str_len(name) + 1);
			return;
		}
		current++;
	}
	reply->words[0] = BUNIX_VFS_ERR_NOENT;
}

static void forward_open_handle(struct bunix_msg *message,
				struct bunix_msg *reply)
{
	struct rootfs_open *open = open_from_handle(message->words[0]);
	const struct rootfs_entry *entry;

	if (open == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	entry = open->entry;
	if (message->type == BUNIX_VFS_CLOSE) {
		forget_open(open);
		reply->words[0] = 0;
	} else if (message->type == BUNIX_VFS_STAT_META) {
		if (open->kind == ROOTFS_OPEN_ROOT) {
			stat_root_dir(reply);
		} else {
			stat_entry(reply, entry);
		}
	} else if (message->type == BUNIX_VFS_STAT) {
		reply->words[0] = 0;
		reply->words[1] = open->kind == ROOTFS_OPEN_ROOT ?
				  0 : entry->size;
		reply->words[2] = open->kind == ROOTFS_OPEN_ROOT ?
				  BUNIX_VFS_TYPE_DIRECTORY : entry->type;
	} else if (message->type == BUNIX_VFS_READ_FILE_BUFFER &&
		   open->kind == ROOTFS_OPEN_ENTRY &&
		   entry->type == BUNIX_VFS_TYPE_REGULAR &&
		   message->cap != 0 &&
		   (message->cap_rights & BUNIX_RIGHT_SEND) != 0) {
		u64 len = message->words[2];
		int nread;

		if (message->words[1] >= entry->size) {
			reply->words[0] = 0;
			reply->words[1] = 0;
			return;
		}
		if (len > entry->size - message->words[1]) {
			len = entry->size - message->words[1];
		}
		nread = block_read_buffer(entry->offset + message->words[1],
					  message->cap, len);
		reply->words[0] = nread < 0 ? (u64)-1 : 0;
		reply->words[1] = nread < 0 ? 0 : (u64)nread;
	} else if (message->type == BUNIX_VFS_READDIR) {
		reply_readdir(open, message->words[1], reply);
	} else {
		reply->words[0] = BUNIX_VFS_ERR_ACCESS;
	}
}

int main(void)
{
	const char online[] = "rootfs: online\n";
	struct bunix_msg message;

	bunix_console_log(online, sizeof(online) - 1);
	bunix_tree_init(&rootfs_by_path);
	bunix_u64_tree_init(&open_files);
	block_service = resolve_service(BUNIX_SERVICE_BLOCK, BUNIX_RIGHT_SEND);
	if (block_service == 0 ||
	    rootfs_mount() != 0 ||
	    register_service(BUNIX_SERVICE_ROOTFS, BUNIX_HANDLE_SELF) != 0) {
		return 1;
	}

	for (;;) {
		struct bunix_msg reply = {
			.protocol = BUNIX_PROTO_VFS,
			.type = 0,
			.cap_rights = 0,
			.words = { 0, 0, 0, 0 },
		};
		char path[ROOTFS_MAX_PATH];

		if (bunix_ipc_recv(BUNIX_HANDLE_SELF, &message) != 0) {
			continue;
		}
		if (message.protocol != BUNIX_PROTO_VFS) {
			continue;
		}
		reply.type = message.type;
		switch (message.type) {
		case BUNIX_VFS_OPEN_BUFFER:
			if (read_resolved_path(&message, path) != 0) {
				reply.words[0] = BUNIX_VFS_ERR_NOENT;
			} else {
				reply_open(&message, &reply, path);
			}
			break;
		case BUNIX_VFS_STAT_PATH_META_BUFFER:
		case BUNIX_VFS_ACCESS_BUFFER:
		case BUNIX_VFS_READLINK_BUFFER:
			if (read_resolved_path(&message, path) != 0) {
				reply.words[0] = BUNIX_VFS_ERR_NOENT;
			} else {
				reply_path_meta(&message, &reply, path);
			}
			break;
		case BUNIX_VFS_STAT:
		case BUNIX_VFS_STAT_META:
		case BUNIX_VFS_READ_FILE_BUFFER:
		case BUNIX_VFS_READDIR:
		case BUNIX_VFS_CLOSE:
			forward_open_handle(&message, &reply);
			break;
		default:
			reply.words[0] = BUNIX_VFS_ERR_ACCESS;
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
