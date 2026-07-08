#ifndef BUNIXOS_ARCH_RISCV64_BOOT_H
#define BUNIXOS_ARCH_RISCV64_BOOT_H

#include <arch/layout.h>
#include "types.h"

struct riscv64_boot_info {
	u64 hart_id;
	u64 fdt;
	u64 phys_base;
	u64 phys_size;
	u64 kernel_load_base;
	u64 direct_map_base;
	u64 direct_map_size;
	u64 user_base;
	u64 user_limit;
	u64 initrd_start;
	u64 initrd_end;
};

const struct riscv64_boot_info *riscv64_boot_info(void);

#endif
