#include <bunix/alloc.h>
#include <bunix/syscall.h>

enum {
	PCI_HANDLE_NAMES = 3,
	PCI_HANDLE_CONFIG = 4,
	PCI_HANDLE_AUTH = BUNIX_HANDLE_PCI_AUTH,
	PCI_CONFIG_ADDRESS = 0,
	PCI_CONFIG_DATA = 4,
	PCI_VENDOR_NONE = 0xffff,
	PCI_COMMAND = 0x04,
	PCI_COMMAND_IO = 1 << 0,
	PCI_COMMAND_MEMORY = 1 << 1,
	PCI_COMMAND_BUS_MASTER = 1 << 2,
	PCI_BAR0 = 0x10,
	PCI_INTERRUPT_LINE = 0x3c,
	PCI_INTERRUPT_PIN = 0x3d,
	PCI_BAR_IO = 1,
	PCI_BAR_MEM_TYPE_MASK = 0x6,
	PCI_BAR_MEM_TYPE_64 = 0x4,
	PCI_BAR_MEM_PREFETCH = 0x8,
	PCI_MAX_BARS = 6,
};

struct pci_bar {
	u64 present;
	u64 type;
	u64 ops;
	u64 base;
	u64 len;
	u64 flags;
};

struct pci_function {
	u64 bus;
	u64 slot;
	u64 function;
	u64 vendor;
	u64 device;
	u64 class_code;
	u64 revision;
	u64 interrupt_line;
	u64 interrupt_pin;
	struct pci_bar bars[PCI_MAX_BARS];
};

static struct pci_function *functions;
static u64 function_count;
static u64 function_capacity;

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

static void log_ready(void)
{
	char line[80];
	u64 offset = 0;

	line[0] = '\0';
	append_text(line, sizeof(line), &offset, "pci: ready functions=");
	append_u64(line, sizeof(line), &offset, function_count);
	append_char(line, sizeof(line), &offset, '\n');
	bunix_console_log(line, offset);
}

static void log_function(const struct pci_function *func, u64 index)
{
	char line[112];
	u64 offset = 0;

	if (func == 0) {
		return;
	}
	line[0] = '\0';
	append_text(line, sizeof(line), &offset, "pci: function index=");
	append_u64(line, sizeof(line), &offset, index);
	append_text(line, sizeof(line), &offset, " bus=");
	append_u64(line, sizeof(line), &offset, func->bus);
	append_text(line, sizeof(line), &offset, " slot=");
	append_u64(line, sizeof(line), &offset, func->slot);
	append_text(line, sizeof(line), &offset, " fn=");
	append_u64(line, sizeof(line), &offset, func->function);
	append_text(line, sizeof(line), &offset, " vendor=");
	append_u64(line, sizeof(line), &offset, func->vendor);
	append_text(line, sizeof(line), &offset, " device=");
	append_u64(line, sizeof(line), &offset, func->device);
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

	if (bunix_ipc_call(PCI_HANDLE_NAMES, &request, &reply) != 0) {
		return -1;
	}
	return reply.words[0] == BUNIX_DEV_OK ? 0 : -1;
}

static long pci_config_read32_raw(u64 bus, u64 slot, u64 function, u64 offset)
{
	const u64 address = 0x80000000ull |
			    ((bus & 0xff) << 16) |
			    ((slot & 0x1f) << 11) |
			    ((function & 0x7) << 8) |
			    (offset & 0xfc);

	if (bunix_hw_port_out32(PCI_HANDLE_CONFIG, PCI_CONFIG_ADDRESS,
				address) != 0) {
		return -1;
	}
	return bunix_hw_port_in32(PCI_HANDLE_CONFIG, PCI_CONFIG_DATA);
}

static long pci_config_write32_raw(u64 bus, u64 slot, u64 function, u64 offset,
				   u64 value)
{
	const u64 address = 0x80000000ull |
			    ((bus & 0xff) << 16) |
			    ((slot & 0x1f) << 11) |
			    ((function & 0x7) << 8) |
			    (offset & 0xfc);

	if (bunix_hw_port_out32(PCI_HANDLE_CONFIG, PCI_CONFIG_ADDRESS,
				address) != 0) {
		return -1;
	}
	return bunix_hw_port_out32(PCI_HANDLE_CONFIG, PCI_CONFIG_DATA, value);
}

static long pci_config_read(u64 bus, u64 slot, u64 function, u64 offset,
			    u64 width)
{
	const long value = pci_config_read32_raw(bus, slot, function, offset);

	if (value < 0) {
		return value;
	}
	if (width == 1) {
		return (long)(((u64)value >> ((offset & 3) * 8)) & 0xff);
	}
	if (width == 2) {
		return (long)(((u64)value >> ((offset & 2) * 8)) & 0xffff);
	}
	if (width == 4) {
		return value;
	}
	return -1;
}

static void pci_enable_device(const struct pci_function *func)
{
	const long value = pci_config_read32_raw(func->bus, func->slot,
						func->function, PCI_COMMAND);
	u64 command;

	if (value < 0) {
		return;
	}
	command = ((u64)value & 0xffffu) | PCI_COMMAND_IO |
		  PCI_COMMAND_MEMORY | PCI_COMMAND_BUS_MASTER;
	(void)pci_config_write32_raw(func->bus, func->slot, func->function,
				     PCI_COMMAND,
				     ((u64)value & 0xffff0000u) | command);
}

static u64 pci_device_pack(const struct pci_function *func, u64 bar)
{
	return (func->bus & 0xff) | ((func->slot & 0x1f) << 8) |
	       ((func->function & 0x7) << 16) | ((bar & 0x7) << 24);
}

static u64 grant_bar(const struct pci_function *func, u64 bar, u64 offset,
		     u64 len, u64 ops)
{
	const long handle = bunix_hw_pci_bar_grant(PCI_HANDLE_AUTH,
						   pci_device_pack(func, bar),
						   offset, len, ops);

	return handle > 0 ? (u64)handle : 0;
}

static u64 grant_irq(const struct pci_function *func)
{
	const long handle = bunix_hw_pci_irq_grant(PCI_HANDLE_AUTH,
						   pci_device_pack(func, 0),
						   func->interrupt_line);

	return handle > 0 ? (u64)handle : 0;
}

static void scan_bars(struct pci_function *func)
{
	for (u64 bar = 0; bar < PCI_MAX_BARS; bar++) {
		const u64 offset = PCI_BAR0 + bar * 4;
		const long original = pci_config_read32_raw(func->bus,
							    func->slot,
							    func->function,
							    offset);
		long original_high = 0;
		long mask_value;
		long mask_high_value = 0;
		u64 raw;
		u64 raw_high = 0;
		u64 mask;
		u64 mask_high = 0;
		u64 size;
		u64 base;
		u64 flags = 0;
		u64 type;

		if (original < 0 || original == 0) {
			continue;
		}
		raw = (u64)original & 0xffffffffu;
		if ((raw & PCI_BAR_IO) == 0 &&
		    (raw & PCI_BAR_MEM_TYPE_MASK) == PCI_BAR_MEM_TYPE_64) {
			if (bar + 1 >= PCI_MAX_BARS) {
				continue;
			}
			original_high = pci_config_read32_raw(func->bus,
							      func->slot,
							      func->function,
							      offset + 4);
			if (original_high < 0) {
				continue;
			}
			(void)pci_config_write32_raw(func->bus, func->slot,
						     func->function,
						     offset + 4,
						     0xffffffffu);
		}
		(void)pci_config_write32_raw(func->bus, func->slot,
					     func->function, offset,
					     0xffffffffu);
		mask_value = pci_config_read32_raw(func->bus, func->slot,
						   func->function, offset);
		if ((raw & PCI_BAR_IO) == 0 &&
		    (raw & PCI_BAR_MEM_TYPE_MASK) == PCI_BAR_MEM_TYPE_64) {
			mask_high_value =
				pci_config_read32_raw(func->bus, func->slot,
						      func->function,
						      offset + 4);
		}
		(void)pci_config_write32_raw(func->bus, func->slot,
					     func->function, offset,
					     (u64)original);
		if ((raw & PCI_BAR_IO) == 0 &&
		    (raw & PCI_BAR_MEM_TYPE_MASK) == PCI_BAR_MEM_TYPE_64) {
			(void)pci_config_write32_raw(func->bus, func->slot,
						     func->function,
						     offset + 4,
						     (u64)original_high);
		}
		if (mask_value <= 0) {
			continue;
		}

		mask = (u64)mask_value & 0xffffffffu;
		if ((raw & PCI_BAR_IO) != 0) {
			base = raw & ~0x3ull;
			size = (~(mask & ~0x3ull) + 1) & 0xffffffffu;
			flags |= BUNIX_DEV_RESOURCE_FLAG_PCI_IO;
			type = BUNIX_DEV_RESOURCE_PIO;
		} else {
			base = raw & ~0xfull;
			size = (~(mask & ~0xfull) + 1) & 0xffffffffu;
			type = BUNIX_DEV_RESOURCE_MMIO;
			if ((raw & PCI_BAR_MEM_TYPE_MASK) == PCI_BAR_MEM_TYPE_64) {
				if (mask_high_value < 0) {
					continue;
				}
				raw_high = (u64)original_high & 0xffffffffu;
				mask_high = (u64)mask_high_value & 0xffffffffu;
				base = ((raw_high << 32) | raw) & ~0xfull;
				size = ~(((mask_high << 32) | mask) &
					 ~0xfull) + 1;
				flags |= BUNIX_DEV_RESOURCE_FLAG_PCI_MEM64;
			}
			if ((raw & PCI_BAR_MEM_PREFETCH) != 0) {
				flags |= BUNIX_DEV_RESOURCE_FLAG_PCI_PREFETCH;
			}
		}

		func->bars[bar].present = 1;
		func->bars[bar].type = type;
		func->bars[bar].ops = BUNIX_DEV_OP_READ | BUNIX_DEV_OP_WRITE;
		func->bars[bar].base = base;
		func->bars[bar].len = size;
		func->bars[bar].flags = flags;
		if ((raw & PCI_BAR_IO) == 0 &&
		    (raw & PCI_BAR_MEM_TYPE_MASK) == PCI_BAR_MEM_TYPE_64) {
			bar++;
		}
	}
}

static int function_push(const struct pci_function *func)
{
	struct pci_function *next;
	u64 old_size;
	u64 new_size;
	u64 new_capacity;

	if (func == 0) {
		return -1;
	}
	if (function_count == function_capacity) {
		old_size = function_capacity * sizeof(functions[0]);
		new_capacity = function_capacity == 0 ? 8 : function_capacity * 2;
		new_size = new_capacity * sizeof(functions[0]);
		next = (struct pci_function *)bunix_realloc(functions, old_size,
							    new_size);
		if (next == 0) {
			return -1;
		}
		functions = next;
		function_capacity = new_capacity;
	}
	functions[function_count++] = *func;
	return 0;
}

static void scan_function(u64 bus, u64 slot, u64 function)
{
	const long id = pci_config_read32_raw(bus, slot, function, 0x00);
	const long class_revision = pci_config_read32_raw(bus, slot, function,
							  0x08);
	struct pci_function func;
	u64 vendor;

	if (id < 0 || class_revision < 0) {
		return;
	}
	vendor = (u64)id & 0xffffu;
	if (vendor == PCI_VENDOR_NONE) {
		return;
	}
	for (u64 i = 0; i < sizeof(func) / sizeof(u64); i++) {
		((u64 *)&func)[i] = 0;
	}
	func.bus = bus;
	func.slot = slot;
	func.function = function;
	func.vendor = vendor;
	func.device = ((u64)id >> 16) & 0xffffu;
	func.class_code = ((u64)class_revision >> 8) & 0xffffffu;
	func.revision = (u64)class_revision & 0xffu;
	func.interrupt_line =
		(u64)pci_config_read(bus, slot, function, PCI_INTERRUPT_LINE, 1);
	func.interrupt_pin =
		(u64)pci_config_read(bus, slot, function, PCI_INTERRUPT_PIN, 1);
	scan_bars(&func);
	(void)function_push(&func);
}

static void scan_bus0(void)
{
	for (u64 slot = 0; slot < 32; slot++) {
		const long header = pci_config_read32_raw(0, slot, 0, 0x0c);
		const u64 header_type = header < 0 ? 0 :
					(((u64)header >> 16) & 0xff);
		const u64 count = (header_type & 0x80) != 0 ? 8 : 1;

		for (u64 function = 0; function < count; function++) {
			scan_function(0, slot, function);
		}
	}
}

static struct pci_function *function_get(u64 index)
{
	return index < function_count ? &functions[index] : 0;
}

static void reply_noent(struct bunix_msg *reply)
{
	reply->words[0] = BUNIX_PCI_ERR_NOENT;
	reply->words[1] = 0;
	reply->words[2] = 0;
	reply->words[3] = 0;
}

static void reply_bar(struct bunix_msg *reply, u64 index, u64 bar,
		      u64 sub_offset, u64 sub_len)
{
	struct pci_function *func = function_get(index);
	struct pci_bar *resource;
	u64 len;
	u64 handle;

	if (func == 0 || bar >= PCI_MAX_BARS || func->bars[bar].present == 0) {
		reply_noent(reply);
		return;
	}
	resource = &func->bars[bar];
	len = sub_len == 0 ? resource->len : sub_len;
	if (sub_offset > resource->len || len > resource->len - sub_offset) {
		reply->words[0] = BUNIX_PCI_ERR_INVAL;
		return;
	}
	handle = grant_bar(func, bar, sub_offset, len, resource->ops);
	reply->words[0] = BUNIX_PCI_OK;
	reply->words[1] = resource->type | (resource->ops << 16) |
			  (resource->flags << 32);
	reply->words[2] = resource->base + sub_offset;
	reply->words[3] = len;
	reply->cap = handle;
	reply->cap_rights = handle != 0 ? BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP :
					   0;
}

int main(void)
{
	const char online[] = "pci: online\n";
	struct bunix_msg message;

	bunix_console_log(online, sizeof(online) - 1);
	scan_bus0();
	for (u64 i = 0; i < function_count; i++) {
		log_function(&functions[i], i);
	}
	if (register_service(BUNIX_SERVICE_PCI, BUNIX_HANDLE_SELF) != 0) {
		return 1;
	}
	log_ready();

	for (;;) {
		struct bunix_msg reply = {
			.protocol = BUNIX_PROTO_PCI,
			.type = 0,
			.sender = 0,
			.cap_rights = 0,
			.reply = 0,
			.cap = 0,
			.words = { BUNIX_PCI_ERR_UNSUPPORTED, 0, 0, 0 },
		};

		if (bunix_ipc_recv(BUNIX_HANDLE_SELF, &message) != 0 ||
		    message.protocol != BUNIX_PROTO_PCI) {
			continue;
		}
		reply.type = message.type;
		switch (message.type) {
		case BUNIX_PCI_ENUMERATE:
			reply.words[0] = BUNIX_PCI_OK;
			reply.words[1] = function_count;
			break;
		case BUNIX_PCI_GET_INFO: {
			struct pci_function *func = function_get(message.words[0]);

			if (func == 0) {
				reply_noent(&reply);
				break;
			}
			reply.words[0] = BUNIX_PCI_OK;
			reply.words[1] = func->bus | (func->slot << 8) |
					  (func->function << 16);
			reply.words[2] = func->vendor | (func->device << 16) |
					  (func->revision << 32);
			reply.words[3] = func->class_code |
					  (func->interrupt_line << 32) |
					  (func->interrupt_pin << 40);
			break;
		}
		case BUNIX_PCI_ENABLE: {
			struct pci_function *func = function_get(message.words[0]);

			if (func == 0) {
				reply_noent(&reply);
				break;
			}
			pci_enable_device(func);
			reply.words[0] = BUNIX_PCI_OK;
			break;
		}
		case BUNIX_PCI_READ_CONFIG: {
			struct pci_function *func = function_get(message.words[0]);
			long value;

			if (func == 0) {
				reply_noent(&reply);
				break;
			}
			value = pci_config_read(func->bus, func->slot,
						func->function, message.words[1],
						message.words[2]);
			if (value < 0) {
				reply.words[0] = BUNIX_PCI_ERR_INVAL;
				break;
			}
			reply.words[0] = BUNIX_PCI_OK;
			reply.words[1] = (u64)value;
			break;
		}
		case BUNIX_PCI_GET_BAR:
			reply_bar(&reply, message.words[0], message.words[1], 0,
				  0);
			break;
		case BUNIX_PCI_GRANT_BAR:
			reply_bar(&reply, message.words[0], message.words[1],
				  message.words[2], message.words[3]);
			break;
		case BUNIX_PCI_GRANT_IRQ: {
			struct pci_function *func = function_get(message.words[0]);
			u64 handle;
			const u64 ops = BUNIX_DEV_OP_BIND_IRQ |
					BUNIX_DEV_OP_ACK_IRQ |
					BUNIX_DEV_OP_MASK_IRQ;

			if (func == 0 || func->interrupt_line == 0 ||
			    func->interrupt_line >= 0xff ||
			    func->interrupt_pin == 0 ||
			    func->interrupt_pin > 4) {
				reply_noent(&reply);
				break;
			}
			handle = grant_irq(func);
			reply.words[0] = BUNIX_PCI_OK;
			reply.words[1] = BUNIX_DEV_RESOURCE_IRQ | (ops << 16);
			reply.words[2] = func->interrupt_line;
			reply.words[3] = func->interrupt_pin;
			reply.cap = handle;
			reply.cap_rights = handle != 0 ?
						   BUNIX_RIGHT_SEND |
							   BUNIX_RIGHT_DUP :
						   0;
			break;
		}
		default:
			break;
		}

		(void)bunix_ipc_send(message.reply, &reply);
	}
}
