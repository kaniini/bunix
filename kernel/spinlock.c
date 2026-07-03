#include "spinlock.h"

static inline u64 irq_save(void)
{
	u64 flags;

	__asm__ volatile ("pushfq; popq %0; cli" : "=r"(flags) : : "memory");
	return flags;
}

static inline void irq_restore(u64 flags)
{
	if ((flags & (1 << 9)) != 0) {
		__asm__ volatile ("sti" : : : "memory");
	}
}

void spinlock_init(struct spinlock *lock, const char *name)
{
	lock->locked = 0;
	lock->name = name;
}

u64 spin_lock_irqsave(struct spinlock *lock)
{
	const u64 flags = irq_save();

	while (__sync_lock_test_and_set(&lock->locked, 1) != 0) {
		while (lock->locked != 0) {
			__asm__ volatile ("pause");
		}
	}

	__sync_synchronize();
	return flags;
}

void spin_unlock_irqrestore(struct spinlock *lock, u64 flags)
{
	__sync_synchronize();
	__sync_lock_release(&lock->locked);
	irq_restore(flags);
}
