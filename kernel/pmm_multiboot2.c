#include "console.h"
#include "multiboot2.h"
#include "pmm.h"

enum {
	MULTIBOOT_MEMORY_AVAILABLE = 1,
	LOW_USER_IMAGE_START = 0x400000,
	LOW_USER_IMAGE_END = 0x1000000,
	PMM_MAX_BOOT_RANGES = 128,
	PMM_MAX_RESERVED_RANGES = 256,
};

extern char __kernel_start[];
extern char __kernel_end[];

struct range_collect_ctx {
	struct pmm_memory_range *ranges;
	u64 capacity;
	u64 count;
	u32 overflow;
};

struct reserve_collect_ctx {
	struct pmm_reserved_range *ranges;
	u64 capacity;
	u64 count;
	u32 overflow;
};

static void collect_available_mmap(const struct multiboot2_mmap_entry *entry,
				   void *ctx)
{
	struct range_collect_ctx *collect = (struct range_collect_ctx *)ctx;

	if (entry->type != MULTIBOOT_MEMORY_AVAILABLE) {
		return;
	}
	if (collect->count >= collect->capacity) {
		collect->overflow = 1;
		return;
	}

	collect->ranges[collect->count++] =
		(struct pmm_memory_range){ .base = entry->base,
					   .length = entry->length };
}

static void collect_reserved_range(struct reserve_collect_ctx *collect,
				   u64 start, u64 end)
{
	if (start >= end) {
		return;
	}
	if (collect->count >= collect->capacity) {
		collect->overflow = 1;
		return;
	}
	collect->ranges[collect->count++] =
		(struct pmm_reserved_range){ .start = start, .end = end };
}

static void collect_reserved_module(const struct multiboot2_module *module,
				    void *ctx)
{
	struct reserve_collect_ctx *collect = (struct reserve_collect_ctx *)ctx;

	collect_reserved_range(collect, module->start, module->end);
}

void pmm_init(u64 multiboot_info)
{
	struct pmm_memory_range available[PMM_MAX_BOOT_RANGES];
	struct pmm_reserved_range reserved[PMM_MAX_RESERVED_RANGES];
	struct range_collect_ctx available_collect = {
		.ranges = available,
		.capacity = PMM_MAX_BOOT_RANGES,
		.count = 0,
		.overflow = 0,
	};
	struct reserve_collect_ctx reserved_collect = {
		.ranges = reserved,
		.capacity = PMM_MAX_RESERVED_RANGES,
		.count = 0,
		.overflow = 0,
	};

	multiboot2_for_each_mmap(multiboot_info, collect_available_mmap,
				 &available_collect);

	collect_reserved_range(&reserved_collect, 0, 0x100000);
	collect_reserved_range(&reserved_collect, (u64)__kernel_start,
			       (u64)__kernel_end);
	collect_reserved_range(&reserved_collect, multiboot_info,
			       multiboot_info +
				       multiboot2_total_size(multiboot_info));
	collect_reserved_range(&reserved_collect, LOW_USER_IMAGE_START,
			       LOW_USER_IMAGE_END);
	multiboot2_for_each_module(multiboot_info, collect_reserved_module,
				   &reserved_collect);

	if (available_collect.overflow || reserved_collect.overflow) {
		console_printf("pmm: too many boot ranges available=%u reserved=%u\n",
			       (u32)available_collect.count,
			       (u32)reserved_collect.count);
		for (;;) {
		}
	}

	pmm_init_from_ranges(available, available_collect.count,
			     reserved, reserved_collect.count);
}
