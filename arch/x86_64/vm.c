#include <arch/vm.h>
#include "console.h"
#include "pmm.h"

enum {
	PTE_PRESENT = 1 << 0,
	PTE_WRITABLE = 1 << 1,
	PTE_USER = 1 << 2,
	PTE_LARGE = 1 << 7,
	PAGE_ENTRIES = 512,
	PAGE_SIZE = 0x1000,
	LARGE_PAGE_SIZE = 0x200000,
};

extern u64 arch_boot_pml4[];

static void zero_page(u64 phys)
{
	u64 *page = (u64 *)phys;

	for (u32 i = 0; i < PAGE_ENTRIES; i++) {
		page[i] = 0;
	}
}

static u64 alloc_table(void)
{
	struct pmm_page *page = pmm_page_alloc();
	const u64 phys = pmm_page_addr(page);

	if (phys == 0) {
		return 0;
	}

	zero_page(phys);
	return phys;
}

static u64 table_flags(u32 writable, u32 user)
{
	u64 flags = PTE_PRESENT;

	if (writable) {
		flags |= PTE_WRITABLE;
	}
	if (user) {
		flags |= PTE_USER;
	}

	return flags;
}

static int ensure_table(u64 *parent, u64 index, u32 writable, u32 user,
			u64 *table_phys)
{
	if ((parent[index] & PTE_PRESENT) == 0) {
		const u64 phys = alloc_table();
		if (phys == 0) {
			return -1;
		}

		parent[index] = phys | table_flags(writable, user);
		*table_phys = phys;
		return 0;
	}

	if (parent[index] & PTE_LARGE) {
		return -1;
	}

	parent[index] |= table_flags(writable, user);
	*table_phys = parent[index] & ~0xfffull;
	return 0;
}

static int split_large_page(u64 *pd, u64 pd_index)
{
	const u64 entry = pd[pd_index];

	if ((entry & PTE_PRESENT) == 0 || (entry & PTE_LARGE) == 0) {
		return 0;
	}

	const u64 pt_phys = alloc_table();
	if (pt_phys == 0) {
		return -1;
	}

	u64 *pt = (u64 *)pt_phys;
	const u64 base = entry & ~((u64)LARGE_PAGE_SIZE - 1);
	const u64 flags = entry & 0xfff;

	for (u64 i = 0; i < PAGE_ENTRIES; i++) {
		pt[i] = (base + i * PAGE_SIZE) |
			((flags & ~((u64)PTE_LARGE)) | PTE_PRESENT);
	}

	pd[pd_index] = pt_phys | ((flags & ~((u64)PTE_LARGE)) | PTE_PRESENT);
	return 0;
}

void arch_vm_kernel_space_init(struct arch_vm_space *space)
{
	space->cr3 = (u64)arch_boot_pml4;
	console_printf("arch-vm: kernel cr3=%p\n", (const void *)space->cr3);
}

int arch_vm_space_init(struct arch_vm_space *space)
{
	const u64 pml4_phys = alloc_table();
	const u64 pdpt_phys = alloc_table();
	const u64 pd_phys = alloc_table();

	if (pml4_phys == 0 || pdpt_phys == 0 || pd_phys == 0) {
		console_printf("arch-vm: failed to allocate page tables\n");
		return -1;
	}

	u64 *pml4 = (u64 *)pml4_phys;
	u64 *pdpt = (u64 *)pdpt_phys;
	u64 *pd = (u64 *)pd_phys;

	pml4[0] = pdpt_phys | PTE_PRESENT | PTE_WRITABLE | PTE_USER;
	pdpt[0] = pd_phys | PTE_PRESENT | PTE_WRITABLE | PTE_USER;

	for (u64 i = 0; i < PAGE_ENTRIES; i++) {
		pd[i] = (i * LARGE_PAGE_SIZE) | PTE_PRESENT | PTE_WRITABLE |
			PTE_USER | PTE_LARGE;
	}

	space->cr3 = pml4_phys;
	console_printf("arch-vm: create space cr3=%p\n", (const void *)space->cr3);
	return 0;
}

int arch_vm_map_page(struct arch_vm_space *space, u64 vaddr, u64 phys,
		     u32 writable, u32 user)
{
	u64 *pml4 = (u64 *)space->cr3;
	u64 pdpt_phys;
	u64 pd_phys;
	u64 pt_phys;
	const u64 pml4_index = (vaddr >> 39) & 0x1ff;
	const u64 pdpt_index = (vaddr >> 30) & 0x1ff;
	const u64 pd_index = (vaddr >> 21) & 0x1ff;
	const u64 pt_index = (vaddr >> 12) & 0x1ff;

	if ((vaddr & (PAGE_SIZE - 1)) != 0 || (phys & (PAGE_SIZE - 1)) != 0) {
		return -1;
	}

	if (ensure_table(pml4, pml4_index, 1, user, &pdpt_phys) != 0) {
		return -1;
	}

	u64 *pdpt = (u64 *)pdpt_phys;
	if (ensure_table(pdpt, pdpt_index, 1, user, &pd_phys) != 0) {
		return -1;
	}

	u64 *pd = (u64 *)pd_phys;
	if (split_large_page(pd, pd_index) != 0) {
		return -1;
	}
	if (ensure_table(pd, pd_index, 1, user, &pt_phys) != 0) {
		return -1;
	}

	u64 *pt = (u64 *)pt_phys;
	pt[pt_index] = phys | table_flags(writable, user);

	__asm__ volatile ("invlpg (%0)" : : "r"(vaddr) : "memory");
	return 0;
}

u64 arch_vm_translate(const struct arch_vm_space *space, u64 vaddr, u32 write)
{
	const u64 *pml4 = (const u64 *)space->cr3;
	const u64 pml4_index = (vaddr >> 39) & 0x1ff;
	const u64 pdpt_index = (vaddr >> 30) & 0x1ff;
	const u64 pd_index = (vaddr >> 21) & 0x1ff;
	const u64 pt_index = (vaddr >> 12) & 0x1ff;
	const u64 page_offset = vaddr & (PAGE_SIZE - 1);
	const u64 large_offset = vaddr & (LARGE_PAGE_SIZE - 1);
	u64 entry;

	entry = pml4[pml4_index];
	if ((entry & PTE_PRESENT) == 0 || (write && (entry & PTE_WRITABLE) == 0)) {
		return 0;
	}

	const u64 *pdpt = (const u64 *)(entry & ~0xfffull);
	entry = pdpt[pdpt_index];
	if ((entry & PTE_PRESENT) == 0 || (write && (entry & PTE_WRITABLE) == 0)) {
		return 0;
	}

	const u64 *pd = (const u64 *)(entry & ~0xfffull);
	entry = pd[pd_index];
	if ((entry & PTE_PRESENT) == 0 || (write && (entry & PTE_WRITABLE) == 0)) {
		return 0;
	}
	if ((entry & PTE_LARGE) != 0) {
		return (entry & ~((u64)LARGE_PAGE_SIZE - 1)) + large_offset;
	}

	const u64 *pt = (const u64 *)(entry & ~0xfffull);
	entry = pt[pt_index];
	if ((entry & PTE_PRESENT) == 0 || (write && (entry & PTE_WRITABLE) == 0)) {
		return 0;
	}

	return (entry & ~0xfffull) + page_offset;
}

void arch_vm_activate(const struct arch_vm_space *space)
{
	__asm__ volatile ("movq %0, %%cr3" : : "r"(space->cr3) : "memory");
}
