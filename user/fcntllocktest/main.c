#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

enum {
	LINUX_SYS_CLOSE_RANGE = 436,
	LINUX_CLOSE_RANGE_CLOEXEC = 1 << 2,
};

static int expect_ok(int result, const char *what)
{
	if (result == -1) {
		fprintf(stderr, "%s: %s\n", what, strerror(errno));
		return 1;
	}
	return 0;
}

static int expect_badf(int result, const char *what)
{
	if (result != -1 || errno != EBADF) {
		fprintf(stderr, "%s: result=%d errno=%d\n",
			what, result, errno);
		return 1;
	}
	return 0;
}

static int test_close_range(int base_fd)
{
	int cloexec_fd;
	int close_fd;
	int failed = 0;

	cloexec_fd = dup(base_fd);
	close_fd = dup(base_fd);
	if (cloexec_fd < 0 || close_fd < 0) {
		perror("dup close_range");
		if (cloexec_fd >= 0) {
			close(cloexec_fd);
		}
		if (close_fd >= 0) {
			close(close_fd);
		}
		return 1;
	}

	failed |= expect_ok((int)syscall(LINUX_SYS_CLOSE_RANGE, cloexec_fd,
					cloexec_fd, LINUX_CLOSE_RANGE_CLOEXEC),
			    "close_range CLOEXEC");
	if (fcntl(cloexec_fd, F_GETFD) != FD_CLOEXEC) {
		fprintf(stderr, "close_range CLOEXEC flag missing\n");
		failed = 1;
	}

	failed |= expect_ok((int)syscall(LINUX_SYS_CLOSE_RANGE, close_fd,
					close_fd, 0),
			    "close_range close");
	errno = 0;
	failed |= expect_badf(fcntl(close_fd, F_GETFD), "closed fd F_GETFD");

	close(cloexec_fd);
	return failed;
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
	failed |= test_close_range(fd);

	close(fd);
	if (failed != 0) {
		return 1;
	}
	puts("linux fcntl and close_range ok");
	return 0;
}
