#define _GNU_SOURCE

#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static int signal_pipe[2] = { -1, -1 };

static void sigchld_handler(int signal)
{
	char byte = 'c';

	(void)signal;
	if (signal_pipe[1] >= 0) {
		(void)write(signal_pipe[1], &byte, 1);
	}
}

int main(void)
{
	struct sigaction action;
	struct pollfd pfd;
	char byte = 0;
	pid_t child;
	int status = 0;

	if (pipe(signal_pipe) != 0) {
		perror("signaltest pipe");
		return 1;
	}

	memset(&action, 0, sizeof(action));
	action.sa_handler = sigchld_handler;
	sigemptyset(&action.sa_mask);
	if (sigaction(SIGCHLD, &action, 0) != 0) {
		perror("signaltest sigaction");
		return 1;
	}

	child = fork();
	if (child < 0) {
		perror("signaltest fork");
		return 1;
	}
	if (child == 0) {
		_exit(0);
	}

	pfd.fd = signal_pipe[0];
	pfd.events = POLLIN;
	pfd.revents = 0;
	for (;;) {
		const int ready = poll(&pfd, 1, 3000);

		if (ready > 0) {
			break;
		}
		if (ready < 0 && errno == EINTR) {
			continue;
		}
		printf("signaltest poll ready=%d errno=%d revents=0x%x\n",
		       ready, errno, pfd.revents);
		return 1;
	}

	if (read(signal_pipe[0], &byte, 1) != 1 || byte != 'c') {
		printf("signaltest read byte=%d errno=%d\n", byte, errno);
		return 1;
	}
	if (waitpid(child, &status, 0) != child || !WIFEXITED(status) ||
	    WEXITSTATUS(status) != 0) {
		printf("signaltest wait status=0x%x errno=%d\n", status, errno);
		return 1;
	}

	puts("linux sigchld self-pipe ok");
	return 0;
}
