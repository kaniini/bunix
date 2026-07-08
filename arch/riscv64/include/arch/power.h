#ifndef BUNIXOS_ARCH_POWER_H
#define BUNIXOS_ARCH_POWER_H

#include "types.h"

void arch_power_init(u64 boot_info);
void arch_reboot(void) __attribute__((noreturn));
void arch_poweroff(void) __attribute__((noreturn));

#endif
