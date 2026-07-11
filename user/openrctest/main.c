#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int ensure_dir(const char *path)
{
	if (mkdir(path, 0755) != 0 && errno != EEXIST) {
		fprintf(stderr, "mkdir %s: %s\n", path, strerror(errno));
		return -1;
	}
	return 0;
}

static int read_exact_file(const char *path, const char *expected)
{
	char buffer[128];
	int fd = open(path, O_RDONLY);
	ssize_t nread;
	const size_t expected_len = strlen(expected);

	if (fd < 0) {
		fprintf(stderr, "open read %s: %s\n", path, strerror(errno));
		return -1;
	}
	nread = read(fd, buffer, sizeof(buffer));
	close(fd);
	if (nread < 0) {
		fprintf(stderr, "read %s: %s\n", path, strerror(errno));
		return -1;
	}
	if ((size_t)nread != expected_len ||
	    memcmp(buffer, expected, expected_len) != 0) {
		fprintf(stderr, "content mismatch %s len=%zd expected=%zu\n",
			path, nread, expected_len);
		return -1;
	}
	return 0;
}

static int write_with_fdopen(const char *path, const char *text)
{
	int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	FILE *file;

	if (fd < 0) {
		fprintf(stderr, "open write %s: %s\n", path, strerror(errno));
		return -1;
	}
	file = fdopen(fd, "w");
	if (file == NULL) {
		fprintf(stderr, "fdopen %s: %s\n", path, strerror(errno));
		close(fd);
		return -1;
	}
	if (fputs(text, file) < 0 || fflush(file) != 0) {
		fprintf(stderr, "stdio write %s: %s\n", path, strerror(errno));
		fclose(file);
		return -1;
	}
	if (fclose(file) != 0) {
		fprintf(stderr, "fclose %s: %s\n", path, strerror(errno));
		return -1;
	}
	return 0;
}

int main(void)
{
	const char *deptree = "/run/openrc/deptree";
	const char *depconfig = "/run/openrc/depconfig";

	if (ensure_dir("/run") != 0 ||
	    ensure_dir("/run/openrc") != 0) {
		return 1;
	}
	if (write_with_fdopen(deptree, "first\n") != 0 ||
	    read_exact_file(deptree, "first\n") != 0) {
		return 1;
	}
	if (write_with_fdopen(deptree, "second\nline\n") != 0 ||
	    read_exact_file(deptree, "second\nline\n") != 0) {
		return 1;
	}
	if (write_with_fdopen(depconfig, "config\n") != 0 ||
	    read_exact_file(depconfig, "config\n") != 0) {
		return 1;
	}
	puts("openrc runtime state ok");
	return 0;
}
