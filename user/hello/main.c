#include <bunix/syscall.h>

int main(void)
{
	const char message[] = "hello: world <3\n";

	bunix_name_register("hello");
	bunix_console_write(message, sizeof(message) - 1);

	return 0;
}
