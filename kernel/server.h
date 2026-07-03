#ifndef BUNIXOS_SERVER_H
#define BUNIXOS_SERVER_H

#include "types.h"

typedef void (*server_entry_t)(void);

struct server {
	const char *name;
	server_entry_t start;
};

void server_start_all(void);
void server_start_boot_modules(u64 multiboot_info);
void hello_server_start(void);
void ping_server_start(void);

#endif
