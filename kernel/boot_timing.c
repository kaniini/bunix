#include "boot_timing.h"
#include "spinlock.h"
#include "timer.h"

enum {
	BOOT_TIMING_MAX_EVENTS = 128,
};

static struct spinlock boot_timing_lock = SPINLOCK_INIT("boot-timing");
static struct boot_timing_event boot_timing_events[BOOT_TIMING_MAX_EVENTS];
static u64 boot_timing_event_count;

void boot_timing_init(void)
{
	const u64 flags = spin_lock_irqsave(&boot_timing_lock);

	boot_timing_event_count = 0;
	for (u64 i = 0; i < BOOT_TIMING_MAX_EVENTS; i++) {
		boot_timing_events[i] = (struct boot_timing_event){ 0 };
	}
	spin_unlock_irqrestore(&boot_timing_lock, flags);
}

int boot_timing_record(const char *name)
{
	struct boot_timing_event event = { 0 };
	u64 len = 0;

	if (name == 0) {
		return -1;
	}
	while (len + 1 < BOOT_TIMING_NAME_BYTES && name[len] != '\0') {
		const char c = name[len];

		if (c == '\n' || c == '\r') {
			break;
		}
		event.name[len++] = c;
	}
	while (len > 0 &&
	       (event.name[len - 1] == ' ' || event.name[len - 1] == '\t')) {
		event.name[--len] = '\0';
	}
	if (len == 0) {
		return -1;
	}
	event.time_ns = timer_monotonic_ns();

	const u64 flags = spin_lock_irqsave(&boot_timing_lock);

	if (boot_timing_event_count >= BOOT_TIMING_MAX_EVENTS) {
		spin_unlock_irqrestore(&boot_timing_lock, flags);
		return -1;
	}
	event.index = boot_timing_event_count;
	boot_timing_events[boot_timing_event_count++] = event;
	spin_unlock_irqrestore(&boot_timing_lock, flags);
	return 0;
}

int boot_timing_read(u64 index, struct boot_timing_event *event)
{
	if (event == 0) {
		return -1;
	}

	const u64 flags = spin_lock_irqsave(&boot_timing_lock);

	if (index >= boot_timing_event_count) {
		spin_unlock_irqrestore(&boot_timing_lock, flags);
		return -1;
	}
	*event = boot_timing_events[index];
	spin_unlock_irqrestore(&boot_timing_lock, flags);
	return 0;
}
