#ifndef BUNIXOS_PMM_H
#define BUNIXOS_PMM_H

#include "types.h"

enum {
	PMM_PAGE_SIZE = 4096,
};

struct pmm_page {
	u64 addr;
	u32 is_free;
	u32 ref_count;
	struct pmm_page *next;
};

struct pmm_memory_range {
	u64 base;
	u64 length;
};

struct pmm_reserved_range {
	u64 start;
	u64 end;
};

void pmm_init(u64 multiboot_info);
void pmm_init_from_ranges(const struct pmm_memory_range *available,
			  u64 available_count,
			  const struct pmm_reserved_range *reserved,
			  u64 reserved_count);
struct pmm_page *pmm_page_alloc(void);
u64 pmm_pages_alloc_contiguous(u64 count);
int pmm_page_retain_addr(u64 addr);
void pmm_page_free(struct pmm_page *page);
void pmm_page_free_addr(u64 addr);
void pmm_pages_free_contiguous(u64 addr, u64 count);
u64 pmm_page_addr(const struct pmm_page *page);
u64 pmm_free_page_count(void);
u64 pmm_total_page_count(void);

#endif
