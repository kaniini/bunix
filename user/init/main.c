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
	u64 ping_port = 0;

	bunix_name_register("init");
	bunix_console_write(launching, sizeof(launching) - 1);
	bunix_launch_module("hello");
	bunix_launch_module("ping");
	while (ping_port == 0) {
		ping_port = (u64)bunix_port_lookup("ping");
	}
	bunix_ipc_call(ping_port, &ping_message, &ping_reply);

	return 0;
}
