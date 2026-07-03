#include <bunix/libbunix.h>

enum {
	LINUX_EBADF = 9,
	LINUX_EINVAL = 22,
	LINUX_MAX_WRITE = 4096,
	LINUX_FD_CONSOLE = 1,
};

struct linux_fd {
	u64 handle;
	u64 kind;
};

static struct linux_fd fds[16];
static char write_buffer[LINUX_MAX_WRITE];

int main(void)
{
	const char online[] = "linux-server: online\n";
	const char write_ok[] = "linux-server: write\n";
	const char bad_fd[] = "linux-server: ebadf\n";
	const char exit_group[] = "linux-server: exit_group\n";
	struct bunix_msg message;

	fds[1].handle = BUNIX_HANDLE_CONSOLE;
	fds[1].kind = LINUX_FD_CONSOLE;
	fds[2].handle = BUNIX_HANDLE_CONSOLE;
	fds[2].kind = LINUX_FD_CONSOLE;

	bunix_console_write(online, sizeof(online) - 1);
	for (;;) {
		struct bunix_msg reply = {
			.protocol = BUNIX_PROTO_LINUX,
			.type = 0,
			.sender = 0,
			.cap_rights = 0,
			.reply = 0,
			.cap = 0,
			.words = { 0, 0, 0, 0 },
		};

		if (bunix_ipc_recv(BUNIX_HANDLE_SELF, &message) != 0 ||
		    message.protocol != BUNIX_PROTO_LINUX) {
			continue;
		}

		reply.type = message.type;
		switch (message.type) {
		case BUNIX_LINUX_WRITE: {
			const u64 fd = message.words[0];
			const u64 len = message.words[1];

			if (fd >= sizeof(fds) / sizeof(fds[0]) ||
			    fds[fd].kind != LINUX_FD_CONSOLE) {
				bunix_console_write(bad_fd, sizeof(bad_fd) - 1);
				reply.words[0] = (u64)-LINUX_EBADF;
			} else if (message.cap == 0 || len > sizeof(write_buffer) ||
				   bunix_buffer_read(message.cap, 0,
						     write_buffer, len) != 0) {
				reply.words[0] = (u64)-LINUX_EINVAL;
			} else {
				const struct bunix_msg console_message = {
					.protocol = BUNIX_PROTO_CONSOLE,
					.type = BUNIX_CONSOLE_WRITE,
					.sender = 0,
					.cap_rights = 0,
					.reply = 0,
					.cap = 0,
					.words = { (u64)write_buffer, len, 0, 0 },
				};

				bunix_ipc_send(fds[fd].handle, &console_message);
				bunix_console_write(write_ok, sizeof(write_ok) - 1);
				reply.words[0] = len;
			}
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		}
		case BUNIX_LINUX_EXIT_GROUP:
			bunix_console_write(exit_group, sizeof(exit_group) - 1);
			reply.words[0] = 0;
			break;
		default:
			reply.words[0] = (u64)-1;
			break;
		}

		if (message.reply != 0) {
			bunix_ipc_send(message.reply, &reply);
		}
	}
}
