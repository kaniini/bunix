#include "../../kernel/console.h"
#include "../../kernel/sched.h"

void ping_server_start(void)
{
	console_printf("ping: one\n");
	thread_yield();
	console_printf("ping: two\n");
}
