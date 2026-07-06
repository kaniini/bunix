#ifndef BUNIXOS_ARCH_POWER_H
#define BUNIXOS_ARCH_POWER_H

#include "types.h"

void arch_power_init(u64 multiboot_info);
void arch_poweroff(void) __attribute__((noreturn));

#endif
