#include <bunix/alloc.h>
#include <bunix/libbunix.h>
#include <bunix/tree.h>

#define TMPFS_HANDLE_NAMES (bunix_handle_find(BUNIX_CAP_NAME))

enum {
	TMPFS_MAX_PATH = 4096,
	TMPFS_TREE_WALK_LIMIT = 8192,
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
	u64 ino;
	unsigned int type;
	unsigned int mode;
	unsigned int uid;
	unsigned int gid;
	u64 atime;
	u64 mtime;
	u64 ctime;
};

struct tmpfs_file {
	struct bunix_tree_node node;
	struct tmpfs_file *prev;
	struct tmpfs_file *next;
	char *path;
	u64 refs;
	int deleted;
	struct tmpfs_inode *inode;
};

struct tmpfs_open {
	struct bunix_u64_tree_node node;
	u64 id;
	u64 owner;
	u64 kind;
	struct tmpfs_file *file;
	char path[TMPFS_MAX_PATH];
};

struct tmpfs_vfs_caller {
	struct bunix_u64_tree_node node;
	u64 task;
};

static struct bunix_tree roots;
static struct tmpfs_file *files_head;
static struct bunix_u64_tree open_files;
static struct bunix_u64_tree vfs_callers;
static u64 next_open_id = 1;
static u64 next_inode_id = 2;
static u64 user_service;
static u64 vfs_service;
static u64 vfs_caller_grantor;

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
		if (path[i] == '\0' || path[i] != prefix[i]) {
			return 0;
		}
		i++;
	}
	return path[i] == '/';
}

static int path_is_at_or_under(const char *path, const char *prefix)
{
	return str_eq(path, prefix) || path_has_prefix_child(path, prefix);
}

static int path_is_root(const char *path)
{
	return str_eq(path, "/") || bunix_tree_get(&roots, path) != 0;
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

	if (file == 0 || file->deleted || file->path == 0 ||
	    !path_has_prefix_child(file->path, dir)) {
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

static void file_list_add(struct tmpfs_file *file)
{
	if (file == 0 || file->prev != 0 || file->next != 0 ||
	    files_head == file) {
		return;
	}
	file->prev = 0;
	file->next = files_head;
	if (files_head != 0) {
		files_head->prev = file;
	}
	files_head = file;
}

static void file_list_remove(struct tmpfs_file *file)
{
	if (file == 0) {
		return;
	}
	if (file->prev != 0) {
		file->prev->next = file->next;
	} else if (files_head == file) {
		files_head = file->next;
	}
	if (file->next != 0) {
		file->next->prev = file->prev;
	}
	file->prev = 0;
	file->next = 0;
}

static struct tmpfs_file *file_find(const char *path)
{
	u64 steps = 0;

	if (path == 0) {
		return 0;
	}
	for (struct tmpfs_file *file = files_head;
	     file != 0 && steps++ < TMPFS_TREE_WALK_LIMIT; file = file->next) {
		if (!file->deleted && file->path != 0 &&
		    str_eq(file->path, path)) {
			return file;
		}
	}
	return 0;
}

static int files_insert_file(struct tmpfs_file *file)
{
	if (file == 0 || file->path == 0 || file_find(file->path) != 0) {
		return -1;
	}
	file->node.left = 0;
	file->node.right = 0;
	file->node.parent = 0;
	file->node.key = 0;
	file->node.value = 0;
	file_list_add(file);
	return 0;
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
	.ino = 1,
	.type = BUNIX_VFS_TYPE_DIRECTORY,
	.mode = 0777,
	.uid = 0,
	.gid = 0,
	.atime = 0,
	.mtime = 0,
	.ctime = 0,
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
	u64 steps = 0;

	for (struct tmpfs_file *file = files_head;
	     file != 0 && steps++ < TMPFS_TREE_WALK_LIMIT; file = file->next) {
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
	       file->path != 0 &&
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

static void files_remove_file(struct tmpfs_file *file)
{
	if (file == 0) {
		return;
	}
	file_list_remove(file);
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
				      u64 task, u64 *status)
{
	struct tmpfs_file *file;
	u64 uid = 0;
	u64 gid = 0;

	if (!path_parent_exists(path)) {
		if (status != 0) {
			*status = BUNIX_VFS_ERR_NOENT;
		}
		return 0;
	}
	if (file_find(path) != 0) {
		if (status != 0) {
			*status = BUNIX_VFS_ERR_EXIST;
		}
		return 0;
	}
	if (type != BUNIX_VFS_TYPE_REGULAR &&
	    type != BUNIX_VFS_TYPE_DIRECTORY &&
	    type != BUNIX_VFS_TYPE_SYMLINK &&
	    type != BUNIX_VFS_TYPE_FIFO) {
		if (status != 0) {
			*status = BUNIX_VFS_ERR_INVAL;
		}
		return 0;
	}
	file = (struct tmpfs_file *)bunix_calloc(1, sizeof(*file));
	if (file == 0) {
		if (status != 0) {
			*status = BUNIX_VFS_ERR_INVAL;
		}
		return 0;
	}
	file->path = str_dup(path);
	if (file->path == 0) {
		bunix_free(file);
		if (status != 0) {
			*status = BUNIX_VFS_ERR_INVAL;
		}
		return 0;
	}
	file->inode = (struct tmpfs_inode *)bunix_calloc(1, sizeof(*file->inode));
	if (file->inode == 0) {
		bunix_free(file->path);
		bunix_free(file);
		if (status != 0) {
			*status = BUNIX_VFS_ERR_INVAL;
		}
		return 0;
	}
	file->inode->refs = 1;
	file->inode->ino = next_inode_id++;
	while (file->inode->ino == 0) {
		file->inode->ino = next_inode_id++;
	}
	if (task != 0) {
		if (user_credential(task, BUNIX_USER_GETEUID, &uid) != 0 ||
		    user_credential(task, BUNIX_USER_GETEGID, &gid) != 0) {
			uid = 0;
			gid = 0;
		}
	}
	file->inode->type = (unsigned int)type;
	file->inode->mode = (unsigned int)(mode & 0777);
	file->inode->uid = (unsigned int)uid;
	file->inode->gid = (unsigned int)gid;
	file->inode->atime = 0;
	file->inode->mtime = 0;
	file->inode->ctime = 0;
	if (files_insert_file(file) != 0) {
		if (status != 0) {
			*status = BUNIX_VFS_ERR_INVAL;
		}
		inode_release(file->inode);
		bunix_free(file->path);
		bunix_free(file);
		return 0;
	}
	if (status != 0) {
		*status = 0;
	}
	return file;
}

static int mkdir_p_path(const char *path, u64 mode, u64 task)
{
	char parent[TMPFS_MAX_PATH];
	struct tmpfs_file *file;
	u64 status = 0;

	if (path == 0 || path[0] != '/') {
		return -1;
	}
	if (path_is_root(path)) {
		return 0;
	}
	file = file_find(path);
	if (file != 0) {
		return file->inode->type == BUNIX_VFS_TYPE_DIRECTORY ? 0 : -1;
	}
	if (path_parent_path(path, parent) != 0 ||
	    mkdir_p_path(parent, mode, task) != 0) {
		return -1;
	}
	file = file_create(path, mode, BUNIX_VFS_TYPE_DIRECTORY, task,
			   &status);
	return file != 0 || status == BUNIX_VFS_ERR_EXIST ? 0 : -1;
}

static u64 open_dir(const char *path, u64 owner)
{
	struct tmpfs_open *open;
	const u64 start = next_open_id;

	open = (struct tmpfs_open *)bunix_calloc(1, sizeof(*open));
	if (open == 0) {
		return 0;
	}
	open->kind = TMPFS_OPEN_DIR;
	open->owner = owner;
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

static u64 open_file(struct tmpfs_file *file, u64 owner)
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
	open->owner = owner;
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

static struct tmpfs_open *open_from_message(u64 handle,
					    const struct bunix_msg *message)
{
	struct tmpfs_open *open = open_from_handle(handle);

	if (open == 0 || message == 0 || open->owner != message->sender) {
		return 0;
	}
	return open;
}

static void close_handle(u64 handle, u64 owner)
{
	struct tmpfs_open *open = open_from_handle(handle);

	if (open != 0 && open->owner == owner) {
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

static int root_has_open_handles(const char *path)
{
	for (struct bunix_u64_tree_node *node =
		     bunix_u64_tree_first_node(&open_files);
	     node != 0; node = bunix_u64_tree_next_node(node)) {
		struct tmpfs_open *open = (struct tmpfs_open *)node->value;

		if (open == 0) {
			continue;
		}
		if (open->kind == TMPFS_OPEN_DIR &&
		    path_is_at_or_under(open->path, path)) {
			return 1;
		}
		if (open->kind == TMPFS_OPEN_FILE && open->file != 0 &&
		    open->file->path != 0 &&
		    path_is_at_or_under(open->file->path, path)) {
			return 1;
		}
	}
	return 0;
}

static void prune_root_files(const char *path)
{
	for (;;) {
		struct tmpfs_file *found = 0;
		u64 steps = 0;

		for (struct tmpfs_file *file = files_head;
		     file != 0 && steps++ < TMPFS_TREE_WALK_LIMIT;
		     file = file->next) {
			if (file != 0 && file->path != 0 &&
			    path_has_prefix_child(file->path, path)) {
				found = file;
				break;
			}
		}
		if (found == 0) {
			return;
		}
		file_unlink(found);
	}
}

static int recycle_stale_root(const char *path)
{
	struct tmpfs_root *root = (struct tmpfs_root *)bunix_tree_get(&roots, path);

	if (root == 0) {
		return 0;
	}
	if (root_has_open_handles(path)) {
		return -1;
	}
	prune_root_files(path);
	bunix_tree_remove_node(&roots, &root->node);
	bunix_free(root->path);
	bunix_free(root);
	return 0;
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
	if (files_insert_file(file) == 0) {
		bunix_free(old_path);
		return 0;
	}
	file->path = old_path;
	if (files_insert_file(file) != 0) {
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
	u64 steps = 0;
	for (struct tmpfs_file *candidate = files_head;
	     candidate != 0 && steps++ < TMPFS_TREE_WALK_LIMIT;
	     candidate = candidate->next) {

		if (file_in_renamed_subtree(candidate, file->path)) {
			count++;
		}
	}
	if (steps >= TMPFS_TREE_WALK_LIMIT) {
		return -1;
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
	steps = 0;
	for (struct tmpfs_file *candidate = files_head;
	     candidate != 0 && steps++ < TMPFS_TREE_WALK_LIMIT;
	     candidate = candidate->next) {

		if (file_in_renamed_subtree(candidate, file->path)) {
			moved[index++] = candidate;
		}
	}
	if (steps >= TMPFS_TREE_WALK_LIMIT) {
		goto out;
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
		if (files_insert_file(moved[i]) != 0) {
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
	if (files_insert_file(link) != 0) {
		inode_release(link->inode);
		bunix_free(link->path);
		bunix_free(link);
		return -1;
	}
	return 0;
}

static u64 stat_buffer_offset(const struct bunix_msg *message)
{
	if (message->type == BUNIX_VFS_STAT_PATH_META_BUFFER) {
		return message->words[0] + message->words[1];
	}
	return message->words[1];
}

static void stat_file(const struct bunix_msg *message, struct bunix_msg *reply,
		      const struct tmpfs_file *file)
{
	reply->words[0] = 0;
	reply->words[1] = file->inode->size;
	reply->words[2] = file->inode->mode | ((u64)file->inode->type << 32);
	reply->words[3] = file->inode->uid | ((u64)file->inode->gid << 32);
	if (message->cap != 0 &&
	    (message->cap_rights & BUNIX_RIGHT_SEND) != 0) {
		(void)bunix_vfs_stat_write_times(
			message->cap, stat_buffer_offset(message),
			file->inode->size, reply->words[2], reply->words[3],
			BUNIX_VFS_DEV_TMPFS, file->inode->ino,
			file->inode->refs, 0, 4096,
			(file->inode->size + 511) / 512,
			file->inode->atime, file->inode->mtime,
			file->inode->ctime);
	}
}

static void stat_dir(const struct bunix_msg *message, struct bunix_msg *reply)
{
	reply->words[0] = 0;
	reply->words[1] = 0;
	reply->words[2] = 0777 | ((u64)BUNIX_VFS_TYPE_DIRECTORY << 32);
	reply->words[3] = 0;
	if (message->cap != 0 &&
	    (message->cap_rights & BUNIX_RIGHT_SEND) != 0) {
		(void)bunix_vfs_stat_write(
			message->cap, stat_buffer_offset(message), 0,
			reply->words[2], 0, BUNIX_VFS_DEV_TMPFS,
			root_inode.ino, root_inode.refs, 0, 4096, 0);
	}
}

static int read_utimens_payload(const struct bunix_msg *message,
				u64 *atime, u64 *mtime)
{
	unsigned char payload[16];
	const u64 offset = message->words[0] + message->words[1];

	if (message == 0 || atime == 0 || mtime == 0 ||
	    message->cap == 0 ||
	    (message->cap_rights & BUNIX_RIGHT_RECV) == 0 ||
	    bunix_buffer_read(message->cap, offset, payload,
			      sizeof(payload)) != 0) {
		return -1;
	}
	*atime = bunix_load_u64_le(payload, 0);
	*mtime = bunix_load_u64_le(payload, 8);
	return 0;
}

static void reply_utimens(const struct bunix_msg *message,
			  struct bunix_msg *reply, struct tmpfs_file *file)
{
	u64 atime;
	u64 mtime;
	const u64 flags = message->words[3] >> 32;
	const u64 now = bunix_clock_monotonic_ns() / 1000000000ull;

	if (file == 0 || file->inode == 0 ||
	    !task_can_access(message->words[3] & 0xffffffff, file, 02) ||
	    read_utimens_payload(message, &atime, &mtime) != 0) {
		reply->words[0] = BUNIX_VFS_ERR_ACCESS;
		return;
	}
	if ((flags & 1) == 0) {
		file->inode->atime = (flags & 4) != 0 ? now : atime;
	}
	if ((flags & 2) == 0) {
		file->inode->mtime = (flags & 8) != 0 ? now : mtime;
	}
	file->inode->ctime = now;
	reply->words[0] = 0;
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
		reply->words[0] = BUNIX_VFS_ERR_INVAL;
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
	(void)service;
	return bunix_names_register_claim(bunix_handle_find(BUNIX_CAP_CLAM),
					  handle);
}

static int sender_is_vfs_caller(u64 sender)
{
	return sender != 0 && bunix_u64_tree_get(&vfs_callers, sender) != 0;
}

static u64 grant_vfs_caller_task(u64 sender, u64 task)
{
	struct tmpfs_vfs_caller *caller;

	if (sender == 0) {
		return BUNIX_VFS_ERR_ACCESS;
	}
	if (vfs_caller_grantor == 0) {
		vfs_caller_grantor = sender;
	} else if (sender != vfs_caller_grantor) {
		return BUNIX_VFS_ERR_ACCESS;
	}
	if (task == 0) {
		task = sender;
	}
	if (bunix_u64_tree_get(&vfs_callers, task) != 0) {
		return 0;
	}
	caller = (struct tmpfs_vfs_caller *)bunix_calloc(1, sizeof(*caller));
	if (caller == 0) {
		return (u64)-1;
	}
	caller->task = task;
	if (bunix_u64_tree_insert_node(&vfs_callers, &caller->node, task,
				       (u64)caller) != 0) {
		bunix_free(caller);
		return (u64)-1;
	}
	return 0;
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

	if (path == 0 || path[0] != '/') {
		return -1;
	}
	if (bunix_tree_get(&roots, path) != 0) {
		return 0;
	}
	if (recycle_stale_root(path) != 0) {
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
	files_head = 0;
	bunix_u64_tree_init(&open_files);
	bunix_u64_tree_init(&vfs_callers);
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
				reply.words[0] = (u64)mount_root(vfs_service, path);
				if (reply.words[0] == 0) {
					bunix_console_log(mounted,
							  sizeof(mounted) - 1);
				}
				break;
			case BUNIX_TMPFS_MKDIR_P_BUFFER:
				if (bunix_read_path_cap(&message, path,
							sizeof(path)) != 0) {
					reply.words[0] = (u64)-1;
					break;
				}
				reply.words[0] =
					(u64)mkdir_p_path(path, message.words[1],
							  message.words[2]);
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
		if (message.type == BUNIX_VFS_GRANT_SUBJECT_TASK) {
			reply.words[0] =
				grant_vfs_caller_task(message.sender,
						      message.words[0]);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			if (message.reply != 0) {
				bunix_ipc_send(message.reply, &reply);
			}
			continue;
		}
		if (!sender_is_vfs_caller(message.sender)) {
			reply.words[0] = BUNIX_VFS_ERR_ACCESS;
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			if (message.reply != 0) {
				bunix_ipc_send(message.reply, &reply);
			}
			continue;
		}
		switch (message.type) {
		case BUNIX_VFS_OPEN_BUFFER:
			if (read_resolved_path(&message, path) != 0) {
				reply.words[0] = BUNIX_VFS_ERR_NOENT;
				break;
			}
			if (path_is_root(path)) {
				reply.words[1] = open_dir(path, message.sender);
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
			reply.words[1] =
				file->inode->type == BUNIX_VFS_TYPE_DIRECTORY ?
				open_dir(path, message.sender) :
				open_file(file, message.sender);
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
				u64 status = 0;

				file = file_create(path, message.words[3] >> 32,
						   BUNIX_VFS_TYPE_REGULAR,
						   message.words[3] &
						   0xffffffff, &status);
				reply.words[0] = status;
				break;
			}
			reply.words[0] =
				file->inode->type == BUNIX_VFS_TYPE_DIRECTORY ?
				BUNIX_VFS_ERR_ISDIR : 0;
			break;
		case BUNIX_VFS_MKDIR_BUFFER:
			if (read_resolved_path(&message, path) != 0) {
				reply.words[0] = BUNIX_VFS_ERR_NOENT;
				break;
			}
			if (path_is_root(path) || file_find(path) != 0) {
				reply.words[0] = BUNIX_VFS_ERR_EXIST;
				break;
			}
			file = parent_file_for_path(path);
			if (!task_can_access(message.words[3] & 0xffffffff,
					     file, 03)) {
				reply.words[0] = BUNIX_VFS_ERR_ACCESS;
				break;
			}
			{
				u64 status = 0;

				file = file_create(path, message.words[3] >> 32,
						   BUNIX_VFS_TYPE_DIRECTORY,
						   message.words[3] &
						   0xffffffff, &status);
				reply.words[0] = status;
			}
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
			{
				u64 status = 0;

				file = file_create(path, mode, type,
						   message.words[3] &
						   0xffffffff, &status);
				reply.words[0] = status;
			}
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
			{
				u64 status = 0;

				file = file_create(path, 0777,
						   BUNIX_VFS_TYPE_SYMLINK,
						   task, &status);
				reply.words[0] = status;
			}
			if (file == 0 ||
			    file_resize(file, target_len) != 0) {
				if (file != 0) {
					file_unlink(file);
				}
				if (reply.words[0] == 0) {
					reply.words[0] = BUNIX_VFS_ERR_INVAL;
				}
				break;
			}
			for (u64 i = 0; i < target_len; i++) {
				file->inode->data[i] = target[i];
			}
			file->inode->size = target_len - 1;
			break;
		}
		case BUNIX_VFS_STAT_PATH_META_BUFFER:
		case BUNIX_VFS_ACCESS_BUFFER:
		case BUNIX_VFS_READLINK_BUFFER:
		case BUNIX_VFS_UTIMENS_BUFFER:
			if (read_resolved_path(&message, path) != 0) {
				reply.words[0] = BUNIX_VFS_ERR_NOENT;
				break;
			}
			if (path_is_root(path)) {
				stat_dir(&message, &reply);
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
			} else if (message.type == BUNIX_VFS_UTIMENS_BUFFER) {
				reply_utimens(&message, &reply, file);
			} else {
				stat_file(&message, &reply, file);
			}
			break;
		case BUNIX_VFS_STAT_META:
			open = open_from_message(message.words[0], &message);
			if (open == 0) {
				reply.words[0] = (u64)-1;
			} else if (open->kind == TMPFS_OPEN_DIR) {
				stat_dir(&message, &reply);
			} else {
				stat_file(&message, &reply, open->file);
			}
			break;
		case BUNIX_VFS_READ_FILE_BUFFER:
			open = open_from_message(message.words[0], &message);
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
		case BUNIX_VFS_MMAP_PAGE_BUFFER:
			open = open_from_message(message.words[0], &message);
			if (open == 0 || open->kind != TMPFS_OPEN_FILE ||
			    message.cap == 0 ||
			    (message.cap_rights & BUNIX_RIGHT_SEND) == 0) {
				reply.words[0] = (u64)-1;
				break;
			}
			if (message.words[1] >= open->file->inode->size) {
				reply.words[0] = 0;
				reply.words[1] = BUNIX_VFS_MMAP_PAGE_BUS;
				reply.words[2] = 0;
				break;
			}
			u64 mmap_nread = open->file->inode->size - message.words[1];
			if (mmap_nread > message.words[2]) {
				mmap_nread = message.words[2];
			}
			if (bunix_buffer_write(message.cap, 0,
					       open->file->inode->data + message.words[1],
					       mmap_nread) != 0) {
				reply.words[0] = (u64)-1;
			} else {
				reply.words[0] = 0;
				reply.words[1] = BUNIX_VFS_MMAP_PAGE_DATA;
				reply.words[2] = mmap_nread;
			}
			break;
		case BUNIX_VFS_WRITE_FILE_BUFFER:
			open = open_from_message(message.words[0], &message);
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
				} else if (!task_can_access(message.words[3] &
							    0xffffffff,
							    file, 02)) {
					reply.words[0] = BUNIX_VFS_ERR_ACCESS;
				} else {
					reply.words[0] = file_set_size(file,
								       message.words[3] >> 32) == 0 ?
							 0 : (u64)-1;
				}
				break;
			}
			open = open_from_message(message.words[0], &message);
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
			open = open_from_message(message.words[0], &message);
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
			open = open_from_message(message.words[0], &message);
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
			open = open_from_message(message.words[0], &message);
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
			u64 steps = 0;
			for (file = files_head;
			     file != 0 && steps++ < TMPFS_TREE_WALK_LIMIT;
			     file = file->next) {
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
			if (file != 0 && steps >= TMPFS_TREE_WALK_LIMIT) {
				reply.words[0] = (u64)-1;
			}
			break;
		case BUNIX_VFS_CLOSE:
			if (open_from_message(message.words[0], &message) == 0) {
				reply.words[0] = (u64)-1;
			} else {
				close_handle(message.words[0], message.sender);
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
