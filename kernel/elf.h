#ifndef BUNIXOS_ELF_H
#define BUNIXOS_ELF_H

#include "types.h"

struct vm_space;

int elf_load_user_image(struct vm_space *space, u64 image_start, u64 image_end,
			u64 *entry);

#endif
