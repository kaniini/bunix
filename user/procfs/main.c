#include <bunix/libbunix.h>

enum {
	PROCFS_HANDLE_NAMES = 3,
	PROCFS_MAX_OPEN_FILES = 16,
	PROCFS_MAX_TEXT = 2048,
	PROCFS_FILE_PROC = 1,
	PROCFS_FILE_KTHREADS = 2,
};

static u64 open_files[PROCFS_MAX_OPEN_FILES];
static char text[PROCFS_MAX_TEXT];

static u64 build_kthreads(void);

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

	if (bunix_ipc_call(PROCFS_HANDLE_NAMES, &request, &reply) != 0) {
		return -1;
	}
	return reply.words[0] == 0 ? 0 : -1;
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

static void unpack_path(char *path, const u64 *words)
{
	for (u64 i = 0; i < 16; i++) {
		const u64 slot = i / 8;
		const u64 shift = (i % 8) * 8;

		path[i] = (char)((words[slot] >> shift) & 0xff);
	}
	path[31] = '\0';
}

static void pack_name(u64 *words, const char *name)
{
	words[0] = 0;
	words[1] = 0;
	for (u64 i = 0; i < 16 && name[i] != '\0'; i++) {
		const u64 slot = i / 8;
		const u64 shift = (i % 8) * 8;

		words[slot] |= ((u64)(unsigned char)name[i]) << shift;
	}
}

static u64 file_for_path(const char *path)
{
	if (str_eq(path, "/proc")) {
		return PROCFS_FILE_PROC;
	}
	if (str_eq(path, "/proc/kthreads")) {
		return PROCFS_FILE_KTHREADS;
	}
	return 0;
}

static u64 file_size(u64 file)
{
	if (file == PROCFS_FILE_KTHREADS) {
		return PROCFS_MAX_TEXT;
	}
	return 0;
}

static void append_char(u64 *len, char c)
{
	if (*len + 1 < sizeof(text)) {
		text[*len] = c;
		*len += 1;
		text[*len] = '\0';
	}
}

static void append_str(u64 *len, const char *src)
{
	while (*src != '\0') {
		append_char(len, *src++);
	}
}

static void append_u64(u64 *len, u64 value)
{
	char tmp[20];
	u64 pos = 0;

	if (value == 0) {
		append_char(len, '0');
		return;
	}
	while (value != 0 && pos < sizeof(tmp)) {
		tmp[pos++] = (char)('0' + (value % 10));
		value /= 10;
	}
	while (pos != 0) {
		append_char(len, tmp[--pos]);
	}
}

static void append_task_name(u64 *len, const u64 *words)
{
	for (u64 i = 0; i < 16; i++) {
		const u64 slot = i / 8;
		const u64 shift = (i % 8) * 8;
		const char c = (char)((words[slot] >> shift) & 0xff);

		if (c == '\0') {
			break;
		}
		append_char(len, c);
	}
}

static u64 build_kthreads(void)
{
	u64 len = 0;

	append_str(&len, "tid threads flags name\n");
	text[len] = '\0';
	for (u64 index = 0;; index++) {
		u64 packed = 0;
		u64 name_words[2] = { 0, 0 };
		const long rc = bunix_task_info(index, &packed, name_words);

		if (rc != 0) {
			break;
		}
		append_u64(&len, packed & 0xffffffff);
		append_char(&len, ' ');
		append_u64(&len, (packed >> 32) & 0xffff);
		append_char(&len, ' ');
		append_u64(&len, packed >> 48);
		append_char(&len, ' ');
		append_task_name(&len, name_words);
		append_char(&len, '\n');
	}
	return len;
}

static u64 open_file(u64 file)
{
	for (u64 i = 0; i < PROCFS_MAX_OPEN_FILES; i++) {
		if (open_files[i] == 0) {
			open_files[i] = file;
			return i + 1;
		}
	}
	return 0;
}

static u64 file_from_handle(u64 handle)
{
	if (handle == 0 || handle > PROCFS_MAX_OPEN_FILES) {
		return 0;
	}
	return open_files[handle - 1];
}

static void close_file(u64 handle)
{
	if (handle != 0 && handle <= PROCFS_MAX_OPEN_FILES) {
		open_files[handle - 1] = 0;
	}
}

static void stat_reply(struct bunix_msg *reply, u64 file)
{
	reply->words[0] = 0;
	reply->words[1] = file_size(file);
	reply->words[2] = file == PROCFS_FILE_PROC ?
			  (0555 | ((u64)BUNIX_VFS_TYPE_DIRECTORY << 32)) :
			  (0444 | ((u64)BUNIX_VFS_TYPE_REGULAR << 32));
	reply->words[3] = 0;
}

int main(void)
{
	const char online[] = "procfs: online\n";
	struct bunix_msg message;

	bunix_console_write(online, sizeof(online) - 1);
	if (register_service(BUNIX_SERVICE_PROCFS, BUNIX_HANDLE_SELF) != 0) {
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

		if (bunix_ipc_recv(BUNIX_HANDLE_SELF, &message) != 0 ||
		    message.protocol != BUNIX_PROTO_VFS) {
			continue;
		}

		reply.type = message.type;
		switch (message.type) {
		case BUNIX_VFS_OPEN:
		case BUNIX_VFS_STAT_PATH_META: {
			char path[32];
			const u64 file = (unpack_path(path, &message.words[0]),
					  file_for_path(path));

			if (file == 0) {
				reply.words[0] = BUNIX_VFS_ERR_NOENT;
				break;
			}
			if (message.type == BUNIX_VFS_STAT_PATH_META) {
				stat_reply(&reply, file);
			} else {
				const u64 handle = open_file(file);

				if (handle == 0) {
					reply.words[0] = (u64)-1;
				} else {
					reply.words[0] = 0;
					reply.words[1] = handle;
					reply.words[2] = file_size(file);
					reply.words[3] = file == PROCFS_FILE_PROC ?
						BUNIX_VFS_TYPE_DIRECTORY :
						BUNIX_VFS_TYPE_REGULAR;
				}
			}
			break;
		}
		case BUNIX_VFS_STAT_META: {
			const u64 file = file_from_handle(message.words[0]);

			if (file == 0) {
				reply.words[0] = (u64)-1;
			} else {
				stat_reply(&reply, file);
			}
			break;
		}
		case BUNIX_VFS_READ_FILE_BUFFER: {
			const u64 file = file_from_handle(message.words[0]);
			const u64 offset = message.words[1];
			u64 len = message.words[2];
			u64 size;

			if (file != PROCFS_FILE_KTHREADS || message.cap == 0) {
				reply.words[0] = (u64)-1;
				break;
			}
			size = build_kthreads();
			if (offset >= size) {
				len = 0;
			} else if (len > size - offset) {
				len = size - offset;
			}
			reply.words[0] = bunix_buffer_write(message.cap, 0,
							    text + offset,
							    len) == 0 ? 0 :
					 (u64)-1;
			reply.words[1] = reply.words[0] == 0 ? len : 0;
			break;
		}
		case BUNIX_VFS_READDIR: {
			const u64 file = file_from_handle(message.words[0]);

			if (file != PROCFS_FILE_PROC) {
				reply.words[0] = BUNIX_VFS_ERR_NOTDIR;
				break;
			}
			if (message.words[1] != 0) {
				reply.words[0] = BUNIX_VFS_ERR_NOENT;
				break;
			}
			reply.words[0] = 0;
			reply.words[1] = 1 | ((u64)BUNIX_VFS_TYPE_REGULAR << 32);
			pack_name(&reply.words[2], "kthreads");
			break;
		}
		case BUNIX_VFS_CLOSE:
			if (file_from_handle(message.words[0]) == 0) {
				reply.words[0] = (u64)-1;
			} else {
				close_file(message.words[0]);
				reply.words[0] = 0;
			}
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
}
