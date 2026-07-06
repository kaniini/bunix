#include <bunix/libbunix.h>

enum {
	EXT2_HANDLE_NAMES = 3,
	EXT2_SUPER_OFFSET = 1024,
	EXT2_SUPER_SIZE = 1024,
	EXT2_MAGIC = 0xef53,
	EXT2_GOOD_OLD_REV = 0,
	EXT2_DYNAMIC_REV = 1,
	EXT2_GOOD_OLD_INODE_SIZE = 128,
	EXT2_MIN_BLOCK_SIZE = 1024,
	EXT2_MAX_BLOCK_SIZE = 4096,
	EXT2_FEATURE_COMPAT_EXT_ATTR = 0x0008,
	EXT2_FEATURE_COMPAT_RESIZE_INODE = 0x0010,
	EXT2_FEATURE_COMPAT_DIR_INDEX = 0x0020,
	EXT2_FEATURE_INCOMPAT_FILETYPE = 0x0002,
	EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER = 0x0001,
	EXT2_FEATURE_RO_COMPAT_LARGE_FILE = 0x0002,
	EXT2_COMPAT_SUPPORTED = EXT2_FEATURE_COMPAT_EXT_ATTR |
				 EXT2_FEATURE_COMPAT_RESIZE_INODE |
				 EXT2_FEATURE_COMPAT_DIR_INDEX,
	EXT2_RO_COMPAT_SUPPORTED = EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER |
				    EXT2_FEATURE_RO_COMPAT_LARGE_FILE,
	EXT2_INCOMPAT_SUPPORTED = EXT2_FEATURE_INCOMPAT_FILETYPE,
};

struct ext2_super {
	u64 inodes_count;
	u64 blocks_count;
	u64 first_data_block;
	u64 log_block_size;
	u64 blocks_per_group;
	u64 inodes_per_group;
	u64 rev_level;
	u64 first_ino;
	u64 inode_size;
	u64 feature_compat;
	u64 feature_incompat;
	u64 feature_ro_compat;
	u64 block_size;
	u64 groups_count;
};

static u64 text_len(const char *text)
{
	u64 len = 0;

	while (text != 0 && text[len] != 0) {
		len++;
	}
	return len;
}

static void log_text(const char *text)
{
	(void)bunix_console_log(text, text_len(text));
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

static void log_mount_error(const char *reason, u64 value)
{
	char line[120];
	u64 offset = 0;

	line[0] = 0;
	append_text(line, sizeof(line), &offset, "ext2: mount rejected: ");
	append_text(line, sizeof(line), &offset, reason);
	append_text(line, sizeof(line), &offset, " ");
	append_u64_hex(line, sizeof(line), &offset, value);
	append_char(line, sizeof(line), &offset, '\n');
	(void)bunix_console_log(line, offset);
}

static void log_super(const struct ext2_super *super)
{
	char line[160];
	u64 offset = 0;

	line[0] = 0;
	append_text(line, sizeof(line), &offset, "ext2: super ok block_size=");
	append_u64_dec(line, sizeof(line), &offset, super->block_size);
	append_text(line, sizeof(line), &offset, " blocks=");
	append_u64_dec(line, sizeof(line), &offset, super->blocks_count);
	append_text(line, sizeof(line), &offset, " inodes=");
	append_u64_dec(line, sizeof(line), &offset, super->inodes_count);
	append_text(line, sizeof(line), &offset, " groups=");
	append_u64_dec(line, sizeof(line), &offset, super->groups_count);
	append_char(line, sizeof(line), &offset, '\n');
	(void)bunix_console_log(line, offset);
}

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

static u64 div_round_up(u64 value, u64 divisor)
{
	return divisor == 0 ? 0 : (value + divisor - 1) / divisor;
}

static u64 resolve_service(u64 service, unsigned int rights)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_NAMES,
		.type = BUNIX_NAMES_WAIT,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { BUNIX_NAMES_ROOT, service, rights, 0 },
	};
	struct bunix_msg reply;

	if (bunix_ipc_call(EXT2_HANDLE_NAMES, &request, &reply) != 0 ||
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
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP,
		.reply = 0,
		.cap = handle,
		.words = { BUNIX_NAMES_ROOT, service, 0, 0 },
	};
	struct bunix_msg reply;

	if (bunix_ipc_call(EXT2_HANDLE_NAMES, &request, &reply) != 0) {
		return -1;
	}
	return reply.words[0] == 0 ? 0 : -1;
}

static long block_get_size(u64 block, u64 *size)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_BLOCK,
		.type = BUNIX_BLOCK_GET_INFO,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { 0, 0, 0, 0 },
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
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_SEND,
		.reply = 0,
		.cap = 0,
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

static long parse_super(const unsigned char *raw, u64 image_size,
			struct ext2_super *super)
{
	u64 block_size;
	u64 fs_bytes;
	u64 unsupported;

	super->inodes_count = load_le32(raw, 0);
	super->blocks_count = load_le32(raw, 4);
	super->first_data_block = load_le32(raw, 20);
	super->log_block_size = load_le32(raw, 24);
	super->blocks_per_group = load_le32(raw, 32);
	super->inodes_per_group = load_le32(raw, 40);
	super->rev_level = load_le32(raw, 76);
	super->feature_compat = load_le32(raw, 92);
	super->feature_incompat = load_le32(raw, 96);
	super->feature_ro_compat = load_le32(raw, 100);

	if (load_le16(raw, 56) != EXT2_MAGIC) {
		log_mount_error("bad magic", load_le16(raw, 56));
		return -1;
	}
	if (super->rev_level != EXT2_GOOD_OLD_REV &&
	    super->rev_level != EXT2_DYNAMIC_REV) {
		log_mount_error("unsupported revision", super->rev_level);
		return -1;
	}
	if (super->log_block_size > 2) {
		log_mount_error("unsupported block size log",
				super->log_block_size);
		return -1;
	}
	block_size = EXT2_MIN_BLOCK_SIZE << super->log_block_size;
	if (block_size < EXT2_MIN_BLOCK_SIZE ||
	    block_size > EXT2_MAX_BLOCK_SIZE) {
		log_mount_error("unsupported block size", block_size);
		return -1;
	}
	super->block_size = block_size;
	if (super->blocks_count == 0 ||
	    super->inodes_count == 0 ||
	    super->blocks_per_group == 0 ||
	    super->inodes_per_group == 0) {
		log_mount_error("zero geometry field", 0);
		return -1;
	}
	if (super->first_data_block > super->blocks_count ||
	    (block_size == 1024 && super->first_data_block != 1) ||
	    (block_size != 1024 && super->first_data_block != 0)) {
		log_mount_error("bad first data block", super->first_data_block);
		return -1;
	}
	if (super->blocks_count > ((u64)-1) / block_size) {
		log_mount_error("image size overflow", super->blocks_count);
		return -1;
	}
	fs_bytes = super->blocks_count * block_size;
	if (fs_bytes > image_size ||
	    image_size < EXT2_SUPER_OFFSET + EXT2_SUPER_SIZE) {
		log_mount_error("image shorter than filesystem", fs_bytes);
		return -1;
	}
	super->groups_count =
		div_round_up(super->blocks_count - super->first_data_block,
			     super->blocks_per_group);
	if (super->groups_count == 0) {
		log_mount_error("zero group count", 0);
		return -1;
	}
	if (super->rev_level == EXT2_GOOD_OLD_REV) {
		super->first_ino = 11;
		super->inode_size = EXT2_GOOD_OLD_INODE_SIZE;
	} else {
		super->first_ino = load_le32(raw, 84);
		super->inode_size = load_le16(raw, 88);
		if (super->inode_size < EXT2_GOOD_OLD_INODE_SIZE ||
		    super->inode_size > block_size ||
		    (super->inode_size & (super->inode_size - 1)) != 0) {
			log_mount_error("bad inode size", super->inode_size);
			return -1;
		}
	}
	unsupported = super->feature_compat & ~EXT2_COMPAT_SUPPORTED;
	if (unsupported != 0) {
		log_mount_error("unsupported compat features", unsupported);
		return -1;
	}
	unsupported = super->feature_incompat & ~EXT2_INCOMPAT_SUPPORTED;
	if (unsupported != 0) {
		log_mount_error("unsupported incompat features", unsupported);
		return -1;
	}
	unsupported = super->feature_ro_compat & ~EXT2_RO_COMPAT_SUPPORTED;
	if (unsupported != 0) {
		log_mount_error("unsupported ro-compat features", unsupported);
		return -1;
	}
	return 0;
}

static long load_initial_super(u64 block)
{
	unsigned char raw[EXT2_SUPER_SIZE];
	struct ext2_super super;
	u64 image_size = 0;

	if (block_get_size(block, &image_size) != 0 ||
	    block_read_bytes(block, EXT2_SUPER_OFFSET, raw, sizeof(raw)) != 0 ||
	    parse_super(raw, image_size, &super) != 0) {
		return -1;
	}
	log_super(&super);
	return 0;
}

int main(void)
{
	struct bunix_msg message;
	const char online[] = "ext2: online\n";
	u64 block;

	bunix_console_log(online, sizeof(online) - 1);
	block = resolve_service(BUNIX_SERVICE_BLOCK, BUNIX_RIGHT_SEND);
	if (block == 0) {
		log_text("ext2: block service unavailable\n");
	} else {
		(void)load_initial_super(block);
	}
	if (register_service(BUNIX_SERVICE_EXT2, BUNIX_HANDLE_SELF) != 0) {
		return 1;
	}
	for (;;) {
		struct bunix_msg reply = {
			.protocol = BUNIX_PROTO_EXT2,
			.type = 0,
			.sender = 0,
			.cap_rights = 0,
			.reply = 0,
			.cap = 0,
			.words = { 0, 0, 0, 0 },
		};

		if (bunix_ipc_recv(BUNIX_HANDLE_SELF, &message) != 0 ||
		    message.protocol != BUNIX_PROTO_EXT2) {
			continue;
		}
		reply.type = message.type;
		reply.words[0] = (u64)-1;
		if (message.cap != 0) {
			bunix_handle_close(message.cap);
		}
		if (message.reply != 0) {
			bunix_ipc_send(message.reply, &reply);
		}
	}
}
