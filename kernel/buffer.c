#include "buffer.h"
#include "console.h"
#include "slab.h"
#include "spinlock.h"

enum {
	BUFFER_MAX_SIZE = 4096,
};

struct shared_buffer {
	u64 id;
	u64 size;
	struct spinlock lock;
	u8 data[BUFFER_MAX_SIZE];
};

static u64 next_buffer_id = 1;
static struct spinlock buffer_table_lock = SPINLOCK_INIT("buffer-table");

static void mem_copy(u8 *dst, const u8 *src, u64 len)
{
	for (u64 i = 0; i < len; i++) {
		dst[i] = src[i];
	}
}

void buffer_init(void)
{
	console_printf("buffer: init slab size=%u\n", BUFFER_MAX_SIZE);
}

struct shared_buffer *buffer_create(u64 size)
{
	if (size == 0 || size > BUFFER_MAX_SIZE) {
		return 0;
	}

	const u64 flags = spin_lock_irqsave(&buffer_table_lock);
	struct shared_buffer *buffer = slab_alloc(sizeof(*buffer));

	if (buffer == 0) {
		spin_unlock_irqrestore(&buffer_table_lock, flags);
		console_printf("buffer: alloc failed\n");
		return 0;
	}

	buffer->id = next_buffer_id++;
	buffer->size = size;
	spinlock_init(&buffer->lock, "buffer");
	spin_unlock_irqrestore(&buffer_table_lock, flags);
	console_printf("buffer: create id=%u size=%u\n",
		       (u32)buffer->id, (u32)size);
	return buffer;
}

void buffer_destroy(struct shared_buffer *buffer)
{
	if (buffer == 0) {
		return;
	}

	const u64 flags = spin_lock_irqsave(&buffer_table_lock);

	buffer->id = 0;
	buffer->size = 0;
	spin_unlock_irqrestore(&buffer_table_lock, flags);
	slab_free(buffer);
	console_printf("buffer: destroy\n");
}

u64 buffer_id(const struct shared_buffer *buffer)
{
	return buffer != 0 ? buffer->id : 0;
}

u64 buffer_size(const struct shared_buffer *buffer)
{
	return buffer != 0 ? buffer->size : 0;
}

int buffer_read(struct shared_buffer *buffer, u64 offset, void *dst, u64 len)
{
	if (buffer == 0 || dst == 0 || offset > buffer->size ||
	    len > buffer->size - offset) {
		return -1;
	}

	const u64 flags = spin_lock_irqsave(&buffer->lock);
	mem_copy((u8 *)dst, buffer->data + offset, len);
	spin_unlock_irqrestore(&buffer->lock, flags);
	console_printf("buffer: read id=%u offset=%u len=%u\n",
		       (u32)buffer->id, (u32)offset, (u32)len);
	return 0;
}

int buffer_write(struct shared_buffer *buffer, u64 offset, const void *src,
		 u64 len)
{
	if (buffer == 0 || src == 0 || offset > buffer->size ||
	    len > buffer->size - offset) {
		return -1;
	}

	const u64 flags = spin_lock_irqsave(&buffer->lock);
	mem_copy(buffer->data + offset, (const u8 *)src, len);
	spin_unlock_irqrestore(&buffer->lock, flags);
	console_printf("buffer: write id=%u offset=%u len=%u\n",
		       (u32)buffer->id, (u32)offset, (u32)len);
	return 0;
}
