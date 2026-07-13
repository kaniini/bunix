#include <bunix/driver.h>

enum {
	CONSOLE_CHUNK = 256,
	COM1_DATA = 0,
	COM1_INT_ENABLE = 1,
	COM1_FIFO_CTRL = 2,
	COM1_LINE_CTRL = 3,
	COM1_MODEM_CTRL = 4,
	COM1_LINE_STATUS = 5,
	COM1_INT_RX_AVAILABLE = 0x01,
	COM1_LINE_STATUS_DATA_READY = 0x01,
	COM1_LINE_STATUS_TX_EMPTY = 0x20,
};

static char console_buffer[CONSOLE_CHUNK];
static char console_input_buffer[CONSOLE_CHUNK];
static u64 console_serial_handle;
static u64 console_irq_handle;
static u64 console_irq_port;
static u64 console_service_handle;
static u64 console_names_handle;
static u64 console_linux_handle;
static int console_serial_ready;
static int console_irq_ready;
static int console_driver_output_enabled;

static int console_poll_input(void);

static int serial_out8(u64 offset, u64 value)
{
	return bunix_hw_port_out8(console_serial_handle, offset, value) == 0 ?
	       0 : -1;
}

static long serial_in8(u64 offset)
{
	return bunix_hw_port_in8(console_serial_handle, offset);
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

static int serial_irq_init(void)
{
	long irq_port;

	if (!console_serial_ready || console_irq_handle == 0) {
		return -1;
	}

	irq_port = bunix_port_create("console-irq");
	if (irq_port <= 0) {
		return -1;
	}
	if (bunix_hw_irq_bind(console_irq_handle, 0, (u64)irq_port) != 0) {
		bunix_handle_close((u64)irq_port);
		return -1;
	}
	console_irq_port = (u64)irq_port;
	(void)console_poll_input();
	if (serial_out8(COM1_INT_ENABLE, COM1_INT_RX_AVAILABLE) != 0 ||
	    bunix_hw_irq_ack(console_irq_handle, 0) != 0 ||
	    bunix_hw_irq_mask(console_irq_handle, 0, 0) != 0) {
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

static int serial_try_getc(char *out)
{
	const long status = serial_in8(COM1_LINE_STATUS);
	long value;

	if (out == 0 || status < 0 ||
	    (status & COM1_LINE_STATUS_DATA_READY) == 0) {
		return 0;
	}

	value = serial_in8(COM1_DATA);
	if (value < 0) {
		return 0;
	}
	*out = (char)(unsigned char)value;
	return 1;
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
		} else if (!console_driver_output_enabled ||
			   serial_write(console_buffer, chunk) != 0) {
			bunix_early_console_write(console_buffer, chunk);
		}
		offset += chunk;
	}
}

static u64 resolve_linux(void)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_NAMES,
		.type = BUNIX_NAMES_RESOLVE,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { BUNIX_NAMES_ROOT, BUNIX_SERVICE_LINUX,
			   BUNIX_RIGHT_SEND, 0 },
	};
	struct bunix_msg reply;

	if (bunix_ipc_call(console_names_handle, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return 0;
	}
	return reply.cap;
}

static int console_forward_input_buffer(const char *input, u64 len)
{
	long buffer;
	struct bunix_msg message = {
		.protocol = BUNIX_PROTO_LINUX,
		.type = BUNIX_LINUX_TTY_INPUT_BUFFER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_RECV,
		.reply = 0,
		.cap = 0,
		.words = { len, 0, 0, 0 },
	};

	if (input == 0 || len == 0) {
		return 0;
	}
	if (console_linux_handle == 0) {
		console_linux_handle = resolve_linux();
	}
	if (console_linux_handle == 0) {
		return -1;
	}
	buffer = bunix_buffer_create(len);
	if (buffer < 0 ||
	    bunix_buffer_write((u64)buffer, 0, input, len) != 0) {
		if (buffer >= 0) {
			bunix_handle_close((u64)buffer);
		}
		return -1;
	}
	message.cap = (u64)buffer;
	if (bunix_ipc_send(console_linux_handle, &message) != 0) {
		bunix_handle_close(console_linux_handle);
		console_linux_handle = 0;
		bunix_handle_close((u64)buffer);
		return -1;
	}
	bunix_handle_close((u64)buffer);
	return 0;
}

static int console_poll_input(void)
{
	int any = 0;
	char c;
	u64 len = 0;

	if (!console_serial_ready) {
		return 0;
	}
	while (serial_try_getc(&c)) {
		console_input_buffer[len++] = c;
		if (len == sizeof(console_input_buffer)) {
			(void)console_forward_input_buffer(console_input_buffer, len);
			len = 0;
		}
		any = 1;
	}
	if (len != 0) {
		(void)console_forward_input_buffer(console_input_buffer, len);
	}
	return any;
}

static void console_handle_irq(const struct bunix_msg *message)
{
	if (message == 0 || message->protocol != BUNIX_PROTO_HW ||
	    message->type != BUNIX_HW_EVENT_IRQ) {
		return;
	}

	(void)console_poll_input();
	(void)bunix_hw_irq_ack(console_irq_handle, 0);
	(void)bunix_hw_irq_mask(console_irq_handle, 0, 0);
}

static void console_handle_message(struct bunix_msg *message)
{
	if (message == 0 || message->protocol != BUNIX_PROTO_CONSOLE) {
		return;
	}

	if (message->type == BUNIX_CONSOLE_LOGS_TO_RING) {
		bunix_early_console_logs_to_ring();
		console_driver_output_enabled = console_serial_ready;
	} else if (message->type == BUNIX_CONSOLE_WRITE ||
	    message->type == BUNIX_CONSOLE_LOG) {
		if (message->cap != 0 && message->words[0] != 0) {
			console_emit(message->cap, message->words[0],
				     message->type == BUNIX_CONSOLE_LOG);
			bunix_handle_close(message->cap);
		}
	}
	if (message->reply != 0) {
		struct bunix_msg reply = {
			.protocol = BUNIX_PROTO_CONSOLE,
			.type = message->type,
			.sender = 0,
			.cap_rights = 0,
			.reply = 0,
			.cap = 0,
			.words = { 0, 0, 0, 0 },
		};

		bunix_ipc_send(message->reply, &reply);
		bunix_handle_close(message->reply);
	}
}

int main(void)
{
	const char online[] = "console: online\n";
	struct bunix_msg message;
	struct bunix_driver_resource console_resources[] = {
		{
			.name = "com1",
			.handle = bunix_handle_find(BUNIX_CAP_COM1),
			.kind = BUNIX_HW_RESOURCE_PORT,
			.ops = BUNIX_HW_OP_READ | BUNIX_HW_OP_WRITE,
			.base = 0x3f8,
			.len = 8,
		},
		{
			.name = "com1-irq",
			.handle = bunix_handle_find(BUNIX_CAP_CIRQ),
			.kind = BUNIX_HW_RESOURCE_IRQ,
			.ops = BUNIX_HW_OP_BIND_IRQ | BUNIX_HW_OP_ACK_IRQ |
			       BUNIX_HW_OP_MASK_IRQ,
			.base = 4,
			.len = 1,
		},
	};
	struct bunix_driver console_driver = {
		.name = "console",
		.service = BUNIX_SERVICE_CONSOLE,
		.service_handle = bunix_handle_find(BUNIX_CAP_CONS),
		.names_handle = bunix_handle_find(BUNIX_CAP_NAME),
		.resources = console_resources,
		.resource_count = sizeof(console_resources) /
				  sizeof(console_resources[0]),
	};
	const struct bunix_driver_resource *com1 =
		bunix_driver_resource_named(&console_driver, "com1");
	const struct bunix_driver_resource *com1_irq =
		bunix_driver_resource_named(&console_driver, "com1-irq");

	console_service_handle = console_driver.service_handle;
	console_names_handle = console_driver.names_handle;
	console_serial_handle = com1 != 0 ? com1->handle : 0;
	console_irq_handle = com1_irq != 0 ? com1_irq->handle : 0;
	console_serial_ready = serial_init() == 0;
	console_irq_ready = serial_irq_init() == 0;
	bunix_early_console_log(online, sizeof(online) - 1);
	(void)bunix_driver_register(&console_driver);
	bunix_driver_log_lifecycle(&console_driver,
				   console_irq_ready ? "driver online irq" :
				   (console_serial_ready ?
				    "driver online polling" :
				    "driver fallback"));
	if (console_irq_ready) {
		u64 ports[2] = { console_service_handle, console_irq_port };

		for (;;) {
			u64 index = 0;

			if (bunix_ipc_recv_any(ports, 2, &message, &index) != 0) {
				continue;
			}
			if (index == 1) {
				console_handle_irq(&message);
			} else {
				console_handle_message(&message);
			}
		}
	}
	for (;;) {
		const long recv =
			bunix_ipc_try_recv(console_service_handle, &message);
		int did_work = console_poll_input();

		if (recv == 1) {
			if (!did_work) {
				bunix_sleep_ns(1000000ull);
			}
			continue;
		}
		if (recv != 0 || message.protocol != BUNIX_PROTO_CONSOLE) {
			continue;
		}
		did_work = 1;
		console_handle_message(&message);
		(void)did_work;
	}
}
