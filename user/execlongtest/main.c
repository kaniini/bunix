#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum {
	FILLER_COUNT = 80,
	FILLER_PAYLOAD = 4096,
	LONG_PAYLOAD = 8192,
};

static char *make_env(const char *name, size_t payload, const char *suffix)
{
	const size_t name_len = strlen(name);
	const size_t suffix_len = suffix == NULL ? 0 : strlen(suffix);
	const size_t len = name_len + 1 + payload + suffix_len + 1;
	char *value = malloc(len);

	if (value == NULL) {
		return NULL;
	}
	memcpy(value, name, name_len);
	value[name_len] = '=';
	memset(value + name_len + 1, 'x', payload);
	if (suffix_len != 0) {
		memcpy(value + name_len + 1 + payload, suffix, suffix_len);
	}
	value[len - 1] = '\0';
	return value;
}

int main(void)
{
	char *envp[FILLER_COUNT + 3];
	char *argv[] = {
		"/bin/busybox",
		"sh",
		"-c",
		"case \"$BUNIX_LONG_SINGLE\" in *tail-ok) test -n \"$BUNIX_FILLER_079\" && echo linux execlong ok;; *) exit 1;; esac",
		NULL,
	};
	char name[32];

	envp[0] = make_env("BUNIX_LONG_SINGLE", LONG_PAYLOAD, "tail-ok");
	if (envp[0] == NULL) {
		return 1;
	}
	for (int i = 0; i < FILLER_COUNT; i++) {
		snprintf(name, sizeof(name), "BUNIX_FILLER_%03d", i);
		envp[i + 1] = make_env(name, FILLER_PAYLOAD, NULL);
		if (envp[i + 1] == NULL) {
			return 1;
		}
	}
	envp[FILLER_COUNT + 1] = "PATH=/bin:/sbin:/usr/bin:/usr/sbin";
	envp[FILLER_COUNT + 2] = NULL;

	execve("/bin/busybox", argv, envp);
	perror("execlongtest execve");
	return 1;
}
