#include <arch/user.h>
#include <arch/io.h>
#include "console.h"
#include "ipc.h"
#include "name.h"
#include "sched.h"
#include "timer.h"
#include "server.h"
#include "../servers/vm/vm_server.h"

enum {
	GDT_KERNEL_CODE = 0x08,
	GDT_KERNEL_DATA = 0x10,
	GDT_USER_DATA = 0x1b,
	GDT_USER_CODE = 0x23,
	GDT_TSS = 0x28,
	MSR_EFER = 0xc0000080,
	MSR_STAR = 0xc0000081,
	MSR_LSTAR = 0xc0000082,
	MSR_FMASK = 0xc0000084,
	EFER_SCE = 1,
	SYSCALL_WRITE = -1,
	SYSCALL_EXIT = -2,
	SYSCALL_VM_PING = -3,
	SYSCALL_TIMER_TICKS = -4,
	SYSCALL_NAME_LOOKUP = -5,
	SYSCALL_SERVICE_WRITE = -6,
	SYSCALL_SERVICE_VM_PING = -7,
	SYSCALL_LAUNCH_MODULE = -8,
	SYSCALL_NAME_REGISTER = -9,
};

struct gdt_ptr {
	u16 limit;
	u64 base;
} __attribute__((packed));

struct tss {
	u32 reserved0;
	u64 rsp0;
	u64 rsp1;
	u64 rsp2;
	u64 reserved1;
	u64 ist[7];
	u64 reserved2;
	u16 reserved3;
	u16 iomap_base;
} __attribute__((packed));

static u64 gdt[7];
static struct tss kernel_tss;
static u8 interrupt_stack[16384] __attribute__((aligned(16)));

extern void arch_syscall_entry(void);

static void gdt_set_tss(u32 index, u64 base, u32 limit)
{
	gdt[index] = ((u64)(limit & 0xffff)) |
		     ((base & 0xffffff) << 16) |
		     ((u64)0x89 << 40) |
		     ((u64)((limit >> 16) & 0x0f) << 48) |
		     (((base >> 24) & 0xff) << 56);
	gdt[index + 1] = base >> 32;
}

void arch_user_init(void)
{
	const struct gdt_ptr ptr = {
		.limit = sizeof(gdt) - 1,
		.base = (u64)gdt,
	};

	gdt[0] = 0;
	gdt[1] = 0x00af9a000000ffff;
	gdt[2] = 0x00af92000000ffff;
	gdt[3] = 0x00aff2000000ffff;
	gdt[4] = 0x00affa000000ffff;

	kernel_tss.rsp0 = (u64)(interrupt_stack + sizeof(interrupt_stack));
	kernel_tss.iomap_base = sizeof(kernel_tss);
	gdt_set_tss(5, (u64)&kernel_tss, sizeof(kernel_tss) - 1);

	__asm__ volatile ("lgdt %0" : : "m"(ptr));
	__asm__ volatile (
		"movw %0, %%ds\n"
		"movw %0, %%es\n"
		"movw %0, %%ss\n"
		:
		: "r"((u16)GDT_KERNEL_DATA)
		: "memory");
	__asm__ volatile ("ltr %0" : : "r"((u16)GDT_TSS));

	arch_wrmsr(MSR_STAR, ((u64)0x13 << 48) | ((u64)GDT_KERNEL_CODE << 32));
	arch_wrmsr(MSR_LSTAR, (u64)arch_syscall_entry);
	arch_wrmsr(MSR_FMASK, 0x200);
	arch_wrmsr(MSR_EFER, arch_rdmsr(MSR_EFER) | EFER_SCE);

	console_printf("user: gdt/tss/syscall ready\n");
}

void arch_user_set_kernel_stack(u64 stack)
{
	kernel_tss.rsp0 = stack;
}

void arch_user_enter(u64 entry, u64 stack)
{
	console_printf("user: enter rip=%p rsp=%p\n",
		       (const void *)entry, (const void *)stack);

	__asm__ volatile (
		"cli\n"
		"movw $0x1b, %%ax\n"
		"movw %%ax, %%ds\n"
		"movw %%ax, %%es\n"
		"pushq $0x1b\n"
		"pushq %[stack]\n"
		"pushfq\n"
		"popq %%rax\n"
		"orq $0x200, %%rax\n"
		"pushq %%rax\n"
		"pushq $0x23\n"
		"pushq %[entry]\n"
		"iretq\n"
		:
		: [entry] "r"(entry),
		  [stack] "r"(stack)
		: "rax", "memory");

	for (;;) {
		__asm__ volatile ("hlt");
	}
}

u64 arch_syscall_dispatch(u64 number, u64 arg0, u64 arg1, u64 arg2)
{
	(void)arg2;

	switch ((i64)number) {
	case SYSCALL_WRITE: {
		const char *text = (const char *)arg0;
		for (u64 i = 0; i < arg1; i++) {
			console_putc(text[i]);
		}
		return arg1;
	}
	case SYSCALL_EXIT:
		console_printf("syscall: exit status=%u\n", (u32)arg0);
		__asm__ volatile ("sti");
		thread_exit();
	case SYSCALL_VM_PING: {
		struct ipc_port *vm_port = ipc_port_find("vm");
		struct ipc_message message = {
			.type = VM_IPC_EVENT_PING,
			.sender = 0,
			.reply_port = 0,
			.words = { arg0, 0, 0, 0 },
		};

		if (vm_port == 0) {
			return (u64)-1;
		}

		return (u64)ipc_send(vm_port, &message);
	}
	case SYSCALL_TIMER_TICKS:
		return timer_ticks();
	case SYSCALL_NAME_LOOKUP:
		return name_service_lookup((const char *)arg0);
	case SYSCALL_SERVICE_WRITE:
		if (name_service_kind(arg0) != NAME_SERVICE_CONSOLE) {
			return (u64)-1;
		}
		for (u64 i = 0; i < arg2; i++) {
			console_putc(((const char *)arg1)[i]);
		}
		return arg2;
	case SYSCALL_SERVICE_VM_PING: {
		if (name_service_kind(arg0) != NAME_SERVICE_IPC_PORT) {
			return (u64)-1;
		}

		struct ipc_port *vm_port =
			(struct ipc_port *)name_service_object(arg0);
		struct ipc_message message = {
			.type = VM_IPC_EVENT_PING,
			.sender = 0,
			.reply_port = 0,
			.words = { arg1, 0, 0, 0 },
		};

		return (u64)ipc_send(vm_port, &message);
	}
	case SYSCALL_LAUNCH_MODULE:
		return (u64)server_launch_module((const char *)arg0);
	case SYSCALL_NAME_REGISTER:
		return name_service_register((const char *)arg0,
					     NAME_SERVICE_TASK,
					     task_id(task_current()));
	default:
		console_printf("syscall: unknown number=%u\n", (u32)number);
		return (u64)-1;
	}
}
