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

int main(void)
{
	const char launching[] = "bootstrap: launching servers\n";
	const char attenuated[] = "bootstrap: bad cap denied\n";
	const char names_ready[] = "bootstrap: names ready\n";
	const char fs_namespace_ready[] = "bootstrap: fs namespace\n";
	const char fs_ready[] = "bootstrap: fs ready\n";
	char file[17];
	u64 console;
	u64 vm;
	u64 time = 0;
	u64 proc = 0;
	u64 linux = 0;
	u64 vfs = 0;
	u64 vfs_launch = 0;
	u64 fs_namespace = 0;
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

	bunix_launch_module_with_caps("block", fs_caps,
				      sizeof(fs_caps) / sizeof(fs_caps[0]));
	bunix_launch_module_with_caps("virtio-bus", fs_caps,
				      sizeof(fs_caps) / sizeof(fs_caps[0]));
	if (wait_service_in_namespace(BUNIX_NAMES_ROOT, BUNIX_SERVICE_DEVICE,
				      BUNIX_RIGHT_SEND) == 0) {
		return 1;
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
	bunix_launch_module_with_caps("rootfs", fs_caps,
				      sizeof(fs_caps) / sizeof(fs_caps[0]));
	{
		u64 rootfs = wait_service_in_namespace(BUNIX_NAMES_ROOT,
						       BUNIX_SERVICE_ROOTFS,
						       BUNIX_RIGHT_SEND);

		if (rootfs == 0 ||
		    send_path_command(rootfs, BUNIX_PROTO_ROOTFS,
				      BUNIX_ROOTFS_MOUNT_PATH, "/.lower") != 0 ||
		    vfs_mount_service(vfs, "/.lower",
				      BUNIX_SERVICE_ROOTFS) != 0) {
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
