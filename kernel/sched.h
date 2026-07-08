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

enum sched_class {
	SCHED_CLASS_KERNEL = 1,
	SCHED_CLASS_SERVER = 2,
	SCHED_CLASS_USER = 3,
	SCHED_CLASS_BATCH = 4,
	SCHED_CLASS_IDLE = 5,
};

enum sched_policy_rights {
	SCHED_POLICY_RIGHT_CLASS = 1 << 0,
	SCHED_POLICY_RIGHT_PRIORITY = 1 << 1,
	SCHED_POLICY_RIGHT_WEIGHT = 1 << 2,
};

struct task;
struct thread;
struct vm_space;
struct ipc_port;
struct shared_buffer;
struct task_hw_resource;

enum task_cap_type {
	TASK_CAP_NONE = 0,
	TASK_CAP_PORT,
	TASK_CAP_BUFFER,
	TASK_CAP_TASK,
	TASK_CAP_HW_RESOURCE,
};

enum task_handle_rights {
	TASK_RIGHT_SEND = 1 << 0,
	TASK_RIGHT_RECV = 1 << 1,
	TASK_RIGHT_DUP = 1 << 2,
};

enum task_hw_resource_type {
	TASK_HW_RESOURCE_PORT = 1,
	TASK_HW_RESOURCE_MMIO = 2,
	TASK_HW_RESOURCE_IRQ = 3,
};

enum task_hw_resource_ops {
	TASK_HW_OP_READ = 1 << 0,
	TASK_HW_OP_WRITE = 1 << 1,
	TASK_HW_OP_BIND_IRQ = 1 << 2,
	TASK_HW_OP_ACK_IRQ = 1 << 3,
	TASK_HW_OP_MASK_IRQ = 1 << 4,
};

enum task_hw_resource_flags {
	TASK_HW_RESOURCE_OWNED = 1 << 0,
};

struct task_hw_resource {
	u32 type;
	u32 ops;
	u32 flags;
	u32 ref_count;
	u64 base;
	u64 len;
};

enum {
	SCHED_STATS_CPUS = 8,
};

struct sched_stats {
	u64 enqueues;
	u64 switches;
	u64 wakeups;
	u64 preemptions;
	u64 migrations;
	u64 runtime_ticks;
	u64 wait_ticks;
	u64 max_wait_ticks;
	u64 wake_to_run_ticks;
	u64 max_wake_to_run_ticks;
	u64 cpu_enqueues[SCHED_STATS_CPUS];
	u64 cpu_switches[SCHED_STATS_CPUS];
	u64 cpu_wakeups[SCHED_STATS_CPUS];
	u64 cpu_preemptions[SCHED_STATS_CPUS];
	u64 cpu_migrations[SCHED_STATS_CPUS];
	u64 cpu_runtime_ticks[SCHED_STATS_CPUS];
	u64 cpu_wait_ticks[SCHED_STATS_CPUS];
	u64 cpu_max_wait_ticks[SCHED_STATS_CPUS];
	u64 cpu_wake_to_run_ticks[SCHED_STATS_CPUS];
	u64 cpu_max_wake_to_run_ticks[SCHED_STATS_CPUS];
	u64 cpu_runq_load[SCHED_STATS_CPUS];
	u64 cpu_min_vruntime[SCHED_STATS_CPUS];
};

enum task_vm_region_kind {
	TASK_VM_REGION_ELF = 1,
	TASK_VM_REGION_STACK,
	TASK_VM_REGION_BRK,
	TASK_VM_REGION_MMAP,
};

enum task_vm_prot {
	TASK_VM_PROT_READ = 1 << 0,
	TASK_VM_PROT_WRITE = 1 << 1,
	TASK_VM_PROT_EXEC = 1 << 2,
};

enum task_vm_map_flags {
	TASK_VM_MAP_PRIVATE = 1 << 0,
	TASK_VM_MAP_ANONYMOUS = 1 << 1,
	TASK_VM_MAP_FIXED = 1 << 2,
};

enum task_vm_object_type {
	TASK_VM_OBJECT_NONE = 0,
	TASK_VM_OBJECT_ANON,
	TASK_VM_OBJECT_FILE,
};

enum thread_handoff_result {
	THREAD_HANDOFF_INVALID = -1,
	THREAD_HANDOFF_DIRECT = 1,
	THREAD_HANDOFF_SCHEDULER = 2,
	THREAD_HANDOFF_CROSS_CPU = 3,
	THREAD_HANDOFF_NESTED = 4,
};

struct task_vm_region {
	u64 base;
	u64 len;
	u64 offset;
	u32 writable;
	u32 kind;
	u32 prot;
	u32 flags;
	u32 object_type;
	u32 object_id;
};

void sched_init(void);
u32 sched_current_cpu_id(void);
void sched_secondary_start(u32 cpu_id) __attribute__((noreturn));
struct task *task_create(const char *name, struct vm_space *vm_space);
struct vm_space *task_vm_space(struct task *task);
void task_set_ipc_affinity(struct task *task, u32 cpu_id);
void task_set_sched_policy(struct task *task, enum sched_class sched_class,
			   u32 priority, u32 rights);
void task_inherit_sched_policy(struct task *dst, const struct task *src);
struct thread *thread_create(struct task *task, const char *name,
			     thread_entry_t entry, void *arg);
struct thread *thread_create_on_cpu(struct task *task, const char *name,
				    thread_entry_t entry, void *arg,
				    u32 cpu_id);
struct task *task_current(void);
struct thread *thread_current(void);
int task_kill(struct task *task);
int task_is_killing(const struct task *task);
enum sched_class task_sched_class(const struct task *task);
u64 task_grant_port(struct task *task, struct ipc_port *port, u32 rights);
u64 task_grant_buffer(struct task *task, struct shared_buffer *buffer,
		      u32 rights);
u64 task_grant_task(struct task *owner, struct task *target, u32 rights);
u64 task_grant_hw_resource(struct task *task,
			   const struct task_hw_resource *resource,
			   u32 rights);
int task_hw_resource_retain(const struct task_hw_resource *resource);
void task_hw_resource_release(const struct task_hw_resource *resource);
int task_clone_handles(struct task *dst, struct task *src);
struct task *task_from_handle(struct task *owner, u64 handle, u32 rights);
int task_can_inherit_handle(struct task *src, u64 handle, u32 rights);
u64 task_grant_inherited_handle(struct task *dst, struct task *src, u64 handle,
				u32 rights);
int task_export_cap(struct task *task, u64 handle, u32 rights,
		    enum task_cap_type *type, void **object);
int task_retain(struct task *task);
void task_release(struct task *task);
struct ipc_port *task_port_from_handle(struct task *task, u64 handle,
				       u32 rights);
struct shared_buffer *task_buffer_from_handle(struct task *task, u64 handle,
					      u32 rights);
const struct task_hw_resource *task_hw_resource_from_handle(struct task *task,
							    u64 handle,
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
int thread_handoff(struct thread *thread);
void sched_wake_sleepers(u64 now);
void sched_enable_preemption(void);
void sched_tick(void);
void sched_stats_snapshot(struct sched_stats *stats);
void thread_exit(void) __attribute__((noreturn));

u32 task_id(const struct task *task);
int task_info_at(u64 index, u64 *pid_threads_flags, u64 *name_words);
u64 task_linux_brk(const struct task *task);
void task_set_linux_brk(struct task *task, u64 brk);
u64 task_linux_mmap_next(const struct task *task);
void task_set_linux_mmap_next(struct task *task, u64 next);
u64 task_linux_fs_base(const struct task *task);
void task_set_linux_fs_base(struct task *task, u64 fs_base);
int task_add_vm_region(struct task *task, u64 base, u64 len, u32 writable,
		       u32 kind);
int task_add_or_extend_vm_region(struct task *task, u64 base, u64 len,
				 u32 writable, u32 kind);
int task_add_vm_mapping(struct task *task, u64 base, u64 len, u32 prot,
			u32 map_flags, u32 kind, u32 object_type,
			u32 object_id, u64 offset);
int task_add_or_extend_vm_mapping(struct task *task, u64 base, u64 len,
				  u32 prot, u32 map_flags, u32 kind,
				  u32 object_type, u32 object_id, u64 offset);
int task_vm_range_is_free(struct task *task, u64 base, u64 len);
int task_protect_vm_region(struct task *task, u64 base, u64 len, u32 prot);
int task_remove_vm_region(struct task *task, u64 base, u64 len);
void task_clear_vm_regions(struct task *task);
u64 task_vm_region_count(const struct task *task);
const struct task_vm_region *task_vm_region_at(const struct task *task,
					       u64 index);
u32 thread_id(const struct thread *thread);
enum thread_state thread_state(const struct thread *thread);
const char *task_name(const struct task *task);
const char *thread_name(const struct thread *thread);

#endif
