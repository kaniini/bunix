#include <bunix/libbunix.h>

enum {
	LOGIN_PID = 4,
	AT_NULL = 0,
	BUNIX_PROC_SPAWN_SET_LOGIN = 1,
};

struct startup_aux {
	u64 stdout_handle;
	u64 proc_handle;
	u64 names_handle;
};

static void write_text(u64 handle, const char *text)
{
	u64 len = 0;
	struct bunix_msg message = {
		.protocol = BUNIX_PROTO_CONSOLE,
		.type = BUNIX_CONSOLE_WRITE,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { (u64)text, 0, 0, 0 },
	};

	while (text[len] != '\0') {
		len++;
	}
	message.words[1] = len;
	if (handle != 0) {
		bunix_ipc_send(handle, &message);
	}
}

static void pack_text(u64 *words, const char *text)
{
	words[0] = 0;
	words[1] = 0;
	for (u64 i = 0; i < 16 && text[i] != '\0'; i++) {
		const u64 slot = i / 8;
		const u64 shift = (i % 8) * 8;

		words[slot] |= ((u64)(unsigned char)text[i]) << shift;
	}
}

static void strip_line(char *text, u64 len)
{
	for (u64 i = 0; i < len; i++) {
		if (text[i] == '\n' || text[i] == '\r') {
			text[i] = '\0';
			return;
		}
	}
	if (len != 0) {
		text[len - 1] = '\0';
	}
}

static void load_auxv(char **envp, struct startup_aux *aux)
{
	u64 *entry;

	aux->stdout_handle = 0;
	aux->proc_handle = 0;
	aux->names_handle = 0;
	while (*envp != 0) {
		envp++;
	}
	entry = (u64 *)(envp + 1);
	for (;;) {
		const u64 type = entry[0];
		const u64 value = entry[1];

		if (type == AT_NULL) {
			return;
		}
		if (type == BUNIX_AT_STDOUT) {
			aux->stdout_handle = value;
		} else if (type == BUNIX_AT_PROC) {
			aux->proc_handle = value;
		} else if (type == BUNIX_AT_NAMES) {
			aux->names_handle = value;
		}
		entry += 2;
	}
}

static u64 resolve_service(u64 names, u64 service, unsigned int rights)
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

	if (names == 0 ||
	    bunix_ipc_call(names, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return 0;
	}

	return reply.cap;
}

static long authenticate(u64 user, const char *name, const char *password,
			 u64 *uid)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_USER,
		.type = BUNIX_USER_AUTH_LOGIN,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { 0, 0, 0, 0 },
	};
	struct bunix_msg reply;

	pack_text(&request.words[0], name);
	pack_text(&request.words[2], password);
	if (user == 0 ||
	    bunix_ipc_call(user, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return -1;
	}

	*uid = reply.words[1];
	return 0;
}

static long spawn_shell(u64 proc, u64 uid)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_PROC,
		.type = BUNIX_PROC_SPAWN,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { 0, 0, uid, BUNIX_PROC_SPAWN_SET_LOGIN },
	};
	struct bunix_msg reply;

	pack_text(&request.words[0], "/bin/sh");
	if (proc == 0 ||
	    bunix_ipc_call(proc, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return -1;
	}

	return 0;
}

static void proc_exit(u64 proc, u64 status)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_PROC,
		.type = BUNIX_PROC_EXIT,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { LOGIN_PID, status, 0, 0 },
	};
	struct bunix_msg reply;

	if (proc != 0) {
		bunix_ipc_call(proc, &request, &reply);
	}
}

int main(int argc, char **argv, char **envp)
{
	struct startup_aux aux;
	char name[16];
	char password[16];
	u64 uid = 0;
	u64 user;
	long nread;

	(void)argc;
	(void)argv;
	load_auxv(envp, &aux);
	user = resolve_service(aux.names_handle, BUNIX_SERVICE_USER,
			       BUNIX_RIGHT_SEND);
	for (;;) {
		write_text(aux.stdout_handle, "login: ");
		nread = bunix_console_read(name, sizeof(name));
		if (nread <= 0) {
			continue;
		}
		strip_line(name, (u64)nread);
		write_text(aux.stdout_handle, "password: ");
		nread = bunix_console_read(password, sizeof(password));
		if (nread <= 0) {
			continue;
		}
		strip_line(password, (u64)nread);

		if (authenticate(user, name, password, &uid) == 0 &&
		    spawn_shell(aux.proc_handle, uid) == 0) {
			write_text(aux.stdout_handle, "login: shell spawned\n");
			proc_exit(aux.proc_handle, 0);
			return 0;
		}
		write_text(aux.stdout_handle, "login: authentication failed\n");
	}
}
