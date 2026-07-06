#include <bunix/libbunix.h>

enum {
	DEVFS_HANDLE_NAMES = 3,
	DEVFS_MAX_PATH = 4096,
	DEVFS_DIR = 1,
	DEVFS_NULL = 2,
	DEVFS_ZERO = 3,
	DEVFS_RANDOM = 4,
	DEVFS_TTY = 5,
	DEVFS_CONSOLE = 6,
	LINUX_S_IFCHR = 0020000,
};

struct devfs_entry {
	const char *name;
	u64 device;
};

static u64 random_state = 0x6465766673726e67ull;
static u64 vfs_service;

static u64 str_len(const char *text)
{
	u64 len = 0;

	while (len < DEVFS_MAX_PATH && text[len] != '\0') {
		len++;
	}
	return len;
}

static int str_eq(const char *left, const char *right)
{
	u64 i = 0;

	while (left[i] != '\0' && right[i] != '\0') {
		if (left[i] != right[i]) {
			return 0;
		}
		i++;
	}
	return left[i] == right[i];
}

static int read_path_buffer_at(u64 buffer, u64 offset, u64 len, char *path)
{
	if (buffer == 0 || len == 0 || len > DEVFS_MAX_PATH) {
		return -1;
	}
	for (u64 i = 0; i < DEVFS_MAX_PATH; i++) {
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
	char cwd[DEVFS_MAX_PATH];

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

static u64 device_for_path(const char *path)
{
	if (str_eq(path, "/dev")) {
		return DEVFS_DIR;
	}
	if (str_eq(path, "/dev/null")) {
		return DEVFS_NULL;
	}
	if (str_eq(path, "/dev/zero")) {
		return DEVFS_ZERO;
	}
	if (str_eq(path, "/dev/random") || str_eq(path, "/dev/urandom")) {
		return DEVFS_RANDOM;
	}
	if (str_eq(path, "/dev/tty")) {
		return DEVFS_TTY;
	}
	if (str_eq(path, "/dev/ttyS0")) {
		return DEVFS_TTY;
	}
	if (str_eq(path, "/dev/console")) {
		return DEVFS_CONSOLE;
	}
	return 0;
}

static void stat_device(struct bunix_msg *reply, u64 device)
{
	reply->words[0] = 0;
	reply->words[1] = 0;
	reply->words[2] = device == DEVFS_DIR ?
			  (((u64)BUNIX_VFS_TYPE_DIRECTORY << 32) | 0555) :
			  (((u64)BUNIX_VFS_TYPE_CHARACTER << 32) |
			   (device == DEVFS_CONSOLE ? 0600 : 0666));
	reply->words[3] = 0;
}

static long read_generated_device(u64 device, u64 buffer, u64 len)
{
	char data[256];
	u64 done = 0;

	while (done < len) {
		u64 chunk = len - done;

		if (chunk > sizeof(data)) {
			chunk = sizeof(data);
		}
		for (u64 i = 0; i < chunk; i++) {
			data[i] = 0;
			if (device == DEVFS_RANDOM) {
				random_state =
					random_state * 6364136223846793005ull + 1;
				data[i] = (char)(random_state >> 56);
			}
		}
		if (bunix_buffer_write(buffer, done, data, chunk) != 0) {
			return -1;
		}
		done += chunk;
	}
	return 0;
}

static long write_console_device(u64 buffer, u64 len)
{
	char data[256];
	u64 done = 0;

	if (buffer == 0) {
		return -1;
	}
	while (done < len) {
		u64 chunk = len - done;

		if (chunk > sizeof(data)) {
			chunk = sizeof(data);
		}
		if (bunix_buffer_read(buffer, done, data, chunk) != 0) {
			return -1;
		}
		bunix_console_write(data, chunk);
		done += chunk;
	}
	return 0;
}

static u64 mount_devfs(u64 vfs, const char *path)
{
	struct bunix_msg reply;

	return bunix_ipc_call_path(vfs, BUNIX_PROTO_VFS,
				   BUNIX_VFS_MOUNT_BUFFER, path,
				   BUNIX_SERVICE_DEVFS, 0, 0, &reply) == 0 &&
	       reply.words[0] == 0 ? 0 : (u64)-1;
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

	if (bunix_ipc_call(DEVFS_HANDLE_NAMES, &request, &reply) != 0 ||
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

	if (bunix_ipc_call(DEVFS_HANDLE_NAMES, &request, &reply) != 0) {
		return -1;
	}
	return reply.words[0] == 0 ? 0 : -1;
}

int main(void)
{
	const char online[] = "devfs: online\n";
	const char mounted[] = "devfs: mounted\n";
	const struct devfs_entry entries[] = {
		{ "console", DEVFS_CONSOLE },
		{ "null", DEVFS_NULL },
		{ "random", DEVFS_RANDOM },
		{ "tty", DEVFS_TTY },
		{ "ttyS0", DEVFS_TTY },
		{ "urandom", DEVFS_RANDOM },
		{ "zero", DEVFS_ZERO },
	};
	struct bunix_msg message;

	bunix_console_log(online, sizeof(online) - 1);
	vfs_service = resolve_service(BUNIX_SERVICE_VFS, BUNIX_RIGHT_SEND);
	if (vfs_service == 0 ||
	    register_service(BUNIX_SERVICE_DEVFS, BUNIX_HANDLE_SELF) != 0) {
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
		char path[DEVFS_MAX_PATH];
		u64 device;

		if (bunix_ipc_recv(BUNIX_HANDLE_SELF, &message) != 0) {
			continue;
		}
		reply.type = message.type;

		if (message.protocol == BUNIX_PROTO_DEVFS) {
			reply.protocol = BUNIX_PROTO_DEVFS;
			switch (message.type) {
			case BUNIX_DEVFS_MOUNT_PATH:
				reply.words[0] =
					bunix_read_path_cap(&message, path,
							    sizeof(path)) == 0 ?
					(u64)mount_devfs(vfs_service, path) :
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
		reply.protocol = BUNIX_PROTO_VFS;

		switch (message.type) {
		case BUNIX_VFS_OPEN_BUFFER:
				if (read_resolved_path(&message, path) != 0 ||
				    (device = device_for_path(path)) == 0) {
					reply.words[0] = BUNIX_VFS_ERR_NOENT;
					break;
				}
				reply.words[0] = 0;
				reply.words[1] = device;
				reply.words[2] = device == DEVFS_ZERO ||
						 device == DEVFS_RANDOM ?
						 (u64)-1 : 0;
				reply.words[3] = device == DEVFS_DIR ?
						 BUNIX_VFS_TYPE_DIRECTORY :
						 BUNIX_VFS_TYPE_CHARACTER;
				break;
		case BUNIX_VFS_STAT_PATH_META_BUFFER:
		case BUNIX_VFS_ACCESS_BUFFER:
			if (read_resolved_path(&message, path) != 0 ||
			    (device = device_for_path(path)) == 0) {
				reply.words[0] = BUNIX_VFS_ERR_NOENT;
				break;
			}
			stat_device(&reply, device);
			break;
		case BUNIX_VFS_STAT_META:
			device = message.words[0];
			if (device < DEVFS_DIR || device > DEVFS_CONSOLE) {
				reply.words[0] = BUNIX_VFS_ERR_NOENT;
			} else {
				stat_device(&reply, device);
			}
			break;
		case BUNIX_VFS_READ_FILE_BUFFER:
			device = message.words[0];
			if (message.cap == 0 ||
			    (message.cap_rights & BUNIX_RIGHT_SEND) == 0 ||
			    device == DEVFS_DIR) {
				reply.words[0] = (u64)-1;
				break;
			}
			if (device == DEVFS_NULL ||
			    device == DEVFS_TTY ||
			    device == DEVFS_CONSOLE) {
				reply.words[0] = 0;
				reply.words[1] = 0;
				break;
			}
				reply.words[0] =
					read_generated_device(device, message.cap,
							      message.words[2]) == 0 ?
					0 : (u64)-1;
				reply.words[1] = reply.words[0] == 0 ?
						 message.words[2] : 0;
				break;
		case BUNIX_VFS_WRITE_FILE_BUFFER:
			device = message.words[0];
			reply.words[0] = device == DEVFS_NULL ||
					 device == DEVFS_ZERO ||
					 device == DEVFS_RANDOM ||
					 device == DEVFS_TTY ||
					 device == DEVFS_CONSOLE ? 0 : (u64)-1;
			if ((device == DEVFS_TTY || device == DEVFS_CONSOLE) &&
			    reply.words[0] == 0 &&
			    message.cap != 0 &&
			    (message.cap_rights & BUNIX_RIGHT_RECV) != 0) {
				reply.words[0] =
					write_console_device(message.cap,
							     message.words[2]) == 0 ?
					0 : (u64)-1;
			}
			reply.words[1] = reply.words[0] == 0 ? message.words[2] : 0;
			break;
		case BUNIX_VFS_TRUNCATE:
			device = message.words[0];
			reply.words[0] = device == DEVFS_NULL ||
					 device == DEVFS_ZERO ||
					 device == DEVFS_RANDOM ||
					 device == DEVFS_TTY ||
					 device == DEVFS_CONSOLE ? 0 : (u64)-1;
			break;
		case BUNIX_VFS_READDIR_BUFFER:
			if (message.words[0] != DEVFS_DIR ||
			    message.words[1] >= sizeof(entries) / sizeof(entries[0])) {
				reply.words[0] = BUNIX_VFS_ERR_NOENT;
				break;
			}
			if (message.cap == 0 ||
			    (message.cap_rights & BUNIX_RIGHT_SEND) == 0) {
				reply.words[0] = (u64)-1;
				break;
			}
			reply.words[0] = 0;
			reply.words[1] = entries[message.words[1]].device |
					 ((u64)BUNIX_VFS_TYPE_CHARACTER << 32);
			reply.words[2] = str_len(entries[message.words[1]].name);
			reply.words[3] = reply.words[2];
			if (reply.words[3] > message.words[3]) {
				reply.words[3] = message.words[3];
			}
			if (reply.words[3] != 0 &&
			    bunix_buffer_write(message.cap, message.words[2],
					       entries[message.words[1]].name,
					       reply.words[3]) != 0) {
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
