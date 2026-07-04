#include <bunix/syscall.h>

enum {
	CONSOLE_CHUNK = 256,
	CONSOLE_HANDLE_NAMES = 3,
};

static char console_buffer[CONSOLE_CHUNK];

static void register_console(void)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_NAMES,
		.type = BUNIX_NAMES_REGISTER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP,
		.reply = 0,
		.cap = BUNIX_HANDLE_CONSOLE,
		.words = { BUNIX_NAMES_ROOT, BUNIX_SERVICE_CONSOLE, 0, 0 },
	};
	struct bunix_msg reply;

	(void)bunix_ipc_call(CONSOLE_HANDLE_NAMES, &request, &reply);
}

static void console_emit(u64 buffer, u64 len, int log)
{
	u64 offset = 0;

	while (offset < len) {
		const u64 chunk = len - offset < sizeof(console_buffer) ?
			len - offset : sizeof(console_buffer);

		if (bunix_buffer_read(buffer, offset, console_buffer, chunk) != 0) {
			break;
		}
		if (log) {
			bunix_early_console_log(console_buffer, chunk);
		} else {
			bunix_early_console_write(console_buffer, chunk);
		}
		offset += chunk;
	}
}

int main(void)
{
	const char online[] = "console: online\n";
	struct bunix_msg message;

	bunix_early_console_log(online, sizeof(online) - 1);
	register_console();
	for (;;) {
		if (bunix_ipc_recv(BUNIX_HANDLE_CONSOLE, &message) != 0 ||
		    message.protocol != BUNIX_PROTO_CONSOLE) {
			continue;
		}

		if (message.type == BUNIX_CONSOLE_LOGS_TO_RING) {
			bunix_early_console_logs_to_ring();
		} else if (message.type == BUNIX_CONSOLE_WRITE ||
		    message.type == BUNIX_CONSOLE_LOG) {
			if (message.cap != 0 && message.words[0] != 0) {
				console_emit(message.cap, message.words[0],
					     message.type == BUNIX_CONSOLE_LOG);
				bunix_handle_close(message.cap);
			}
		}
		if (message.reply != 0) {
			struct bunix_msg reply = {
				.protocol = BUNIX_PROTO_CONSOLE,
				.type = message.type,
				.sender = 0,
				.cap_rights = 0,
				.reply = 0,
				.cap = 0,
				.words = { 0, 0, 0, 0 },
			};

			bunix_ipc_send(message.reply, &reply);
			bunix_handle_close(message.reply);
		}
	}
}
