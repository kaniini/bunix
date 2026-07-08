#include <bunix/alloc.h>
#include <bunix/libbunix.h>
#include <bunix/tree.h>

enum {
	SQUASHFS_HANDLE_NAMES = 3,
	SQUASHFS_MAX_PATH = 4096,
	SQUASHFS_NAME_MAX = 255,
	SQUASHFS_MAGIC = 0x73717368,
	SQUASHFS_SUPER_SIZE = 96,
	SQUASHFS_MAJOR = 4,
	SQUASHFS_MAX_BLOCK_SIZE = 128 * 1024,
	SQUASHFS_METADATA_MAX = 8192,
	SQUASHFS_METADATA_HEADER = 2,
	SQUASHFS_METADATA_UNCOMPRESSED = 0x8000,
	SQUASHFS_METADATA_SIZE_MASK = 0x7fff,
	SQUASHFS_BLOCK_UNCOMPRESSED = 0x01000000,
	SQUASHFS_BLOCK_SIZE_MASK = 0x00ffffff,
	SQUASHFS_INVALID_TABLE = (u64)-1,
	SQUASHFS_INVALID_FRAGMENT = 0xffffffffUL,
	SQUASHFS_COMPRESSOR_MIN = 1,
	SQUASHFS_COMPRESSOR_MAX = 6,
	SQUASHFS_INODE_BASIC_DIR = 1,
	SQUASHFS_INODE_BASIC_FILE = 2,
	SQUASHFS_INODE_BASIC_SYMLINK = 3,
	SQUASHFS_INODE_BASIC_BLOCKDEV = 4,
	SQUASHFS_INODE_BASIC_CHARDEV = 5,
	SQUASHFS_INODE_EXT_DIR = 8,
	SQUASHFS_INODE_EXT_FILE = 9,
	SQUASHFS_INODE_EXT_SYMLINK = 10,
	SQUASHFS_INODE_EXT_BLOCKDEV = 11,
	SQUASHFS_INODE_EXT_CHARDEV = 12,
	SQUASHFS_OPEN_NODE = 1,
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

struct squashfs_inode {
	u64 ref;
	u64 type;
	u64 mode;
	u64 uid_index;
	u64 gid_index;
	u64 mtime;
	u64 ino;
	u64 nlink;
	u64 size;
	u64 block_start;
	u64 directory_offset;
	u64 fragment;
	u64 fragment_offset;
	u64 rdev;
	u64 xattr;
	u64 inline_offset;
	u64 inline_size;
	u64 header_size;
	u64 blocks_count;
	u64 *block_sizes;
};

struct squashfs_node {
	struct bunix_tree_node path_node;
	struct squashfs_node *parent;
	struct squashfs_node *first_child;
	struct squashfs_node *last_child;
	struct squashfs_node *next_sibling;
	struct squashfs_node *next_all;
	char *path;
	char *name;
	struct squashfs_inode inode;
	u64 dir_index;
};

struct squashfs_open {
	struct bunix_u64_tree_node node;
	u64 id;
	u64 kind;
	struct squashfs_node *fs_node;
};

static struct squashfs_super root_super;
static struct squashfs_metadata_cache *metadata_cache;
static struct squashfs_inode root_inode;
static struct bunix_tree nodes_by_path;
static struct bunix_u64_tree open_files;
static struct squashfs_node *root_node;
static struct squashfs_node *all_nodes;
static u64 *id_values;
static u64 node_count;
static u64 next_open_id = 1;
static u64 block_service;
static u64 user_service;
static u64 image_size;
static char squashfs_mount_path[SQUASHFS_MAX_PATH] = "/";

static int squashfs_is_directory_type(u64 type);

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

	while (text != 0 && len < SQUASHFS_MAX_PATH && text[len] != 0) {
		len++;
	}
	return len;
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

static int text_eq_len(const char *left, const char *right, u64 right_len)
{
	u64 i;

	if (left == 0 || right == 0) {
		return 0;
	}
	for (i = 0; i < right_len; i++) {
		if (left[i] == 0 || left[i] != right[i]) {
			return 0;
		}
	}
	return left[i] == 0;
}

static char *copy_text_len(const char *text, u64 len)
{
	char *copy;

	if (text == 0 || len >= SQUASHFS_MAX_PATH) {
		return 0;
	}
	copy = (char *)bunix_alloc(len + 1);
	if (copy == 0) {
		return 0;
	}
	for (u64 i = 0; i < len; i++) {
		copy[i] = text[i];
	}
	copy[len] = 0;
	return copy;
}

static int copy_path(char *target, const char *path)
{
	u64 i;

	if (target == 0 || path == 0 || path[0] != '/') {
		return -1;
	}
	for (i = 0; i + 1 < SQUASHFS_MAX_PATH && path[i] != 0; i++) {
		target[i] = path[i];
	}
	if (path[i] != 0) {
		return -1;
	}
	target[i] = 0;
	return 0;
}

static int set_mount_relative_path(char *out, const char *full_path)
{
	const u64 mount_len = text_len(squashfs_mount_path);

	if (copy_path(out, full_path) != 0) {
		return -1;
	}
	if (text_eq(squashfs_mount_path, "/")) {
		return 0;
	}
	for (u64 i = 0; i < mount_len; i++) {
		if (full_path[i] != squashfs_mount_path[i]) {
			return -1;
		}
	}
	if (full_path[mount_len] == 0) {
		out[0] = '/';
		out[1] = 0;
		return 0;
	}
	if (full_path[mount_len] != '/') {
		return -1;
	}
	out[0] = '/';
	for (u64 i = 1; i < SQUASHFS_MAX_PATH; i++) {
		out[i] = full_path[mount_len + i];
		if (out[i] == 0) {
			return 0;
		}
	}
	return -1;
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

static void log_index(void)
{
	char line[128];
	u64 offset = 0;

	line[0] = 0;
	append_text(line, sizeof(line), &offset, "squashfs: indexed nodes=");
	append_u64_dec(line, sizeof(line), &offset, node_count);
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
	(void)service;
	return bunix_names_register_claim(bunix_handle_find(BUNIX_CAP_CLAM),
					  handle);
}

static u64 service_user(void)
{
	if (user_service == 0) {
		user_service = resolve_service(BUNIX_SERVICE_USER,
					       BUNIX_RIGHT_SEND);
	}
	return user_service;
}

static int node_owner_mode(const struct squashfs_node *node, u64 *uid,
			   u64 *gid, u64 *mode)
{
	if (node == 0 || id_values == 0 ||
	    node->inode.uid_index >= root_super.ids ||
	    node->inode.gid_index >= root_super.ids) {
		return -1;
	}
	if (uid != 0) {
		*uid = id_values[node->inode.uid_index];
	}
	if (gid != 0) {
		*gid = id_values[node->inode.gid_index];
	}
	if (mode != 0) {
		*mode = node->inode.mode & 07777;
	}
	return 0;
}

static int user_can_access(u64 task, const struct squashfs_node *node, u64 mask)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_USER,
		.type = BUNIX_USER_CAN_ACCESS,
		.words = { task, 0, 0, 0 },
	};
	struct bunix_msg reply;
	const u64 user = service_user();
	u64 uid;
	u64 gid;
	u64 mode;

	if (mask == 0) {
		return node != 0;
	}
	if (task == 0) {
		return node != 0;
	}
	if (node_owner_mode(node, &uid, &gid, &mode) != 0 || user == 0) {
		return 0;
	}
	request.words[1] = uid;
	request.words[2] = gid;
	request.words[3] = (mode << 32) | mask;
	if (bunix_ipc_call(user, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return 0;
	}
	return reply.words[1] != 0;
}

static int node_can_open(u64 task, const struct squashfs_node *node)
{
	if (node == 0) {
		return 0;
	}
	if (squashfs_is_directory_type(node->inode.type)) {
		return user_can_access(task, node, 05);
	}
	return user_can_access(task, node, 04);
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

static long block_read_to_buffer(u64 offset, u64 buffer, u64 buffer_offset,
				 u64 len)
{
	u64 done = 0;

	while (done < len) {
		struct bunix_msg request = {
			.protocol = BUNIX_PROTO_BLOCK,
			.type = BUNIX_BLOCK_READ_BUFFER,
			.cap_rights = BUNIX_RIGHT_SEND,
			.cap = buffer,
			.words = { offset + done, len - done,
				   buffer_offset + done, 0 },
		};
		struct bunix_msg reply;

		if (bunix_ipc_call(block_service, &request, &reply) != 0 ||
		    reply.words[0] != 0 ||
		    reply.words[1] == 0 ||
		    reply.words[1] > len - done) {
			return -1;
		}
		done += reply.words[1];
	}
	return 0;
}

static int is_power_of_two(u64 value)
{
	return value != 0 && (value & (value - 1)) == 0;
}

static u64 div_round_up(u64 value, u64 divisor)
{
	return divisor == 0 ? 0 : (value + divisor - 1) / divisor;
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

static int squashfs_is_directory_type(u64 type)
{
	return type == SQUASHFS_INODE_BASIC_DIR ||
	       type == SQUASHFS_INODE_EXT_DIR;
}

static int squashfs_is_regular_type(u64 type)
{
	return type == SQUASHFS_INODE_BASIC_FILE ||
	       type == SQUASHFS_INODE_EXT_FILE;
}

static int squashfs_is_symlink_type(u64 type)
{
	return type == SQUASHFS_INODE_BASIC_SYMLINK ||
	       type == SQUASHFS_INODE_EXT_SYMLINK;
}

static u64 squashfs_vfs_type(u64 type)
{
	if (squashfs_is_directory_type(type)) {
		return BUNIX_VFS_TYPE_DIRECTORY;
	}
	if (squashfs_is_regular_type(type)) {
		return BUNIX_VFS_TYPE_REGULAR;
	}
	if (squashfs_is_symlink_type(type)) {
		return BUNIX_VFS_TYPE_SYMLINK;
	}
	if (type == SQUASHFS_INODE_BASIC_CHARDEV ||
	    type == SQUASHFS_INODE_EXT_CHARDEV) {
		return BUNIX_VFS_TYPE_CHARACTER;
	}
	return BUNIX_VFS_TYPE_REGULAR;
}

static int dir_name_valid(const char *name, u64 len)
{
	if (name == 0 || len == 0 || len > SQUASHFS_NAME_MAX) {
		return 0;
	}
	for (u64 i = 0; i < len; i++) {
		if (name[i] == 0 || name[i] == '/') {
			return 0;
		}
	}
	return 1;
}

static char *make_child_path(const char *parent, const char *name,
			     u64 name_len)
{
	const u64 parent_len = text_len(parent);
	const u64 slash = text_eq(parent, "/") ? 0 : 1;
	const u64 len = parent_len + slash + name_len;
	char *path;
	u64 offset = 0;

	if (parent == 0 || name == 0 || len == 0 || len >= SQUASHFS_MAX_PATH) {
		return 0;
	}
	path = (char *)bunix_alloc(len + 1);
	if (path == 0) {
		return 0;
	}
	for (u64 i = 0; i < parent_len; i++) {
		path[offset++] = parent[i];
	}
	if (slash != 0) {
		path[offset++] = '/';
	}
	for (u64 i = 0; i < name_len; i++) {
		path[offset++] = name[i];
	}
	path[offset] = 0;
	return path;
}

static struct squashfs_node *find_child(const struct squashfs_node *parent,
					const char *name, u64 name_len)
{
	struct squashfs_node *child;

	if (parent == 0) {
		return 0;
	}
	child = parent->first_child;
	while (child != 0) {
		if (text_eq_len(child->name, name, name_len)) {
			return child;
		}
		child = child->next_sibling;
	}
	return 0;
}

static struct squashfs_node *find_node(const char *path)
{
	return (struct squashfs_node *)bunix_tree_get(&nodes_by_path, path);
}

static long add_node(struct squashfs_node *parent, const char *name,
		     u64 name_len, const struct squashfs_inode *inode,
		     struct squashfs_node **out)
{
	struct squashfs_node *node;

	if (out == 0 || inode == 0 ||
	    (parent != 0 && !dir_name_valid(name, name_len)) ||
	    (parent != 0 && find_child(parent, name, name_len) != 0)) {
		log_mount_error("duplicate or invalid directory name",
				inode->ref);
		return -1;
	}
	node = (struct squashfs_node *)bunix_calloc(1, sizeof(*node));
	if (node == 0) {
		log_text("squashfs: mount rejected: node alloc failed\n");
		return -1;
	}
	node->parent = parent;
	node->inode = *inode;
	if (parent == 0) {
		node->name = copy_text_len("/", 1);
		node->path = copy_text_len("/", 1);
	} else {
		node->name = copy_text_len(name, name_len);
		node->path = make_child_path(parent->path, name, name_len);
	}
	if (node->name == 0 || node->path == 0 ||
	    bunix_tree_insert_node(&nodes_by_path, &node->path_node,
				   node->path, (u64)node) != 0) {
		log_mount_error("path index insert failed", inode->ref);
		return -1;
	}
	if (parent != 0) {
		node->dir_index = parent->last_child == 0 ? 0 :
				  parent->last_child->dir_index + 1;
		if (parent->last_child == 0) {
			parent->first_child = node;
		} else {
			parent->last_child->next_sibling = node;
		}
		parent->last_child = node;
	}
	node->next_all = all_nodes;
	all_nodes = node;
	node_count++;
	*out = node;
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

static long read_id_table(void)
{
	const u64 id_bytes = root_super.ids * 4;
	const u64 table_blocks = div_round_up(id_bytes, SQUASHFS_METADATA_MAX);
	unsigned char pointers_raw[64];
	unsigned char *ids_raw;

	if (root_super.ids == 0 ||
	    root_super.ids > 1024 ||
	    table_blocks == 0 ||
	    table_blocks * 8 > sizeof(pointers_raw)) {
		log_mount_error("unsupported id table size", root_super.ids);
		return -1;
	}
	if (block_read_bytes(block_service, root_super.id_table_start,
			     pointers_raw, table_blocks * 8) != 0) {
		log_mount_error("id table pointer read failed",
				root_super.id_table_start);
		return -1;
	}
	ids_raw = (unsigned char *)bunix_alloc(id_bytes);
	id_values = (u64 *)bunix_calloc(root_super.ids, sizeof(id_values[0]));
	if (ids_raw == 0 || id_values == 0) {
		log_text("squashfs: mount rejected: id table alloc failed\n");
		return -1;
	}
	for (u64 block = 0; block < table_blocks; block++) {
		const u64 pointer = load_le64(pointers_raw, block * 8);
		const u64 out_offset = block * SQUASHFS_METADATA_MAX;
		u64 chunk = id_bytes - out_offset;

		if (chunk > SQUASHFS_METADATA_MAX) {
			chunk = SQUASHFS_METADATA_MAX;
		}
		if (metadata_read_exact(pointer, 0, ids_raw + out_offset,
					chunk) != 0) {
			log_mount_error("id metadata read failed", pointer);
			return -1;
		}
	}
	for (u64 i = 0; i < root_super.ids; i++) {
		id_values[i] = load_le32(ids_raw, i * 4);
	}
	bunix_free(ids_raw);
	return 0;
}

static long decode_inode_at(u64 inode_ref, struct squashfs_inode *inode)
{
	unsigned char raw[64];
	u64 table_offset = inode_ref & 0xffff;
	u64 block_offset = inode_ref >> 16;
	u64 fixed_size;

	if (inode == 0) {
		return -1;
	}
	if (metadata_read_exact(root_super.inode_table_start + block_offset,
				table_offset, raw, sizeof(raw)) != 0) {
		log_mount_error("inode read failed", inode_ref);
		return -1;
	}

	inode->ref = inode_ref;
	inode->type = load_le16(raw, 0);
	inode->mode = load_le16(raw, 2);
	inode->uid_index = load_le16(raw, 4);
	inode->gid_index = load_le16(raw, 6);
	inode->mtime = load_le32(raw, 8);
	inode->ino = load_le32(raw, 12);
	inode->nlink = 1;
	inode->size = 0;
	inode->block_start = SQUASHFS_INVALID_TABLE;
	inode->directory_offset = 0;
	inode->fragment = 0xffffffffUL;
	inode->fragment_offset = 0;
	inode->rdev = 0;
	inode->xattr = 0xffffffffUL;
	inode->inline_offset = table_offset;
	inode->inline_size = 0;
	inode->header_size = 16;
	inode->blocks_count = 0;
	inode->block_sizes = 0;

	if (inode->uid_index >= root_super.ids ||
	    inode->gid_index >= root_super.ids) {
		log_mount_error("inode id index out of range", inode_ref);
		return -1;
	}

	switch (inode->type) {
	case SQUASHFS_INODE_BASIC_DIR:
		fixed_size = 32;
		inode->block_start = load_le32(raw, 16);
		inode->nlink = load_le32(raw, 20);
		inode->size = load_le16(raw, 24);
		inode->directory_offset = load_le16(raw, 26);
		break;
	case SQUASHFS_INODE_BASIC_FILE:
		fixed_size = 32;
		inode->block_start = load_le32(raw, 16);
		inode->fragment = load_le32(raw, 20);
		inode->fragment_offset = load_le32(raw, 24);
		inode->size = load_le32(raw, 28);
		break;
	case SQUASHFS_INODE_BASIC_SYMLINK:
		fixed_size = 24;
		inode->nlink = load_le32(raw, 16);
		inode->size = load_le32(raw, 20);
		inode->inline_offset = table_offset + fixed_size;
		inode->inline_size = inode->size;
		break;
	case SQUASHFS_INODE_BASIC_BLOCKDEV:
	case SQUASHFS_INODE_BASIC_CHARDEV:
		fixed_size = 24;
		inode->nlink = load_le32(raw, 16);
		inode->rdev = load_le32(raw, 20);
		break;
	case SQUASHFS_INODE_EXT_DIR:
		fixed_size = 40;
		inode->nlink = load_le32(raw, 16);
		inode->size = load_le32(raw, 20);
		inode->block_start = load_le32(raw, 24);
		inode->directory_offset = load_le16(raw, 34);
		inode->xattr = load_le32(raw, 36);
		break;
	case SQUASHFS_INODE_EXT_FILE:
		fixed_size = 56;
		inode->block_start = load_le64(raw, 16);
		inode->size = load_le64(raw, 24);
		inode->nlink = load_le32(raw, 40);
		inode->fragment = load_le32(raw, 44);
		inode->fragment_offset = load_le32(raw, 48);
		inode->xattr = load_le32(raw, 52);
		break;
	case SQUASHFS_INODE_EXT_SYMLINK:
		fixed_size = 24;
		inode->nlink = load_le32(raw, 16);
		inode->size = load_le32(raw, 20);
		inode->inline_offset = table_offset + fixed_size;
		inode->inline_size = inode->size;
		break;
	case SQUASHFS_INODE_EXT_BLOCKDEV:
	case SQUASHFS_INODE_EXT_CHARDEV:
		fixed_size = 28;
		inode->nlink = load_le32(raw, 16);
		inode->rdev = load_le32(raw, 20);
		inode->xattr = load_le32(raw, 24);
		break;
	default:
		log_mount_error("unsupported inode type", inode->type);
		return -1;
	}

	if (inode->nlink == 0) {
		log_mount_error("inode has zero links", inode_ref);
		return -1;
	}
	if (squashfs_is_regular_type(inode->type)) {
		if (inode->fragment != SQUASHFS_INVALID_FRAGMENT) {
			log_mount_error("file fragments unsupported", inode_ref);
			return -1;
		}
		inode->blocks_count = div_round_up(inode->size,
						   root_super.block_size);
		if (inode->blocks_count != 0) {
			u64 bytes;
			unsigned char *raw_sizes;

			if (inode->blocks_count > ((u64)-1) / 4) {
				log_mount_error("file block count overflow",
						inode_ref);
				return -1;
			}
			bytes = inode->blocks_count * 4;
			raw_sizes = (unsigned char *)bunix_alloc(bytes);
			inode->block_sizes = (u64 *)bunix_calloc(
				inode->blocks_count, sizeof(inode->block_sizes[0]));
			if (raw_sizes == 0 || inode->block_sizes == 0 ||
			    metadata_read_exact(root_super.inode_table_start +
						block_offset,
						table_offset + fixed_size,
						raw_sizes, bytes) != 0) {
				log_mount_error("file block list read failed",
						inode_ref);
				return -1;
			}
			for (u64 i = 0; i < inode->blocks_count; i++) {
				inode->block_sizes[i] = load_le32(raw_sizes, i * 4);
			}
			bunix_free(raw_sizes);
		}
	}
	if ((inode->type == SQUASHFS_INODE_BASIC_SYMLINK ||
	     inode->type == SQUASHFS_INODE_EXT_SYMLINK) &&
	    inode->size > 4096) {
		log_mount_error("symlink target too large", inode->size);
		return -1;
	}
	inode->header_size = fixed_size;
	return 0;
}

static long validate_root_inode(void)
{
	if (decode_inode_at(root_super.root_inode, &root_inode) != 0) {
		return -1;
	}
	if (root_inode.type != SQUASHFS_INODE_BASIC_DIR &&
	    root_inode.type != SQUASHFS_INODE_EXT_DIR) {
		log_mount_error("root inode is not directory",
				root_inode.type);
		return -1;
	}
	return 0;
}

static long index_directory(struct squashfs_node *directory)
{
	u64 dir_size;
	u64 consumed = 0;

	if (directory == 0 || !squashfs_is_directory_type(directory->inode.type)) {
		return -1;
	}
	if (directory->inode.size <= 3) {
		return 0;
	}
	dir_size = directory->inode.size - 3;
	while (consumed < dir_size) {
		unsigned char header[12];
		u64 count;
		u64 start_block;
		u64 inode_base;

		if (dir_size - consumed < sizeof(header) ||
		    metadata_read_exact(root_super.directory_table_start +
					directory->inode.block_start,
					directory->inode.directory_offset +
					consumed, header, sizeof(header)) != 0) {
			log_mount_error("bad directory header",
					directory->inode.ref);
			return -1;
		}
		count = load_le32(header, 0) + 1;
		start_block = load_le32(header, 4);
		inode_base = load_le32(header, 8);
		consumed += sizeof(header);
		if (count == 0 || count > 256) {
			log_mount_error("bad directory entry count", count);
			return -1;
		}
		for (u64 i = 0; i < count; i++) {
			unsigned char entry_raw[8 + SQUASHFS_NAME_MAX + 1];
			struct squashfs_inode inode;
			struct squashfs_node *child;
			u64 offset;
			u64 inode_delta;
			u64 type;
			u64 name_len;
			u64 entry_len;
			u64 inode_ref;

			if (dir_size - consumed < 8 ||
			    metadata_read_exact(root_super.directory_table_start +
						directory->inode.block_start,
						directory->inode.directory_offset +
						consumed, entry_raw, 8) != 0) {
				log_mount_error("bad directory entry",
						directory->inode.ref);
				return -1;
			}
			offset = load_le16(entry_raw, 0);
			inode_delta = load_le16(entry_raw, 2);
			type = load_le16(entry_raw, 4);
			name_len = load_le16(entry_raw, 6) + 1;
			entry_len = 8 + name_len;
			if (name_len == 0 ||
			    name_len > SQUASHFS_NAME_MAX ||
			    entry_len > sizeof(entry_raw) ||
			    entry_len > dir_size - consumed ||
			    metadata_read_exact(root_super.directory_table_start +
						directory->inode.block_start,
						directory->inode.directory_offset +
						consumed, entry_raw,
						entry_len) != 0 ||
			    !dir_name_valid((const char *)&entry_raw[8],
					    name_len)) {
				log_mount_error("bad directory name",
						directory->inode.ref);
				return -1;
			}
			(void)inode_base;
			(void)type;
			inode_ref = (start_block << 16) | offset;
			if (decode_inode_at(inode_ref, &inode) != 0 ||
			    add_node(directory, (const char *)&entry_raw[8],
				     name_len, &inode, &child) != 0) {
				return -1;
			}
			if (squashfs_is_directory_type(child->inode.type) &&
			    index_directory(child) != 0) {
				return -1;
			}
			(void)inode_delta;
			consumed += entry_len;
		}
	}
	return consumed == dir_size ? 0 : -1;
}

static long build_directory_index(void)
{
	if (add_node(0, "/", 1, &root_inode, &root_node) != 0) {
		return -1;
	}
	if (index_directory(root_node) != 0) {
		return -1;
	}
	log_index();
	return 0;
}

static int read_path_buffer_at(u64 buffer, u64 offset, u64 len, char *path)
{
	if (buffer == 0 || len == 0 || len > SQUASHFS_MAX_PATH) {
		return -1;
	}
	for (u64 i = 0; i < SQUASHFS_MAX_PATH; i++) {
		path[i] = 0;
	}
	if (bunix_buffer_read(buffer, offset, path, len) != 0 ||
	    path[len - 1] != 0) {
		return -1;
	}
	return text_len(path) < SQUASHFS_MAX_PATH ? 0 : -1;
}

static int read_resolved_path(const struct bunix_msg *message, char *path)
{
	char cwd[SQUASHFS_MAX_PATH];
	char full_path[SQUASHFS_MAX_PATH];

	if ((message->cap_rights & BUNIX_RIGHT_RECV) == 0 ||
	    read_path_buffer_at(message->cap, 0, message->words[0],
				cwd) != 0 ||
	    read_path_buffer_at(message->cap, message->words[0],
				message->words[1], full_path) != 0 ||
	    full_path[0] != '/') {
		return -1;
	}
	(void)cwd;
	return set_mount_relative_path(path, full_path);
}

static u64 remember_open(struct squashfs_node *node)
{
	struct squashfs_open *open;
	u64 id;

	open = (struct squashfs_open *)bunix_calloc(1, sizeof(*open));
	if (open == 0) {
		return 0;
	}
	id = next_open_id++;
	while (id == 0 || bunix_u64_tree_get(&open_files, id) != 0) {
		id = next_open_id++;
	}
	open->id = id;
	open->kind = SQUASHFS_OPEN_NODE;
	open->fs_node = node;
	if (bunix_u64_tree_insert_node(&open_files, &open->node, id,
				       (u64)open) != 0) {
		bunix_free(open);
		return 0;
	}
	return id;
}

static struct squashfs_open *open_from_handle(u64 handle)
{
	return (struct squashfs_open *)bunix_u64_tree_get(&open_files, handle);
}

static void forget_open(struct squashfs_open *open)
{
	if (open == 0) {
		return;
	}
	bunix_u64_tree_remove_node(&open_files, &open->node);
	bunix_free(open);
}

static void reply_open(struct bunix_msg *message, struct bunix_msg *reply,
		       const char *path)
{
	struct squashfs_node *node = find_node(path);
	u64 handle;

	(void)message;
	if (node == 0) {
		reply->words[0] = BUNIX_VFS_ERR_NOENT;
		return;
	}
	if (!node_can_open(message->words[3] & 0xffffffff, node)) {
		reply->words[0] = BUNIX_VFS_ERR_ACCESS;
		return;
	}
	handle = remember_open(node);
	if (handle == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	reply->words[0] = 0;
	reply->words[1] = handle;
	reply->words[2] = node->inode.size;
	reply->words[3] = squashfs_vfs_type(node->inode.type);
}

static void reply_readlink(const struct bunix_msg *message,
			   struct bunix_msg *reply, const char *path)
{
	struct squashfs_node *node = find_node(path);
	unsigned char target[SQUASHFS_MAX_PATH];
	const u64 out_cap = message->words[3] >> 32;
	u64 written;

	if (node == 0) {
		reply->words[0] = BUNIX_VFS_ERR_NOENT;
		return;
	}
	if (!user_can_access(message->words[3] & 0xffffffff, node, 04)) {
		reply->words[0] = BUNIX_VFS_ERR_ACCESS;
		return;
	}
	if (!squashfs_is_symlink_type(node->inode.type)) {
		reply->words[0] = BUNIX_VFS_ERR_INVAL;
		return;
	}
	if (node->inode.inline_size >= sizeof(target) ||
	    metadata_read_exact(root_super.inode_table_start +
				(node->inode.ref >> 16),
				node->inode.inline_offset, target,
				node->inode.inline_size) != 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	reply->words[0] = 0;
	reply->words[1] = node->inode.inline_size;
	if (out_cap == 0) {
		return;
	}
	if (message->cap == 0 ||
	    (message->cap_rights & BUNIX_RIGHT_SEND) == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	written = node->inode.inline_size;
	if (written > out_cap) {
		written = out_cap;
	}
	if (written != 0 &&
	    bunix_buffer_write(message->cap, message->words[0] +
			       message->words[1], target, written) != 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	reply->words[2] = node->inode.inline_size;
	reply->words[3] = written;
}

static u64 stat_buffer_offset(const struct bunix_msg *message)
{
	if (message->type == BUNIX_VFS_STAT_PATH_META_BUFFER) {
		return message->words[0] + message->words[1];
	}
	return message->words[1];
}

static u64 node_rdev(const struct squashfs_node *node)
{
	if (node == 0 ||
	    (node->inode.type != SQUASHFS_INODE_BASIC_CHARDEV &&
	     node->inode.type != SQUASHFS_INODE_EXT_CHARDEV &&
	     node->inode.type != SQUASHFS_INODE_BASIC_BLOCKDEV &&
	     node->inode.type != SQUASHFS_INODE_EXT_BLOCKDEV)) {
		return 0;
	}
	return node->inode.rdev;
}

static void reply_stat_node(const struct bunix_msg *message,
			    struct bunix_msg *reply,
			    const struct squashfs_node *node)
{
	u64 uid;
	u64 gid;
	u64 mode_type;
	u64 owner;
	u64 blocks;

	if (node == 0 ||
	    node->inode.uid_index >= root_super.ids ||
	    node->inode.gid_index >= root_super.ids ||
	    id_values == 0) {
		reply->words[0] = BUNIX_VFS_ERR_NOENT;
		return;
	}
	uid = id_values[node->inode.uid_index];
	gid = id_values[node->inode.gid_index];
	mode_type = (node->inode.mode & 07777) |
		    (squashfs_vfs_type(node->inode.type) << 32);
	owner = uid | (gid << 32);
	blocks = (node->inode.size + 511) / 512;
	reply->words[0] = 0;
	reply->words[1] = node->inode.size;
	reply->words[2] = mode_type;
	reply->words[3] = owner;
	if (message->cap != 0 &&
	    (message->cap_rights & BUNIX_RIGHT_SEND) != 0) {
		(void)bunix_vfs_stat_write_times(
			message->cap, stat_buffer_offset(message),
			node->inode.size, mode_type, owner,
			BUNIX_VFS_DEV_SQUASHFS, node->inode.ino,
			node->inode.nlink, node_rdev(node),
			root_super.block_size, blocks, node->inode.mtime,
			node->inode.mtime, node->inode.mtime);
	}
}

static void reply_path_stat(const struct bunix_msg *message,
			    struct bunix_msg *reply, const char *path)
{
	struct squashfs_node *node = find_node(path);

	if (node == 0) {
		reply->words[0] = BUNIX_VFS_ERR_NOENT;
		return;
	}
	reply_stat_node(message, reply, node);
}

static void reply_access(const struct bunix_msg *message,
			 struct bunix_msg *reply, const char *path)
{
	struct squashfs_node *node = find_node(path);
	const u64 task = message->words[3] & 0xffffffff;
	const u64 mask = message->words[3] >> 32;

	if (node == 0) {
		reply->words[0] = BUNIX_VFS_ERR_NOENT;
		return;
	}
	reply->words[0] = user_can_access(task, node, mask) ? 0 :
			  BUNIX_VFS_ERR_ACCESS;
}

static void reply_readdir_buffer(struct squashfs_open *open,
				 const struct bunix_msg *message,
				 struct bunix_msg *reply)
{
	struct squashfs_node *child;

	if (open == 0 || open->kind != SQUASHFS_OPEN_NODE ||
	    open->fs_node == 0 ||
	    !squashfs_is_directory_type(open->fs_node->inode.type)) {
		reply->words[0] = BUNIX_VFS_ERR_NOTDIR;
		return;
	}
	if (message->cap == 0 ||
	    (message->cap_rights & BUNIX_RIGHT_SEND) == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	child = open->fs_node->first_child;
	while (child != 0 && child->dir_index < message->words[1]) {
		child = child->next_sibling;
	}
	if (child == 0 || child->dir_index != message->words[1]) {
		reply->words[0] = BUNIX_VFS_ERR_NOENT;
		return;
	}
	const u64 name_len = text_len(child->name);
	u64 written = name_len;

	if (written > message->words[3]) {
		written = message->words[3];
	}
	if (written != 0 &&
	    bunix_buffer_write(message->cap, message->words[2], child->name,
			       written) != 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	reply->words[0] = 0;
	reply->words[1] = (child->dir_index + 1) |
			  (squashfs_vfs_type(child->inode.type) << 32);
	reply->words[2] = name_len;
	reply->words[3] = written;
}

static long zero_buffer_range(u64 buffer, u64 offset, u64 len)
{
	unsigned char zeros[128];
	u64 done = 0;

	for (u64 i = 0; i < sizeof(zeros); i++) {
		zeros[i] = 0;
	}
	while (done < len) {
		u64 chunk = len - done;

		if (chunk > sizeof(zeros)) {
			chunk = sizeof(zeros);
		}
		if (bunix_buffer_write(buffer, offset + done, zeros, chunk) != 0) {
			return -1;
		}
		done += chunk;
	}
	return 0;
}

static u64 file_data_disk_offset(const struct squashfs_inode *inode,
				 u64 block_index)
{
	u64 offset = inode->block_start;

	for (u64 i = 0; i < block_index; i++) {
		offset += inode->block_sizes[i] & SQUASHFS_BLOCK_SIZE_MASK;
	}
	return offset;
}

static void reply_read_file_buffer(struct squashfs_open *open,
				   const struct bunix_msg *message,
				   struct bunix_msg *reply)
{
	const struct squashfs_inode *inode;
	u64 file_offset = message->words[1];
	u64 len = message->words[2];
	u64 done = 0;

	if (open == 0 || open->kind != SQUASHFS_OPEN_NODE ||
	    open->fs_node == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	inode = &open->fs_node->inode;
	if (!squashfs_is_regular_type(inode->type)) {
		reply->words[0] = squashfs_is_directory_type(inode->type) ?
				  BUNIX_VFS_ERR_ISDIR : BUNIX_VFS_ERR_INVAL;
		return;
	}
	if (message->cap == 0 ||
	    (message->cap_rights & BUNIX_RIGHT_SEND) == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	if (file_offset >= inode->size) {
		reply->words[0] = 0;
		reply->words[1] = 0;
		return;
	}
	if (len > inode->size - file_offset) {
		len = inode->size - file_offset;
	}
	while (done < len) {
		const u64 logical_offset = file_offset + done;
		const u64 block_index = logical_offset / root_super.block_size;
		const u64 block_offset = logical_offset % root_super.block_size;
		u64 logical_block_size = root_super.block_size;
		u64 chunk;
		u64 stored;

		if (block_index >= inode->blocks_count ||
		    inode->block_sizes == 0) {
			reply->words[0] = (u64)-1;
			reply->words[1] = done;
			return;
		}
		if (block_index + 1 == inode->blocks_count) {
			const u64 tail = inode->size -
					 block_index * root_super.block_size;

			if (tail < logical_block_size) {
				logical_block_size = tail;
			}
		}
		chunk = logical_block_size - block_offset;
		if (chunk > len - done) {
			chunk = len - done;
		}
		stored = inode->block_sizes[block_index];
		if (stored == 0) {
			if (zero_buffer_range(message->cap, done,
					      chunk) != 0) {
				reply->words[0] = (u64)-1;
				reply->words[1] = done;
				return;
			}
		} else {
			const u64 stored_size = stored & SQUASHFS_BLOCK_SIZE_MASK;

			if ((stored & SQUASHFS_BLOCK_UNCOMPRESSED) == 0 ||
			    stored_size < logical_block_size) {
				reply->words[0] = (u64)-1;
				reply->words[1] = done;
				return;
			}
			if (block_read_to_buffer(file_data_disk_offset(inode,
								       block_index) +
						 block_offset, message->cap,
						 done, chunk) != 0) {
				reply->words[0] = (u64)-1;
				reply->words[1] = done;
				return;
			}
		}
		done += chunk;
	}
	reply->words[0] = 0;
	reply->words[1] = done;
}

static void forward_open_handle(struct bunix_msg *message,
				struct bunix_msg *reply)
{
	struct squashfs_open *open = open_from_handle(message->words[0]);

	if (open == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	if (message->type == BUNIX_VFS_CLOSE) {
		forget_open(open);
		reply->words[0] = 0;
	} else if (message->type == BUNIX_VFS_STAT_META) {
		reply_stat_node(message, reply, open->fs_node);
	} else if (message->type == BUNIX_VFS_READDIR_BUFFER) {
		reply_readdir_buffer(open, message, reply);
	} else if (message->type == BUNIX_VFS_READ_FILE_BUFFER) {
		reply_read_file_buffer(open, message, reply);
	} else {
		reply->words[0] = BUNIX_VFS_ERR_INVAL;
	}
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
	if (read_id_table() != 0) {
		return -1;
	}
	if (validate_root_inode() != 0) {
		return -1;
	}
	if (build_directory_index() != 0) {
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
	bunix_tree_init(&nodes_by_path);
	bunix_u64_tree_init(&open_files);
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
		char path[SQUASHFS_MAX_PATH];

		if (bunix_ipc_recv(BUNIX_HANDLE_SELF, &message) != 0) {
			continue;
		}
		if (message.protocol == BUNIX_PROTO_SQUASHFS &&
		    message.type == BUNIX_SQUASHFS_MOUNT_PATH) {
			reply.protocol = BUNIX_PROTO_SQUASHFS;
			reply.type = message.type;
			if (bunix_read_path_cap(&message, squashfs_mount_path,
						sizeof(squashfs_mount_path)) != 0) {
				squashfs_mount_path[0] = '/';
				squashfs_mount_path[1] = 0;
				reply.words[0] = (u64)-1;
			} else {
				reply.words[0] = 0;
			}
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			if (message.reply != 0) {
				bunix_ipc_send(message.reply, &reply);
			}
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
			case BUNIX_VFS_READLINK_BUFFER:
				if (read_resolved_path(&message, path) != 0) {
					reply.words[0] = BUNIX_VFS_ERR_NOENT;
				} else {
					reply_readlink(&message, &reply, path);
				}
				break;
			case BUNIX_VFS_STAT_PATH_META_BUFFER:
				if (read_resolved_path(&message, path) != 0) {
					reply.words[0] = BUNIX_VFS_ERR_NOENT;
				} else {
					reply_path_stat(&message, &reply, path);
				}
				break;
			case BUNIX_VFS_ACCESS_BUFFER:
				if (read_resolved_path(&message, path) != 0) {
					reply.words[0] = BUNIX_VFS_ERR_NOENT;
				} else {
					reply_access(&message, &reply, path);
				}
				break;
			case BUNIX_VFS_STAT_META:
			case BUNIX_VFS_READDIR_BUFFER:
			case BUNIX_VFS_READ_FILE_BUFFER:
			case BUNIX_VFS_CLOSE:
				forward_open_handle(&message, &reply);
				break;
			case BUNIX_VFS_CREATE_BUFFER:
			case BUNIX_VFS_WRITE_FILE_BUFFER:
			case BUNIX_VFS_TRUNCATE:
			case BUNIX_VFS_UNLINK_BUFFER:
			case BUNIX_VFS_CHMOD_BUFFER:
			case BUNIX_VFS_CHOWN_BUFFER:
			case BUNIX_VFS_MKDIR_BUFFER:
			case BUNIX_VFS_RMDIR_BUFFER:
			case BUNIX_VFS_MKNOD_BUFFER:
			case BUNIX_VFS_SYMLINK_BUFFER:
			case BUNIX_VFS_RENAME_BUFFER:
			case BUNIX_VFS_LINK_BUFFER:
			case BUNIX_VFS_CHMOD:
			case BUNIX_VFS_CHOWN:
				reply.words[0] = BUNIX_VFS_ERR_ACCESS;
				break;
			default:
				reply.words[0] = BUNIX_VFS_ERR_INVAL;
				break;
			}
		}
		if (message.cap != 0) {
			bunix_handle_close(message.cap);
		}
		if (message.protocol == BUNIX_PROTO_VFS &&
		    message.reply != 0) {
			bunix_ipc_send(message.reply, &reply);
		}
	}
}
