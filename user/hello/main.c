#include <bunix/syscall.h>

int main(void)
{
	const char message[] = "hello: world <3\n";
	const char denied[] = "hello: vm denied\n";
	const char allowed[] = "hello: vm allowed\n";
	const struct bunix_msg vm_message = {
		.protocol = BUNIX_PROTO_VM,
		.type = 1,
		.sender = 0,
		.reply = 0,
		.words = { 0, 0, 0, 0 },
	};
	const u64 tick = bunix_timer_ticks();

	while (bunix_timer_ticks() < tick + 20) {
	}

	bunix_console_write(message, sizeof(message) - 1);
	if (bunix_ipc_send(BUNIX_HANDLE_VM, &vm_message) != 0) {
		bunix_console_write(denied, sizeof(denied) - 1);
	} else {
		bunix_console_write(allowed, sizeof(allowed) - 1);
	}

	return 0;
}
