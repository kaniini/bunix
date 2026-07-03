#include <bunix/syscall.h>

enum {
	VFS_HANDLE_NAMES = 3,
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

	if (bunix_ipc_call(VFS_HANDLE_NAMES, &request, &reply) != 0) {
		return -1;
	}

	return reply.words[0] == 0 ? 0 : -1;
}

static u64 resolve_service(u64 service, unsigned int rights)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_NAMES,
		.type = BUNIX_NAMES_WAIT,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { BUNIX_NAMES_ROOT, service, rights, 0 },
	};
	struct bunix_msg reply;

	if (bunix_ipc_call(VFS_HANDLE_NAMES, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return 0;
	}

	return reply.cap;
}

int main(void)
{
	const char online[] = "vfs: online\n";
	const char ready[] = "vfs: mounted block\n";
	struct bunix_msg message;
	u64 block = 0;

	bunix_console_write(online, sizeof(online) - 1);
	register_service(BUNIX_SERVICE_VFS, BUNIX_HANDLE_SELF);
	block = resolve_service(BUNIX_SERVICE_BLOCK, BUNIX_RIGHT_SEND);
	if (block == 0) {
		return 1;
	}
	bunix_console_write(ready, sizeof(ready) - 1);

	for (;;) {
		struct bunix_msg reply = {
			.protocol = BUNIX_PROTO_VFS,
			.type = 0,
			.sender = 0,
			.cap_rights = 0,
			.reply = 0,
			.cap = 0,
			.words = { 0, 0, 0, 0 },
		};

		if (bunix_ipc_recv(BUNIX_HANDLE_SELF, &message) != 0 ||
		    message.protocol != BUNIX_PROTO_VFS) {
			continue;
		}

		reply.type = message.type;
		switch (message.type) {
		case BUNIX_VFS_READ: {
			struct bunix_msg request = {
				.protocol = BUNIX_PROTO_BLOCK,
				.type = BUNIX_BLOCK_READ,
				.sender = 0,
				.cap_rights = 0,
				.reply = 0,
				.cap = 0,
				.words = { message.words[0], message.words[1], 0, 0 },
			};
			struct bunix_msg block_reply;

			if (bunix_ipc_call(block, &request, &block_reply) == 0 &&
			    block_reply.words[0] == 0) {
				reply.words[0] = 0;
				reply.words[1] = block_reply.words[1];
				reply.words[2] = block_reply.words[2];
				reply.words[3] = block_reply.words[3];
			} else {
				reply.words[0] = (u64)-1;
			}
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
