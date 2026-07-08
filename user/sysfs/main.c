#include <bunix/libbunix.h>

enum {
	SYSFS_HANDLE_NAMES = 3,
	SYSFS_MAX_PATH = 4096,
};

struct sysfs_node {
	const char *path;
	const char *name;
	const char *parent;
	u64 type;
	const char *content;
};

static const struct sysfs_node nodes[] = {
	{ "/sys", "sys", "", BUNIX_VFS_TYPE_DIRECTORY, 0 },
	{ "/sys/block", "block", "/sys", BUNIX_VFS_TYPE_DIRECTORY, 0 },
	{ "/sys/class", "class", "/sys", BUNIX_VFS_TYPE_DIRECTORY, 0 },
	{ "/sys/devices", "devices", "/sys", BUNIX_VFS_TYPE_DIRECTORY, 0 },
	{ "/sys/kernel", "kernel", "/sys", BUNIX_VFS_TYPE_DIRECTORY, 0 },
	{ "/sys/module", "module", "/sys", BUNIX_VFS_TYPE_DIRECTORY, 0 },
	{ "/sys/class/tty", "tty", "/sys/class", BUNIX_VFS_TYPE_DIRECTORY, 0 },
	{ "/sys/class/tty/console", "console", "/sys/class/tty", BUNIX_VFS_TYPE_DIRECTORY, 0 },
	{ "/sys/class/tty/tty", "tty", "/sys/class/tty", BUNIX_VFS_TYPE_DIRECTORY, 0 },
	{ "/sys/class/tty/ttyS0", "ttyS0", "/sys/class/tty", BUNIX_VFS_TYPE_DIRECTORY, 0 },
	{ "/sys/devices/system", "system", "/sys/devices", BUNIX_VFS_TYPE_DIRECTORY, 0 },
	{ "/sys/devices/system/cpu", "cpu", "/sys/devices/system", BUNIX_VFS_TYPE_DIRECTORY, 0 },
	{ "/sys/devices/system/cpu/cpu0", "cpu0", "/sys/devices/system/cpu", BUNIX_VFS_TYPE_DIRECTORY, 0 },
	{ "/sys/devices/system/cpu/cpu1", "cpu1", "/sys/devices/system/cpu", BUNIX_VFS_TYPE_DIRECTORY, 0 },
	{ "/sys/devices/system/cpu/online", "online", "/sys/devices/system/cpu", BUNIX_VFS_TYPE_REGULAR, "0-1\n" },
	{ "/sys/devices/system/cpu/possible", "possible", "/sys/devices/system/cpu", BUNIX_VFS_TYPE_REGULAR, "0-1\n" },
	{ "/sys/devices/system/cpu/present", "present", "/sys/devices/system/cpu", BUNIX_VFS_TYPE_REGULAR, "0-1\n" },
	{ "/sys/devices/system/cpu/cpu0/online", "online", "/sys/devices/system/cpu/cpu0", BUNIX_VFS_TYPE_REGULAR, "1\n" },
	{ "/sys/devices/system/cpu/cpu1/online", "online", "/sys/devices/system/cpu/cpu1", BUNIX_VFS_TYPE_REGULAR, "1\n" },
};

static u64 vfs_service;

static u64 str_len(const char *text)
{
	u64 len = 0;

	while (text[len] != '\0') {
		len++;
	}
	return len;
}

static int str_eq(const char *left, const char *right)
{
	while (*left != '\0' && *right != '\0') {
		if (*left++ != *right++) {
			return 0;
		}
	}
	return *left == '\0' && *right == '\0';
}

static int read_path_buffer_at(u64 buffer, u64 offset, u64 len, char *path)
{
	if (buffer == 0 || len == 0 || len > SYSFS_MAX_PATH) {
		return -1;
	}
	for (u64 i = 0; i < SYSFS_MAX_PATH; i++) {
		path[i] = '\0';
	}
	if (bunix_buffer_read(buffer, offset, path, len) != 0 ||
	    path[len - 1] != '\0') {
		return -1;
	}
	return 0;
}

static int read_resolved_path(const struct bunix_msg *message, char *path)
{
	char cwd[SYSFS_MAX_PATH];

	if ((message->cap_rights & BUNIX_RIGHT_RECV) == 0 ||
	    read_path_buffer_at(message->cap, 0, message->words[0], cwd) != 0 ||
	    read_path_buffer_at(message->cap, message->words[0],
				message->words[1], path) != 0 ||
	    path[0] != '/') {
		return -1;
	}
	(void)cwd;
	return 0;
}

static u64 node_count(void)
{
	return sizeof(nodes) / sizeof(nodes[0]);
}

static u64 node_handle(u64 index)
{
	return index + 1;
}

static const struct sysfs_node *node_from_handle(u64 handle)
{
	if (handle == 0 || handle > node_count()) {
		return 0;
	}
	return &nodes[handle - 1];
}

static const struct sysfs_node *node_for_path(const char *path)
{
	for (u64 i = 0; i < node_count(); i++) {
		if (str_eq(path, nodes[i].path)) {
			return &nodes[i];
		}
	}
	return 0;
}

static u64 handle_for_node(const struct sysfs_node *node)
{
	if (node == 0) {
		return 0;
	}
	return node_handle((u64)(node - nodes));
}

static const struct sysfs_node *child_at(const struct sysfs_node *parent,
					 u64 offset)
{
	u64 seen = 0;

	if (parent == 0 || parent->type != BUNIX_VFS_TYPE_DIRECTORY) {
		return 0;
	}
	for (u64 i = 0; i < node_count(); i++) {
		if (str_eq(nodes[i].parent, parent->path)) {
			if (seen == offset) {
				return &nodes[i];
			}
			seen++;
		}
	}
	return 0;
}

static u64 node_size(const struct sysfs_node *node)
{
	if (node == 0 || node->content == 0) {
		return 0;
	}
	return str_len(node->content);
}

static u64 stat_buffer_offset(const struct bunix_msg *message)
{
	if (message->type == BUNIX_VFS_STAT_PATH_META_BUFFER) {
		return message->words[0] + message->words[1];
	}
	return message->words[1];
}

static void stat_node(const struct bunix_msg *message, struct bunix_msg *reply,
		      const struct sysfs_node *node)
{
	const u64 size = node_size(node);

	reply->words[0] = 0;
	reply->words[1] = size;
	reply->words[2] = ((u64)node->type << 32) |
			  (node->type == BUNIX_VFS_TYPE_DIRECTORY ? 0555 : 0444);
	reply->words[3] = 0;
	if (message->cap != 0 &&
	    (message->cap_rights & BUNIX_RIGHT_SEND) != 0) {
		(void)bunix_vfs_stat_write(
			message->cap, stat_buffer_offset(message), size,
			reply->words[2], 0, BUNIX_VFS_DEV_SYSFS,
			handle_for_node(node), 1, 0, 4096,
			(size + 511) / 512);
	}
}

static long mount_sysfs(u64 vfs, const char *path)
{
	struct bunix_msg reply;

	return bunix_ipc_call_path(vfs, BUNIX_PROTO_VFS,
				   BUNIX_VFS_MOUNT_BUFFER, path,
				   BUNIX_SERVICE_SYSFS, 0, 0, &reply) == 0 &&
	       reply.words[0] == 0 ? 0 : -1;
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

	if (bunix_ipc_call(SYSFS_HANDLE_NAMES, &request, &reply) != 0 ||
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

int main(void)
{
	const char online[] = "sysfs: online\n";
	const char mounted[] = "sysfs: mounted\n";
	struct bunix_msg message;

	bunix_console_log(online, sizeof(online) - 1);
	vfs_service = resolve_service(BUNIX_SERVICE_VFS, BUNIX_RIGHT_SEND);
	if (vfs_service == 0 ||
	    register_service(BUNIX_SERVICE_SYSFS, BUNIX_HANDLE_SELF) != 0) {
		return 1;
	}

	for (;;) {
		struct bunix_msg reply = {
			.protocol = BUNIX_PROTO_VFS,
			.type = message.type,
			.sender = 0,
			.cap_rights = 0,
			.reply = 0,
			.cap = 0,
			.words = { 0, 0, 0, 0 },
		};
		char path[SYSFS_MAX_PATH];
		const struct sysfs_node *node;

		if (bunix_ipc_recv(BUNIX_HANDLE_SELF, &message) != 0) {
			continue;
		}
		reply.type = message.type;

		if (message.protocol == BUNIX_PROTO_SYSFS) {
			reply.protocol = BUNIX_PROTO_SYSFS;
			switch (message.type) {
			case BUNIX_SYSFS_MOUNT_PATH:
				reply.words[0] =
					bunix_read_path_cap(&message, path,
							    sizeof(path)) == 0 ?
					(u64)mount_sysfs(vfs_service, path) :
					(u64)-1;
				if (reply.words[0] == 0) {
					bunix_console_log(mounted,
							  sizeof(mounted) - 1);
				}
				break;
			default:
				reply.words[0] = (u64)-1;
				break;
			}
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			bunix_ipc_send(message.reply, &reply);
			continue;
		}
		if (message.protocol != BUNIX_PROTO_VFS) {
			continue;
		}

		switch (message.type) {
		case BUNIX_VFS_OPEN_BUFFER:
			if (read_resolved_path(&message, path) != 0 ||
			    (node = node_for_path(path)) == 0) {
				reply.words[0] = BUNIX_VFS_ERR_NOENT;
				break;
			}
			reply.words[0] = 0;
			reply.words[1] = handle_for_node(node);
			reply.words[2] = node_size(node);
			reply.words[3] = node->type;
			break;
		case BUNIX_VFS_STAT_PATH_META_BUFFER:
		case BUNIX_VFS_ACCESS_BUFFER:
			if (read_resolved_path(&message, path) != 0 ||
			    (node = node_for_path(path)) == 0) {
				reply.words[0] = BUNIX_VFS_ERR_NOENT;
				break;
			}
			stat_node(&message, &reply, node);
			break;
		case BUNIX_VFS_STAT_META:
			node = node_from_handle(message.words[0]);
			if (node == 0) {
				reply.words[0] = BUNIX_VFS_ERR_NOENT;
			} else {
				stat_node(&message, &reply, node);
			}
			break;
		case BUNIX_VFS_READ_FILE_BUFFER: {
			u64 nread;

			node = node_from_handle(message.words[0]);
			if (node == 0 || node->type != BUNIX_VFS_TYPE_REGULAR ||
			    message.cap == 0 ||
			    (message.cap_rights & BUNIX_RIGHT_SEND) == 0) {
				reply.words[0] = (u64)-1;
				break;
			}
			nread = node_size(node);
			if (message.words[1] >= nread) {
				reply.words[0] = 0;
				reply.words[1] = 0;
				break;
			}
			nread -= message.words[1];
			if (nread > message.words[2]) {
				nread = message.words[2];
			}
			reply.words[0] =
				bunix_buffer_write(message.cap, 0,
						   node->content +
						   message.words[1],
						   nread) == 0 ?
				0 : (u64)-1;
			reply.words[1] = reply.words[0] == 0 ? nread : 0;
			break;
		}
		case BUNIX_VFS_READDIR_BUFFER:
			node = child_at(node_from_handle(message.words[0]),
					message.words[1]);
			if (node == 0) {
				reply.words[0] = BUNIX_VFS_ERR_NOENT;
				break;
			}
			if (message.cap == 0 ||
			    (message.cap_rights & BUNIX_RIGHT_SEND) == 0) {
				reply.words[0] = (u64)-1;
				break;
			}
			reply.words[0] = 0;
			reply.words[1] = handle_for_node(node) |
					 ((u64)node->type << 32);
			reply.words[2] = str_len(node->name);
			reply.words[3] = reply.words[2];
			if (reply.words[3] > message.words[3]) {
				reply.words[3] = message.words[3];
			}
			if (reply.words[3] != 0 &&
			    bunix_buffer_write(message.cap, message.words[2],
					       node->name, reply.words[3]) != 0) {
				reply.words[0] = (u64)-1;
			}
			break;
		case BUNIX_VFS_CLOSE:
			reply.words[0] = 0;
			break;
		default:
			reply.words[0] = (u64)-1;
			break;
		}

		bunix_ipc_send(message.reply, &reply);
	}
}
