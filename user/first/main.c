#include <bunix/syscall.h>

enum {
	FIRST_PID = 1,
	FIRST_FD_STDOUT = 2,
	FIRST_HANDLE_TIME = 3,
	FIRST_HANDLE_PROC = 4,
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

int main(int argc, char **argv)
{
	const char started[] = "first: stdout ready\n";
	const char argc_ok[] = "first: argc=1\n";
	const char exiting[] = "first: exit 0\n";

	stdout_write(started, sizeof(started) - 1);
	if (argc == 1 && argv != 0 && argv[0] != 0) {
		stdout_write(argc_ok, sizeof(argc_ok) - 1);
		stdout_write_argv0(argv[0]);
	}
	time_sleep(100000000ULL);
	stdout_write(exiting, sizeof(exiting) - 1);
	proc_exit(0);

	return 0;
}
