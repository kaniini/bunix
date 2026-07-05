#include <bunix/alloc.h>
#include <bunix/libbunix.h>
#include <bunix/tree.h>

enum {
	VFS_HANDLE_NAMES = 3,
	ROOTFS_MAX_SYMLINKS = 8,
	VFS_MAX_PATH = 4096,
	VFS_OPEN_ROOT = 1,
	VFS_OPEN_REMOTE = 2,
};

struct rootfs_entry {
	struct bunix_tree_node node;
	char *path;
	u64 offset;
	u64 size;
	unsigned int uid;
	unsigned int gid;
	unsigned int mode;
	unsigned int type;
};

struct vfs_mount {
	struct bunix_tree_node node;
	char *path;
	u64 service;
	u64 fstype;
};

struct vfs_open {
	struct bunix_u64_tree_node node;
	u64 id;
	u64 kind;
	const struct rootfs_entry *entry;
	u64 service;
	u64 remote_handle;
	char *path;
	u64 type;
};

static struct bunix_tree rootfs_by_path;
static struct bunix_tree mounts;
static struct bunix_u64_tree open_files;
static u64 next_open_id = 1;
static u64 user_service;
static u64 root_block;

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

static void unpack_path(char *path, const u64 *words)
{
	for (u64 i = 0; i < VFS_MAX_PATH; i++) {
		path[i] = '\0';
	}
	unpack_bytes((unsigned char *)path, words, 16);
}

static int path_eq(const char *left, const char *right)
{
	for (u64 i = 0; i < VFS_MAX_PATH; i++) {
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

	while (len < VFS_MAX_PATH && text[len] != '\0') {
		len++;
	}

	return len;
}

static char *str_dup(const char *text)
{
	const u64 len = str_len(text);
	char *copy;

	if (text == 0 || len == VFS_MAX_PATH) {
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

static int read_path_buffer_at(u64 buffer, u64 offset, u64 len, char *path)
{
	if (buffer == 0 || len == 0 || len > VFS_MAX_PATH) {
		return -1;
	}
	for (u64 i = 0; i < VFS_MAX_PATH; i++) {
		path[i] = '\0';
	}
	if (bunix_buffer_read(buffer, offset, path, len) != 0 ||
	    path[len - 1] != '\0') {
		return -1;
	}
	return str_len(path) < VFS_MAX_PATH ? 0 : -1;
}

static int path_component_eq(const char *component, const char *literal)
{
	u64 i = 0;

	while (component[i] != '\0' && literal[i] != '\0') {
		if (component[i] != literal[i]) {
			return 0;
		}
		i++;
	}
	return component[i] == literal[i];
}

static struct vfs_open *open_from_handle(u64 handle);
static const struct rootfs_entry *root_file_from_handle(u64 handle);

static int vfs_resolve_path(const char *cwd, const char *input, char *out)
{
	char temp[VFS_MAX_PATH];
	u64 pos = 0;
	u64 in = 0;

	if (input == 0 || input[0] == '\0' ||
	    cwd == 0 || cwd[0] != '/') {
		return -1;
	}
	if (input[0] == '/') {
		temp[pos++] = '/';
	} else {
		const u64 cwd_len = str_len(cwd);

		for (u64 i = 0; i < cwd_len && pos < sizeof(temp) - 1; i++) {
			temp[pos++] = cwd[i];
		}
		if (pos == 0) {
			temp[pos++] = '/';
		}
	}

	while (input[in] != '\0') {
		char component[VFS_MAX_PATH];
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
		if (comp_len == 0 || path_component_eq(component, ".")) {
			continue;
		}
		if (path_component_eq(component, "..")) {
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
	for (u64 i = 0; i <= pos; i++) {
		out[i] = temp[i];
	}
	return 0;
}

static int vfs_resolve_with_base(u64 base_handle, const char *cwd,
				 const char *input, char *out, u64 *error)
{
	const struct rootfs_entry *base;

	if (input != 0 && input[0] == '/') {
		return vfs_resolve_path(cwd, input, out);
	}
	if (base_handle == 0) {
		return vfs_resolve_path(cwd, input, out);
	}

	base = root_file_from_handle(base_handle);
	if (base == 0) {
		struct vfs_open *open = open_from_handle(base_handle);

		if (open != 0 && open->kind == VFS_OPEN_REMOTE) {
			if (open->type != BUNIX_VFS_TYPE_DIRECTORY ||
			    open->path == 0) {
				if (error != 0) {
					*error = BUNIX_VFS_ERR_NOTDIR;
				}
				return -1;
			}
			return vfs_resolve_path(open->path, input, out);
		}
	}
	if (base == 0 || base->type != BUNIX_VFS_TYPE_DIRECTORY) {
		if (error != 0) {
			*error = BUNIX_VFS_ERR_NOTDIR;
		}
		return -1;
	}
	return vfs_resolve_path(base->path, input, out);
}

static int path_is_at_or_under(const char *path, const char *prefix)
{
	const u64 prefix_len = str_len(prefix);

	if (prefix_len == 0) {
		return 0;
	}
	if (prefix_len == 1 && prefix[0] == '/') {
		return path[0] == '/';
	}
	for (u64 i = 0; i < prefix_len; i++) {
		if (path[i] != prefix[i]) {
			return 0;
		}
	}
	return path[prefix_len] == '\0' || path[prefix_len] == '/';
}

static struct vfs_mount *mount_for_path(const char *path)
{
	struct vfs_mount *best = 0;
	u64 best_len = 0;

	for (struct bunix_tree_node *node = bunix_tree_first_node(&mounts);
	     node != 0; node = bunix_tree_next_node(node)) {
		struct vfs_mount *mount =
			(struct vfs_mount *)node->value;
		const u64 len = mount != 0 ? str_len(mount->path) : 0;

		if (mount == 0 || len < best_len ||
		    !path_is_at_or_under(path, mount->path)) {
			continue;
		}
		best = mount;
		best_len = len;
	}
	return best;
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

static const struct rootfs_entry *rootfs_find_ref(const char *path)
{
	return (const struct rootfs_entry *)
		bunix_tree_get(&rootfs_by_path, path);
}

static int rootfs_read_entry_text(u64 block, const struct rootfs_entry *entry,
				  char *out, u64 max)
{
	if (entry == 0 || out == 0 || max == 0 ||
	    entry->size + 1 > max ||
	    block_read_bytes(block, entry->offset,
			     (unsigned char *)out, entry->size) != 0) {
		return -1;
	}
	out[entry->size] = '\0';
	return str_len(out) == entry->size ? 0 : -1;
}

static int path_dirname(const char *path, char *out)
{
	u64 len;

	if (path == 0 || out == 0 || path[0] != '/') {
		return -1;
	}
	len = str_len(path);
	while (len > 1 && path[len - 1] != '/') {
		len--;
	}
	if (len <= 1) {
		out[0] = '/';
		out[1] = '\0';
		return 0;
	}
	len--;
	for (u64 i = 0; i < len; i++) {
		out[i] = path[i];
	}
	out[len] = '\0';
	return 0;
}

static const struct rootfs_entry *rootfs_resolve_ref(u64 block,
						     const char *path,
						     char *resolved)
{
	char current[VFS_MAX_PATH];

	if (vfs_resolve_path("/", path, current) != 0) {
		return 0;
	}
	for (u64 depth = 0; depth < ROOTFS_MAX_SYMLINKS; depth++) {
		const struct rootfs_entry *entry = rootfs_find_ref(current);
		char target[VFS_MAX_PATH];
		char base[VFS_MAX_PATH];

		if (entry == 0 || entry->type != BUNIX_VFS_TYPE_SYMLINK) {
			if (resolved != 0) {
				for (u64 i = 0; i <= str_len(current); i++) {
					resolved[i] = current[i];
				}
			}
			return entry;
		}
		if (rootfs_read_entry_text(block, entry, target,
					   sizeof(target)) != 0 ||
		    path_dirname(current, base) != 0 ||
		    vfs_resolve_path(base, target, current) != 0) {
			return 0;
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

	for (struct bunix_tree_node *node = bunix_tree_first_node(&rootfs_by_path);
	     node != 0; node = bunix_tree_next_node(node)) {
		const struct rootfs_entry *entry =
			(const struct rootfs_entry *)node->value;

		if (!entry_is_child_of(entry, directory->path)) {
			continue;
		}
		if (current == index) {
			return entry;
		}
		current++;
	}

	return 0;
}

static u64 open_root_file(const struct rootfs_entry *entry)
{
	struct vfs_open *open;
	u64 handle;

	if (entry == 0) {
		return 0;
	}

	open = (struct vfs_open *)bunix_calloc(1, sizeof(*open));
	if (open == 0) {
		return 0;
	}
	open->kind = VFS_OPEN_ROOT;
	open->entry = entry;
	handle = next_open_id++;
	while (handle == 0 || bunix_u64_tree_get(&open_files, handle) != 0) {
		handle = next_open_id++;
	}
	open->id = handle;
	if (bunix_u64_tree_insert_node(&open_files, &open->node, handle,
				       (u64)open) != 0) {
		bunix_free(open);
		return 0;
	}
	return handle;
}

static u64 open_remote_file(u64 service, u64 remote_handle, const char *path,
			    u64 type)
{
	struct vfs_open *open;
	u64 handle;

	if (service == 0 || remote_handle == 0 || path == 0) {
		return 0;
	}
	open = (struct vfs_open *)bunix_calloc(1, sizeof(*open));
	if (open == 0) {
		return 0;
	}
	open->kind = VFS_OPEN_REMOTE;
	open->service = service;
	open->remote_handle = remote_handle;
	open->path = str_dup(path);
	open->type = type;
	if (open->path == 0) {
		bunix_free(open);
		return 0;
	}
	handle = next_open_id++;
	while (handle == 0 || bunix_u64_tree_get(&open_files, handle) != 0) {
		handle = next_open_id++;
	}
	open->id = handle;
	if (bunix_u64_tree_insert_node(&open_files, &open->node, handle,
				       (u64)open) != 0) {
		bunix_free(open->path);
		bunix_free(open);
		return 0;
	}
	return handle;
}

static struct vfs_open *open_from_handle(u64 handle)
{
	return (struct vfs_open *)bunix_u64_tree_get(&open_files, handle);
}

static const struct rootfs_entry *root_file_from_handle(u64 handle)
{
	struct vfs_open *open = open_from_handle(handle);

	return open != 0 && open->kind == VFS_OPEN_ROOT ? open->entry : 0;
}

static void forget_open_file(u64 handle)
{
	struct vfs_open *open = open_from_handle(handle);

	if (open != 0) {
		bunix_u64_tree_remove_node(&open_files, &open->node);
		if (open->path != 0) {
			bunix_free(open->path);
		}
		bunix_free(open);
	}
}

static int forward_mount_path(struct vfs_mount *mount, struct bunix_msg *message,
			      struct bunix_msg *reply, const char *path)
{
	if (mount == 0 || mount->service == 0) {
		reply->words[0] = BUNIX_VFS_ERR_NOENT;
		return -1;
	}
	if (bunix_ipc_call(mount->service, message, reply) != 0) {
		reply->words[0] = (u64)-1;
		return -1;
	}
	if ((message->type == BUNIX_VFS_OPEN ||
	     message->type == BUNIX_VFS_OPEN_BUFFER) &&
	    reply->words[0] == 0) {
		const u64 remote_handle = reply->words[1];
		const u64 local_handle =
			open_remote_file(mount->service, remote_handle, path,
					 reply->words[3]);

		if (local_handle == 0) {
			struct bunix_msg close = {
				.protocol = BUNIX_PROTO_VFS,
				.type = BUNIX_VFS_CLOSE,
				.sender = 0,
				.cap_rights = 0,
				.reply = 0,
				.cap = 0,
				.words = { remote_handle, 0, 0, 0 },
			};
			struct bunix_msg ignored;

			(void)bunix_ipc_call(mount->service, &close, &ignored);
			reply->words[0] = (u64)-1;
			return -1;
		}
		reply->words[1] = local_handle;
	}
	return 0;
}

static int forward_mount_buffer_path(struct vfs_mount *mount,
				     struct bunix_msg *message,
				     struct bunix_msg *reply,
				     const char *path)
{
	const char root[] = "/";
	const u64 cwd_len = 2;
	const u64 path_len = str_len(path) + 1;
	const long buffer = bunix_buffer_create(cwd_len + path_len);
	struct bunix_msg forwarded = *message;
	int result;

	if (buffer < 0 ||
	    bunix_buffer_write((u64)buffer, 0, root, cwd_len) != 0 ||
	    bunix_buffer_write((u64)buffer, cwd_len, path, path_len) != 0) {
		if (buffer >= 0) {
			bunix_handle_close((u64)buffer);
		}
		reply->words[0] = (u64)-1;
		return -1;
	}

	forwarded.cap = (u64)buffer;
	forwarded.cap_rights = BUNIX_RIGHT_RECV;
	if (forwarded.type == BUNIX_VFS_OPEN) {
		forwarded.type = BUNIX_VFS_OPEN_BUFFER;
	}
	forwarded.words[0] = cwd_len;
	forwarded.words[1] = path_len;
	forwarded.words[2] = 0;
	result = forward_mount_path(mount, &forwarded, reply, path);
	bunix_handle_close((u64)buffer);
	return result;
}

static int forward_remote_handle(struct vfs_open *open,
				 struct bunix_msg *message,
				 struct bunix_msg *reply)
{
	if (open == 0 || open->kind != VFS_OPEN_REMOTE ||
	    open->service == 0 || open->remote_handle == 0) {
		reply->words[0] = (u64)-1;
		return -1;
	}

	message->words[0] = open->remote_handle;
	if (bunix_ipc_call(open->service, message, reply) != 0) {
		reply->words[0] = (u64)-1;
		return -1;
	}
	return 0;
}

static void vfs_open_path(struct bunix_msg *message,
			  struct bunix_msg *reply,
			  const char *path);

static void close_open_handle(u64 handle)
{
	struct vfs_open *open = open_from_handle(handle);

	if (open == 0) {
		return;
	}
	if (open->kind == VFS_OPEN_REMOTE) {
		struct bunix_msg close = {
			.protocol = BUNIX_PROTO_VFS,
			.type = BUNIX_VFS_CLOSE,
			.words = { open->remote_handle, 0, 0, 0 },
		};
		struct bunix_msg ignored;

		(void)bunix_ipc_call(open->service, &close, &ignored);
	}
	forget_open_file(handle);
}

static void read_file_handle_to_buffer(u64 handle,
				       u64 offset,
				       u64 len,
				       u64 buffer,
				       struct bunix_msg *reply)
{
	struct vfs_open *open = open_from_handle(handle);
	const struct rootfs_entry *entry = root_file_from_handle(handle);
	int read_len;

	if (open != 0 && open->kind == VFS_OPEN_REMOTE) {
		struct bunix_msg read = {
			.protocol = BUNIX_PROTO_VFS,
			.type = BUNIX_VFS_READ_FILE_BUFFER,
			.cap_rights = BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP,
			.cap = buffer,
			.words = { handle, offset, len, 0 },
		};

		(void)forward_remote_handle(open, &read, reply);
		return;
	}
	if (entry == 0 ||
	    entry->type != BUNIX_VFS_TYPE_REGULAR ||
	    buffer == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	if (offset >= entry->size) {
		len = 0;
	} else if (len > entry->size - offset) {
		len = entry->size - offset;
	}
	if (len == 0) {
		reply->words[0] = 0;
		reply->words[1] = 0;
		return;
	}
	read_len = block_read_buffer(root_block, entry->offset + offset,
				     buffer, len);
	if (read_len < 0) {
		reply->words[0] = (u64)-1;
		reply->words[1] = 0;
	} else {
		reply->words[0] = 0;
		reply->words[1] = (u64)read_len;
	}
}

static void read_path_to_buffer(const char *path,
				u64 offset,
				u64 len,
				u64 buffer,
				struct bunix_msg *reply)
{
	struct bunix_msg open_request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_OPEN_BUFFER,
		.words = { 0, 0, 0, 0 },
	};
	struct bunix_msg open_reply = {
		.words = { (u64)-1, 0, 0, 0 },
	};

	vfs_open_path(&open_request, &open_reply, path);
	if (open_reply.words[0] != 0) {
		reply->words[0] = open_reply.words[0];
		return;
	}
	read_file_handle_to_buffer(open_reply.words[1], offset, len, buffer,
				   reply);
	close_open_handle(open_reply.words[1]);
}

static long mount_translator(const char *path, u64 service, u64 fstype)
{
	struct vfs_mount *mount;

	if (path == 0 || path[0] != '/' || service == 0) {
		return -1;
	}

	mount = (struct vfs_mount *)bunix_tree_get(&mounts, path);
	if (mount != 0) {
		mount->service = service;
		mount->fstype = fstype;
		return 0;
	}

	mount = (struct vfs_mount *)bunix_calloc(1, sizeof(*mount));
	if (mount == 0) {
		return -1;
	}
	mount->path = str_dup(path);
	if (mount->path == 0) {
		bunix_free(mount);
		return -1;
	}
	mount->service = service;
	mount->fstype = fstype;
	if (bunix_tree_insert_node(&mounts, &mount->node, mount->path,
				   (u64)mount) != 0) {
		bunix_free(mount->path);
		bunix_free(mount);
		return -1;
	}
	return 0;
}

static void notify_procfs_mount(const char *path, u64 fstype)
{
	struct vfs_mount *procfs =
		(struct vfs_mount *)bunix_tree_get(&mounts, "/proc");
	const u64 path_len = str_len(path) + 1;
	const long buffer = bunix_buffer_create(path_len);
	struct bunix_msg message = {
		.protocol = BUNIX_PROTO_PROCFS,
		.type = BUNIX_PROCFS_MOUNT_NOTIFY,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_RECV,
		.reply = 0,
		.cap = buffer < 0 ? 0 : (u64)buffer,
		.words = { path_len, fstype, 0, 0 },
	};

	if (procfs == 0 || procfs->service == 0 || buffer < 0) {
		if (buffer >= 0) {
			bunix_handle_close((u64)buffer);
		}
		return;
	}
	if (bunix_buffer_write((u64)buffer, 0, path, path_len) == 0) {
		(void)bunix_ipc_send(procfs->service, &message);
	}
	bunix_handle_close((u64)buffer);
}

static void vfs_open_path(struct bunix_msg *message, struct bunix_msg *reply,
			  const char *path)
{
	struct vfs_mount *mount = mount_for_path(path);
	const struct rootfs_entry *entry;
	u64 file;

	if (mount != 0) {
		struct bunix_msg forwarded = *message;

		if (message->type == BUNIX_VFS_OPEN ||
		    message->type == BUNIX_VFS_OPEN_BUFFER) {
			(void)forward_mount_buffer_path(mount, message, reply,
							path);
			return;
		}
		(void)forward_mount_path(mount, &forwarded, reply, path);
		return;
	}

	entry = rootfs_resolve_ref(root_block, path, 0);
	if (entry == 0) {
		reply->words[0] = BUNIX_VFS_ERR_NOENT;
		return;
	}
	if (entry->type == BUNIX_VFS_TYPE_DIRECTORY) {
		const u64 task = message->words[3] & 0xffffffff;

		if (!can_search_entry(entry, task) ||
		    !can_read_entry(entry, task)) {
			reply->words[0] = BUNIX_VFS_ERR_ACCESS;
			return;
		}
	} else if (!can_read_entry(entry, message->words[3] & 0xffffffff)) {
		reply->words[0] = BUNIX_VFS_ERR_ACCESS;
		return;
	}

	file = open_root_file(entry);
	if (file == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	reply->words[0] = 0;
	reply->words[1] = file;
	reply->words[2] = entry->size;
	reply->words[3] = entry->type;
	bunix_console_log("vfs: open\n", 10);
}

static void vfs_stat_path(struct bunix_msg *message, struct bunix_msg *reply,
			  const char *path)
{
	struct vfs_mount *mount = mount_for_path(path);
	const struct rootfs_entry *entry;
	const int nofollow = ((message->words[3] >> 32) & 1) != 0;

	if (mount != 0) {
		struct bunix_msg forwarded = *message;

		if (message->type == BUNIX_VFS_STAT_PATH_META_BUFFER) {
			(void)forward_mount_buffer_path(mount, message, reply,
							path);
			return;
		}
		(void)forward_mount_path(mount, &forwarded, reply, path);
		return;
	}

	entry = nofollow ? rootfs_find_ref(path) :
			   rootfs_resolve_ref(root_block, path, 0);
	if (entry == 0) {
		reply->words[0] = BUNIX_VFS_ERR_NOENT;
	} else {
		stat_meta_reply(reply, entry);
	}
}

static void vfs_access_path(struct bunix_msg *message, struct bunix_msg *reply,
			    const char *path)
{
	struct vfs_mount *mount = mount_for_path(path);
	const struct rootfs_entry *entry;
	const u64 task = message->words[3] & 0xffffffff;
	const u64 mask = message->words[3] >> 32;

	if (mount != 0) {
		struct bunix_msg forwarded = *message;

		if (message->type == BUNIX_VFS_ACCESS_BUFFER) {
			(void)forward_mount_buffer_path(mount, message, reply,
							path);
			return;
		}
		(void)forward_mount_path(mount, &forwarded, reply, path);
		return;
	}

	entry = rootfs_resolve_ref(root_block, path, 0);
	if (entry == 0) {
		reply->words[0] = BUNIX_VFS_ERR_NOENT;
		return;
	}
	if (mask != 0 && !user_can_access(task, entry, mask)) {
		reply->words[0] = BUNIX_VFS_ERR_ACCESS;
		return;
	}
	reply->words[0] = 0;
}

static void vfs_mutate_path(struct bunix_msg *message, struct bunix_msg *reply,
			    const char *path)
{
	struct vfs_mount *mount = mount_for_path(path);

	if (mount != 0) {
		(void)forward_mount_buffer_path(mount, message, reply, path);
		return;
	}
	reply->words[0] = BUNIX_VFS_ERR_ACCESS;
}

static void vfs_readlink_path(struct bunix_msg *message,
			      struct bunix_msg *reply, const char *path)
{
	struct vfs_mount *mount = mount_for_path(path);
	const struct rootfs_entry *entry;
	char target[VFS_MAX_PATH];

	if (mount != 0) {
		(void)forward_mount_buffer_path(mount, message, reply, path);
		return;
	}

	entry = rootfs_find_ref(path);
	if (entry == 0) {
		reply->words[0] = BUNIX_VFS_ERR_NOENT;
		return;
	}
	if (entry->type != BUNIX_VFS_TYPE_SYMLINK) {
		reply->words[0] = (u64)-1;
		return;
	}
	if (!can_read_entry(entry, message->words[3] & 0xffffffff) ||
	    rootfs_read_entry_text(root_block, entry, target,
				   sizeof(target)) != 0) {
		reply->words[0] = BUNIX_VFS_ERR_ACCESS;
		return;
	}
	reply->words[0] = 0;
	reply->words[1] = str_len(target);
	pack_bytes(&reply->words[2], (const unsigned char *)target,
		   str_len(target) + 1);
}

int main(void)
{
	const char online[] = "vfs: online\n";
	const char ready[] = "vfs: ready\n";
	struct bunix_msg message;

	bunix_console_log(online, sizeof(online) - 1);
	register_service(BUNIX_SERVICE_VFS, BUNIX_HANDLE_SELF);
	bunix_tree_init(&rootfs_by_path);
	bunix_tree_init(&mounts);
	bunix_u64_tree_init(&open_files);
	next_open_id = 1;
	bunix_console_log(ready, sizeof(ready) - 1);

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
		case BUNIX_VFS_READ:
			reply.words[0] = (u64)-1;
			break;
		case BUNIX_VFS_READ_PATH: {
			char path[VFS_MAX_PATH];
			const u64 offset = message.words[2];
			u64 len = message.words[3];
			const long buffer =
				bunix_buffer_create(BUNIX_IPC_DATA_BYTES);
			unsigned char data[BUNIX_IPC_DATA_BYTES];

			unpack_path(path, &message.words[0]);
			if (buffer < 0) {
				reply.words[0] = (u64)-1;
				break;
			}
			if (len > BUNIX_IPC_DATA_BYTES) {
				len = BUNIX_IPC_DATA_BYTES;
			}
			read_path_to_buffer(path, offset, len, (u64)buffer,
					    &reply);
			if (reply.words[0] == 0 && reply.words[1] != 0 &&
			    bunix_buffer_read((u64)buffer, 0, data,
					      reply.words[1]) != 0) {
				reply.words[0] = (u64)-1;
				reply.words[1] = 0;
			}
			if (reply.words[0] == 0) {
				pack_bytes(&reply.words[2], data,
					   reply.words[1]);
			}
			bunix_handle_close((u64)buffer);
			break;
		}
		case BUNIX_VFS_READ_PATH_BUFFER: {
			char path[VFS_MAX_PATH];
			const u64 offset = message.words[2];
			u64 len = message.words[3];

			unpack_path(path, &message.words[0]);

			if (message.cap == 0 ||
			    (message.cap_rights & (BUNIX_RIGHT_SEND |
						   BUNIX_RIGHT_DUP)) !=
			    (BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP)) {
				reply.words[0] = (u64)-1;
				break;
			}
			read_path_to_buffer(path, offset, len, message.cap,
					    &reply);
			break;
		}
		case BUNIX_VFS_OPEN: {
			char path[VFS_MAX_PATH];

			unpack_path(path, &message.words[0]);
			vfs_open_path(&message, &reply, path);
			break;
		}
		case BUNIX_VFS_OPEN_BUFFER: {
			char cwd[VFS_MAX_PATH];
			char input[VFS_MAX_PATH];
			char path[VFS_MAX_PATH];
			const u64 cwd_len = message.words[0];
			const u64 path_len = message.words[1];
			u64 error = (u64)-1;

			if ((message.cap_rights & BUNIX_RIGHT_RECV) == 0 ||
			    read_path_buffer_at(message.cap, 0, cwd_len,
						cwd) != 0 ||
			    read_path_buffer_at(message.cap, cwd_len, path_len,
						input) != 0 ||
			    vfs_resolve_with_base(message.words[2], cwd,
						  input, path, &error) != 0) {
				reply.words[0] = error;
				break;
			}
			vfs_open_path(&message, &reply, path);
			break;
		}
		case BUNIX_VFS_STAT_PATH_META: {
			char path[VFS_MAX_PATH];

			unpack_path(path, &message.words[0]);
			vfs_stat_path(&message, &reply, path);
			break;
		}
		case BUNIX_VFS_STAT_PATH_META_BUFFER: {
			char cwd[VFS_MAX_PATH];
			char input[VFS_MAX_PATH];
			char path[VFS_MAX_PATH];
			const u64 cwd_len = message.words[0];
			const u64 path_len = message.words[1];
			u64 error = (u64)-1;

			if ((message.cap_rights & BUNIX_RIGHT_RECV) == 0 ||
			    read_path_buffer_at(message.cap, 0, cwd_len,
						cwd) != 0 ||
			    read_path_buffer_at(message.cap, cwd_len, path_len,
						input) != 0 ||
			    vfs_resolve_with_base(message.words[2], cwd,
						  input, path, &error) != 0) {
				reply.words[0] = error;
				break;
			}
			vfs_stat_path(&message, &reply, path);
			break;
		}
		case BUNIX_VFS_ACCESS_BUFFER: {
			char cwd[VFS_MAX_PATH];
			char input[VFS_MAX_PATH];
			char path[VFS_MAX_PATH];
			const u64 cwd_len = message.words[0];
			const u64 path_len = message.words[1];
			u64 error = (u64)-1;

			if ((message.cap_rights & BUNIX_RIGHT_RECV) == 0 ||
			    read_path_buffer_at(message.cap, 0, cwd_len,
						cwd) != 0 ||
			    read_path_buffer_at(message.cap, cwd_len, path_len,
						input) != 0 ||
			    vfs_resolve_with_base(message.words[2], cwd,
						  input, path, &error) != 0) {
				reply.words[0] = error;
				break;
			}
			vfs_access_path(&message, &reply, path);
			break;
		}
		case BUNIX_VFS_CREATE_BUFFER:
		case BUNIX_VFS_CHMOD_BUFFER:
		case BUNIX_VFS_CHOWN_BUFFER:
		case BUNIX_VFS_MKDIR_BUFFER:
		case BUNIX_VFS_RMDIR_BUFFER:
		case BUNIX_VFS_UNLINK_BUFFER: {
			char cwd[VFS_MAX_PATH];
			char input[VFS_MAX_PATH];
			char path[VFS_MAX_PATH];
			const u64 cwd_len = message.words[0];
			const u64 path_len = message.words[1];
			u64 error = (u64)-1;

			if ((message.cap_rights & BUNIX_RIGHT_RECV) == 0 ||
			    read_path_buffer_at(message.cap, 0, cwd_len,
						cwd) != 0 ||
			    read_path_buffer_at(message.cap, cwd_len, path_len,
						input) != 0 ||
			    vfs_resolve_with_base(message.words[2], cwd,
						  input, path, &error) != 0) {
				reply.words[0] = error;
				break;
			}
			vfs_mutate_path(&message, &reply, path);
			break;
		}
		case BUNIX_VFS_CHMOD: {
			struct vfs_open *open = open_from_handle(message.words[0]);

			if (open != 0 && open->kind == VFS_OPEN_REMOTE) {
				(void)forward_remote_handle(open, &message, &reply);
			} else {
				reply.words[0] = BUNIX_VFS_ERR_ACCESS;
			}
			break;
		}
		case BUNIX_VFS_CHOWN: {
			struct vfs_open *open = open_from_handle(message.words[0]);

			if (open != 0 && open->kind == VFS_OPEN_REMOTE) {
				(void)forward_remote_handle(open, &message, &reply);
			} else {
				reply.words[0] = BUNIX_VFS_ERR_ACCESS;
			}
			break;
		}
		case BUNIX_VFS_TRUNCATE: {
			struct vfs_open *open = open_from_handle(message.words[0]);

			if (message.cap != 0) {
				char cwd[VFS_MAX_PATH];
				char input[VFS_MAX_PATH];
				char path[VFS_MAX_PATH];
				const u64 cwd_len = message.words[0];
				const u64 path_len = message.words[1];
				u64 error = (u64)-1;

				if ((message.cap_rights & BUNIX_RIGHT_RECV) == 0 ||
				    read_path_buffer_at(message.cap, 0, cwd_len,
							cwd) != 0 ||
				    read_path_buffer_at(message.cap, cwd_len,
							path_len, input) != 0 ||
				    vfs_resolve_with_base(message.words[2], cwd,
							  input, path,
							  &error) != 0) {
					reply.words[0] = error;
					break;
				}
				vfs_mutate_path(&message, &reply, path);
				break;
			}
			if (open != 0 && open->kind == VFS_OPEN_REMOTE) {
				(void)forward_remote_handle(open, &message, &reply);
			} else {
				reply.words[0] = BUNIX_VFS_ERR_ACCESS;
			}
			break;
		}
		case BUNIX_VFS_READLINK_BUFFER: {
			char cwd[VFS_MAX_PATH];
			char input[VFS_MAX_PATH];
			char path[VFS_MAX_PATH];
			const u64 cwd_len = message.words[0];
			const u64 path_len = message.words[1];
			u64 error = (u64)-1;

			if ((message.cap_rights & BUNIX_RIGHT_RECV) == 0 ||
			    read_path_buffer_at(message.cap, 0, cwd_len,
						cwd) != 0 ||
			    read_path_buffer_at(message.cap, cwd_len, path_len,
						input) != 0 ||
			    vfs_resolve_with_base(message.words[2], cwd,
						  input, path, &error) != 0) {
				reply.words[0] = error;
				break;
			}
			vfs_readlink_path(&message, &reply, path);
			break;
		}
		case BUNIX_VFS_STAT: {
			struct vfs_open *open = open_from_handle(message.words[0]);
			const struct rootfs_entry *entry =
				root_file_from_handle(message.words[0]);

			if (open != 0 && open->kind == VFS_OPEN_REMOTE) {
				(void)forward_remote_handle(open, &message, &reply);
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
			struct vfs_open *open = open_from_handle(message.words[0]);
			const struct rootfs_entry *entry =
				root_file_from_handle(message.words[0]);

			if (open != 0 && open->kind == VFS_OPEN_REMOTE) {
				(void)forward_remote_handle(open, &message, &reply);
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
			struct vfs_open *open = open_from_handle(message.words[0]);
			const struct rootfs_entry *entry =
				root_file_from_handle(message.words[0]);
			const u64 offset = message.words[1];
			u64 len = message.words[2];
			int read_len;

			if (open != 0 && open->kind == VFS_OPEN_REMOTE) {
				(void)forward_remote_handle(open, &message, &reply);
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

			read_len = block_read_buffer(root_block,
						     entry->offset + offset,
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
		case BUNIX_VFS_WRITE_FILE_BUFFER: {
			struct vfs_open *open = open_from_handle(message.words[0]);

			if (open != 0 && open->kind == VFS_OPEN_REMOTE) {
				(void)forward_remote_handle(open, &message, &reply);
			} else {
				reply.words[0] = BUNIX_VFS_ERR_ACCESS;
			}
			break;
		}
		case BUNIX_VFS_READDIR:
		case BUNIX_VFS_READDIR_BUFFER: {
			struct vfs_open *open = open_from_handle(message.words[0]);
			const struct rootfs_entry *directory =
				root_file_from_handle(message.words[0]);
			const struct rootfs_entry *child;
			const char *name;

			if (open != 0 && open->kind == VFS_OPEN_REMOTE) {
				(void)forward_remote_handle(open, &message, &reply);
				break;
			}
			if (message.type == BUNIX_VFS_READDIR_BUFFER &&
			    (message.cap == 0 ||
			     (message.cap_rights & BUNIX_RIGHT_SEND) == 0)) {
				reply.words[0] = (u64)-1;
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
			if (message.type == BUNIX_VFS_READDIR_BUFFER) {
				const u64 name_len = str_len(name);
				u64 written = name_len;

				if (written > message.words[3]) {
					written = message.words[3];
				}
				if (written != 0 &&
				    bunix_buffer_write(message.cap,
						       message.words[2],
						       name, written) != 0) {
					reply.words[0] = (u64)-1;
				} else {
					reply.words[2] = name_len;
					reply.words[3] = written;
				}
			} else {
				pack_bytes(&reply.words[2],
					   (const unsigned char *)name,
					   str_len(name) + 1);
			}
			break;
		}
		case BUNIX_VFS_CLOSE:
			if (open_from_handle(message.words[0]) != 0 &&
			    open_from_handle(message.words[0])->kind == VFS_OPEN_REMOTE) {
				const u64 local_handle = message.words[0];
				struct vfs_open *open = open_from_handle(message.words[0]);

				if (forward_remote_handle(open, &message, &reply) == 0 &&
				    reply.words[0] == 0) {
					forget_open_file(local_handle);
				}
				break;
			}
			if (root_file_from_handle(message.words[0]) == 0) {
				reply.words[0] = (u64)-1;
			} else {
				forget_open_file(message.words[0]);
				reply.words[0] = 0;
				bunix_console_log("vfs: close\n", 11);
			}
			break;
		case BUNIX_VFS_MOUNT: {
			char path[VFS_MAX_PATH];

			unpack_path(path, &message.words[0]);
			if (message.cap == 0 ||
			    (message.cap_rights & BUNIX_RIGHT_SEND) == 0 ||
			    mount_translator(path, message.cap,
					     message.words[2]) != 0) {
				reply.words[0] = (u64)-1;
			} else {
				reply.words[0] = 0;
				message.cap = 0;
				notify_procfs_mount(path, message.words[2]);
				bunix_console_log("vfs: mounted translator\n", 24);
			}
			break;
		}
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
