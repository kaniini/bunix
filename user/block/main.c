#include <bunix/syscall.h>

enum {
	BLOCK_HANDLE_NAMES = 3,
};

static const char root_file[] = "rootfs: hello\n";

static long register_service(u64 service, u64 handle)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_NAMES,
		.type = BUNIX_NAMES_REGISTER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP,
		.reply = 0,
		.cap = handle,
		.words = { service, 0, 0, 0 },
	};
	struct bunix_msg reply;

	if (bunix_ipc_call(BLOCK_HANDLE_NAMES, &request, &reply) != 0) {
		return -1;
	}

	return reply.words[0] == 0 ? 0 : -1;
}

static void pack_bytes(u64 *words, const char *data, u64 len)
{
	words[0] = 0;
	words[1] = 0;

	for (u64 i = 0; i < len && i < 16; i++) {
		const u64 slot = i / 8;
		const u64 shift = (i % 8) * 8;

		words[slot] |= ((u64)(unsigned char)data[i]) << shift;
	}
}

int main(void)
{
	const char online[] = "block: online\n";
	struct bunix_msg message;

	bunix_console_write(online, sizeof(online) - 1);
	register_service(BUNIX_SERVICE_BLOCK, BUNIX_HANDLE_SELF);

	for (;;) {
		struct bunix_msg reply = {
			.protocol = BUNIX_PROTO_BLOCK,
			.type = 0,
			.sender = 0,
			.cap_rights = 0,
			.reply = 0,
			.cap = 0,
			.words = { 0, 0, 0, 0 },
		};

		if (bunix_ipc_recv(BUNIX_HANDLE_SELF, &message) != 0 ||
		    message.protocol != BUNIX_PROTO_BLOCK) {
			continue;
		}

		reply.type = message.type;
		switch (message.type) {
		case BUNIX_BLOCK_GET_INFO:
			reply.words[0] = 0;
			reply.words[1] = sizeof(root_file) - 1;
			reply.words[2] = 16;
			break;
		case BUNIX_BLOCK_READ: {
			const u64 offset = message.words[0];
			u64 len = message.words[1];

			if (offset >= sizeof(root_file) - 1) {
				len = 0;
			} else if (len > sizeof(root_file) - 1 - offset) {
				len = sizeof(root_file) - 1 - offset;
			}
			if (len > 16) {
				len = 16;
			}

			reply.words[0] = 0;
			reply.words[1] = len;
			pack_bytes(&reply.words[2], root_file + offset, len);
			break;
		}
		default:
			reply.words[0] = (u64)-1;
			break;
		}

		if (message.reply != 0) {
			bunix_ipc_send(message.reply, &reply);
		}
	}
}
