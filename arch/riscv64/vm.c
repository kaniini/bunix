#include <arch/vm.h>

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
