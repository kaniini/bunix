#include <bunix/id_table.h>
#include <bunix/libbunix.h>

enum {
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

static struct bunix_map names;
static struct bunix_id_table waits;
static u64 next_namespace = NAMES_ROOT + 1;

static u64 name_key(u64 namespace, u64 service)
{
	return (namespace << 32) | (service & 0xffffffff);
}

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

static const struct name_entry *resolve_name(u64 namespace, u64 service)
{
	return (const struct name_entry *)
		bunix_map_get(&names, name_key(namespace, service));
}

static long register_name(const struct bunix_msg *message)
{
	const u64 namespace = message->words[0];
	const u64 service = message->words[1];
	struct name_entry *entry;

	if (namespace == 0 || service == 0 || message->cap == 0 ||
	    (message->cap_rights & BUNIX_RIGHT_SEND) == 0) {
		return -1;
	}

	entry = (struct name_entry *)
		bunix_map_get(&names, name_key(namespace, service));
	if (entry != 0) {
		entry->handle = message->cap;
		entry->rights = message->cap_rights;
		return 0;
	}

	entry = (struct name_entry *)bunix_calloc(1, sizeof(*entry));
	if (entry == 0) {
		return -1;
	}
	entry->namespace = namespace;
	entry->service = service;
	entry->handle = message->cap;
	entry->rights = message->cap_rights;
	if (bunix_map_set(&names, name_key(namespace, service),
			  (u64)entry) != 0) {
		bunix_free(entry);
		return -1;
	}
	return 0;
}

static void wake_waiters(const struct name_entry *entry)
{
	const char resolved[] = "names: resolved\n";

	for (u64 i = 0; i < waits.capacity; i++) {
		struct wait_entry *wait =
			(struct wait_entry *)waits.slots[i].value;

		if (wait == 0 ||
		    wait->namespace != entry->namespace ||
		    wait->service != entry->service ||
		    (wait->rights & ~entry->rights) != 0) {
			continue;
		}

		send_resolve_reply(wait->reply, entry, wait->rights);
		bunix_free(wait);
		(void)bunix_id_remove(&waits, waits.slots[i].id);
		bunix_console_log(resolved, sizeof(resolved) - 1);
	}
}

static long add_waiter(const struct bunix_msg *message, unsigned int rights)
{
	struct wait_entry *wait;

	if (message->words[0] == 0 || message->words[1] == 0 ||
	    message->reply == 0) {
		return -1;
	}

	wait = (struct wait_entry *)bunix_calloc(1, sizeof(*wait));
	if (wait == 0) {
		return -1;
	}
	wait->namespace = message->words[0];
	wait->service = message->words[1];
	wait->reply = message->reply;
	wait->rights = rights;
	if (bunix_id_alloc(&waits, (u64)wait) == 0) {
		bunix_free(wait);
		return -1;
	}
	return 0;
}

int main(void)
{
	const char online[] = "names: online\n";
	const char namespace_created[] = "names: namespace\n";
	const char registered[] = "names: registered\n";
	const char resolved[] = "names: resolved\n";
	const char waiting[] = "names: wait\n";
	struct bunix_msg message;

	bunix_map_init(&names);
	bunix_id_table_init(&waits);
	bunix_console_log(online, sizeof(online) - 1);

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
			bunix_console_log(namespace_created,
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
				bunix_console_log(registered,
						    sizeof(registered) - 1);
			} else {
				reply.words[0] = (u64)-1;
			}
			break;
		case BUNIX_NAMES_RESOLVE: {
			const u64 rights = (unsigned int)message.words[2];
			const struct name_entry *entry =
				resolve_name(message.words[0], message.words[1]);

			if (entry != 0 && (rights & ~entry->rights) == 0) {
				reply.words[0] = 0;
				reply.cap = entry->handle;
				reply.cap_rights = (unsigned int)rights;
				bunix_console_log(resolved,
						    sizeof(resolved) - 1);
			} else {
				reply.words[0] = (u64)-1;
			}
			break;
		}
		case BUNIX_NAMES_WAIT: {
			const u64 rights = (unsigned int)message.words[2];
			const struct name_entry *entry =
				resolve_name(message.words[0], message.words[1]);

			if (entry != 0 && (rights & ~entry->rights) == 0) {
				reply.words[0] = 0;
				reply.cap = entry->handle;
				reply.cap_rights = (unsigned int)rights;
				bunix_console_log(resolved,
						    sizeof(resolved) - 1);
			} else if (add_waiter(&message, (unsigned int)rights) == 0) {
				should_reply = 0;
				bunix_console_log(waiting,
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

		if (should_reply) {
			(void)bunix_ipc_send(message.reply, &reply);
		}
	}
}
