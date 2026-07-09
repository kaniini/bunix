#include <bunix/libbunix.h>

static u64 text_len(const char *text)
{
	u64 len = 0;

	while (text != 0 && text[len] != '\0') {
		len++;
	}
	return len;
}

static void log_text(const char *text)
{
	bunix_console_log(text, text_len(text));
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

	if (bunix_ipc_call(bunix_handle_find(BUNIX_CAP_NAME),
			   &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return 0;
	}
	return reply.cap;
}

static int call_denied(u64 port, unsigned int protocol, unsigned int type,
		       u64 word0, u64 word1, u64 word2, u64 word3)
{
	struct bunix_msg request = {
		.protocol = protocol,
		.type = type,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { word0, word1, word2, word3 },
	};
	struct bunix_msg reply;

	if (port == 0 ||
	    bunix_ipc_call(port, &request, &reply) != 0) {
		return -1;
	}
	return reply.words[0] != 0 ? 0 : -1;
}

static int path_call_denied(u64 port, unsigned int protocol, unsigned int type,
			    const char *path,
			    u64 word0, u64 word1, u64 word2)
{
	struct bunix_msg reply;

	if (port == 0 ||
	    bunix_ipc_call_path(port, protocol, type, path,
				word0, word1, word2, &reply) != 0) {
		return -1;
	}
	if (reply.cap != 0) {
		bunix_handle_close(reply.cap);
	}
	return reply.words[0] != 0 ? 0 : -1;
}

static int vfs_path_call_status(u64 vfs, unsigned int type, const char *path,
				u64 word3)
{
	const char cwd[] = "/";
	const u64 cwd_len = sizeof(cwd);
	const u64 path_len = text_len(path) + 1;
	const long buffer = bunix_buffer_create(cwd_len + path_len);
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = type,
		.cap_rights = BUNIX_RIGHT_RECV,
		.cap = buffer > 0 ? (u64)buffer : 0,
		.words = { cwd_len, path_len, 0, word3 },
	};
	struct bunix_msg reply;
	u64 status = (u64)-1;

	if (buffer > 0 &&
	    bunix_buffer_write((u64)buffer, 0, cwd, cwd_len) == 0 &&
	    bunix_buffer_write((u64)buffer, cwd_len, path, path_len) == 0 &&
	    bunix_ipc_call(vfs, &request, &reply) == 0) {
		status = reply.words[0];
	}
	if (buffer > 0) {
		bunix_handle_close((u64)buffer);
	}
	return (int)status;
}

static long vfs_open_path(u64 vfs, const char *path, u64 *handle)
{
	const char cwd[] = "/";
	const u64 cwd_len = sizeof(cwd);
	const u64 path_len = text_len(path) + 1;
	const long buffer = bunix_buffer_create(cwd_len + path_len);
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_OPEN_BUFFER,
		.cap_rights = BUNIX_RIGHT_RECV,
		.cap = buffer > 0 ? (u64)buffer : 0,
		.words = { cwd_len, path_len, 0, 0 },
	};
	struct bunix_msg reply;

	if (buffer <= 0 ||
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
	*handle = reply.words[1];
	return 0;
}

static long vfs_read_handle(u64 vfs, u64 handle)
{
	const long buffer = bunix_buffer_create(64);
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_READ_FILE_BUFFER,
		.cap_rights = BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP,
		.cap = buffer > 0 ? (u64)buffer : 0,
		.words = { handle, 0, 63, 0 },
	};
	struct bunix_msg reply;
	long result = -1;

	if (buffer > 0 &&
	    bunix_ipc_call(vfs, &request, &reply) == 0 &&
	    reply.words[0] == 0) {
		result = 0;
	}
	if (buffer > 0) {
		bunix_handle_close((u64)buffer);
	}
	return result;
}

static long vfs_close_handle(u64 vfs, u64 handle)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_CLOSE,
		.words = { handle, 0, 0, 0 },
	};
	struct bunix_msg reply;

	return bunix_ipc_call(vfs, &request, &reply) == 0 &&
	       reply.words[0] == 0 ? 0 : -1;
}

static int run_vfs_authority_test(u64 vfs)
{
	u64 own = 0;

	if (vfs_open_path(vfs, "/hello.txt", &own) != 0 ||
	    vfs_read_handle(vfs, own) != 0) {
		return 1;
	}
	log_text("vfs-auth-test: own handle ok\n");

	for (u64 handle = 1; handle <= 32; handle++) {
		if (handle == own) {
			continue;
		}
		if (vfs_read_handle(vfs, handle) == 0) {
			log_text("vfs-auth-test: read steal succeeded\n");
			return 1;
		}
	}
	log_text("vfs-auth-test: read denied\n");

	for (u64 handle = 1; handle <= 32; handle++) {
		if (handle == own) {
			continue;
		}
		if (vfs_close_handle(vfs, handle) == 0) {
			log_text("vfs-auth-test: close steal succeeded\n");
			return 1;
		}
	}
	log_text("vfs-auth-test: close denied\n");

	if (vfs_read_handle(vfs, own) != 0 ||
	    vfs_close_handle(vfs, own) != 0) {
		return 1;
	}
	log_text("vfs-auth-test: ok\n");
	return 0;
}

static int run_vfs_mount_authority_test(u64 vfs)
{
	if (path_call_denied(vfs, BUNIX_PROTO_VFS,
			     BUNIX_VFS_MOUNT_BUFFER, "/tmp/evil",
			     BUNIX_SERVICE_TMPFS, 0, 0) != 0) {
		log_text("vfs-mount-auth-test: mount succeeded\n");
		return 1;
	}
	log_text("vfs-mount-auth-test: mount denied\n");

	if (path_call_denied(vfs, BUNIX_PROTO_VFS,
			     BUNIX_VFS_UNMOUNT_BUFFER, "/tmp",
			     0, 0, 0) != 0) {
		log_text("vfs-mount-auth-test: unmount succeeded\n");
		return 1;
	}
	log_text("vfs-mount-auth-test: unmount denied\n");

	if (path_call_denied(vfs, BUNIX_PROTO_VFS,
			     BUNIX_VFS_RESOLVE_MOUNT_BUFFER, "/",
			     0, 0, 0) != 0) {
		log_text("vfs-mount-auth-test: resolve succeeded\n");
		return 1;
	}
	log_text("vfs-mount-auth-test: resolve denied\n");

	if (path_call_denied(vfs, BUNIX_PROTO_VFS,
			     BUNIX_VFS_PIN_ROUTE_BUFFER, "/hello.txt",
			     0, 0, 0) != 0) {
		log_text("vfs-mount-auth-test: pin succeeded\n");
		return 1;
	}
	log_text("vfs-mount-auth-test: pin denied\n");

	if (call_denied(vfs, BUNIX_PROTO_VFS,
			BUNIX_VFS_UNPIN_ROUTE, 1, 0, 0, 0) != 0) {
		log_text("vfs-mount-auth-test: unpin succeeded\n");
		return 1;
	}
	log_text("vfs-mount-auth-test: unpin denied\n");

	log_text("vfs-mount-auth-test: ok\n");
	return 0;
}

static int run_vfs_subject_authority_test(u64 vfs)
{
	const u64 forged_root = (u64)0700 << 32;

	if (vfs_path_call_status(vfs, BUNIX_VFS_MKDIR_BUFFER,
				 "/root/forged-root",
				 forged_root) == 0) {
		log_text("vfs-subject-auth-test: forged root succeeded\n");
		return 1;
	}
	log_text("vfs-subject-auth-test: forged root denied\n");
	log_text("vfs-subject-auth-test: ok\n");
	return 0;
}

int main(void)
{
	const char user_denied[] = "mgmt-test: user denied\n";
	const char proc_denied[] = "mgmt-test: proc denied\n";
	const char linux_denied[] = "mgmt-test: linux denied\n";
	const char ok[] = "mgmt-test: ok\n";
	const u64 user = resolve_service(BUNIX_SERVICE_USER, BUNIX_RIGHT_SEND);
	const u64 proc = resolve_service(BUNIX_SERVICE_PROC, BUNIX_RIGHT_SEND);
	const u64 linux = resolve_service(BUNIX_SERVICE_LINUX,
					  BUNIX_RIGHT_SEND);
	const u64 vfs = bunix_handle_find(BUNIX_CAP_VFS);

	if (vfs != 0) {
		if (bunix_cmdline_has("vfs-mount-auth-test") > 0) {
			return run_vfs_mount_authority_test(vfs);
		}
		if (bunix_cmdline_has("vfs-subject-auth-test") > 0) {
			return run_vfs_subject_authority_test(vfs);
		}
		return run_vfs_authority_test(vfs);
	}

	if (call_denied(user, BUNIX_PROTO_USER,
			BUNIX_USER_REGISTER_PROCESS, 0x55550001, 0, 0, 0) != 0) {
		return 1;
	}
	bunix_console_log(user_denied, sizeof(user_denied) - 1);

	if (call_denied(proc, BUNIX_PROTO_PROC,
			BUNIX_PROC_EXIT, 0x55550002, 0, 1, 0) != 0) {
		return 1;
	}
	bunix_console_log(proc_denied, sizeof(proc_denied) - 1);

	if (call_denied(linux, BUNIX_PROTO_LINUX,
			BUNIX_LINUX_REGISTER_PROCESS, 0x55550003, 0, 0, 0) != 0) {
		return 1;
	}
	bunix_console_log(linux_denied, sizeof(linux_denied) - 1);

	bunix_console_log(ok, sizeof(ok) - 1);
	return 0;
}
