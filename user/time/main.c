#include <bunix/libbunix.h>

static long register_service(u64 service, u64 handle)
{
	(void)service;
	return bunix_names_register_claim(bunix_handle_find(BUNIX_CAP_CLAM),
					  handle);
}

int main(void)
{
	const char online[] = "time: online\n";
	const char ready[] = "time: ready\n";
	struct bunix_msg message;

	bunix_console_log(online, sizeof(online) - 1);
	if (register_service(BUNIX_SERVICE_TIME, BUNIX_HANDLE_SELF) != 0) {
		return 1;
	}
	bunix_console_log(ready, sizeof(ready) - 1);

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
