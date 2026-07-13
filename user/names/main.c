#include <bunix/id_table.h>
#include <bunix/libbunix.h>

enum {
	NAMES_ROOT = 1,
};

struct name_entry {
	u64 namespace;
	u64 service;
	u64 handle;
	u64 owner;
	unsigned int rights;
};

struct namespace_entry {
	u64 namespace;
	u64 admin;
};

struct claim_entry {
	u64 handle;
	u64 namespace;
	u64 service;
	u64 owner;
	u64 used;
};

struct wait_entry {
	u64 namespace;
	u64 service;
	u64 reply;
	unsigned int rights;
};

static struct bunix_map names;
static struct bunix_map namespaces;
static struct bunix_id_table waits;
static struct bunix_id_table claims;
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

static struct namespace_entry *namespace_find(u64 namespace)
{
	return (struct namespace_entry *)bunix_map_get(&namespaces, namespace);
}

static u64 namespace_admin(u64 namespace)
{
	struct namespace_entry *entry = namespace_find(namespace);

	return entry != 0 ? entry->admin : 0;
}

static long namespace_set_admin(u64 namespace, u64 admin)
{
	struct namespace_entry *entry;

	if (namespace == 0 || admin == 0) {
		return -1;
	}
	entry = namespace_find(namespace);
	if (entry != 0) {
		entry->admin = admin;
		return 0;
	}
	entry = (struct namespace_entry *)bunix_calloc(1, sizeof(*entry));
	if (entry == 0) {
		return -1;
	}
	entry->namespace = namespace;
	entry->admin = admin;
	if (bunix_map_set(&namespaces, namespace, (u64)entry) != 0) {
		bunix_free(entry);
		return -1;
	}
	return 0;
}

static int sender_is_namespace_admin(u64 namespace, u64 sender)
{
	const u64 admin = namespace_admin(namespace);

	return admin != 0 && sender == admin;
}

static long claim_root_admin(const struct bunix_msg *message)
{
	if (message->sender == 0 || message->words[0] != NAMES_ROOT ||
	    namespace_admin(NAMES_ROOT) != 0) {
		return -1;
	}
	return namespace_set_admin(NAMES_ROOT, message->sender);
}

static long create_namespace(const struct bunix_msg *message, u64 *namespace)
{
	u64 created;

	if (namespace == 0 || !sender_is_namespace_admin(NAMES_ROOT,
							message->sender)) {
		return -1;
	}
	created = next_namespace++;
	if (namespace_set_admin(created, message->sender) != 0) {
		return -1;
	}
	*namespace = created;
	return 0;
}

static long register_entry(u64 namespace, u64 service, u64 sender, u64 handle,
			   unsigned int rights, int claimed)
{
	const u64 admin = namespace_admin(namespace);
	struct name_entry *entry;

	if (namespace == 0 || service == 0 || sender == 0 || handle == 0 ||
	    (rights & BUNIX_RIGHT_SEND) == 0) {
		return -1;
	}
	if (admin == 0 && namespace != NAMES_ROOT) {
		return -1;
	}

	entry = (struct name_entry *)
		bunix_map_get(&names, name_key(namespace, service));
	if (entry != 0) {
		if (entry->owner != sender && admin != sender) {
			return -1;
		}
		entry->handle = handle;
		entry->owner = sender;
		entry->rights = rights;
		return 0;
	}
	if (!claimed && admin != sender &&
	    !(namespace == NAMES_ROOT && admin == 0)) {
		return -1;
	}

	entry = (struct name_entry *)bunix_calloc(1, sizeof(*entry));
	if (entry == 0) {
		return -1;
	}
	entry->namespace = namespace;
	entry->service = service;
	entry->handle = handle;
	entry->owner = sender;
	entry->rights = rights;
	if (bunix_map_set(&names, name_key(namespace, service),
			  (u64)entry) != 0) {
		bunix_free(entry);
		return -1;
	}
	return 0;
}

static long register_name(const struct bunix_msg *message)
{
	return register_entry(message->words[0], message->words[1],
			      message->sender, message->cap,
			      message->cap_rights, 0);
}

static long create_registration_claim(const struct bunix_msg *message,
				      struct bunix_msg *reply)
{
	struct claim_entry *claim;
	long handle;

	if (reply == 0 || message->words[0] == 0 || message->words[1] == 0 ||
	    !sender_is_namespace_admin(message->words[0], message->sender)) {
		return -1;
	}
	claim = (struct claim_entry *)bunix_calloc(1, sizeof(*claim));
	if (claim == 0) {
		return -1;
	}
	handle = bunix_port_create("names-claim");
	if (handle < 0) {
		bunix_free(claim);
		return -1;
	}
	claim->handle = (u64)handle;
	claim->namespace = message->words[0];
	claim->service = message->words[1];
	claim->owner = message->sender;
	if (bunix_id_alloc(&claims, (u64)claim) == 0) {
		bunix_handle_close((u64)handle);
		bunix_free(claim);
		return -1;
	}
	reply->cap = (u64)handle;
	reply->cap_rights = BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP;
	reply->words[0] = 0;
	return 0;
}

static long register_claimed_name(struct claim_entry *claim,
				  const struct bunix_msg *message,
				  struct name_entry **entry_out)
{
	struct name_entry *entry;

	if (claim == 0 || message == 0 || claim->used != 0) {
		return -1;
	}
	if (register_entry(claim->namespace, claim->service, message->sender,
			   message->cap, message->cap_rights, 1) != 0) {
		return -1;
	}
	claim->used = 1;
	entry = (struct name_entry *)
		bunix_map_get(&names, name_key(claim->namespace,
					       claim->service));
	if (entry_out != 0) {
		*entry_out = entry;
	}
	return entry != 0 ? 0 : -1;
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

static int poll_claims(void)
{
	const char registered[] = "names: registered\n";
	int any = 0;

	for (u64 i = 0; i < claims.capacity; i++) {
		struct claim_entry *claim =
			(struct claim_entry *)claims.slots[i].value;
		struct bunix_msg message;
		struct bunix_msg reply = {
			.protocol = BUNIX_PROTO_NAMES,
			.type = BUNIX_NAMES_REGISTER,
			.sender = 0,
			.cap_rights = 0,
			.reply = 0,
			.cap = 0,
			.words = { 0, 0, 0, 0 },
		};
		struct name_entry *entry = 0;
		long recv;

		if (claim == 0 || claim->used != 0) {
			continue;
		}
		recv = bunix_ipc_try_recv(claim->handle, &message);
		if (recv != 0) {
			continue;
		}
		any = 1;
		if (message.protocol == BUNIX_PROTO_NAMES &&
		    message.type == BUNIX_NAMES_REGISTER &&
		    register_claimed_name(claim, &message, &entry) == 0) {
			reply.words[0] = 0;
			if (entry != 0) {
				wake_waiters(entry);
			}
			bunix_console_log(registered, sizeof(registered) - 1);
		} else {
			reply.words[0] = (u64)-1;
		}
		if (message.reply != 0) {
			(void)bunix_ipc_send(message.reply, &reply);
		}
	}
	return any;
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
	bunix_map_init(&namespaces);
	bunix_id_table_init(&waits);
	bunix_id_table_init(&claims);
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

		if (poll_claims()) {
			continue;
		}
		if (bunix_ipc_recv(BUNIX_HANDLE_SELF, &message) != 0) {
			continue;
		}
		if (message.protocol != BUNIX_PROTO_NAMES) {
			continue;
		}

		reply.type = message.type;
		switch (message.type) {
		case BUNIX_NAMES_CLAIM_ADMIN:
			reply.words[0] = claim_root_admin(&message) == 0 ?
					 0 : (u64)-1;
			break;
		case BUNIX_NAMES_CREATE_NS:
			if (create_namespace(&message, &reply.words[1]) == 0) {
				reply.words[0] = 0;
				bunix_console_log(namespace_created,
						    sizeof(namespace_created) - 1);
			} else {
				reply.words[0] = (u64)-1;
			}
			break;
		case BUNIX_NAMES_CREATE_CLAIM:
			if (create_registration_claim(&message, &reply) != 0) {
				reply.words[0] = (u64)-1;
			}
			break;
		case BUNIX_NAMES_CLAIM_READY:
			(void)poll_claims();
			reply.words[0] = 0;
			should_reply = 0;
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
