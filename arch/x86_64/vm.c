#include <arch/vm.h>
#include "console.h"
#include "pmm.h"

enum {
	PTE_PRESENT = 1 << 0,
	PTE_WRITABLE = 1 << 1,
	PTE_USER = 1 << 2,
	PTE_LARGE = 1 << 7,
	PAGE_ENTRIES = 512,
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

void arch_vm_activate(const struct arch_vm_space *space)
{
	__asm__ volatile ("movq %0, %%cr3" : : "r"(space->cr3) : "memory");
}
