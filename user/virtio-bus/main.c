#include <bunix/alloc.h>
#include <bunix/virtio.h>

enum {
	VIRTIO_BUS_HANDLE_NAMES = 3,
	VIRTIO_BUS_HANDLE_PCI_CONFIG = 4,
	PCI_CONFIG_ADDRESS = 0,
	PCI_CONFIG_DATA = 4,
	PCI_VENDOR_NONE = 0xffff,
};

struct virtio_bus_device {
	struct bunix_virtio_device_info info;
};

static struct virtio_bus_device *devices;
static u64 device_count;
static u64 device_capacity;

static void reply_no_device(struct bunix_msg *reply);

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
		digits[count++] = (char)('0' + (value % 10));
		value /= 10;
	}
	while (count != 0) {
		append_char(out, out_size, offset, digits[--count]);
	}
}

static void log_ready(void)
{
	char line[64];
	u64 offset = 0;

	line[0] = '\0';
	append_text(line, sizeof(line), &offset, "virtio-bus: ready devices=");
	append_u64(line, sizeof(line), &offset, device_count);
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

	if (bunix_ipc_call(VIRTIO_BUS_HANDLE_NAMES, &request, &reply) != 0) {
		return -1;
	}
	return reply.words[0] == BUNIX_DEV_OK ? 0 : -1;
}

static long pci_config_read32(u64 bus, u64 slot, u64 function, u64 offset)
{
	const u64 address = 0x80000000ull |
			    ((bus & 0xff) << 16) |
			    ((slot & 0x1f) << 11) |
			    ((function & 0x7) << 8) |
			    (offset & 0xfc);

	if (bunix_hw_port_out32(VIRTIO_BUS_HANDLE_PCI_CONFIG,
				PCI_CONFIG_ADDRESS, address) != 0) {
		return -1;
	}
	return bunix_hw_port_in32(VIRTIO_BUS_HANDLE_PCI_CONFIG,
				  PCI_CONFIG_DATA);
}

static int device_push(const struct bunix_virtio_device_info *info)
{
	struct virtio_bus_device *next;
	u64 old_size;
	u64 new_size;
	u64 new_capacity;

	if (info == 0) {
		return -1;
	}
	if (device_count == device_capacity) {
		old_size = device_capacity * sizeof(devices[0]);
		new_capacity = device_capacity == 0 ? 4 : device_capacity * 2;
		new_size = new_capacity * sizeof(devices[0]);
		next = (struct virtio_bus_device *)
			bunix_realloc(devices, old_size, new_size);
		if (next == 0) {
			return -1;
		}
		devices = next;
		device_capacity = new_capacity;
	}
	devices[device_count++].info = *info;
	return 0;
}

static int pci_is_virtio(u64 vendor, u64 device)
{
	if (vendor != BUNIX_VIRTIO_PCI_VENDOR_ID) {
		return 0;
	}
	return (device >= 0x1000 && device <= 0x107f) ||
	       (device >= 0x1040 && device <= 0x107f);
}

static void scan_pci_function(u64 bus, u64 slot, u64 function)
{
	const long id = pci_config_read32(bus, slot, function, 0x00);
	const long class_revision = pci_config_read32(bus, slot, function, 0x08);
	struct bunix_virtio_device_info info;
	u64 vendor;
	u64 device;

	if (id < 0 || class_revision < 0) {
		return;
	}
	vendor = (u64)id & 0xffff;
	device = ((u64)id >> 16) & 0xffff;
	if (vendor == PCI_VENDOR_NONE || !pci_is_virtio(vendor, device)) {
		return;
	}

	info.id.transport = BUNIX_DEV_TRANSPORT_PCI;
	info.id.bus = bus;
	info.id.slot = slot;
	info.id.function = function;
	info.id.vendor = vendor;
	info.id.device = device;
	info.id.subsystem_vendor = 0;
	info.id.subsystem_device = 0;
	info.id.class_code = ((u64)class_revision >> 8) & 0xffffff;
	info.id.revision = (u64)class_revision & 0xff;
	info.transport.transport = BUNIX_VIRTIO_TRANSPORT_PCI;
	info.transport.device_index = device_count;
	info.transport.device_type = device >= 0x1040 ? device - 0x1040 : 0;
	info.transport.vendor = vendor;
	info.transport.device = device;
	info.transport.common_cfg_resource = 0;
	info.transport.notify_resource = 0;
	info.transport.isr_resource = 0;
	info.transport.device_cfg_resource = 0;
	info.transport.notify_off_multiplier = 0;
	info.features.device_features = 0;
	info.features.driver_features = 0;
	info.features.required_features = 0;
	info.features.negotiated_features = 0;
	(void)device_push(&info);
}

static void scan_pci_bus0(void)
{
	for (u64 slot = 0; slot < 32; slot++) {
		const long header = pci_config_read32(0, slot, 0, 0x0c);
		const u64 header_type = header < 0 ? 0 : ((u64)header >> 16) & 0xff;
		const u64 functions = (header_type & 0x80) != 0 ? 8 : 1;

		for (u64 function = 0; function < functions; function++) {
			scan_pci_function(0, slot, function);
		}
	}
}

static void reply_empty_enumeration(struct bunix_msg *reply)
{
	reply->words[0] = BUNIX_DEV_OK;
	reply->words[1] = device_count;
	reply->words[2] = 0;
	reply->words[3] = 0;
}

static void reply_device_info(struct bunix_msg *reply, u64 index)
{
	struct bunix_virtio_device_info *info;

	if (index >= device_count) {
		reply_no_device(reply);
		return;
	}
	info = &devices[index].info;
	reply->words[0] = BUNIX_DEV_OK;
	reply->words[1] = info->id.transport;
	reply->words[2] = info->id.vendor | (info->id.device << 32);
	reply->words[3] = info->id.bus | (info->id.slot << 8) |
			  (info->id.function << 16) |
			  (info->transport.device_type << 32);
}

static void reply_no_device(struct bunix_msg *reply)
{
	reply->words[0] = BUNIX_DEV_ERR_NOENT;
	reply->words[1] = 0;
	reply->words[2] = 0;
	reply->words[3] = 0;
}

int main(void)
{
	const char online[] = "virtio-bus: online\n";
	struct bunix_msg message;

	bunix_console_log(online, sizeof(online) - 1);
	scan_pci_bus0();
	if (register_service(BUNIX_SERVICE_DEVICE, BUNIX_HANDLE_SELF) != 0) {
		return 1;
	}
	log_ready();

	for (;;) {
		struct bunix_msg reply = {
			.protocol = BUNIX_PROTO_DEVICE,
			.type = 0,
			.sender = 0,
			.cap_rights = 0,
			.reply = 0,
			.cap = 0,
			.words = { BUNIX_DEV_ERR_UNSUPPORTED, 0, 0, 0 },
		};

		if (bunix_ipc_recv(BUNIX_HANDLE_SELF, &message) != 0 ||
		    message.protocol != BUNIX_PROTO_DEVICE) {
			continue;
		}

		reply.type = message.type;
		switch (message.type) {
		case BUNIX_DEV_ENUMERATE:
			reply_empty_enumeration(&reply);
			break;
		case BUNIX_DEV_GET_INFO:
			reply_device_info(&reply, message.words[0]);
			break;
		case BUNIX_DEV_GET_RESOURCE:
		case BUNIX_DEV_BIND_DRIVER:
		case BUNIX_DEV_RESET:
		case BUNIX_DEV_READ_FEATURES:
		case BUNIX_DEV_NEGOTIATE_FEATURES:
		case BUNIX_DEV_SETUP_QUEUE:
		case BUNIX_DEV_NOTIFY_QUEUE:
		case BUNIX_DEV_ACK_INTERRUPT:
			reply_no_device(&reply);
			break;
		default:
			reply.words[0] = BUNIX_DEV_ERR_UNSUPPORTED;
			break;
		}

		if (message.reply != 0) {
			bunix_ipc_send(message.reply, &reply);
		}
	}
}
