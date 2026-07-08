#ifndef BUNIX_LINUX_ABI_H
#define BUNIX_LINUX_ABI_H

#include "sched.h"
#include "types.h"

enum {
	LINUX_PROT_READ = 0x1,
	LINUX_PROT_WRITE = 0x2,
	LINUX_PROT_EXEC = 0x4,
};

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
