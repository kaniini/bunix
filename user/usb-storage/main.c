#include <bunix/syscall.h>

#define STORAGE_HANDLE_USB (bunix_handle_find(BUNIX_CAP_USB))

enum {
	STORAGE_MAX_DESCRIPTOR = 512,
	STORAGE_CLASS_MASS = 8,
	STORAGE_SUBCLASS_SCSI = 6,
	STORAGE_PROTOCOL_BULK_ONLY = 80,
	USB_ENDPOINT_ATTR_BULK = 2,
};

struct usb_storage_disk {
	u64 device_index;
	u64 interface_number;
	u64 bulk_in_address;
	u64 bulk_out_address;
	u64 bulk_in_handle;
	u64 bulk_out_handle;
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

static void log_claimed(const struct usb_storage_disk *disk)
{
	char line[160];
	u64 offset = 0;

	line[0] = '\0';
	append_text(line, sizeof(line), &offset,
		    "usb-storage: claimed device=");
	append_u64(line, sizeof(line), &offset, disk->device_index);
	append_text(line, sizeof(line), &offset, " interface=");
	append_u64(line, sizeof(line), &offset, disk->interface_number);
	append_text(line, sizeof(line), &offset, " bulk_in=");
	append_u64(line, sizeof(line), &offset, disk->bulk_in_address);
	append_text(line, sizeof(line), &offset, " bulk_out=");
	append_u64(line, sizeof(line), &offset, disk->bulk_out_address);
	append_char(line, sizeof(line), &offset, '\n');
	bunix_console_log(line, offset);
}

static int usb_call(struct bunix_msg *request, struct bunix_msg *reply)
{
	return bunix_ipc_call(STORAGE_HANDLE_USB, request, reply) == 0 &&
		       reply->protocol == BUNIX_PROTO_USB &&
		       reply->words[0] == BUNIX_USB_OK ?
		       0 :
		       -1;
}

static long usb_device_count(void)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_USB,
		.type = BUNIX_USB_DEVICE_COUNT,
	};
	struct bunix_msg reply;

	if (usb_call(&request, &reply) != 0) {
		return -1;
	}
	return (long)reply.words[1];
}

static int usb_read_descriptor(u64 device_index, unsigned char *out,
			       u64 out_len, u64 *actual_len)
{
	long buffer = bunix_buffer_create(out_len);
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_USB,
		.type = BUNIX_USB_DESCRIPTOR_READ,
		.cap_rights = BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP,
		.cap = buffer > 0 ? (u64)buffer : 0,
		.words = { device_index, 0, out_len, 0 },
	};
	struct bunix_msg reply;

	if (buffer <= 0 || out == 0 || actual_len == 0 ||
	    usb_call(&request, &reply) != 0 ||
	    reply.words[1] > out_len ||
	    bunix_buffer_read((u64)buffer, 0, out, reply.words[1]) != 0) {
		if (buffer > 0) {
			bunix_handle_close((u64)buffer);
		}
		return -1;
	}
	*actual_len = reply.words[1];
	bunix_handle_close((u64)buffer);
	return 0;
}

static int usb_claim_interface(u64 device_index, u64 interface_number)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_USB,
		.type = BUNIX_USB_CLAIM_INTERFACE,
		.words = { device_index, interface_number, 0, 0 },
	};
	struct bunix_msg reply;

	return usb_call(&request, &reply);
}

static long usb_open_bulk_endpoint(u64 device_index, u64 interface_number,
				   u64 endpoint_address, u64 dir)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_USB,
		.type = BUNIX_USB_OPEN_ENDPOINT,
		.words = { device_index, interface_number, endpoint_address,
			   BUNIX_USB_TRANSFER_BULK | (dir << 8) },
	};
	struct bunix_msg reply;

	if (usb_call(&request, &reply) != 0 || reply.cap == 0 ||
	    (reply.cap_rights & BUNIX_RIGHT_SEND) == 0) {
		if (reply.cap != 0) {
			bunix_handle_close(reply.cap);
		}
		return -1;
	}
	return (long)reply.cap;
}

static int claim_storage_from_descriptor(u64 device_index,
					 const unsigned char *descriptor,
					 u64 descriptor_len,
					 struct usb_storage_disk *disk)
{
	u64 offset = 0;
	u64 current_interface = (u64)-1;
	int current_is_storage = 0;
	u64 storage_interface = (u64)-1;
	u64 bulk_in = 0;
	u64 bulk_out = 0;

	while (offset + 2 <= descriptor_len) {
		const u64 len = descriptor[offset];
		const u64 type = descriptor[offset + 1];

		if (len < 2 || offset + len > descriptor_len) {
			return -1;
		}
		if (type == BUNIX_USB_DESCRIPTOR_INTERFACE && len >= 9) {
			current_interface = descriptor[offset + 2];
			current_is_storage =
				descriptor[offset + 5] == STORAGE_CLASS_MASS &&
				descriptor[offset + 6] == STORAGE_SUBCLASS_SCSI &&
				descriptor[offset + 7] ==
					STORAGE_PROTOCOL_BULK_ONLY;
			if (current_is_storage) {
				storage_interface = current_interface;
			}
		} else if (type == BUNIX_USB_DESCRIPTOR_ENDPOINT && len >= 7 &&
			   current_is_storage) {
			const u64 address = descriptor[offset + 2];
			const u64 attributes = descriptor[offset + 3];

			if ((attributes & 0x3u) == USB_ENDPOINT_ATTR_BULK) {
				if ((address & 0x80u) != 0) {
					bulk_in = address;
				} else {
					bulk_out = address;
				}
			}
		}
		offset += len;
	}
	if (storage_interface == (u64)-1 || bulk_in == 0 || bulk_out == 0) {
		return -1;
	}
	if (usb_claim_interface(device_index, storage_interface) != 0) {
		return -1;
	}
	disk->bulk_in_handle = (u64)usb_open_bulk_endpoint(
		device_index, storage_interface, bulk_in, BUNIX_USB_DIR_IN);
	disk->bulk_out_handle = (u64)usb_open_bulk_endpoint(
		device_index, storage_interface, bulk_out, BUNIX_USB_DIR_OUT);
	if (disk->bulk_in_handle == 0 || disk->bulk_out_handle == 0) {
		return -1;
	}
	disk->device_index = device_index;
	disk->interface_number = storage_interface;
	disk->bulk_in_address = bulk_in;
	disk->bulk_out_address = bulk_out;
	log_claimed(disk);
	return 0;
}

static int scan_for_storage(struct usb_storage_disk *disk)
{
	unsigned char descriptor[STORAGE_MAX_DESCRIPTOR];
	long count = usb_device_count();

	if (disk == 0 || disk->bulk_in_handle != 0 || count <= 0) {
		return -1;
	}
	for (u64 i = 0; i < (u64)count; i++) {
		u64 descriptor_len = 0;

		if (usb_read_descriptor(i, descriptor, sizeof(descriptor),
					&descriptor_len) == 0 &&
		    claim_storage_from_descriptor(i, descriptor, descriptor_len,
						  disk) == 0) {
			return 0;
		}
	}
	return -1;
}

int main(void)
{
	const char online[] = "usb-storage: online\n";
	const char ready[] = "usb-storage: ready\n";
	struct usb_storage_disk disk = { 0 };

	bunix_console_log(online, sizeof(online) - 1);
	bunix_console_log(ready, sizeof(ready) - 1);
	for (;;) {
		if (disk.bulk_in_handle == 0) {
			(void)scan_for_storage(&disk);
		}
		bunix_sleep_ns(100000000ull);
	}
}
