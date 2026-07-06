#include <bunix/syscall.h>

enum {
	CONSOLE_CHUNK = 256,
	CONSOLE_HANDLE_NAMES = 3,
	CONSOLE_HANDLE_COM1 = 4,
	COM1_DATA = 0,
	COM1_INT_ENABLE = 1,
	COM1_FIFO_CTRL = 2,
	COM1_LINE_CTRL = 3,
	COM1_MODEM_CTRL = 4,
	COM1_LINE_STATUS = 5,
	COM1_LINE_STATUS_TX_EMPTY = 0x20,
};

static char console_buffer[CONSOLE_CHUNK];
static int console_serial_ready;

static int serial_out8(u64 offset, u64 value)
{
	return bunix_hw_port_out8(CONSOLE_HANDLE_COM1, offset, value) == 0 ?
	       0 : -1;
}

static long serial_in8(u64 offset)
{
	return bunix_hw_port_in8(CONSOLE_HANDLE_COM1, offset);
}

static int serial_init(void)
{
	if (serial_out8(COM1_INT_ENABLE, 0x00) != 0 ||
	    serial_out8(COM1_LINE_CTRL, 0x80) != 0 ||
	    serial_out8(COM1_DATA, 0x03) != 0 ||
	    serial_out8(COM1_INT_ENABLE, 0x00) != 0 ||
	    serial_out8(COM1_LINE_CTRL, 0x03) != 0 ||
	    serial_out8(COM1_FIFO_CTRL, 0xc7) != 0 ||
	    serial_out8(COM1_MODEM_CTRL, 0x0b) != 0) {
		return -1;
	}
	return 0;
}

static int serial_putc(char c)
{
	for (;;) {
		const long status = serial_in8(COM1_LINE_STATUS);

		if (status < 0) {
			return -1;
		}
		if ((status & COM1_LINE_STATUS_TX_EMPTY) != 0) {
			break;
		}
	}

	return serial_out8(COM1_DATA, (u64)(unsigned char)c);
}

static int serial_write(const char *text, u64 len)
{
	if (!console_serial_ready) {
		return -1;
	}
	for (u64 i = 0; i < len; i++) {
		if (serial_putc(text[i]) != 0) {
			return -1;
		}
	}
	return 0;
}

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
		} else if (serial_write(console_buffer, chunk) != 0) {
			bunix_early_console_write(console_buffer, chunk);
		}
		offset += chunk;
	}
}

int main(void)
{
	const char online[] = "console: online\n";
	struct bunix_msg message;

	console_serial_ready = serial_init() == 0;
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
