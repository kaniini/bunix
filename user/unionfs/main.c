#include <bunix/alloc.h>
#include <bunix/libbunix.h>
#include <bunix/tree.h>

enum {
	UNIONFS_HANDLE_NAMES = 3,
	UNIONFS_MAX_PATH = 4096,
	UNIONFS_OPEN_LOWER = 1,
	UNIONFS_OPEN_UPPER = 2,
	UNIONFS_OPEN_DIR = 3,
};

struct unionfs_open {
	struct bunix_u64_tree_node node;
	u64 id;
	u64 kind;
	u64 service;
	u64 remote_handle;
	u64 lower_layer_service;
	u64 lower_handle;
	u64 lower_type;
	u64 lower_size;
	char upper_path[UNIONFS_MAX_PATH];
	char relative_path[UNIONFS_MAX_PATH];
};

struct unionfs_whiteout {
	struct bunix_tree_node node;
	char *path;
};

static struct bunix_tree whiteouts;
static struct bunix_u64_tree open_files;
static u64 next_open_id = 1;
static u64 vfs_service;
static u64 upper_service;
static u64 lower_layer_service;
static char unionfs_mount_path[UNIONFS_MAX_PATH];
static char unionfs_upper_path[UNIONFS_MAX_PATH];
static char unionfs_lower_path[UNIONFS_MAX_PATH];

static int set_path(char *target, const char *path);
static int close_remote_handle(u64 service, u64 handle);

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

static int name_is_valid(const char *name)
{
	if (name == 0 || name[0] == '\0') {
		return 0;
	}
	for (u64 i = 0; i < UNIONFS_MAX_PATH && name[i] != '\0'; i++) {
		if (name[i] == '/') {
			return 0;
		}
	}
	return 1;
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

static int read_rename_buffer(const struct bunix_msg *message, char *old_path,
			      char *new_path)
{
	char old_cwd[UNIONFS_MAX_PATH];
	char new_cwd[UNIONFS_MAX_PATH];
	const u64 old_cwd_len = message->words[1] & 0xffffffff;
	const u64 old_path_len = message->words[1] >> 32;
	const u64 new_cwd_len = message->words[2] & 0xffffffff;
	const u64 new_path_len = message->words[2] >> 32;
	u64 offset = 0;

	if ((message->cap_rights & BUNIX_RIGHT_RECV) == 0 ||
	    read_path_buffer_at(message->cap, offset, old_cwd_len,
				old_cwd) != 0) {
		return -1;
	}
	offset += old_cwd_len;
	if (read_path_buffer_at(message->cap, offset, old_path_len,
				old_path) != 0) {
		return -1;
	}
	offset += old_path_len;
	if (read_path_buffer_at(message->cap, offset, new_cwd_len,
				new_cwd) != 0) {
		return -1;
	}
	offset += new_cwd_len;
	if (read_path_buffer_at(message->cap, offset, new_path_len,
				new_path) != 0) {
		return -1;
	}
	(void)old_cwd;
	(void)new_cwd;
	return old_path[0] == '/' && new_path[0] == '/' ? 0 : -1;
}

static int mounted_relative_path(const char *path, char *relative)
{
	const u64 mount_len = str_len(unionfs_mount_path);

	if (str_eq(unionfs_mount_path, "/")) {
		if (path == 0 || path[0] != '/') {
			return -1;
		}
		for (u64 i = 0; i < UNIONFS_MAX_PATH; i++) {
			relative[i] = path[i];
			if (relative[i] == '\0') {
				return 0;
			}
		}
		return -1;
	}
	if (str_eq(path, unionfs_mount_path)) {
		relative[0] = '/';
		relative[1] = '\0';
		return 0;
	}
	for (u64 i = 0; unionfs_mount_path[i] != '\0'; i++) {
		if (path[i] != unionfs_mount_path[i]) {
			return -1;
		}
	}
	if (path[mount_len] != '/') {
		return -1;
	}
	for (u64 i = 0; i < UNIONFS_MAX_PATH; i++) {
		relative[i] = path[mount_len + i];
		if (relative[i] == '\0') {
			return 0;
		}
	}
	return -1;
}

static int compose_upper_path(const char *relative, char *path)
{
	u64 pos = 0;

	if (unionfs_upper_path[0] == '\0') {
		return -1;
	}
	for (u64 i = 0; unionfs_upper_path[i] != '\0'; i++) {
		path[pos++] = unionfs_upper_path[i];
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

static int compose_layer_path(const char *base, const char *relative, char *path)
{
	u64 pos = 0;
	u64 i = 0;

	if (base == 0 || relative == 0 || base[0] != '/' ||
	    relative[0] != '/') {
		return -1;
	}
	if (str_eq(base, "/")) {
		return compose_lower_path(relative, path);
	}
	while (base[i] != '\0') {
		if (pos + 1 >= UNIONFS_MAX_PATH) {
			return -1;
		}
		path[pos++] = base[i++];
	}
	i = 0;
	while (relative[i] != '\0') {
		if (pos + 1 >= UNIONFS_MAX_PATH) {
			return -1;
		}
		path[pos++] = relative[i++];
	}
	path[pos] = '\0';
	return 0;
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

static void stat_root_dir(struct bunix_msg *reply)
{
	reply->words[0] = 0;
	reply->words[1] = 0;
	reply->words[2] = 0555 | ((u64)BUNIX_VFS_TYPE_DIRECTORY << 32);
	reply->words[3] = 0;
}

static int service_path_call(u64 service, u64 type, const char *path, u64 word3,
			     const struct bunix_msg *source,
			     struct bunix_msg *reply)
{
	const char root[] = "/";
	const u64 cwd_len = 2;
	const u64 path_len = str_len(path) + 1;
	const u64 out_cap = type == BUNIX_VFS_READLINK_BUFFER && source != 0 ?
			    source->words[3] >> 32 : 0;
	const long buffer = bunix_buffer_create(cwd_len + path_len + out_cap);
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = type,
		.cap_rights = BUNIX_RIGHT_RECV |
			      (out_cap != 0 ? BUNIX_RIGHT_SEND : 0),
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
	if (type == BUNIX_VFS_READLINK_BUFFER) {
		request.words[3] = (word3 & 0xffffffff) | (out_cap << 32);
	}
	if (bunix_ipc_call(service, &request, reply) == 0) {
		result = 0;
	}
	if (result == 0 && type == BUNIX_VFS_READLINK_BUFFER &&
	    out_cap != 0 && reply->words[0] == 0) {
		char target[UNIONFS_MAX_PATH];
		const u64 source_offset = source->words[0] + source->words[1];
		const u64 temp_offset = cwd_len + path_len;
		const u64 written = reply->words[3];

		if (source->cap == 0 ||
		    (source->cap_rights & BUNIX_RIGHT_SEND) == 0 ||
		    written > out_cap ||
		    (written != 0 &&
		     (bunix_buffer_read((u64)buffer, temp_offset, target,
					written) != 0 ||
		      bunix_buffer_write(source->cap, source_offset, target,
					 written) != 0))) {
			reply->words[0] = (u64)-1;
			result = -1;
		}
	}
	bunix_handle_close((u64)buffer);
	return result;
}

static int service_symlink_call(u64 service, const char *target,
				const char *path, u64 word3,
				struct bunix_msg *reply)
{
	const char root[] = "/";
	const u64 target_len = str_len(target) + 1;
	const u64 cwd_len = 2;
	const u64 path_len = str_len(path) + 1;
	const long buffer = bunix_buffer_create(target_len + cwd_len +
						path_len);
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_SYMLINK_BUFFER,
		.cap_rights = BUNIX_RIGHT_RECV,
		.words = { target_len, cwd_len, path_len, word3 },
	};
	int result = -1;

	if (buffer < 0 ||
	    bunix_buffer_write((u64)buffer, 0, target, target_len) != 0 ||
	    bunix_buffer_write((u64)buffer, target_len, root, cwd_len) != 0 ||
	    bunix_buffer_write((u64)buffer, target_len + cwd_len, path,
			       path_len) != 0) {
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

static int service_two_path_call(u64 service, unsigned int type,
				 const char *old_path, const char *new_path,
				 u64 word3, struct bunix_msg *reply)
{
	const char root[] = "/";
	const u64 cwd_len = 2;
	const u64 old_len = str_len(old_path) + 1;
	const u64 new_len = str_len(new_path) + 1;
	const long buffer = bunix_buffer_create(cwd_len + old_len +
						cwd_len + new_len);
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = type,
		.cap_rights = BUNIX_RIGHT_RECV,
		.words = {
			0,
			cwd_len | (old_len << 32),
			cwd_len | (new_len << 32),
			word3,
		},
	};
	int result = -1;

	if (buffer < 0 ||
	    bunix_buffer_write((u64)buffer, 0, root, cwd_len) != 0 ||
	    bunix_buffer_write((u64)buffer, cwd_len, old_path, old_len) != 0 ||
	    bunix_buffer_write((u64)buffer, cwd_len + old_len, root,
			       cwd_len) != 0 ||
	    bunix_buffer_write((u64)buffer, cwd_len + old_len + cwd_len,
			       new_path, new_len) != 0) {
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

static int service_rename_call(u64 service, const char *old_path,
			       const char *new_path, u64 word3,
			       struct bunix_msg *reply)
{
	return service_two_path_call(service, BUNIX_VFS_RENAME_BUFFER,
				     old_path, new_path, word3, reply);
}

static int lower_path_call(u64 type, const char *relative, u64 word3,
			   const struct bunix_msg *source,
			   struct bunix_msg *reply)
{
	char path[UNIONFS_MAX_PATH];

	if (lower_layer_service == 0 ||
	    compose_layer_path(unionfs_lower_path, relative, path) != 0) {
		return -1;
	}
	return service_path_call(lower_layer_service, type, path, word3, source,
				 reply);
}

static int lower_is_directory(const char *relative)
{
	struct bunix_msg reply;

	return lower_path_call(BUNIX_VFS_STAT_PATH_META_BUFFER, relative, 0,
			       0, &reply) == 0 &&
	       reply.words[0] == 0 &&
	       (reply.words[2] >> 32) == BUNIX_VFS_TYPE_DIRECTORY;
}

static int relative_parent_path(const char *path, char *parent)
{
	u64 last = 0;

	if (path == 0 || parent == 0 || path[0] != '/') {
		return -1;
	}
	if (path[1] == '\0') {
		parent[0] = '/';
		parent[1] = '\0';
		return 0;
	}
	for (u64 i = 1; path[i] != '\0'; i++) {
		if (path[i] == '/') {
			last = i;
		}
	}
	if (last == 0) {
		parent[0] = '/';
		parent[1] = '\0';
		return 0;
	}
	for (u64 i = 0; i < last; i++) {
		parent[i] = path[i];
	}
	parent[last] = '\0';
	return 0;
}

static int upper_directory_exists(const char *relative)
{
	char upper[UNIONFS_MAX_PATH];
	struct bunix_msg reply;

	return compose_upper_path(relative, upper) == 0 &&
	       service_path_call(upper_service, BUNIX_VFS_STAT_PATH_META_BUFFER,
				 upper, 0, 0, &reply) == 0 &&
	       reply.words[0] == 0 &&
	       (reply.words[2] >> 32) == BUNIX_VFS_TYPE_DIRECTORY;
}

static int materialize_upper_directory(const char *relative, u64 task)
{
	char parent[UNIONFS_MAX_PATH];
	char upper[UNIONFS_MAX_PATH];
	struct bunix_msg stat_reply;
	struct bunix_msg mkdir_reply;
	u64 mode;

	if (relative == 0 || relative[0] != '/') {
		return -1;
	}
	if (str_eq(relative, "/") || upper_directory_exists(relative)) {
		return 0;
	}
	if (relative_parent_path(relative, parent) != 0 ||
	    materialize_upper_directory(parent, task) != 0 ||
	    lower_path_call(BUNIX_VFS_STAT_PATH_META_BUFFER, relative, 0, 0,
			    &stat_reply) != 0 ||
	    stat_reply.words[0] != 0 ||
	    (stat_reply.words[2] >> 32) != BUNIX_VFS_TYPE_DIRECTORY ||
	    compose_upper_path(relative, upper) != 0) {
		return -1;
	}
	mode = stat_reply.words[2] & 0777;
	if (service_path_call(upper_service, BUNIX_VFS_MKDIR_BUFFER, upper,
			      mode << 32, 0, &mkdir_reply) != 0) {
		return -1;
	}
	if (mkdir_reply.words[0] == 0) {
		return 0;
	}
	return upper_directory_exists(relative) ? 0 : -1;
}

static int materialize_upper_parent(const char *relative, u64 task)
{
	char parent[UNIONFS_MAX_PATH];

	if (relative_parent_path(relative, parent) != 0) {
		return -1;
	}
	return materialize_upper_directory(parent, task);
}

static int vfs_mount_path(const char *path)
{
	struct bunix_msg reply;

	return bunix_ipc_call_path(vfs_service, BUNIX_PROTO_VFS,
				   BUNIX_VFS_MOUNT_BUFFER, path,
				   BUNIX_SERVICE_UNIONFS, 0, 0, &reply) == 0 &&
	       reply.words[0] == 0 ? 0 : -1;
}

static u64 vfs_resolve_mount_service(const char *path)
{
	struct bunix_msg reply;

	if (vfs_service == 0 ||
	    bunix_ipc_call_path(vfs_service, BUNIX_PROTO_VFS,
				BUNIX_VFS_RESOLVE_MOUNT_BUFFER, path,
				0, 0, 0, &reply) != 0 ||
	    reply.words[0] != 0) {
		return 0;
	}
	return reply.cap;
}

static int set_path(char *target, const char *path)
{
	u64 i;

	if (target == 0 || path == 0 || path[0] != '/') {
		return -1;
	}
	for (i = 0; i + 1 < UNIONFS_MAX_PATH && path[i] != '\0'; i++) {
		target[i] = path[i];
	}
	if (path[i] != '\0') {
		return -1;
	}
	target[i] = '\0';
	return 0;
}

static int unionfs_set_upper_path(const char *path)
{
	u64 service;
	struct bunix_msg stat_reply;
	struct bunix_msg mkdir_reply;

	if (set_path(unionfs_upper_path, path) != 0) {
		unionfs_upper_path[0] = '\0';
		return -1;
	}
	service = vfs_resolve_mount_service(unionfs_upper_path);
	if (service == 0) {
		unionfs_upper_path[0] = '\0';
		return -1;
	}
	upper_service = service;
	if (service_path_call(upper_service, BUNIX_VFS_STAT_PATH_META_BUFFER,
			      unionfs_upper_path, 0, 0, &stat_reply) == 0 &&
	    stat_reply.words[0] == 0 &&
	    (stat_reply.words[2] >> 32) == BUNIX_VFS_TYPE_DIRECTORY) {
		return 0;
	}
	if (service_path_call(upper_service, BUNIX_VFS_MKDIR_BUFFER,
			      unionfs_upper_path, (u64)0777 << 32,
			      0, &mkdir_reply) != 0 ||
	    mkdir_reply.words[0] != 0) {
		unionfs_upper_path[0] = '\0';
		return -1;
	}
	return 0;
}

static int unionfs_set_lower_path(const char *path)
{
	u64 service;

	if (set_path(unionfs_lower_path, path) != 0) {
		unionfs_lower_path[0] = '\0';
		return -1;
	}
	service = vfs_resolve_mount_service(unionfs_lower_path);
	if (service == 0) {
		unionfs_lower_path[0] = '\0';
		return -1;
	}
	lower_layer_service = service;
	return 0;
}

static int unionfs_mount_root_path(const char *path)
{
	if (unionfs_lower_path[0] == '\0' ||
	    unionfs_upper_path[0] == '\0' ||
	    set_path(unionfs_mount_path, path) != 0 ||
	    vfs_mount_path(unionfs_mount_path) != 0) {
		unionfs_mount_path[0] = '\0';
		return -1;
	}
	return 0;
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

static u64 remember_lower_open(u64 service, u64 remote_handle, u64 type,
			       u64 size, const char *relative)
{
	struct unionfs_open *open;
	u64 id;

	if (service == 0 || remote_handle == 0 || relative == 0) {
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
	open->service = service;
	open->remote_handle = remote_handle;
	open->lower_layer_service = service;
	open->lower_handle = remote_handle;
	open->lower_type = type;
	open->lower_size = size;
	(void)compose_upper_path(relative, open->upper_path);
	for (u64 i = 0; i < UNIONFS_MAX_PATH; i++) {
		open->relative_path[i] = relative[i];
		if (relative[i] == '\0') {
			break;
		}
	}
	if (bunix_u64_tree_insert_node(&open_files, &open->node, id,
				       (u64)open) != 0) {
		(void)close_remote_handle(service, remote_handle);
		bunix_free(open);
		return 0;
	}
	return id;
}

static u64 remember_dir_open(const char *relative, u64 upper_handle,
			     u64 lower_handle)
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
	open->service = upper_service;
	open->remote_handle = upper_handle;
	open->lower_layer_service = lower_layer_service;
	open->lower_handle = lower_handle;
	open->lower_type = BUNIX_VFS_TYPE_DIRECTORY;
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

static int copy_lower_file_to_upper(u64 lower_service, u64 lower_handle,
				    const char *relative, const char *upper,
				    u64 lower_type, u64 size, u64 mode,
				    u64 task, u64 *upper_handle)
{
	enum {
		COPY_CHUNK = 4096,
	};
	struct bunix_msg reply;
	u64 remote_handle;
	u64 done = 0;

	if (relative == 0 || upper == 0 ||
	    lower_type != BUNIX_VFS_TYPE_REGULAR ||
	    materialize_upper_parent(relative, task) != 0 ||
	    service_path_call(upper_service, BUNIX_VFS_CREATE_BUFFER,
			      upper,
			      (task & 0xffffffff) |
			      (mode << 32),
			      0, &reply) != 0) {
		return -1;
	}
	if (service_path_call(upper_service, BUNIX_VFS_OPEN_BUFFER,
			      upper, 0, 0, &reply) != 0 ||
	    reply.words[0] != 0) {
		return -1;
	}
	remote_handle = reply.words[1];
	while (done < size) {
		u64 len = size - done;
		const long buffer = bunix_buffer_create(COPY_CHUNK);
		struct bunix_msg read = {
			.protocol = BUNIX_PROTO_VFS,
			.type = BUNIX_VFS_READ_FILE_BUFFER,
			.cap_rights = BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP,
			.words = { lower_handle, done, 0, 0 },
		};
		struct bunix_msg write = {
			.protocol = BUNIX_PROTO_VFS,
			.type = BUNIX_VFS_WRITE_FILE_BUFFER,
			.cap_rights = BUNIX_RIGHT_RECV | BUNIX_RIGHT_DUP,
			.words = { remote_handle, done, 0, 0 },
		};

		if (len > COPY_CHUNK) {
			len = COPY_CHUNK;
		}
		if (buffer < 0) {
			if (buffer >= 0) {
				bunix_handle_close((u64)buffer);
			}
			return -1;
		}
		read.cap = (u64)buffer;
		read.words[2] = len;
		if (bunix_ipc_call(lower_service, &read, &reply) != 0 ||
		    reply.words[0] != 0 ||
		    reply.words[1] != len) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
		write.cap = (u64)buffer;
		write.words[2] = len;
		if (bunix_ipc_call(upper_service, &write, &reply) != 0 ||
		    reply.words[0] != 0 ||
		    reply.words[1] != len) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
		bunix_handle_close((u64)buffer);
		done += len;
	}
	if (upper_handle != 0) {
		*upper_handle = remote_handle;
	} else {
		(void)close_remote_handle(upper_service, remote_handle);
	}
	return 0;
}

static int copy_lower_to_upper(struct unionfs_open *open, u64 task)
{
	struct bunix_msg stat_request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_STAT_META,
		.words = { 0, 0, 0, 0 },
	};
	struct bunix_msg stat_reply;
	u64 remote_handle = 0;

	if (open == 0 || open->kind != UNIONFS_OPEN_LOWER) {
		return -1;
	}
	stat_request.words[0] = open->lower_handle;
	if (bunix_ipc_call(open->lower_layer_service, &stat_request, &stat_reply) != 0 ||
	    stat_reply.words[0] != 0 ||
	    copy_lower_file_to_upper(open->lower_layer_service,
				     open->lower_handle, open->relative_path,
				     open->upper_path, open->lower_type,
				     stat_reply.words[1],
				     stat_reply.words[2] & 0777, task,
				     &remote_handle) != 0) {
		return -1;
	}
	open->kind = UNIONFS_OPEN_UPPER;
	open->service = upper_service;
	open->remote_handle = remote_handle;
	return 0;
}

static int copy_lower_path_to_upper(const char *relative, u64 task)
{
	char lower[UNIONFS_MAX_PATH];
	char upper[UNIONFS_MAX_PATH];
	struct bunix_msg stat_reply;
	struct bunix_msg open_reply;
	u64 remote_handle = 0;
	int result;

	if (relative == 0 ||
	    compose_lower_path(relative, lower) != 0 ||
	    compose_upper_path(relative, upper) != 0 ||
	    lower_path_call(BUNIX_VFS_STAT_PATH_META_BUFFER, relative, 0, 0,
			    &stat_reply) != 0 ||
	    stat_reply.words[0] != 0 ||
	    lower_path_call(BUNIX_VFS_OPEN_BUFFER, relative, 0, 0,
			    &open_reply) != 0 ||
	    open_reply.words[0] != 0) {
		return -1;
	}
	result = copy_lower_file_to_upper(lower_layer_service,
					  open_reply.words[1], relative, upper,
					  stat_reply.words[2] >> 32,
					  stat_reply.words[1],
					  stat_reply.words[2] & 0777, task,
					  &remote_handle);
	(void)close_remote_handle(lower_layer_service, open_reply.words[1]);
	if (result != 0) {
		return -1;
	}
	if (remote_handle != 0) {
		(void)close_remote_handle(upper_service, remote_handle);
	}
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
	if ((open->kind == UNIONFS_OPEN_LOWER ||
	     open->kind == UNIONFS_OPEN_DIR) &&
	    open->lower_layer_service != 0 &&
	    open->lower_handle != 0 &&
	    open->lower_handle != open->remote_handle) {
		(void)close_remote_handle(open->lower_layer_service,
					  open->lower_handle);
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
	int upper_ok;

	if (mounted_relative_path(path, relative) != 0 ||
	    compose_lower_path(relative, lower) != 0) {
		bunix_console_log("unionfs: open path failed\n",
				  sizeof("unionfs: open path failed\n") - 1);
		reply->words[0] = BUNIX_VFS_ERR_NOENT;
		return;
	}
	upper_ok = compose_upper_path(relative, upper) == 0;
	if (upper_ok &&
	    service_path_call(upper_service, message->type, upper,
			      message->words[3], message, reply) == 0 &&
	    reply->words[0] == 0) {
		return;
	}
	if (upper_ok && whiteout_exists(relative)) {
		reply->words[0] = BUNIX_VFS_ERR_NOENT;
		return;
	}
	const int nofollow = message->type == BUNIX_VFS_READLINK_BUFFER ||
			     (((message->words[3] >> 32) & 1) != 0 &&
			      message->type == BUNIX_VFS_STAT_PATH_META_BUFFER);
	u64 word3 = message->words[3];

	if (nofollow && message->type == BUNIX_VFS_STAT_PATH_META_BUFFER) {
		word3 |= 1UL << 32;
	}
	if (lower_path_call(message->type, lower, word3, message, reply) != 0) {
		reply->words[0] = BUNIX_VFS_ERR_NOENT;
	}
}

static void reply_open(struct bunix_msg *message, struct bunix_msg *reply,
		       const char *path)
{
	char relative[UNIONFS_MAX_PATH];
	char upper[UNIONFS_MAX_PATH];
	char lower[UNIONFS_MAX_PATH];
	struct bunix_msg layer_reply;
	u64 handle;
	u64 upper_handle = 0;
	u64 upper_type = 0;
	u64 upper_size = 0;
	u64 lower_handle = 0;
	u64 lower_type = 0;
	u64 lower_size = 0;
	int upper_ok;
	int lower_dir;

	if (mounted_relative_path(path, relative) != 0 ||
	    compose_lower_path(relative, lower) != 0) {
		reply->words[0] = BUNIX_VFS_ERR_NOENT;
		return;
	}
	upper_ok = compose_upper_path(relative, upper) == 0;
	if (!upper_ok ||
	    service_path_call(upper_service, BUNIX_VFS_OPEN_BUFFER, upper,
			      message->words[3], 0, &layer_reply) != 0 ||
	    layer_reply.words[0] != 0) {
		if (upper_ok && whiteout_exists(relative)) {
			bunix_console_log("unionfs: open whiteout\n",
					  sizeof("unionfs: open whiteout\n") - 1);
			reply->words[0] = BUNIX_VFS_ERR_NOENT;
			return;
		}
		if (lower_path_call(BUNIX_VFS_OPEN_BUFFER, lower,
				    message->words[3], 0, &layer_reply) != 0 ||
		    layer_reply.words[0] != 0) {
			bunix_console_log("unionfs: open lower missing\n",
					  sizeof("unionfs: open lower missing\n") - 1);
			reply->words[0] = BUNIX_VFS_ERR_NOENT;
			return;
		}
		lower_handle = layer_reply.words[1];
		lower_size = layer_reply.words[2];
		lower_type = layer_reply.words[3];
		if (lower_type == BUNIX_VFS_TYPE_DIRECTORY) {
			handle = remember_dir_open(relative, 0, lower_handle);
			if (handle == 0) {
				bunix_console_log("unionfs: open dir handle\n",
						  sizeof("unionfs: open dir handle\n") - 1);
				(void)close_remote_handle(lower_layer_service,
							  lower_handle);
				reply->words[0] = (u64)-1;
				return;
			}
			reply->words[0] = 0;
			reply->words[1] = handle;
			reply->words[2] = 0;
			reply->words[3] = BUNIX_VFS_TYPE_DIRECTORY;
			return;
		}
		handle = remember_lower_open(lower_layer_service, lower_handle,
					     lower_type, lower_size, relative);
		if (handle == 0) {
			bunix_console_log("unionfs: open lower handle\n",
					  sizeof("unionfs: open lower handle\n") - 1);
			reply->words[0] = (u64)-1;
			return;
		}
		reply->words[0] = 0;
		reply->words[1] = handle;
		reply->words[2] = lower_size;
		reply->words[3] = lower_type;
		return;
	}
	upper_handle = layer_reply.words[1];
	upper_size = layer_reply.words[2];
	upper_type = layer_reply.words[3];
	lower_dir = lower_is_directory(lower) &&
		    (!upper_ok || !whiteout_exists(relative));
	if (upper_type == BUNIX_VFS_TYPE_DIRECTORY || lower_dir) {
		if (lower_dir &&
		    lower_path_call(BUNIX_VFS_OPEN_BUFFER, lower,
				    message->words[3], 0, &layer_reply) == 0 &&
		    layer_reply.words[0] == 0) {
			lower_handle = layer_reply.words[1];
		}
		handle = remember_dir_open(relative,
					   upper_type == BUNIX_VFS_TYPE_DIRECTORY ?
					   upper_handle : 0,
					   lower_handle);
		if (handle == 0) {
			(void)close_remote_handle(upper_service, upper_handle);
			(void)close_remote_handle(lower_layer_service, lower_handle);
			reply->words[0] = (u64)-1;
			return;
		}
		reply->words[0] = 0;
		reply->words[1] = handle;
		reply->words[2] = 0;
		reply->words[3] = BUNIX_VFS_TYPE_DIRECTORY;
		return;
	}

	handle = remember_upper_open(upper_service, upper_handle);
	if (handle == 0) {
		(void)close_remote_handle(upper_service, upper_handle);
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
	    ((message->type == BUNIX_VFS_CREATE_BUFFER ||
	      message->type == BUNIX_VFS_MKNOD_BUFFER ||
	      message->type == BUNIX_VFS_MKDIR_BUFFER) &&
	     materialize_upper_parent(relative, message->words[3]) != 0) ||
	    service_path_call(upper_service, message->type, upper,
			      message->words[3], 0, reply) != 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	if (message->type == BUNIX_VFS_CREATE_BUFFER ||
	    message->type == BUNIX_VFS_MKNOD_BUFFER ||
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
	const u64 want_type = message->type == BUNIX_VFS_RMDIR_BUFFER ?
			      BUNIX_VFS_TYPE_DIRECTORY :
			      BUNIX_VFS_TYPE_REGULAR;
	struct bunix_msg lower_reply;

	if (lower_path_call(BUNIX_VFS_STAT_PATH_META_BUFFER, lower, 0, 0,
			    &lower_reply) == 0 &&
	    lower_reply.words[0] == 0 &&
	    (lower_reply.words[2] >> 32) == want_type &&
	    whiteout_add(relative) == 0) {
		reply->words[0] = 0;
	}
}

static void reply_symlink(struct bunix_msg *message, struct bunix_msg *reply,
			  const char *target, const char *path)
{
	char relative[UNIONFS_MAX_PATH];
	char upper[UNIONFS_MAX_PATH];

	if (mounted_relative_path(path, relative) != 0 ||
	    compose_upper_path(relative, upper) != 0 ||
	    materialize_upper_parent(relative, message->words[3]) != 0 ||
	    service_symlink_call(upper_service, target, upper,
				 message->words[3] & 0xffffffff,
				 reply) != 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	if (reply->words[0] == 0) {
		whiteout_remove(relative);
	}
}

static void reply_rename(struct bunix_msg *message, struct bunix_msg *reply,
			 const char *old_path, const char *new_path)
{
	char old_relative[UNIONFS_MAX_PATH];
	char new_relative[UNIONFS_MAX_PATH];
	char old_upper[UNIONFS_MAX_PATH];
	char new_upper[UNIONFS_MAX_PATH];

	if (mounted_relative_path(old_path, old_relative) != 0 ||
	    mounted_relative_path(new_path, new_relative) != 0 ||
	    compose_upper_path(old_relative, old_upper) != 0 ||
	    compose_upper_path(new_relative, new_upper) != 0 ||
	    materialize_upper_parent(new_relative, message->words[3]) != 0 ||
	    service_rename_call(upper_service, old_upper, new_upper,
				message->words[3], reply) != 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	if (reply->words[0] == BUNIX_VFS_ERR_NOENT &&
	    !whiteout_exists(old_relative) &&
	    copy_lower_path_to_upper(old_relative, message->words[3]) == 0 &&
	    service_rename_call(upper_service, old_upper, new_upper,
				message->words[3], reply) != 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	if (reply->words[0] == 0) {
		(void)whiteout_add(old_relative);
		whiteout_remove(new_relative);
	}
}

static void reply_link(struct bunix_msg *message, struct bunix_msg *reply,
		       const char *old_path, const char *new_path)
{
	char old_relative[UNIONFS_MAX_PATH];
	char new_relative[UNIONFS_MAX_PATH];
	char old_upper[UNIONFS_MAX_PATH];
	char new_upper[UNIONFS_MAX_PATH];

	if (mounted_relative_path(old_path, old_relative) != 0 ||
	    mounted_relative_path(new_path, new_relative) != 0 ||
	    compose_upper_path(old_relative, old_upper) != 0 ||
	    compose_upper_path(new_relative, new_upper) != 0 ||
	    materialize_upper_parent(new_relative, message->words[3]) != 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	if (service_two_path_call(upper_service, BUNIX_VFS_LINK_BUFFER,
				  old_upper, new_upper, message->words[3],
				  reply) != 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	if (reply->words[0] == BUNIX_VFS_ERR_NOENT &&
	    !whiteout_exists(old_relative) &&
	    copy_lower_path_to_upper(old_relative, message->words[3]) == 0 &&
	    service_two_path_call(upper_service, BUNIX_VFS_LINK_BUFFER,
				  old_upper, new_upper, message->words[3],
				  reply) != 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	if (reply->words[0] == 0) {
		whiteout_remove(new_relative);
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
	return service_path_call(upper_service,
				 BUNIX_VFS_STAT_PATH_META_BUFFER, upper, 0,
				 0, &reply) == 0 &&
	       reply.words[0] == 0;
}

static int remote_readdir_name(u64 service, u64 handle, u64 index,
			       u64 scratch_buffer, char *name, u64 name_cap,
			       u64 *type)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_READDIR_BUFFER,
		.cap_rights = BUNIX_RIGHT_SEND,
		.cap = scratch_buffer,
		.words = { handle, index, 0, name_cap - 1 },
	};
	struct bunix_msg reply;

	if (service == 0 || handle == 0 || scratch_buffer == 0 ||
	    name == 0 || name_cap == 0 || type == 0) {
		return -1;
	}
	for (u64 i = 0; i < name_cap; i++) {
		name[i] = '\0';
	}
	if (bunix_ipc_call(service, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return 0;
	}
	const u64 name_len = reply.words[2];
	const u64 written = reply.words[3];

	if (name_len >= name_cap || written < name_len ||
	    (name_len != 0 &&
	     bunix_buffer_read(scratch_buffer, 0, name, name_len) != 0)) {
		return -1;
	}
	name[name_len] = '\0';
	*type = reply.words[1] >> 32;
	return 1;
}

static void readdir_reply_name(const struct bunix_msg *message,
			       struct bunix_msg *reply, const char *name,
			       u64 type)
{
	const u64 name_len = str_len(name);

	reply->words[0] = 0;
	reply->words[1] = (message->words[1] + 1) | (type << 32);
	u64 written = name_len;

	if (message->cap == 0 ||
	    (message->cap_rights & BUNIX_RIGHT_SEND) == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	if (written > message->words[3]) {
		written = message->words[3];
	}
	if (written != 0 &&
	    bunix_buffer_write(message->cap, message->words[2],
			       name, written) != 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	reply->words[2] = name_len;
	reply->words[3] = written;
}

static int whiteout_child_exists(const char *directory, const char *name)
{
	char relative[UNIONFS_MAX_PATH];

	return compose_child_path(directory, name, relative) == 0 &&
	       whiteout_exists(relative);
}

static int read_upper_dir(struct unionfs_open *open,
			  const struct bunix_msg *message,
			  u64 scratch_buffer, u64 index,
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
		char name[UNIONFS_MAX_PATH];
		u64 type = 0;
		const int result = remote_readdir_name(open->service,
						       open->remote_handle,
						       remote_index,
						       scratch_buffer, name,
						       sizeof(name), &type);

		if (result <= 0) {
			return 0;
		}
		if (!name_is_valid(name)) {
			remote_index++;
			continue;
		}
		if (count != 0) {
			(*count)++;
		}
		if (visible_index == index) {
			readdir_reply_name(message, reply, name, type);
			return 1;
		}
		remote_index++;
		visible_index++;
	}
}

static int read_lower_dir(struct unionfs_open *open,
			  const struct bunix_msg *message,
			  u64 scratch_buffer, u64 index,
			  struct bunix_msg *reply)
{
	u64 remote_index = 0;
	u64 visible_index = 0;

	if (open->lower_handle == 0) {
		return 0;
	}
	for (;;) {
		char name[UNIONFS_MAX_PATH];
		u64 type = 0;
		const int result = remote_readdir_name(open->lower_layer_service,
						       open->lower_handle,
						       remote_index,
						       scratch_buffer, name,
						       sizeof(name), &type);

		if (result <= 0) {
			return 0;
		}
		if (!name_is_valid(name)) {
			remote_index++;
			continue;
		}
		if (whiteout_child_exists(open->relative_path, name) ||
		    upper_child_exists(open->relative_path, name)) {
			remote_index++;
			continue;
		}
		if (visible_index == index) {
			readdir_reply_name(message, reply, name, type);
			return 1;
		}
		remote_index++;
		visible_index++;
	}
}

static void readdir_merged(struct unionfs_open *open, struct bunix_msg *message,
			   struct bunix_msg *reply)
{
	u64 upper_count = 0;
	const u64 index = message->words[1];
	long scratch_buffer;

	if (open == 0 || open->kind != UNIONFS_OPEN_DIR) {
		reply->words[0] = BUNIX_VFS_ERR_NOTDIR;
		return;
	}
	if (message->cap == 0 ||
	    (message->cap_rights & BUNIX_RIGHT_SEND) == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	scratch_buffer = bunix_buffer_create(UNIONFS_MAX_PATH);
	if (scratch_buffer <= 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	if (read_upper_dir(open, message, (u64)scratch_buffer, index, reply,
			   &upper_count)) {
		bunix_handle_close((u64)scratch_buffer);
		return;
	}
	if (index >= upper_count &&
	    read_lower_dir(open, message, (u64)scratch_buffer,
			   index - upper_count, reply)) {
		reply->words[1] = (index + 1) |
				  (reply->words[1] & 0xffffffff00000000UL);
		bunix_handle_close((u64)scratch_buffer);
		return;
	}
	bunix_handle_close((u64)scratch_buffer);
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
		} else if (message->type == BUNIX_VFS_READDIR_BUFFER) {
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
		} else if (message->type == BUNIX_VFS_STAT_META ||
			   message->type == BUNIX_VFS_READ_FILE_BUFFER) {
			message->words[0] = open->lower_handle;
			if (bunix_ipc_call(open->lower_layer_service, message, reply) != 0) {
				reply->words[0] = (u64)-1;
			}
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

	bunix_console_log(online, sizeof(online) - 1);
	bunix_tree_init(&whiteouts);
	bunix_u64_tree_init(&open_files);
	vfs_service = resolve_service(BUNIX_SERVICE_VFS, BUNIX_RIGHT_SEND);
	if (vfs_service == 0 ||
	    register_service(BUNIX_SERVICE_UNIONFS, BUNIX_HANDLE_SELF) != 0) {
		return 1;
	}

	for (;;) {
		struct bunix_msg reply = {
			.protocol = BUNIX_PROTO_VFS,
			.type = 0,
			.cap_rights = 0,
			.words = { 0, 0, 0, 0 },
		};
		char path[UNIONFS_MAX_PATH];

			if (bunix_ipc_recv(BUNIX_HANDLE_SELF, &message) != 0) {
				continue;
			}
			if (message.protocol == BUNIX_PROTO_UNIONFS) {
				reply.protocol = BUNIX_PROTO_UNIONFS;
				reply.type = message.type;
				if (bunix_read_path_cap(&message, path,
							sizeof(path)) != 0) {
					reply.words[0] = (u64)-1;
					if (message.cap != 0) {
						bunix_handle_close(message.cap);
					}
					bunix_ipc_send(message.reply, &reply);
					continue;
				}
				switch (message.type) {
			case BUNIX_UNIONFS_SET_LOWER:
				reply.words[0] =
					(u64)unionfs_set_lower_path(path);
				break;
			case BUNIX_UNIONFS_SET_UPPER:
				reply.words[0] =
					(u64)unionfs_set_upper_path(path);
				break;
			case BUNIX_UNIONFS_MOUNT_PATH:
				reply.words[0] =
					(u64)unionfs_mount_root_path(path);
				if (reply.words[0] == 0) {
					bunix_console_log(mounted,
							  sizeof(mounted) - 1);
				}
				break;
				default:
					reply.words[0] = (u64)-1;
					break;
				}
				if (message.cap != 0) {
					bunix_handle_close(message.cap);
				}
				bunix_ipc_send(message.reply, &reply);
				continue;
			}
		if (message.protocol != BUNIX_PROTO_VFS) {
			continue;
		}
		reply.protocol = BUNIX_PROTO_VFS;
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
		case BUNIX_VFS_MKNOD_BUFFER:
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
		case BUNIX_VFS_SYMLINK_BUFFER: {
			char target[UNIONFS_MAX_PATH];
			char cwd[UNIONFS_MAX_PATH];
			char input[UNIONFS_MAX_PATH];
			const u64 target_len = message.words[0];
			const u64 cwd_len = message.words[1];
			const u64 path_len = message.words[2];

			if ((message.cap_rights & BUNIX_RIGHT_RECV) == 0 ||
			    read_path_buffer_at(message.cap, 0, target_len,
						target) != 0 ||
			    read_path_buffer_at(message.cap, target_len,
						cwd_len, cwd) != 0 ||
			    read_path_buffer_at(message.cap,
						target_len + cwd_len,
						path_len, input) != 0 ||
			    input[0] != '/' ||
			    target[0] == '\0') {
				reply.words[0] = BUNIX_VFS_ERR_NOENT;
			} else {
				(void)cwd;
				reply_symlink(&message, &reply, target, input);
			}
			break;
		}
		case BUNIX_VFS_RENAME_BUFFER:
		case BUNIX_VFS_LINK_BUFFER: {
			char old_path[UNIONFS_MAX_PATH];
			char new_path[UNIONFS_MAX_PATH];

			if (read_rename_buffer(&message, old_path,
					       new_path) != 0) {
				reply.words[0] = BUNIX_VFS_ERR_NOENT;
			} else if (message.type == BUNIX_VFS_RENAME_BUFFER) {
				reply_rename(&message, &reply, old_path,
					     new_path);
			} else {
				reply_link(&message, &reply, old_path, new_path);
			}
			break;
		}
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
		case BUNIX_VFS_STAT_META:
		case BUNIX_VFS_READ_FILE_BUFFER:
		case BUNIX_VFS_WRITE_FILE_BUFFER:
		case BUNIX_VFS_READDIR_BUFFER:
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
