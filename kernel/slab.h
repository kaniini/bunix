#ifndef BUNIXOS_SLAB_H
#define BUNIXOS_SLAB_H

#include "types.h"

void slab_init(void);
void *slab_alloc(u64 size);
void *slab_zalloc(u64 size);
void slab_free(void *ptr);

#endif
