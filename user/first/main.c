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

int main(void)
{
	const char started[] = "first: stdout ready\n";
	const char exiting[] = "first: exit 0\n";

	stdout_write(started, sizeof(started) - 1);
	time_sleep(100000000ULL);
	stdout_write(exiting, sizeof(exiting) - 1);
	proc_exit(0);

	return 0;
}
