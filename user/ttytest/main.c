#include <errno.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

int main(void)
{
	int fg = -1;
	int own = (int)getpgrp();
	int bad = 999999;

	if (ioctl(1, TIOCGPGRP, &fg) != 0 || fg <= 0) {
		perror("ttytest TIOCGPGRP");
		return 1;
	}
	errno = 0;
	if (ioctl(1, TIOCSPGRP, &bad) == 0) {
		fprintf(stderr, "ttytest: bogus foreground pgid accepted\n");
		return 1;
	}
	if (errno != EPERM && errno != ESRCH) {
		perror("ttytest TIOCSPGRP bogus");
		return 1;
	}
	if (ioctl(1, TIOCSPGRP, &own) != 0) {
		perror("ttytest TIOCSPGRP own");
		return 1;
	}
	fg = -1;
	if (ioctl(1, TIOCGPGRP, &fg) != 0 || fg != own) {
		perror("ttytest TIOCGPGRP own");
		return 1;
	}
	printf("ttytest ok\n");
	return 0;
}
