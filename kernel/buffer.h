#ifndef BUNIXOS_BUFFER_H
#define BUNIXOS_BUFFER_H

#include "types.h"

struct shared_buffer;

void buffer_init(void);
struct shared_buffer *buffer_create(u64 size);
u64 buffer_id(const struct shared_buffer *buffer);
u64 buffer_size(const struct shared_buffer *buffer);
int buffer_read(struct shared_buffer *buffer, u64 offset, void *dst, u64 len);
int buffer_write(struct shared_buffer *buffer, u64 offset, const void *src,
		 u64 len);

#endif
