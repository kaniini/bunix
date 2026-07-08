#include "buffer.h"
#include "console.h"
#include "ipc.h"
#include "sched.h"
#include "slab.h"
#include "spinlock.h"
#include "timer.h"
#include "tree.h"
#include "vm.h"
#include <arch/interrupts.h>
#include <arch/smp.h>
#include <arch/thread.h>
#include <arch/user.h>

enum {
	MAX_CPUS = 8,
	INITIAL_TASK_HANDLES = 16,
	INITIAL_TASK_VM_REGIONS = 32,
	KERNEL_STACK_SIZE = 32768,
	SCHED_QUANTUM_TICKS = 5,
	SCHED_BASE_WEIGHT = 1024,
	SCHED_BASE_SLICE_TICKS = 5,
	SCHED_WAKE_BONUS_MAX_TICKS = 20,
	TASK_NAME_MAX = 64,
};

enum task_handle_type {
	TASK_HANDLE_EMPTY = 0,
	TASK_HANDLE_PORT,
	TASK_HANDLE_TASK,
	TASK_HANDLE_BUFFER,
	TASK_HANDLE_HW_RESOURCE,
};

struct task_handle {
	enum task_handle_type type;
	u32 rights;
	void *object;
};

struct task {
	struct u64_tree_node pid_node;
	u32 pid;
	const char *name;
	u32 ref_count;
	u32 dead;
	u32 killing;
	u32 thread_count;
	struct thread *threads;
	struct vm_space *vm_space;
	u64 linux_brk;
	u64 linux_mmap_next;
	u64 linux_fs_base;
	u32 ipc_affinity_cpu;
	u32 ipc_affinity_valid;
	enum sched_class sched_class;
	u32 sched_priority;
	u32 sched_rights;
	struct task_vm_region *vm_regions;
	u32 vm_region_count;
	u32 vm_region_capacity;
	struct ipc_port *reply_port;
	struct task_handle *handles;
	u32 handle_capacity;
	struct spinlock lock;
};

struct thread {
	u32 tid;
	const char *name;
	struct task *task;
	enum thread_state state;
	thread_entry_t entry;
	void *arg;
	u32 cpu_id;
	u32 weight;
	enum sched_class sched_class;
	u32 sched_priority;
	struct thread *run_next;
	struct thread *sleep_next;
	struct thread *task_next;
	u64 wake_tick;
	u64 vruntime;
	u64 virtual_deadline;
	u64 eligible_time;
	u64 enqueue_seq;
	u64 enqueue_tick;
	u64 exec_start_tick;
	u64 blocked_since_tick;
	u64 wake_ready_tick;
	u64 total_runtime_ticks;
	u64 wakeups;
	u64 migrations;
	u64 preemptions;
	struct arch_thread_context context;
	u8 kernel_stack[KERNEL_STACK_SIZE] __attribute__((aligned(16)));
	u8 trap_stack[KERNEL_STACK_SIZE] __attribute__((aligned(16)));
};

struct run_queue {
	struct thread *head;
	struct thread *tail;
	u32 count;
	u32 idle;
	u64 min_vruntime;
	u64 enqueue_seq;
	struct spinlock lock;
};

struct cpu_sched {
	u32 id;
	struct run_queue runq;
	struct thread *current;
	struct thread *reap;
	struct thread scheduler_thread;
	u32 quantum_left;
	u32 handoff_depth;
};

static struct u64_tree tasks_by_pid;
static struct task kernel_task;
static struct cpu_sched cpus[MAX_CPUS];
static u32 boot_cpu_id;
static u32 next_pid = 1;
static u32 next_tid = 1;
static u32 preemption_enabled;
static u32 sched_cpu_count = 1;
static u32 next_auto_cpu;
static struct thread *sleep_list;
static struct spinlock task_table_lock = SPINLOCK_INIT("task-table");
static struct spinlock thread_table_lock = SPINLOCK_INIT("thread-table");
static struct spinlock placement_lock = SPINLOCK_INIT("sched-placement");
static struct spinlock sleep_lock = SPINLOCK_INIT("sched-sleep");
static struct spinlock sched_stats_lock = SPINLOCK_INIT("sched-stats");
static struct sched_stats sched_counters;

static int task_handle_retain(enum task_handle_type type, void *object);
static int task_handle_retain_from_locked_task(struct task *locked_task,
					       enum task_handle_type type,
					       void *object);
static void task_handle_release(enum task_handle_type type, void *object);
static char *task_name_copy(const char *name);

static void mem_copy(void *dst, const void *src, u64 len)
{
	u8 *out = (u8 *)dst;
	const u8 *in = (const u8 *)src;

	for (u64 i = 0; i < len; i++) {
		out[i] = in[i];
	}
}

static int task_grow_handles_locked(struct task *task, u32 min_capacity)
{
	u32 old_capacity;
	u32 new_capacity;
	struct task_handle *handles;

	if (task == 0) {
		return -1;
	}
	if (task->handle_capacity >= min_capacity) {
		return 0;
	}

	old_capacity = task->handle_capacity;
	new_capacity = old_capacity == 0 ? INITIAL_TASK_HANDLES : old_capacity;
	while (new_capacity < min_capacity) {
		new_capacity *= 2;
	}

	handles = slab_zalloc((u64)new_capacity * sizeof(handles[0]));
	if (handles == 0) {
		return -1;
	}
	if (task->handles != 0) {
		mem_copy(handles, task->handles,
			 (u64)old_capacity * sizeof(handles[0]));
		slab_free(task->handles);
	}
	task->handles = handles;
	task->handle_capacity = new_capacity;
	return 0;
}

static int task_grow_vm_regions_locked(struct task *task, u32 min_capacity)
{
	u32 old_capacity;
	u32 new_capacity;
	struct task_vm_region *regions;

	if (task == 0) {
		return -1;
	}
	if (task->vm_region_capacity >= min_capacity) {
		return 0;
	}

	old_capacity = task->vm_region_capacity;
	new_capacity = old_capacity == 0 ? INITIAL_TASK_VM_REGIONS :
		       old_capacity;
	while (new_capacity < min_capacity) {
		new_capacity *= 2;
	}

	regions = slab_zalloc((u64)new_capacity * sizeof(regions[0]));
	if (regions == 0) {
		return -1;
	}
	if (task->vm_regions != 0) {
		mem_copy(regions, task->vm_regions,
			 (u64)old_capacity * sizeof(regions[0]));
		slab_free(task->vm_regions);
	}
	task->vm_regions = regions;
	task->vm_region_capacity = new_capacity;
	return 0;
}

static int task_append_vm_region_locked(struct task *task,
					struct task_vm_region region);

static struct cpu_sched *sched_current_cpu(void)
{
	return &cpus[arch_smp_current_cpu_id()];
}

static void runq_push(struct run_queue *runq, struct thread *thread)
{
	struct thread **link;

	thread->run_next = 0;
	link = &runq->head;
	while (*link != 0) {
		struct thread *queued = *link;
		const int after =
			thread->virtual_deadline > queued->virtual_deadline ||
			(thread->virtual_deadline == queued->virtual_deadline &&
			 thread->eligible_time > queued->eligible_time) ||
			(thread->virtual_deadline == queued->virtual_deadline &&
			 thread->eligible_time == queued->eligible_time &&
			 thread->enqueue_seq > queued->enqueue_seq) ||
			(thread->virtual_deadline == queued->virtual_deadline &&
			 thread->eligible_time == queued->eligible_time &&
			 thread->enqueue_seq == queued->enqueue_seq &&
			 thread->tid > queued->tid);

		if (!after) {
			break;
		}
		link = &queued->run_next;
	}
	thread->run_next = *link;
	*link = thread;
	if (thread->run_next == 0) {
		runq->tail = thread;
	}
	if (runq->tail == 0) {
		runq->tail = thread;
	}
	runq->count++;
}

static u64 sched_scale_delta(u64 delta, u32 weight)
{
	if (weight == 0) {
		weight = SCHED_BASE_WEIGHT;
	}
	return (delta * SCHED_BASE_WEIGHT + weight - 1) / weight;
}

static void sched_refresh_deadline(struct thread *thread)
{
	const u64 slice = sched_scale_delta(SCHED_BASE_SLICE_TICKS,
					    thread->weight);

	thread->eligible_time = thread->vruntime;
	thread->virtual_deadline = thread->vruntime + (slice != 0 ? slice : 1);
}

static void sched_stats_add_cpu(u64 *global, u64 cpu_values[SCHED_STATS_CPUS],
				u32 cpu_id, u64 value)
{
	const u64 flags = spin_lock_irqsave(&sched_stats_lock);

	*global += value;
	if (cpu_id < SCHED_STATS_CPUS) {
		cpu_values[cpu_id] += value;
	}
	spin_unlock_irqrestore(&sched_stats_lock, flags);
}

static void sched_stats_max_cpu(u64 *global, u64 cpu_values[SCHED_STATS_CPUS],
				u32 cpu_id, u64 value)
{
	const u64 flags = spin_lock_irqsave(&sched_stats_lock);

	if (value > *global) {
		*global = value;
	}
	if (cpu_id < SCHED_STATS_CPUS && value > cpu_values[cpu_id]) {
		cpu_values[cpu_id] = value;
	}
	spin_unlock_irqrestore(&sched_stats_lock, flags);
}

static void sched_account_runtime(struct cpu_sched *cpu)
{
	struct thread *thread = cpu->current;
	const u64 now = timer_ticks();
	u64 delta;

	if (thread == 0 || thread == &cpu->scheduler_thread ||
	    thread->state != THREAD_RUNNING) {
		return;
	}
	if (thread->exec_start_tick == 0) {
		thread->exec_start_tick = now;
		return;
	}
	if (now <= thread->exec_start_tick) {
		return;
	}

	delta = now - thread->exec_start_tick;
	thread->exec_start_tick = now;
	thread->total_runtime_ticks += delta;
	sched_stats_add_cpu(&sched_counters.runtime_ticks,
			    sched_counters.cpu_runtime_ticks, cpu->id, delta);
	thread->vruntime += sched_scale_delta(delta, thread->weight);
	if (thread->vruntime > cpu->runq.min_vruntime) {
		cpu->runq.min_vruntime = thread->vruntime;
	}
	sched_refresh_deadline(thread);
}

static void sched_note_switch(struct cpu_sched *cpu, struct thread *next)
{
	const u64 now = timer_ticks();

	if (next == 0 || next == &cpu->scheduler_thread) {
		return;
	}

	sched_stats_add_cpu(&sched_counters.switches,
			    sched_counters.cpu_switches, cpu->id, 1);
	if (next->enqueue_tick != 0 && now >= next->enqueue_tick) {
		const u64 wait = now - next->enqueue_tick;

		sched_stats_add_cpu(&sched_counters.wait_ticks,
				    sched_counters.cpu_wait_ticks,
				    cpu->id, wait);
		sched_stats_max_cpu(&sched_counters.max_wait_ticks,
				    sched_counters.cpu_max_wait_ticks,
				    cpu->id, wait);
	}
	if (next->wake_ready_tick != 0 && now >= next->wake_ready_tick) {
		const u64 wake_to_run = now - next->wake_ready_tick;

		sched_stats_add_cpu(&sched_counters.wake_to_run_ticks,
				    sched_counters.cpu_wake_to_run_ticks,
				    cpu->id, wake_to_run);
		sched_stats_max_cpu(&sched_counters.max_wake_to_run_ticks,
				    sched_counters.cpu_max_wake_to_run_ticks,
				    cpu->id, wake_to_run);
	}

	next->enqueue_tick = 0;
	next->wake_ready_tick = 0;
	next->exec_start_tick = now;
}

static void sched_prepare_enqueue_locked(struct cpu_sched *cpu,
					 struct thread *thread,
					 int wakeup)
{
	const u64 now = timer_ticks();

	if (thread->weight == 0) {
		thread->weight = SCHED_BASE_WEIGHT;
	}
	if (thread->vruntime < cpu->runq.min_vruntime) {
		thread->vruntime = cpu->runq.min_vruntime;
	}
	if (wakeup) {
		u64 slept = thread->blocked_since_tick != 0 &&
			    now > thread->blocked_since_tick ?
			    now - thread->blocked_since_tick : 0;

		if (slept > SCHED_WAKE_BONUS_MAX_TICKS) {
			slept = SCHED_WAKE_BONUS_MAX_TICKS;
		}
		if (slept > 0) {
			const u64 credit = sched_scale_delta(slept,
							     thread->weight);

			if (thread->vruntime > cpu->runq.min_vruntime + credit) {
				thread->vruntime -= credit;
			} else {
				thread->vruntime = cpu->runq.min_vruntime;
			}
		}
		thread->wakeups++;
		thread->wake_ready_tick = now;
		thread->blocked_since_tick = 0;
	}
	thread->enqueue_seq = ++cpu->runq.enqueue_seq;
	thread->enqueue_tick = now;
	sched_refresh_deadline(thread);
}

static struct thread *runq_pop(struct run_queue *runq)
{
	struct thread *thread = runq->head;

	if (thread == 0) {
		return 0;
	}

	runq->head = thread->run_next;
	if (runq->head == 0) {
		runq->tail = 0;
	}

	thread->run_next = 0;
	runq->count--;
	if (thread->vruntime > runq->min_vruntime) {
		runq->min_vruntime = thread->vruntime;
	}
	return thread;
}

static int runq_remove(struct run_queue *runq, struct thread *thread)
{
	struct thread **link;

	if (runq == 0 || thread == 0) {
		return 0;
	}
	link = &runq->head;
	while (*link != 0) {
		if (*link == thread) {
			*link = thread->run_next;
			if (runq->tail == thread) {
				runq->tail = 0;
				for (struct thread *scan = runq->head; scan != 0;
				     scan = scan->run_next) {
					runq->tail = scan;
				}
			}
			thread->run_next = 0;
			if (runq->count > 0) {
				runq->count--;
			}
			return 1;
		}
		link = &(*link)->run_next;
	}
	return 0;
}

static void sched_enqueue_on_reason(struct cpu_sched *cpu, struct thread *thread,
				    int wakeup)
{
	const u32 remote = cpu->id != sched_current_cpu_id();
	const u64 flags = spin_lock_irqsave(&cpu->runq.lock);
	const u32 was_idle = cpu->runq.idle;
	const u32 old_cpu = thread->cpu_id;
	const enum thread_state old_state = thread->state;

	thread->cpu_id = cpu->id;
	thread->state = THREAD_READY;
	if (old_state != THREAD_EMPTY && old_cpu != cpu->id) {
		thread->migrations++;
		sched_stats_add_cpu(&sched_counters.migrations,
				    sched_counters.cpu_migrations,
				    cpu->id, 1);
	}
	sched_prepare_enqueue_locked(cpu, thread, wakeup);
	sched_stats_add_cpu(&sched_counters.enqueues,
			    sched_counters.cpu_enqueues, cpu->id, 1);
	if (wakeup) {
		sched_stats_add_cpu(&sched_counters.wakeups,
				    sched_counters.cpu_wakeups, cpu->id, 1);
	}
	cpu->runq.idle = 0;
	runq_push(&cpu->runq, thread);
	console_printf("sched: enqueue tid=%u cpu=%u runq=%u vruntime=%u deadline=%u\n",
		       thread->tid, cpu->id, cpu->runq.count,
		       (u32)thread->vruntime, (u32)thread->virtual_deadline);
	spin_unlock_irqrestore(&cpu->runq.lock, flags);

	if (remote && was_idle) {
		arch_smp_send_scheduler_ipi(cpu->id);
	}
}

static void sched_enqueue_on(struct cpu_sched *cpu, struct thread *thread)
{
	sched_enqueue_on_reason(cpu, thread, 0);
}

static u32 sched_cpu_load(struct cpu_sched *cpu)
{
	u32 load;
	const u64 flags = spin_lock_irqsave(&cpu->runq.lock);

	load = cpu->runq.count;
	if (cpu->current != &cpu->scheduler_thread &&
	    cpu->current->state == THREAD_RUNNING) {
		load++;
	}

	spin_unlock_irqrestore(&cpu->runq.lock, flags);
	return load;
}

static void sched_reap_thread(struct thread *thread)
{
	if (thread == 0 || thread->state != THREAD_DEAD ||
	    thread->task == 0) {
		return;
	}

	struct task *task = thread->task;
	const u64 task_flags = spin_lock_irqsave(&task->lock);
	const u64 thread_flags = spin_lock_irqsave(&thread_table_lock);
	struct thread **task_link = &task->threads;

	while (*task_link != 0) {
		if (*task_link == thread) {
			*task_link = thread->task_next;
			break;
		}
		task_link = &(*task_link)->task_next;
	}

	if (task->thread_count > 0) {
		task->thread_count--;
	}

	const u32 remaining = task->thread_count;
	const u32 teardown = remaining == 0 && task != &kernel_task;
	if (teardown) {
		task->dead = 1;
	}

	const u32 tid = thread->tid;
	const char *name = thread->name;
	const u32 pid = task->pid;

	thread->tid = 0;
	thread->name = 0;
	thread->task = 0;
	thread->entry = 0;
	thread->arg = 0;
	thread->cpu_id = 0;
	thread->run_next = 0;
	thread->sleep_next = 0;
	thread->task_next = 0;
	thread->wake_tick = 0;
	thread->vruntime = 0;
	thread->virtual_deadline = 0;
	thread->eligible_time = 0;
	thread->enqueue_seq = 0;
	thread->enqueue_tick = 0;
	thread->exec_start_tick = 0;
	thread->blocked_since_tick = 0;
	thread->wake_ready_tick = 0;
	thread->total_runtime_ticks = 0;
	thread->wakeups = 0;
	thread->migrations = 0;
	thread->preemptions = 0;
	thread->state = THREAD_EMPTY;
	spin_unlock_irqrestore(&thread_table_lock, thread_flags);
	spin_unlock_irqrestore(&task->lock, task_flags);

	console_printf("sched: reap tid=%u task=%u name=%s remaining=%u\n",
		       tid, pid, name, remaining);
	slab_free(thread);
	if (teardown) {
		task_release(task);
	}
}

static int thread_task_is_killing(const struct thread *thread)
{
	return thread != 0 && thread->task != 0 && thread->task->killing;
}

static u32 sched_select_auto_cpu(void)
{
	const u64 flags = spin_lock_irqsave(&placement_lock);
	u32 best_cpu = next_auto_cpu % sched_cpu_count;
	u32 best_load = sched_cpu_load(&cpus[best_cpu]);

	for (u32 scanned = 1; scanned < sched_cpu_count; scanned++) {
		const u32 cpu_id = (next_auto_cpu + scanned) % sched_cpu_count;
		const u32 load = sched_cpu_load(&cpus[cpu_id]);

		if (load < best_load) {
			best_cpu = cpu_id;
			best_load = load;
		}
	}

	next_auto_cpu = (best_cpu + 1) % sched_cpu_count;
	spin_unlock_irqrestore(&placement_lock, flags);
	return best_cpu;
}

static void sched_activate_thread_space(struct thread *thread)
{
	if (thread == 0 || thread->task == 0 || thread->task->vm_space == 0) {
		return;
	}

	if (thread == &sched_current_cpu()->scheduler_thread) {
		arch_user_set_kernel_stack(0);
		arch_user_set_fs_base(0);
	} else {
		arch_user_set_kernel_stack((u64)(thread->trap_stack +
						 KERNEL_STACK_SIZE));
		arch_user_set_fs_base(task_linux_fs_base(thread->task));
	}

	vm_rpc_activate_space(thread->task->vm_space);
}

static void sched_reset_quantum(struct cpu_sched *cpu)
{
	cpu->quantum_left = SCHED_QUANTUM_TICKS;
}

static void sched_thread_bootstrap(void)
{
	struct thread *thread = sched_current_cpu()->current;

	thread->entry(thread->arg);
	thread_exit();
}

void sched_init(void)
{
	kernel_task.pid = 0;
	kernel_task.name = "kernel";
	kernel_task.ref_count = 1;
	kernel_task.dead = 0;
	kernel_task.killing = 0;
	kernel_task.thread_count = 1;
	kernel_task.threads = 0;
	kernel_task.vm_space = vm_kernel_space();
	kernel_task.linux_brk = 0;
	kernel_task.linux_mmap_next = 0;
	kernel_task.linux_fs_base = 0;
	kernel_task.ipc_affinity_cpu = 0;
	kernel_task.ipc_affinity_valid = 0;
	kernel_task.sched_class = SCHED_CLASS_KERNEL;
	kernel_task.sched_priority = 0;
	kernel_task.sched_rights = SCHED_POLICY_RIGHT_CLASS |
				   SCHED_POLICY_RIGHT_PRIORITY |
				   SCHED_POLICY_RIGHT_WEIGHT;
	kernel_task.vm_regions = 0;
	kernel_task.vm_region_count = 0;
	kernel_task.vm_region_capacity = 0;
	kernel_task.reply_port = 0;
	kernel_task.handles = 0;
	kernel_task.handle_capacity = 0;
	spinlock_init(&kernel_task.lock, "task");

	u64_tree_init(&tasks_by_pid);
	sched_cpu_count = arch_smp_cpu_count();
	if (sched_cpu_count > MAX_CPUS) {
		sched_cpu_count = MAX_CPUS;
	}

	for (u32 i = 0; i < sched_cpu_count; i++) {
		cpus[i].id = i;
		cpus[i].runq.head = 0;
		cpus[i].runq.tail = 0;
		cpus[i].runq.count = 0;
		cpus[i].runq.idle = 0;
		cpus[i].runq.min_vruntime = 0;
		cpus[i].runq.enqueue_seq = 0;
		spinlock_init(&cpus[i].runq.lock, "runq");

		cpus[i].scheduler_thread.tid = 0;
		cpus[i].scheduler_thread.name = "scheduler";
		cpus[i].scheduler_thread.task = &kernel_task;
		cpus[i].scheduler_thread.state = THREAD_RUNNING;
		cpus[i].scheduler_thread.cpu_id = i;
		cpus[i].scheduler_thread.weight = SCHED_BASE_WEIGHT;
		cpus[i].scheduler_thread.sched_class = SCHED_CLASS_KERNEL;
		cpus[i].scheduler_thread.sched_priority = 0;
		cpus[i].current = &cpus[i].scheduler_thread;
		cpus[i].reap = 0;
		cpus[i].quantum_left = SCHED_QUANTUM_TICKS;
	}

	boot_cpu_id = 0;
	arch_thread_context_init_current(&cpus[boot_cpu_id].scheduler_thread.context);
	next_auto_cpu = 0;
	console_printf("sched: init cpus=%u boot_cpu=%u\n", sched_cpu_count,
		       boot_cpu_id);
}

u32 sched_current_cpu_id(void)
{
	return sched_current_cpu()->id;
}

void sched_secondary_start(u32 cpu_id)
{
	if (cpu_id == 0 || cpu_id >= sched_cpu_count) {
		console_printf("sched: invalid secondary cpu=%u\n", cpu_id);
		for (;;) {
			arch_interrupts_disable();
			arch_cpu_wait_for_interrupt();
		}
	}

	console_printf("sched: cpu=%u online\n", cpu_id);
	arch_thread_context_init_current(&cpus[cpu_id].scheduler_thread.context);

	sched_idle_loop();
}

struct task *task_create(const char *name, struct vm_space *vm_space)
{
	if (vm_space == 0) {
		console_printf("sched: refusing task %s without vm space\n", name);
		return 0;
	}

	char *owned_name = task_name_copy(name);
	if (owned_name == 0) {
		console_printf("sched: refusing task with invalid name\n");
		return 0;
	}

	const u64 flags = spin_lock_irqsave(&task_table_lock);
	struct task *task = slab_zalloc(sizeof(*task));

	if (task == 0) {
		spin_unlock_irqrestore(&task_table_lock, flags);
		slab_free(owned_name);
		console_printf("sched: task alloc failed for %s\n", name);
		return 0;
	}

	task->pid = next_pid++;
	task->name = owned_name;
	task->ref_count = 1;
	task->dead = 0;
	task->killing = 0;
	task->thread_count = 0;
	task->threads = 0;
	task->vm_space = vm_space;
	task->linux_brk = 0x900000;
	task->linux_mmap_next = 0x10000000;
	task->linux_fs_base = 0;
	task->ipc_affinity_cpu = 0;
	task->ipc_affinity_valid = 0;
	task->sched_class = SCHED_CLASS_USER;
	task->sched_priority = 0;
	task->sched_rights = 0;
	task->vm_regions = 0;
	task->vm_region_count = 0;
	task->vm_region_capacity = 0;
	task->reply_port = 0;
	task->handles = 0;
	task->handle_capacity = 0;
	spinlock_init(&task->lock, "task");
	if (u64_tree_insert_node(&tasks_by_pid, &task->pid_node,
				 task->pid, (u64)task) != 0) {
		spin_unlock_irqrestore(&task_table_lock, flags);
		slab_free(owned_name);
		slab_free(task);
		console_printf("sched: task registry failed for %s\n", name);
		return 0;
	}
	console_printf("sched: task pid=%u name=%s vm=%u\n",
		       task->pid, owned_name, task->vm_space->id);
	spin_unlock_irqrestore(&task_table_lock, flags);
	return task;
}

static char *task_name_copy(const char *name)
{
	u64 len = 0;
	char *copy;

	if (name == 0) {
		return 0;
	}
	while (len < TASK_NAME_MAX && name[len] != '\0') {
		len++;
	}
	if (len == 0 || len == TASK_NAME_MAX) {
		return 0;
	}
	copy = (char *)slab_alloc(len + 1);
	if (copy == 0) {
		return 0;
	}
	for (u64 i = 0; i <= len; i++) {
		copy[i] = name[i];
	}
	return copy;
}

struct vm_space *task_vm_space(struct task *task)
{
	return task != 0 ? task->vm_space : 0;
}

void task_set_ipc_affinity(struct task *task, u32 cpu_id)
{
	if (task == 0 || cpu_id >= sched_cpu_count) {
		return;
	}

	const u64 flags = spin_lock_irqsave(&task->lock);
	if (task->thread_count == 0) {
		task->ipc_affinity_cpu = cpu_id;
		task->ipc_affinity_valid = 1;
	}
	spin_unlock_irqrestore(&task->lock, flags);
}

void task_set_sched_policy(struct task *task, enum sched_class sched_class,
			   u32 priority, u32 rights)
{
	if (task == 0 ||
	    sched_class < SCHED_CLASS_KERNEL ||
	    sched_class > SCHED_CLASS_IDLE) {
		return;
	}

	const u64 flags = spin_lock_irqsave(&task->lock);

	task->sched_class = sched_class;
	task->sched_priority = priority;
	task->sched_rights = rights;
	spin_unlock_irqrestore(&task->lock, flags);
}

void task_inherit_sched_policy(struct task *dst, const struct task *src)
{
	if (dst == 0 || src == 0) {
		return;
	}

	const u64 src_flags = spin_lock_irqsave(&((struct task *)src)->lock);
	const enum sched_class sched_class = src->sched_class;
	const u32 priority = src->sched_priority;
	const u32 rights = src->sched_rights;
	spin_unlock_irqrestore(&((struct task *)src)->lock, src_flags);

	task_set_sched_policy(dst, sched_class, priority, rights);
}

static u32 task_select_cpu(struct task *task, const char **policy)
{
	if (task != 0) {
		const u64 flags = spin_lock_irqsave(&task->lock);
		const u32 valid = task->ipc_affinity_valid &&
				  task->ipc_affinity_cpu < sched_cpu_count;
		const u32 cpu = task->ipc_affinity_cpu;

		spin_unlock_irqrestore(&task->lock, flags);
		if (valid) {
			if (policy != 0) {
				*policy = "ipc-affinity";
			}
			return cpu;
		}
	}

	if (policy != 0) {
		*policy = "auto";
	}
	return sched_select_auto_cpu();
}

static struct thread *thread_create_placed(struct task *task, const char *name,
					   thread_entry_t entry, void *arg,
					   u32 cpu_id,
					   const char *placement_policy)
{
	if (task == 0 || entry == 0) {
		return 0;
	}

	const u64 task_flags = spin_lock_irqsave(&task->lock);
	if (task->dead || task->killing) {
		spin_unlock_irqrestore(&task->lock, task_flags);
		return 0;
	}

	const u64 thread_flags = spin_lock_irqsave(&thread_table_lock);
	struct thread *thread = slab_zalloc(sizeof(*thread));

	if (thread == 0) {
		spin_unlock_irqrestore(&thread_table_lock, thread_flags);
		spin_unlock_irqrestore(&task->lock, task_flags);
		console_printf("sched: thread alloc failed for %s\n", name);
		return 0;
	}

	thread->tid = next_tid++;
	thread->name = name;
	thread->task = task;
	thread->entry = entry;
	thread->arg = arg;
	thread->weight = SCHED_BASE_WEIGHT;
	thread->sched_class = task->sched_class;
	thread->sched_priority = task->sched_priority;
	thread->run_next = 0;
	thread->sleep_next = 0;
	thread->task_next = task->threads;
	thread->wake_tick = 0;
	thread->vruntime = 0;
	thread->virtual_deadline = 0;
	thread->eligible_time = 0;
	thread->enqueue_seq = 0;
	thread->enqueue_tick = 0;
	thread->exec_start_tick = 0;
	thread->blocked_since_tick = 0;
	thread->wake_ready_tick = 0;
	thread->total_runtime_ticks = 0;
	thread->wakeups = 0;
	thread->migrations = 0;
	thread->preemptions = 0;
	arch_thread_context_init(&thread->context,
				 thread->kernel_stack + KERNEL_STACK_SIZE,
				 sched_thread_bootstrap);
	task->threads = thread;
	task->thread_count++;
	console_printf("sched: thread tid=%u task=%u name=%s\n",
		       thread->tid, task->pid, name);
	console_printf("sched: place tid=%u cpu=%u policy=%s\n",
		       thread->tid, cpu_id, placement_policy);
	spin_unlock_irqrestore(&thread_table_lock, thread_flags);
	spin_unlock_irqrestore(&task->lock, task_flags);
	sched_enqueue_on(&cpus[cpu_id], thread);
	return thread;
}

struct thread *thread_create(struct task *task, const char *name,
			     thread_entry_t entry, void *arg)
{
	const char *policy = "auto";
	const u32 cpu_id = task_select_cpu(task, &policy);

	return thread_create_placed(task, name, entry, arg,
				    cpu_id, policy);
}

struct thread *thread_create_on_cpu(struct task *task, const char *name,
				    thread_entry_t entry, void *arg,
				    u32 cpu_id)
{
	if (cpu_id >= sched_cpu_count) {
		cpu_id = 0;
	}

	return thread_create_placed(task, name, entry, arg, cpu_id, "explicit");
}

struct task *task_current(void)
{
	struct thread *thread = thread_current();

	return thread != 0 ? thread->task : 0;
}

struct thread *thread_current(void)
{
	return sched_current_cpu()->current;
}

u64 task_grant_port(struct task *task, struct ipc_port *port, u32 rights)
{
	if (task == 0 || port == 0) {
		return 0;
	}

	u32 affinity_cpu = 0;
	const int has_affinity =
		(rights & TASK_RIGHT_SEND) != 0 &&
		ipc_port_affinity_cpu(port, &affinity_cpu) == 0;

	const u64 flags = spin_lock_irqsave(&task->lock);

	for (u32 i = 0; i < task->handle_capacity; i++) {
		if (task->handles[i].type == TASK_HANDLE_PORT &&
		    task->handles[i].object == port &&
		    task->handles[i].rights == rights) {
			if (has_affinity && task->thread_count == 0) {
				task->ipc_affinity_cpu = affinity_cpu;
				task->ipc_affinity_valid = 1;
			}
			spin_unlock_irqrestore(&task->lock, flags);
			return i + 1;
		}
	}

	for (;;) {
		for (u32 i = 0; i < task->handle_capacity; i++) {
			if (task->handles[i].type != TASK_HANDLE_EMPTY) {
				continue;
			}

			task->handles[i].type = TASK_HANDLE_PORT;
			task->handles[i].rights = rights;
			task->handles[i].object = port;
			if (has_affinity && task->thread_count == 0) {
				task->ipc_affinity_cpu = affinity_cpu;
				task->ipc_affinity_valid = 1;
			}
			ipc_port_retain(port);
			console_printf("sched: grant task=%u handle=%u type=port rights=0x%x\n",
				       task->pid, i + 1, rights);
			spin_unlock_irqrestore(&task->lock, flags);
			return i + 1;
		}
		if (task_grow_handles_locked(task,
					     task->handle_capacity + 1) != 0) {
			break;
		}
	}

	spin_unlock_irqrestore(&task->lock, flags);
	console_printf("sched: handle table grow failed task=%u\n", task->pid);
	return 0;
}

u64 task_grant_buffer(struct task *task, struct shared_buffer *buffer,
		      u32 rights)
{
	if (task == 0 || buffer == 0) {
		return 0;
	}

	const u64 flags = spin_lock_irqsave(&task->lock);

	for (u32 i = 0; i < task->handle_capacity; i++) {
		if (task->handles[i].type == TASK_HANDLE_BUFFER &&
		    task->handles[i].object == buffer &&
		    task->handles[i].rights == rights) {
			spin_unlock_irqrestore(&task->lock, flags);
			return i + 1;
		}
	}

	for (;;) {
		for (u32 i = 0; i < task->handle_capacity; i++) {
			if (task->handles[i].type != TASK_HANDLE_EMPTY) {
				continue;
			}

			task->handles[i].type = TASK_HANDLE_BUFFER;
			task->handles[i].rights = rights;
			task->handles[i].object = buffer;
			buffer_retain(buffer);
			console_printf("sched: grant task=%u handle=%u type=buffer rights=0x%x target=%u\n",
				       task->pid, i + 1, rights,
				       (u32)buffer_id(buffer));
			spin_unlock_irqrestore(&task->lock, flags);
			return i + 1;
		}
		if (task_grow_handles_locked(task,
					     task->handle_capacity + 1) != 0) {
			break;
		}
	}

	spin_unlock_irqrestore(&task->lock, flags);
	console_printf("sched: handle table grow failed task=%u\n", task->pid);
	return 0;
}

u64 task_grant_task(struct task *owner, struct task *target, u32 rights)
{
	if (owner == 0 || target == 0 || rights == 0 || target->dead) {
		return 0;
	}

	const u64 flags = spin_lock_irqsave(&owner->lock);

	for (u32 i = 0; i < owner->handle_capacity; i++) {
		if (owner->handles[i].type == TASK_HANDLE_TASK &&
		    owner->handles[i].object == target &&
		    owner->handles[i].rights == rights) {
			spin_unlock_irqrestore(&owner->lock, flags);
			return i + 1;
		}
	}

	for (;;) {
		for (u32 i = 0; i < owner->handle_capacity; i++) {
			if (owner->handles[i].type != TASK_HANDLE_EMPTY) {
				continue;
			}

			owner->handles[i].type = TASK_HANDLE_TASK;
			owner->handles[i].rights = rights;
			owner->handles[i].object = target;
			if (target == owner) {
				if (target->dead) {
					owner->handles[i].type =
						TASK_HANDLE_EMPTY;
					owner->handles[i].rights = 0;
					owner->handles[i].object = 0;
					spin_unlock_irqrestore(&owner->lock,
							       flags);
					return 0;
				}
				target->ref_count++;
				console_printf("sched: grant task=%u handle=%u type=task rights=0x%x target=%u\n",
					       owner->pid, i + 1, rights,
					       target->pid);
				spin_unlock_irqrestore(&owner->lock, flags);
				return i + 1;
			}
			const u64 target_flags = spin_lock_irqsave(&target->lock);
			if (target->dead) {
				owner->handles[i].type = TASK_HANDLE_EMPTY;
				owner->handles[i].rights = 0;
				owner->handles[i].object = 0;
				spin_unlock_irqrestore(&target->lock,
						       target_flags);
				spin_unlock_irqrestore(&owner->lock, flags);
				return 0;
			}
			target->ref_count++;
			spin_unlock_irqrestore(&target->lock, target_flags);
			console_printf("sched: grant task=%u handle=%u type=task rights=0x%x target=%u\n",
				       owner->pid, i + 1, rights, target->pid);
			spin_unlock_irqrestore(&owner->lock, flags);
			return i + 1;
		}
		if (task_grow_handles_locked(owner,
					     owner->handle_capacity + 1) != 0) {
			break;
		}
	}

	spin_unlock_irqrestore(&owner->lock, flags);
	console_printf("sched: handle table grow failed task=%u\n", owner->pid);
	return 0;
}

u64 task_grant_hw_resource(struct task *task,
			   const struct task_hw_resource *resource,
			   u32 rights)
{
	if (task == 0 || resource == 0 || rights == 0) {
		return 0;
	}

	const u64 flags = spin_lock_irqsave(&task->lock);

	for (u32 i = 0; i < task->handle_capacity; i++) {
		if (task->handles[i].type == TASK_HANDLE_HW_RESOURCE &&
		    task->handles[i].object == resource &&
		    task->handles[i].rights == rights) {
			spin_unlock_irqrestore(&task->lock, flags);
			return i + 1;
		}
	}

	for (;;) {
		for (u32 i = 0; i < task->handle_capacity; i++) {
			if (task->handles[i].type != TASK_HANDLE_EMPTY) {
				continue;
			}

			if (task_handle_retain(TASK_HANDLE_HW_RESOURCE,
					       (void *)resource) != 0) {
				spin_unlock_irqrestore(&task->lock, flags);
				return 0;
			}
			task->handles[i].type = TASK_HANDLE_HW_RESOURCE;
			task->handles[i].rights = rights;
			task->handles[i].object = (void *)resource;
			console_printf("sched: grant task=%u handle=%u type=hw rights=0x%x target=%u base=%p len=%u ops=0x%x\n",
				       task->pid, i + 1, rights,
				       resource->type, (const void *)resource->base,
				       (u32)resource->len, resource->ops);
			spin_unlock_irqrestore(&task->lock, flags);
			return i + 1;
		}
		if (task_grow_handles_locked(task,
					     task->handle_capacity + 1) != 0) {
			break;
		}
	}

	spin_unlock_irqrestore(&task->lock, flags);
	console_printf("sched: handle table grow failed task=%u\n", task->pid);
	return 0;
}

int task_hw_resource_retain(const struct task_hw_resource *resource)
{
	if (resource == 0) {
		return -1;
	}
	if ((resource->flags & TASK_HW_RESOURCE_OWNED) != 0) {
		__sync_fetch_and_add((u32 *)&resource->ref_count, 1);
	}
	return 0;
}

void task_hw_resource_release(const struct task_hw_resource *resource)
{
	if (resource != 0 &&
	    (resource->flags & TASK_HW_RESOURCE_OWNED) != 0 &&
	    __sync_sub_and_fetch((u32 *)&resource->ref_count, 1) == 0) {
		slab_free((void *)resource);
	}
}

int task_clone_handles(struct task *dst, struct task *src)
{
	if (dst == 0 || src == 0) {
		return -1;
	}

	const u64 src_flags = spin_lock_irqsave(&src->lock);
	const u64 dst_flags = spin_lock_irqsave(&dst->lock);

	if (task_grow_handles_locked(dst, src->handle_capacity) != 0) {
		spin_unlock_irqrestore(&dst->lock, dst_flags);
		spin_unlock_irqrestore(&src->lock, src_flags);
		return -1;
	}

	for (u32 i = 0; i < src->handle_capacity; i++) {
		struct task_handle handle = src->handles[i];

		if (handle.type == TASK_HANDLE_EMPTY) {
			continue;
		}
		if (handle.type == TASK_HANDLE_TASK &&
		    handle.object == src) {
			handle.object = dst;
			dst->ref_count++;
		} else if (task_handle_retain(handle.type, handle.object) != 0) {
			continue;
		}
		dst->handles[i] = handle;
	}

	spin_unlock_irqrestore(&dst->lock, dst_flags);
	spin_unlock_irqrestore(&src->lock, src_flags);
	return 0;
}

struct task *task_from_handle(struct task *owner, u64 handle, u32 rights)
{
	if (owner == 0 || handle == 0) {
		return 0;
	}

	const u64 flags = spin_lock_irqsave(&owner->lock);
	if (handle > owner->handle_capacity) {
		spin_unlock_irqrestore(&owner->lock, flags);
		return 0;
	}
	const struct task_handle task_handle = owner->handles[handle - 1];

	if (task_handle.type != TASK_HANDLE_TASK ||
	    ((struct task *)task_handle.object)->dead ||
	    (task_handle.rights & rights) != rights ||
	    task_handle_retain_from_locked_task(owner, task_handle.type,
						task_handle.object) != 0) {
		spin_unlock_irqrestore(&owner->lock, flags);
		console_printf("sched: task handle denied task=%u handle=%u need=0x%x rights=0x%x\n",
			       owner->pid, (u32)handle, rights,
			       task_handle.rights);
		return 0;
	}

	struct task *target = (struct task *)task_handle.object;

	spin_unlock_irqrestore(&owner->lock, flags);
	return target;
}

int task_can_inherit_handle(struct task *src, u64 handle, u32 rights)
{
	if (src == 0 || handle == 0 || rights == 0) {
		return -1;
	}

	const u64 flags = spin_lock_irqsave(&src->lock);
	if (handle > src->handle_capacity) {
		spin_unlock_irqrestore(&src->lock, flags);
		return -1;
	}
	const struct task_handle src_handle = src->handles[handle - 1];

	if (src_handle.type == TASK_HANDLE_EMPTY ||
	    (src_handle.rights & TASK_RIGHT_DUP) == 0 ||
	    (rights & ~src_handle.rights) != 0) {
		spin_unlock_irqrestore(&src->lock, flags);
		console_printf("sched: inherit denied task=%u handle=%u requested=0x%x rights=0x%x\n",
			       src->pid, (u32)handle, rights, src_handle.rights);
		return -1;
	}

	spin_unlock_irqrestore(&src->lock, flags);
	return 0;
}

u64 task_grant_inherited_handle(struct task *dst, struct task *src, u64 handle,
				u32 rights)
{
	if (dst == 0 || src == 0 || handle == 0 || rights == 0) {
		return 0;
	}

	const u64 flags = spin_lock_irqsave(&src->lock);
	if (handle > src->handle_capacity) {
		spin_unlock_irqrestore(&src->lock, flags);
		return 0;
	}
	const struct task_handle src_handle = src->handles[handle - 1];

	if (src_handle.type == TASK_HANDLE_EMPTY ||
	    (src_handle.rights & TASK_RIGHT_DUP) == 0 ||
	    (rights & ~src_handle.rights) != 0 ||
	    task_handle_retain_from_locked_task(src, src_handle.type,
						src_handle.object) != 0) {
		spin_unlock_irqrestore(&src->lock, flags);
		console_printf("sched: inherit denied task=%u handle=%u requested=0x%x rights=0x%x\n",
			       src->pid, (u32)handle, rights, src_handle.rights);
		return 0;
	}
	spin_unlock_irqrestore(&src->lock, flags);

	u64 granted = 0;
	if (src_handle.type == TASK_HANDLE_PORT) {
		granted = task_grant_port(dst,
					  (struct ipc_port *)src_handle.object,
					  rights);
	} else if (src_handle.type == TASK_HANDLE_BUFFER) {
		granted = task_grant_buffer(
			dst, (struct shared_buffer *)src_handle.object, rights);
	} else if (src_handle.type == TASK_HANDLE_TASK) {
		granted = task_grant_task(dst, (struct task *)src_handle.object,
					  rights);
	} else if (src_handle.type == TASK_HANDLE_HW_RESOURCE) {
		granted = task_grant_hw_resource(
			dst, (const struct task_hw_resource *)src_handle.object,
			rights);
	}

	task_handle_release(src_handle.type, src_handle.object);
	return granted;
}

int task_export_cap(struct task *task, u64 handle, u32 rights,
		    enum task_cap_type *type, void **object)
{
	if (task == 0 || handle == 0 || type == 0 || object == 0) {
		return -1;
	}

	const u64 flags = spin_lock_irqsave(&task->lock);
	if (handle > task->handle_capacity) {
		spin_unlock_irqrestore(&task->lock, flags);
		return -1;
	}
	const struct task_handle task_handle = task->handles[handle - 1];

	if ((task_handle.type != TASK_HANDLE_PORT &&
	     task_handle.type != TASK_HANDLE_BUFFER &&
	     task_handle.type != TASK_HANDLE_TASK &&
	     task_handle.type != TASK_HANDLE_HW_RESOURCE) ||
	    (task_handle.rights & rights) != rights ||
	    task_handle_retain_from_locked_task(task, task_handle.type,
						task_handle.object) != 0) {
		spin_unlock_irqrestore(&task->lock, flags);
		console_printf("sched: cap denied task=%u handle=%u need=0x%x rights=0x%x\n",
			       task->pid, (u32)handle, rights,
			       task_handle.rights);
		return -1;
	}

	*object = task_handle.object;
	*type = task_handle.type == TASK_HANDLE_PORT ? TASK_CAP_PORT :
		(task_handle.type == TASK_HANDLE_BUFFER ? TASK_CAP_BUFFER :
		 task_handle.type == TASK_HANDLE_TASK ? TASK_CAP_TASK :
		 TASK_CAP_HW_RESOURCE);
	spin_unlock_irqrestore(&task->lock, flags);
	return 0;
}

struct ipc_port *task_port_from_handle(struct task *task, u64 handle,
				       u32 rights)
{
	if (task == 0 || handle == 0) {
		return 0;
	}

	const u64 flags = spin_lock_irqsave(&task->lock);
	if (handle > task->handle_capacity) {
		spin_unlock_irqrestore(&task->lock, flags);
		return 0;
	}
	const struct task_handle task_handle = task->handles[handle - 1];

	if (task_handle.type != TASK_HANDLE_PORT ||
	    (task_handle.rights & rights) != rights ||
	    task_handle_retain_from_locked_task(task, task_handle.type,
						task_handle.object) != 0) {
		spin_unlock_irqrestore(&task->lock, flags);
		console_printf("sched: handle denied task=%u handle=%u need=0x%x rights=0x%x\n",
			       task->pid, (u32)handle, rights,
			       task_handle.rights);
		return 0;
	}

	struct ipc_port *port = (struct ipc_port *)task_handle.object;

	spin_unlock_irqrestore(&task->lock, flags);
	return port;
}

struct shared_buffer *task_buffer_from_handle(struct task *task, u64 handle,
					      u32 rights)
{
	if (task == 0 || handle == 0) {
		return 0;
	}

	const u64 flags = spin_lock_irqsave(&task->lock);
	if (handle > task->handle_capacity) {
		spin_unlock_irqrestore(&task->lock, flags);
		return 0;
	}
	const struct task_handle task_handle = task->handles[handle - 1];

	if (task_handle.type != TASK_HANDLE_BUFFER ||
	    (task_handle.rights & rights) != rights ||
	    task_handle_retain_from_locked_task(task, task_handle.type,
						task_handle.object) != 0) {
		spin_unlock_irqrestore(&task->lock, flags);
		console_printf("sched: buffer handle denied task=%u handle=%u need=0x%x rights=0x%x\n",
			       task->pid, (u32)handle, rights,
			       task_handle.rights);
		return 0;
	}

	struct shared_buffer *buffer = (struct shared_buffer *)task_handle.object;

	spin_unlock_irqrestore(&task->lock, flags);
	return buffer;
}

const struct task_hw_resource *task_hw_resource_from_handle(struct task *task,
							    u64 handle,
							    u32 rights)
{
	if (task == 0 || handle == 0) {
		return 0;
	}

	const u64 flags = spin_lock_irqsave(&task->lock);
	if (handle > task->handle_capacity) {
		spin_unlock_irqrestore(&task->lock, flags);
		return 0;
	}
	const struct task_handle task_handle = task->handles[handle - 1];

	if (task_handle.type != TASK_HANDLE_HW_RESOURCE ||
	    (task_handle.rights & rights) != rights ||
	    task_handle_retain_from_locked_task(task, task_handle.type,
						task_handle.object) != 0) {
		spin_unlock_irqrestore(&task->lock, flags);
		console_printf("sched: hw handle denied task=%u handle=%u need=0x%x rights=0x%x\n",
			       task->pid, (u32)handle, rights,
			       task_handle.rights);
		return 0;
	}

	const struct task_hw_resource *resource =
		(const struct task_hw_resource *)task_handle.object;

	spin_unlock_irqrestore(&task->lock, flags);
	return resource;
}

int task_close_handle(struct task *task, u64 handle)
{
	if (task == 0 || handle == 0 || handle > task->handle_capacity) {
		return -1;
	}

	const u64 flags = spin_lock_irqsave(&task->lock);
	struct task_handle *task_handle = &task->handles[handle - 1];

	if (task_handle->type == TASK_HANDLE_EMPTY) {
		spin_unlock_irqrestore(&task->lock, flags);
		console_printf("sched: close denied task=%u handle=%u\n",
			       task->pid, (u32)handle);
		return -1;
	}

	const enum task_handle_type type = task_handle->type;
	const u32 rights = task_handle->rights;
	void *object = task_handle->object;

	task_handle->type = TASK_HANDLE_EMPTY;
	task_handle->rights = 0;
	task_handle->object = 0;
	spin_unlock_irqrestore(&task->lock, flags);

	task_handle_release(type, object);
	console_printf("sched: close task=%u handle=%u type=%s rights=0x%x\n",
		       task->pid, (u32)handle,
		       type == TASK_HANDLE_PORT ? "port" :
		       type == TASK_HANDLE_TASK ? "task" :
		       type == TASK_HANDLE_BUFFER ? "buffer" :
		       type == TASK_HANDLE_HW_RESOURCE ? "hw" : "unknown",
		       rights);
	return 0;
}

static int task_handle_retain(enum task_handle_type type, void *object)
{
	if (type == TASK_HANDLE_PORT) {
		ipc_port_retain((struct ipc_port *)object);
		return 0;
	}
	if (type == TASK_HANDLE_BUFFER) {
		buffer_retain((struct shared_buffer *)object);
		return 0;
	}
	if (type == TASK_HANDLE_TASK) {
		struct task *task = (struct task *)object;

		if (task == task_current()) {
			const u64 flags = spin_lock_irqsave(&task->lock);

			if (task->dead) {
				spin_unlock_irqrestore(&task->lock, flags);
				return -1;
			}
			task->ref_count++;
			spin_unlock_irqrestore(&task->lock, flags);
			return 0;
		}
		const u64 flags = spin_lock_irqsave(&task->lock);

		if (task->dead) {
			spin_unlock_irqrestore(&task->lock, flags);
			return -1;
		}
		task->ref_count++;
		spin_unlock_irqrestore(&task->lock, flags);
		return 0;
	}
	if (type == TASK_HANDLE_HW_RESOURCE) {
		return task_hw_resource_retain(
			(const struct task_hw_resource *)object);
	}
	return -1;
}

static int task_handle_retain_from_locked_task(struct task *locked_task,
					       enum task_handle_type type,
					       void *object)
{
	if (type == TASK_HANDLE_TASK && object == locked_task) {
		struct task *task = (struct task *)object;

		if (task == 0 || task->dead) {
			return -1;
		}
		task->ref_count++;
		return 0;
	}
	return task_handle_retain(type, object);
}

static void task_handle_release(enum task_handle_type type, void *object)
{
	if (type == TASK_HANDLE_PORT) {
		ipc_port_release((struct ipc_port *)object);
	} else if (type == TASK_HANDLE_BUFFER) {
		buffer_release((struct shared_buffer *)object);
	} else if (type == TASK_HANDLE_TASK) {
		task_release((struct task *)object);
	} else if (type == TASK_HANDLE_HW_RESOURCE) {
		task_hw_resource_release(
			(const struct task_hw_resource *)object);
	}
}

int task_handle_lifetime_selftest(void)
{
	struct task *owner = task_create("handle-selftest", vm_kernel_space());
	struct task *target = task_create("handle-selftest-target",
					  vm_kernel_space());
	struct ipc_port *port = ipc_port_create_private("handle-selftest");
	struct shared_buffer *buffer = buffer_create(16);
	struct task_hw_resource *resource =
		(struct task_hw_resource *)slab_zalloc(sizeof(*resource));
	enum task_cap_type cap_type = TASK_CAP_NONE;
	void *cap_object = 0;
	u64 handle;

	if (owner == 0 || target == 0 || port == 0 || buffer == 0 ||
	    resource == 0) {
		return -1;
	}

	handle = task_grant_port(owner, port, TASK_RIGHT_SEND | TASK_RIGHT_DUP);
	port = task_port_from_handle(owner, handle, TASK_RIGHT_SEND);
	if (port == 0 || task_close_handle(owner, handle) != 0 ||
	    ipc_port_id(port) == 0) {
		ipc_port_release(port);
		return -1;
	}
	ipc_port_release(port);

	handle = task_grant_buffer(owner, buffer,
				   TASK_RIGHT_SEND | TASK_RIGHT_RECV |
				   TASK_RIGHT_DUP);
	buffer_release(buffer);
	buffer = task_buffer_from_handle(owner, handle, TASK_RIGHT_SEND);
	if (buffer == 0 || task_export_cap(owner, handle,
					   TASK_RIGHT_SEND | TASK_RIGHT_DUP,
					   &cap_type, &cap_object) != 0 ||
	    cap_type != TASK_CAP_BUFFER ||
	    task_close_handle(owner, handle) != 0 ||
	    buffer_write(buffer, 0, "ok", 2) != 0 ||
	    buffer_id(buffer) == 0) {
		buffer_release(buffer);
		task_handle_release(TASK_HANDLE_BUFFER, cap_object);
		return -1;
	}
	buffer_release(buffer);
	task_handle_release(TASK_HANDLE_BUFFER, cap_object);

	handle = task_grant_task(owner, target, TASK_RIGHT_SEND | TASK_RIGHT_DUP);
	target = task_from_handle(owner, handle, TASK_RIGHT_SEND);
	if (target == 0 || task_close_handle(owner, handle) != 0 ||
	    task_id(target) == 0) {
		task_release(target);
		return -1;
	}
	task_release(target);

	resource->type = TASK_HW_RESOURCE_MMIO;
	resource->ops = TASK_HW_OP_READ;
	resource->flags = TASK_HW_RESOURCE_OWNED;
	resource->ref_count = 0;
	resource->base = 0x1000;
	resource->len = 0x1000;
	handle = task_grant_hw_resource(owner, resource, TASK_RIGHT_SEND);
	resource = (struct task_hw_resource *)task_hw_resource_from_handle(
		owner, handle, TASK_RIGHT_SEND);
	if (resource == 0 || task_close_handle(owner, handle) != 0 ||
	    resource->type != TASK_HW_RESOURCE_MMIO ||
	    resource->base != 0x1000 || resource->len != 0x1000) {
		task_hw_resource_release(resource);
		return -1;
	}
	task_hw_resource_release(resource);

	console_printf("sched: handle lifetime selftest ok\n");
	return 0;
}

static void task_remove_from_list(struct task *task)
{
	const u64 flags = spin_lock_irqsave(&task_table_lock);
	if (task != 0 && task->pid_node.value != 0) {
		u64_tree_remove_node(&tasks_by_pid, &task->pid_node);
	}
	spin_unlock_irqrestore(&task_table_lock, flags);
}

static void task_teardown(struct task *task)
{
	if (task == 0 || task == &kernel_task) {
		return;
	}

	struct task_handle *handles;
	u32 handle_count;
	struct ipc_port *reply_port;
	struct vm_space *space;
	struct task_vm_region *vm_regions;
	const u32 pid = task->pid;
	const char *name = task->name;

	while (task_vm_region_count(task) != 0) {
		const struct task_vm_region *region = task_vm_region_at(task, 0);
		if (region == 0) {
			break;
		}
		const u64 base = region->base;
		const u64 len = region->len;

		(void)vm_unmap_user_range(task->vm_space, base, len);
		(void)task_remove_vm_region(task, base, len);
	}

	const u64 flags = spin_lock_irqsave(&task->lock);
	handles = task->handles;
	handle_count = task->handle_capacity;
	task->handles = 0;
	task->handle_capacity = 0;
	reply_port = task->reply_port;
	task->reply_port = 0;
	space = task->vm_space;
	task->vm_space = 0;
	vm_regions = task->vm_regions;
	task->vm_regions = 0;
	task->vm_region_capacity = 0;
	spin_unlock_irqrestore(&task->lock, flags);

	for (u32 i = 0; i < handle_count; i++) {
		task_handle_release(handles[i].type, handles[i].object);
	}
	slab_free(handles);
	slab_free(vm_regions);
	ipc_port_release(reply_port);
	vm_rpc_destroy_space(space);
	task_remove_from_list(task);
	console_printf("sched: task pid=%u name=%s destroyed\n", pid, name);
	slab_free((void *)name);
}

int task_retain(struct task *task)
{
	if (task == 0 || task == &kernel_task) {
		return -1;
	}

	const u64 flags = spin_lock_irqsave(&task->lock);
	if (task->dead) {
		spin_unlock_irqrestore(&task->lock, flags);
		return -1;
	}
	task->ref_count++;
	spin_unlock_irqrestore(&task->lock, flags);
	return 0;
}

void task_release(struct task *task)
{
	if (task == 0 || task == &kernel_task) {
		return;
	}

	u32 free_task = 0;
	const u64 flags = spin_lock_irqsave(&task->lock);

	if (task->ref_count > 0) {
		task->ref_count--;
	}
	if (task->ref_count == 0 && task->dead) {
		free_task = 1;
	}
	spin_unlock_irqrestore(&task->lock, flags);

	if (free_task) {
		task_teardown(task);
		slab_free(task);
	}
}

struct ipc_port *task_reply_port(struct task *task)
{
	if (task == 0) {
		return 0;
	}

	const u64 flags = spin_lock_irqsave(&task->lock);

	if (task->reply_port == 0) {
		task->reply_port = ipc_port_create_private("reply");
		ipc_port_retain(task->reply_port);
	}

	struct ipc_port *port = task->reply_port;

	spin_unlock_irqrestore(&task->lock, flags);
	return port;
}

void sched_run(void)
{
	struct cpu_sched *cpu = sched_current_cpu();

	for (;;) {
		const u64 flags = spin_lock_irqsave(&cpu->runq.lock);
		struct thread *next = runq_pop(&cpu->runq);

		if (next == 0) {
			spin_unlock_irqrestore(&cpu->runq.lock, flags);
			return;
		}
		spin_unlock_irqrestore(&cpu->runq.lock, flags);

		struct thread *prev = cpu->current;
		if (thread_task_is_killing(next)) {
			next->state = THREAD_DEAD;
			sched_reap_thread(next);
			continue;
		}

		next->state = THREAD_RUNNING;
		sched_note_switch(cpu, next);
		cpu->current = next;
		console_printf("sched: switch cpu=%u prev=%u next=%u\n",
			       cpu->id, prev->tid, next->tid);
		sched_reset_quantum(cpu);
		sched_activate_thread_space(next);
		arch_thread_switch(&prev->context, &next->context);
		sched_activate_thread_space(cpu->current);
		struct thread *dead = cpu->reap;
		cpu->reap = 0;
		sched_reap_thread(dead);
	}
}

static void sched_idle_wait(struct cpu_sched *cpu)
{
	arch_interrupts_disable();

	const u64 flags = spin_lock_irqsave(&cpu->runq.lock);
	if (cpu->runq.count == 0) {
		cpu->runq.idle = 1;
		spin_unlock_irqrestore(&cpu->runq.lock, flags);
		arch_interrupts_enable();
		arch_cpu_wait_for_interrupt();
		return;
	}

	cpu->runq.idle = 0;
	spin_unlock_irqrestore(&cpu->runq.lock, flags);
	arch_interrupts_enable();
}

void sched_idle_loop(void)
{
	for (;;) {
		sched_run();
		sched_idle_wait(sched_current_cpu());
	}
}

void thread_yield(void)
{
	struct cpu_sched *cpu = sched_current_cpu();
	struct thread *prev = cpu->current;

	if (prev == &cpu->scheduler_thread) {
		sched_run();
		return;
	}
	if (thread_task_is_killing(prev)) {
		thread_exit();
	}

	sched_account_runtime(cpu);
	console_printf("sched: yield tid=%u cpu=%u\n", prev->tid, cpu->id);
	sched_enqueue_on(cpu, prev);
	cpu->current = &cpu->scheduler_thread;
	sched_activate_thread_space(cpu->current);
	arch_thread_switch(&prev->context, &cpu->scheduler_thread.context);
}

void sched_enable_preemption(void)
{
	preemption_enabled = 1;
	console_printf("sched: preemption enabled\n");
}

void sched_stats_snapshot(struct sched_stats *stats)
{
	if (stats == 0) {
		return;
	}

	u64 flags = spin_lock_irqsave(&sched_stats_lock);
	*stats = sched_counters;
	spin_unlock_irqrestore(&sched_stats_lock, flags);

	for (u32 cpu_id = 0; cpu_id < sched_cpu_count &&
	     cpu_id < SCHED_STATS_CPUS; cpu_id++) {
		struct cpu_sched *cpu = &cpus[cpu_id];
		u64 load;

		flags = spin_lock_irqsave(&cpu->runq.lock);
		load = cpu->runq.count;
		if (cpu->current != &cpu->scheduler_thread &&
		    cpu->current->state == THREAD_RUNNING) {
			load++;
		}
		stats->cpu_runq_load[cpu_id] = load;
		stats->cpu_min_vruntime[cpu_id] = cpu->runq.min_vruntime;
		spin_unlock_irqrestore(&cpu->runq.lock, flags);
	}
}

void sched_tick(void)
{
	struct cpu_sched *cpu = sched_current_cpu();
	struct thread *prev = cpu->current;
	u32 runnable;

	if (!preemption_enabled || prev == &cpu->scheduler_thread ||
	    prev->state != THREAD_RUNNING) {
		return;
	}

	const u64 flags = spin_lock_irqsave(&cpu->runq.lock);
	runnable = cpu->runq.count;
	spin_unlock_irqrestore(&cpu->runq.lock, flags);

	if (runnable == 0) {
		return;
	}

	sched_account_runtime(cpu);
	if (cpu->quantum_left > 1) {
		cpu->quantum_left--;
		return;
	}

	console_printf("sched: preempt tid=%u cpu=%u\n", prev->tid, cpu->id);
	prev->preemptions++;
	sched_stats_add_cpu(&sched_counters.preemptions,
			    sched_counters.cpu_preemptions, cpu->id, 1);
	sched_reset_quantum(cpu);
	sched_enqueue_on(cpu, prev);
	cpu->current = &cpu->scheduler_thread;
	sched_activate_thread_space(cpu->current);
	arch_thread_switch(&prev->context, &cpu->scheduler_thread.context);
}

void thread_prepare_block(void)
{
	struct cpu_sched *cpu = sched_current_cpu();
	struct thread *prev = cpu->current;

	if (prev == &cpu->scheduler_thread) {
		return;
	}
	if (thread_task_is_killing(prev)) {
		thread_exit();
	}

	sched_account_runtime(cpu);
	console_printf("sched: block tid=%u cpu=%u\n", prev->tid, cpu->id);
	prev->blocked_since_tick = timer_ticks();
	prev->state = THREAD_BLOCKED;
}

void thread_block_prepared(void)
{
	struct cpu_sched *cpu = sched_current_cpu();
	struct thread *prev = cpu->current;

	if (prev == &cpu->scheduler_thread) {
		return;
	}

	cpu->current = &cpu->scheduler_thread;
	sched_activate_thread_space(cpu->current);
	arch_thread_switch(&prev->context, &cpu->scheduler_thread.context);
}

void thread_block(void)
{
	thread_prepare_block();
	thread_block_prepared();
}

void thread_sleep_ticks(u64 ticks)
{
	struct cpu_sched *cpu = sched_current_cpu();
	struct thread *prev = cpu->current;

	if (ticks == 0) {
		thread_yield();
		return;
	}

	if (prev == &cpu->scheduler_thread) {
		return;
	}
	if (thread_task_is_killing(prev)) {
		thread_exit();
	}

	sched_account_runtime(cpu);
	const u64 flags = spin_lock_irqsave(&sleep_lock);
	prev->wake_tick = timer_ticks() + ticks;
	prev->blocked_since_tick = timer_ticks();
	struct thread **link = &sleep_list;
	while (*link != 0 && (*link)->wake_tick <= prev->wake_tick) {
		link = &(*link)->sleep_next;
	}
	prev->sleep_next = *link;
	*link = prev;
	prev->state = THREAD_BLOCKED;
	spin_unlock_irqrestore(&sleep_lock, flags);

	console_printf("sched: sleep tid=%u cpu=%u ticks=%u\n",
		       prev->tid, cpu->id, (u32)ticks);
	cpu->current = &cpu->scheduler_thread;
	sched_activate_thread_space(cpu->current);
	arch_thread_switch(&prev->context, &cpu->scheduler_thread.context);
}

void thread_sleep_ns(u64 ns)
{
	thread_sleep_ticks(timer_ns_to_ticks_ceil(ns));
}

void sched_wake_sleepers(u64 now)
{
	struct thread *ready = 0;
	const u64 flags = spin_lock_irqsave(&sleep_lock);
	struct thread **link = &sleep_list;

	while (*link != 0) {
		struct thread *thread = *link;

		if (thread->wake_tick > now) {
			link = &thread->sleep_next;
			continue;
		}

		*link = thread->sleep_next;
		thread->sleep_next = ready;
		ready = thread;
	}

	spin_unlock_irqrestore(&sleep_lock, flags);

	while (ready != 0) {
		struct thread *thread = ready;

		ready = thread->sleep_next;
		thread->sleep_next = 0;
		thread->wake_tick = 0;
		console_printf("sched: wake tid=%u\n", thread->tid);
		thread_unblock(thread);
	}
}

void thread_unblock(struct thread *thread)
{
	if (thread == 0 || thread->state != THREAD_BLOCKED) {
		return;
	}

	sched_enqueue_on_reason(&cpus[thread->cpu_id], thread, 1);
}

int thread_handoff(struct thread *thread)
{
	struct cpu_sched *cpu = sched_current_cpu();
	struct thread *prev = cpu->current;

	if (thread == 0 || thread->state != THREAD_BLOCKED) {
		return THREAD_HANDOFF_INVALID;
	}
	if (prev == &cpu->scheduler_thread) {
		thread_unblock(thread);
		return THREAD_HANDOFF_SCHEDULER;
	}
	if (thread->cpu_id != cpu->id) {
		thread_unblock(thread);
		return THREAD_HANDOFF_CROSS_CPU;
	}
	if (cpu->handoff_depth != 0) {
		thread_unblock(thread);
		return THREAD_HANDOFF_NESTED;
	}
	if (thread_task_is_killing(prev)) {
		thread_unblock(thread);
		thread_exit();
	}

	sched_account_runtime(cpu);
	sched_enqueue_on(cpu, prev);
	thread->state = THREAD_RUNNING;
	thread->blocked_since_tick = 0;
	thread->enqueue_tick = 0;
	thread->wake_ready_tick = timer_ticks();
	thread->wakeups++;
	sched_stats_add_cpu(&sched_counters.wakeups,
			    sched_counters.cpu_wakeups, cpu->id, 1);
	sched_note_switch(cpu, thread);
	cpu->current = thread;
	sched_reset_quantum(cpu);
	sched_activate_thread_space(thread);
	cpu->handoff_depth++;
	arch_thread_switch(&prev->context, &thread->context);
	cpu->handoff_depth--;
	sched_activate_thread_space(cpu->current);
	struct thread *dead = cpu->reap;
	cpu->reap = 0;
	sched_reap_thread(dead);
	return THREAD_HANDOFF_DIRECT;
}

static void sched_cancel_sleep(struct thread *thread)
{
	if (thread == 0) {
		return;
	}

	const u64 flags = spin_lock_irqsave(&sleep_lock);
	struct thread **link = &sleep_list;

	while (*link != 0) {
		if (*link == thread) {
			*link = thread->sleep_next;
			thread->sleep_next = 0;
			thread->wake_tick = 0;
			break;
		}
		link = &(*link)->sleep_next;
	}

	spin_unlock_irqrestore(&sleep_lock, flags);
}

static int sched_cancel_ready_task(struct task *task)
{
	if (task == 0) {
		return 0;
	}
	for (u32 cpu_id = 0; cpu_id < sched_cpu_count; cpu_id++) {
		struct cpu_sched *cpu = &cpus[cpu_id];
		struct thread *removed = 0;
		const u64 flags = spin_lock_irqsave(&cpu->runq.lock);

		for (struct thread *thread = cpu->runq.head; thread != 0;
		     thread = thread->run_next) {
			if (thread->task == task && thread->state == THREAD_READY) {
				removed = thread;
				(void)runq_remove(&cpu->runq, thread);
				removed->state = THREAD_DEAD;
				break;
			}
		}
		spin_unlock_irqrestore(&cpu->runq.lock, flags);
		if (removed != 0) {
			sched_reap_thread(removed);
			return 1;
		}
	}
	return 0;
}

int task_kill(struct task *task)
{
	if (task == 0 || task == &kernel_task) {
		return -1;
	}

	u64 flags = spin_lock_irqsave(&task->lock);
	if (task->dead) {
		spin_unlock_irqrestore(&task->lock, flags);
		return -1;
	}
	task->killing = 1;
	task->ref_count++;
	spin_unlock_irqrestore(&task->lock, flags);

	for (;;) {
		struct thread *blocked = 0;

		flags = spin_lock_irqsave(&task->lock);
		for (struct thread *thread = task->threads; thread != 0;
		     thread = thread->task_next) {
			if (thread->state == THREAD_BLOCKED) {
				blocked = thread;
				break;
			}
		}
		spin_unlock_irqrestore(&task->lock, flags);

		if (blocked == 0) {
			break;
		}

		ipc_cancel_thread(blocked);
		sched_cancel_sleep(blocked);
		thread_unblock(blocked);
	}

	while (sched_cancel_ready_task(task) != 0) {
	}

	console_printf("sched: kill task=%u name=%s\n", task->pid, task->name);
	task_release(task);
	return 0;
}

int task_is_killing(const struct task *task)
{
	return task != 0 && task->killing;
}

enum sched_class task_sched_class(const struct task *task)
{
	return task != 0 ? task->sched_class : SCHED_CLASS_KERNEL;
}

void thread_exit(void)
{
	struct cpu_sched *cpu = sched_current_cpu();
	struct thread *prev = cpu->current;

	console_printf("sched: thread tid=%u exited\n", prev->tid);
	sched_account_runtime(cpu);
	prev->state = THREAD_DEAD;
	cpu->reap = prev;
	cpu->current = &cpu->scheduler_thread;
	sched_activate_thread_space(cpu->current);
	arch_thread_switch(&prev->context, &cpu->scheduler_thread.context);

	for (;;) {
		arch_cpu_wait_for_interrupt();
	}
}

static u64 region_end(const struct task_vm_region *region)
{
	return region->base + region->len;
}

static u32 writable_to_prot(u32 writable)
{
	return TASK_VM_PROT_READ |
	       (writable != 0 ? TASK_VM_PROT_WRITE : 0);
}

static u32 writable_to_flags(u32 kind)
{
	return TASK_VM_MAP_PRIVATE |
	       (kind == TASK_VM_REGION_MMAP ||
		kind == TASK_VM_REGION_BRK ||
		kind == TASK_VM_REGION_STACK ? TASK_VM_MAP_ANONYMOUS : 0);
}

static u32 kind_to_object_type(u32 kind)
{
	return kind == TASK_VM_REGION_ELF ? TASK_VM_OBJECT_FILE :
					    TASK_VM_OBJECT_ANON;
}

static int regions_overlap(u64 a_base, u64 a_len, u64 b_base, u64 b_len)
{
	const u64 a_end = a_base + a_len;
	const u64 b_end = b_base + b_len;

	return a_base < b_end && b_base < a_end;
}

u32 task_id(const struct task *task)
{
	return task != 0 ? task->pid : 0;
}

static void pack_name_words(const char *name, u64 words[2])
{
	words[0] = 0;
	words[1] = 0;
	if (name == 0) {
		return;
	}
	for (u64 i = 0; i < 16 && name[i] != '\0'; i++) {
		words[i / 8] |= ((u64)(u8)name[i]) << ((i % 8) * 8);
	}
}

int task_info_at(u64 index, u64 *pid_threads_flags, u64 *name_words)
{
	u64 seen = 0;
	const u64 flags = spin_lock_irqsave(&task_table_lock);

	for (struct u64_tree_node *node = u64_tree_first_node(&tasks_by_pid);
	     node != 0; node = u64_tree_next_node(node)) {
		struct task *task = (struct task *)node->value;

		if (seen++ != index) {
			continue;
		}

		if (pid_threads_flags != 0) {
			*pid_threads_flags = ((u64)task->pid) |
					     ((u64)task->thread_count << 32) |
					     ((u64)task->dead << 48) |
					     ((u64)task->killing << 49);
		}
		if (name_words != 0) {
			pack_name_words(task->name, name_words);
		}

		spin_unlock_irqrestore(&task_table_lock, flags);
		return 0;
	}

	spin_unlock_irqrestore(&task_table_lock, flags);
	return -1;
}

int sched_thread_info_at(u64 index, struct sched_thread_info *info)
{
	u64 seen = 0;
	const u64 now = timer_ticks();
	const u64 task_flags = spin_lock_irqsave(&task_table_lock);
	const u64 thread_flags = spin_lock_irqsave(&thread_table_lock);

	if (info == 0) {
		spin_unlock_irqrestore(&thread_table_lock, thread_flags);
		spin_unlock_irqrestore(&task_table_lock, task_flags);
		return -1;
	}

	for (struct u64_tree_node *node = u64_tree_first_node(&tasks_by_pid);
	     node != 0; node = u64_tree_next_node(node)) {
		struct task *task = (struct task *)node->value;

		for (struct thread *thread = task->threads; thread != 0;
		     thread = thread->task_next) {
			if (seen++ != index) {
				continue;
			}

			info->task_id = task->pid;
			info->thread_id = thread->tid;
			info->state = thread->state;
			info->cpu_id = thread->cpu_id;
			info->sched_class = thread->sched_class;
			info->sched_priority = thread->sched_priority;
			info->weight = thread->weight;
			info->runtime_ticks = thread->total_runtime_ticks;
			info->wakeups = thread->wakeups;
			info->migrations = thread->migrations;
			info->preemptions = thread->preemptions;
			info->vruntime = thread->vruntime;
			info->virtual_deadline = thread->virtual_deadline;
			info->runnable_wait_ticks =
				thread->state == THREAD_READY &&
						thread->enqueue_tick != 0 &&
						now >= thread->enqueue_tick ?
					now - thread->enqueue_tick :
					0;
			info->wake_to_run_pending_ticks =
				thread->wake_ready_tick != 0 &&
						now >= thread->wake_ready_tick ?
					now - thread->wake_ready_tick :
					0;
			pack_name_words(task->name, info->task_name_words);
			pack_name_words(thread->name, info->thread_name_words);

			spin_unlock_irqrestore(&thread_table_lock,
					       thread_flags);
			spin_unlock_irqrestore(&task_table_lock, task_flags);
			return 0;
		}
	}

	spin_unlock_irqrestore(&thread_table_lock, thread_flags);
	spin_unlock_irqrestore(&task_table_lock, task_flags);
	return -1;
}

u64 task_linux_brk(const struct task *task)
{
	return task != 0 ? task->linux_brk : 0;
}

void task_set_linux_brk(struct task *task, u64 brk)
{
	if (task != 0) {
		task->linux_brk = brk;
	}
}

u64 task_linux_mmap_next(const struct task *task)
{
	return task != 0 ? task->linux_mmap_next : 0;
}

void task_set_linux_mmap_next(struct task *task, u64 next)
{
	if (task != 0) {
		task->linux_mmap_next = next;
	}
}

u64 task_linux_fs_base(const struct task *task)
{
	return task != 0 ? task->linux_fs_base : 0;
}

void task_set_linux_fs_base(struct task *task, u64 fs_base)
{
	if (task != 0) {
		task->linux_fs_base = fs_base;
	}
}

int task_add_vm_region(struct task *task, u64 base, u64 len, u32 writable,
		       u32 kind)
{
	return task_add_vm_mapping(task, base, len, writable_to_prot(writable),
				   writable_to_flags(kind), kind,
				   kind_to_object_type(kind), 0, 0);
}

int task_add_vm_mapping(struct task *task, u64 base, u64 len, u32 prot,
			u32 map_flags, u32 kind, u32 object_type,
			u32 object_id, u64 offset)
{
	if (task == 0 || len == 0 || base + len < base) {
		return -1;
	}

	const u64 irq_flags = spin_lock_irqsave(&task->lock);

	for (u32 i = 0; i < task->vm_region_count; i++) {
		struct task_vm_region *region = &task->vm_regions[i];
		const int offset_extends =
			object_id == 0 ||
			object_type == TASK_VM_OBJECT_ANON ||
			region->offset + region->len == offset;

		if (region->kind == kind &&
		    region->prot == prot &&
		    region->flags == map_flags &&
		    region->object_type == object_type &&
		    region->object_id == object_id &&
		    offset_extends &&
		    region_end(region) == base) {
			region->len += len;
			spin_unlock_irqrestore(&task->lock, irq_flags);
			return 0;
		}
	}

	for (u32 i = 0; i < task->vm_region_count; i++) {
		const struct task_vm_region *region = &task->vm_regions[i];

		if (regions_overlap(base, len, region->base, region->len)) {
			spin_unlock_irqrestore(&task->lock, irq_flags);
			console_printf("sched: vma overlap task=%u base=%p len=%u\n",
				       task->pid, (const void *)base, (u32)len);
			return -1;
		}
	}

	if (task_grow_vm_regions_locked(task, task->vm_region_count + 1) != 0) {
		spin_unlock_irqrestore(&task->lock, irq_flags);
		console_printf("sched: vma table grow failed task=%u\n",
			       task->pid);
		return -1;
	}

	struct task_vm_region *region = &task->vm_regions[task->vm_region_count++];
	region->base = base;
	region->len = len;
	region->offset = offset;
	region->writable = (prot & TASK_VM_PROT_WRITE) != 0;
	region->kind = kind;
	region->prot = prot;
	region->flags = map_flags;
	region->object_type = object_type;
	region->object_id = object_id;
	spin_unlock_irqrestore(&task->lock, irq_flags);
	return 0;
}

int task_add_or_extend_vm_region(struct task *task, u64 base, u64 len,
				 u32 writable, u32 kind)
{
	return task_add_or_extend_vm_mapping(task, base, len,
					     writable_to_prot(writable),
					     writable_to_flags(kind), kind,
					     kind_to_object_type(kind), 0, 0);
}

int task_add_or_extend_vm_mapping(struct task *task, u64 base, u64 len,
				  u32 prot, u32 map_flags, u32 kind,
				  u32 object_type, u32 object_id, u64 offset)
{
	if (task == 0 || len == 0 || base + len < base) {
		return -1;
	}

	const u64 flags = spin_lock_irqsave(&task->lock);

	for (u32 i = 0; i < task->vm_region_count; i++) {
		struct task_vm_region *region = &task->vm_regions[i];
		const u64 end = region_end(region);

		if (region->kind == kind &&
		    region->prot == prot &&
		    region->flags == map_flags &&
		    region->object_type == object_type &&
		    region->object_id == object_id &&
		    region->base <= base &&
		    base + len >= region->base &&
		    base <= end) {
			const u64 new_end = base + len > end ? base + len : end;

			region->len = new_end - region->base;
			spin_unlock_irqrestore(&task->lock, flags);
			return 0;
		}
	}

	spin_unlock_irqrestore(&task->lock, flags);
	return task_add_vm_mapping(task, base, len, prot, map_flags, kind,
				   object_type, object_id, offset);
}

int task_vm_range_is_free(struct task *task, u64 base, u64 len)
{
	if (task == 0 || len == 0 || base + len < base) {
		return 0;
	}

	const u64 flags = spin_lock_irqsave(&task->lock);

	for (u32 i = 0; i < task->vm_region_count; i++) {
		const struct task_vm_region *region = &task->vm_regions[i];

		if (regions_overlap(base, len, region->base, region->len)) {
			spin_unlock_irqrestore(&task->lock, flags);
			return 0;
		}
	}

	spin_unlock_irqrestore(&task->lock, flags);
	return 1;
}

static void region_set_prot(struct task_vm_region *region, u32 prot)
{
	region->prot = prot;
	region->writable = (prot & TASK_VM_PROT_WRITE) != 0;
}

static int task_vm_range_is_covered_locked(struct task *task, u64 base, u64 len)
{
	const u64 end = base + len;
	u64 cursor = base;

	while (cursor < end) {
		int found = 0;

		for (u32 i = 0; i < task->vm_region_count; i++) {
			const struct task_vm_region *region = &task->vm_regions[i];
			const u64 region_end = region->base + region->len;

			if (region->base <= cursor && cursor < region_end) {
				cursor = region_end < end ? region_end : end;
				found = 1;
				break;
			}
		}
		if (!found) {
			return 0;
		}
	}

	return 1;
}

static int task_append_vm_region_locked(struct task *task,
					struct task_vm_region region)
{
	if (task_grow_vm_regions_locked(task, task->vm_region_count + 1) != 0) {
		console_printf("sched: vma protect split denied task=%u\n",
			       task->pid);
		return -1;
	}

	task->vm_regions[task->vm_region_count++] = region;
	return 0;
}

int task_protect_vm_region(struct task *task, u64 base, u64 len, u32 prot)
{
	if (task == 0 || len == 0 || base + len < base) {
		return -1;
	}

	const u64 flags = spin_lock_irqsave(&task->lock);
	const u64 end = base + len;

	if (!task_vm_range_is_covered_locked(task, base, len)) {
		spin_unlock_irqrestore(&task->lock, flags);
		return -1;
	}

	for (u32 i = 0; i < task->vm_region_count; i++) {
		struct task_vm_region *region = &task->vm_regions[i];
		const u64 region_base = region->base;
		const u64 region_end = region->base + region->len;

		if (!regions_overlap(base, len, region_base, region->len)) {
			continue;
		}

		const u64 protect_base =
			base > region_base ? base : region_base;
		const u64 protect_end = end < region_end ? end : region_end;

		if (protect_base == region_base && protect_end == region_end) {
			region_set_prot(region, prot);
			continue;
		}

		struct task_vm_region middle = *region;

		middle.base = protect_base;
		middle.len = protect_end - protect_base;
		region_set_prot(&middle, prot);

		if (protect_base == region_base) {
			region->base = protect_end;
			region->len = region_end - protect_end;
			if (task_append_vm_region_locked(task, middle) != 0) {
				spin_unlock_irqrestore(&task->lock, flags);
				return -1;
			}
			continue;
		}

		if (protect_end == region_end) {
			region->len = protect_base - region_base;
			if (task_append_vm_region_locked(task, middle) != 0) {
				spin_unlock_irqrestore(&task->lock, flags);
				return -1;
			}
			continue;
		}

		struct task_vm_region right = *region;

		right.base = protect_end;
		right.len = region_end - protect_end;
		region->len = protect_base - region_base;
		if (task_append_vm_region_locked(task, middle) != 0 ||
		    task_append_vm_region_locked(task, right) != 0) {
			spin_unlock_irqrestore(&task->lock, flags);
			return -1;
		}
	}

	spin_unlock_irqrestore(&task->lock, flags);
	return 0;
}

int task_remove_vm_region(struct task *task, u64 base, u64 len)
{
	if (task == 0 || len == 0 || base + len < base) {
		return -1;
	}

	const u64 flags = spin_lock_irqsave(&task->lock);
	const u64 end = base + len;

	for (u32 i = 0; i < task->vm_region_count; i++) {
		struct task_vm_region *region = &task->vm_regions[i];
		const u64 region_base = region->base;
		const u64 region_end = region->base + region->len;

		if (base < region_base || end > region_end) {
			continue;
		}

		if (base == region_base && end == region_end) {
			task->vm_regions[i] =
				task->vm_regions[task->vm_region_count - 1];
			task->vm_region_count--;
		} else if (base == region_base) {
			region->base = end;
			region->len = region_end - end;
		} else if (end == region_end) {
			region->len = base - region_base;
		} else {
			struct task_vm_region right = *region;

			if (task_grow_vm_regions_locked(task,
							task->vm_region_count + 1) != 0) {
				spin_unlock_irqrestore(&task->lock, flags);
				console_printf("sched: vma split denied task=%u\n",
					       task->pid);
				return -1;
			}
			right.base = end;
			right.len = region_end - end;
			region->len = base - region_base;
			task->vm_regions[task->vm_region_count++] = right;
		}
		spin_unlock_irqrestore(&task->lock, flags);
		return 0;
	}

	spin_unlock_irqrestore(&task->lock, flags);
	return -1;
}

void task_clear_vm_regions(struct task *task)
{
	if (task == 0) {
		return;
	}

	const u64 flags = spin_lock_irqsave(&task->lock);

	task->vm_region_count = 0;
	spin_unlock_irqrestore(&task->lock, flags);
}

u64 task_vm_region_count(const struct task *task)
{
	return task != 0 ? task->vm_region_count : 0;
}

const struct task_vm_region *task_vm_region_at(const struct task *task,
					       u64 index)
{
	if (task == 0 || index >= task->vm_region_count) {
		return 0;
	}

	return &task->vm_regions[index];
}

u32 thread_id(const struct thread *thread)
{
	return thread != 0 ? thread->tid : 0;
}

enum thread_state thread_state(const struct thread *thread)
{
	return thread != 0 ? thread->state : THREAD_EMPTY;
}

const char *task_name(const struct task *task)
{
	return task != 0 ? task->name : "(null)";
}

const char *thread_name(const struct thread *thread)
{
	return thread != 0 ? thread->name : "(null)";
}
