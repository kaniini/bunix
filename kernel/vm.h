#ifndef BUNIXOS_VM_H
#define BUNIXOS_VM_H

#include "types.h"
#include <arch/vm.h>

enum {
	VM_PAGE_SIZE = 4096,
};

struct vm_frame {
	u64 addr;
};

struct vm_space {
	u32 id;
	const char *owner;
	struct arch_vm_space arch;
};

void vm_init(u64 multiboot_info);

struct vm_space *vm_kernel_space(void);
struct vm_space *vm_rpc_create_space(const char *owner);
void vm_rpc_activate_space(struct vm_space *space);
struct vm_frame vm_rpc_alloc_frame(void);
void vm_rpc_free_frame(struct vm_frame frame);
u64 vm_rpc_free_frames(void);
u64 vm_rpc_total_frames(void);
void vm_self_test(void);

#endif
