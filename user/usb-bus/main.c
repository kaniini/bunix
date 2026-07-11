#include <bunix/alloc.h>
#include <bunix/syscall.h>

#define USB_BUS_HANDLE_NAMES (bunix_handle_find(BUNIX_CAP_NAME))

enum {
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
	u64 claim_owner;
};

struct usb_endpoint {
	u64 interface_number;
	u64 address;
	u64 attributes;
	u64 max_packet;
	u64 interval;
	u64 port;
	u64 owner;
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
	(void)service;
	return bunix_names_register_claim(bunix_handle_find(BUNIX_CAP_CLAM),
					  handle);
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
			interface->claim_owner = 0;
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
			endpoint->port = 0;
			endpoint->owner = 0;
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

static struct usb_interface *find_interface(struct usb_device *device,
					    u64 interface_number)
{
	if (device == 0) {
		return 0;
	}
	for (u64 i = 0; i < device->interface_count; i++) {
		if (device->interfaces[i].number == interface_number) {
			return &device->interfaces[i];
		}
	}
	return 0;
}

static struct usb_endpoint *find_endpoint(struct usb_device *device,
					  u64 interface_number,
					  u64 endpoint_address)
{
	if (device == 0) {
		return 0;
	}
	for (u64 i = 0; i < device->endpoint_count; i++) {
		struct usb_endpoint *endpoint = &device->endpoints[i];

		if (endpoint->interface_number == interface_number &&
		    endpoint->address == endpoint_address) {
			return endpoint;
		}
	}
	return 0;
}

static void close_interface_endpoints(struct usb_device *device,
				      u64 interface_number, u64 owner)
{
	if (device == 0) {
		return;
	}
	for (u64 i = 0; i < device->endpoint_count; i++) {
		struct usb_endpoint *endpoint = &device->endpoints[i];

		if (endpoint->interface_number == interface_number &&
		    endpoint->owner == owner) {
			if (endpoint->port != 0) {
				bunix_handle_close(endpoint->port);
			}
			endpoint->port = 0;
			endpoint->owner = 0;
		}
	}
}

static void reply_claim_interface(struct bunix_msg *reply,
				  const struct bunix_msg *message, u64 claim)
{
	struct usb_device *device;
	struct usb_interface *interface;
	const u64 device_index = message->words[0];
	const u64 interface_number = message->words[1];

	if (device_index >= device_count) {
		reply_noent(reply);
		return;
	}
	device = &devices[device_index];
	interface = find_interface(device, interface_number);
	if (interface == 0) {
		reply_noent(reply);
		return;
	}
	if (claim != 0 && interface->claim_owner != 0 &&
	    interface->claim_owner != message->sender) {
		reply->words[0] = BUNIX_USB_ERR_BUSY;
		return;
	}
	if (claim == 0 && interface->claim_owner != 0 &&
	    interface->claim_owner != message->sender) {
		reply->words[0] = BUNIX_USB_ERR_BUSY;
		return;
	}
	if (claim != 0) {
		interface->claim_owner = message->sender;
	} else {
		close_interface_endpoints(device, interface_number,
					  interface->claim_owner);
		interface->claim_owner = 0;
	}
	reply->words[0] = BUNIX_USB_OK;
	reply->words[1] = interface->class_code |
			  (interface->subclass << 8) |
			  (interface->protocol << 16);
	reply->words[2] = interface->endpoint_count;
	reply->words[3] = 0;
}

static void reply_open_endpoint(struct bunix_msg *reply,
				const struct bunix_msg *message)
{
	struct usb_device *device;
	struct usb_interface *interface;
	struct usb_endpoint *endpoint;
	const u64 device_index = message->words[0];
	const u64 interface_number = message->words[1];
	const u64 endpoint_address = message->words[2] & 0xffu;
	const u64 requested_type = message->words[3] & 0xffu;
	const u64 requested_dir = (message->words[3] >> 8) & 0xffu;
	const u64 endpoint_dir = (endpoint_address & 0x80u) != 0 ?
					 BUNIX_USB_DIR_IN :
					 BUNIX_USB_DIR_OUT;
	long port;

	if (device_index >= device_count) {
		reply_noent(reply);
		return;
	}
	device = &devices[device_index];
	interface = find_interface(device, interface_number);
	endpoint = find_endpoint(device, interface_number, endpoint_address);
	if (interface == 0 || endpoint == 0) {
		reply_noent(reply);
		return;
	}
	if (interface->claim_owner != message->sender) {
		reply->words[0] = BUNIX_USB_ERR_BUSY;
		return;
	}
	if (requested_type != 0 &&
	    requested_type != (endpoint->attributes & 0x3u) + 1) {
		reply->words[0] = BUNIX_USB_ERR_INVAL;
		return;
	}
	if (requested_dir != 0 && requested_dir != endpoint_dir) {
		reply->words[0] = BUNIX_USB_ERR_INVAL;
		return;
	}
	if (endpoint->owner != 0 && endpoint->owner != message->sender) {
		reply->words[0] = BUNIX_USB_ERR_BUSY;
		return;
	}
	if (endpoint->port == 0) {
		port = bunix_port_create("usb-endpoint");
		if (port <= 0) {
			reply->words[0] = BUNIX_USB_ERR_INVAL;
			return;
		}
		endpoint->port = (u64)port;
		endpoint->owner = message->sender;
	}
	reply->words[0] = BUNIX_USB_OK;
	reply->words[1] = endpoint->address |
			  (((endpoint->attributes & 0x3u) + 1) << 8) |
			  (endpoint_dir << 16);
	reply->words[2] = endpoint->max_packet;
	reply->words[3] = endpoint->interval;
	reply->cap = endpoint->port;
	reply->cap_rights = BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP;
}

static void reply_endpoint_unsupported(struct bunix_msg *reply,
				       const struct bunix_msg *message)
{
	reply->protocol = BUNIX_PROTO_USB;
	reply->type = message->type;
	reply->words[0] = BUNIX_USB_ERR_UNSUPPORTED;
	reply->words[1] = 0;
	reply->words[2] = 0;
	reply->words[3] = 0;
}

static int handle_endpoint_message(struct bunix_msg *message)
{
	for (u64 d = 0; d < device_count; d++) {
		struct usb_device *device = &devices[d];

		for (u64 e = 0; e < device->endpoint_count; e++) {
			struct usb_endpoint *endpoint = &device->endpoints[e];

			if (endpoint->port == 0) {
				continue;
			}
			if (bunix_ipc_try_recv(endpoint->port, message) != 0) {
				continue;
			}
			return 1;
		}
	}
	return 0;
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

		int endpoint_message = handle_endpoint_message(&message);

		if (!endpoint_message) {
			const long recv =
				bunix_ipc_try_recv(BUNIX_HANDLE_SELF, &message);

			if (recv == 1) {
				bunix_sleep_ns(1000000ull);
				continue;
			}
			if (recv != 0) {
				continue;
			}
		}
		if (message.protocol != BUNIX_PROTO_USB) {
			continue;
		}

		reply.type = message.type;
		if (endpoint_message) {
			reply_endpoint_unsupported(&reply, &message);
			goto send_reply;
		}
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
			reply_claim_interface(&reply, &message, 1);
			break;
		case BUNIX_USB_RELEASE_INTERFACE:
			reply_claim_interface(&reply, &message, 0);
			break;
		case BUNIX_USB_OPEN_ENDPOINT:
			reply_open_endpoint(&reply, &message);
			break;
		default:
			break;
		}

send_reply:
		if (message.cap != 0) {
			bunix_handle_close(message.cap);
		}
		if (message.reply != 0) {
			(void)bunix_ipc_send(message.reply, &reply);
		}
	}
}
