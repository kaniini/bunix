#include <arch/user.h>
#include <arch/interrupts.h>
#include <arch/layout.h>
#include <arch/power.h>
#include <arch/sbi.h>
#include "buffer.h"
#include "cmdline.h"
#include "console.h"
#include "ipc.h"
#include "sched.h"
#include "server.h"
#include "timer.h"

enum {
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
	SYSCALL_BUFFER_CREATE = -32,
	SYSCALL_BUFFER_READ = -34,
	SYSCALL_BUFFER_WRITE = -36,
	SYSCALL_TASK_ID = -42,
	SYSCALL_TASK_ALLOC = -44,
	SYSCALL_EARLY_CONSOLE_WRITE = -54,
	SYSCALL_EARLY_CONSOLE_LOG = -56,
	SYSCALL_MACHINE_POWER = -64,
	LINUX_RISCV64_WRITE = 64,
	LINUX_RISCV64_EXIT = 93,
	LINUX_RISCV64_EXIT_GROUP = 94,
	LINUX_RISCV64_SET_TID_ADDRESS = 96,
	LINUX_RISCV64_RPC_SYSCALL = 1010,
	LINUX_RISCV64_ACTION_WRITE = 1,
	LINUX_RISCV64_ACTION_EXIT = 2,
	LINUX_SHARED_WRITE = 1,
	LINUX_SHARED_SET_TID_ADDRESS = 218,
	LINUX_SHARED_REGISTER_PROCESS = 1000,
	LINUX_SHARED_EXIT_GROUP = 231,
	LINUX_ENOMEM = 12,
	LINUX_EFAULT = 14,
	LINUX_EINVAL = 22,
	LINUX_ENOSYS = 38,
	LINUX_MAX_SYSCALL_BUFFER = 4096,
	USER_FOURCC_LINX = ('L') | ('I' << 8) | ('N' << 16) | ('X' << 24),
	USER_IPC_WORDS = 4,
	RISCV64_SSTATUS_SUM = 1ULL << 18,
	RISCV64_USER_COPY_CHUNK = 256,
	RISCV64_MAX_CSTR = 256,
	MAX_INHERITED_CAPS = 8,
};

struct native_syscall_args {
	u64 arg0;
	u64 arg1;
	u64 arg2;
	u64 arg3;
};

struct native_syscall_entry {
	i64 number;
	const char *name;
	u64 (*fn)(const struct native_syscall_args *args);
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

static const char *strace_mode;
static u64 current_kernel_stack;
static u32 linux_registered_task;

void riscv64_user_enter(u64 entry, u64 stack, u64 kernel_stack)
	__attribute__((noreturn));

void arch_user_init(void)
{
	strace_mode = "off";
	current_kernel_stack = 0;
}

void arch_user_set_strace_mode(const char *mode)
{
	strace_mode = mode;
}

void arch_user_init_cpu(u32 cpu_id)
{
	(void)cpu_id;
}

void arch_user_set_kernel_stack(u64 stack)
{
	current_kernel_stack = stack;
}

void arch_user_set_fs_base(u64 fs_base)
{
	(void)fs_base;
}

u16 arch_user_elf_machine(void)
{
	return 243;
}

void arch_user_enter(u64 entry, u64 stack)
{
	riscv64_user_enter(entry, stack, current_kernel_stack);
}

void arch_user_resume(const struct arch_syscall_frame *frame)
{
	(void)frame;
	for (;;) {
		__asm__ volatile ("wfi");
	}
}

static u64 riscv64_sstatus_read(void)
{
	u64 value;

	__asm__ volatile ("csrr %0, sstatus" : "=r"(value));
	return value;
}

static void riscv64_sstatus_write(u64 value)
{
	__asm__ volatile ("csrw sstatus, %0" : : "r"(value) : "memory");
}

static void mem_copy(u8 *dst, const u8 *src, u64 len)
{
	for (u64 i = 0; i < len; i++) {
		dst[i] = src[i];
	}
}

static u64 min_u64(u64 left, u64 right)
{
	return left < right ? left : right;
}

static int user_copy_args_valid(const void *kernel, u64 user, u64 len)
{
	if (len == 0) {
		return 0;
	}
	return kernel != 0 && riscv64_user_addr_valid(user, len);
}

int arch_user_copy_from(void *dst, u64 user_src, u64 len)
{
	const u64 saved_status = riscv64_sstatus_read();

	if (!user_copy_args_valid(dst, user_src, len)) {
		return -1;
	}

	riscv64_sstatus_write(saved_status | RISCV64_SSTATUS_SUM);
	mem_copy((u8 *)dst, (const u8 *)user_src, len);
	riscv64_sstatus_write(saved_status);
	return 0;
}

int arch_user_copy_to(u64 user_dst, const void *src, u64 len)
{
	const u64 saved_status = riscv64_sstatus_read();

	if (!user_copy_args_valid(src, user_dst, len)) {
		return -1;
	}

	riscv64_sstatus_write(saved_status | RISCV64_SSTATUS_SUM);
	mem_copy((u8 *)user_dst, (const u8 *)src, len);
	riscv64_sstatus_write(saved_status);
	return 0;
}

static void early_console_write_user(u64 text, u64 len)
{
	u8 buffer[RISCV64_USER_COPY_CHUNK];
	u64 done = 0;

	if (text == 0) {
		return;
	}
	while (done < len) {
		const u64 chunk = min_u64(len - done, sizeof(buffer));

		if (arch_user_copy_from(buffer, text + done, chunk) != 0) {
			return;
		}
		console_write_len((const char *)buffer, chunk);
		done += chunk;
	}
}

static int copy_cstr_from_user(char *dst, u64 user_src, u64 capacity)
{
	if (dst == 0 || user_src == 0 || capacity == 0) {
		return -1;
	}
	for (u64 i = 0; i < capacity; i++) {
		if (arch_user_copy_from(&dst[i], user_src + i, 1) != 0) {
			return -1;
		}
		if (dst[i] == '\0') {
			return 0;
		}
	}
	dst[capacity - 1] = '\0';
	return -1;
}

static int user_message_to_ipc(const struct user_ipc_message *user_message,
			       struct ipc_message *message)
{
	if (user_message == 0 || message == 0) {
		return -1;
	}

	message->protocol = user_message->protocol;
	message->type = user_message->type;
	message->sender = 0;
	message->cap_rights = user_message->cap_rights;
	message->reply_port = task_port_from_handle(task_current(),
						    user_message->reply,
						    TASK_RIGHT_SEND);
	ipc_port_retain(message->reply_port);
	message->cap_type = IPC_CAP_NONE;
	message->cap_object = 0;
	if (user_message->cap == 0 && user_message->cap_rights != 0) {
		return -1;
	}

	if (user_message->cap != 0) {
		const u32 valid_rights = TASK_RIGHT_SEND | TASK_RIGHT_RECV |
					 TASK_RIGHT_DUP;
		const u32 rights = user_message->cap_rights | TASK_RIGHT_DUP;
		enum task_cap_type type = TASK_CAP_NONE;
		void *object = 0;

		if ((user_message->cap_rights & ~valid_rights) != 0 ||
		    user_message->cap_rights == 0 ||
		    task_export_cap(task_current(), user_message->cap, rights,
				    &type, &object) != 0) {
			return -1;
		}

		message->cap_type = type == TASK_CAP_PORT ? IPC_CAP_PORT :
			(type == TASK_CAP_BUFFER ? IPC_CAP_BUFFER :
			 type == TASK_CAP_TASK ? IPC_CAP_TASK :
			 IPC_CAP_HW_RESOURCE);
		message->cap_object = object;
		if (message->cap_type == IPC_CAP_PORT) {
			ipc_port_retain((struct ipc_port *)message->cap_object);
		} else if (message->cap_type == IPC_CAP_BUFFER) {
			buffer_retain((struct shared_buffer *)message->cap_object);
		} else if (message->cap_type == IPC_CAP_HW_RESOURCE) {
			if (task_hw_resource_retain(
				    (const struct task_hw_resource *)
					    message->cap_object) != 0) {
				return -1;
			}
		} else if (task_retain((struct task *)message->cap_object) != 0) {
			return -1;
		}
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
	} else if (message->cap_type == IPC_CAP_TASK) {
		user_message->cap =
			task_grant_task(task_current(),
					(struct task *)message->cap_object,
					message->cap_rights);
	} else if (message->cap_type == IPC_CAP_HW_RESOURCE) {
		user_message->cap =
			task_grant_hw_resource(
				task_current(),
				(const struct task_hw_resource *)
					message->cap_object,
				message->cap_rights);
	}
	for (u64 i = 0; i < USER_IPC_WORDS; i++) {
		user_message->words[i] = message->words[i];
	}
}

static u64 native_sys_exit(const struct native_syscall_args *args)
{
	console_printf("syscall: exit status=%u\n", (u32)args->arg0);
	thread_exit();
}

static u64 native_sys_timer_ticks(const struct native_syscall_args *args)
{
	(void)args;
	return timer_ticks();
}

static u64 native_sys_clock_monotonic_ns(const struct native_syscall_args *args)
{
	(void)args;
	return timer_monotonic_ns();
}

static u64 native_sys_sleep_ns(const struct native_syscall_args *args)
{
	thread_sleep_ns(args->arg0);
	return 0;
}

static u64 native_sys_launch_module(const struct native_syscall_args *args)
{
	char name[RISCV64_MAX_CSTR];
	struct task_launch_cap caps[MAX_INHERITED_CAPS];

	if (copy_cstr_from_user(name, args->arg0, sizeof(name)) != 0 ||
	    args->arg2 > MAX_INHERITED_CAPS) {
		return (u64)-1;
	}
	if (args->arg2 != 0 &&
	    (args->arg1 == 0 ||
	     arch_user_copy_from(caps, args->arg1,
				 args->arg2 * sizeof(caps[0])) != 0)) {
		return (u64)-1;
	}
	return server_launch_module_with_caps(name, task_current(),
					      args->arg2 != 0 ? caps : 0,
					      args->arg2);
}

static u64 native_sys_port_create(const struct native_syscall_args *args)
{
	char name[RISCV64_MAX_CSTR];

	if (copy_cstr_from_user(name, args->arg0, sizeof(name)) != 0) {
		return (u64)-1;
	}
	return task_grant_port(task_current(), ipc_port_create_private(name),
			       TASK_RIGHT_SEND | TASK_RIGHT_RECV |
			       TASK_RIGHT_DUP);
}

static u64 native_sys_handle_close(const struct native_syscall_args *args)
{
	return (u64)task_close_handle(task_current(), args->arg0);
}

static u64 native_sys_boot_module_read(const struct native_syscall_args *args)
{
	u8 buffer[RISCV64_USER_COPY_CHUNK];
	u64 done = 0;

	if (args->arg1 == 0) {
		return server_boot_module_size();
	}
	while (done < args->arg2) {
		const u64 chunk = min_u64(args->arg2 - done, sizeof(buffer));

		if (server_boot_module_read(args->arg0 + done, buffer, chunk) != 0 ||
		    arch_user_copy_to(args->arg1 + done, buffer, chunk) != 0) {
			return (u64)-1;
		}
		done += chunk;
	}
	return 0;
}

static u64 native_sys_ipc_send(const struct native_syscall_args *args)
{
	struct user_ipc_message user_message;
	struct ipc_message message = { 0 };
	struct ipc_port *port =
		task_port_from_handle(task_current(), args->arg0,
				      TASK_RIGHT_SEND);
	int result;

	if (port == 0 || args->arg1 == 0 ||
	    arch_user_copy_from(&user_message, args->arg1,
				sizeof(user_message)) != 0 ||
	    user_message_to_ipc(&user_message, &message) != 0) {
		ipc_message_release(&message);
		return (u64)-1;
	}

	result = ipc_send(port, &message);
	ipc_message_release(&message);
	return (u64)result;
}

static u64 native_sys_ipc_recv(const struct native_syscall_args *args)
{
	struct user_ipc_message user_message;
	struct ipc_message message;
	struct ipc_port *port =
		task_port_from_handle(task_current(), args->arg0,
				      TASK_RIGHT_RECV);

	if (port == 0 || args->arg1 == 0 || ipc_recv(port, &message) != 0) {
		return (u64)-1;
	}
	ipc_message_to_user(&message, &user_message);
	if (arch_user_copy_to(args->arg1, &user_message,
			      sizeof(user_message)) != 0) {
		ipc_message_release(&message);
		return (u64)-1;
	}
	ipc_message_release(&message);
	return 0;
}

static u64 native_sys_ipc_call(const struct native_syscall_args *args)
{
	struct user_ipc_message user_request;
	struct user_ipc_message user_reply;
	struct ipc_message message = { 0 };
	struct ipc_message reply;
	struct ipc_port *port =
		task_port_from_handle(task_current(), args->arg0,
				      TASK_RIGHT_SEND);
	struct ipc_port *reply_port = task_reply_port(task_current());

	if (port == 0 || reply_port == 0 || args->arg1 == 0 ||
	    args->arg2 == 0 ||
	    arch_user_copy_from(&user_request, args->arg1,
				sizeof(user_request)) != 0 ||
	    user_message_to_ipc(&user_request, &message) != 0) {
		ipc_message_release(&message);
		return (u64)-1;
	}

	ipc_port_release(message.reply_port);
	message.reply_port = reply_port;
	ipc_port_retain(message.reply_port);
	if (ipc_send(port, &message) != 0) {
		ipc_message_release(&message);
		return (u64)-1;
	}
	ipc_message_release(&message);
	if (ipc_recv(reply_port, &reply) != 0) {
		return (u64)-1;
	}

	ipc_message_to_user(&reply, &user_reply);
	if (arch_user_copy_to(args->arg2, &user_reply,
			      sizeof(user_reply)) != 0) {
		ipc_message_release(&reply);
		return (u64)-1;
	}
	ipc_message_release(&reply);
	return 0;
}

static u64 native_sys_task_id(const struct native_syscall_args *args)
{
	struct task *task;

	if (args->arg0 == 0) {
		return task_id(task_current());
	}
	task = task_from_handle(task_current(), args->arg0, TASK_RIGHT_SEND);
	return task != 0 ? task_id(task) : (u64)-1;
}

static u64 native_sys_task_alloc(const struct native_syscall_args *args)
{
	u64 alloc_args[4];

	if (args->arg0 == 0 ||
	    arch_user_copy_from(alloc_args, args->arg0,
				sizeof(alloc_args)) != 0) {
		return (u64)-1;
	}
	return (u64)server_task_alloc(task_current(), alloc_args[0],
				      alloc_args[1], alloc_args[2],
				      (u32)alloc_args[3]);
}

static u64 native_sys_buffer_create(const struct native_syscall_args *args)
{
	struct shared_buffer *buffer = buffer_create(args->arg0);
	u64 handle;

	if (buffer == 0) {
		return (u64)-1;
	}
	handle = task_grant_buffer(task_current(), buffer,
				   TASK_RIGHT_SEND | TASK_RIGHT_RECV |
				   TASK_RIGHT_DUP);
	buffer_release(buffer);
	return handle != 0 ? handle : (u64)-1;
}

static u64 native_sys_buffer_read(const struct native_syscall_args *args)
{
	u64 read_args[4];
	u8 copy[RISCV64_USER_COPY_CHUNK];
	struct shared_buffer *buffer;

	if (args->arg0 == 0 ||
	    arch_user_copy_from(read_args, args->arg0,
				sizeof(read_args)) != 0) {
		return (u64)-1;
	}
	buffer = task_buffer_from_handle(task_current(), read_args[0],
					 TASK_RIGHT_RECV);
	if (buffer == 0) {
		return (u64)-1;
	}
	for (u64 done = 0; done < read_args[3];) {
		const u64 chunk = min_u64(read_args[3] - done, sizeof(copy));

		if (buffer_read(buffer, read_args[1] + done, copy, chunk) != 0 ||
		    arch_user_copy_to(read_args[2] + done, copy, chunk) != 0) {
			return (u64)-1;
		}
		done += chunk;
	}
	return 0;
}

static u64 native_sys_buffer_write(const struct native_syscall_args *args)
{
	u64 write_args[4];
	u8 copy[RISCV64_USER_COPY_CHUNK];
	struct shared_buffer *buffer;

	if (args->arg0 == 0 ||
	    arch_user_copy_from(write_args, args->arg0,
				sizeof(write_args)) != 0) {
		return (u64)-1;
	}
	buffer = task_buffer_from_handle(task_current(), write_args[0],
					 TASK_RIGHT_SEND);
	if (buffer == 0) {
		return (u64)-1;
	}
	for (u64 done = 0; done < write_args[3];) {
		const u64 chunk = min_u64(write_args[3] - done, sizeof(copy));

		if (arch_user_copy_from(copy, write_args[2] + done, chunk) != 0 ||
		    buffer_write(buffer, write_args[1] + done, copy, chunk) != 0) {
			return (u64)-1;
		}
		done += chunk;
	}
	return 0;
}

static u64 native_sys_early_console_write(const struct native_syscall_args *args)
{
	early_console_write_user(args->arg0, args->arg1);
	return 0;
}

static u64 native_sys_early_console_log(const struct native_syscall_args *args)
{
	u8 buffer[RISCV64_USER_COPY_CHUNK];
	u64 done = 0;

	if (args->arg0 == 0) {
		return (u64)-1;
	}
	while (done < args->arg1) {
		const u64 chunk = min_u64(args->arg1 - done, sizeof(buffer));

		if (arch_user_copy_from(buffer, args->arg0 + done, chunk) != 0) {
			return (u64)-1;
		}
		console_log_write_len((const char *)buffer, chunk);
		done += chunk;
	}
	return 0;
}

static u64 native_sys_machine_power(const struct native_syscall_args *args)
{
	(void)args;
	arch_poweroff();
}

static const struct native_syscall_entry native_syscalls[] = {
	{ SYSCALL_EXIT, "exit", native_sys_exit },
	{ SYSCALL_TIMER_TICKS, "timer_ticks", native_sys_timer_ticks },
	{ SYSCALL_LAUNCH_MODULE, "launch_module", native_sys_launch_module },
	{ SYSCALL_PORT_CREATE, "port_create", native_sys_port_create },
	{ SYSCALL_IPC_SEND, "ipc_send", native_sys_ipc_send },
	{ SYSCALL_IPC_RECV, "ipc_recv", native_sys_ipc_recv },
	{ SYSCALL_IPC_CALL, "ipc_call", native_sys_ipc_call },
	{ SYSCALL_HANDLE_CLOSE, "handle_close", native_sys_handle_close },
	{ SYSCALL_BOOT_MODULE_READ, "boot_module_read",
	  native_sys_boot_module_read },
	{ SYSCALL_CLOCK_MONOTONIC_NS, "clock_monotonic_ns",
	  native_sys_clock_monotonic_ns },
	{ SYSCALL_SLEEP_NS, "sleep_ns", native_sys_sleep_ns },
	{ SYSCALL_TASK_ID, "task_id", native_sys_task_id },
	{ SYSCALL_TASK_ALLOC, "task_alloc", native_sys_task_alloc },
	{ SYSCALL_BUFFER_CREATE, "buffer_create", native_sys_buffer_create },
	{ SYSCALL_BUFFER_READ, "buffer_read", native_sys_buffer_read },
	{ SYSCALL_BUFFER_WRITE, "buffer_write", native_sys_buffer_write },
	{ SYSCALL_EARLY_CONSOLE_WRITE, "early_console_write",
	  native_sys_early_console_write },
	{ SYSCALL_EARLY_CONSOLE_LOG, "early_console_log",
	  native_sys_early_console_log },
	{ SYSCALL_MACHINE_POWER, "machine_power", native_sys_machine_power },
};

static const struct native_syscall_entry *native_syscall_lookup(i64 number)
{
	for (u64 i = 0; i < sizeof(native_syscalls) / sizeof(native_syscalls[0]);
	     i++) {
		if (native_syscalls[i].number == number) {
			return &native_syscalls[i];
		}
	}
	return 0;
}

static int linux_forward_message(struct ipc_port *linux,
				 struct ipc_port *reply_port,
				 struct ipc_message *request,
				 struct ipc_message *reply)
{
	if (linux == 0 || reply_port == 0 || request == 0 || reply == 0) {
		return -1;
	}
	request->protocol = USER_FOURCC_LINX;
	request->reply_port = reply_port;
	return ipc_send(linux, request) == 0 &&
	       ipc_recv(reply_port, reply) == 0 ? 0 : -1;
}

static u64 linux_register_current(struct ipc_port *linux,
				  struct ipc_port *reply_port)
{
	const u32 current_task = task_id(task_current());
	struct ipc_message request = {
		.protocol = USER_FOURCC_LINX,
		.type = LINUX_SHARED_REGISTER_PROCESS,
		.sender = 0,
		.cap_rights = 0,
		.reply_port = reply_port,
		.cap_type = IPC_CAP_NONE,
		.cap_object = 0,
		.words = { current_task, 0, 0, current_task },
	};
	struct ipc_message reply;

	if (linux_registered_task == current_task) {
		return 0;
	}
	if (linux_forward_message(linux, reply_port, &request, &reply) != 0) {
		return (u64)-LINUX_ENOSYS;
	}
	if ((i64)reply.words[0] > 0 ||
	    (i64)reply.words[0] == -LINUX_EINVAL) {
		linux_registered_task = current_task;
		console_printf("linux-riscv64: registered task=%u pid=%u\n",
			       current_task, (u32)reply.words[0]);
		ipc_message_release(&reply);
		return 0;
	}
	const u64 result = reply.words[0];
	ipc_message_release(&reply);
	return result;
}

static u64 linux_write_one(struct ipc_port *linux, struct ipc_port *reply_port,
			   u64 fd, u64 user_buffer, u64 len)
{
	struct shared_buffer *buffer;
	struct ipc_message request = {
		.protocol = USER_FOURCC_LINX,
		.type = LINUX_SHARED_WRITE,
		.sender = 0,
		.cap_rights = TASK_RIGHT_RECV | TASK_RIGHT_DUP,
		.reply_port = reply_port,
		.cap_type = IPC_CAP_BUFFER,
		.cap_object = 0,
		.words = { fd, len, 0, 0 },
	};
	struct ipc_message reply;
	u8 copy[RISCV64_USER_COPY_CHUNK];

	if (user_buffer == 0 && len != 0) {
		return (u64)-LINUX_EFAULT;
	}
	if (len > LINUX_MAX_SYSCALL_BUFFER) {
		len = LINUX_MAX_SYSCALL_BUFFER;
	}
	buffer = buffer_create(len == 0 ? 1 : len);
	if (buffer == 0) {
		return (u64)-LINUX_ENOMEM;
	}
	for (u64 done = 0; done < len;) {
		const u64 chunk = min_u64(len - done, sizeof(copy));

		if (arch_user_copy_from(copy, user_buffer + done, chunk) != 0 ||
		    buffer_write(buffer, done, copy, chunk) != 0) {
			buffer_release(buffer);
			return (u64)-LINUX_EFAULT;
		}
		done += chunk;
	}
	request.cap_object = buffer;
	if (linux_forward_message(linux, reply_port, &request, &reply) != 0) {
		buffer_release(buffer);
		return (u64)-LINUX_ENOSYS;
	}
	buffer_release(buffer);
	const u64 result = reply.words[0];
	ipc_message_release(&reply);
	return result;
}

static u64 linux_write_chunked(struct ipc_port *linux,
			       struct ipc_port *reply_port,
			       u64 fd, u64 user_buffer, u64 len)
{
	u64 total = 0;

	while (total < len) {
		const u64 wrote = linux_write_one(linux, reply_port, fd,
						  user_buffer + total,
						  len - total);

		if ((i64)wrote < 0) {
			return total != 0 ? total : wrote;
		}
		if (wrote == 0) {
			return total;
		}
		if (kernel_cmdline_has("riscv64-bootpkg-test") &&
		    (fd == 1 || fd == 2)) {
			early_console_write_user(user_buffer + total, wrote);
		}
		total += wrote;
	}
	return total;
}

static u64 linux_exit_current(struct ipc_port *linux, struct ipc_port *reply_port,
			      u64 status)
{
	struct ipc_message request = {
		.protocol = USER_FOURCC_LINX,
		.type = LINUX_SHARED_EXIT_GROUP,
		.sender = 0,
		.cap_rights = 0,
		.reply_port = reply_port,
		.cap_type = IPC_CAP_NONE,
		.cap_object = 0,
		.words = { status, 0, 0, 0 },
	};
	struct ipc_message reply = { 0 };

	if (linux_forward_message(linux, reply_port, &request, &reply) == 0) {
		ipc_message_release(&reply);
	}
	console_printf("linux-riscv64: exit_group status=%u\n", (u32)status);
	if (kernel_cmdline_has("riscv64-bootpkg-test")) {
		arch_poweroff();
	}
	thread_exit();
}

static u64 linux_riscv64_legacy_syscall_dispatch(struct arch_syscall_frame *frame)
{
	struct ipc_port *linux = ipc_port_find("linux");
	struct ipc_port *reply_port = task_reply_port(task_current());
	struct ipc_message request = {
		.protocol = USER_FOURCC_LINX,
		.type = LINUX_RISCV64_RPC_SYSCALL,
		.sender = 0,
		.cap_rights = 0,
		.reply_port = reply_port,
		.cap_type = IPC_CAP_NONE,
		.cap_object = 0,
		.words = { frame->number, frame->arg0, frame->arg1,
			   frame->arg2 },
	};
	struct ipc_message reply;
	u64 result;

	if (linux == 0 || reply_port == 0 ||
	    ipc_send(linux, &request) != 0 ||
	    ipc_recv(reply_port, &reply) != 0) {
		console_printf("linux-riscv64: legacy server unavailable syscall=%u pc=%p\n",
			       (u32)frame->number,
			       (const void *)frame->user_pc);
		return (u64)-LINUX_ENOSYS;
	}

	result = reply.words[0];
	if (reply.words[1] == LINUX_RISCV64_ACTION_WRITE &&
	    frame->number == LINUX_RISCV64_WRITE && (i64)result > 0) {
		early_console_write_user(frame->arg1, result);
	} else if (reply.words[1] == LINUX_RISCV64_ACTION_EXIT &&
		   (frame->number == LINUX_RISCV64_EXIT ||
		    frame->number == LINUX_RISCV64_EXIT_GROUP)) {
		console_printf("linux-riscv64: exit_group status=%u\n",
			       (u32)frame->arg0);
		ipc_message_release(&reply);
		thread_exit();
	}
	ipc_message_release(&reply);
	return result;
}

static u64 linux_riscv64_syscall_dispatch(struct arch_syscall_frame *frame)
{
	struct ipc_port *linux = ipc_port_find("linux");
	struct ipc_port *reply_port = task_reply_port(task_current());
	u64 registered;

	if (kernel_cmdline_has("riscv64-uart-console")) {
		return linux_riscv64_legacy_syscall_dispatch(frame);
	}
	if (linux == 0 || reply_port == 0) {
		console_printf("linux-riscv64: server unavailable syscall=%u pc=%p\n",
			       (u32)frame->number,
			       (const void *)frame->user_pc);
		return (u64)-LINUX_ENOSYS;
	}
	registered = linux_register_current(linux, reply_port);
	if ((i64)registered < 0) {
		console_printf("linux-riscv64: register failed syscall=%u rc=%d\n",
			       (u32)frame->number, (int)registered);
		return registered;
	}

	switch (frame->number) {
	case LINUX_RISCV64_WRITE:
		return linux_write_chunked(linux, reply_port, frame->arg0,
					   frame->arg1, frame->arg2);
	case LINUX_RISCV64_EXIT:
	case LINUX_RISCV64_EXIT_GROUP:
		return linux_exit_current(linux, reply_port, frame->arg0);
	case LINUX_RISCV64_SET_TID_ADDRESS:
		return thread_id(thread_current());
	default:
		console_printf("linux-riscv64: unknown syscall=%u pc=%p arg0=%p arg1=%p arg2=%p arg3=%p\n",
			       (u32)frame->number,
			       (const void *)frame->user_pc,
			       (const void *)frame->arg0,
			       (const void *)frame->arg1,
			       (const void *)frame->arg2,
			       (const void *)frame->arg3);
		return (u64)-LINUX_ENOSYS;
	}
}

u64 arch_syscall_dispatch(struct arch_syscall_frame *frame)
{
	const struct native_syscall_entry *entry;
	struct native_syscall_args args;

	(void)strace_mode;
	if (frame == 0) {
		return (u64)-1;
	}
	if ((i64)frame->number >= 0) {
		return linux_riscv64_syscall_dispatch(frame);
	}
	entry = native_syscall_lookup((i64)frame->number);
	if (entry == 0) {
		console_printf("riscv64: unknown native syscall=%d pc=%p\n",
			       (int)frame->number,
			       (const void *)frame->user_pc);
		return (u64)-1;
	}
	args.arg0 = frame->arg0;
	args.arg1 = frame->arg1;
	args.arg2 = frame->arg2;
	args.arg3 = frame->arg3;
	return entry->fn(&args);
}
