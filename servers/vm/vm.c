#include "../../kernel/console.h"
#include "../../kernel/ipc.h"
#include "../../kernel/vm.h"
#include "vm_server.h"

static u32 granted_spaces;
static struct ipc_port *vm_port;

struct vm_space *vm_server_create_space(const char *owner)
{
	struct vm_space *space = vm_rpc_create_space(owner);

	if (space == 0) {
		console_printf("vm-server: failed to grant space owner=%s\n", owner);
		return 0;
	}

	granted_spaces++;
	console_printf("vm-server: grant_space owner=%s id=%u grants=%u\n",
		       owner, space->id, granted_spaces);
	return space;
}

void vm_server_start(void)
{
	struct ipc_message message;
	const u64 total = vm_rpc_total_frames();
	const u64 free_before = vm_rpc_free_frames();
	const struct vm_frame probe = vm_rpc_alloc_frame();
	const u64 free_after = vm_rpc_free_frames();

	console_printf("vm-server: memory authority online total=%u free=%u\n",
		       (u32)total, (u32)free_before);
	console_printf("vm-server: spaces granted=%u\n", granted_spaces);
	console_printf("vm-server: rpc alloc_frame addr=%p free=%u\n",
		       (const void *)probe.addr, (u32)free_after);
	vm_rpc_free_frame(probe);
	console_printf("vm-server: rpc free_frame free=%u\n",
		       (u32)vm_rpc_free_frames());

	vm_port = ipc_port_create("vm");
	if (vm_port == 0) {
		return;
	}

	if (ipc_recv(vm_port, &message) == 0) {
		console_printf("vm-server: ipc event type=%u sender=%u word0=0x%x\n",
			       message.type, message.sender, (u32)message.words[0]);
	}
}
