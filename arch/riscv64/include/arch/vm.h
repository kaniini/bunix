#ifndef BUNIXOS_ARCH_VM_H
#define BUNIXOS_ARCH_VM_H

#include <arch/layout.h>
#include "types.h"

struct arch_vm_space {
	u64 root_table;
};

void arch_vm_kernel_space_init(struct arch_vm_space *space);
int arch_vm_space_init(struct arch_vm_space *space);
void arch_vm_space_destroy(struct arch_vm_space *space);
int arch_vm_register_mmio_identity(u64 start, u64 len);
int arch_vm_map_page(struct arch_vm_space *space, u64 vaddr, u64 phys,
		     u32 writable, u32 user, u32 executable);
int arch_vm_protect_page(struct arch_vm_space *space, u64 vaddr, u32 writable,
			 u32 executable);
int arch_vm_protect_user_page(struct arch_vm_space *space, u64 vaddr,
			      u32 writable, u32 executable);
u64 arch_vm_unmap_page(struct arch_vm_space *space, u64 vaddr);
u64 arch_vm_unmap_user_page(struct arch_vm_space *space, u64 vaddr);
u64 arch_vm_translate(const struct arch_vm_space *space, u64 vaddr,
		      u32 write);
u64 arch_vm_translate_user(const struct arch_vm_space *space, u64 vaddr,
			   u32 write);
void arch_vm_activate(const struct arch_vm_space *space);
u64 arch_vm_root(const struct arch_vm_space *space);

#endif
