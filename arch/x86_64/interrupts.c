#include <arch/interrupts.h>
#include <arch/io.h>
#include <arch/smp.h>
#include <arch/user.h>
#include "console.h"
#include "ipc.h"
#include "sched.h"
#include "spinlock.h"
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
	IRQ_DEVICE_VECTOR_BASE = 66,
	IRQ_DEVICE_VECTOR_COUNT = 30,

	IOAPIC_REG_SELECT = 0x00,
	IOAPIC_REG_WINDOW = 0x10,
	IOAPIC_REDIR_BASE = 0x10,
	IOAPIC_REDIR_MASKED = 1 << 16,
	IOAPIC_REDIR_TRIGGER_LEVEL = 1 << 15,
	IOAPIC_REDIR_POLARITY_LOW = 1 << 13,
	IOAPIC_REDIR_DELIVERY_FIXED = 0,

	USER_FOURCC_LINX = ('L') | ('I' << 8) | ('N' << 16) | ('X' << 24),
	USER_FOURCC_HW = ('H') | ('W' << 8) | ('R' << 16) | ('0' << 24),
	LINUX_RPC_TASK_FAULT = 1009,
	HW_EVENT_IRQ = 14,

	TASK_FAULT_ACCESS_READ = 1,
	TASK_FAULT_ACCESS_WRITE = 2,
	TASK_FAULT_ACCESS_EXEC = 3,
	TASK_FAULT_CLASS_UNKNOWN = 0,
	TASK_FAULT_CLASS_MAPPING = 1,
	TASK_FAULT_CLASS_PROTECTION = 2,
	TASK_FAULT_CLASS_BUS = 3,

	IPC_RIGHT_SEND = 1 << 0,
	LINUX_SIGNAL_FRAME_MAGIC = 0x534947424e555858ull,
	LINUX_SIGBUS = 7,
	LINUX_SIGSEGV = 11,
	LINUX_SEGV_MAPERR = 1,
	LINUX_SEGV_ACCERR = 2,
	LINUX_BUS_ADRALN = 1,
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

struct linux_siginfo {
	u32 signo;
	u32 errno_value;
	u32 code;
	u32 pad0;
	u64 addr;
	u8 pad[104];
};

u64 arch_interrupts_save(void)
{
	u64 flags;

	__asm__ volatile ("pushfq; popq %0; cli" : "=r"(flags) : : "memory");
	return flags;
}

void arch_interrupts_restore(u64 flags)
{
	if ((flags & (1 << 9)) != 0) {
		__asm__ volatile ("sti" : : : "memory");
	}
}

void arch_interrupts_enable(void)
{
	__asm__ volatile ("sti" : : : "memory");
}

void arch_interrupts_disable(void)
{
	__asm__ volatile ("cli" : : : "memory");
}

void arch_cpu_wait_for_interrupt(void)
{
	__asm__ volatile ("hlt" : : : "memory");
}

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
static struct spinlock irq_lock = SPINLOCK_INIT("irq");

struct irq_binding {
	u32 active;
	u32 gsi;
	u32 input;
	u32 vector;
	u32 resource_flags;
	u32 masked;
	u32 pending;
	u64 delivery_count;
	u64 coalesced_count;
	u64 ioapic_base;
	struct ipc_port *port;
};

static struct irq_binding irq_bindings[IRQ_DEVICE_VECTOR_COUNT];

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
DECL_ISR(66);
DECL_ISR(67);
DECL_ISR(68);
DECL_ISR(69);
DECL_ISR(70);
DECL_ISR(71);
DECL_ISR(72);
DECL_ISR(73);
DECL_ISR(74);
DECL_ISR(75);
DECL_ISR(76);
DECL_ISR(77);
DECL_ISR(78);
DECL_ISR(79);
DECL_ISR(80);
DECL_ISR(81);
DECL_ISR(82);
DECL_ISR(83);
DECL_ISR(84);
DECL_ISR(85);
DECL_ISR(86);
DECL_ISR(87);
DECL_ISR(88);
DECL_ISR(89);
DECL_ISR(90);
DECL_ISR(91);
DECL_ISR(92);
DECL_ISR(93);
DECL_ISR(94);
DECL_ISR(95);

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

static volatile u32 *ioapic_reg(u64 base, u32 reg)
{
	return (volatile u32 *)(base + reg);
}

static u32 ioapic_read(u64 base, u32 reg)
{
	*ioapic_reg(base, IOAPIC_REG_SELECT) = reg;
	return *ioapic_reg(base, IOAPIC_REG_WINDOW);
}

static void ioapic_write(u64 base, u32 reg, u32 value)
{
	*ioapic_reg(base, IOAPIC_REG_SELECT) = reg;
	*ioapic_reg(base, IOAPIC_REG_WINDOW) = value;
}

static void irq_program_locked(const struct irq_binding *binding)
{
	u32 low = binding->vector | IOAPIC_REDIR_DELIVERY_FIXED;
	const u32 high = arch_smp_lapic_id(0) << 24;
	const u32 reg = IOAPIC_REDIR_BASE + binding->input * 2;
	struct vm_space *space = task_vm_space(task_current());

	if (binding->masked) {
		low |= IOAPIC_REDIR_MASKED;
	}
	if ((binding->resource_flags & TASK_HW_RESOURCE_IRQ_LEVEL_LOW) != 0) {
		low |= IOAPIC_REDIR_TRIGGER_LEVEL | IOAPIC_REDIR_POLARITY_LOW;
	}

	vm_rpc_activate_space(vm_kernel_space());
	ioapic_write(binding->ioapic_base, reg + 1, high);
	ioapic_write(binding->ioapic_base, reg, low);
	(void)ioapic_read(binding->ioapic_base, reg);
	vm_rpc_activate_space(space);
}

static struct irq_binding *irq_binding_for_gsi_locked(u32 gsi)
{
	for (u32 i = 0; i < IRQ_DEVICE_VECTOR_COUNT; i++) {
		if (irq_bindings[i].active && irq_bindings[i].gsi == gsi) {
			return &irq_bindings[i];
		}
	}
	return 0;
}

static struct irq_binding *irq_binding_for_vector_locked(u32 vector)
{
	if (vector < IRQ_DEVICE_VECTOR_BASE ||
	    vector >= IRQ_DEVICE_VECTOR_BASE + IRQ_DEVICE_VECTOR_COUNT) {
		return 0;
	}
	struct irq_binding *binding =
		&irq_bindings[vector - IRQ_DEVICE_VECTOR_BASE];
	return binding->active ? binding : 0;
}

int arch_irq_bind(u32 gsi, u32 resource_flags, struct ipc_port *port)
{
	u64 ioapic_base;
	u32 ioapic_id;
	u32 input;

	if (port == 0 ||
	    arch_smp_ioapic_for_gsi(gsi, &ioapic_base, &ioapic_id, &input) != 0 ||
	    vm_map_kernel_page(ioapic_base, ioapic_base, 1) != 0) {
		return -1;
	}

	const u64 flags = spin_lock_irqsave(&irq_lock);
	if (irq_binding_for_gsi_locked(gsi) != 0) {
		spin_unlock_irqrestore(&irq_lock, flags);
		return -1;
	}
	for (u32 i = 0; i < IRQ_DEVICE_VECTOR_COUNT; i++) {
		if (irq_bindings[i].active) {
			continue;
		}
		struct irq_binding *binding = &irq_bindings[i];
		binding->active = 1;
		binding->gsi = gsi;
		binding->input = input;
		binding->vector = IRQ_DEVICE_VECTOR_BASE + i;
		binding->resource_flags = resource_flags;
		binding->masked = 1;
		binding->pending = 0;
		binding->delivery_count = 0;
		binding->coalesced_count = 0;
		binding->ioapic_base = ioapic_base;
		binding->port = port;
		ipc_port_retain(port);
		irq_program_locked(binding);
		spin_unlock_irqrestore(&irq_lock, flags);
		console_printf("irq: bind gsi=%u vector=%u ioapic=%u input=%u\n",
			       gsi, binding->vector, ioapic_id, input);
		return 0;
	}
	spin_unlock_irqrestore(&irq_lock, flags);
	return -1;
}

int arch_irq_mask(u32 gsi, int masked)
{
	const u64 flags = spin_lock_irqsave(&irq_lock);
	struct irq_binding *binding = irq_binding_for_gsi_locked(gsi);

	if (binding == 0) {
		spin_unlock_irqrestore(&irq_lock, flags);
		return -1;
	}
	binding->masked = masked != 0;
	irq_program_locked(binding);
	spin_unlock_irqrestore(&irq_lock, flags);
	return 0;
}

int arch_irq_ack(u32 gsi)
{
	const u64 flags = spin_lock_irqsave(&irq_lock);
	struct irq_binding *binding = irq_binding_for_gsi_locked(gsi);

	if (binding == 0) {
		spin_unlock_irqrestore(&irq_lock, flags);
		return -1;
	}
	binding->pending = 0;
	binding->coalesced_count = 0;
	spin_unlock_irqrestore(&irq_lock, flags);
	return 0;
}

static int handle_device_irq(u32 vector)
{
	struct ipc_port *port;
	struct ipc_message message;
	u32 gsi;
	u64 delivery_count;
	u64 coalesced_count;

	const u64 flags = spin_lock_irqsave(&irq_lock);
	struct irq_binding *binding = irq_binding_for_vector_locked(vector);
	if (binding == 0) {
		spin_unlock_irqrestore(&irq_lock, flags);
		return -1;
	}

	binding->masked = 1;
	irq_program_locked(binding);
	if (binding->pending) {
		binding->coalesced_count++;
		gsi = binding->gsi;
		coalesced_count = binding->coalesced_count;
		spin_unlock_irqrestore(&irq_lock, flags);
		arch_smp_lapic_eoi();
		console_printf("irq: coalesced gsi=%u count=%u\n", gsi,
			       (u32)coalesced_count);
		return 0;
	}
	binding->pending = 1;
	binding->delivery_count++;
	port = binding->port;
	ipc_port_retain(port);
	gsi = binding->gsi;
	delivery_count = binding->delivery_count;
	coalesced_count = binding->coalesced_count;
	spin_unlock_irqrestore(&irq_lock, flags);

	message = (struct ipc_message){
		.protocol = USER_FOURCC_HW,
		.type = HW_EVENT_IRQ,
		.sender = 0,
		.cap_rights = 0,
		.reply_port = 0,
		.cap_type = IPC_CAP_NONE,
		.cap_object = 0,
		.words = { gsi, delivery_count, coalesced_count, vector },
	};
	(void)ipc_send(port, &message);
	ipc_port_release(port);
	arch_smp_lapic_eoi();
	return 0;
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
			      u64 old_mask, const struct task_fault_event *event)
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
	struct linux_siginfo siginfo = {
		.signo = (u32)signal,
		.errno_value = 0,
		.code = 0,
		.pad0 = 0,
		.addr = 0,
		.pad = { 0 },
	};
	const u64 frame_addr =
		((frame->rsp - sizeof(signal_frame) - sizeof(siginfo) - 8) &
		 ~15ull) - 8;
	const u64 signal_frame_addr = frame_addr + 8;
	const u64 siginfo_addr = signal_frame_addr + sizeof(signal_frame);

	if (event != 0 && signal == LINUX_SIGSEGV) {
		const u64 fault_class = event->flags >> 32;

		siginfo.code = fault_class == TASK_FAULT_CLASS_PROTECTION ?
			       LINUX_SEGV_ACCERR : LINUX_SEGV_MAPERR;
		siginfo.addr = event->fault_addr;
	} else if (event != 0 && signal == LINUX_SIGBUS) {
		siginfo.code = LINUX_BUS_ADRALN;
		siginfo.addr = event->fault_addr;
	}

	if (vm_write_user(task_vm_space(task_current()), frame_addr,
			  &restorer, sizeof(restorer)) != 0 ||
	    vm_write_user(task_vm_space(task_current()), signal_frame_addr,
			  &signal_frame, sizeof(signal_frame)) != 0 ||
	    vm_write_user(task_vm_space(task_current()), siginfo_addr,
			  &siginfo, sizeof(siginfo)) != 0) {
		return -1;
	}

	regs->rax = 0;
	regs->rdi = signal;
	regs->rsi = siginfo_addr;
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
			       reply.words[2], reply.words[3], &event) != 0) {
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

	if (frame->vector >= IRQ_DEVICE_VECTOR_BASE &&
	    frame->vector < IRQ_DEVICE_VECTOR_BASE + IRQ_DEVICE_VECTOR_COUNT &&
	    handle_device_irq((u32)frame->vector) == 0) {
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
	SET_ISR(66);
	SET_ISR(67);
	SET_ISR(68);
	SET_ISR(69);
	SET_ISR(70);
	SET_ISR(71);
	SET_ISR(72);
	SET_ISR(73);
	SET_ISR(74);
	SET_ISR(75);
	SET_ISR(76);
	SET_ISR(77);
	SET_ISR(78);
	SET_ISR(79);
	SET_ISR(80);
	SET_ISR(81);
	SET_ISR(82);
	SET_ISR(83);
	SET_ISR(84);
	SET_ISR(85);
	SET_ISR(86);
	SET_ISR(87);
	SET_ISR(88);
	SET_ISR(89);
	SET_ISR(90);
	SET_ISR(91);
	SET_ISR(92);
	SET_ISR(93);
	SET_ISR(94);
	SET_ISR(95);
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
