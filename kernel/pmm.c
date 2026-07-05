#include "console.h"
#include "multiboot2.h"
#include "pmm.h"
#include "spinlock.h"

enum {
	MULTIBOOT_MEMORY_AVAILABLE = 1,
	LOW_USER_IMAGE_START = 0x400000,
	LOW_USER_IMAGE_END = 0x1000000,
};

extern char __kernel_start[];
extern char __kernel_end[];

struct module_overlap_ctx {
	u64 start;
	u64 end;
	u32 overlap;
};

static struct pmm_page *pages;
static struct pmm_page *free_pages;
static u64 page_capacity;
static u64 total_pages;
static u64 free_pages_count;
static u64 metadata_start;
static u64 metadata_end;
static struct spinlock pmm_lock = SPINLOCK_INIT("pmm");

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

static void mem_zero(void *ptr, u64 len)
{
	u8 *bytes = (u8 *)ptr;

	for (u64 i = 0; i < len; i++) {
		bytes[i] = 0;
	}
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

static void page_list_remove(struct pmm_page *page)
{
	struct pmm_page **link = &free_pages;

	while (*link != 0) {
		if (*link == page) {
			*link = page->next;
			page->next = 0;
			return;
		}
		link = &(*link)->next;
	}
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

	while (start < end && total_pages < page_capacity) {
		pages[total_pages].addr = start;
		pages[total_pages].is_free = 0;
		pages[total_pages].next = 0;
		page_list_push(&pages[total_pages]);
		total_pages++;
		start += PMM_PAGE_SIZE;
	}
}

static void count_mmap_entry(const struct multiboot2_mmap_entry *entry,
			     void *ctx)
{
	u64 *count = (u64 *)ctx;

	if (entry->type != MULTIBOOT_MEMORY_AVAILABLE) {
		return;
	}

	const u64 start = align_up(entry->base, PMM_PAGE_SIZE);
	const u64 end = align_down(entry->base + entry->length, PMM_PAGE_SIZE);

	if (end > start) {
		*count += (end - start) / PMM_PAGE_SIZE;
	}
}

static void check_module_overlap(const struct multiboot2_module *module,
				 void *ctx)
{
	struct module_overlap_ctx *overlap =
		(struct module_overlap_ctx *)ctx;

	if (ranges_overlap(overlap->start, overlap->end,
			   module->start, module->end)) {
		overlap->overlap = 1;
	}
}

static int range_is_reserved(u64 start, u64 end, u64 multiboot_info)
{
	struct module_overlap_ctx overlap = {
		.start = start,
		.end = end,
		.overlap = 0,
	};

	if (ranges_overlap(start, end, 0, 0x100000) ||
	    ranges_overlap(start, end, (u64)__kernel_start,
			   (u64)__kernel_end) ||
	    ranges_overlap(start, end, multiboot_info,
			   multiboot_info + multiboot2_total_size(multiboot_info)) ||
	    ranges_overlap(start, end, LOW_USER_IMAGE_START,
			   LOW_USER_IMAGE_END)) {
		return 1;
	}

	multiboot2_for_each_module(multiboot_info, check_module_overlap,
				   &overlap);
	return overlap.overlap != 0;
}

static void find_metadata_entry(const struct multiboot2_mmap_entry *entry,
				void *ctx)
{
	u64 *state = (u64 *)ctx;
	const u64 multiboot_info = state[0];
	const u64 bytes = state[1];

	if (state[2] != 0 || entry->type != MULTIBOOT_MEMORY_AVAILABLE) {
		return;
	}

	u64 start = align_up(entry->base, PMM_PAGE_SIZE);
	const u64 end = align_down(entry->base + entry->length, PMM_PAGE_SIZE);

	while (start < end && bytes <= end - start) {
		if (!range_is_reserved(start, start + bytes, multiboot_info)) {
			state[2] = start;
			return;
		}
		start += PMM_PAGE_SIZE;
	}
}

static u64 find_metadata_storage(u64 multiboot_info, u64 page_count)
{
	const u64 bytes = align_up(page_count * sizeof(struct pmm_page),
				   PMM_PAGE_SIZE);
	u64 state[3] = { multiboot_info, bytes, 0 };

	multiboot2_for_each_mmap(multiboot_info, find_metadata_entry, state);
	metadata_start = state[2];
	metadata_end = state[2] + bytes;
	return state[2];
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
	u64 detected_pages = 0;

	total_pages = 0;
	free_pages_count = 0;
	free_pages = 0;
	page_capacity = 0;
	pages = 0;
	metadata_start = 0;
	metadata_end = 0;

	multiboot2_for_each_mmap(multiboot_info, count_mmap_entry,
				 &detected_pages);
	pages = (struct pmm_page *)find_metadata_storage(multiboot_info,
							 detected_pages);
	if (pages == 0 || metadata_end <= metadata_start) {
		console_printf("pmm: metadata allocation failed pages=%u\n",
			       (u32)detected_pages);
		for (;;) {
			__asm__ volatile ("cli; hlt");
		}
	}
	page_capacity = detected_pages;
	mem_zero(pages, metadata_end - metadata_start);

	multiboot2_for_each_mmap(multiboot_info, import_mmap_entry, 0);

	reserve_range(0, 0x100000);
	reserve_range((u64)__kernel_start, (u64)__kernel_end);
	reserve_range(multiboot_info, multiboot_info + multiboot2_total_size(multiboot_info));
	multiboot2_for_each_module(multiboot_info, reserve_module, 0);
	reserve_range(LOW_USER_IMAGE_START, LOW_USER_IMAGE_END);
	reserve_range(metadata_start, metadata_end);

	console_printf("pmm: pages total=%u free=%u metadata=%p-%p\n",
		       (u32)total_pages, (u32)free_pages_count,
		       (const void *)metadata_start, (const void *)metadata_end);
}

struct pmm_page *pmm_page_alloc(void)
{
	const u64 flags = spin_lock_irqsave(&pmm_lock);
	struct pmm_page *page = free_pages;

	if (page == 0) {
		spin_unlock_irqrestore(&pmm_lock, flags);
		return 0;
	}

	free_pages = page->next;
	page->is_free = 0;
	page->next = 0;
	free_pages_count--;
	spin_unlock_irqrestore(&pmm_lock, flags);
	return page;
}

u64 pmm_pages_alloc_contiguous(u64 count)
{
	if (count == 0) {
		return 0;
	}

	const u64 flags = spin_lock_irqsave(&pmm_lock);

	for (u64 i = 0; i + count <= total_pages; i++) {
		u32 usable = 1;
		const u64 start = pages[i].addr;

		for (u64 j = 0; j < count; j++) {
			if (!pages[i + j].is_free ||
			    pages[i + j].addr != start + j * PMM_PAGE_SIZE) {
				usable = 0;
				break;
			}
		}

		if (!usable) {
			continue;
		}

		for (u64 j = 0; j < count; j++) {
			page_list_remove(&pages[i + j]);
			pages[i + j].is_free = 0;
		}
		free_pages_count -= count;
		spin_unlock_irqrestore(&pmm_lock, flags);
		return start;
	}

	spin_unlock_irqrestore(&pmm_lock, flags);
	return 0;
}

void pmm_page_free(struct pmm_page *page)
{
	const u64 flags = spin_lock_irqsave(&pmm_lock);

	if (page == 0 || find_page(page->addr) != page || page->is_free) {
		spin_unlock_irqrestore(&pmm_lock, flags);
		return;
	}

	page_list_push(page);
	spin_unlock_irqrestore(&pmm_lock, flags);
}

void pmm_page_free_addr(u64 addr)
{
	pmm_page_free(find_page(addr));
}

void pmm_pages_free_contiguous(u64 addr, u64 count)
{
	for (u64 i = 0; i < count; i++) {
		pmm_page_free_addr(addr + i * PMM_PAGE_SIZE);
	}
}

u64 pmm_page_addr(const struct pmm_page *page)
{
	return page != 0 ? page->addr : 0;
}

u64 pmm_free_page_count(void)
{
	const u64 flags = spin_lock_irqsave(&pmm_lock);
	const u64 count = free_pages_count;

	spin_unlock_irqrestore(&pmm_lock, flags);
	return count;
}

u64 pmm_total_page_count(void)
{
	const u64 flags = spin_lock_irqsave(&pmm_lock);
	const u64 count = total_pages;

	spin_unlock_irqrestore(&pmm_lock, flags);
	return count;
}
