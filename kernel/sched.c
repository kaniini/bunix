#include "console.h"
#include "ipc.h"
#include "sched.h"
#include "spinlock.h"
#include "timer.h"
#include "vm.h"
#include <arch/smp.h>
#include <arch/thread.h>
#include <arch/user.h>

enum {
	MAX_CPUS = 8,
	MAX_TASKS = 16,
	MAX_THREADS = 32,
	MAX_TASK_HANDLES = 32,
	KERNEL_STACK_SIZE = 16384,
	SCHED_QUANTUM_TICKS = 5,
};

enum task_handle_type {
	TASK_HANDLE_EMPTY = 0,
	TASK_HANDLE_PORT,
	TASK_HANDLE_TASK,
};

struct task_handle {
	enum task_handle_type type;
	u32 rights;
	void *object;
};

struct task {
	u32 pid;
	const char *name;
	u32 thread_count;
	struct vm_space *vm_space;
	struct ipc_port *reply_port;
	struct task_handle handles[MAX_TASK_HANDLES];
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
	struct thread *run_next;
	struct thread *sleep_next;
	u64 wake_tick;
	struct arch_thread_context context;
	u8 kernel_stack[KERNEL_STACK_SIZE] __attribute__((aligned(16)));
	u8 trap_stack[KERNEL_STACK_SIZE] __attribute__((aligned(16)));
};

struct run_queue {
	struct thread *head;
	struct thread *tail;
	u32 count;
	u32 idle;
	struct spinlock lock;
};

struct cpu_sched {
	u32 id;
	struct run_queue runq;
	struct thread *current;
	struct thread *reap;
	struct thread scheduler_thread;
	u32 quantum_left;
};

static struct task tasks[MAX_TASKS];
static struct thread threads[MAX_THREADS];
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

static struct cpu_sched *sched_current_cpu(void)
{
	return &cpus[arch_smp_current_cpu_id()];
}

static void runq_push(struct run_queue *runq, struct thread *thread)
{
	thread->run_next = 0;
	if (runq->tail != 0) {
		runq->tail->run_next = thread;
	} else {
		runq->head = thread;
	}

	runq->tail = thread;
	runq->count++;
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
	return thread;
}

static void sched_enqueue_on(struct cpu_sched *cpu, struct thread *thread)
{
	const u32 remote = cpu->id != sched_current_cpu_id();
	const u64 flags = spin_lock_irqsave(&cpu->runq.lock);
	const u32 was_idle = cpu->runq.idle;

	thread->cpu_id = cpu->id;
	thread->state = THREAD_READY;
	cpu->runq.idle = 0;
	runq_push(&cpu->runq, thread);
	console_printf("sched: enqueue tid=%u cpu=%u runq=%u\n",
		       thread->tid, cpu->id, cpu->runq.count);
	spin_unlock_irqrestore(&cpu->runq.lock, flags);

	if (remote && was_idle) {
		arch_smp_send_scheduler_ipi(cpu->id);
	}
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

	if (task->thread_count > 0) {
		task->thread_count--;
	}

	const u32 remaining = task->thread_count;

	spin_unlock_irqrestore(&task->lock, task_flags);

	const u64 thread_flags = spin_lock_irqsave(&thread_table_lock);
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
	thread->wake_tick = 0;
	thread->state = THREAD_EMPTY;
	spin_unlock_irqrestore(&thread_table_lock, thread_flags);

	console_printf("sched: reap tid=%u task=%u name=%s remaining=%u\n",
		       tid, pid, name, remaining);
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
	} else {
		arch_user_set_kernel_stack((u64)(thread->trap_stack +
						 KERNEL_STACK_SIZE));
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
	kernel_task.thread_count = 1;
	kernel_task.vm_space = vm_kernel_space();
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
		spinlock_init(&cpus[i].runq.lock, "runq");

		cpus[i].scheduler_thread.tid = 0;
		cpus[i].scheduler_thread.name = "scheduler";
		cpus[i].scheduler_thread.task = &kernel_task;
		cpus[i].scheduler_thread.state = THREAD_RUNNING;
		cpus[i].scheduler_thread.cpu_id = i;
		cpus[i].current = &cpus[i].scheduler_thread;
		cpus[i].reap = 0;
		cpus[i].quantum_left = SCHED_QUANTUM_TICKS;
	}

	boot_cpu_id = 0;
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
			__asm__ volatile ("cli; hlt");
		}
	}

	console_printf("sched: cpu=%u online\n", cpu_id);

	sched_idle_loop();
}

struct task *task_create(const char *name, struct vm_space *vm_space)
{
	if (vm_space == 0) {
		console_printf("sched: refusing task %s without vm space\n", name);
		return 0;
	}

	const u64 flags = spin_lock_irqsave(&task_table_lock);

	for (u32 i = 0; i < MAX_TASKS; i++) {
		if (tasks[i].pid != 0) {
			continue;
		}

		tasks[i].pid = next_pid++;
		tasks[i].name = name;
		tasks[i].thread_count = 0;
		tasks[i].vm_space = vm_space;
		tasks[i].reply_port = 0;
		spinlock_init(&tasks[i].lock, "task");
		for (u32 handle = 0; handle < MAX_TASK_HANDLES; handle++) {
			tasks[i].handles[handle].type = TASK_HANDLE_EMPTY;
			tasks[i].handles[handle].rights = 0;
			tasks[i].handles[handle].object = 0;
		}
		console_printf("sched: task pid=%u name=%s vm=%u\n",
			       tasks[i].pid, name, tasks[i].vm_space->id);
		spin_unlock_irqrestore(&task_table_lock, flags);
		return &tasks[i];
	}

	spin_unlock_irqrestore(&task_table_lock, flags);
	console_printf("sched: task table full for %s\n", name);
	return 0;
}

struct vm_space *task_vm_space(struct task *task)
{
	return task != 0 ? task->vm_space : 0;
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
	const u64 thread_flags = spin_lock_irqsave(&thread_table_lock);

	for (u32 i = 0; i < MAX_THREADS; i++) {
		if (threads[i].state != THREAD_EMPTY) {
			continue;
		}

		threads[i].tid = next_tid++;
		threads[i].name = name;
		threads[i].task = task;
		threads[i].entry = entry;
		threads[i].arg = arg;
		threads[i].run_next = 0;
		arch_thread_context_init(&threads[i].context,
					 threads[i].kernel_stack + KERNEL_STACK_SIZE,
					 sched_thread_bootstrap);
		task->thread_count++;
		console_printf("sched: thread tid=%u task=%u name=%s\n",
			       threads[i].tid, task->pid, name);
		console_printf("sched: place tid=%u cpu=%u policy=%s\n",
			       threads[i].tid, cpu_id, placement_policy);
		spin_unlock_irqrestore(&thread_table_lock, thread_flags);
		spin_unlock_irqrestore(&task->lock, task_flags);
		sched_enqueue_on(&cpus[cpu_id], &threads[i]);
		return &threads[i];
	}

	spin_unlock_irqrestore(&thread_table_lock, thread_flags);
	spin_unlock_irqrestore(&task->lock, task_flags);
	console_printf("sched: thread table full for %s\n", name);
	return 0;
}

struct thread *thread_create(struct task *task, const char *name,
			     thread_entry_t entry, void *arg)
{
	return thread_create_placed(task, name, entry, arg,
				    sched_select_auto_cpu(), "auto");
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

	const u64 flags = spin_lock_irqsave(&task->lock);

	for (u32 i = 0; i < MAX_TASK_HANDLES; i++) {
		if (task->handles[i].type == TASK_HANDLE_PORT &&
		    task->handles[i].object == port &&
		    task->handles[i].rights == rights) {
			spin_unlock_irqrestore(&task->lock, flags);
			return i + 1;
		}
	}

	for (u32 i = 0; i < MAX_TASK_HANDLES; i++) {
		if (task->handles[i].type != TASK_HANDLE_EMPTY) {
			continue;
		}

		task->handles[i].type = TASK_HANDLE_PORT;
		task->handles[i].rights = rights;
		task->handles[i].object = port;
		console_printf("sched: grant task=%u handle=%u type=port rights=0x%x\n",
			       task->pid, i + 1, rights);
		spin_unlock_irqrestore(&task->lock, flags);
		return i + 1;
	}

	spin_unlock_irqrestore(&task->lock, flags);
	console_printf("sched: handle table full task=%u\n", task->pid);
	return 0;
}

u64 task_grant_task(struct task *owner, struct task *target, u32 rights)
{
	if (owner == 0 || target == 0 || rights == 0) {
		return 0;
	}

	const u64 flags = spin_lock_irqsave(&owner->lock);

	for (u32 i = 0; i < MAX_TASK_HANDLES; i++) {
		if (owner->handles[i].type == TASK_HANDLE_TASK &&
		    owner->handles[i].object == target &&
		    owner->handles[i].rights == rights) {
			spin_unlock_irqrestore(&owner->lock, flags);
			return i + 1;
		}
	}

	for (u32 i = 0; i < MAX_TASK_HANDLES; i++) {
		if (owner->handles[i].type != TASK_HANDLE_EMPTY) {
			continue;
		}

		owner->handles[i].type = TASK_HANDLE_TASK;
		owner->handles[i].rights = rights;
		owner->handles[i].object = target;
		console_printf("sched: grant task=%u handle=%u type=task rights=0x%x target=%u\n",
			       owner->pid, i + 1, rights, target->pid);
		spin_unlock_irqrestore(&owner->lock, flags);
		return i + 1;
	}

	spin_unlock_irqrestore(&owner->lock, flags);
	console_printf("sched: handle table full task=%u\n", owner->pid);
	return 0;
}

struct task *task_from_handle(struct task *owner, u64 handle, u32 rights)
{
	if (owner == 0 || handle == 0 || handle > MAX_TASK_HANDLES) {
		return 0;
	}

	const u64 flags = spin_lock_irqsave(&owner->lock);
	const struct task_handle task_handle = owner->handles[handle - 1];

	if (task_handle.type != TASK_HANDLE_TASK ||
	    (task_handle.rights & rights) != rights) {
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
	if (src == 0 || handle == 0 || handle > MAX_TASK_HANDLES ||
	    rights == 0) {
		return -1;
	}

	const u64 flags = spin_lock_irqsave(&src->lock);
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
	if (dst == 0 || task_can_inherit_handle(src, handle, rights) != 0) {
		return 0;
	}

	const u64 flags = spin_lock_irqsave(&src->lock);
	const struct task_handle src_handle = src->handles[handle - 1];
	spin_unlock_irqrestore(&src->lock, flags);

	if (src_handle.type == TASK_HANDLE_PORT) {
		return task_grant_port(dst, (struct ipc_port *)src_handle.object,
				       rights);
	}

	return 0;
}

struct ipc_port *task_port_from_handle(struct task *task, u64 handle,
				       u32 rights)
{
	if (task == 0 || handle == 0 || handle > MAX_TASK_HANDLES) {
		return 0;
	}

	const u64 flags = spin_lock_irqsave(&task->lock);
	const struct task_handle task_handle = task->handles[handle - 1];

	if (task_handle.type != TASK_HANDLE_PORT ||
	    (task_handle.rights & rights) != rights) {
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

int task_close_handle(struct task *task, u64 handle)
{
	if (task == 0 || handle == 0 || handle > MAX_TASK_HANDLES) {
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

	task_handle->type = TASK_HANDLE_EMPTY;
	task_handle->rights = 0;
	task_handle->object = 0;
	spin_unlock_irqrestore(&task->lock, flags);

	console_printf("sched: close task=%u handle=%u type=%s rights=0x%x\n",
		       task->pid, (u32)handle,
		       type == TASK_HANDLE_PORT ? "port" :
		       type == TASK_HANDLE_TASK ? "task" : "unknown", rights);
	return 0;
}

struct ipc_port *task_reply_port(struct task *task)
{
	if (task == 0) {
		return 0;
	}

	const u64 flags = spin_lock_irqsave(&task->lock);

	if (task->reply_port == 0) {
		task->reply_port = ipc_port_create_private("reply");
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
		next->state = THREAD_RUNNING;
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
	__asm__ volatile ("cli" ::: "memory");

	const u64 flags = spin_lock_irqsave(&cpu->runq.lock);
	if (cpu->runq.count == 0) {
		cpu->runq.idle = 1;
		spin_unlock_irqrestore(&cpu->runq.lock, flags);
		__asm__ volatile ("sti; hlt" ::: "memory");
		return;
	}

	cpu->runq.idle = 0;
	spin_unlock_irqrestore(&cpu->runq.lock, flags);
	__asm__ volatile ("sti" ::: "memory");
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

	if (cpu->quantum_left > 1) {
		cpu->quantum_left--;
		return;
	}

	console_printf("sched: preempt tid=%u cpu=%u\n", prev->tid, cpu->id);
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

	console_printf("sched: block tid=%u cpu=%u\n", prev->tid, cpu->id);
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

	const u64 flags = spin_lock_irqsave(&sleep_lock);
	prev->wake_tick = timer_ticks() + ticks;
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

	sched_enqueue_on(&cpus[thread->cpu_id], thread);
}

void thread_exit(void)
{
	struct cpu_sched *cpu = sched_current_cpu();
	struct thread *prev = cpu->current;

	console_printf("sched: thread tid=%u exited\n", prev->tid);
	prev->state = THREAD_DEAD;
	cpu->reap = prev;
	cpu->current = &cpu->scheduler_thread;
	sched_activate_thread_space(cpu->current);
	arch_thread_switch(&prev->context, &cpu->scheduler_thread.context);

	for (;;) {
		__asm__ volatile ("hlt");
	}
}

u32 task_id(const struct task *task)
{
	return task != 0 ? task->pid : 0;
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
