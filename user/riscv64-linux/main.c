#include <bunix/libbunix.h>

enum {
	LINUX_RISCV64_WRITE = 64,
	LINUX_RISCV64_EXIT = 93,
	LINUX_RISCV64_EXIT_GROUP = 94,
	LINUX_RISCV64_SET_TID_ADDRESS = 96,
};

static void log_line(const char *text, u64 len)
{
	(void)bunix_syscall2(BUNIX_SYSCALL_EARLY_CONSOLE_LOG, (u64)text, len);
}

static void reply_to(const struct bunix_msg *message, u64 result, u64 action)
{
	struct bunix_msg reply = {
		.protocol = BUNIX_PROTO_LINUX,
		.type = message->type,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { result, action, 0, 0 },
	};

	if (message->reply != 0) {
		(void)bunix_ipc_send(message->reply, &reply);
	}
}

int main(void)
{
	const char online[] = "linux-riscv64-server: online\n";
	const char write[] = "linux-riscv64-server: write\n";
	const char exit_group[] = "linux-riscv64-server: exit_group\n";
	const char poweroff[] = "linux-riscv64-server: poweroff\n";
	struct bunix_msg message;

	log_line(online, sizeof(online) - 1);
	for (;;) {
		if (bunix_ipc_recv(BUNIX_HANDLE_SELF, &message) != 0 ||
		    message.protocol != BUNIX_PROTO_LINUX) {
			continue;
		}
		if (message.type != BUNIX_LINUX_RISCV64_SYSCALL) {
			reply_to(&message, (u64)-1,
				 BUNIX_LINUX_RISCV64_ACTION_NONE);
			continue;
		}

		switch (message.words[0]) {
		case LINUX_RISCV64_WRITE:
			if (message.words[1] != 1 && message.words[1] != 2) {
				reply_to(&message, (u64)-1,
					 BUNIX_LINUX_RISCV64_ACTION_NONE);
				break;
			}
			log_line(write, sizeof(write) - 1);
			reply_to(&message, message.words[3],
				 BUNIX_LINUX_RISCV64_ACTION_WRITE);
			break;
		case LINUX_RISCV64_EXIT:
		case LINUX_RISCV64_EXIT_GROUP:
			log_line(exit_group, sizeof(exit_group) - 1);
			reply_to(&message, 0, BUNIX_LINUX_RISCV64_ACTION_EXIT);
			log_line(poweroff, sizeof(poweroff) - 1);
			(void)bunix_machine_poweroff(0);
			break;
		case LINUX_RISCV64_SET_TID_ADDRESS:
			reply_to(&message, message.sender,
				 BUNIX_LINUX_RISCV64_ACTION_NONE);
			break;
		default:
			reply_to(&message, (u64)-1,
				 BUNIX_LINUX_RISCV64_ACTION_NONE);
			break;
		}
	}
}
