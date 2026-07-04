#include "console.h"
#include "pmm.h"
#include "slab.h"
#include "spinlock.h"

enum {
	SLAB_MAGIC = 0x534c4142,
	SLAB_LARGE_MAGIC = 0x534c4247,
	SLAB_MIN_ALIGN = 64,
};

struct slab_free {
	struct slab_free *next;
};

struct slab_cache;

struct slab_header {
	u32 magic;
	u32 page_count;
	u32 object_size;
	u32 object_count;
	u32 free_count;
	u32 first_offset;
	struct slab_free *free;
	struct slab_header *next;
	struct slab_cache *cache;
};

struct slab_cache {
	u32 object_size;
	struct slab_header *slabs;
	struct spinlock lock;
};

struct slab_large_header {
	u32 magic;
	u32 page_count;
	u64 size;
};

static struct slab_cache slab_caches[] = {
	{ .object_size = 32 },
	{ .object_size = 64 },
	{ .object_size = 128 },
	{ .object_size = 256 },
	{ .object_size = 512 },
	{ .object_size = 1024 },
	{ .object_size = 2048 },
	{ .object_size = 4096 },
	{ .object_size = 8192 },
	{ .object_size = 16384 },
	{ .object_size = 32768 },
	{ .object_size = 65536 },
	{ .object_size = 131072 },
};

static u32 slab_ready;

static u64 align_up(u64 value, u64 align)
{
	return (value + align - 1) & ~(align - 1);
}

static void mem_zero(u8 *dst, u64 len)
{
	for (u64 i = 0; i < len; i++) {
		dst[i] = 0;
	}
}

static struct slab_cache *cache_for_size(u64 size)
{
	for (u32 i = 0; i < sizeof(slab_caches) / sizeof(slab_caches[0]); i++) {
		if (size <= slab_caches[i].object_size) {
			return &slab_caches[i];
		}
	}

	return 0;
}

static struct slab_header *slab_grow(struct slab_cache *cache)
{
	const u64 first_offset = align_up(sizeof(struct slab_header),
					  SLAB_MIN_ALIGN);
	u64 bytes = PMM_PAGE_SIZE;

	if (first_offset + cache->object_size > bytes) {
		bytes = align_up(first_offset + cache->object_size,
				 PMM_PAGE_SIZE);
	}

	const u64 pages = bytes / PMM_PAGE_SIZE;
	const u64 base = pmm_pages_alloc_contiguous(pages);

	if (base == 0) {
		return 0;
	}

	struct slab_header *slab = (struct slab_header *)base;
	slab->magic = SLAB_MAGIC;
	slab->page_count = pages;
	slab->object_size = cache->object_size;
	slab->first_offset = first_offset;
	slab->object_count = (pages * PMM_PAGE_SIZE - first_offset) /
			     cache->object_size;
	slab->free_count = slab->object_count;
	slab->free = 0;
	slab->cache = cache;

	for (u32 i = 0; i < slab->object_count; i++) {
		struct slab_free *node =
			(struct slab_free *)(base + first_offset +
					     (u64)i * cache->object_size);
		node->next = slab->free;
		slab->free = node;
	}

	slab->next = cache->slabs;
	cache->slabs = slab;
	return slab;
}

static void *slab_alloc_large(u64 size)
{
	const u64 offset = align_up(sizeof(struct slab_large_header),
				    SLAB_MIN_ALIGN);
	const u64 bytes = align_up(offset + size, PMM_PAGE_SIZE);
	const u64 pages = bytes / PMM_PAGE_SIZE;
	const u64 base = pmm_pages_alloc_contiguous(pages);

	if (base == 0) {
		return 0;
	}

	struct slab_large_header *header = (struct slab_large_header *)base;
	header->magic = SLAB_LARGE_MAGIC;
	header->page_count = pages;
	header->size = size;
	return (void *)(base + offset);
}

void slab_init(void)
{
	for (u32 i = 0; i < sizeof(slab_caches) / sizeof(slab_caches[0]); i++) {
		spinlock_init(&slab_caches[i].lock, "slab");
		slab_caches[i].slabs = 0;
	}

	slab_ready = 1;
	console_printf("slab: init caches=%u max=%u\n",
		       (u32)(sizeof(slab_caches) / sizeof(slab_caches[0])),
		       slab_caches[(sizeof(slab_caches) / sizeof(slab_caches[0])) -
				   1].object_size);
}

void *slab_alloc(u64 size)
{
	if (!slab_ready || size == 0) {
		return 0;
	}

	struct slab_cache *cache = cache_for_size(size);
	if (cache == 0) {
		return slab_alloc_large(size);
	}

	const u64 flags = spin_lock_irqsave(&cache->lock);
	struct slab_header *slab = cache->slabs;

	while (slab != 0 && slab->free == 0) {
		slab = slab->next;
	}
	if (slab == 0) {
		slab = slab_grow(cache);
	}
	if (slab == 0 || slab->free == 0) {
		spin_unlock_irqrestore(&cache->lock, flags);
		return 0;
	}

	struct slab_free *node = slab->free;
	slab->free = node->next;
	slab->free_count--;
	spin_unlock_irqrestore(&cache->lock, flags);
	return node;
}

void *slab_zalloc(u64 size)
{
	void *ptr = slab_alloc(size);

	if (ptr != 0) {
		mem_zero((u8 *)ptr, size);
	}
	return ptr;
}

void slab_free(void *ptr)
{
	if (ptr == 0) {
		return;
	}

	const u64 large_base = (u64)ptr -
			       align_up(sizeof(struct slab_large_header),
					SLAB_MIN_ALIGN);
	struct slab_large_header *large =
		(struct slab_large_header *)large_base;
	if ((large_base & (PMM_PAGE_SIZE - 1)) == 0 &&
	    large->magic == SLAB_LARGE_MAGIC) {
		const u32 pages = large->page_count;

		large->magic = 0;
		pmm_pages_free_contiguous(large_base, pages);
		return;
	}

	for (u32 i = 0; i < sizeof(slab_caches) / sizeof(slab_caches[0]); i++) {
		struct slab_cache *cache = &slab_caches[i];
		const u64 flags = spin_lock_irqsave(&cache->lock);

		for (struct slab_header *slab = cache->slabs; slab != 0;
		     slab = slab->next) {
			const u64 base = (u64)slab;
			const u64 start = base + slab->first_offset;
			const u64 end = base + (u64)slab->page_count *
					PMM_PAGE_SIZE;
			const u64 addr = (u64)ptr;

			if (slab->magic != SLAB_MAGIC || addr < start ||
			    addr >= end ||
			    ((addr - start) % slab->object_size) != 0) {
				continue;
			}

			struct slab_free *node = (struct slab_free *)ptr;
			node->next = slab->free;
			slab->free = node;
			slab->free_count++;
			spin_unlock_irqrestore(&cache->lock, flags);
			return;
		}

		spin_unlock_irqrestore(&cache->lock, flags);
	}
}
