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

void sched_init(void);
struct task *task_create(const char *name);
struct thread *thread_create(struct task *task, const char *name,
			     thread_entry_t entry, void *arg);
void sched_run(void);
void thread_yield(void);
void thread_block(void);
void thread_unblock(struct thread *thread);
void thread_exit(void) __attribute__((noreturn));

u32 task_id(const struct task *task);
u32 thread_id(const struct thread *thread);
enum thread_state thread_state(const struct thread *thread);
const char *task_name(const struct task *task);
const char *thread_name(const struct thread *thread);

#endif
