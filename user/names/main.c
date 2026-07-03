#include <bunix/syscall.h>

enum {
	MAX_NAMES = 16,
	MAX_WAITS = 16,
	NAMES_ROOT = 1,
};

struct name_entry {
	u64 namespace;
	u64 service;
	u64 handle;
	unsigned int rights;
};

struct wait_entry {
	u64 namespace;
	u64 service;
	u64 reply;
	unsigned int rights;
};

static struct name_entry names[MAX_NAMES];
static struct wait_entry waits[MAX_WAITS];
static u64 next_namespace = NAMES_ROOT + 1;

static void send_resolve_reply(u64 reply_handle, const struct name_entry *entry,
			       unsigned int rights)
{
	struct bunix_msg reply = {
		.protocol = BUNIX_PROTO_NAMES,
		.type = BUNIX_NAMES_RESOLVE,
		.sender = 0,
		.cap_rights = rights,
		.reply = 0,
		.cap = entry->handle,
		.words = { 0, 0, 0, 0 },
	};

	bunix_ipc_send(reply_handle, &reply);
}

static long register_name(const struct bunix_msg *message)
{
	const u64 namespace = message->words[0];
	const u64 service = message->words[1];

	if (namespace == 0 || service == 0 || message->cap == 0 ||
	    (message->cap_rights & BUNIX_RIGHT_SEND) == 0) {
		return -1;
	}

	for (unsigned int i = 0; i < MAX_NAMES; i++) {
		if (names[i].namespace == namespace &&
		    names[i].service == service) {
			names[i].handle = message->cap;
			names[i].rights = message->cap_rights;
			return 0;
		}
	}

	for (unsigned int i = 0; i < MAX_NAMES; i++) {
		if (names[i].namespace != 0) {
			continue;
		}

		names[i].namespace = namespace;
		names[i].service = service;
		names[i].handle = message->cap;
		names[i].rights = message->cap_rights;
		return 0;
	}

	return -1;
}

static const struct name_entry *resolve_name(u64 namespace, u64 service)
{
	for (unsigned int i = 0; i < MAX_NAMES; i++) {
		if (names[i].namespace == namespace &&
		    names[i].service == service) {
			return &names[i];
		}
	}

	return 0;
}

static void wake_waiters(const struct name_entry *entry)
{
	const char resolved[] = "names: resolved\n";

	for (unsigned int i = 0; i < MAX_WAITS; i++) {
		if (waits[i].reply == 0 ||
		    waits[i].namespace != entry->namespace ||
		    waits[i].service != entry->service ||
		    (waits[i].rights & ~entry->rights) != 0) {
			continue;
		}

		send_resolve_reply(waits[i].reply, entry, waits[i].rights);
		waits[i].namespace = 0;
		waits[i].service = 0;
		waits[i].reply = 0;
		waits[i].rights = 0;
		bunix_console_write(resolved, sizeof(resolved) - 1);
	}
}

static long add_waiter(const struct bunix_msg *message, unsigned int rights)
{
	if (message->words[0] == 0 || message->words[1] == 0 ||
	    message->reply == 0) {
		return -1;
	}

	for (unsigned int i = 0; i < MAX_WAITS; i++) {
		if (waits[i].reply != 0) {
			continue;
		}

		waits[i].namespace = message->words[0];
		waits[i].service = message->words[1];
		waits[i].reply = message->reply;
		waits[i].rights = rights;
		return 0;
	}

	return -1;
}

int main(void)
{
	const char online[] = "names: online\n";
	const char namespace_created[] = "names: namespace\n";
	const char registered[] = "names: registered\n";
	const char resolved[] = "names: resolved\n";
	const char waiting[] = "names: wait\n";
	struct bunix_msg message;

	bunix_console_write(online, sizeof(online) - 1);

	for (;;) {
		int should_reply = 1;
		struct bunix_msg reply = {
			.protocol = BUNIX_PROTO_NAMES,
			.type = 0,
			.sender = 0,
			.cap_rights = 0,
			.reply = 0,
			.cap = 0,
			.words = { 0, 0, 0, 0 },
		};

		if (bunix_ipc_recv(BUNIX_HANDLE_SELF, &message) != 0 ||
		    message.protocol != BUNIX_PROTO_NAMES) {
			continue;
		}

		reply.type = message.type;
		switch (message.type) {
		case BUNIX_NAMES_CREATE_NS:
			reply.words[0] = 0;
			reply.words[1] = next_namespace++;
			bunix_console_write(namespace_created,
					    sizeof(namespace_created) - 1);
			break;
		case BUNIX_NAMES_REGISTER:
			if (register_name(&message) == 0) {
				const struct name_entry *entry =
					resolve_name(message.words[0],
						     message.words[1]);

				reply.words[0] = 0;
				if (entry != 0) {
					wake_waiters(entry);
				}
				bunix_console_write(registered,
						    sizeof(registered) - 1);
			} else {
				reply.words[0] = (u64)-1;
			}
			break;
		case BUNIX_NAMES_RESOLVE: {
			const struct name_entry *entry =
				resolve_name(message.words[0], message.words[1]);
			const unsigned int rights =
				message.words[2] != 0 ?
				(unsigned int)message.words[2] : BUNIX_RIGHT_SEND;
			if (entry != 0 && (rights & ~entry->rights) == 0) {
				reply.cap = entry->handle;
				reply.cap_rights = rights;
				reply.words[0] = 0;
				bunix_console_write(resolved,
						    sizeof(resolved) - 1);
			} else {
				reply.words[0] = (u64)-1;
			}
			break;
		}
		case BUNIX_NAMES_WAIT: {
			const struct name_entry *entry =
				resolve_name(message.words[0], message.words[1]);
			const unsigned int rights =
				message.words[2] != 0 ?
				(unsigned int)message.words[2] : BUNIX_RIGHT_SEND;

			if (entry != 0 && (rights & ~entry->rights) == 0) {
				reply.cap = entry->handle;
				reply.cap_rights = rights;
				reply.words[0] = 0;
				bunix_console_write(resolved,
						    sizeof(resolved) - 1);
			} else if (add_waiter(&message, rights) == 0) {
				should_reply = 0;
				bunix_console_write(waiting,
						    sizeof(waiting) - 1);
			} else {
				reply.words[0] = (u64)-1;
			}
			break;
		}
		default:
			reply.words[0] = (u64)-1;
			break;
		}

		if (should_reply && message.reply != 0) {
			bunix_ipc_send(message.reply, &reply);
		}
	}
}
