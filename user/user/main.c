#include <bunix/libbunix.h>

enum {
	USER_HANDLE_NAMES = 3,
	USER_MAX_CREDENTIALS = 32,
	USER_MAX_GROUPS = 2,
};

struct user_credential {
	u64 task;
	u64 uid;
	u64 gid;
	u64 euid;
	u64 egid;
	u64 suid;
	u64 sgid;
	u64 group_count;
	u64 groups[USER_MAX_GROUPS];
};

static struct user_credential credentials[USER_MAX_CREDENTIALS];

static long register_service(u64 service)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_NAMES,
		.type = BUNIX_NAMES_REGISTER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP,
		.reply = 0,
		.cap = BUNIX_HANDLE_SELF,
		.words = { BUNIX_NAMES_ROOT, service, 0, 0 },
	};
	struct bunix_msg reply;

	if (bunix_ipc_call(USER_HANDLE_NAMES, &request, &reply) != 0) {
		return -1;
	}

	return reply.words[0] == 0 ? 0 : -1;
}

static struct user_credential *credential_find(u64 task)
{
	for (u64 i = 0; i < USER_MAX_CREDENTIALS; i++) {
		if (credentials[i].task == task) {
			return &credentials[i];
		}
	}

	return 0;
}

static struct user_credential *credential_alloc(u64 task)
{
	for (u64 i = 0; i < USER_MAX_CREDENTIALS; i++) {
		if (credentials[i].task == 0) {
			credentials[i].task = task;
			return &credentials[i];
		}
	}

	return 0;
}

static void credential_clear(struct user_credential *credential)
{
	if (credential == 0) {
		return;
	}

	credential->task = 0;
	credential->uid = 0;
	credential->gid = 0;
	credential->euid = 0;
	credential->egid = 0;
	credential->suid = 0;
	credential->sgid = 0;
	credential->group_count = 0;
	for (u64 i = 0; i < USER_MAX_GROUPS; i++) {
		credential->groups[i] = 0;
	}
}

static long credential_register(u64 task)
{
	struct user_credential *credential;

	if (task == 0 || credential_find(task) != 0) {
		return -1;
	}

	credential = credential_alloc(task);
	if (credential == 0) {
		return -1;
	}

	credential->uid = 0;
	credential->gid = 0;
	credential->euid = 0;
	credential->egid = 0;
	credential->suid = 0;
	credential->sgid = 0;
	credential->group_count = 2;
	credential->groups[0] = 0;
	credential->groups[1] = 1;
	for (u64 i = 2; i < USER_MAX_GROUPS; i++) {
		credential->groups[i] = 0;
	}
	return 0;
}

static long credential_fork(u64 parent_task, u64 child_task)
{
	struct user_credential *parent;
	struct user_credential *child;

	if (parent_task == 0 || child_task == 0 ||
	    credential_find(child_task) != 0) {
		return -1;
	}

	parent = credential_find(parent_task);
	if (parent == 0) {
		return -1;
	}

	child = credential_alloc(child_task);
	if (child == 0) {
		return -1;
	}

	child->uid = parent->uid;
	child->gid = parent->gid;
	child->euid = parent->euid;
	child->egid = parent->egid;
	child->suid = parent->suid;
	child->sgid = parent->sgid;
	child->group_count = parent->group_count;
	for (u64 i = 0; i < USER_MAX_GROUPS; i++) {
		child->groups[i] = parent->groups[i];
	}
	return 0;
}

static long credential_exit(u64 task)
{
	struct user_credential *credential = credential_find(task);

	if (credential == 0) {
		return 0;
	}

	credential_clear(credential);
	return 0;
}

static long credential_get(u64 task, u64 type, u64 *value)
{
	struct user_credential *credential = credential_find(task);

	if (credential == 0 || value == 0) {
		return -1;
	}

	switch (type) {
	case BUNIX_USER_GETUID:
		*value = credential->uid;
		return 0;
	case BUNIX_USER_GETGID:
		*value = credential->gid;
		return 0;
	case BUNIX_USER_GETEUID:
		*value = credential->euid;
		return 0;
	case BUNIX_USER_GETEGID:
		*value = credential->egid;
		return 0;
	default:
		return -1;
	}
}

static long credential_getgroups(u64 task, u64 max_groups,
				 struct bunix_msg *reply)
{
	struct user_credential *credential = credential_find(task);

	if (credential == 0 || reply == 0) {
		return -1;
	}
	if (credential->group_count > USER_MAX_GROUPS ||
	    (max_groups != 0 && max_groups < credential->group_count)) {
		return -1;
	}

	reply->words[1] = credential->group_count;
	for (u64 i = 0; i < USER_MAX_GROUPS; i++) {
		reply->words[2 + i] = i < credential->group_count ?
				      credential->groups[i] : 0;
	}
	return 0;
}

int main(void)
{
	const char online[] = "user: online\n";
	const char ready[] = "user: ready\n";
	const char registered[] = "user: registered\n";
	const char forked[] = "user: forked\n";
	struct bunix_msg message;

	bunix_console_write(online, sizeof(online) - 1);
	if (register_service(BUNIX_SERVICE_USER) != 0) {
		return 1;
	}
	bunix_console_write(ready, sizeof(ready) - 1);

	for (;;) {
		struct bunix_msg reply = {
			.protocol = BUNIX_PROTO_USER,
			.type = 0,
			.sender = 0,
			.cap_rights = 0,
			.reply = 0,
			.cap = 0,
			.words = { 0, 0, 0, 0 },
		};

		if (bunix_ipc_recv(BUNIX_HANDLE_SELF, &message) != 0 ||
		    message.protocol != BUNIX_PROTO_USER) {
			continue;
		}

		reply.type = message.type;
		switch (message.type) {
		case BUNIX_USER_GETUID:
		case BUNIX_USER_GETGID:
		case BUNIX_USER_GETEUID:
		case BUNIX_USER_GETEGID:
			reply.words[0] = (u64)credential_get(message.words[0],
							     message.type,
							     &reply.words[1]);
			break;
		case BUNIX_USER_REGISTER_PROCESS:
			reply.words[0] = (u64)credential_register(message.words[0]);
			if (reply.words[0] == 0) {
				bunix_console_write(registered,
						    sizeof(registered) - 1);
			}
			break;
		case BUNIX_USER_FORK_PROCESS:
			reply.words[0] = (u64)credential_fork(message.words[0],
							     message.words[1]);
			if (reply.words[0] == 0) {
				bunix_console_write(forked, sizeof(forked) - 1);
			}
			break;
		case BUNIX_USER_EXIT_PROCESS:
			reply.words[0] = (u64)credential_exit(message.words[0]);
			break;
		case BUNIX_USER_GETGROUPS:
			reply.words[0] = (u64)credential_getgroups(message.words[0],
								   message.words[1],
								   &reply);
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
