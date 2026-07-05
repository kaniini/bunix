#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

int main(void)
{
	const char *path = "/tmp/fchmodat-r10-test";
	struct stat st;
	int fd;
	long result;

	fd = open(path, O_CREAT | O_TRUNC | O_WRONLY, 0644);
	if (fd < 0) {
		perror("fchmodattest open");
		return 1;
	}
	if (close(fd) != 0) {
		perror("fchmodattest close");
		return 1;
	}

	errno = 0;
	result = syscall(SYS_fchmodat, AT_FDCWD, path, 0600, 0xdeadbeefUL);
	if (result != 0) {
		printf("fchmodattest fchmodat result=%ld errno=%d\n",
		       result, errno);
		return 1;
	}
	if (stat(path, &st) != 0) {
		perror("fchmodattest stat");
		return 1;
	}
	if ((st.st_mode & 0777) != 0600) {
		printf("fchmodattest mode=%o\n", (unsigned)(st.st_mode & 0777));
		return 1;
	}

	printf("linux fchmodat checks ok\n");
	return 0;
}
