#include <bunix/syscall.h>

int main(void)
{
	const char one[] = "ping: one\n";
	const char two[] = "ping: two\n";
	const char closed[] = "ping: vm closed\n";
	const char close_failed[] = "ping: vm close failed\n";
	const char still_open[] = "ping: vm still open\n";
	struct bunix_msg message;
	struct bunix_msg vm_message = {
		.type = 1,
		.sender = 0,
		.reply = 0,
		.words = { 0, 0, 0, 0 },
	};
	struct bunix_msg reply = {
		.type = 2,
		.sender = 0,
		.reply = 0,
		.words = { 0, 0, 0, 0 },
	};
	const u64 tick = bunix_timer_ticks();

	bunix_ipc_recv(BUNIX_HANDLE_SELF, &message);
	bunix_console_write(one, sizeof(one) - 1);
	vm_message.words[0] = message.words[0];
	bunix_ipc_send(BUNIX_HANDLE_VM, &vm_message);
	if (bunix_handle_close(BUNIX_HANDLE_VM) != 0) {
		bunix_console_write(close_failed, sizeof(close_failed) - 1);
	} else if (bunix_ipc_send(BUNIX_HANDLE_VM, &vm_message) != 0) {
		bunix_console_write(closed, sizeof(closed) - 1);
	} else {
		bunix_console_write(still_open, sizeof(still_open) - 1);
	}

	while (bunix_timer_ticks() < tick + 8) {
	}

	bunix_console_write(two, sizeof(two) - 1);
	bunix_ipc_send(message.reply, &reply);

	return 0;
}
