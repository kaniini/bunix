#include <bunix/alloc.h>
#include <bunix/virtio.h>

enum {
	VIRTIO_BUS_HANDLE_NAMES = 3,
	VIRTIO_BUS_HANDLE_PCI_CONFIG = 4,
	VIRTIO_BUS_MAX_RESOURCES = 16,
	PCI_CONFIG_ADDRESS = 0,
	PCI_CONFIG_DATA = 4,
	PCI_VENDOR_NONE = 0xffff,
	PCI_COMMAND = 0x04,
	PCI_STATUS = 0x06,
	PCI_BAR0 = 0x10,
	PCI_CAP_PTR = 0x34,
	PCI_STATUS_CAP_LIST = 1 << 4,
	PCI_CAP_ID_VENDOR = 0x09,
	PCI_BAR_IO = 1,
	PCI_BAR_MEM_TYPE_MASK = 0x6,
	PCI_BAR_MEM_TYPE_64 = 0x4,
	PCI_BAR_MEM_PREFETCH = 0x8,
	VIRTIO_PCI_CAP_COMMON_CFG = 1,
	VIRTIO_PCI_CAP_NOTIFY_CFG = 2,
	VIRTIO_PCI_CAP_ISR_CFG = 3,
	VIRTIO_PCI_CAP_DEVICE_CFG = 4,
};

struct virtio_bus_device {
	struct bunix_virtio_device_info info;
	struct bunix_device_resource resources[VIRTIO_BUS_MAX_RESOURCES];
	u64 resource_count;
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

static void log_device(const struct virtio_bus_device *device, u64 index)
{
	char line[96];
	u64 offset = 0;

	if (device == 0) {
		return;
	}
	line[0] = '\0';
	append_text(line, sizeof(line), &offset, "virtio-bus: device index=");
	append_u64(line, sizeof(line), &offset, index);
	append_text(line, sizeof(line), &offset, " vendor=");
	append_u64(line, sizeof(line), &offset, device->info.id.vendor);
	append_text(line, sizeof(line), &offset, " device=");
	append_u64(line, sizeof(line), &offset, device->info.id.device);
	append_text(line, sizeof(line), &offset, " resources=");
	append_u64(line, sizeof(line), &offset, device->resource_count);
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

static long pci_config_write32(u64 bus, u64 slot, u64 function, u64 offset,
			       u64 value)
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
	return bunix_hw_port_out32(VIRTIO_BUS_HANDLE_PCI_CONFIG,
				   PCI_CONFIG_DATA, value);
}

static long pci_config_read8(u64 bus, u64 slot, u64 function, u64 offset)
{
	const long value = pci_config_read32(bus, slot, function, offset);

	return value < 0 ? value : (long)(((u64)value >> ((offset & 3) * 8)) & 0xff);
}

static long pci_config_read16(u64 bus, u64 slot, u64 function, u64 offset)
{
	const long value = pci_config_read32(bus, slot, function, offset);

	return value < 0 ? value : (long)(((u64)value >> ((offset & 2) * 8)) & 0xffff);
}

static int device_push(const struct virtio_bus_device *device)
{
	struct virtio_bus_device *next;
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
		next = (struct virtio_bus_device *)
			bunix_realloc(devices, old_size, new_size);
		if (next == 0) {
			return -1;
		}
		devices = next;
		device_capacity = new_capacity;
	}
	devices[device_count++] = *device;
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

static u64 pci_bar_grant(u64 bus, u64 slot, u64 function, u64 bar, u64 offset,
			 u64 len, u64 ops)
{
	const u64 packed = (bus & 0xff) | ((slot & 0x1f) << 8) |
			   ((function & 0x7) << 16) | ((bar & 0x7) << 24);
	const long handle = bunix_hw_pci_bar_grant(packed, offset, len, ops);

	return handle > 0 ? (u64)handle : 0;
}

static void resource_add(struct virtio_bus_device *device, u64 type, u64 ops,
			 u64 handle, u64 base, u64 len, u64 index, u64 flags)
{
	struct bunix_device_resource *resource;

	if (device == 0 || len == 0 ||
	    device->resource_count >= VIRTIO_BUS_MAX_RESOURCES) {
		return;
	}
	resource = &device->resources[device->resource_count++];
	resource->name = 0;
	resource->type = type;
	resource->ops = ops;
	resource->handle = handle;
	resource->base = base;
	resource->len = len;
	resource->alignment = 0;
	resource->index = index;
	resource->flags = flags;
}

static void scan_pci_bars(struct virtio_bus_device *device, u64 bus, u64 slot,
			  u64 function)
{
	for (u64 bar = 0; bar < 6; bar++) {
		const u64 offset = PCI_BAR0 + bar * 4;
		const long original = pci_config_read32(bus, slot, function,
							offset);
		long mask_value;
		u64 raw;
		u64 mask;
		u64 size;
		u64 base;
		u64 flags = 0;

		if (original < 0 || original == 0) {
			continue;
		}
		(void)pci_config_write32(bus, slot, function, offset,
					 0xffffffffu);
		mask_value = pci_config_read32(bus, slot, function, offset);
		(void)pci_config_write32(bus, slot, function, offset,
					 (u64)original);
		if (mask_value <= 0) {
			continue;
		}

		raw = (u64)original & 0xffffffffu;
		mask = (u64)mask_value & 0xffffffffu;
		if ((raw & PCI_BAR_IO) != 0) {
			base = raw & ~0x3ull;
			size = (~(mask & ~0x3ull) + 1) & 0xffffffffu;
			flags |= BUNIX_DEV_RESOURCE_FLAG_PCI_IO;
			const u64 ops = BUNIX_DEV_OP_READ | BUNIX_DEV_OP_WRITE;
			const u64 handle = pci_bar_grant(bus, slot, function,
							 bar, 0, size, ops);
			resource_add(device, BUNIX_DEV_RESOURCE_PIO, ops,
				     handle, base, size, bar, flags);
			continue;
		}

		base = raw & ~0xfull;
		size = (~(mask & ~0xfull) + 1) & 0xffffffffu;
		const u64 original_bar = bar;
		if ((raw & PCI_BAR_MEM_TYPE_MASK) == PCI_BAR_MEM_TYPE_64) {
			flags |= BUNIX_DEV_RESOURCE_FLAG_PCI_MEM64;
			bar++;
		}
		if ((raw & PCI_BAR_MEM_PREFETCH) != 0) {
			flags |= BUNIX_DEV_RESOURCE_FLAG_PCI_PREFETCH;
		}
		const u64 ops = BUNIX_DEV_OP_READ | BUNIX_DEV_OP_WRITE;
		const u64 handle = pci_bar_grant(bus, slot, function,
						 original_bar, 0, size, ops);
		resource_add(device, BUNIX_DEV_RESOURCE_MMIO, ops, handle,
			     base, size, original_bar, flags);
	}
}

static u64 virtio_cfg_flag(u64 cfg_type)
{
	switch (cfg_type) {
	case VIRTIO_PCI_CAP_COMMON_CFG:
		return BUNIX_DEV_RESOURCE_FLAG_VIRTIO_COMMON_CFG;
	case VIRTIO_PCI_CAP_NOTIFY_CFG:
		return BUNIX_DEV_RESOURCE_FLAG_VIRTIO_NOTIFY_CFG;
	case VIRTIO_PCI_CAP_ISR_CFG:
		return BUNIX_DEV_RESOURCE_FLAG_VIRTIO_ISR_CFG;
	case VIRTIO_PCI_CAP_DEVICE_CFG:
		return BUNIX_DEV_RESOURCE_FLAG_VIRTIO_DEVICE_CFG;
	default:
		return 0;
	}
}

static void scan_virtio_capabilities(struct virtio_bus_device *device, u64 bus,
				     u64 slot, u64 function)
{
	const long status = pci_config_read16(bus, slot, function, PCI_STATUS);
	long cap;
	u64 guard = 0;

	if (status < 0 || (((u64)status & PCI_STATUS_CAP_LIST) == 0)) {
		return;
	}
	cap = pci_config_read8(bus, slot, function, PCI_CAP_PTR);
	while (cap > 0 && guard++ < 48) {
		const u64 offset = (u64)cap & ~0x3ull;
		const long header = pci_config_read32(bus, slot, function,
						      offset);
		const long data0 = pci_config_read32(bus, slot, function,
						     offset + 4);
		const long cap_offset = pci_config_read32(bus, slot, function,
							  offset + 8);
		const long cap_len = pci_config_read32(bus, slot, function,
						       offset + 12);
		u64 cfg_type;
		u64 bar;
		u64 flag;

		if (header < 0) {
			return;
		}
		if (((u64)header & 0xff) == PCI_CAP_ID_VENDOR &&
		    data0 >= 0 && cap_offset >= 0 && cap_len > 0) {
			cfg_type = ((u64)header >> 24) & 0xff;
			bar = (u64)data0 & 0xff;
			flag = virtio_cfg_flag(cfg_type);
			if (flag != 0 && bar < 6) {
				for (u64 i = 0; i < device->resource_count; i++) {
					struct bunix_device_resource *resource =
						&device->resources[i];
					if (resource->index == bar) {
						const u64 sub_offset =
							(u64)cap_offset &
							0xffffffffu;
						const u64 sub_len =
							(u64)cap_len &
							0xffffffffu;
						const u64 handle =
							pci_bar_grant(
								bus, slot,
								function, bar,
								sub_offset,
								sub_len,
								resource->ops);
						resource_add(
							device, resource->type,
							resource->ops, handle,
							resource->base +
							sub_offset, sub_len,
							bar, resource->flags |
							flag);
						break;
					}
				}
			}
		}
		cap = (long)(((u64)header >> 8) & 0xff);
	}
}

static void scan_pci_function(u64 bus, u64 slot, u64 function)
{
	const long id = pci_config_read32(bus, slot, function, 0x00);
	const long class_revision = pci_config_read32(bus, slot, function, 0x08);
	struct virtio_bus_device device_record;
	struct bunix_virtio_device_info *info = &device_record.info;
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

	device_record.resource_count = 0;
	info->id.transport = BUNIX_DEV_TRANSPORT_PCI;
	info->id.bus = bus;
	info->id.slot = slot;
	info->id.function = function;
	info->id.vendor = vendor;
	info->id.device = device;
	info->id.subsystem_vendor = 0;
	info->id.subsystem_device = 0;
	info->id.class_code = ((u64)class_revision >> 8) & 0xffffff;
	info->id.revision = (u64)class_revision & 0xff;
	info->transport.transport = BUNIX_VIRTIO_TRANSPORT_PCI;
	info->transport.device_index = device_count;
	info->transport.device_type = device >= 0x1040 ? device - 0x1040 : 0;
	info->transport.vendor = vendor;
	info->transport.device = device;
	info->transport.common_cfg_resource = 0;
	info->transport.notify_resource = 0;
	info->transport.isr_resource = 0;
	info->transport.device_cfg_resource = 0;
	info->transport.notify_off_multiplier = 0;
	info->features.device_features = 0;
	info->features.driver_features = 0;
	info->features.required_features = 0;
	info->features.negotiated_features = 0;
	scan_pci_bars(&device_record, bus, slot, function);
	scan_virtio_capabilities(&device_record, bus, slot, function);
	(void)device_push(&device_record);
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
	struct virtio_bus_device *device;
	struct bunix_virtio_device_info *info;

	if (index >= device_count) {
		reply_no_device(reply);
		return;
	}
	device = &devices[index];
	info = &device->info;
	reply->words[0] = BUNIX_DEV_OK;
	reply->words[1] = info->id.transport | (device->resource_count << 32);
	reply->words[2] = info->id.vendor | (info->id.device << 32);
	reply->words[3] = info->id.bus | (info->id.slot << 8) |
			  (info->id.function << 16) |
			  (info->transport.device_type << 32);
}

static void reply_device_resource(struct bunix_msg *reply, u64 device_index,
				  u64 resource_index)
{
	struct virtio_bus_device *device;
	struct bunix_device_resource *resource;

	if (device_index >= device_count) {
		reply_no_device(reply);
		return;
	}
	device = &devices[device_index];
	if (resource_index >= device->resource_count) {
		reply_no_device(reply);
		return;
	}
	resource = &device->resources[resource_index];
	reply->words[0] = BUNIX_DEV_OK;
	reply->words[1] = resource->type | (resource->ops << 16) |
			  (resource->flags << 32);
	reply->words[2] = resource->base;
	reply->words[3] = resource->len;
	reply->cap = resource->handle;
	reply->cap_rights = resource->handle != 0 ? BUNIX_RIGHT_SEND : 0;
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
	for (u64 i = 0; i < device_count; i++) {
		log_device(&devices[i], i);
	}
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
			reply_device_resource(&reply, message.words[0],
					      message.words[1]);
			break;
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
