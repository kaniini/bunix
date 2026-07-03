#ifndef BUNIXOS_TIMER_H
#define BUNIXOS_TIMER_H

#include "types.h"

u64 timer_ticks(void);
u64 timer_hz(void);
u64 timer_monotonic_ns(void);
u64 timer_ns_to_ticks_ceil(u64 ns);

#endif
