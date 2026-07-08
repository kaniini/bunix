#include <bunix/syscall.h>

enum {
	XHCI_HANDLE_PCI = 4,
	PCI_CLASS_SERIAL_USB_XHCI = 0x0c0330,
	XHCI_PCI_BAR = 0,
	XHCI_CAP_LENGTH_VERSION = 0x00,
	XHCI_CAP_HCSPARAMS1 = 0x04,
	XHCI_CAP_HCSPARAMS2 = 0x08,
	XHCI_CAP_HCCPARAMS1 = 0x10,
	XHCI_CAP_DBOFF = 0x14,
	XHCI_CAP_RTSOFF = 0x18,
	XHCI_OP_USBCMD = 0x00,
	XHCI_OP_USBSTS = 0x04,
	XHCI_OP_PAGESIZE = 0x08,
	XHCI_OP_CRCR = 0x18,
	XHCI_OP_DCBAAP = 0x30,
	XHCI_OP_CONFIG = 0x38,
	XHCI_OP_PORTSC_BASE = 0x400,
	XHCI_OP_PORT_STRIDE = 0x10,
	XHCI_RUNTIME_IR0 = 0x20,
	XHCI_IR_ERSTSZ = 0x08,
	XHCI_IR_ERSTBA = 0x10,
	XHCI_IR_ERDP = 0x18,
	XHCI_USBCMD_RUN = 1 << 0,
	XHCI_USBCMD_HCRST = 1 << 1,
	XHCI_USBSTS_HCH = 1 << 0,
	XHCI_TRB_TYPE_LINK = 6,
	XHCI_TRB_CYCLE = 1 << 0,
	XHCI_TRB_TOGGLE_CYCLE = 1 << 1,
	XHCI_WAIT_POLLS = 1000,
	XHCI_PAGE_SIZE = 4096,
	XHCI_RING_TRBS = 256,
	XHCI_TRB_SIZE = 16,
	XHCI_ERST_ENTRIES = 1,
	XHCI_ERST_ENTRY_SIZE = 16,
	XHCI_MAX_SCRATCHPADS = 16,
};

struct xhci_controller {
	u64 pci_index;
	u64 mmio;
	u64 cap_length;
	u64 hci_version;
	u64 max_slots;
	u64 max_ports;
	u64 max_scratchpads;
	u64 hccparams1;
	u64 doorbell_offset;
	u64 runtime_offset;
};

struct xhci_dma_buffer {
	u64 handle;
	u64 phys;
	u64 offset;
	u64 size;
};

struct xhci_rings {
	struct xhci_dma_buffer dcbaa;
	struct xhci_dma_buffer command_ring;
	struct xhci_dma_buffer event_ring;
	struct xhci_dma_buffer erst;
	struct xhci_dma_buffer scratchpad_array;
	struct xhci_dma_buffer scratchpad[XHCI_MAX_SCRATCHPADS];
	u64 scratchpad_count;
};

static const unsigned char zeroes[64];

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

static u64 align_up(u64 value, u64 alignment)
{
	return alignment == 0 ? value : (value + alignment - 1) &
				       ~(alignment - 1);
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

static int mmio_write64(u64 handle, u64 offset, u64 value)
{
	return mmio_write32(handle, offset, value & 0xffffffffull) == 0 &&
	       mmio_write32(handle, offset + 4, value >> 32) == 0 ?
		       0 :
		       -1;
}

static int buffer_zero(u64 handle, u64 offset, u64 len)
{
	u64 done = 0;

	while (done < len) {
		u64 chunk = len - done;

		if (chunk > sizeof(zeroes)) {
			chunk = sizeof(zeroes);
		}
		if (bunix_buffer_write(handle, offset + done, zeroes,
				       chunk) != 0) {
			return -1;
		}
		done += chunk;
	}
	return 0;
}

static int buffer_write32(u64 handle, u64 offset, u64 value)
{
	const unsigned int low = (unsigned int)value;

	return bunix_buffer_write(handle, offset, &low, sizeof(low)) == 0 ? 0 :
									-1;
}

static int buffer_write64(u64 handle, u64 offset, u64 value)
{
	const unsigned int low = (unsigned int)(value & 0xffffffffull);
	const unsigned int high = (unsigned int)(value >> 32);

	return bunix_buffer_write(handle, offset, &low, sizeof(low)) == 0 &&
	       bunix_buffer_write(handle, offset + 4, &high, sizeof(high)) == 0 ?
		       0 :
		       -1;
}

static int dma_alloc(struct xhci_dma_buffer *buffer, u64 size, u64 alignment)
{
	long handle;
	long phys;
	u64 aligned;

	if (buffer == 0 || size == 0 || alignment == 0) {
		return -1;
	}
	handle = bunix_buffer_create(size + alignment - 1);
	if (handle <= 0) {
		return -1;
	}
	phys = bunix_buffer_phys((u64)handle);
	if (phys <= 0) {
		bunix_handle_close((u64)handle);
		return -1;
	}
	aligned = align_up((u64)phys, alignment);
	buffer->handle = (u64)handle;
	buffer->phys = aligned;
	buffer->offset = aligned - (u64)phys;
	buffer->size = size;
	if (buffer->offset + size > size + alignment - 1 ||
	    buffer_zero(buffer->handle, buffer->offset, buffer->size) != 0) {
		bunix_handle_close(buffer->handle);
		buffer->handle = 0;
		return -1;
	}
	return 0;
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
	long hcsparams2;
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
	hcsparams2 = mmio_read32(bar_reply.cap, XHCI_CAP_HCSPARAMS2);
	hccparams1 = mmio_read32(bar_reply.cap, XHCI_CAP_HCCPARAMS1);
	dboff = mmio_read32(bar_reply.cap, XHCI_CAP_DBOFF);
	rtsoff = mmio_read32(bar_reply.cap, XHCI_CAP_RTSOFF);
	if (length_version < 0 || (((u64)length_version) & 0xffu) == 0 ||
	    hcsparams1 < 0 || hcsparams2 < 0 ||
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
	out->max_scratchpads = ((((u64)hcsparams2) >> 27) & 0x1fu) |
			       (((((u64)hcsparams2) >> 21) & 0x1fu) << 5);
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

static int xhci_setup_scratchpads(const struct xhci_controller *controller,
				  struct xhci_rings *rings)
{
	if (controller->max_scratchpads == 0) {
		return buffer_write64(rings->dcbaa.handle, rings->dcbaa.offset, 0);
	}
	if (controller->max_scratchpads > XHCI_MAX_SCRATCHPADS ||
	    dma_alloc(&rings->scratchpad_array,
		      controller->max_scratchpads * sizeof(u64), 64) != 0) {
		return -1;
	}
	for (u64 i = 0; i < controller->max_scratchpads; i++) {
		if (dma_alloc(&rings->scratchpad[i], XHCI_PAGE_SIZE,
			      XHCI_PAGE_SIZE) != 0 ||
		    buffer_write64(rings->scratchpad_array.handle,
				   rings->scratchpad_array.offset +
					   i * sizeof(u64),
				   rings->scratchpad[i].phys) != 0) {
			return -1;
		}
	}
	rings->scratchpad_count = controller->max_scratchpads;
	return buffer_write64(rings->dcbaa.handle, rings->dcbaa.offset,
			      rings->scratchpad_array.phys);
}

static int xhci_setup_command_ring(const struct xhci_controller *controller,
				   struct xhci_rings *rings)
{
	const u64 link_offset = rings->command_ring.offset +
				(XHCI_RING_TRBS - 1) * XHCI_TRB_SIZE;
	const u64 link_control = ((u64)XHCI_TRB_TYPE_LINK << 10) |
				 XHCI_TRB_CYCLE | XHCI_TRB_TOGGLE_CYCLE;

	if (buffer_write64(rings->command_ring.handle, link_offset,
			   rings->command_ring.phys) != 0 ||
	    buffer_write32(rings->command_ring.handle, link_offset + 8, 0) !=
		    0 ||
	    buffer_write32(rings->command_ring.handle, link_offset + 12,
			   link_control) != 0) {
		return -1;
	}
	return mmio_write64(controller->mmio,
			    controller->cap_length + XHCI_OP_CRCR,
			    rings->command_ring.phys | XHCI_TRB_CYCLE);
}

static int xhci_setup_event_ring(const struct xhci_controller *controller,
				 struct xhci_rings *rings)
{
	const u64 ir0 = controller->runtime_offset + XHCI_RUNTIME_IR0;

	if (buffer_write64(rings->erst.handle, rings->erst.offset,
			   rings->event_ring.phys) != 0 ||
	    buffer_write32(rings->erst.handle, rings->erst.offset + 8,
			   XHCI_RING_TRBS) != 0 ||
	    buffer_write32(rings->erst.handle, rings->erst.offset + 12, 0) !=
		    0 ||
	    mmio_write32(controller->mmio, ir0 + XHCI_IR_ERSTSZ,
			 XHCI_ERST_ENTRIES) != 0 ||
	    mmio_write64(controller->mmio, ir0 + XHCI_IR_ERSTBA,
			 rings->erst.phys) != 0 ||
	    mmio_write64(controller->mmio, ir0 + XHCI_IR_ERDP,
			 rings->event_ring.phys) != 0) {
		return -1;
	}
	return 0;
}

static int xhci_setup_rings(const struct xhci_controller *controller,
			    struct xhci_rings *rings)
{
	long pagesize;
	u64 dcbaa_len;

	if (controller == 0 || rings == 0 || controller->max_slots == 0) {
		return -1;
	}
	pagesize = mmio_read32(controller->mmio,
			       controller->cap_length + XHCI_OP_PAGESIZE);
	if (pagesize < 0 || (((u64)pagesize) & 1) == 0) {
		return -1;
	}
	dcbaa_len = (controller->max_slots + 1) * sizeof(u64);
	if (dma_alloc(&rings->dcbaa, dcbaa_len, 64) != 0 ||
	    dma_alloc(&rings->command_ring, XHCI_RING_TRBS * XHCI_TRB_SIZE,
		      64) != 0 ||
	    dma_alloc(&rings->event_ring, XHCI_RING_TRBS * XHCI_TRB_SIZE,
		      64) != 0 ||
	    dma_alloc(&rings->erst, XHCI_ERST_ENTRY_SIZE, 64) != 0 ||
	    xhci_setup_scratchpads(controller, rings) != 0 ||
	    xhci_setup_command_ring(controller, rings) != 0 ||
	    mmio_write64(controller->mmio,
			 controller->cap_length + XHCI_OP_DCBAAP,
			 rings->dcbaa.phys) != 0 ||
	    mmio_write32(controller->mmio,
			 controller->cap_length + XHCI_OP_CONFIG,
			 controller->max_slots) != 0 ||
	    xhci_setup_event_ring(controller, rings) != 0) {
		return -1;
	}
	return 0;
}

static void log_rings(const struct xhci_rings *rings)
{
	char line[160];
	u64 offset = 0;

	line[0] = '\0';
	append_text(line, sizeof(line), &offset, "xhci: rings ready command=");
	append_u64(line, sizeof(line), &offset, rings->command_ring.phys);
	append_text(line, sizeof(line), &offset, " event=");
	append_u64(line, sizeof(line), &offset, rings->event_ring.phys);
	append_text(line, sizeof(line), &offset, " scratchpads=");
	append_u64(line, sizeof(line), &offset, rings->scratchpad_count);
	append_char(line, sizeof(line), &offset, '\n');
	bunix_console_log(line, offset);
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
				{
					struct xhci_rings rings = { 0 };

					if (xhci_setup_rings(&controller,
							     &rings) == 0) {
						log_rings(&rings);
					}
				}
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
