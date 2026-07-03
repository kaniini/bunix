#include <bunix/syscall.h>

enum {
	PROC_HANDLE_CONSOLE = 2,
	PROC_HANDLE_NAMES = 3,
	PROC_HANDLE_TIME = 4,
};

struct process {
	u64 pid;
	u64 status;
	u64 exited;
	u64 waiter;
};

static struct process first_process;
static u64 next_pid = 1;

static long register_service(u64 service, u64 handle)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_NAMES,
		.type = BUNIX_NAMES_REGISTER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP,
		.reply = 0,
		.cap = handle,
		.words = { BUNIX_NAMES_ROOT, service, 0, 0 },
	};
	struct bunix_msg reply;

	if (bunix_ipc_call(PROC_HANDLE_NAMES, &request, &reply) != 0) {
		return -1;
	}

	return reply.words[0] == 0 ? 0 : -1;
}

static void reply_status(u64 reply_handle, u64 pid, u64 status)
{
	struct bunix_msg reply = {
		.protocol = BUNIX_PROTO_PROC,
		.type = BUNIX_PROC_WAIT,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { 0, pid, status, 0 },
	};

	bunix_ipc_send(reply_handle, &reply);
}

static long spawn_first(void)
{
	const struct bunix_launch_cap caps[] = {
		{ PROC_HANDLE_CONSOLE, BUNIX_RIGHT_SEND, 0 },
		{ PROC_HANDLE_TIME, BUNIX_RIGHT_SEND, 0 },
		{ BUNIX_HANDLE_SELF, BUNIX_RIGHT_SEND, 0 },
	};

	if (first_process.pid != 0) {
		return -1;
	}

	first_process.pid = next_pid++;
	first_process.status = 0;
	first_process.exited = 0;
	first_process.waiter = 0;

	if (bunix_launch_module_with_caps("first", caps,
					  sizeof(caps) / sizeof(caps[0])) < 0) {
		first_process.pid = 0;
		return -1;
	}

	return 0;
}

int main(void)
{
	const char online[] = "proc: online\n";
	const char ready[] = "proc: ready\n";
	const char spawned[] = "proc: spawned pid=1\n";
	const char exited[] = "proc: exited pid=1 status=0\n";
	const char waited[] = "proc: wait pid=1 status=0\n";
	struct bunix_msg message;

	bunix_console_write(online, sizeof(online) - 1);
	if (register_service(BUNIX_SERVICE_PROC, BUNIX_HANDLE_SELF) != 0) {
		return 1;
	}
	bunix_console_write(ready, sizeof(ready) - 1);

	for (;;) {
		struct bunix_msg reply = {
			.protocol = BUNIX_PROTO_PROC,
			.type = 0,
			.sender = 0,
			.cap_rights = 0,
			.reply = 0,
			.cap = 0,
			.words = { 0, 0, 0, 0 },
		};
		int should_reply = 1;

		if (bunix_ipc_recv(BUNIX_HANDLE_SELF, &message) != 0 ||
		    message.protocol != BUNIX_PROTO_PROC) {
			continue;
		}

		reply.type = message.type;
		switch (message.type) {
		case BUNIX_PROC_SPAWN_FIRST:
			if (spawn_first() == 0) {
				reply.words[0] = 0;
				reply.words[1] = first_process.pid;
				bunix_console_write(spawned, sizeof(spawned) - 1);
			} else {
				reply.words[0] = (u64)-1;
			}
			break;
		case BUNIX_PROC_WAIT:
			if (first_process.pid == 0) {
				reply.words[0] = (u64)-1;
			} else if (first_process.exited) {
				reply.words[0] = 0;
				reply.words[1] = first_process.pid;
				reply.words[2] = first_process.status;
				bunix_console_write(waited, sizeof(waited) - 1);
			} else if (message.reply != 0) {
				first_process.waiter = message.reply;
				should_reply = 0;
			} else {
				reply.words[0] = (u64)-1;
			}
			break;
		case BUNIX_PROC_EXIT:
			if (first_process.pid == message.words[0]) {
				first_process.status = message.words[1];
				first_process.exited = 1;
				reply.words[0] = 0;
				bunix_console_write(exited, sizeof(exited) - 1);
				if (first_process.waiter != 0) {
					reply_status(first_process.waiter,
						     first_process.pid,
						     first_process.status);
					first_process.waiter = 0;
					bunix_console_write(waited,
							    sizeof(waited) - 1);
				}
			} else {
				reply.words[0] = (u64)-1;
			}
			break;
		default:
			reply.words[0] = (u64)-1;
			break;
		}

		if (should_reply && message.reply != 0) {
			bunix_ipc_send(message.reply, &reply);
		}
	}
}
