#ifndef BUNIXOS_PMM_H
#define BUNIXOS_PMM_H

#include "types.h"

enum {
	PMM_PAGE_SIZE = 4096,
};

struct pmm_page {
	u64 addr;
	u32 is_free;
	struct pmm_page *next;
};

void pmm_init(u64 multiboot_info);
struct pmm_page *pmm_page_alloc(void);
void pmm_page_free(struct pmm_page *page);
void pmm_page_free_addr(u64 addr);
u64 pmm_page_addr(const struct pmm_page *page);
u64 pmm_free_page_count(void);
u64 pmm_total_page_count(void);

#endif
