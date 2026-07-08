#include <bunix/alloc.h>
#include <bunix/syscall.h>

enum {
	USB_BUS_HANDLE_NAMES = 3,
	USB_BUS_MAX_DESCRIPTOR = 512,
	USB_BUS_MAX_INTERFACES = 16,
	USB_BUS_MAX_ENDPOINTS = 32,
};

struct usb_interface {
	u64 number;
	u64 alternate;
	u64 class_code;
	u64 subclass;
	u64 protocol;
	u64 endpoint_count;
	u64 claimed;
};

struct usb_endpoint {
	u64 interface_number;
	u64 address;
	u64 attributes;
	u64 max_packet;
	u64 interval;
};

struct usb_device {
	unsigned char *descriptor;
	u64 descriptor_len;
	u64 controller_id;
	u64 address;
	u64 vendor;
	u64 product;
	u64 device_class;
	u64 device_subclass;
	u64 device_protocol;
	u64 max_packet0;
	u64 configuration_value;
	u64 interface_count;
	u64 endpoint_count;
	struct usb_interface interfaces[USB_BUS_MAX_INTERFACES];
	struct usb_endpoint endpoints[USB_BUS_MAX_ENDPOINTS];
};

static struct usb_device *devices;
static u64 device_count;
static u64 device_capacity;

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

static void log_device(const struct usb_device *device, u64 index)
{
	char line[144];
	u64 offset = 0;

	if (device == 0) {
		return;
	}
	line[0] = '\0';
	append_text(line, sizeof(line), &offset, "usb-bus: device index=");
	append_u64(line, sizeof(line), &offset, index);
	append_text(line, sizeof(line), &offset, " vendor=");
	append_u64(line, sizeof(line), &offset, device->vendor);
	append_text(line, sizeof(line), &offset, " product=");
	append_u64(line, sizeof(line), &offset, device->product);
	append_text(line, sizeof(line), &offset, " interfaces=");
	append_u64(line, sizeof(line), &offset, device->interface_count);
	append_text(line, sizeof(line), &offset, " endpoints=");
	append_u64(line, sizeof(line), &offset, device->endpoint_count);
	append_char(line, sizeof(line), &offset, '\n');
	bunix_console_log(line, offset);
}

static long register_service(u64 service, u64 handle)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_NAMES,
		.type = BUNIX_NAMES_REGISTER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP,
		.reply = 0,
		.cap = handle,
		.words = { BUNIX_NAMES_ROOT, service, 0, 0 },
	};
	struct bunix_msg reply;

	if (bunix_ipc_call(USB_BUS_HANDLE_NAMES, &request, &reply) != 0) {
		return -1;
	}
	return reply.words[0] == BUNIX_DEV_OK ? 0 : -1;
}

static u64 le16(const unsigned char *data)
{
	return (u64)data[0] | ((u64)data[1] << 8);
}

static int device_push(const struct usb_device *device)
{
	struct usb_device *next;
	u64 old_size;
	u64 new_size;
	u64 new_capacity;

	if (device == 0) {
		return -1;
	}
	if (device_count == device_capacity) {
		old_size = device_capacity * sizeof(devices[0]);
		new_capacity = device_capacity == 0 ? 4 : device_capacity * 2;
		new_size = new_capacity * sizeof(devices[0]);
		next = (struct usb_device *)bunix_realloc(devices, old_size,
							  new_size);
		if (next == 0) {
			return -1;
		}
		devices = next;
		device_capacity = new_capacity;
	}
	devices[device_count++] = *device;
	return 0;
}

static int parse_usb_descriptors(const unsigned char *data, u64 len,
				 struct usb_device *device)
{
	u64 offset = 0;
	u64 current_interface = (u64)-1;

	if (data == 0 || device == 0 || len < 18 || data[0] < 18 ||
	    data[1] != BUNIX_USB_DESCRIPTOR_DEVICE) {
		return -1;
	}
	device->vendor = le16(&data[8]);
	device->product = le16(&data[10]);
	device->device_class = data[4];
	device->device_subclass = data[5];
	device->device_protocol = data[6];
	device->max_packet0 = data[7];
	device->address = device_count + 1;

	while (offset + 2 <= len) {
		const u64 desc_len = data[offset];
		const u64 desc_type = data[offset + 1];

		if (desc_len < 2 || offset + desc_len > len) {
			return -1;
		}
		if (desc_type == BUNIX_USB_DESCRIPTOR_CONFIGURATION &&
		    desc_len >= 9) {
			const u64 total = le16(&data[offset + 2]);

			if (total != 0 && total > len - offset) {
				return -1;
			}
			device->configuration_value = data[offset + 5];
		} else if (desc_type == BUNIX_USB_DESCRIPTOR_INTERFACE &&
			   desc_len >= 9) {
			struct usb_interface *interface;

			if (device->interface_count >= USB_BUS_MAX_INTERFACES) {
				return -1;
			}
			current_interface = device->interface_count++;
			interface = &device->interfaces[current_interface];
			interface->number = data[offset + 2];
			interface->alternate = data[offset + 3];
			interface->endpoint_count = data[offset + 4];
			interface->class_code = data[offset + 5];
			interface->subclass = data[offset + 6];
			interface->protocol = data[offset + 7];
			interface->claimed = 0;
		} else if (desc_type == BUNIX_USB_DESCRIPTOR_ENDPOINT &&
			   desc_len >= 7) {
			struct usb_endpoint *endpoint;

			if (current_interface == (u64)-1 ||
			    device->endpoint_count >= USB_BUS_MAX_ENDPOINTS) {
				return -1;
			}
			endpoint = &device->endpoints[device->endpoint_count++];
			endpoint->interface_number =
				device->interfaces[current_interface].number;
			endpoint->address = data[offset + 2];
			endpoint->attributes = data[offset + 3];
			endpoint->max_packet = le16(&data[offset + 4]);
			endpoint->interval = data[offset + 6];
		}
		offset += desc_len;
	}

	return offset == len && device->configuration_value != 0 ? 0 : -1;
}

static void reply_noent(struct bunix_msg *reply)
{
	reply->words[0] = BUNIX_USB_ERR_NOENT;
	reply->words[1] = 0;
	reply->words[2] = 0;
	reply->words[3] = 0;
}

static void reply_register_controller(struct bunix_msg *reply,
				      const struct bunix_msg *message)
{
	unsigned char local[USB_BUS_MAX_DESCRIPTOR];
	struct usb_device device;
	unsigned char *descriptor;
	u64 len = message->words[0];

	if (message->cap == 0 || len == 0 || len > sizeof(local) ||
	    (message->cap_rights & BUNIX_RIGHT_RECV) == 0 ||
	    bunix_buffer_read(message->cap, 0, local, len) != 0) {
		reply->words[0] = BUNIX_USB_ERR_INVAL;
		return;
	}
	for (u64 i = 0; i < sizeof(device) / sizeof(u64); i++) {
		((u64 *)&device)[i] = 0;
	}
	descriptor = (unsigned char *)bunix_alloc(len);
	if (descriptor == 0) {
		reply->words[0] = BUNIX_USB_ERR_INVAL;
		return;
	}
	for (u64 i = 0; i < len; i++) {
		descriptor[i] = local[i];
	}
	device.descriptor = descriptor;
	device.descriptor_len = len;
	device.controller_id = message->sender;
	if (parse_usb_descriptors(descriptor, len, &device) != 0 ||
	    device_push(&device) != 0) {
		bunix_free(descriptor);
		reply->words[0] = BUNIX_USB_ERR_INVAL;
		return;
	}
	reply->words[0] = BUNIX_USB_OK;
	reply->words[1] = device_count - 1;
	reply->words[2] = device.interface_count;
	reply->words[3] = device.endpoint_count;
	log_device(&devices[device_count - 1], device_count - 1);
}

static void reply_device_info(struct bunix_msg *reply, u64 index)
{
	struct usb_device *device;

	if (index >= device_count) {
		reply_noent(reply);
		return;
	}
	device = &devices[index];
	reply->words[0] = BUNIX_USB_OK;
	reply->words[1] = device->vendor | (device->product << 16) |
			  (device->address << 32);
	reply->words[2] = device->device_class |
			  (device->device_subclass << 8) |
			  (device->device_protocol << 16) |
			  (device->configuration_value << 24);
	reply->words[3] = device->interface_count |
			  (device->endpoint_count << 16) |
			  (device->max_packet0 << 32);
}

static void reply_descriptor_read(struct bunix_msg *reply,
				  const struct bunix_msg *message)
{
	struct usb_device *device;
	u64 len;

	if (message->words[0] >= device_count) {
		reply_noent(reply);
		return;
	}
	device = &devices[message->words[0]];
	len = message->words[2];
	if (len > device->descriptor_len) {
		len = device->descriptor_len;
	}
	if (message->cap == 0 ||
	    (message->cap_rights & BUNIX_RIGHT_SEND) == 0 ||
	    message->words[1] > device->descriptor_len ||
	    len > device->descriptor_len - message->words[1] ||
	    bunix_buffer_write(message->cap, 0,
			       device->descriptor + message->words[1],
			       len) != 0) {
		reply->words[0] = BUNIX_USB_ERR_INVAL;
		return;
	}
	reply->words[0] = BUNIX_USB_OK;
	reply->words[1] = len;
	reply->words[2] = device->descriptor_len;
	reply->words[3] = 0;
}

static void reply_claim_interface(struct bunix_msg *reply, u64 device_index,
				  u64 interface_number, u64 claim)
{
	struct usb_device *device;

	if (device_index >= device_count) {
		reply_noent(reply);
		return;
	}
	device = &devices[device_index];
	for (u64 i = 0; i < device->interface_count; i++) {
		struct usb_interface *interface = &device->interfaces[i];

		if (interface->number != interface_number) {
			continue;
		}
		if (claim != 0 && interface->claimed != 0) {
			reply->words[0] = BUNIX_USB_ERR_BUSY;
			return;
		}
		interface->claimed = claim != 0 ? 1 : 0;
		reply->words[0] = BUNIX_USB_OK;
		reply->words[1] = interface->class_code |
				  (interface->subclass << 8) |
				  (interface->protocol << 16);
		reply->words[2] = interface->endpoint_count;
		reply->words[3] = 0;
		return;
	}
	reply_noent(reply);
}

int main(void)
{
	const char online[] = "usb-bus: online\n";
	const char ready[] = "usb-bus: ready\n";
	struct bunix_msg message;

	bunix_console_log(online, sizeof(online) - 1);
	if (register_service(BUNIX_SERVICE_USB, BUNIX_HANDLE_SELF) != 0) {
		return 1;
	}
	bunix_console_log(ready, sizeof(ready) - 1);

	for (;;) {
		struct bunix_msg reply = {
			.protocol = BUNIX_PROTO_USB,
			.type = 0,
			.sender = 0,
			.cap_rights = 0,
			.reply = 0,
			.cap = 0,
			.words = { BUNIX_USB_ERR_UNSUPPORTED, 0, 0, 0 },
		};

		if (bunix_ipc_recv(BUNIX_HANDLE_SELF, &message) != 0 ||
		    message.protocol != BUNIX_PROTO_USB) {
			continue;
		}

		reply.type = message.type;
		switch (message.type) {
		case BUNIX_USB_CONTROLLER_REGISTER:
			reply_register_controller(&reply, &message);
			break;
		case BUNIX_USB_DEVICE_COUNT:
			reply.words[0] = BUNIX_USB_OK;
			reply.words[1] = device_count;
			break;
		case BUNIX_USB_DEVICE_INFO:
			reply_device_info(&reply, message.words[0]);
			break;
		case BUNIX_USB_DESCRIPTOR_READ:
			reply_descriptor_read(&reply, &message);
			break;
		case BUNIX_USB_CLAIM_INTERFACE:
			reply_claim_interface(&reply, message.words[0],
					      message.words[1], 1);
			break;
		case BUNIX_USB_RELEASE_INTERFACE:
			reply_claim_interface(&reply, message.words[0],
					      message.words[1], 0);
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
