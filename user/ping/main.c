#include <bunix/libbunix.h>

#define PING_HANDLE_TIME (bunix_handle_find(BUNIX_CAP_TIME))
#define PING_HANDLE_VM (bunix_handle_find(BUNIX_CAP_VM))

static u64 time_call(u64 type, u64 word0)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_TIME,
		.type = (unsigned int)type,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { word0, 0, 0, 0 },
	};
	struct bunix_msg reply;

	if (bunix_ipc_call(PING_HANDLE_TIME, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return 0;
	}

	return reply.words[1];
}

int main(void)
{
	const char online[] = "ping: online\n";
	const char heartbeat[] = "ping: heartbeat\n";
	u64 sequence = 1;
	const u64 interval_ns = 2000000000ULL;

	bunix_console_log(online, sizeof(online) - 1);

	for (;;) {
		struct bunix_msg vm_message = {
			.protocol = BUNIX_PROTO_VM,
			.type = 1,
			.sender = 0,
			.reply = 0,
			.words = { 0, 0, 0, 0 },
		};
		const u64 now = time_call(BUNIX_TIME_SLEEP_NS, interval_ns);

		bunix_console_log(heartbeat, sizeof(heartbeat) - 1);
		vm_message.words[0] = sequence++;
		vm_message.words[1] = now != 0 ? now :
				      time_call(BUNIX_TIME_NOW_MONOTONIC, 0);
		bunix_ipc_send(PING_HANDLE_VM, &vm_message);
	}
}
