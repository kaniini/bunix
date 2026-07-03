#ifndef BUNIXOS_VM_SERVER_H
#define BUNIXOS_VM_SERVER_H

#include "../../kernel/vm.h"

enum {
	VM_IPC_EVENT_PING = 1,
	VM_RPC_CREATE_SPACE = 2,
	VM_RPC_ALLOC_FRAME = 3,
	VM_RPC_FREE_FRAME = 4,
};

void vm_server_init(void);
struct vm_space *vm_server_bootstrap_space(const char *owner);
struct vm_space *vm_server_rpc_create_space(const char *owner);
void vm_server_start(void);

#endif
