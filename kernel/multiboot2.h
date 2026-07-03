#ifndef BUNIXOS_MULTIBOOT2_H
#define BUNIXOS_MULTIBOOT2_H

#include "types.h"

#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36d76289u

struct multiboot2_module {
	u64 start;
	u64 end;
	const char *cmdline;
};

struct multiboot2_mmap_entry {
	u64 base;
	u64 length;
	u32 type;
};

typedef void (*multiboot2_module_fn)(const struct multiboot2_module *module,
				     void *ctx);
typedef void (*multiboot2_mmap_fn)(const struct multiboot2_mmap_entry *entry,
				   void *ctx);

void multiboot2_dump(u64 info_addr);
void multiboot2_for_each_module(u64 info_addr, multiboot2_module_fn fn,
				void *ctx);
u64 multiboot2_total_size(u64 info_addr);
void multiboot2_for_each_mmap(u64 info_addr, multiboot2_mmap_fn fn, void *ctx);

#endif
