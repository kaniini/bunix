#include "../../kernel/console.h"
#include "../../kernel/ipc.h"
#include "../../kernel/vm.h"
#include "vm_server.h"

static u32 granted_spaces;
static struct ipc_port *vm_port;

static struct vm_space *grant_space(const char *owner)
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

void vm_server_init(void)
{
	vm_port = ipc_port_create("vm");
}

struct vm_space *vm_server_bootstrap_space(const char *owner)
{
	return grant_space(owner);
}

struct vm_space *vm_server_rpc_create_space(const char *owner)
{
	struct ipc_message request = {
		.type = VM_RPC_CREATE_SPACE,
		.sender = 0,
		.reply_port = 0,
		.words = { (u64)owner, 0, 0, 0 },
	};
	struct ipc_message reply;

	if (ipc_call_kernel(vm_port, &request, &reply) != 0) {
		console_printf("vm-server: create_space rpc failed owner=%s\n", owner);
		return 0;
	}

	return (struct vm_space *)reply.words[0];
}

static void vm_server_handle_message(const struct ipc_message *message)
{
	struct ipc_message reply = {
		.type = message->type,
		.sender = 0,
		.reply_port = 0,
		.words = { 0, 0, 0, 0 },
	};

	switch (message->type) {
	case VM_IPC_EVENT_PING:
		console_printf("vm-server: ipc event type=%u sender=%u word0=0x%x\n",
			       message->type, message->sender, (u32)message->words[0]);
		break;
	case VM_RPC_CREATE_SPACE: {
		const char *owner = (const char *)message->words[0];
		struct vm_space *space = grant_space(owner);
		reply.words[0] = (u64)space;
		reply.words[1] = space != 0 ? space->id : 0;
		if (message->reply_port != 0) {
			ipc_send(message->reply_port, &reply);
		}
		break;
	}
	case VM_RPC_ALLOC_FRAME: {
		struct vm_frame frame = vm_rpc_alloc_frame();
		reply.words[0] = frame.addr;
		if (message->reply_port != 0) {
			ipc_send(message->reply_port, &reply);
		}
		break;
	}
	case VM_RPC_FREE_FRAME:
		vm_rpc_free_frame((struct vm_frame){ .addr = message->words[0] });
		if (message->reply_port != 0) {
			ipc_send(message->reply_port, &reply);
		}
		break;
	default:
		console_printf("vm-server: unknown ipc type=%u sender=%u\n",
			       message->type, message->sender);
		break;
	}
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

	if (vm_port == 0) {
		return;
	}

	for (;;) {
		if (ipc_recv(vm_port, &message) == 0) {
			vm_server_handle_message(&message);
		}
	}
}
