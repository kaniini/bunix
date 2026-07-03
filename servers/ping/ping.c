#include "../../kernel/console.h"
#include "../../kernel/ipc.h"
#include "../../kernel/timer.h"
#include "../vm/vm_server.h"

void ping_server_start(void)
{
	struct ipc_port *vm_port;
	struct ipc_message message = {
		.type = VM_IPC_EVENT_PING,
		.sender = 0,
		.words = { 0x2a, 0, 0, 0 },
	};

	console_printf("ping: one\n");
	vm_port = ipc_port_find("vm");
	if (vm_port != 0) {
		ipc_send(vm_port, &message);
	}

	const u64 start = timer_ticks();
	while (timer_ticks() == start) {
		__asm__ volatile ("pause");
	}

	console_printf("ping: two\n");
}
