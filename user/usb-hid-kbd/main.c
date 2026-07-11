#include <bunix/syscall.h>

#define HID_HANDLE_USB (bunix_handle_find(BUNIX_CAP_USB))
#define HID_HANDLE_INPUT (bunix_handle_find(BUNIX_CAP_INP0))

enum {
	HID_MAX_DESCRIPTOR = 512,
	HID_BOOT_REPORT_LEN = 8,
	HID_CLASS = 3,
	HID_SUBCLASS_BOOT = 1,
	HID_PROTOCOL_KEYBOARD = 1,
	USB_ENDPOINT_ATTR_INTERRUPT = 3,
};

struct hid_keyboard {
	u64 device_index;
	u64 interface_number;
	u64 endpoint_address;
	u64 endpoint_handle;
	u64 modifiers;
	unsigned char keys[6];
};

static void append_char(char *out, u64 out_size, u64 *offset, char c)
{
	if (out == 0 || offset == 0 || *offset + 1 >= out_size) {
		return;
	}
	out[(*offset)++] = c;
	out[*offset] = '\0';
}

static void append_text(char *out, u64 out_size, u64 *offset, const char *text)
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

static void log_claimed(const struct hid_keyboard *keyboard)
{
	char line[128];
	u64 offset = 0;

	line[0] = '\0';
	append_text(line, sizeof(line), &offset,
		    "usb-hid-kbd: claimed device=");
	append_u64(line, sizeof(line), &offset, keyboard->device_index);
	append_text(line, sizeof(line), &offset, " interface=");
	append_u64(line, sizeof(line), &offset, keyboard->interface_number);
	append_text(line, sizeof(line), &offset, " endpoint=");
	append_u64(line, sizeof(line), &offset, keyboard->endpoint_address);
	append_char(line, sizeof(line), &offset, '\n');
	bunix_console_log(line, offset);
}

static int usb_call(struct bunix_msg *request, struct bunix_msg *reply)
{
	return bunix_ipc_call(HID_HANDLE_USB, request, reply) == 0 &&
		       reply->protocol == BUNIX_PROTO_USB &&
		       reply->words[0] == BUNIX_USB_OK ?
		       0 :
		       -1;
}

static long usb_device_count(void)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_USB,
		.type = BUNIX_USB_DEVICE_COUNT,
	};
	struct bunix_msg reply;

	if (usb_call(&request, &reply) != 0) {
		return -1;
	}
	return (long)reply.words[1];
}

static int usb_read_descriptor(u64 device_index, unsigned char *out,
			       u64 out_len, u64 *actual_len)
{
	long buffer = bunix_buffer_create(out_len);
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_USB,
		.type = BUNIX_USB_DESCRIPTOR_READ,
		.cap_rights = BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP,
		.cap = buffer > 0 ? (u64)buffer : 0,
		.words = { device_index, 0, out_len, 0 },
	};
	struct bunix_msg reply;

	if (buffer <= 0 || out == 0 || actual_len == 0 ||
	    usb_call(&request, &reply) != 0 ||
	    reply.words[1] > out_len ||
	    bunix_buffer_read((u64)buffer, 0, out, reply.words[1]) != 0) {
		if (buffer > 0) {
			bunix_handle_close((u64)buffer);
		}
		return -1;
	}
	*actual_len = reply.words[1];
	bunix_handle_close((u64)buffer);
	return 0;
}

static int usb_claim_interface(u64 device_index, u64 interface_number)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_USB,
		.type = BUNIX_USB_CLAIM_INTERFACE,
		.words = { device_index, interface_number, 0, 0 },
	};
	struct bunix_msg reply;

	return usb_call(&request, &reply);
}

static long usb_open_endpoint(u64 device_index, u64 interface_number,
			      u64 endpoint_address)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_USB,
		.type = BUNIX_USB_OPEN_ENDPOINT,
		.words = { device_index, interface_number, endpoint_address,
			   BUNIX_USB_TRANSFER_INTERRUPT |
				   (BUNIX_USB_DIR_IN << 8) },
	};
	struct bunix_msg reply;

	if (usb_call(&request, &reply) != 0 || reply.cap == 0 ||
	    (reply.cap_rights & BUNIX_RIGHT_SEND) == 0) {
		if (reply.cap != 0) {
			bunix_handle_close(reply.cap);
		}
		return -1;
	}
	return (long)reply.cap;
}

static int usage_present(const unsigned char *keys, unsigned char usage)
{
	for (u64 i = 0; i < 6; i++) {
		if (keys[i] == usage) {
			return 1;
		}
	}
	return 0;
}

static void send_key_event(u64 source, unsigned char usage, u64 pressed,
			   u64 modifiers)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_INPUT,
		.type = BUNIX_INPUT_KEY_EVENT,
		.words = { source, usage, pressed, modifiers },
	};
	struct bunix_msg reply;

	(void)bunix_ipc_call(HID_HANDLE_INPUT, &request, &reply);
}

static void parse_boot_report(struct hid_keyboard *keyboard,
			      const unsigned char *report, u64 len)
{
	unsigned char next_keys[6];
	u64 next_modifiers;

	if (keyboard == 0 || report == 0 || len < HID_BOOT_REPORT_LEN) {
		return;
	}
	next_modifiers = report[0];
	for (u64 i = 0; i < 6; i++) {
		next_keys[i] = report[2 + i];
	}
	for (u64 bit = 0; bit < 8; bit++) {
		const u64 mask = 1ull << bit;
		if ((keyboard->modifiers & mask) != (next_modifiers & mask)) {
			send_key_event(keyboard->device_index, 0xe0u + bit,
				       (next_modifiers & mask) != 0,
				       next_modifiers);
		}
	}
	for (u64 i = 0; i < 6; i++) {
		const unsigned char usage = keyboard->keys[i];

		if (usage != 0 && !usage_present(next_keys, usage)) {
			send_key_event(keyboard->device_index, usage, 0,
				       next_modifiers);
		}
	}
	for (u64 i = 0; i < 6; i++) {
		const unsigned char usage = next_keys[i];

		if (usage != 0 && !usage_present(keyboard->keys, usage)) {
			send_key_event(keyboard->device_index, usage, 1,
				       next_modifiers);
		}
	}
	keyboard->modifiers = next_modifiers;
	for (u64 i = 0; i < 6; i++) {
		keyboard->keys[i] = next_keys[i];
	}
}

static void poll_keyboard_endpoint(struct hid_keyboard *keyboard)
{
	unsigned char report[HID_BOOT_REPORT_LEN];
	long buffer;
	struct bunix_msg submit = {
		.protocol = BUNIX_PROTO_USB,
		.type = BUNIX_USB_SUBMIT_TRANSFER,
		.cap_rights = BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP,
		.words = { HID_BOOT_REPORT_LEN, 0, 0, 0 },
	};
	struct bunix_msg wait = {
		.protocol = BUNIX_PROTO_USB,
		.type = BUNIX_USB_WAIT_COMPLETION,
	};
	struct bunix_msg reply;

	if (keyboard == 0 || keyboard->endpoint_handle == 0) {
		return;
	}
	buffer = bunix_buffer_create(HID_BOOT_REPORT_LEN);
	if (buffer <= 0) {
		return;
	}
	submit.cap = (u64)buffer;
	if (bunix_ipc_call(keyboard->endpoint_handle, &submit, &reply) != 0 ||
	    reply.words[0] != BUNIX_USB_OK ||
	    bunix_ipc_call(keyboard->endpoint_handle, &wait, &reply) != 0 ||
	    reply.words[0] != BUNIX_USB_OK ||
	    reply.words[1] < HID_BOOT_REPORT_LEN ||
	    bunix_buffer_read((u64)buffer, 0, report, sizeof(report)) != 0) {
		bunix_handle_close((u64)buffer);
		return;
	}
	parse_boot_report(keyboard, report, sizeof(report));
	bunix_handle_close((u64)buffer);
}

static int claim_keyboard_from_descriptor(u64 device_index,
					  const unsigned char *descriptor,
					  u64 descriptor_len,
					  struct hid_keyboard *keyboard)
{
	u64 offset = 0;
	u64 current_interface = (u64)-1;
	int current_is_keyboard = 0;
	u64 keyboard_interface = (u64)-1;
	u64 interrupt_in_endpoint = 0;

	while (offset + 2 <= descriptor_len) {
		const u64 len = descriptor[offset];
		const u64 type = descriptor[offset + 1];

		if (len < 2 || offset + len > descriptor_len) {
			return -1;
		}
		if (type == BUNIX_USB_DESCRIPTOR_INTERFACE && len >= 9) {
			current_interface = descriptor[offset + 2];
			current_is_keyboard =
				descriptor[offset + 5] == HID_CLASS &&
				descriptor[offset + 6] == HID_SUBCLASS_BOOT &&
				descriptor[offset + 7] == HID_PROTOCOL_KEYBOARD;
			if (current_is_keyboard) {
				keyboard_interface = current_interface;
			}
		} else if (type == BUNIX_USB_DESCRIPTOR_ENDPOINT && len >= 7 &&
			   current_is_keyboard) {
			const u64 address = descriptor[offset + 2];
			const u64 attributes = descriptor[offset + 3];

			if ((address & 0x80u) != 0 &&
			    (attributes & 0x3u) == USB_ENDPOINT_ATTR_INTERRUPT) {
				interrupt_in_endpoint = address;
			}
		}
		offset += len;
	}
	if (keyboard_interface == (u64)-1 || interrupt_in_endpoint == 0) {
		return -1;
	}
	if (usb_claim_interface(device_index, keyboard_interface) != 0) {
		return -1;
	}
	keyboard->endpoint_handle = (u64)usb_open_endpoint(
		device_index, keyboard_interface, interrupt_in_endpoint);
	if (keyboard->endpoint_handle == 0) {
		return -1;
	}
	keyboard->device_index = device_index;
	keyboard->interface_number = keyboard_interface;
	keyboard->endpoint_address = interrupt_in_endpoint;
	keyboard->modifiers = 0;
	for (u64 i = 0; i < 6; i++) {
		keyboard->keys[i] = 0;
	}
	log_claimed(keyboard);
	return 0;
}

static int scan_for_keyboard(struct hid_keyboard *keyboard)
{
	unsigned char descriptor[HID_MAX_DESCRIPTOR];
	long count = usb_device_count();

	if (keyboard == 0 || keyboard->endpoint_handle != 0 || count <= 0) {
		return -1;
	}
	for (u64 i = 0; i < (u64)count; i++) {
		u64 descriptor_len = 0;

		if (usb_read_descriptor(i, descriptor, sizeof(descriptor),
					&descriptor_len) == 0 &&
		    claim_keyboard_from_descriptor(i, descriptor, descriptor_len,
						   keyboard) == 0) {
			return 0;
		}
	}
	return -1;
}

int main(void)
{
	const char online[] = "usb-hid-kbd: online\n";
	const char ready[] = "usb-hid-kbd: ready\n";
	struct hid_keyboard keyboard = { 0 };

	bunix_console_log(online, sizeof(online) - 1);
	bunix_console_log(ready, sizeof(ready) - 1);
	for (;;) {
		if (keyboard.endpoint_handle == 0) {
			(void)scan_for_keyboard(&keyboard);
			bunix_sleep_ns(100000000ull);
			continue;
		}
		poll_keyboard_endpoint(&keyboard);
		bunix_sleep_ns(10000000ull);
	}
}
