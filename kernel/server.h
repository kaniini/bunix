#ifndef BUNIXOS_SERVER_H
#define BUNIXOS_SERVER_H

#include "types.h"

typedef void (*server_entry_t)(void);

struct task;

struct server {
	const char *name;
	server_entry_t start;
};

void server_start_all(void);
void server_start_boot_modules(u64 multiboot_info);
int server_launch_module(const char *name);
u64 server_launch_module_with_caps(const char *name, struct task *parent,
				   const u64 *handles, u64 handle_count);
void vm_server_start(void);

#endif
