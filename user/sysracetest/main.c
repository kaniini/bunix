#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

enum {
	WORKERS = 4,
	ITERATIONS = 12,
	PATH_SIZE = 512,
	ARGV_COUNT = 96,
	ENV_COUNT = 96,
};

static int write_file(const char *path, const char *payload)
{
	const size_t len = strlen(payload);
	const int fd = open(path, O_CREAT | O_TRUNC | O_WRONLY, 0644);

	if (fd < 0) {
		perror("sysracetest open-write");
		return -1;
	}
	if (write(fd, payload, len) != (ssize_t)len) {
		perror("sysracetest write");
		close(fd);
		return -1;
	}
	if (close(fd) != 0) {
		perror("sysracetest close-write");
		return -1;
	}
	return 0;
}

static int read_file_check(const char *path, const char *expected)
{
	char buf[128];
	const size_t expected_len = strlen(expected);
	const int fd = open(path, O_RDONLY);
	ssize_t n;

	if (fd < 0) {
		perror("sysracetest open-read");
		return -1;
	}
	n = read(fd, buf, sizeof(buf));
	if (close(fd) != 0) {
		perror("sysracetest close-read");
		return -1;
	}
	if (n != (ssize_t)expected_len ||
	    memcmp(buf, expected, expected_len) != 0) {
		fprintf(stderr, "sysracetest read mismatch path=%s n=%ld\n",
			path, (long)n);
		return -1;
	}
	return 0;
}

static int stress_open_stat(int worker, int iter)
{
	char path[PATH_SIZE];
	char payload[64];
	struct stat st;
	int fd;

	snprintf(path, sizeof(path), "/tmp/sysrace-%d-open-%d.txt",
		 worker, iter);
	snprintf(payload, sizeof(payload), "sysrace-open-%d-%d\n",
		 worker, iter);

	if (write_file(path, payload) != 0) {
		return -1;
	}
	if (stat(path, &st) != 0 || st.st_size != (off_t)strlen(payload)) {
		perror("sysracetest stat");
		return -1;
	}
	fd = open("/hello.txt", O_RDONLY);
	if (fd < 0) {
		perror("sysracetest open-rootfs");
		return -1;
	}
	if (close(fd) != 0) {
		perror("sysracetest close-rootfs");
		return -1;
	}
	return read_file_check(path, payload);
}

static int stress_symlink_readlink(int worker, int iter)
{
	char target[PATH_SIZE];
	char link_path[PATH_SIZE];
	char out[PATH_SIZE];
	ssize_t n;

	snprintf(target, sizeof(target), "/tmp/sysrace-%d-target-%d.txt",
		 worker, iter);
	snprintf(link_path, sizeof(link_path), "/tmp/sysrace-%d-link-%d",
		 worker, iter);

	unlink(link_path);
	if (write_file(target, "sysrace-symlink\n") != 0) {
		return -1;
	}
	if (symlink(target, link_path) != 0) {
		perror("sysracetest symlink");
		return -1;
	}
	n = readlink(link_path, out, sizeof(out) - 1);
	if (n < 0) {
		perror("sysracetest readlink");
		return -1;
	}
	out[n] = '\0';
	if (strcmp(out, target) != 0) {
		fprintf(stderr, "sysracetest readlink mismatch %s != %s\n",
			out, target);
		return -1;
	}
	return read_file_check(target, "sysrace-symlink\n");
}

static int stress_rename_link(int worker, int iter)
{
	char old_path[PATH_SIZE];
	char new_path[PATH_SIZE];
	char link_src[PATH_SIZE];
	char link_dst[PATH_SIZE];

	snprintf(old_path, sizeof(old_path), "/tmp/sysrace-%d-rename-%d.old",
		 worker, iter);
	snprintf(new_path, sizeof(new_path), "/tmp/sysrace-%d-rename-%d.new",
		 worker, iter);
	snprintf(link_src, sizeof(link_src), "/tmp/sysrace-%d-linksrc-%d.txt",
		 worker, iter);
	snprintf(link_dst, sizeof(link_dst), "/tmp/sysrace-%d-linkdst-%d.txt",
		 worker, iter);

	unlink(new_path);
	unlink(link_dst);
	if (write_file(link_src, "sysrace-link\n") != 0) {
		return -1;
	}
	if (link(link_src, link_dst) != 0) {
		perror("sysracetest link");
		return -1;
	}
	if (read_file_check(link_dst, "sysrace-link\n") != 0) {
		return -1;
	}

	if (write_file(old_path, "sysrace-rename\n") != 0) {
		return -1;
	}
	if (rename(old_path, new_path) != 0) {
		perror("sysracetest rename");
		return -1;
	}
	errno = 0;
	if (access(old_path, F_OK) == 0 || errno != ENOENT) {
		fprintf(stderr, "sysracetest old path survived rename\n");
		return -1;
	}
	if (read_file_check(new_path, "sysrace-rename\n") != 0) {
		return -1;
	}
	return 0;
}

static int stress_exec_large_argv_env(int worker, int iter)
{
	char *argv[ARGV_COUNT + 4];
	char *envp[ENV_COUNT + 1];
	char args[ARGV_COUNT][32];
	char envs[ENV_COUNT][48];
	pid_t pid;
	int status;

	argv[0] = "/bin/busybox";
	argv[1] = "true";
	for (int i = 0; i < ARGV_COUNT; i++) {
		snprintf(args[i], sizeof(args[i]), "arg-%d-%d-%d",
			 worker, iter, i);
		argv[i + 2] = args[i];
	}
	argv[ARGV_COUNT + 2] = 0;

	for (int i = 0; i < ENV_COUNT; i++) {
		snprintf(envs[i], sizeof(envs[i]), "SYSRACE_ENV_%d_%d_%d=value",
			 worker, iter, i);
		envp[i] = envs[i];
	}
	envp[ENV_COUNT] = 0;

	pid = fork();
	if (pid < 0) {
		perror("sysracetest fork-exec");
		return -1;
	}
	if (pid == 0) {
		execve("/bin/busybox", argv, envp);
		perror("sysracetest execve");
		_exit(127);
	}
	if (waitpid(pid, &status, 0) != pid) {
		perror("sysracetest waitpid-exec");
		return -1;
	}
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
		fprintf(stderr, "sysracetest exec status=%d\n", status);
		return -1;
	}
	return 0;
}

static int worker_main(int worker)
{
	for (int i = 0; i < ITERATIONS; i++) {
		if (stress_open_stat(worker, i) != 0 ||
		    stress_symlink_readlink(worker, i) != 0 ||
		    stress_rename_link(worker, i) != 0 ||
		    stress_exec_large_argv_env(worker, i) != 0) {
			return 1;
		}
	}
	return 0;
}

int main(void)
{
	pid_t pids[WORKERS];
	int failed = 0;

	for (int i = 0; i < WORKERS; i++) {
		pids[i] = fork();
		if (pids[i] < 0) {
			perror("sysracetest fork");
			return 1;
		}
		if (pids[i] == 0) {
			return worker_main(i);
		}
	}

	for (int i = 0; i < WORKERS; i++) {
		int status;

		if (waitpid(pids[i], &status, 0) != pids[i]) {
			perror("sysracetest waitpid");
			failed = 1;
			continue;
		}
		if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
			fprintf(stderr, "sysracetest worker=%d status=%d\n",
				i, status);
			failed = 1;
		}
	}

	if (failed) {
		return 1;
	}
	puts("sysrace ok");
	return 0;
}
