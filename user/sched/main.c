#include <bunix/libbunix.h>

static long register_service(u64 service, u64 handle)
{
	(void)service;
	return bunix_names_register_claim(bunix_handle_find(BUNIX_CAP_CLAM),
					  handle);
}

static u64 policy_authority(void)
{
	return bunix_handle_find(BUNIX_CAP_SCHD);
}

static void reply_to(const struct bunix_msg *message,
		     const struct bunix_msg *reply)
{
	if (message->reply != 0) {
		bunix_ipc_send(message->reply, reply);
	}
}

static void handle_info(const struct bunix_msg *message, u64 authority)
{
	struct bunix_sched_policy_state state = {
		.target_kind = BUNIX_SCHED_TARGET_SYSTEM,
		.target_id = 0,
	};
	struct bunix_msg reply = {
		.protocol = BUNIX_PROTO_SCHED,
		.type = BUNIX_SCHED_INFO,
		.words = { (u64)-1, 0, 0, 0 },
	};

	if (bunix_sched_policy_get(authority, &state) == 0) {
		reply.words[0] = 0;
		reply.words[1] = state.online_cpu_mask;
		reply.words[2] = state.cpu_mask;
		reply.words[3] = BUNIX_SCHED_RIGHT_OBSERVE |
				 BUNIX_SCHED_RIGHT_CLASS |
				 BUNIX_SCHED_RIGHT_PRIORITY |
				 BUNIX_SCHED_RIGHT_WEIGHT |
				 BUNIX_SCHED_RIGHT_AFFINITY |
				 BUNIX_SCHED_RIGHT_DELEGATE;
	}
	reply_to(message, &reply);
}

static void handle_getp(const struct bunix_msg *message, u64 authority)
{
	const u64 policy = message->cap;
	struct bunix_sched_policy_state state = {
		.target_kind = message->words[0],
		.target_id = message->words[1],
	};
	struct bunix_msg reply = {
		.protocol = BUNIX_PROTO_SCHED,
		.type = BUNIX_SCHED_GETP,
		.words = { (u64)-1, 0, 0, 0 },
	};

	(void)authority;
	if (policy != 0 && bunix_sched_policy_get(policy, &state) == 0) {
		reply.words[0] = 0;
		reply.words[1] = state.sched_class;
		reply.words[2] = state.priority;
		reply.words[3] = state.cpu_mask;
	}
	reply_to(message, &reply);
}

static void handle_setp(const struct bunix_msg *message, u64 authority)
{
	const u64 policy = message->cap;
	struct bunix_sched_policy_state state = {
		.target_kind = message->words[0],
		.target_id = message->words[1],
	};
	struct bunix_msg reply = {
		.protocol = BUNIX_PROTO_SCHED,
		.type = BUNIX_SCHED_SETP,
		.words = { (u64)-1, 0, 0, 0 },
	};

	(void)authority;
	if (policy != 0 && bunix_sched_policy_get(policy, &state) == 0) {
		state.cpu_mask = message->words[2];
		if (message->words[3] != 0) {
			state.sched_class = message->words[3];
		}
		if (bunix_sched_policy_set(policy, &state) == 0) {
			reply.words[0] = 0;
			reply.words[1] = state.sched_class;
			reply.words[2] = state.priority;
			reply.words[3] = state.cpu_mask;
		}
	}
	reply_to(message, &reply);
}

static void handle_grant(const struct bunix_msg *message, u64 authority)
{
	struct bunix_sched_policy policy = {
		.target_kind = message->words[0],
		.target_id = message->words[1],
		.rights = message->words[2],
		.class_mask = (1ull << BUNIX_SCHED_CLASS_USER) |
			      (1ull << BUNIX_SCHED_CLASS_BATCH) |
			      (1ull << BUNIX_SCHED_CLASS_IDLE),
		.min_priority = 0,
		.max_priority = 127,
		.min_weight = 1,
		.max_weight = 65535,
		.cpu_mask = message->words[3],
		.delegation_depth = 0,
	};
	struct bunix_msg reply = {
		.protocol = BUNIX_PROTO_SCHED,
		.type = BUNIX_SCHED_GRANT,
		.words = { (u64)-1, 0, 0, 0 },
	};
	const long handle =
		message->cap != 0 ?
			bunix_sched_policy_grant(authority, message->cap,
						 &policy) :
			-1;

	if (handle > 0) {
		reply.words[0] = 0;
		reply.words[1] = (u64)handle;
	}
	reply_to(message, &reply);
}

int main(void)
{
	const char online[] = "sched: online\n";
	const char ready[] = "sched: ready\n";
	const u64 authority = policy_authority();
	struct bunix_msg message;

	bunix_console_log(online, sizeof(online) - 1);
	if (authority == 0 ||
	    register_service(BUNIX_SERVICE_SCHED, BUNIX_HANDLE_SELF) != 0) {
		return 1;
	}
	bunix_console_log(ready, sizeof(ready) - 1);

	for (;;) {
		if (bunix_ipc_recv(BUNIX_HANDLE_SELF, &message) != 0 ||
		    message.protocol != BUNIX_PROTO_SCHED) {
			continue;
		}

		switch (message.type) {
		case BUNIX_SCHED_INFO:
			handle_info(&message, authority);
			break;
		case BUNIX_SCHED_GETP:
			handle_getp(&message, authority);
			break;
		case BUNIX_SCHED_SETP:
			handle_setp(&message, authority);
			break;
		case BUNIX_SCHED_GRANT:
			handle_grant(&message, authority);
			break;
		default: {
			struct bunix_msg reply = {
				.protocol = BUNIX_PROTO_SCHED,
				.type = message.type,
				.words = { (u64)-1, 0, 0, 0 },
			};
			reply_to(&message, &reply);
			break;
		}
		}
	}
}
