#include <bunix/syscall.h>

int main(void)
{
	const char launching[] = "init: launching servers\n";
	const char attenuated[] = "init: bad cap denied\n";
	struct bunix_msg ping_message = {
		.protocol = BUNIX_PROTO_PING,
		.type = 1,
		.sender = 0,
		.reply = 0,
		.words = { 0x2a, 0, 0, 0 },
	};
	struct bunix_msg ping_reply;
	const struct bunix_launch_cap bad_caps[] = {
		{ BUNIX_HANDLE_CONSOLE, BUNIX_RIGHT_SEND | BUNIX_RIGHT_RECV, 0 },
	};
	const struct bunix_launch_cap hello_caps[] = {
		{ BUNIX_HANDLE_CONSOLE, BUNIX_RIGHT_SEND, 0 },
	};
	const struct bunix_launch_cap ping_caps[] = {
		{ BUNIX_HANDLE_CONSOLE, BUNIX_RIGHT_SEND, 0 },
		{ BUNIX_HANDLE_VM, BUNIX_RIGHT_SEND, 0 },
	};

	bunix_console_write(launching, sizeof(launching) - 1);
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
