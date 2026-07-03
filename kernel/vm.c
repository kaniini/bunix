#include "console.h"
#include "pmm.h"
#include "spinlock.h"
#include "vm.h"
#include <arch/smp.h>

enum {
	MAX_VM_SPACES = 32,
};

static struct vm_space kernel_space;
static struct vm_space spaces[MAX_VM_SPACES];
static u32 next_space_id = 1;
static struct spinlock vm_spaces_lock = SPINLOCK_INIT("vm-spaces");

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
	const u64 flags = spin_lock_irqsave(&vm_spaces_lock);

	for (u32 i = 0; i < MAX_VM_SPACES; i++) {
		if (spaces[i].id != 0) {
			continue;
		}

		if (arch_vm_space_init(&spaces[i].arch) != 0) {
			spin_unlock_irqrestore(&vm_spaces_lock, flags);
			return 0;
		}
		const u64 lapic = arch_smp_lapic_address();
		if (lapic != 0 &&
		    arch_vm_map_page(&spaces[i].arch, lapic, lapic, 1, 0) != 0) {
			spin_unlock_irqrestore(&vm_spaces_lock, flags);
			return 0;
		}

		spaces[i].id = next_space_id++;
		spaces[i].owner = owner;
		console_printf("vm: create space id=%u owner=%s cr3=%p\n",
			       spaces[i].id, owner,
			       (const void *)spaces[i].arch.cr3);
		spin_unlock_irqrestore(&vm_spaces_lock, flags);
		return &spaces[i];
	}

	spin_unlock_irqrestore(&vm_spaces_lock, flags);
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

static void mem_copy(u8 *dst, const u8 *src, u64 len)
{
	for (u64 i = 0; i < len; i++) {
		dst[i] = src[i];
	}
}

static void mem_zero(u8 *dst, u64 len)
{
	for (u64 i = 0; i < len; i++) {
		dst[i] = 0;
	}
}

static u64 min_u64(u64 left, u64 right)
{
	return left < right ? left : right;
}

static u64 align_down(u64 value, u64 align)
{
	return value & ~(align - 1);
}

static u64 align_up(u64 value, u64 align)
{
	return align_down(value + align - 1, align);
}

int vm_read_user(struct vm_space *space, u64 vaddr, void *dst, u64 len)
{
	u64 done = 0;

	if (space == 0 || dst == 0 || vaddr + len < vaddr) {
		return -1;
	}

	while (done < len) {
		const u64 current = vaddr + done;
		const u64 phys = arch_vm_translate(&space->arch, current, 0);
		const u64 page_remaining =
			VM_PAGE_SIZE - (current & (VM_PAGE_SIZE - 1));
		const u64 chunk = min_u64(len - done, page_remaining);

		if (phys == 0) {
			return -1;
		}

		mem_copy((u8 *)dst + done, (const u8 *)phys, chunk);
		done += chunk;
	}

	return 0;
}

int vm_write_user(struct vm_space *space, u64 vaddr, const void *src, u64 len)
{
	u64 done = 0;

	if (space == 0 || src == 0 || vaddr + len < vaddr) {
		return -1;
	}

	while (done < len) {
		const u64 current = vaddr + done;
		const u64 phys = arch_vm_translate(&space->arch, current, 1);
		const u64 page_remaining = VM_PAGE_SIZE - (current & (VM_PAGE_SIZE - 1));
		const u64 chunk = min_u64(len - done, page_remaining);

		if (phys == 0) {
			return -1;
		}

		mem_copy((u8 *)phys, (const u8 *)src + done, chunk);
		done += chunk;
	}

	return 0;
}

int vm_map_kernel_page(u64 vaddr, u64 phys, u32 writable)
{
	return arch_vm_map_page(&kernel_space.arch, vaddr, phys, writable, 0);
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

int vm_alloc_user_range(struct vm_space *space, u64 vaddr, u64 len,
			u32 writable)
{
	if (space == 0 || len == 0 || vaddr + len < vaddr) {
		return -1;
	}

	const u64 start = align_down(vaddr, VM_PAGE_SIZE);
	const u64 end = align_up(vaddr + len, VM_PAGE_SIZE);
	if (end <= start) {
		return -1;
	}

	for (u64 page = start; page < end; page += VM_PAGE_SIZE) {
		struct vm_frame frame = vm_alloc_user_page(space, page, writable);

		if (frame.addr == 0) {
			return -1;
		}

		mem_zero((u8 *)frame.addr, VM_PAGE_SIZE);
	}

	return 0;
}

int vm_clone_user_range(struct vm_space *dst, struct vm_space *src,
			u64 vaddr, u64 len, u32 writable)
{
	if (dst == 0 || src == 0 || len == 0 || vaddr + len < vaddr) {
		return -1;
	}

	const u64 start = align_down(vaddr, VM_PAGE_SIZE);
	const u64 end = align_up(vaddr + len, VM_PAGE_SIZE);
	if (end <= start) {
		return -1;
	}

	for (u64 page = start; page < end; page += VM_PAGE_SIZE) {
		struct vm_frame frame = vm_alloc_user_page(dst, page, writable);
		const u64 src_phys = arch_vm_translate(&src->arch, page, 0);

		if (frame.addr == 0 || src_phys == 0) {
			return -1;
		}

		mem_copy((u8 *)frame.addr, (const u8 *)src_phys, VM_PAGE_SIZE);
	}

	return 0;
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
