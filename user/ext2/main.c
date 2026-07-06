#include <bunix/alloc.h>
#include <bunix/libbunix.h>
#include <bunix/tree.h>

enum {
	EXT2_HANDLE_NAMES = 3,
	EXT2_SUPER_OFFSET = 1024,
	EXT2_SUPER_SIZE = 1024,
	EXT2_GROUP_DESC_SIZE = 32,
	EXT2_INODE_MIN_SIZE = 128,
	EXT2_ROOT_INO = 2,
	EXT2_BLOCK_READ_MAX = 128 * 1024,
	EXT2_NAME_MAX = 255,
	EXT2_MAX_PATH = 4096,
	EXT2_NDIR_BLOCKS = 12,
	EXT2_IND_BLOCK = 12,
	EXT2_DIND_BLOCK = 13,
	EXT2_TIND_BLOCK = 14,
	EXT2_S_IFMT = 0170000,
	EXT2_S_IFIFO = 0010000,
	EXT2_S_IFCHR = 0020000,
	EXT2_S_IFDIR = 0040000,
	EXT2_S_IFREG = 0100000,
	EXT2_S_IFLNK = 0120000,
	EXT2_FT_UNKNOWN = 0,
	EXT2_FT_REG_FILE = 1,
	EXT2_FT_DIR = 2,
	EXT2_FT_CHRDEV = 3,
	EXT2_FT_FIFO = 5,
	EXT2_FT_SYMLINK = 7,
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
	u64 free_blocks_count;
	u64 free_inodes_count;
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

struct ext2_group {
	u64 block_bitmap;
	u64 inode_bitmap;
	u64 inode_table;
	u64 free_blocks_count;
	u64 free_inodes_count;
	u64 used_dirs_count;
};

struct ext2_inode {
	u64 mode;
	u64 uid;
	u64 size;
	u64 atime;
	u64 ctime;
	u64 mtime;
	u64 gid;
	u64 links_count;
	u64 blocks;
	u64 block[15];
	unsigned char block_bytes[60];
};

struct ext2_dirent {
	u64 ino;
	u64 rec_len;
	u64 name_len;
	u64 file_type;
	u64 offset;
	char name[EXT2_NAME_MAX + 1];
};

struct ext2_mount {
	u64 block;
	u64 image_size;
	u64 use_boot_data;
	unsigned char *image_data;
	struct ext2_super super;
	struct ext2_group *groups;
};

struct ext2_open {
	struct bunix_u64_tree_node node;
	u64 handle;
	u64 ino;
	struct ext2_inode inode;
};

static struct ext2_mount root_mount;
static int root_mount_ready;
static struct bunix_u64_tree open_files;
static u64 next_open_handle = 1;

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

static void store_le16(unsigned char *buffer, u64 offset, u64 value)
{
	buffer[offset] = (unsigned char)(value & 0xff);
	buffer[offset + 1] = (unsigned char)((value >> 8) & 0xff);
}

static void store_le32(unsigned char *buffer, u64 offset, u64 value)
{
	buffer[offset] = (unsigned char)(value & 0xff);
	buffer[offset + 1] = (unsigned char)((value >> 8) & 0xff);
	buffer[offset + 2] = (unsigned char)((value >> 16) & 0xff);
	buffer[offset + 3] = (unsigned char)((value >> 24) & 0xff);
}

static int text_eq_len(const char *text, const char *name, u64 name_len)
{
	u64 i;

	if (text == 0 || name == 0) {
		return 0;
	}
	for (i = 0; i < name_len; i++) {
		if (text[i] == 0 || text[i] != name[i]) {
			return 0;
		}
	}
	return text[i] == 0;
}

static int text_eq(const char *left, const char *right)
{
	u64 i = 0;

	while (left != 0 && right != 0 && left[i] != 0 && right[i] != 0) {
		if (left[i] != right[i]) {
			return 0;
		}
		i++;
	}
	return left != 0 && right != 0 && left[i] == right[i];
}

static int text_starts_with(const char *text, const char *prefix)
{
	u64 i = 0;

	if (text == 0 || prefix == 0) {
		return 0;
	}
	while (prefix[i] != 0) {
		if (text[i] != prefix[i]) {
			return 0;
		}
		i++;
	}
	return 1;
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

static long block_read_once(u64 block, u64 offset, unsigned char *out, u64 len)
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

static long block_read_bytes(u64 block, u64 offset, unsigned char *out, u64 len)
{
	u64 done = 0;

	if (out == 0) {
		return -1;
	}
	while (done < len) {
		u64 chunk = len - done;

		if (chunk > EXT2_BLOCK_READ_MAX) {
			chunk = EXT2_BLOCK_READ_MAX;
		}
		if (block_read_once(block, offset + done, out + done, chunk) != 0) {
			return -1;
		}
		done += chunk;
	}
	return 0;
}

static long block_write_once(u64 block, u64 offset, const unsigned char *data,
			     u64 len)
{
	const long buffer = bunix_buffer_create(len == 0 ? 1 : len);
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_BLOCK,
		.type = BUNIX_BLOCK_WRITE_BUFFER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_RECV,
		.reply = 0,
		.cap = 0,
		.words = { offset, len, 0, 0 },
	};
	struct bunix_msg reply;

	if (data == 0 || buffer < 0) {
		return -1;
	}
	request.cap = (u64)buffer;
	if ((len != 0 &&
	     bunix_buffer_write((u64)buffer, 0, data, len) != 0) ||
	    bunix_ipc_call(block, &request, &reply) != 0 ||
	    reply.words[0] != 0 ||
	    reply.words[1] != len) {
		bunix_handle_close((u64)buffer);
		return -1;
	}
	bunix_handle_close((u64)buffer);
	return 0;
}

static long block_write_bytes(u64 block, u64 offset, const unsigned char *data,
			      u64 len)
{
	u64 done = 0;

	if (data == 0) {
		return -1;
	}
	while (done < len) {
		u64 chunk = len - done;

		if (chunk > EXT2_BLOCK_READ_MAX) {
			chunk = EXT2_BLOCK_READ_MAX;
		}
		if (block_write_once(block, offset + done, data + done,
				     chunk) != 0) {
			return -1;
		}
		done += chunk;
	}
	return 0;
}

static long mount_read_bytes(const struct ext2_mount *mount, u64 offset,
			     unsigned char *out, u64 len)
{
	if (mount == 0 || out == 0 ||
	    offset > mount->image_size ||
	    len > mount->image_size - offset) {
		return -1;
	}
	if (mount->use_boot_data != 0) {
		if (mount->image_data != 0) {
			for (u64 i = 0; i < len; i++) {
				out[i] = mount->image_data[offset + i];
			}
			return 0;
		}
		return bunix_boot_module_read(offset, out, len);
	}
	return block_read_bytes(mount->block, offset, out, len);
}

static long mount_write_bytes(struct ext2_mount *mount, u64 offset,
			      const unsigned char *data, u64 len)
{
	if (mount == 0 || data == 0 ||
	    offset > mount->image_size ||
	    len > mount->image_size - offset) {
		return -1;
	}
	if (mount->image_data != 0) {
		for (u64 i = 0; i < len; i++) {
			mount->image_data[offset + i] = data[i];
		}
		return 0;
	}
	if (mount->use_boot_data != 0) {
		return -1;
	}
	return block_write_bytes(mount->block, offset, data, len);
}

static long parse_super(const unsigned char *raw, u64 image_size,
			struct ext2_super *super)
{
	u64 block_size;
	u64 fs_bytes;
	u64 unsupported;

	super->inodes_count = load_le32(raw, 0);
	super->blocks_count = load_le32(raw, 4);
	super->free_blocks_count = load_le32(raw, 12);
	super->free_inodes_count = load_le32(raw, 16);
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

static u64 block_to_offset(const struct ext2_mount *mount, u64 block)
{
	if (mount == 0 || block > ((u64)-1) / mount->super.block_size) {
		return (u64)-1;
	}
	return block * mount->super.block_size;
}

static int block_range_valid(const struct ext2_mount *mount, u64 first,
			     u64 count)
{
	if (mount == 0 || count == 0 ||
	    first >= mount->super.blocks_count ||
	    count > mount->super.blocks_count - first) {
		return 0;
	}
	if (block_to_offset(mount, first) == (u64)-1) {
		return 0;
	}
	return 1;
}

static long load_group_descriptors(struct ext2_mount *mount)
{
	const u64 table_block = mount->super.block_size == 1024 ? 2 : 1;
	const u64 table_offset = table_block * mount->super.block_size;
	const u64 inode_table_blocks =
		div_round_up(mount->super.inodes_per_group *
				     mount->super.inode_size,
			     mount->super.block_size);
	u64 table_size;
	unsigned char *raw;

	if (mount->super.groups_count > ((u64)-1) / EXT2_GROUP_DESC_SIZE) {
		log_mount_error("group table size overflow",
				mount->super.groups_count);
		return -1;
	}
	table_size = mount->super.groups_count * EXT2_GROUP_DESC_SIZE;
	if (table_size > mount->image_size ||
	    table_offset > mount->image_size - table_size) {
		log_mount_error("group table outside image", table_size);
		return -1;
	}
	raw = (unsigned char *)bunix_calloc(1, table_size);
	mount->groups = (struct ext2_group *)
		bunix_calloc(mount->super.groups_count, sizeof(mount->groups[0]));
	if (raw == 0 || mount->groups == 0 ||
	    mount_read_bytes(mount, table_offset, raw, table_size) != 0) {
		bunix_free(raw);
		bunix_free(mount->groups);
		mount->groups = 0;
		log_mount_error("group table read failed", table_offset);
		return -1;
	}
	for (u64 i = 0; i < mount->super.groups_count; i++) {
		const u64 offset = i * EXT2_GROUP_DESC_SIZE;
		struct ext2_group *group = &mount->groups[i];

		group->block_bitmap = load_le32(raw, offset);
		group->inode_bitmap = load_le32(raw, offset + 4);
		group->inode_table = load_le32(raw, offset + 8);
		group->free_blocks_count = load_le16(raw, offset + 12);
		group->free_inodes_count = load_le16(raw, offset + 14);
		group->used_dirs_count = load_le16(raw, offset + 16);
		if (!block_range_valid(mount, group->block_bitmap, 1) ||
		    !block_range_valid(mount, group->inode_bitmap, 1) ||
		    !block_range_valid(mount, group->inode_table,
				       inode_table_blocks)) {
			bunix_free(raw);
			bunix_free(mount->groups);
			mount->groups = 0;
			log_mount_error("bad group descriptor", i);
			return -1;
		}
	}
	bunix_free(raw);
	return 0;
}

static long inode_disk_offset(const struct ext2_mount *mount, u64 ino,
			      u64 *out)
{
	u64 group_index;
	u64 inode_index;
	const struct ext2_group *group;
	u64 offset;

	if (mount == 0 || out == 0 ||
	    ino == 0 ||
	    ino > mount->super.inodes_count ||
	    (ino - 1) / mount->super.inodes_per_group >=
		    mount->super.groups_count) {
		return -1;
	}
	group_index = (ino - 1) / mount->super.inodes_per_group;
	inode_index = (ino - 1) % mount->super.inodes_per_group;
	group = &mount->groups[group_index];
	offset = block_to_offset(mount, group->inode_table);
	if (offset == (u64)-1 ||
	    inode_index > ((u64)-1) / mount->super.inode_size ||
	    offset > (u64)-1 - inode_index * mount->super.inode_size) {
		return -1;
	}
	*out = offset + inode_index * mount->super.inode_size;
	return 0;
}

static long read_inode(const struct ext2_mount *mount, u64 ino,
		       struct ext2_inode *inode)
{
	unsigned char raw[EXT2_INODE_MIN_SIZE];
	u64 offset;

	if (inode_disk_offset(mount, ino, &offset) != 0) {
		return -1;
	}
	if (offset > mount->image_size ||
	    sizeof(raw) > mount->image_size - offset ||
	    mount_read_bytes(mount, offset, raw, sizeof(raw)) != 0) {
		return -1;
	}
	inode->mode = load_le16(raw, 0);
	inode->uid = load_le16(raw, 2);
	inode->size = load_le32(raw, 4);
	inode->atime = load_le32(raw, 8);
	inode->ctime = load_le32(raw, 12);
	inode->mtime = load_le32(raw, 16);
	inode->gid = load_le16(raw, 24);
	inode->links_count = load_le16(raw, 26);
	inode->blocks = load_le32(raw, 28);
	for (u64 i = 0; i < 15; i++) {
		inode->block[i] = load_le32(raw, 40 + i * 4);
	}
	for (u64 i = 0; i < sizeof(inode->block_bytes); i++) {
		inode->block_bytes[i] = raw[40 + i];
	}
	if ((inode->mode & EXT2_S_IFMT) == EXT2_S_IFREG) {
		inode->size |= load_le32(raw, 108) << 32;
	}
	return 0;
}

static long write_inode_size(struct ext2_mount *mount, u64 ino,
			     struct ext2_inode *inode, u64 size)
{
	unsigned char raw[4];
	u64 offset;

	if (mount == 0 || inode == 0 ||
	    inode_disk_offset(mount, ino, &offset) != 0) {
		return -1;
	}
	store_le32(raw, 0, size & 0xffffffff);
	if (mount_write_bytes(mount, offset + 4, raw, sizeof(raw)) != 0) {
		return -1;
	}
	if ((inode->mode & EXT2_S_IFMT) == EXT2_S_IFREG) {
		store_le32(raw, 0, size >> 32);
		if (mount_write_bytes(mount, offset + 108, raw,
				      sizeof(raw)) != 0) {
			return -1;
		}
	}
	inode->size = size;
	return 0;
}

static long write_inode_blocks(struct ext2_mount *mount, u64 ino,
			       struct ext2_inode *inode, u64 blocks)
{
	unsigned char raw[4];
	u64 offset;

	if (mount == 0 || inode == 0 ||
	    inode_disk_offset(mount, ino, &offset) != 0) {
		return -1;
	}
	store_le32(raw, 0, blocks);
	if (mount_write_bytes(mount, offset + 28, raw, sizeof(raw)) != 0) {
		return -1;
	}
	inode->blocks = blocks;
	return 0;
}

static long write_inode_direct_block(struct ext2_mount *mount, u64 ino,
				     struct ext2_inode *inode, u64 logical,
				     u64 physical)
{
	unsigned char raw[4];
	u64 offset;

	if (mount == 0 || inode == 0 || logical >= EXT2_NDIR_BLOCKS ||
	    physical > 0xffffffff ||
	    inode_disk_offset(mount, ino, &offset) != 0) {
		return -1;
	}
	store_le32(raw, 0, physical);
	if (mount_write_bytes(mount, offset + 40 + logical * 4, raw,
			      sizeof(raw)) != 0) {
		return -1;
	}
	inode->block[logical] = physical;
	return 0;
}

static long write_inode_mode(struct ext2_mount *mount, u64 ino,
			     struct ext2_inode *inode, u64 mode)
{
	unsigned char raw[2];
	u64 offset;

	if (mount == 0 || inode == 0 ||
	    inode_disk_offset(mount, ino, &offset) != 0) {
		return -1;
	}
	inode->mode = (inode->mode & EXT2_S_IFMT) | (mode & 07777);
	store_le16(raw, 0, inode->mode);
	return mount_write_bytes(mount, offset, raw, sizeof(raw));
}

static long write_inode_owner(struct ext2_mount *mount, u64 ino,
			      struct ext2_inode *inode, u64 uid, u64 gid)
{
	unsigned char raw[2];
	u64 offset;

	if (mount == 0 || inode == 0 ||
	    inode_disk_offset(mount, ino, &offset) != 0) {
		return -1;
	}
	if (uid != (u64)-1) {
		inode->uid = uid & 0xffff;
		store_le16(raw, 0, inode->uid);
		if (mount_write_bytes(mount, offset + 2, raw,
				      sizeof(raw)) != 0) {
			return -1;
		}
	}
	if (gid != (u64)-1) {
		inode->gid = gid & 0xffff;
		store_le16(raw, 0, inode->gid);
		if (mount_write_bytes(mount, offset + 24, raw,
				      sizeof(raw)) != 0) {
			return -1;
		}
	}
	return 0;
}

static long write_inode_new(struct ext2_mount *mount, u64 ino,
			    const struct ext2_inode *inode)
{
	unsigned char raw[EXT2_INODE_MIN_SIZE];
	u64 offset;

	if (mount == 0 || inode == 0 ||
	    inode_disk_offset(mount, ino, &offset) != 0) {
		return -1;
	}
	for (u64 i = 0; i < sizeof(raw); i++) {
		raw[i] = 0;
	}
	store_le16(raw, 0, inode->mode);
	store_le16(raw, 2, inode->uid);
	store_le32(raw, 4, inode->size);
	store_le32(raw, 8, inode->atime);
	store_le32(raw, 12, inode->ctime);
	store_le32(raw, 16, inode->mtime);
	store_le16(raw, 24, inode->gid);
	store_le16(raw, 26, inode->links_count);
	store_le32(raw, 28, inode->blocks);
	for (u64 i = 0; i < 15; i++) {
		store_le32(raw, 40 + i * 4, inode->block[i]);
	}
	return mount_write_bytes(mount, offset, raw, sizeof(raw));
}

static u64 group_desc_offset(const struct ext2_mount *mount, u64 group_index)
{
	const u64 table_block = mount->super.block_size == 1024 ? 2 : 1;
	const u64 table_offset = table_block * mount->super.block_size;

	if (mount == 0 || group_index >= mount->super.groups_count ||
	    group_index > ((u64)-1) / EXT2_GROUP_DESC_SIZE ||
	    table_offset > (u64)-1 - group_index * EXT2_GROUP_DESC_SIZE) {
		return (u64)-1;
	}
	return table_offset + group_index * EXT2_GROUP_DESC_SIZE;
}

static long write_super_count32(struct ext2_mount *mount, u64 field_offset,
				u64 value)
{
	unsigned char raw[4];

	if (mount == 0) {
		return -1;
	}
	store_le32(raw, 0, value);
	return mount_write_bytes(mount, EXT2_SUPER_OFFSET + field_offset,
				 raw, sizeof(raw));
}

static long write_group_count16(struct ext2_mount *mount, u64 group_index,
				u64 field_offset, u64 value)
{
	unsigned char raw[2];
	const u64 offset = group_desc_offset(mount, group_index);

	if (offset == (u64)-1) {
		return -1;
	}
	store_le16(raw, 0, value);
	return mount_write_bytes(mount, offset + field_offset, raw,
				 sizeof(raw));
}

static long bitmap_bit_is_set(const struct ext2_mount *mount, u64 bitmap_block,
			      u64 bit, u64 *set)
{
	unsigned char value = 0;
	u64 offset;

	if (mount == 0 || set == 0 ||
	    bit >= mount->super.block_size * 8 ||
	    !block_range_valid(mount, bitmap_block, 1)) {
		return -1;
	}
	offset = block_to_offset(mount, bitmap_block);
	if (offset == (u64)-1 ||
	    offset > mount->image_size ||
	    bit / 8 >= mount->image_size - offset ||
	    mount_read_bytes(mount, offset + bit / 8, &value, 1) != 0) {
		return -1;
	}
	*set = (value & (1u << (bit % 8))) != 0;
	return 0;
}

static long bitmap_set_bit(struct ext2_mount *mount, u64 bitmap_block,
			   u64 bit, int set)
{
	unsigned char value;
	unsigned char updated;
	u64 offset;

	if (mount == 0 ||
	    bit >= mount->super.block_size * 8 ||
	    !block_range_valid(mount, bitmap_block, 1)) {
		return -1;
	}
	offset = block_to_offset(mount, bitmap_block);
	if (offset == (u64)-1 ||
	    offset > mount->image_size ||
	    bit / 8 >= mount->image_size - offset ||
	    mount_read_bytes(mount, offset + bit / 8, &value, 1) != 0) {
		return -1;
	}
	updated = set != 0 ?
		  (unsigned char)(value | (1u << (bit % 8))) :
		  (unsigned char)(value & ~(1u << (bit % 8)));
	if (updated == value) {
		return 0;
	}
	return mount_write_bytes(mount, offset + bit / 8, &updated, 1);
}

static long inode_is_allocated(const struct ext2_mount *mount, u64 ino,
			       u64 *allocated)
{
	u64 group_index;
	u64 inode_index;

	if (mount == 0 || allocated == 0 ||
	    ino == 0 ||
	    ino > mount->super.inodes_count ||
	    (ino - 1) / mount->super.inodes_per_group >=
		    mount->super.groups_count) {
		return -1;
	}
	group_index = (ino - 1) / mount->super.inodes_per_group;
	inode_index = (ino - 1) % mount->super.inodes_per_group;
	return bitmap_bit_is_set(mount,
				 mount->groups[group_index].inode_bitmap,
				 inode_index, allocated);
}

static long allocate_inode(struct ext2_mount *mount, u64 *ino_out)
{
	if (mount == 0 || ino_out == 0 ||
	    mount->super.free_inodes_count == 0) {
		return -1;
	}
	for (u64 group_index = 0; group_index < mount->super.groups_count;
	     group_index++) {
		struct ext2_group *group = &mount->groups[group_index];

		if (group->free_inodes_count == 0) {
			continue;
		}
		for (u64 bit = 0; bit < mount->super.inodes_per_group; bit++) {
			const u64 ino = group_index *
					mount->super.inodes_per_group + bit + 1;
			u64 allocated = 0;

			if (ino < mount->super.first_ino ||
			    ino > mount->super.inodes_count) {
				continue;
			}
			if (bitmap_bit_is_set(mount, group->inode_bitmap, bit,
					      &allocated) != 0) {
				return -1;
			}
			if (allocated != 0) {
				continue;
			}
			if (bitmap_set_bit(mount, group->inode_bitmap, bit,
					   1) != 0 ||
			    group->free_inodes_count == 0 ||
			    mount->super.free_inodes_count == 0) {
				return -1;
			}
			group->free_inodes_count--;
			mount->super.free_inodes_count--;
			if (write_group_count16(mount, group_index, 14,
						group->free_inodes_count) != 0 ||
			    write_super_count32(mount, 16,
						mount->super.free_inodes_count) != 0) {
				return -1;
			}
			*ino_out = ino;
			return 0;
		}
	}
	return -1;
}

static long free_inode(struct ext2_mount *mount, u64 ino)
{
	u64 group_index;
	u64 inode_index;
	u64 allocated = 0;
	struct ext2_group *group;

	if (mount == 0 || ino == 0 ||
	    ino > mount->super.inodes_count ||
	    (ino - 1) / mount->super.inodes_per_group >=
		    mount->super.groups_count) {
		return -1;
	}
	group_index = (ino - 1) / mount->super.inodes_per_group;
	inode_index = (ino - 1) % mount->super.inodes_per_group;
	group = &mount->groups[group_index];
	if (bitmap_bit_is_set(mount, group->inode_bitmap, inode_index,
			      &allocated) != 0 ||
	    allocated == 0 ||
	    bitmap_set_bit(mount, group->inode_bitmap, inode_index, 0) != 0) {
		return -1;
	}
	group->free_inodes_count++;
	mount->super.free_inodes_count++;
	if (write_group_count16(mount, group_index, 14,
				group->free_inodes_count) != 0 ||
	    write_super_count32(mount, 16,
				mount->super.free_inodes_count) != 0) {
		return -1;
	}
	return 0;
}

static u64 group_first_block(const struct ext2_mount *mount, u64 group_index)
{
	if (mount == 0 ||
	    group_index > ((u64)-1) / mount->super.blocks_per_group) {
		return (u64)-1;
	}
	return mount->super.first_data_block +
	       group_index * mount->super.blocks_per_group;
}

static u64 group_block_count(const struct ext2_mount *mount, u64 group_index)
{
	u64 first;

	if (mount == 0 || group_index >= mount->super.groups_count) {
		return 0;
	}
	first = group_first_block(mount, group_index);
	if (first == (u64)-1 || first >= mount->super.blocks_count) {
		return 0;
	}
	if (mount->super.blocks_count - first < mount->super.blocks_per_group) {
		return mount->super.blocks_count - first;
	}
	return mount->super.blocks_per_group;
}

static long zero_block(struct ext2_mount *mount, u64 block)
{
	unsigned char *zero;
	long result;

	if (mount == 0 || !block_range_valid(mount, block, 1)) {
		return -1;
	}
	zero = (unsigned char *)bunix_calloc(1, mount->super.block_size);
	if (zero == 0) {
		return -1;
	}
	result = mount_write_bytes(mount, block_to_offset(mount, block), zero,
				   mount->super.block_size);
	bunix_free(zero);
	return result;
}

static long allocate_block(struct ext2_mount *mount, u64 *block_out)
{
	if (mount == 0 || block_out == 0 ||
	    mount->super.free_blocks_count == 0) {
		return -1;
	}
	for (u64 group_index = 0; group_index < mount->super.groups_count;
	     group_index++) {
		struct ext2_group *group = &mount->groups[group_index];
		const u64 first = group_first_block(mount, group_index);
		const u64 count = group_block_count(mount, group_index);

		if (first == (u64)-1 || count == 0 ||
		    group->free_blocks_count == 0) {
			continue;
		}
		for (u64 bit = 0; bit < count; bit++) {
			const u64 block = first + bit;
			u64 allocated = 0;

			if (bitmap_bit_is_set(mount, group->block_bitmap, bit,
					      &allocated) != 0) {
				return -1;
			}
			if (allocated != 0) {
				continue;
			}
			if (bitmap_set_bit(mount, group->block_bitmap, bit,
					   1) != 0 ||
			    group->free_blocks_count == 0 ||
			    mount->super.free_blocks_count == 0) {
				return -1;
			}
			group->free_blocks_count--;
			mount->super.free_blocks_count--;
			if (write_group_count16(mount, group_index, 12,
						group->free_blocks_count) != 0 ||
			    write_super_count32(mount, 12,
						mount->super.free_blocks_count) != 0 ||
			    zero_block(mount, block) != 0) {
				return -1;
			}
			*block_out = block;
			return 0;
		}
	}
	return -1;
}

static long free_block(struct ext2_mount *mount, u64 block)
{
	u64 group_index;
	u64 first;
	u64 bit;
	u64 allocated = 0;
	struct ext2_group *group;

	if (mount == 0 ||
	    block < mount->super.first_data_block ||
	    block >= mount->super.blocks_count) {
		return -1;
	}
	group_index = (block - mount->super.first_data_block) /
		      mount->super.blocks_per_group;
	if (group_index >= mount->super.groups_count) {
		return -1;
	}
	first = group_first_block(mount, group_index);
	if (first == (u64)-1 || block < first) {
		return -1;
	}
	bit = block - first;
	group = &mount->groups[group_index];
	if (bitmap_bit_is_set(mount, group->block_bitmap, bit, &allocated) != 0 ||
	    allocated == 0 ||
	    bitmap_set_bit(mount, group->block_bitmap, bit, 0) != 0) {
		return -1;
	}
	group->free_blocks_count++;
	mount->super.free_blocks_count++;
	if (write_group_count16(mount, group_index, 12,
				group->free_blocks_count) != 0 ||
	    write_super_count32(mount, 12,
				mount->super.free_blocks_count) != 0) {
		return -1;
	}
	return 0;
}

static u64 ext2_inode_vfs_type(const struct ext2_inode *inode)
{
	if (inode == 0) {
		return 0;
	}
	switch (inode->mode & EXT2_S_IFMT) {
	case EXT2_S_IFREG:
		return BUNIX_VFS_TYPE_REGULAR;
	case EXT2_S_IFDIR:
		return BUNIX_VFS_TYPE_DIRECTORY;
	case EXT2_S_IFLNK:
		return BUNIX_VFS_TYPE_SYMLINK;
	case EXT2_S_IFIFO:
		return BUNIX_VFS_TYPE_FIFO;
	case EXT2_S_IFCHR:
		return BUNIX_VFS_TYPE_CHARACTER;
	default:
		return 0;
	}
}

static u64 ext2_dirent_vfs_type(u64 file_type)
{
	switch (file_type) {
	case EXT2_FT_REG_FILE:
		return BUNIX_VFS_TYPE_REGULAR;
	case EXT2_FT_DIR:
		return BUNIX_VFS_TYPE_DIRECTORY;
	case EXT2_FT_CHRDEV:
		return BUNIX_VFS_TYPE_CHARACTER;
	case EXT2_FT_FIFO:
		return BUNIX_VFS_TYPE_FIFO;
	case EXT2_FT_SYMLINK:
		return BUNIX_VFS_TYPE_SYMLINK;
	default:
		return 0;
	}
}

static long read_block_u32(const struct ext2_mount *mount, u64 block,
			   u64 index, u64 *value)
{
	unsigned char raw[4];
	u64 offset;
	u64 entry_offset;

	if (mount == 0 || value == 0 ||
	    index >= mount->super.block_size / 4 ||
	    index > ((u64)-1) / 4 ||
	    !block_range_valid(mount, block, 1)) {
		return -1;
	}
	entry_offset = index * 4;
	offset = block_to_offset(mount, block);
	if (offset == (u64)-1 ||
	    offset > mount->image_size ||
	    entry_offset > mount->image_size - offset ||
	    sizeof(raw) > mount->image_size - offset - entry_offset ||
	    mount_read_bytes(mount, offset + entry_offset, raw,
			     sizeof(raw)) != 0) {
		return -1;
	}
	*value = load_le32(raw, 0);
	return 0;
}

static long map_logical_block(const struct ext2_mount *mount,
			      const struct ext2_inode *inode, u64 logical,
			      u64 *physical)
{
	u64 per_block;
	u64 index;
	u64 first;
	u64 second;

	if (mount == 0 || inode == 0 || physical == 0) {
		return -1;
	}
	per_block = mount->super.block_size / 4;
	if (per_block == 0) {
		return -1;
	}
	if (logical < EXT2_NDIR_BLOCKS) {
		*physical = inode->block[logical];
		return 0;
	}
	logical -= EXT2_NDIR_BLOCKS;
	if (logical < per_block) {
		if (inode->block[EXT2_IND_BLOCK] == 0) {
			*physical = 0;
			return 0;
		}
		return read_block_u32(mount, inode->block[EXT2_IND_BLOCK],
				      logical, physical);
	}
	logical -= per_block;
	if (per_block != 0 &&
	    logical / per_block < per_block) {
		if (inode->block[EXT2_DIND_BLOCK] == 0) {
			*physical = 0;
			return 0;
		}
		index = logical / per_block;
		if (read_block_u32(mount, inode->block[EXT2_DIND_BLOCK],
				   index, &first) != 0) {
			return -1;
		}
		if (first == 0) {
			*physical = 0;
			return 0;
		}
		return read_block_u32(mount, first, logical % per_block,
				      physical);
	}
	if (per_block > ((u64)-1) / per_block) {
		return -1;
	}
	logical -= per_block * per_block;
	if (per_block != 0 && logical / per_block / per_block < per_block) {
		if (inode->block[EXT2_TIND_BLOCK] == 0) {
			*physical = 0;
			return 0;
		}
		index = logical / (per_block * per_block);
		if (read_block_u32(mount, inode->block[EXT2_TIND_BLOCK],
				   index, &first) != 0) {
			return -1;
		}
		if (first == 0) {
			*physical = 0;
			return 0;
		}
		logical %= per_block * per_block;
		if (read_block_u32(mount, first, logical / per_block,
				   &second) != 0) {
			return -1;
		}
		if (second == 0) {
			*physical = 0;
			return 0;
		}
		return read_block_u32(mount, second, logical % per_block,
				      physical);
	}
	return -1;
}

static long read_mapped_block(const struct ext2_mount *mount,
			      const struct ext2_inode *inode, u64 logical,
			      unsigned char *out)
{
	u64 physical;
	u64 offset;

	if (mount == 0 || inode == 0 || out == 0 ||
	    map_logical_block(mount, inode, logical, &physical) != 0) {
		return -1;
	}
	if (physical == 0) {
		for (u64 i = 0; i < mount->super.block_size; i++) {
			out[i] = 0;
		}
		return 0;
	}
	if (!block_range_valid(mount, physical, 1)) {
		return -1;
	}
	offset = block_to_offset(mount, physical);
	if (offset == (u64)-1 ||
	    offset > mount->image_size ||
	    mount->super.block_size > mount->image_size - offset ||
	    mount_read_bytes(mount, offset, out,
			     mount->super.block_size) != 0) {
		return -1;
	}
	return 0;
}

static long write_mapped_block(struct ext2_mount *mount,
			       const struct ext2_inode *inode, u64 logical,
			       const unsigned char *data)
{
	u64 physical;
	u64 offset;

	if (mount == 0 || inode == 0 || data == 0 ||
	    map_logical_block(mount, inode, logical, &physical) != 0 ||
	    physical == 0 ||
	    !block_range_valid(mount, physical, 1)) {
		return -1;
	}
	offset = block_to_offset(mount, physical);
	if (offset == (u64)-1 ||
	    offset > mount->image_size ||
	    mount->super.block_size > mount->image_size - offset) {
		return -1;
	}
	return mount_write_bytes(mount, offset, data, mount->super.block_size);
}

static long ensure_direct_block(struct ext2_mount *mount, u64 ino,
				struct ext2_inode *inode, u64 logical,
				u64 *physical)
{
	u64 block;
	u64 sectors;

	if (mount == 0 || inode == 0 || physical == 0 ||
	    logical >= EXT2_NDIR_BLOCKS) {
		return -1;
	}
	if (inode->block[logical] != 0) {
		*physical = inode->block[logical];
		return block_range_valid(mount, *physical, 1) ? 0 : -1;
	}
	if (allocate_block(mount, &block) != 0) {
		return -1;
	}
	sectors = mount->super.block_size / 512;
	if (sectors == 0 ||
	    write_inode_direct_block(mount, ino, inode, logical, block) != 0 ||
	    write_inode_blocks(mount, ino, inode,
			       inode->blocks + sectors) != 0) {
		(void)write_inode_direct_block(mount, ino, inode, logical, 0);
		(void)free_block(mount, block);
		return -1;
	}
	*physical = block;
	return 0;
}

static long write_direct_file_block(struct ext2_mount *mount, u64 ino,
				    struct ext2_inode *inode, u64 logical,
				    const unsigned char *data)
{
	u64 physical;
	u64 offset;

	if (mount == 0 || inode == 0 || data == 0 ||
	    ensure_direct_block(mount, ino, inode, logical, &physical) != 0 ||
	    !block_range_valid(mount, physical, 1)) {
		return -1;
	}
	offset = block_to_offset(mount, physical);
	if (offset == (u64)-1 ||
	    offset > mount->image_size ||
	    mount->super.block_size > mount->image_size - offset) {
		return -1;
	}
	return mount_write_bytes(mount, offset, data, mount->super.block_size);
}

static long read_dirent_at(const struct ext2_mount *mount,
			   const struct ext2_inode *directory, u64 offset,
			   struct ext2_dirent *entry)
{
	unsigned char *block;
	u64 block_index;
	u64 block_offset;

	if (mount == 0 || directory == 0 || entry == 0 ||
	    ext2_inode_vfs_type(directory) != BUNIX_VFS_TYPE_DIRECTORY ||
	    offset >= directory->size ||
	    offset >= EXT2_NDIR_BLOCKS * mount->super.block_size) {
		return -1;
	}
	block_index = offset / mount->super.block_size;
	block_offset = offset % mount->super.block_size;
	if (mount->super.block_size - block_offset < 8) {
		return -1;
	}
	block = (unsigned char *)bunix_calloc(1, mount->super.block_size);
	if (block == 0 ||
	    read_mapped_block(mount, directory, block_index, block) != 0) {
		bunix_free(block);
		return -1;
	}
	entry->ino = load_le32(block, block_offset);
	entry->rec_len = load_le16(block, block_offset + 4);
	entry->name_len = block[block_offset + 6];
	entry->file_type = block[block_offset + 7];
	entry->offset = offset;
	if (entry->rec_len < 8 ||
	    (entry->rec_len & 3) != 0 ||
	    entry->rec_len > mount->super.block_size - block_offset ||
	    entry->name_len > EXT2_NAME_MAX ||
	    entry->name_len > entry->rec_len - 8) {
		bunix_free(block);
		return -1;
	}
	for (u64 i = 0; i < entry->name_len; i++) {
		entry->name[i] = (char)block[block_offset + 8 + i];
	}
	entry->name[entry->name_len] = 0;
	bunix_free(block);
	return 0;
}

static long next_dirent(const struct ext2_mount *mount,
			const struct ext2_inode *directory, u64 *cookie,
			struct ext2_dirent *entry, int skip_dots)
{
	u64 offset;

	if (cookie == 0) {
		return -1;
	}
	offset = *cookie;
	while (offset < directory->size &&
	       offset < EXT2_NDIR_BLOCKS * mount->super.block_size) {
		if (read_dirent_at(mount, directory, offset, entry) != 0) {
			return -1;
		}
		offset += entry->rec_len;
		*cookie = offset;
		if (entry->ino != 0 &&
		    (!skip_dots ||
		     (!text_eq_len(".", entry->name, entry->name_len) &&
		      !text_eq_len("..", entry->name, entry->name_len)))) {
			return 0;
		}
	}
	return BUNIX_VFS_ERR_NOENT;
}

static long lookup_dirent(const struct ext2_mount *mount,
			  const struct ext2_inode *directory,
			  const char *name, struct ext2_dirent *entry)
{
	u64 cookie = 0;

	while (next_dirent(mount, directory, &cookie, entry, 0) == 0) {
		if (text_eq_len(name, entry->name, entry->name_len)) {
			return 0;
		}
	}
	return BUNIX_VFS_ERR_NOENT;
}

static long lookup_path(const struct ext2_mount *mount, const char *path,
			u64 *ino, struct ext2_inode *inode)
{
	struct ext2_inode current;
	u64 current_ino = EXT2_ROOT_INO;
	u64 pos = 1;

	if (mount == 0 || path == 0 || path[0] != '/' ||
	    ino == 0 || inode == 0 ||
	    read_inode(mount, EXT2_ROOT_INO, &current) != 0) {
		return BUNIX_VFS_ERR_NOENT;
	}
	if (path[1] == 0) {
		*ino = current_ino;
		*inode = current;
		return 0;
	}
	while (path[pos] != 0) {
		struct ext2_dirent entry;
		char name[EXT2_NAME_MAX + 1];
		u64 name_len = 0;

		while (path[pos] == '/') {
			pos++;
		}
		if (path[pos] == 0) {
			break;
		}
		while (path[pos + name_len] != 0 &&
		       path[pos + name_len] != '/') {
			if (name_len >= EXT2_NAME_MAX) {
				return BUNIX_VFS_ERR_NOENT;
			}
			name[name_len] = path[pos + name_len];
			name_len++;
		}
		name[name_len] = 0;
		if (ext2_inode_vfs_type(&current) != BUNIX_VFS_TYPE_DIRECTORY ||
		    lookup_dirent(mount, &current, name, &entry) != 0 ||
		    read_inode(mount, entry.ino, &current) != 0) {
			return BUNIX_VFS_ERR_NOENT;
		}
		current_ino = entry.ino;
		pos += name_len;
	}
	*ino = current_ino;
	*inode = current;
	return 0;
}

static u64 ext2_dirent_min_len(u64 name_len)
{
	return (8 + name_len + 3) & ~3ull;
}

static long split_parent_name(const char *path, char *parent, char *name)
{
	u64 len;
	u64 slash = 0;
	u64 name_len;

	if (path == 0 || parent == 0 || name == 0 || path[0] != '/') {
		return -1;
	}
	len = text_len(path);
	if (len <= 1 || len >= EXT2_MAX_PATH) {
		return -1;
	}
	for (u64 i = 1; i < len; i++) {
		if (path[i] == '/') {
			slash = i;
		}
	}
	name_len = len - slash - 1;
	if (name_len == 0 || name_len > EXT2_NAME_MAX) {
		return -1;
	}
	if (slash == 0) {
		parent[0] = '/';
		parent[1] = '\0';
	} else {
		for (u64 i = 0; i < slash; i++) {
			parent[i] = path[i];
		}
		parent[slash] = '\0';
	}
	for (u64 i = 0; i < name_len; i++) {
		name[i] = path[slash + 1 + i];
	}
	name[name_len] = '\0';
	return 0;
}

static void write_dirent(unsigned char *block, u64 offset, u64 ino,
			 u64 rec_len, u64 name_len, u64 file_type,
			 const char *name)
{
	store_le32(block, offset, ino);
	store_le16(block, offset + 4, rec_len);
	block[offset + 6] = (unsigned char)name_len;
	block[offset + 7] = (unsigned char)file_type;
	for (u64 i = 0; i < rec_len - 8; i++) {
		block[offset + 8 + i] = 0;
	}
	for (u64 i = 0; i < name_len; i++) {
		block[offset + 8 + i] = (unsigned char)name[i];
	}
}

static long insert_dirent_existing_space(struct ext2_mount *mount,
					 const struct ext2_inode *directory,
					 u64 ino, const char *name,
					 u64 file_type)
{
	const u64 name_len = text_len(name);
	const u64 need = ext2_dirent_min_len(name_len);
	unsigned char *block;

	if (mount == 0 || directory == 0 || name == 0 ||
	    name_len == 0 || name_len > EXT2_NAME_MAX ||
	    ext2_inode_vfs_type(directory) != BUNIX_VFS_TYPE_DIRECTORY ||
	    need > mount->super.block_size) {
		return -1;
	}
	block = (unsigned char *)bunix_calloc(1, mount->super.block_size);
	if (block == 0) {
		return -1;
	}
	for (u64 block_index = 0;
	     block_index * mount->super.block_size < directory->size;
	     block_index++) {
		u64 offset = 0;

		if (read_mapped_block(mount, directory, block_index,
				      block) != 0) {
			bunix_free(block);
			return -1;
		}
		while (offset + 8 <= mount->super.block_size &&
		       block_index * mount->super.block_size + offset <
			       directory->size) {
			const u64 entry_ino = load_le32(block, offset);
			const u64 rec_len = load_le16(block, offset + 4);
			const u64 existing_name_len = block[offset + 6];
			const u64 actual = entry_ino == 0 ? 8 :
					   ext2_dirent_min_len(existing_name_len);

			if (rec_len < 8 ||
			    (rec_len & 3) != 0 ||
			    rec_len > mount->super.block_size - offset ||
			    existing_name_len > rec_len - 8 ||
			    actual > rec_len) {
				bunix_free(block);
				return -1;
			}
			if (entry_ino == 0 && rec_len >= need) {
				write_dirent(block, offset, ino, rec_len,
					     name_len, file_type, name);
				if (write_mapped_block(mount, directory,
						       block_index,
						       block) != 0) {
					bunix_free(block);
					return -1;
				}
				bunix_free(block);
				return 0;
			}
			if (entry_ino != 0 && rec_len >= actual + need) {
				const u64 new_offset = offset + actual;
				const u64 new_rec_len = rec_len - actual;

				store_le16(block, offset + 4, actual);
				write_dirent(block, new_offset, ino,
					     new_rec_len, name_len, file_type,
					     name);
				if (write_mapped_block(mount, directory,
						       block_index,
						       block) != 0) {
					bunix_free(block);
					return -1;
				}
				bunix_free(block);
				return 0;
			}
			offset += rec_len;
		}
	}
	bunix_free(block);
	return -1;
}

static u64 remember_open(u64 ino, const struct ext2_inode *inode)
{
	struct ext2_open *open;
	u64 handle;

	open = (struct ext2_open *)bunix_calloc(1, sizeof(*open));
	if (open == 0 || inode == 0) {
		bunix_free(open);
		return 0;
	}
	handle = next_open_handle++;
	while (handle == 0 || bunix_u64_tree_get(&open_files, handle) != 0) {
		handle = next_open_handle++;
	}
	open->handle = handle;
	open->ino = ino;
	open->inode = *inode;
	if (bunix_u64_tree_insert_node(&open_files, &open->node, handle,
				       (u64)open) != 0) {
		bunix_free(open);
		return 0;
	}
	return handle;
}

static struct ext2_open *open_from_handle(u64 handle)
{
	return (struct ext2_open *)bunix_u64_tree_get(&open_files, handle);
}

static void forget_open(u64 handle)
{
	struct ext2_open *open = open_from_handle(handle);

	if (open == 0) {
		return;
	}
	bunix_u64_tree_remove_node(&open_files, &open->node);
	bunix_free(open);
}

static int read_path_buffer_at(u64 buffer, u64 offset, u64 len, char *path)
{
	if (buffer == 0 || path == 0 || len == 0 || len > EXT2_MAX_PATH) {
		return -1;
	}
	for (u64 i = 0; i < EXT2_MAX_PATH; i++) {
		path[i] = 0;
	}
	if (bunix_buffer_read(buffer, offset, path, len) != 0 ||
	    path[len - 1] != 0) {
		return -1;
	}
	return text_len(path) < EXT2_MAX_PATH ? 0 : -1;
}

static int read_resolved_path(const struct bunix_msg *message, char *path)
{
	char cwd[EXT2_MAX_PATH];
	const char mount_prefix[] = "/mnt/ext2";
	const u64 mount_prefix_len = sizeof(mount_prefix) - 1;

	if (message == 0 ||
	    (message->cap_rights & BUNIX_RIGHT_RECV) == 0 ||
	    read_path_buffer_at(message->cap, 0, message->words[0], cwd) != 0 ||
	    read_path_buffer_at(message->cap, message->words[0],
				message->words[1], path) != 0 ||
	    !text_eq(cwd, "/") ||
	    path[0] != '/') {
		return -1;
	}
	if (text_starts_with(path, mount_prefix) &&
	    (path[mount_prefix_len] == '\0' ||
	     path[mount_prefix_len] == '/')) {
		if (path[mount_prefix_len] == '\0') {
			path[0] = '/';
			path[1] = '\0';
			return 0;
		}
		for (u64 i = 0; i < EXT2_MAX_PATH; i++) {
			path[i] = path[mount_prefix_len + i];
			if (path[i] == '\0') {
				break;
			}
		}
	}
	return 0;
}

static u64 stat_buffer_offset(const struct bunix_msg *message)
{
	if (message->type == BUNIX_VFS_STAT_PATH_META_BUFFER) {
		return message->words[0] + message->words[1];
	}
	return message->words[1];
}

static u64 ext2_inode_rdev(const struct ext2_inode *inode)
{
	if (inode == 0 ||
	    ext2_inode_vfs_type(inode) != BUNIX_VFS_TYPE_CHARACTER) {
		return 0;
	}
	return inode->block[0];
}

static void reply_stat_inode(const struct bunix_msg *message,
			     struct bunix_msg *reply, u64 ino,
			     const struct ext2_inode *inode)
{
	const u64 type = ext2_inode_vfs_type(inode);
	const u64 mode = inode->mode & 07777;
	const u64 mode_type = mode | (type << 32);
	const u64 owner = inode->uid | (inode->gid << 32);
	const u64 rdev = ext2_inode_rdev(inode);

	if (type == 0) {
		reply->words[0] = BUNIX_VFS_ERR_ACCESS;
		return;
	}
	reply->words[0] = 0;
	reply->words[1] = inode->size;
	reply->words[2] = mode_type;
	reply->words[3] = owner;
	if (message->cap != 0 &&
	    (message->cap_rights & BUNIX_RIGHT_SEND) != 0) {
		(void)bunix_vfs_stat_write_times(
			message->cap, stat_buffer_offset(message), inode->size,
			mode_type, owner, BUNIX_VFS_DEV_EXT2, ino,
			inode->links_count, rdev, root_mount.super.block_size,
			inode->blocks, inode->atime, inode->mtime,
			inode->ctime);
	}
}

static void reply_path_stat(const struct bunix_msg *message,
			    struct bunix_msg *reply, const char *path)
{
	struct ext2_inode inode;
	u64 ino;

	if (!root_mount_ready ||
	    lookup_path(&root_mount, path, &ino, &inode) != 0) {
		reply->words[0] = BUNIX_VFS_ERR_NOENT;
		return;
	}
	reply_stat_inode(message, reply, ino, &inode);
}

static void reply_path_chmod(struct bunix_msg *reply, const char *path,
			     u64 mode)
{
	struct ext2_inode inode;
	u64 ino;

	if (!root_mount_ready ||
	    lookup_path(&root_mount, path, &ino, &inode) != 0) {
		reply->words[0] = BUNIX_VFS_ERR_NOENT;
		return;
	}
	if (ext2_inode_vfs_type(&inode) == 0 ||
	    write_inode_mode(&root_mount, ino, &inode, mode) != 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	reply->words[0] = 0;
}

static void reply_path_chown(struct bunix_msg *reply, const char *path,
			     u64 uid, u64 gid)
{
	struct ext2_inode inode;
	u64 ino;

	if (!root_mount_ready ||
	    lookup_path(&root_mount, path, &ino, &inode) != 0) {
		reply->words[0] = BUNIX_VFS_ERR_NOENT;
		return;
	}
	if (ext2_inode_vfs_type(&inode) == 0 ||
	    write_inode_owner(&root_mount, ino, &inode, uid, gid) != 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	reply->words[0] = 0;
}

static void reply_create(struct bunix_msg *reply, const char *path, u64 mode)
{
	char parent_path[EXT2_MAX_PATH];
	char name[EXT2_NAME_MAX + 1];
	struct ext2_inode parent_inode;
	struct ext2_inode existing;
	struct ext2_inode inode;
	u64 parent_ino;
	u64 existing_ino;
	u64 ino = 0;

	if (!root_mount_ready) {
		reply->words[0] = BUNIX_VFS_ERR_NOENT;
		return;
	}
	if (lookup_path(&root_mount, path, &existing_ino, &existing) == 0) {
		reply->words[0] = BUNIX_VFS_ERR_EXIST;
		return;
	}
	if (split_parent_name(path, parent_path, name) != 0 ||
	    lookup_path(&root_mount, parent_path, &parent_ino,
			&parent_inode) != 0 ||
	    ext2_inode_vfs_type(&parent_inode) != BUNIX_VFS_TYPE_DIRECTORY) {
		reply->words[0] = BUNIX_VFS_ERR_NOENT;
		return;
	}
	for (u64 i = 0; i < sizeof(inode.block) / sizeof(inode.block[0]); i++) {
		inode.block[i] = 0;
	}
	for (u64 i = 0; i < sizeof(inode.block_bytes); i++) {
		inode.block_bytes[i] = 0;
	}
	inode.mode = EXT2_S_IFREG | (mode & 07777);
	inode.uid = 0;
	inode.size = 0;
	inode.atime = 0;
	inode.ctime = 0;
	inode.mtime = 0;
	inode.gid = 0;
	inode.links_count = 1;
	inode.blocks = 0;
	if (allocate_inode(&root_mount, &ino) != 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	if (write_inode_new(&root_mount, ino, &inode) != 0 ||
	    insert_dirent_existing_space(&root_mount, &parent_inode, ino, name,
					 EXT2_FT_REG_FILE) != 0) {
		(void)free_inode(&root_mount, ino);
		reply->words[0] = (u64)-1;
		return;
	}
	reply->words[0] = 0;
}

static long read_inode_bytes(const struct ext2_inode *inode, u64 offset,
			     unsigned char *out, u64 len)
{
	u64 done = 0;

	if (inode == 0 || out == 0 ||
	    offset > inode->size ||
	    len > inode->size - offset) {
		return -1;
	}
	while (done < len) {
		const u64 file_offset = offset + done;
		const u64 logical = file_offset / root_mount.super.block_size;
		const u64 block_offset = file_offset % root_mount.super.block_size;
		u64 chunk = root_mount.super.block_size - block_offset;
		unsigned char *block;

		if (chunk > len - done) {
			chunk = len - done;
		}
		block = (unsigned char *)bunix_calloc(1,
						      root_mount.super.block_size);
		if (block == 0 ||
		    read_mapped_block(&root_mount, inode, logical, block) != 0) {
			bunix_free(block);
			return -1;
		}
		for (u64 i = 0; i < chunk; i++) {
			out[done + i] = block[block_offset + i];
		}
		bunix_free(block);
		done += chunk;
	}
	return 0;
}

static void reply_readlink(const struct bunix_msg *message,
			   struct bunix_msg *reply, const char *path)
{
	struct ext2_inode inode;
	unsigned char target[EXT2_MAX_PATH];
	u64 ino;
	u64 written;
	const u64 out_cap = message->words[3] >> 32;
	const u64 out_offset = message->words[0] + message->words[1];

	if (!root_mount_ready ||
	    lookup_path(&root_mount, path, &ino, &inode) != 0) {
		reply->words[0] = BUNIX_VFS_ERR_NOENT;
		return;
	}
	if (ext2_inode_vfs_type(&inode) != BUNIX_VFS_TYPE_SYMLINK ||
	    inode.size >= sizeof(target)) {
		reply->words[0] = BUNIX_VFS_ERR_INVAL;
		return;
	}
	if (inode.size <= sizeof(inode.block_bytes) && inode.blocks == 0) {
		for (u64 i = 0; i < inode.size; i++) {
			target[i] = inode.block_bytes[i];
		}
	} else if (read_inode_bytes(&inode, 0, target, inode.size) != 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	reply->words[0] = 0;
	reply->words[1] = inode.size;
	if (out_cap == 0) {
		return;
	}
	written = inode.size;
	if (written > out_cap) {
		written = out_cap;
	}
	if (message->cap == 0 ||
	    (message->cap_rights & BUNIX_RIGHT_SEND) == 0 ||
	    (written != 0 &&
	     bunix_buffer_write(message->cap, out_offset, target,
				written) != 0)) {
		reply->words[0] = (u64)-1;
		return;
	}
	reply->words[2] = inode.size;
	reply->words[3] = written;
}

static void reply_open(struct bunix_msg *message, struct bunix_msg *reply,
		       const char *path)
{
	struct ext2_inode inode;
	u64 ino;
	u64 handle;
	u64 type;

	if (!root_mount_ready ||
	    lookup_path(&root_mount, path, &ino, &inode) != 0) {
		reply->words[0] = BUNIX_VFS_ERR_NOENT;
		return;
	}
	type = ext2_inode_vfs_type(&inode);
	if (type == 0) {
		reply->words[0] = BUNIX_VFS_ERR_ACCESS;
		return;
	}
	handle = remember_open(ino, &inode);
	if (handle == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	reply->words[0] = 0;
	reply->words[1] = handle;
	reply->words[2] = inode.size;
	reply->words[3] = type;
	(void)message;
}

static void reply_readdir(struct ext2_open *open,
			  const struct bunix_msg *message,
			  struct bunix_msg *reply)
{
	struct ext2_dirent entry;
	u64 cookie;
	u64 type;
	u64 written;

	if (open == 0 ||
	    ext2_inode_vfs_type(&open->inode) != BUNIX_VFS_TYPE_DIRECTORY) {
		reply->words[0] = BUNIX_VFS_ERR_NOTDIR;
		return;
	}
	if (message->cap == 0 ||
	    (message->cap_rights & BUNIX_RIGHT_SEND) == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	cookie = message->words[1];
	if (next_dirent(&root_mount, &open->inode, &cookie, &entry, 1) != 0) {
		reply->words[0] = BUNIX_VFS_ERR_NOENT;
		return;
	}
	type = ext2_dirent_vfs_type(entry.file_type);
	if (type == 0) {
		struct ext2_inode inode;

		if (read_inode(&root_mount, entry.ino, &inode) != 0) {
			reply->words[0] = (u64)-1;
			return;
		}
		type = ext2_inode_vfs_type(&inode);
	}
	if (type == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	written = entry.name_len;
	if (written > message->words[3]) {
		written = message->words[3];
	}
	if (written != 0 &&
	    bunix_buffer_write(message->cap, message->words[2],
			       entry.name, written) != 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	reply->words[0] = 0;
	reply->words[1] = cookie | (type << 32);
	reply->words[2] = entry.name_len;
	reply->words[3] = written;
}

static void reply_read_file(struct ext2_open *open,
			    const struct bunix_msg *message,
			    struct bunix_msg *reply)
{
	unsigned char *block;
	u64 offset;
	u64 len;
	u64 done = 0;

	if (open == 0 ||
	    ext2_inode_vfs_type(&open->inode) != BUNIX_VFS_TYPE_REGULAR ||
	    message->cap == 0 ||
	    (message->cap_rights & BUNIX_RIGHT_SEND) == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	offset = message->words[1];
	len = message->words[2];
	if (offset >= open->inode.size) {
		reply->words[0] = 0;
		reply->words[1] = 0;
		return;
	}
	if (len > open->inode.size - offset) {
		len = open->inode.size - offset;
	}
	block = (unsigned char *)bunix_calloc(1, root_mount.super.block_size);
	if (block == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	while (done < len) {
		const u64 file_offset = offset + done;
		const u64 logical = file_offset / root_mount.super.block_size;
		const u64 block_offset = file_offset % root_mount.super.block_size;
		u64 chunk = root_mount.super.block_size - block_offset;

		if (chunk > len - done) {
			chunk = len - done;
		}
		if (read_mapped_block(&root_mount, &open->inode,
				      logical, block) != 0 ||
		    bunix_buffer_write(message->cap, done,
				       block + block_offset, chunk) != 0) {
			bunix_free(block);
			reply->words[0] = (u64)-1;
			reply->words[1] = done;
			return;
		}
		done += chunk;
	}
	bunix_free(block);
	reply->words[0] = 0;
	reply->words[1] = done;
}

static void reply_write_file(struct ext2_open *open,
			     const struct bunix_msg *message,
			     struct bunix_msg *reply)
{
	unsigned char *block;
	u64 offset;
	u64 len;
	u64 new_size;
	u64 done = 0;

	if (open == 0 ||
	    ext2_inode_vfs_type(&open->inode) != BUNIX_VFS_TYPE_REGULAR ||
	    message->cap == 0 ||
	    (message->cap_rights & BUNIX_RIGHT_RECV) == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	offset = message->words[1];
	len = message->words[2];
	if (offset > open->inode.size ||
	    len > (u64)-1 - offset) {
		reply->words[0] = BUNIX_VFS_ERR_ACCESS;
		reply->words[1] = 0;
		return;
	}
	new_size = offset + len;
	if (new_size > EXT2_NDIR_BLOCKS * root_mount.super.block_size) {
		reply->words[0] = BUNIX_VFS_ERR_ACCESS;
		reply->words[1] = 0;
		return;
	}
	block = (unsigned char *)bunix_calloc(1, root_mount.super.block_size);
	if (block == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	while (done < len) {
		const u64 file_offset = offset + done;
		const u64 logical = file_offset / root_mount.super.block_size;
		const u64 block_offset = file_offset % root_mount.super.block_size;
		u64 physical;
		u64 chunk = root_mount.super.block_size - block_offset;

		if (chunk > len - done) {
			chunk = len - done;
		}
		if (logical >= EXT2_NDIR_BLOCKS ||
		    ensure_direct_block(&root_mount, open->ino, &open->inode,
					logical, &physical) != 0 ||
		    !block_range_valid(&root_mount, physical, 1) ||
		    read_mapped_block(&root_mount, &open->inode,
				      logical, block) != 0 ||
		    bunix_buffer_read(message->cap, done,
				      block + block_offset, chunk) != 0 ||
		    write_direct_file_block(&root_mount, open->ino,
					    &open->inode, logical, block) != 0) {
			bunix_free(block);
			reply->words[0] = (u64)-1;
			reply->words[1] = done;
			return;
		}
		done += chunk;
	}
	bunix_free(block);
	if (new_size > open->inode.size &&
	    write_inode_size(&root_mount, open->ino, &open->inode,
			     new_size) != 0) {
		reply->words[0] = (u64)-1;
		reply->words[1] = done;
		return;
	}
	reply->words[0] = 0;
	reply->words[1] = done;
}

static void reply_truncate(struct ext2_open *open,
			   const struct bunix_msg *message,
			   struct bunix_msg *reply)
{
	const u64 size = message->words[1];

	if (open == 0 ||
	    ext2_inode_vfs_type(&open->inode) != BUNIX_VFS_TYPE_REGULAR) {
		reply->words[0] = (u64)-1;
		return;
	}
	if (size > open->inode.size) {
		reply->words[0] = BUNIX_VFS_ERR_ACCESS;
		return;
	}
	if (write_inode_size(&root_mount, open->ino, &open->inode, size) != 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	reply->words[0] = 0;
	reply->words[1] = size;
}

static void reply_chmod(struct ext2_open *open, const struct bunix_msg *message,
			struct bunix_msg *reply)
{
	if (open == 0 || ext2_inode_vfs_type(&open->inode) == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	if (write_inode_mode(&root_mount, open->ino, &open->inode,
			     message->words[1]) != 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	reply->words[0] = 0;
}

static void reply_chown(struct ext2_open *open, const struct bunix_msg *message,
			struct bunix_msg *reply)
{
	if (open == 0 || ext2_inode_vfs_type(&open->inode) == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	if (write_inode_owner(&root_mount, open->ino, &open->inode,
			      message->words[1], message->words[2]) != 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	reply->words[0] = 0;
}

static long load_initial_mount(struct ext2_mount *mount)
{
	unsigned char raw[EXT2_SUPER_SIZE];
	struct ext2_inode root_inode;
	struct ext2_dirent self;
	u64 root_allocated = 0;

	mount->groups = 0;
	if ((mount->use_boot_data == 0 &&
	     block_get_size(mount->block, &mount->image_size) != 0) ||
	    mount_read_bytes(mount, EXT2_SUPER_OFFSET, raw, sizeof(raw)) != 0 ||
	    parse_super(raw, mount->image_size, &mount->super) != 0 ||
	    load_group_descriptors(mount) != 0 ||
	    inode_is_allocated(mount, EXT2_ROOT_INO, &root_allocated) != 0 ||
	    read_inode(mount, EXT2_ROOT_INO, &root_inode) != 0 ||
	    root_allocated == 0 ||
	    ext2_inode_vfs_type(&root_inode) != BUNIX_VFS_TYPE_DIRECTORY ||
	    lookup_dirent(mount, &root_inode, ".", &self) != 0 ||
	    self.ino != EXT2_ROOT_INO ||
	    ext2_dirent_vfs_type(self.file_type) != BUNIX_VFS_TYPE_DIRECTORY) {
		bunix_free(mount->groups);
		mount->groups = 0;
		return -1;
	}
	log_super(&mount->super);
	return 0;
}

int main(void)
{
	struct bunix_msg message;
	const char online[] = "ext2: online\n";
	u64 block;
	u64 ext2_size;

	bunix_console_log(online, sizeof(online) - 1);
	bunix_u64_tree_init(&open_files);
	ext2_size = bunix_boot_module_size();
	root_mount.image_size = ext2_size;
	root_mount.use_boot_data = ext2_size != 0;
	if (root_mount.use_boot_data != 0) {
		root_mount.image_data = (unsigned char *)bunix_alloc(ext2_size);
		if (root_mount.image_data != 0 &&
		    bunix_boot_module_read(0, root_mount.image_data,
					   ext2_size) == 0 &&
		    load_initial_mount(&root_mount) == 0) {
			root_mount_ready = 1;
		}
	} else if (root_mount.use_boot_data == 0 &&
		   (block = resolve_service(BUNIX_SERVICE_BLOCK,
					    BUNIX_RIGHT_SEND)) != 0) {
		root_mount.block = block;
		if (load_initial_mount(&root_mount) == 0) {
			root_mount_ready = 1;
		}
	} else {
		log_text("ext2: block service unavailable\n");
	}
	if (register_service(BUNIX_SERVICE_EXT2, BUNIX_HANDLE_SELF) != 0) {
		return 1;
	}
	for (;;) {
		struct bunix_msg reply = {
			.protocol = BUNIX_PROTO_VFS,
			.type = 0,
			.sender = 0,
			.cap_rights = 0,
			.reply = 0,
			.cap = 0,
			.words = { 0, 0, 0, 0 },
		};
		char path[EXT2_MAX_PATH];

		if (bunix_ipc_recv(BUNIX_HANDLE_SELF, &message) != 0) {
			continue;
		}
		reply.type = message.type;
		if (message.protocol == BUNIX_PROTO_VFS) {
			switch (message.type) {
			case BUNIX_VFS_OPEN_BUFFER:
				if (read_resolved_path(&message, path) != 0) {
					reply.words[0] = BUNIX_VFS_ERR_NOENT;
				} else {
					reply_open(&message, &reply, path);
				}
				break;
			case BUNIX_VFS_STAT_PATH_META_BUFFER:
				if (read_resolved_path(&message, path) != 0) {
					reply.words[0] = BUNIX_VFS_ERR_NOENT;
				} else {
					reply_path_stat(&message, &reply, path);
				}
				break;
			case BUNIX_VFS_CREATE_BUFFER:
				if (read_resolved_path(&message, path) != 0) {
					reply.words[0] = BUNIX_VFS_ERR_NOENT;
				} else {
					reply_create(&reply, path,
						     message.words[3] >> 32);
				}
				break;
			case BUNIX_VFS_CHMOD_BUFFER:
				if (read_resolved_path(&message, path) != 0) {
					reply.words[0] = BUNIX_VFS_ERR_NOENT;
				} else {
					reply_path_chmod(&reply, path,
							 message.words[3] >> 32);
				}
				break;
			case BUNIX_VFS_CHOWN_BUFFER:
				if (read_resolved_path(&message, path) != 0) {
					reply.words[0] = BUNIX_VFS_ERR_NOENT;
				} else {
					reply_path_chown(&reply, path,
							 message.words[2],
							 message.words[3] >> 32);
				}
				break;
			case BUNIX_VFS_READLINK_BUFFER:
				if (read_resolved_path(&message, path) != 0) {
					reply.words[0] = BUNIX_VFS_ERR_NOENT;
				} else {
					reply_readlink(&message, &reply, path);
				}
				break;
			case BUNIX_VFS_STAT_META: {
				struct ext2_open *open =
					open_from_handle(message.words[0]);

				if (open == 0) {
					reply.words[0] = (u64)-1;
				} else {
					reply_stat_inode(&message, &reply,
							 open->ino,
							 &open->inode);
				}
				break;
			}
			case BUNIX_VFS_READDIR_BUFFER:
				reply_readdir(open_from_handle(message.words[0]),
					      &message, &reply);
				break;
			case BUNIX_VFS_READ_FILE_BUFFER:
				reply_read_file(open_from_handle(message.words[0]),
						&message, &reply);
				break;
			case BUNIX_VFS_WRITE_FILE_BUFFER:
				reply_write_file(open_from_handle(message.words[0]),
						 &message, &reply);
				break;
			case BUNIX_VFS_TRUNCATE:
				reply_truncate(open_from_handle(message.words[0]),
					       &message, &reply);
				break;
			case BUNIX_VFS_CHMOD:
				reply_chmod(open_from_handle(message.words[0]),
					    &message, &reply);
				break;
			case BUNIX_VFS_CHOWN:
				reply_chown(open_from_handle(message.words[0]),
					    &message, &reply);
				break;
			case BUNIX_VFS_CLOSE:
				if (open_from_handle(message.words[0]) == 0) {
					reply.words[0] = (u64)-1;
				} else {
					forget_open(message.words[0]);
					reply.words[0] = 0;
				}
				break;
			default:
				reply.words[0] = BUNIX_VFS_ERR_ACCESS;
				break;
			}
		} else if (message.protocol == BUNIX_PROTO_EXT2) {
			reply.protocol = BUNIX_PROTO_EXT2;
			reply.words[0] = (u64)-1;
		} else {
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			continue;
		}
		if (message.cap != 0) {
			bunix_handle_close(message.cap);
		}
		if (message.reply != 0) {
			bunix_ipc_send(message.reply, &reply);
		}
	}
}
