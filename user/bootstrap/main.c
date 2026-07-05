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

static void unpack_bytes(char *out, const u64 *words, u64 len)
{
	for (u64 i = 0; i < len; i++) {
		const u64 slot = i / 8;
		const u64 shift = (i % 8) * 8;

		out[i] = (char)((words[slot] >> shift) & 0xff);
	}
}

static void pack_path(u64 *words, const char *path)
{
	words[0] = 0;
	words[1] = 0;

	for (u64 i = 0; i < 16 && path[i] != '\0'; i++) {
		const u64 slot = i / 8;
		const u64 shift = (i % 8) * 8;

		words[slot] |= ((u64)(unsigned char)path[i]) << shift;
	}
}

static u64 str_len(const char *text)
{
	u64 len = 0;

	while (text[len] != '\0') {
		len++;
	}
	return len;
}

static long proc_register_exec(u64 proc, const char *path,
			       const char *task_name, u64 linux,
			       u64 log_kind)
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
		.words = { path_len, task_len, linux, log_kind },
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

struct boot_exec {
	const char *path;
	const char *task_name;
	u64 linux;
	u64 log_kind;
};

struct boot_path {
	const char *path;
};

static long register_proc_execs(u64 proc)
{
	const struct boot_exec execs[] = {
		{ "/bin/lxtest", "lxtest", 1, BUNIX_PROC_EXEC_LOG_LINUX },
		{ "/bin/musl-hello", "musl-hello", 1,
		  BUNIX_PROC_EXEC_LOG_MUSL },
		{ "/bin/fputest", "fputest", 1,
		  BUNIX_PROC_EXEC_LOG_DEFAULT },
		{ "/bin/sh", "busybox", 1, BUNIX_PROC_EXEC_LOG_SHELL },
		{ "/bin/busybox", "busybox", 1,
		  BUNIX_PROC_EXEC_LOG_DEFAULT },
		{ "/bin/login", "login", 1, BUNIX_PROC_EXEC_LOG_LOGIN },
		{ "/sbin/init", "busybox", 1, BUNIX_PROC_EXEC_LOG_INIT },
		{ "/bin/ipcstress", "ipcstress", 0,
		  BUNIX_PROC_EXEC_LOG_DEFAULT },
	};

	for (u64 i = 0; i < sizeof(execs) / sizeof(execs[0]); i++) {
		if (proc_register_exec(proc, execs[i].path,
				       execs[i].task_name, execs[i].linux,
				       execs[i].log_kind) != 0) {
			return -1;
		}
	}
	return 0;
}

static long tmpfs_mount_root(u64 tmpfs, const char *path)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_TMPFS,
		.type = BUNIX_TMPFS_MOUNT_ROOT,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { 0, 0, 0, 0 },
	};
	struct bunix_msg reply;

	pack_path(&request.words[0], path);
	if (bunix_ipc_call(tmpfs, &request, &reply) != 0) {
		return -1;
	}
	return reply.words[0] == 0 ? 0 : -1;
}

static long mount_tmpfs_roots(u64 tmpfs)
{
	const struct boot_path roots[] = {
		{ "/tmp" },
		{ "/run" },
		{ "/var/tmp" },
	};

	for (u64 i = 0; i < sizeof(roots) / sizeof(roots[0]); i++) {
		if (tmpfs_mount_root(tmpfs, roots[i].path) != 0) {
			return -1;
		}
	}
	return 0;
}

static long utmpfs_mount_path(u64 utmpfs, const char *path)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_UTMPFS,
		.type = BUNIX_UTMPFS_MOUNT_PATH,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { 0, 0, 0, 0 },
	};
	struct bunix_msg reply;

	pack_path(&request.words[0], path);
	if (bunix_ipc_call(utmpfs, &request, &reply) != 0) {
		return -1;
	}
	return reply.words[0] == 0 ? 0 : -1;
}

static long mount_utmpfs_paths(u64 utmpfs)
{
	const struct boot_path paths[] = {
		{ "/run/utmp" },
		{ "/var/run/utmp" },
	};

	for (u64 i = 0; i < sizeof(paths) / sizeof(paths[0]); i++) {
		if (utmpfs_mount_path(utmpfs, paths[i].path) != 0) {
			return -1;
		}
	}
	return 0;
}

static long devfs_mount_path(u64 devfs, const char *path)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_DEVFS,
		.type = BUNIX_DEVFS_MOUNT_PATH,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { 0, 0, 0, 0 },
	};
	struct bunix_msg reply;

	pack_path(&request.words[0], path);
	if (bunix_ipc_call(devfs, &request, &reply) != 0) {
		return -1;
	}
	return reply.words[0] == 0 ? 0 : -1;
}

static long procfs_mount_path(u64 procfs, const char *path)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_PROCFS,
		.type = BUNIX_PROCFS_MOUNT_PATH,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { 0, 0, 0, 0 },
	};
	struct bunix_msg reply;

	pack_path(&request.words[0], path);
	if (bunix_ipc_call(procfs, &request, &reply) != 0) {
		return -1;
	}
	return reply.words[0] == 0 ? 0 : -1;
}

static long unionfs_set_upper(u64 unionfs, const char *path)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_UNIONFS,
		.type = BUNIX_UNIONFS_SET_UPPER,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { 0, 0, 0, 0 },
	};
	struct bunix_msg reply;

	pack_path(&request.words[0], path);
	if (bunix_ipc_call(unionfs, &request, &reply) != 0) {
		return -1;
	}
	return reply.words[0] == 0 ? 0 : -1;
}

static long unionfs_mount_path(u64 unionfs, const char *path)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_UNIONFS,
		.type = BUNIX_UNIONFS_MOUNT_PATH,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { 0, 0, 0, 0 },
	};
	struct bunix_msg reply;

	pack_path(&request.words[0], path);
	if (bunix_ipc_call(unionfs, &request, &reply) != 0) {
		return -1;
	}
	return reply.words[0] == 0 ? 0 : -1;
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
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { 0, 0, 0, 0 },
	};
	struct bunix_msg reply;

	pack_path(&request.words[0], path);
	if (linux == 0 ||
	    bunix_ipc_call(linux, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
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
	struct bunix_msg vfs_request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_READ_PATH,
		.sender = 0,
		.reply = 0,
		.words = { 0, 0, 0, 16 },
	};
	struct bunix_msg vfs_reply;
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
	if (proc == 0 || register_proc_execs(proc) != 0) {
		return 1;
	}

	bunix_launch_module_with_caps("block", fs_caps,
				      sizeof(fs_caps) / sizeof(fs_caps[0]));
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

		if (procfs == 0 || procfs_mount_path(procfs, "/proc") != 0) {
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

		if (devfs == 0 || devfs_mount_path(devfs, "/dev") != 0) {
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
	bunix_launch_module_with_caps("unionfs", fs_caps,
				      sizeof(fs_caps) / sizeof(fs_caps[0]));
	{
		u64 unionfs = wait_service_in_namespace(BUNIX_NAMES_ROOT,
							BUNIX_SERVICE_UNIONFS,
							BUNIX_RIGHT_SEND);

		if (unionfs == 0 ||
		    unionfs_set_upper(unionfs, "/tmp/union") != 0 ||
		    unionfs_mount_path(unionfs, "/") != 0) {
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
	pack_path(&vfs_request.words[0], "/hello.txt");
	if (bunix_ipc_call(vfs, &vfs_request, &vfs_reply) == 0 &&
	    vfs_reply.words[0] == 0 && vfs_reply.words[1] <= 16) {
		unpack_bytes(file, &vfs_reply.words[2], vfs_reply.words[1]);
		file[vfs_reply.words[1]] = '\0';
		bunix_console_log(file, vfs_reply.words[1]);
	}

	struct bunix_msg proc_request = {
		.protocol = BUNIX_PROTO_PROC,
		.type = BUNIX_PROC_SPAWN,
		.sender = 0,
		.reply = 0,
		.words = { 0, 0, 0, 0 },
	};
	struct bunix_msg proc_reply;
	const char first_done[] = "bootstrap: first process exited\n";
	const char ipcstress_done[] = "bootstrap: ipcstress exited\n";
	const char linux_spawned[] = "bootstrap: linux process spawned\n";
	const char linux_done[] = "bootstrap: linux process exited\n";
	const char linux_init_exec[] = "bootstrap: linux init exec\n";
	const char musl_spawned[] = "bootstrap: musl process spawned\n";
	const char musl_done[] = "bootstrap: musl process exited\n";

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

	proc_request.type = BUNIX_PROC_SPAWN;
	proc_request.words[2] = 0;
	proc_request.words[3] = 0;
	pack_path(&proc_request.words[0], "/bin/first");
	if (bunix_ipc_call(proc, &proc_request, &proc_reply) != 0 ||
	    proc_reply.words[0] != 0) {
		return 1;
	}
	proc_request.type = BUNIX_PROC_WAIT;
	proc_request.words[0] = proc_reply.words[1];
	if (bunix_ipc_call(proc, &proc_request, &proc_reply) != 0 ||
	    proc_reply.words[0] != 0) {
		return 1;
	}
	bunix_console_log(first_done, sizeof(first_done) - 1);

	proc_request.type = BUNIX_PROC_SPAWN;
	proc_request.words[2] = 0;
	proc_request.words[3] = 0;
	pack_path(&proc_request.words[0], "/bin/ipcstress");
	if (bunix_ipc_call(proc, &proc_request, &proc_reply) != 0 ||
	    proc_reply.words[0] != 0) {
		return 1;
	}
	proc_request.type = BUNIX_PROC_WAIT;
	proc_request.words[0] = proc_reply.words[1];
	if (bunix_ipc_call(proc, &proc_request, &proc_reply) != 0 ||
	    proc_reply.words[0] != 0) {
		return 1;
	}
	bunix_console_log(ipcstress_done, sizeof(ipcstress_done) - 1);

	proc_request.type = BUNIX_PROC_SPAWN;
	proc_request.words[2] = 0;
	proc_request.words[3] = 0;
	pack_path(&proc_request.words[0], "/bin/lxtest");
	if (bunix_ipc_call(proc, &proc_request, &proc_reply) != 0 ||
	    proc_reply.words[0] != 0) {
		return 1;
	}
	bunix_console_log(linux_spawned, sizeof(linux_spawned) - 1);
	proc_request.type = BUNIX_PROC_WAIT;
	proc_request.words[0] = proc_reply.words[1];
	if (bunix_ipc_call(proc, &proc_request, &proc_reply) != 0 ||
	    proc_reply.words[0] != 0) {
		return 1;
	}
	bunix_console_log(linux_done, sizeof(linux_done) - 1);

	proc_request.type = BUNIX_PROC_SPAWN;
	pack_path(&proc_request.words[0], "/bin/musl-hello");
	if (bunix_ipc_call(proc, &proc_request, &proc_reply) != 0 ||
	    proc_reply.words[0] != 0) {
		return 1;
	}
	bunix_console_log(musl_spawned, sizeof(musl_spawned) - 1);
	proc_request.type = BUNIX_PROC_WAIT;
	proc_request.words[0] = proc_reply.words[1];
	if (bunix_ipc_call(proc, &proc_request, &proc_reply) != 0 ||
	    proc_reply.words[0] != 0) {
		return 1;
	}
	bunix_console_log(musl_done, sizeof(musl_done) - 1);

	bunix_console_logs_to_ring();

	for (;;) {
		sleep_ns(time, 1000000000ull);
	}
}
