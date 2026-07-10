#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

static int wait_child(pid_t child, const char *label, int iter,
		      const char *role)
{
	int status = 0;

	if (waitpid(child, &status, 0) != child) {
		printf("PROCFSPIPETEST_WAIT_FAIL label=%s iter=%d role=%s child=%d errno=%d\n",
		       label, iter, role, (int)child, errno);
		return 1;
	}
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
		printf("PROCFSPIPETEST_CHILD_FAIL label=%s iter=%d role=%s child=%d status=0x%x\n",
		       label, iter, role, (int)child, status);
		return 1;
	}
	return 0;
}

static void exec_busybox_cat_file(const char *path)
{
	execl("/bin/busybox", "busybox", "cat", path, (char *)0);
	_exit(127);
}

static void exec_busybox_ps(void)
{
	execl("/bin/busybox", "busybox", "ps", (char *)0);
	_exit(127);
}

static int run_pipeline(const char *label, int iter,
			void (*exec_writer)(const char *), const char *arg)
{
	int fds[2] = { -1, -1 };
	pid_t writer;
	pid_t reader;
	int nullfd;

	printf("PROCFSPIPETEST_BEGIN_%s_%d\n", label, iter);
	fflush(stdout);

	if (pipe2(fds, O_CLOEXEC) != 0) {
		printf("PROCFSPIPETEST_PIPE_FAIL label=%s iter=%d errno=%d\n",
		       label, iter, errno);
		return 1;
	}
	writer = fork();
	if (writer < 0) {
		printf("PROCFSPIPETEST_FORK_WRITER_FAIL label=%s iter=%d errno=%d\n",
		       label, iter, errno);
		close(fds[0]);
		close(fds[1]);
		return 1;
	}
	if (writer == 0) {
		if (dup2(fds[1], STDOUT_FILENO) < 0) {
			_exit(125);
		}
		close(fds[0]);
		close(fds[1]);
		exec_writer(arg);
	}
	reader = fork();
	if (reader < 0) {
		printf("PROCFSPIPETEST_FORK_READER_FAIL label=%s iter=%d errno=%d\n",
		       label, iter, errno);
		close(fds[0]);
		close(fds[1]);
		return 1;
	}
	if (reader == 0) {
		nullfd = open("/dev/null", O_WRONLY | O_CLOEXEC);
		if (nullfd < 0 ||
		    dup2(fds[0], STDIN_FILENO) < 0 ||
		    dup2(nullfd, STDOUT_FILENO) < 0) {
			_exit(126);
		}
		close(fds[0]);
		close(fds[1]);
		close(nullfd);
		execl("/bin/busybox", "busybox", "cat", (char *)0);
		_exit(127);
	}
	close(fds[0]);
	close(fds[1]);

	if (wait_child(writer, label, iter, "writer") != 0) {
		return 1;
	}
	if (wait_child(reader, label, iter, "reader") != 0) {
		return 1;
	}
	printf("PROCFSPIPETEST_%s_%d\n", label, iter);
	fflush(stdout);
	return 0;
}

static void exec_ps_adapter(const char *unused)
{
	(void)unused;
	exec_busybox_ps();
}

int main(void)
{
	setvbuf(stdout, 0, _IOLBF, 0);

	for (int i = 1; i <= 20; i++) {
		if (run_pipeline("STAT", i, exec_busybox_cat_file,
				 "/proc/self/stat") != 0 ||
		    run_pipeline("CMDLINE", i, exec_busybox_cat_file,
				 "/proc/self/cmdline") != 0 ||
		    run_pipeline("PS", i, exec_ps_adapter, 0) != 0) {
			return 1;
		}
	}

	puts("PROCFSPIPETEST_OK");
	return 0;
}
