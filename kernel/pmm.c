#include "console.h"
#include "multiboot2.h"
#include "pmm.h"

enum {
	MAX_PHYS_PAGES = 32768,
	MULTIBOOT_MEMORY_AVAILABLE = 1,
};

extern char __kernel_start[];
extern char __kernel_end[];

static struct pmm_page pages[MAX_PHYS_PAGES];
static struct pmm_page *free_pages;
static u64 total_pages;
static u64 free_pages_count;

static u64 align_up(u64 value, u64 align)
{
	return (value + align - 1) & ~(align - 1);
}

static u64 align_down(u64 value, u64 align)
{
	return value & ~(align - 1);
}

static int ranges_overlap(u64 a_start, u64 a_end, u64 b_start, u64 b_end)
{
	return a_start < b_end && b_start < a_end;
}

static void page_list_push(struct pmm_page *page)
{
	page->is_free = 1;
	page->next = free_pages;
	free_pages = page;
	free_pages_count++;
}

static struct pmm_page *find_page(u64 addr)
{
	for (u64 i = 0; i < total_pages; i++) {
		if (pages[i].addr == addr) {
			return &pages[i];
		}
	}

	return 0;
}

static void import_mmap_entry(const struct multiboot2_mmap_entry *entry,
			      void *ctx)
{
	(void)ctx;

	if (entry->type != MULTIBOOT_MEMORY_AVAILABLE) {
		return;
	}

	u64 start = align_up(entry->base, PMM_PAGE_SIZE);
	const u64 end = align_down(entry->base + entry->length, PMM_PAGE_SIZE);

	while (start < end && total_pages < MAX_PHYS_PAGES) {
		pages[total_pages].addr = start;
		pages[total_pages].is_free = 0;
		pages[total_pages].next = 0;
		page_list_push(&pages[total_pages]);
		total_pages++;
		start += PMM_PAGE_SIZE;
	}
}

static void reserve_range(u64 start, u64 end)
{
	struct pmm_page *new_free = 0;
	u64 removed = 0;

	start = align_down(start, PMM_PAGE_SIZE);
	end = align_up(end, PMM_PAGE_SIZE);

	while (free_pages != 0) {
		struct pmm_page *page = free_pages;
		free_pages = page->next;

		if (ranges_overlap(page->addr, page->addr + PMM_PAGE_SIZE, start, end)) {
			page->is_free = 0;
			page->next = 0;
			removed++;
			continue;
		}

		page->next = new_free;
		new_free = page;
	}

	free_pages = new_free;
	free_pages_count -= removed;

	if (removed != 0) {
		console_printf("pmm: reserve %p-%p pages=%u\n",
			       (const void *)start, (const void *)end,
			       (u32)removed);
	}
}

static void reserve_module(const struct multiboot2_module *module, void *ctx)
{
	(void)ctx;

	reserve_range(module->start, module->end);
}

void pmm_init(u64 multiboot_info)
{
	total_pages = 0;
	free_pages_count = 0;
	free_pages = 0;

	multiboot2_for_each_mmap(multiboot_info, import_mmap_entry, 0);

	reserve_range(0, 0x100000);
	reserve_range((u64)__kernel_start, (u64)__kernel_end);
	reserve_range(multiboot_info, multiboot_info + multiboot2_total_size(multiboot_info));
	multiboot2_for_each_module(multiboot_info, reserve_module, 0);

	console_printf("pmm: pages total=%u free=%u\n",
		       (u32)total_pages, (u32)free_pages_count);
}

struct pmm_page *pmm_page_alloc(void)
{
	struct pmm_page *page = free_pages;

	if (page == 0) {
		return 0;
	}

	free_pages = page->next;
	page->is_free = 0;
	page->next = 0;
	free_pages_count--;
	return page;
}

void pmm_page_free(struct pmm_page *page)
{
	if (page == 0 || find_page(page->addr) != page || page->is_free) {
		return;
	}

	page_list_push(page);
}

void pmm_page_free_addr(u64 addr)
{
	pmm_page_free(find_page(addr));
}

u64 pmm_page_addr(const struct pmm_page *page)
{
	return page != 0 ? page->addr : 0;
}

u64 pmm_free_page_count(void)
{
	return free_pages_count;
}

u64 pmm_total_page_count(void)
{
	return total_pages;
}
