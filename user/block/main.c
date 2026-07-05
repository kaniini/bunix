#include <bunix/libbunix.h>

enum {
	BLOCK_HANDLE_NAMES = 3,
	BLOCK_BUFFER_MAX = 128 * 1024,
};

static unsigned char block_buffer[BLOCK_BUFFER_MAX];

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

	if (bunix_ipc_call(BLOCK_HANDLE_NAMES, &request, &reply) != 0) {
		return -1;
	}

	return reply.words[0] == 0 ? 0 : -1;
}

static void pack_bytes(u64 *words, const unsigned char *data, u64 len)
{
	words[0] = 0;
	words[1] = 0;

	for (u64 i = 0; i < len && i < BUNIX_IPC_DATA_BYTES; i++) {
		const u64 slot = i / 8;
		const u64 shift = (i % 8) * 8;

		words[slot] |= ((u64)(unsigned char)data[i]) << shift;
	}
}

int main(void)
{
	const char online[] = "block: online\n";
	struct bunix_msg message;
	const u64 disk_size = bunix_boot_module_size();

	bunix_console_log(online, sizeof(online) - 1);
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
			reply.words[1] = disk_size;
			reply.words[2] = 16;
			break;
		case BUNIX_BLOCK_READ: {
			const u64 offset = message.words[0];
			u64 len = message.words[1];
			unsigned char buffer[BUNIX_IPC_DATA_BYTES];

			if (offset >= disk_size) {
				len = 0;
			} else if (len > disk_size - offset) {
				len = disk_size - offset;
			}
			if (len > BUNIX_IPC_DATA_BYTES) {
				len = BUNIX_IPC_DATA_BYTES;
			}

			reply.words[0] = 0;
			reply.words[1] = len;
			if (len != 0 &&
			    bunix_boot_module_read(offset, buffer, len) != 0) {
				reply.words[0] = (u64)-1;
				reply.words[1] = 0;
			} else {
				pack_bytes(&reply.words[2], buffer, len);
			}
			break;
		}
		case BUNIX_BLOCK_READ_BUFFER: {
			const u64 offset = message.words[0];
			const u64 buffer_offset = message.words[2];
			u64 len = message.words[1];

			if (message.cap == 0 ||
			    (message.cap_rights & BUNIX_RIGHT_SEND) == 0) {
				reply.words[0] = (u64)-1;
				break;
			}

			if (offset >= disk_size) {
				len = 0;
			} else if (len > disk_size - offset) {
				len = disk_size - offset;
			}
			if (len > sizeof(block_buffer)) {
				len = sizeof(block_buffer);
			}

			reply.words[0] = 0;
			reply.words[1] = len;
			if (len != 0 &&
			    (bunix_boot_module_read(offset, block_buffer,
						    len) != 0 ||
			     bunix_buffer_write(message.cap, buffer_offset,
						block_buffer, len) != 0)) {
				reply.words[0] = (u64)-1;
				reply.words[1] = 0;
			}
			break;
		}
		default:
			reply.words[0] = (u64)-1;
			break;
		}

		if (message.cap != 0) {
			bunix_handle_close(message.cap);
		}
		if (message.reply != 0) {
			bunix_ipc_send(message.reply, &reply);
		}
	}
}
