#include "../../kernel/console.h"
#include "../../kernel/vm.h"

void vm_server_start(void)
{
	const u64 total = vm_rpc_total_frames();
	const u64 free_before = vm_rpc_free_frames();
	const struct vm_frame probe = vm_rpc_alloc_frame();
	const u64 free_after = vm_rpc_free_frames();

	console_printf("vm-server: memory authority online total=%u free=%u\n",
		       (u32)total, (u32)free_before);
	console_printf("vm-server: rpc alloc_frame addr=%p free=%u\n",
		       (const void *)probe.addr, (u32)free_after);
	vm_rpc_free_frame(probe);
	console_printf("vm-server: rpc free_frame free=%u\n",
		       (u32)vm_rpc_free_frames());
}
