#include <bunix/syscall.h>

int main(void)
{
	const char online[] = "ping: online\n";
	const char heartbeat[] = "ping: heartbeat\n";
	u64 sequence = 1;

	bunix_console_write(online, sizeof(online) - 1);

	for (;;) {
		struct bunix_msg vm_message = {
			.protocol = BUNIX_PROTO_VM,
			.type = 1,
			.sender = 0,
			.reply = 0,
			.words = { 0, 0, 0, 0 },
		};
		bunix_sleep_ticks(200);
		bunix_console_write(heartbeat, sizeof(heartbeat) - 1);
		vm_message.words[0] = sequence++;
		bunix_ipc_send(BUNIX_HANDLE_VM, &vm_message);
	}
}
