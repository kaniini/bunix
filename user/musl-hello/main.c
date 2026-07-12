#include <stdio.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

static int is_pid_name(const char *name)
{
	if (name[0] == '\0') {
		return 0;
	}
	for (size_t i = 0; name[i] != '\0'; i++) {
		if (name[i] < '0' || name[i] > '9') {
			return 0;
		}
	}
	return 1;
}

static int cmdline_matches(const char *path, const char *argv0,
			   size_t argv0_len)
{
	char cmdline[4096];
	const int fd = open(path, O_RDONLY);

	if (fd < 0) {
		return 0;
	}
	const ssize_t nread = read(fd, cmdline, sizeof(cmdline));

	close(fd);
	return nread > 0 && (size_t)nread > argv0_len &&
	       cmdline[argv0_len] == '\0' &&
	       strncmp(cmdline, argv0, argv0_len) == 0;
}

int main(int argc, char **argv)
{
	char line[128];
	const int len = snprintf(line, sizeof(line),
				 "musl hello argc=%d argv0=%s\n",
				 argc, argv[0]);

	if (len > 0) {
		write(1, line, (size_t)len < sizeof(line) ?
		      (size_t)len : sizeof(line) - 1);
	}
	if (argc > 0 && strlen(argv[0]) > 16) {
		char path[64];
		const size_t argv0_len = strlen(argv[0]);
		DIR *dir = opendir("/proc");
		const int self_len = snprintf(path, sizeof(path),
					      "/proc/%ld/cmdline",
					      (long)getpid());

		if (self_len > 0 && (size_t)self_len < sizeof(path) &&
		    cmdline_matches(path, argv[0], argv0_len)) {
			const char ok[] = "proc long cmdline ok\n";

			write(1, ok, sizeof(ok) - 1);
			return 0;
		}
		if (dir != NULL) {
			struct dirent *entry;

			while ((entry = readdir(dir)) != NULL) {
				const int path_len = snprintf(path, sizeof(path),
							      "/proc/%s/cmdline",
							      entry->d_name);

				if (!is_pid_name(entry->d_name) ||
				    path_len <= 0 ||
				    (size_t)path_len >= sizeof(path)) {
					continue;
				}
				if (cmdline_matches(path, argv[0], argv0_len)) {
					const char ok[] =
						"proc long cmdline ok\n";

					write(1, ok, sizeof(ok) - 1);
					break;
				}
			}
			closedir(dir);
		}
	}
	return 0;
}
