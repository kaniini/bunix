#include "timer.h"
#include <arch/interrupts.h>

enum {
	TIMER_NSEC_PER_SEC = 1000000000ULL,
};

u64 timer_ticks(void)
{
	return arch_timer_ticks();
}

u64 timer_hz(void)
{
	return arch_timer_hz();
}

u64 timer_monotonic_ns(void)
{
	return (timer_ticks() * TIMER_NSEC_PER_SEC) / timer_hz();
}

u64 timer_ns_to_ticks_ceil(u64 ns)
{
	const u64 hz = timer_hz();
	const u64 seconds = ns / TIMER_NSEC_PER_SEC;
	const u64 remainder = ns % TIMER_NSEC_PER_SEC;

	if (ns == 0) {
		return 0;
	}

	return (seconds * hz) +
	       ((remainder * hz) + TIMER_NSEC_PER_SEC - 1) /
	       TIMER_NSEC_PER_SEC;
}
