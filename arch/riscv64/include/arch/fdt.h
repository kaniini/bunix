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

#define RISCV64_FDT_MAX_PATH 128
#define RISCV64_FDT_MAX_COMPATIBLE 64
#define RISCV64_FDT_MAX_CPUS 16
#define RISCV64_FDT_MAX_DEVICES 8
#define RISCV64_FDT_MAX_ALIASES 16

struct riscv64_fdt_cpu {
	u64 hart_id;
};

struct riscv64_fdt_device {
	char path[RISCV64_FDT_MAX_PATH];
	char compatible[RISCV64_FDT_MAX_COMPATIBLE];
	u64 reg_base;
	u64 reg_size;
	u32 reg_shift;
	u32 reg_io_width;
};

struct riscv64_fdt_alias {
	char name[32];
	char path[RISCV64_FDT_MAX_PATH];
};

struct riscv64_fdt_platform {
	u32 cpu_count;
	u32 timebase_frequency;
	char stdout_path[RISCV64_FDT_MAX_PATH];
	char stdout_resolved_path[RISCV64_FDT_MAX_PATH];
	u32 stdout_uart_index;
	u32 stdout_uart_valid;
	u32 uart_count;
	u32 interrupt_controller_count;
	struct riscv64_fdt_cpu cpus[RISCV64_FDT_MAX_CPUS];
	struct riscv64_fdt_alias aliases[RISCV64_FDT_MAX_ALIASES];
	struct riscv64_fdt_device uarts[RISCV64_FDT_MAX_DEVICES];
	struct riscv64_fdt_device interrupt_controllers[RISCV64_FDT_MAX_DEVICES];
};

int riscv64_fdt_scan_memory(const void *fdt,
			    struct riscv64_fdt_memory_range *ranges,
			    u32 capacity);
int riscv64_fdt_scan_initrd(const void *fdt,
			    struct riscv64_fdt_initrd *initrd);
int riscv64_fdt_scan_platform(const void *fdt,
			      struct riscv64_fdt_platform *platform);
u64 riscv64_fdt_total_size(const void *fdt);

#endif
