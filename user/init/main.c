#include <bunix/syscall.h>

int main(void)
{
	const char launching[] = "init: launching servers\n";
	const char done[] = "init: done\n";
	const u64 console = (u64)bunix_name_lookup("console");

	bunix_name_register("init");
	bunix_service_write(console, launching, sizeof(launching) - 1);
	bunix_launch_module("hello");
	bunix_launch_module("ping");
	bunix_service_write(console, done, sizeof(done) - 1);

	return 0;
}
