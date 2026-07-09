#include <bunix/libbunix.h>

enum {
	AT_NULL = 0,
	LINUX_SYSCALL_READ = 0,
	LINUX_SYSCALL_WRITE = 1,
	LINUX_SYSCALL_CHDIR = 80,
	LINUX_SYSCALL_IOCTL = 16,
	LINUX_SYSCALL_EXECVE = 59,
	LINUX_SYSCALL_SETUID = 105,
	LINUX_SYSCALL_SETGID = 106,
	LINUX_SYSCALL_SETGROUPS = 116,
	LINUX_SYSCALL_EXIT_GROUP = 231,
	LINUX_TCGETS = 0x5401,
	LINUX_TCSETS = 0x5402,
	LINUX_ECHO = 0000010,
	LINUX_TERM_LFLAG = 12,
	LOGIN_MAX_GROUPS = 64,
	LOGIN_NAME_MAX = 256,
	LOGIN_PASSWORD_MAX = 512,
};

struct startup_aux {
	u64 stdout_handle;
	u64 proc_handle;
	u64 names_handle;
};

static long linux_syscall4(u64 number, u64 arg0, u64 arg1, u64 arg2, u64 arg3)
{
	long result;
	register u64 r10 __asm__("r10") = arg3;

	__asm__ volatile("syscall"
			 : "=a"(result)
			 : "a"(number), "D"(arg0), "S"(arg1), "d"(arg2),
			   "r"(r10)
			 : "rcx", "r11", "memory");
	return result;
}

static void write_text(const char *text)
{
	u64 len = 0;

	while (text[len] != '\0') {
		len++;
	}
	(void)linux_syscall4(LINUX_SYSCALL_WRITE, 1, (u64)text, len, 0);
}

static long read_text(char *text, u64 len)
{
	return linux_syscall4(LINUX_SYSCALL_READ, 0, (u64)text, len, 0);
}

static unsigned int load_u32(const char *buffer, u64 offset)
{
	unsigned int value = 0;

	for (u64 i = 0; i < 4; i++) {
		value |= ((unsigned int)(unsigned char)buffer[offset + i]) <<
			 (i * 8);
	}

	return value;
}

static void store_u32(char *buffer, u64 offset, unsigned int value)
{
	for (u64 i = 0; i < 4; i++) {
		buffer[offset + i] = (char)((value >> (i * 8)) & 0xff);
	}
}

static long tty_set_echo(int enabled, char *saved)
{
	char termios[60];
	unsigned int lflag;

	if (linux_syscall4(LINUX_SYSCALL_IOCTL, 0, LINUX_TCGETS,
			   (u64)termios, 0) != 0) {
		return -1;
	}
	if (saved != 0) {
		for (u64 i = 0; i < sizeof(termios); i++) {
			saved[i] = termios[i];
		}
	}
	lflag = load_u32(termios, LINUX_TERM_LFLAG);
	if (enabled) {
		lflag |= LINUX_ECHO;
	} else {
		lflag &= ~LINUX_ECHO;
	}
	store_u32(termios, LINUX_TERM_LFLAG, lflag);
	return linux_syscall4(LINUX_SYSCALL_IOCTL, 0, LINUX_TCSETS,
			      (u64)termios, 0);
}

static void tty_restore(const char *saved)
{
	if (saved != 0) {
		(void)linux_syscall4(LINUX_SYSCALL_IOCTL, 0, LINUX_TCSETS,
				     (u64)saved, 0);
	}
}

static void exit_group(u64 status)
{
	(void)linux_syscall4(LINUX_SYSCALL_EXIT_GROUP, status, 0, 0, 0);
	for (;;) {
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

static u64 text_len(const char *text)
{
	u64 len = 0;

	while (text[len] != '\0') {
		len++;
	}
	return len;
}

static void copy_text(char *out, u64 out_size, const char *text)
{
	u64 i = 0;

	if (out_size == 0) {
		return;
	}
	while (i + 1 < out_size && text[i] != '\0') {
		out[i] = text[i];
		i++;
	}
	out[i] = '\0';
}

static void append_text(char *out, u64 out_size, const char *text)
{
	u64 pos = 0;
	u64 in = 0;

	if (out_size == 0) {
		return;
	}
	while (pos + 1 < out_size && out[pos] != '\0') {
		pos++;
	}
	while (pos + 1 < out_size && text[in] != '\0') {
		out[pos++] = text[in++];
	}
	out[pos] = '\0';
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
			 u64 *uid, u64 *gid)
{
	const u64 name_len = text_len(name);
	const u64 password_len = text_len(password);
	const u64 buffer_size = name_len + password_len;
	const long buffer = bunix_buffer_create(buffer_size == 0 ? 1 :
						buffer_size);
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_USER,
		.type = BUNIX_USER_AUTH_LOGIN_BUFFER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_RECV,
		.reply = 0,
		.cap = buffer > 0 ? (u64)buffer : 0,
		.words = { name_len, password_len, 0, 0 },
	};
	struct bunix_msg reply;

	if (uid == 0 || gid == 0 || name_len == 0 || buffer <= 0 ||
	    bunix_buffer_write((u64)buffer, 0, name, name_len) != 0 ||
	    (password_len != 0 &&
	     bunix_buffer_write((u64)buffer, name_len, password,
				password_len) != 0)) {
		if (buffer > 0) {
			bunix_handle_close((u64)buffer);
		}
		return -1;
	}
	if (user == 0 ||
	    bunix_ipc_call(user, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		bunix_handle_close((u64)buffer);
		return -1;
	}

	*uid = reply.words[1];
	*gid = reply.words[2];
	bunix_handle_close((u64)buffer);
	return 0;
}

static long login_groups(u64 user, const char *name, u64 gid, u64 *count,
			 unsigned int *groups)
{
	const u64 name_len = text_len(name);
	const u64 group_bytes = LOGIN_MAX_GROUPS * sizeof(*groups);
	const u64 buffer_size = name_len > group_bytes ? name_len :
				group_bytes;
	const long buffer = bunix_buffer_create(buffer_size == 0 ? 1 :
						buffer_size);
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_USER,
		.type = BUNIX_USER_LOGIN_GROUPS_NAME_BUFFER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_RECV | BUNIX_RIGHT_SEND |
			      BUNIX_RIGHT_DUP,
		.reply = 0,
		.cap = (u64)buffer,
		.words = { name_len, gid, 0, 0 },
	};
	struct bunix_msg reply;

	if (count == 0 || groups == 0 || name_len == 0 || buffer <= 0 ||
	    bunix_buffer_write((u64)buffer, 0, name, name_len) != 0) {
		if (buffer > 0) {
			bunix_handle_close((u64)buffer);
		}
		return -1;
	}
	if (user == 0 ||
	    bunix_ipc_call(user, &request, &reply) != 0 ||
	    reply.words[0] != 0 ||
	    reply.words[1] > LOGIN_MAX_GROUPS ||
	    bunix_buffer_read((u64)buffer, 0, groups,
			      reply.words[1] * sizeof(*groups)) != 0) {
		bunix_handle_close((u64)buffer);
		return -1;
	}
	*count = reply.words[1];
	bunix_handle_close((u64)buffer);
	return 0;
}

static long apply_login(u64 uid, u64 gid, u64 group_count,
			unsigned int *groups)
{
	if (group_count > LOGIN_MAX_GROUPS || groups == 0) {
		return -1;
	}
	if (linux_syscall4(LINUX_SYSCALL_SETGROUPS, group_count,
			   (u64)groups, 0, 0) != 0) {
		return -1;
	}
	if (linux_syscall4(LINUX_SYSCALL_SETGID, gid, 0, 0, 0) != 0) {
		return -1;
	}
	return linux_syscall4(LINUX_SYSCALL_SETUID, uid, 0, 0, 0) == 0 ?
	       0 : -1;
}

static void make_login_env(const char *name, u64 uid,
			   char *home, u64 home_size,
			   char *user, u64 user_size,
			   char *logname, u64 logname_size,
			   char **envp)
{
	static char shell[] = "SHELL=/bin/sh";
	static char path[] = "PATH=/bin:/sbin:/usr/bin:/usr/sbin";
	static char term[] = "TERM=bunix";
	static char histfile[] = "HISTFILE=/dev/null";

	if (uid == 0) {
		copy_text(home, home_size, "HOME=/root");
	} else {
		copy_text(home, home_size, "HOME=/home/");
		append_text(home, home_size, name);
	}
	copy_text(user, user_size, "USER=");
	append_text(user, user_size, name);
	copy_text(logname, logname_size, "LOGNAME=");
	append_text(logname, logname_size, name);
	envp[0] = home;
	envp[1] = user;
	envp[2] = logname;
	envp[3] = shell;
	envp[4] = path;
	envp[5] = term;
	envp[6] = histfile;
	envp[7] = 0;
}

static void make_home_path(const char *name, u64 uid, char *home,
			   u64 home_size)
{
	if (uid == 0) {
		copy_text(home, home_size, "/root");
	} else {
		copy_text(home, home_size, "/home/");
		append_text(home, home_size, name);
	}
}

static long exec_shell(const char *name, u64 uid)
{
	char path[] = "/bin/sh";
	char *argv[] = { path, 0 };
	char home[LOGIN_NAME_MAX + 16];
	char home_path[LOGIN_NAME_MAX + 8];
	char user[LOGIN_NAME_MAX + 8];
	char logname[LOGIN_NAME_MAX + 12];
	char *shell_env[8];

	make_login_env(name, uid, home, sizeof(home), user, sizeof(user),
		       logname, sizeof(logname), shell_env);
	make_home_path(name, uid, home_path, sizeof(home_path));
	if (linux_syscall4(LINUX_SYSCALL_CHDIR, (u64)home_path, 0, 0, 0) != 0) {
		return -1;
	}
	return linux_syscall4(LINUX_SYSCALL_EXECVE, (u64)path,
			      (u64)argv, (u64)shell_env, 0);
}

static long session_begin(u64 user, u64 uid, u64 gid, u64 *session_id)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_USER,
		.type = BUNIX_USER_SESSION_BEGIN,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { uid, gid, 1, 0 },
	};
	struct bunix_msg reply;

	if (user == 0 ||
	    session_id == 0 ||
	    bunix_ipc_call(user, &request, &reply) != 0 ||
	    reply.words[0] != 0 ||
	    reply.words[1] == 0) {
		return -1;
	}

	*session_id = reply.words[1];
	return 0;
}

static long session_end(u64 user, u64 session_id)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_USER,
		.type = BUNIX_USER_SESSION_END,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { session_id, 0, 0, 0 },
	};
	struct bunix_msg reply;

	if (user == 0 ||
	    bunix_ipc_call(user, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return -1;
	}

	return 0;
}

static long attach_session(u64 linux, u64 session_id)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_LINUX,
		.type = BUNIX_LINUX_ATTACH_SESSION,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { session_id, 0, 0, 0 },
	};
	struct bunix_msg reply;

	if (linux == 0 || session_id == 0 ||
	    bunix_ipc_call(linux, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return -1;
	}

	return 0;
}

int main(int argc, char **argv, char **envp)
{
	struct startup_aux aux;
	char name[LOGIN_NAME_MAX];
	char password[LOGIN_PASSWORD_MAX];
	u64 uid = 0;
	u64 gid = 0;
	u64 group_count = 0;
	unsigned int groups[LOGIN_MAX_GROUPS];
	u64 user;
	u64 user_mgmt;
	u64 linux_mgmt;
	u64 session_id;
	long nread;
	char saved_termios[60];
	int restore_termios;

	(void)argc;
	(void)argv;
	load_auxv(envp, &aux);
	user = resolve_service(aux.names_handle, BUNIX_SERVICE_USER,
			       BUNIX_RIGHT_SEND);
	user_mgmt = bunix_handle_find(BUNIX_CAP_USRM);
	linux_mgmt = bunix_handle_find(BUNIX_CAP_LNXM);
	for (;;) {
		restore_termios = tty_set_echo(0, saved_termios) == 0;
		write_text("login: ");
		nread = read_text(name, sizeof(name));
		if (nread <= 0) {
			if (restore_termios) {
				tty_restore(saved_termios);
			}
			continue;
		}
		strip_line(name, (u64)nread);
		write_text(name);
		write_text("\n");
		write_text("password: ");
		nread = read_text(password, sizeof(password));
		if (restore_termios) {
			tty_restore(saved_termios);
		}
		write_text("\n");
		if (nread <= 0) {
			continue;
		}
		strip_line(password, (u64)nread);

		if (user == 0) {
			write_text("login: user service missing\n");
		} else if (authenticate(user, name, password, &uid, &gid) != 0) {
			write_text("login: auth failed\n");
		} else if (login_groups(user, name, gid, &group_count,
					groups) != 0) {
			write_text("login: groups failed\n");
		} else if (session_begin(user_mgmt, uid, gid, &session_id) != 0) {
			write_text("login: session failed\n");
		} else if (attach_session(linux_mgmt, session_id) != 0) {
			write_text("login: session attach failed\n");
			(void)session_end(user_mgmt, session_id);
		} else if (apply_login(uid, gid, group_count, groups) != 0) {
			write_text("login: apply failed\n");
			(void)session_end(user_mgmt, session_id);
		} else {
			write_text("login: shell exec\n");
			if (exec_shell(name, uid) != 0) {
				(void)session_end(user_mgmt, session_id);
			}
		}
		write_text("login: authentication failed\n");
	}
	exit_group(1);
	return 1;
}
