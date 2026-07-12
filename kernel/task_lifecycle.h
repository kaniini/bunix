#ifndef BUNIX_TASK_LIFECYCLE_H
#define BUNIX_TASK_LIFECYCLE_H

#include "types.h"

struct task;

typedef void (*task_death_hook_t)(u32 task_id, const char *task_name);

/*
 * Death hooks are registered during subsystem initialization and are invoked
 * from task teardown after the final thread has died, before the task is
 * removed from the task table.  The scheduler calls hooks with no task,
 * scheduler, VM, or IPC locks held; hooks may take subsystem-private locks but
 * must not retain or restart the dying task.
 *
 * State that is fundamental to scheduling or address-space ownership belongs
 * directly on struct task.  State owned by an optional subsystem or syscall
 * frontend should use a lifecycle hook so teardown does not depend on every
 * possible exit path remembering subsystem-local cleanup.
 */
int task_lifecycle_register_death_hook(task_death_hook_t hook);
void task_lifecycle_notify_death(struct task *task);

#endif
