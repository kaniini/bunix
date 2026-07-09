#include <bunix/syscall.h>

static void log_line(const char *text, u64 len)
{
	(void)bunix_syscall2(BUNIX_SYSCALL_EARLY_CONSOLE_LOG, (u64)text, len);
}

static int str_eq(const char *left, const char *right)
{
	u64 i = 0;

	while (left != 0 && right != 0 &&
	       left[i] != '\0' && right[i] != '\0') {
		if (left[i] != right[i]) {
			return 0;
		}
		i++;
	}
	return left != 0 && right != 0 && left[i] == right[i];
}

static void launch_or_log(const char *name, const char *ok, u64 ok_len,
			  const char *fail, u64 fail_len)
{
	if (bunix_launch_module(name) >= 0) {
		log_line(ok, ok_len);
	} else {
		log_line(fail, fail_len);
	}
}

static void launch_with_caps_or_log(const char *name,
				    const struct bunix_launch_cap *caps,
				    u64 cap_count, const char *ok, u64 ok_len,
				    const char *fail, u64 fail_len)
{
	if (bunix_launch_module_with_caps(name, caps, cap_count) >= 0) {
		log_line(ok, ok_len);
	} else {
		log_line(fail, fail_len);
	}
}

static long launch_with_caps_task_id_or_log(const char *name,
					    const struct bunix_launch_cap *caps,
					    u64 cap_count, const char *ok,
					    u64 ok_len, const char *fail,
					    u64 fail_len)
{
	const long task = bunix_launch_module_with_caps(name, caps, cap_count);
	u64 pid_threads_flags = 0;
	u64 name_words[2] = { 0, 0 };

	if (task < 0) {
		log_line(fail, fail_len);
		return -1;
	}
	log_line(ok, ok_len);
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

static u64 str_len(const char *text)
{
	u64 len = 0;

	while (text != 0 && text[len] != '\0') {
		len++;
	}
	return len;
}

static u64 wait_service(u64 service, unsigned int rights)
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

	if (bunix_ipc_call(BUNIX_HANDLE_NAMES, &request, &reply) != 0 ||
	    reply.words[0] != 0 ||
	    (reply.cap_rights & rights) != rights) {
		return 0;
	}
	return reply.cap;
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

static long proc_spawn_wait_argv(u64 proc, const char *path,
				 const char *const *argv, u64 argc)
{
	const u64 path_len = str_len(path) + 1;
	u64 total = path_len;
	for (u64 i = 0; i < argc; i++) {
		total += str_len(argv[i]) + 1;
	}
	const long buffer = bunix_buffer_create(total);
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_PROC,
		.type = BUNIX_PROC_SPAWN_BUFFER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_RECV | BUNIX_RIGHT_DUP,
		.reply = 0,
		.cap = buffer > 0 ? (u64)buffer : 0,
		.words = { total, argc, 0, 2 },
	};
	struct bunix_msg reply;
	u64 pid;
	u64 offset = path_len;

	if (proc == 0 || buffer <= 0 ||
	    bunix_buffer_write((u64)buffer, 0, path, path_len) != 0) {
		if (buffer > 0) {
			bunix_handle_close((u64)buffer);
		}
		return -1;
	}
	for (u64 i = 0; i < argc; i++) {
		const u64 len = str_len(argv[i]) + 1;

		if (bunix_buffer_write((u64)buffer, offset, argv[i],
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
	pid = reply.words[1];
	bunix_handle_close((u64)buffer);

	request.type = BUNIX_PROC_WAIT;
	request.cap = 0;
	request.cap_rights = 0;
	request.words[0] = pid;
	request.words[1] = 0;
	request.words[2] = 0;
	request.words[3] = 0;
	if (bunix_ipc_call(proc, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return -1;
	}
	return 0;
}

static long proc_spawn_wait(u64 proc, const char *path)
{
	const char *const argv[] = { path };

	return proc_spawn_wait_argv(proc, path, argv,
				    sizeof(argv) / sizeof(argv[0]));
}

static long send_path_command(u64 service, unsigned int protocol,
			      unsigned int type, const char *path)
{
	struct bunix_msg reply;

	if (bunix_ipc_call_path(service, protocol, type, path, 0, 0, 0,
				&reply) != 0) {
		return -1;
	}
	return reply.words[0] == 0 ? 0 : -1;
}

static long vfs_mount_service(u64 vfs, const char *path, u64 service)
{
	struct bunix_msg reply;

	if (bunix_ipc_call_path(vfs, BUNIX_PROTO_VFS,
				BUNIX_VFS_MOUNT_BUFFER, path, service,
				0, 0, &reply) != 0) {
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

int main(void)
{
	const char online[] = "bootstrap-riscv64: online\n";
	const char abi_ok[] = "bootstrap-riscv64: abi-smoke launched\n";
	const char abi_fail[] = "bootstrap-riscv64: abi-smoke failed\n";
	const char time_ok[] = "bootstrap-riscv64: time launched\n";
	const char time_fail[] = "bootstrap-riscv64: time failed\n";
	const char time_ready[] = "bootstrap-riscv64: time ready\n";
	const char time_wait_fail[] = "bootstrap-riscv64: time wait failed\n";
	const char user_ok[] = "bootstrap-riscv64: user launched\n";
	const char user_fail[] = "bootstrap-riscv64: user failed\n";
	const char proc_ok[] = "bootstrap-riscv64: proc launched\n";
	const char proc_fail[] = "bootstrap-riscv64: proc failed\n";
	const char block_ok[] = "bootstrap-riscv64: block launched\n";
	const char block_fail[] = "bootstrap-riscv64: block failed\n";
	const char block_ready[] = "bootstrap-riscv64: block ready\n";
	const char block_wait_fail[] = "bootstrap-riscv64: block wait failed\n";
	const char vfs_ok[] = "bootstrap-riscv64: vfs launched\n";
	const char vfs_fail[] = "bootstrap-riscv64: vfs failed\n";
	const char vfs_ready[] = "bootstrap-riscv64: vfs ready\n";
	const char vfs_wait_fail[] = "bootstrap-riscv64: vfs wait failed\n";
	const char squashfs_ok[] = "bootstrap-riscv64: squashfs launched\n";
	const char squashfs_fail[] = "bootstrap-riscv64: squashfs failed\n";
	const char squashfs_ready[] = "bootstrap-riscv64: squashfs ready\n";
	const char squashfs_wait_fail[] = "bootstrap-riscv64: squashfs wait failed\n";
	const char root_ok[] = "bootstrap-riscv64: rootfs mounted\n";
	const char root_fail[] = "bootstrap-riscv64: rootfs mount failed\n";
	const char linux_ok[] = "bootstrap-riscv64: linux launched\n";
	const char linux_fail[] = "bootstrap-riscv64: linux failed\n";
	const char linux_ready[] = "bootstrap-riscv64: linux ready\n";
	const char linux_wait_fail[] = "bootstrap-riscv64: linux wait failed\n";
	const char dyn_ok[] = "bootstrap-riscv64: dyn-hello ok\n";
	const char dyn_fail[] = "bootstrap-riscv64: dyn-hello failed\n";
	const char alpine_sh_ok[] = "bootstrap-riscv64: alpine sh ok\n";
	const char alpine_sh_fail[] = "bootstrap-riscv64: alpine sh failed\n";
	const char syscall_ok[] = "bootstrap-riscv64: syscall-smoke ok\n";
	const char syscall_fail[] = "bootstrap-riscv64: syscall-smoke failed\n";
	const char hello_ok[] = "bootstrap-riscv64: musl-hello ok\n";
	const char hello_fail[] = "bootstrap-riscv64: musl-hello failed\n";
	const char done[] = "bootstrap-riscv64: done\n";
	const int alpine_test = bunix_cmdline_has("riscv64-alpine-test") > 0;
	const int uart_console_test =
		bunix_cmdline_has("riscv64-uart-console") > 0;

	log_line(online, sizeof(online) - 1);
	launch_or_log("abi-smoke.user", abi_ok, sizeof(abi_ok) - 1,
		      abi_fail, sizeof(abi_fail) - 1);
	if (uart_console_test) {
		log_line(done, sizeof(done) - 1);
		(void)bunix_machine_poweroff(BUNIX_HANDLE_POWER_AUTH);
		return 0;
	}
	const struct bunix_launch_cap fs_caps[] = {
		{ BUNIX_HANDLE_CONSOLE, BUNIX_RIGHT_SEND, BUNIX_CAP_CONS },
		{ BUNIX_HANDLE_NAMES, BUNIX_RIGHT_SEND, BUNIX_CAP_NAME },
	};
	launch_with_caps_or_log("time", fs_caps,
				sizeof(fs_caps) / sizeof(fs_caps[0]),
				time_ok, sizeof(time_ok) - 1,
				time_fail, sizeof(time_fail) - 1);
	const u64 time = wait_service(BUNIX_SERVICE_TIME,
				      BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP);
	if (time != 0) {
		log_line(time_ready, sizeof(time_ready) - 1);
	} else {
		log_line(time_wait_fail, sizeof(time_wait_fail) - 1);
	}
	launch_with_caps_or_log("user", fs_caps,
				sizeof(fs_caps) / sizeof(fs_caps[0]),
				user_ok, sizeof(user_ok) - 1,
				user_fail, sizeof(user_fail) - 1);
	const struct bunix_launch_cap proc_caps[] = {
		{ BUNIX_HANDLE_CONSOLE, BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP,
		  BUNIX_CAP_CONS },
		{ BUNIX_HANDLE_NAMES, BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP,
		  BUNIX_CAP_NAME },
		{ time, BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP, BUNIX_CAP_TIME },
	};
	if (time != 0) {
		launch_with_caps_or_log("proc", proc_caps,
					sizeof(proc_caps) / sizeof(proc_caps[0]),
					proc_ok, sizeof(proc_ok) - 1,
					proc_fail, sizeof(proc_fail) - 1);
	} else {
		log_line(proc_fail, sizeof(proc_fail) - 1);
	}
	const u64 proc = wait_service(BUNIX_SERVICE_PROC,
				      BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP);
	launch_with_caps_or_log("block", fs_caps,
				sizeof(fs_caps) / sizeof(fs_caps[0]),
				block_ok, sizeof(block_ok) - 1,
				block_fail, sizeof(block_fail) - 1);
	const u64 block = wait_service(BUNIX_SERVICE_BLOCK, BUNIX_RIGHT_SEND);
	if (block != 0) {
		log_line(block_ready, sizeof(block_ready) - 1);
	} else {
		log_line(block_wait_fail, sizeof(block_wait_fail) - 1);
	}
	const long vfs_task = launch_with_caps_task_id_or_log(
		"vfs", fs_caps, sizeof(fs_caps) / sizeof(fs_caps[0]),
		vfs_ok, sizeof(vfs_ok) - 1, vfs_fail, sizeof(vfs_fail) - 1);
	const u64 vfs = wait_service(BUNIX_SERVICE_VFS,
				     BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP);
	if (vfs != 0 && vfs_task > 0 && vfs_grant_admin_task(vfs, 0) == 0) {
		log_line(vfs_ready, sizeof(vfs_ready) - 1);
	} else {
		log_line(vfs_wait_fail, sizeof(vfs_wait_fail) - 1);
	}
	const long squashfs_task = launch_with_caps_task_id_or_log(
		"squashfs", fs_caps, sizeof(fs_caps) / sizeof(fs_caps[0]),
		squashfs_ok, sizeof(squashfs_ok) - 1,
		squashfs_fail, sizeof(squashfs_fail) - 1);
	const u64 squashfs = wait_service(BUNIX_SERVICE_SQUASHFS,
					  BUNIX_RIGHT_SEND);
	if (vfs != 0 && squashfs != 0 && squashfs_task > 0 &&
	    vfs_grant_admin_task(vfs, squashfs_task) == 0) {
		log_line(squashfs_ready, sizeof(squashfs_ready) - 1);
	} else {
		log_line(squashfs_wait_fail, sizeof(squashfs_wait_fail) - 1);
	}
	if (vfs != 0 && squashfs != 0 &&
	    send_path_command(squashfs, BUNIX_PROTO_SQUASHFS,
			      BUNIX_SQUASHFS_MOUNT_PATH, "/") == 0 &&
	    vfs_mount_service(vfs, "/", BUNIX_SERVICE_SQUASHFS) == 0) {
		log_line(root_ok, sizeof(root_ok) - 1);
	} else {
		log_line(root_fail, sizeof(root_fail) - 1);
	}
	const struct bunix_launch_cap linux_caps[] = {
		{ BUNIX_HANDLE_CONSOLE, BUNIX_RIGHT_SEND, BUNIX_CAP_CONS },
		{ vfs, BUNIX_RIGHT_SEND, BUNIX_CAP_VFS },
		{ BUNIX_HANDLE_NAMES, BUNIX_RIGHT_SEND, BUNIX_CAP_NAME },
	};
	launch_with_caps_or_log("linux", linux_caps,
				sizeof(linux_caps) / sizeof(linux_caps[0]),
				linux_ok, sizeof(linux_ok) - 1,
				linux_fail, sizeof(linux_fail) - 1);
	const u64 linux = wait_service(BUNIX_SERVICE_LINUX, BUNIX_RIGHT_SEND);
	if (linux != 0) {
		log_line(linux_ready, sizeof(linux_ready) - 1);
	} else {
		log_line(linux_wait_fail, sizeof(linux_wait_fail) - 1);
	}
	if (proc != 0 && linux != 0 &&
	    proc_register_exec(proc, "/bin/dyn-hello", "dyn-hello", 1) == 0 &&
	    proc_spawn_wait(proc, "/bin/dyn-hello") == 0) {
		log_line(dyn_ok, sizeof(dyn_ok) - 1);
	} else {
		log_line(dyn_fail, sizeof(dyn_fail) - 1);
	}
	if (alpine_test) {
		const char *const sh_argv[] = {
			"/bin/sh",
			"-c",
			"echo BUNIX_RISCV64_ALPINE_SH_OK",
		};

		if (proc != 0 &&
		    proc_register_exec(proc, "/bin/sh", "alpine-sh", 1) == 0 &&
		    proc_spawn_wait_argv(proc, "/bin/sh", sh_argv,
					sizeof(sh_argv) / sizeof(sh_argv[0])) == 0) {
			log_line(alpine_sh_ok, sizeof(alpine_sh_ok) - 1);
		} else {
			log_line(alpine_sh_fail, sizeof(alpine_sh_fail) - 1);
		}
		log_line(done, sizeof(done) - 1);
		(void)bunix_machine_poweroff(BUNIX_HANDLE_POWER_AUTH);
		return 0;
	}
	if (proc != 0 && linux != 0 &&
	    proc_register_exec(proc, "/bin/rv64-syscall-smoke",
			       "rv64-syscall-smoke", 1) == 0 &&
	    proc_spawn_wait(proc, "/bin/rv64-syscall-smoke") == 0) {
		log_line(syscall_ok, sizeof(syscall_ok) - 1);
	} else {
		log_line(syscall_fail, sizeof(syscall_fail) - 1);
	}
	if (proc != 0 && linux != 0 &&
	    proc_register_exec(proc, "/bin/musl-hello", "musl-hello", 1) == 0 &&
	    proc_spawn_wait(proc, "/bin/musl-hello") == 0) {
		log_line(hello_ok, sizeof(hello_ok) - 1);
	} else {
		log_line(hello_fail, sizeof(hello_fail) - 1);
	}

	log_line(done, sizeof(done) - 1);
	(void)bunix_machine_poweroff(BUNIX_HANDLE_POWER_AUTH);
	return 0;
}
