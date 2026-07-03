#ifndef BUNIXOS_VM_H
#define BUNIXOS_VM_H

#include "types.h"

enum {
	VM_PAGE_SIZE = 4096,
};

struct vm_page {
	u64 addr;
	struct vm_page *next;
};

void vm_init(u64 multiboot_info);
struct vm_page *vm_page_alloc(void);
void vm_page_free(struct vm_page *page);
u64 vm_page_addr(const struct vm_page *page);
u64 vm_free_page_count(void);
u64 vm_total_page_count(void);
void vm_self_test(void);

#endif
