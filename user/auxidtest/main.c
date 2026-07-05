#include <stdio.h>
#include <sys/auxv.h>
#include <unistd.h>

#ifndef AT_UID
#define AT_UID 11
#endif
#ifndef AT_EUID
#define AT_EUID 12
#endif
#ifndef AT_GID
#define AT_GID 13
#endif
#ifndef AT_EGID
#define AT_EGID 14
#endif
#ifndef AT_SECURE
#define AT_SECURE 23
#endif

static int check_aux(const char *name, unsigned long type, unsigned long expected)
{
	const unsigned long actual = getauxval(type);

	if (actual != expected) {
		fprintf(stderr, "%s: aux=%lu expected=%lu\n",
			name, actual, expected);
		return 1;
	}
	return 0;
}

int main(void)
{
	int failed = 0;

	failed |= check_aux("AT_UID", AT_UID, (unsigned long)getuid());
	failed |= check_aux("AT_EUID", AT_EUID, (unsigned long)geteuid());
	failed |= check_aux("AT_GID", AT_GID, (unsigned long)getgid());
	failed |= check_aux("AT_EGID", AT_EGID, (unsigned long)getegid());
	failed |= check_aux("AT_SECURE", AT_SECURE, 0);
	if (failed != 0) {
		return 1;
	}

	puts("linux auxid ok");
	return 0;
}
