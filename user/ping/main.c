#include <bunix/syscall.h>

int main(void)
{
	const char one[] = "ping: one\n";
	const char two[] = "ping: two\n";
	const u64 console = (u64)bunix_name_lookup("console");
	const u64 vm = (u64)bunix_name_lookup("vm");
	const u64 tick = bunix_timer_ticks();

	bunix_name_register("ping");
	bunix_service_write(console, one, sizeof(one) - 1);
	bunix_enable_preemption();
	bunix_service_vm_ping(vm, 0x2a);

	while (bunix_timer_ticks() == tick) {
	}

	bunix_service_write(console, two, sizeof(two) - 1);

	return 0;
}
