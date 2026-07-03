#ifndef BUNIXOS_ARCH_VM_H
#define BUNIXOS_ARCH_VM_H

#include "types.h"

struct arch_vm_space {
	u64 cr3;
};

void arch_vm_kernel_space_init(struct arch_vm_space *space);
int arch_vm_space_init(struct arch_vm_space *space);
int arch_vm_map_page(struct arch_vm_space *space, u64 vaddr, u64 phys,
		     u32 writable, u32 user);
u64 arch_vm_unmap_page(struct arch_vm_space *space, u64 vaddr);
u64 arch_vm_translate(const struct arch_vm_space *space, u64 vaddr,
		      u32 write);
void arch_vm_activate(const struct arch_vm_space *space);

#endif
