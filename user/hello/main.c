#include <bunix/syscall.h>

int main(void)
{
	const char message[] = "hello: world <3\n";
	const u64 tick = bunix_timer_ticks();

	while (bunix_timer_ticks() < tick + 20) {
	}

	bunix_console_write(message, sizeof(message) - 1);

	return 0;
}
