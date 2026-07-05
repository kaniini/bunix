#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

static int expect_enoent(const char *name, int result)
{
	if (result != -1 || errno != ENOENT) {
		printf("linux patherr %s result=%d errno=%d\n",
		       name, result, errno);
		return -1;
	}
	return 0;
}

int main(void)
{
	struct stat st;
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

	puts("linux patherr ok");
	return 0;
}
