#include <bunix/libbunix.h>

static void unpack_bytes(char *out, const u64 *words, u64 len)
{
	for (u64 i = 0; i < len; i++) {
		const u64 slot = i / 8;
		const u64 shift = (i % 8) * 8;

		out[i] = (char)((words[slot] >> shift) & 0xff);
	}
}

int main(void)
{
	const char online[] = "linux-server: online\n";
	const char write_ok[] = "linux-server: write\n";
	const char exit_group[] = "linux-server: exit_group\n";
	struct bunix_msg message;

	bunix_console_write(online, sizeof(online) - 1);
	for (;;) {
		struct bunix_msg reply = {
			.protocol = BUNIX_PROTO_LINUX,
			.type = 0,
			.sender = 0,
			.cap_rights = 0,
			.reply = 0,
			.cap = 0,
			.words = { 0, 0, 0, 0 },
		};

		if (bunix_ipc_recv(BUNIX_HANDLE_SELF, &message) != 0 ||
		    message.protocol != BUNIX_PROTO_LINUX) {
			continue;
		}

		reply.type = message.type;
		switch (message.type) {
		case BUNIX_LINUX_WRITE: {
			char text[16];
			const u64 fd = message.words[0];
			const u64 len = message.words[1];

			if ((fd == 1 || fd == 2) && len <= sizeof(text)) {
				unpack_bytes(text, &message.words[2], len);
				bunix_console_write(text, len);
				bunix_console_write(write_ok, sizeof(write_ok) - 1);
				reply.words[0] = len;
			} else {
				reply.words[0] = (u64)-1;
			}
			break;
		}
		case BUNIX_LINUX_EXIT_GROUP:
			bunix_console_write(exit_group, sizeof(exit_group) - 1);
			reply.words[0] = 0;
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
