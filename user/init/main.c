#include <bunix/syscall.h>

static long register_service(u64 service, u64 handle)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_NAMES,
		.type = BUNIX_NAMES_REGISTER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP,
		.reply = 0,
		.cap = handle,
		.words = { service, 0, 0, 0 },
	};
	struct bunix_msg reply;

	if (bunix_ipc_call(BUNIX_HANDLE_NAMES, &request, &reply) != 0) {
		return -1;
	}

	return reply.words[0] == 0 ? 0 : -1;
}

static u64 resolve_service(u64 service, unsigned int rights)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_NAMES,
		.type = BUNIX_NAMES_RESOLVE,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { service, rights, 0, 0 },
	};
	struct bunix_msg reply;

	if (bunix_ipc_call(BUNIX_HANDLE_NAMES, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return 0;
	}

	return reply.cap;
}

static void unpack_bytes(char *out, const u64 *words, u64 len)
{
	for (u64 i = 0; i < len; i++) {
		const u64 slot = i / 8;
		const u64 shift = (i % 8) * 8;

		out[i] = (char)((words[slot] >> shift) & 0xff);
	}
}

int main(void)
{
	const char launching[] = "init: launching servers\n";
	const char attenuated[] = "init: bad cap denied\n";
	const char names_ready[] = "init: names ready\n";
	const char fs_ready[] = "init: fs ready\n";
	struct bunix_msg ping_message = {
		.protocol = BUNIX_PROTO_PING,
		.type = 1,
		.sender = 0,
		.reply = 0,
		.words = { 0x2a, 0, 0, 0 },
	};
	struct bunix_msg ping_reply;
	struct bunix_msg vfs_request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_READ,
		.sender = 0,
		.reply = 0,
		.words = { 0, 16, 0, 0 },
	};
	struct bunix_msg vfs_reply;
	char file[17];
	u64 console;
	u64 vm;
	u64 vfs = 0;
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

	const struct bunix_launch_cap hello_caps[] = {
		{ console, BUNIX_RIGHT_SEND, 0 },
	};
	const struct bunix_launch_cap ping_caps[] = {
		{ console, BUNIX_RIGHT_SEND, 0 },
		{ vm, BUNIX_RIGHT_SEND, 0 },
	};
	const struct bunix_launch_cap fs_caps[] = {
		{ console, BUNIX_RIGHT_SEND, 0 },
		{ BUNIX_HANDLE_NAMES, BUNIX_RIGHT_SEND, 0 },
	};

	bunix_launch_module_with_caps("block", fs_caps,
				      sizeof(fs_caps) / sizeof(fs_caps[0]));
	bunix_launch_module_with_caps("vfs", fs_caps,
				      sizeof(fs_caps) / sizeof(fs_caps[0]));
	while (vfs == 0) {
		vfs = resolve_service(BUNIX_SERVICE_VFS, BUNIX_RIGHT_SEND);
	}
	bunix_console_write(fs_ready, sizeof(fs_ready) - 1);
	if (bunix_ipc_call(vfs, &vfs_request, &vfs_reply) == 0 &&
	    vfs_reply.words[0] == 0 && vfs_reply.words[1] <= 16) {
		unpack_bytes(file, &vfs_reply.words[2], vfs_reply.words[1]);
		file[vfs_reply.words[1]] = '\0';
		bunix_console_write(file, vfs_reply.words[1]);
	}

	if (bunix_launch_module_with_caps("hello", bad_caps,
					  sizeof(bad_caps) /
					  sizeof(bad_caps[0])) < 0) {
		bunix_console_write(attenuated, sizeof(attenuated) - 1);
	}
	bunix_launch_module_with_caps("hello", hello_caps,
				      sizeof(hello_caps) / sizeof(hello_caps[0]));
	const u64 ping_port =
		(u64)bunix_launch_module_with_caps("ping", ping_caps,
						   sizeof(ping_caps) /
						   sizeof(ping_caps[0]));
	bunix_ipc_call(ping_port, &ping_message, &ping_reply);

	return 0;
}
