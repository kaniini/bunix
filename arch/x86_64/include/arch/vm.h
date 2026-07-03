#ifndef BUNIXOS_ARCH_VM_H
#define BUNIXOS_ARCH_VM_H

#include "types.h"

struct arch_vm_space {
	u64 cr3;
};

void arch_vm_kernel_space_init(struct arch_vm_space *space);
int arch_vm_space_init(struct arch_vm_space *space);
void arch_vm_activate(const struct arch_vm_space *space);

#endif
