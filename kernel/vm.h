#ifndef BUNIXOS_VM_H
#define BUNIXOS_VM_H

#include "types.h"

enum {
	VM_PAGE_SIZE = 4096,
};

struct vm_frame {
	u64 addr;
};

void vm_init(u64 multiboot_info);

struct vm_frame vm_rpc_alloc_frame(void);
void vm_rpc_free_frame(struct vm_frame frame);
u64 vm_rpc_free_frames(void);
u64 vm_rpc_total_frames(void);
void vm_self_test(void);

#endif
