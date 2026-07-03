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

int main(void)
{
	const char launching[] = "init: launching servers\n";
	const char attenuated[] = "init: bad cap denied\n";
	const char names_ready[] = "init: names ready\n";
	struct bunix_msg ping_message = {
		.protocol = BUNIX_PROTO_PING,
		.type = 1,
		.sender = 0,
		.reply = 0,
		.words = { 0x2a, 0, 0, 0 },
	};
	struct bunix_msg ping_reply;
	u64 console;
	u64 vm;
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
