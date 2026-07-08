#ifndef BUNIXOS_ARCH_RISCV64_FDT_H
#define BUNIXOS_ARCH_RISCV64_FDT_H

#include "types.h"

struct riscv64_fdt_memory_range {
	u64 base;
	u64 size;
};

int riscv64_fdt_scan_memory(const void *fdt,
			    struct riscv64_fdt_memory_range *ranges,
			    u32 capacity);

#endif
