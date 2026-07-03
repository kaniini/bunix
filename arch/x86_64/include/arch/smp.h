#ifndef BUNIXOS_ARCH_SMP_H
#define BUNIXOS_ARCH_SMP_H

#include "types.h"

void arch_smp_init(u64 multiboot_info);
void arch_smp_release_aps(void);
u32 arch_smp_cpu_count(void);
u32 arch_smp_current_cpu_id(void);
u32 arch_smp_lapic_id(u32 cpu_index);
u64 arch_smp_lapic_address(void);
void arch_smp_send_scheduler_ipi(u32 cpu_index);
void arch_smp_handle_scheduler_ipi(void);
void arch_smp_handle_timer_interrupt(void);
u32 arch_smp_started_count(void);

#endif
