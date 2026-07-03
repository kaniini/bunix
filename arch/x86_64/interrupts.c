#include <arch/interrupts.h>
#include <arch/io.h>
#include <arch/smp.h>
#include "console.h"
#include "sched.h"

enum {
	IDT_ENTRIES = 256,
	KERNEL_CODE_SELECTOR = 0x08,

	PIC1_COMMAND = 0x20,
	PIC1_DATA = 0x21,
	PIC2_COMMAND = 0xa0,
	PIC2_DATA = 0xa1,
	PIC_EOI = 0x20,

	PIT_CHANNEL0 = 0x40,
	PIT_COMMAND = 0x43,
	PIT_HZ = 100,
	PIT_BASE_HZ = 1193182,

	IRQ_TIMER_VECTOR = 32,
	IRQ_SCHED_IPI_VECTOR = 64,
	IRQ_LAPIC_TIMER_VECTOR = 65,
};

struct idt_entry {
	u16 offset_low;
	u16 selector;
	u8 ist;
	u8 flags;
	u16 offset_mid;
	u32 offset_high;
	u32 zero;
} __attribute__((packed));

struct idt_ptr {
	u16 limit;
	u64 base;
} __attribute__((packed));

static struct idt_entry idt[IDT_ENTRIES];
static volatile u64 timer_ticks;

extern void isr_stub_0(void);
extern void isr_stub_6(void);
extern void isr_stub_8(void);
extern void isr_stub_13(void);
extern void isr_stub_14(void);
extern void isr_stub_32(void);
extern void isr_stub_64(void);
extern void isr_stub_65(void);

static void idt_set_gate_flags(u8 vector, void (*handler)(void), u8 flags)
{
	const u64 addr = (u64)handler;

	idt[vector].offset_low = addr & 0xffff;
	idt[vector].selector = KERNEL_CODE_SELECTOR;
	idt[vector].ist = 0;
	idt[vector].flags = flags;
	idt[vector].offset_mid = (addr >> 16) & 0xffff;
	idt[vector].offset_high = (addr >> 32) & 0xffffffff;
	idt[vector].zero = 0;
}

static void idt_set_gate(u8 vector, void (*handler)(void))
{
	idt_set_gate_flags(vector, handler, 0x8e);
}

void arch_interrupts_load(void)
{
	const struct idt_ptr ptr = {
		.limit = sizeof(idt) - 1,
		.base = (u64)idt,
	};

	__asm__ volatile ("lidt %0" : : "m"(ptr));
}

static void pic_remap(void)
{
	const u8 pic1_mask = arch_inb(PIC1_DATA);
	const u8 pic2_mask = arch_inb(PIC2_DATA);

	arch_outb(PIC1_COMMAND, 0x11);
	arch_outb(PIC2_COMMAND, 0x11);
	arch_outb(PIC1_DATA, 0x20);
	arch_outb(PIC2_DATA, 0x28);
	arch_outb(PIC1_DATA, 0x04);
	arch_outb(PIC2_DATA, 0x02);
	arch_outb(PIC1_DATA, 0x01);
	arch_outb(PIC2_DATA, 0x01);

	arch_outb(PIC1_DATA, pic1_mask & ~0x01);
	arch_outb(PIC2_DATA, pic2_mask);
}

static void pit_init(void)
{
	const u16 divisor = PIT_BASE_HZ / PIT_HZ;

	arch_outb(PIT_COMMAND, 0x36);
	arch_outb(PIT_CHANNEL0, divisor & 0xff);
	arch_outb(PIT_CHANNEL0, divisor >> 8);
	console_printf("timer: pit %uhz\n", PIT_HZ);
}

static void pic_eoi(u64 vector)
{
	if (vector >= 40) {
		arch_outb(PIC2_COMMAND, PIC_EOI);
	}
	arch_outb(PIC1_COMMAND, PIC_EOI);
}

u64 arch_timer_ticks(void)
{
	return timer_ticks;
}

void arch_interrupt_dispatch(struct arch_interrupt_frame *frame)
{
	if (frame->vector == IRQ_TIMER_VECTOR) {
		timer_ticks++;
		if (timer_ticks <= 3) {
			console_printf("timer: tick %u\n", (u32)timer_ticks);
		}
		sched_wake_sleepers(timer_ticks);
		pic_eoi(frame->vector);
		if ((frame->cs & 3) == 3) {
			sched_tick();
		}
		return;
	}

	if (frame->vector == IRQ_SCHED_IPI_VECTOR) {
		arch_smp_handle_scheduler_ipi();
		return;
	}

	if (frame->vector == IRQ_LAPIC_TIMER_VECTOR) {
		arch_smp_handle_timer_interrupt();
		if ((frame->cs & 3) == 3) {
			sched_tick();
		}
		return;
	}

	console_printf("interrupts: vector=%u error=0x%x rip=%p\n",
		       (u32)frame->vector, (u32)frame->error_code,
		       (const void *)frame->rip);

	for (;;) {
		__asm__ volatile ("cli; hlt");
	}
}

void arch_interrupts_init(void)
{
	idt_set_gate(0, isr_stub_0);
	idt_set_gate(6, isr_stub_6);
	idt_set_gate(8, isr_stub_8);
	idt_set_gate(13, isr_stub_13);
	idt_set_gate(14, isr_stub_14);
	idt_set_gate(IRQ_TIMER_VECTOR, isr_stub_32);
	idt_set_gate(IRQ_SCHED_IPI_VECTOR, isr_stub_64);
	idt_set_gate(IRQ_LAPIC_TIMER_VECTOR, isr_stub_65);
	arch_interrupts_load();
	console_printf("interrupts: idt loaded\n");

	pic_remap();
	pit_init();
	console_printf("interrupts: enabled\n");
	__asm__ volatile ("sti");
	while (timer_ticks < 3) {
		__asm__ volatile ("pause");
	}
}
