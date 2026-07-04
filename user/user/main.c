#include <bunix/id_table.h>
#include <bunix/libbunix.h>

enum {
	USER_HANDLE_NAMES = 3,
	USER_MAX_GROUPS = 2,
	USER_ID_KEEP = (u64)-1,
	USER_ACCOUNT_BUFFER = 512,
	USER_NAME_MAX = 16,
	USER_PASSWORD_MAX = 16,
	USER_TTY_CONSOLE = 1,
};

struct user_account {
	u64 uid;
	u64 gid;
	u64 found;
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

struct user_session {
	u64 id;
	u64 uid;
	u64 gid;
	u64 tty;
	u64 foreground;
};

static struct bunix_map credentials;
static struct bunix_id_table sessions;
static char account_buffer[USER_ACCOUNT_BUFFER];

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

	if (bunix_ipc_call(USER_HANDLE_NAMES, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return 0;
	}

	return reply.cap;
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
	return (struct user_credential *)bunix_map_get(&credentials, task);
}

static struct user_credential *credential_alloc(u64 task)
{
	struct user_credential *credential;

	if (task == 0) {
		return 0;
	}

	credential = (struct user_credential *)
		bunix_calloc(1, sizeof(*credential));
	if (credential == 0) {
		return 0;
	}
	credential->task = task;
	if (bunix_map_set(&credentials, task, (u64)credential) != 0) {
		bunix_free(credential);
		return 0;
	}
	return credential;
}

static void credential_clear(struct user_credential *credential)
{
	if (credential == 0) {
		return;
	}

	(void)bunix_map_remove(&credentials, credential->task);
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
	bunix_free(credential);
}

static int str_eq_len(const char *left, u64 left_len,
		      const char *right, u64 right_len)
{
	if (left_len != right_len) {
		return 0;
	}
	for (u64 i = 0; i < left_len; i++) {
		if (left[i] != right[i]) {
			return 0;
		}
	}
	return 1;
}

static u64 parse_uint(const char *text, u64 len, u64 *cursor)
{
	u64 value = 0;

	while (*cursor < len &&
	       text[*cursor] >= '0' &&
	       text[*cursor] <= '9') {
		value = value * 10 + (u64)(text[*cursor] - '0');
		(*cursor)++;
	}
	return value;
}

static u64 field_end(const char *text, u64 len, u64 cursor, char sep)
{
	while (cursor < len && text[cursor] != sep && text[cursor] != '\n') {
		cursor++;
	}
	return cursor;
}

static long read_account_file(const char *path, char *buffer, u64 buffer_size,
			      u64 *out_len)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_READ_PATH_BUFFER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP,
		.reply = 0,
		.cap = 0,
		.words = { 0, 0, 0, buffer_size - 1 },
	};
	struct bunix_msg reply;
	const u64 vfs = resolve_service(BUNIX_SERVICE_VFS, BUNIX_RIGHT_SEND);
	const long io_buffer = bunix_buffer_create(buffer_size);

	if (vfs == 0 || io_buffer <= 0 || buffer == 0 || out_len == 0) {
		if (io_buffer > 0) {
			bunix_handle_close((u64)io_buffer);
		}
		return -1;
	}

	pack_path(&request.words[0], path);
	request.cap = (u64)io_buffer;
	if (bunix_ipc_call(vfs, &request, &reply) != 0 ||
	    reply.words[0] != 0 ||
	    reply.words[1] >= buffer_size ||
	    bunix_buffer_read((u64)io_buffer, 0, buffer,
			      reply.words[1]) != 0) {
		bunix_handle_close((u64)io_buffer);
		return -1;
	}
	buffer[reply.words[1]] = '\0';
	*out_len = reply.words[1];
	bunix_handle_close((u64)io_buffer);
	return 0;
}

static long account_lookup_name(const char *name, u64 name_len,
				struct user_account *account)
{
	u64 len = 0;

	if (account == 0 ||
	    read_account_file("/etc/passwd", account_buffer,
			      sizeof(account_buffer), &len) != 0) {
		return -1;
	}

	for (u64 cursor = 0; cursor < len;) {
		const u64 line = cursor;
		const u64 name_end = field_end(account_buffer, len, cursor, ':');
		u64 field;

		cursor = name_end;
		if (cursor >= len || account_buffer[cursor] != ':') {
			cursor = field_end(account_buffer, len, line, '\n') + 1;
			continue;
		}
		cursor++;
		field = field_end(account_buffer, len, cursor, ':');
		cursor = field < len ? field + 1 : field;
		account->uid = parse_uint(account_buffer, len, &cursor);
		if (cursor >= len || account_buffer[cursor] != ':') {
			cursor = field_end(account_buffer, len, line, '\n') + 1;
			continue;
		}
		cursor++;
		account->gid = parse_uint(account_buffer, len, &cursor);
		cursor = field_end(account_buffer, len, cursor, '\n') + 1;

		if (str_eq_len(account_buffer + line, name_end - line,
			       name, name_len)) {
			account->found = 1;
			return 0;
		}
	}

	return -1;
}

static long account_lookup_uid(u64 uid, struct user_account *account)
{
	u64 len = 0;

	if (account == 0 ||
	    read_account_file("/etc/passwd", account_buffer,
			      sizeof(account_buffer), &len) != 0) {
		return -1;
	}

	for (u64 cursor = 0; cursor < len;) {
		const u64 line = cursor;
		const u64 name_end = field_end(account_buffer, len, cursor, ':');
		u64 field;
		u64 parsed_uid;

		cursor = name_end;
		if (cursor >= len || account_buffer[cursor] != ':') {
			cursor = field_end(account_buffer, len, line, '\n') + 1;
			continue;
		}
		cursor++;
		field = field_end(account_buffer, len, cursor, ':');
		cursor = field < len ? field + 1 : field;
		parsed_uid = parse_uint(account_buffer, len, &cursor);
		if (cursor >= len || account_buffer[cursor] != ':') {
			cursor = field_end(account_buffer, len, line, '\n') + 1;
			continue;
		}
		cursor++;
		account->gid = parse_uint(account_buffer, len, &cursor);
		cursor = field_end(account_buffer, len, cursor, '\n') + 1;

		if (parsed_uid == uid) {
			(void)name_end;
			account->uid = uid;
			account->found = 1;
			return 0;
		}
	}

	return -1;
}

static long shadow_authenticate(const char *name, u64 name_len,
				const char *password, u64 password_len)
{
	u64 len = 0;

	if (read_account_file("/etc/shadow", account_buffer,
			      sizeof(account_buffer), &len) != 0) {
		return -1;
	}

	for (u64 cursor = 0; cursor < len;) {
		const u64 line = cursor;
		const u64 name_end = field_end(account_buffer, len, cursor, ':');
		u64 password_end;

		cursor = name_end;
		if (cursor >= len || account_buffer[cursor] != ':') {
			cursor = field_end(account_buffer, len, line, '\n') + 1;
			continue;
		}
		cursor++;
		password_end = field_end(account_buffer, len, cursor, ':');
		if (password_end >= len ||
		    account_buffer[password_end] == '\n') {
			password_end = field_end(account_buffer, len, cursor,
						 '\n');
		}

		if (str_eq_len(account_buffer + line, name_end - line,
			       name, name_len) &&
		    str_eq_len(account_buffer + cursor, password_end - cursor,
			       password, password_len)) {
			return 0;
		}
		cursor = field_end(account_buffer, len, line, '\n') + 1;
	}

	return -1;
}

static int group_line_has_member(const char *text, u64 start, u64 end,
				 const char *name, u64 name_len)
{
	u64 cursor = start;

	while (cursor < end) {
		const u64 member_start = cursor;

		while (cursor < end && text[cursor] != ',' &&
		       text[cursor] != '\n') {
			cursor++;
		}
		if (str_eq_len(text + member_start, cursor - member_start,
			       name, name_len)) {
			return 1;
		}
		if (cursor < end && text[cursor] == ',') {
			cursor++;
		}
	}
	return 0;
}

static long login_groups_lookup(const char *name, u64 name_len,
				u64 primary_gid, struct bunix_msg *reply)
{
	u64 len = 0;
	u64 count = 0;
	u64 groups[USER_MAX_GROUPS];

	if (reply == 0 ||
	    read_account_file("/etc/group", account_buffer,
			      sizeof(account_buffer), &len) != 0) {
		return -1;
	}
	for (u64 i = 0; i < USER_MAX_GROUPS; i++) {
		groups[i] = 0;
	}
	for (u64 cursor = 0; cursor < len && count < USER_MAX_GROUPS;) {
		const u64 line = cursor;
		u64 field;
		u64 gid;
		u64 members_start;
		u64 members_end;

		field = field_end(account_buffer, len, cursor, ':');
		cursor = field < len ? field + 1 : field;
		field = field_end(account_buffer, len, cursor, ':');
		cursor = field < len ? field + 1 : field;
		gid = parse_uint(account_buffer, len, &cursor);
		if (cursor >= len || account_buffer[cursor] != ':') {
			cursor = field_end(account_buffer, len, line, '\n') + 1;
			continue;
		}
		cursor++;
		members_start = cursor;
		members_end = field_end(account_buffer, len, cursor, '\n');
		if (gid == primary_gid ||
		    group_line_has_member(account_buffer, members_start,
					  members_end, name, name_len)) {
			int duplicate = 0;

			for (u64 i = 0; i < count; i++) {
				if (groups[i] == gid) {
					duplicate = 1;
				}
			}
			if (!duplicate) {
				groups[count++] = gid;
			}
		}
		cursor = members_end < len ? members_end + 1 : members_end;
	}
	reply->words[1] = count;
	reply->words[2] = groups[0];
	reply->words[3] = USER_MAX_GROUPS > 1 ? groups[1] : 0;
	return 0;
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

static int credential_can_mutate(const struct user_credential *credential)
{
	return credential != 0 && credential->euid == 0;
}

static long credential_setresuid(u64 task, u64 uid, u64 euid, u64 suid)
{
	struct user_credential *credential = credential_find(task);

	if (!credential_can_mutate(credential)) {
		return -1;
	}

	if (uid != USER_ID_KEEP) {
		credential->uid = uid;
	}
	if (euid != USER_ID_KEEP) {
		credential->euid = euid;
	}
	if (suid != USER_ID_KEEP) {
		credential->suid = suid;
	}
	return 0;
}

static long credential_setresgid(u64 task, u64 gid, u64 egid, u64 sgid)
{
	struct user_credential *credential = credential_find(task);

	if (!credential_can_mutate(credential)) {
		return -1;
	}

	if (gid != USER_ID_KEEP) {
		credential->gid = gid;
	}
	if (egid != USER_ID_KEEP) {
		credential->egid = egid;
	}
	if (sgid != USER_ID_KEEP) {
		credential->sgid = sgid;
	}
	return 0;
}

static long credential_setgroups(u64 task, u64 group_count, u64 group0,
				 u64 group1)
{
	struct user_credential *credential = credential_find(task);

	if (!credential_can_mutate(credential) ||
	    group_count > USER_MAX_GROUPS) {
		return -1;
	}

	credential->group_count = group_count;
	credential->groups[0] = group_count > 0 ? group0 : 0;
	credential->groups[1] = group_count > 1 ? group1 : 0;
	return 0;
}

static long credential_has_group(u64 task, u64 gid, u64 *has_group)
{
	struct user_credential *credential = credential_find(task);

	if (credential == 0 || has_group == 0) {
		return -1;
	}

	*has_group = credential->gid == gid;
	for (u64 i = 0; i < credential->group_count && i < USER_MAX_GROUPS; i++) {
		if (credential->groups[i] == gid) {
			*has_group = 1;
		}
	}
	return 0;
}

static long credential_can_access(u64 task, u64 owner, u64 group, u64 mode,
				  u64 mask, u64 *allowed)
{
	struct user_credential *credential = credential_find(task);
	u64 group_member = 0;

	if (allowed == 0 || credential == 0) {
		return -1;
	}

	*allowed = 0;
	if (credential->euid == 0) {
		*allowed = 1;
		return 0;
	}

	if (credential->euid == owner) {
		*allowed = (mode & (mask << 6)) == (mask << 6);
		return 0;
	}

	if (credential_has_group(task, group, &group_member) != 0) {
		return -1;
	}
	if (credential->egid == group || group_member) {
		*allowed = (mode & (mask << 3)) == (mask << 3);
		return 0;
	}

	*allowed = (mode & mask) == mask;
	return 0;
}

static long credential_apply_login(u64 task, u64 login_uid)
{
	struct user_credential *credential = credential_find(task);
	struct user_account account = { 0, 0, 0 };

	if (credential == 0 ||
	    account_lookup_uid(login_uid, &account) != 0 ||
	    !account.found) {
		return -1;
	}

	credential->uid = account.uid;
	credential->gid = account.gid;
	credential->euid = account.uid;
	credential->egid = account.gid;
	credential->suid = account.uid;
	credential->sgid = account.gid;
	credential->group_count = 1;
	credential->groups[0] = account.gid;
	credential->groups[1] = 0;
	return 0;
}

static void unpack_text(char *out, u64 out_size, u64 left, u64 right)
{
	u64 words[2];

	words[0] = left;
	words[1] = right;
	if (out_size == 0) {
		return;
	}
	for (u64 i = 0; i + 1 < out_size && i < 16; i++) {
		const u64 slot = i / 8;
		const u64 shift = (i % 8) * 8;

		out[i] = (char)((words[slot] >> shift) & 0xff);
		if (out[i] == '\0') {
			return;
		}
	}
	out[out_size - 1] = '\0';
}

static u64 text_len(const char *text)
{
	u64 len = 0;

	while (text[len] != '\0') {
		len++;
	}
	return len;
}

static long credential_auth_login(u64 name_left, u64 name_right,
				  u64 password_left, u64 password_right,
				  struct bunix_msg *reply)
{
	char name[USER_NAME_MAX];
	char password[USER_PASSWORD_MAX];
	struct user_account account = { 0, 0, 0 };
	u64 name_len;
	u64 password_len;

	if (reply == 0) {
		return -1;
	}

	unpack_text(name, sizeof(name), name_left, name_right);
	unpack_text(password, sizeof(password), password_left, password_right);
	name_len = text_len(name);
	password_len = text_len(password);
	if (name_len == 0 ||
	    account_lookup_name(name, name_len, &account) != 0 ||
	    !account.found ||
	    shadow_authenticate(name, name_len, password, password_len) != 0) {
		return -1;
	}

	reply->words[1] = account.uid;
	reply->words[2] = account.gid;
	return 0;
}

static struct user_session *session_find(u64 session_id)
{
	if (session_id == 0) {
		return 0;
	}

	return (struct user_session *)bunix_id_get(&sessions, session_id);
}

static long session_begin(u64 uid, u64 gid, u64 tty, struct bunix_msg *reply)
{
	struct user_session *session;
	u64 session_id;

	if (reply == 0 || uid == (u64)-1 || gid == (u64)-1) {
		return -1;
	}
	if (tty == 0) {
		tty = USER_TTY_CONSOLE;
	}

	session = (struct user_session *)bunix_calloc(1, sizeof(*session));
	if (session == 0) {
		return -1;
	}
	session->uid = uid;
	session->gid = gid;
	session->tty = tty;
	session_id = bunix_id_alloc(&sessions, (u64)session);
	if (session_id == 0) {
		bunix_free(session);
		return -1;
	}
	session->id = session_id;
	reply->words[1] = session_id;
	return 0;
}

static long session_end(u64 session_id)
{
	struct user_session *session = session_find(session_id);

	if (session == 0) {
		return -1;
	}

	(void)bunix_id_remove(&sessions, session_id);
	bunix_free(session);
	return 0;
}

static long session_get(u64 session_id, struct bunix_msg *reply)
{
	struct user_session *session = session_find(session_id);

	if (session == 0 || reply == 0) {
		return -1;
	}

	reply->words[1] = session->uid;
	reply->words[2] = session->gid;
	reply->words[3] = (session->tty << 32) | session->foreground;
	return 0;
}

static u64 session_count(void)
{
	return sessions.count;
}

static long session_at(u64 index, struct bunix_msg *reply)
{
	u64 count = 0;

	if (reply == 0) {
		return -1;
	}

	for (u64 i = 0; i < sessions.capacity; i++) {
		struct user_session *session =
			(struct user_session *)sessions.slots[i].value;

		if (session == 0) {
			continue;
		}
		if (count == index) {
			reply->words[1] = session->id;
			reply->words[2] = session->uid;
			reply->words[3] = (session->tty << 32) |
					  session->foreground;
			return 0;
		}
		count++;
	}

	return -1;
}

static long session_set_foreground(u64 session_id, u64 foreground)
{
	struct user_session *session = session_find(session_id);

	if (session == 0 || foreground == 0) {
		return -1;
	}

	session->foreground = foreground;
	return 0;
}

int main(void)
{
	const char online[] = "user: online\n";
	const char ready[] = "user: ready\n";
	const char registered[] = "user: registered\n";
	const char forked[] = "user: forked\n";
	const char session_started[] = "user: session begin\n";
	const char session_ended[] = "user: session end\n";
	struct bunix_msg message;

	bunix_map_init(&credentials);
	bunix_id_table_init(&sessions);
	bunix_console_log(online, sizeof(online) - 1);
	if (register_service(BUNIX_SERVICE_USER) != 0) {
		return 1;
	}
	bunix_console_log(ready, sizeof(ready) - 1);

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
				bunix_console_log(registered,
						    sizeof(registered) - 1);
			}
			break;
		case BUNIX_USER_FORK_PROCESS:
			reply.words[0] = (u64)credential_fork(message.words[0],
							     message.words[1]);
			if (reply.words[0] == 0) {
				bunix_console_log(forked, sizeof(forked) - 1);
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
		case BUNIX_USER_SETRESUID:
			reply.words[0] = (u64)credential_setresuid(message.words[0],
								   message.words[1],
								   message.words[2],
								   message.words[3]);
			break;
		case BUNIX_USER_SETRESGID:
			reply.words[0] = (u64)credential_setresgid(message.words[0],
								   message.words[1],
								   message.words[2],
								   message.words[3]);
			break;
		case BUNIX_USER_SETGROUPS:
			reply.words[0] = (u64)credential_setgroups(message.words[0],
								   message.words[1],
								   message.words[2],
								   message.words[3]);
			break;
		case BUNIX_USER_HAS_GROUP:
			reply.words[0] = (u64)credential_has_group(message.words[0],
								   message.words[1],
								   &reply.words[1]);
			break;
		case BUNIX_USER_CAN_ACCESS:
			reply.words[0] = (u64)credential_can_access(message.words[0],
								    message.words[1],
								    message.words[2],
								    message.words[3] >> 32,
								    message.words[3] & 0xffffffff,
								    &reply.words[1]);
			break;
		case BUNIX_USER_APPLY_LOGIN:
			reply.words[0] = (u64)credential_apply_login(message.words[0],
								     message.words[1]);
			break;
		case BUNIX_USER_AUTH_LOGIN:
			reply.words[0] = (u64)credential_auth_login(message.words[0],
								   message.words[1],
								   message.words[2],
								   message.words[3],
								   &reply);
			break;
		case BUNIX_USER_LOGIN_GROUPS: {
			char name[USER_NAME_MAX];

			unpack_text(name, sizeof(name), message.words[0],
				    message.words[1]);
			reply.words[0] = (u64)login_groups_lookup(name,
								  text_len(name),
								  message.words[2],
								  &reply);
			break;
		}
		case BUNIX_USER_SESSION_BEGIN:
			reply.words[0] = (u64)session_begin(message.words[0],
							    message.words[1],
							    message.words[2],
							    &reply);
			if (reply.words[0] == 0) {
				bunix_console_log(session_started,
						    sizeof(session_started) - 1);
			}
			break;
		case BUNIX_USER_SESSION_END:
			reply.words[0] = (u64)session_end(message.words[0]);
			if (reply.words[0] == 0) {
				bunix_console_log(session_ended,
						    sizeof(session_ended) - 1);
			}
			break;
		case BUNIX_USER_SESSION_GET:
			reply.words[0] = (u64)session_get(message.words[0],
							  &reply);
			break;
		case BUNIX_USER_SESSION_SET_FOREGROUND:
			reply.words[0] = (u64)session_set_foreground(message.words[0],
								     message.words[1]);
			break;
		case BUNIX_USER_SESSION_COUNT:
			reply.words[0] = 0;
			reply.words[1] = session_count();
			break;
		case BUNIX_USER_SESSION_AT:
			reply.words[0] = (u64)session_at(message.words[0],
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
