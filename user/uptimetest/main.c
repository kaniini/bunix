#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <time.h>
#include <unistd.h>

static int read_proc_uptime(long *seconds_out)
{
	char buf[64];
	long seconds = 0;
	int fd;
	ssize_t n;

	if (seconds_out == NULL) {
		errno = EINVAL;
		return -1;
	}
	fd = open("/proc/uptime", O_RDONLY);
	if (fd < 0) {
		return -1;
	}
	n = read(fd, buf, sizeof(buf) - 1);
	if (close(fd) != 0 || n <= 0) {
		return -1;
	}
	buf[n] = '\0';
	for (ssize_t i = 0; i < n; i++) {
		if (buf[i] == '.') {
			*seconds_out = seconds;
			return 0;
		}
		if (buf[i] < '0' || buf[i] > '9') {
			errno = EINVAL;
			return -1;
		}
		seconds = seconds * 10 + (buf[i] - '0');
	}
	errno = EINVAL;
	return -1;
}

static int sample(long *sysinfo_seconds, long *clock_seconds,
		  long *proc_seconds)
{
	struct sysinfo info;
	struct timespec ts;

	if (sysinfo(&info) != 0) {
		perror("uptimetest sysinfo");
		return -1;
	}
	if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
		perror("uptimetest clock_gettime");
		return -1;
	}
	if (read_proc_uptime(proc_seconds) != 0) {
		perror("uptimetest proc uptime");
		return -1;
	}
	*sysinfo_seconds = info.uptime;
	*clock_seconds = ts.tv_sec;
	return 0;
}

static long abs_delta(long a, long b)
{
	return a > b ? a - b : b - a;
}

int main(void)
{
	long sys0;
	long clk0;
	long proc0;
	long sys1;
	long clk1;
	long proc1;

	if (sample(&sys0, &clk0, &proc0) != 0) {
		return 1;
	}
	if (sleep(2) != 0) {
		perror("uptimetest sleep");
		return 1;
	}
	if (sample(&sys1, &clk1, &proc1) != 0) {
		return 1;
	}
	if (sys1 - sys0 < 1 || clk1 - clk0 < 1 || proc1 - proc0 < 1) {
		fprintf(stderr,
			"uptimetest did not advance sys=%ld->%ld clock=%ld->%ld proc=%ld->%ld\n",
			sys0, sys1, clk0, clk1, proc0, proc1);
		return 1;
	}
	if (abs_delta(sys1, clk1) > 1 || abs_delta(sys1, proc1) > 1) {
		fprintf(stderr,
			"uptimetest source mismatch sys=%ld clock=%ld proc=%ld\n",
			sys1, clk1, proc1);
		return 1;
	}
	printf("uptimetest ok sys=%ld clock=%ld proc=%ld\n",
	       sys1, clk1, proc1);
	return 0;
}
