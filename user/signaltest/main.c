#define _GNU_SOURCE

#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static int signal_pipe[2] = { -1, -1 };
static int expected_fault_signal;
static int expected_fault_code;
static void *expected_fault_addr;

static void fault_handler(int signal, siginfo_t *info, void *context)
{
	(void)context;
	if (signal != expected_fault_signal || info == 0 ||
	    info->si_signo != expected_fault_signal ||
	    info->si_code != expected_fault_code ||
	    info->si_addr != expected_fault_addr) {
		_exit(4);
	}
	_exit(signal == SIGSEGV ? 42 : 43);
}

static void sigchld_handler(int signal)
{
	char byte = 'c';

	(void)signal;
	if (signal_pipe[1] >= 0) {
		(void)write(signal_pipe[1], &byte, 1);
	}
}

static int wait_exit(pid_t child, int code, const char *label)
{
	int status = 0;

	if (waitpid(child, &status, 0) != child || !WIFEXITED(status) ||
	    WEXITSTATUS(status) != code) {
		printf("%s wait status=0x%x errno=%d\n", label, status, errno);
		return 1;
	}
	return 0;
}

static int test_caught_faults(void)
{
	struct sigaction action;
	pid_t child;

	memset(&action, 0, sizeof(action));
	action.sa_sigaction = fault_handler;
	action.sa_flags = SA_SIGINFO;
	sigemptyset(&action.sa_mask);

	child = fork();
	if (child < 0) {
		perror("signaltest sigsegv fork");
		return 1;
	}
	if (child == 0) {
		volatile int *bad = (volatile int *)0x0000700000000000ull;

		expected_fault_signal = SIGSEGV;
		expected_fault_code = SEGV_MAPERR;
		expected_fault_addr = (void *)bad;
		if (sigaction(SIGSEGV, &action, 0) != 0) {
			_exit(2);
		}
		*bad = 1;
		_exit(3);
	}
	if (wait_exit(child, 42, "signaltest sigsegv") != 0) {
		return 1;
	}

	child = fork();
	if (child < 0) {
		perror("signaltest sigbus fork");
		return 1;
	}
	if (child == 0) {
		static unsigned int word = 0x12345678;
		volatile unsigned int value;

		expected_fault_signal = SIGBUS;
		expected_fault_code = BUS_ADRALN;
		expected_fault_addr = 0;
		if (sigaction(SIGBUS, &action, 0) != 0) {
			_exit(2);
		}
		__asm__ volatile (
			"pushfq\n"
			"orq $0x40000, (%%rsp)\n"
			"popfq\n"
			: : : "memory", "cc");
		value = *(volatile unsigned int *)((char *)&word + 1);
		(void)value;
		_exit(3);
	}
	if (wait_exit(child, 43, "signaltest sigbus") != 0) {
		return 1;
	}

	puts("linux caught fault signals ok");
	return 0;
}

static int test_sigchld_self_pipe(void)
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
		const int ready = poll(&pfd, 1, 10000);

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

int main(void)
{
	if (test_sigchld_self_pipe() != 0 ||
	    test_caught_faults() != 0) {
		return 1;
	}

	puts("linux signaltest ok");
	return 0;
}
