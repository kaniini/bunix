#include <bunix/syscall.h>

enum {
	FIRST_PID = 1,
	FIRST_FD_STDOUT = 2,
	FIRST_HANDLE_TIME = 3,
	FIRST_HANDLE_PROC = 4,
	AT_NULL = 0,
	AT_PHDR = 3,
	AT_PHENT = 4,
	AT_PHNUM = 5,
	AT_PAGESZ = 6,
	AT_ENTRY = 9,
	AT_EXECFN = 31,
};

static void stdout_write(const char *text, u64 len)
{
	const struct bunix_msg message = {
		.protocol = BUNIX_PROTO_CONSOLE,
		.type = BUNIX_CONSOLE_WRITE,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { (u64)text, len, 0, 0 },
	};

	bunix_ipc_send(FIRST_FD_STDOUT, &message);
}

static void time_sleep(u64 ns)
{
	const struct bunix_msg request = {
		.protocol = BUNIX_PROTO_TIME,
		.type = BUNIX_TIME_SLEEP_NS,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { ns, 0, 0, 0 },
	};
	struct bunix_msg reply;

	bunix_ipc_call(FIRST_HANDLE_TIME, &request, &reply);
}

static void proc_exit(u64 status)
{
	const struct bunix_msg request = {
		.protocol = BUNIX_PROTO_PROC,
		.type = BUNIX_PROC_EXIT,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { FIRST_PID, status, 0, 0 },
	};
	struct bunix_msg reply;

	bunix_ipc_call(FIRST_HANDLE_PROC, &request, &reply);
}

static void stdout_write_argv0(const char *value)
{
	const char prefix[] = "first: argv0=";
	char line[64];
	u64 cursor = 0;

	for (u64 i = 0; i < sizeof(prefix) - 1 && cursor < sizeof(line); i++) {
		line[cursor++] = prefix[i];
	}
	for (u64 i = 0; value[i] != '\0' && cursor + 1 < sizeof(line); i++) {
		line[cursor++] = value[i];
	}
	line[cursor++] = '\n';
	stdout_write(line, cursor);
}

static void append_dec(char *line, u64 *cursor, u64 value)
{
	char digits[20];
	u64 count = 0;

	if (value == 0) {
		line[(*cursor)++] = '0';
		return;
	}

	while (value != 0 && count < sizeof(digits)) {
		digits[count++] = (char)('0' + value % 10);
		value /= 10;
	}
	while (count != 0) {
		line[(*cursor)++] = digits[--count];
	}
}

static void append_hex(char *line, u64 *cursor, u64 value)
{
	const char hex[] = "0123456789abcdef";
	int started = 0;

	line[(*cursor)++] = '0';
	line[(*cursor)++] = 'x';
	for (int shift = 60; shift >= 0; shift -= 4) {
		const u64 digit = (value >> shift) & 0xf;

		if (digit != 0 || started || shift == 0) {
			line[(*cursor)++] = hex[digit];
			started = 1;
		}
	}
}

static void stdout_write_aux_dec(const char *name, u64 value)
{
	char line[64];
	u64 cursor = 0;

	for (u64 i = 0; name[i] != '\0'; i++) {
		line[cursor++] = name[i];
	}
	append_dec(line, &cursor, value);
	line[cursor++] = '\n';
	stdout_write(line, cursor);
}

static void stdout_write_aux_hex(const char *name, u64 value)
{
	char line[64];
	u64 cursor = 0;

	for (u64 i = 0; name[i] != '\0'; i++) {
		line[cursor++] = name[i];
	}
	append_hex(line, &cursor, value);
	line[cursor++] = '\n';
	stdout_write(line, cursor);
}

static void inspect_auxv(char **envp)
{
	u64 *auxv;
	u64 pagesz = 0;
	u64 entry = 0;
	u64 phdr = 0;
	u64 phent = 0;
	u64 phnum = 0;
	const char *execfn = 0;

	while (*envp != 0) {
		envp++;
	}
	auxv = (u64 *)(envp + 1);

	for (;;) {
		const u64 type = auxv[0];
		const u64 value = auxv[1];

		if (type == AT_NULL) {
			break;
		}
		if (type == AT_PAGESZ) {
			pagesz = value;
		} else if (type == AT_ENTRY) {
			entry = value;
		} else if (type == AT_PHDR) {
			phdr = value;
		} else if (type == AT_PHENT) {
			phent = value;
		} else if (type == AT_PHNUM) {
			phnum = value;
		} else if (type == AT_EXECFN) {
			execfn = (const char *)value;
		}
		auxv += 2;
	}

	stdout_write_aux_dec("first: aux pagesz=", pagesz);
	stdout_write_aux_hex("first: aux entry=", entry);
	stdout_write_aux_hex("first: aux phdr=", phdr);
	stdout_write_aux_dec("first: aux phent=", phent);
	stdout_write_aux_dec("first: aux phnum=", phnum);
	if (execfn != 0) {
		stdout_write_argv0(execfn);
	}
}

int main(int argc, char **argv, char **envp)
{
	const char started[] = "first: stdout ready\n";
	const char argc_ok[] = "first: argc=1\n";
	const char exiting[] = "first: exit 0\n";

	stdout_write(started, sizeof(started) - 1);
	if (argc == 1 && argv != 0 && argv[0] != 0) {
		stdout_write(argc_ok, sizeof(argc_ok) - 1);
		stdout_write_argv0(argv[0]);
	}
	if (envp != 0) {
		inspect_auxv(envp);
	}
	time_sleep(100000000ULL);
	stdout_write(exiting, sizeof(exiting) - 1);
	proc_exit(0);

	return 0;
}
