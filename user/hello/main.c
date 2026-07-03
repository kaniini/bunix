#include <bunix/syscall.h>

int main(void)
{
	const char message[] = "hello: world <3\n";
	const u64 console = (u64)bunix_name_lookup("console");

	bunix_name_register("hello");
	bunix_service_write(console, message, sizeof(message) - 1);

	return 0;
}
