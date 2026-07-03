#include "console.h"
#include "pmm.h"
#include "vm.h"

enum {
	MAX_VM_SPACES = 32,
};

static struct vm_space kernel_space;
static struct vm_space spaces[MAX_VM_SPACES];
static u32 next_space_id = 1;

void vm_init(u64 multiboot_info)
{
	pmm_init(multiboot_info);
	arch_vm_kernel_space_init(&kernel_space.arch);
	kernel_space.id = 0;
	kernel_space.owner = "kernel";
}

struct vm_space *vm_kernel_space(void)
{
	return &kernel_space;
}

struct vm_space *vm_rpc_create_space(const char *owner)
{
	for (u32 i = 0; i < MAX_VM_SPACES; i++) {
		if (spaces[i].id != 0) {
			continue;
		}

		if (arch_vm_space_init(&spaces[i].arch) != 0) {
			return 0;
		}

		spaces[i].id = next_space_id++;
		spaces[i].owner = owner;
		console_printf("vm: create space id=%u owner=%s cr3=%p\n",
			       spaces[i].id, owner,
			       (const void *)spaces[i].arch.cr3);
		return &spaces[i];
	}

	console_printf("vm: space table full for %s\n", owner);
	return 0;
}

void vm_rpc_activate_space(struct vm_space *space)
{
	if (space == 0) {
		return;
	}

	arch_vm_activate(&space->arch);
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

int vm_map_user_page(struct vm_space *space, u64 vaddr, struct vm_frame frame,
		     u32 writable)
{
	if (space == 0 || frame.addr == 0) {
		return -1;
	}

	return arch_vm_map_page(&space->arch, vaddr, frame.addr, writable, 1);
}

struct vm_frame vm_alloc_user_page(struct vm_space *space, u64 vaddr,
				   u32 writable)
{
	struct vm_frame frame = vm_rpc_alloc_frame();

	if (frame.addr == 0) {
		return frame;
	}

	if (vm_map_user_page(space, vaddr, frame, writable) != 0) {
		vm_rpc_free_frame(frame);
		return (struct vm_frame){ .addr = 0 };
	}

	return frame;
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
