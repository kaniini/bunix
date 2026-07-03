#include "console.h"
#include "pmm.h"
#include "vm.h"

void vm_init(u64 multiboot_info)
{
	pmm_init(multiboot_info);
}

struct vm_frame vm_rpc_alloc_frame(void)
{
	struct pmm_page *page = pmm_page_alloc();

	if (page == 0) {
		return (struct vm_frame){ .addr = 0 };
	}

	return (struct vm_frame){ .addr = pmm_page_addr(page) };
}

void vm_rpc_free_frame(struct vm_frame frame)
{
	pmm_page_free_addr(frame.addr);
}

u64 vm_rpc_total_frames(void)
{
	return pmm_total_page_count();
}

u64 vm_rpc_free_frames(void)
{
	return pmm_free_page_count();
}

void vm_self_test(void)
{
	struct vm_frame a = vm_rpc_alloc_frame();
	struct vm_frame b = vm_rpc_alloc_frame();

	console_printf("vm: selftest alloc a=%p b=%p free=%u\n",
		       (const void *)a.addr,
		       (const void *)b.addr,
		       (u32)vm_rpc_free_frames());

	vm_rpc_free_frame(b);
	vm_rpc_free_frame(a);

	console_printf("vm: selftest free=%u\n", (u32)vm_rpc_free_frames());
}
