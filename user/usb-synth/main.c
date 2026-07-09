#include <bunix/syscall.h>

#define USB_SYNTH_HANDLE_USB (bunix_handle_find(BUNIX_CAP_USB))

static const unsigned char synthetic_keyboard_descriptors[] = {
	18, 1, 0x00, 0x02, 0x00, 0x00, 0x00, 64,
	0x34, 0x12, 0x78, 0x56, 0x00, 0x01, 1, 2, 3, 1,
	9, 2, 25, 0, 1, 1, 0, 0xa0, 50,
	9, 4, 0, 0, 1, 3, 1, 1, 0,
	7, 5, 0x81, 3, 8, 0, 10,
};

int main(void)
{
	const char online[] = "usb-synth: online\n";
	const char registered[] = "usb-synth: registered keyboard\n";
	const char failed[] = "usb-synth: register failed\n";
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_USB,
		.type = BUNIX_USB_CONTROLLER_REGISTER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_RECV | BUNIX_RIGHT_DUP,
		.reply = 0,
		.cap = 0,
		.words = { sizeof(synthetic_keyboard_descriptors), 1, 0, 0 },
	};
	struct bunix_msg reply;
	long buffer;

	bunix_console_log(online, sizeof(online) - 1);
	buffer = bunix_buffer_create(sizeof(synthetic_keyboard_descriptors));
	if (buffer <= 0 ||
	    bunix_buffer_write((u64)buffer, 0, synthetic_keyboard_descriptors,
			       sizeof(synthetic_keyboard_descriptors)) != 0) {
		bunix_console_log(failed, sizeof(failed) - 1);
		return 1;
	}
	request.cap = (u64)buffer;
	if (bunix_ipc_call(USB_SYNTH_HANDLE_USB, &request, &reply) != 0 ||
	    reply.protocol != BUNIX_PROTO_USB ||
	    reply.words[0] != BUNIX_USB_OK) {
		bunix_console_log(failed, sizeof(failed) - 1);
		bunix_handle_close((u64)buffer);
		return 1;
	}
	bunix_handle_close((u64)buffer);
	bunix_console_log(registered, sizeof(registered) - 1);

	for (;;) {
		struct bunix_msg message;

		if (bunix_ipc_recv(BUNIX_HANDLE_SELF, &message) == 0 &&
		    message.reply != 0) {
			struct bunix_msg response = {
				.protocol = BUNIX_PROTO_USBHC,
				.type = message.type,
				.words = { BUNIX_USB_ERR_UNSUPPORTED, 0, 0, 0 },
			};
			(void)bunix_ipc_send(message.reply, &response);
		}
	}
}
