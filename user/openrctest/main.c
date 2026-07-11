#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

static int ensure_dir(const char *path)
{
	if (mkdir(path, 0755) != 0 && errno != EEXIST) {
		fprintf(stderr, "mkdir %s: %s\n", path, strerror(errno));
		return -1;
	}
	return 0;
}

static FILE *openrc_fopenat(int dirfd, const char *path, int flags)
{
	int fd = openat(dirfd, path, flags, 0666);
	const char *mode;
	FILE *file;

	if (fd < 0) {
		fprintf(stderr, "openat %s: %s\n", path, strerror(errno));
		return NULL;
	}
	if ((flags & O_RDWR) == O_RDWR) {
		mode = "r+";
	} else if ((flags & O_WRONLY) == O_WRONLY) {
		mode = "w";
	} else {
		mode = "r";
	}
	file = fdopen(fd, mode);
	if (file == NULL) {
		fprintf(stderr, "fdopen %s: %s\n", path, strerror(errno));
		close(fd);
	}
	return file;
}

static int open_svcdir(void)
{
	int dirfd = open("/run/openrc", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
	int flags;

	if (dirfd < 0) {
		fprintf(stderr, "open /run/openrc: %s\n", strerror(errno));
		return -1;
	}
	flags = fcntl(dirfd, F_GETFD, 0);
	if (flags < 0) {
		fprintf(stderr, "fcntl F_GETFD /run/openrc: %s\n",
			strerror(errno));
		close(dirfd);
		return -1;
	}
	if ((flags & FD_CLOEXEC) == 0) {
		fprintf(stderr, "missing FD_CLOEXEC on /run/openrc\n");
		close(dirfd);
		return -1;
	}
	return dirfd;
}

static int probe_write_permission_at(int dirfd, const char *path)
{
	int fd = openat(dirfd, path, O_WRONLY);

	if (fd < 0) {
		if (errno == ENOENT) {
			return 0;
		}
		fprintf(stderr, "open write probe %s: %s\n", path,
			strerror(errno));
		return -1;
	}
	if (close(fd) != 0) {
		fprintf(stderr, "close write probe %s: %s\n", path,
			strerror(errno));
		return -1;
	}
	return 0;
}

static int read_exact_file_at(int dirfd, const char *path, const char *expected)
{
	char buffer[128];
	int fd = openat(dirfd, path, O_RDONLY);
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

static int write_with_fdopen_at(int dirfd, const char *path, const char *text)
{
	FILE *file = openrc_fopenat(dirfd, path, O_WRONLY | O_CREAT | O_TRUNC);
	if (file == NULL) {
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

static int run_generator_probe(void)
{
	char line[128];
	FILE *file = popen("printf \"depinfo_2_service='postpopen'\\n\"", "r");
	int status;

	if (file == NULL) {
		fprintf(stderr, "popen generator: %s\n", strerror(errno));
		return -1;
	}
	if (fgets(line, sizeof(line), file) == NULL) {
		fprintf(stderr, "popen generator read: %s\n", strerror(errno));
		pclose(file);
		return -1;
	}
	if (strcmp(line, "depinfo_2_service='postpopen'\n") != 0) {
		fprintf(stderr, "popen generator mismatch: %s\n", line);
		pclose(file);
		return -1;
	}
	status = pclose(file);
	if (status != 0) {
		fprintf(stderr, "pclose generator status=%d errno=%s\n",
			status, strerror(errno));
		return -1;
	}
	return 0;
}

static int read_lines_with_fdopen_at(int dirfd, const char *path)
{
	char line[128];
	FILE *file = openrc_fopenat(dirfd, path, O_RDONLY);
	unsigned int lines = 0;

	if (file == NULL) {
		return -1;
	}
	while (fgets(line, sizeof(line), file) != NULL) {
		lines++;
	}
	if (ferror(file)) {
		fprintf(stderr, "fgets %s: %s\n", path, strerror(errno));
		fclose(file);
		return -1;
	}
	if (fclose(file) != 0) {
		fprintf(stderr, "fclose read %s: %s\n", path, strerror(errno));
		return -1;
	}
	if (lines == 0) {
		fprintf(stderr, "read no lines from %s\n", path);
		return -1;
	}
	return 0;
}

static int check_stat_at(int dirfd, const char *path)
{
	struct stat st;

	if (fstatat(dirfd, path, &st, 0) != 0) {
		fprintf(stderr, "fstatat %s: %s\n", path, strerror(errno));
		return -1;
	}
	if (!S_ISREG(st.st_mode) || st.st_size <= 0) {
		fprintf(stderr, "bad stat %s mode=%o size=%lld\n", path,
			(unsigned int)st.st_mode, (long long)st.st_size);
		return -1;
	}
	return 0;
}

static int adjust_mtime_at(int dirfd, const char *path)
{
	struct timespec ts[2] = {
		{ .tv_sec = 1767225621, .tv_nsec = 0 },
		{ .tv_sec = 1767225621, .tv_nsec = 0 },
	};

	if (utimensat(dirfd, path, ts, 0) != 0) {
		fprintf(stderr, "utimensat %s: %s\n", path, strerror(errno));
		return -1;
	}
	return 0;
}

static int unlink_if_present_at(int dirfd, const char *path)
{
	if (unlinkat(dirfd, path, 0) == 0 || errno == ENOENT) {
		return 0;
	}
	fprintf(stderr, "unlinkat %s: %s\n", path, strerror(errno));
	return -1;
}

int main(void)
{
	const char *deptree = "deptree";
	const char *depconfig = "depconfig";
	const char *clock_skewed = "clock-skewed";
	int dirfd;

	if (ensure_dir("/run") != 0 ||
	    ensure_dir("/run/openrc") != 0) {
		return 1;
	}
	dirfd = open_svcdir();
	if (dirfd < 0) {
		return 1;
	}

	for (unsigned int i = 0; i < 8; i++) {
		if (probe_write_permission_at(dirfd, deptree) != 0 ||
		    run_generator_probe() != 0 ||
		    write_with_fdopen_at(
			    dirfd, deptree,
			    "depinfo_0_service='networking'\n"
			    "depinfo_0_need_0='localmount'\n"
			    "depinfo_1_service='localmount'\n") != 0 ||
		    check_stat_at(dirfd, deptree) != 0 ||
		    adjust_mtime_at(dirfd, deptree) != 0 ||
		    write_with_fdopen_at(dirfd, clock_skewed,
					 "/etc/init.d/networking\n") != 0 ||
		    read_exact_file_at(dirfd, clock_skewed,
				       "/etc/init.d/networking\n") != 0 ||
		    unlink_if_present_at(dirfd, clock_skewed) != 0 ||
		    read_exact_file_at(
			    dirfd, deptree,
			    "depinfo_0_service='networking'\n"
			    "depinfo_0_need_0='localmount'\n"
			    "depinfo_1_service='localmount'\n") != 0 ||
		    read_lines_with_fdopen_at(dirfd, deptree) != 0) {
			close(dirfd);
			return 1;
		}
		close(dirfd);
		dirfd = open_svcdir();
		if (dirfd < 0) {
			return 1;
		}
		if ((i & 1) == 0) {
			if (write_with_fdopen_at(dirfd, depconfig,
						 "/etc/init.d/networking\n") != 0 ||
			    read_exact_file_at(dirfd, depconfig,
					       "/etc/init.d/networking\n") != 0) {
				close(dirfd);
				return 1;
			}
		} else if (unlinkat(dirfd, depconfig, 0) != 0) {
			fprintf(stderr, "unlinkat %s: %s\n", depconfig,
				strerror(errno));
			close(dirfd);
			return 1;
		} else {
			struct stat st;

			if (fstatat(dirfd, depconfig, &st, 0) == 0 ||
			    errno != ENOENT) {
				fprintf(stderr,
					"depconfig unlink check errno=%d\n",
					errno);
				close(dirfd);
				return 1;
			}
		}
	}
	if (close(dirfd) != 0) {
		perror("close /run/openrc");
		return 1;
	}
	puts("openrc runtime state ok");
	return 0;
}
