#include <bunix/alloc.h>
#include <bunix/libbunix.h>
#include <bunix/tree.h>

enum {
	UNIONFS_HANDLE_NAMES = 3,
	ROOTFS_MAGIC = 0x30534652,
	ROOTFS_MAX_PATH = 128,
	UNIONFS_MAX_PATH = 256,
	UNIONFS_MOUNT_PATH_LEN = 10,
	UNIONFS_OPEN_LOWER = 1,
	UNIONFS_OPEN_UPPER = 2,
	UNIONFS_OPEN_DIR = 3,
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
	char path[ROOTFS_MAX_PATH];
	u64 offset;
	u64 size;
	unsigned int uid;
	unsigned int gid;
	unsigned int mode;
	unsigned int type;
};

struct unionfs_open {
	struct bunix_u64_tree_node node;
	u64 id;
	u64 kind;
	u64 service;
	u64 remote_handle;
	struct rootfs_entry lower;
	char upper_path[UNIONFS_MAX_PATH];
	char relative_path[UNIONFS_MAX_PATH];
};

struct unionfs_whiteout {
	struct bunix_tree_node node;
	char *path;
};

static struct bunix_tree whiteouts;
static struct bunix_u64_tree open_files;
static struct rootfs_entry *root_entries;
static u64 root_entry_count;
static u64 next_open_id = 1;
static u64 vfs_service;
static u64 tmpfs_service;
static u64 block_service;

static u64 str_len(const char *text)
{
	u64 len = 0;

	while (len < UNIONFS_MAX_PATH && text[len] != '\0') {
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

static void unpack_name(char *name, u64 word0, u64 word1)
{
	const u64 words[] = { word0, word1 };

	for (u64 i = 0; i < BUNIX_IPC_DATA_BYTES; i++) {
		name[i] = (char)((words[i / 8] >> ((i % 8) * 8)) & 0xff);
		if (name[i] == '\0') {
			return;
		}
	}
	name[BUNIX_IPC_DATA_BYTES - 1] = '\0';
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

static void pack_path(u64 *words, const char *path)
{
	pack_bytes(words, (const unsigned char *)path, str_len(path) + 1);
}

static int read_path_buffer_at(u64 buffer, u64 offset, u64 len, char *path)
{
	if (buffer == 0 || len == 0 || len > UNIONFS_MAX_PATH) {
		return -1;
	}
	for (u64 i = 0; i < UNIONFS_MAX_PATH; i++) {
		path[i] = '\0';
	}
	if (bunix_buffer_read(buffer, offset, path, len) != 0 ||
	    path[len - 1] != '\0') {
		return -1;
	}
	return str_len(path) < UNIONFS_MAX_PATH ? 0 : -1;
}

static int read_resolved_path(const struct bunix_msg *message, char *path)
{
	char cwd[UNIONFS_MAX_PATH];

	if ((message->cap_rights & BUNIX_RIGHT_RECV) == 0 ||
	    read_path_buffer_at(message->cap, 0, message->words[0],
				cwd) != 0 ||
	    read_path_buffer_at(message->cap, message->words[0],
				message->words[1], path) != 0) {
		return -1;
	}
	return 0;
}

static int mounted_relative_path(const char *path, char *relative)
{
	const char mount[] = "/mnt/union";

	if (str_eq(path, mount)) {
		relative[0] = '/';
		relative[1] = '\0';
		return 0;
	}
	for (u64 i = 0; mount[i] != '\0'; i++) {
		if (path[i] != mount[i]) {
			return -1;
		}
	}
	if (path[UNIONFS_MOUNT_PATH_LEN] != '/') {
		return -1;
	}
	for (u64 i = 0; i < UNIONFS_MAX_PATH; i++) {
		relative[i] = path[UNIONFS_MOUNT_PATH_LEN + i];
		if (relative[i] == '\0') {
			return 0;
		}
	}
	return -1;
}

static int compose_upper_path(const char *relative, char *path)
{
	const char upper[] = "/tmp/union";
	u64 pos = 0;

	for (u64 i = 0; upper[i] != '\0'; i++) {
		path[pos++] = upper[i];
	}
	if (str_eq(relative, "/")) {
		path[pos] = '\0';
		return 0;
	}
	for (u64 i = 0; relative[i] != '\0'; i++) {
		if (pos + 1 >= UNIONFS_MAX_PATH) {
			return -1;
		}
		path[pos++] = relative[i];
	}
	path[pos] = '\0';
	return 0;
}

static int compose_lower_path(const char *relative, char *path)
{
	for (u64 i = 0; i < UNIONFS_MAX_PATH; i++) {
		path[i] = relative[i];
		if (path[i] == '\0') {
			return 0;
		}
	}
	return -1;
}

static int compose_child_path(const char *directory, const char *name,
			      char *path)
{
	u64 pos = 0;

	if (directory == 0 || name == 0 || directory[0] != '/' ||
	    name[0] == '\0') {
		return -1;
	}
	for (u64 i = 0; directory[i] != '\0'; i++) {
		if (pos + 1 >= UNIONFS_MAX_PATH) {
			return -1;
		}
		path[pos++] = directory[i];
	}
	if (pos > 1) {
		if (pos + 1 >= UNIONFS_MAX_PATH) {
			return -1;
		}
		path[pos++] = '/';
	}
	for (u64 i = 0; name[i] != '\0'; i++) {
		if (name[i] == '/' || pos + 1 >= UNIONFS_MAX_PATH) {
			return -1;
		}
		path[pos++] = name[i];
	}
	path[pos] = '\0';
	return 0;
}

static char *str_dup(const char *text)
{
	const u64 len = str_len(text);
	char *copy;

	if (text == 0 || len == UNIONFS_MAX_PATH) {
		return 0;
	}
	copy = (char *)bunix_alloc(len + 1);
	if (copy == 0) {
		return 0;
	}
	for (u64 i = 0; i <= len; i++) {
		copy[i] = text[i];
	}
	return copy;
}

static int whiteout_exists(const char *relative)
{
	return bunix_tree_get(&whiteouts, relative) != 0;
}

static void whiteout_remove(const char *relative)
{
	struct bunix_tree_node *node = bunix_tree_find_node(&whiteouts,
							    relative);
	struct unionfs_whiteout *whiteout;

	if (node == 0) {
		return;
	}
	whiteout = (struct unionfs_whiteout *)node->value;
	bunix_tree_remove_node(&whiteouts, &whiteout->node);
	bunix_free(whiteout->path);
	bunix_free(whiteout);
}

static int whiteout_add(const char *relative)
{
	struct unionfs_whiteout *whiteout;

	if (whiteout_exists(relative)) {
		return 0;
	}
	whiteout = (struct unionfs_whiteout *)
		bunix_calloc(1, sizeof(*whiteout));
	if (whiteout == 0) {
		return -1;
	}
	whiteout->path = str_dup(relative);
	if (whiteout->path == 0 ||
	    bunix_tree_insert_node(&whiteouts, &whiteout->node,
				   whiteout->path, (u64)whiteout) != 0) {
		if (whiteout->path != 0) {
			bunix_free(whiteout->path);
		}
		bunix_free(whiteout);
		return -1;
	}
	return 0;
}

static u64 resolve_service(u64 service, unsigned int rights)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_NAMES,
		.type = BUNIX_NAMES_WAIT,
		.cap_rights = 0,
		.words = { BUNIX_NAMES_ROOT, service, rights, 0 },
	};
	struct bunix_msg reply;

	if (bunix_ipc_call(UNIONFS_HANDLE_NAMES, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return 0;
	}
	return reply.cap;
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

	if (bunix_ipc_call(UNIONFS_HANDLE_NAMES, &request, &reply) != 0) {
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
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_BLOCK,
		.type = BUNIX_BLOCK_READ_BUFFER,
		.cap_rights = BUNIX_RIGHT_SEND,
		.cap = buffer,
		.words = { offset, len, 0, 0 },
	};
	struct bunix_msg reply;

	if (bunix_ipc_call(block_service, &request, &reply) != 0 ||
	    reply.words[0] != 0 ||
	    reply.words[1] > len) {
		return -1;
	}
	return (int)reply.words[1];
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
	}
	bunix_free(disk_entries);
	root_entry_count = header.entries;
	return 0;
}

static const struct rootfs_entry *rootfs_find(const char *path)
{
	for (u64 i = 0; i < root_entry_count; i++) {
		if (str_eq(root_entries[i].path, path)) {
			return &root_entries[i];
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

static int lower_is_directory(const char *path)
{
	const struct rootfs_entry *entry;

	if (str_eq(path, "/")) {
		return 1;
	}
	entry = rootfs_find(path);
	return entry != 0 && entry->type == BUNIX_VFS_TYPE_DIRECTORY;
}

static void stat_lower(struct bunix_msg *reply, const struct rootfs_entry *entry)
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

static int service_path_call(u64 service, u64 type, const char *path, u64 word3,
			     struct bunix_msg *reply)
{
	const char root[] = "/";
	const u64 cwd_len = 2;
	const u64 path_len = str_len(path) + 1;
	const long buffer = bunix_buffer_create(cwd_len + path_len);
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = type,
		.cap_rights = BUNIX_RIGHT_RECV,
		.words = { cwd_len, path_len, 0, word3 },
	};
	int result = -1;

	if (buffer < 0 ||
	    bunix_buffer_write((u64)buffer, 0, root, cwd_len) != 0 ||
	    bunix_buffer_write((u64)buffer, cwd_len, path, path_len) != 0) {
		if (buffer >= 0) {
			bunix_handle_close((u64)buffer);
		}
		return -1;
	}
	request.cap = (u64)buffer;
	if (bunix_ipc_call(service, &request, reply) == 0) {
		result = 0;
	}
	bunix_handle_close((u64)buffer);
	return result;
}

static int vfs_mount_path(const char *path)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_MOUNT,
		.cap_rights = BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP,
		.cap = BUNIX_HANDLE_SELF,
	};
	struct bunix_msg reply;

	pack_path(&request.words[0], path);
	return bunix_ipc_call(vfs_service, &request, &reply) == 0 &&
	       reply.words[0] == 0 ? 0 : -1;
}

static u64 remember_upper_open(u64 service, u64 remote_handle)
{
	struct unionfs_open *open;
	u64 id;

	if (service == 0 || remote_handle == 0) {
		return 0;
	}
	open = (struct unionfs_open *)bunix_calloc(1, sizeof(*open));
	if (open == 0) {
		return 0;
	}
	id = next_open_id++;
	while (id == 0 || bunix_u64_tree_get(&open_files, id) != 0) {
		id = next_open_id++;
	}
	open->id = id;
	open->kind = UNIONFS_OPEN_UPPER;
	open->service = service;
	open->remote_handle = remote_handle;
	if (bunix_u64_tree_insert_node(&open_files, &open->node, id,
				       (u64)open) != 0) {
		bunix_free(open);
		return 0;
	}
	return id;
}

static u64 remember_lower_open(const struct rootfs_entry *entry)
{
	struct unionfs_open *open;
	u64 id;

	if (entry == 0) {
		return 0;
	}
	open = (struct unionfs_open *)bunix_calloc(1, sizeof(*open));
	if (open == 0) {
		return 0;
	}
	id = next_open_id++;
	while (id == 0 || bunix_u64_tree_get(&open_files, id) != 0) {
		id = next_open_id++;
	}
	open->id = id;
	open->kind = UNIONFS_OPEN_LOWER;
	open->lower = *entry;
	(void)compose_upper_path(entry->path, open->upper_path);
	if (bunix_u64_tree_insert_node(&open_files, &open->node, id,
				       (u64)open) != 0) {
		bunix_free(open);
		return 0;
	}
	return id;
}

static u64 remember_dir_open(const char *relative, u64 upper_handle)
{
	struct unionfs_open *open;
	u64 id;

	if (relative == 0) {
		return 0;
	}
	open = (struct unionfs_open *)bunix_calloc(1, sizeof(*open));
	if (open == 0) {
		return 0;
	}
	id = next_open_id++;
	while (id == 0 || bunix_u64_tree_get(&open_files, id) != 0) {
		id = next_open_id++;
	}
	open->id = id;
	open->kind = UNIONFS_OPEN_DIR;
	open->service = tmpfs_service;
	open->remote_handle = upper_handle;
	for (u64 i = 0; i < UNIONFS_MAX_PATH; i++) {
		open->relative_path[i] = relative[i];
		if (relative[i] == '\0') {
			break;
		}
	}
	if (bunix_u64_tree_insert_node(&open_files, &open->node, id,
				       (u64)open) != 0) {
		bunix_free(open);
		return 0;
	}
	return id;
}

static int copy_lower_to_upper(struct unionfs_open *open, u64 task)
{
	enum {
		COPY_CHUNK = 4096,
	};
	struct bunix_msg reply;
	u64 remote_handle;
	u64 done = 0;

	if (open == 0 || open->kind != UNIONFS_OPEN_LOWER ||
	    service_path_call(tmpfs_service, BUNIX_VFS_CREATE_BUFFER,
			      open->upper_path,
			      (task & 0xffffffff) |
			      ((u64)(open->lower.mode & 0777) << 32),
			      &reply) != 0) {
		return -1;
	}
	if (service_path_call(tmpfs_service, BUNIX_VFS_OPEN_BUFFER,
			      open->upper_path, 0, &reply) != 0 ||
	    reply.words[0] != 0) {
		return -1;
	}
	remote_handle = reply.words[1];
	while (done < open->lower.size) {
		u64 len = open->lower.size - done;
		const long buffer = bunix_buffer_create(COPY_CHUNK);
		struct bunix_msg write = {
			.protocol = BUNIX_PROTO_VFS,
			.type = BUNIX_VFS_WRITE_FILE_BUFFER,
			.cap_rights = BUNIX_RIGHT_RECV | BUNIX_RIGHT_DUP,
			.words = { remote_handle, done, 0, 0 },
		};

		if (len > COPY_CHUNK) {
			len = COPY_CHUNK;
		}
		if (buffer < 0 ||
		    block_read_buffer(open->lower.offset + done,
				      (u64)buffer, len) < 0) {
			if (buffer >= 0) {
				bunix_handle_close((u64)buffer);
			}
			return -1;
		}
		write.cap = (u64)buffer;
		write.words[2] = len;
		if (bunix_ipc_call(tmpfs_service, &write, &reply) != 0 ||
		    reply.words[0] != 0 ||
		    reply.words[1] != len) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
		bunix_handle_close((u64)buffer);
		done += len;
	}
	open->kind = UNIONFS_OPEN_UPPER;
	open->service = tmpfs_service;
	open->remote_handle = remote_handle;
	return 0;
}

static struct unionfs_open *open_from_handle(u64 handle)
{
	return (struct unionfs_open *)bunix_u64_tree_get(&open_files, handle);
}

static int close_remote_handle(u64 service, u64 handle)
{
	struct bunix_msg close = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_CLOSE,
		.words = { handle, 0, 0, 0 },
	};
	struct bunix_msg ignored;

	return service != 0 && handle != 0 ?
	       bunix_ipc_call(service, &close, &ignored) : 0;
}

static void forget_open(struct unionfs_open *open)
{
	if (open == 0) {
		return;
	}
	if ((open->kind == UNIONFS_OPEN_UPPER ||
	     open->kind == UNIONFS_OPEN_DIR) &&
	    open->service != 0 &&
	    open->remote_handle != 0) {
		(void)close_remote_handle(open->service, open->remote_handle);
	}
	bunix_u64_tree_remove_node(&open_files, &open->node);
	bunix_free(open);
}

static void reply_path_meta(struct bunix_msg *message, struct bunix_msg *reply,
			    const char *path)
{
	char relative[UNIONFS_MAX_PATH];
	char upper[UNIONFS_MAX_PATH];
	char lower[UNIONFS_MAX_PATH];

	if (mounted_relative_path(path, relative) != 0 ||
	    compose_upper_path(relative, upper) != 0 ||
	    compose_lower_path(relative, lower) != 0) {
		reply->words[0] = BUNIX_VFS_ERR_NOENT;
		return;
	}
	if (service_path_call(tmpfs_service, message->type, upper,
			      message->words[3], reply) == 0 &&
	    reply->words[0] == 0) {
		return;
	}
	if (whiteout_exists(relative)) {
		reply->words[0] = BUNIX_VFS_ERR_NOENT;
		return;
	}
	const struct rootfs_entry *entry = rootfs_find(lower);
	if (entry == 0) {
		if (str_eq(lower, "/")) {
			stat_root_dir(reply);
		} else {
			reply->words[0] = BUNIX_VFS_ERR_NOENT;
		}
	} else if (message->type == BUNIX_VFS_ACCESS_BUFFER) {
		reply->words[0] = 0;
	} else {
		stat_lower(reply, entry);
	}
}

static void reply_open(struct bunix_msg *message, struct bunix_msg *reply,
		       const char *path)
{
	char relative[UNIONFS_MAX_PATH];
	char upper[UNIONFS_MAX_PATH];
	char lower[UNIONFS_MAX_PATH];
	struct bunix_msg layer_reply;
	const struct rootfs_entry *entry;
	u64 handle;
	u64 upper_handle = 0;
	u64 upper_type = 0;
	u64 upper_size = 0;
	int lower_dir;

	if (mounted_relative_path(path, relative) != 0 ||
	    compose_upper_path(relative, upper) != 0 ||
	    compose_lower_path(relative, lower) != 0) {
		reply->words[0] = BUNIX_VFS_ERR_NOENT;
		return;
	}
	if (service_path_call(tmpfs_service, BUNIX_VFS_OPEN_BUFFER, upper,
			      message->words[3], &layer_reply) != 0 ||
	    layer_reply.words[0] != 0) {
		if (whiteout_exists(relative)) {
			reply->words[0] = BUNIX_VFS_ERR_NOENT;
			return;
		}
		entry = rootfs_find(lower);
		if (entry == 0) {
			reply->words[0] = BUNIX_VFS_ERR_NOENT;
			return;
		}
		if (entry->type == BUNIX_VFS_TYPE_DIRECTORY) {
			handle = remember_dir_open(relative, 0);
			if (handle == 0) {
				reply->words[0] = (u64)-1;
				return;
			}
			reply->words[0] = 0;
			reply->words[1] = handle;
			reply->words[2] = 0;
			reply->words[3] = BUNIX_VFS_TYPE_DIRECTORY;
			return;
		}
		handle = remember_lower_open(entry);
		if (handle == 0) {
			reply->words[0] = (u64)-1;
			return;
		}
		reply->words[0] = 0;
		reply->words[1] = handle;
		reply->words[2] = entry->size;
		reply->words[3] = entry->type;
		return;
	}
	upper_handle = layer_reply.words[1];
	upper_size = layer_reply.words[2];
	upper_type = layer_reply.words[3];
	lower_dir = lower_is_directory(lower) && !whiteout_exists(relative);
	if (upper_type == BUNIX_VFS_TYPE_DIRECTORY || lower_dir) {
		handle = remember_dir_open(relative,
					   upper_type == BUNIX_VFS_TYPE_DIRECTORY ?
					   upper_handle : 0);
		if (handle == 0) {
			(void)close_remote_handle(tmpfs_service, upper_handle);
			reply->words[0] = (u64)-1;
			return;
		}
		reply->words[0] = 0;
		reply->words[1] = handle;
		reply->words[2] = 0;
		reply->words[3] = BUNIX_VFS_TYPE_DIRECTORY;
		return;
	}

	handle = remember_upper_open(tmpfs_service, upper_handle);
	if (handle == 0) {
		(void)close_remote_handle(tmpfs_service, upper_handle);
		reply->words[0] = (u64)-1;
		return;
	}
	*reply = layer_reply;
	reply->words[1] = handle;
	reply->words[2] = upper_size;
}

static void reply_mutate(struct bunix_msg *message, struct bunix_msg *reply,
			 const char *path)
{
	char relative[UNIONFS_MAX_PATH];
	char upper[UNIONFS_MAX_PATH];
	char lower[UNIONFS_MAX_PATH];

	if (mounted_relative_path(path, relative) != 0 ||
	    compose_upper_path(relative, upper) != 0 ||
	    compose_lower_path(relative, lower) != 0 ||
	    service_path_call(tmpfs_service, message->type, upper,
			      message->words[3], reply) != 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	if (message->type == BUNIX_VFS_CREATE_BUFFER ||
	    message->type == BUNIX_VFS_MKDIR_BUFFER) {
		if (reply->words[0] == 0) {
			whiteout_remove(relative);
		}
		return;
	}
	if (message->type != BUNIX_VFS_UNLINK_BUFFER &&
	    message->type != BUNIX_VFS_RMDIR_BUFFER) {
		return;
	}
	if (reply->words[0] == 0) {
		(void)whiteout_add(relative);
		return;
	}
	const struct rootfs_entry *entry = rootfs_find(lower);
	const u64 want_type = message->type == BUNIX_VFS_RMDIR_BUFFER ?
			      BUNIX_VFS_TYPE_DIRECTORY :
			      BUNIX_VFS_TYPE_REGULAR;

	if (entry != 0 && entry->type == want_type &&
	    whiteout_add(relative) == 0) {
		reply->words[0] = 0;
	}
}

static int upper_child_exists(const char *directory, const char *name)
{
	char relative[UNIONFS_MAX_PATH];
	char upper[UNIONFS_MAX_PATH];
	struct bunix_msg reply;

	if (compose_child_path(directory, name, relative) != 0 ||
	    compose_upper_path(relative, upper) != 0) {
		return 0;
	}
	return service_path_call(tmpfs_service,
				 BUNIX_VFS_STAT_PATH_META_BUFFER, upper, 0,
				 &reply) == 0 &&
	       reply.words[0] == 0;
}

static int whiteout_child_exists(const char *directory, const char *name)
{
	char relative[UNIONFS_MAX_PATH];

	return compose_child_path(directory, name, relative) == 0 &&
	       whiteout_exists(relative);
}

static int read_upper_dir(struct unionfs_open *open, u64 index,
			  struct bunix_msg *reply, u64 *count)
{
	u64 remote_index = 0;
	u64 visible_index = 0;

	if (count != 0) {
		*count = 0;
	}
	if (open->remote_handle == 0) {
		return 0;
	}
	for (;;) {
		char name[BUNIX_IPC_DATA_BYTES];
		struct bunix_msg request = {
			.protocol = BUNIX_PROTO_VFS,
			.type = BUNIX_VFS_READDIR,
			.words = { open->remote_handle, remote_index, 0, 0 },
		};
		struct bunix_msg upper_reply;

		if (bunix_ipc_call(open->service, &request, &upper_reply) != 0 ||
		    upper_reply.words[0] != 0) {
			return 0;
		}
		unpack_name(name, upper_reply.words[2], upper_reply.words[3]);
		if (!name_is_valid(name)) {
			remote_index++;
			continue;
		}
		if (count != 0) {
			(*count)++;
		}
		if (visible_index == index) {
			*reply = upper_reply;
			reply->words[1] = (index + 1) |
					  (upper_reply.words[1] &
					   0xffffffff00000000UL);
			return 1;
		}
		remote_index++;
		visible_index++;
	}
}

static int read_lower_dir(struct unionfs_open *open, u64 index,
			  struct bunix_msg *reply)
{
	u64 current = 0;

	for (u64 i = 0; i < root_entry_count; i++) {
		const struct rootfs_entry *entry = &root_entries[i];
		const char *name;

		if (!path_is_child_of(entry->path, open->relative_path)) {
			continue;
		}
		name = path_name_in_dir(entry->path, open->relative_path);
		if (!name_is_valid(name)) {
			continue;
		}
		if (whiteout_child_exists(open->relative_path, name) ||
		    upper_child_exists(open->relative_path, name)) {
			continue;
		}
		if (current == index) {
			reply->words[0] = 0;
			reply->words[1] = (index + 1) |
					 ((u64)entry->type << 32);
			pack_bytes(&reply->words[2],
				   (const unsigned char *)name,
				   str_len(name) + 1);
			return 1;
		}
		current++;
	}
	return 0;
}

static void readdir_merged(struct unionfs_open *open, struct bunix_msg *message,
			   struct bunix_msg *reply)
{
	u64 upper_count = 0;
	const u64 index = message->words[1];

	if (open == 0 || open->kind != UNIONFS_OPEN_DIR) {
		reply->words[0] = BUNIX_VFS_ERR_NOTDIR;
		return;
	}
	if (read_upper_dir(open, index, reply, &upper_count)) {
		return;
	}
	if (index >= upper_count &&
	    read_lower_dir(open, index - upper_count, reply)) {
		reply->words[1] = (index + 1) |
				  (reply->words[1] & 0xffffffff00000000UL);
		return;
	}
	reply->words[0] = BUNIX_VFS_ERR_NOENT;
}

static void forward_open_handle(struct bunix_msg *message,
				struct bunix_msg *reply)
{
	struct unionfs_open *open = open_from_handle(message->words[0]);

	if (open == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	if (open->kind == UNIONFS_OPEN_DIR) {
		if (message->type == BUNIX_VFS_CLOSE) {
			forget_open(open);
			reply->words[0] = 0;
		} else if (message->type == BUNIX_VFS_STAT_META) {
			stat_root_dir(reply);
		} else if (message->type == BUNIX_VFS_STAT) {
			reply->words[0] = 0;
			reply->words[1] = 0;
			reply->words[2] = BUNIX_VFS_TYPE_DIRECTORY;
		} else if (message->type == BUNIX_VFS_READDIR) {
			readdir_merged(open, message, reply);
		} else {
			reply->words[0] = BUNIX_VFS_ERR_ACCESS;
		}
		return;
	}
	if (open->kind == UNIONFS_OPEN_LOWER) {
		if (message->type == BUNIX_VFS_CLOSE) {
			forget_open(open);
			reply->words[0] = 0;
		} else if (message->type == BUNIX_VFS_STAT_META) {
			stat_lower(reply, &open->lower);
		} else if (message->type == BUNIX_VFS_STAT) {
			reply->words[0] = 0;
			reply->words[1] = open->lower.size;
			reply->words[2] = open->lower.type;
		} else if (message->type == BUNIX_VFS_READ_FILE_BUFFER &&
			   open->lower.type == BUNIX_VFS_TYPE_REGULAR &&
			   message->cap != 0 &&
			   (message->cap_rights & BUNIX_RIGHT_SEND) != 0) {
			u64 len = message->words[2];
			int nread;

			if (message->words[1] >= open->lower.size) {
				reply->words[0] = 0;
				reply->words[1] = 0;
				return;
			}
			if (len > open->lower.size - message->words[1]) {
				len = open->lower.size - message->words[1];
			}
			nread = block_read_buffer(open->lower.offset +
						  message->words[1],
						  message->cap, len);
			reply->words[0] = nread < 0 ? (u64)-1 : 0;
			reply->words[1] = nread < 0 ? 0 : (u64)nread;
		} else {
			if ((message->type == BUNIX_VFS_WRITE_FILE_BUFFER ||
			     message->type == BUNIX_VFS_TRUNCATE ||
			     message->type == BUNIX_VFS_CHMOD ||
			     message->type == BUNIX_VFS_CHOWN) &&
			    copy_lower_to_upper(open, message->words[3]) == 0) {
				forward_open_handle(message, reply);
			} else {
				reply->words[0] = BUNIX_VFS_ERR_ACCESS;
			}
		}
		return;
	}
	message->words[0] = open->remote_handle;
	if (bunix_ipc_call(open->service, message, reply) != 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	if (message->type == BUNIX_VFS_CLOSE && reply->words[0] == 0) {
		open->remote_handle = 0;
		forget_open(open);
	}
}

int main(void)
{
	const char online[] = "unionfs: online\n";
	const char mounted[] = "unionfs: mounted\n";
	struct bunix_msg message;
	struct bunix_msg mkdir_reply;

	bunix_console_log(online, sizeof(online) - 1);
	bunix_tree_init(&whiteouts);
	bunix_u64_tree_init(&open_files);
	vfs_service = resolve_service(BUNIX_SERVICE_VFS, BUNIX_RIGHT_SEND);
	tmpfs_service = resolve_service(BUNIX_SERVICE_TMPFS, BUNIX_RIGHT_SEND);
	block_service = resolve_service(BUNIX_SERVICE_BLOCK, BUNIX_RIGHT_SEND);
	if (vfs_service == 0 ||
	    tmpfs_service == 0 ||
	    block_service == 0 ||
	    rootfs_mount() != 0 ||
	    service_path_call(tmpfs_service, BUNIX_VFS_MKDIR_BUFFER,
			      "/tmp/union", (u64)0777 << 32,
			      &mkdir_reply) != 0 ||
	    mkdir_reply.words[0] != 0 ||
	    vfs_mount_path("/mnt/union") != 0 ||
	    register_service(BUNIX_SERVICE_UNIONFS, BUNIX_HANDLE_SELF) != 0) {
		return 1;
	}
	bunix_console_log(mounted, sizeof(mounted) - 1);

	for (;;) {
		struct bunix_msg reply = {
			.protocol = BUNIX_PROTO_VFS,
			.type = 0,
			.cap_rights = 0,
			.words = { 0, 0, 0, 0 },
		};
		char path[UNIONFS_MAX_PATH];

		if (bunix_ipc_recv(BUNIX_HANDLE_SELF, &message) != 0 ||
		    message.protocol != BUNIX_PROTO_VFS) {
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
		case BUNIX_VFS_CREATE_BUFFER:
		case BUNIX_VFS_MKDIR_BUFFER:
		case BUNIX_VFS_CHMOD_BUFFER:
		case BUNIX_VFS_CHOWN_BUFFER:
		case BUNIX_VFS_UNLINK_BUFFER:
		case BUNIX_VFS_RMDIR_BUFFER:
			if (read_resolved_path(&message, path) != 0) {
				reply.words[0] = BUNIX_VFS_ERR_NOENT;
			} else {
				reply_mutate(&message, &reply, path);
			}
			break;
		case BUNIX_VFS_TRUNCATE:
			if (message.cap != 0) {
				if (read_resolved_path(&message, path) != 0) {
					reply.words[0] = BUNIX_VFS_ERR_NOENT;
				} else {
					reply_mutate(&message, &reply, path);
				}
			} else {
				forward_open_handle(&message, &reply);
			}
			break;
		case BUNIX_VFS_STAT:
		case BUNIX_VFS_STAT_META:
		case BUNIX_VFS_READ_FILE_BUFFER:
		case BUNIX_VFS_WRITE_FILE_BUFFER:
		case BUNIX_VFS_READDIR:
		case BUNIX_VFS_CHMOD:
		case BUNIX_VFS_CHOWN:
		case BUNIX_VFS_CLOSE:
			forward_open_handle(&message, &reply);
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
