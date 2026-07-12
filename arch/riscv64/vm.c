#include <arch/boot.h>
#include <arch/vm.h>
#include "pmm.h"

enum {
	RISCV64_SATP_MODE_SV39 = 8ULL << 60,
	RISCV64_PAGE_ENTRIES = 512,
	RISCV64_PTE_TABLE_MASK = RISCV64_PTE_V,
	RISCV64_PTE_LEAF_MASK = RISCV64_PTE_R | RISCV64_PTE_W | RISCV64_PTE_X,
	RISCV64_MAX_MMIO_IDENTITY_MAPPINGS = 16,
};

struct mmio_identity_mapping {
	u64 start;
	u64 end;
};

static struct mmio_identity_mapping mmio_identity_mappings[RISCV64_MAX_MMIO_IDENTITY_MAPPINGS];
static u32 mmio_identity_mapping_count;

static u64 pte_phys(u64 pte);
static int pte_table_valid(u64 pte);
static int map_supervisor_identity_window(struct arch_vm_space *space,
					  u64 start, u64 end);
static int map_registered_mmio(struct arch_vm_space *space);

static void zero_table(u64 *table)
{
	for (u32 i = 0; i < RISCV64_PAGE_ENTRIES; i++) {
		table[i] = 0;
	}
}

static u64 *alloc_table(void)
{
	struct pmm_page *page = pmm_page_alloc();
	u64 *table;

	if (page == 0) {
		return 0;
	}

	table = (u64 *)pmm_page_addr(page);
	zero_table(table);
	return table;
}

static void free_table(u64 *table, u32 level)
{
	if (table == 0) {
		return;
	}

	if (level > 0) {
		for (u32 i = 0; i < RISCV64_PAGE_ENTRIES; i++) {
			const u64 entry = table[i];

			if (pte_table_valid(entry)) {
				free_table((u64 *)pte_phys(entry), level - 1);
			}
		}
	}
	zero_table(table);
	pmm_page_free_addr((u64)table);
}

static u64 pte_phys(u64 pte)
{
	return (pte >> 10) << RISCV64_PAGE_SHIFT;
}

static u64 pte_table(u64 phys)
{
	return ((phys >> RISCV64_PAGE_SHIFT) << 10) | RISCV64_PTE_TABLE_MASK;
}

static u64 page_flags(u32 writable, u32 user, u32 executable)
{
	u64 flags = RISCV64_PTE_V | RISCV64_PTE_R |
		    RISCV64_PTE_A | RISCV64_PTE_D;

	if (writable != 0) {
		flags |= RISCV64_PTE_W;
	}
	if (user != 0) {
		flags |= RISCV64_PTE_U;
	}
	if (executable != 0) {
		flags |= RISCV64_PTE_X;
	}
	return flags;
}

static u64 pte_leaf(u64 phys, u32 writable, u32 user, u32 executable)
{
	return ((phys >> RISCV64_PAGE_SHIFT) << 10) |
	       page_flags(writable, user, executable);
}

static u32 vpn_index(u64 vaddr, u32 level)
{
	return (u32)((vaddr >> (RISCV64_PAGE_SHIFT + level * 9)) & 0x1ffULL);
}

static int pte_valid(u64 pte)
{
	return (pte & RISCV64_PTE_V) != 0;
}

static int pte_leaf_valid(u64 pte)
{
	return pte_valid(pte) && (pte & RISCV64_PTE_LEAF_MASK) != 0;
}

static int pte_table_valid(u64 pte)
{
	return pte_valid(pte) && (pte & RISCV64_PTE_LEAF_MASK) == 0;
}

static int ensure_table(u64 *parent, u32 index, u64 **child)
{
	u64 entry = parent[index];

	if (!pte_valid(entry)) {
		u64 *table = alloc_table();

		if (table == 0) {
			return -1;
		}
		parent[index] = pte_table((u64)table);
		*child = table;
		return 0;
	}
	if (!pte_table_valid(entry)) {
		return -1;
	}

	*child = (u64 *)pte_phys(entry);
	return 0;
}

static int lookup_leaf(const struct arch_vm_space *space, u64 vaddr,
		       u64 **slot_out)
{
	u64 *root;
	u64 *l1;
	u64 *l0;
	u64 entry;

	if (space == 0 || space->root_table == 0) {
		return -1;
	}

	root = (u64 *)space->root_table;
	entry = root[vpn_index(vaddr, 2)];
	if (!pte_table_valid(entry)) {
		return -1;
	}
	l1 = (u64 *)pte_phys(entry);

	entry = l1[vpn_index(vaddr, 1)];
	if (!pte_table_valid(entry)) {
		return -1;
	}
	l0 = (u64 *)pte_phys(entry);

	*slot_out = &l0[vpn_index(vaddr, 0)];
	return 0;
}

static void flush_vma(void)
{
	__asm__ volatile ("sfence.vma" ::: "memory");
}

void arch_vm_kernel_space_init(struct arch_vm_space *space)
{
	if (space != 0) {
		space->root_table = 0;
	}
}

int arch_vm_space_init(struct arch_vm_space *space)
{
	u64 *root;
	const struct riscv64_boot_info *boot_info;

	if (space == 0) {
		return -1;
	}

	root = alloc_table();
	if (root == 0) {
		space->root_table = 0;
		return -1;
	}

	space->root_table = (u64)root;
	boot_info = riscv64_boot_info();
	if (boot_info != 0 && boot_info->phys_size != 0 &&
	    map_supervisor_identity_window(
		    space, boot_info->phys_base,
		    boot_info->phys_base + boot_info->phys_size) != 0) {
		arch_vm_space_destroy(space);
		return -1;
	}
	if (map_registered_mmio(space) != 0) {
		arch_vm_space_destroy(space);
		return -1;
	}
	return 0;
}

void arch_vm_space_destroy(struct arch_vm_space *space)
{
	if (space == 0 || space->root_table == 0) {
		return;
	}

	free_table((u64 *)space->root_table, 2);
	space->root_table = 0;
	flush_vma();
}

int arch_vm_map_page(struct arch_vm_space *space, u64 vaddr, u64 phys,
		     u32 writable, u32 user, u32 executable)
{
	u64 *root;
	u64 *l1;
	u64 *l0;

	if (space == 0 || space->root_table == 0 ||
	    (vaddr & (RISCV64_PAGE_SIZE - 1)) != 0 ||
	    (phys & (RISCV64_PAGE_SIZE - 1)) != 0) {
		return -1;
	}

	root = (u64 *)space->root_table;
	if (ensure_table(root, vpn_index(vaddr, 2), &l1) != 0 ||
	    ensure_table(l1, vpn_index(vaddr, 1), &l0) != 0) {
		return -1;
	}

	l0[vpn_index(vaddr, 0)] = pte_leaf(phys, writable, user, executable);
	flush_vma();
	return 0;
}

static int map_supervisor_identity_window(struct arch_vm_space *space,
					  u64 start, u64 end)
{
	start &= ~(RISCV64_PAGE_SIZE - 1);
	end = (end + RISCV64_PAGE_SIZE - 1) & ~(RISCV64_PAGE_SIZE - 1);

	for (u64 page = start; page < end; page += RISCV64_PAGE_SIZE) {
		if (arch_vm_map_page(space, page, page, 1, 0, 0) != 0) {
			return -1;
		}
	}
	return 0;
}

static int map_registered_mmio(struct arch_vm_space *space)
{
	for (u32 i = 0; i < mmio_identity_mapping_count; i++) {
		if (map_supervisor_identity_window(
			    space, mmio_identity_mappings[i].start,
			    mmio_identity_mappings[i].end) != 0) {
			return -1;
		}
	}
	return 0;
}

int arch_vm_register_mmio_identity(u64 start, u64 len)
{
	u64 end;

	if (len == 0 || start + len < start ||
	    mmio_identity_mapping_count >= RISCV64_MAX_MMIO_IDENTITY_MAPPINGS) {
		return -1;
	}

	end = start + len;
	start &= ~(RISCV64_PAGE_SIZE - 1);
	if (end + RISCV64_PAGE_SIZE - 1 < end) {
		return -1;
	}
	end = (end + RISCV64_PAGE_SIZE - 1) & ~(RISCV64_PAGE_SIZE - 1);
	if (end <= start) {
		return -1;
	}

	mmio_identity_mappings[mmio_identity_mapping_count].start = start;
	mmio_identity_mappings[mmio_identity_mapping_count].end = end;
	mmio_identity_mapping_count++;
	return 0;
}

int arch_vm_protect_page(struct arch_vm_space *space, u64 vaddr, u32 writable,
			 u32 executable)
{
	u64 *slot;
	u64 entry;

	if ((vaddr & (RISCV64_PAGE_SIZE - 1)) != 0 ||
	    lookup_leaf(space, vaddr, &slot) != 0) {
		return -1;
	}

	entry = *slot;
	if (!pte_leaf_valid(entry)) {
		return -1;
	}

	if (writable != 0) {
		entry |= RISCV64_PTE_W | RISCV64_PTE_D;
	} else {
		entry &= ~((u64)RISCV64_PTE_W);
	}
	if (executable != 0) {
		entry |= RISCV64_PTE_X;
	} else {
		entry &= ~((u64)RISCV64_PTE_X);
	}
	*slot = entry;
	flush_vma();
	return 0;
}

int arch_vm_protect_user_page(struct arch_vm_space *space, u64 vaddr,
			      u32 writable, u32 executable)
{
	u64 *slot;
	u64 entry;

	if ((vaddr & (RISCV64_PAGE_SIZE - 1)) != 0 ||
	    lookup_leaf(space, vaddr, &slot) != 0) {
		return -1;
	}

	entry = *slot;
	if (!pte_leaf_valid(entry) || (entry & RISCV64_PTE_U) == 0) {
		return -1;
	}

	if (writable != 0) {
		entry |= RISCV64_PTE_W | RISCV64_PTE_D;
	} else {
		entry &= ~((u64)RISCV64_PTE_W);
	}
	if (executable != 0) {
		entry |= RISCV64_PTE_X;
	} else {
		entry &= ~((u64)RISCV64_PTE_X);
	}
	*slot = entry;
	flush_vma();
	return 0;
}

u64 arch_vm_unmap_page(struct arch_vm_space *space, u64 vaddr)
{
	u64 *slot;
	u64 entry;
	u64 phys;

	if ((vaddr & (RISCV64_PAGE_SIZE - 1)) != 0 ||
	    lookup_leaf(space, vaddr, &slot) != 0) {
		return 0;
	}

	entry = *slot;
	if (!pte_leaf_valid(entry)) {
		return 0;
	}

	phys = pte_phys(entry);
	*slot = 0;
	flush_vma();
	return phys;
}

u64 arch_vm_unmap_user_page(struct arch_vm_space *space, u64 vaddr)
{
	u64 *slot;
	u64 entry;
	u64 phys;

	if ((vaddr & (RISCV64_PAGE_SIZE - 1)) != 0 ||
	    lookup_leaf(space, vaddr, &slot) != 0) {
		return 0;
	}

	entry = *slot;
	if (!pte_leaf_valid(entry) || (entry & RISCV64_PTE_U) == 0) {
		return 0;
	}

	phys = pte_phys(entry);
	*slot = 0;
	flush_vma();
	return phys;
}

u64 arch_vm_translate(const struct arch_vm_space *space, u64 vaddr, u32 write)
{
	const u64 page_offset = vaddr & (RISCV64_PAGE_SIZE - 1);
	u64 *slot;
	u64 entry;

	if (lookup_leaf(space, vaddr, &slot) != 0) {
		return 0;
	}

	entry = *slot;
	if (!pte_leaf_valid(entry) ||
	    (write != 0 && (entry & RISCV64_PTE_W) == 0)) {
		return 0;
	}

	return pte_phys(entry) + page_offset;
}

u64 arch_vm_translate_user(const struct arch_vm_space *space, u64 vaddr,
			   u32 write)
{
	const u64 page_offset = vaddr & (RISCV64_PAGE_SIZE - 1);
	u64 *slot;
	u64 entry;

	if (lookup_leaf(space, vaddr, &slot) != 0) {
		return 0;
	}

	entry = *slot;
	if (!pte_leaf_valid(entry) || (entry & RISCV64_PTE_U) == 0 ||
	    (write != 0 && (entry & RISCV64_PTE_W) == 0)) {
		return 0;
	}

	return pte_phys(entry) + page_offset;
}

void arch_vm_activate(const struct arch_vm_space *space)
{
	u64 satp = 0;

	if (space != 0 && space->root_table != 0) {
		satp = RISCV64_SATP_MODE_SV39 |
		       (((u64)space->root_table) >> RISCV64_PAGE_SHIFT);
	}

	__asm__ volatile ("csrw satp, %0" : : "r"(satp) : "memory");
	flush_vma();
	__asm__ volatile ("fence.i" ::: "memory");
}

u64 arch_vm_root(const struct arch_vm_space *space)
{
	return space != 0 ? space->root_table : 0;
}
