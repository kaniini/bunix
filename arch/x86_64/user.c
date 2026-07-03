#include <arch/user.h>
#include <arch/io.h>
#include <arch/smp.h>
#include "buffer.h"
#include "console.h"
#include "ipc.h"
#include "sched.h"
#include "slab.h"
#include "spinlock.h"
#include "timer.h"
#include "server.h"
#include "vm.h"
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
	MSR_FS_BASE = 0xc0000100,
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
	SYSCALL_TASK_ID = -42,
	SYSCALL_TASK_ALLOC = -44,
	SYSCALL_TASK_CLONE_RANGE = -46,
	SYSCALL_CONSOLE_READ = -48,
	LINUX_SYSCALL_READ = 0,
	LINUX_SYSCALL_WRITE = 1,
	LINUX_SYSCALL_OPEN = 2,
	LINUX_SYSCALL_CLOSE = 3,
	LINUX_SYSCALL_STAT = 4,
	LINUX_SYSCALL_FSTAT = 5,
	LINUX_SYSCALL_POLL = 7,
	LINUX_SYSCALL_MMAP = 9,
	LINUX_SYSCALL_MPROTECT = 10,
	LINUX_SYSCALL_MUNMAP = 11,
	LINUX_SYSCALL_BRK = 12,
	LINUX_SYSCALL_RT_SIGACTION = 13,
	LINUX_SYSCALL_RT_SIGPROCMASK = 14,
	LINUX_SYSCALL_IOCTL = 16,
	LINUX_SYSCALL_WRITEV = 20,
	LINUX_SYSCALL_FCNTL = 72,
	LINUX_SYSCALL_GETCWD = 79,
	LINUX_SYSCALL_CHDIR = 80,
	LINUX_SYSCALL_GETUID = 102,
	LINUX_SYSCALL_SYSLOG = 103,
	LINUX_SYSCALL_GETGID = 104,
	LINUX_SYSCALL_GETEUID = 107,
	LINUX_SYSCALL_GETEGID = 108,
	LINUX_SYSCALL_GETPPID = 110,
	LINUX_SYSCALL_GETPGRP = 111,
	LINUX_SYSCALL_CLONE = 56,
	LINUX_SYSCALL_FORK = 57,
	LINUX_SYSCALL_VFORK = 58,
	LINUX_SYSCALL_KILL = 62,
	LINUX_SYSCALL_EXIT = 60,
	LINUX_SYSCALL_UNAME = 63,
	LINUX_SYSCALL_SETPGID = 109,
	LINUX_SYSCALL_SETSID = 112,
	LINUX_SYSCALL_GETPGID = 121,
	LINUX_SYSCALL_ARCH_PRCTL = 158,
	LINUX_SYSCALL_FUTEX = 202,
	LINUX_SYSCALL_SET_TID_ADDRESS = 218,
	LINUX_SYSCALL_CLOCK_GETTIME = 228,
	LINUX_SYSCALL_EXECVE = 59,
	LINUX_SYSCALL_GETPID = 39,
	LINUX_SYSCALL_WAIT4 = 61,
	LINUX_SYSCALL_GETTID = 186,
	LINUX_SYSCALL_NEWFSTATAT = 262,
	LINUX_SYSCALL_SET_ROBUST_LIST = 273,
	LINUX_SYSCALL_OPENAT = 257,
	LINUX_SYSCALL_PRLIMIT64 = 302,
	LINUX_SYSCALL_GETRANDOM = 318,
	LINUX_SYSCALL_EXIT_GROUP = 231,
	LINUX_EBADF = 9,
	LINUX_EAGAIN = 11,
	LINUX_ENOMEM = 12,
	LINUX_EINVAL = 22,
	LINUX_ENOSYS = 38,
	LINUX_ENOTTY = 25,
	LINUX_POLLIN = 0x0001,
	LINUX_POLLOUT = 0x0004,
	LINUX_ARCH_SET_FS = 0x1002,
	LINUX_FUTEX_WAIT = 0,
	LINUX_FUTEX_WAKE = 1,
	LINUX_TCGETS = 0x5401,
	LINUX_TCSETS = 0x5402,
	LINUX_TIOCGPGRP = 0x540f,
	LINUX_TIOCSPGRP = 0x5410,
	LINUX_TIOCGWINSZ = 0x5413,
	ARCH_USER_MAX_CPUS = 8,
	LINUX_MAX_SYSCALL_BUFFER = 4096,
	LINUX_EXEC_MAX_IMAGE = 2 * 1024 * 1024,
	LINUX_EXEC_MAX_PATH = 32,
	LINUX_EXEC_STACK_TOP = 0x800000,
	LINUX_EXEC_STACK_PAGES = 16,
	LINUX_STAT_SIZE = 144,
	LINUX_WAIT_STATUS_SIZE = 4,
	LINUX_INITIAL_BRK = 0x900000,
	LINUX_MAX_BRK = 0x10000000,
	LINUX_MMAP_BASE = 0x10000000,
	LINUX_MMAP_LIMIT = 0x20000000,
	LINUX_MAX_MMAP_SIZE = 0x1000000,
	LINUX_PROT_READ = 0x1,
	LINUX_PROT_WRITE = 0x2,
	LINUX_PROT_EXEC = 0x4,
	LINUX_MAP_PRIVATE = 0x2,
	LINUX_MAP_FIXED = 0x10,
	LINUX_MAP_ANONYMOUS = 0x20,
	USER_IPC_WORDS = 4,
	USER_FOURCC_CONS = ('C') | ('O' << 8) | ('N' << 16) | ('S' << 24),
	USER_FOURCC_LINX = ('L') | ('I' << 8) | ('N' << 16) | ('X' << 24),
	USER_FOURCC_VFS = ('V') | ('F' << 8) | ('S' << 16) | ('0' << 24),
	USER_CONSOLE_WRITE = 1,
	USER_CONSOLE_LOG = 2,
	USER_CONSOLE_LOGS_TO_RING = 3,
	USER_VFS_OPEN = 4,
	USER_VFS_STAT = 5,
	USER_VFS_READ_FILE_BUFFER = 6,
	USER_VFS_CLOSE = 7,
	USER_VFS_TYPE_REGULAR = 1,
	USER_LINUX_READ = 0,
	USER_LINUX_WRITE = 1,
	USER_LINUX_OPEN = 2,
	USER_LINUX_CLOSE = 3,
	USER_LINUX_FSTAT = 5,
	USER_LINUX_IOCTL = 16,
	USER_LINUX_GETPID = 39,
	USER_LINUX_SETPGID = 109,
	USER_LINUX_GETPPID = 110,
	USER_LINUX_GETPGRP = 111,
	USER_LINUX_SETSID = 112,
	USER_LINUX_GETPGID = 121,
	USER_LINUX_FCNTL = 72,
	USER_LINUX_WAIT4 = 61,
	USER_LINUX_GETTID = 186,
	USER_LINUX_OPENAT = 257,
	USER_LINUX_NEWFSTATAT = 262,
	USER_LINUX_EXIT_GROUP = 231,
	USER_LINUX_REGISTER_PROCESS = 1000,
	USER_LINUX_FORK_PROCESS = 1001,
};

enum {
	ELF_MAGIC0 = 0x7f,
	ELF_MAGIC1 = 'E',
	ELF_MAGIC2 = 'L',
	ELF_MAGIC3 = 'F',
	ELF_CLASS_64 = 2,
	ELF_DATA_LSB = 1,
	ELF_TYPE_EXEC = 2,
	ELF_TYPE_DYN = 3,
	ELF_MACHINE_X86_64 = 62,
	ELF_PH_LOAD = 1,
	ELF_PF_X = 1,
	ELF_PF_W = 2,
	ELF_EHDR_SIZE = 64,
	ELF_PHDR_SIZE = 56,
	ELF_STACK_WORDS = 8,
};

struct elf64_ehdr {
	u8 ident[16];
	u16 type;
	u16 machine;
	u32 version;
	u64 entry;
	u64 phoff;
	u64 shoff;
	u32 flags;
	u16 ehsize;
	u16 phentsize;
	u16 phnum;
	u16 shentsize;
	u16 shnum;
	u16 shstrndx;
} __attribute__((packed));

struct elf64_phdr {
	u32 type;
	u32 flags;
	u64 offset;
	u64 vaddr;
	u64 paddr;
	u64 filesz;
	u64 memsz;
	u64 align;
} __attribute__((packed));

static u8 syscall_copy_buffer[LINUX_MAX_SYSCALL_BUFFER];
static u8 console_input_buffer[LINUX_MAX_SYSCALL_BUFFER];
static struct spinlock syscall_copy_lock = SPINLOCK_INIT("syscall-copy");
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

static u64 align_down(u64 value, u64 align)
{
	return value & ~(align - 1);
}

static u64 align_up(u64 value, u64 align)
{
	return align_down(value + align - 1, align);
}

static u32 linux_prot_to_task(u64 prot)
{
	u32 task_prot = 0;

	if ((prot & LINUX_PROT_READ) != 0) {
		task_prot |= TASK_VM_PROT_READ;
	}
	if ((prot & LINUX_PROT_WRITE) != 0) {
		task_prot |= TASK_VM_PROT_WRITE;
	}
	if ((prot & LINUX_PROT_EXEC) != 0) {
		task_prot |= TASK_VM_PROT_EXEC;
	}
	return task_prot;
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

static u64 min_u64(u64 left, u64 right)
{
	return left < right ? left : right;
}

static u64 max_u64(u64 left, u64 right)
{
	return left > right ? left : right;
}

static void mem_copy(u8 *dst, const u8 *src, u64 len)
{
	for (u64 i = 0; i < len; i++) {
		dst[i] = src[i];
	}
}

static void mem_zero(u8 *dst, u64 len)
{
	for (u64 i = 0; i < len; i++) {
		dst[i] = 0;
	}
}

static int copy_cstr_from_user(char *dst, const char *src, u64 max_len)
{
	if (dst == 0 || src == 0 || max_len == 0) {
		return -1;
	}

	for (u64 i = 0; i < max_len; i++) {
		if (vm_read_user(task_vm_space(task_current()), (u64)src + i,
				 &dst[i], 1) != 0) {
			dst[i] = src[i];
		}
		if (dst[i] == '\0') {
			return 0;
		}
	}

	dst[max_len - 1] = '\0';
	return -1;
}

static u64 str_len(const char *value)
{
	u64 len = 0;

	while (value[len] != '\0') {
		len++;
	}

	return len;
}

static void pack_bytes(u64 *words, const u8 *data, u64 len)
{
	words[0] = 0;
	words[1] = 0;
	for (u64 i = 0; i < len && i < 16; i++) {
		const u64 slot = i / 8;
		const u64 shift = (i % 8) * 8;

		words[slot] |= ((u64)data[i]) << shift;
	}
}

static int read_current_user(u64 vaddr, void *dst, u64 len)
{
	return vm_read_user(task_vm_space(task_current()), vaddr, dst, len);
}

static int write_current_user(u64 vaddr, const void *src, u64 len)
{
	return vm_write_user(task_vm_space(task_current()), vaddr, src, len);
}

static int console_write_user(u64 vaddr, u64 len)
{
	enum {
		CONSOLE_COPY_CHUNK = 128,
	};

	u8 buffer[CONSOLE_COPY_CHUNK];
	u64 done = 0;

	if (vaddr == 0) {
		return -1;
	}

	while (done < len) {
		const u64 chunk = min_u64(len - done, CONSOLE_COPY_CHUNK);

		if (read_current_user(vaddr + done, buffer, chunk) != 0) {
			return -1;
		}
		console_write_len((const char *)buffer, chunk);
		done += chunk;
	}

	return 0;
}

static int console_log_user(u64 vaddr, u64 len)
{
	enum {
		CONSOLE_COPY_CHUNK = 128,
	};

	u8 buffer[CONSOLE_COPY_CHUNK];
	u64 done = 0;

	if (vaddr == 0) {
		return -1;
	}

	while (done < len) {
		const u64 chunk = min_u64(len - done, CONSOLE_COPY_CHUNK);

		if (read_current_user(vaddr + done, buffer, chunk) != 0) {
			return -1;
		}
		console_log_write_len((const char *)buffer, chunk);
		done += chunk;
	}

	return 0;
}

static int linux_vfs_close(struct task *task, u64 file)
{
	struct ipc_port *vfs = ipc_port_find("vfs");
	struct ipc_port *reply_port = task_reply_port(task);
	struct ipc_message request = {
		.protocol = USER_FOURCC_VFS,
		.type = USER_VFS_CLOSE,
		.sender = 0,
		.cap_rights = 0,
		.reply_port = reply_port,
		.cap_type = IPC_CAP_NONE,
		.cap_object = 0,
		.words = { file, 0, 0, 0 },
	};
	struct ipc_message reply;

	if (vfs == 0 || reply_port == 0 ||
	    ipc_send(vfs, &request) != 0 ||
	    ipc_recv(reply_port, &reply) != 0 ||
	    reply.words[0] != 0) {
		return -1;
	}

	return 0;
}

static int linux_vfs_read_file(struct task *task, const char *path,
			       u8 *image, u64 image_cap, u64 *image_size)
{
	struct ipc_port *vfs = ipc_port_find("vfs");
	struct ipc_port *reply_port = task_reply_port(task);
	u64 packed[2];
	struct ipc_message request;
	struct ipc_message reply;
	u64 file;
	u64 size;
	struct shared_buffer *buffer;

	if (vfs == 0 || reply_port == 0 || path == 0 || image == 0 ||
	    image_size == 0) {
		return -1;
	}

	pack_bytes(packed, (const u8 *)path, str_len(path) + 1);
	request = (struct ipc_message){
		.protocol = USER_FOURCC_VFS,
		.type = USER_VFS_OPEN,
		.sender = 0,
		.cap_rights = 0,
		.reply_port = reply_port,
		.cap_type = IPC_CAP_NONE,
		.cap_object = 0,
		.words = { packed[0], packed[1], 0, 0 },
	};
	if (ipc_send(vfs, &request) != 0 ||
	    ipc_recv(reply_port, &reply) != 0 ||
	    reply.words[0] != 0 ||
	    reply.words[3] != USER_VFS_TYPE_REGULAR) {
		return -1;
	}

	file = reply.words[1];
	size = reply.words[2];
	if (size == 0 || size > image_cap) {
		(void)linux_vfs_close(task, file);
		return -1;
	}

	buffer = buffer_create(LINUX_MAX_SYSCALL_BUFFER);
	if (buffer == 0) {
		(void)linux_vfs_close(task, file);
		return -1;
	}

	for (u64 offset = 0; offset < size;) {
		const u64 chunk = min_u64(size - offset,
					  LINUX_MAX_SYSCALL_BUFFER);

		request = (struct ipc_message){
			.protocol = USER_FOURCC_VFS,
			.type = USER_VFS_READ_FILE_BUFFER,
			.sender = 0,
			.cap_rights = TASK_RIGHT_SEND | TASK_RIGHT_DUP,
			.reply_port = reply_port,
			.cap_type = IPC_CAP_BUFFER,
			.cap_object = buffer,
			.words = { file, offset, chunk, 0 },
		};
		if (ipc_send(vfs, &request) != 0 ||
		    ipc_recv(reply_port, &reply) != 0 ||
		    reply.words[0] != 0 ||
		    reply.words[1] != chunk ||
		    buffer_read(buffer, 0, image + offset, chunk) != 0) {
			buffer_release(buffer);
			(void)linux_vfs_close(task, file);
			return -1;
		}
		offset += chunk;
	}

	buffer_release(buffer);
	if (linux_vfs_close(task, file) != 0) {
		return -1;
	}

	*image_size = size;
	return 0;
}

static int linux_exec_validate(const u8 *image, u64 image_size,
			       const struct elf64_ehdr **ehdr_out)
{
	const struct elf64_ehdr *ehdr;
	u64 phdr_bytes;

	if (image == 0 || ehdr_out == 0 || image_size < ELF_EHDR_SIZE) {
		return -1;
	}

	ehdr = (const struct elf64_ehdr *)image;
	phdr_bytes = ehdr->phoff + (u64)ehdr->phnum * ehdr->phentsize;
	if (ehdr->phoff > phdr_bytes ||
	    ehdr->ident[0] != ELF_MAGIC0 ||
	    ehdr->ident[1] != ELF_MAGIC1 ||
	    ehdr->ident[2] != ELF_MAGIC2 ||
	    ehdr->ident[3] != ELF_MAGIC3 ||
	    ehdr->ident[4] != ELF_CLASS_64 ||
	    ehdr->ident[5] != ELF_DATA_LSB ||
	    (ehdr->type != ELF_TYPE_EXEC && ehdr->type != ELF_TYPE_DYN) ||
	    ehdr->machine != ELF_MACHINE_X86_64 ||
	    ehdr->ehsize != ELF_EHDR_SIZE ||
	    ehdr->phentsize != ELF_PHDR_SIZE ||
	    ehdr->phnum == 0 ||
	    phdr_bytes > image_size) {
		return -1;
	}

	for (u64 i = 0; i < ehdr->phnum; i++) {
		const struct elf64_phdr *phdr =
			(const struct elf64_phdr *)(image + ehdr->phoff +
						    i * ehdr->phentsize);

		if (phdr->type != ELF_PH_LOAD) {
			continue;
		}
		if (phdr->filesz > phdr->memsz ||
		    phdr->offset + phdr->filesz < phdr->offset ||
		    phdr->offset + phdr->filesz > image_size ||
		    phdr->vaddr + phdr->memsz < phdr->vaddr ||
		    phdr->memsz == 0) {
			return -1;
		}
	}

	*ehdr_out = ehdr;
	return 0;
}

static int linux_exec_unmap_current(struct task *task)
{
	struct vm_space *space = task_vm_space(task);

	while (task_vm_region_count(task) != 0) {
		const struct task_vm_region *region =
			task_vm_region_at(task, 0);
		const u64 base = region->base;
		const u64 len = region->len;

		if (vm_unmap_user_range(space, base, len) != 0) {
			return -1;
		}
		if (task_remove_vm_region(task, base, len) != 0) {
			return -1;
		}
	}

	task_clear_vm_regions(task);
	task_set_linux_brk(task, LINUX_INITIAL_BRK);
	task_set_linux_mmap_next(task, LINUX_MMAP_BASE);
	return 0;
}

static int linux_unmap_task_range(struct task *task, u64 base, u64 len)
{
	if (vm_unmap_user_range(task_vm_space(task), base, len) != 0) {
		return -1;
	}
	return task_remove_vm_region(task, base, len);
}

static u64 linux_exit_current(u64 status)
{
	struct ipc_port *linux = ipc_port_find("linux");
	struct ipc_port *reply_port = task_reply_port(task_current());
	struct ipc_message request = {
		.protocol = USER_FOURCC_LINX,
		.type = USER_LINUX_EXIT_GROUP,
		.sender = 0,
		.cap_rights = 0,
		.reply_port = reply_port,
		.cap_type = IPC_CAP_NONE,
		.cap_object = 0,
		.words = { status, 0, 0, 0 },
	};
	struct ipc_message reply;

	if (linux != 0 && reply_port != 0) {
		(void)ipc_send(linux, &request);
		(void)ipc_recv(reply_port, &reply);
	}
	console_printf("linux: exit_group status=%u\n", (u32)status);
	__asm__ volatile ("sti");
	thread_exit();
}

static u64 linux_write_one(struct ipc_port *linux, struct ipc_port *reply_port,
			   u64 fd, u64 user_buffer, u64 len)
{
	struct ipc_message request = {
		.protocol = USER_FOURCC_LINX,
		.type = USER_LINUX_WRITE,
		.sender = 0,
		.cap_rights = 0,
		.reply_port = reply_port,
		.cap_type = IPC_CAP_NONE,
		.cap_object = 0,
		.words = { fd, len, 0, 0 },
	};
	struct ipc_message reply;
	struct shared_buffer *buffer;
	u64 flags;

	if (len > LINUX_MAX_SYSCALL_BUFFER) {
		return (u64)-LINUX_EINVAL;
	}

	buffer = buffer_create(len == 0 ? 1 : len);
	if (buffer == 0) {
		buffer_release(buffer);
		return (u64)-LINUX_EINVAL;
	}
	if (len != 0) {
		flags = spin_lock_irqsave(&syscall_copy_lock);
		if (read_current_user(user_buffer, syscall_copy_buffer, len) != 0 ||
		    buffer_write(buffer, 0, syscall_copy_buffer, len) != 0) {
			spin_unlock_irqrestore(&syscall_copy_lock, flags);
			buffer_release(buffer);
			return (u64)-LINUX_EINVAL;
		}
		spin_unlock_irqrestore(&syscall_copy_lock, flags);
	}

	request.cap_type = IPC_CAP_BUFFER;
	request.cap_rights = TASK_RIGHT_RECV;
	request.cap_object = buffer;
	if (ipc_send(linux, &request) != 0 ||
	    ipc_recv(reply_port, &reply) != 0) {
		buffer_release(buffer);
		return (u64)-LINUX_ENOSYS;
	}

	buffer_release(buffer);
	return reply.words[0];
}

static int linux_exec_map_segment(struct task *task, const u8 *image,
				  const struct elf64_phdr *phdr)
{
	struct vm_space *space = task_vm_space(task);
	const u32 writable = (phdr->flags & ELF_PF_W) != 0;
	const u64 page_start = align_down(phdr->vaddr, VM_PAGE_SIZE);
	const u64 page_end = align_up(phdr->vaddr + phdr->memsz, VM_PAGE_SIZE);

	for (u64 page = page_start; page < page_end; page += VM_PAGE_SIZE) {
		struct vm_frame frame = vm_alloc_user_page(space, page,
							   writable);
		const u64 page_end_addr = page + VM_PAGE_SIZE;
		const u64 copy_start = max_u64(page, phdr->vaddr);
		const u64 copy_end = min_u64(page_end_addr,
					     phdr->vaddr + phdr->filesz);

		if (frame.addr == 0) {
			return -1;
		}
		mem_zero((u8 *)frame.addr, VM_PAGE_SIZE);
		if (copy_start < copy_end) {
			const u64 dst_offset = copy_start - page;
			const u64 src_offset = copy_start - phdr->vaddr;

			mem_copy((u8 *)(frame.addr + dst_offset),
				 image + phdr->offset + src_offset,
				 copy_end - copy_start);
		}
	}

	const u32 prot = TASK_VM_PROT_READ |
			 (writable != 0 ? TASK_VM_PROT_WRITE : 0) |
			 ((phdr->flags & ELF_PF_X) != 0 ?
			  TASK_VM_PROT_EXEC : 0);

	if (task_add_vm_mapping(task, page_start, page_end - page_start,
				prot, TASK_VM_MAP_PRIVATE, TASK_VM_REGION_ELF,
				TASK_VM_OBJECT_FILE, 0, phdr->offset) != 0) {
		return -1;
	}

	return 0;
}

static int linux_exec_push_word(u8 *stack, u64 *sp, u64 word)
{
	if (*sp < sizeof(word)) {
		return -1;
	}

	*sp -= sizeof(word);
	mem_copy(stack + *sp, (const u8 *)&word, sizeof(word));
	return 0;
}

static int linux_exec_build_stack(const char *path, u8 *stack_page,
				  u64 *new_sp)
{
	const u64 path_len = str_len(path) + 1;
	u64 sp = VM_PAGE_SIZE;
	u64 path_vaddr;

	mem_zero(stack_page, VM_PAGE_SIZE);
	if (path_len >= VM_PAGE_SIZE / 2) {
		return -1;
	}

	sp -= path_len;
	mem_copy(stack_page + sp, (const u8 *)path, path_len);
	path_vaddr = LINUX_EXEC_STACK_TOP - VM_PAGE_SIZE + sp;
	sp = align_down(sp, 16);

	if (linux_exec_push_word(stack_page, &sp, 0) != 0 ||
	    linux_exec_push_word(stack_page, &sp, 0) != 0 ||
	    linux_exec_push_word(stack_page, &sp, 0) != 0 ||
	    linux_exec_push_word(stack_page, &sp, 0) != 0 ||
	    linux_exec_push_word(stack_page, &sp, path_vaddr) != 0 ||
	    linux_exec_push_word(stack_page, &sp, 1) != 0) {
		return -1;
	}

	*new_sp = LINUX_EXEC_STACK_TOP - VM_PAGE_SIZE + sp;
	return 0;
}

static u64 linux_execve(struct task *task, struct arch_syscall_frame *frame,
			const char *user_path) __attribute__((noinline));

static u64 linux_execve(struct task *task, struct arch_syscall_frame *frame,
			const char *user_path)
{
	char path[LINUX_EXEC_MAX_PATH];
	u64 image_size = 0;
	const struct elf64_ehdr *ehdr;
	u8 stack_page[VM_PAGE_SIZE];
	u8 *image;
	u64 new_sp = 0;
	const u64 stack_base =
		LINUX_EXEC_STACK_TOP - LINUX_EXEC_STACK_PAGES * VM_PAGE_SIZE;

	if (copy_cstr_from_user(path, user_path, sizeof(path)) != 0 ||
	    str_len(path) >= 16) {
		return (u64)-LINUX_EINVAL;
	}

	image = slab_alloc(LINUX_EXEC_MAX_IMAGE);
	if (image == 0) {
		return (u64)-LINUX_ENOMEM;
	}

	if (linux_vfs_read_file(task, path, image, LINUX_EXEC_MAX_IMAGE,
				&image_size) != 0 ||
	    linux_exec_validate(image, image_size, &ehdr) != 0 ||
	    linux_exec_build_stack(path, stack_page, &new_sp) != 0) {
		slab_free(image);
		return (u64)-LINUX_EINVAL;
	}

	if (linux_exec_unmap_current(task) != 0) {
		slab_free(image);
		return (u64)-LINUX_EINVAL;
	}

	for (u64 i = 0; i < ehdr->phnum; i++) {
		const struct elf64_phdr *phdr =
			(const struct elf64_phdr *)(image +
						    ehdr->phoff +
						    i * ehdr->phentsize);

		if (phdr->type == ELF_PH_LOAD &&
		    linux_exec_map_segment(task, image, phdr) != 0) {
			slab_free(image);
			return (u64)-LINUX_ENOMEM;
		}
	}

	if (vm_alloc_user_range(task_vm_space(task), stack_base,
				LINUX_EXEC_STACK_PAGES * VM_PAGE_SIZE, 1) != 0 ||
	    task_add_vm_mapping(task, stack_base,
				LINUX_EXEC_STACK_PAGES * VM_PAGE_SIZE,
				TASK_VM_PROT_READ | TASK_VM_PROT_WRITE,
				TASK_VM_MAP_PRIVATE | TASK_VM_MAP_ANONYMOUS,
				TASK_VM_REGION_STACK, TASK_VM_OBJECT_ANON,
				0, 0) != 0 ||
		    vm_write_user(task_vm_space(task), LINUX_EXEC_STACK_TOP -
				  VM_PAGE_SIZE, stack_page, VM_PAGE_SIZE) != 0) {
		slab_free(image);
		return (u64)-LINUX_ENOMEM;
	}

	frame->user_rip = ehdr->entry;
	frame->user_rsp = new_sp;
	console_printf("linux: execve task=%u path=%s entry=%p stack=%p\n",
		       task_id(task), path, (const void *)ehdr->entry,
		       (const void *)new_sp);
	slab_free(image);
	return 0;
}

static u64 linux_fork_process(struct task *parent, struct task *child)
{
	struct ipc_port *linux = ipc_port_find("linux");
	struct ipc_port *reply_port = task_reply_port(parent);
	struct ipc_message request = {
		.protocol = USER_FOURCC_LINX,
		.type = USER_LINUX_FORK_PROCESS,
		.sender = 0,
		.cap_rights = 0,
		.reply_port = reply_port,
		.cap_type = IPC_CAP_NONE,
		.cap_object = 0,
		.words = { task_id(parent), task_id(child), 0, 0 },
	};
	struct ipc_message reply;

	if (linux == 0 || reply_port == 0 ||
	    ipc_send(linux, &request) != 0 ||
	    ipc_recv(reply_port, &reply) != 0) {
		return (u64)-LINUX_ENOSYS;
	}

	return reply.words[0];
}

static const char *linux_syscall_name(u64 number)
{
	switch (number) {
	case LINUX_SYSCALL_READ:
		return "read";
	case LINUX_SYSCALL_WRITE:
		return "write";
	case LINUX_SYSCALL_OPEN:
		return "open";
	case LINUX_SYSCALL_CLOSE:
		return "close";
	case LINUX_SYSCALL_STAT:
		return "stat";
	case LINUX_SYSCALL_FSTAT:
		return "fstat";
	case LINUX_SYSCALL_POLL:
		return "poll";
	case LINUX_SYSCALL_MMAP:
		return "mmap";
	case LINUX_SYSCALL_MPROTECT:
		return "mprotect";
	case LINUX_SYSCALL_MUNMAP:
		return "munmap";
	case LINUX_SYSCALL_BRK:
		return "brk";
	case LINUX_SYSCALL_RT_SIGACTION:
		return "rt_sigaction";
	case LINUX_SYSCALL_RT_SIGPROCMASK:
		return "rt_sigprocmask";
	case LINUX_SYSCALL_IOCTL:
		return "ioctl";
	case LINUX_SYSCALL_WRITEV:
		return "writev";
	case LINUX_SYSCALL_GETCWD:
		return "getcwd";
	case LINUX_SYSCALL_CHDIR:
		return "chdir";
	case LINUX_SYSCALL_GETUID:
		return "getuid";
	case LINUX_SYSCALL_SYSLOG:
		return "syslog";
	case LINUX_SYSCALL_GETGID:
		return "getgid";
	case LINUX_SYSCALL_GETEUID:
		return "geteuid";
	case LINUX_SYSCALL_GETEGID:
		return "getegid";
	case LINUX_SYSCALL_GETPPID:
		return "getppid";
	case LINUX_SYSCALL_GETPGRP:
		return "getpgrp";
	case LINUX_SYSCALL_GETPGID:
		return "getpgid";
	case LINUX_SYSCALL_SETPGID:
		return "setpgid";
	case LINUX_SYSCALL_SETSID:
		return "setsid";
	case LINUX_SYSCALL_CLONE:
		return "clone";
	case LINUX_SYSCALL_FORK:
		return "fork";
	case LINUX_SYSCALL_VFORK:
		return "vfork";
	case LINUX_SYSCALL_KILL:
		return "kill";
	case LINUX_SYSCALL_EXIT:
		return "exit";
	case LINUX_SYSCALL_UNAME:
		return "uname";
	case LINUX_SYSCALL_FCNTL:
		return "fcntl";
	case LINUX_SYSCALL_ARCH_PRCTL:
		return "arch_prctl";
	case LINUX_SYSCALL_FUTEX:
		return "futex";
	case LINUX_SYSCALL_SET_TID_ADDRESS:
		return "set_tid_address";
	case LINUX_SYSCALL_CLOCK_GETTIME:
		return "clock_gettime";
	case LINUX_SYSCALL_EXECVE:
		return "execve";
	case LINUX_SYSCALL_GETPID:
		return "getpid";
	case LINUX_SYSCALL_WAIT4:
		return "wait4";
	case LINUX_SYSCALL_GETTID:
		return "gettid";
	case LINUX_SYSCALL_NEWFSTATAT:
		return "newfstatat";
	case LINUX_SYSCALL_SET_ROBUST_LIST:
		return "set_robust_list";
	case LINUX_SYSCALL_OPENAT:
		return "openat";
	case LINUX_SYSCALL_PRLIMIT64:
		return "prlimit64";
	case LINUX_SYSCALL_GETRANDOM:
		return "getrandom";
	case LINUX_SYSCALL_EXIT_GROUP:
		return "exit_group";
	default:
		return "unknown";
	}
}

static void linux_strace_log(const struct arch_syscall_frame *frame, u64 result)
{
	const struct task *task = task_current();
	const u64 number = frame->number;

	console_printf("linux-strace: task=%u name=%s rip=%p %s(%p,%p,%p,%p) = %p\n",
		       task_id(task), task_name(task),
		       (const void *)frame->user_rip,
		       linux_syscall_name(number),
		       (const void *)frame->arg0, (const void *)frame->arg1,
		       (const void *)frame->arg2, (const void *)frame->arg3,
		       (const void *)result);
}

static void linux_strace_enter(const struct arch_syscall_frame *frame)
{
	const struct task *task = task_current();
	const u64 number = frame->number;

	console_printf("linux-strace-enter: task=%u name=%s rip=%p %s(%p,%p,%p,%p)\n",
		       task_id(task), task_name(task),
		       (const void *)frame->user_rip,
		       linux_syscall_name(number),
		       (const void *)frame->arg0, (const void *)frame->arg1,
		       (const void *)frame->arg2, (const void *)frame->arg3);
}

static u64 linux_syscall_handle(struct arch_syscall_frame *frame)
	__attribute__((noinline));

static u64 linux_syscall_handle(struct arch_syscall_frame *frame)
{
	struct task *task = task_current();
	const u64 number = frame->number;
	const u64 arg0 = frame->arg0;
	const u64 arg1 = frame->arg1;
	const u64 arg2 = frame->arg2;
	const u64 arg3 = frame->arg3;
	struct ipc_port *linux = ipc_port_find("linux");
	struct ipc_port *reply_port = task_reply_port(task);
	struct ipc_message request = {
		.protocol = USER_FOURCC_LINX,
		.type = (u32)number,
		.sender = 0,
		.cap_rights = 0,
		.reply_port = 0,
		.cap_type = IPC_CAP_NONE,
		.cap_object = 0,
		.words = { arg0, arg1, arg2, 0 },
	};
	struct ipc_message reply;

	switch (number) {
	case LINUX_SYSCALL_EXECVE:
		return linux_execve(task, frame, (const char *)arg0);
	case LINUX_SYSCALL_MMAP: {
		const u64 prot = arg2;
		const u64 flags = arg3;
		const u32 task_prot = linux_prot_to_task(prot);
		const u32 task_flags = linux_map_flags_to_task(flags);
		const u32 writable = (task_prot & TASK_VM_PROT_WRITE) != 0;
		u64 base = arg0;
		u64 len = arg1;

		if (len == 0 || len > LINUX_MAX_MMAP_SIZE ||
		    (flags & LINUX_MAP_ANONYMOUS) == 0 ||
		    (flags & LINUX_MAP_PRIVATE) == 0 ||
		    len + VM_PAGE_SIZE - 1 < len) {
			return (u64)-LINUX_EINVAL;
		}

		len = align_up(len, VM_PAGE_SIZE);
		if (base == 0) {
			base = task_linux_mmap_next(task);
		} else if ((flags & LINUX_MAP_FIXED) == 0) {
			base = align_up(base, VM_PAGE_SIZE);
		} else if ((base & (VM_PAGE_SIZE - 1)) != 0) {
			return (u64)-LINUX_EINVAL;
		}

		if (base < LINUX_MMAP_BASE || base + len < base ||
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
					writable) != 0) {
			return (u64)-LINUX_ENOMEM;
		}
		if (task_add_vm_mapping(task, base, len, task_prot,
					task_flags, TASK_VM_REGION_MMAP,
					TASK_VM_OBJECT_ANON, 0, 0) != 0) {
			(void)vm_unmap_user_range(task_vm_space(task), base, len);
			return (u64)-LINUX_ENOMEM;
		}

		if (base + len > task_linux_mmap_next(task)) {
			task_set_linux_mmap_next(task, base + len);
		}
		console_printf("linux: mmap task=%u addr=%p len=%u flags=0x%x\n",
			       task_id(task), (const void *)base, (u32)len,
			       (u32)flags);
		return base;
	}
	case LINUX_SYSCALL_MUNMAP: {
		u64 base = arg0;
		u64 len = arg1;

		if (base == 0 || len == 0 ||
		    (base & (VM_PAGE_SIZE - 1)) != 0 ||
		    len + VM_PAGE_SIZE - 1 < len) {
			return (u64)-LINUX_EINVAL;
		}

		len = align_up(len, VM_PAGE_SIZE);
		if (linux_unmap_task_range(task, base, len) != 0) {
			return (u64)-LINUX_EINVAL;
		}
		console_printf("linux: munmap task=%u addr=%p len=%u\n",
			       task_id(task), (const void *)base, (u32)len);
		return 0;
	}
	case LINUX_SYSCALL_CLONE:
	case LINUX_SYSCALL_FORK:
	case LINUX_SYSCALL_VFORK: {
		struct task *child = server_task_fork_current(frame);
		u64 pid;

		if (child == 0) {
			return (u64)-LINUX_ENOMEM;
		}

		pid = linux_fork_process(task, child);
		if ((i64)pid < 0) {
			return pid;
		}
		console_printf("linux: fork parent=%u child=%u linux_pid=%u\n",
			       task_id(task), task_id(child), (u32)pid);
		return pid;
	}
	case LINUX_SYSCALL_BRK:
		if (arg0 == 0) {
			return task_linux_brk(task);
		}
		if (arg0 >= LINUX_INITIAL_BRK && arg0 < LINUX_MAX_BRK) {
			const u64 old_brk = task_linux_brk(task);
			const u64 old_page = align_up(old_brk, VM_PAGE_SIZE);
			const u64 new_page = align_up(arg0, VM_PAGE_SIZE);

			if (new_page > old_page &&
			    (vm_alloc_user_range(task_vm_space(task), old_page,
						 new_page - old_page, 1) != 0 ||
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
			task_set_linux_brk(task, arg0);
		}
		return task_linux_brk(task);
	case LINUX_SYSCALL_ARCH_PRCTL:
		if (arg0 != LINUX_ARCH_SET_FS) {
			return (u64)-LINUX_EINVAL;
		}
		task_set_linux_fs_base(task, arg1);
		arch_user_set_fs_base(arg1);
		console_printf("linux: arch_prctl set_fs=%p\n",
			       (const void *)arg1);
		return 0;
	case LINUX_SYSCALL_SET_TID_ADDRESS:
		return thread_id(thread_current());
	case LINUX_SYSCALL_GETUID:
	case LINUX_SYSCALL_GETGID:
	case LINUX_SYSCALL_GETEUID:
	case LINUX_SYSCALL_GETEGID:
		return 0;
	case LINUX_SYSCALL_SYSLOG: {
		enum {
			LINUX_SYSLOG_ACTION_READ = 2,
			LINUX_SYSLOG_ACTION_READ_ALL = 3,
			LINUX_SYSLOG_ACTION_READ_CLEAR = 4,
			LINUX_SYSLOG_ACTION_CLEAR = 5,
			LINUX_SYSLOG_ACTION_CONSOLE_OFF = 6,
			LINUX_SYSLOG_ACTION_CONSOLE_ON = 7,
			LINUX_SYSLOG_ACTION_CONSOLE_LEVEL = 8,
			LINUX_SYSLOG_ACTION_SIZE_BUFFER = 10,
		};
		const u64 size = min_u64(console_log_size(),
					 LINUX_MAX_SYSCALL_BUFFER);
		u64 nread;
		u64 flags;

		switch (arg0) {
		case LINUX_SYSLOG_ACTION_SIZE_BUFFER:
			return size;
		case LINUX_SYSLOG_ACTION_CLEAR:
		case LINUX_SYSLOG_ACTION_CONSOLE_OFF:
		case LINUX_SYSLOG_ACTION_CONSOLE_ON:
		case LINUX_SYSLOG_ACTION_CONSOLE_LEVEL:
			return 0;
		case LINUX_SYSLOG_ACTION_READ:
		case LINUX_SYSLOG_ACTION_READ_ALL:
		case LINUX_SYSLOG_ACTION_READ_CLEAR:
			if (arg1 == 0) {
				return (u64)-LINUX_EINVAL;
			}
			nread = min_u64(arg2, size);
			flags = spin_lock_irqsave(&syscall_copy_lock);
			nread = console_log_read((char *)syscall_copy_buffer,
						 nread);
			if (write_current_user(arg1, syscall_copy_buffer,
					       nread) != 0) {
				spin_unlock_irqrestore(&syscall_copy_lock, flags);
				return (u64)-LINUX_EINVAL;
			}
			spin_unlock_irqrestore(&syscall_copy_lock, flags);
			return nread;
		default:
			return (u64)-LINUX_EINVAL;
		}
	}
	case LINUX_SYSCALL_KILL:
		return 0;
	case LINUX_SYSCALL_GETCWD: {
		const char cwd[] = "/";

		if (arg0 == 0 || arg1 < sizeof(cwd)) {
			return (u64)-LINUX_EINVAL;
		}
		return write_current_user(arg0, cwd, sizeof(cwd)) == 0 ?
		       sizeof(cwd) : (u64)-LINUX_EINVAL;
	}
	case LINUX_SYSCALL_CHDIR:
		return 0;
	case LINUX_SYSCALL_SET_ROBUST_LIST:
	case LINUX_SYSCALL_RT_SIGPROCMASK:
		return 0;
	case LINUX_SYSCALL_RT_SIGACTION:
		if (arg2 != 0) {
			u8 action[32];

			mem_zero(action, sizeof(action));
			if (write_current_user(arg2, action, sizeof(action)) != 0) {
				return (u64)-LINUX_EINVAL;
			}
		}
		return 0;
	case LINUX_SYSCALL_MPROTECT:
		if (arg0 == 0 || (arg0 & (VM_PAGE_SIZE - 1)) != 0 ||
		    arg1 == 0 || arg1 + VM_PAGE_SIZE - 1 < arg1) {
			return (u64)-LINUX_EINVAL;
		}
		{
			const u64 len = align_up(arg1, VM_PAGE_SIZE);
			const u32 prot = linux_prot_to_task(arg2);
			const u32 writable =
				(prot & TASK_VM_PROT_WRITE) != 0;

			if (arg0 + len < arg0 ||
			    vm_protect_user_range(task_vm_space(task), arg0,
						  len, writable) != 0 ||
			    task_protect_vm_region(task, arg0, len,
						   prot) != 0) {
				return (u64)-LINUX_EINVAL;
			}
			console_printf("linux: mprotect task=%u addr=%p len=%u prot=0x%x\n",
				       task_id(task), (const void *)arg0,
				       (u32)len, (u32)arg2);
		}
		return 0;
	case LINUX_SYSCALL_IOCTL:
		if (arg1 == LINUX_TCGETS || arg1 == LINUX_TCSETS ||
		    arg1 == LINUX_TIOCGPGRP || arg1 == LINUX_TIOCSPGRP ||
		    arg1 == LINUX_TIOCGWINSZ) {
			break;
		}
		return (u64)-LINUX_ENOTTY;
	case LINUX_SYSCALL_FCNTL:
		break;
	case LINUX_SYSCALL_CLOCK_GETTIME: {
		u64 timespec[2];
		const u64 ns = timer_monotonic_ns();

		if (arg1 == 0) {
			return (u64)-LINUX_EINVAL;
		}
		timespec[0] = ns / 1000000000ull;
		timespec[1] = ns % 1000000000ull;
		return write_current_user(arg1, timespec, sizeof(timespec)) == 0 ?
		       0 : (u64)-LINUX_EINVAL;
	}
	case LINUX_SYSCALL_GETRANDOM: {
		u64 done = 0;

		if (arg0 == 0 || arg1 > LINUX_MAX_SYSCALL_BUFFER) {
			return (u64)-LINUX_EINVAL;
		}
		while (done < arg1) {
			const u8 value = (u8)(0xa5u ^ (u8)done);

			if (write_current_user(arg0 + done, &value, 1) != 0) {
				return (u64)-LINUX_EINVAL;
			}
			done++;
		}
		return arg1;
	}
	case LINUX_SYSCALL_UNAME: {
		u8 uts[65 * 6];
		const char sysname[] = "Linux";
		const char nodename[] = "bunix";
		const char release[] = "6.0.0-bunix";
		const char version[] = "#1";
		const char machine[] = "x86_64";
		const char domain[] = "local";
		const char *fields[] = {
			sysname, nodename, release, version, machine, domain,
		};

		if (arg0 == 0) {
			return (u64)-LINUX_EINVAL;
		}
		mem_zero(uts, sizeof(uts));
		for (u64 field = 0; field < 6; field++) {
			const char *text = fields[field];
			u64 i = 0;

			while (text[i] != '\0' && i < 64) {
				uts[field * 65 + i] = (u8)text[i];
				i++;
			}
		}
		return write_current_user(arg0, uts, sizeof(uts)) == 0 ?
		       0 : (u64)-LINUX_EINVAL;
	}
	case LINUX_SYSCALL_PRLIMIT64:
		if (arg3 != 0) {
			u64 limit[2] = { 0x800000, 0x800000 };

			if (write_current_user(arg3, limit, sizeof(limit)) != 0) {
				return (u64)-LINUX_EINVAL;
			}
		}
		return 0;
	case LINUX_SYSCALL_FUTEX: {
		const u64 op = arg1 & 0x7f;

		if (op == LINUX_FUTEX_WAKE) {
			return 0;
		}
		if (op == LINUX_FUTEX_WAIT) {
			u32 value = 0;

			if (arg0 == 0 ||
			    read_current_user(arg0, &value, sizeof(value)) != 0) {
				return (u64)-LINUX_EINVAL;
			}
			return value != (u32)arg2 ? (u64)-LINUX_EAGAIN : 0;
		}
		return (u64)-LINUX_ENOSYS;
	}
	case LINUX_SYSCALL_POLL: {
		struct {
			int fd;
			short events;
			short revents;
		} pollfd;
		u64 ready = 0;

		if (arg0 == 0 || arg1 > 64) {
			return (u64)-LINUX_EINVAL;
		}
		for (u64 i = 0; i < arg1; i++) {
			const u64 addr = arg0 + i * sizeof(pollfd);
			short revents = 0;

			if (read_current_user(addr, &pollfd, sizeof(pollfd)) != 0) {
				return (u64)-LINUX_EINVAL;
			}
			if (pollfd.fd >= 0) {
				revents = pollfd.events &
					(LINUX_POLLIN | LINUX_POLLOUT);
			}
			if (revents != 0) {
				ready++;
			}
			if (write_current_user(addr + 6, &revents,
					       sizeof(revents)) != 0) {
				return (u64)-LINUX_EINVAL;
			}
		}
		return ready;
	}
	case LINUX_SYSCALL_WRITEV: {
		struct {
			u64 base;
			u64 len;
		} iov;
		u64 total = 0;

		if (linux == 0 || reply_port == 0 || arg1 == 0 || arg2 > 16) {
			return (u64)-LINUX_EINVAL;
		}
		for (u64 i = 0; i < arg2; i++) {
			if (read_current_user(arg1 + i * sizeof(iov), &iov,
					      sizeof(iov)) != 0) {
				return (u64)-LINUX_EINVAL;
			}
			if (iov.base == 0 && iov.len != 0) {
				return (u64)-LINUX_EINVAL;
			}
			for (u64 offset = 0; offset < iov.len;) {
				const u64 chunk =
					min_u64(iov.len - offset,
						LINUX_MAX_SYSCALL_BUFFER);
				const u64 wrote =
					linux_write_one(linux, reply_port, arg0,
							iov.base + offset,
							chunk);

				if ((i64)wrote < 0) {
					return total != 0 ? total : wrote;
				}
				if (wrote == 0) {
					return total;
				}
				total += wrote;
				offset += wrote;
				if (wrote != chunk) {
					return total;
				}
			}
		}
		return total;
	}
	case LINUX_SYSCALL_EXIT:
		return linux_exit_current(arg0);
	default:
		break;
	}

	if (linux == 0 || reply_port == 0) {
		return (u64)-LINUX_ENOSYS;
	}
	request.reply_port = reply_port;

	switch (number) {
	case LINUX_SYSCALL_READ: {
		struct shared_buffer *buffer;

		if (arg1 == 0 || arg2 > LINUX_MAX_SYSCALL_BUFFER) {
			return (u64)-LINUX_EINVAL;
		}

		buffer = buffer_create(arg2 == 0 ? 1 : arg2);
		if (buffer == 0) {
			return (u64)-LINUX_EINVAL;
		}

		request.type = USER_LINUX_READ;
		request.words[0] = arg0;
		request.words[1] = arg2;
		request.words[2] = 0;
		request.words[3] = 0;
		request.cap_type = IPC_CAP_BUFFER;
		request.cap_rights = TASK_RIGHT_SEND | TASK_RIGHT_DUP;
		request.cap_object = buffer;
		if (ipc_send(linux, &request) != 0 ||
		    ipc_recv(reply_port, &reply) != 0) {
			buffer_release(buffer);
			return (u64)-LINUX_ENOSYS;
		}
		if ((i64)reply.words[0] > 0 &&
		    buffer_read(buffer, 0, (void *)arg1, reply.words[0]) != 0) {
			buffer_release(buffer);
			return (u64)-LINUX_EINVAL;
		}
		buffer_release(buffer);
		return reply.words[0];
	}
	case LINUX_SYSCALL_WRITE: {
		if (arg1 == 0 || arg2 > LINUX_MAX_SYSCALL_BUFFER) {
			return (u64)-LINUX_EINVAL;
		}
		return linux_write_one(linux, reply_port, arg0, arg1, arg2);
	}
	case LINUX_SYSCALL_GETPID:
		request.type = USER_LINUX_GETPID;
		request.words[0] = task_id(task);
		request.words[1] = thread_id(thread_current());
		request.words[2] = 0;
		request.words[3] = 0;
		if (ipc_send(linux, &request) != 0 ||
		    ipc_recv(reply_port, &reply) != 0) {
			return (u64)-LINUX_ENOSYS;
		}
		return reply.words[0];
	case LINUX_SYSCALL_GETTID:
		request.type = USER_LINUX_GETTID;
		request.words[0] = task_id(task);
		request.words[1] = thread_id(thread_current());
		request.words[2] = 0;
		request.words[3] = 0;
		if (ipc_send(linux, &request) != 0 ||
		    ipc_recv(reply_port, &reply) != 0) {
			return (u64)-LINUX_ENOSYS;
		}
		return reply.words[0];
	case LINUX_SYSCALL_GETPPID:
	case LINUX_SYSCALL_GETPGRP:
	case LINUX_SYSCALL_SETSID:
		request.type = number == LINUX_SYSCALL_GETPPID ?
			       USER_LINUX_GETPPID :
			       number == LINUX_SYSCALL_GETPGRP ?
			       USER_LINUX_GETPGRP : USER_LINUX_SETSID;
		request.words[0] = 0;
		request.words[1] = 0;
		request.words[2] = 0;
		request.words[3] = 0;
		if (ipc_send(linux, &request) != 0 ||
		    ipc_recv(reply_port, &reply) != 0) {
			return (u64)-LINUX_ENOSYS;
		}
		return reply.words[0];
	case LINUX_SYSCALL_GETPGID:
	case LINUX_SYSCALL_SETPGID:
		request.type = number == LINUX_SYSCALL_GETPGID ?
			       USER_LINUX_GETPGID : USER_LINUX_SETPGID;
		request.words[0] = arg0;
		request.words[1] = arg1;
		request.words[2] = 0;
		request.words[3] = 0;
		if (ipc_send(linux, &request) != 0 ||
		    ipc_recv(reply_port, &reply) != 0) {
			return (u64)-LINUX_ENOSYS;
		}
		return reply.words[0];
	case LINUX_SYSCALL_IOCTL: {
		struct shared_buffer *buffer = 0;
		u64 output_size = 0;
		u32 value = 0;

		if (arg1 != LINUX_TCGETS && arg1 != LINUX_TCSETS &&
		    arg1 != LINUX_TIOCGPGRP && arg1 != LINUX_TIOCSPGRP &&
		    arg1 != LINUX_TIOCGWINSZ) {
			return (u64)-LINUX_ENOTTY;
		}
		if (arg2 == 0 && arg1 != LINUX_TCSETS) {
			return (u64)-LINUX_EINVAL;
		}
		if (arg1 == LINUX_TIOCSPGRP &&
		    read_current_user(arg2, &value, sizeof(value)) != 0) {
			return (u64)-LINUX_EINVAL;
		}
		if (arg1 == LINUX_TCGETS) {
			output_size = 64;
		} else if (arg1 == LINUX_TIOCGPGRP) {
			output_size = sizeof(value);
		} else if (arg1 == LINUX_TIOCGWINSZ) {
			output_size = 8;
		}
		if (output_size != 0) {
			buffer = buffer_create(output_size);
			if (buffer == 0) {
				return (u64)-LINUX_EINVAL;
			}
		}

		request.type = USER_LINUX_IOCTL;
		request.words[0] = arg0;
		request.words[1] = arg1;
		request.words[2] = value;
		request.words[3] = 0;
		request.cap_type = buffer != 0 ? IPC_CAP_BUFFER : IPC_CAP_NONE;
		request.cap_rights = buffer != 0 ? TASK_RIGHT_SEND : 0;
		request.cap_object = buffer;
		if (ipc_send(linux, &request) != 0 ||
		    ipc_recv(reply_port, &reply) != 0) {
			if (buffer != 0) {
				buffer_release(buffer);
			}
			return (u64)-LINUX_ENOSYS;
		}
		if (reply.words[0] == 0 && buffer != 0 &&
		    buffer_read(buffer, 0, (void *)arg2, output_size) != 0) {
			buffer_release(buffer);
			return (u64)-LINUX_EINVAL;
		}
		if (buffer != 0) {
			buffer_release(buffer);
		}
		return reply.words[0];
	}
	case LINUX_SYSCALL_WAIT4: {
		struct shared_buffer *buffer = 0;

		if (arg1 != 0) {
			buffer = buffer_create(LINUX_WAIT_STATUS_SIZE);
			if (buffer == 0) {
				return (u64)-LINUX_EINVAL;
			}
			request.cap_type = IPC_CAP_BUFFER;
			request.cap_rights = TASK_RIGHT_SEND;
			request.cap_object = buffer;
		}

		request.type = USER_LINUX_WAIT4;
		request.words[0] = arg0;
		request.words[1] = arg2;
		request.words[2] = 0;
		request.words[3] = 0;
		if (ipc_send(linux, &request) != 0 ||
		    ipc_recv(reply_port, &reply) != 0) {
			buffer_release(buffer);
			return (u64)-LINUX_ENOSYS;
		}
		if ((i64)reply.words[0] > 0 && arg1 != 0 &&
		    buffer_read(buffer, 0, (void *)arg1,
				LINUX_WAIT_STATUS_SIZE) != 0) {
			buffer_release(buffer);
			return (u64)-LINUX_EINVAL;
		}
		buffer_release(buffer);
		return reply.words[0];
	}
	case LINUX_SYSCALL_FSTAT: {
		struct shared_buffer *buffer;

		if (arg1 == 0) {
			return (u64)-LINUX_EINVAL;
		}

		buffer = buffer_create(LINUX_STAT_SIZE);
		if (buffer == 0) {
			return (u64)-LINUX_EINVAL;
		}

		request.type = USER_LINUX_FSTAT;
		request.words[0] = arg0;
		request.words[1] = LINUX_STAT_SIZE;
		request.words[2] = 0;
		request.words[3] = 0;
		request.cap_type = IPC_CAP_BUFFER;
		request.cap_rights = TASK_RIGHT_SEND;
		request.cap_object = buffer;
		if (ipc_send(linux, &request) != 0 ||
		    ipc_recv(reply_port, &reply) != 0) {
			buffer_release(buffer);
			return (u64)-LINUX_ENOSYS;
		}
		if (reply.words[0] == 0 &&
		    buffer_read(buffer, 0, (void *)arg1, LINUX_STAT_SIZE) != 0) {
			buffer_release(buffer);
			return (u64)-LINUX_EINVAL;
		}
		buffer_release(buffer);
		return reply.words[0];
	}
	case LINUX_SYSCALL_CLOSE:
		request.type = USER_LINUX_CLOSE;
		request.words[0] = arg0;
		if (ipc_send(linux, &request) != 0 ||
		    ipc_recv(reply_port, &reply) != 0) {
			return (u64)-LINUX_ENOSYS;
		}
		return reply.words[0];
	case LINUX_SYSCALL_FCNTL:
		request.type = USER_LINUX_FCNTL;
		request.words[0] = arg0;
		request.words[1] = arg1;
		request.words[2] = arg2;
		if (ipc_send(linux, &request) != 0 ||
		    ipc_recv(reply_port, &reply) != 0) {
			return (u64)-LINUX_ENOSYS;
		}
		return reply.words[0];
	case LINUX_SYSCALL_OPEN:
	case LINUX_SYSCALL_OPENAT: {
		struct shared_buffer *buffer;
		const char *path = number == LINUX_SYSCALL_OPEN ?
				    (const char *)arg0 : (const char *)arg1;
		const u64 dirfd = number == LINUX_SYSCALL_OPEN ?
				  (u64)-100 : arg0;
		const u64 flags = number == LINUX_SYSCALL_OPEN ? arg1 : arg2;
		u64 len = 0;

		if (path == 0) {
			return (u64)-LINUX_EINVAL;
		}
		while (len < LINUX_MAX_SYSCALL_BUFFER && path[len] != '\0') {
			len++;
		}
		if (len == LINUX_MAX_SYSCALL_BUFFER) {
			return (u64)-LINUX_EINVAL;
		}
		len++;

		buffer = buffer_create(len);
		if (buffer == 0 ||
		    buffer_write(buffer, 0, path, len) != 0) {
			buffer_release(buffer);
			return (u64)-LINUX_EINVAL;
		}

		request.type = USER_LINUX_OPEN;
		request.words[0] = dirfd;
		request.words[1] = len;
		request.words[2] = flags;
		request.words[3] = 0;
		request.cap_type = IPC_CAP_BUFFER;
		request.cap_rights = TASK_RIGHT_RECV;
		request.cap_object = buffer;
		if (ipc_send(linux, &request) != 0 ||
		    ipc_recv(reply_port, &reply) != 0) {
			buffer_release(buffer);
			return (u64)-LINUX_ENOSYS;
		}
		buffer_release(buffer);
		return reply.words[0];
	}
	case LINUX_SYSCALL_STAT:
	case LINUX_SYSCALL_NEWFSTATAT: {
		struct shared_buffer *buffer;
		const char *path = number == LINUX_SYSCALL_STAT ?
				    (const char *)arg0 : (const char *)arg1;
		const u64 dirfd = number == LINUX_SYSCALL_STAT ?
				  (u64)-100 : arg0;
		const u64 stat_addr = number == LINUX_SYSCALL_STAT ?
				      arg1 : arg2;
		u64 packed[2] = { 0, 0 };
		u64 len = 0;

		if (path == 0 || stat_addr == 0) {
			return (u64)-LINUX_EINVAL;
		}
		while (len < sizeof(packed) && path[len] != '\0') {
			const u64 slot = len / 8;
			const u64 shift = (len % 8) * 8;

			packed[slot] |= ((u64)(unsigned char)path[len]) << shift;
			len++;
		}
		if (len == sizeof(packed)) {
			return (u64)-LINUX_EINVAL;
		}

		buffer = buffer_create(LINUX_STAT_SIZE);
		if (buffer == 0) {
			return (u64)-LINUX_EINVAL;
		}

		request.type = USER_LINUX_NEWFSTATAT;
		request.words[0] = dirfd;
		request.words[1] = LINUX_STAT_SIZE;
		request.words[2] = packed[0];
		request.words[3] = packed[1];
		request.cap_type = IPC_CAP_BUFFER;
		request.cap_rights = TASK_RIGHT_SEND;
		request.cap_object = buffer;
		if (ipc_send(linux, &request) != 0 ||
		    ipc_recv(reply_port, &reply) != 0) {
			buffer_release(buffer);
			return (u64)-LINUX_ENOSYS;
		}
		if (reply.words[0] == 0 &&
		    buffer_read(buffer, 0, (void *)stat_addr,
				LINUX_STAT_SIZE) != 0) {
			buffer_release(buffer);
			return (u64)-LINUX_EINVAL;
		}
		buffer_release(buffer);
		return reply.words[0];
	}
	case LINUX_SYSCALL_EXIT_GROUP:
		return linux_exit_current(arg0);
	default:
		console_printf("linux: unknown syscall=%u rip=%p arg0=%p arg1=%p arg2=%p arg3=%p\n",
			       (u32)number, (const void *)frame->user_rip,
			       (const void *)arg0, (const void *)arg1,
			       (const void *)arg2, (const void *)arg3);
		return (u64)-1;
	}
}

static u64 linux_syscall_dispatch(struct arch_syscall_frame *frame)
{
	u64 result;

	linux_strace_enter(frame);
	result = linux_syscall_handle(frame);
	linux_strace_log(frame, result);
	return result;
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

static void arch_enable_sse(void)
{
	u64 cr0;
	u64 cr4;

	__asm__ volatile ("movq %%cr0, %0" : "=r"(cr0));
	cr0 |= 1u << 1;
	cr0 &= ~(1u << 2);
	cr0 &= ~(1u << 3);
	__asm__ volatile ("movq %0, %%cr0" : : "r"(cr0) : "memory");

	__asm__ volatile ("movq %%cr4, %0" : "=r"(cr4));
	cr4 |= (1u << 9) | (1u << 10);
	__asm__ volatile ("movq %0, %%cr4" : : "r"(cr4) : "memory");
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
	arch_enable_sse();

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
	arch_smp_set_syscall_stack(stack);
}

void arch_user_set_fs_base(u64 fs_base)
{
	arch_wrmsr(MSR_FS_BASE, fs_base);
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

u64 arch_syscall_dispatch(struct arch_syscall_frame *frame)
{
	const u64 number = frame->number;
	const u64 arg0 = frame->arg0;
	const u64 arg1 = frame->arg1;
	const u64 arg2 = frame->arg2;

	if ((i64)number >= 0) {
		return linux_syscall_dispatch(frame);
	}

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
	case SYSCALL_LAUNCH_MODULE: {
		char name[LINUX_EXEC_MAX_PATH];
		struct task_launch_cap caps[8];

		if (copy_cstr_from_user(name, (const char *)arg0,
					sizeof(name)) != 0 ||
		    arg2 > sizeof(caps) / sizeof(caps[0])) {
			return (u64)-1;
		}
		if (arg2 != 0 &&
		    (arg1 == 0 ||
		     read_current_user(arg1, caps,
				       arg2 * sizeof(caps[0])) != 0)) {
			return (u64)-1;
		}
		return server_launch_module_with_caps(name, task_current(),
						      arg2 != 0 ? caps : 0,
						      arg2);
	}
	case SYSCALL_TASK_CREATE: {
		char name[LINUX_EXEC_MAX_PATH];
		u64 handle;

		if (copy_cstr_from_user(name, (const char *)arg0,
					sizeof(name)) != 0) {
			return (u64)-1;
		}
		handle = server_task_create(task_current(), name);
		return handle;
	}
	case SYSCALL_TASK_ID: {
		struct task *task =
			task_from_handle(task_current(), arg0, TASK_RIGHT_SEND);

		return task != 0 ? task_id(task) : (u64)-1;
	}
	case SYSCALL_TASK_MAP: {
		u64 args[6];
		u64 flags;
		u64 result;

		if (arg0 == 0 ||
		    read_current_user(arg0, args, sizeof(args)) != 0 ||
		    args[3] > LINUX_MAX_SYSCALL_BUFFER) {
			return (u64)-1;
		}

		flags = spin_lock_irqsave(&syscall_copy_lock);
		if (read_current_user(args[2], syscall_copy_buffer,
				      args[3]) != 0) {
			spin_unlock_irqrestore(&syscall_copy_lock, flags);
			return (u64)-1;
		}
		result = (u64)server_task_map(task_current(), args[0], args[1],
					      syscall_copy_buffer, args[3],
					      args[4], (u32)args[5]);
		spin_unlock_irqrestore(&syscall_copy_lock, flags);
		return result;
	}
	case SYSCALL_TASK_GRANT:
		return (u64)server_task_grant(task_current(), arg0, arg1,
					      (u32)arg2);
	case SYSCALL_TASK_START:
		return (u64)server_task_start(task_current(), arg0, arg1);
	case SYSCALL_TASK_WRITE: {
		u64 args[4];
		u64 flags;
		u64 result;

		if (arg0 == 0 ||
		    read_current_user(arg0, args, sizeof(args)) != 0 ||
		    args[3] > LINUX_MAX_SYSCALL_BUFFER) {
			return (u64)-1;
		}

		flags = spin_lock_irqsave(&syscall_copy_lock);
		if (read_current_user(args[2], syscall_copy_buffer,
				      args[3]) != 0) {
			spin_unlock_irqrestore(&syscall_copy_lock, flags);
			return (u64)-1;
		}
		result = (u64)server_task_write(task_current(), args[0], args[1],
						syscall_copy_buffer, args[3]);
		spin_unlock_irqrestore(&syscall_copy_lock, flags);
		return result;
	}
	case SYSCALL_TASK_ALLOC: {
		u64 args[4];

		if (arg0 == 0 ||
		    read_current_user(arg0, args, sizeof(args)) != 0) {
			return (u64)-1;
		}

		return (u64)server_task_alloc(task_current(), args[0], args[1],
					      args[2], (u32)args[3]);
	}
	case SYSCALL_TASK_CLONE_RANGE: {
		u64 args[5];

		if (arg0 == 0 ||
		    read_current_user(arg0, args, sizeof(args)) != 0) {
			return (u64)-1;
		}

		return (u64)server_task_clone_range(task_current(), args[0],
						    args[1], args[2], args[3],
						    (u32)args[4]);
	}
	case SYSCALL_TASK_START_AT:
		return (u64)server_task_start_at(task_current(), arg0, arg1,
						 arg2);
	case SYSCALL_BUFFER_CREATE: {
		struct shared_buffer *buffer = buffer_create(arg0);
		if (buffer == 0) {
			return (u64)-1;
		}
		const u64 handle =
			task_grant_buffer(task_current(), buffer,
					  TASK_RIGHT_SEND | TASK_RIGHT_RECV |
					  TASK_RIGHT_DUP);

		buffer_release(buffer);
		return handle != 0 ? handle : (u64)-1;
	}
	case SYSCALL_BUFFER_READ: {
		u64 args[4];
		u64 flags;

		if (arg0 == 0 ||
		    read_current_user(arg0, args, sizeof(args)) != 0 ||
		    args[3] > LINUX_MAX_SYSCALL_BUFFER) {
			return (u64)-1;
		}

		struct shared_buffer *buffer =
			task_buffer_from_handle(task_current(), args[0],
						TASK_RIGHT_RECV);
		flags = spin_lock_irqsave(&syscall_copy_lock);
		if (buffer_read(buffer, args[1], syscall_copy_buffer,
				args[3]) != 0 ||
		    write_current_user(args[2], syscall_copy_buffer,
				       args[3]) != 0) {
			spin_unlock_irqrestore(&syscall_copy_lock, flags);
			return (u64)-1;
		}
		spin_unlock_irqrestore(&syscall_copy_lock, flags);
		return 0;
	}
	case SYSCALL_BUFFER_WRITE: {
		u64 args[4];
		u64 flags;
		u64 result;

		if (arg0 == 0 ||
		    read_current_user(arg0, args, sizeof(args)) != 0 ||
		    args[3] > LINUX_MAX_SYSCALL_BUFFER) {
			return (u64)-1;
		}
		flags = spin_lock_irqsave(&syscall_copy_lock);
		if (read_current_user(args[2], syscall_copy_buffer,
				      args[3]) != 0) {
			spin_unlock_irqrestore(&syscall_copy_lock, flags);
			return (u64)-1;
		}

		struct shared_buffer *buffer =
			task_buffer_from_handle(task_current(), args[0],
						TASK_RIGHT_SEND);
		result = (u64)buffer_write(buffer, args[1],
					   syscall_copy_buffer, args[3]);
		spin_unlock_irqrestore(&syscall_copy_lock, flags);
		return result;
	}
	case SYSCALL_PORT_CREATE: {
		char name[LINUX_EXEC_MAX_PATH];

		if (copy_cstr_from_user(name, (const char *)arg0,
					sizeof(name)) != 0) {
			return (u64)-1;
		}
		return task_grant_port(task_current(),
				       ipc_port_create_private(name),
				       TASK_RIGHT_SEND | TASK_RIGHT_RECV |
				       TASK_RIGHT_DUP);
	}
	case SYSCALL_HANDLE_CLOSE:
		return (u64)task_close_handle(task_current(), arg0);
	case SYSCALL_BOOT_MODULE_READ: {
		u64 flags;

		if (arg1 == 0) {
			return server_boot_module_size();
		}
		if (arg2 > LINUX_MAX_SYSCALL_BUFFER) {
			return (u64)-1;
		}
		flags = spin_lock_irqsave(&syscall_copy_lock);
		if (server_boot_module_read(arg0, syscall_copy_buffer, arg2) != 0 ||
		    write_current_user(arg1, syscall_copy_buffer, arg2) != 0) {
			spin_unlock_irqrestore(&syscall_copy_lock, flags);
			return (u64)-1;
		}
		spin_unlock_irqrestore(&syscall_copy_lock, flags);
		return 0;
	}
	case SYSCALL_CONSOLE_READ: {
		u64 nread;
		u64 flags;

		if (arg0 == 0 || arg1 > LINUX_MAX_SYSCALL_BUFFER) {
			return (u64)-1;
		}

		nread = console_read_line((char *)console_input_buffer, arg1);
		flags = spin_lock_irqsave(&syscall_copy_lock);
		if (write_current_user(arg0, console_input_buffer, nread) != 0) {
			spin_unlock_irqrestore(&syscall_copy_lock, flags);
			return (u64)-1;
		}
		spin_unlock_irqrestore(&syscall_copy_lock, flags);
		return nread;
	}
	case SYSCALL_IPC_SEND: {
		struct user_ipc_message user_message;
		struct ipc_port *port =
			task_port_from_handle(task_current(), arg0,
					      TASK_RIGHT_SEND);

		if (arg1 == 0 ||
		    read_current_user(arg1, &user_message,
				      sizeof(user_message)) != 0) {
			return (u64)-1;
		}

		if (port != 0 &&
		    user_message.protocol == USER_FOURCC_CONS &&
		    str_eq(ipc_port_name(port), "console")) {
			if (user_message.type == USER_CONSOLE_WRITE) {
				return (u64)console_write_user(
					user_message.words[0],
					user_message.words[1]);
			}
			if (user_message.type == USER_CONSOLE_LOG) {
				return (u64)console_log_user(
					user_message.words[0],
					user_message.words[1]);
			}
			if (user_message.type == USER_CONSOLE_LOGS_TO_RING) {
				console_logs_to_ring();
				return 0;
			}
		}

		struct ipc_message message = {
			.protocol = 0,
			.type = 0,
		};

		if (user_message_to_ipc(&user_message, &message) != 0) {
			return (u64)-1;
		}

		return (u64)ipc_send(port, &message);
	}
	case SYSCALL_IPC_RECV: {
		struct user_ipc_message user_message;
		struct ipc_port *port =
			task_port_from_handle(task_current(), arg0,
					      TASK_RIGHT_RECV);
		struct ipc_message message;

		if (arg1 == 0 ||
		    ipc_recv(port, &message) != 0) {
			return (u64)-1;
		}

		ipc_message_to_user(&message, &user_message);
		if (write_current_user(arg1, &user_message,
				       sizeof(user_message)) != 0) {
			return (u64)-1;
		}
		return 0;
	}
	case SYSCALL_IPC_CALL: {
		struct user_ipc_message user_request;
		struct user_ipc_message user_reply;
		struct ipc_port *port =
			task_port_from_handle(task_current(), arg0,
					      TASK_RIGHT_SEND);
		struct ipc_port *reply_port = task_reply_port(task_current());
		struct ipc_message message = { .protocol = 0, .type = 0 };
		struct ipc_message reply;

		if (arg1 == 0 || arg2 == 0 || port == 0 || reply_port == 0 ||
		    read_current_user(arg1, &user_request,
				      sizeof(user_request)) != 0) {
			return (u64)-1;
		}

		if (user_message_to_ipc(&user_request, &message) != 0) {
			return (u64)-1;
		}

		message.reply_port = reply_port;
		if (ipc_send(port, &message) != 0 ||
		    ipc_recv(reply_port, &reply) != 0) {
			return (u64)-1;
		}

		ipc_message_to_user(&reply, &user_reply);
		if (write_current_user(arg2, &user_reply,
				       sizeof(user_reply)) != 0) {
			return (u64)-1;
		}
		return 0;
	}
	default:
		console_printf("syscall: unknown number=%u\n", (u32)number);
		return (u64)-1;
	}
}
