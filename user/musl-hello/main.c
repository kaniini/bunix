#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv)
{
	char line[128];
	const int len = snprintf(line, sizeof(line),
				 "musl hello argc=%d argv0=%s\n",
				 argc, argv[0]);

	if (len > 0) {
		write(1, line, (size_t)len < sizeof(line) ?
		      (size_t)len : sizeof(line) - 1);
	}
	_exit(0);
}
