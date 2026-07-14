#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/uio.h>
#include <unistd.h>

int main(void)
{
	struct iovec iov[32];
	struct iovec riov[3];
	struct iovec bad;
	char bytes[32];
	char read_left[3] = { 0, 0, 0 };
	char read_right[7] = { 0, 0, 0, 0, 0, 0, 0 };
	int pipefds[2];
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
	if (pipe(pipefds) != 0) {
		perror("iovtest pipe");
		return 1;
	}
	if (write(pipefds[1], "readv-ok", 8) != 8) {
		perror("iovtest pipe write");
		return 1;
	}
	riov[0].iov_base = read_left;
	riov[0].iov_len = 2;
	riov[1].iov_base = bytes;
	riov[1].iov_len = 0;
	riov[2].iov_base = read_right;
	riov[2].iov_len = 6;
	result = readv(pipefds[0], riov, 3);
	if (result != 8 || strcmp(read_left, "re") != 0 ||
	    strcmp(read_right, "adv-ok") != 0) {
		printf("iovtest readv result=%ld left=%s right=%s errno=%d\n",
		       (long)result, read_left, read_right, errno);
		return 1;
	}

	result = readv(pipefds[0], 0, 0);
	if (result != 0) {
		printf("iovtest readv0 result=%ld errno=%d\n",
		       (long)result, errno);
		return 1;
	}

	if (write(pipefds[1], "x", 1) != 1) {
		perror("iovtest pipe bad write");
		return 1;
	}
	errno = 0;
	bad.iov_base = 0;
	bad.iov_len = 1;
	result = readv(pipefds[0], &bad, 1);
	if (result != -1 || errno != EFAULT) {
		printf("iovtest readv bad result=%ld errno=%d\n",
		       (long)result, errno);
		return 1;
	}
	close(pipefds[0]);
	close(pipefds[1]);
	printf("linux iovtest ok\n");
	return 0;
}
