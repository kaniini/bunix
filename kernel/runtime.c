#include "buffer.h"
#include "ipc.h"
#include "name.h"
#include "runtime.h"
#include "sched.h"
#include "slab.h"
#include "vm.h"
#include "../servers/vm/vm_server.h"
#include <arch/user.h>

void kernel_runtime_memory_init(void)
{
	vm_init();
	slab_init();
	buffer_init();
}

void kernel_runtime_services_init(void)
{
	arch_user_init();
	ipc_init();
	name_service_init();
	vm_server_init();
}

void kernel_runtime_scheduler_init(void)
{
	sched_init();
}
