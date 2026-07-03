#include <bunix/syscall.h>

int main(void)
{
	const char online[] = "ping: online\n";
	const char heartbeat[] = "ping: heartbeat\n";
	u64 sequence = 1;
	const u64 interval_ns = 2000000000ULL;

	bunix_console_write(online, sizeof(online) - 1);

	for (;;) {
		struct bunix_msg vm_message = {
			.protocol = BUNIX_PROTO_VM,
			.type = 1,
			.sender = 0,
			.reply = 0,
			.words = { 0, 0, 0, 0 },
		};
		bunix_sleep_ns(interval_ns);
		bunix_console_write(heartbeat, sizeof(heartbeat) - 1);
		vm_message.words[0] = sequence++;
		vm_message.words[1] = bunix_clock_monotonic_ns();
		bunix_ipc_send(BUNIX_HANDLE_VM, &vm_message);
	}
}
