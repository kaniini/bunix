#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

enum {
	LINUX_STATX_BASIC_STATS = 0x7ff,
	LINUX_STATX_BUFFER_SIZE = 256,
};

static int expect_enoent(const char *name, int result)
{
	if (result != -1 || errno != ENOENT) {
		printf("linux patherr %s result=%d errno=%d\n",
		       name, result, errno);
		return -1;
	}
	return 0;
}

static int expect_errno(const char *name, int result, int expected)
{
	if (result != -1 || errno != expected) {
		printf("linux patherr %s result=%d errno=%d expected=%d\n",
		       name, result, errno, expected);
		return -1;
	}
	return 0;
}

int main(void)
{
	struct stat st;
	unsigned char statx_buf[LINUX_STATX_BUFFER_SIZE];
	char link_buf[16];
	int fd;

	errno = 0;
	fd = open("", O_RDONLY);
	if (expect_enoent("open-empty", fd) != 0) {
		if (fd >= 0) {
			close(fd);
		}
		return 1;
	}

	errno = 0;
	if (expect_enoent("stat-empty", stat("", &st)) != 0) {
		return 1;
	}

	errno = 0;
	if (expect_enoent("chdir-empty", chdir("")) != 0) {
		return 1;
	}

	errno = 0;
	if (expect_enoent("readlink-empty",
			  (int)readlink("", link_buf, sizeof(link_buf))) != 0) {
		return 1;
	}

	errno = 0;
	if (expect_errno("readlink-nonsymlink",
			 (int)readlink("/hello.txt", link_buf,
				       sizeof(link_buf)),
			 EINVAL) != 0) {
		return 1;
	}

	(void)unlink("/tmp/patherr-link");
	if (symlink("/hello.txt", "/tmp/patherr-link") != 0) {
		perror("linux patherr symlink");
		return 1;
	}
	errno = 0;
	fd = open("/tmp/patherr-link", O_RDONLY | O_NOFOLLOW);
	if (expect_errno("open-nofollow-symlink", fd, ELOOP) != 0) {
		if (fd >= 0) {
			close(fd);
		}
		return 1;
	}
	(void)unlink("/tmp/patherr-loop-a");
	(void)unlink("/tmp/patherr-loop-b");
	if (symlink("patherr-loop-b", "/tmp/patherr-loop-a") != 0 ||
	    symlink("patherr-loop-a", "/tmp/patherr-loop-b") != 0) {
		perror("linux patherr loop symlink");
		return 1;
	}
	errno = 0;
	fd = open("/tmp/patherr-loop-a", O_RDONLY);
	if (expect_errno("open-symlink-loop", fd, ELOOP) != 0) {
		if (fd >= 0) {
			close(fd);
		}
		return 1;
	}

	fd = open("/hello.txt", O_RDONLY);
	if (fd < 0) {
		perror("linux patherr open-fstatat");
		return 1;
	}
	if (fstatat(fd, "", &st, AT_EMPTY_PATH) != 0 || st.st_size != 15) {
		printf("linux patherr fstatat-empty errno=%d size=%llu\n",
		       errno, (unsigned long long)st.st_size);
		close(fd);
		return 1;
	}
	for (unsigned int i = 0; i < sizeof(statx_buf); i++) {
		statx_buf[i] = 0;
	}
	errno = 0;
	if (syscall(SYS_statx, fd, "", AT_EMPTY_PATH,
		    LINUX_STATX_BASIC_STATS, statx_buf) != 0) {
		printf("linux patherr statx-empty errno=%d\n", errno);
		close(fd);
		return 1;
	}
	close(fd);

	puts("linux patherr ok");
	return 0;
}
