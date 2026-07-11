#include <bunix/syscall.h>

static u64 input_events;
static u64 input_key_events;

static long register_service(u64 handle)
{
	return bunix_names_register_claim(bunix_handle_find(BUNIX_CAP_CLAM),
					  handle);
}

static void append_char(char *out, u64 out_size, u64 *offset, char c)
{
	if (out == 0 || offset == 0 || *offset + 1 >= out_size) {
		return;
	}
	out[(*offset)++] = c;
	out[*offset] = '\0';
}

static void append_text(char *out, u64 out_size, u64 *offset,
			const char *text)
{
	if (text == 0) {
		return;
	}
	for (u64 i = 0; text[i] != '\0'; i++) {
		append_char(out, out_size, offset, text[i]);
	}
}

static void append_u64(char *out, u64 out_size, u64 *offset, u64 value)
{
	char digits[20];
	u64 count = 0;

	if (value == 0) {
		append_char(out, out_size, offset, '0');
		return;
	}
	while (value != 0 && count < sizeof(digits)) {
		digits[count++] = (char)('0' + value % 10);
		value /= 10;
	}
	while (count != 0) {
		append_char(out, out_size, offset, digits[--count]);
	}
}

static void log_key_event(const struct bunix_msg *message)
{
	char line[128];
	u64 offset = 0;

	line[0] = '\0';
	append_text(line, sizeof(line), &offset, "input: key source=");
	append_u64(line, sizeof(line), &offset, message->words[0]);
	append_text(line, sizeof(line), &offset, " usage=");
	append_u64(line, sizeof(line), &offset, message->words[1]);
	append_text(line, sizeof(line), &offset, " pressed=");
	append_u64(line, sizeof(line), &offset, message->words[2]);
	append_text(line, sizeof(line), &offset, " mods=");
	append_u64(line, sizeof(line), &offset, message->words[3]);
	append_char(line, sizeof(line), &offset, '\n');
	bunix_console_log(line, offset);
}

int main(void)
{
	const char online[] = "input: online\n";
	const char ready[] = "input: ready\n";
	struct bunix_msg message;

	bunix_console_log(online, sizeof(online) - 1);
	if (register_service(BUNIX_HANDLE_SELF) != 0) {
		return 1;
	}
	bunix_console_log(ready, sizeof(ready) - 1);

	for (;;) {
		struct bunix_msg reply = {
			.protocol = BUNIX_PROTO_INPUT,
			.type = 0,
			.words = { BUNIX_INPUT_ERR_INVAL, 0, 0, 0 },
		};

		if (bunix_ipc_recv(BUNIX_HANDLE_SELF, &message) != 0 ||
		    message.protocol != BUNIX_PROTO_INPUT) {
			continue;
		}

		reply.type = message.type;
		switch (message.type) {
		case BUNIX_INPUT_KEY_EVENT:
			input_events++;
			input_key_events++;
			log_key_event(&message);
			reply.words[0] = BUNIX_INPUT_OK;
			reply.words[1] = input_events;
			reply.words[2] = input_key_events;
			break;
		case BUNIX_INPUT_STATS:
			reply.words[0] = BUNIX_INPUT_OK;
			reply.words[1] = input_events;
			reply.words[2] = input_key_events;
			break;
		default:
			break;
		}

		if (message.cap != 0) {
			bunix_handle_close(message.cap);
		}
		if (message.reply != 0) {
			(void)bunix_ipc_send(message.reply, &reply);
		}
	}
}
