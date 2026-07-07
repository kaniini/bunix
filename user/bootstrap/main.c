#include <bunix/alloc.h>
#include <bunix/libbunix.h>

static long register_service_in_namespace(u64 namespace, u64 service, u64 handle)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_NAMES,
		.type = BUNIX_NAMES_REGISTER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP,
		.reply = 0,
		.cap = handle,
		.words = { namespace, service, 0, 0 },
	};
	struct bunix_msg reply;

	if (bunix_ipc_call(BUNIX_HANDLE_NAMES, &request, &reply) != 0) {
		return -1;
	}

	return reply.words[0] == 0 ? 0 : -1;
}

static long register_service(u64 service, u64 handle)
{
	return register_service_in_namespace(BUNIX_NAMES_ROOT, service, handle);
}

static u64 create_namespace(void)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_NAMES,
		.type = BUNIX_NAMES_CREATE_NS,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { 0, 0, 0, 0 },
	};
	struct bunix_msg reply;

	if (bunix_ipc_call(BUNIX_HANDLE_NAMES, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return 0;
	}

	return reply.words[1];
}

static u64 resolve_service_in_namespace(u64 namespace, u64 service,
					unsigned int rights)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_NAMES,
		.type = BUNIX_NAMES_RESOLVE,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { namespace, service, rights, 0 },
	};
	struct bunix_msg reply;

	if (bunix_ipc_call(BUNIX_HANDLE_NAMES, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return 0;
	}

	return reply.cap;
}

static u64 wait_service_in_namespace(u64 namespace, u64 service,
				     unsigned int rights)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_NAMES,
		.type = BUNIX_NAMES_WAIT,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { namespace, service, rights, 0 },
	};
	struct bunix_msg reply;

	if (bunix_ipc_call(BUNIX_HANDLE_NAMES, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return 0;
	}

	return reply.cap;
}

static u64 resolve_service(u64 service, unsigned int rights)
{
	return resolve_service_in_namespace(BUNIX_NAMES_ROOT, service, rights);
}

static u64 str_len(const char *text)
{
	u64 len = 0;

	while (text[len] != '\0') {
		len++;
	}
	return len;
}

static int str_eq(const char *left, const char *right)
{
	u64 i = 0;

	while (left != 0 && right != 0 && left[i] != '\0' &&
	       right[i] != '\0') {
		if (left[i] != right[i]) {
			return 0;
		}
		i++;
	}
	return left != 0 && right != 0 && left[i] == right[i];
}

enum {
	BOOT_CONFIG_MAX = 512 * 1024,
	BOOT_TOKEN_MAX = 4096,
	BOOT_NAME_MAX = 256,
};

struct boot_spawn_args {
	char **args;
	char **envs;
	u64 arg_count;
	u64 env_count;
	u64 arg_cap;
	u64 env_cap;
};

static int is_space(char c)
{
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static void skip_line(const char *text, u64 *pos)
{
	while (text[*pos] != '\0' && text[*pos] != '\n') {
		(*pos)++;
	}
	if (text[*pos] == '\n') {
		(*pos)++;
	}
}

static int read_token(const char *text, u64 *pos, char *out, u64 out_size)
{
	u64 len = 0;

	while (is_space(text[*pos])) {
		(*pos)++;
	}
	if (text[*pos] == '\0' || text[*pos] == '#') {
		return -1;
	}
	while (text[*pos] != '\0' && !is_space(text[*pos]) &&
	       text[*pos] != '#') {
		if (len + 1 >= out_size) {
			return -1;
		}
		out[len++] = text[*pos];
		(*pos)++;
	}
	if (len == 0) {
		return -1;
	}
	out[len] = '\0';
	return 0;
}

static char *str_dup(const char *text)
{
	const u64 len = str_len(text) + 1;
	char *copy = (char *)bunix_alloc(len);

	if (copy == 0) {
		return 0;
	}
	for (u64 i = 0; i < len; i++) {
		copy[i] = text[i];
	}
	return copy;
}

static void boot_spawn_args_free(struct boot_spawn_args *spawn)
{
	if (spawn == 0) {
		return;
	}
	for (u64 i = 0; i < spawn->arg_count; i++) {
		bunix_free(spawn->args[i]);
	}
	for (u64 i = 0; i < spawn->env_count; i++) {
		bunix_free(spawn->envs[i]);
	}
	bunix_free(spawn->args);
	bunix_free(spawn->envs);
	spawn->args = 0;
	spawn->envs = 0;
	spawn->arg_count = 0;
	spawn->env_count = 0;
	spawn->arg_cap = 0;
	spawn->env_cap = 0;
}

static int boot_spawn_push(char ***values, u64 *count, u64 *cap,
			   const char *text)
{
	char **next;
	char *copy;
	u64 old_size;
	u64 new_cap;

	if (*count == *cap) {
		old_size = *cap * sizeof(char *);
		new_cap = *cap == 0 ? 8 : *cap * 2;
		next = (char **)bunix_realloc(*values, old_size,
					      new_cap * sizeof(char *));
		if (next == 0) {
			return -1;
		}
		*values = next;
		*cap = new_cap;
	}
	copy = str_dup(text);
	if (copy == 0) {
		return -1;
	}
	(*values)[(*count)++] = copy;
	return 0;
}

static long proc_register_exec(u64 proc, const char *path,
			       const char *task_name, u64 linux)
{
	const u64 path_len = str_len(path) + 1;
	const u64 task_len = str_len(task_name) + 1;
	const long buffer = bunix_buffer_create(path_len + task_len);
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_PROC,
		.type = BUNIX_PROC_REGISTER_EXEC,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_RECV | BUNIX_RIGHT_DUP,
		.reply = 0,
		.cap = buffer > 0 ? (u64)buffer : 0,
		.words = { path_len, task_len, linux, 0 },
	};
	struct bunix_msg reply;

	if (proc == 0 || buffer <= 0 ||
	    bunix_buffer_write((u64)buffer, 0, path, path_len) != 0 ||
	    bunix_buffer_write((u64)buffer, path_len, task_name,
			       task_len) != 0 ||
	    bunix_ipc_call(proc, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		if (buffer > 0) {
			bunix_handle_close((u64)buffer);
		}
		return -1;
	}
	bunix_handle_close((u64)buffer);
	return 0;
}

struct boot_path {
	const char *path;
};

struct boot_path_command {
	unsigned int protocol;
	unsigned int type;
	const struct boot_path *paths;
	u64 path_count;
};

static long vfs_read_text(u64 vfs, const char *path, char *out, u64 out_size)
{
	const char cwd[] = "/";
	const u64 cwd_len = sizeof(cwd);
	const u64 path_len = path != 0 ? str_len(path) + 1 : 0;
	const long path_buffer = path_len != 0 ?
				 bunix_buffer_create(cwd_len + path_len) : -1;
	const long io_buffer = out_size != 0 ? bunix_buffer_create(out_size) :
					       -1;
	struct bunix_msg open_request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_OPEN_BUFFER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_RECV,
		.reply = 0,
		.cap = path_buffer > 0 ? (u64)path_buffer : 0,
		.words = { cwd_len, path_len, 0, 0 },
	};
	struct bunix_msg read_request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_READ_FILE_BUFFER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP,
		.reply = 0,
		.cap = io_buffer > 0 ? (u64)io_buffer : 0,
		.words = { 0, 0, out_size != 0 ? out_size - 1 : 0, 0 },
	};
	struct bunix_msg close_request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_CLOSE,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { 0, 0, 0, 0 },
	};
	struct bunix_msg reply;
	u64 read_len;

	if (vfs == 0 || path == 0 || out == 0 || out_size == 0 ||
	    out_size > BOOT_CONFIG_MAX || path_buffer <= 0 ||
	    io_buffer <= 0 ||
	    bunix_buffer_write((u64)path_buffer, 0, cwd, cwd_len) != 0 ||
	    bunix_buffer_write((u64)path_buffer, cwd_len, path,
			       path_len) != 0) {
		if (path_buffer > 0) {
			bunix_handle_close((u64)path_buffer);
		}
		if (io_buffer > 0) {
			bunix_handle_close((u64)io_buffer);
		}
		return -1;
	}
	if (bunix_ipc_call(vfs, &open_request, &reply) != 0 ||
	    reply.words[0] != 0 || reply.words[1] == 0 ||
	    reply.words[3] != BUNIX_VFS_TYPE_REGULAR) {
		bunix_handle_close((u64)path_buffer);
		bunix_handle_close((u64)io_buffer);
		return -1;
	}
	bunix_handle_close((u64)path_buffer);
	read_request.words[0] = reply.words[1];
	if (reply.words[2] < read_request.words[2]) {
		read_request.words[2] = reply.words[2];
	}
	if (bunix_ipc_call(vfs, &read_request, &reply) != 0 ||
	    reply.words[0] != 0 || reply.words[1] >= out_size ||
	    bunix_buffer_read((u64)io_buffer, 0, out, reply.words[1]) != 0) {
		close_request.words[0] = read_request.words[0];
		(void)bunix_ipc_call(vfs, &close_request, &reply);
		bunix_handle_close((u64)io_buffer);
		return -1;
	}
	read_len = reply.words[1];
	close_request.words[0] = read_request.words[0];
	if (bunix_ipc_call(vfs, &close_request, &reply) != 0 ||
	    reply.words[0] != 0) {
		bunix_handle_close((u64)io_buffer);
		return -1;
	}
	out[read_len] = '\0';
	bunix_handle_close((u64)io_buffer);
	return 0;
}

static long vfs_open_path(u64 vfs, const char *path, u64 *handle, u64 *type);
static void vfs_close_handle(u64 vfs, u64 handle);

static long vfs_write_text(u64 vfs, const char *path, u64 offset,
			   const char *text, u64 len)
{
	u64 handle = 0;
	u64 type = 0;
	const long buffer = len != 0 ? bunix_buffer_create(len) : -1;
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_WRITE_FILE_BUFFER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_RECV | BUNIX_RIGHT_DUP,
		.reply = 0,
		.cap = buffer > 0 ? (u64)buffer : 0,
		.words = { 0, offset, len, 0 },
	};
	struct bunix_msg reply;

	if (buffer <= 0 ||
	    bunix_buffer_write((u64)buffer, 0, text, len) != 0 ||
	    vfs_open_path(vfs, path, &handle, &type) != 0 ||
	    type != BUNIX_VFS_TYPE_REGULAR) {
		if (buffer > 0) {
			bunix_handle_close((u64)buffer);
		}
		vfs_close_handle(vfs, handle);
		return -1;
	}
	request.words[0] = handle;
	if (bunix_ipc_call(vfs, &request, &reply) != 0 ||
	    reply.words[0] != 0 ||
	    reply.words[1] != len) {
		bunix_handle_close((u64)buffer);
		vfs_close_handle(vfs, handle);
		return -1;
	}
	bunix_handle_close((u64)buffer);
	vfs_close_handle(vfs, handle);
	return 0;
}

static long vfs_truncate_path(u64 vfs, const char *path, u64 size)
{
	u64 handle = 0;
	u64 type = 0;
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_TRUNCATE,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { 0, size, 0, 0 },
	};
	struct bunix_msg reply;

	if (vfs_open_path(vfs, path, &handle, &type) != 0 ||
	    type != BUNIX_VFS_TYPE_REGULAR) {
		vfs_close_handle(vfs, handle);
		return -1;
	}
	request.words[0] = handle;
	if (bunix_ipc_call(vfs, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		vfs_close_handle(vfs, handle);
		return -1;
	}
	vfs_close_handle(vfs, handle);
	return 0;
}

static long vfs_chmod_path(u64 vfs, const char *path, u64 mode)
{
	u64 handle = 0;
	u64 type = 0;
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_CHMOD,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { 0, mode, 0, 0 },
	};
	struct bunix_msg reply;

	if (vfs_open_path(vfs, path, &handle, &type) != 0 || type == 0) {
		vfs_close_handle(vfs, handle);
		return -1;
	}
	request.words[0] = handle;
	if (bunix_ipc_call(vfs, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		vfs_close_handle(vfs, handle);
		return -1;
	}
	vfs_close_handle(vfs, handle);
	return 0;
}

static long vfs_chown_path(u64 vfs, const char *path, u64 uid, u64 gid)
{
	u64 handle = 0;
	u64 type = 0;
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_CHOWN,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { 0, uid, gid, 0 },
	};
	struct bunix_msg reply;

	if (vfs_open_path(vfs, path, &handle, &type) != 0 || type == 0) {
		vfs_close_handle(vfs, handle);
		return -1;
	}
	request.words[0] = handle;
	if (bunix_ipc_call(vfs, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		vfs_close_handle(vfs, handle);
		return -1;
	}
	vfs_close_handle(vfs, handle);
	return 0;
}

static long vfs_chmod_path_buffer(u64 vfs, const char *path, u64 mode)
{
	const char cwd[] = "/";
	const u64 cwd_len = sizeof(cwd);
	const u64 path_len = path != 0 ? str_len(path) + 1 : 0;
	const long buffer = path_len != 0 ?
			    bunix_buffer_create(cwd_len + path_len) : -1;
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_CHMOD_BUFFER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_RECV,
		.reply = 0,
		.cap = buffer > 0 ? (u64)buffer : 0,
		.words = { cwd_len, path_len, 0, mode << 32 },
	};
	struct bunix_msg reply;

	if (vfs == 0 || path == 0 || buffer <= 0 ||
	    bunix_buffer_write((u64)buffer, 0, cwd, cwd_len) != 0 ||
	    bunix_buffer_write((u64)buffer, cwd_len, path, path_len) != 0 ||
	    bunix_ipc_call(vfs, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		if (buffer > 0) {
			bunix_handle_close((u64)buffer);
		}
		return -1;
	}
	bunix_handle_close((u64)buffer);
	return 0;
}

static long vfs_chown_path_buffer(u64 vfs, const char *path, u64 uid, u64 gid)
{
	const char cwd[] = "/";
	const u64 cwd_len = sizeof(cwd);
	const u64 path_len = path != 0 ? str_len(path) + 1 : 0;
	const long buffer = path_len != 0 ?
			    bunix_buffer_create(cwd_len + path_len) : -1;
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_CHOWN_BUFFER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_RECV,
		.reply = 0,
		.cap = buffer > 0 ? (u64)buffer : 0,
		.words = { cwd_len, path_len, uid, gid << 32 },
	};
	struct bunix_msg reply;

	if (vfs == 0 || path == 0 || buffer <= 0 ||
	    bunix_buffer_write((u64)buffer, 0, cwd, cwd_len) != 0 ||
	    bunix_buffer_write((u64)buffer, cwd_len, path, path_len) != 0 ||
	    bunix_ipc_call(vfs, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		if (buffer > 0) {
			bunix_handle_close((u64)buffer);
		}
		return -1;
	}
	bunix_handle_close((u64)buffer);
	return 0;
}

static long vfs_create_path_buffer(u64 vfs, const char *path, u64 mode)
{
	const char cwd[] = "/";
	const u64 cwd_len = sizeof(cwd);
	const u64 path_len = path != 0 ? str_len(path) + 1 : 0;
	const long buffer = path_len != 0 ?
			    bunix_buffer_create(cwd_len + path_len) : -1;
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_CREATE_BUFFER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_RECV,
		.reply = 0,
		.cap = buffer > 0 ? (u64)buffer : 0,
		.words = { cwd_len, path_len, 0, mode << 32 },
	};
	struct bunix_msg reply;

	if (vfs == 0 || path == 0 || buffer <= 0 ||
	    bunix_buffer_write((u64)buffer, 0, cwd, cwd_len) != 0 ||
	    bunix_buffer_write((u64)buffer, cwd_len, path, path_len) != 0 ||
	    bunix_ipc_call(vfs, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		if (buffer > 0) {
			bunix_handle_close((u64)buffer);
		}
		return -1;
	}
	bunix_handle_close((u64)buffer);
	return 0;
}

static long vfs_mkdir_path_buffer(u64 vfs, const char *path, u64 mode)
{
	const char cwd[] = "/";
	const u64 cwd_len = sizeof(cwd);
	const u64 path_len = path != 0 ? str_len(path) + 1 : 0;
	const long buffer = path_len != 0 ?
			    bunix_buffer_create(cwd_len + path_len) : -1;
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_MKDIR_BUFFER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_RECV,
		.reply = 0,
		.cap = buffer > 0 ? (u64)buffer : 0,
		.words = { cwd_len, path_len, 0, mode << 32 },
	};
	struct bunix_msg reply;

	if (vfs == 0 || path == 0 || buffer <= 0 ||
	    bunix_buffer_write((u64)buffer, 0, cwd, cwd_len) != 0 ||
	    bunix_buffer_write((u64)buffer, cwd_len, path, path_len) != 0 ||
	    bunix_ipc_call(vfs, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		if (buffer > 0) {
			bunix_handle_close((u64)buffer);
		}
		return -1;
	}
	bunix_handle_close((u64)buffer);
	return 0;
}

static long vfs_unlink_path_buffer(u64 vfs, const char *path)
{
	const char cwd[] = "/";
	const u64 cwd_len = sizeof(cwd);
	const u64 path_len = path != 0 ? str_len(path) + 1 : 0;
	const long buffer = path_len != 0 ?
			    bunix_buffer_create(cwd_len + path_len) : -1;
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_UNLINK_BUFFER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_RECV,
		.reply = 0,
		.cap = buffer > 0 ? (u64)buffer : 0,
		.words = { cwd_len, path_len, 0, 0 },
	};
	struct bunix_msg reply;

	if (vfs == 0 || path == 0 || buffer <= 0 ||
	    bunix_buffer_write((u64)buffer, 0, cwd, cwd_len) != 0 ||
	    bunix_buffer_write((u64)buffer, cwd_len, path, path_len) != 0 ||
	    bunix_ipc_call(vfs, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		if (buffer > 0) {
			bunix_handle_close((u64)buffer);
		}
		return -1;
	}
	bunix_handle_close((u64)buffer);
	return 0;
}

static long vfs_rmdir_path_buffer(u64 vfs, const char *path)
{
	const char cwd[] = "/";
	const u64 cwd_len = sizeof(cwd);
	const u64 path_len = path != 0 ? str_len(path) + 1 : 0;
	const long buffer = path_len != 0 ?
			    bunix_buffer_create(cwd_len + path_len) : -1;
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_RMDIR_BUFFER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_RECV,
		.reply = 0,
		.cap = buffer > 0 ? (u64)buffer : 0,
		.words = { cwd_len, path_len, 0, 0 },
	};
	struct bunix_msg reply;

	if (vfs == 0 || path == 0 || buffer <= 0 ||
	    bunix_buffer_write((u64)buffer, 0, cwd, cwd_len) != 0 ||
	    bunix_buffer_write((u64)buffer, cwd_len, path, path_len) != 0 ||
	    bunix_ipc_call(vfs, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		if (buffer > 0) {
			bunix_handle_close((u64)buffer);
		}
		return -1;
	}
	bunix_handle_close((u64)buffer);
	return 0;
}

static long vfs_link_path_buffer(u64 vfs, const char *old_path,
				 const char *new_path)
{
	const char cwd[] = "/";
	const u64 cwd_len = sizeof(cwd);
	const u64 old_len = old_path != 0 ? str_len(old_path) + 1 : 0;
	const u64 new_len = new_path != 0 ? str_len(new_path) + 1 : 0;
	const u64 old_off = cwd_len;
	const u64 new_cwd_off = old_off + old_len;
	const u64 new_off = new_cwd_off + cwd_len;
	const long buffer = old_len != 0 && new_len != 0 ?
			    bunix_buffer_create(new_off + new_len) : -1;
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_LINK_BUFFER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_RECV,
		.reply = 0,
		.cap = buffer > 0 ? (u64)buffer : 0,
		.words = {
			0,
			cwd_len | (old_len << 32),
			cwd_len | (new_len << 32),
			0,
		},
	};
	struct bunix_msg reply;

	if (vfs == 0 || old_path == 0 || new_path == 0 || buffer <= 0 ||
	    bunix_buffer_write((u64)buffer, 0, cwd, cwd_len) != 0 ||
	    bunix_buffer_write((u64)buffer, old_off, old_path, old_len) != 0 ||
	    bunix_buffer_write((u64)buffer, new_cwd_off, cwd, cwd_len) != 0 ||
	    bunix_buffer_write((u64)buffer, new_off, new_path, new_len) != 0 ||
	    bunix_ipc_call(vfs, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		if (buffer > 0) {
			bunix_handle_close((u64)buffer);
		}
		return -1;
	}
	bunix_handle_close((u64)buffer);
	return 0;
}

static long vfs_rename_path_buffer(u64 vfs, const char *old_path,
				   const char *new_path, u64 flags)
{
	const char cwd[] = "/";
	const u64 cwd_len = sizeof(cwd);
	const u64 old_len = old_path != 0 ? str_len(old_path) + 1 : 0;
	const u64 new_len = new_path != 0 ? str_len(new_path) + 1 : 0;
	const u64 old_off = cwd_len;
	const u64 new_cwd_off = old_off + old_len;
	const u64 new_off = new_cwd_off + cwd_len;
	const long buffer = old_len != 0 && new_len != 0 ?
			    bunix_buffer_create(new_off + new_len) : -1;
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_RENAME_BUFFER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_RECV,
		.reply = 0,
		.cap = buffer > 0 ? (u64)buffer : 0,
		.words = {
			0,
			cwd_len | (old_len << 32),
			cwd_len | (new_len << 32),
			flags << 32,
		},
	};
	struct bunix_msg reply;

	if (vfs == 0 || old_path == 0 || new_path == 0 || buffer <= 0 ||
	    bunix_buffer_write((u64)buffer, 0, cwd, cwd_len) != 0 ||
	    bunix_buffer_write((u64)buffer, old_off, old_path, old_len) != 0 ||
	    bunix_buffer_write((u64)buffer, new_cwd_off, cwd, cwd_len) != 0 ||
	    bunix_buffer_write((u64)buffer, new_off, new_path, new_len) != 0 ||
	    bunix_ipc_call(vfs, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		if (buffer > 0) {
			bunix_handle_close((u64)buffer);
		}
		return -1;
	}
	bunix_handle_close((u64)buffer);
	return 0;
}

static long vfs_symlink_path_buffer(u64 vfs, const char *target,
				    const char *path)
{
	const char cwd[] = "/";
	const u64 target_len = target != 0 ? str_len(target) + 1 : 0;
	const u64 cwd_len = sizeof(cwd);
	const u64 path_len = path != 0 ? str_len(path) + 1 : 0;
	const u64 cwd_off = target_len;
	const u64 path_off = cwd_off + cwd_len;
	const long buffer = target_len != 0 && path_len != 0 ?
			    bunix_buffer_create(path_off + path_len) : -1;
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_SYMLINK_BUFFER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_RECV,
		.reply = 0,
		.cap = buffer > 0 ? (u64)buffer : 0,
		.words = { target_len, cwd_len, path_len, 0 },
	};
	struct bunix_msg reply;

	if (vfs == 0 || target == 0 || path == 0 || buffer <= 0 ||
	    bunix_buffer_write((u64)buffer, 0, target, target_len) != 0 ||
	    bunix_buffer_write((u64)buffer, cwd_off, cwd, cwd_len) != 0 ||
	    bunix_buffer_write((u64)buffer, path_off, path, path_len) != 0 ||
	    bunix_ipc_call(vfs, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		if (buffer > 0) {
			bunix_handle_close((u64)buffer);
		}
		return -1;
	}
	bunix_handle_close((u64)buffer);
	return 0;
}

static long vfs_stat_path(u64 vfs, const char *path, u64 *size, u64 *ino,
			  u64 *nlink, u64 *type, u64 *mode, u64 *uid,
			  u64 *gid, u64 *blocks)
{
	const char cwd[] = "/";
	const u64 cwd_len = sizeof(cwd);
	const u64 path_len = path != 0 ? str_len(path) + 1 : 0;
	const u64 stat_offset = cwd_len + path_len;
	const long buffer = path_len != 0 ?
			    bunix_buffer_create(stat_offset +
						BUNIX_VFS_STAT_BYTES) : -1;
	unsigned char stat[BUNIX_VFS_STAT_BYTES];
	u64 mode_type;
	u64 owner;
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_STAT_PATH_META_BUFFER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_RECV | BUNIX_RIGHT_SEND,
		.reply = 0,
		.cap = buffer > 0 ? (u64)buffer : 0,
		.words = { cwd_len, path_len, 0, 0 },
	};
	struct bunix_msg reply;

	if (vfs == 0 || path == 0 || buffer <= 0 ||
	    bunix_buffer_write((u64)buffer, 0, cwd, cwd_len) != 0 ||
	    bunix_buffer_write((u64)buffer, cwd_len, path, path_len) != 0 ||
	    bunix_ipc_call(vfs, &request, &reply) != 0 ||
	    reply.words[0] != 0 ||
	    bunix_buffer_read((u64)buffer, stat_offset, stat,
			      sizeof(stat)) != 0) {
		if (buffer > 0) {
			bunix_handle_close((u64)buffer);
		}
		return -1;
	}
	mode_type = bunix_load_u64_le(stat, BUNIX_VFS_STAT_MODE_TYPE);
	owner = bunix_load_u64_le(stat, BUNIX_VFS_STAT_OWNER);
	if (size != 0) {
		*size = bunix_load_u64_le(stat, BUNIX_VFS_STAT_SIZE);
	}
	if (ino != 0) {
		*ino = bunix_load_u64_le(stat, BUNIX_VFS_STAT_INO);
	}
	if (nlink != 0) {
		*nlink = bunix_load_u64_le(stat, BUNIX_VFS_STAT_NLINK);
	}
	if (type != 0) {
		*type = mode_type >> 32;
	}
	if (mode != 0) {
		*mode = mode_type & 0xffffffff;
	}
	if (uid != 0) {
		*uid = owner & 0xffffffff;
	}
	if (gid != 0) {
		*gid = owner >> 32;
	}
	if (blocks != 0) {
		*blocks = bunix_load_u64_le(stat, BUNIX_VFS_STAT_BLOCKS);
	}
	bunix_handle_close((u64)buffer);
	return 0;
}

static long vfs_open_path(u64 vfs, const char *path, u64 *handle, u64 *type)
{
	const char cwd[] = "/";
	const u64 cwd_len = sizeof(cwd);
	const u64 path_len = path != 0 ? str_len(path) + 1 : 0;
	const long buffer = path_len != 0 ?
			    bunix_buffer_create(cwd_len + path_len) : -1;
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_OPEN_BUFFER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_RECV,
		.reply = 0,
		.cap = buffer > 0 ? (u64)buffer : 0,
		.words = { cwd_len, path_len, 0, 0 },
	};
	struct bunix_msg reply;

	if (vfs == 0 || path == 0 || buffer <= 0 ||
	    bunix_buffer_write((u64)buffer, 0, cwd, cwd_len) != 0 ||
	    bunix_buffer_write((u64)buffer, cwd_len, path, path_len) != 0 ||
	    bunix_ipc_call(vfs, &request, &reply) != 0 ||
	    reply.words[0] != 0 || reply.words[1] == 0) {
		if (buffer > 0) {
			bunix_handle_close((u64)buffer);
		}
		return -1;
	}
	bunix_handle_close((u64)buffer);
	if (handle != 0) {
		*handle = reply.words[1];
	}
	if (type != 0) {
		*type = reply.words[3];
	}
	return 0;
}

static void vfs_close_handle(u64 vfs, u64 handle)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_CLOSE,
		.words = { handle, 0, 0, 0 },
	};
	struct bunix_msg reply;

	if (vfs != 0 && handle != 0) {
		(void)bunix_ipc_call(vfs, &request, &reply);
	}
}

static long vfs_readdir_has(u64 vfs, const char *path, const char *name)
{
	u64 handle = 0;
	u64 type = 0;
	u64 cookie = 0;
	const long buffer = bunix_buffer_create(BOOT_TOKEN_MAX);
	char entry[BOOT_TOKEN_MAX];
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_READDIR_BUFFER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_SEND,
		.reply = 0,
		.cap = buffer > 0 ? (u64)buffer : 0,
		.words = { 0, 0, 0, BOOT_TOKEN_MAX - 1 },
	};
	struct bunix_msg reply;

	if (buffer <= 0 ||
	    vfs_open_path(vfs, path, &handle, &type) != 0 ||
	    type != BUNIX_VFS_TYPE_DIRECTORY) {
		if (buffer > 0) {
			bunix_handle_close((u64)buffer);
		}
		vfs_close_handle(vfs, handle);
		return -1;
	}
	for (;;) {
		request.words[0] = handle;
		request.words[1] = cookie;
		if (bunix_ipc_call(vfs, &request, &reply) != 0 ||
		    reply.words[0] != 0) {
			break;
		}
		if (reply.words[3] >= sizeof(entry) ||
		    bunix_buffer_read((u64)buffer, 0, entry,
				      reply.words[3]) != 0) {
			break;
		}
		entry[reply.words[3]] = '\0';
		if (str_eq(entry, name)) {
			bunix_handle_close((u64)buffer);
			vfs_close_handle(vfs, handle);
			return 0;
		}
		cookie = reply.words[1] & 0xffffffff;
	}
	bunix_handle_close((u64)buffer);
	vfs_close_handle(vfs, handle);
	return -1;
}

static long ext2_readonly_selftest(u64 vfs)
{
	const char hello[] = "ext2 hello\n";
	const char via_link[] = "ext2 hello\n";
	const char long_name[] =
		"long-name-abcdefghijklmnopqrstuvwxyz0123456789-"
		"abcdefghijklmnopqrstuvwxyz0123456789-"
		"abcdefghijklmnopqrstuvwxyz0123456789.txt";
	char text[64];
	u64 hello_size = 0;
	u64 hello_ino = 0;
	u64 hello_nlink = 0;
	u64 hello_type = 0;
	u64 hard_ino = 0;
	u64 hard_nlink = 0;
	u64 sparse_size = 0;
	u64 sparse_type = 0;
	u64 hello_mode = 0;
	u64 hello_uid = 0;
	u64 hello_gid = 0;
	u64 hello_blocks = 0;
	u64 dir_type = 0;
	u64 dir_mode = 0;
	u64 dir_nlink = 0;
	u64 created_ino = 0;
	u64 linked_ino = 0;
	const int ext2_fsck_mode = bunix_cmdline_has("ext2-fsck-test") > 0;
	const char *created_symlink_target = ext2_fsck_mode ?
					     "created.txt" :
					     "/mnt/ext2/created.txt";

#define EXT2_STEP(text) bunix_console_log((text), sizeof(text) - 1)

	if (vfs_read_text(vfs, "/mnt/ext2/hello.txt", text,
			  sizeof(text)) != 0 ||
	    !str_eq(text, hello)) {
		return -1;
	}
	EXT2_STEP("bootstrap: ext2 step read hello\n");
	if (!ext2_fsck_mode &&
	    (vfs_read_text(vfs, "/mnt/ext2/link-to-hello", text,
			   sizeof(text)) != 0 ||
	     !str_eq(text, via_link))) {
		return -1;
	}
	if (vfs_stat_path(vfs, "/mnt/ext2/hello.txt", &hello_size,
			  &hello_ino, &hello_nlink, &hello_type, 0, 0,
			  0, 0) != 0 ||
	    hello_size != sizeof(hello) - 1 ||
	    hello_type != BUNIX_VFS_TYPE_REGULAR ||
	    hello_nlink < 2 ||
	    vfs_stat_path(vfs, "/mnt/ext2/hard/hello-hard", 0, &hard_ino,
			  &hard_nlink, 0, 0, 0, 0, 0) != 0 ||
	    hard_ino != hello_ino ||
	    hard_nlink != hello_nlink) {
		return -1;
	}
	EXT2_STEP("bootstrap: ext2 step metadata\n");
	if (vfs_readdir_has(vfs, "/mnt/ext2/names", long_name) != 0 ||
	    vfs_read_text(vfs, "/mnt/ext2/names/long-name-abcdefghijklmnopqrstuvwxyz0123456789-abcdefghijklmnopqrstuvwxyz0123456789-abcdefghijklmnopqrstuvwxyz0123456789.txt",
			  text, sizeof(text)) != 0 ||
	    !str_eq(text, "long name ok\n")) {
		return -1;
	}
	if (vfs_stat_path(vfs, "/mnt/ext2/sparse-ish.bin", &sparse_size,
			  0, 0, &sparse_type, 0, 0, 0, 0) != 0 ||
	    sparse_type != BUNIX_VFS_TYPE_REGULAR ||
	    sparse_size < 8192) {
		return -1;
	}
	EXT2_STEP("bootstrap: ext2 step readonly\n");
	if (vfs_write_text(vfs, "/mnt/ext2/hello.txt", 0, "EXT2", 4) != 0 ||
	    vfs_read_text(vfs, "/mnt/ext2/hello.txt", text,
			  sizeof(text)) != 0 ||
	    !str_eq(text, "EXT2 hello\n") ||
	    vfs_read_text(vfs, "/mnt/ext2/hard/hello-hard", text,
			  sizeof(text)) != 0 ||
	    !str_eq(text, "EXT2 hello\n")) {
		return -1;
	}
	if (vfs_truncate_path(vfs, "/mnt/ext2/hello.txt", 4) != 0 ||
	    vfs_read_text(vfs, "/mnt/ext2/hard/hello-hard", text,
			  sizeof(text)) != 0 ||
	    !str_eq(text, "EXT2")) {
		return -1;
	}
	EXT2_STEP("bootstrap: ext2 step write truncate\n");
	if (vfs_chmod_path(vfs, "/mnt/ext2/hello.txt", 0600) != 0 ||
	    vfs_chown_path(vfs, "/mnt/ext2/hello.txt", 123, 45) != 0 ||
	    vfs_stat_path(vfs, "/mnt/ext2/hard/hello-hard", 0, 0, 0, 0,
			  &hello_mode, &hello_uid, &hello_gid, 0) != 0 ||
	    hello_mode != 0600 ||
	    hello_uid != 123 ||
	    hello_gid != 45) {
		return -1;
	}
	if (vfs_chmod_path_buffer(vfs, "/mnt/ext2/hello.txt", 0644) != 0 ||
	    vfs_chown_path_buffer(vfs, "/mnt/ext2/hello.txt", 321, 54) != 0 ||
	    vfs_stat_path(vfs, "/mnt/ext2/hard/hello-hard", 0, 0, 0, 0,
			  &hello_mode, &hello_uid, &hello_gid, 0) != 0 ||
	    hello_mode != 0644 ||
	    hello_uid != 321 ||
	    hello_gid != 54) {
		return -1;
	}
	EXT2_STEP("bootstrap: ext2 step ownership\n");
	if (vfs_create_path_buffer(vfs, "/mnt/ext2/created.txt", 0640) != 0 ||
	    vfs_stat_path(vfs, "/mnt/ext2/created.txt", &hello_size, 0, 0,
			  &hello_type, &hello_mode, &hello_uid,
			  &hello_gid, 0) != 0 ||
	    hello_size != 0 ||
	    hello_type != BUNIX_VFS_TYPE_REGULAR ||
	    hello_mode != 0640 ||
	    hello_uid != 0 ||
	    hello_gid != 0 ||
	    vfs_readdir_has(vfs, "/mnt/ext2", "created.txt") != 0) {
		return -1;
	}
	EXT2_STEP("bootstrap: ext2 step create\n");
	if (vfs_write_text(vfs, "/mnt/ext2/created.txt", 0, "created ok\n",
			   11) != 0 ||
	    vfs_read_text(vfs, "/mnt/ext2/created.txt", text,
			  sizeof(text)) != 0 ||
	    !str_eq(text, "created ok\n") ||
	    vfs_stat_path(vfs, "/mnt/ext2/created.txt", &hello_size, 0, 0,
			  &hello_type, 0, 0, 0, &hello_blocks) != 0 ||
	    hello_size != 11 ||
	    hello_type != BUNIX_VFS_TYPE_REGULAR ||
	    hello_blocks == 0) {
		return -1;
	}
	if (vfs_rename_path_buffer(vfs, "/mnt/ext2/created.txt",
				   "/mnt/ext2/renamed.txt", 0) != 0 ||
	    vfs_stat_path(vfs, "/mnt/ext2/created.txt", 0, 0, 0, 0,
			  0, 0, 0, 0) == 0 ||
	    vfs_read_text(vfs, "/mnt/ext2/renamed.txt", text,
			  sizeof(text)) != 0 ||
	    !str_eq(text, "created ok\n") ||
	    vfs_readdir_has(vfs, "/mnt/ext2", "renamed.txt") != 0) {
		return -1;
	}
	if (vfs_create_path_buffer(vfs, "/mnt/ext2/rename-target.txt",
				   0600) != 0 ||
	    vfs_write_text(vfs, "/mnt/ext2/rename-target.txt", 0,
			   "target\n", 7) != 0 ||
	    vfs_rename_path_buffer(vfs, "/mnt/ext2/renamed.txt",
				   "/mnt/ext2/rename-target.txt", 1) == 0 ||
	    vfs_read_text(vfs, "/mnt/ext2/rename-target.txt", text,
			  sizeof(text)) != 0 ||
	    !str_eq(text, "target\n") ||
	    vfs_rename_path_buffer(vfs, "/mnt/ext2/renamed.txt",
				   "/mnt/ext2/rename-target.txt", 0) != 0 ||
	    vfs_stat_path(vfs, "/mnt/ext2/renamed.txt", 0, 0, 0, 0,
			  0, 0, 0, 0) == 0 ||
	    vfs_read_text(vfs, "/mnt/ext2/rename-target.txt", text,
			  sizeof(text)) != 0 ||
	    !str_eq(text, "created ok\n")) {
		return -1;
	}
	if (vfs_rename_path_buffer(vfs, "/mnt/ext2/rename-target.txt",
				   "/mnt/ext2/created.txt", 0) != 0) {
		return -1;
	}
	EXT2_STEP("bootstrap: ext2 step rename\n");
	if (vfs_symlink_path_buffer(vfs, created_symlink_target,
				    "/mnt/ext2/created-link") != 0 ||
	    vfs_readdir_has(vfs, "/mnt/ext2", "created-link") != 0 ||
	    (!ext2_fsck_mode &&
	     (vfs_read_text(vfs, "/mnt/ext2/created-link", text,
			    sizeof(text)) != 0 ||
	      !str_eq(text, "created ok\n"))) ||
	    vfs_unlink_path_buffer(vfs, "/mnt/ext2/created-link") != 0 ||
	    vfs_stat_path(vfs, "/mnt/ext2/created-link", 0, 0, 0, 0,
			  0, 0, 0, 0) == 0) {
		return -1;
	}
	EXT2_STEP("bootstrap: ext2 step symlink\n");
	if (vfs_truncate_path(vfs, "/mnt/ext2/created.txt", 0) != 0 ||
	    vfs_stat_path(vfs, "/mnt/ext2/created.txt", &hello_size, 0, 0,
			  &hello_type, 0, 0, 0, &hello_blocks) != 0 ||
	    hello_size != 0 ||
	    hello_type != BUNIX_VFS_TYPE_REGULAR ||
	    hello_blocks != 0) {
		return -1;
	}
	if (vfs_link_path_buffer(vfs, "/mnt/ext2/created.txt",
				 "/mnt/ext2/created-hard.txt") != 0 ||
	    vfs_stat_path(vfs, "/mnt/ext2/created.txt", 0, &created_ino,
			  &hello_nlink, 0, 0, 0, 0, 0) != 0 ||
	    vfs_stat_path(vfs, "/mnt/ext2/created-hard.txt", 0, &linked_ino,
			  &hard_nlink, 0, 0, 0, 0, 0) != 0 ||
	    created_ino == 0 ||
	    created_ino != linked_ino ||
	    hello_nlink != 2 ||
	    hard_nlink != 2 ||
	    vfs_readdir_has(vfs, "/mnt/ext2", "created-hard.txt") != 0) {
		return -1;
	}
	if (vfs_unlink_path_buffer(vfs, "/mnt/ext2/created.txt") != 0 ||
	    vfs_stat_path(vfs, "/mnt/ext2/created.txt", 0, 0, 0, 0,
			  0, 0, 0, 0) == 0 ||
	    vfs_readdir_has(vfs, "/mnt/ext2", "created.txt") == 0 ||
	    vfs_stat_path(vfs, "/mnt/ext2/created-hard.txt", 0, &linked_ino,
			  &hard_nlink, 0, 0, 0, 0, 0) != 0 ||
	    linked_ino != created_ino ||
	    hard_nlink != 1) {
		return -1;
	}
	if (vfs_unlink_path_buffer(vfs, "/mnt/ext2/created-hard.txt") != 0 ||
	    vfs_stat_path(vfs, "/mnt/ext2/created-hard.txt", 0, 0, 0, 0,
			  0, 0, 0, 0) == 0 ||
	    vfs_readdir_has(vfs, "/mnt/ext2", "created-hard.txt") == 0) {
		return -1;
	}
	EXT2_STEP("bootstrap: ext2 step link unlink\n");
	if (vfs_mkdir_path_buffer(vfs, "/mnt/ext2/newdir", 0750) != 0 ||
	    vfs_stat_path(vfs, "/mnt/ext2/newdir", 0, 0, &dir_nlink,
			  &dir_type, &dir_mode, 0, 0, 0) != 0 ||
	    dir_type != BUNIX_VFS_TYPE_DIRECTORY ||
	    dir_mode != 0750 ||
	    dir_nlink < 2 ||
	    vfs_readdir_has(vfs, "/mnt/ext2", "newdir") != 0) {
		return -1;
	}
	if (vfs_create_path_buffer(vfs, "/mnt/ext2/newdir/nested.txt",
				   0644) != 0 ||
	    vfs_write_text(vfs, "/mnt/ext2/newdir/nested.txt", 0,
			   "nested ok\n", 10) != 0 ||
	    vfs_read_text(vfs, "/mnt/ext2/newdir/nested.txt", text,
			  sizeof(text)) != 0 ||
	    !str_eq(text, "nested ok\n") ||
	    vfs_rmdir_path_buffer(vfs, "/mnt/ext2/newdir") == 0 ||
	    vfs_unlink_path_buffer(vfs, "/mnt/ext2/newdir/nested.txt") != 0 ||
	    vfs_rmdir_path_buffer(vfs, "/mnt/ext2/newdir") != 0 ||
	    vfs_stat_path(vfs, "/mnt/ext2/newdir", 0, 0, 0, 0,
			  0, 0, 0, 0) == 0 ||
	    vfs_readdir_has(vfs, "/mnt/ext2", "newdir") == 0) {
		return -1;
	}
	EXT2_STEP("bootstrap: ext2 step mkdir rmdir\n");
	if (!ext2_fsck_mode) {
		if (vfs_mkdir_path_buffer(vfs, "/mnt/ext2/rename-dir-a",
					  0755) != 0 ||
		    vfs_mkdir_path_buffer(vfs, "/mnt/ext2/rename-dir-b",
					  0755) != 0 ||
		    vfs_mkdir_path_buffer(vfs, "/mnt/ext2/rename-dir-a/child",
					  0755) != 0 ||
		    vfs_mkdir_path_buffer(vfs, "/mnt/ext2/rename-dir-b/target",
					  0755) != 0 ||
		    vfs_rename_path_buffer(vfs,
					   "/mnt/ext2/rename-dir-a/child",
					   "/mnt/ext2/rename-dir-b/target",
					   0) != 0 ||
		    vfs_stat_path(vfs, "/mnt/ext2/rename-dir-a/child", 0, 0,
				  0, 0, 0, 0, 0, 0) == 0 ||
		    vfs_stat_path(vfs, "/mnt/ext2/rename-dir-b/target", 0, 0,
				  0, &dir_type, 0, 0, 0, 0) != 0 ||
		    dir_type != BUNIX_VFS_TYPE_DIRECTORY ||
		    vfs_rmdir_path_buffer(vfs, "/mnt/ext2/rename-dir-a") != 0 ||
		    vfs_rmdir_path_buffer(vfs,
					  "/mnt/ext2/rename-dir-b/target") != 0 ||
		    vfs_rmdir_path_buffer(vfs, "/mnt/ext2/rename-dir-b") != 0) {
			return -1;
		}
		EXT2_STEP("bootstrap: ext2 step dir rename\n");
	}
#undef EXT2_STEP
	return 0;
}

static long register_proc_execs(u64 proc, u64 vfs)
{
	char *text;
	u64 pos = 0;
	u64 count = 0;
	long result = -1;

	text = (char *)bunix_alloc(BOOT_CONFIG_MAX);
	if (text == 0) {
		return -1;
	}
	if (vfs_read_text(vfs, "/etc/execs", text, BOOT_CONFIG_MAX) != 0) {
		result = 0;
		goto out;
	}
	while (text[pos] != '\0') {
		char path[BOOT_TOKEN_MAX];
		char task_name[BOOT_NAME_MAX];
		char linux_text[BOOT_NAME_MAX];
		u64 linux;

		while (is_space(text[pos])) {
			pos++;
		}
		if (text[pos] == '#') {
			skip_line(text, &pos);
			continue;
		}
		if (text[pos] == '\0') {
			break;
		}
		if (read_token(text, &pos, path, sizeof(path)) != 0 ||
		    read_token(text, &pos, task_name, sizeof(task_name)) != 0 ||
		    read_token(text, &pos, linux_text, sizeof(linux_text)) != 0) {
			goto out;
		}
		if (linux_text[0] == '0' && linux_text[1] == '\0') {
			linux = 0;
		} else if (linux_text[0] == '1' && linux_text[1] == '\0') {
			linux = 1;
		} else {
			goto out;
		}
		if (proc_register_exec(proc, path, task_name, linux) != 0) {
			goto out;
		}
		count++;
		skip_line(text, &pos);
	}
	result = count != 0 ? 0 : -1;
out:
	bunix_free(text);
	return result;
}

static void log_path_line(const char *prefix, const char *path)
{
	char line[160];
	u64 cursor = 0;

	for (u64 i = 0; prefix[i] != '\0' && cursor < sizeof(line); i++) {
		line[cursor++] = prefix[i];
	}
	if (path != 0) {
		for (u64 i = 0; path[i] != '\0' && cursor + 1 < sizeof(line);
		     i++) {
			line[cursor++] = path[i];
		}
	}
	if (cursor < sizeof(line)) {
		line[cursor++] = '\n';
	}
	bunix_console_log(line, cursor);
}

static long proc_spawn_wait_args(u64 proc, const char *path,
				 const char **args, u64 arg_count,
				 const char **envs, u64 env_count);

static long proc_spawn_wait(u64 proc, const char *path)
{
	return proc_spawn_wait_args(proc, path, 0, 0, 0, 0);
}

static int token_is_env(const char *token)
{
	const char prefix[] = "env:";

	for (u64 i = 0; i < sizeof(prefix) - 1; i++) {
		if (token[i] != prefix[i]) {
			return 0;
		}
	}
	return 1;
}

static long proc_spawn_wait_args(u64 proc, const char *path,
				 const char **args, u64 arg_count,
				 const char **envs, u64 env_count)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_PROC,
		.type = BUNIX_PROC_SPAWN_BUFFER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_RECV | BUNIX_RIGHT_DUP,
		.reply = 0,
		.cap = 0,
		.words = { 0, 1 + arg_count, env_count, 0 },
	};
	struct bunix_msg reply;
	u64 total = str_len(path) + 1 + str_len(path) + 1;
	u64 offset = 0;
	long buffer;

	for (u64 i = 0; i < arg_count; i++) {
		total += str_len(args[i]) + 1;
	}
	for (u64 i = 0; i < env_count; i++) {
		total += str_len(envs[i]) + 1;
	}
	buffer = bunix_buffer_create(total);
	if (proc == 0 || path == 0 || buffer <= 0) {
		if (buffer > 0) {
			bunix_handle_close((u64)buffer);
		}
		return -1;
	}
	request.cap = (u64)buffer;
	request.words[0] = total;
	if (bunix_buffer_write((u64)buffer, offset, path,
			       str_len(path) + 1) != 0) {
		bunix_handle_close((u64)buffer);
		return -1;
	}
	offset += str_len(path) + 1;
	if (bunix_buffer_write((u64)buffer, offset, path,
			       str_len(path) + 1) != 0) {
		bunix_handle_close((u64)buffer);
		return -1;
	}
	offset += str_len(path) + 1;
	for (u64 i = 0; i < arg_count; i++) {
		const u64 len = str_len(args[i]) + 1;

		if (bunix_buffer_write((u64)buffer, offset, args[i],
				       len) != 0) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
		offset += len;
	}
	for (u64 i = 0; i < env_count; i++) {
		const u64 len = str_len(envs[i]) + 1;

		if (bunix_buffer_write((u64)buffer, offset, envs[i],
				       len) != 0) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
		offset += len;
	}
	if (bunix_ipc_call(proc, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		bunix_handle_close((u64)buffer);
		return -1;
	}
	bunix_handle_close((u64)buffer);
	log_path_line("bootstrap: spawned ", path);
	request.type = BUNIX_PROC_WAIT;
	request.cap = 0;
	request.cap_rights = 0;
	request.words[0] = reply.words[1];
	request.words[1] = 0;
	request.words[2] = 0;
	request.words[3] = 0;
	if (bunix_ipc_call(proc, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return -1;
	}
	log_path_line("bootstrap: exited ", path);
	return 0;
}

static long run_boot_spawns(u64 proc, u64 vfs)
{
	char *text;
	u64 pos = 0;
	u64 count = 0;
	long final = -1;

	text = (char *)bunix_alloc(BOOT_CONFIG_MAX);
	if (text == 0) {
		return -1;
	}
	if (vfs_read_text(vfs, "/etc/spawns", text, BOOT_CONFIG_MAX) != 0) {
		final = 0;
		goto out;
	}
	while (text[pos] != '\0') {
		char path[BOOT_TOKEN_MAX];
		struct boot_spawn_args spawn = { 0, 0, 0, 0, 0, 0 };
		long result;

		while (is_space(text[pos])) {
			pos++;
		}
		if (text[pos] == '#') {
			skip_line(text, &pos);
			continue;
		}
		if (text[pos] == '\0') {
			break;
		}
		if (read_token(text, &pos, path, sizeof(path)) != 0) {
			goto out;
		}
		while (text[pos] != '\0' && text[pos] != '\n') {
			char token[BOOT_TOKEN_MAX];

			if (text[pos] == '#') {
				break;
			}
			if (is_space(text[pos])) {
				pos++;
				continue;
			}
			if (read_token(text, &pos, token, sizeof(token)) != 0) {
				boot_spawn_args_free(&spawn);
				goto out;
			}
			if (token_is_env(token)) {
				if (boot_spawn_push(&spawn.envs,
						    &spawn.env_count,
						    &spawn.env_cap,
						    token + 4) != 0) {
					boot_spawn_args_free(&spawn);
					goto out;
				}
			} else {
				if (boot_spawn_push(&spawn.args,
						    &spawn.arg_count,
						    &spawn.arg_cap,
						    token) != 0) {
					boot_spawn_args_free(&spawn);
					goto out;
				}
			}
		}
		if (spawn.arg_count == 0 && spawn.env_count == 0) {
			result = proc_spawn_wait(proc, path);
		} else {
			result = proc_spawn_wait_args(proc, path,
						     (const char **)spawn.args,
						     spawn.arg_count,
						     (const char **)spawn.envs,
						     spawn.env_count);
		}
		boot_spawn_args_free(&spawn);
		if (result != 0) {
			goto out;
		}
		count++;
		skip_line(text, &pos);
	}
	final = count != 0 ? 0 : -1;
out:
	bunix_free(text);
	return final;
}

static long send_path_command(u64 service,
			      unsigned int protocol,
			      unsigned int type,
			      const char *path)
{
	struct bunix_msg reply;

	if (bunix_ipc_call_path(service, protocol, type, path, 0, 0, 0,
				&reply) != 0) {
		return -1;
	}
	return reply.words[0] == 0 ? 0 : -1;
}

static long send_path_commands(u64 service,
			       const struct boot_path_command *command)
{
	for (u64 i = 0; i < command->path_count; i++) {
		if (send_path_command(service, command->protocol, command->type,
				      command->paths[i].path) != 0) {
			return -1;
		}
	}
	return 0;
}

static long mount_tmpfs_roots(u64 tmpfs)
{
	const struct boot_path roots[] = {
		{ "/.upper" },
		{ "/tmp" },
		{ "/run" },
		{ "/var/tmp" },
	};
	const struct boot_path_command command = {
		BUNIX_PROTO_TMPFS,
		BUNIX_TMPFS_MOUNT_ROOT,
		roots,
		sizeof(roots) / sizeof(roots[0]),
	};

	return send_path_commands(tmpfs, &command);
}

static long mount_utmpfs_paths(u64 utmpfs)
{
	const struct boot_path paths[] = {
		{ "/run/utmp" },
		{ "/var/run/utmp" },
	};
	const struct boot_path_command command = {
		BUNIX_PROTO_UTMPFS,
		BUNIX_UTMPFS_MOUNT_PATH,
		paths,
		sizeof(paths) / sizeof(paths[0]),
	};

	return send_path_commands(utmpfs, &command);
}

static long vfs_mount_service(u64 vfs, const char *path, u64 service)
{
	struct bunix_msg reply;

	if (bunix_ipc_call_path(vfs, BUNIX_PROTO_VFS,
				BUNIX_VFS_MOUNT_BUFFER, path,
				service, 0, 0, &reply) != 0) {
		return -1;
	}
	return reply.words[0] == 0 ? 0 : -1;
}

static long unionfs_set_upper(u64 unionfs, const char *path)
{
	return send_path_command(unionfs, BUNIX_PROTO_UNIONFS,
				 BUNIX_UNIONFS_SET_UPPER, path);
}

static long unionfs_set_lower(u64 unionfs, const char *path)
{
	return send_path_command(unionfs, BUNIX_PROTO_UNIONFS,
				 BUNIX_UNIONFS_SET_LOWER, path);
}

static void sleep_ns(u64 time, u64 ns)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_TIME,
		.type = BUNIX_TIME_SLEEP_NS,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { ns, 0, 0, 0 },
	};
	struct bunix_msg reply;

	if (time != 0) {
		(void)bunix_ipc_call(time, &request, &reply);
	}
}

static long spawn_linux_init(u64 linux, const char *path)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_LINUX,
		.type = BUNIX_LINUX_EXEC_INIT,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_RECV | BUNIX_RIGHT_DUP,
		.reply = 0,
		.cap = 0,
		.words = { 0, 0, 0, 0 },
	};
	struct bunix_msg reply;
	const u64 len = path == 0 ? 0 : str_len(path) + 1;
	const long buffer = len == 0 ? -1 : bunix_buffer_create(len);

	if (linux == 0 || buffer <= 0) {
		return -1;
	}
	request.cap = (u64)buffer;
	request.words[0] = len;
	if (bunix_buffer_write((u64)buffer, 0, path, len) != 0 ||
	    bunix_ipc_call(linux, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		bunix_handle_close((u64)buffer);
		return -1;
	}
	bunix_handle_close((u64)buffer);
	return 0;
}

static long net_loopback_selftest(u64 net)
{
	const char payload[] = "bunix-loopback";
	char out[sizeof(payload)];
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_NET,
		.type = BUNIX_NET_LOOPBACK_SEND,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_RECV,
		.reply = 0,
		.cap = 0,
		.words = {
			1,
			BUNIX_NET_ADDR_FAMILY_IPV4,
			sizeof(payload),
			0,
		},
	};
	struct bunix_msg reply;
	const long buffer = bunix_buffer_create(sizeof(payload));

	if (net == 0 || buffer <= 0) {
		return -1;
	}
	if (bunix_buffer_write((u64)buffer, 0, payload, sizeof(payload)) != 0) {
		bunix_handle_close((u64)buffer);
		return -1;
	}
	request.cap = (u64)buffer;
	if (bunix_ipc_call(net, &request, &reply) != 0 ||
	    reply.words[0] != 0 || reply.words[1] != sizeof(payload)) {
		bunix_handle_close((u64)buffer);
		return -1;
	}

	request.type = BUNIX_NET_LOOPBACK_RECV;
	request.cap_rights = BUNIX_RIGHT_SEND;
	request.words[0] = 1;
	request.words[1] = sizeof(out);
	request.words[2] = 0;
	request.words[3] = 0;
	if (bunix_ipc_call(net, &request, &reply) != 0 ||
	    reply.words[0] != 0 ||
	    reply.words[1] != BUNIX_NET_ADDR_FAMILY_IPV4 ||
	    reply.words[2] != sizeof(payload) ||
	    bunix_buffer_read((u64)buffer, 0, out, sizeof(out)) != 0) {
		bunix_handle_close((u64)buffer);
		return -1;
	}
	bunix_handle_close((u64)buffer);
	for (u64 i = 0; i < sizeof(payload); i++) {
		if (out[i] != payload[i]) {
			return -1;
		}
	}
	return 0;
}

static long net_udp_selftest(u64 net, u64 family)
{
	const char payload[] = "bunix-udp";
	char out[sizeof(payload)];
	const u64 lo_hi = 0;
	const u64 lo_lo = family == BUNIX_NET_ADDR_FAMILY_IPV4 ?
			  0x7f000001 : 1;
	u64 server = 0;
	u64 client = 0;
	u64 server_port = 0;
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_NET,
		.type = BUNIX_NET_UDP_OPEN,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { family, 0, 0, 0 },
	};
	struct bunix_msg reply;
	const long buffer = bunix_buffer_create(sizeof(payload));

	if (net == 0 || buffer <= 0) {
		return -1;
	}
	if (bunix_ipc_call(net, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		bunix_handle_close((u64)buffer);
		return -1;
	}
	server = reply.words[1];
	if (bunix_ipc_call(net, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		bunix_handle_close((u64)buffer);
		return -1;
	}
	client = reply.words[1];

	request.type = BUNIX_NET_UDP_BIND;
	request.words[0] = server;
	request.words[1] = 0;
	request.words[2] = 0;
	request.words[3] = 0;
	if (bunix_ipc_call(net, &request, &reply) != 0 ||
	    reply.words[0] != 0 || reply.words[1] == 0) {
		bunix_handle_close((u64)buffer);
		return -1;
	}
	server_port = reply.words[1];

	request.type = BUNIX_NET_UDP_CONNECT;
	request.words[0] = client;
	request.words[1] = lo_hi;
	request.words[2] = lo_lo;
	request.words[3] = server_port;
	if (bunix_ipc_call(net, &request, &reply) != 0 ||
	    reply.words[0] != 0 || reply.words[1] == 0 ||
	    bunix_buffer_write((u64)buffer, 0, payload, sizeof(payload)) != 0) {
		bunix_handle_close((u64)buffer);
		return -1;
	}

	request.type = BUNIX_NET_UDP_SEND;
	request.cap = (u64)buffer;
	request.cap_rights = BUNIX_RIGHT_RECV;
	request.words[0] = client;
	request.words[1] = sizeof(payload);
	request.words[2] = 0;
	request.words[3] = 0;
	if (bunix_ipc_call(net, &request, &reply) != 0 ||
	    reply.words[0] != 0 || reply.words[1] != sizeof(payload)) {
		bunix_handle_close((u64)buffer);
		return -1;
	}

	request.type = BUNIX_NET_UDP_POLL;
	request.cap = 0;
	request.cap_rights = 0;
	request.words[0] = server;
	request.words[1] = 0;
	if (bunix_ipc_call(net, &request, &reply) != 0 ||
	    reply.words[0] != 0 || (reply.words[1] & 1) == 0) {
		bunix_handle_close((u64)buffer);
		return -1;
	}

	request.type = BUNIX_NET_UDP_RECV;
	request.cap = (u64)buffer;
	request.cap_rights = BUNIX_RIGHT_SEND;
	request.words[0] = server;
	request.words[1] = sizeof(out);
	request.words[2] = 0;
	request.words[3] = 0;
	if (bunix_ipc_call(net, &request, &reply) != 0 ||
	    reply.words[0] != 0 || reply.words[1] != sizeof(payload) ||
	    reply.words[2] == 0 ||
	    reply.words[3] == 0 ||
	    bunix_buffer_read((u64)buffer, 0, out, sizeof(out)) != 0) {
		bunix_handle_close((u64)buffer);
		return -1;
	}
	for (u64 i = 0; i < sizeof(payload); i++) {
		if (out[i] != payload[i]) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
	}

	request.type = BUNIX_NET_UDP_CLOSE;
	request.cap = 0;
	request.cap_rights = 0;
	request.words[0] = client;
	(void)bunix_ipc_call(net, &request, &reply);
	request.words[0] = server;
	(void)bunix_ipc_call(net, &request, &reply);
	bunix_handle_close((u64)buffer);
	return 0;
}

static long net_tcp_call(u64 net, struct bunix_msg *request,
			 struct bunix_msg *reply)
{
	return bunix_ipc_call(net, request, reply) == 0 &&
	       reply->words[0] == 0 ? 0 : -1;
}

static long net_tcp_selftest(u64 net, u64 family)
{
	const char client_payload[] = "bunix-tcp-client";
	const char server_payload[] = "bunix-tcp-server";
	char out[sizeof(client_payload) > sizeof(server_payload) ?
		 sizeof(client_payload) : sizeof(server_payload)];
	const u64 lo_hi = 0;
	const u64 lo_lo = family == BUNIX_NET_ADDR_FAMILY_IPV4 ?
			  0x7f000001 : 1;
	u64 listener = 0;
	u64 client = 0;
	u64 accepted = 0;
	u64 listener_port = 0;
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_NET,
		.type = BUNIX_NET_TCP_OPEN,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { family, 0, 0, 0 },
	};
	struct bunix_msg reply;
	const long buffer = bunix_buffer_create(sizeof(out));

	if (net == 0 || buffer <= 0) {
		return -1;
	}
	if (net_tcp_call(net, &request, &reply) != 0) {
		bunix_handle_close((u64)buffer);
		return -1;
	}
	listener = reply.words[1];
	if (net_tcp_call(net, &request, &reply) != 0) {
		bunix_handle_close((u64)buffer);
		return -1;
	}
	client = reply.words[1];

	request.type = BUNIX_NET_TCP_BIND;
	request.words[0] = listener;
	request.words[1] = 0;
	request.words[2] = 0;
	request.words[3] = 0;
	if (net_tcp_call(net, &request, &reply) != 0 || reply.words[1] == 0) {
		bunix_handle_close((u64)buffer);
		return -1;
	}
	listener_port = reply.words[1];

	request.type = BUNIX_NET_TCP_LISTEN;
	request.words[0] = listener;
	request.words[1] = 4;
	request.words[2] = 0;
	request.words[3] = 0;
	if (net_tcp_call(net, &request, &reply) != 0 || reply.words[1] == 0) {
		bunix_handle_close((u64)buffer);
		return -1;
	}

	request.type = BUNIX_NET_TCP_CONNECT;
	request.words[0] = client;
	request.words[1] = lo_hi;
	request.words[2] = lo_lo;
	request.words[3] = listener_port;
	if (net_tcp_call(net, &request, &reply) != 0 || reply.words[1] == 0) {
		bunix_handle_close((u64)buffer);
		return -1;
	}

	request.type = BUNIX_NET_TCP_POLL;
	request.words[0] = listener;
	if (net_tcp_call(net, &request, &reply) != 0 ||
	    (reply.words[1] & 1) == 0) {
		bunix_handle_close((u64)buffer);
		return -1;
	}

	request.type = BUNIX_NET_TCP_ACCEPT;
	request.words[0] = listener;
	if (net_tcp_call(net, &request, &reply) != 0 || reply.words[1] == 0) {
		bunix_handle_close((u64)buffer);
		return -1;
	}
	accepted = reply.words[1];

	if (bunix_buffer_write((u64)buffer, 0, client_payload,
			       sizeof(client_payload)) != 0) {
		bunix_handle_close((u64)buffer);
		return -1;
	}
	request.type = BUNIX_NET_TCP_WRITE;
	request.cap = (u64)buffer;
	request.cap_rights = BUNIX_RIGHT_RECV;
	request.words[0] = client;
	request.words[1] = sizeof(client_payload);
	request.words[2] = 0;
	request.words[3] = 0;
	if (net_tcp_call(net, &request, &reply) != 0 ||
	    reply.words[1] != sizeof(client_payload)) {
		bunix_handle_close((u64)buffer);
		return -1;
	}

	request.type = BUNIX_NET_TCP_POLL;
	request.cap = 0;
	request.cap_rights = 0;
	request.words[0] = accepted;
	if (net_tcp_call(net, &request, &reply) != 0 ||
	    (reply.words[1] & 1) == 0) {
		bunix_handle_close((u64)buffer);
		return -1;
	}
	request.type = BUNIX_NET_TCP_READ;
	request.cap = (u64)buffer;
	request.cap_rights = BUNIX_RIGHT_SEND;
	request.words[0] = accepted;
	request.words[1] = sizeof(out);
	if (net_tcp_call(net, &request, &reply) != 0 ||
	    reply.words[1] != sizeof(client_payload) ||
	    bunix_buffer_read((u64)buffer, 0, out,
			      sizeof(client_payload)) != 0) {
		bunix_handle_close((u64)buffer);
		return -1;
	}
	for (u64 i = 0; i < sizeof(client_payload); i++) {
		if (out[i] != client_payload[i]) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
	}

	if (bunix_buffer_write((u64)buffer, 0, server_payload,
			       sizeof(server_payload)) != 0) {
		bunix_handle_close((u64)buffer);
		return -1;
	}
	request.type = BUNIX_NET_TCP_WRITE;
	request.cap = (u64)buffer;
	request.cap_rights = BUNIX_RIGHT_RECV;
	request.words[0] = accepted;
	request.words[1] = sizeof(server_payload);
	if (net_tcp_call(net, &request, &reply) != 0 ||
	    reply.words[1] != sizeof(server_payload)) {
		bunix_handle_close((u64)buffer);
		return -1;
	}
	request.type = BUNIX_NET_TCP_READ;
	request.cap_rights = BUNIX_RIGHT_SEND;
	request.words[0] = client;
	request.words[1] = sizeof(out);
	if (net_tcp_call(net, &request, &reply) != 0 ||
	    reply.words[1] != sizeof(server_payload) ||
	    bunix_buffer_read((u64)buffer, 0, out,
			      sizeof(server_payload)) != 0) {
		bunix_handle_close((u64)buffer);
		return -1;
	}
	for (u64 i = 0; i < sizeof(server_payload); i++) {
		if (out[i] != server_payload[i]) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
	}

	request.type = BUNIX_NET_TCP_SHUTDOWN;
	request.cap = 0;
	request.cap_rights = 0;
	request.words[0] = client;
	request.words[1] = 2;
	if (net_tcp_call(net, &request, &reply) != 0) {
		bunix_handle_close((u64)buffer);
		return -1;
	}
	request.type = BUNIX_NET_TCP_READ;
	request.cap = (u64)buffer;
	request.cap_rights = BUNIX_RIGHT_SEND;
	request.words[0] = accepted;
	request.words[1] = sizeof(out);
	if (net_tcp_call(net, &request, &reply) != 0 ||
	    reply.words[1] != 0 || reply.words[2] == 0) {
		bunix_handle_close((u64)buffer);
		return -1;
	}

	request.type = BUNIX_NET_TCP_CLOSE;
	request.cap = 0;
	request.cap_rights = 0;
	request.words[0] = client;
	(void)bunix_ipc_call(net, &request, &reply);
	request.words[0] = accepted;
	(void)bunix_ipc_call(net, &request, &reply);
	request.words[0] = listener;
	(void)bunix_ipc_call(net, &request, &reply);

	request.type = BUNIX_NET_TCP_OPEN;
	request.words[0] = family;
	request.words[1] = 0;
	request.words[2] = 0;
	request.words[3] = 0;
	if (net_tcp_call(net, &request, &reply) != 0) {
		bunix_handle_close((u64)buffer);
		return -1;
	}
	client = reply.words[1];
	request.type = BUNIX_NET_TCP_CONNECT;
	request.words[0] = client;
	request.words[1] = lo_hi;
	request.words[2] = lo_lo;
	request.words[3] = listener_port;
	if (bunix_ipc_call(net, &request, &reply) != 0 ||
	    reply.words[0] == 0) {
		bunix_handle_close((u64)buffer);
		return -1;
	}
	request.type = BUNIX_NET_TCP_CLOSE;
	request.words[0] = client;
	(void)bunix_ipc_call(net, &request, &reply);
	bunix_handle_close((u64)buffer);
	return 0;
}

static long net_packet_interface_selftest(u64 net)
{
	struct bunix_net_packet_interface_info iface = {
		.id = 0,
		.flags = BUNIX_NET_IFACE_FLAG_BROADCAST,
		.mtu = 1500,
		.mac_hi = 0x020000000000ull,
		.mac_lo = 0x000000001801ull,
		.rx_packets = 0,
		.tx_packets = 0,
		.rx_drops = 0,
		.tx_drops = 0,
	};
	struct {
		struct bunix_net_packet_info info;
		unsigned char frame[64];
	} packet;
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_NET,
		.type = BUNIX_NET_PACKET_INTERFACE_ATTACH,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_SEND | BUNIX_RIGHT_RECV,
		.reply = 0,
		.cap = 0,
		.words = { 0, 0, 0, 0 },
	};
	struct bunix_msg reply;
	long buffer;
	u64 iface_id;

	if (net == 0) {
		return -1;
	}
	buffer = bunix_buffer_create(sizeof(packet));
	if (buffer <= 0) {
		return -1;
	}
	if (bunix_buffer_write((u64)buffer, 0, &iface, sizeof(iface)) != 0) {
		bunix_handle_close((u64)buffer);
		return -1;
	}
	request.cap = (u64)buffer;
	if (bunix_ipc_call(net, &request, &reply) != 0 ||
	    reply.words[0] != 0 || reply.words[1] < 2 ||
	    bunix_buffer_read((u64)buffer, 0, &iface, sizeof(iface)) != 0 ||
	    iface.id != reply.words[1]) {
		bunix_handle_close((u64)buffer);
		return -1;
	}
	iface_id = reply.words[1];

	request.type = BUNIX_NET_PACKET_INTERFACE_LINK;
	request.cap = 0;
	request.cap_rights = 0;
	request.words[0] = iface_id;
	request.words[1] = BUNIX_NET_IFACE_FLAG_UP |
			   BUNIX_NET_IFACE_FLAG_RUNNING |
			   BUNIX_NET_IFACE_FLAG_BROADCAST;
	if (bunix_ipc_call(net, &request, &reply) != 0 ||
	    reply.words[0] != 0 ||
	    (reply.words[2] & BUNIX_NET_IFACE_FLAG_RUNNING) == 0) {
		bunix_handle_close((u64)buffer);
		return -1;
	}

	packet.info.iface = iface_id;
	packet.info.len = sizeof(packet.frame);
	packet.info.flags = 0;
	packet.info.reserved = 0;
	for (u64 i = 0; i < sizeof(packet.frame); i++) {
		packet.frame[i] = (unsigned char)i;
	}
	if (bunix_buffer_write((u64)buffer, 0, &packet, sizeof(packet)) != 0) {
		bunix_handle_close((u64)buffer);
		return -1;
	}
	request.type = BUNIX_NET_PACKET_RX_SUBMIT;
	request.cap = (u64)buffer;
	request.cap_rights = BUNIX_RIGHT_RECV;
	request.words[0] = iface_id;
	request.words[1] = sizeof(packet.frame);
	if (bunix_ipc_call(net, &request, &reply) != 0 ||
	    reply.words[0] != 0 || reply.words[2] != sizeof(packet.frame)) {
		bunix_handle_close((u64)buffer);
		return -1;
	}

	request.type = BUNIX_NET_INTERFACE_COUNT;
	request.cap = 0;
	request.cap_rights = 0;
	request.words[0] = 0;
	request.words[1] = 0;
	if (bunix_ipc_call(net, &request, &reply) != 0 ||
	    reply.words[0] != 0 || reply.words[1] < 2) {
		bunix_handle_close((u64)buffer);
		return -1;
	}
	request.type = BUNIX_NET_INTERFACE_STATS;
	request.words[0] = iface_id;
	if (bunix_ipc_call(net, &request, &reply) != 0 ||
	    reply.words[0] != 0 || reply.words[1] != 1) {
		bunix_handle_close((u64)buffer);
		return -1;
	}
	request.type = BUNIX_NET_INTERFACE_AT;
	request.words[0] = 1;
	if (bunix_ipc_call(net, &request, &reply) != 0 ||
	    reply.words[0] != 0 || reply.words[1] != iface_id) {
		bunix_handle_close((u64)buffer);
		return -1;
	}
	{
		struct bunix_net_addr_info addr = {
			.family = BUNIX_NET_ADDR_FAMILY_IPV4,
			.addr_hi = 0,
			.addr_lo = 0x0a120001ull,
			.prefix_len = 16,
			.iface = iface_id,
			.flags = 1,
			.preferred_lifetime = 0,
			.valid_lifetime = 0,
		};
		struct bunix_net_config_status status;
		u64 addr_count;
		int found = 0;

		if (bunix_buffer_write((u64)buffer, 0, &addr,
				       sizeof(addr)) != 0) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
		request.type = BUNIX_NET_ADDR_ADD;
		request.cap = (u64)buffer;
		request.cap_rights = BUNIX_RIGHT_RECV;
		request.words[0] = 0;
		if (bunix_ipc_call(net, &request, &reply) != 0 ||
		    reply.words[0] != 0) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
		request.type = BUNIX_NET_ADDR_COUNT;
		request.cap = 0;
		request.cap_rights = 0;
		if (bunix_ipc_call(net, &request, &reply) != 0 ||
		    reply.words[0] != 0 || reply.words[1] < 3) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
		addr_count = reply.words[1];
		for (u64 i = 0; i < addr_count; i++) {
			request.type = BUNIX_NET_ADDR_AT;
			request.cap = (u64)buffer;
			request.cap_rights = BUNIX_RIGHT_SEND;
			request.words[0] = i;
			if (bunix_ipc_call(net, &request, &reply) != 0 ||
			    reply.words[0] != 0 ||
			    bunix_buffer_read((u64)buffer, 0, &addr,
					      sizeof(addr)) != 0) {
				bunix_handle_close((u64)buffer);
				return -1;
			}
			if (addr.iface == iface_id &&
			    addr.family == BUNIX_NET_ADDR_FAMILY_IPV4 &&
			    addr.addr_lo == 0x0a120001ull &&
			    addr.prefix_len == 16) {
				found = 1;
			}
		}
		if (!found) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
		request.type = BUNIX_NET_CONFIG_STATUS;
		request.cap = (u64)buffer;
		request.cap_rights = BUNIX_RIGHT_SEND;
		request.words[0] = 0;
		if (bunix_ipc_call(net, &request, &reply) != 0 ||
		    reply.words[0] != 0 ||
		    bunix_buffer_read((u64)buffer, 0, &status,
				      sizeof(status)) != 0 ||
		    (status.flags & BUNIX_NET_CONFIG_LOOPBACK) == 0 ||
		    status.address_count < 3) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
	}
	{
		struct bunix_net_route_info route = {
			.family = BUNIX_NET_ADDR_FAMILY_IPV4,
			.prefix_hi = 0,
			.prefix_lo = 0x0a120000ull,
			.prefix_len = 16,
			.iface = iface_id,
			.gateway_hi = 0,
			.gateway_lo = 0,
			.flags = 1,
			.metric = 25,
		};
		u64 route_count;
		int found = 0;

		if (bunix_buffer_write((u64)buffer, 0, &route,
				       sizeof(route)) != 0) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
		request.type = BUNIX_NET_ROUTE_ADD;
		request.cap = (u64)buffer;
		request.cap_rights = BUNIX_RIGHT_RECV;
		request.words[0] = 0;
		if (bunix_ipc_call(net, &request, &reply) != 0 ||
		    reply.words[0] != 0) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
		request.type = BUNIX_NET_ROUTE_COUNT;
		request.cap = 0;
		request.cap_rights = 0;
		if (bunix_ipc_call(net, &request, &reply) != 0 ||
		    reply.words[0] != 0 || reply.words[1] < 3) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
		route_count = reply.words[1];
		for (u64 i = 0; i < route_count; i++) {
			request.type = BUNIX_NET_ROUTE_AT;
			request.cap = (u64)buffer;
			request.cap_rights = BUNIX_RIGHT_SEND;
			request.words[0] = i;
			if (bunix_ipc_call(net, &request, &reply) != 0 ||
			    reply.words[0] != 0 ||
			    bunix_buffer_read((u64)buffer, 0, &route,
					      sizeof(route)) != 0) {
				bunix_handle_close((u64)buffer);
				return -1;
			}
			if (route.iface == iface_id &&
			    route.family == BUNIX_NET_ADDR_FAMILY_IPV4 &&
			    route.prefix_lo == 0x0a120000ull &&
			    route.prefix_len == 16) {
				found = 1;
			}
		}
		if (!found) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
	}
	packet.info.iface = iface_id;
	packet.info.len = 42;
	packet.info.flags = 0;
	packet.info.reserved = 0;
	for (u64 i = 0; i < sizeof(packet.frame); i++) {
		packet.frame[i] = 0;
	}
	for (u64 i = 0; i < 6; i++) {
		packet.frame[i] = 0xff;
	}
	packet.frame[6] = 0x02;
	packet.frame[10] = 0x18;
	packet.frame[11] = 0x02;
	packet.frame[12] = 0x08;
	packet.frame[13] = 0x06;
	packet.frame[15] = 0x01;
	packet.frame[16] = 0x08;
	packet.frame[18] = 0x06;
	packet.frame[19] = 0x04;
	packet.frame[21] = 0x01;
	packet.frame[22] = 0x02;
	packet.frame[26] = 0x18;
	packet.frame[27] = 0x02;
	packet.frame[28] = 0x0a;
	packet.frame[29] = 0x12;
	packet.frame[31] = 0x02;
	packet.frame[38] = 0x0a;
	packet.frame[39] = 0x12;
	packet.frame[41] = 0x01;
	if (bunix_buffer_write((u64)buffer, 0, &packet, sizeof(packet)) != 0) {
		bunix_handle_close((u64)buffer);
		return -1;
	}
	request.type = BUNIX_NET_PACKET_RX_SUBMIT;
	request.cap = (u64)buffer;
	request.cap_rights = BUNIX_RIGHT_RECV;
	request.words[0] = iface_id;
	request.words[1] = packet.info.len;
	if (bunix_ipc_call(net, &request, &reply) != 0 ||
	    reply.words[0] != 0 || reply.words[2] != packet.info.len) {
		bunix_handle_close((u64)buffer);
		return -1;
	}
	request.type = BUNIX_NET_PACKET_TX_DEQUEUE;
	request.cap = (u64)buffer;
	request.cap_rights = BUNIX_RIGHT_SEND;
	request.words[0] = iface_id;
	request.words[1] = sizeof(packet.frame);
	if (bunix_ipc_call(net, &request, &reply) != 0 ||
	    reply.words[0] != 0 || reply.words[2] != 42 ||
	    bunix_buffer_read((u64)buffer, 0, &packet, sizeof(packet)) != 0 ||
	    packet.info.iface != iface_id || packet.info.len != 42 ||
	    packet.frame[0] != 0x02 || packet.frame[5] != 0x02 ||
	    packet.frame[6] != 0x02 || packet.frame[12] != 0x08 ||
	    packet.frame[13] != 0x06 || packet.frame[20] != 0x00 ||
	    packet.frame[21] != 0x02 || packet.frame[28] != 0x0a ||
	    packet.frame[29] != 0x12 || packet.frame[31] != 0x01 ||
	    packet.frame[32] != 0x02 || packet.frame[37] != 0x02 ||
	    packet.frame[38] != 0x0a || packet.frame[39] != 0x12 ||
	    packet.frame[41] != 0x02) {
		bunix_handle_close((u64)buffer);
		return -1;
	}
	{
		struct bunix_net_neighbor_info neighbor = {
			.family = BUNIX_NET_ADDR_FAMILY_IPV4,
			.addr_hi = 0,
			.addr_lo = 0x0a120002ull,
			.iface = iface_id,
			.mac_hi = 0x020000001802ull,
			.mac_lo = 0,
			.flags = 2,
			.state = 1,
		};
		u64 neighbor_count;
		int found = 0;

		if (bunix_buffer_write((u64)buffer, 0, &neighbor,
				       sizeof(neighbor)) != 0) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
		request.type = BUNIX_NET_NEIGHBOR_ADD;
		request.cap = (u64)buffer;
		request.cap_rights = BUNIX_RIGHT_RECV;
		request.words[0] = 0;
		if (bunix_ipc_call(net, &request, &reply) != 0 ||
		    reply.words[0] != 0) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
		request.type = BUNIX_NET_NEIGHBOR_COUNT;
		request.cap = 0;
		request.cap_rights = 0;
		if (bunix_ipc_call(net, &request, &reply) != 0 ||
		    reply.words[0] != 0 || reply.words[1] == 0) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
		neighbor_count = reply.words[1];
		for (u64 i = 0; i < neighbor_count; i++) {
			request.type = BUNIX_NET_NEIGHBOR_AT;
			request.cap = (u64)buffer;
			request.cap_rights = BUNIX_RIGHT_SEND;
			request.words[0] = i;
			if (bunix_ipc_call(net, &request, &reply) != 0 ||
			    reply.words[0] != 0 ||
			    bunix_buffer_read((u64)buffer, 0, &neighbor,
					      sizeof(neighbor)) != 0) {
				bunix_handle_close((u64)buffer);
				return -1;
			}
			if (neighbor.iface == iface_id &&
			    neighbor.family == BUNIX_NET_ADDR_FAMILY_IPV4 &&
			    neighbor.addr_lo == 0x0a120002ull &&
			    neighbor.mac_hi == 0x020000001802ull) {
				found = 1;
			}
		}
		if (!found) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
		request.type = BUNIX_NET_NEIGHBOR_DELETE;
		request.cap = 0;
		request.cap_rights = 0;
		request.words[0] = BUNIX_NET_ADDR_FAMILY_IPV4;
		request.words[1] = 0;
		request.words[2] = 0x0a120002ull;
		request.words[3] = iface_id;
		if (bunix_ipc_call(net, &request, &reply) != 0 ||
		    reply.words[0] != 0) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
	}
	packet.info.iface = iface_id;
	packet.info.len = sizeof(packet.frame);
	packet.info.flags = 0;
	packet.info.reserved = 0;
	for (u64 i = 0; i < sizeof(packet.frame); i++) {
		packet.frame[i] = (unsigned char)(0x80 + i);
	}
	if (bunix_buffer_write((u64)buffer, 0, &packet, sizeof(packet)) != 0) {
		bunix_handle_close((u64)buffer);
		return -1;
	}
	request.type = BUNIX_NET_PACKET_TX_ENQUEUE;
	request.cap = (u64)buffer;
	request.cap_rights = BUNIX_RIGHT_RECV;
	request.words[0] = 0;
	if (bunix_ipc_call(net, &request, &reply) != 0 ||
	    reply.words[0] != 0 || reply.words[2] != sizeof(packet.frame)) {
		bunix_handle_close((u64)buffer);
		return -1;
	}
	request.type = BUNIX_NET_PACKET_TX_DEQUEUE;
	request.cap = (u64)buffer;
	request.cap_rights = BUNIX_RIGHT_SEND;
	request.words[0] = iface_id;
	request.words[1] = sizeof(packet.frame);
	if (bunix_ipc_call(net, &request, &reply) != 0 ||
	    reply.words[0] != 0 || reply.words[2] != sizeof(packet.frame) ||
	    bunix_buffer_read((u64)buffer, 0, &packet, sizeof(packet)) != 0 ||
	    packet.info.iface != iface_id ||
	    packet.info.len != sizeof(packet.frame) ||
	    packet.frame[0] != 0x80) {
		bunix_handle_close((u64)buffer);
		return -1;
	}
	request.type = BUNIX_NET_PACKET_TX_COMPLETE;
	request.cap = 0;
	request.cap_rights = 0;
	request.words[0] = iface_id;
	request.words[1] = sizeof(packet.frame);
	if (bunix_ipc_call(net, &request, &reply) != 0 ||
	    reply.words[0] != 0 || reply.words[2] != 1) {
		bunix_handle_close((u64)buffer);
		return -1;
	}
	request.type = BUNIX_NET_INTERFACE_STATS;
	request.words[0] = iface_id;
	if (bunix_ipc_call(net, &request, &reply) != 0 ||
	    reply.words[0] != 0 || reply.words[1] != 2 ||
	    reply.words[2] != 1) {
		bunix_handle_close((u64)buffer);
		return -1;
	}
	request.type = BUNIX_NET_PACKET_INTERFACE_LINK;
	request.cap = 0;
	request.cap_rights = 0;
	request.words[0] = iface_id;
	request.words[1] = 0;
	if (bunix_ipc_call(net, &request, &reply) != 0 ||
	    reply.words[0] != 0 ||
	    (reply.words[2] & BUNIX_NET_IFACE_FLAG_RUNNING) != 0) {
		bunix_handle_close((u64)buffer);
		return -1;
	}
	bunix_handle_close((u64)buffer);
	return 0;
}

int main(void)
{
	const char launching[] = "bootstrap: launching servers\n";
	const char attenuated[] = "bootstrap: bad cap denied\n";
	const char names_ready[] = "bootstrap: names ready\n";
	const char fs_namespace_ready[] = "bootstrap: fs namespace\n";
	const char fs_ready[] = "bootstrap: fs ready\n";
	const char virtio_blk_test[] = "bootstrap: virtio-blk test\n";
	const char net_loopback_ok[] = "bootstrap: net loopback ok\n";
	const char net_udp_ok[] = "bootstrap: net udp ok\n";
	const char net_tcp_ok[] = "bootstrap: net tcp ok\n";
	const char net_packet_ok[] = "bootstrap: net packet iface ok\n";
	const char net_route_ok[] = "bootstrap: net route ok\n";
	const char netcfg_ready[] = "bootstrap: netcfg ready\n";
	const char ext2_ok[] = "bootstrap: ext2 readonly ok\n";
	const char ext2_root_ok[] = "bootstrap: ext2 root ok\n";
	const char squashfs_root[] = "bootstrap: squashfs root\n";
	char file[17];
	char ext2_root_text[32];
	u64 console;
	u64 vm;
	u64 time = 0;
	u64 proc = 0;
	u64 linux = 0;
	u64 net = 0;
	u64 vfs = 0;
	u64 vfs_launch = 0;
	u64 fs_namespace = 0;
	const int ext2_root_mode = bunix_cmdline_has("ext2-root-test") > 0;
	const struct bunix_launch_cap bad_caps[] = {
		{ BUNIX_HANDLE_CONSOLE, BUNIX_RIGHT_SEND | BUNIX_RIGHT_RECV, 0 },
	};

	bunix_console_log(launching, sizeof(launching) - 1);
	register_service(BUNIX_SERVICE_VM, BUNIX_HANDLE_VM);
	console = resolve_service(BUNIX_SERVICE_CONSOLE,
				  BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP);
	vm = resolve_service(BUNIX_SERVICE_VM,
			     BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP);
	if (console == 0 || vm == 0) {
		return 1;
	}
	bunix_console_log(names_ready, sizeof(names_ready) - 1);

	const struct bunix_launch_cap fs_caps[] = {
		{ console, BUNIX_RIGHT_SEND, 0 },
		{ BUNIX_HANDLE_NAMES, BUNIX_RIGHT_SEND, 0 },
	};

	bunix_launch_module_with_caps("time", fs_caps,
				      sizeof(fs_caps) / sizeof(fs_caps[0]));
	time = wait_service_in_namespace(BUNIX_NAMES_ROOT, BUNIX_SERVICE_TIME,
					 BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP);
	if (time == 0) {
		return 1;
	}
	bunix_launch_module_with_caps("user", fs_caps,
				      sizeof(fs_caps) / sizeof(fs_caps[0]));
	if (wait_service_in_namespace(BUNIX_NAMES_ROOT, BUNIX_SERVICE_USER,
				      BUNIX_RIGHT_SEND) == 0) {
		return 1;
	}

	const struct bunix_launch_cap proc_caps[] = {
		{ console, BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP, 0 },
		{ BUNIX_HANDLE_NAMES, BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP, 0 },
		{ time, BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP, 0 },
	};
	bunix_launch_module_with_caps("proc", proc_caps,
				      sizeof(proc_caps) / sizeof(proc_caps[0]));
	proc = wait_service_in_namespace(BUNIX_NAMES_ROOT, BUNIX_SERVICE_PROC,
					 BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP);
	if (proc == 0) {
		return 1;
	}

	bunix_launch_module_with_caps("virtio-bus", fs_caps,
				      sizeof(fs_caps) / sizeof(fs_caps[0]));
	if (wait_service_in_namespace(BUNIX_NAMES_ROOT, BUNIX_SERVICE_DEVICE,
				      BUNIX_RIGHT_SEND) == 0) {
		return 1;
	}
	bunix_launch_module_with_caps("net", fs_caps,
				      sizeof(fs_caps) / sizeof(fs_caps[0]));
	net = wait_service_in_namespace(BUNIX_NAMES_ROOT, BUNIX_SERVICE_NET,
					BUNIX_RIGHT_SEND);
	if (net == 0 || net_loopback_selftest(net) != 0 ||
	    net_udp_selftest(net, BUNIX_NET_ADDR_FAMILY_IPV4) != 0 ||
	    net_udp_selftest(net, BUNIX_NET_ADDR_FAMILY_IPV6) != 0 ||
	    net_tcp_selftest(net, BUNIX_NET_ADDR_FAMILY_IPV4) != 0 ||
	    net_tcp_selftest(net, BUNIX_NET_ADDR_FAMILY_IPV6) != 0 ||
	    net_packet_interface_selftest(net) != 0) {
		return 1;
	}
	bunix_console_log(net_loopback_ok, sizeof(net_loopback_ok) - 1);
	bunix_console_log(net_udp_ok, sizeof(net_udp_ok) - 1);
	bunix_console_log(net_tcp_ok, sizeof(net_tcp_ok) - 1);
	bunix_console_log(net_packet_ok, sizeof(net_packet_ok) - 1);
	bunix_console_log(net_route_ok, sizeof(net_route_ok) - 1);
	if (bunix_cmdline_has("virtio-blk-block-test") <= 0) {
		bunix_launch_module_with_caps("block", fs_caps,
					      sizeof(fs_caps) /
						      sizeof(fs_caps[0]));
	}
	if (bunix_cmdline_has("virtio-blk-test") > 0) {
		bunix_console_log(virtio_blk_test,
				  sizeof(virtio_blk_test) - 1);
		bunix_launch_module_with_caps("virtio-blk", fs_caps,
					      sizeof(fs_caps) /
						      sizeof(fs_caps[0]));
	}
	if (bunix_cmdline_has("virtio-net-test") > 0) {
		bunix_launch_module_with_caps("virtio-net", fs_caps,
					      sizeof(fs_caps) /
						      sizeof(fs_caps[0]));
	}
	bunix_launch_module_with_caps("vfs", fs_caps,
				      sizeof(fs_caps) / sizeof(fs_caps[0]));
	vfs = wait_service_in_namespace(BUNIX_NAMES_ROOT, BUNIX_SERVICE_VFS,
					BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP);
	if (vfs == 0) {
		return 1;
	}
	vfs_launch = vfs;
	bunix_launch_module_with_caps("procfs", fs_caps,
				      sizeof(fs_caps) / sizeof(fs_caps[0]));
	{
		u64 procfs = wait_service_in_namespace(BUNIX_NAMES_ROOT,
						       BUNIX_SERVICE_PROCFS,
						       BUNIX_RIGHT_SEND);

		if (procfs == 0 ||
		    send_path_command(procfs, BUNIX_PROTO_PROCFS,
				      BUNIX_PROCFS_MOUNT_PATH, "/proc") != 0) {
			return 1;
		}
	}
	bunix_launch_module_with_caps("tmpfs", fs_caps,
				      sizeof(fs_caps) / sizeof(fs_caps[0]));
	{
		u64 tmpfs = wait_service_in_namespace(BUNIX_NAMES_ROOT,
						      BUNIX_SERVICE_TMPFS,
						      BUNIX_RIGHT_SEND);

		if (tmpfs == 0 || mount_tmpfs_roots(tmpfs) != 0) {
			return 1;
		}
	}
	bunix_launch_module_with_caps("devfs", fs_caps,
				      sizeof(fs_caps) / sizeof(fs_caps[0]));
	{
		u64 devfs = wait_service_in_namespace(BUNIX_NAMES_ROOT,
						      BUNIX_SERVICE_DEVFS,
						      BUNIX_RIGHT_SEND);

		if (devfs == 0 ||
		    send_path_command(devfs, BUNIX_PROTO_DEVFS,
				      BUNIX_DEVFS_MOUNT_PATH, "/dev") != 0) {
			return 1;
		}
	}
	bunix_launch_module_with_caps("sysfs", fs_caps,
				      sizeof(fs_caps) / sizeof(fs_caps[0]));
	{
		u64 sysfs = wait_service_in_namespace(BUNIX_NAMES_ROOT,
						      BUNIX_SERVICE_SYSFS,
						      BUNIX_RIGHT_SEND);

		if (sysfs == 0 ||
		    send_path_command(sysfs, BUNIX_PROTO_SYSFS,
				      BUNIX_SYSFS_MOUNT_PATH, "/sys") != 0) {
			return 1;
		}
	}
	bunix_launch_module_with_caps("utmpfs", fs_caps,
				      sizeof(fs_caps) / sizeof(fs_caps[0]));
	{
		u64 utmpfs = wait_service_in_namespace(BUNIX_NAMES_ROOT,
						       BUNIX_SERVICE_UTMPFS,
						       BUNIX_RIGHT_SEND);

		if (utmpfs == 0 || mount_utmpfs_paths(utmpfs) != 0) {
			return 1;
		}
	}
	if (bunix_cmdline_has("ext2-fsck-test") > 0) {
		u64 ext2;
		u64 tmpfs_root;

		if (wait_service_in_namespace(BUNIX_NAMES_ROOT,
					      BUNIX_SERVICE_BLOCK,
					      BUNIX_RIGHT_SEND) == 0) {
			return 1;
		}
		tmpfs_root = wait_service_in_namespace(BUNIX_NAMES_ROOT,
						       BUNIX_SERVICE_TMPFS,
						       BUNIX_RIGHT_SEND);
		if (tmpfs_root == 0 ||
		    send_path_command(tmpfs_root, BUNIX_PROTO_TMPFS,
				      BUNIX_TMPFS_MOUNT_ROOT, "/") != 0) {
			return 1;
		}
		bunix_launch_module_with_caps("ext2", fs_caps,
					      sizeof(fs_caps) /
						      sizeof(fs_caps[0]));
		ext2 = wait_service_in_namespace(BUNIX_NAMES_ROOT,
						  BUNIX_SERVICE_EXT2,
						  BUNIX_RIGHT_SEND);
		if (ext2 == 0 ||
		    vfs_mount_service(vfs, "/mnt/ext2",
				      BUNIX_SERVICE_EXT2) != 0 ||
		    ext2_readonly_selftest(vfs) != 0) {
			return 1;
		}
		bunix_console_log(ext2_ok, sizeof(ext2_ok) - 1);
		(void)bunix_machine_poweroff(0);
		for (;;) {
		}
	}
	if (ext2_root_mode) {
		u64 ext2;

		bunix_launch_module_with_caps("ext2", fs_caps,
					      sizeof(fs_caps) /
						      sizeof(fs_caps[0]));
		ext2 = wait_service_in_namespace(BUNIX_NAMES_ROOT,
						  BUNIX_SERVICE_EXT2,
						  BUNIX_RIGHT_SEND);
		if (ext2 == 0 ||
		    vfs_mount_service(vfs, "/", BUNIX_SERVICE_EXT2) != 0) {
			return 1;
		}
		if (vfs_read_text(vfs, "/hello.txt", ext2_root_text,
				  sizeof(ext2_root_text)) != 0 ||
		    !str_eq(ext2_root_text, "ext2 hello\n")) {
			return 1;
		}
		bunix_console_log(ext2_root_ok, sizeof(ext2_root_ok) - 1);
		(void)bunix_machine_poweroff(0);
		for (;;) {
		}
	} else {
		u64 squashfs;

		bunix_console_log(squashfs_root, sizeof(squashfs_root) - 1);
		bunix_launch_module_with_caps("squashfs", fs_caps,
					      sizeof(fs_caps) /
						      sizeof(fs_caps[0]));
		squashfs = wait_service_in_namespace(BUNIX_NAMES_ROOT,
						      BUNIX_SERVICE_SQUASHFS,
						      BUNIX_RIGHT_SEND);
		if (squashfs == 0 ||
		    send_path_command(squashfs, BUNIX_PROTO_SQUASHFS,
				      BUNIX_SQUASHFS_MOUNT_PATH,
				      "/.lower") != 0 ||
		    vfs_mount_service(vfs, "/.lower",
				      BUNIX_SERVICE_SQUASHFS) != 0) {
			return 1;
		}
	}
	bunix_launch_module_with_caps("unionfs", fs_caps,
				      sizeof(fs_caps) / sizeof(fs_caps[0]));
	{
		u64 unionfs = wait_service_in_namespace(BUNIX_NAMES_ROOT,
							BUNIX_SERVICE_UNIONFS,
							BUNIX_RIGHT_SEND);

		if (unionfs == 0 ||
		    unionfs_set_lower(unionfs, "/.lower") != 0 ||
		    unionfs_set_upper(unionfs, "/.upper") != 0 ||
		    send_path_command(unionfs, BUNIX_PROTO_UNIONFS,
				      BUNIX_UNIONFS_MOUNT_PATH, "/") != 0) {
			return 1;
		}
	}
	bunix_launch_module_with_caps("netcfg", fs_caps,
				      sizeof(fs_caps) / sizeof(fs_caps[0]));
	if (wait_service_in_namespace(BUNIX_NAMES_ROOT, BUNIX_SERVICE_NETCFG,
				      BUNIX_RIGHT_SEND) == 0) {
		return 1;
	}
	bunix_console_log(netcfg_ready, sizeof(netcfg_ready) - 1);
	if (bunix_cmdline_has("ext2-test") > 0 &&
	    bunix_cmdline_has("ext2-fsck-test") <= 0) {
		u64 ext2;

		if (bunix_cmdline_has("virtio-blk-block-test") > 0 &&
		    wait_service_in_namespace(BUNIX_NAMES_ROOT,
					      BUNIX_SERVICE_BLOCK,
					      BUNIX_RIGHT_SEND) == 0) {
			return 1;
		}
		bunix_launch_module_with_caps("ext2", fs_caps,
					      sizeof(fs_caps) /
						      sizeof(fs_caps[0]));
		ext2 = wait_service_in_namespace(BUNIX_NAMES_ROOT,
						  BUNIX_SERVICE_EXT2,
						  BUNIX_RIGHT_SEND);
		if (ext2 == 0 ||
		    vfs_mount_service(vfs, "/mnt/ext2",
				      BUNIX_SERVICE_EXT2) != 0 ||
		    ext2_readonly_selftest(vfs) != 0) {
			return 1;
		}
		bunix_console_log(ext2_ok, sizeof(ext2_ok) - 1);
	}
	fs_namespace = create_namespace();
	if (fs_namespace == 0 ||
	    register_service_in_namespace(fs_namespace, BUNIX_SERVICE_VFS,
					  vfs) != 0) {
		return 1;
	}
	bunix_console_log(fs_namespace_ready,
			    sizeof(fs_namespace_ready) - 1);
	vfs = resolve_service_in_namespace(fs_namespace, BUNIX_SERVICE_VFS,
					   BUNIX_RIGHT_SEND);
	if (vfs == 0) {
		return 1;
	}
	bunix_console_log(fs_ready, sizeof(fs_ready) - 1);
	if (register_proc_execs(proc, vfs) != 0) {
		return 1;
	}
	if (vfs_read_text(vfs, "/hello.txt", file, sizeof(file)) == 0) {
		bunix_console_log(file, str_len(file));
	}

	const char linux_init_exec[] = "bootstrap: linux init exec\n";

	if (bunix_launch_module_with_caps("ping", bad_caps,
					  sizeof(bad_caps) /
					  sizeof(bad_caps[0])) < 0) {
		bunix_console_log(attenuated, sizeof(attenuated) - 1);
	}
	const struct bunix_launch_cap ping_caps[] = {
		{ console, BUNIX_RIGHT_SEND, 0 },
		{ vm, BUNIX_RIGHT_SEND, 0 },
		{ time, BUNIX_RIGHT_SEND, 0 },
	};
	bunix_launch_module_with_caps("ping", ping_caps,
				      sizeof(ping_caps) / sizeof(ping_caps[0]));

	const struct bunix_launch_cap linux_caps[] = {
		{ console, BUNIX_RIGHT_SEND, 0 },
		{ vfs_launch, BUNIX_RIGHT_SEND, 0 },
		{ BUNIX_HANDLE_NAMES, BUNIX_RIGHT_SEND, 0 },
	};
	bunix_launch_module_with_caps("linux", linux_caps,
				      sizeof(linux_caps) / sizeof(linux_caps[0]));
	linux = wait_service_in_namespace(BUNIX_NAMES_ROOT, BUNIX_SERVICE_LINUX,
					  BUNIX_RIGHT_SEND);
	if (linux == 0) {
		return 1;
	}

	if (spawn_linux_init(linux, "/sbin/init") != 0) {
		return 1;
	}
	bunix_console_log(linux_init_exec, sizeof(linux_init_exec) - 1);

	if (run_boot_spawns(proc, vfs) != 0) {
		return 1;
	}

	bunix_console_logs_to_ring();

	for (;;) {
		sleep_ns(time, 1000000000ull);
	}
}
