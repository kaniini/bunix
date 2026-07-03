#include "console.h"
#include "sched.h"
#include <arch/thread.h>

enum {
	MAX_CPUS = 1,
	MAX_TASKS = 16,
	MAX_THREADS = 32,
	KERNEL_STACK_SIZE = 16384,
};

struct task {
	u32 pid;
	const char *name;
	u32 thread_count;
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
	struct arch_thread_context context;
	u8 kernel_stack[KERNEL_STACK_SIZE] __attribute__((aligned(16)));
};

struct run_queue {
	struct thread *head;
	struct thread *tail;
	u32 count;
};

struct cpu_sched {
	u32 id;
	struct run_queue runq;
	struct thread *current;
	struct thread scheduler_thread;
};

static struct task tasks[MAX_TASKS];
static struct thread threads[MAX_THREADS];
static struct task kernel_task;
static struct cpu_sched cpus[MAX_CPUS];
static u32 boot_cpu_id;
static u32 next_pid = 1;
static u32 next_tid = 1;

static struct cpu_sched *sched_current_cpu(void)
{
	return &cpus[boot_cpu_id];
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
	thread->cpu_id = cpu->id;
	thread->state = THREAD_READY;
	runq_push(&cpu->runq, thread);
	console_printf("sched: enqueue tid=%u cpu=%u runq=%u\n",
		       thread->tid, cpu->id, cpu->runq.count);
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

	for (u32 i = 0; i < MAX_CPUS; i++) {
		cpus[i].id = i;
		cpus[i].runq.head = 0;
		cpus[i].runq.tail = 0;
		cpus[i].runq.count = 0;

		cpus[i].scheduler_thread.tid = 0;
		cpus[i].scheduler_thread.name = "scheduler";
		cpus[i].scheduler_thread.task = &kernel_task;
		cpus[i].scheduler_thread.state = THREAD_RUNNING;
		cpus[i].scheduler_thread.cpu_id = i;
		cpus[i].current = &cpus[i].scheduler_thread;
	}

	boot_cpu_id = 0;
	console_printf("sched: init cpus=%u boot_cpu=%u\n", MAX_CPUS, boot_cpu_id);
}

struct task *task_create(const char *name)
{
	for (u32 i = 0; i < MAX_TASKS; i++) {
		if (tasks[i].pid != 0) {
			continue;
		}

		tasks[i].pid = next_pid++;
		tasks[i].name = name;
		tasks[i].thread_count = 0;
		console_printf("sched: task pid=%u name=%s\n", tasks[i].pid, name);
		return &tasks[i];
	}

	console_printf("sched: task table full for %s\n", name);
	return 0;
}

struct thread *thread_create(struct task *task, const char *name,
			     thread_entry_t entry, void *arg)
{
	if (task == 0 || entry == 0) {
		return 0;
	}

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
		sched_enqueue_on(sched_current_cpu(), &threads[i]);
		return &threads[i];
	}

	console_printf("sched: thread table full for %s\n", name);
	return 0;
}

void sched_run(void)
{
	struct cpu_sched *cpu = sched_current_cpu();

	for (;;) {
		struct thread *next = runq_pop(&cpu->runq);

		if (next == 0) {
			return;
		}

		struct thread *prev = cpu->current;
		next->state = THREAD_RUNNING;
		cpu->current = next;
		console_printf("sched: switch cpu=%u prev=%u next=%u\n",
			       cpu->id, prev->tid, next->tid);
		arch_thread_switch(&prev->context, &next->context);
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
	arch_thread_switch(&prev->context, &cpu->scheduler_thread.context);
}

void thread_block(void)
{
	struct cpu_sched *cpu = sched_current_cpu();
	struct thread *prev = cpu->current;

	if (prev == &cpu->scheduler_thread) {
		return;
	}

	console_printf("sched: block tid=%u cpu=%u\n", prev->tid, cpu->id);
	prev->state = THREAD_BLOCKED;
	cpu->current = &cpu->scheduler_thread;
	arch_thread_switch(&prev->context, &cpu->scheduler_thread.context);
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
	cpu->current = &cpu->scheduler_thread;
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
