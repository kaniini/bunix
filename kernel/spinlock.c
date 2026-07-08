#include <arch/interrupts.h>
#include "spinlock.h"

void spinlock_init(struct spinlock *lock, const char *name)
{
	lock->locked = 0;
	lock->name = name;
}

u64 spin_lock_irqsave(struct spinlock *lock)
{
	const u64 flags = arch_interrupts_save();

	while (__sync_lock_test_and_set(&lock->locked, 1) != 0) {
		while (lock->locked != 0) {
		}
	}

	__sync_synchronize();
	return flags;
}

void spin_unlock_irqrestore(struct spinlock *lock, u64 flags)
{
	__sync_synchronize();
	__sync_lock_release(&lock->locked);
	arch_interrupts_restore(flags);
}
