#ifndef BUNIX_USER_ALLOC_H
#define BUNIX_USER_ALLOC_H

#include <bunix/syscall.h>

enum {
	BUNIX_ALLOC_MIN_ORDER = 6,
	BUNIX_ALLOC_MAX_ORDER = 20,
	BUNIX_ALLOC_ORDERS = BUNIX_ALLOC_MAX_ORDER - BUNIX_ALLOC_MIN_ORDER + 1,
	BUNIX_ALLOC_ARENA_SIZE = 1ull << BUNIX_ALLOC_MAX_ORDER,
	BUNIX_ALLOC_HEAP_BASE = 0x10000000ull,
};

struct bunix_alloc_block {
	u64 order;
	u64 free;
	struct bunix_alloc_block *prev;
	struct bunix_alloc_block *next;
};

static struct bunix_alloc_block *bunix_alloc_free[BUNIX_ALLOC_ORDERS];
static struct bunix_alloc_block *bunix_alloc_large_free;
static u64 bunix_alloc_next = BUNIX_ALLOC_HEAP_BASE;

static inline u64 bunix_alloc_align(u64 value, u64 align)
{
	return (value + align - 1) & ~(align - 1);
}

static inline u64 bunix_alloc_order_for(u64 size)
{
	u64 order = BUNIX_ALLOC_MIN_ORDER;
	u64 block_size = 1ull << order;
	const u64 need = bunix_alloc_align(size +
					   sizeof(struct bunix_alloc_block),
					   16);

	while (order < BUNIX_ALLOC_MAX_ORDER && block_size < need) {
		order++;
		block_size <<= 1;
	}
	return block_size >= need ? order : 0;
}

static inline u64 bunix_alloc_large_order_for(u64 size)
{
	u64 order = BUNIX_ALLOC_MAX_ORDER + 1;
	u64 block_size = 1ull << order;
	u64 need;

	if (size > ((u64)-1) - sizeof(struct bunix_alloc_block)) {
		return 0;
	}
	need = bunix_alloc_align(size + sizeof(struct bunix_alloc_block), 16);
	if (need < size) {
		return 0;
	}
	while (order < 63 && block_size < need) {
		order++;
		block_size <<= 1;
	}
	return block_size >= need ? order : 0;
}

static inline u64 bunix_alloc_index(u64 order)
{
	return order - BUNIX_ALLOC_MIN_ORDER;
}

static inline void bunix_alloc_list_push(struct bunix_alloc_block *block)
{
	const u64 index = bunix_alloc_index(block->order);

	block->free = 1;
	block->prev = 0;
	block->next = bunix_alloc_free[index];
	if (block->next != 0) {
		block->next->prev = block;
	}
	bunix_alloc_free[index] = block;
}

static inline void bunix_alloc_list_remove(struct bunix_alloc_block *block)
{
	const u64 index = bunix_alloc_index(block->order);

	if (block->prev != 0) {
		block->prev->next = block->next;
	} else {
		bunix_alloc_free[index] = block->next;
	}
	if (block->next != 0) {
		block->next->prev = block->prev;
	}
	block->free = 0;
	block->prev = 0;
	block->next = 0;
}

static inline void bunix_alloc_large_push(struct bunix_alloc_block *block)
{
	block->free = 1;
	block->prev = 0;
	block->next = bunix_alloc_large_free;
	if (block->next != 0) {
		block->next->prev = block;
	}
	bunix_alloc_large_free = block;
}

static inline void bunix_alloc_large_remove(struct bunix_alloc_block *block)
{
	if (block->prev != 0) {
		block->prev->next = block->next;
	} else {
		bunix_alloc_large_free = block->next;
	}
	if (block->next != 0) {
		block->next->prev = block->prev;
	}
	block->free = 0;
	block->prev = 0;
	block->next = 0;
}

static inline int bunix_alloc_add_arena(void)
{
	struct bunix_alloc_block *block;
	const u64 base = bunix_alloc_next;

	if (bunix_task_alloc(0, base, BUNIX_ALLOC_ARENA_SIZE, 1) != 0) {
		return -1;
	}
	bunix_alloc_next += BUNIX_ALLOC_ARENA_SIZE;
	block = (struct bunix_alloc_block *)base;
	block->order = BUNIX_ALLOC_MAX_ORDER;
	bunix_alloc_list_push(block);
	return 0;
}

static inline void *bunix_alloc_large(u64 size)
{
	struct bunix_alloc_block *block;
	const u64 order = bunix_alloc_large_order_for(size);
	const u64 block_size = order == 0 ? 0 : 1ull << order;

	if (order == 0) {
		return 0;
	}
	for (block = bunix_alloc_large_free; block != 0; block = block->next) {
		if (block->order >= order) {
			bunix_alloc_large_remove(block);
			block->free = 0;
			return (void *)(block + 1);
		}
	}
	bunix_alloc_next = bunix_alloc_align(bunix_alloc_next, block_size);
	if (bunix_alloc_next > ((u64)-1) - block_size) {
		return 0;
	}
	block = (struct bunix_alloc_block *)bunix_alloc_next;
	if (bunix_task_alloc(0, (u64)block, block_size, 1) != 0) {
		return 0;
	}
	bunix_alloc_next += block_size;
	block->order = order;
	block->free = 0;
	block->prev = 0;
	block->next = 0;
	return (void *)(block + 1);
}

static inline void *bunix_alloc(u64 size)
{
	u64 order = bunix_alloc_order_for(size);
	struct bunix_alloc_block *block = 0;

	if (order == 0) {
		return bunix_alloc_large(size);
	}

	for (;;) {
		for (u64 scan = order; scan <= BUNIX_ALLOC_MAX_ORDER; scan++) {
			if (bunix_alloc_free[bunix_alloc_index(scan)] != 0) {
				block = bunix_alloc_free[bunix_alloc_index(scan)];
				bunix_alloc_list_remove(block);
				break;
			}
		}
		if (block != 0) {
			break;
		}
		if (bunix_alloc_add_arena() != 0) {
			return 0;
		}
	}

	while (block->order > order) {
		struct bunix_alloc_block *buddy;

		block->order--;
		buddy = (struct bunix_alloc_block *)
			((u64)block + (1ull << block->order));
		buddy->order = block->order;
		bunix_alloc_list_push(buddy);
	}

	block->free = 0;
	return (void *)(block + 1);
}

static inline void bunix_free(void *ptr)
{
	struct bunix_alloc_block *block;

	if (ptr == 0) {
		return;
	}

	block = ((struct bunix_alloc_block *)ptr) - 1;
	if (block->order > BUNIX_ALLOC_MAX_ORDER) {
		bunix_alloc_large_push(block);
		return;
	}
	while (block->order < BUNIX_ALLOC_MAX_ORDER) {
		const u64 size = 1ull << block->order;
		const u64 arena_offset = ((u64)block - BUNIX_ALLOC_HEAP_BASE) &
					 (BUNIX_ALLOC_ARENA_SIZE - 1);
		const u64 buddy_offset = arena_offset ^ size;
		struct bunix_alloc_block *buddy;

		if ((arena_offset & ~(BUNIX_ALLOC_ARENA_SIZE - 1)) !=
		    (buddy_offset & ~(BUNIX_ALLOC_ARENA_SIZE - 1))) {
			break;
		}
		buddy = (struct bunix_alloc_block *)
			(((u64)block & ~(BUNIX_ALLOC_ARENA_SIZE - 1)) +
			 buddy_offset);
		if (buddy->free == 0 || buddy->order != block->order) {
			break;
		}
		bunix_alloc_list_remove(buddy);
		if ((u64)buddy < (u64)block) {
			block = buddy;
		}
		block->order++;
	}
	bunix_alloc_list_push(block);
}

static inline void *bunix_realloc(void *ptr, u64 old_size, u64 new_size)
{
	unsigned char *old_ptr = (unsigned char *)ptr;
	unsigned char *new_ptr;
	u64 copy = old_size < new_size ? old_size : new_size;

	if (ptr == 0) {
		return bunix_alloc(new_size);
	}
	if (new_size == 0) {
		bunix_free(ptr);
		return 0;
	}
	new_ptr = (unsigned char *)bunix_alloc(new_size);
	if (new_ptr == 0) {
		return 0;
	}
	for (u64 i = 0; i < copy; i++) {
		new_ptr[i] = old_ptr[i];
	}
	bunix_free(ptr);
	return new_ptr;
}

static inline void *bunix_calloc(u64 count, u64 size)
{
	const u64 total = count * size;
	unsigned char *ptr;

	if (size != 0 && total / size != count) {
		return 0;
	}
	ptr = (unsigned char *)bunix_alloc(total);
	if (ptr == 0) {
		return 0;
	}
	for (u64 i = 0; i < total; i++) {
		ptr[i] = 0;
	}
	return ptr;
}

#endif
