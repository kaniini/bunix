#include <bunix/libbunix.h>

enum {
	UTMPFS_HANDLE_NAMES = 3,
	UTMPFS_MAX_PATH = 256,
	UTMPFS_RECORD_SIZE = 400,
	UTMPFS_USER_PROCESS = 7,
	UTMPFS_MAX_READ = 4096,
};

static char read_buffer[UTMPFS_MAX_READ];

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

static void zero_bytes(char *buffer, u64 len)
{
	for (u64 i = 0; i < len; i++) {
		buffer[i] = 0;
	}
}

static void store_u16(char *buffer, u64 offset, unsigned int value)
{
	for (u64 i = 0; i < 2; i++) {
		buffer[offset + i] = (char)((value >> (i * 8)) & 0xff);
	}
}

static void store_u32(char *buffer, u64 offset, unsigned int value)
{
	for (u64 i = 0; i < 4; i++) {
		buffer[offset + i] = (char)((value >> (i * 8)) & 0xff);
	}
}

static void pack_path(u64 *words, const char *path)
{
	words[0] = 0;
	words[1] = 0;
	for (u64 i = 0; i < 16 && path[i] != '\0'; i++) {
		const u64 slot = i / 8;
		const u64 shift = (i % 8) * 8;

		words[slot] |= ((u64)(unsigned char)path[i]) << shift;
	}
}

static void copy_literal(char *dst, u64 dst_len, const char *src)
{
	for (u64 i = 0; i < dst_len; i++) {
		dst[i] = src[i];
		if (src[i] == '\0') {
			return;
		}
	}
}

static int read_path_buffer_at(u64 buffer, u64 offset, u64 len, char *path)
{
	if (buffer == 0 || len == 0 || len > UTMPFS_MAX_PATH) {
		return -1;
	}
	for (u64 i = 0; i < UTMPFS_MAX_PATH; i++) {
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
	char cwd[UTMPFS_MAX_PATH];

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

static int utmp_path(const char *path)
{
	return str_eq(path, "/run/utmp") || str_eq(path, "/var/run/utmp");
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

	if (bunix_ipc_call(UTMPFS_HANDLE_NAMES, &request, &reply) != 0 ||
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

	if (bunix_ipc_call(UTMPFS_HANDLE_NAMES, &request, &reply) != 0) {
		return -1;
	}
	return reply.words[0] == 0 ? 0 : -1;
}

static u64 mount_path(u64 vfs, const char *path)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_MOUNT,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP,
		.reply = 0,
		.cap = BUNIX_HANDLE_SELF,
		.words = { 0, 0, BUNIX_SERVICE_UTMPFS, 0 },
	};
	struct bunix_msg reply;

	pack_path(&request.words[0], path);
	return bunix_ipc_call(vfs, &request, &reply) == 0 &&
	       reply.words[0] == 0 ? 0 : (u64)-1;
}

static long user_session_count(u64 user, u64 *count)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_USER,
		.type = BUNIX_USER_SESSION_COUNT,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { 0, 0, 0, 0 },
	};
	struct bunix_msg reply;

	if (count == 0 ||
	    bunix_ipc_call(user, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return -1;
	}
	*count = reply.words[1];
	return 0;
}

static long user_session_at(u64 user, u64 index, u64 *session_id, u64 *uid,
			    u64 *tty, u64 *foreground)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_USER,
		.type = BUNIX_USER_SESSION_AT,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { index, 0, 0, 0 },
	};
	struct bunix_msg reply;

	if (bunix_ipc_call(user, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return -1;
	}
	*session_id = reply.words[1];
	*uid = reply.words[2];
	*tty = reply.words[3] >> 32;
	*foreground = reply.words[3] & 0xffffffff;
	return 0;
}

static long user_name_for_uid(u64 user, u64 uid, char *name, u64 name_len)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_USER,
		.type = BUNIX_USER_NAME_FOR_UID,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { uid, 0, 0, 0 },
	};
	struct bunix_msg reply;

	if (name == 0 || name_len == 0 ||
	    bunix_ipc_call(user, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return -1;
	}
	for (u64 i = 0; i + 1 < name_len && i < 16; i++) {
		const u64 word = i < 8 ? reply.words[1] : reply.words[2];
		const u64 shift = (i % 8) * 8;

		name[i] = (char)((word >> shift) & 0xff);
		if (name[i] == '\0') {
			return 0;
		}
	}
	name[name_len - 1] = '\0';
	return 0;
}

static void build_record(u64 user, char *record, u64 index)
{
	u64 session_id = 0;
	u64 uid = 0;
	u64 tty = 0;
	u64 foreground = 0;
	char user_name[16];

	zero_bytes(record, UTMPFS_RECORD_SIZE);
	if (user_session_at(user, index, &session_id, &uid,
			    &tty, &foreground) != 0) {
		return;
	}
	if (user_name_for_uid(user, uid, user_name, sizeof(user_name)) != 0 ||
	    user_name[0] == '\0') {
		copy_literal(user_name, sizeof(user_name), "user");
	}

	store_u16(record, 0, UTMPFS_USER_PROCESS);
	store_u32(record, 4, (unsigned int)foreground);
	copy_literal(record + 8, 32, tty == 1 ? "ttyS0" : "tty");
	copy_literal(record + 40, 4, tty == 1 ? "S0" : "tt");
	copy_literal(record + 44, 32, user_name);
	store_u32(record, 336, (unsigned int)session_id);
}

static u64 utmp_size(u64 user)
{
	u64 count = 0;

	if (user == 0 || user_session_count(user, &count) != 0) {
		return 0;
	}
	return count * UTMPFS_RECORD_SIZE;
}

static long read_utmp(u64 user, u64 offset, u64 len, u64 buffer)
{
	const u64 size = utmp_size(user);
	u64 done = 0;

	if (buffer == 0) {
		return -1;
	}
	if (offset >= size) {
		return 0;
	}
	if (len > sizeof(read_buffer)) {
		len = sizeof(read_buffer);
	}
	if (len > size - offset) {
		len = size - offset;
	}

	while (done < len) {
		const u64 absolute = offset + done;
		const u64 record_index = absolute / UTMPFS_RECORD_SIZE;
		const u64 record_offset = absolute % UTMPFS_RECORD_SIZE;
		u64 chunk = UTMPFS_RECORD_SIZE - record_offset;
		char record[UTMPFS_RECORD_SIZE];

		if (chunk > len - done) {
			chunk = len - done;
		}
		build_record(user, record, record_index);
		for (u64 i = 0; i < chunk; i++) {
			read_buffer[done + i] = record[record_offset + i];
		}
		done += chunk;
	}

	if (bunix_buffer_write(buffer, 0, read_buffer, done) != 0) {
		return -1;
	}
	return (long)done;
}

static long getent_utmp(u64 user, u64 index, u64 buffer)
{
	u64 count = 0;
	char record[UTMPFS_RECORD_SIZE];

	if (buffer == 0 ||
	    user_session_count(user, &count) != 0 ||
	    index >= count) {
		return -1;
	}
	build_record(user, record, index);
	return bunix_buffer_write(buffer, 0, record,
				  sizeof(record)) == 0 ? 0 : -1;
}

static void stat_utmp(struct bunix_msg *reply, u64 user)
{
	reply->words[0] = 0;
	reply->words[1] = utmp_size(user);
	reply->words[2] = 0444 | ((u64)BUNIX_VFS_TYPE_REGULAR << 32);
	reply->words[3] = 0;
}

int main(void)
{
	const char online[] = "utmpfs: online\n";
	const char mounted[] = "utmpfs: mounted\n";
	struct bunix_msg message;
	const u64 vfs = resolve_service(BUNIX_SERVICE_VFS, BUNIX_RIGHT_SEND);
	const u64 user = resolve_service(BUNIX_SERVICE_USER, BUNIX_RIGHT_SEND);

	bunix_console_log(online, sizeof(online) - 1);
	if (vfs == 0 || user == 0 ||
	    mount_path(vfs, "/run/utmp") != 0 ||
	    mount_path(vfs, "/var/run/utmp") != 0 ||
	    register_service(BUNIX_SERVICE_UTMPFS, BUNIX_HANDLE_SELF) != 0) {
		return 1;
	}
	bunix_console_log(mounted, sizeof(mounted) - 1);

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
		char path[UTMPFS_MAX_PATH];

		if (bunix_ipc_recv(BUNIX_HANDLE_SELF, &message) != 0 ||
		    (message.protocol != BUNIX_PROTO_VFS &&
		     message.protocol != BUNIX_PROTO_UTMPFS)) {
			continue;
		}
		reply.type = message.type;

		if (message.protocol == BUNIX_PROTO_UTMPFS) {
			reply.protocol = BUNIX_PROTO_UTMPFS;
			switch (message.type) {
			case BUNIX_UTMPFS_GETENT:
				reply.words[0] =
					(message.cap_rights & BUNIX_RIGHT_SEND) != 0 &&
					getent_utmp(user, message.words[0],
						    message.cap) == 0 ? 0 : (u64)-1;
				break;
			default:
				reply.words[0] = (u64)-1;
				break;
			}
			bunix_ipc_send(message.reply, &reply);
			continue;
		}

		switch (message.type) {
		case BUNIX_VFS_OPEN_BUFFER:
			if (read_resolved_path(&message, path) != 0 ||
			    !utmp_path(path)) {
				reply.words[0] = BUNIX_VFS_ERR_NOENT;
				break;
			}
			reply.words[0] = 0;
			reply.words[1] = 1;
			reply.words[2] = utmp_size(user);
			reply.words[3] = BUNIX_VFS_TYPE_REGULAR;
			break;
		case BUNIX_VFS_STAT_PATH_META_BUFFER:
		case BUNIX_VFS_ACCESS_BUFFER:
			if (read_resolved_path(&message, path) != 0 ||
			    !utmp_path(path)) {
				reply.words[0] = BUNIX_VFS_ERR_NOENT;
				break;
			}
			stat_utmp(&reply, user);
			break;
		case BUNIX_VFS_STAT_META:
			if (message.words[0] != 1) {
				reply.words[0] = BUNIX_VFS_ERR_NOENT;
			} else {
				stat_utmp(&reply, user);
			}
			break;
		case BUNIX_VFS_READ_FILE_BUFFER: {
			long nread;

			if (message.words[0] != 1 ||
			    message.cap == 0 ||
			    (message.cap_rights & BUNIX_RIGHT_SEND) == 0) {
				reply.words[0] = (u64)-1;
				break;
			}
			nread = read_utmp(user, message.words[1],
					  message.words[2], message.cap);
			reply.words[0] = nread < 0 ? (u64)-1 : 0;
			reply.words[1] = nread < 0 ? 0 : (u64)nread;
			break;
		}
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
