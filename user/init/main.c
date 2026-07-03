#include <bunix/syscall.h>

int main(void)
{
	const char launching[] = "init: launching servers\n";
	struct bunix_msg ping_message = {
		.type = 1,
		.sender = 0,
		.reply = 0,
		.words = { 0x2a, 0, 0, 0 },
	};
	struct bunix_msg ping_reply;
	const u64 hello_caps[] = { BUNIX_HANDLE_CONSOLE };
	const u64 ping_caps[] = { BUNIX_HANDLE_CONSOLE, BUNIX_HANDLE_VM };

	bunix_console_write(launching, sizeof(launching) - 1);
	bunix_launch_module_with_caps("hello", hello_caps,
				      sizeof(hello_caps) / sizeof(hello_caps[0]));
	const u64 ping_port =
		(u64)bunix_launch_module_with_caps("ping", ping_caps,
						   sizeof(ping_caps) /
						   sizeof(ping_caps[0]));
	bunix_ipc_call(ping_port, &ping_message, &ping_reply);

	return 0;
}
