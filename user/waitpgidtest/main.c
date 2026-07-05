#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

static int check_status(int status, int code)
{
	return WIFEXITED(status) && WEXITSTATUS(status) == code;
}

int main(void)
{
	int status;
	pid_t child;
	pid_t waited;
	pid_t pgrp;

	child = fork();
	if (child < 0) {
		perror("waitpgidtest fork pid0");
		return 1;
	}
	if (child == 0) {
		_exit(11);
	}
	waited = waitpid(0, &status, 0);
	if (waited != child || !check_status(status, 11)) {
		printf("waitpgidtest pid0 waited=%ld child=%ld status=0x%x errno=%d\n",
		       (long)waited, (long)child, status, errno);
		return 1;
	}

	child = fork();
	if (child < 0) {
		perror("waitpgidtest fork pgrp");
		return 1;
	}
	if (child == 0) {
		_exit(12);
	}
	pgrp = getpgrp();
	if (pgrp <= 0) {
		perror("waitpgidtest getpgrp");
		return 1;
	}
	waited = waitpid(-pgrp, &status, 0);
	if (waited != child || !check_status(status, 12)) {
		printf("waitpgidtest pgrp waited=%ld child=%ld pgrp=%ld status=0x%x errno=%d\n",
		       (long)waited, (long)child, (long)pgrp, status, errno);
		return 1;
	}

	printf("linux waitpgid checks ok\n");
	return 0;
}
