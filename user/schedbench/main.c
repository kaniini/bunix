#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

enum {
	WORKERS = 4,
	PROC_SCHED_BUF = 4096,
	BENCH_DURATION_NS = 750000000,
	MAX_FAIRNESS_SKEW = 12,
};

struct sched_snapshot {
	unsigned long switches;
	unsigned long wakeups;
	unsigned long preemptions;
	unsigned long runtime_ticks;
	unsigned long max_wait_ticks;
	unsigned long max_wake_to_run_ticks;
	unsigned long active_cpus;
};

struct worker_result {
	int worker;
	unsigned long long iterations;
	unsigned long long checksum;
};

static unsigned long long monotonic_ns(void)
{
	struct timespec ts;

	if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
		perror("schedbench clock_gettime");
		_exit(2);
	}
	return (unsigned long long)ts.tv_sec * 1000000000ull +
	       (unsigned long long)ts.tv_nsec;
}

static int read_full(int fd, void *buf, size_t len)
{
	char *cursor = buf;
	size_t done = 0;

	while (done < len) {
		ssize_t nread = read(fd, cursor + done, len - done);
		if (nread < 0) {
			if (errno == EINTR || errno == EAGAIN ||
			    errno == EWOULDBLOCK) {
				continue;
			}
			return -1;
		}
		if (nread == 0) {
			errno = EPIPE;
			return -1;
		}
		done += (size_t)nread;
	}
	return 0;
}

static int write_full(int fd, const void *buf, size_t len)
{
	const char *cursor = buf;
	size_t done = 0;

	while (done < len) {
		ssize_t nwritten = write(fd, cursor + done, len - done);
		if (nwritten < 0) {
			if (errno == EINTR || errno == EAGAIN ||
			    errno == EWOULDBLOCK) {
				continue;
			}
			return -1;
		}
		done += (size_t)nwritten;
	}
	return 0;
}

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

static unsigned long parse_line_counter(const char *line, const char *name)
{
	const size_t name_len = strlen(name);
	const char *cursor = line;

	while (*cursor != '\0' && *cursor != '\n') {
		if (strncmp(cursor, name, name_len) == 0 &&
		    cursor[name_len] == ' ') {
			return strtoul(cursor + name_len + 1, 0, 10);
		}
		cursor++;
	}
	return 0;
}

static unsigned long count_active_cpu_switch_lines(const char *buf)
{
	unsigned long active = 0;
	const char *cursor = buf;

	while (*cursor != '\0') {
		if (strncmp(cursor, "cpu", 3) == 0 &&
		    cursor[3] >= '0' && cursor[3] <= '9' &&
		    parse_line_counter(cursor, "switches") > 0) {
			active++;
		}
		while (*cursor != '\0' && *cursor != '\n') {
			cursor++;
		}
		if (*cursor == '\n') {
			cursor++;
		}
	}
	return active;
}

static int read_sched_snapshot(struct sched_snapshot *snapshot)
{
	char buf[PROC_SCHED_BUF];
	const int fd = open("/proc/sched", O_RDONLY);
	ssize_t nread;

	memset(snapshot, 0, sizeof(*snapshot));
	if (fd < 0) {
		perror("schedbench open /proc/sched");
		return -1;
	}
	nread = read(fd, buf, sizeof(buf) - 1);
	if (close(fd) != 0) {
		perror("schedbench close /proc/sched");
		return -1;
	}
	if (nread <= 0) {
		fprintf(stderr, "schedbench empty /proc/sched\n");
		return -1;
	}
	buf[nread] = '\0';
	snapshot->switches = parse_counter(buf, "switches");
	snapshot->wakeups = parse_counter(buf, "wakeups");
	snapshot->preemptions = parse_counter(buf, "preemptions");
	snapshot->runtime_ticks = parse_counter(buf, "runtime_ticks");
	snapshot->max_wait_ticks = parse_counter(buf, "max_wait_ticks");
	snapshot->max_wake_to_run_ticks =
		parse_counter(buf, "max_wake_to_run_ticks");
	snapshot->active_cpus = count_active_cpu_switch_lines(buf);
	if (snapshot->switches == 0 || snapshot->active_cpus == 0) {
		fprintf(stderr, "schedbench missing scheduler counters\n");
		return -1;
	}
	return 0;
}

static int worker_main(int worker, int start_fd, int result_fd)
{
	unsigned long long deadline;
	unsigned long long now;
	unsigned long long iterations = 0;
	unsigned long long checksum = (unsigned long long)(worker + 1);
	struct worker_result result;

	if (read_full(start_fd, &deadline, sizeof(deadline)) != 0) {
		perror("schedbench worker start");
		return 1;
	}

	now = monotonic_ns();
	while (now < deadline) {
		for (int i = 0; i < 4096; i++) {
			checksum = checksum * 2862933555777941757ull +
				   3037000493ull + iterations;
			iterations++;
		}
		now = monotonic_ns();
	}

	result.worker = worker;
	result.iterations = iterations;
	result.checksum = checksum;
	if (write_full(result_fd, &result, sizeof(result)) != 0) {
		perror("schedbench worker result");
		return 1;
	}
	return 0;
}

static int run_fairness(void)
{
	int start_pipe[2];
	int result_pipe[2];
	pid_t pids[WORKERS];
	struct worker_result results[WORKERS];
	struct sched_snapshot before;
	struct sched_snapshot after;
	unsigned long long deadline;
	unsigned long long min = ULLONG_MAX;
	unsigned long long max = 0;
	unsigned long long total = 0;
	int status;

	if (read_sched_snapshot(&before) != 0) {
		return 1;
	}
	if (pipe(start_pipe) != 0 || pipe(result_pipe) != 0) {
		perror("schedbench pipe");
		return 1;
	}

	for (int worker = 0; worker < WORKERS; worker++) {
		pids[worker] = fork();
		if (pids[worker] < 0) {
			perror("schedbench fork");
			return 1;
		}
		if (pids[worker] == 0) {
			close(start_pipe[1]);
			close(result_pipe[0]);
			_exit(worker_main(worker, start_pipe[0],
					  result_pipe[1]));
		}
	}

	close(start_pipe[0]);
	close(result_pipe[1]);

	deadline = monotonic_ns() + BENCH_DURATION_NS;
	for (int worker = 0; worker < WORKERS; worker++) {
		if (write_full(start_pipe[1], &deadline,
			       sizeof(deadline)) != 0) {
			perror("schedbench start write");
			return 1;
		}
	}
	if (close(start_pipe[1]) != 0) {
		perror("schedbench close start");
		return 1;
	}

	for (int worker = 0; worker < WORKERS; worker++) {
		if (read_full(result_pipe[0], &results[worker],
			      sizeof(results[worker])) != 0) {
			perror("schedbench result read");
			return 1;
		}
	}
	if (close(result_pipe[0]) != 0) {
		perror("schedbench close result");
		return 1;
	}

	for (int worker = 0; worker < WORKERS; worker++) {
		if (waitpid(pids[worker], &status, 0) != pids[worker]) {
			perror("schedbench waitpid");
			return 1;
		}
		if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
			fprintf(stderr, "schedbench worker status=%d\n", status);
			return 1;
		}
	}

	for (int i = 0; i < WORKERS; i++) {
		if (results[i].iterations == 0 || results[i].checksum == 0) {
			fprintf(stderr, "schedbench empty worker result\n");
			return 1;
		}
		if (results[i].iterations < min) {
			min = results[i].iterations;
		}
		if (results[i].iterations > max) {
			max = results[i].iterations;
		}
		total += results[i].iterations;
	}

	if (min == 0 || max > min * MAX_FAIRNESS_SKEW) {
		fprintf(stderr,
			"schedbench fairness skew too high min=%llu max=%llu limit=%u\n",
			min, max, MAX_FAIRNESS_SKEW);
		return 1;
	}

	if (read_sched_snapshot(&after) != 0) {
		return 1;
	}
	if (after.switches <= before.switches) {
		fprintf(stderr, "schedbench switches did not advance %lu <= %lu\n",
			after.switches, before.switches);
		return 1;
	}
	if (after.runtime_ticks <= before.runtime_ticks) {
		fprintf(stderr,
			"schedbench runtime did not advance %lu <= %lu\n",
			after.runtime_ticks, before.runtime_ticks);
		return 1;
	}

	printf("schedbench fairness workers=%u total=%llu min=%llu max=%llu skew_x100=%llu\n",
	       WORKERS, total, min, max, (max * 100ull) / min);
	printf("schedbench counters switches %lu -> %lu runtime %lu -> %lu wakeups %lu -> %lu preemptions %lu -> %lu max_wait %lu -> %lu max_wake_to_run %lu -> %lu cpus %lu\n",
	       before.switches, after.switches,
	       before.runtime_ticks, after.runtime_ticks,
	       before.wakeups, after.wakeups,
	       before.preemptions, after.preemptions,
	       before.max_wait_ticks, after.max_wait_ticks,
	       before.max_wake_to_run_ticks, after.max_wake_to_run_ticks,
	       after.active_cpus);
	printf("schedbench fairness ok\n");
	return 0;
}

int main(int argc, char **argv)
{
	if (argc == 1 || strcmp(argv[1], "fairness") == 0) {
		return run_fairness();
	}
	fprintf(stderr, "usage: %s [fairness]\n", argv[0]);
	return 2;
}
