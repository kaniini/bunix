#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

enum {
	WORKERS = 4,
	ITERATIONS = 32,
	PROC_SCHED_BUF = 4096,
};

static unsigned long parse_counter(const char *buf, const char *name)
{
	const size_t name_len = strlen(name);
	const char *cursor = buf;

	while (*cursor != '\0') {
		if (strncmp(cursor, name, name_len) == 0 &&
		    cursor[name_len] == ' ') {
			return strtoul(cursor + name_len + 1, 0, 10);
		}
		while (*cursor != '\0' && *cursor != '\n') {
			cursor++;
		}
		if (*cursor == '\n') {
			cursor++;
		}
	}
	return 0;
}

static int read_sched_switches(unsigned long *switches)
{
	char buf[PROC_SCHED_BUF];
	const int fd = open("/proc/sched", O_RDONLY);
	ssize_t nread;

	if (fd < 0) {
		perror("schedstress open /proc/sched");
		return -1;
	}
	nread = read(fd, buf, sizeof(buf) - 1);
	if (close(fd) != 0) {
		perror("schedstress close /proc/sched");
		return -1;
	}
	if (nread <= 0) {
		fprintf(stderr, "schedstress empty /proc/sched\n");
		return -1;
	}
	buf[nread] = '\0';
	*switches = parse_counter(buf, "switches");
	if (*switches == 0) {
		fprintf(stderr, "schedstress missing switches counter\n");
		return -1;
	}
	return 0;
}

static int worker_main(int worker, int write_fd)
{
	struct timespec delay = {
		.tv_sec = 0,
		.tv_nsec = 1000000,
	};
	unsigned long checksum = (unsigned long)(worker + 1);
	char done = (char)('A' + worker);

	for (int i = 0; i < ITERATIONS; i++) {
		for (int spin = 0; spin < 4096; spin++) {
			checksum = checksum * 1103515245ul +
				   (unsigned long)i + (unsigned long)spin;
		}
		if ((i % 3) == 0) {
			if (sched_yield() != 0) {
				perror("schedstress sched_yield");
				return 1;
			}
		} else if (nanosleep(&delay, 0) != 0 && errno != EINTR) {
			perror("schedstress nanosleep");
			return 1;
		}
	}

	if (checksum == 0) {
		return 1;
	}
	if (write(write_fd, &done, 1) != 1) {
		perror("schedstress worker write");
		return 1;
	}
	return 0;
}

int main(void)
{
	int pipefd[2];
	pid_t pids[WORKERS];
	unsigned long before = 0;
	unsigned long after = 0;
	char seen[WORKERS];
	int status;

	if (read_sched_switches(&before) != 0) {
		return 1;
	}
	if (pipe(pipefd) != 0) {
		perror("schedstress pipe");
		return 1;
	}

	for (int worker = 0; worker < WORKERS; worker++) {
		pids[worker] = fork();
		if (pids[worker] < 0) {
			perror("schedstress fork");
			return 1;
		}
		if (pids[worker] == 0) {
			close(pipefd[0]);
			return worker_main(worker, pipefd[1]);
		}
	}

	close(pipefd[1]);
	for (int worker = 0; worker < WORKERS; worker++) {
		if (read(pipefd[0], &seen[worker], 1) != 1) {
			perror("schedstress parent read");
			return 1;
		}
	}
	if (close(pipefd[0]) != 0) {
		perror("schedstress close pipe");
		return 1;
	}

	for (int worker = 0; worker < WORKERS; worker++) {
		if (waitpid(pids[worker], &status, 0) != pids[worker]) {
			perror("schedstress waitpid");
			return 1;
		}
		if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
			fprintf(stderr, "schedstress worker status=%d\n", status);
			return 1;
		}
	}

	if (read_sched_switches(&after) != 0) {
		return 1;
	}
	if (after <= before) {
		fprintf(stderr, "schedstress switches did not advance %lu <= %lu\n",
			after, before);
		return 1;
	}

	printf("schedstress switches %lu -> %lu\n", before, after);
	printf("schedstress ok\n");
	return 0;
}
