#ifndef BUNIXOS_SCHED_H
#define BUNIXOS_SCHED_H

#include "types.h"

typedef void (*thread_entry_t)(void *arg);

enum thread_state {
	THREAD_EMPTY = 0,
	THREAD_READY,
	THREAD_RUNNING,
	THREAD_BLOCKED,
	THREAD_DEAD,
};

struct task;
struct thread;
struct vm_space;
struct ipc_port;
struct shared_buffer;

enum task_cap_type {
	TASK_CAP_NONE = 0,
	TASK_CAP_PORT,
	TASK_CAP_BUFFER,
};

enum task_handle_rights {
	TASK_RIGHT_SEND = 1 << 0,
	TASK_RIGHT_RECV = 1 << 1,
	TASK_RIGHT_DUP = 1 << 2,
};

enum task_vm_region_kind {
	TASK_VM_REGION_ELF = 1,
	TASK_VM_REGION_STACK,
	TASK_VM_REGION_BRK,
	TASK_VM_REGION_MMAP,
};

struct task_vm_region {
	u64 base;
	u64 len;
	u32 writable;
	u32 kind;
};

void sched_init(void);
u32 sched_current_cpu_id(void);
void sched_secondary_start(u32 cpu_id) __attribute__((noreturn));
struct task *task_create(const char *name, struct vm_space *vm_space);
struct vm_space *task_vm_space(struct task *task);
struct thread *thread_create(struct task *task, const char *name,
			     thread_entry_t entry, void *arg);
struct thread *thread_create_on_cpu(struct task *task, const char *name,
				    thread_entry_t entry, void *arg,
				    u32 cpu_id);
struct task *task_current(void);
struct thread *thread_current(void);
u64 task_grant_port(struct task *task, struct ipc_port *port, u32 rights);
u64 task_grant_buffer(struct task *task, struct shared_buffer *buffer,
		      u32 rights);
u64 task_grant_task(struct task *owner, struct task *target, u32 rights);
struct task *task_from_handle(struct task *owner, u64 handle, u32 rights);
int task_can_inherit_handle(struct task *src, u64 handle, u32 rights);
u64 task_grant_inherited_handle(struct task *dst, struct task *src, u64 handle,
				u32 rights);
int task_export_cap(struct task *task, u64 handle, u32 rights,
		    enum task_cap_type *type, void **object);
struct ipc_port *task_port_from_handle(struct task *task, u64 handle,
				       u32 rights);
struct shared_buffer *task_buffer_from_handle(struct task *task, u64 handle,
					      u32 rights);
int task_close_handle(struct task *task, u64 handle);
struct ipc_port *task_reply_port(struct task *task);
void sched_run(void);
void sched_idle_loop(void) __attribute__((noreturn));
void thread_yield(void);
void thread_prepare_block(void);
void thread_block_prepared(void);
void thread_block(void);
void thread_sleep_ticks(u64 ticks);
void thread_sleep_ns(u64 ns);
void thread_unblock(struct thread *thread);
void sched_wake_sleepers(u64 now);
void sched_enable_preemption(void);
void sched_tick(void);
void thread_exit(void) __attribute__((noreturn));

u32 task_id(const struct task *task);
u64 task_linux_brk(const struct task *task);
void task_set_linux_brk(struct task *task, u64 brk);
u64 task_linux_mmap_next(const struct task *task);
void task_set_linux_mmap_next(struct task *task, u64 next);
int task_add_vm_region(struct task *task, u64 base, u64 len, u32 writable,
		       u32 kind);
int task_add_or_extend_vm_region(struct task *task, u64 base, u64 len,
				 u32 writable, u32 kind);
int task_remove_vm_region(struct task *task, u64 base, u64 len);
u64 task_vm_region_count(const struct task *task);
const struct task_vm_region *task_vm_region_at(const struct task *task,
					       u64 index);
u32 thread_id(const struct thread *thread);
enum thread_state thread_state(const struct thread *thread);
const char *task_name(const struct task *task);
const char *thread_name(const struct thread *thread);

#endif
