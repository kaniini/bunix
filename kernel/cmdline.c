#include "cmdline.h"
#include "console.h"
#include "types.h"
#include <arch/user.h>

static const char *kernel_cmdline;

static int starts_with(const char *text, const char *prefix)
{
	while (*prefix != '\0') {
		if (*text++ != *prefix++) {
			return 0;
		}
	}

	return 1;
}

static u64 str_len(const char *s)
{
	u64 len = 0;

	while (s[len] != '\0') {
		len++;
	}
	return len;
}

void kernel_cmdline_configure(const char *cmdline)
{
	int have_log = 0;

	if (cmdline == 0) {
		cmdline = "";
	}
	kernel_cmdline = cmdline;
	while (*cmdline != '\0') {
		while (*cmdline == ' ') {
			cmdline++;
		}

		if (starts_with(cmdline, "log=")) {
			console_set_verbosity(cmdline + 4);
			have_log = 1;
		}
		if (starts_with(cmdline, "strace=")) {
			arch_user_set_strace_mode(cmdline + 7);
		}

		while (*cmdline != '\0' && *cmdline != ' ') {
			cmdline++;
		}
	}

	if (!have_log) {
		console_set_verbosity("info");
	}
}

int kernel_cmdline_has(const char *token)
{
	const char *cmdline = kernel_cmdline;
	const u64 token_len = token == 0 ? 0 : str_len(token);

	if (cmdline == 0 || token_len == 0) {
		return 0;
	}

	while (*cmdline != '\0') {
		while (*cmdline == ' ') {
			cmdline++;
		}

		u64 len = 0;
		while (cmdline[len] != '\0' && cmdline[len] != ' ') {
			len++;
		}
		if (len == token_len) {
			u64 i;

			for (i = 0; i < len; i++) {
				if (cmdline[i] != token[i]) {
					break;
				}
			}
			if (i == len) {
				return 1;
			}
		}
		cmdline += len;
	}

	return 0;
}

int kernel_cmdline_get_u64(const char *prefix, u64 *value)
{
	const char *cmdline = kernel_cmdline;
	const u64 prefix_len = prefix == 0 ? 0 : str_len(prefix);

	if (cmdline == 0 || prefix_len == 0 || value == 0) {
		return 0;
	}

	while (*cmdline != '\0') {
		u64 len = 0;
		u64 parsed = 0;
		u64 i;

		while (*cmdline == ' ') {
			cmdline++;
		}
		while (cmdline[len] != '\0' && cmdline[len] != ' ') {
			len++;
		}
		if (len <= prefix_len) {
			cmdline += len;
			continue;
		}
		for (i = 0; i < prefix_len; i++) {
			if (cmdline[i] != prefix[i]) {
				break;
			}
		}
		if (i != prefix_len) {
			cmdline += len;
			continue;
		}
		for (i = prefix_len; i < len; i++) {
			if (cmdline[i] < '0' || cmdline[i] > '9') {
				return 0;
			}
			parsed = parsed * 10 + (u64)(cmdline[i] - '0');
		}
		*value = parsed;
		return 1;
	}

	return 0;
}
