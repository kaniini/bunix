#include <arch/vm.h>

enum {
	PTE_V = 1 << 0,
	PTE_R = 1 << 1,
	PTE_W = 1 << 2,
	PTE_X = 1 << 3,
	PTE_U = 1 << 4,
	PTE_G = 1 << 5,
	PTE_A = 1 << 6,
	PTE_D = 1 << 7,
};

void arch_vm_kernel_space_init(struct arch_vm_space *space)
{
	space->root_table = 0;
}

int arch_vm_space_init(struct arch_vm_space *space)
{
	space->root_table = 0;
	return 0;
}

void arch_vm_space_destroy(struct arch_vm_space *space)
{
	if (space != 0) {
		space->root_table = 0;
	}
}

int arch_vm_map_page(struct arch_vm_space *space, u64 vaddr, u64 phys,
		     u32 writable, u32 user)
{
	(void)space;
	(void)vaddr;
	(void)phys;
	(void)writable;
	(void)user;
	return -1;
}

int arch_vm_protect_page(struct arch_vm_space *space, u64 vaddr, u32 writable)
{
	(void)space;
	(void)vaddr;
	(void)writable;
	return -1;
}

u64 arch_vm_unmap_page(struct arch_vm_space *space, u64 vaddr)
{
	(void)space;
	(void)vaddr;
	return 0;
}

u64 arch_vm_translate(const struct arch_vm_space *space, u64 vaddr, u32 write)
{
	(void)space;
	(void)vaddr;
	(void)write;
	return 0;
}

void arch_vm_activate(const struct arch_vm_space *space)
{
	(void)space;
}

u64 riscv64_vm_page_flags(u32 writable, u32 user, u32 executable)
{
	u64 flags = PTE_V | PTE_R | PTE_A | PTE_D;

	if (writable != 0) {
		flags |= PTE_W;
	}
	if (user != 0) {
		flags |= PTE_U;
	}
	if (executable != 0) {
		flags |= PTE_X;
	}
	return flags;
}
