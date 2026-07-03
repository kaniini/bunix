#include <arch/user.h>
#include <arch/io.h>
#include <arch/smp.h>
#include "buffer.h"
#include "console.h"
#include "ipc.h"
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
	SYSCALL_EXIT = -2,
	SYSCALL_TIMER_TICKS = -4,
	SYSCALL_LAUNCH_MODULE = -8,
	SYSCALL_PORT_CREATE = -10,
	SYSCALL_IPC_SEND = -12,
	SYSCALL_IPC_RECV = -13,
	SYSCALL_IPC_CALL = -14,
	SYSCALL_HANDLE_CLOSE = -16,
	SYSCALL_BOOT_MODULE_READ = -18,
	SYSCALL_CLOCK_MONOTONIC_NS = -20,
	SYSCALL_SLEEP_NS = -22,
	SYSCALL_TASK_CREATE = -24,
	SYSCALL_TASK_MAP = -26,
	SYSCALL_TASK_GRANT = -28,
	SYSCALL_TASK_START = -30,
	SYSCALL_BUFFER_CREATE = -32,
	SYSCALL_BUFFER_READ = -34,
	SYSCALL_BUFFER_WRITE = -36,
	SYSCALL_TASK_WRITE = -38,
	SYSCALL_TASK_START_AT = -40,
	USER_IPC_WORDS = 4,
	USER_FOURCC_CONS = ('C') | ('O' << 8) | ('N' << 16) | ('S' << 24),
	USER_CONSOLE_WRITE = 1,
	ARCH_USER_MAX_CPUS = 8,
};

struct user_ipc_message {
	u32 protocol;
	u32 type;
	u32 sender;
	u32 cap_rights;
	u64 reply;
	u64 cap;
	u64 words[USER_IPC_WORDS];
};

static int user_message_to_ipc(const struct user_ipc_message *user_message,
			       struct ipc_message *message)
{
	message->protocol = user_message->protocol;
	message->type = user_message->type;
	message->sender = 0;
	message->cap_rights = user_message->cap_rights;
	message->reply_port = task_port_from_handle(task_current(),
						    user_message->reply,
						    TASK_RIGHT_SEND);
	message->cap_type = IPC_CAP_NONE;
	message->cap_object = 0;
	if (user_message->cap == 0 && user_message->cap_rights != 0) {
		return -1;
	}

	if (user_message->cap != 0) {
		const u32 valid_rights = TASK_RIGHT_SEND | TASK_RIGHT_RECV |
					 TASK_RIGHT_DUP;
		const u32 rights = user_message->cap_rights | TASK_RIGHT_DUP;

		if ((user_message->cap_rights & ~valid_rights) != 0) {
			return -1;
		}

		enum task_cap_type type = TASK_CAP_NONE;
		void *object = 0;

		if (task_export_cap(task_current(), user_message->cap, rights,
				    &type, &object) != 0 ||
		    user_message->cap_rights == 0) {
			return -1;
		}

		message->cap_type = type == TASK_CAP_PORT ?
			IPC_CAP_PORT : IPC_CAP_BUFFER;
		message->cap_object = object;
	}

	for (u64 i = 0; i < USER_IPC_WORDS; i++) {
		message->words[i] = user_message->words[i];
	}

	return 0;
}

static void ipc_message_to_user(const struct ipc_message *message,
				struct user_ipc_message *user_message)
{
	user_message->protocol = message->protocol;
	user_message->type = message->type;
	user_message->sender = message->sender;
	user_message->cap_rights = message->cap_rights;
	user_message->reply = task_grant_port(task_current(), message->reply_port,
					      TASK_RIGHT_SEND);
	user_message->cap = 0;
	if (message->cap_type == IPC_CAP_PORT) {
		user_message->cap =
			task_grant_port(task_current(),
					(struct ipc_port *)message->cap_object,
					message->cap_rights);
	} else if (message->cap_type == IPC_CAP_BUFFER) {
		user_message->cap =
			task_grant_buffer(task_current(),
					  (struct shared_buffer *)message->cap_object,
					  message->cap_rights);
	}
	for (u64 i = 0; i < USER_IPC_WORDS; i++) {
		user_message->words[i] = message->words[i];
	}
}

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

struct arch_user_cpu {
	u64 gdt[7];
	struct tss tss;
	u8 interrupt_stack[16384] __attribute__((aligned(16)));
};

static struct arch_user_cpu user_cpus[ARCH_USER_MAX_CPUS];
u64 arch_current_syscall_stack[ARCH_USER_MAX_CPUS];

extern void arch_syscall_entry(void);

static int str_eq(const char *left, const char *right)
{
	while (*left != '\0' && *right != '\0') {
		if (*left++ != *right++) {
			return 0;
		}
	}

	return *left == *right;
}

static void gdt_set_tss(struct arch_user_cpu *cpu, u32 index, u64 base,
			u32 limit)
{
	cpu->gdt[index] = ((u64)(limit & 0xffff)) |
			  ((base & 0xffffff) << 16) |
			  ((u64)0x89 << 40) |
			  ((u64)((limit >> 16) & 0x0f) << 48) |
			  (((base >> 24) & 0xff) << 56);
	cpu->gdt[index + 1] = base >> 32;
}

void arch_user_init_cpu(u32 cpu_id)
{
	struct arch_user_cpu *cpu = &user_cpus[cpu_id];
	const struct gdt_ptr ptr = {
		.limit = sizeof(cpu->gdt) - 1,
		.base = (u64)cpu->gdt,
	};

	cpu->gdt[0] = 0;
	cpu->gdt[1] = 0x00af9a000000ffff;
	cpu->gdt[2] = 0x00af92000000ffff;
	cpu->gdt[3] = 0x00aff2000000ffff;
	cpu->gdt[4] = 0x00affa000000ffff;

	cpu->tss.rsp0 = (u64)(cpu->interrupt_stack + sizeof(cpu->interrupt_stack));
	cpu->tss.iomap_base = sizeof(cpu->tss);
	gdt_set_tss(cpu, 5, (u64)&cpu->tss, sizeof(cpu->tss) - 1);

	__asm__ volatile ("lgdt %0" : : "m"(ptr));
	/* APs arrive on the trampoline GDT; reload CS before interrupts return. */
	__asm__ volatile (
		"pushq %[code]\n"
		"leaq 1f(%%rip), %%rax\n"
		"pushq %%rax\n"
		"lretq\n"
		"1:\n"
		:
		: [code] "i"(GDT_KERNEL_CODE)
		: "rax", "memory");
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

	if (cpu_id == 0) {
		console_printf("user: gdt/tss/syscall ready\n");
	}
}

void arch_user_init(void)
{
	arch_user_init_cpu(arch_smp_current_cpu_id());
}

void arch_user_set_kernel_stack(u64 stack)
{
	const u32 cpu_id = arch_smp_current_cpu_id();

	user_cpus[cpu_id].tss.rsp0 = stack;
	arch_current_syscall_stack[cpu_id] = stack;
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
	switch ((i64)number) {
	case SYSCALL_EXIT:
		console_printf("syscall: exit status=%u\n", (u32)arg0);
		__asm__ volatile ("sti");
		thread_exit();
	case SYSCALL_TIMER_TICKS:
		return timer_ticks();
	case SYSCALL_CLOCK_MONOTONIC_NS:
		return timer_monotonic_ns();
	case SYSCALL_SLEEP_NS:
		thread_sleep_ns(arg0);
		return 0;
	case SYSCALL_LAUNCH_MODULE:
		return server_launch_module_with_caps((const char *)arg0,
						      task_current(),
						      (const struct task_launch_cap *)arg1,
						      arg2);
	case SYSCALL_TASK_CREATE:
		return server_task_create(task_current(), (const char *)arg0);
	case SYSCALL_TASK_MAP: {
		const u64 *args = (const u64 *)arg0;

		if (args == 0) {
			return (u64)-1;
		}

		return (u64)server_task_map(task_current(), args[0], args[1],
					    (const void *)args[2], args[3],
					    args[4], (u32)args[5]);
	}
	case SYSCALL_TASK_GRANT:
		return (u64)server_task_grant(task_current(), arg0, arg1,
					      (u32)arg2);
	case SYSCALL_TASK_START:
		return (u64)server_task_start(task_current(), arg0, arg1);
	case SYSCALL_TASK_WRITE: {
		const u64 *args = (const u64 *)arg0;

		if (args == 0) {
			return (u64)-1;
		}

		return (u64)server_task_write(task_current(), args[0], args[1],
					      (const void *)args[2], args[3]);
	}
	case SYSCALL_TASK_START_AT:
		return (u64)server_task_start_at(task_current(), arg0, arg1,
						 arg2);
	case SYSCALL_BUFFER_CREATE: {
		struct shared_buffer *buffer = buffer_create(arg0);
		const u64 handle =
			task_grant_buffer(task_current(), buffer,
					  TASK_RIGHT_SEND | TASK_RIGHT_RECV |
					  TASK_RIGHT_DUP);

		return handle != 0 ? handle : (u64)-1;
	}
	case SYSCALL_BUFFER_READ: {
		const u64 *args = (const u64 *)arg0;

		if (args == 0) {
			return (u64)-1;
		}

		struct shared_buffer *buffer =
			task_buffer_from_handle(task_current(), args[0],
						TASK_RIGHT_RECV);
		return (u64)buffer_read(buffer, args[1], (void *)args[2],
					args[3]);
	}
	case SYSCALL_BUFFER_WRITE: {
		const u64 *args = (const u64 *)arg0;

		if (args == 0) {
			return (u64)-1;
		}

		struct shared_buffer *buffer =
			task_buffer_from_handle(task_current(), args[0],
						TASK_RIGHT_SEND);
		return (u64)buffer_write(buffer, args[1],
					 (const void *)args[2], args[3]);
	}
	case SYSCALL_PORT_CREATE:
		return task_grant_port(task_current(),
				       ipc_port_create_private((const char *)arg0),
				       TASK_RIGHT_SEND | TASK_RIGHT_RECV |
				       TASK_RIGHT_DUP);
	case SYSCALL_HANDLE_CLOSE:
		return (u64)task_close_handle(task_current(), arg0);
	case SYSCALL_BOOT_MODULE_READ:
		if (arg1 == 0) {
			return server_boot_module_size();
		}
		return (u64)server_boot_module_read(arg0, (void *)arg1, arg2);
	case SYSCALL_IPC_SEND: {
		const struct user_ipc_message *user_message =
			(const struct user_ipc_message *)arg1;
		struct ipc_port *port =
			task_port_from_handle(task_current(), arg0,
					      TASK_RIGHT_SEND);

		if (user_message == 0) {
			return (u64)-1;
		}

		if (port != 0 &&
		    user_message->protocol == USER_FOURCC_CONS &&
		    user_message->type == USER_CONSOLE_WRITE &&
		    str_eq(ipc_port_name(port), "console")) {
			const char *text = (const char *)user_message->words[0];
			const u64 len = user_message->words[1];

			console_write_len(text, len);
			return 0;
		}

		struct ipc_message message = {
			.protocol = 0,
			.type = 0,
		};

		if (user_message_to_ipc(user_message, &message) != 0) {
			return (u64)-1;
		}

		return (u64)ipc_send(port, &message);
	}
	case SYSCALL_IPC_RECV: {
		struct user_ipc_message *user_message =
			(struct user_ipc_message *)arg1;
		struct ipc_port *port =
			task_port_from_handle(task_current(), arg0,
					      TASK_RIGHT_RECV);
		struct ipc_message message;

		if (user_message == 0 ||
		    ipc_recv(port, &message) != 0) {
			return (u64)-1;
		}

		ipc_message_to_user(&message, user_message);
		return 0;
	}
	case SYSCALL_IPC_CALL: {
		const struct user_ipc_message *request =
			(const struct user_ipc_message *)arg1;
		struct user_ipc_message *user_reply =
			(struct user_ipc_message *)arg2;
		struct ipc_port *port =
			task_port_from_handle(task_current(), arg0,
					      TASK_RIGHT_SEND);
		struct ipc_port *reply_port = task_reply_port(task_current());
		struct ipc_message message = { .protocol = 0, .type = 0 };
		struct ipc_message reply;

		if (request == 0 || user_reply == 0 || port == 0 ||
		    reply_port == 0) {
			return (u64)-1;
		}

		if (user_message_to_ipc(request, &message) != 0) {
			return (u64)-1;
		}

		message.reply_port = reply_port;
		if (ipc_send(port, &message) != 0 ||
		    ipc_recv(reply_port, &reply) != 0) {
			return (u64)-1;
		}

		ipc_message_to_user(&reply, user_reply);
		return 0;
	}
	default:
		console_printf("syscall: unknown number=%u\n", (u32)number);
		return (u64)-1;
	}
}
