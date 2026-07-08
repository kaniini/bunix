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
	XHCI_DOORBELL_STRIDE = 0x04,
	XHCI_USBCMD_RUN = 1 << 0,
	XHCI_USBCMD_HCRST = 1 << 1,
	XHCI_USBCMD_INTE = 1 << 2,
	XHCI_USBSTS_HCH = 1 << 0,
	XHCI_IR_IMAN = 0x00,
	XHCI_IR_IMAN_IE = 1 << 1,
	XHCI_PORTSC_CCS = 1 << 0,
	XHCI_PORTSC_PR = 1 << 4,
	XHCI_PORTSC_SPEED_SHIFT = 10,
	XHCI_PORTSC_SPEED_MASK = 0xf,
	XHCI_PORTSC_CHANGE_BITS = (1 << 17) | (1 << 18) | (1 << 19) |
				  (1 << 20) | (1 << 21) | (1 << 22) |
				  (1 << 23),
	XHCI_TRB_TYPE_LINK = 6,
	XHCI_TRB_TYPE_SETUP_STAGE = 2,
	XHCI_TRB_TYPE_DATA_STAGE = 3,
	XHCI_TRB_TYPE_STATUS_STAGE = 4,
	XHCI_TRB_TYPE_ENABLE_SLOT = 9,
	XHCI_TRB_TYPE_ADDRESS_DEVICE = 11,
	XHCI_TRB_TYPE_TRANSFER_EVENT = 32,
	XHCI_TRB_TYPE_COMMAND_COMPLETION = 33,
	XHCI_TRB_TYPE_PORT_STATUS_CHANGE = 34,
	XHCI_TRB_CYCLE = 1 << 0,
	XHCI_TRB_IOC = 1 << 5,
	XHCI_TRB_IDT = 1 << 6,
	XHCI_TRB_TOGGLE_CYCLE = 1 << 1,
	XHCI_TRB_DIR_IN = 1 << 16,
	XHCI_TRB_SETUP_TRT_IN = 3 << 16,
	XHCI_COMPLETION_SUCCESS = 1,
	XHCI_COMPLETION_SHORT_PACKET = 13,
	XHCI_HCCPARAMS1_CONTEXT_SIZE = 1 << 2,
	XHCI_SLOT_CONTEXT_ENTRIES_SHIFT = 27,
	XHCI_SLOT_CONTEXT_ROOT_PORT_SHIFT = 16,
	XHCI_SLOT_CONTEXT_SPEED_SHIFT = 20,
	XHCI_EP_TYPE_CONTROL = 4,
	XHCI_EP0_MAX_PACKET_SIZE = 8,
	XHCI_EP_CONTEXT_CERR_SHIFT = 1,
	XHCI_EP_CONTEXT_TYPE_SHIFT = 3,
	XHCI_EP_CONTEXT_MAX_PACKET_SHIFT = 16,
	XHCI_WAIT_POLLS = 1000,
	XHCI_COMMAND_POLLS = 2000,
	XHCI_PAGE_SIZE = 4096,
	XHCI_RING_TRBS = 256,
	XHCI_TRB_SIZE = 16,
	XHCI_CONTEXT_DWORD_SIZE = 4,
	XHCI_CONTEXT_COUNT = 32,
	XHCI_USB_DT_DEVICE = 1,
	XHCI_USB_DT_CONFIGURATION = 2,
	XHCI_USB_REQ_GET_DESCRIPTOR = 6,
	XHCI_USB_REQ_SET_CONFIGURATION = 9,
	XHCI_USB_DIR_IN = 0x80,
	XHCI_USB_DEVICE_DESCRIPTOR_SIZE = 18,
	XHCI_USB_CONFIG_DESCRIPTOR_HEADER_SIZE = 9,
	XHCI_USB_CONFIG_DESCRIPTOR_MAX_SIZE = 256,
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
	u64 command_producer;
	u64 command_cycle;
	u64 event_consumer;
	u64 event_cycle;
};

struct xhci_device {
	struct xhci_dma_buffer device_context;
	struct xhci_dma_buffer input_context;
	struct xhci_dma_buffer ep0_ring;
	struct xhci_dma_buffer control_buffer;
	u64 slot_id;
	u64 port;
	u64 speed;
	u64 ep0_producer;
	u64 ep0_cycle;
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

static long mmio_read64(u64 handle, u64 offset, u64 *value)
{
	const long low = bunix_hw_mmio_read32(handle, offset);
	const long high = bunix_hw_mmio_read32(handle, offset + 4);

	if (value == 0 || low < 0 || high < 0) {
		return -1;
	}
	*value = ((u64)low & 0xffffffffull) |
		 (((u64)high & 0xffffffffull) << 32);
	return 0;
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

static int buffer_read32(u64 handle, u64 offset, u64 *value)
{
	unsigned int low;

	if (value == 0 ||
	    bunix_buffer_read(handle, offset, &low, sizeof(low)) != 0) {
		return -1;
	}
	*value = low;
	return 0;
}

static int buffer_read64(u64 handle, u64 offset, u64 *value)
{
	u64 low;
	u64 high;

	if (value == 0 || buffer_read32(handle, offset, &low) != 0 ||
	    buffer_read32(handle, offset + 4, &high) != 0) {
		return -1;
	}
	*value = low | (high << 32);
	return 0;
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

static int xhci_start_controller(const struct xhci_controller *controller)
{
	const u64 op_base = controller->cap_length;
	long command;

	command = mmio_read32(controller->mmio, op_base + XHCI_OP_USBCMD);
	if (command < 0 ||
	    mmio_write32(controller->mmio, op_base + XHCI_OP_USBCMD,
			 ((u64)command) | XHCI_USBCMD_RUN |
				 XHCI_USBCMD_INTE) != 0) {
		return -1;
	}
	return wait_status_bits(controller->mmio, op_base, XHCI_USBSTS_HCH, 0);
}

static int xhci_stop_controller(const struct xhci_controller *controller)
{
	const u64 op_base = controller->cap_length;
	long command;

	command = mmio_read32(controller->mmio, op_base + XHCI_OP_USBCMD);
	if (command < 0 ||
	    mmio_write32(controller->mmio, op_base + XHCI_OP_USBCMD,
			 ((u64)command) & ~XHCI_USBCMD_RUN) != 0) {
		return -1;
	}
	return wait_status_bits(controller->mmio, op_base, XHCI_USBSTS_HCH,
				XHCI_USBSTS_HCH);
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
			 rings->event_ring.phys) != 0 ||
	    mmio_write32(controller->mmio, ir0 + XHCI_IR_IMAN,
			 XHCI_IR_IMAN_IE) != 0) {
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
	rings->command_producer = 0;
	rings->command_cycle = 1;
	rings->event_consumer = 0;
	rings->event_cycle = 1;
	return 0;
}

static u64 xhci_port_offset(const struct xhci_controller *controller, u64 port)
{
	return controller->cap_length + XHCI_OP_PORTSC_BASE +
	       port * XHCI_OP_PORT_STRIDE;
}

static int xhci_find_connected_port(const struct xhci_controller *controller,
				    u64 *port, u64 *status)
{
	for (u64 i = 0; i < controller->max_ports; i++) {
		const long portsc =
			mmio_read32(controller->mmio,
				    xhci_port_offset(controller, i));

		if (portsc >= 0 && (((u64)portsc) & XHCI_PORTSC_CCS) != 0) {
			*port = i;
			*status = (u64)portsc;
			return 0;
		}
	}
	return -1;
}

static int xhci_reset_port(const struct xhci_controller *controller, u64 *port,
			   u64 *speed)
{
	u64 selected = 0;
	u64 status = 0;
	const u64 clear_mask = XHCI_PORTSC_CHANGE_BITS;

	if (port == 0 || speed == 0 ||
	    xhci_find_connected_port(controller, &selected, &status) != 0) {
		return -1;
	}
	if (mmio_write32(controller->mmio, xhci_port_offset(controller, selected),
			 (status & ~clear_mask) | XHCI_PORTSC_PR) != 0) {
		return -1;
	}
	for (u64 i = 0; i < XHCI_WAIT_POLLS; i++) {
		const long portsc =
			mmio_read32(controller->mmio,
				    xhci_port_offset(controller, selected));

		if (portsc < 0) {
			return -1;
		}
		status = (u64)portsc;
		if ((status & XHCI_PORTSC_PR) == 0 &&
		    (status & XHCI_PORTSC_CCS) != 0) {
			*port = selected;
			*speed = (status >> XHCI_PORTSC_SPEED_SHIFT) &
				 XHCI_PORTSC_SPEED_MASK;
			(void)mmio_write32(controller->mmio,
					   xhci_port_offset(controller,
							    selected),
					   status & clear_mask);
			return 0;
		}
		bunix_sleep_ns(1000000ull);
	}
	return -1;
}

static int xhci_write_command_trb(struct xhci_rings *rings, u64 parameter,
				  u64 status, u64 control)
{
	u64 index;
	u64 offset;

	if (rings == 0 || rings->command_producer >= XHCI_RING_TRBS - 1) {
		return -1;
	}
	index = rings->command_producer;
	offset = rings->command_ring.offset + index * XHCI_TRB_SIZE;
	if (buffer_write64(rings->command_ring.handle, offset, parameter) != 0 ||
	    buffer_write32(rings->command_ring.handle, offset + 8, status) !=
		    0 ||
	    buffer_write32(rings->command_ring.handle, offset + 12,
			   control | rings->command_cycle) != 0) {
		return -1;
	}
	rings->command_producer++;
	return 0;
}

static int xhci_write_ep0_trb(struct xhci_device *device, u64 parameter,
			      u64 status, u64 control)
{
	u64 index;
	u64 offset;

	if (device == 0 || device->ep0_producer >= XHCI_RING_TRBS - 1) {
		return -1;
	}
	index = device->ep0_producer;
	offset = device->ep0_ring.offset + index * XHCI_TRB_SIZE;
	if (buffer_write64(device->ep0_ring.handle, offset, parameter) != 0 ||
	    buffer_write32(device->ep0_ring.handle, offset + 8, status) != 0 ||
	    buffer_write32(device->ep0_ring.handle, offset + 12,
			   control | device->ep0_cycle) != 0) {
		return -1;
	}
	device->ep0_producer++;
	return 0;
}

static int xhci_ring_command_doorbell(const struct xhci_controller *controller)
{
	return mmio_write32(controller->mmio, controller->doorbell_offset, 0);
}

static int xhci_ring_ep0_doorbell(const struct xhci_controller *controller,
				  const struct xhci_device *device)
{
	return mmio_write32(controller->mmio,
			    controller->doorbell_offset +
				    device->slot_id * XHCI_DOORBELL_STRIDE,
			    1);
}

static int xhci_wait_command_completion(const struct xhci_controller *controller,
					struct xhci_rings *rings, u64 *slot_id,
					u64 *completion_code)
{
	for (u64 poll = 0; poll < XHCI_COMMAND_POLLS; poll++) {
		const u64 offset = rings->event_ring.offset +
				   rings->event_consumer * XHCI_TRB_SIZE;
		u64 parameter = 0;
		u64 status = 0;
		u64 control = 0;
		u64 type;

		if (buffer_read64(rings->event_ring.handle, offset,
				  &parameter) != 0 ||
		    buffer_read32(rings->event_ring.handle, offset + 8,
				  &status) != 0 ||
		    buffer_read32(rings->event_ring.handle, offset + 12,
				  &control) != 0) {
			return -1;
		}
		if ((control & XHCI_TRB_CYCLE) != rings->event_cycle) {
			bunix_sleep_ns(1000000ull);
			continue;
		}
		rings->event_consumer++;
		if (rings->event_consumer == XHCI_RING_TRBS) {
			rings->event_consumer = 0;
			rings->event_cycle ^= 1;
		}
		{
			const u64 erdp = rings->event_ring.phys +
					 rings->event_consumer * XHCI_TRB_SIZE;

			(void)mmio_write64(controller->mmio,
					   controller->runtime_offset +
						   XHCI_RUNTIME_IR0 +
						   XHCI_IR_ERDP,
					   erdp | (1 << 3));
		}
		type = (control >> 10) & 0x3fu;
		if (type == XHCI_TRB_TYPE_COMMAND_COMPLETION) {
			*completion_code = (status >> 24) & 0xffu;
			*slot_id = (control >> 24) & 0xffu;
			return parameter != 0 ? 0 : -1;
		}
		if (type != XHCI_TRB_TYPE_PORT_STATUS_CHANGE) {
			return -1;
		}
	}
	return -1;
}

static int xhci_enable_slot(const struct xhci_controller *controller,
			    struct xhci_rings *rings, u64 *slot_id,
			    u64 *completion_code)
{
	const u64 control = ((u64)XHCI_TRB_TYPE_ENABLE_SLOT) << 10;

	if (xhci_write_command_trb(rings, 0, 0, control) != 0 ||
	    xhci_ring_command_doorbell(controller) != 0 ||
	    xhci_wait_command_completion(controller, rings, slot_id,
					 completion_code) != 0) {
		return -1;
	}
	return *completion_code == XHCI_COMPLETION_SUCCESS && *slot_id != 0 ? 0 :
									-1;
}

static int xhci_wait_transfer_completion(
	const struct xhci_controller *controller, struct xhci_rings *rings,
	const struct xhci_device *device, u64 *completion_code,
	u64 requested_length, u64 *actual_length)
{
	for (u64 poll = 0; poll < XHCI_COMMAND_POLLS; poll++) {
		const u64 offset = rings->event_ring.offset +
				   rings->event_consumer * XHCI_TRB_SIZE;
		u64 parameter = 0;
		u64 status = 0;
		u64 control = 0;
		u64 type;

		if (buffer_read64(rings->event_ring.handle, offset,
				  &parameter) != 0 ||
		    buffer_read32(rings->event_ring.handle, offset + 8,
				  &status) != 0 ||
		    buffer_read32(rings->event_ring.handle, offset + 12,
				  &control) != 0) {
			return -1;
		}
		if ((control & XHCI_TRB_CYCLE) != rings->event_cycle) {
			bunix_sleep_ns(1000000ull);
			continue;
		}
		rings->event_consumer++;
		if (rings->event_consumer == XHCI_RING_TRBS) {
			rings->event_consumer = 0;
			rings->event_cycle ^= 1;
		}
		{
			const u64 erdp = rings->event_ring.phys +
					 rings->event_consumer * XHCI_TRB_SIZE;

			(void)mmio_write64(controller->mmio,
					   controller->runtime_offset +
						   XHCI_RUNTIME_IR0 +
						   XHCI_IR_ERDP,
					   erdp | (1 << 3));
		}
		type = (control >> 10) & 0x3fu;
		if (type == XHCI_TRB_TYPE_TRANSFER_EVENT) {
			const u64 event_slot = (control >> 24) & 0xffu;
			const u64 endpoint = (control >> 16) & 0x1fu;
			const u64 residual = status & 0xffffffu;

			if (event_slot != device->slot_id || endpoint != 1) {
				return -1;
			}
			*completion_code = (status >> 24) & 0xffu;
			*actual_length = residual > requested_length ?
						 0 :
						 requested_length - residual;
			return 0;
		}
		if (type != XHCI_TRB_TYPE_PORT_STATUS_CHANGE) {
			return -1;
		}
	}
	return -1;
}

static u64 xhci_context_size(const struct xhci_controller *controller)
{
	return (controller->hccparams1 & XHCI_HCCPARAMS1_CONTEXT_SIZE) != 0 ? 64 :
									   32;
}

static u64 xhci_context_offset(const struct xhci_controller *controller,
			       u64 index)
{
	return index * xhci_context_size(controller);
}

static int xhci_setup_ep0_transfer_ring(struct xhci_device *device)
{
	const u64 link_offset = device->ep0_ring.offset +
				(XHCI_RING_TRBS - 1) * XHCI_TRB_SIZE;
	const u64 link_control = ((u64)XHCI_TRB_TYPE_LINK << 10) |
				 XHCI_TRB_CYCLE | XHCI_TRB_TOGGLE_CYCLE;

	return buffer_write64(device->ep0_ring.handle, link_offset,
			      device->ep0_ring.phys) == 0 &&
		       buffer_write32(device->ep0_ring.handle,
				      link_offset + 8, 0) == 0 &&
		       buffer_write32(device->ep0_ring.handle,
				      link_offset + 12, link_control) == 0 ?
		       0 :
		       -1;
}

static int xhci_prepare_address_device(const struct xhci_controller *controller,
				       struct xhci_rings *rings,
				       struct xhci_device *device)
{
	const u64 context_size = xhci_context_size(controller);
	const u64 context_bytes = (XHCI_CONTEXT_COUNT + 1) * context_size;
	const u64 input_slot = xhci_context_offset(controller, 1);
	const u64 input_ep0 = xhci_context_offset(controller, 2);
	const u64 slot0 = 0 * XHCI_CONTEXT_DWORD_SIZE;
	const u64 slot1 = 1 * XHCI_CONTEXT_DWORD_SIZE;
	const u64 ep0_0 = 0 * XHCI_CONTEXT_DWORD_SIZE;
	const u64 ep0_1 = 1 * XHCI_CONTEXT_DWORD_SIZE;
	const u64 ep0_2 = 2 * XHCI_CONTEXT_DWORD_SIZE;
	const u64 ep0_4 = 4 * XHCI_CONTEXT_DWORD_SIZE;
	const u64 slot_dword0 =
		((device->speed & XHCI_PORTSC_SPEED_MASK)
		 << XHCI_SLOT_CONTEXT_SPEED_SHIFT) |
		(1ull << XHCI_SLOT_CONTEXT_ENTRIES_SHIFT);
	const u64 slot_dword1 =
		((device->port + 1) << XHCI_SLOT_CONTEXT_ROOT_PORT_SHIFT);
	const u64 ep0_dword1 =
		(3ull << XHCI_EP_CONTEXT_CERR_SHIFT) |
		((u64)XHCI_EP_TYPE_CONTROL << XHCI_EP_CONTEXT_TYPE_SHIFT) |
		((u64)XHCI_EP0_MAX_PACKET_SIZE
		 << XHCI_EP_CONTEXT_MAX_PACKET_SHIFT);

	if (device == 0 || device->slot_id == 0 ||
	    dma_alloc(&device->device_context, XHCI_CONTEXT_COUNT * context_size,
		      64) != 0 ||
	    dma_alloc(&device->input_context, context_bytes, 64) != 0 ||
	    dma_alloc(&device->ep0_ring, XHCI_RING_TRBS * XHCI_TRB_SIZE,
		      64) != 0 ||
	    dma_alloc(&device->control_buffer, XHCI_PAGE_SIZE,
		      XHCI_PAGE_SIZE) != 0 ||
	    xhci_setup_ep0_transfer_ring(device) != 0 ||
	    buffer_write64(rings->dcbaa.handle,
			   rings->dcbaa.offset + device->slot_id * sizeof(u64),
			   device->device_context.phys) != 0 ||
	    buffer_write32(device->input_context.handle,
			   device->input_context.offset + 4, 0x3) != 0 ||
	    buffer_write32(device->input_context.handle,
			   device->input_context.offset + input_slot + slot0,
			   slot_dword0) != 0 ||
	    buffer_write32(device->input_context.handle,
			   device->input_context.offset + input_slot + slot1,
			   slot_dword1) != 0 ||
	    buffer_write32(device->input_context.handle,
			   device->input_context.offset + input_ep0 + ep0_0,
			   0) != 0 ||
	    buffer_write32(device->input_context.handle,
			   device->input_context.offset + input_ep0 + ep0_1,
			   ep0_dword1) != 0 ||
	    buffer_write64(device->input_context.handle,
			   device->input_context.offset + input_ep0 + ep0_2,
			   device->ep0_ring.phys | XHCI_TRB_CYCLE) != 0 ||
	    buffer_write32(device->input_context.handle,
			   device->input_context.offset + input_ep0 + ep0_4,
			   XHCI_EP0_MAX_PACKET_SIZE) != 0) {
		return -1;
	}
	device->ep0_producer = 0;
	device->ep0_cycle = 1;
	return 0;
}

static int xhci_address_device(const struct xhci_controller *controller,
			       struct xhci_rings *rings,
			       struct xhci_device *device,
			       u64 *completion_code)
{
	u64 completion_slot = 0;
	const u64 control = ((u64)XHCI_TRB_TYPE_ADDRESS_DEVICE << 10) |
			    (device->slot_id << 24);

	if (xhci_prepare_address_device(controller, rings, device) != 0 ||
	    xhci_write_command_trb(rings, device->input_context.phys, 0,
				   control) != 0 ||
	    xhci_ring_command_doorbell(controller) != 0 ||
	    xhci_wait_command_completion(controller, rings, &completion_slot,
					 completion_code) != 0) {
		return -1;
	}
	return *completion_code == XHCI_COMPLETION_SUCCESS &&
		       completion_slot == device->slot_id ?
		       0 :
		       -1;
}

static int xhci_control_read(const struct xhci_controller *controller,
			     struct xhci_rings *rings,
			     struct xhci_device *device, u64 request_type,
			     u64 request, u64 value, u64 index,
			     unsigned char *out, u64 out_len,
			     u64 *actual_length, u64 *completion_code)
{
	const u64 setup = request_type | (request << 8) | (value << 16) |
			  (index << 32) | (out_len << 48);
	const u64 setup_status = 8;
	const u64 setup_control =
		((u64)XHCI_TRB_TYPE_SETUP_STAGE << 10) | XHCI_TRB_IDT |
		XHCI_TRB_SETUP_TRT_IN;
	const u64 data_status = out_len;
	const u64 data_control =
		((u64)XHCI_TRB_TYPE_DATA_STAGE << 10) | XHCI_TRB_DIR_IN;
	const u64 status_control =
		((u64)XHCI_TRB_TYPE_STATUS_STAGE << 10) | XHCI_TRB_IOC;

	if (out == 0 || out_len == 0 || out_len > XHCI_PAGE_SIZE ||
	    buffer_zero(device->control_buffer.handle,
			device->control_buffer.offset, out_len) != 0 ||
	    xhci_write_ep0_trb(device, setup, setup_status, setup_control) != 0 ||
	    xhci_write_ep0_trb(device, device->control_buffer.phys,
			       data_status, data_control) != 0 ||
	    xhci_write_ep0_trb(device, 0, 0, status_control) != 0 ||
	    xhci_ring_ep0_doorbell(controller, device) != 0 ||
	    xhci_wait_transfer_completion(controller, rings, device,
					  completion_code, out_len,
					  actual_length) != 0) {
		return -1;
	}
	if (*completion_code != XHCI_COMPLETION_SUCCESS &&
	    *completion_code != XHCI_COMPLETION_SHORT_PACKET) {
		return -1;
	}
	if (*actual_length > out_len) {
		*actual_length = out_len;
	}
	return bunix_buffer_read(device->control_buffer.handle,
				 device->control_buffer.offset, out, out_len) == 0 ?
		       0 :
		       -1;
}

static int xhci_control_no_data(const struct xhci_controller *controller,
				struct xhci_rings *rings,
				struct xhci_device *device, u64 request_type,
				u64 request, u64 value, u64 index,
				u64 *completion_code)
{
	u64 actual = 0;
	const u64 setup = request_type | (request << 8) | (value << 16) |
			  (index << 32);
	const u64 setup_status = 8;
	const u64 setup_control =
		((u64)XHCI_TRB_TYPE_SETUP_STAGE << 10) | XHCI_TRB_IDT;
	const u64 status_control =
		((u64)XHCI_TRB_TYPE_STATUS_STAGE << 10) | XHCI_TRB_IOC |
		XHCI_TRB_DIR_IN;

	if (xhci_write_ep0_trb(device, setup, setup_status, setup_control) != 0 ||
	    xhci_write_ep0_trb(device, 0, 0, status_control) != 0 ||
	    xhci_ring_ep0_doorbell(controller, device) != 0 ||
	    xhci_wait_transfer_completion(controller, rings, device,
					  completion_code, 0, &actual) != 0) {
		return -1;
	}
	return *completion_code == XHCI_COMPLETION_SUCCESS ? 0 : -1;
}

static int xhci_get_device_descriptor(
	const struct xhci_controller *controller, struct xhci_rings *rings,
	struct xhci_device *device, unsigned char *descriptor, u64 descriptor_len,
	u64 *actual_length, u64 *completion_code)
{
	if (descriptor_len < XHCI_USB_DEVICE_DESCRIPTOR_SIZE) {
		return -1;
	}
	return xhci_control_read(controller, rings, device, XHCI_USB_DIR_IN,
				 XHCI_USB_REQ_GET_DESCRIPTOR,
				 ((u64)XHCI_USB_DT_DEVICE) << 8, 0,
				 descriptor, XHCI_USB_DEVICE_DESCRIPTOR_SIZE,
				 actual_length, completion_code);
}

static int xhci_get_config_descriptor(
	const struct xhci_controller *controller, struct xhci_rings *rings,
	struct xhci_device *device, unsigned char *descriptor, u64 descriptor_len,
	u64 requested_len, u64 *actual_length, u64 *completion_code)
{
	if (descriptor == 0 || requested_len == 0 ||
	    requested_len > descriptor_len ||
	    requested_len > XHCI_USB_CONFIG_DESCRIPTOR_MAX_SIZE) {
		return -1;
	}
	return xhci_control_read(controller, rings, device, XHCI_USB_DIR_IN,
				 XHCI_USB_REQ_GET_DESCRIPTOR,
				 ((u64)XHCI_USB_DT_CONFIGURATION) << 8, 0,
				 descriptor, requested_len, actual_length,
				 completion_code);
}

static int xhci_set_configuration(const struct xhci_controller *controller,
				  struct xhci_rings *rings,
				  struct xhci_device *device,
				  u64 configuration_value,
				  u64 *completion_code)
{
	return xhci_control_no_data(controller, rings, device, 0,
				    XHCI_USB_REQ_SET_CONFIGURATION,
				    configuration_value, 0, completion_code);
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

static void log_port_reset(u64 port, u64 speed)
{
	char line[96];
	u64 offset = 0;

	line[0] = '\0';
	append_text(line, sizeof(line), &offset, "xhci: port reset port=");
	append_u64(line, sizeof(line), &offset, port + 1);
	append_text(line, sizeof(line), &offset, " speed=");
	append_u64(line, sizeof(line), &offset, speed);
	append_char(line, sizeof(line), &offset, '\n');
	bunix_console_log(line, offset);
}

static void log_enable_slot(u64 slot_id, u64 completion_code)
{
	char line[96];
	u64 offset = 0;

	line[0] = '\0';
	append_text(line, sizeof(line), &offset, "xhci: enable slot slot=");
	append_u64(line, sizeof(line), &offset, slot_id);
	append_text(line, sizeof(line), &offset, " code=");
	append_u64(line, sizeof(line), &offset, completion_code);
	append_char(line, sizeof(line), &offset, '\n');
	bunix_console_log(line, offset);
}

static void log_address_device(u64 slot_id, u64 completion_code)
{
	char line[96];
	u64 offset = 0;

	line[0] = '\0';
	append_text(line, sizeof(line), &offset, "xhci: address device slot=");
	append_u64(line, sizeof(line), &offset, slot_id);
	append_text(line, sizeof(line), &offset, " code=");
	append_u64(line, sizeof(line), &offset, completion_code);
	append_char(line, sizeof(line), &offset, '\n');
	bunix_console_log(line, offset);
}

static void log_device_descriptor(const unsigned char *descriptor,
				  u64 actual_length, u64 completion_code)
{
	char line[128];
	u64 offset = 0;
	u64 vendor = 0;
	u64 product = 0;

	if (descriptor != 0 && actual_length >= 12) {
		vendor = (u64)descriptor[8] | ((u64)descriptor[9] << 8);
		product = (u64)descriptor[10] | ((u64)descriptor[11] << 8);
	}
	line[0] = '\0';
	append_text(line, sizeof(line), &offset, "xhci: device descriptor len=");
	append_u64(line, sizeof(line), &offset, actual_length);
	append_text(line, sizeof(line), &offset, " code=");
	append_u64(line, sizeof(line), &offset, completion_code);
	append_text(line, sizeof(line), &offset, " vendor=");
	append_u64(line, sizeof(line), &offset, vendor);
	append_text(line, sizeof(line), &offset, " product=");
	append_u64(line, sizeof(line), &offset, product);
	append_char(line, sizeof(line), &offset, '\n');
	bunix_console_log(line, offset);
}

static void log_config_descriptor(const unsigned char *descriptor,
				  u64 actual_length, u64 completion_code)
{
	char line[144];
	u64 offset = 0;
	u64 total = 0;
	u64 interfaces = 0;
	u64 config_value = 0;

	if (descriptor != 0 && actual_length >= XHCI_USB_CONFIG_DESCRIPTOR_HEADER_SIZE) {
		total = (u64)descriptor[2] | ((u64)descriptor[3] << 8);
		interfaces = descriptor[4];
		config_value = descriptor[5];
	}
	line[0] = '\0';
	append_text(line, sizeof(line), &offset, "xhci: config descriptor len=");
	append_u64(line, sizeof(line), &offset, actual_length);
	append_text(line, sizeof(line), &offset, " code=");
	append_u64(line, sizeof(line), &offset, completion_code);
	append_text(line, sizeof(line), &offset, " total=");
	append_u64(line, sizeof(line), &offset, total);
	append_text(line, sizeof(line), &offset, " interfaces=");
	append_u64(line, sizeof(line), &offset, interfaces);
	append_text(line, sizeof(line), &offset, " value=");
	append_u64(line, sizeof(line), &offset, config_value);
	append_char(line, sizeof(line), &offset, '\n');
	bunix_console_log(line, offset);
}

static void log_set_configuration(u64 configuration_value, u64 completion_code)
{
	char line[96];
	u64 offset = 0;

	line[0] = '\0';
	append_text(line, sizeof(line), &offset, "xhci: set configuration value=");
	append_u64(line, sizeof(line), &offset, configuration_value);
	append_text(line, sizeof(line), &offset, " code=");
	append_u64(line, sizeof(line), &offset, completion_code);
	append_char(line, sizeof(line), &offset, '\n');
	bunix_console_log(line, offset);
}

static void log_command_debug(const struct xhci_controller *controller,
			      const struct xhci_rings *rings)
{
	char line[192];
	u64 offset = 0;
	u64 crcr = 0;
	u64 erdp = 0;
	u64 event0_control = 0;
	u64 event0_status = 0;
	long usbsts;

	usbsts = mmio_read32(controller->mmio,
			     controller->cap_length + XHCI_OP_USBSTS);
	(void)mmio_read64(controller->mmio,
			  controller->cap_length + XHCI_OP_CRCR, &crcr);
	(void)mmio_read64(controller->mmio,
			  controller->runtime_offset + XHCI_RUNTIME_IR0 +
				  XHCI_IR_ERDP,
			  &erdp);
	(void)buffer_read32(rings->event_ring.handle,
			    rings->event_ring.offset + 8, &event0_status);
	(void)buffer_read32(rings->event_ring.handle,
			    rings->event_ring.offset + 12, &event0_control);
	line[0] = '\0';
	append_text(line, sizeof(line), &offset, "xhci: command debug usbsts=");
	append_u64(line, sizeof(line), &offset, usbsts < 0 ? 0 : (u64)usbsts);
	append_text(line, sizeof(line), &offset, " crcr=");
	append_u64(line, sizeof(line), &offset, crcr);
	append_text(line, sizeof(line), &offset, " erdp=");
	append_u64(line, sizeof(line), &offset, erdp);
	append_text(line, sizeof(line), &offset, " evst=");
	append_u64(line, sizeof(line), &offset, event0_status);
	append_text(line, sizeof(line), &offset, " evctl=");
	append_u64(line, sizeof(line), &offset, event0_control);
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
						u64 port = 0;
						u64 speed = 0;
						u64 slot_id = 0;
						u64 code = 0;

						log_rings(&rings);
						if (xhci_start_controller(
							    &controller) == 0 &&
						    xhci_reset_port(&controller,
								    &port,
								    &speed) == 0) {
							log_port_reset(port,
								       speed);
							if (xhci_enable_slot(
								    &controller,
								    &rings,
								    &slot_id,
								    &code) == 0) {
								struct xhci_device
									device = {
										0
									};

								log_enable_slot(
									slot_id,
									code);
								device.slot_id =
									slot_id;
								device.port = port;
								device.speed =
									speed;
								if (xhci_address_device(
									    &controller,
									    &rings,
									    &device,
									    &code) == 0) {
									unsigned char
										descriptor
											[XHCI_USB_DEVICE_DESCRIPTOR_SIZE];
									u64 actual = 0;

									log_address_device(
										slot_id,
										code);
									if (xhci_get_device_descriptor(
										    &controller,
										    &rings,
										    &device,
										    descriptor,
										    sizeof(descriptor),
										    &actual,
										    &code) == 0) {
										unsigned char
											config
												[XHCI_USB_CONFIG_DESCRIPTOR_MAX_SIZE];
										u64 config_actual =
											0;
										u64 config_len =
											XHCI_USB_CONFIG_DESCRIPTOR_HEADER_SIZE;
										u64 config_value =
											0;

										log_device_descriptor(
											descriptor,
											actual,
											code);
										if (xhci_get_config_descriptor(
											    &controller,
											    &rings,
											    &device,
											    config,
											    sizeof(config),
											    config_len,
											    &config_actual,
											    &code) == 0 &&
										    config_actual >=
											    XHCI_USB_CONFIG_DESCRIPTOR_HEADER_SIZE) {
											config_len =
												(u64)config[2] |
												((u64)config[3] << 8);
											config_value =
												config[5];
											if (config_len >
											    sizeof(config)) {
												config_len =
													sizeof(config);
											}
											if (xhci_get_config_descriptor(
												    &controller,
												    &rings,
												    &device,
												    config,
												    sizeof(config),
												    config_len,
												    &config_actual,
												    &code) == 0) {
												log_config_descriptor(
													config,
													config_actual,
													code);
												if (xhci_set_configuration(
													    &controller,
													    &rings,
													    &device,
													    config_value,
													    &code) == 0) {
													log_set_configuration(
														config_value,
														code);
												} else {
													log_command_debug(
														&controller,
														&rings);
												}
											} else {
												log_command_debug(
													&controller,
													&rings);
											}
										} else {
											log_command_debug(
												&controller,
												&rings);
										}
									} else {
										log_command_debug(
											&controller,
											&rings);
									}
								} else {
									log_command_debug(
										&controller,
										&rings);
								}
							} else {
								log_command_debug(
									&controller,
									&rings);
							}
						}
						(void)xhci_stop_controller(
							&controller);
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
