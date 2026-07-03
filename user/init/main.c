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

int main(void)
{
	const char launching[] = "init: launching servers\n";
	const char attenuated[] = "init: bad cap denied\n";
	const char names_ready[] = "init: names ready\n";
	const char fs_namespace_ready[] = "init: fs namespace\n";
	const char fs_ready[] = "init: fs ready\n";
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
	u64 vfs = 0;
	u64 vfs_launch = 0;
	u64 fs_namespace = 0;
	const struct bunix_launch_cap bad_caps[] = {
		{ BUNIX_HANDLE_CONSOLE, BUNIX_RIGHT_SEND | BUNIX_RIGHT_RECV, 0 },
	};

	bunix_console_write(launching, sizeof(launching) - 1);
	register_service(BUNIX_SERVICE_CONSOLE, BUNIX_HANDLE_CONSOLE);
	register_service(BUNIX_SERVICE_VM, BUNIX_HANDLE_VM);
	console = resolve_service(BUNIX_SERVICE_CONSOLE,
				  BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP);
	vm = resolve_service(BUNIX_SERVICE_VM,
			     BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP);
	if (console == 0 || vm == 0) {
		return 1;
	}
	bunix_console_write(names_ready, sizeof(names_ready) - 1);

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
		{ BUNIX_HANDLE_NAMES, BUNIX_RIGHT_SEND, 0 },
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
	bunix_launch_module_with_caps("vfs", fs_caps,
				      sizeof(fs_caps) / sizeof(fs_caps[0]));
	vfs = wait_service_in_namespace(BUNIX_NAMES_ROOT, BUNIX_SERVICE_VFS,
					BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP);
	vfs_launch = vfs;
	fs_namespace = create_namespace();
	if (fs_namespace == 0 ||
	    register_service_in_namespace(fs_namespace, BUNIX_SERVICE_VFS,
					  vfs) != 0) {
		return 1;
	}
	bunix_console_write(fs_namespace_ready,
			    sizeof(fs_namespace_ready) - 1);
	vfs = resolve_service_in_namespace(fs_namespace, BUNIX_SERVICE_VFS,
					   BUNIX_RIGHT_SEND);
	bunix_console_write(fs_ready, sizeof(fs_ready) - 1);
	pack_path(&vfs_request.words[0], "/hello.txt");
	if (bunix_ipc_call(vfs, &vfs_request, &vfs_reply) == 0 &&
	    vfs_reply.words[0] == 0 && vfs_reply.words[1] <= 16) {
		unpack_bytes(file, &vfs_reply.words[2], vfs_reply.words[1]);
		file[vfs_reply.words[1]] = '\0';
		bunix_console_write(file, vfs_reply.words[1]);
	}

	struct bunix_msg proc_request = {
		.protocol = BUNIX_PROTO_PROC,
		.type = BUNIX_PROC_SPAWN,
		.sender = 0,
		.reply = 0,
		.words = { 0, 0, 0, 0 },
	};
	struct bunix_msg proc_reply;
	const char first_done[] = "init: first process exited\n";
	const char linux_spawned[] = "init: linux process spawned\n";
	const char linux_done[] = "init: linux process exited\n";
	const char musl_spawned[] = "init: musl process spawned\n";
	const char musl_done[] = "init: musl process exited\n";
	const char shell_spawned[] = "init: busybox shell spawned\n";

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
	bunix_console_write(first_done, sizeof(first_done) - 1);

	if (bunix_launch_module_with_caps("ping", bad_caps,
					  sizeof(bad_caps) /
					  sizeof(bad_caps[0])) < 0) {
		bunix_console_write(attenuated, sizeof(attenuated) - 1);
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

	proc_request.type = BUNIX_PROC_SPAWN;
	pack_path(&proc_request.words[0], "/bin/lxtest");
	if (bunix_ipc_call(proc, &proc_request, &proc_reply) != 0 ||
	    proc_reply.words[0] != 0) {
		return 1;
	}
	bunix_console_write(linux_spawned, sizeof(linux_spawned) - 1);
	proc_request.type = BUNIX_PROC_WAIT;
	proc_request.words[0] = proc_reply.words[1];
	if (bunix_ipc_call(proc, &proc_request, &proc_reply) != 0 ||
	    proc_reply.words[0] != 0) {
		return 1;
	}
	bunix_console_write(linux_done, sizeof(linux_done) - 1);

	proc_request.type = BUNIX_PROC_SPAWN;
	pack_path(&proc_request.words[0], "/bin/musl-hello");
	if (bunix_ipc_call(proc, &proc_request, &proc_reply) != 0 ||
	    proc_reply.words[0] != 0) {
		return 1;
	}
	bunix_console_write(musl_spawned, sizeof(musl_spawned) - 1);
	proc_request.type = BUNIX_PROC_WAIT;
	proc_request.words[0] = proc_reply.words[1];
	if (bunix_ipc_call(proc, &proc_request, &proc_reply) != 0 ||
	    proc_reply.words[0] != 0) {
		return 1;
	}
	bunix_console_write(musl_done, sizeof(musl_done) - 1);

	proc_request.type = BUNIX_PROC_SPAWN;
	pack_path(&proc_request.words[0], "/bin/sh");
	if (bunix_ipc_call(proc, &proc_request, &proc_reply) != 0 ||
	    proc_reply.words[0] != 0) {
		return 1;
	}
	bunix_console_write(shell_spawned, sizeof(shell_spawned) - 1);
	bunix_console_logs_to_ring();

	return 0;
}
