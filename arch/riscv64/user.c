#include <arch/user.h>
#include <arch/interrupts.h>
#include <arch/layout.h>
#include <arch/power.h>
#include <arch/sbi.h>
#include "boot_timing.h"
#include "buffer.h"
#include "cmdline.h"
#include "console.h"
#include "ipc.h"
#include "linux_abi.h"
#include "sched.h"
#include "server.h"
#include "slab.h"
#include "spinlock.h"
#include "task_lifecycle.h"
#include "timer.h"
#include "vm.h"

enum {
	SYSCALL_EXIT = -2,
	SYSCALL_TIMER_TICKS = -4,
	SYSCALL_LAUNCH_MODULE = -8,
	SYSCALL_PORT_CREATE = -10,
	SYSCALL_IPC_SEND = -12,
	SYSCALL_IPC_RECV = -13,
	SYSCALL_IPC_CALL = -14,
	SYSCALL_IPC_TRY_RECV = -72,
	SYSCALL_IPC_RECV_ANY = -126,
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
	SYSCALL_TASK_ID = -42,
	SYSCALL_TASK_ALLOC = -44,
	SYSCALL_TASK_CLONE_RANGE = -46,
	SYSCALL_TASK_KILL = -50,
	SYSCALL_TASK_INFO = -52,
	SYSCALL_EARLY_CONSOLE_WRITE = -54,
	SYSCALL_EARLY_CONSOLE_LOG = -56,
	SYSCALL_MACHINE_POWER = -64,
	SYSCALL_TASK_CLEAR = -66,
	SYSCALL_CMDLINE_HAS = -98,
	SYSCALL_HANDLE_FIND = -112,
	SYSCALL_TASK_GRANT_TAGGED = -114,
	SYSCALL_TASK_HANDLE_FIND = -116,
	SYSCALL_BOOT_EVENT = -124,
	BOOT_EVENT_RECORD = 1,
	BOOT_EVENT_READ = 2,
	LINUX_RISCV64_OPENAT = 56,
	LINUX_RISCV64_CLOSE = 57,
	LINUX_RISCV64_READ = 63,
	LINUX_RISCV64_WRITE = 64,
	LINUX_RISCV64_READLINKAT = 78,
	LINUX_RISCV64_NEWFSTATAT = 79,
	LINUX_RISCV64_FSTAT = 80,
	LINUX_RISCV64_EXIT = 93,
	LINUX_RISCV64_EXIT_GROUP = 94,
	LINUX_RISCV64_SET_TID_ADDRESS = 96,
	LINUX_RISCV64_GETPID = 172,
	LINUX_RISCV64_GETPPID = 173,
	LINUX_RISCV64_GETUID = 174,
	LINUX_RISCV64_GETEUID = 175,
	LINUX_RISCV64_GETGID = 176,
	LINUX_RISCV64_GETEGID = 177,
	LINUX_RISCV64_GETTID = 178,
	LINUX_RISCV64_BRK = 214,
	LINUX_RISCV64_MUNMAP = 215,
	LINUX_RISCV64_MMAP = 222,
	LINUX_RISCV64_MPROTECT = 226,
	LINUX_EPERM = 1,
	LINUX_ENOMEM = 12,
	LINUX_EFAULT = 14,
	LINUX_EINVAL = 22,
	LINUX_EBADF = 9,
	LINUX_ENOSYS = 38,
	LINUX_INITIAL_BRK = 0x900000,
	LINUX_MAX_BRK = 0x10000000,
	LINUX_MMAP_BASE = 0x10000000,
	LINUX_MMAP_LIMIT = 0x20000000,
	LINUX_MAP_FIXED_MIN = 0x10000,
	LINUX_MAP_PRIVATE = 0x2,
	LINUX_MAP_FIXED = 0x10,
	LINUX_MAP_ANONYMOUS = 0x20,
	LINUX_STAT_SIZE = 144,
	LINUX_MAX_SHARED_BUFFER = 1024 * 1024,
	LINUX_MAX_SYSCALL_BUFFER = 4096,
	USER_VFS_MMAP_PAGE_DATA = 1,
	USER_VFS_MMAP_PAGE_ZERO = 2,
	USER_VFS_MMAP_PAGE_BUS = 3,
	USER_FOURCC_LINX = ('L') | ('I' << 8) | ('N' << 16) | ('X' << 24),
	USER_IPC_WORDS = 4,
	RISCV64_SSTATUS_SUM = 1ULL << 18,
	RISCV64_USER_COPY_CHUNK = 256,
	RISCV64_MAX_CSTR = 256,
	MAX_INHERITED_CAPS = 16,
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

static void linux_frontend_task_died(u32 task_id, const char *task_name);

void riscv64_user_enter(u64 entry, u64 stack, u64 kernel_stack)
	__attribute__((noreturn));

void arch_user_init(void)
{
	strace_mode = "off";
	current_kernel_stack = 0;
	(void)task_lifecycle_register_death_hook(linux_frontend_task_died);
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

static u64 min_u64(u64 left, u64 right)
{
	return left < right ? left : right;
}

static u64 align_down(u64 value, u64 align)
{
	return value & ~(align - 1);
}

static u64 align_up(u64 value, u64 align)
{
	return align_down(value + align - 1, align);
}

static u32 linux_map_flags_to_task(u64 flags)
{
	u32 task_flags = 0;

	if ((flags & LINUX_MAP_PRIVATE) != 0) {
		task_flags |= TASK_VM_MAP_PRIVATE;
	}
	if ((flags & LINUX_MAP_ANONYMOUS) != 0) {
		task_flags |= TASK_VM_MAP_ANONYMOUS;
	}
	if ((flags & LINUX_MAP_FIXED) != 0) {
		task_flags |= TASK_VM_MAP_FIXED;
	}
	return task_flags;
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
	if (!user_copy_args_valid(dst, user_src, len)) {
		return -1;
	}

	return vm_read_user(task_vm_space(task_current()), user_src, dst, len);
}

int arch_user_copy_to(u64 user_dst, const void *src, u64 len)
{
	if (!user_copy_args_valid(src, user_dst, len)) {
		return -1;
	}

	return vm_write_user(task_vm_space(task_current()), user_dst, src, len);
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

static u64 user_cstr_len_limited(u64 user_src, u64 max_len)
{
	char c;

	if (user_src == 0) {
		return max_len + 1;
	}
	for (u64 i = 0; i < max_len; i++) {
		if (arch_user_copy_from(&c, user_src + i, 1) != 0) {
			return max_len + 1;
		}
		if (c == '\0') {
			return i;
		}
	}
	return max_len + 1;
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
			 type == TASK_CAP_HW_RESOURCE ? IPC_CAP_HW_RESOURCE :
			 IPC_CAP_SCHED_POLICY);
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
	} else if (message->cap_type == IPC_CAP_SCHED_POLICY) {
		user_message->cap =
			task_grant_sched_policy(
				task_current(),
				(const struct sched_policy_cap *)
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

static u64 native_sys_handle_find(const struct native_syscall_args *args)
{
	const u64 handle = task_handle_find(task_current(), (u32)args->arg0);

	return handle != 0 ? handle : (u64)-1;
}

static u64 native_sys_task_handle_find(const struct native_syscall_args *args)
{
	const u64 handle =
		server_task_handle_find(task_current(), args->arg0,
					(u32)args->arg1);

	return handle != 0 ? handle : (u64)-1;
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
		ipc_port_release(port);
		return (u64)-1;
	}

	result = ipc_send(port, &message);
	ipc_message_release(&message);
	ipc_port_release(port);
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
		ipc_port_release(port);
		return (u64)-1;
	}
	ipc_message_to_user(&message, &user_message);
	if (arch_user_copy_to(args->arg1, &user_message,
			      sizeof(user_message)) != 0) {
		ipc_message_release(&message);
		ipc_port_release(port);
		return (u64)-1;
	}
	ipc_message_release(&message);
	ipc_port_release(port);
	return 0;
}

static u64 native_sys_ipc_try_recv(const struct native_syscall_args *args)
{
	struct user_ipc_message user_message;
	struct ipc_message message;
	struct ipc_port *port =
		task_port_from_handle(task_current(), args->arg0,
				      TASK_RIGHT_RECV);
	int result;

	if (port == 0 || args->arg1 == 0) {
		ipc_port_release(port);
		return (u64)-1;
	}
	result = ipc_try_recv(port, &message);
	if (result < 0) {
		ipc_port_release(port);
		return (u64)-1;
	}
	if (result > 0) {
		ipc_port_release(port);
		return 1;
	}
	ipc_message_to_user(&message, &user_message);
	if (arch_user_copy_to(args->arg1, &user_message,
			      sizeof(user_message)) != 0) {
		ipc_message_release(&message);
		ipc_port_release(port);
		return (u64)-1;
	}
	ipc_message_release(&message);
	ipc_port_release(port);
	return 0;
}

static u64 native_sys_ipc_recv_any(const struct native_syscall_args *args)
{
	enum {
		IPC_RECV_ANY_MAX = 4,
	};
	u64 handles[IPC_RECV_ANY_MAX];
	struct ipc_port *ports[IPC_RECV_ANY_MAX] = { 0 };
	struct ipc_message message;
	struct user_ipc_message user_message;
	u64 index = 0;
	u64 count = args->arg1;

	if (args->arg0 == 0 || args->arg2 == 0 || args->arg3 == 0 ||
	    count == 0 || count > IPC_RECV_ANY_MAX ||
	    arch_user_copy_from(handles, args->arg0,
				count * sizeof(handles[0])) != 0) {
		return (u64)-1;
	}

	for (u64 i = 0; i < count; i++) {
		ports[i] = task_port_from_handle(task_current(), handles[i],
						 TASK_RIGHT_RECV);
		if (ports[i] == 0) {
			for (u64 j = 0; j < i; j++) {
				ipc_port_release(ports[j]);
			}
			return (u64)-1;
		}
	}

	if (ipc_recv_any(ports, count, &message, &index) != 0) {
		for (u64 i = 0; i < count; i++) {
			ipc_port_release(ports[i]);
		}
		return (u64)-1;
	}

	ipc_message_to_user(&message, &user_message);
	if (arch_user_copy_to(args->arg2, &user_message,
			      sizeof(user_message)) != 0 ||
	    arch_user_copy_to(args->arg3, &index, sizeof(index)) != 0) {
		ipc_message_release(&message);
		for (u64 i = 0; i < count; i++) {
			ipc_port_release(ports[i]);
		}
		return (u64)-1;
	}

	ipc_message_release(&message);
	for (u64 i = 0; i < count; i++) {
		ipc_port_release(ports[i]);
	}
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
		ipc_port_release(port);
		return (u64)-1;
	}

	ipc_port_release(message.reply_port);
	message.reply_port = reply_port;
	ipc_port_retain(message.reply_port);
	if (ipc_send(port, &message) != 0) {
		ipc_message_release(&message);
		ipc_port_release(port);
		return (u64)-1;
	}
	ipc_message_release(&message);
	if (ipc_recv(reply_port, &reply) != 0) {
		ipc_port_release(port);
		return (u64)-1;
	}

	ipc_message_to_user(&reply, &user_reply);
	if (arch_user_copy_to(args->arg2, &user_reply,
			      sizeof(user_reply)) != 0) {
		ipc_message_release(&reply);
		ipc_port_release(port);
		return (u64)-1;
	}
	ipc_message_release(&reply);
	ipc_port_release(port);
	return 0;
}

static u64 native_sys_task_id(const struct native_syscall_args *args)
{
	struct task *task;

	if (args->arg0 == 0) {
		return task_id(task_current());
	}
	task = task_from_handle(task_current(), args->arg0, TASK_RIGHT_SEND);
	if (task == 0) {
		return (u64)-1;
	}
	const u64 id = task_id(task);
	task_release(task);
	return id;
}

static u64 native_sys_task_info(const struct native_syscall_args *args)
{
	u64 pid_threads_flags = 0;
	u64 name_words[2] = { 0, 0 };

	if (args->arg1 == 0 || args->arg2 == 0 ||
	    task_info_at(args->arg0, &pid_threads_flags, name_words) != 0 ||
	    arch_user_copy_to(args->arg1, &pid_threads_flags,
			      sizeof(pid_threads_flags)) != 0 ||
	    arch_user_copy_to(args->arg2, name_words, sizeof(name_words)) != 0) {
		return (u64)-1;
	}

	return 0;
}

static u64 native_sys_task_create(const struct native_syscall_args *args)
{
	char name[RISCV64_MAX_CSTR];

	if (copy_cstr_from_user(name, args->arg0, sizeof(name)) != 0) {
		return (u64)-1;
	}
	return server_task_create(task_current(), name);
}

static u64 native_sys_task_map(const struct native_syscall_args *args)
{
	u64 map_args[6];
	u8 *copy;
	u64 copy_len;
	u64 result;

	if (args->arg0 == 0 ||
	    arch_user_copy_from(map_args, args->arg0, sizeof(map_args)) != 0 ||
	    map_args[3] > LINUX_MAX_SYSCALL_BUFFER) {
		return (u64)-1;
	}
	copy_len = map_args[3] != 0 ? map_args[3] : 1;
	copy = (u8 *)slab_alloc(copy_len);
	if (copy == 0) {
		return (u64)-1;
	}
	if (map_args[3] != 0 &&
	    arch_user_copy_from(copy, map_args[2], map_args[3]) != 0) {
		slab_free(copy);
		return (u64)-1;
	}
	result = (u64)server_task_map(task_current(), map_args[0],
				      map_args[1], copy, map_args[3],
				      map_args[4], (u32)map_args[5]);
	slab_free(copy);
	return result;
}

static u64 native_sys_task_grant(const struct native_syscall_args *args)
{
	return (u64)server_task_grant(task_current(), args->arg0, args->arg1,
				      (u32)args->arg2);
}

static u64 native_sys_task_grant_tagged(const struct native_syscall_args *args)
{
	return server_task_grant_tagged(task_current(), args->arg0, args->arg1,
					(u32)args->arg2, (u32)args->arg3);
}

static u64 native_sys_task_start(const struct native_syscall_args *args)
{
	return (u64)server_task_start(task_current(), args->arg0, args->arg1);
}

static u64 native_sys_task_write(const struct native_syscall_args *args)
{
	u64 write_args[4];
	u8 copy[RISCV64_USER_COPY_CHUNK];

	if (args->arg0 == 0 ||
	    arch_user_copy_from(write_args, args->arg0,
				sizeof(write_args)) != 0) {
		return (u64)-1;
	}
	for (u64 done = 0; done < write_args[3];) {
		const u64 chunk = min_u64(write_args[3] - done, sizeof(copy));

		if (arch_user_copy_from(copy, write_args[2] + done, chunk) != 0 ||
		    server_task_write(task_current(), write_args[0],
				      write_args[1] + done, copy, chunk) != 0) {
			return (u64)-1;
		}
		done += chunk;
	}
	return 0;
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

static u64 native_sys_task_clone_range(const struct native_syscall_args *args)
{
	u64 clone_args[5];

	if (args->arg0 == 0 ||
	    arch_user_copy_from(clone_args, args->arg0,
				sizeof(clone_args)) != 0) {
		return (u64)-1;
	}
	return (u64)server_task_clone_range(task_current(), clone_args[0],
					    clone_args[1], clone_args[2],
					    clone_args[3],
					    (u32)clone_args[4]);
}

static u64 native_sys_task_start_at(const struct native_syscall_args *args)
{
	return (u64)server_task_start_at(task_current(), args->arg0,
					 args->arg1, args->arg2);
}

static u64 native_sys_task_kill(const struct native_syscall_args *args)
{
	return (u64)server_task_kill(task_current(), args->arg0);
}

static u64 native_sys_task_clear(const struct native_syscall_args *args)
{
	return (u64)server_task_clear(task_current(), args->arg0);
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
			buffer_release(buffer);
			return (u64)-1;
		}
		done += chunk;
	}
	buffer_release(buffer);
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
			buffer_release(buffer);
			return (u64)-1;
		}
		done += chunk;
	}
	buffer_release(buffer);
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
	const struct task_hw_resource *authority =
		task_hw_resource_from_handle(task_current(), args->arg0,
					     TASK_RIGHT_SEND);

	if (authority == 0 ||
	    authority->type != TASK_HW_RESOURCE_POWER_AUTH ||
	    (authority->ops & TASK_HW_OP_POWER) == 0) {
		task_hw_resource_release(authority);
		return (u64)-1;
	}

	task_hw_resource_release(authority);
	arch_poweroff();
	return 0;
}

static u64 native_sys_cmdline_has(const struct native_syscall_args *args)
{
	char token[128];

	if (copy_cstr_from_user(token, args->arg0, sizeof(token)) != 0) {
		return 0;
	}
	return kernel_cmdline_has(token) != 0 ? 1 : 0;
}

static u64 native_sys_boot_event(const struct native_syscall_args *args)
{
	if (args->arg0 == BOOT_EVENT_RECORD) {
		char name[BOOT_TIMING_NAME_BYTES];

		if (copy_cstr_from_user(name, args->arg1,
					sizeof(name)) != 0) {
			return (u64)-1;
		}
		return boot_timing_record(name) == 0 ? 0 : (u64)-1;
	}
	if (args->arg0 == BOOT_EVENT_READ) {
		struct boot_timing_event event;

		if (args->arg2 == 0 ||
		    boot_timing_read(args->arg1, &event) != 0 ||
		    arch_user_copy_to(args->arg2, &event,
				      sizeof(event)) != 0) {
			return (u64)-1;
		}
		return 0;
	}
	return (u64)-1;
}

static const struct native_syscall_entry native_syscalls[] = {
	{ SYSCALL_EXIT, "exit", native_sys_exit },
	{ SYSCALL_TIMER_TICKS, "timer_ticks", native_sys_timer_ticks },
	{ SYSCALL_LAUNCH_MODULE, "launch_module", native_sys_launch_module },
	{ SYSCALL_PORT_CREATE, "port_create", native_sys_port_create },
	{ SYSCALL_IPC_SEND, "ipc_send", native_sys_ipc_send },
	{ SYSCALL_IPC_RECV, "ipc_recv", native_sys_ipc_recv },
	{ SYSCALL_IPC_TRY_RECV, "ipc_try_recv", native_sys_ipc_try_recv },
	{ SYSCALL_IPC_RECV_ANY, "ipc_recv_any", native_sys_ipc_recv_any },
	{ SYSCALL_IPC_CALL, "ipc_call", native_sys_ipc_call },
	{ SYSCALL_HANDLE_CLOSE, "handle_close", native_sys_handle_close },
	{ SYSCALL_HANDLE_FIND, "handle_find", native_sys_handle_find },
	{ SYSCALL_TASK_HANDLE_FIND, "task_handle_find",
	  native_sys_task_handle_find },
	{ SYSCALL_BOOT_MODULE_READ, "boot_module_read",
	  native_sys_boot_module_read },
	{ SYSCALL_CLOCK_MONOTONIC_NS, "clock_monotonic_ns",
	  native_sys_clock_monotonic_ns },
	{ SYSCALL_SLEEP_NS, "sleep_ns", native_sys_sleep_ns },
	{ SYSCALL_TASK_CREATE, "task_create", native_sys_task_create },
	{ SYSCALL_TASK_MAP, "task_map", native_sys_task_map },
	{ SYSCALL_TASK_GRANT, "task_grant", native_sys_task_grant },
	{ SYSCALL_TASK_GRANT_TAGGED, "task_grant_tagged",
	  native_sys_task_grant_tagged },
	{ SYSCALL_TASK_START, "task_start", native_sys_task_start },
	{ SYSCALL_TASK_WRITE, "task_write", native_sys_task_write },
	{ SYSCALL_TASK_START_AT, "task_start_at", native_sys_task_start_at },
	{ SYSCALL_TASK_ID, "task_id", native_sys_task_id },
	{ SYSCALL_TASK_INFO, "task_info", native_sys_task_info },
	{ SYSCALL_TASK_ALLOC, "task_alloc", native_sys_task_alloc },
	{ SYSCALL_TASK_CLONE_RANGE, "task_clone_range",
	  native_sys_task_clone_range },
	{ SYSCALL_TASK_KILL, "task_kill", native_sys_task_kill },
	{ SYSCALL_TASK_CLEAR, "task_clear", native_sys_task_clear },
	{ SYSCALL_BUFFER_CREATE, "buffer_create", native_sys_buffer_create },
	{ SYSCALL_BUFFER_READ, "buffer_read", native_sys_buffer_read },
	{ SYSCALL_BUFFER_WRITE, "buffer_write", native_sys_buffer_write },
	{ SYSCALL_EARLY_CONSOLE_WRITE, "early_console_write",
	  native_sys_early_console_write },
	{ SYSCALL_EARLY_CONSOLE_LOG, "early_console_log",
	  native_sys_early_console_log },
	{ SYSCALL_MACHINE_POWER, "machine_power", native_sys_machine_power },
	{ SYSCALL_CMDLINE_HAS, "cmdline_has", native_sys_cmdline_has },
	{ SYSCALL_BOOT_EVENT, "boot_event", native_sys_boot_event },
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

struct riscv64_linux_frontend_task {
	u32 task_id;
	u32 linux_pid;
	struct riscv64_linux_frontend_task *next;
};

static struct spinlock linux_frontend_lock = SPINLOCK_INIT("rv64-linux-fe");
static struct riscv64_linux_frontend_task *linux_frontend_tasks;

static struct riscv64_linux_frontend_task *linux_frontend_find_locked(
	u32 task_id)
{
	for (struct riscv64_linux_frontend_task *state = linux_frontend_tasks;
	     state != 0; state = state->next) {
		if (state->task_id == task_id) {
			return state;
		}
	}
	return 0;
}

static int linux_frontend_is_registered(u32 task_id)
{
	const u64 flags = spin_lock_irqsave(&linux_frontend_lock);
	const int registered = linux_frontend_find_locked(task_id) != 0;

	spin_unlock_irqrestore(&linux_frontend_lock, flags);
	return registered;
}

static int linux_frontend_record(u32 task_id, u32 linux_pid)
{
	struct riscv64_linux_frontend_task *state;
	u64 flags = spin_lock_irqsave(&linux_frontend_lock);

	if (linux_frontend_find_locked(task_id) != 0) {
		spin_unlock_irqrestore(&linux_frontend_lock, flags);
		return 0;
	}
	spin_unlock_irqrestore(&linux_frontend_lock, flags);

	state = slab_zalloc(sizeof(*state));
	if (state == 0) {
		return -1;
	}
	state->task_id = task_id;
	state->linux_pid = linux_pid;

	flags = spin_lock_irqsave(&linux_frontend_lock);
	if (linux_frontend_find_locked(task_id) != 0) {
		spin_unlock_irqrestore(&linux_frontend_lock, flags);
		slab_free(state);
		return 0;
	}
	state->next = linux_frontend_tasks;
	linux_frontend_tasks = state;
	spin_unlock_irqrestore(&linux_frontend_lock, flags);
	return 0;
}

static void linux_frontend_forget(u32 task_id)
{
	const u64 flags = spin_lock_irqsave(&linux_frontend_lock);
	struct riscv64_linux_frontend_task **link = &linux_frontend_tasks;

	while (*link != 0) {
		struct riscv64_linux_frontend_task *state = *link;

		if (state->task_id == task_id) {
			*link = state->next;
			spin_unlock_irqrestore(&linux_frontend_lock, flags);
			slab_free(state);
			return;
		}
		link = &state->next;
	}
	spin_unlock_irqrestore(&linux_frontend_lock, flags);
}

static void linux_frontend_task_died(u32 task_id, const char *task_name)
{
	(void)task_name;
	linux_frontend_forget(task_id);
}

static u64 linux_register_current(struct ipc_port *linux,
				  struct ipc_port *reply_port)
{
	const u32 current_task = task_id(task_current());
	struct ipc_message request = {
		.protocol = USER_FOURCC_LINX,
		.type = LINUX_MSG_REGISTER_PROCESS,
		.sender = 0,
		.cap_rights = 0,
		.reply_port = reply_port,
		.cap_type = IPC_CAP_NONE,
		.cap_object = 0,
		.words = { current_task, 0, 0, current_task },
	};
	struct ipc_message reply;

	if (linux_frontend_is_registered(current_task)) {
		return 0;
	}
	if (linux_forward_message(linux, reply_port, &request, &reply) != 0) {
		return (u64)-LINUX_ENOSYS;
	}
	if ((i64)reply.words[0] > 0) {
		console_printf("linux-riscv64: registered task=%u pid=%u\n",
			       current_task, (u32)reply.words[0]);
		ipc_message_release(&reply);
		return linux_frontend_record(current_task, (u32)reply.words[0]) ==
			       0 ?
			0 : (u64)-LINUX_ENOMEM;
	}
	if ((i64)reply.words[0] == -LINUX_EINVAL ||
	    (i64)reply.words[0] == -LINUX_EPERM) {
		ipc_message_release(&reply);
		return linux_frontend_record(current_task, 0) == 0 ?
			0 : (u64)-LINUX_ENOMEM;
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
		.type = LINUX_MSG_WRITE,
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

static u64 linux_forward_scalar(struct ipc_port *linux,
				struct ipc_port *reply_port, u32 type)
{
	struct ipc_message request = {
		.protocol = USER_FOURCC_LINX,
		.type = type,
		.sender = 0,
		.cap_rights = 0,
		.reply_port = reply_port,
		.cap_type = IPC_CAP_NONE,
		.cap_object = 0,
		.words = { 0, 0, 0, 0 },
	};
	struct ipc_message reply;

	if (linux_forward_message(linux, reply_port, &request, &reply) != 0) {
		return (u64)-LINUX_ENOSYS;
	}
	const u64 result = reply.words[0];
	ipc_message_release(&reply);
	return result;
}

static u64 linux_forward_words(struct ipc_port *linux,
			       struct ipc_port *reply_port, u32 type,
			       u64 word0, u64 word1, u64 word2, u64 word3)
{
	struct ipc_message request = {
		.protocol = USER_FOURCC_LINX,
		.type = type,
		.sender = 0,
		.cap_rights = 0,
		.reply_port = reply_port,
		.cap_type = IPC_CAP_NONE,
		.cap_object = 0,
		.words = { word0, word1, word2, word3 },
	};
	struct ipc_message reply;

	if (linux_forward_message(linux, reply_port, &request, &reply) != 0) {
		return (u64)-LINUX_ENOSYS;
	}
	const u64 result = reply.words[0];
	ipc_message_release(&reply);
	return result;
}

static u64 linux_forward_user_buffer(struct ipc_port *linux,
				     struct ipc_port *reply_port,
				     u32 type, u32 rights,
				     u64 user_buffer, u64 len,
				     u64 word0, u64 word1,
				     u64 word2, u64 word3,
				     int copy_in, int copy_out)
{
	struct shared_buffer *buffer;
	struct ipc_message request = {
		.protocol = USER_FOURCC_LINX,
		.type = type,
		.sender = 0,
		.cap_rights = rights,
		.reply_port = reply_port,
		.cap_type = IPC_CAP_BUFFER,
		.cap_object = 0,
		.words = { word0, word1, word2, word3 },
	};
	struct ipc_message reply;
	u8 copy[RISCV64_USER_COPY_CHUNK];

	if (len != 0 && user_buffer == 0) {
		return (u64)-LINUX_EFAULT;
	}
	if (len > LINUX_MAX_SHARED_BUFFER) {
		return (u64)-LINUX_EINVAL;
	}
	buffer = buffer_create(len == 0 ? 1 : len);
	if (buffer == 0) {
		return (u64)-LINUX_ENOMEM;
	}
	if (copy_in) {
		for (u64 done = 0; done < len;) {
			const u64 chunk = min_u64(len - done, sizeof(copy));

			if (arch_user_copy_from(copy, user_buffer + done,
						chunk) != 0 ||
			    buffer_write(buffer, done, copy, chunk) != 0) {
				buffer_release(buffer);
				return (u64)-LINUX_EFAULT;
			}
			done += chunk;
		}
	}
	request.cap_object = buffer;
	if (linux_forward_message(linux, reply_port, &request, &reply) != 0) {
		buffer_release(buffer);
		return (u64)-LINUX_ENOSYS;
	}
	const u64 result = reply.words[0];
	if (copy_out && (i64)result >= 0) {
		u64 out_len = type == LINUX_MSG_READ ||
				      type == LINUX_MSG_MMAP ||
				      type == LINUX_MSG_READLINKAT ?
				      result : len;

		if (out_len > len) {
			out_len = len;
		}
		for (u64 done = 0; done < out_len;) {
			const u64 chunk = min_u64(out_len - done,
						  sizeof(copy));

			if (buffer_read(buffer, done, copy, chunk) != 0 ||
			    arch_user_copy_to(user_buffer + done, copy,
					      chunk) != 0) {
				ipc_message_release(&reply);
				buffer_release(buffer);
				return (u64)-LINUX_EFAULT;
			}
			done += chunk;
		}
	}
	ipc_message_release(&reply);
	buffer_release(buffer);
	return result;
}

static u64 linux_forward_path_output(struct ipc_port *linux,
				     struct ipc_port *reply_port,
				     u32 type, u64 path_user,
				     u64 out_user, u64 out_len,
				     u64 word0, u64 word2, u64 word3)
{
	const u64 path_len = user_cstr_len_limited(path_user,
						   LINUX_MAX_SYSCALL_BUFFER);
	u64 buffer_len;
	struct shared_buffer *buffer;
	struct ipc_message request = {
		.protocol = USER_FOURCC_LINX,
		.type = type,
		.sender = 0,
		.cap_rights = TASK_RIGHT_SEND | TASK_RIGHT_RECV |
			      TASK_RIGHT_DUP,
		.reply_port = reply_port,
		.cap_type = IPC_CAP_BUFFER,
		.cap_object = 0,
		.words = { word0, 0, word2, word3 },
	};
	struct ipc_message reply;
	u8 copy[RISCV64_USER_COPY_CHUNK];

	if (path_len > LINUX_MAX_SYSCALL_BUFFER ||
	    out_user == 0 ||
	    out_len > LINUX_MAX_SHARED_BUFFER) {
		return (u64)-LINUX_EFAULT;
	}
	buffer_len = path_len + 1 > out_len ? path_len + 1 : out_len;
	buffer = buffer_create(buffer_len == 0 ? 1 : buffer_len);
	if (buffer == 0) {
		return (u64)-LINUX_ENOMEM;
	}
	for (u64 done = 0; done < path_len + 1;) {
		const u64 chunk = min_u64(path_len + 1 - done, sizeof(copy));

		if (arch_user_copy_from(copy, path_user + done, chunk) != 0 ||
		    buffer_write(buffer, done, copy, chunk) != 0) {
			buffer_release(buffer);
			return (u64)-LINUX_EFAULT;
		}
		done += chunk;
	}
	request.cap_object = buffer;
	request.words[1] = path_len + 1;
	if (linux_forward_message(linux, reply_port, &request, &reply) != 0) {
		buffer_release(buffer);
		return (u64)-LINUX_ENOSYS;
	}
	const u64 result = reply.words[0];
	if ((i64)result >= 0) {
		u64 copy_len = type == LINUX_MSG_READLINKAT ? result :
			       out_len;

		if (copy_len > out_len) {
			copy_len = out_len;
		}
		for (u64 done = 0; done < copy_len;) {
			const u64 chunk = min_u64(copy_len - done,
						  sizeof(copy));

			if (buffer_read(buffer, done, copy, chunk) != 0 ||
			    arch_user_copy_to(out_user + done, copy,
					      chunk) != 0) {
				ipc_message_release(&reply);
				buffer_release(buffer);
				return (u64)-LINUX_EFAULT;
			}
			done += chunk;
		}
	}
	ipc_message_release(&reply);
	buffer_release(buffer);
	return result;
}

static int linux_unmap_task_range(struct task *task, u64 base, u64 len)
{
	if (vm_unmap_user_range(task_vm_space(task), base, len) != 0) {
		return -1;
	}
	return task_remove_vm_region(task, base, len);
}

static int linux_mmap_file_into_task(struct task *task, struct ipc_port *linux,
				     struct ipc_port *reply_port, u64 base,
				     u64 len, u64 fd, u64 offset,
				     u64 *populated_len)
{
	struct shared_buffer *buffer;
	u8 copy[RISCV64_USER_COPY_CHUNK];
	u64 populated = 0;

	if (linux == 0 || reply_port == 0 || len == 0 ||
	    populated_len == 0 || (offset & (VM_PAGE_SIZE - 1)) != 0) {
		return -1;
	}
	*populated_len = 0;
	buffer = buffer_create(LINUX_MAX_SYSCALL_BUFFER);
	if (buffer == 0) {
		return -1;
	}
	for (u64 done = 0; done < len;) {
		const u64 chunk = min_u64(len - done,
					  LINUX_MAX_SYSCALL_BUFFER);
		struct ipc_message request = {
			.protocol = USER_FOURCC_LINX,
			.type = LINUX_MSG_MMAP,
			.sender = 0,
			.cap_rights = TASK_RIGHT_SEND | TASK_RIGHT_DUP,
			.reply_port = reply_port,
			.cap_type = IPC_CAP_BUFFER,
			.cap_object = buffer,
			.words = { fd, offset + done, chunk, 0 },
		};
		struct ipc_message reply;

		if (linux_forward_message(linux, reply_port, &request, &reply) != 0 ||
		    (i64)reply.words[0] < 0 ||
		    reply.words[0] > chunk ||
		    (reply.words[1] != USER_VFS_MMAP_PAGE_DATA &&
		     reply.words[1] != USER_VFS_MMAP_PAGE_ZERO &&
		     reply.words[1] != USER_VFS_MMAP_PAGE_BUS)) {
			buffer_release(buffer);
			return -1;
		}
		const u64 got = reply.words[0];
		const u64 page_class = reply.words[1];
		ipc_message_release(&reply);
		if (page_class == USER_VFS_MMAP_PAGE_BUS || got == 0) {
			break;
		}
		if (page_class == USER_VFS_MMAP_PAGE_ZERO) {
			done += got;
			populated = done;
			continue;
		}
		for (u64 copied = 0; copied < got;) {
			const u64 part = min_u64(got - copied, sizeof(copy));

			if (buffer_read(buffer, copied, copy, part) != 0 ||
			    vm_write_user(task_vm_space(task),
					  base + done + copied,
					  copy, part) != 0) {
				buffer_release(buffer);
				return -1;
			}
			copied += part;
		}
		done += got;
		populated = done;
	}
	buffer_release(buffer);
	*populated_len = populated;
	if (populated < len) {
		const u64 keep = align_up(populated, VM_PAGE_SIZE);

		if (keep < len &&
		    vm_unmap_user_range(task_vm_space(task), base + keep,
					len - keep) != 0) {
			return -1;
		}
	}
	return 0;
}

static u64 linux_mmap_current(struct ipc_port *linux,
			      struct ipc_port *reply_port,
			      const struct arch_syscall_frame *frame)
{
	struct task *task = task_current();
	const u64 prot = frame->arg2;
	const u64 flags = frame->arg3;
	const u64 fd = frame->a[4];
	const u64 offset = frame->a[5];
	const int anonymous = (flags & LINUX_MAP_ANONYMOUS) != 0;
	const u32 task_prot = linux_prot_to_task(prot);
	const u32 task_flags = linux_map_flags_to_task(flags);
	const u32 writable = (task_prot & TASK_VM_PROT_WRITE) != 0;
	const u32 executable = (task_prot & TASK_VM_PROT_EXEC) != 0;
	const u32 alloc_writable = writable || !anonymous;
	u64 base = frame->arg0;
	u64 len = frame->arg1;
	u64 populated_len;

	if (len == 0 || (flags & LINUX_MAP_PRIVATE) == 0 ||
	    len + VM_PAGE_SIZE - 1 < len ||
	    (!anonymous && (offset & (VM_PAGE_SIZE - 1)) != 0)) {
		return (u64)-LINUX_EINVAL;
	}
	len = align_up(len, VM_PAGE_SIZE);
	populated_len = len;
	if (base == 0) {
		base = task_linux_mmap_next(task);
	} else if ((flags & LINUX_MAP_FIXED) == 0) {
		base = align_up(base, VM_PAGE_SIZE);
	} else if ((base & (VM_PAGE_SIZE - 1)) != 0) {
		return (u64)-LINUX_EINVAL;
	}

	const u64 min_base = (flags & LINUX_MAP_FIXED) != 0 ?
			     LINUX_MAP_FIXED_MIN : LINUX_MMAP_BASE;
	if (base < min_base || base + len < base ||
	    base + len > LINUX_MMAP_LIMIT) {
		return (u64)-LINUX_ENOMEM;
	}
	if ((flags & LINUX_MAP_FIXED) != 0 &&
	    !task_vm_range_is_free(task, base, len) &&
	    linux_unmap_task_range(task, base, len) != 0) {
		return (u64)-LINUX_EINVAL;
	}
	if ((flags & LINUX_MAP_FIXED) == 0 &&
	    !task_vm_range_is_free(task, base, len)) {
		return (u64)-LINUX_ENOMEM;
	}
	if (vm_alloc_user_range(task_vm_space(task), base, len,
				alloc_writable, executable) != 0) {
		return (u64)-LINUX_ENOMEM;
	}
	if (task_add_vm_mapping(task, base, len, task_prot, task_flags,
				TASK_VM_REGION_MMAP,
				anonymous ? TASK_VM_OBJECT_ANON :
					    TASK_VM_OBJECT_FILE,
				anonymous ? 0 : (u32)fd,
				anonymous ? 0 : offset) != 0) {
		(void)vm_unmap_user_range(task_vm_space(task), base, len);
		return (u64)-LINUX_ENOMEM;
	}
	if (!anonymous &&
	    linux_mmap_file_into_task(task, linux, reply_port, base, len,
				      fd, offset, &populated_len) != 0) {
		(void)linux_unmap_task_range(task, base, len);
		return (u64)-LINUX_EINVAL;
	}
	if (!anonymous && !writable && populated_len != 0 &&
	    vm_protect_user_range(task_vm_space(task), base,
				  align_up(populated_len, VM_PAGE_SIZE),
				  0, executable) != 0) {
		(void)linux_unmap_task_range(task, base, len);
		return (u64)-LINUX_EINVAL;
	}
	if (base + len > task_linux_mmap_next(task)) {
		task_set_linux_mmap_next(task, base + len);
	}
	return base;
}

static u64 linux_brk_current(u64 requested)
{
	struct task *task = task_current();

	if (requested == 0) {
		return task_linux_brk(task);
	}
	if (requested >= LINUX_INITIAL_BRK && requested < LINUX_MAX_BRK) {
		const u64 old_brk = task_linux_brk(task);
		const u64 old_page = align_up(old_brk, VM_PAGE_SIZE);
		const u64 new_page = align_up(requested, VM_PAGE_SIZE);

		if (new_page > old_page &&
		    (vm_alloc_user_range(task_vm_space(task), old_page,
					 new_page - old_page, 1, 0) != 0 ||
		     task_add_or_extend_vm_mapping(task, old_page,
						   new_page - old_page,
						   TASK_VM_PROT_READ |
						   TASK_VM_PROT_WRITE,
						   TASK_VM_MAP_PRIVATE |
						   TASK_VM_MAP_ANONYMOUS,
						   TASK_VM_REGION_BRK,
						   TASK_VM_OBJECT_ANON,
						   0, 0) != 0)) {
			return task_linux_brk(task);
		}
		if (new_page < old_page &&
		    linux_unmap_task_range(task, new_page,
					   old_page - new_page) != 0) {
			return task_linux_brk(task);
		}
		task_set_linux_brk(task, requested);
	}
	return task_linux_brk(task);
}

static u64 linux_mprotect_current(u64 addr, u64 size, u64 linux_prot)
{
	struct task *task = task_current();
	const u64 len = align_up(size, VM_PAGE_SIZE);
	const u32 prot = linux_prot_to_task(linux_prot);
	const u32 writable = (prot & TASK_VM_PROT_WRITE) != 0;
	const u32 executable = (prot & TASK_VM_PROT_EXEC) != 0;
	int vm_rc;
	int task_rc;

	if (addr == 0 || (addr & (VM_PAGE_SIZE - 1)) != 0 ||
	    size == 0 || size + VM_PAGE_SIZE - 1 < size ||
	    addr + len < addr) {
		return (u64)-LINUX_EINVAL;
	}

	vm_rc = vm_protect_user_range(task_vm_space(task), addr, len, writable,
				      executable);
	task_rc = vm_rc != 0 ? -1 :
		task_protect_vm_region(task, addr, len, prot);
	if (vm_rc != 0 || task_rc != 0) {
		return (u64)-LINUX_EINVAL;
	}
	console_printf("linux-riscv64: mprotect task=%u addr=%p len=%u prot=0x%x\n",
		       task_id(task), (const void *)addr, (u32)len,
		       (u32)linux_prot);
	return 0;
}

static u64 linux_exit_current(struct ipc_port *linux, struct ipc_port *reply_port,
			      u64 status)
{
	struct ipc_message request = {
		.protocol = USER_FOURCC_LINX,
		.type = LINUX_MSG_EXIT_GROUP,
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
	thread_exit();
}

static const struct linux_syscall_forward_entry riscv64_linux_syscalls[] = {
	{ LINUX_RISCV64_OPENAT, LINUX_FORWARD_OPENAT,
	  LINUX_MSG_OPENAT },
	{ LINUX_RISCV64_CLOSE, LINUX_FORWARD_CLOSE,
	  LINUX_MSG_CLOSE },
	{ LINUX_RISCV64_READ, LINUX_FORWARD_READ,
	  LINUX_MSG_READ },
	{ LINUX_RISCV64_WRITE, LINUX_FORWARD_WRITE,
	  LINUX_MSG_WRITE },
	{ LINUX_RISCV64_READLINKAT, LINUX_FORWARD_READLINKAT,
	  LINUX_MSG_READLINKAT },
	{ LINUX_RISCV64_NEWFSTATAT, LINUX_FORWARD_NEWFSTATAT,
	  LINUX_MSG_NEWFSTATAT },
	{ LINUX_RISCV64_FSTAT, LINUX_FORWARD_FSTAT,
	  LINUX_MSG_FSTAT },
	{ LINUX_RISCV64_EXIT, LINUX_FORWARD_EXIT,
	  LINUX_MSG_EXIT_GROUP },
	{ LINUX_RISCV64_EXIT_GROUP, LINUX_FORWARD_EXIT,
	  LINUX_MSG_EXIT_GROUP },
	{ LINUX_RISCV64_SET_TID_ADDRESS, LINUX_FORWARD_SET_TID_ADDRESS,
	  0 },
	{ LINUX_RISCV64_GETPID, LINUX_FORWARD_SCALAR,
	  LINUX_MSG_GETPID },
	{ LINUX_RISCV64_GETPPID, LINUX_FORWARD_SCALAR,
	  LINUX_MSG_GETPPID },
	{ LINUX_RISCV64_GETUID, LINUX_FORWARD_SCALAR,
	  LINUX_MSG_GETUID },
	{ LINUX_RISCV64_GETEUID, LINUX_FORWARD_SCALAR,
	  LINUX_MSG_GETEUID },
	{ LINUX_RISCV64_GETGID, LINUX_FORWARD_SCALAR,
	  LINUX_MSG_GETGID },
	{ LINUX_RISCV64_GETEGID, LINUX_FORWARD_SCALAR,
	  LINUX_MSG_GETEGID },
	{ LINUX_RISCV64_GETTID, LINUX_FORWARD_SCALAR,
	  LINUX_MSG_GETTID },
	{ LINUX_RISCV64_BRK, LINUX_FORWARD_BRK, 0 },
	{ LINUX_RISCV64_MUNMAP, LINUX_FORWARD_MUNMAP, 0 },
	{ LINUX_RISCV64_MMAP, LINUX_FORWARD_MMAP, 0 },
	{ LINUX_RISCV64_MPROTECT, LINUX_FORWARD_MPROTECT, 0 },
};

static u64 linux_riscv64_syscall_dispatch(struct arch_syscall_frame *frame)
{
	struct ipc_port *linux = ipc_port_find("linux");
	struct ipc_port *reply_port = task_reply_port(task_current());
	const struct linux_syscall_forward_entry *entry;
	u64 registered;

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

	entry = linux_syscall_forward_lookup(
		riscv64_linux_syscalls,
		sizeof(riscv64_linux_syscalls) /
			sizeof(riscv64_linux_syscalls[0]),
		frame->number);
	if (entry == 0) {
		console_printf("linux-riscv64: unknown syscall=%u pc=%p arg0=%p arg1=%p arg2=%p arg3=%p\n",
			       (u32)frame->number,
			       (const void *)frame->user_pc,
			       (const void *)frame->arg0,
			       (const void *)frame->arg1,
			       (const void *)frame->arg2,
			       (const void *)frame->arg3);
		return (u64)-LINUX_ENOSYS;
	}

	switch (entry->op) {
	case LINUX_FORWARD_OPENAT: {
		const u64 path_len = user_cstr_len_limited(
			frame->arg1, LINUX_MAX_SYSCALL_BUFFER);

		if (path_len > LINUX_MAX_SYSCALL_BUFFER) {
			return (u64)-LINUX_EFAULT;
		}
			return linux_forward_user_buffer(
			linux, reply_port, entry->message_type,
			TASK_RIGHT_RECV | TASK_RIGHT_DUP,
			frame->arg1, path_len + 1,
			frame->arg0, path_len + 1,
			frame->arg2, frame->arg3, 1, 0);
	}
	case LINUX_FORWARD_CLOSE:
		return linux_forward_words(linux, reply_port, entry->message_type,
					   frame->arg0, 0, 0, 0);
	case LINUX_FORWARD_READ:
		return linux_forward_user_buffer(
			linux, reply_port, entry->message_type,
			TASK_RIGHT_SEND | TASK_RIGHT_DUP,
			frame->arg1, frame->arg2,
			frame->arg0, frame->arg2, 0, 0, 0, 1);
	case LINUX_FORWARD_WRITE:
		return linux_write_chunked(linux, reply_port, frame->arg0,
					   frame->arg1, frame->arg2);
	case LINUX_FORWARD_READLINKAT:
		return linux_forward_path_output(
			linux, reply_port, entry->message_type,
			frame->arg1, frame->arg2, frame->arg3,
			frame->arg0, frame->arg3, 0);
	case LINUX_FORWARD_NEWFSTATAT:
		return linux_forward_path_output(
			linux, reply_port, entry->message_type,
			frame->arg1, frame->arg2, LINUX_STAT_SIZE,
			frame->arg0, frame->arg3, 0);
	case LINUX_FORWARD_FSTAT:
		return linux_forward_user_buffer(
			linux, reply_port, entry->message_type,
			TASK_RIGHT_SEND | TASK_RIGHT_DUP,
			frame->arg1, LINUX_STAT_SIZE,
			frame->arg0, 0, 0, 0, 0, 1);
	case LINUX_FORWARD_EXIT:
		return linux_exit_current(linux, reply_port, frame->arg0);
	case LINUX_FORWARD_SET_TID_ADDRESS:
		return thread_id(thread_current());
	case LINUX_FORWARD_SCALAR:
		return linux_forward_scalar(linux, reply_port,
					    entry->message_type);
	case LINUX_FORWARD_BRK:
		return linux_brk_current(frame->arg0);
	case LINUX_FORWARD_MUNMAP:
		if (frame->arg0 == 0 ||
		    (frame->arg0 & (VM_PAGE_SIZE - 1)) != 0 ||
		    frame->arg1 == 0 ||
		    frame->arg1 + VM_PAGE_SIZE - 1 < frame->arg1) {
			return (u64)-LINUX_EINVAL;
		}
		return linux_unmap_task_range(task_current(), frame->arg0,
					      align_up(frame->arg1,
						       VM_PAGE_SIZE)) == 0 ?
			0 : (u64)-LINUX_EINVAL;
	case LINUX_FORWARD_MMAP:
		return linux_mmap_current(linux, reply_port, frame);
	case LINUX_FORWARD_MPROTECT:
		return linux_mprotect_current(frame->arg0, frame->arg1,
					      frame->arg2);
	default:
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
