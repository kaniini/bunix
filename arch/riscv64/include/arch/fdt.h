#ifndef BUNIXOS_ARCH_RISCV64_FDT_H
#define BUNIXOS_ARCH_RISCV64_FDT_H

#include "types.h"

struct riscv64_fdt_memory_range {
	u64 base;
	u64 size;
};

struct riscv64_fdt_initrd {
	u64 start;
	u64 end;
};

int riscv64_fdt_scan_memory(const void *fdt,
			    struct riscv64_fdt_memory_range *ranges,
			    u32 capacity);
int riscv64_fdt_scan_initrd(const void *fdt,
			    struct riscv64_fdt_initrd *initrd);
u64 riscv64_fdt_total_size(const void *fdt);

#endif
