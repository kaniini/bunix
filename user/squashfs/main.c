#include <bunix/alloc.h>
#include <bunix/libbunix.h>

enum {
	SQUASHFS_HANDLE_NAMES = 3,
	SQUASHFS_MAGIC = 0x73717368,
	SQUASHFS_SUPER_SIZE = 96,
	SQUASHFS_MAJOR = 4,
	SQUASHFS_MAX_BLOCK_SIZE = 128 * 1024,
	SQUASHFS_METADATA_MAX = 8192,
	SQUASHFS_METADATA_HEADER = 2,
	SQUASHFS_METADATA_UNCOMPRESSED = 0x8000,
	SQUASHFS_METADATA_SIZE_MASK = 0x7fff,
	SQUASHFS_INVALID_TABLE = (u64)-1,
	SQUASHFS_COMPRESSOR_MIN = 1,
	SQUASHFS_COMPRESSOR_MAX = 6,
};

struct squashfs_super {
	u64 inodes;
	u64 block_size;
	u64 fragments;
	u64 compression;
	u64 block_log;
	u64 flags;
	u64 ids;
	u64 major;
	u64 minor;
	u64 root_inode;
	u64 bytes_used;
	u64 id_table_start;
	u64 xattr_id_table_start;
	u64 inode_table_start;
	u64 directory_table_start;
	u64 fragment_table_start;
	u64 lookup_table_start;
};

struct squashfs_metadata_cache {
	struct squashfs_metadata_cache *next;
	u64 disk_offset;
	u64 size;
	unsigned char data[SQUASHFS_METADATA_MAX];
};

static struct squashfs_super root_super;
static struct squashfs_metadata_cache *metadata_cache;
static u64 block_service;
static u64 image_size;

static u64 load_le16(const unsigned char *buffer, u64 offset)
{
	return ((u64)buffer[offset]) | ((u64)buffer[offset + 1] << 8);
}

static u64 load_le32(const unsigned char *buffer, u64 offset)
{
	return ((u64)buffer[offset]) |
	       ((u64)buffer[offset + 1] << 8) |
	       ((u64)buffer[offset + 2] << 16) |
	       ((u64)buffer[offset + 3] << 24);
}

static u64 load_le64(const unsigned char *buffer, u64 offset)
{
	return load_le32(buffer, offset) | (load_le32(buffer, offset + 4) << 32);
}

static u64 text_len(const char *text)
{
	u64 len = 0;

	while (text != 0 && text[len] != 0) {
		len++;
	}
	return len;
}

static void append_char(char *line, u64 size, u64 *offset, char c)
{
	if (*offset + 1 >= size) {
		return;
	}
	line[*offset] = c;
	*offset += 1;
	line[*offset] = 0;
}

static void append_text(char *line, u64 size, u64 *offset, const char *text)
{
	for (u64 i = 0; text != 0 && text[i] != 0; i++) {
		append_char(line, size, offset, text[i]);
	}
}

static void append_u64_dec(char *line, u64 size, u64 *offset, u64 value)
{
	char tmp[20];
	u64 count = 0;

	if (value == 0) {
		append_char(line, size, offset, '0');
		return;
	}
	while (value != 0 && count < sizeof(tmp)) {
		tmp[count++] = (char)('0' + (value % 10));
		value /= 10;
	}
	while (count != 0) {
		append_char(line, size, offset, tmp[--count]);
	}
}

static void append_u64_hex(char *line, u64 size, u64 *offset, u64 value)
{
	static const char digits[] = "0123456789abcdef";
	int started = 0;

	append_text(line, size, offset, "0x");
	for (int shift = 60; shift >= 0; shift -= 4) {
		const u64 nibble = (value >> shift) & 0xf;

		if (nibble != 0 || started || shift == 0) {
			append_char(line, size, offset, digits[nibble]);
			started = 1;
		}
	}
}

static void log_text(const char *text)
{
	(void)bunix_console_log(text, text_len(text));
}

static void log_mount_error(const char *reason, u64 value)
{
	char line[144];
	u64 offset = 0;

	line[0] = 0;
	append_text(line, sizeof(line), &offset, "squashfs: mount rejected: ");
	append_text(line, sizeof(line), &offset, reason);
	append_text(line, sizeof(line), &offset, " ");
	append_u64_hex(line, sizeof(line), &offset, value);
	append_char(line, sizeof(line), &offset, '\n');
	(void)bunix_console_log(line, offset);
}

static void log_super(const struct squashfs_super *super)
{
	char line[192];
	u64 offset = 0;

	line[0] = 0;
	append_text(line, sizeof(line), &offset,
		    "squashfs: super ok block_size=");
	append_u64_dec(line, sizeof(line), &offset, super->block_size);
	append_text(line, sizeof(line), &offset, " bytes_used=");
	append_u64_dec(line, sizeof(line), &offset, super->bytes_used);
	append_text(line, sizeof(line), &offset, " inodes=");
	append_u64_dec(line, sizeof(line), &offset, super->inodes);
	append_text(line, sizeof(line), &offset, " ids=");
	append_u64_dec(line, sizeof(line), &offset, super->ids);
	append_char(line, sizeof(line), &offset, '\n');
	(void)bunix_console_log(line, offset);
}

static u64 resolve_service(u64 service, unsigned int rights)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_NAMES,
		.type = BUNIX_NAMES_WAIT,
		.words = { BUNIX_NAMES_ROOT, service, rights, 0 },
	};
	struct bunix_msg reply;

	if (bunix_ipc_call(SQUASHFS_HANDLE_NAMES, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return 0;
	}
	return reply.cap;
}

static long register_service(u64 service, u64 handle)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_NAMES,
		.type = BUNIX_NAMES_REGISTER,
		.cap_rights = BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP,
		.cap = handle,
		.words = { BUNIX_NAMES_ROOT, service, 0, 0 },
	};
	struct bunix_msg reply;

	if (bunix_ipc_call(SQUASHFS_HANDLE_NAMES, &request, &reply) != 0) {
		return -1;
	}
	return reply.words[0] == 0 ? 0 : -1;
}

static long block_get_size(u64 block, u64 *size)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_BLOCK,
		.type = BUNIX_BLOCK_GET_INFO,
	};
	struct bunix_msg reply;

	if (size == 0 ||
	    bunix_ipc_call(block, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return -1;
	}
	*size = reply.words[1];
	return 0;
}

static long block_read_bytes(u64 block, u64 offset, unsigned char *out, u64 len)
{
	const long buffer = bunix_buffer_create(len == 0 ? 1 : len);
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_BLOCK,
		.type = BUNIX_BLOCK_READ_BUFFER,
		.cap_rights = BUNIX_RIGHT_SEND,
		.words = { offset, len, 0, 0 },
	};
	struct bunix_msg reply;

	if (out == 0 || buffer < 0) {
		return -1;
	}
	request.cap = (u64)buffer;
	if (bunix_ipc_call(block, &request, &reply) != 0 ||
	    reply.words[0] != 0 ||
	    reply.words[1] != len ||
	    bunix_buffer_read((u64)buffer, 0, out, len) != 0) {
		bunix_handle_close((u64)buffer);
		return -1;
	}
	bunix_handle_close((u64)buffer);
	return 0;
}

static int is_power_of_two(u64 value)
{
	return value != 0 && (value & (value - 1)) == 0;
}

static long validate_table_start(const char *name, u64 start,
				 int required)
{
	if (start == SQUASHFS_INVALID_TABLE) {
		if (required) {
			log_mount_error(name, start);
			return -1;
		}
		return 0;
	}
	if (start < SQUASHFS_SUPER_SIZE || start >= root_super.bytes_used) {
		log_mount_error(name, start);
		return -1;
	}
	return 0;
}

static struct squashfs_metadata_cache *metadata_cache_find(u64 disk_offset)
{
	struct squashfs_metadata_cache *entry = metadata_cache;

	while (entry != 0) {
		if (entry->disk_offset == disk_offset) {
			return entry;
		}
		entry = entry->next;
	}
	return 0;
}

static long metadata_read_block(u64 disk_offset,
				struct squashfs_metadata_cache **out)
{
	unsigned char header_raw[SQUASHFS_METADATA_HEADER];
	struct squashfs_metadata_cache *entry;
	u64 header;
	u64 stored_size;
	u64 data_offset;

	if (out == 0 ||
	    disk_offset > root_super.bytes_used ||
	    SQUASHFS_METADATA_HEADER > root_super.bytes_used - disk_offset) {
		log_mount_error("metadata block out of range", disk_offset);
		return -1;
	}
	entry = metadata_cache_find(disk_offset);
	if (entry != 0) {
		*out = entry;
		return 0;
	}
	if (block_read_bytes(block_service, disk_offset, header_raw,
			     sizeof(header_raw)) != 0) {
		log_mount_error("metadata header read failed", disk_offset);
		return -1;
	}
	header = load_le16(header_raw, 0);
	stored_size = header & SQUASHFS_METADATA_SIZE_MASK;
	data_offset = disk_offset + SQUASHFS_METADATA_HEADER;
	if (stored_size == 0 || stored_size > SQUASHFS_METADATA_MAX ||
	    data_offset > root_super.bytes_used ||
	    stored_size > root_super.bytes_used - data_offset) {
		log_mount_error("bad metadata block size", stored_size);
		return -1;
	}
	if ((header & SQUASHFS_METADATA_UNCOMPRESSED) == 0) {
		log_mount_error("compressed metadata unsupported", disk_offset);
		return -1;
	}
	entry = (struct squashfs_metadata_cache *)
		bunix_calloc(1, sizeof(*entry));
	if (entry == 0) {
		log_text("squashfs: mount rejected: metadata cache alloc failed\n");
		return -1;
	}
	if (block_read_bytes(block_service, data_offset, entry->data,
			     stored_size) != 0) {
		bunix_free(entry);
		log_mount_error("metadata block read failed", disk_offset);
		return -1;
	}
	entry->disk_offset = disk_offset;
	entry->size = stored_size;
	entry->next = metadata_cache;
	metadata_cache = entry;
	*out = entry;
	return 0;
}

static long metadata_read_exact(u64 table_start, u64 table_offset,
				unsigned char *out, u64 len)
{
	u64 disk_offset = table_start;
	u64 skip = table_offset;
	u64 done = 0;

	if (out == 0) {
		return -1;
	}
	while (skip != 0 || done < len) {
		struct squashfs_metadata_cache *entry;
		u64 copy_offset;
		u64 copy_len;

		if (metadata_read_block(disk_offset, &entry) != 0) {
			return -1;
		}
		if (skip >= entry->size) {
			skip -= entry->size;
			disk_offset += SQUASHFS_METADATA_HEADER + entry->size;
			continue;
		}
		copy_offset = skip;
		copy_len = entry->size - copy_offset;
		skip = 0;
		if (copy_len > len - done) {
			copy_len = len - done;
		}
		for (u64 i = 0; i < copy_len; i++) {
			out[done + i] = entry->data[copy_offset + i];
		}
		done += copy_len;
		disk_offset += SQUASHFS_METADATA_HEADER + entry->size;
	}
	return 0;
}

static long validate_initial_metadata_blocks(void)
{
	unsigned char sample[16];

	if (metadata_read_exact(root_super.inode_table_start, 0, sample,
				sizeof(sample)) != 0) {
		return -1;
	}
	if (metadata_read_exact(root_super.directory_table_start, 0, sample,
				sizeof(sample)) != 0) {
		return -1;
	}
	return 0;
}

static long parse_super(const unsigned char *raw, u64 size,
			struct squashfs_super *super)
{
	u64 block_from_log;

	if (load_le32(raw, 0) != SQUASHFS_MAGIC) {
		log_mount_error("bad magic", load_le32(raw, 0));
		return -1;
	}

	super->inodes = load_le32(raw, 4);
	super->block_size = load_le32(raw, 12);
	super->fragments = load_le32(raw, 16);
	super->compression = load_le16(raw, 20);
	super->block_log = load_le16(raw, 22);
	super->flags = load_le16(raw, 24);
	super->ids = load_le16(raw, 26);
	super->major = load_le16(raw, 28);
	super->minor = load_le16(raw, 30);
	super->root_inode = load_le64(raw, 32);
	super->bytes_used = load_le64(raw, 40);
	super->id_table_start = load_le64(raw, 48);
	super->xattr_id_table_start = load_le64(raw, 56);
	super->inode_table_start = load_le64(raw, 64);
	super->directory_table_start = load_le64(raw, 72);
	super->fragment_table_start = load_le64(raw, 80);
	super->lookup_table_start = load_le64(raw, 88);

	if (super->major != SQUASHFS_MAJOR) {
		log_mount_error("unsupported major version", super->major);
		return -1;
	}
	if (super->inodes == 0) {
		log_mount_error("zero inode count", 0);
		return -1;
	}
	if (super->ids == 0) {
		log_mount_error("zero id count", 0);
		return -1;
	}
	if (super->compression < SQUASHFS_COMPRESSOR_MIN ||
	    super->compression > SQUASHFS_COMPRESSOR_MAX) {
		log_mount_error("unsupported compression id",
				super->compression);
		return -1;
	}
	if (super->block_log >= 63) {
		log_mount_error("unsupported block log", super->block_log);
		return -1;
	}
	block_from_log = 1UL << super->block_log;
	if (super->block_size != block_from_log ||
	    !is_power_of_two(super->block_size) ||
	    super->block_size > SQUASHFS_MAX_BLOCK_SIZE) {
		log_mount_error("unsupported block size", super->block_size);
		return -1;
	}
	if (size < SQUASHFS_SUPER_SIZE ||
	    super->bytes_used < SQUASHFS_SUPER_SIZE ||
	    super->bytes_used > size) {
		log_mount_error("bad bytes_used", super->bytes_used);
		return -1;
	}
	if (super->fragments != 0) {
		log_mount_error("fragments unsupported", super->fragments);
		return -1;
	}
	if (super->root_inode == 0) {
		log_mount_error("missing root inode", 0);
		return -1;
	}
	if (super->lookup_table_start != SQUASHFS_INVALID_TABLE) {
		log_mount_error("export table unsupported",
				super->lookup_table_start);
		return -1;
	}
	if (super->xattr_id_table_start != SQUASHFS_INVALID_TABLE) {
		log_mount_error("xattrs unsupported",
				super->xattr_id_table_start);
		return -1;
	}
	if (validate_table_start("bad inode table", super->inode_table_start,
				 1) != 0 ||
	    validate_table_start("bad directory table",
				 super->directory_table_start, 1) != 0 ||
	    validate_table_start("bad id table", super->id_table_start, 1) != 0 ||
	    validate_table_start("bad fragment table",
				 super->fragment_table_start, 0) != 0) {
		return -1;
	}
	return 0;
}

static long squashfs_mount(void)
{
	unsigned char raw[SQUASHFS_SUPER_SIZE];

	if (block_get_size(block_service, &image_size) != 0) {
		log_text("squashfs: mount rejected: block info failed\n");
		return -1;
	}
	if (image_size < SQUASHFS_SUPER_SIZE) {
		log_mount_error("image too small", image_size);
		return -1;
	}
	if (block_read_bytes(block_service, 0, raw, sizeof(raw)) != 0) {
		log_text("squashfs: mount rejected: superblock read failed\n");
		return -1;
	}
	if (parse_super(raw, image_size, &root_super) != 0) {
		return -1;
	}
	if (validate_initial_metadata_blocks() != 0) {
		return -1;
	}
	log_super(&root_super);
	return 0;
}

int main(void)
{
	const char online[] = "squashfs: online\n";
	struct bunix_msg message;

	bunix_console_log(online, sizeof(online) - 1);
	block_service = resolve_service(BUNIX_SERVICE_BLOCK, BUNIX_RIGHT_SEND);
	if (block_service == 0 ||
	    squashfs_mount() != 0 ||
	    register_service(BUNIX_SERVICE_SQUASHFS, BUNIX_HANDLE_SELF) != 0) {
		return 1;
	}

	for (;;) {
		struct bunix_msg reply = {
			.protocol = BUNIX_PROTO_VFS,
			.type = 0,
			.words = { BUNIX_VFS_ERR_INVAL, 0, 0, 0 },
		};

		if (bunix_ipc_recv(BUNIX_HANDLE_SELF, &message) != 0) {
			continue;
		}
		reply.type = message.type;
		if (message.cap != 0) {
			bunix_handle_close(message.cap);
		}
		if (message.protocol == BUNIX_PROTO_VFS &&
		    message.reply != 0) {
			bunix_ipc_send(message.reply, &reply);
		}
	}
}
