#include <bunix/alloc.h>
#include <bunix/libbunix.h>

enum {
	NAMES_TEST_SERVICE = ('N') | ('T' << 8) | ('S' << 16) | ('T' << 24),
	NAMES_TEST_OWNED_SERVICE =
		('N') | ('O' << 8) | ('W' << 16) | ('N' << 24),
};

static int str_eq(const char *left, const char *right);

static long claim_names_admin(void)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_NAMES,
		.type = BUNIX_NAMES_CLAIM_ADMIN,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { BUNIX_NAMES_ROOT, 0, 0, 0 },
	};
	struct bunix_msg reply;

	if (bunix_ipc_call(BUNIX_HANDLE_NAMES, &request, &reply) != 0) {
		return -1;
	}
	return reply.words[0] == 0 ? 0 : -1;
}

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

static u64 create_registration_claim(u64 namespace, u64 service)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_NAMES,
		.type = BUNIX_NAMES_CREATE_CLAIM,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { namespace, service, 0, 0 },
	};
	struct bunix_msg reply;

	if (bunix_ipc_call(BUNIX_HANDLE_NAMES, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return 0;
	}
	return reply.cap;
}

static long register_service(u64 service, u64 handle)
{
	return register_service_in_namespace(BUNIX_NAMES_ROOT, service, handle);
}

static long launch_claimed_module(const char *name, u64 namespace, u64 service,
				  const struct bunix_launch_cap *caps,
				  u64 cap_count)
{
	struct bunix_launch_cap claimed_caps[16];
	u64 claim;

	if (name == 0 || service == 0 ||
	    cap_count + 1 > sizeof(claimed_caps) / sizeof(claimed_caps[0])) {
		return -1;
	}
	claim = create_registration_claim(namespace, service);
	if (claim == 0) {
		return -1;
	}
	for (u64 i = 0; i < cap_count; i++) {
		claimed_caps[i] = caps[i];
	}
	claimed_caps[cap_count].handle = claim;
	claimed_caps[cap_count].rights = BUNIX_RIGHT_SEND;
	claimed_caps[cap_count].tag = BUNIX_CAP_CLAM;
	return bunix_launch_module_with_caps(name, claimed_caps, cap_count + 1);
}

static long task_id_by_name(const char *name)
{
	u64 pid_threads_flags = 0;
	u64 name_words[2] = { 0, 0 };

	for (u64 i = 0; bunix_task_info(i, &pid_threads_flags,
					name_words) == 0; i++) {
		char task_name[17];

		for (u64 j = 0; j < 16; j++) {
			task_name[j] = (char)(name_words[j / 8] >>
					      ((j % 8) * 8));
		}
		task_name[16] = '\0';
		if (str_eq(task_name, name)) {
			return (long)(pid_threads_flags & 0xffffffffu);
		}
	}
	return -1;
}

static long launch_claimed_module_task_id(
	const char *name, u64 namespace, u64 service,
	const struct bunix_launch_cap *caps, u64 cap_count)
{
	if (launch_claimed_module(name, namespace, service, caps, cap_count) <
	    0) {
		return -1;
	}
	return task_id_by_name(name);
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

static long vfs_grant_admin_task(u64 vfs, u64 task)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_GRANT_ADMIN_TASK,
		.words = { task, 0, 0, 0 },
	};
	struct bunix_msg reply;

	return bunix_ipc_call(vfs, &request, &reply) == 0 &&
	       reply.words[0] == 0 ? 0 : -1;
}

static long vfs_grant_subject_task(u64 vfs, u64 task)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_GRANT_SUBJECT_TASK,
		.words = { task, 0, 0, 0 },
	};
	struct bunix_msg reply;

	return bunix_ipc_call(vfs, &request, &reply) == 0 &&
	       reply.words[0] == 0 ? 0 : -1;
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

static long linux_grant_tty_input_task(u64 linux_mgmt, u64 task)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_LINUX,
		.type = BUNIX_LINUX_GRANT_TTY_INPUT_TASK,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { task, 0, 0, 0 },
	};
	struct bunix_msg reply;

	if (linux_mgmt == 0 || task == 0 ||
	    bunix_ipc_call(linux_mgmt, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return -1;
	}
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
		unsigned char frame[128];
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
	{
		struct bunix_net_addr_info addr = {
			.family = BUNIX_NET_ADDR_FAMILY_IPV6,
			.addr_hi = 0x20010db800180000ull,
			.addr_lo = 1,
			.prefix_len = 64,
			.iface = iface_id,
			.flags = 1,
			.preferred_lifetime = 0,
			.valid_lifetime = 0,
		};
		struct bunix_net_route_info route = {
			.family = BUNIX_NET_ADDR_FAMILY_IPV6,
			.prefix_hi = 0x20010db800180000ull,
			.prefix_lo = 0,
			.prefix_len = 64,
			.iface = iface_id,
			.gateway_hi = 0,
			.gateway_lo = 0,
			.flags = 1,
			.metric = 25,
		};

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
	{
		struct {
			u64 dest[4];
			unsigned char echo[8];
		} icmp = {
			.dest = {
				BUNIX_NET_ADDR_FAMILY_IPV6,
				0x20010db800180000ull,
				3,
				0,
			},
			.echo = { 128, 0, 0, 0, 0, 2, 0, 1 },
		};
		u64 icmp_socket;

		request.type = BUNIX_NET_ICMP_OPEN;
		request.cap = 0;
		request.cap_rights = 0;
		request.words[0] = BUNIX_NET_ADDR_FAMILY_IPV6;
		request.words[1] = 58;
		request.words[2] = 0;
		request.words[3] = 0;
		if (bunix_ipc_call(net, &request, &reply) != 0 ||
		    reply.words[0] != 0) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
		icmp_socket = reply.words[1];
		if (bunix_buffer_write((u64)buffer, 0, &icmp,
				       sizeof(icmp)) != 0) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
		request.type = BUNIX_NET_ICMP_SENDTO;
		request.cap = (u64)buffer;
		request.cap_rights = BUNIX_RIGHT_RECV;
		request.words[0] = icmp_socket;
		request.words[1] = sizeof(icmp.echo);
		request.words[2] = 0;
		request.words[3] = 0;
		if (bunix_ipc_call(net, &request, &reply) != 0 ||
		    reply.words[0] != 0 ||
		    reply.words[1] != sizeof(icmp.echo)) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
		request.type = BUNIX_NET_PACKET_TX_DEQUEUE;
		request.cap = (u64)buffer;
		request.cap_rights = BUNIX_RIGHT_SEND;
		request.words[0] = iface_id;
		request.words[1] = sizeof(packet.frame);
		if (bunix_ipc_call(net, &request, &reply) != 0 ||
		    reply.words[0] != 0 || reply.words[2] != 86 ||
		    bunix_buffer_read((u64)buffer, 0, &packet,
				      sizeof(packet)) != 0 ||
		    packet.info.iface != iface_id || packet.info.len != 86 ||
		    packet.frame[0] != 0x33 || packet.frame[1] != 0x33 ||
		    packet.frame[2] != 0xff || packet.frame[5] != 0x03 ||
		    packet.frame[12] != 0x86 || packet.frame[13] != 0xdd ||
		    packet.frame[20] != 58 || packet.frame[21] != 255 ||
		    packet.frame[22] != 0x20 || packet.frame[23] != 0x01 ||
		    packet.frame[24] != 0x0d || packet.frame[25] != 0xb8 ||
		    packet.frame[26] != 0x00 || packet.frame[27] != 0x18 ||
		    packet.frame[53] != 0x03 ||
		    packet.frame[54] != 135 || packet.frame[78] != 1 ||
		    packet.frame[79] != 1 || packet.frame[80] != 0x02 ||
		    packet.frame[85] != 0x00) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
		packet.info.iface = iface_id;
		packet.info.len = 86;
		packet.info.flags = 0;
		packet.info.reserved = 0;
		for (u64 i = 0; i < sizeof(packet.frame); i++) {
			packet.frame[i] = 0;
		}
		packet.frame[0] = 0x02;
		packet.frame[4] = 0x18;
		packet.frame[5] = 0x01;
		packet.frame[6] = 0x02;
		packet.frame[10] = 0x18;
		packet.frame[11] = 0x03;
		packet.frame[12] = 0x86;
		packet.frame[13] = 0xdd;
		packet.frame[14] = 0x60;
		packet.frame[18] = 0x00;
		packet.frame[19] = 0x20;
		packet.frame[20] = 58;
		packet.frame[21] = 255;
		packet.frame[22] = 0x20;
		packet.frame[23] = 0x01;
		packet.frame[24] = 0x0d;
		packet.frame[25] = 0xb8;
		packet.frame[26] = 0x00;
		packet.frame[27] = 0x18;
		packet.frame[37] = 0x03;
		packet.frame[38] = 0x20;
		packet.frame[39] = 0x01;
		packet.frame[40] = 0x0d;
		packet.frame[41] = 0xb8;
		packet.frame[42] = 0x00;
		packet.frame[43] = 0x18;
		packet.frame[53] = 0x01;
		packet.frame[54] = 136;
		packet.frame[58] = 0x60;
		packet.frame[62] = 0x20;
		packet.frame[63] = 0x01;
		packet.frame[64] = 0x0d;
		packet.frame[65] = 0xb8;
		packet.frame[66] = 0x00;
		packet.frame[67] = 0x18;
		packet.frame[77] = 0x03;
		packet.frame[78] = 2;
		packet.frame[79] = 1;
		packet.frame[80] = 0x02;
		packet.frame[84] = 0x18;
		packet.frame[85] = 0x03;
		if (bunix_buffer_write((u64)buffer, 0, &packet,
				       sizeof(packet)) != 0) {
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
		    reply.words[0] != 0 || reply.words[2] != 62 ||
		    bunix_buffer_read((u64)buffer, 0, &packet,
				      sizeof(packet)) != 0 ||
		    packet.info.iface != iface_id || packet.info.len != 62 ||
		    packet.frame[0] != 0x02 || packet.frame[5] != 0x03 ||
		    packet.frame[12] != 0x86 || packet.frame[13] != 0xdd ||
		    packet.frame[20] != 58 || packet.frame[21] != 64 ||
		    packet.frame[22] != 0x20 || packet.frame[23] != 0x01 ||
		    packet.frame[26] != 0x00 || packet.frame[27] != 0x18 ||
		    packet.frame[37] != 0x01 || packet.frame[38] != 0x20 ||
		    packet.frame[39] != 0x01 || packet.frame[42] != 0x00 ||
		    packet.frame[43] != 0x18 || packet.frame[53] != 0x03 ||
		    packet.frame[54] != 128) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
		request.type = BUNIX_NET_ICMP_CLOSE;
		request.cap = 0;
		request.cap_rights = 0;
		request.words[0] = icmp_socket;
		request.words[1] = 0;
		request.words[2] = 0;
		request.words[3] = 0;
		if (bunix_ipc_call(net, &request, &reply) != 0 ||
		    reply.words[0] != 0) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
	}
	{
		struct {
			u64 dest[4];
			unsigned char echo[8];
		} icmp = {
			.dest = {
				BUNIX_NET_ADDR_FAMILY_IPV4,
				0,
				0x0a120003ull,
				0,
			},
			.echo = { 8, 0, 0, 0, 0, 1, 0, 1 },
		};
		u64 icmp_socket;

		request.type = BUNIX_NET_ICMP_OPEN;
		request.cap = 0;
		request.cap_rights = 0;
		request.words[0] = BUNIX_NET_ADDR_FAMILY_IPV4;
		request.words[1] = 1;
		request.words[2] = 0;
		request.words[3] = 0;
		if (bunix_ipc_call(net, &request, &reply) != 0 ||
		    reply.words[0] != 0) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
		icmp_socket = reply.words[1];
		if (bunix_buffer_write((u64)buffer, 0, &icmp,
				       sizeof(icmp)) != 0) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
		request.type = BUNIX_NET_ICMP_SENDTO;
		request.cap = (u64)buffer;
		request.cap_rights = BUNIX_RIGHT_RECV;
		request.words[0] = icmp_socket;
		request.words[1] = sizeof(icmp.echo);
		request.words[2] = 0;
		request.words[3] = 0;
		if (bunix_ipc_call(net, &request, &reply) != 0 ||
		    reply.words[0] != 0 ||
		    reply.words[1] != sizeof(icmp.echo)) {
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
		    packet.frame[0] != 0xff || packet.frame[5] != 0xff ||
		    packet.frame[6] != 0x02 || packet.frame[12] != 0x08 ||
		    packet.frame[13] != 0x06 || packet.frame[20] != 0x00 ||
		    packet.frame[21] != 0x01 || packet.frame[28] != 0x0a ||
		    packet.frame[29] != 0x12 || packet.frame[31] != 0x01 ||
		    packet.frame[38] != 0x0a || packet.frame[39] != 0x12 ||
		    packet.frame[41] != 0x03) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
		packet.info.iface = iface_id;
		packet.info.len = 42;
		packet.info.flags = 0;
		packet.info.reserved = 0;
		for (u64 i = 0; i < sizeof(packet.frame); i++) {
			packet.frame[i] = 0;
		}
		packet.frame[0] = 0x02;
		packet.frame[4] = 0x18;
		packet.frame[5] = 0x01;
		packet.frame[6] = 0x02;
		packet.frame[10] = 0x18;
		packet.frame[11] = 0x03;
		packet.frame[12] = 0x08;
		packet.frame[13] = 0x06;
		packet.frame[15] = 0x01;
		packet.frame[16] = 0x08;
		packet.frame[18] = 0x06;
		packet.frame[19] = 0x04;
		packet.frame[21] = 0x02;
		packet.frame[22] = 0x02;
		packet.frame[26] = 0x18;
		packet.frame[27] = 0x03;
		packet.frame[28] = 0x0a;
		packet.frame[29] = 0x12;
		packet.frame[31] = 0x03;
		packet.frame[32] = 0x02;
		packet.frame[36] = 0x18;
		packet.frame[37] = 0x01;
		packet.frame[38] = 0x0a;
		packet.frame[39] = 0x12;
		packet.frame[41] = 0x01;
		if (bunix_buffer_write((u64)buffer, 0, &packet,
				       sizeof(packet)) != 0) {
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
		    packet.frame[0] != 0x02 || packet.frame[5] != 0x03 ||
		    packet.frame[12] != 0x08 || packet.frame[13] != 0x00 ||
		    packet.frame[23] != 1 || packet.frame[30] != 0x0a ||
		    packet.frame[31] != 0x12 || packet.frame[33] != 0x03) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
		request.type = BUNIX_NET_ICMP_CLOSE;
		request.cap = 0;
		request.cap_rights = 0;
		request.words[0] = icmp_socket;
		request.words[1] = 0;
		request.words[2] = 0;
		request.words[3] = 0;
		if (bunix_ipc_call(net, &request, &reply) != 0 ||
		    reply.words[0] != 0) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
	}
	{
		struct {
			u64 dest[4];
			unsigned char payload[4];
		} udp = {
			.dest = {
				BUNIX_NET_ADDR_FAMILY_IPV4,
				0,
				0x0a120004ull,
				0x1234,
			},
			.payload = { 'u', 'd', 'p', '4' },
		};
		u64 udp_socket;

		request.type = BUNIX_NET_UDP_OPEN;
		request.cap = 0;
		request.cap_rights = 0;
		request.words[0] = BUNIX_NET_ADDR_FAMILY_IPV4;
		request.words[1] = 0;
		request.words[2] = 0;
		request.words[3] = 0;
		if (bunix_ipc_call(net, &request, &reply) != 0 ||
		    reply.words[0] != 0) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
		udp_socket = reply.words[1];
		if (bunix_buffer_write((u64)buffer, 0, &udp, sizeof(udp)) !=
		    0) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
		request.type = BUNIX_NET_UDP_SENDTO;
		request.cap = (u64)buffer;
		request.cap_rights = BUNIX_RIGHT_RECV;
		request.words[0] = udp_socket;
		request.words[1] = sizeof(udp.payload);
		request.words[2] = 0;
		request.words[3] = 0;
		if (bunix_ipc_call(net, &request, &reply) != 0 ||
		    reply.words[0] != 0 ||
		    reply.words[1] != sizeof(udp.payload)) {
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
		    bunix_buffer_read((u64)buffer, 0, &packet, sizeof(packet)) !=
			    0 ||
		    packet.info.iface != iface_id || packet.info.len != 42 ||
		    packet.frame[0] != 0xff || packet.frame[5] != 0xff ||
		    packet.frame[12] != 0x08 || packet.frame[13] != 0x06 ||
		    packet.frame[20] != 0x00 || packet.frame[21] != 0x01 ||
		    packet.frame[28] != 0x0a || packet.frame[29] != 0x12 ||
		    packet.frame[31] != 0x01 || packet.frame[38] != 0x0a ||
		    packet.frame[39] != 0x12 || packet.frame[41] != 0x04) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
		packet.info.iface = iface_id;
		packet.info.len = 42;
		packet.info.flags = 0;
		packet.info.reserved = 0;
		for (u64 i = 0; i < sizeof(packet.frame); i++) {
			packet.frame[i] = 0;
		}
		packet.frame[0] = 0x02;
		packet.frame[4] = 0x18;
		packet.frame[5] = 0x01;
		packet.frame[6] = 0x02;
		packet.frame[10] = 0x18;
		packet.frame[11] = 0x04;
		packet.frame[12] = 0x08;
		packet.frame[13] = 0x06;
		packet.frame[15] = 0x01;
		packet.frame[16] = 0x08;
		packet.frame[18] = 0x06;
		packet.frame[19] = 0x04;
		packet.frame[21] = 0x02;
		packet.frame[22] = 0x02;
		packet.frame[26] = 0x18;
		packet.frame[27] = 0x04;
		packet.frame[28] = 0x0a;
		packet.frame[29] = 0x12;
		packet.frame[31] = 0x04;
		packet.frame[32] = 0x02;
		packet.frame[36] = 0x18;
		packet.frame[37] = 0x01;
		packet.frame[38] = 0x0a;
		packet.frame[39] = 0x12;
		packet.frame[41] = 0x01;
		if (bunix_buffer_write((u64)buffer, 0, &packet,
				       sizeof(packet)) != 0) {
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
		    reply.words[0] != 0 || reply.words[2] != 46 ||
		    bunix_buffer_read((u64)buffer, 0, &packet, sizeof(packet)) !=
			    0 ||
		    packet.info.iface != iface_id || packet.info.len != 46 ||
		    packet.frame[0] != 0x02 || packet.frame[5] != 0x04 ||
		    packet.frame[12] != 0x08 || packet.frame[13] != 0x00 ||
		    packet.frame[23] != 17 || packet.frame[30] != 0x0a ||
		    packet.frame[31] != 0x12 || packet.frame[33] != 0x04 ||
		    packet.frame[36] != 0x12 || packet.frame[37] != 0x34 ||
		    packet.frame[38] != 0x00 || packet.frame[39] != 0x0c ||
		    packet.frame[42] != 'u' || packet.frame[43] != 'd' ||
		    packet.frame[44] != 'p' || packet.frame[45] != '4') {
			bunix_handle_close((u64)buffer);
			return -1;
		}
		request.type = BUNIX_NET_UDP_CLOSE;
		request.cap = 0;
		request.cap_rights = 0;
		request.words[0] = udp_socket;
		request.words[1] = 0;
		request.words[2] = 0;
		request.words[3] = 0;
		if (bunix_ipc_call(net, &request, &reply) != 0 ||
		    reply.words[0] != 0) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
	}
	{
		unsigned char payload[4];
		u64 udp_socket;

		request.type = BUNIX_NET_UDP_OPEN;
		request.cap = 0;
		request.cap_rights = 0;
		request.words[0] = BUNIX_NET_ADDR_FAMILY_IPV4;
		request.words[1] = 0;
		request.words[2] = 0;
		request.words[3] = 0;
		if (bunix_ipc_call(net, &request, &reply) != 0 ||
		    reply.words[0] != 0) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
		udp_socket = reply.words[1];
		request.type = BUNIX_NET_UDP_BIND;
		request.cap = 0;
		request.cap_rights = 0;
		request.words[0] = udp_socket;
		request.words[1] = 0;
		request.words[2] = 0x0a120001ull;
		request.words[3] = 0x4321;
		if (bunix_ipc_call(net, &request, &reply) != 0 ||
		    reply.words[0] != 0 || reply.words[1] != 0x4321) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
		packet.info.iface = iface_id;
		packet.info.len = 46;
		packet.info.flags = 0;
		packet.info.reserved = 0;
		for (u64 i = 0; i < sizeof(packet.frame); i++) {
			packet.frame[i] = 0;
		}
		packet.frame[0] = 0x02;
		packet.frame[4] = 0x18;
		packet.frame[5] = 0x01;
		packet.frame[6] = 0x02;
		packet.frame[10] = 0x18;
		packet.frame[11] = 0x05;
		packet.frame[12] = 0x08;
		packet.frame[13] = 0x00;
		packet.frame[14] = 0x45;
		packet.frame[16] = 0x00;
		packet.frame[17] = 0x20;
		packet.frame[22] = 64;
		packet.frame[23] = 17;
		packet.frame[26] = 0x0a;
		packet.frame[27] = 0x12;
		packet.frame[29] = 0x05;
		packet.frame[30] = 0x0a;
		packet.frame[31] = 0x12;
		packet.frame[33] = 0x01;
		packet.frame[34] = 0x56;
		packet.frame[35] = 0x78;
		packet.frame[36] = 0x43;
		packet.frame[37] = 0x21;
		packet.frame[38] = 0x00;
		packet.frame[39] = 0x0c;
		packet.frame[42] = 'r';
		packet.frame[43] = 'x';
		packet.frame[44] = 'u';
		packet.frame[45] = 'd';
		if (bunix_buffer_write((u64)buffer, 0, &packet,
				       sizeof(packet)) != 0) {
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
		request.type = BUNIX_NET_UDP_POLL;
		request.cap = 0;
		request.cap_rights = 0;
		request.words[0] = udp_socket;
		request.words[1] = 0;
		request.words[2] = 0;
		request.words[3] = 0;
		if (bunix_ipc_call(net, &request, &reply) != 0 ||
		    reply.words[0] != 0 || (reply.words[1] & 1) == 0) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
		request.type = BUNIX_NET_UDP_RECV;
		request.cap = (u64)buffer;
		request.cap_rights = BUNIX_RIGHT_SEND;
		request.words[0] = udp_socket;
		request.words[1] = sizeof(payload);
		request.words[2] = 0;
		request.words[3] = 0;
		if (bunix_ipc_call(net, &request, &reply) != 0 ||
		    reply.words[0] != 0 || reply.words[1] != sizeof(payload) ||
		    reply.words[2] != 0x5678 ||
		    bunix_buffer_read((u64)buffer, 0, payload,
				      sizeof(payload)) != 0 ||
		    payload[0] != 'r' || payload[1] != 'x' ||
		    payload[2] != 'u' || payload[3] != 'd') {
			bunix_handle_close((u64)buffer);
			return -1;
		}
		request.type = BUNIX_NET_UDP_CLOSE;
		request.cap = 0;
		request.cap_rights = 0;
		request.words[0] = udp_socket;
		request.words[1] = 0;
		request.words[2] = 0;
		request.words[3] = 0;
		if (bunix_ipc_call(net, &request, &reply) != 0 ||
		    reply.words[0] != 0) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
	}
	{
		u64 listener;
		u64 accepted;

		request.type = BUNIX_NET_TCP_OPEN;
		request.cap = 0;
		request.cap_rights = 0;
		request.words[0] = BUNIX_NET_ADDR_FAMILY_IPV4;
		request.words[1] = 0;
		request.words[2] = 0;
		request.words[3] = 0;
		if (bunix_ipc_call(net, &request, &reply) != 0 ||
		    reply.words[0] != 0) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
		listener = reply.words[1];
		request.type = BUNIX_NET_TCP_BIND;
		request.words[0] = listener;
		request.words[1] = 0;
		request.words[2] = 0x0a120001ull;
		request.words[3] = 0x2345;
		if (bunix_ipc_call(net, &request, &reply) != 0 ||
		    reply.words[0] != 0 || reply.words[1] != 0x2345) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
		request.type = BUNIX_NET_TCP_LISTEN;
		request.words[0] = listener;
		request.words[1] = 1;
		request.words[2] = 0;
		request.words[3] = 0;
		if (bunix_ipc_call(net, &request, &reply) != 0 ||
		    reply.words[0] != 0) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
		packet.info.iface = iface_id;
		packet.info.len = 54;
		packet.info.flags = 0;
		packet.info.reserved = 0;
		for (u64 i = 0; i < sizeof(packet.frame); i++) {
			packet.frame[i] = 0;
		}
		packet.frame[0] = 0x02;
		packet.frame[4] = 0x18;
		packet.frame[5] = 0x01;
		packet.frame[6] = 0x02;
		packet.frame[10] = 0x18;
		packet.frame[11] = 0x06;
		packet.frame[12] = 0x08;
		packet.frame[13] = 0x00;
		packet.frame[14] = 0x45;
		packet.frame[16] = 0x00;
		packet.frame[17] = 0x28;
		packet.frame[22] = 64;
		packet.frame[23] = 6;
		packet.frame[26] = 0x0a;
		packet.frame[27] = 0x12;
		packet.frame[29] = 0x06;
		packet.frame[30] = 0x0a;
		packet.frame[31] = 0x12;
		packet.frame[33] = 0x01;
		packet.frame[34] = 0x67;
		packet.frame[35] = 0x89;
		packet.frame[36] = 0x23;
		packet.frame[37] = 0x45;
		packet.frame[38] = 0x01;
		packet.frame[39] = 0x02;
		packet.frame[40] = 0x03;
		packet.frame[41] = 0x04;
		packet.frame[46] = 0x50;
		packet.frame[47] = 0x02;
		packet.frame[48] = 0xff;
		packet.frame[49] = 0xff;
		if (bunix_buffer_write((u64)buffer, 0, &packet,
				       sizeof(packet)) != 0) {
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
		    reply.words[0] != 0 || reply.words[2] != 54 ||
		    bunix_buffer_read((u64)buffer, 0, &packet,
				      sizeof(packet)) != 0 ||
		    packet.info.iface != iface_id || packet.info.len != 54 ||
		    packet.frame[0] != 0x02 || packet.frame[5] != 0x06 ||
		    packet.frame[12] != 0x08 || packet.frame[13] != 0x00 ||
		    packet.frame[23] != 6 || packet.frame[26] != 0x0a ||
		    packet.frame[27] != 0x12 || packet.frame[29] != 0x01 ||
		    packet.frame[30] != 0x0a || packet.frame[31] != 0x12 ||
		    packet.frame[33] != 0x06 || packet.frame[34] != 0x23 ||
		    packet.frame[35] != 0x45 || packet.frame[36] != 0x67 ||
		    packet.frame[37] != 0x89 || packet.frame[42] != 0x01 ||
		    packet.frame[43] != 0x02 || packet.frame[44] != 0x03 ||
		    packet.frame[45] != 0x05 || packet.frame[47] != 0x12) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
		request.type = BUNIX_NET_TCP_POLL;
		request.cap = 0;
		request.cap_rights = 0;
		request.words[0] = listener;
		request.words[1] = 0;
		request.words[2] = 0;
		request.words[3] = 0;
		if (bunix_ipc_call(net, &request, &reply) != 0 ||
		    reply.words[0] != 0 || (reply.words[1] & 1) == 0) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
		request.type = BUNIX_NET_TCP_ACCEPT;
		request.words[0] = listener;
		if (bunix_ipc_call(net, &request, &reply) != 0 ||
		    reply.words[0] != 0 || reply.words[1] == 0) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
		accepted = reply.words[1];
		request.type = BUNIX_NET_TCP_PEER;
		request.words[0] = accepted;
		if (bunix_ipc_call(net, &request, &reply) != 0 ||
		    reply.words[0] != 0 || reply.words[1] != 0 ||
		    reply.words[2] != 0x0a120006ull ||
		    reply.words[3] != 0x6789) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
		request.type = BUNIX_NET_TCP_CLOSE;
		request.words[0] = accepted;
		(void)bunix_ipc_call(net, &request, &reply);
		request.words[0] = listener;
		(void)bunix_ipc_call(net, &request, &reply);
	}
	{
		u64 client;
		u64 client_port;

		request.type = BUNIX_NET_TCP_OPEN;
		request.cap = 0;
		request.cap_rights = 0;
		request.words[0] = BUNIX_NET_ADDR_FAMILY_IPV4;
		request.words[1] = 0;
		request.words[2] = 0;
		request.words[3] = 0;
		if (bunix_ipc_call(net, &request, &reply) != 0 ||
		    reply.words[0] != 0) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
		client = reply.words[1];
		request.type = BUNIX_NET_TCP_CONNECT;
		request.words[0] = client;
		request.words[1] = 0;
		request.words[2] = 0x0a120007ull;
		request.words[3] = 0x3456;
		if (bunix_ipc_call(net, &request, &reply) != 0 ||
		    reply.words[0] != 0 || reply.words[1] == 0) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
		client_port = reply.words[1];
		request.type = BUNIX_NET_PACKET_TX_DEQUEUE;
		request.cap = (u64)buffer;
		request.cap_rights = BUNIX_RIGHT_SEND;
		request.words[0] = iface_id;
		request.words[1] = sizeof(packet.frame);
		if (bunix_ipc_call(net, &request, &reply) != 0 ||
		    reply.words[0] != 0 || reply.words[2] != 42 ||
		    bunix_buffer_read((u64)buffer, 0, &packet,
				      sizeof(packet)) != 0 ||
		    packet.info.iface != iface_id || packet.info.len != 42 ||
		    packet.frame[0] != 0xff || packet.frame[5] != 0xff ||
		    packet.frame[12] != 0x08 || packet.frame[13] != 0x06 ||
		    packet.frame[20] != 0x00 || packet.frame[21] != 0x01 ||
		    packet.frame[28] != 0x0a || packet.frame[29] != 0x12 ||
		    packet.frame[31] != 0x01 || packet.frame[38] != 0x0a ||
		    packet.frame[39] != 0x12 || packet.frame[41] != 0x07) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
		packet.info.iface = iface_id;
		packet.info.len = 42;
		packet.info.flags = 0;
		packet.info.reserved = 0;
		for (u64 i = 0; i < sizeof(packet.frame); i++) {
			packet.frame[i] = 0;
		}
		packet.frame[0] = 0x02;
		packet.frame[4] = 0x18;
		packet.frame[5] = 0x01;
		packet.frame[6] = 0x02;
		packet.frame[10] = 0x18;
		packet.frame[11] = 0x07;
		packet.frame[12] = 0x08;
		packet.frame[13] = 0x06;
		packet.frame[15] = 0x01;
		packet.frame[16] = 0x08;
		packet.frame[18] = 0x06;
		packet.frame[19] = 0x04;
		packet.frame[21] = 0x02;
		packet.frame[22] = 0x02;
		packet.frame[26] = 0x18;
		packet.frame[27] = 0x07;
		packet.frame[28] = 0x0a;
		packet.frame[29] = 0x12;
		packet.frame[31] = 0x07;
		packet.frame[32] = 0x02;
		packet.frame[36] = 0x18;
		packet.frame[37] = 0x01;
		packet.frame[38] = 0x0a;
		packet.frame[39] = 0x12;
		packet.frame[41] = 0x01;
		if (bunix_buffer_write((u64)buffer, 0, &packet,
				       sizeof(packet)) != 0) {
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
		    reply.words[0] != 0 || reply.words[2] != 54 ||
		    bunix_buffer_read((u64)buffer, 0, &packet,
				      sizeof(packet)) != 0 ||
		    packet.info.iface != iface_id || packet.info.len != 54 ||
		    packet.frame[0] != 0x02 || packet.frame[5] != 0x07 ||
		    packet.frame[12] != 0x08 || packet.frame[13] != 0x00 ||
		    packet.frame[23] != 6 || packet.frame[30] != 0x0a ||
		    packet.frame[31] != 0x12 || packet.frame[33] != 0x07 ||
		    packet.frame[36] != 0x34 || packet.frame[37] != 0x56 ||
		    packet.frame[46] != 0x50 || packet.frame[47] != 0x02 ||
		    (((u64)packet.frame[34] << 8) | packet.frame[35]) !=
			    client_port) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
		request.type = BUNIX_NET_TCP_CLOSE;
		request.cap = 0;
		request.cap_rights = 0;
		request.words[0] = client;
		request.words[1] = 0;
		request.words[2] = 0;
		request.words[3] = 0;
		(void)bunix_ipc_call(net, &request, &reply);
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
	    reply.words[0] != 0 || reply.words[1] != 8 ||
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

static long usb_call(u64 usb, u64 type, u64 word0, u64 word1, u64 word2,
		     struct bunix_msg *reply)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_USB,
		.type = type,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { word0, word1, word2, 0 },
	};

	if (reply == 0 ||
	    bunix_ipc_call(usb, &request, reply) != 0 ||
	    reply->protocol != BUNIX_PROTO_USB ||
	    reply->words[0] != BUNIX_USB_OK) {
		return -1;
	}
	return 0;
}

static long usb_synth_selftest(u64 usb)
{
	struct bunix_msg reply;
	u64 count = 0;

	for (u64 i = 0; i < 100; i++) {
		if (usb_call(usb, BUNIX_USB_DEVICE_COUNT, 0, 0, 0,
			     &reply) == 0 && reply.words[1] != 0) {
			count = reply.words[1];
			break;
		}
		bunix_sleep_ns(10000000ull);
	}
	if (count == 0 ||
	    usb_call(usb, BUNIX_USB_DEVICE_INFO, 0, 0, 0, &reply) != 0) {
		return -1;
	}
	if ((reply.words[1] & 0xffffu) != 0x1234u ||
	    ((reply.words[1] >> 16) & 0xffffu) != 0x5678u ||
	    (reply.words[3] & 0xffffu) != 1 ||
	    ((reply.words[3] >> 16) & 0xffffu) != 1) {
		return -1;
	}
	if (usb_call(usb, BUNIX_USB_CLAIM_INTERFACE, 0, 0, 0, &reply) != 0 ||
	    (reply.words[1] & 0xffu) != 3 ||
	    ((reply.words[1] >> 8) & 0xffu) != 1 ||
	    ((reply.words[1] >> 16) & 0xffu) != 1 ||
	    usb_call(usb, BUNIX_USB_RELEASE_INTERFACE, 0, 0, 0,
		     &reply) != 0) {
		return -1;
	}
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
	const char usb_synth_test[] = "bootstrap: usb synth test\n";
	const char usb_synth_ok[] = "bootstrap: usb synth ok\n";
	const char xhci_test[] = "bootstrap: xhci test\n";
	const char input_ready[] = "bootstrap: input ready\n";
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
	u64 proc_mgmt = 0;
	u64 linux = 0;
	u64 linux_mgmt = 0;
	u64 user_mgmt = 0;
	u64 net = 0;
	u64 usb = 0;
	u64 vfs = 0;
	u64 vfs_launch = 0;
	u64 squashfs = 0;
	u64 fs_namespace = 0;
	const int ext2_root_mode = bunix_cmdline_has("ext2-root-test") > 0;
	const struct bunix_launch_cap bad_caps[] = {
		{ BUNIX_HANDLE_CONSOLE, BUNIX_RIGHT_SEND | BUNIX_RIGHT_RECV,
		  BUNIX_CAP_CONS },
	};

	bunix_console_log(launching, sizeof(launching) - 1);
	if (claim_names_admin() != 0) {
		return 1;
	}
	register_service(BUNIX_SERVICE_VM, BUNIX_HANDLE_VM);
	console = resolve_service(BUNIX_SERVICE_CONSOLE,
				  BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP);
	vm = resolve_service(BUNIX_SERVICE_VM,
			     BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP);
	if (console == 0 || vm == 0) {
		return 1;
	}
	user_mgmt = (u64)bunix_port_create("user-mgmt");
	proc_mgmt = (u64)bunix_port_create("proc-mgmt");
	linux_mgmt = (u64)bunix_port_create("linux-mgmt");
	if ((long)user_mgmt <= 0 || (long)proc_mgmt <= 0 ||
	    (long)linux_mgmt <= 0) {
		return 1;
	}
	bunix_console_log(names_ready, sizeof(names_ready) - 1);

	const struct bunix_launch_cap fs_caps[] = {
		{ console, BUNIX_RIGHT_SEND, BUNIX_CAP_CONS },
		{ BUNIX_HANDLE_NAMES, BUNIX_RIGHT_SEND, BUNIX_CAP_NAME },
	};

	launch_claimed_module("time", BUNIX_NAMES_ROOT, BUNIX_SERVICE_TIME,
			      fs_caps, sizeof(fs_caps) / sizeof(fs_caps[0]));
	time = wait_service_in_namespace(BUNIX_NAMES_ROOT, BUNIX_SERVICE_TIME,
					 BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP);
	if (time == 0) {
		return 1;
	}
	const struct bunix_launch_cap user_caps[] = {
		{ console, BUNIX_RIGHT_SEND, BUNIX_CAP_CONS },
		{ BUNIX_HANDLE_NAMES, BUNIX_RIGHT_SEND, BUNIX_CAP_NAME },
		{ user_mgmt, BUNIX_RIGHT_RECV, BUNIX_CAP_USRM },
	};

	const long user_task = launch_claimed_module_task_id(
		"user", BUNIX_NAMES_ROOT, BUNIX_SERVICE_USER,
		user_caps, sizeof(user_caps) / sizeof(user_caps[0]));
	if (wait_service_in_namespace(BUNIX_NAMES_ROOT, BUNIX_SERVICE_USER,
				      BUNIX_RIGHT_SEND) == 0 ||
	    user_task <= 0) {
		return 1;
	}

	const struct bunix_launch_cap proc_caps[] = {
		{ console, BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP, BUNIX_CAP_CONS },
		{ BUNIX_HANDLE_NAMES, BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP,
		  BUNIX_CAP_NAME },
		{ time, BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP, BUNIX_CAP_TIME },
		{ proc_mgmt, BUNIX_RIGHT_RECV, BUNIX_CAP_PRMG },
		{ user_mgmt, BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP,
		  BUNIX_CAP_USRM },
		{ linux_mgmt, BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP,
		  BUNIX_CAP_LNXM },
	};
	const long proc_task = launch_claimed_module_task_id(
		"proc", BUNIX_NAMES_ROOT, BUNIX_SERVICE_PROC,
		proc_caps, sizeof(proc_caps) / sizeof(proc_caps[0]));
	proc = wait_service_in_namespace(BUNIX_NAMES_ROOT, BUNIX_SERVICE_PROC,
					 BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP);
	if (proc_task <= 0 || proc == 0) {
		return 1;
	}

	launch_claimed_module("pci", BUNIX_NAMES_ROOT, BUNIX_SERVICE_PCI,
			      fs_caps, sizeof(fs_caps) / sizeof(fs_caps[0]));
	const u64 pci = wait_service_in_namespace(BUNIX_NAMES_ROOT,
						  BUNIX_SERVICE_PCI,
						  BUNIX_RIGHT_SEND |
							  BUNIX_RIGHT_DUP);
	if (pci == 0) {
		return 1;
	}
	const struct bunix_launch_cap virtio_bus_caps[] = {
		{ console, BUNIX_RIGHT_SEND, BUNIX_CAP_CONS },
		{ BUNIX_HANDLE_NAMES, BUNIX_RIGHT_SEND, BUNIX_CAP_NAME },
		{ pci, BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP, BUNIX_CAP_PCI },
	};
	launch_claimed_module(
		"virtio-bus", BUNIX_NAMES_ROOT, BUNIX_SERVICE_DEVICE,
		virtio_bus_caps,
		sizeof(virtio_bus_caps) / sizeof(virtio_bus_caps[0]));
	if (wait_service_in_namespace(BUNIX_NAMES_ROOT, BUNIX_SERVICE_DEVICE,
				      BUNIX_RIGHT_SEND) == 0) {
		return 1;
	}
	if (bunix_cmdline_has("usb-synth-test") > 0) {
		struct bunix_launch_cap usb_synth_caps[] = {
			{ console, BUNIX_RIGHT_SEND, BUNIX_CAP_CONS },
			{ BUNIX_HANDLE_NAMES, BUNIX_RIGHT_SEND,
			  BUNIX_CAP_NAME },
			{ 0, BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP,
			  BUNIX_CAP_USB },
		};

		bunix_console_log(usb_synth_test,
				  sizeof(usb_synth_test) - 1);
		launch_claimed_module("usb-bus", BUNIX_NAMES_ROOT,
				      BUNIX_SERVICE_USB, fs_caps,
				      sizeof(fs_caps) / sizeof(fs_caps[0]));
		usb = wait_service_in_namespace(BUNIX_NAMES_ROOT,
						 BUNIX_SERVICE_USB,
						 BUNIX_RIGHT_SEND |
							 BUNIX_RIGHT_DUP);
		if (usb == 0) {
			return 1;
		}
		usb_synth_caps[2].handle = usb;
		bunix_launch_module_with_caps(
			"usb-synth", usb_synth_caps,
			sizeof(usb_synth_caps) / sizeof(usb_synth_caps[0]));
		if (usb_synth_selftest(usb) != 0) {
			return 1;
		}
		bunix_console_log(usb_synth_ok, sizeof(usb_synth_ok) - 1);
		bunix_sleep_ns(100000000ull);
		(void)bunix_machine_poweroff(BUNIX_HANDLE_POWER_AUTH);
		for (;;) {
		}
	}
	if (bunix_cmdline_has("xhci-test") > 0) {
		struct bunix_launch_cap xhci_caps[] = {
			{ console, BUNIX_RIGHT_SEND, BUNIX_CAP_CONS },
			{ BUNIX_HANDLE_NAMES, BUNIX_RIGHT_SEND,
			  BUNIX_CAP_NAME },
			{ pci, BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP,
			  BUNIX_CAP_PCI },
			{ 0, BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP,
			  BUNIX_CAP_USB },
		};

		bunix_console_log(xhci_test, sizeof(xhci_test) - 1);
		launch_claimed_module("usb-bus", BUNIX_NAMES_ROOT,
				      BUNIX_SERVICE_USB, fs_caps,
				      sizeof(fs_caps) / sizeof(fs_caps[0]));
		usb = wait_service_in_namespace(BUNIX_NAMES_ROOT,
						 BUNIX_SERVICE_USB,
						 BUNIX_RIGHT_SEND |
							 BUNIX_RIGHT_DUP);
		if (usb == 0) {
			return 1;
		}
		xhci_caps[3].handle = usb;
		bunix_launch_module_with_caps(
			"xhci", xhci_caps,
			sizeof(xhci_caps) / sizeof(xhci_caps[0]));
		bunix_sleep_ns(1000000000ull);
		(void)bunix_machine_poweroff(BUNIX_HANDLE_POWER_AUTH);
		for (;;) {
		}
	}
	launch_claimed_module("input", BUNIX_NAMES_ROOT, BUNIX_SERVICE_INPUT,
			      fs_caps,
			      sizeof(fs_caps) / sizeof(fs_caps[0]));
	if (wait_service_in_namespace(BUNIX_NAMES_ROOT, BUNIX_SERVICE_INPUT,
				      BUNIX_RIGHT_SEND) == 0) {
		return 1;
	}
	bunix_console_log(input_ready, sizeof(input_ready) - 1);
	launch_claimed_module("net", BUNIX_NAMES_ROOT, BUNIX_SERVICE_NET,
			      fs_caps, sizeof(fs_caps) / sizeof(fs_caps[0]));
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
		launch_claimed_module("block", BUNIX_NAMES_ROOT,
				      BUNIX_SERVICE_BLOCK, fs_caps,
				      sizeof(fs_caps) / sizeof(fs_caps[0]));
	}
	if (bunix_cmdline_has("virtio-blk-test") > 0) {
		bunix_console_log(virtio_blk_test,
				  sizeof(virtio_blk_test) - 1);
		launch_claimed_module("virtio-blk", BUNIX_NAMES_ROOT,
				      BUNIX_SERVICE_BLOCK, fs_caps,
				      sizeof(fs_caps) / sizeof(fs_caps[0]));
	}
	if (bunix_cmdline_has("virtio-net-test") > 0) {
		bunix_launch_module_with_caps("virtio-net", fs_caps,
					      sizeof(fs_caps) /
						      sizeof(fs_caps[0]));
	}
	const long vfs_task = launch_claimed_module_task_id(
		"vfs", BUNIX_NAMES_ROOT, BUNIX_SERVICE_VFS,
		fs_caps, sizeof(fs_caps) / sizeof(fs_caps[0]));
	vfs = wait_service_in_namespace(BUNIX_NAMES_ROOT, BUNIX_SERVICE_VFS,
					BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP);
	if (vfs_task <= 0 ||
	    vfs == 0 ||
	    vfs_grant_admin_task(vfs, 0) != 0 ||
	    vfs_grant_subject_task(vfs, 0) != 0 ||
	    vfs_grant_subject_task(vfs, (u64)user_task) != 0 ||
	    vfs_grant_subject_task(vfs, (u64)proc_task) != 0) {
		return 1;
	}
	vfs_launch = vfs;
	const long procfs_task = launch_claimed_module_task_id(
		"procfs", BUNIX_NAMES_ROOT, BUNIX_SERVICE_PROCFS,
		fs_caps, sizeof(fs_caps) / sizeof(fs_caps[0]));
	if (procfs_task <= 0 || vfs_grant_admin_task(vfs, procfs_task) != 0) {
		return 1;
	}
	{
		u64 procfs = wait_service_in_namespace(BUNIX_NAMES_ROOT,
						       BUNIX_SERVICE_PROCFS,
						       BUNIX_RIGHT_SEND);

		if (procfs == 0 ||
		    vfs_grant_subject_task(procfs, vfs_task) != 0 ||
		    send_path_command(procfs, BUNIX_PROTO_PROCFS,
				      BUNIX_PROCFS_MOUNT_PATH, "/proc") != 0) {
			return 1;
		}
	}
	const long tmpfs_task = launch_claimed_module_task_id(
		"tmpfs", BUNIX_NAMES_ROOT, BUNIX_SERVICE_TMPFS,
		fs_caps, sizeof(fs_caps) / sizeof(fs_caps[0]));
	if (tmpfs_task <= 0 || vfs_grant_admin_task(vfs, tmpfs_task) != 0) {
		return 1;
	}
	u64 tmpfs = 0;
	{
		tmpfs = wait_service_in_namespace(BUNIX_NAMES_ROOT,
						  BUNIX_SERVICE_TMPFS,
						  BUNIX_RIGHT_SEND);

		if (tmpfs == 0 ||
		    vfs_grant_subject_task(tmpfs, vfs_task) != 0 ||
		    mount_tmpfs_roots(tmpfs) != 0) {
			return 1;
		}
	}
	const long devfs_task = launch_claimed_module_task_id(
		"devfs", BUNIX_NAMES_ROOT, BUNIX_SERVICE_DEVFS,
		fs_caps, sizeof(fs_caps) / sizeof(fs_caps[0]));
	if (devfs_task <= 0 || vfs_grant_admin_task(vfs, devfs_task) != 0) {
		return 1;
	}
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
	const long sysfs_task = launch_claimed_module_task_id(
		"sysfs", BUNIX_NAMES_ROOT, BUNIX_SERVICE_SYSFS,
		fs_caps, sizeof(fs_caps) / sizeof(fs_caps[0]));
	if (sysfs_task <= 0 || vfs_grant_admin_task(vfs, sysfs_task) != 0) {
		return 1;
	}
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
	const long utmpfs_task = launch_claimed_module_task_id(
		"utmpfs", BUNIX_NAMES_ROOT, BUNIX_SERVICE_UTMPFS,
		fs_caps, sizeof(fs_caps) / sizeof(fs_caps[0]));
	if (utmpfs_task <= 0 || vfs_grant_admin_task(vfs, utmpfs_task) != 0) {
		return 1;
	}
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
		const long ext2_task = launch_claimed_module_task_id(
			"ext2", BUNIX_NAMES_ROOT, BUNIX_SERVICE_EXT2,
			fs_caps, sizeof(fs_caps) / sizeof(fs_caps[0]));
		if (ext2_task <= 0 ||
		    vfs_grant_admin_task(vfs, ext2_task) != 0) {
			return 1;
		}
		ext2 = wait_service_in_namespace(BUNIX_NAMES_ROOT,
						  BUNIX_SERVICE_EXT2,
						  BUNIX_RIGHT_SEND);
		if (ext2 == 0 ||
		    vfs_grant_subject_task(ext2, vfs_task) != 0 ||
		    vfs_mount_service(vfs, "/mnt/ext2",
				      BUNIX_SERVICE_EXT2) != 0 ||
		    ext2_readonly_selftest(vfs) != 0) {
			return 1;
		}
		bunix_console_log(ext2_ok, sizeof(ext2_ok) - 1);
		(void)bunix_machine_poweroff(BUNIX_HANDLE_POWER_AUTH);
		for (;;) {
		}
	}
	if (ext2_root_mode) {
		u64 ext2;

		const long ext2_task = launch_claimed_module_task_id(
			"ext2", BUNIX_NAMES_ROOT, BUNIX_SERVICE_EXT2,
			fs_caps, sizeof(fs_caps) / sizeof(fs_caps[0]));
		if (ext2_task <= 0 ||
		    vfs_grant_admin_task(vfs, ext2_task) != 0) {
			return 1;
		}
		ext2 = wait_service_in_namespace(BUNIX_NAMES_ROOT,
						  BUNIX_SERVICE_EXT2,
						  BUNIX_RIGHT_SEND);
		if (ext2 == 0 ||
		    vfs_grant_subject_task(ext2, vfs_task) != 0 ||
		    vfs_mount_service(vfs, "/", BUNIX_SERVICE_EXT2) != 0) {
			return 1;
		}
		if (vfs_read_text(vfs, "/hello.txt", ext2_root_text,
				  sizeof(ext2_root_text)) != 0 ||
		    !str_eq(ext2_root_text, "ext2 hello\n")) {
			return 1;
		}
		bunix_console_log(ext2_root_ok, sizeof(ext2_root_ok) - 1);
		(void)bunix_machine_poweroff(BUNIX_HANDLE_POWER_AUTH);
		for (;;) {
		}
	} else {
		bunix_console_log(squashfs_root, sizeof(squashfs_root) - 1);
		const long squashfs_task = launch_claimed_module_task_id(
			"squashfs", BUNIX_NAMES_ROOT, BUNIX_SERVICE_SQUASHFS,
			fs_caps, sizeof(fs_caps) / sizeof(fs_caps[0]));
		if (squashfs_task <= 0 ||
		    vfs_grant_admin_task(vfs, squashfs_task) != 0) {
			return 1;
		}
		squashfs = wait_service_in_namespace(BUNIX_NAMES_ROOT,
						      BUNIX_SERVICE_SQUASHFS,
						      BUNIX_RIGHT_SEND);
		if (squashfs == 0 ||
		    vfs_grant_subject_task(squashfs, vfs_task) != 0 ||
		    send_path_command(squashfs, BUNIX_PROTO_SQUASHFS,
				      BUNIX_SQUASHFS_MOUNT_PATH,
				      "/.lower") != 0 ||
		    vfs_mount_service(vfs, "/.lower",
				      BUNIX_SERVICE_SQUASHFS) != 0) {
			return 1;
		}
	}
	const long unionfs_task = launch_claimed_module_task_id(
		"unionfs", BUNIX_NAMES_ROOT, BUNIX_SERVICE_UNIONFS,
		fs_caps, sizeof(fs_caps) / sizeof(fs_caps[0]));
	if (unionfs_task <= 0 || vfs_grant_admin_task(vfs, unionfs_task) != 0) {
		return 1;
	}
	if (tmpfs == 0 || vfs_grant_subject_task(tmpfs, unionfs_task) != 0) {
		return 1;
	}
	if (squashfs != 0 &&
	    vfs_grant_subject_task(squashfs, unionfs_task) != 0) {
		return 1;
	}
	{
		u64 unionfs = wait_service_in_namespace(BUNIX_NAMES_ROOT,
							BUNIX_SERVICE_UNIONFS,
							BUNIX_RIGHT_SEND);

		if (unionfs == 0 ||
		    vfs_grant_subject_task(unionfs, vfs_task) != 0 ||
		    unionfs_set_lower(unionfs, "/.lower") != 0 ||
		    unionfs_set_upper(unionfs, "/.upper") != 0 ||
		    send_path_command(unionfs, BUNIX_PROTO_UNIONFS,
				      BUNIX_UNIONFS_MOUNT_PATH, "/") != 0) {
			return 1;
		}
	}
	const long netcfg_task = launch_claimed_module_task_id(
		"netcfg", BUNIX_NAMES_ROOT, BUNIX_SERVICE_NETCFG,
		fs_caps, sizeof(fs_caps) / sizeof(fs_caps[0]));
	if (netcfg_task <= 0 ||
	    vfs_grant_subject_task(vfs, netcfg_task) != 0 ||
	    wait_service_in_namespace(BUNIX_NAMES_ROOT, BUNIX_SERVICE_NETCFG,
				      BUNIX_RIGHT_SEND) == 0) {
		return 1;
	}
	bunix_console_log(netcfg_ready, sizeof(netcfg_ready) - 1);
	if ((bunix_cmdline_has("ext2-test") > 0 ||
	     bunix_cmdline_has("ext2-subject-auth-test") > 0) &&
	    bunix_cmdline_has("ext2-fsck-test") <= 0) {
		u64 ext2;

		if (bunix_cmdline_has("virtio-blk-block-test") > 0 &&
		    wait_service_in_namespace(BUNIX_NAMES_ROOT,
					      BUNIX_SERVICE_BLOCK,
					      BUNIX_RIGHT_SEND) == 0) {
			return 1;
		}
		const long ext2_task = launch_claimed_module_task_id(
			"ext2", BUNIX_NAMES_ROOT, BUNIX_SERVICE_EXT2,
			fs_caps, sizeof(fs_caps) / sizeof(fs_caps[0]));
		if (ext2_task <= 0 ||
		    vfs_grant_admin_task(vfs, ext2_task) != 0) {
			return 1;
		}
		ext2 = wait_service_in_namespace(BUNIX_NAMES_ROOT,
						  BUNIX_SERVICE_EXT2,
						  BUNIX_RIGHT_SEND);
		if (ext2 == 0 ||
		    vfs_grant_subject_task(ext2, vfs_task) != 0 ||
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
	if (register_proc_execs(proc_mgmt, vfs) != 0) {
		return 1;
	}
	if (vfs_read_text(vfs, "/hello.txt", file, sizeof(file)) == 0) {
		bunix_console_log(file, str_len(file));
	}
	if (bunix_cmdline_has("names-auth-test") > 0) {
		const u64 claim = create_registration_claim(
			BUNIX_NAMES_ROOT, NAMES_TEST_OWNED_SERVICE);

		if (register_service(NAMES_TEST_SERVICE, console) != 0) {
			return 1;
		}
		if (claim == 0) {
			return 1;
		}
		const struct bunix_launch_cap names_test_caps[] = {
			{ claim, BUNIX_RIGHT_SEND, BUNIX_CAP_CLAM },
			{ BUNIX_HANDLE_NAMES, BUNIX_RIGHT_SEND,
			  BUNIX_CAP_NAME },
			{ console, BUNIX_RIGHT_SEND, BUNIX_CAP_CONS },
		};
		if (bunix_launch_module_with_caps(
			    "names-test", names_test_caps,
			    sizeof(names_test_caps) /
				    sizeof(names_test_caps[0])) < 0) {
			return 1;
		}
		bunix_sleep_ns(1000000000ull);
		(void)bunix_machine_poweroff(BUNIX_HANDLE_POWER_AUTH);
		for (;;) {
		}
	}

	const char linux_init_exec[] = "bootstrap: linux init exec\n";

	if (bunix_launch_module_with_caps("ping", bad_caps,
					  sizeof(bad_caps) /
					  sizeof(bad_caps[0])) < 0) {
		bunix_console_log(attenuated, sizeof(attenuated) - 1);
	}
	const struct bunix_launch_cap ping_caps[] = {
		{ console, BUNIX_RIGHT_SEND, BUNIX_CAP_CONS },
		{ vm, BUNIX_RIGHT_SEND, BUNIX_CAP_VM },
		{ time, BUNIX_RIGHT_SEND, BUNIX_CAP_TIME },
	};
	bunix_launch_module_with_caps("ping", ping_caps,
				      sizeof(ping_caps) / sizeof(ping_caps[0]));

	const struct bunix_launch_cap linux_caps[] = {
		{ console, BUNIX_RIGHT_SEND, BUNIX_CAP_CONS },
		{ vfs_launch, BUNIX_RIGHT_SEND, BUNIX_CAP_VFS },
		{ BUNIX_HANDLE_NAMES, BUNIX_RIGHT_SEND, BUNIX_CAP_NAME },
		{ BUNIX_HANDLE_POWER_AUTH, BUNIX_RIGHT_SEND, BUNIX_CAP_POWR },
		{ linux_mgmt, BUNIX_RIGHT_RECV, BUNIX_CAP_LNXM },
		{ user_mgmt, BUNIX_RIGHT_SEND, BUNIX_CAP_USRM },
		{ proc_mgmt, BUNIX_RIGHT_SEND, BUNIX_CAP_PRMG },
	};
	const long linux_task = launch_claimed_module_task_id(
		"linux", BUNIX_NAMES_ROOT, BUNIX_SERVICE_LINUX,
		linux_caps, sizeof(linux_caps) / sizeof(linux_caps[0]));
	if (linux_task <= 0 ||
	    vfs_grant_admin_task(vfs_launch, linux_task) != 0 ||
	    vfs_grant_subject_task(vfs_launch, linux_task) != 0) {
		return 1;
	}
	if (tmpfs != 0 && vfs_grant_subject_task(tmpfs, linux_task) != 0) {
		return 1;
	}
	linux = wait_service_in_namespace(BUNIX_NAMES_ROOT, BUNIX_SERVICE_LINUX,
					  BUNIX_RIGHT_SEND);
	if (linux == 0) {
		return 1;
	}
	{
		const long console_task = task_id_by_name("consoled");

		if (console_task <= 0 ||
		    linux_grant_tty_input_task(linux_mgmt,
					       (u64)console_task) != 0) {
			return 1;
		}
	}

	if (bunix_cmdline_has("mgmt-auth-test") > 0) {
		const struct bunix_launch_cap mgmt_test_caps[] = {
			{ BUNIX_HANDLE_NAMES, BUNIX_RIGHT_SEND,
			  BUNIX_CAP_NAME },
			{ console, BUNIX_RIGHT_SEND, BUNIX_CAP_CONS },
		};
		if (bunix_launch_module_with_caps(
			    "mgmt-test", mgmt_test_caps,
			    sizeof(mgmt_test_caps) /
				    sizeof(mgmt_test_caps[0])) < 0) {
			return 1;
		}
		bunix_sleep_ns(1000000000ull);
		(void)bunix_machine_poweroff(BUNIX_HANDLE_POWER_AUTH);
		for (;;) {
		}
	}

	if (bunix_cmdline_has("tty-input-auth-test") > 0) {
		const struct bunix_launch_cap tty_input_auth_test_caps[] = {
			{ BUNIX_HANDLE_NAMES, BUNIX_RIGHT_SEND,
			  BUNIX_CAP_NAME },
			{ console, BUNIX_RIGHT_SEND, BUNIX_CAP_CONS },
		};

		if (bunix_launch_module_with_caps(
			    "mgmt-test", tty_input_auth_test_caps,
			    sizeof(tty_input_auth_test_caps) /
				    sizeof(tty_input_auth_test_caps[0])) < 0) {
			return 1;
		}
		bunix_sleep_ns(1000000000ull);
		(void)bunix_machine_poweroff(BUNIX_HANDLE_POWER_AUTH);
		for (;;) {
		}
	}

	if (bunix_cmdline_has("vfs-auth-test") > 0) {
		u64 victim_handle = 0;
		u64 victim_type = 0;
		long test_task;
		long test_task_id;
		const struct bunix_launch_cap vfs_auth_test_caps[] = {
			{ vfs_launch, BUNIX_RIGHT_SEND, BUNIX_CAP_VFS },
			{ console, BUNIX_RIGHT_SEND, BUNIX_CAP_CONS },
		};

		if (vfs_open_path(vfs, "/hello.txt", &victim_handle,
				  &victim_type) != 0 ||
		    victim_type != BUNIX_VFS_TYPE_REGULAR) {
			return 1;
		}
		test_task = bunix_launch_module_with_caps(
			"mgmt-test", vfs_auth_test_caps,
			sizeof(vfs_auth_test_caps) /
				sizeof(vfs_auth_test_caps[0]));
		test_task_id = test_task > 0 ? task_id_by_name("mgmt-test") :
			       -1;
		if (test_task_id <= 0 ||
		    vfs_grant_subject_task(vfs_launch, (u64)test_task_id) != 0) {
			return 1;
		}
		(void)victim_handle;
		bunix_sleep_ns(1000000000ull);
		(void)bunix_machine_poweroff(BUNIX_HANDLE_POWER_AUTH);
		for (;;) {
		}
	}

	if (bunix_cmdline_has("vfs-mount-auth-test") > 0) {
		const struct bunix_launch_cap vfs_mount_auth_test_caps[] = {
			{ vfs_launch, BUNIX_RIGHT_SEND, BUNIX_CAP_VFS },
			{ console, BUNIX_RIGHT_SEND, BUNIX_CAP_CONS },
		};

		if (bunix_launch_module_with_caps(
			    "mgmt-test", vfs_mount_auth_test_caps,
			    sizeof(vfs_mount_auth_test_caps) /
				    sizeof(vfs_mount_auth_test_caps[0])) < 0) {
			return 1;
		}
		bunix_sleep_ns(1000000000ull);
		(void)bunix_machine_poweroff(BUNIX_HANDLE_POWER_AUTH);
		for (;;) {
		}
	}

	if (bunix_cmdline_has("vfs-subject-auth-test") > 0) {
		const struct bunix_launch_cap vfs_subject_auth_test_caps[] = {
			{ vfs_launch, BUNIX_RIGHT_SEND, BUNIX_CAP_VFS },
			{ console, BUNIX_RIGHT_SEND, BUNIX_CAP_CONS },
		};

		if (bunix_launch_module_with_caps(
			    "mgmt-test", vfs_subject_auth_test_caps,
			    sizeof(vfs_subject_auth_test_caps) /
				    sizeof(vfs_subject_auth_test_caps[0])) < 0) {
			return 1;
		}
		bunix_sleep_ns(1000000000ull);
		(void)bunix_machine_poweroff(BUNIX_HANDLE_POWER_AUTH);
		for (;;) {
		}
	}

	if (bunix_cmdline_has("tmpfs-subject-auth-test") > 0 ||
	    bunix_cmdline_has("squashfs-subject-auth-test") > 0 ||
	    bunix_cmdline_has("procfs-subject-auth-test") > 0 ||
	    bunix_cmdline_has("unionfs-subject-auth-test") > 0 ||
	    bunix_cmdline_has("ext2-subject-auth-test") > 0) {
		const struct bunix_launch_cap translator_subject_auth_test_caps[] = {
			{ BUNIX_HANDLE_NAMES, BUNIX_RIGHT_SEND,
			  BUNIX_CAP_NAME },
			{ console, BUNIX_RIGHT_SEND, BUNIX_CAP_CONS },
		};

		if (bunix_launch_module_with_caps(
			    "mgmt-test", translator_subject_auth_test_caps,
			    sizeof(translator_subject_auth_test_caps) /
				    sizeof(translator_subject_auth_test_caps[0])) < 0) {
			return 1;
		}
		bunix_sleep_ns(1000000000ull);
		(void)bunix_machine_poweroff(BUNIX_HANDLE_POWER_AUTH);
		for (;;) {
		}
	}

	if (spawn_linux_init(linux_mgmt, "/sbin/init") != 0) {
		return 1;
	}
	bunix_console_log(linux_init_exec, sizeof(linux_init_exec) - 1);
	if (bunix_cmdline_has("shell-test") > 0 &&
	    bunix_cmdline_has("debug-serial-shell") <= 0) {
		bunix_console_logs_to_ring();
	}

	if (run_boot_spawns(proc_mgmt, vfs) != 0) {
		return 1;
	}
	if (bunix_cmdline_has("shell-test") <= 0 &&
	    bunix_cmdline_has("debug-serial-userspace") <= 0) {
		bunix_console_logs_to_ring();
	}

	for (;;) {
		sleep_ns(time, 1000000000ull);
	}
}
