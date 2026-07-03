#ifndef BUNIXOS_ARCH_IO_H
#define BUNIXOS_ARCH_IO_H

#include "types.h"

static inline void arch_outb(u16 port, u8 value)
{
	__asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline u8 arch_inb(u16 port)
{
	u8 value;
	__asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
	return value;
}

#endif
