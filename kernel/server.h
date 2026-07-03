#ifndef BUNIXOS_SERVER_H
#define BUNIXOS_SERVER_H

#include "types.h"

typedef void (*server_entry_t)(void);

struct task;
struct arch_syscall_frame;

struct task_launch_cap {
	u64 handle;
	u32 rights;
	u32 reserved;
};

struct server {
	const char *name;
	server_entry_t start;
};

void server_start_all(void);
void server_start_boot_modules(u64 multiboot_info);
int server_launch_module(const char *name);
u64 server_launch_module_with_caps(const char *name, struct task *parent,
				   const struct task_launch_cap *caps,
				   u64 cap_count);
u64 server_task_create(struct task *parent, const char *name);
int server_task_map(struct task *parent, u64 task_handle, u64 vaddr,
		    const void *src, u64 filesz, u64 memsz, u32 writable);
int server_task_write(struct task *parent, u64 task_handle, u64 vaddr,
		      const void *src, u64 len);
int server_task_alloc(struct task *parent, u64 task_handle, u64 vaddr,
		      u64 len, u32 writable);
int server_task_clone_range(struct task *parent, u64 dst_handle,
			    u64 src_handle, u64 vaddr, u64 len, u32 writable);
int server_task_grant(struct task *parent, u64 task_handle, u64 handle,
		      u32 rights);
int server_task_start(struct task *parent, u64 task_handle, u64 entry);
int server_task_start_at(struct task *parent, u64 task_handle, u64 entry,
			 u64 stack);
struct task *server_task_fork_current(const struct arch_syscall_frame *frame);
u64 server_boot_module_size(void);
int server_boot_module_read(u64 offset, void *buffer, u64 len);
void vm_server_start(void);

#endif
