#include <bunix/syscall.h>

static void log_line(const char *text, u64 len)
{
	(void)bunix_syscall2(BUNIX_SYSCALL_EARLY_CONSOLE_LOG, (u64)text, len);
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
	const char syscall_ok[] = "bootstrap-riscv64: syscall-smoke launched\n";
	const char syscall_fail[] = "bootstrap-riscv64: syscall-smoke failed\n";
	const char hello_ok[] = "bootstrap-riscv64: musl-hello launched\n";
	const char hello_fail[] = "bootstrap-riscv64: musl-hello failed\n";
	const char done[] = "bootstrap-riscv64: done\n";

	log_line(online, sizeof(online) - 1);
	launch_or_log("abi-smoke.user", abi_ok, sizeof(abi_ok) - 1,
		      abi_fail, sizeof(abi_fail) - 1);
	const struct bunix_launch_cap fs_caps[] = {
		{ BUNIX_HANDLE_CONSOLE, BUNIX_RIGHT_SEND, 0 },
		{ BUNIX_HANDLE_NAMES, BUNIX_RIGHT_SEND, 0 },
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
		{ BUNIX_HANDLE_CONSOLE, BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP, 0 },
		{ BUNIX_HANDLE_NAMES, BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP, 0 },
		{ time, BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP, 0 },
	};
	if (time != 0) {
		launch_with_caps_or_log("proc", proc_caps,
					sizeof(proc_caps) / sizeof(proc_caps[0]),
					proc_ok, sizeof(proc_ok) - 1,
					proc_fail, sizeof(proc_fail) - 1);
	} else {
		log_line(proc_fail, sizeof(proc_fail) - 1);
	}
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
	launch_with_caps_or_log("vfs", fs_caps,
				sizeof(fs_caps) / sizeof(fs_caps[0]),
				vfs_ok, sizeof(vfs_ok) - 1,
				vfs_fail, sizeof(vfs_fail) - 1);
	const u64 vfs = wait_service(BUNIX_SERVICE_VFS, BUNIX_RIGHT_SEND);
	if (vfs != 0) {
		log_line(vfs_ready, sizeof(vfs_ready) - 1);
	} else {
		log_line(vfs_wait_fail, sizeof(vfs_wait_fail) - 1);
	}
	launch_with_caps_or_log("squashfs", fs_caps,
				sizeof(fs_caps) / sizeof(fs_caps[0]),
				squashfs_ok, sizeof(squashfs_ok) - 1,
				squashfs_fail, sizeof(squashfs_fail) - 1);
	const u64 squashfs = wait_service(BUNIX_SERVICE_SQUASHFS,
					  BUNIX_RIGHT_SEND);
	if (squashfs != 0) {
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
		{ BUNIX_HANDLE_CONSOLE, BUNIX_RIGHT_SEND, 0 },
		{ BUNIX_HANDLE_VM, BUNIX_RIGHT_SEND, 0 },
		{ BUNIX_HANDLE_NAMES, BUNIX_RIGHT_SEND, 0 },
	};
	launch_with_caps_or_log("linux", linux_caps,
				sizeof(linux_caps) / sizeof(linux_caps[0]),
				linux_ok, sizeof(linux_ok) - 1,
				linux_fail, sizeof(linux_fail) - 1);
	launch_or_log("/bin/rv64-syscall-smoke", syscall_ok,
		      sizeof(syscall_ok) - 1, syscall_fail,
		      sizeof(syscall_fail) - 1);
	launch_or_log("/bin/musl-hello", hello_ok, sizeof(hello_ok) - 1,
		      hello_fail, sizeof(hello_fail) - 1);

	log_line(done, sizeof(done) - 1);
	return 0;
}
