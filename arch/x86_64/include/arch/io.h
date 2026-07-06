#ifndef BUNIXOS_ARCH_IO_H
#define BUNIXOS_ARCH_IO_H

#include "types.h"

static inline void arch_outb(u16 port, u8 value)
{
	__asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline void arch_outw(u16 port, u16 value)
{
	__asm__ volatile ("outw %0, %1" : : "a"(value), "Nd"(port));
}

static inline void arch_outl(u16 port, u32 value)
{
	__asm__ volatile ("outl %0, %1" : : "a"(value), "Nd"(port));
}

static inline u8 arch_inb(u16 port)
{
	u8 value;
	__asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
	return value;
}

static inline u16 arch_inw(u16 port)
{
	u16 value;
	__asm__ volatile ("inw %1, %0" : "=a"(value) : "Nd"(port));
	return value;
}

static inline u32 arch_inl(u16 port)
{
	u32 value;
	__asm__ volatile ("inl %1, %0" : "=a"(value) : "Nd"(port));
	return value;
}

static inline void arch_wrmsr(u32 msr, u64 value)
{
	const u32 low = value & 0xffffffff;
	const u32 high = value >> 32;

	__asm__ volatile ("wrmsr" : : "c"(msr), "a"(low), "d"(high));
}

static inline u64 arch_rdmsr(u32 msr)
{
	u32 low;
	u32 high;

	__asm__ volatile ("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
	return ((u64)high << 32) | low;
}

#endif
