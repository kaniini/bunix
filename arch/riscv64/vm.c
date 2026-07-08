#include <arch/vm.h>

enum {
	RISCV64_SATP_MODE_SV39 = 8ULL << 60,
	RISCV64_PAGE_ENTRIES = 512,
	RISCV64_EARLY_VM_TABLES = 64,
	RISCV64_PTE_TABLE_MASK = RISCV64_PTE_V,
	RISCV64_PTE_LEAF_MASK = RISCV64_PTE_R | RISCV64_PTE_W | RISCV64_PTE_X,
};

static u64 early_vm_tables[RISCV64_EARLY_VM_TABLES][RISCV64_PAGE_ENTRIES]
	__attribute__((aligned(RISCV64_PAGE_SIZE)));
static u32 early_vm_next_table;

static void zero_table(u64 *table)
{
	for (u32 i = 0; i < RISCV64_PAGE_ENTRIES; i++) {
		table[i] = 0;
	}
}

static u64 *alloc_table(void)
{
	if (early_vm_next_table >= RISCV64_EARLY_VM_TABLES) {
		return 0;
	}

	u64 *table = early_vm_tables[early_vm_next_table++];
	zero_table(table);
	return table;
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

static u64 pte_leaf(u64 phys, u32 writable, u32 user)
{
	/*
	 * The existing generic VM contract has no execute bit because the
	 * x86_64 path does not use NX.  Keep riscv64 mappings executable until
	 * the shared VM API grows explicit execute permissions.
	 */
	return ((phys >> RISCV64_PAGE_SHIFT) << 10) |
	       page_flags(writable, user, 1);
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

	if (space == 0) {
		return -1;
	}

	root = alloc_table();
	if (root == 0) {
		space->root_table = 0;
		return -1;
	}

	space->root_table = (u64)root;
	return 0;
}

void arch_vm_space_destroy(struct arch_vm_space *space)
{
	if (space == 0 || space->root_table == 0) {
		return;
	}

	zero_table((u64 *)space->root_table);
	space->root_table = 0;
	flush_vma();
}

int arch_vm_map_page(struct arch_vm_space *space, u64 vaddr, u64 phys,
		     u32 writable, u32 user)
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

	l0[vpn_index(vaddr, 0)] = pte_leaf(phys, writable, user);
	flush_vma();
	return 0;
}

int arch_vm_protect_page(struct arch_vm_space *space, u64 vaddr, u32 writable)
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
