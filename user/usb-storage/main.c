#include <bunix/syscall.h>

#define STORAGE_HANDLE_USB (bunix_handle_find(BUNIX_CAP_USB))

enum {
	STORAGE_MAX_DESCRIPTOR = 512,
	STORAGE_CLASS_MASS = 8,
	STORAGE_SUBCLASS_SCSI = 6,
	STORAGE_PROTOCOL_BULK_ONLY = 80,
	USB_ENDPOINT_ATTR_BULK = 2,
	STORAGE_SECTOR_SIZE = 512,
	STORAGE_BUFFER_MAX = 128 * 1024,
	STORAGE_CBW_SIZE = 31,
	STORAGE_CSW_SIZE = 13,
	STORAGE_CBW_SIGNATURE = 0x43425355,
	STORAGE_CSW_SIGNATURE = 0x53425355,
	USB_REQ_CLEAR_FEATURE = 1,
	USB_RECIP_ENDPOINT = 2,
	USB_TYPE_CLASS = 0x20,
	USB_RECIP_INTERFACE = 1,
	USB_ENDPOINT_HALT = 0,
	BOT_REQUEST_RESET = 0xff,
	SCSI_TEST_UNIT_READY = 0x00,
	SCSI_REQUEST_SENSE = 0x03,
	SCSI_INQUIRY = 0x12,
	SCSI_READ_CAPACITY_10 = 0x25,
	SCSI_READ_10 = 0x28,
	SCSI_WRITE_10 = 0x2a,
};

struct usb_storage_disk {
	u64 device_index;
	u64 interface_number;
	u64 bulk_in_address;
	u64 bulk_out_address;
	u64 bulk_in_handle;
	u64 bulk_out_handle;
	u64 block_size;
	u64 block_count;
	u64 ready;
	u64 block_registered;
	u64 next_tag;
};

static unsigned char storage_buffer[STORAGE_BUFFER_MAX];
static unsigned char scratch_buffer[STORAGE_BUFFER_MAX];

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

static void put_le32(unsigned char *out, u64 value)
{
	out[0] = (unsigned char)(value & 0xffu);
	out[1] = (unsigned char)((value >> 8) & 0xffu);
	out[2] = (unsigned char)((value >> 16) & 0xffu);
	out[3] = (unsigned char)((value >> 24) & 0xffu);
}

static u64 get_le32(const unsigned char *in)
{
	return (u64)in[0] | ((u64)in[1] << 8) | ((u64)in[2] << 16) |
	       ((u64)in[3] << 24);
}

static u64 get_be32(const unsigned char *in)
{
	return ((u64)in[0] << 24) | ((u64)in[1] << 16) |
	       ((u64)in[2] << 8) | (u64)in[3];
}

static void put_be32(unsigned char *out, u64 value)
{
	out[0] = (unsigned char)((value >> 24) & 0xffu);
	out[1] = (unsigned char)((value >> 16) & 0xffu);
	out[2] = (unsigned char)((value >> 8) & 0xffu);
	out[3] = (unsigned char)(value & 0xffu);
}

static void put_be16(unsigned char *out, u64 value)
{
	out[0] = (unsigned char)((value >> 8) & 0xffu);
	out[1] = (unsigned char)(value & 0xffu);
}

static void bytes_copy(unsigned char *out, const unsigned char *in, u64 len)
{
	for (u64 i = 0; i < len; i++) {
		out[i] = in[i];
	}
}

static void bytes_zero(unsigned char *out, u64 len)
{
	for (u64 i = 0; i < len; i++) {
		out[i] = 0;
	}
}

static void pack_bytes(u64 *words, const unsigned char *data, u64 len)
{
	words[0] = 0;
	words[1] = 0;
	for (u64 i = 0; i < len && i < BUNIX_IPC_DATA_BYTES; i++) {
		const u64 slot = i / 8;
		const u64 shift = (i % 8) * 8;

		words[slot] |= ((u64)data[i]) << shift;
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

static void log_capacity(const struct usb_storage_disk *disk)
{
	char line[160];
	u64 offset = 0;

	line[0] = '\0';
	append_text(line, sizeof(line), &offset,
		    "usb-storage: capacity blocks=");
	append_u64(line, sizeof(line), &offset, disk->block_count);
	append_text(line, sizeof(line), &offset, " block_size=");
	append_u64(line, sizeof(line), &offset, disk->block_size);
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

static int usb_control_no_data(u64 device_index, u64 interface_number,
			       u64 request_type, u64 request, u64 value,
			       u64 index)
{
	struct bunix_msg message = {
		.protocol = BUNIX_PROTO_USB,
		.type = BUNIX_USB_CONTROL_NO_DATA,
		.words = { device_index, interface_number,
			   request_type | (request << 8) | (value << 16),
			   index },
	};
	struct bunix_msg reply;

	return bunix_ipc_call(STORAGE_HANDLE_USB, &message, &reply) == 0 &&
		       reply.protocol == BUNIX_PROTO_USB &&
		       reply.words[0] == BUNIX_USB_OK ?
		       0 :
		       -1;
}

static int bot_recover(struct usb_storage_disk *disk)
{
	if (disk == 0) {
		return -1;
	}
	if (usb_control_no_data(disk->device_index, disk->interface_number,
				USB_TYPE_CLASS | USB_RECIP_INTERFACE,
				BOT_REQUEST_RESET, 0,
				disk->interface_number) != 0 ||
	    usb_control_no_data(disk->device_index, disk->interface_number,
				USB_RECIP_ENDPOINT, USB_REQ_CLEAR_FEATURE,
				USB_ENDPOINT_HALT, disk->bulk_in_address) != 0 ||
	    usb_control_no_data(disk->device_index, disk->interface_number,
				USB_RECIP_ENDPOINT, USB_REQ_CLEAR_FEATURE,
				USB_ENDPOINT_HALT, disk->bulk_out_address) != 0) {
		return -1;
	}
	bunix_console_log("usb-storage: recovery ok\n", 25);
	return 0;
}

static int endpoint_transfer(u64 endpoint, unsigned char *data, u64 len,
			     u64 direction_in, u64 *actual)
{
	long buffer;
	struct bunix_msg submit = {
		.protocol = BUNIX_PROTO_USB,
		.type = BUNIX_USB_SUBMIT_TRANSFER,
		.cap_rights = BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP,
		.words = { len, 0, 0, 0 },
	};
	struct bunix_msg wait = {
		.protocol = BUNIX_PROTO_USB,
		.type = BUNIX_USB_WAIT_COMPLETION,
	};
	struct bunix_msg reply;

	if (endpoint == 0 || data == 0 || len == 0 ||
	    len > BUNIX_USB_TRANSFER_MAX) {
		return -1;
	}
	buffer = bunix_buffer_create(len);
	if (buffer <= 0) {
		return -1;
	}
	if (direction_in == 0 &&
	    bunix_buffer_write((u64)buffer, 0, data, len) != 0) {
		bunix_handle_close((u64)buffer);
		return -1;
	}
	submit.cap = (u64)buffer;
	if (bunix_ipc_call(endpoint, &submit, &reply) != 0 ||
	    reply.words[0] != BUNIX_USB_OK ||
	    bunix_ipc_call(endpoint, &wait, &reply) != 0 ||
	    reply.words[0] != BUNIX_USB_OK ||
	    reply.words[1] > len) {
		bunix_handle_close((u64)buffer);
		return -1;
	}
	if (direction_in != 0 &&
	    bunix_buffer_read((u64)buffer, 0, data, reply.words[1]) != 0) {
		bunix_handle_close((u64)buffer);
		return -1;
	}
	if (actual != 0) {
		*actual = reply.words[1];
	}
	bunix_handle_close((u64)buffer);
	return 0;
}

static int bot_command_once(struct usb_storage_disk *disk,
			    const unsigned char *cdb, u64 cdb_len,
			    unsigned char *data, u64 data_len,
			    u64 direction_in)
{
	unsigned char cbw[STORAGE_CBW_SIZE];
	unsigned char csw[STORAGE_CSW_SIZE];
	u64 actual = 0;
	u64 residue;

	if (disk == 0 || cdb == 0 || cdb_len == 0 || cdb_len > 16 ||
	    data_len > STORAGE_BUFFER_MAX) {
		return -1;
	}
	bytes_zero(cbw, sizeof(cbw));
	put_le32(&cbw[0], STORAGE_CBW_SIGNATURE);
	put_le32(&cbw[4], disk->next_tag++);
	put_le32(&cbw[8], data_len);
	cbw[12] = direction_in != 0 ? 0x80 : 0x00;
	cbw[13] = 0;
	cbw[14] = (unsigned char)cdb_len;
	bytes_copy(&cbw[15], cdb, cdb_len);
	if (endpoint_transfer(disk->bulk_out_handle, cbw, sizeof(cbw), 0,
			      &actual) != 0 ||
	    actual != sizeof(cbw)) {
		return -1;
	}
	if (data_len != 0) {
		const u64 endpoint = direction_in != 0 ? disk->bulk_in_handle :
							  disk->bulk_out_handle;

		if (endpoint_transfer(endpoint, data, data_len, direction_in,
				      &actual) != 0) {
			return -1;
		}
		if (direction_in == 0 && actual != data_len) {
			return -1;
		}
	}
	bytes_zero(csw, sizeof(csw));
	if (endpoint_transfer(disk->bulk_in_handle, csw, sizeof(csw), 1,
			      &actual) != 0 ||
	    actual != sizeof(csw) || get_le32(&csw[0]) != STORAGE_CSW_SIGNATURE ||
	    get_le32(&csw[4]) + 1 != disk->next_tag || csw[12] != 0) {
		return -1;
	}
	residue = get_le32(&csw[8]);
	if (residue > data_len) {
		return -1;
	}
	return 0;
}

static int bot_command(struct usb_storage_disk *disk, const unsigned char *cdb,
		       u64 cdb_len, unsigned char *data, u64 data_len,
		       u64 direction_in)
{
	if (bot_command_once(disk, cdb, cdb_len, data, data_len,
			     direction_in) == 0) {
		return 0;
	}
	if (bot_recover(disk) != 0) {
		return -1;
	}
	if (bot_command_once(disk, cdb, cdb_len, data, data_len,
			     direction_in) == 0) {
		return 0;
	}
	(void)bot_recover(disk);
	return -1;
}

static int scsi_inquiry(struct usb_storage_disk *disk)
{
	unsigned char cdb[6];
	unsigned char data[36];

	bytes_zero(cdb, sizeof(cdb));
	bytes_zero(data, sizeof(data));
	cdb[0] = SCSI_INQUIRY;
	cdb[4] = sizeof(data);
	return bot_command(disk, cdb, sizeof(cdb), data, sizeof(data), 1);
}

static int scsi_read_capacity(struct usb_storage_disk *disk)
{
	unsigned char cdb[10];
	unsigned char data[8];
	u64 last_lba;
	u64 block_size;

	bytes_zero(cdb, sizeof(cdb));
	bytes_zero(data, sizeof(data));
	cdb[0] = SCSI_READ_CAPACITY_10;
	if (bot_command(disk, cdb, sizeof(cdb), data, sizeof(data), 1) != 0) {
		return -1;
	}
	last_lba = get_be32(&data[0]);
	block_size = get_be32(&data[4]);
	if (block_size == 0 || block_size > STORAGE_SECTOR_SIZE) {
		return -1;
	}
	disk->block_count = last_lba + 1;
	disk->block_size = block_size;
	log_capacity(disk);
	return 0;
}

static int scsi_read_blocks(struct usb_storage_disk *disk, u64 lba,
			    u64 blocks, unsigned char *out)
{
	unsigned char cdb[10];
	u64 len;

	if (disk == 0 || out == 0 || blocks == 0 ||
	    disk->block_size == 0) {
		return -1;
	}
	len = blocks * disk->block_size;
	if (len > STORAGE_BUFFER_MAX) {
		return -1;
	}
	bytes_zero(cdb, sizeof(cdb));
	cdb[0] = SCSI_READ_10;
	put_be32(&cdb[2], lba);
	put_be16(&cdb[7], blocks);
	return bot_command(disk, cdb, sizeof(cdb), out, len, 1);
}

static int scsi_write_blocks(struct usb_storage_disk *disk, u64 lba,
			     u64 blocks, unsigned char *data)
{
	unsigned char cdb[10];
	u64 len;

	if (disk == 0 || data == 0 || blocks == 0 ||
	    disk->block_size == 0) {
		return -1;
	}
	len = blocks * disk->block_size;
	if (len > STORAGE_BUFFER_MAX) {
		return -1;
	}
	bytes_zero(cdb, sizeof(cdb));
	cdb[0] = SCSI_WRITE_10;
	put_be32(&cdb[2], lba);
	put_be16(&cdb[7], blocks);
	return bot_command(disk, cdb, sizeof(cdb), data, len, 0);
}

static int storage_smoke(struct usb_storage_disk *disk)
{
	if (disk == 0 || disk->block_size == 0 ||
	    disk->block_size > STORAGE_BUFFER_MAX) {
		return -1;
	}
	if (scsi_read_blocks(disk, 0, 1, storage_buffer) != 0) {
		return -1;
	}
	bytes_copy(scratch_buffer, storage_buffer, disk->block_size);
	storage_buffer[0] ^= 0xa5u;
	if (scsi_write_blocks(disk, 0, 1, storage_buffer) != 0 ||
	    scsi_read_blocks(disk, 0, 1, storage_buffer) != 0 ||
	    storage_buffer[0] != (unsigned char)(scratch_buffer[0] ^ 0xa5u) ||
	    scsi_write_blocks(disk, 0, 1, scratch_buffer) != 0) {
		return -1;
	}
	if (bot_recover(disk) != 0 || scsi_inquiry(disk) != 0) {
		return -1;
	}
	bunix_console_log("usb-storage: smoke ok\n", 22);
	return 0;
}

static u64 storage_capacity_bytes(const struct usb_storage_disk *disk)
{
	return disk->block_count * disk->block_size;
}

static int storage_read_bytes(struct usb_storage_disk *disk, u64 offset,
			      unsigned char *out, u64 len)
{
	u64 done = 0;

	if (disk == 0 || out == 0 || disk->block_size == 0) {
		return -1;
	}
	while (done < len) {
		const u64 position = offset + done;
		const u64 lba = position / disk->block_size;
		const u64 block_offset = position % disk->block_size;
		u64 chunk = disk->block_size - block_offset;

		if (chunk > len - done) {
			chunk = len - done;
		}
		if (scsi_read_blocks(disk, lba, 1, scratch_buffer) != 0) {
			return -1;
		}
		bytes_copy(out + done, scratch_buffer + block_offset, chunk);
		done += chunk;
	}
	return 0;
}

static int storage_write_bytes(struct usb_storage_disk *disk, u64 offset,
			       const unsigned char *in, u64 len)
{
	u64 done = 0;

	if (disk == 0 || in == 0 || disk->block_size == 0) {
		return -1;
	}
	while (done < len) {
		const u64 position = offset + done;
		const u64 lba = position / disk->block_size;
		const u64 block_offset = position % disk->block_size;
		u64 chunk = disk->block_size - block_offset;

		if (chunk > len - done) {
			chunk = len - done;
		}
		if ((block_offset != 0 || chunk != disk->block_size) &&
		    scsi_read_blocks(disk, lba, 1, scratch_buffer) != 0) {
			return -1;
		}
		if (block_offset == 0 && chunk == disk->block_size) {
			bytes_copy(scratch_buffer, in + done, chunk);
		} else {
			bytes_copy(scratch_buffer + block_offset, in + done,
				   chunk);
		}
		if (scsi_write_blocks(disk, lba, 1, scratch_buffer) != 0) {
			return -1;
		}
		done += chunk;
	}
	return 0;
}

static int initialize_storage(struct usb_storage_disk *disk)
{
	if (disk == 0 || disk->bulk_in_handle == 0 ||
	    disk->bulk_out_handle == 0) {
		return -1;
	}
	disk->next_tag = 1;
	if (scsi_inquiry(disk) != 0 || scsi_read_capacity(disk) != 0 ||
	    storage_smoke(disk) != 0) {
		return -1;
	}
	disk->ready = 1;
	return 0;
}

static void maybe_register_block(struct usb_storage_disk *disk)
{
	const char online[] = "usb-storage: block online\n";

	if (disk == 0 || disk->ready == 0 || disk->block_registered != 0) {
		return;
	}
	if (bunix_names_register_claim(bunix_handle_find(BUNIX_CAP_CLAM),
				       BUNIX_HANDLE_SELF) == 0) {
		disk->block_registered = 1;
		bunix_console_log(online, sizeof(online) - 1);
	}
}

static void handle_block_message(struct usb_storage_disk *disk)
{
	struct bunix_msg message;
	struct bunix_msg reply = {
		.protocol = BUNIX_PROTO_BLOCK,
		.type = 0,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { 0, 0, 0, 0 },
	};
	u64 capacity;

	if (disk == 0 || disk->block_registered == 0 ||
	    bunix_ipc_try_recv(BUNIX_HANDLE_SELF, &message) != 0 ||
	    message.protocol != BUNIX_PROTO_BLOCK) {
		return;
	}
	capacity = storage_capacity_bytes(disk);
	reply.type = message.type;
	switch (message.type) {
	case BUNIX_BLOCK_GET_INFO:
		reply.words[0] = 0;
		reply.words[1] = capacity;
		reply.words[2] = disk->block_size;
		break;
	case BUNIX_BLOCK_READ: {
		const u64 offset = message.words[0];
		u64 len = message.words[1];

		if (offset >= capacity) {
			len = 0;
		} else if (len > capacity - offset) {
			len = capacity - offset;
		}
		if (len > BUNIX_IPC_DATA_BYTES) {
			len = BUNIX_IPC_DATA_BYTES;
		}
		reply.words[0] = 0;
		reply.words[1] = len;
		if (len != 0 &&
		    storage_read_bytes(disk, offset, storage_buffer, len) != 0) {
			reply.words[0] = (u64)-1;
			reply.words[1] = 0;
		} else {
			pack_bytes(&reply.words[2], storage_buffer, len);
		}
		break;
	}
	case BUNIX_BLOCK_READ_BUFFER: {
		const u64 offset = message.words[0];
		const u64 buffer_offset = message.words[2];
		u64 len = message.words[1];

		if (message.cap == 0 ||
		    (message.cap_rights & BUNIX_RIGHT_SEND) == 0) {
			reply.words[0] = (u64)-1;
			break;
		}
		if (offset >= capacity) {
			len = 0;
		} else if (len > capacity - offset) {
			len = capacity - offset;
		}
		if (len > sizeof(storage_buffer)) {
			len = sizeof(storage_buffer);
		}
		reply.words[0] = 0;
		reply.words[1] = len;
		if (len != 0 &&
		    (storage_read_bytes(disk, offset, storage_buffer, len) !=
			     0 ||
		     bunix_buffer_write(message.cap, buffer_offset,
					storage_buffer, len) != 0)) {
			reply.words[0] = (u64)-1;
			reply.words[1] = 0;
		}
		break;
	}
	case BUNIX_BLOCK_WRITE_BUFFER: {
		const u64 offset = message.words[0];
		const u64 buffer_offset = message.words[2];
		u64 len = message.words[1];

		if (message.cap == 0 ||
		    (message.cap_rights & BUNIX_RIGHT_RECV) == 0 ||
		    offset > capacity || len > capacity - offset) {
			reply.words[0] = (u64)-1;
			break;
		}
		if (len > sizeof(storage_buffer)) {
			len = sizeof(storage_buffer);
		}
		reply.words[0] = 0;
		reply.words[1] = len;
		if (len != 0 &&
		    (bunix_buffer_read(message.cap, buffer_offset,
				       storage_buffer, len) != 0 ||
		     storage_write_bytes(disk, offset, storage_buffer, len) !=
			     0)) {
			reply.words[0] = (u64)-1;
			reply.words[1] = 0;
		}
		break;
	}
	case BUNIX_BLOCK_FLUSH:
		reply.words[0] = 0;
		break;
	default:
		reply.words[0] = (u64)-1;
		break;
	}
	if (message.cap != 0) {
		bunix_handle_close(message.cap);
	}
	if (message.reply != 0) {
		bunix_ipc_send(message.reply, &reply);
	}
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
						  disk) == 0 &&
		    initialize_storage(disk) == 0) {
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
		} else if (disk.ready != 0) {
			maybe_register_block(&disk);
			handle_block_message(&disk);
		}
		bunix_sleep_ns(100000000ull);
	}
}
