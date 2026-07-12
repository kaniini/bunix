#include "sched.h"
#include "slab.h"
#include "spinlock.h"
#include "task_lifecycle.h"

struct task_death_hook_node {
	task_death_hook_t hook;
	struct task_death_hook_node *next;
};

static struct spinlock task_lifecycle_lock = SPINLOCK_INIT("task-lifecycle");
static struct task_death_hook_node *death_hooks;

int task_lifecycle_register_death_hook(task_death_hook_t hook)
{
	if (hook == 0) {
		return -1;
	}

	struct task_death_hook_node *node =
		(struct task_death_hook_node *)slab_zalloc(sizeof(*node));
	if (node == 0) {
		return -1;
	}
	node->hook = hook;

	const u64 flags = spin_lock_irqsave(&task_lifecycle_lock);
	for (const struct task_death_hook_node *existing = death_hooks;
	     existing != 0; existing = existing->next) {
		if (existing->hook == hook) {
			spin_unlock_irqrestore(&task_lifecycle_lock, flags);
			slab_free(node);
			return 0;
		}
	}
	node->next = death_hooks;
	death_hooks = node;
	spin_unlock_irqrestore(&task_lifecycle_lock, flags);
	return 0;
}

void task_lifecycle_notify_death(struct task *task)
{
	if (task == 0) {
		return;
	}

	const u32 id = task_id(task);
	const char *name = task_name(task);
	const u64 flags = spin_lock_irqsave(&task_lifecycle_lock);
	struct task_death_hook_node *hooks = death_hooks;
	spin_unlock_irqrestore(&task_lifecycle_lock, flags);

	for (struct task_death_hook_node *node = hooks; node != 0;
	     node = node->next) {
		node->hook(id, name);
	}
}
