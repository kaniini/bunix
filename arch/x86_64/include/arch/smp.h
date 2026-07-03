#ifndef BUNIXOS_ARCH_SMP_H
#define BUNIXOS_ARCH_SMP_H

#include "types.h"

void arch_smp_init(u64 multiboot_info);
u32 arch_smp_cpu_count(void);
u32 arch_smp_lapic_id(u32 cpu_index);

#endif
