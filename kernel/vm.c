#include "console.h"
#include "pmm.h"
#include "slab.h"
#include "spinlock.h"
#include "tree.h"
#include "vm.h"
#include <arch/smp.h>

static struct vm_space kernel_space;
static struct u64_tree spaces_by_id;
static u32 next_space_id = 1;
static struct spinlock vm_spaces_lock = SPINLOCK_INIT("vm-spaces");

void vm_init(void)
{
	arch_vm_kernel_space_init(&kernel_space.arch);
	u64_tree_init(&spaces_by_id);
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
	struct vm_space *space = slab_zalloc(sizeof(*space));

	if (space == 0) {
		spin_unlock_irqrestore(&vm_spaces_lock, flags);
		console_printf("vm: space alloc failed for %s\n", owner);
		return 0;
	}

	if (arch_vm_space_init(&space->arch) != 0) {
		slab_free(space);
		spin_unlock_irqrestore(&vm_spaces_lock, flags);
		return 0;
	}
	const u64 lapic = arch_smp_lapic_address();
	if (lapic != 0 &&
	    arch_vm_map_page(&space->arch, lapic, lapic, 1, 0) != 0) {
		slab_free(space);
		spin_unlock_irqrestore(&vm_spaces_lock, flags);
		return 0;
	}

	space->id = next_space_id++;
	if (space->id == 0) {
		space->id = next_space_id++;
	}
	space->owner = owner;
	if (u64_tree_insert_node(&spaces_by_id, &space->id_node,
				 space->id, (u64)space) != 0) {
		arch_vm_space_destroy(&space->arch);
		slab_free(space);
		spin_unlock_irqrestore(&vm_spaces_lock, flags);
		console_printf("vm: space registry failed for %s\n", owner);
		return 0;
	}
	console_printf("vm: create space id=%u owner=%s root=%p\n",
		       space->id, owner,
		       (const void *)arch_vm_root(&space->arch));
	spin_unlock_irqrestore(&vm_spaces_lock, flags);
	return space;
}

void vm_rpc_destroy_space(struct vm_space *space)
{
	if (space == 0 || space == &kernel_space) {
		return;
	}

	const u64 flags = spin_lock_irqsave(&vm_spaces_lock);
	if (space->id_node.value != 0) {
		u64_tree_remove_node(&spaces_by_id, &space->id_node);
	}
	spin_unlock_irqrestore(&vm_spaces_lock, flags);

	console_printf("vm: destroy space id=%u owner=%s\n", space->id,
		       space->owner);
	arch_vm_space_destroy(&space->arch);
	space->id = 0;
	space->owner = 0;
	slab_free(space);
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
		const u64 phys =
			arch_vm_translate_user(&space->arch, current, 0);
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
		const u64 phys =
			arch_vm_translate_user(&space->arch, current, 1);
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
		struct vm_frame frame = vm_rpc_alloc_frame();

		if (frame.addr == 0) {
			(void)vm_unmap_user_range(space, start, page - start);
			return -1;
		}

		mem_zero((u8 *)frame.addr, VM_PAGE_SIZE);
		if (vm_map_user_page(space, page, frame, writable) != 0) {
			vm_rpc_free_frame(frame);
			(void)vm_unmap_user_range(space, start, page - start);
			return -1;
		}
	}

	return 0;
}

int vm_protect_user_range(struct vm_space *space, u64 vaddr, u64 len,
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
		if (arch_vm_protect_user_page(&space->arch, page, writable) != 0) {
			return -1;
		}
	}

	return 0;
}

int vm_unmap_user_range(struct vm_space *space, u64 vaddr, u64 len)
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
		const u64 phys = arch_vm_unmap_user_page(&space->arch, page);

		if (phys == 0) {
			continue;
		}
		vm_rpc_free_frame((struct vm_frame){ .addr = phys });
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
		const u64 src_phys =
			arch_vm_translate_user(&src->arch, page, 0);

		if (frame.addr == 0 || src_phys == 0) {
			if (frame.addr != 0) {
				(void)arch_vm_unmap_page(&dst->arch, page);
				vm_rpc_free_frame(frame);
			}
			(void)vm_unmap_user_range(dst, start, page - start);
			return -1;
		}

		mem_copy((u8 *)frame.addr, (const u8 *)src_phys, VM_PAGE_SIZE);
	}

	return 0;
}

int vm_share_user_range(struct vm_space *dst, struct vm_space *src,
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
		const u64 src_phys =
			arch_vm_translate_user(&src->arch, page, 0);

		if (src_phys == 0 || pmm_page_retain_addr(src_phys) != 0) {
			(void)vm_unmap_user_range(dst, start, page - start);
			return -1;
		}
		if (arch_vm_map_page(&dst->arch, page, src_phys, writable, 1) != 0) {
			vm_rpc_free_frame((struct vm_frame){ .addr = src_phys });
			(void)vm_unmap_user_range(dst, start, page - start);
			return -1;
		}
	}

	return 0;
}

int vm_share_cow_user_range(struct vm_space *dst, struct vm_space *src,
			    u64 vaddr, u64 len)
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
		const u64 src_phys =
			arch_vm_translate_user(&src->arch, page, 0);

		if (src_phys == 0 || pmm_page_retain_addr(src_phys) != 0) {
			(void)vm_unmap_user_range(dst, start, page - start);
			return -1;
		}
		if (arch_vm_protect_user_page(&src->arch, page, 0) != 0 ||
		    arch_vm_map_page(&dst->arch, page, src_phys, 0, 1) != 0) {
			vm_rpc_free_frame((struct vm_frame){ .addr = src_phys });
			(void)vm_unmap_user_range(dst, start, page - start);
			return -1;
		}
	}

	return 0;
}

int vm_cow_user_page(struct vm_space *space, u64 vaddr)
{
	const u64 page = align_down(vaddr, VM_PAGE_SIZE);
	const u64 old_phys = arch_vm_translate_user(&space->arch, page, 0);
	struct vm_frame frame;

	if (space == 0 || old_phys == 0) {
		return -1;
	}
	frame = vm_rpc_alloc_frame();
	if (frame.addr == 0) {
		return -1;
	}
	mem_copy((u8 *)frame.addr, (const u8 *)old_phys, VM_PAGE_SIZE);
	if (arch_vm_map_page(&space->arch, page, frame.addr, 1, 1) != 0) {
		vm_rpc_free_frame(frame);
		return -1;
	}
	vm_rpc_free_frame((struct vm_frame){ .addr = old_phys });
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
