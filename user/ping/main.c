#include <bunix/syscall.h>

int main(void)
{
	const char one[] = "ping: one\n";
	const char two[] = "ping: two\n";
	struct bunix_msg message;
	const u64 port = (u64)bunix_port_create("ping");
	const u64 vm = (u64)bunix_name_lookup("vm");
	const u64 tick = bunix_timer_ticks();

	bunix_name_register("ping");
	bunix_ipc_recv(port, &message);
	bunix_console_write(one, sizeof(one) - 1);
	bunix_service_vm_ping(vm, message.words[0]);

	while (bunix_timer_ticks() < tick + 8) {
	}

	bunix_console_write(two, sizeof(two) - 1);

	return 0;
}
