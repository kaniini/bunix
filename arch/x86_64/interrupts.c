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

#define DECL_ISR(vector) extern void isr_stub_##vector(void)
DECL_ISR(0);
DECL_ISR(1);
DECL_ISR(2);
DECL_ISR(3);
DECL_ISR(4);
DECL_ISR(5);
DECL_ISR(6);
DECL_ISR(7);
DECL_ISR(8);
DECL_ISR(9);
DECL_ISR(10);
DECL_ISR(11);
DECL_ISR(12);
DECL_ISR(13);
DECL_ISR(14);
DECL_ISR(15);
DECL_ISR(16);
DECL_ISR(17);
DECL_ISR(18);
DECL_ISR(19);
DECL_ISR(20);
DECL_ISR(21);
DECL_ISR(22);
DECL_ISR(23);
DECL_ISR(24);
DECL_ISR(25);
DECL_ISR(26);
DECL_ISR(27);
DECL_ISR(28);
DECL_ISR(29);
DECL_ISR(30);
DECL_ISR(31);
DECL_ISR(32);
DECL_ISR(64);
DECL_ISR(65);

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

u64 arch_timer_hz(void)
{
	return PIT_HZ;
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

	u64 cr2 = 0;

	if (frame->vector == 14) {
		__asm__ volatile ("movq %%cr2, %0" : "=r"(cr2));
	}
	console_printf("interrupts: vector=%u error=0x%x rip=%p cr2=%p rsp=%p rflags=0x%x task=%u thread=%u task_name=%s thread_name=%s\n",
		       (u32)frame->vector, (u32)frame->error_code,
		       (const void *)frame->rip, (const void *)cr2,
		       (const void *)frame->rsp, (u32)frame->rflags,
		       task_id(task_current()),
		       thread_id(thread_current()), task_name(task_current()),
		       thread_name(thread_current()));

	for (;;) {
		__asm__ volatile ("cli; hlt");
	}
}

void arch_interrupts_init(void)
{
#define SET_ISR(vector) idt_set_gate(vector, isr_stub_##vector)
	idt_set_gate(0, isr_stub_0);
	SET_ISR(1);
	SET_ISR(2);
	SET_ISR(3);
	SET_ISR(4);
	SET_ISR(5);
	SET_ISR(6);
	SET_ISR(7);
	SET_ISR(8);
	SET_ISR(9);
	SET_ISR(10);
	SET_ISR(11);
	SET_ISR(12);
	SET_ISR(13);
	SET_ISR(14);
	SET_ISR(15);
	SET_ISR(16);
	SET_ISR(17);
	SET_ISR(18);
	SET_ISR(19);
	SET_ISR(20);
	SET_ISR(21);
	SET_ISR(22);
	SET_ISR(23);
	SET_ISR(24);
	SET_ISR(25);
	SET_ISR(26);
	SET_ISR(27);
	SET_ISR(28);
	SET_ISR(29);
	SET_ISR(30);
	SET_ISR(31);
	SET_ISR(32);
	SET_ISR(64);
	SET_ISR(65);
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
