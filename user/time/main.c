#include <bunix/syscall.h>

enum {
	TIME_HANDLE_NAMES = 3,
};

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

	if (bunix_ipc_call(TIME_HANDLE_NAMES, &request, &reply) != 0) {
		return -1;
	}

	return reply.words[0] == 0 ? 0 : -1;
}

int main(void)
{
	const char online[] = "time: online\n";
	const char ready[] = "time: ready\n";
	struct bunix_msg message;

	bunix_console_write(online, sizeof(online) - 1);
	if (register_service(BUNIX_SERVICE_TIME, BUNIX_HANDLE_SELF) != 0) {
		return 1;
	}
	bunix_console_write(ready, sizeof(ready) - 1);

	for (;;) {
		struct bunix_msg reply = {
			.protocol = BUNIX_PROTO_TIME,
			.type = 0,
			.sender = 0,
			.cap_rights = 0,
			.reply = 0,
			.cap = 0,
			.words = { 0, 0, 0, 0 },
		};

		if (bunix_ipc_recv(BUNIX_HANDLE_SELF, &message) != 0 ||
		    message.protocol != BUNIX_PROTO_TIME) {
			continue;
		}

		reply.type = message.type;
		switch (message.type) {
		case BUNIX_TIME_NOW_MONOTONIC:
			reply.words[0] = 0;
			reply.words[1] = bunix_clock_monotonic_ns();
			break;
		case BUNIX_TIME_SLEEP_NS:
			bunix_sleep_ns(message.words[0]);
			reply.words[0] = 0;
			reply.words[1] = bunix_clock_monotonic_ns();
			break;
		default:
			reply.words[0] = (u64)-1;
			break;
		}

		if (message.reply != 0) {
			bunix_ipc_send(message.reply, &reply);
		}
	}
}
