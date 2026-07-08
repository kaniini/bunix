#ifndef BUNIX_LINUX_ABI_H
#define BUNIX_LINUX_ABI_H

#include "sched.h"
#include "types.h"

enum {
	LINUX_PROT_READ = 0x1,
	LINUX_PROT_WRITE = 0x2,
	LINUX_PROT_EXEC = 0x4,
};

enum {
	LINUX_MSG_READ = 0,
	LINUX_MSG_WRITE = 1,
	LINUX_MSG_CLOSE = 3,
	LINUX_MSG_FSTAT = 5,
	LINUX_MSG_MMAP = 9,
	LINUX_MSG_GETPID = 39,
	LINUX_MSG_GETUID = 102,
	LINUX_MSG_GETGID = 104,
	LINUX_MSG_GETEUID = 107,
	LINUX_MSG_GETEGID = 108,
	LINUX_MSG_GETPPID = 110,
	LINUX_MSG_GETTID = 186,
	LINUX_MSG_SET_TID_ADDRESS = 218,
	LINUX_MSG_EXIT_GROUP = 231,
	LINUX_MSG_OPENAT = 257,
	LINUX_MSG_NEWFSTATAT = 262,
	LINUX_MSG_READLINKAT = 267,
	LINUX_MSG_REGISTER_PROCESS = 1000,
};

enum linux_forward_op {
	LINUX_FORWARD_OPENAT,
	LINUX_FORWARD_CLOSE,
	LINUX_FORWARD_READ,
	LINUX_FORWARD_WRITE,
	LINUX_FORWARD_READLINKAT,
	LINUX_FORWARD_NEWFSTATAT,
	LINUX_FORWARD_FSTAT,
	LINUX_FORWARD_EXIT,
	LINUX_FORWARD_SET_TID_ADDRESS,
	LINUX_FORWARD_SCALAR,
	LINUX_FORWARD_BRK,
	LINUX_FORWARD_MUNMAP,
	LINUX_FORWARD_MMAP,
	LINUX_FORWARD_MPROTECT,
};

struct linux_syscall_forward_entry {
	u64 number;
	enum linux_forward_op op;
	u32 message_type;
};

static inline const struct linux_syscall_forward_entry *
linux_syscall_forward_lookup(const struct linux_syscall_forward_entry *entries,
			     u64 count, u64 number)
{
	for (u64 i = 0; i < count; i++) {
		if (entries[i].number == number) {
			return &entries[i];
		}
	}
	return 0;
}

static inline u32 linux_prot_to_task(u64 prot)
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

#endif
