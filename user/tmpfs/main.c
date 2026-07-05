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

struct tmpfs_file {
	struct bunix_tree_node node;
	char *path;
	char *data;
	u64 size;
	u64 capacity;
	u64 refs;
	int deleted;
	unsigned int type;
	unsigned int mode;
	unsigned int uid;
	unsigned int gid;
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
	return file != 0 && file->type == BUNIX_VFS_TYPE_DIRECTORY;
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
	request.words[1] = file->uid;
	request.words[2] = file->gid;
	request.words[3] = ((u64)file->mode << 32) | mask;
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
	return euid == 0 || euid == file->uid;
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
			.data = 0,
			.size = 0,
			.capacity = 0,
			.refs = 0,
			.deleted = 0,
			.type = BUNIX_VFS_TYPE_DIRECTORY,
			.mode = 0777,
			.uid = 0,
			.gid = 0,
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

static void file_release(struct tmpfs_file *file)
{
	if (file == 0) {
		return;
	}
	if (file->refs != 0) {
		file->refs--;
	}
	if (file->refs == 0 && file->deleted) {
		if (file->data != 0) {
			bunix_free(file->data);
		}
		bunix_free(file->path);
		bunix_free(file);
	}
}

static int file_resize(struct tmpfs_file *file, u64 size)
{
	u64 capacity;
	char *data;

	if (file == 0) {
		return -1;
	}
	if (size <= file->capacity) {
		return 0;
	}
	capacity = file->capacity == 0 ? 64 : file->capacity;
	while (capacity < size) {
		capacity *= 2;
	}
	data = (char *)bunix_realloc(file->data, file->capacity, capacity);
	if (data == 0) {
		return -1;
	}
	for (u64 i = file->capacity; i < capacity; i++) {
		data[i] = 0;
	}
	file->data = data;
	file->capacity = capacity;
	return 0;
}

static int file_set_size(struct tmpfs_file *file, u64 size)
{
	const u64 old_size = file != 0 ? file->size : 0;

	if (file == 0 || file_resize(file, size) != 0) {
		return -1;
	}
	if (size > old_size) {
		for (u64 i = old_size; i < size; i++) {
			file->data[i] = 0;
		}
	}
	file->size = size;
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
	     type != BUNIX_VFS_TYPE_DIRECTORY)) {
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
	if (task != 0) {
		if (user_credential(task, BUNIX_USER_GETEUID, &uid) != 0 ||
		    user_credential(task, BUNIX_USER_GETEGID, &gid) != 0) {
			bunix_free(file->path);
			bunix_free(file);
			return 0;
		}
	}
	file->type = (unsigned int)type;
	file->mode = (unsigned int)(mode & 0777);
	file->uid = (unsigned int)uid;
	file->gid = (unsigned int)gid;
	if (bunix_tree_insert_node(&files, &file->node, file->path,
				   (u64)file) != 0) {
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
	bunix_tree_remove_node(&files, &file->node);
	file_release(file);
}

static void stat_file(struct bunix_msg *reply, const struct tmpfs_file *file)
{
	reply->words[0] = 0;
	reply->words[1] = file->size;
	reply->words[2] = file->mode | ((u64)file->type << 32);
	reply->words[3] = file->uid | ((u64)file->gid << 32);
}

static void stat_dir(struct bunix_msg *reply)
{
	reply->words[0] = 0;
	reply->words[1] = 0;
	reply->words[2] = 0777 | ((u64)BUNIX_VFS_TYPE_DIRECTORY << 32);
	reply->words[3] = 0;
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
			reply.words[1] = file->type == BUNIX_VFS_TYPE_DIRECTORY ?
					 open_dir(path) : open_file(file);
			reply.words[2] = file->size;
			reply.words[3] = file->type;
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
		case BUNIX_VFS_STAT_PATH_META_BUFFER:
		case BUNIX_VFS_ACCESS_BUFFER:
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
			if (message.words[1] >= open->file->size) {
				reply.words[0] = 0;
				reply.words[1] = 0;
				break;
			}
			u64 nread = open->file->size - message.words[1];
			if (nread > message.words[2]) {
				nread = message.words[2];
			}
			if (bunix_buffer_write(message.cap, 0,
					       open->file->data + message.words[1],
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
					      open->file->data + message.words[1],
					      message.words[2]) != 0) {
				reply.words[0] = (u64)-1;
				break;
			}
			if (message.words[1] + message.words[2] > open->file->size) {
				open->file->size = message.words[1] + message.words[2];
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
				    file->type != BUNIX_VFS_TYPE_REGULAR) {
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
			file->mode = (unsigned int)((message.words[3] >> 32) &
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
			file->mode = (unsigned int)(message.words[1] & 0777);
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
				file->uid = (unsigned int)message.words[2];
			}
			if ((message.words[3] >> 32) != 0xffffffff) {
				file->gid = (unsigned int)(message.words[3] >> 32);
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
				file->uid = (unsigned int)message.words[1];
			}
			if (message.words[2] != (u64)-1) {
				file->gid = (unsigned int)message.words[2];
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
			if (file->type == BUNIX_VFS_TYPE_DIRECTORY) {
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
			} else if (file->type != BUNIX_VFS_TYPE_DIRECTORY) {
				reply.words[0] = BUNIX_VFS_ERR_NOTDIR;
			} else if (!directory_is_empty(path)) {
				reply.words[0] = (u64)-1;
			} else {
				file_unlink(file);
				reply.words[0] = 0;
			}
			break;
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
							 ((u64)file->type << 32);
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
