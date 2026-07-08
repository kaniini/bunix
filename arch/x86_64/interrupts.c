#include <arch/interrupts.h>
#include <arch/io.h>
#include <arch/smp.h>
#include <arch/user.h>
#include "console.h"
#include "ipc.h"
#include "sched.h"
#include "vm.h"

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

	USER_FOURCC_LINX = ('L') | ('I' << 8) | ('N' << 16) | ('X' << 24),
	LINUX_RPC_TASK_FAULT = 1009,

	TASK_FAULT_ACCESS_READ = 1,
	TASK_FAULT_ACCESS_WRITE = 2,
	TASK_FAULT_ACCESS_EXEC = 3,
	TASK_FAULT_CLASS_UNKNOWN = 0,
	TASK_FAULT_CLASS_MAPPING = 1,
	TASK_FAULT_CLASS_PROTECTION = 2,
	TASK_FAULT_CLASS_BUS = 3,

	IPC_RIGHT_SEND = 1 << 0,
	LINUX_SIGNAL_FRAME_MAGIC = 0x534947424e555858ull,
	RFLAGS_AC = 1u << 18,
};

struct task_fault_event {
	u64 task;
	u64 thread;
	u64 trap;
	u64 error;
	u64 fault_addr;
	u64 ip;
	u64 sp;
	u64 flags;
	u64 arch0;
	u64 arch1;
};

struct interrupt_saved_regs {
	u64 r15;
	u64 r14;
	u64 r13;
	u64 r12;
	u64 r11;
	u64 r10;
	u64 r9;
	u64 r8;
	u64 rbp;
	u64 rdi;
	u64 rsi;
	u64 rdx;
	u64 rcx;
	u64 rbx;
	u64 rax;
};

struct linux_signal_frame {
	u64 magic;
	u64 saved_rax;
	u64 old_mask;
	struct arch_syscall_frame frame;
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

static int user_fault_signal_vector(u64 vector)
{
	return vector == 13 || vector == 14 || vector == 17;
}

static int user_fault_from_user_context(const struct arch_interrupt_frame *frame)
{
	return ((frame->cs & 3) == 3) ||
		task_sched_class(task_current()) == SCHED_CLASS_USER;
}

static u64 user_fault_access(const struct arch_interrupt_frame *frame)
{
	if (frame->vector == 14) {
		if ((frame->error_code & (1ull << 4)) != 0) {
			return TASK_FAULT_ACCESS_EXEC;
		}
		if ((frame->error_code & (1ull << 1)) != 0) {
			return TASK_FAULT_ACCESS_WRITE;
		}
	}
	return TASK_FAULT_ACCESS_READ;
}

static u64 user_fault_class(const struct arch_interrupt_frame *frame)
{
	if (frame->vector == 17) {
		return TASK_FAULT_CLASS_BUS;
	}
	if (frame->vector == 14) {
		return (frame->error_code & 1) != 0 ?
			TASK_FAULT_CLASS_PROTECTION :
			TASK_FAULT_CLASS_MAPPING;
	}
	if (frame->vector == 13) {
		return TASK_FAULT_CLASS_PROTECTION;
	}
	return TASK_FAULT_CLASS_UNKNOWN;
}

static int write_signal_frame(struct arch_interrupt_frame *frame,
			      struct interrupt_saved_regs *regs,
			      u64 signal, u64 handler, u64 restorer,
			      u64 old_mask)
{
	const struct linux_signal_frame signal_frame = {
		.magic = LINUX_SIGNAL_FRAME_MAGIC,
		.saved_rax = regs->rax,
		.old_mask = old_mask,
		.frame = {
			.number = regs->rax,
			.arg0 = regs->rdi,
			.arg1 = regs->rsi,
			.arg2 = regs->rdx,
			.arg3 = regs->r10,
			.user_rip = frame->rip,
			.user_rflags = frame->rflags,
			.user_rsp = frame->rsp,
			.rbx = regs->rbx,
			.rbp = regs->rbp,
			.r12 = regs->r12,
			.r13 = regs->r13,
			.r14 = regs->r14,
			.r15 = regs->r15,
			.r8 = regs->r8,
			.r9 = regs->r9,
		},
	};
	const u64 frame_addr =
		((frame->rsp - sizeof(signal_frame) - 8) & ~15ull) - 8;

	if (vm_write_user(task_vm_space(task_current()), frame_addr,
			  &restorer, sizeof(restorer)) != 0 ||
	    vm_write_user(task_vm_space(task_current()), frame_addr + 8,
			  &signal_frame, sizeof(signal_frame)) != 0) {
		return -1;
	}

	regs->rax = 0;
	regs->rdi = signal;
	regs->rsi = 0;
	regs->rdx = 0;
	regs->r10 = 0;
	regs->r8 = 0;
	regs->r9 = 0;
	frame->rip = handler;
	frame->rsp = frame_addr;
	frame->rflags &= ~((u64)RFLAGS_AC);
	return 0;
}

static int notify_linux_task_fault(struct arch_interrupt_frame *frame, u64 cr2)
{
	struct ipc_port *linux = ipc_port_find("linux");
	struct ipc_port *reply_port = task_reply_port(task_current());
	struct interrupt_saved_regs *regs =
		(struct interrupt_saved_regs *)((u64 *)frame - 15);
	const struct task_fault_event event = {
		.task = task_id(task_current()),
		.thread = thread_id(thread_current()),
		.trap = frame->vector,
		.error = frame->error_code,
		.fault_addr = cr2,
		.ip = frame->rip,
		.sp = frame->rsp,
		.flags = user_fault_access(frame) |
			(user_fault_class(frame) << 32),
		.arch0 = frame->cs,
		.arch1 = frame->rflags,
	};

	if (linux == 0) {
		console_printf("interrupts: user fault notify failed vector=%u rip=%p task=%u\n",
			       (u32)frame->vector, (const void *)frame->rip,
			       task_id(task_current()));
		return 0;
	}
	const struct ipc_message message = {
		.protocol = USER_FOURCC_LINX,
		.type = LINUX_RPC_TASK_FAULT,
		.sender = 0,
		.cap_rights = IPC_RIGHT_SEND,
		.reply_port = reply_port,
		.cap_type = IPC_CAP_NONE,
		.cap_object = 0,
		.words = { event.task, event.thread, event.trap, event.flags },
	};
	struct ipc_message reply;

	if (reply_port == 0 ||
	    ipc_send(linux, &message) != 0 ||
	    ipc_recv(reply_port, &reply) != 0) {
		console_printf("interrupts: user fault send failed vector=%u rip=%p task=%u\n",
			       (u32)frame->vector, (const void *)frame->rip,
			       task_id(task_current()));
		return 0;
	}

	if ((i64)reply.words[0] <= 0) {
		return 0;
	}
	if (write_signal_frame(frame, regs, reply.words[0], reply.words[1],
			       reply.words[2], reply.words[3]) != 0) {
		console_printf("interrupts: user fault signal frame failed vector=%u rip=%p task=%u\n",
			       (u32)frame->vector, (const void *)frame->rip,
			       task_id(task_current()));
		return 0;
	}
	return 1;
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

	if (user_fault_from_user_context(frame) &&
	    user_fault_signal_vector(frame->vector)) {
		if (notify_linux_task_fault(frame, cr2) != 0) {
			return;
		}
		(void)task_kill(task_current());
		thread_exit();
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
