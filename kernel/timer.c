#include "timer.h"
#include <arch/interrupts.h>

u64 timer_ticks(void)
{
	return arch_timer_ticks();
}
