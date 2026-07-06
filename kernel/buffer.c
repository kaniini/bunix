#include "buffer.h"
#include "console.h"
#include "slab.h"
#include "spinlock.h"

enum {
	BUFFER_MAX_SIZE = 2 * 1024 * 1024,
};

struct shared_buffer {
	u64 id;
	u64 size;
	u32 ref_count;
	struct spinlock lock;
	u8 *data;
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
	console_printf("buffer: init max=%u\n", BUFFER_MAX_SIZE);
}

struct shared_buffer *buffer_create(u64 size)
{
	if (size == 0 || size > BUFFER_MAX_SIZE) {
		return 0;
	}

	struct shared_buffer *buffer = slab_zalloc(sizeof(*buffer));

	if (buffer == 0) {
		console_printf("buffer: alloc failed\n");
		return 0;
	}

	buffer->data = slab_zalloc(size);
	if (buffer->data == 0) {
		slab_free(buffer);
		console_printf("buffer: data alloc failed size=%u\n",
			       (u32)size);
		return 0;
	}

	const u64 flags = spin_lock_irqsave(&buffer_table_lock);
	buffer->id = next_buffer_id++;
	buffer->size = size;
	buffer->ref_count = 1;
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

	if (buffer->ref_count != 0) {
		spin_unlock_irqrestore(&buffer_table_lock, flags);
		return;
	}
	buffer->id = 0;
	buffer->size = 0;
	u8 *data = buffer->data;
	buffer->data = 0;
	spin_unlock_irqrestore(&buffer_table_lock, flags);
	slab_free(data);
	slab_free(buffer);
	console_printf("buffer: destroy\n");
}

void buffer_retain(struct shared_buffer *buffer)
{
	if (buffer == 0) {
		return;
	}

	const u64 flags = spin_lock_irqsave(&buffer_table_lock);

	buffer->ref_count++;
	spin_unlock_irqrestore(&buffer_table_lock, flags);
}

void buffer_release(struct shared_buffer *buffer)
{
	if (buffer == 0) {
		return;
	}

	const u64 flags = spin_lock_irqsave(&buffer_table_lock);

	if (buffer->ref_count > 0) {
		buffer->ref_count--;
	}
	const u32 refs = buffer->ref_count;
	spin_unlock_irqrestore(&buffer_table_lock, flags);

	if (refs == 0) {
		buffer_destroy(buffer);
	}
}

u64 buffer_id(const struct shared_buffer *buffer)
{
	return buffer != 0 ? buffer->id : 0;
}

u64 buffer_size(const struct shared_buffer *buffer)
{
	return buffer != 0 ? buffer->size : 0;
}

u64 buffer_phys(const struct shared_buffer *buffer)
{
	return buffer != 0 ? (u64)buffer->data : 0;
}

int buffer_read(struct shared_buffer *buffer, u64 offset, void *dst, u64 len)
{
	if (buffer == 0 || buffer->data == 0 || dst == 0 ||
	    offset > buffer->size ||
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
	if (buffer == 0 || buffer->data == 0 || src == 0 ||
	    offset > buffer->size ||
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
