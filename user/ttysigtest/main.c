#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

static int wait_for_signal(pid_t child, int signal)
{
	int status = 0;

	if (waitpid(child, &status, 0) != child) {
		perror("ttysigtest wait reader");
		return 1;
	}
	if (!WIFSIGNALED(status) || WTERMSIG(status) != signal) {
		printf("ttysigtest reader status=0x%x\n", status);
		return 1;
	}
	return 0;
}

int main(void)
{
	pid_t sleeper;
	pid_t reader;
	char byte = 0;

	if (signal(SIGINT, SIG_IGN) == SIG_ERR) {
		perror("ttysigtest ignore");
		return 1;
	}

	sleeper = fork();
	if (sleeper < 0) {
		perror("ttysigtest fork sleeper");
		return 1;
	}
	if (sleeper == 0) {
		if (setsid() < 0) {
			_exit(20);
		}
		sleep(30);
		_exit(21);
	}

	reader = fork();
	if (reader < 0) {
		perror("ttysigtest fork reader");
		(void)kill(sleeper, SIGTERM);
		return 1;
	}
	if (reader == 0) {
		if (signal(SIGINT, SIG_DFL) == SIG_ERR) {
			_exit(30);
		}
		if (read(0, &byte, 1) < 0) {
			_exit(31);
		}
		_exit(32);
	}

	printf("ttysigtest ready\n");
	fflush(stdout);

	if (wait_for_signal(reader, SIGINT) != 0) {
		(void)kill(sleeper, SIGTERM);
		return 1;
	}
	if (kill(sleeper, 0) != 0) {
		printf("ttysigtest sleeper missing errno=%d\n", errno);
		return 1;
	}
	(void)kill(sleeper, SIGTERM);
	(void)waitpid(sleeper, 0, 0);
	printf("ttysigtest ok\n");
	return 0;
}
