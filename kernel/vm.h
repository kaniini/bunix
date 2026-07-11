#ifndef BUNIXOS_VM_H
#define BUNIXOS_VM_H

#include "tree.h"
#include "types.h"
#include <arch/vm.h>

enum {
	VM_PAGE_SIZE = 4096,
};

struct vm_frame {
	u64 addr;
};

struct vm_space {
	struct u64_tree_node id_node;
	u32 id;
	const char *owner;
	struct arch_vm_space arch;
};

void vm_init(void);

struct vm_space *vm_kernel_space(void);
struct vm_space *vm_rpc_create_space(const char *owner);
void vm_rpc_destroy_space(struct vm_space *space);
void vm_rpc_activate_space(struct vm_space *space);
struct vm_frame vm_rpc_alloc_frame(void);
void vm_rpc_free_frame(struct vm_frame frame);
int vm_map_user_page(struct vm_space *space, u64 vaddr, struct vm_frame frame,
		     u32 writable);
int vm_read_user(struct vm_space *space, u64 vaddr, void *dst, u64 len);
int vm_write_user(struct vm_space *space, u64 vaddr, const void *src, u64 len);
int vm_map_kernel_page(u64 vaddr, u64 phys, u32 writable);
struct vm_frame vm_alloc_user_page(struct vm_space *space, u64 vaddr,
				   u32 writable);
int vm_alloc_user_range(struct vm_space *space, u64 vaddr, u64 len,
			u32 writable);
int vm_protect_user_range(struct vm_space *space, u64 vaddr, u64 len,
			  u32 writable);
int vm_unmap_user_range(struct vm_space *space, u64 vaddr, u64 len);
int vm_clone_user_range(struct vm_space *dst, struct vm_space *src,
			u64 vaddr, u64 len, u32 writable);
int vm_share_user_range(struct vm_space *dst, struct vm_space *src,
			u64 vaddr, u64 len, u32 writable);
int vm_share_cow_user_range(struct vm_space *dst, struct vm_space *src,
			    u64 vaddr, u64 len);
int vm_cow_user_page(struct vm_space *space, u64 vaddr);
u64 vm_rpc_free_frames(void);
u64 vm_rpc_total_frames(void);
void vm_self_test(void);

#endif
