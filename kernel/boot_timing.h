#ifndef BUNIXOS_BOOT_TIMING_H
#define BUNIXOS_BOOT_TIMING_H

#include "types.h"

enum {
	BOOT_TIMING_NAME_BYTES = 64,
};

struct boot_timing_event {
	u64 index;
	u64 time_ns;
	char name[BOOT_TIMING_NAME_BYTES];
};

void boot_timing_init(void);
int boot_timing_record(const char *name);
int boot_timing_read(u64 index, struct boot_timing_event *event);

#endif
