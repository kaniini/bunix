#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/uio.h>
#include <unistd.h>

int main(void)
{
	struct iovec iov[32];
	struct iovec bad;
	char bytes[32];
	int fd;
	ssize_t result;

	fd = open("/dev/null", O_WRONLY);
	if (fd < 0) {
		perror("iovtest open");
		return 1;
	}

	for (size_t i = 0; i < sizeof(bytes); i++) {
		bytes[i] = 'x';
		iov[i].iov_base = &bytes[i];
		iov[i].iov_len = 1;
	}
	result = writev(fd, iov, 32);
	if (result != 32) {
		printf("iovtest writev32 result=%ld errno=%d\n",
		       (long)result, errno);
		return 1;
	}

	result = writev(fd, 0, 0);
	if (result != 0) {
		printf("iovtest writev0 result=%ld errno=%d\n",
		       (long)result, errno);
		return 1;
	}

	errno = 0;
	bad.iov_base = 0;
	bad.iov_len = 1;
	result = writev(fd, &bad, 1);
	if (result != -1 || errno != EFAULT) {
		printf("iovtest bad result=%ld errno=%d\n",
		       (long)result, errno);
		return 1;
	}

	close(fd);
	printf("linux iovtest ok\n");
	return 0;
}
