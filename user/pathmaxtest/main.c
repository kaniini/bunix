#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

enum {
	PATHMAX_TEST_PATH = 4096,
	TARGET_PATH_LEN = 3600,
};

static int append_segment(char *path, size_t *len)
{
	static const char segment[] =
		"/segment-aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
		"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
		"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
		"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
	const size_t segment_len = sizeof(segment) - 1;

	if (*len + segment_len >= PATHMAX_TEST_PATH) {
		errno = ENAMETOOLONG;
		return -1;
	}
	memcpy(path + *len, segment, segment_len + 1);
	*len += segment_len;
	return 0;
}

static int write_payload(const char *path)
{
	static const char payload[] = "PATHMAX4096_PAYLOAD";
	char file[PATHMAX_TEST_PATH];
	char buf[sizeof(payload)];
	struct stat st;
	size_t len = strlen(path);
	int fd;
	ssize_t n;

	if (len + sizeof("/file.txt") >= sizeof(file)) {
		errno = ENAMETOOLONG;
		return -1;
	}
	memcpy(file, path, len);
	memcpy(file + len, "/file.txt", sizeof("/file.txt"));

	fd = open(file, O_CREAT | O_TRUNC | O_WRONLY, 0644);
	if (fd < 0) {
		perror("pathmaxtest open-create");
		return -1;
	}
	if (write(fd, payload, sizeof(payload)) != (ssize_t)sizeof(payload)) {
		perror("pathmaxtest write");
		close(fd);
		return -1;
	}
	if (close(fd) != 0) {
		perror("pathmaxtest close-write");
		return -1;
	}

	if (stat(file, &st) != 0) {
		perror("pathmaxtest stat");
		return -1;
	}
	if (st.st_size != (off_t)sizeof(payload)) {
		printf("pathmaxtest stat size=%lld expected=%llu\n",
		       (long long)st.st_size,
		       (unsigned long long)sizeof(payload));
		return -1;
	}

	fd = open(file, O_RDONLY);
	if (fd < 0) {
		perror("pathmaxtest open-read");
		return -1;
	}
	n = read(fd, buf, sizeof(buf));
	if (close(fd) != 0 || n != (ssize_t)sizeof(buf) ||
	    memcmp(buf, payload, sizeof(payload)) != 0) {
		perror("pathmaxtest read");
		return -1;
	}

	return 0;
}

int main(void)
{
	char path[PATHMAX_TEST_PATH] = "/tmp/bunix-pathmax4096";
	char cwd[PATHMAX_TEST_PATH];
	size_t len = strlen(path);

	if (mkdir(path, 0755) != 0 && errno != EEXIST) {
		perror("pathmaxtest mkdir root");
		return 1;
	}

	while (len < TARGET_PATH_LEN) {
		if (append_segment(path, &len) != 0) {
			perror("pathmaxtest append");
			return 1;
		}
		if (mkdir(path, 0755) != 0 && errno != EEXIST) {
			perror("pathmaxtest mkdir");
			return 1;
		}
	}

	if (write_payload(path) != 0) {
		perror("pathmaxtest file");
		return 1;
	}
	if (chdir(path) != 0) {
		perror("pathmaxtest chdir");
		return 1;
	}
	if (getcwd(cwd, sizeof(cwd)) == NULL || strcmp(cwd, path) != 0) {
		perror("pathmaxtest getcwd");
		return 1;
	}

	puts("linux pathmax ok");
	return 0;
}
