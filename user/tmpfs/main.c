#include <bunix/alloc.h>
#include <bunix/libbunix.h>
#include <bunix/tree.h>

enum {
	TMPFS_HANDLE_NAMES = 3,
	TMPFS_MAX_PATH = 4096,
	TMPFS_OPEN_DIR = 1,
	TMPFS_OPEN_FILE = 2,
};

struct tmpfs_root {
	struct bunix_tree_node node;
	char *path;
};

struct tmpfs_inode {
	char *data;
	u64 size;
	u64 capacity;
	u64 refs;
	unsigned int type;
	unsigned int mode;
	unsigned int uid;
	unsigned int gid;
};

struct tmpfs_file {
	struct bunix_tree_node node;
	char *path;
	u64 refs;
	int deleted;
	struct tmpfs_inode *inode;
};

struct tmpfs_open {
	struct bunix_u64_tree_node node;
	u64 id;
	u64 kind;
	struct tmpfs_file *file;
	char path[TMPFS_MAX_PATH];
};

static struct bunix_tree roots;
static struct bunix_tree files;
static struct bunix_u64_tree open_files;
static u64 next_open_id = 1;
static u64 user_service;
static u64 vfs_service;

static u64 str_len(const char *text)
{
	u64 len = 0;

	while (len < TMPFS_MAX_PATH && text[len] != '\0') {
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

static char *str_dup(const char *text)
{
	const u64 len = str_len(text);
	char *copy;

	if (text == 0 || len == TMPFS_MAX_PATH) {
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

static int path_has_prefix_child(const char *path, const char *prefix)
{
	u64 i = 0;

	while (prefix[i] != '\0') {
		if (path[i] != prefix[i]) {
			return 0;
		}
		i++;
	}
	return path[i] == '/';
}

static int path_is_root(const char *path)
{
	return bunix_tree_get(&roots, path) != 0;
}

static int path_parent_path(const char *path, char *parent)
{
	u64 last = 0;

	if (path == 0 || path[0] != '/') {
		return -1;
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

static const char *path_name_in_dir(const char *path, const char *dir)
{
	const u64 len = str_len(dir);

	return path + len + 1;
}

static int file_is_child_of(const struct tmpfs_file *file, const char *dir)
{
	const u64 len = str_len(dir);
	u64 pos;

	if (file == 0 || file->deleted || !path_has_prefix_child(file->path, dir)) {
		return 0;
	}
	pos = len + 1;
	while (file->path[pos] != '\0') {
		if (file->path[pos] == '/') {
			return 0;
		}
		pos++;
	}
	return 1;
}

static int read_path_buffer_at(u64 buffer, u64 offset, u64 len, char *path)
{
	if (buffer == 0 || len == 0 || len > TMPFS_MAX_PATH) {
		return -1;
	}
	for (u64 i = 0; i < TMPFS_MAX_PATH; i++) {
		path[i] = '\0';
	}
	if (bunix_buffer_read(buffer, offset, path, len) != 0 ||
	    path[len - 1] != '\0') {
		return -1;
	}
	return 0;
}

static int read_resolved_path(const struct bunix_msg *message, char *path)
{
	char cwd[TMPFS_MAX_PATH];
	char input[TMPFS_MAX_PATH];

	if ((message->cap_rights & BUNIX_RIGHT_RECV) == 0 ||
	    read_path_buffer_at(message->cap, 0, message->words[0], cwd) != 0 ||
	    read_path_buffer_at(message->cap, message->words[0],
				message->words[1], input) != 0) {
		return -1;
	}
	(void)cwd;
	if (input[0] != '/') {
		return -1;
	}
	for (u64 i = 0; i < TMPFS_MAX_PATH; i++) {
		path[i] = input[i];
		if (input[i] == '\0') {
			return 0;
		}
	}
	return -1;
}

static int read_symlink_buffer(const struct bunix_msg *message, char *target,
			       char *path)
{
	char cwd[TMPFS_MAX_PATH];
	const u64 target_len = message->words[0];
	const u64 cwd_len = message->words[1];
	const u64 path_len = message->words[2];

	if ((message->cap_rights & BUNIX_RIGHT_RECV) == 0 ||
	    read_path_buffer_at(message->cap, 0, target_len, target) != 0 ||
	    read_path_buffer_at(message->cap, target_len, cwd_len, cwd) != 0 ||
	    read_path_buffer_at(message->cap, target_len + cwd_len,
				path_len, path) != 0) {
		return -1;
	}
	(void)cwd;
	return path[0] == '/' && target[0] != '\0' ? 0 : -1;
}

static int read_rename_buffer(const struct bunix_msg *message, char *old_path,
			      char *new_path)
{
	char old_cwd[TMPFS_MAX_PATH];
	char new_cwd[TMPFS_MAX_PATH];
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

static struct tmpfs_file *file_find(const char *path)
{
	return (struct tmpfs_file *)bunix_tree_get(&files, path);
}

static int path_parent_exists(const char *path)
{
	char parent[TMPFS_MAX_PATH];
	struct tmpfs_file *file;

	if (path_parent_path(path, parent) != 0) {
		return 0;
	}
	if (path_is_root(parent)) {
		return 1;
	}
	file = file_find(parent);
	return file != 0 && file->inode->type == BUNIX_VFS_TYPE_DIRECTORY;
}

static u64 resolve_service(u64 service, unsigned int rights);

static u64 service_user(void)
{
	if (user_service == 0) {
		user_service = resolve_service(BUNIX_SERVICE_USER,
					       BUNIX_RIGHT_SEND);
	}
	return user_service;
}

static long user_credential(u64 task, u64 type, u64 *value)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_USER,
		.type = (unsigned int)type,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { task, 0, 0, 0 },
	};
	struct bunix_msg reply;
	const u64 user = service_user();

	if (value == 0 || user == 0 ||
	    bunix_ipc_call(user, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return -1;
	}
	*value = reply.words[1];
	return 0;
}

static int task_can_access(u64 task, const struct tmpfs_file *file, u64 mask)
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

	if (file == 0) {
		return 0;
	}
	if (task == 0 || mask == 0) {
		return 1;
	}
	request.words[1] = file->inode->uid;
	request.words[2] = file->inode->gid;
	request.words[3] = ((u64)file->inode->mode << 32) | mask;
	if (user == 0 ||
	    bunix_ipc_call(user, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return 0;
	}
	return reply.words[1] != 0;
}

static int task_can_chmod(u64 task, const struct tmpfs_file *file)
{
	u64 euid = 0;

	if (task == 0) {
		return 1;
	}
	if (file == 0 || user_credential(task, BUNIX_USER_GETEUID, &euid) != 0) {
		return 0;
	}
	return euid == 0 || euid == file->inode->uid;
}

static int task_can_chown(u64 task)
{
	u64 euid = 0;

	if (task == 0) {
		return 1;
	}
	return user_credential(task, BUNIX_USER_GETEUID, &euid) == 0 &&
	       euid == 0;
}

static struct tmpfs_inode root_inode = {
	.data = 0,
	.size = 0,
	.capacity = 0,
	.refs = 1,
	.type = BUNIX_VFS_TYPE_DIRECTORY,
	.mode = 0777,
	.uid = 0,
	.gid = 0,
};

static struct tmpfs_file *parent_file_for_path(const char *path)
{
	char parent[TMPFS_MAX_PATH];

	if (path_parent_path(path, parent) != 0) {
		return 0;
	}
	if (path_is_root(parent)) {
		static struct tmpfs_file root = {
			.node = { 0, 0, 0, 0, 0 },
			.path = 0,
			.refs = 0,
			.deleted = 0,
			.inode = &root_inode,
		};

		return &root;
	}
	return file_find(parent);
}

static int directory_is_empty(const char *path)
{
	for (struct bunix_tree_node *node = bunix_tree_first_node(&files);
	     node != 0; node = bunix_tree_next_node(node)) {
		struct tmpfs_file *file = (struct tmpfs_file *)node->value;

		if (file_is_child_of(file, path)) {
			return 0;
		}
	}
	return 1;
}

static int file_in_renamed_subtree(const struct tmpfs_file *file,
				   const char *old_path)
{
	return file != 0 && !file->deleted &&
	       (str_eq(file->path, old_path) ||
		path_has_prefix_child(file->path, old_path));
}

static int tmpfs_file_ptr_in_set(struct tmpfs_file **fileset, u64 count,
				 const struct tmpfs_file *file)
{
	for (u64 i = 0; i < count; i++) {
		if (fileset[i] == file) {
			return 1;
		}
	}
	return 0;
}

static int tmpfs_renamed_subtree_path(const char *old_path,
				      const char *new_path,
				      const char *path,
				      char *out)
{
	const u64 new_len = str_len(new_path);
	const u64 old_len = str_len(old_path);
	const char *suffix = path + old_len;
	u64 pos = 0;

	if (new_len >= TMPFS_MAX_PATH || old_len >= TMPFS_MAX_PATH ||
	    (suffix[0] != '\0' && suffix[0] != '/')) {
		return -1;
	}
	for (u64 i = 0; i < new_len; i++) {
		out[pos++] = new_path[i];
	}
	for (u64 i = 0; suffix[i] != '\0'; i++) {
		if (pos + 1 >= TMPFS_MAX_PATH) {
			return -1;
		}
		out[pos++] = suffix[i];
	}
	out[pos] = '\0';
	return 0;
}

static void inode_release(struct tmpfs_inode *inode)
{
	if (inode == 0) {
		return;
	}
	if (inode->refs != 0) {
		inode->refs--;
	}
	if (inode->refs == 0) {
		if (inode->data != 0) {
			bunix_free(inode->data);
		}
		bunix_free(inode);
	}
}

static void file_release(struct tmpfs_file *file)
{
	if (file == 0) {
		return;
	}
	if (file->refs != 0) {
		file->refs--;
	}
	if (file->refs == 0 && file->deleted) {
		inode_release(file->inode);
		bunix_free(file->path);
		bunix_free(file);
	}
}

static void files_rebuild_without_node(struct bunix_tree_node *node,
				       const struct tmpfs_file *skip,
				       struct bunix_tree *rebuilt)
{
	struct bunix_tree_node *left;
	struct bunix_tree_node *right;
	struct tmpfs_file *file;

	if (node == 0) {
		return;
	}
	left = node->left;
	right = node->right;
	file = (struct tmpfs_file *)node->value;
	node->left = 0;
	node->right = 0;
	node->parent = 0;
	node->key = 0;
	node->value = 0;
	files_rebuild_without_node(left, skip, rebuilt);
	files_rebuild_without_node(right, skip, rebuilt);
	if (file != 0 && file != skip && !file->deleted) {
		(void)bunix_tree_insert_node(rebuilt, &file->node,
					     file->path, (u64)file);
	}
}

static void files_remove_file(struct tmpfs_file *file)
{
	struct bunix_tree rebuilt;

	bunix_tree_init(&rebuilt);
	files_rebuild_without_node(files.root, file, &rebuilt);
	files = rebuilt;
}

static int file_resize(struct tmpfs_file *file, u64 size)
{
	u64 capacity;
	char *data;

	if (file == 0) {
		return -1;
	}
	if (size <= file->inode->capacity) {
		return 0;
	}
	capacity = file->inode->capacity == 0 ? 64 : file->inode->capacity;
	while (capacity < size) {
		capacity *= 2;
	}
	data = (char *)bunix_realloc(file->inode->data, file->inode->capacity, capacity);
	if (data == 0) {
		return -1;
	}
	for (u64 i = file->inode->capacity; i < capacity; i++) {
		data[i] = 0;
	}
	file->inode->data = data;
	file->inode->capacity = capacity;
	return 0;
}

static int file_set_size(struct tmpfs_file *file, u64 size)
{
	const u64 old_size = file != 0 ? file->inode->size : 0;

	if (file == 0 || file_resize(file, size) != 0) {
		return -1;
	}
	if (size > old_size) {
		for (u64 i = old_size; i < size; i++) {
			file->inode->data[i] = 0;
		}
	}
	file->inode->size = size;
	return 0;
}

static struct tmpfs_file *file_create(const char *path, u64 mode, u64 type,
				      u64 task)
{
	struct tmpfs_file *file;
	u64 uid = 0;
	u64 gid = 0;

	if (!path_parent_exists(path) || file_find(path) != 0 ||
	    (type != BUNIX_VFS_TYPE_REGULAR &&
	     type != BUNIX_VFS_TYPE_DIRECTORY &&
	     type != BUNIX_VFS_TYPE_SYMLINK &&
	     type != BUNIX_VFS_TYPE_FIFO)) {
		return 0;
	}
	file = (struct tmpfs_file *)bunix_calloc(1, sizeof(*file));
	if (file == 0) {
		return 0;
	}
	file->path = str_dup(path);
	if (file->path == 0) {
		bunix_free(file);
		return 0;
	}
	file->inode = (struct tmpfs_inode *)bunix_calloc(1, sizeof(*file->inode));
	if (file->inode == 0) {
		bunix_free(file->path);
		bunix_free(file);
		return 0;
	}
	file->inode->refs = 1;
	if (task != 0) {
		if (user_credential(task, BUNIX_USER_GETEUID, &uid) != 0 ||
		    user_credential(task, BUNIX_USER_GETEGID, &gid) != 0) {
			inode_release(file->inode);
			bunix_free(file->path);
			bunix_free(file);
			return 0;
		}
	}
	file->inode->type = (unsigned int)type;
	file->inode->mode = (unsigned int)(mode & 0777);
	file->inode->uid = (unsigned int)uid;
	file->inode->gid = (unsigned int)gid;
	if (bunix_tree_insert_node(&files, &file->node, file->path,
				   (u64)file) != 0) {
		inode_release(file->inode);
		bunix_free(file->path);
		bunix_free(file);
		return 0;
	}
	return file;
}

static u64 open_dir(const char *path)
{
	struct tmpfs_open *open;
	const u64 start = next_open_id;

	open = (struct tmpfs_open *)bunix_calloc(1, sizeof(*open));
	if (open == 0) {
		return 0;
	}
	open->kind = TMPFS_OPEN_DIR;
	for (u64 i = 0; i <= str_len(path); i++) {
		open->path[i] = path[i];
	}
	for (;;) {
		const u64 id = next_open_id++;
		int wrapped;

		if (next_open_id == 0) {
			next_open_id = 1;
		}
		wrapped = next_open_id == start;
		if (bunix_u64_tree_get(&open_files, id) != 0) {
			if (wrapped) {
				break;
			}
			continue;
		}
		open->id = id;
		if (bunix_u64_tree_insert_node(&open_files, &open->node,
					       id, (u64)open) == 0) {
			return id;
		}
		break;
	}
	if (open->id == 0) {
		bunix_free(open);
	}
	return 0;
}

static u64 open_file(struct tmpfs_file *file)
{
	struct tmpfs_open *open;
	const u64 start = next_open_id;

	if (file == 0 || file->deleted) {
		return 0;
	}
	open = (struct tmpfs_open *)bunix_calloc(1, sizeof(*open));
	if (open == 0) {
		return 0;
	}
	open->kind = TMPFS_OPEN_FILE;
	open->file = file;
	file->refs++;
	for (;;) {
		const u64 id = next_open_id++;
		int wrapped;

		if (next_open_id == 0) {
			next_open_id = 1;
		}
		wrapped = next_open_id == start;
		if (bunix_u64_tree_get(&open_files, id) != 0) {
			if (wrapped) {
				break;
			}
			continue;
		}
		open->id = id;
		if (bunix_u64_tree_insert_node(&open_files, &open->node,
					       id, (u64)open) == 0) {
			return id;
		}
		break;
	}
	if (open->id == 0) {
		file_release(file);
		bunix_free(open);
	}
	return 0;
}

static struct tmpfs_open *open_from_handle(u64 handle)
{
	return (struct tmpfs_open *)bunix_u64_tree_get(&open_files, handle);
}

static void close_handle(u64 handle)
{
	struct tmpfs_open *open = open_from_handle(handle);

	if (open != 0) {
		if (open->kind == TMPFS_OPEN_FILE) {
			file_release(open->file);
		}
		bunix_u64_tree_remove_node(&open_files, &open->node);
		bunix_free(open);
	}
}

static void file_unlink(struct tmpfs_file *file)
{
	if (file == 0) {
		return;
	}
	file->deleted = 1;
	files_remove_file(file);
	file_release(file);
}

static int file_move_to_path(struct tmpfs_file *file, const char *new_path)
{
	char *old_path;
	char *path_copy;

	if (file == 0 || new_path == 0 || file_find(new_path) != 0) {
		return -1;
	}
	path_copy = str_dup(new_path);
	if (path_copy == 0) {
		return -1;
	}
	old_path = file->path;
	files_remove_file(file);
	file->path = path_copy;
	if (bunix_tree_insert_node(&files, &file->node, file->path,
				   (u64)file) == 0) {
		bunix_free(old_path);
		return 0;
	}
	file->path = old_path;
	if (bunix_tree_insert_node(&files, &file->node, file->path,
				   (u64)file) != 0) {
		file->deleted = 1;
	}
	bunix_free(path_copy);
	return -1;
}

static int file_move_subtree_to_path(struct tmpfs_file *file,
				     const char *new_path)
{
	struct tmpfs_file **moved;
	char **new_paths;
	u64 count = 0;
	u64 index = 0;
	int result = -1;

	if (file == 0 || new_path == 0 || file->deleted ||
	    file->inode->type != BUNIX_VFS_TYPE_DIRECTORY ||
	    !path_parent_exists(new_path) ||
	    path_has_prefix_child(new_path, file->path) ||
	    str_len(new_path) >= TMPFS_MAX_PATH) {
		return -1;
	}
	for (struct bunix_tree_node *node = bunix_tree_first_node(&files);
	     node != 0; node = bunix_tree_next_node(node)) {
		struct tmpfs_file *candidate = (struct tmpfs_file *)node->value;

		if (file_in_renamed_subtree(candidate, file->path)) {
			count++;
		}
	}
	if (count == 0) {
		return -1;
	}
	moved = (struct tmpfs_file **)bunix_calloc(count, sizeof(moved[0]));
	new_paths = (char **)bunix_calloc(count, sizeof(new_paths[0]));
	if (moved == 0 || new_paths == 0) {
		bunix_free(moved);
		bunix_free(new_paths);
		return -1;
	}
	for (struct bunix_tree_node *node = bunix_tree_first_node(&files);
	     node != 0; node = bunix_tree_next_node(node)) {
		struct tmpfs_file *candidate = (struct tmpfs_file *)node->value;

		if (file_in_renamed_subtree(candidate, file->path)) {
			moved[index++] = candidate;
		}
	}
	for (u64 i = 0; i < count; i++) {
		char path[TMPFS_MAX_PATH];
		struct tmpfs_file *existing;

		if (tmpfs_renamed_subtree_path(file->path, new_path,
					       moved[i]->path, path) != 0) {
			goto out;
		}
		existing = file_find(path);
		if (existing != 0 &&
		    !tmpfs_file_ptr_in_set(moved, count, existing)) {
			goto out;
		}
		new_paths[i] = str_dup(path);
		if (new_paths[i] == 0) {
			goto out;
		}
	}
	for (u64 i = 0; i < count; i++) {
		files_remove_file(moved[i]);
	}
	for (u64 i = 0; i < count; i++) {
		char *old_path = moved[i]->path;

		moved[i]->path = new_paths[i];
		new_paths[i] = 0;
		if (bunix_tree_insert_node(&files, &moved[i]->node,
					   moved[i]->path, (u64)moved[i]) != 0) {
			moved[i]->deleted = 1;
			goto out;
		}
		bunix_free(old_path);
	}
	result = 0;

out:
	for (u64 i = 0; i < count; i++) {
		if (new_paths[i] != 0) {
			bunix_free(new_paths[i]);
		}
	}
	bunix_free(new_paths);
	bunix_free(moved);
	return result;
}

static int file_link_to_path(struct tmpfs_file *file, const char *new_path)
{
	struct tmpfs_file *link;

	if (file == 0 || file->deleted || file->inode == 0 ||
	    new_path == 0 || file_find(new_path) != 0 ||
	    !path_parent_exists(new_path) ||
	    file->inode->type == BUNIX_VFS_TYPE_DIRECTORY) {
		return -1;
	}
	link = (struct tmpfs_file *)bunix_calloc(1, sizeof(*link));
	if (link == 0) {
		return -1;
	}
	link->path = str_dup(new_path);
	if (link->path == 0) {
		bunix_free(link);
		return -1;
	}
	link->inode = file->inode;
	link->inode->refs++;
	if (bunix_tree_insert_node(&files, &link->node, link->path,
				   (u64)link) != 0) {
		inode_release(link->inode);
		bunix_free(link->path);
		bunix_free(link);
		return -1;
	}
	return 0;
}

static void stat_file(struct bunix_msg *reply, const struct tmpfs_file *file)
{
	reply->words[0] = 0;
	reply->words[1] = file->inode->size;
	reply->words[2] = file->inode->mode | ((u64)file->inode->type << 32);
	reply->words[3] = file->inode->uid | ((u64)file->inode->gid << 32);
}

static void stat_dir(struct bunix_msg *reply)
{
	reply->words[0] = 0;
	reply->words[1] = 0;
	reply->words[2] = 0777 | ((u64)BUNIX_VFS_TYPE_DIRECTORY << 32);
	reply->words[3] = 0;
}

static void reply_readlink(const struct bunix_msg *message,
			   struct bunix_msg *reply,
			   const struct tmpfs_file *file)
{
	u64 written;
	const u64 target_len = file != 0 ? file->inode->size : 0;
	const u64 out_cap = message->words[3] >> 32;

	if (file == 0 || file->inode->type != BUNIX_VFS_TYPE_SYMLINK ||
	    file->inode->data == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	reply->words[0] = 0;
	reply->words[1] = target_len;
	if (out_cap == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	if (message->cap == 0 ||
	    (message->cap_rights & BUNIX_RIGHT_SEND) == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	written = target_len > out_cap ? out_cap : target_len;
	if (written != 0 &&
	    bunix_buffer_write(message->cap, message->words[0] +
			       message->words[1], file->inode->data,
			       written) != 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	reply->words[2] = target_len;
	reply->words[3] = written;
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

	if (bunix_ipc_call(TMPFS_HANDLE_NAMES, &request, &reply) != 0 ||
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
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP,
		.reply = 0,
		.cap = handle,
		.words = { BUNIX_NAMES_ROOT, service, 0, 0 },
	};
	struct bunix_msg reply;

	if (bunix_ipc_call(TMPFS_HANDLE_NAMES, &request, &reply) != 0) {
		return -1;
	}
	return reply.words[0] == 0 ? 0 : -1;
}

static long mount_path(u64 vfs, const char *path)
{
	struct bunix_msg reply;

	return bunix_ipc_call_path(vfs, BUNIX_PROTO_VFS,
				   BUNIX_VFS_MOUNT_BUFFER, path,
				   BUNIX_SERVICE_TMPFS, 0, 0, &reply) == 0 &&
	       reply.words[0] == 0 ? 0 : -1;
}

static long mount_root(u64 vfs, const char *path)
{
	struct tmpfs_root *root;

	if (path == 0 || path[0] != '/' || bunix_tree_get(&roots, path) != 0) {
		return -1;
	}
	root = (struct tmpfs_root *)bunix_calloc(1, sizeof(*root));
	if (root == 0) {
		return -1;
	}
	root->path = str_dup(path);
	if (root->path == 0) {
		bunix_free(root);
		return -1;
	}
	if (mount_path(vfs, root->path) != 0 ||
	    bunix_tree_insert_node(&roots, &root->node, root->path,
				   (u64)root) != 0) {
		bunix_free(root->path);
		bunix_free(root);
		return -1;
	}
	return 0;
}

int main(void)
{
	const char online[] = "tmpfs: online\n";
	const char mounted[] = "tmpfs: mounted\n";
	struct bunix_msg message;

	bunix_console_log(online, sizeof(online) - 1);
	bunix_tree_init(&roots);
	bunix_tree_init(&files);
	bunix_u64_tree_init(&open_files);
	next_open_id = 1;
	vfs_service = resolve_service(BUNIX_SERVICE_VFS, BUNIX_RIGHT_SEND);
	if (vfs_service == 0 ||
	    register_service(BUNIX_SERVICE_TMPFS, BUNIX_HANDLE_SELF) != 0) {
		return 1;
	}

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
		char path[TMPFS_MAX_PATH];
		struct tmpfs_file *file;
		struct tmpfs_open *open;

		if (bunix_ipc_recv(BUNIX_HANDLE_SELF, &message) != 0) {
			continue;
		}

		if (message.protocol == BUNIX_PROTO_TMPFS) {
			reply.protocol = BUNIX_PROTO_TMPFS;
			reply.type = message.type;
				switch (message.type) {
				case BUNIX_TMPFS_MOUNT_ROOT:
					if (bunix_read_path_cap(&message, path,
								sizeof(path)) != 0) {
						reply.words[0] = (u64)-1;
						break;
					}
					reply.words[0] =
						(u64)mount_root(vfs_service, path);
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
				break;
			}
			if (path_is_root(path)) {
				reply.words[1] = open_dir(path);
				reply.words[2] = 0;
				reply.words[3] = BUNIX_VFS_TYPE_DIRECTORY;
				reply.words[0] = reply.words[1] == 0 ?
						 (u64)-1 : 0;
				break;
			}
			file = file_find(path);
			if (file == 0) {
				reply.words[0] = BUNIX_VFS_ERR_NOENT;
				break;
			}
			reply.words[1] = file->inode->type == BUNIX_VFS_TYPE_DIRECTORY ?
					 open_dir(path) : open_file(file);
			reply.words[2] = file->inode->size;
			reply.words[3] = file->inode->type;
			reply.words[0] = reply.words[1] == 0 ? (u64)-1 : 0;
			break;
		case BUNIX_VFS_CREATE_BUFFER:
			if (read_resolved_path(&message, path) != 0) {
				reply.words[0] = BUNIX_VFS_ERR_NOENT;
				break;
			}
			file = parent_file_for_path(path);
			if (!task_can_access(message.words[3] & 0xffffffff,
					     file, 03)) {
				reply.words[0] = BUNIX_VFS_ERR_ACCESS;
				break;
			}
			file = file_find(path);
			if (file == 0) {
				file = file_create(path, message.words[3] >> 32,
						   BUNIX_VFS_TYPE_REGULAR,
						   message.words[3] &
						   0xffffffff);
			}
			reply.words[0] = file == 0 ? (u64)-1 : 0;
			break;
		case BUNIX_VFS_MKDIR_BUFFER:
			if (read_resolved_path(&message, path) != 0) {
				reply.words[0] = BUNIX_VFS_ERR_NOENT;
				break;
			}
			file = parent_file_for_path(path);
			if (!task_can_access(message.words[3] & 0xffffffff,
					     file, 03)) {
				reply.words[0] = BUNIX_VFS_ERR_ACCESS;
				break;
			}
			file = file_create(path, message.words[3] >> 32,
					   BUNIX_VFS_TYPE_DIRECTORY,
					   message.words[3] & 0xffffffff);
			reply.words[0] = file == 0 ? (u64)-1 : 0;
			break;
		case BUNIX_VFS_MKNOD_BUFFER: {
			const u64 type = message.words[3] >> 48;
			const u64 mode = (message.words[3] >> 32) & 0777;

			if (read_resolved_path(&message, path) != 0) {
				reply.words[0] = BUNIX_VFS_ERR_NOENT;
				break;
			}
			file = parent_file_for_path(path);
			if (!task_can_access(message.words[3] & 0xffffffff,
					     file, 03)) {
				reply.words[0] = BUNIX_VFS_ERR_ACCESS;
				break;
			}
			if (file_find(path) != 0) {
				reply.words[0] = BUNIX_VFS_ERR_EXIST;
				break;
			}
			file = file_create(path, mode, type,
					   message.words[3] & 0xffffffff);
			reply.words[0] = file == 0 ? (u64)-1 : 0;
			break;
		}
		case BUNIX_VFS_SYMLINK_BUFFER: {
			char target[TMPFS_MAX_PATH];
			const u64 task = message.words[3] & 0xffffffff;
			const u64 target_len = message.words[0];

			if (read_symlink_buffer(&message, target, path) != 0) {
				reply.words[0] = BUNIX_VFS_ERR_NOENT;
				break;
			}
			file = parent_file_for_path(path);
			if (!task_can_access(task, file, 03)) {
				reply.words[0] = BUNIX_VFS_ERR_ACCESS;
				break;
			}
			file = file_create(path, 0777, BUNIX_VFS_TYPE_SYMLINK,
					   task);
			if (file == 0 ||
			    file_resize(file, target_len) != 0) {
				if (file != 0) {
					file_unlink(file);
				}
				reply.words[0] = (u64)-1;
				break;
			}
			for (u64 i = 0; i < target_len; i++) {
				file->inode->data[i] = target[i];
			}
			file->inode->size = target_len - 1;
			reply.words[0] = 0;
			break;
		}
		case BUNIX_VFS_STAT_PATH_META_BUFFER:
		case BUNIX_VFS_ACCESS_BUFFER:
		case BUNIX_VFS_READLINK_BUFFER:
			if (read_resolved_path(&message, path) != 0) {
				reply.words[0] = BUNIX_VFS_ERR_NOENT;
				break;
			}
			if (path_is_root(path)) {
				stat_dir(&reply);
				break;
			}
			file = file_find(path);
			if (file == 0) {
				reply.words[0] = BUNIX_VFS_ERR_NOENT;
			} else if (message.type == BUNIX_VFS_READLINK_BUFFER) {
				reply_readlink(&message, &reply, file);
			} else if (message.type == BUNIX_VFS_ACCESS_BUFFER) {
				reply.words[0] =
					task_can_access(message.words[3] &
							0xffffffff,
							file,
							message.words[3] >> 32) ?
					0 : BUNIX_VFS_ERR_ACCESS;
			} else {
				stat_file(&reply, file);
			}
			break;
		case BUNIX_VFS_STAT_META:
			open = open_from_handle(message.words[0]);
			if (open == 0) {
				reply.words[0] = (u64)-1;
			} else if (open->kind == TMPFS_OPEN_DIR) {
				stat_dir(&reply);
			} else {
				stat_file(&reply, open->file);
			}
			break;
		case BUNIX_VFS_READ_FILE_BUFFER:
			open = open_from_handle(message.words[0]);
			if (open == 0 || open->kind != TMPFS_OPEN_FILE ||
			    message.cap == 0 ||
			    (message.cap_rights & BUNIX_RIGHT_SEND) == 0) {
				reply.words[0] = (u64)-1;
				break;
			}
			if (message.words[1] >= open->file->inode->size) {
				reply.words[0] = 0;
				reply.words[1] = 0;
				break;
			}
			u64 nread = open->file->inode->size - message.words[1];
			if (nread > message.words[2]) {
				nread = message.words[2];
			}
			if (bunix_buffer_write(message.cap, 0,
					       open->file->inode->data + message.words[1],
					       nread) != 0) {
				reply.words[0] = (u64)-1;
			} else {
				reply.words[0] = 0;
				reply.words[1] = nread;
			}
			break;
		case BUNIX_VFS_WRITE_FILE_BUFFER:
			open = open_from_handle(message.words[0]);
			if (open == 0 || open->kind != TMPFS_OPEN_FILE ||
			    !task_can_access(message.words[3], open->file, 02) ||
			    message.cap == 0 ||
			    (message.cap_rights & BUNIX_RIGHT_RECV) == 0 ||
			    file_resize(open->file, message.words[1] +
					message.words[2]) != 0 ||
			    bunix_buffer_read(message.cap, 0,
					      open->file->inode->data + message.words[1],
					      message.words[2]) != 0) {
				reply.words[0] = (u64)-1;
				break;
			}
			if (message.words[1] + message.words[2] > open->file->inode->size) {
				open->file->inode->size = message.words[1] + message.words[2];
			}
			reply.words[0] = 0;
			reply.words[1] = message.words[2];
			break;
		case BUNIX_VFS_TRUNCATE:
			if (message.cap != 0) {
				if (read_resolved_path(&message, path) != 0) {
					reply.words[0] = BUNIX_VFS_ERR_NOENT;
					break;
				}
				file = file_find(path);
				if (file == 0 ||
				    file->inode->type != BUNIX_VFS_TYPE_REGULAR) {
					reply.words[0] = BUNIX_VFS_ERR_NOENT;
				} else if (!task_can_access(0, file, 02)) {
					reply.words[0] = BUNIX_VFS_ERR_ACCESS;
				} else {
					reply.words[0] = file_set_size(file,
								       message.words[3]) == 0 ?
							 0 : (u64)-1;
				}
				break;
			}
			open = open_from_handle(message.words[0]);
			if (open == 0 || open->kind != TMPFS_OPEN_FILE) {
				reply.words[0] = (u64)-1;
			} else if (!task_can_access(message.words[3],
						    open->file, 02)) {
				reply.words[0] = BUNIX_VFS_ERR_ACCESS;
			} else {
				reply.words[0] = file_set_size(open->file,
							       message.words[1]) == 0 ?
						 0 : (u64)-1;
			}
			break;
		case BUNIX_VFS_CHMOD_BUFFER:
			if (read_resolved_path(&message, path) != 0) {
				reply.words[0] = BUNIX_VFS_ERR_NOENT;
				break;
			}
			file = file_find(path);
			if (file == 0 || !task_can_chmod(message.words[3] &
							 0xffffffff, file)) {
				reply.words[0] = BUNIX_VFS_ERR_ACCESS;
				break;
			}
			file->inode->mode = (unsigned int)((message.words[3] >> 32) &
						    0777);
			reply.words[0] = 0;
			break;
		case BUNIX_VFS_CHMOD:
			open = open_from_handle(message.words[0]);
			if (open == 0 ||
			    (open->kind != TMPFS_OPEN_FILE &&
			     open->kind != TMPFS_OPEN_DIR)) {
				reply.words[0] = (u64)-1;
				break;
			}
			file = open->kind == TMPFS_OPEN_FILE ?
			       open->file : file_find(open->path);
			if (file == 0 ||
			    !task_can_chmod(message.words[3] & 0xffffffff,
					    file)) {
				reply.words[0] = BUNIX_VFS_ERR_ACCESS;
				break;
			}
			file->inode->mode = (unsigned int)(message.words[1] & 0777);
			reply.words[0] = 0;
			break;
		case BUNIX_VFS_CHOWN_BUFFER:
			if (read_resolved_path(&message, path) != 0) {
				reply.words[0] = BUNIX_VFS_ERR_NOENT;
				break;
			}
			file = file_find(path);
			if (file == 0) {
				reply.words[0] = BUNIX_VFS_ERR_NOENT;
				break;
			}
			if (!task_can_chown(message.words[3])) {
				reply.words[0] = BUNIX_VFS_ERR_ACCESS;
				break;
			}
			if (message.words[2] != (u64)-1) {
				file->inode->uid = (unsigned int)message.words[2];
			}
			if ((message.words[3] >> 32) != 0xffffffff) {
				file->inode->gid = (unsigned int)(message.words[3] >> 32);
			}
			reply.words[0] = 0;
			break;
		case BUNIX_VFS_CHOWN:
			open = open_from_handle(message.words[0]);
			if (open == 0 ||
			    (open->kind != TMPFS_OPEN_FILE &&
			     open->kind != TMPFS_OPEN_DIR)) {
				reply.words[0] = (u64)-1;
				break;
			}
			file = open->kind == TMPFS_OPEN_FILE ?
			       open->file : file_find(open->path);
			if (file == 0 || !task_can_chown(message.words[3])) {
				reply.words[0] = BUNIX_VFS_ERR_ACCESS;
				break;
			}
			if (message.words[1] != (u64)-1) {
				file->inode->uid = (unsigned int)message.words[1];
			}
			if (message.words[2] != (u64)-1) {
				file->inode->gid = (unsigned int)message.words[2];
			}
			reply.words[0] = 0;
			break;
		case BUNIX_VFS_UNLINK_BUFFER:
			if (read_resolved_path(&message, path) != 0) {
				reply.words[0] = BUNIX_VFS_ERR_NOENT;
				break;
			}
			if (!task_can_access(message.words[3] & 0xffffffff,
					     parent_file_for_path(path), 03)) {
				reply.words[0] = BUNIX_VFS_ERR_ACCESS;
				break;
			}
			file = file_find(path);
			if (file == 0) {
				reply.words[0] = BUNIX_VFS_ERR_NOENT;
				break;
			}
			if (file->inode->type == BUNIX_VFS_TYPE_DIRECTORY) {
				reply.words[0] = BUNIX_VFS_ERR_ISDIR;
				break;
			}
			file_unlink(file);
			reply.words[0] = 0;
			break;
		case BUNIX_VFS_RMDIR_BUFFER:
			if (read_resolved_path(&message, path) != 0) {
				reply.words[0] = BUNIX_VFS_ERR_NOENT;
				break;
			}
			if (!task_can_access(message.words[3] & 0xffffffff,
					     parent_file_for_path(path), 03)) {
				reply.words[0] = BUNIX_VFS_ERR_ACCESS;
				break;
			}
			file = file_find(path);
			if (file == 0) {
				reply.words[0] = BUNIX_VFS_ERR_NOENT;
			} else if (file->inode->type != BUNIX_VFS_TYPE_DIRECTORY) {
				reply.words[0] = BUNIX_VFS_ERR_NOTDIR;
			} else if (!directory_is_empty(path)) {
				reply.words[0] = (u64)-1;
			} else {
				file_unlink(file);
				reply.words[0] = 0;
			}
			break;
			case BUNIX_VFS_RENAME_BUFFER: {
				char new_path[TMPFS_MAX_PATH];
				struct tmpfs_file *target;
				struct tmpfs_file *old_parent;
				struct tmpfs_file *new_parent;
				const u64 task = message.words[3] & 0xffffffff;
				const u64 flags = message.words[3] >> 32;

				if (read_rename_buffer(&message, path, new_path) != 0 ||
				    path_is_root(path) || path_is_root(new_path)) {
					reply.words[0] = BUNIX_VFS_ERR_NOENT;
					break;
				}
				file = file_find(path);
				if (file == 0) {
					reply.words[0] = BUNIX_VFS_ERR_NOENT;
					break;
				}
				if (str_eq(path, new_path)) {
					reply.words[0] = 0;
					break;
				}
				old_parent = parent_file_for_path(path);
				new_parent = parent_file_for_path(new_path);
				if (!task_can_access(task, old_parent, 03) ||
				    !task_can_access(task, new_parent, 03)) {
					reply.words[0] = BUNIX_VFS_ERR_ACCESS;
					break;
				}
				if (file->inode->type == BUNIX_VFS_TYPE_DIRECTORY &&
				    path_has_prefix_child(new_path, path)) {
					reply.words[0] = BUNIX_VFS_ERR_INVAL;
					break;
				}
				target = file_find(new_path);
				if (target != 0 && (flags & 1) != 0) {
					reply.words[0] = BUNIX_VFS_ERR_EXIST;
					break;
				}
				if (target != 0) {
					if (target->inode->type == BUNIX_VFS_TYPE_DIRECTORY &&
					    file->inode->type != BUNIX_VFS_TYPE_DIRECTORY) {
						reply.words[0] = BUNIX_VFS_ERR_ISDIR;
						break;
					}
					if (target->inode->type != BUNIX_VFS_TYPE_DIRECTORY &&
					    file->inode->type == BUNIX_VFS_TYPE_DIRECTORY) {
						reply.words[0] = BUNIX_VFS_ERR_NOTDIR;
						break;
					}
					if (target->inode->type == BUNIX_VFS_TYPE_DIRECTORY &&
					    !directory_is_empty(new_path)) {
						reply.words[0] = BUNIX_VFS_ERR_NOTEMPTY;
						break;
					}
					file_unlink(target);
				}
				if (file->inode->type == BUNIX_VFS_TYPE_DIRECTORY) {
					reply.words[0] =
						file_move_subtree_to_path(file,
									  new_path) == 0 ?
						0 : BUNIX_VFS_ERR_NOENT;
				} else {
					reply.words[0] =
						file_move_to_path(file, new_path) == 0 ?
						0 : BUNIX_VFS_ERR_NOENT;
				}
				break;
			}
		case BUNIX_VFS_LINK_BUFFER: {
			char new_path[TMPFS_MAX_PATH];
			struct tmpfs_file *old_parent;
			struct tmpfs_file *new_parent;
			const u64 task = message.words[3] & 0xffffffff;

			if (read_rename_buffer(&message, path, new_path) != 0 ||
			    path_is_root(path) || path_is_root(new_path)) {
				reply.words[0] = BUNIX_VFS_ERR_NOENT;
				break;
			}
			file = file_find(path);
			if (file == 0) {
				reply.words[0] = BUNIX_VFS_ERR_NOENT;
				break;
			}
			if (file_find(new_path) != 0) {
				reply.words[0] = BUNIX_VFS_ERR_EXIST;
				break;
			}
			if (file->inode->type == BUNIX_VFS_TYPE_DIRECTORY) {
				reply.words[0] = BUNIX_VFS_ERR_ACCESS;
				break;
			}
			old_parent = parent_file_for_path(path);
			new_parent = parent_file_for_path(new_path);
			if (!task_can_access(task, old_parent, 01) ||
			    !task_can_access(task, new_parent, 03)) {
				reply.words[0] = BUNIX_VFS_ERR_ACCESS;
				break;
			}
			reply.words[0] = file_link_to_path(file, new_path) == 0 ?
					 0 : (u64)-1;
			break;
		}
		case BUNIX_VFS_READDIR_BUFFER:
			open = open_from_handle(message.words[0]);
			if (open == 0 || open->kind != TMPFS_OPEN_DIR) {
				reply.words[0] = BUNIX_VFS_ERR_NOTDIR;
				break;
			}
			if (message.cap == 0 ||
			    (message.cap_rights & BUNIX_RIGHT_SEND) == 0) {
				reply.words[0] = (u64)-1;
				break;
			}
			reply.words[0] = BUNIX_VFS_ERR_NOENT;
			u64 current = 0;
			for (struct bunix_tree_node *node =
				     bunix_tree_first_node(&files);
			     node != 0; node = bunix_tree_next_node(node)) {
				file = (struct tmpfs_file *)node->value;
				if (!file_is_child_of(file, open->path)) {
					continue;
				}
				if (current == message.words[1]) {
					const char *name =
						path_name_in_dir(file->path,
								 open->path);
					const u64 name_len = str_len(name);
					reply.words[0] = 0;
					reply.words[1] = (message.words[1] + 1) |
							 ((u64)file->inode->type << 32);
					u64 written = name_len;

					if (written > message.words[3]) {
						written = message.words[3];
					}
					if (written != 0 &&
					    bunix_buffer_write(message.cap,
							       message.words[2],
							       name,
							       written) != 0) {
						reply.words[0] = (u64)-1;
					} else {
						reply.words[2] = name_len;
						reply.words[3] = written;
					}
					break;
				}
				current++;
			}
			break;
		case BUNIX_VFS_CLOSE:
			if (open_from_handle(message.words[0]) == 0) {
				reply.words[0] = (u64)-1;
			} else {
				close_handle(message.words[0]);
				reply.words[0] = 0;
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
