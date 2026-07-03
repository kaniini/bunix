#ifndef BUNIXOS_NAME_H
#define BUNIXOS_NAME_H

#include "types.h"

enum name_service_kind {
	NAME_SERVICE_EMPTY = 0,
	NAME_SERVICE_CONSOLE = 1,
	NAME_SERVICE_IPC_PORT = 2,
	NAME_SERVICE_TASK = 3,
};

void name_service_init(void);
u64 name_service_register(const char *name, enum name_service_kind kind,
			  u64 object);
u64 name_service_lookup(const char *name);
enum name_service_kind name_service_kind(u64 id);
u64 name_service_object(u64 id);

#endif
