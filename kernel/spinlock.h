#ifndef BUNIXOS_SPINLOCK_H
#define BUNIXOS_SPINLOCK_H

#include "types.h"

struct spinlock {
	volatile u32 locked;
	const char *name;
};

#define SPINLOCK_INIT(lock_name) { .locked = 0, .name = (lock_name) }

void spinlock_init(struct spinlock *lock, const char *name);
u64 spin_lock_irqsave(struct spinlock *lock);
void spin_unlock_irqrestore(struct spinlock *lock, u64 flags);

#endif
