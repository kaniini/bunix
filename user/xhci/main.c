#include <bunix/syscall.h>

enum {
	XHCI_HANDLE_PCI = 4,
	PCI_CLASS_SERIAL_USB_XHCI = 0x0c0330,
	XHCI_PCI_BAR = 0,
	XHCI_CAP_LENGTH_VERSION = 0x00,
	XHCI_CAP_HCSPARAMS1 = 0x04,
	XHCI_CAP_HCCPARAMS1 = 0x10,
	XHCI_CAP_DBOFF = 0x14,
	XHCI_CAP_RTSOFF = 0x18,
	XHCI_OP_USBCMD = 0x00,
	XHCI_OP_USBSTS = 0x04,
	XHCI_OP_PAGESIZE = 0x08,
	XHCI_OP_PORTSC_BASE = 0x400,
	XHCI_OP_PORT_STRIDE = 0x10,
	XHCI_USBCMD_RUN = 1 << 0,
	XHCI_USBCMD_HCRST = 1 << 1,
	XHCI_USBSTS_HCH = 1 << 0,
	XHCI_WAIT_POLLS = 1000,
};

struct xhci_controller {
	u64 pci_index;
	u64 mmio;
	u64 cap_length;
	u64 hci_version;
	u64 max_slots;
	u64 max_ports;
	u64 hccparams1;
	u64 doorbell_offset;
	u64 runtime_offset;
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

static int pci_enable(u64 index)
{
	struct bunix_msg request = {
		.type = BUNIX_PCI_ENABLE,
		.words = { index, 0, 0, 0 },
	};
	struct bunix_msg reply;

	return pci_call(&request, &reply);
}

static int pci_get_bar(u64 index, u64 bar, struct bunix_msg *reply)
{
	struct bunix_msg request = {
		.type = BUNIX_PCI_GET_BAR,
		.words = { index, bar, 0, 0 },
	};

	return pci_call(&request, reply);
}

static long mmio_read32(u64 handle, u64 offset)
{
	return bunix_hw_mmio_read32(handle, offset);
}

static int mmio_write32(u64 handle, u64 offset, u64 value)
{
	return bunix_hw_mmio_write32(handle, offset, value) == 0 ? 0 : -1;
}

static int wait_status_bits(u64 handle, u64 op_base, u64 mask, u64 value)
{
	for (u64 i = 0; i < XHCI_WAIT_POLLS; i++) {
		const long status = mmio_read32(handle, op_base + XHCI_OP_USBSTS);

		if (status < 0) {
			return -1;
		}
		if ((((u64)status) & mask) == value) {
			return 0;
		}
		bunix_sleep_ns(1000000ull);
	}
	return -1;
}

static int xhci_map_controller(u64 pci_index, struct xhci_controller *out)
{
	struct bunix_msg bar_reply;
	long length_version;
	long hcsparams1;
	long hccparams1;
	long dboff;
	long rtsoff;

	if (out == 0 || pci_enable(pci_index) != 0 ||
	    pci_get_bar(pci_index, XHCI_PCI_BAR, &bar_reply) != 0 ||
	    bar_reply.cap == 0 ||
	    (bar_reply.words[1] & 0xffffu) != BUNIX_DEV_RESOURCE_MMIO) {
		return -1;
	}
	length_version = mmio_read32(bar_reply.cap, XHCI_CAP_LENGTH_VERSION);
	hcsparams1 = mmio_read32(bar_reply.cap, XHCI_CAP_HCSPARAMS1);
	hccparams1 = mmio_read32(bar_reply.cap, XHCI_CAP_HCCPARAMS1);
	dboff = mmio_read32(bar_reply.cap, XHCI_CAP_DBOFF);
	rtsoff = mmio_read32(bar_reply.cap, XHCI_CAP_RTSOFF);
	if (length_version < 0 || (((u64)length_version) & 0xffu) == 0 ||
	    hcsparams1 < 0 ||
	    hccparams1 < 0 || dboff < 0 || rtsoff < 0) {
		bunix_handle_close(bar_reply.cap);
		return -1;
	}
	out->pci_index = pci_index;
	out->mmio = bar_reply.cap;
	out->cap_length = ((u64)length_version) & 0xffu;
	out->hci_version = (((u64)length_version) >> 16) & 0xffffu;
	out->max_slots = ((u64)hcsparams1) & 0xffu;
	out->max_ports = (((u64)hcsparams1) >> 24) & 0xffu;
	out->hccparams1 = (u64)hccparams1;
	out->doorbell_offset = ((u64)dboff) & ~0x3ull;
	out->runtime_offset = ((u64)rtsoff) & ~0x1full;
	return 0;
}

static int xhci_reset_controller(const struct xhci_controller *controller)
{
	const u64 op_base = controller->cap_length;
	long command;

	command = mmio_read32(controller->mmio, op_base + XHCI_OP_USBCMD);
	if (command < 0) {
		return -1;
	}
	if ((((u64)command) & XHCI_USBCMD_RUN) != 0 &&
	    mmio_write32(controller->mmio, op_base + XHCI_OP_USBCMD,
			 ((u64)command) & ~XHCI_USBCMD_RUN) != 0) {
		return -1;
	}
	if (wait_status_bits(controller->mmio, op_base, XHCI_USBSTS_HCH,
			     XHCI_USBSTS_HCH) != 0) {
		return -1;
	}
	command = mmio_read32(controller->mmio, op_base + XHCI_OP_USBCMD);
	if (command < 0 ||
	    mmio_write32(controller->mmio, op_base + XHCI_OP_USBCMD,
			 ((u64)command) | XHCI_USBCMD_HCRST) != 0) {
		return -1;
	}
	for (u64 i = 0; i < XHCI_WAIT_POLLS; i++) {
		command = mmio_read32(controller->mmio,
				      op_base + XHCI_OP_USBCMD);
		if (command < 0) {
			return -1;
		}
		if ((((u64)command) & XHCI_USBCMD_HCRST) == 0) {
			return wait_status_bits(controller->mmio, op_base,
					       XHCI_USBSTS_HCH,
					       XHCI_USBSTS_HCH);
		}
		bunix_sleep_ns(1000000ull);
	}
	return -1;
}

static void log_controller(const struct xhci_controller *controller)
{
	char line[144];
	u64 offset = 0;

	line[0] = '\0';
	append_text(line, sizeof(line), &offset, "xhci: regs version=");
	append_u64(line, sizeof(line), &offset, controller->hci_version);
	append_text(line, sizeof(line), &offset, " slots=");
	append_u64(line, sizeof(line), &offset, controller->max_slots);
	append_text(line, sizeof(line), &offset, " ports=");
	append_u64(line, sizeof(line), &offset, controller->max_ports);
	append_text(line, sizeof(line), &offset, " dboff=");
	append_u64(line, sizeof(line), &offset, controller->doorbell_offset);
	append_text(line, sizeof(line), &offset, " rtsoff=");
	append_u64(line, sizeof(line), &offset, controller->runtime_offset);
	append_char(line, sizeof(line), &offset, '\n');
	bunix_console_log(line, offset);
}

static void log_port0(const struct xhci_controller *controller)
{
	char line[96];
	u64 offset = 0;
	long portsc = -1;

	if (controller->max_ports != 0) {
		portsc = mmio_read32(controller->mmio,
				     controller->cap_length +
					     XHCI_OP_PORTSC_BASE);
	}
	line[0] = '\0';
	append_text(line, sizeof(line), &offset, "xhci: reset ok port0=");
	append_u64(line, sizeof(line), &offset, portsc < 0 ? 0 : (u64)portsc);
	append_char(line, sizeof(line), &offset, '\n');
	bunix_console_log(line, offset);
}

int main(void)
{
	const char online[] = "xhci: online\n";
	const char ready[] = "xhci: ready skeleton\n";
	char line[96];
	u64 offset = 0;
	u64 found = 0;
	u64 first = 0;
	long count;

	bunix_console_log(online, sizeof(online) - 1);
	count = pci_count();
	if (count > 0) {
		for (u64 i = 0; i < (u64)count; i++) {
			u64 class_code = 0;

			if (pci_info(i, &class_code) == 0 &&
			    class_code == PCI_CLASS_SERIAL_USB_XHCI) {
				if (found == 0) {
					first = i;
				}
				found++;
			}
		}
	}

	line[0] = '\0';
	append_text(line, sizeof(line), &offset, "xhci: pci controllers=");
	append_u64(line, sizeof(line), &offset, found);
	append_text(line, sizeof(line), &offset, " dma=deferred\n");
	bunix_console_log(line, offset);
	if (found != 0) {
		struct xhci_controller controller;

		if (xhci_map_controller(first, &controller) == 0) {
			log_controller(&controller);
			if (xhci_reset_controller(&controller) == 0) {
				log_port0(&controller);
			}
		}
	}
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
