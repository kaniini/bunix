#ifndef BUNIXOS_VM_SERVER_H
#define BUNIXOS_VM_SERVER_H

#include "../../kernel/vm.h"

enum {
	VM_IPC_EVENT_PING = 1,
};

struct vm_space *vm_server_create_space(const char *owner);
void vm_server_start(void);

#endif
