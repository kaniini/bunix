#ifndef BUNIXOS_ELF_H
#define BUNIXOS_ELF_H

#include "types.h"

int elf_load_user_image(u64 image_start, u64 image_end, u64 *entry);

#endif
