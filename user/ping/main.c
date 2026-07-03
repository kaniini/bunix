#include <bunix/syscall.h>

int main(void)
{
	const char one[] = "ping: one\n";
	const char two[] = "ping: two\n";
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
	const u64 port = (u64)bunix_port_create("ping");
	const u64 vm = (u64)bunix_port_lookup("vm");
	const u64 tick = bunix_timer_ticks();

	bunix_name_register("ping");
	bunix_ipc_recv(port, &message);
	bunix_console_write(one, sizeof(one) - 1);
	vm_message.words[0] = message.words[0];
	bunix_ipc_send(vm, &vm_message);

	while (bunix_timer_ticks() < tick + 8) {
	}

	bunix_console_write(two, sizeof(two) - 1);
	bunix_ipc_send(message.reply, &reply);

	return 0;
}
