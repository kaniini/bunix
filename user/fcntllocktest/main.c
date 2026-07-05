#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int expect_ok(int result, const char *what)
{
	if (result == -1) {
		fprintf(stderr, "%s: %s\n", what, strerror(errno));
		return 1;
	}
	return 0;
}

int main(void)
{
	struct flock lock;
	int fd;
	int failed = 0;

	fd = open("/tmp/bunix-fcntl-lock.txt", O_CREAT | O_RDWR | O_TRUNC, 0644);
	if (fd < 0) {
		perror("open");
		return 1;
	}
	if (write(fd, "lock", 4) != 4) {
		perror("write");
		close(fd);
		return 1;
	}

	memset(&lock, 0, sizeof(lock));
	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0;
	failed |= expect_ok(fcntl(fd, F_SETLK, &lock), "F_SETLK");
	failed |= expect_ok(fcntl(fd, F_SETLKW, &lock), "F_SETLKW");

	lock.l_type = F_WRLCK;
	failed |= expect_ok(fcntl(fd, F_GETLK, &lock), "F_GETLK");
	if (lock.l_type != F_UNLCK) {
		fprintf(stderr, "F_GETLK: l_type=%d\n", lock.l_type);
		failed = 1;
	}

	close(fd);
	if (failed != 0) {
		return 1;
	}
	puts("linux fcntl locks ok");
	return 0;
}
