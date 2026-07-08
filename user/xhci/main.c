#include <bunix/syscall.h>

enum {
	XHCI_HANDLE_PCI = 4,
	PCI_CLASS_SERIAL_USB_XHCI = 0x0c0330,
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

static int pci_call(struct bunix_msg *request, struct bunix_msg *reply)
{
	if (request == 0 || reply == 0) {
		return -1;
	}
	request->protocol = BUNIX_PROTO_PCI;
	return bunix_ipc_call(XHCI_HANDLE_PCI, request, reply) == 0 &&
		       reply->protocol == BUNIX_PROTO_PCI &&
		       reply->words[0] == BUNIX_PCI_OK ?
	       0 :
	       -1;
}

static long pci_count(void)
{
	struct bunix_msg request = {
		.type = BUNIX_PCI_ENUMERATE,
		.words = { 0, 0, 0, 0 },
	};
	struct bunix_msg reply;

	return pci_call(&request, &reply) == 0 ? (long)reply.words[1] : -1;
}

static int pci_info(u64 index, u64 *class_code)
{
	struct bunix_msg request = {
		.type = BUNIX_PCI_GET_INFO,
		.words = { index, 0, 0, 0 },
	};
	struct bunix_msg reply;

	if (pci_call(&request, &reply) != 0) {
		return -1;
	}
	*class_code = reply.words[3] & 0xffffffu;
	return 0;
}

int main(void)
{
	const char online[] = "xhci: online\n";
	const char ready[] = "xhci: ready skeleton\n";
	char line[96];
	u64 offset = 0;
	u64 found = 0;
	long count;

	bunix_console_log(online, sizeof(online) - 1);
	count = pci_count();
	if (count > 0) {
		for (u64 i = 0; i < (u64)count; i++) {
			u64 class_code = 0;

			if (pci_info(i, &class_code) == 0 &&
			    class_code == PCI_CLASS_SERIAL_USB_XHCI) {
				found++;
			}
		}
	}

	line[0] = '\0';
	append_text(line, sizeof(line), &offset, "xhci: pci controllers=");
	append_u64(line, sizeof(line), &offset, found);
	append_text(line, sizeof(line), &offset, " dma=deferred\n");
	bunix_console_log(line, offset);
	bunix_console_log(ready, sizeof(ready) - 1);

	for (;;) {
		struct bunix_msg message;

		if (bunix_ipc_recv(BUNIX_HANDLE_SELF, &message) == 0 &&
		    message.reply != 0) {
			struct bunix_msg reply = {
				.protocol = BUNIX_PROTO_USBHC,
				.type = message.type,
				.words = { BUNIX_USB_ERR_UNSUPPORTED, 0, 0, 0 },
			};
			(void)bunix_ipc_send(message.reply, &reply);
		}
	}
}
