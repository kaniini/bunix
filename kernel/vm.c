#include "console.h"
#include "multiboot2.h"
#include "vm.h"

enum {
	MAX_PHYS_PAGES = 32768,
	MULTIBOOT_MEMORY_AVAILABLE = 1,
};

extern char __kernel_start[];
extern char __kernel_end[];

static struct vm_page pages[MAX_PHYS_PAGES];
static struct vm_page *free_pages;
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

static void page_list_push(struct vm_page *page)
{
	page->next = free_pages;
	free_pages = page;
	free_pages_count++;
}

static struct vm_page *find_page(u64 addr)
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

	u64 start = align_up(entry->base, VM_PAGE_SIZE);
	const u64 end = align_down(entry->base + entry->length, VM_PAGE_SIZE);

	while (start < end && total_pages < MAX_PHYS_PAGES) {
		pages[total_pages].addr = start;
		pages[total_pages].next = 0;
		page_list_push(&pages[total_pages]);
		total_pages++;
		start += VM_PAGE_SIZE;
	}
}

static void reserve_range(u64 start, u64 end)
{
	struct vm_page *new_free = 0;
	u64 removed = 0;

	start = align_down(start, VM_PAGE_SIZE);
	end = align_up(end, VM_PAGE_SIZE);

	while (free_pages != 0) {
		struct vm_page *page = free_pages;
		free_pages = page->next;

		if (ranges_overlap(page->addr, page->addr + VM_PAGE_SIZE, start, end)) {
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
		console_printf("vm: reserve %p-%p pages=%u\n",
			       (const void *)start, (const void *)end,
			       (u32)removed);
	}
}

static void reserve_module(const struct multiboot2_module *module, void *ctx)
{
	(void)ctx;

	reserve_range(module->start, module->end);
}

void vm_init(u64 multiboot_info)
{
	total_pages = 0;
	free_pages_count = 0;
	free_pages = 0;

	multiboot2_for_each_mmap(multiboot_info, import_mmap_entry, 0);

	reserve_range(0, 0x100000);
	reserve_range((u64)__kernel_start, (u64)__kernel_end);
	reserve_range(multiboot_info, multiboot_info + multiboot2_total_size(multiboot_info));
	multiboot2_for_each_module(multiboot_info, reserve_module, 0);

	console_printf("vm: pages total=%u free=%u\n",
		       (u32)total_pages, (u32)free_pages_count);
}

struct vm_page *vm_page_alloc(void)
{
	struct vm_page *page = free_pages;

	if (page == 0) {
		return 0;
	}

	free_pages = page->next;
	page->next = 0;
	free_pages_count--;
	return page;
}

void vm_page_free(struct vm_page *page)
{
	if (page == 0 || find_page(page->addr) != page) {
		return;
	}

	page_list_push(page);
}

u64 vm_page_addr(const struct vm_page *page)
{
	return page != 0 ? page->addr : 0;
}

u64 vm_free_page_count(void)
{
	return free_pages_count;
}

u64 vm_total_page_count(void)
{
	return total_pages;
}

void vm_self_test(void)
{
	struct vm_page *a = vm_page_alloc();
	struct vm_page *b = vm_page_alloc();

	console_printf("vm: selftest alloc a=%p b=%p free=%u\n",
		       (const void *)vm_page_addr(a),
		       (const void *)vm_page_addr(b),
		       (u32)vm_free_page_count());

	vm_page_free(b);
	vm_page_free(a);

	console_printf("vm: selftest free=%u\n", (u32)vm_free_page_count());
}
