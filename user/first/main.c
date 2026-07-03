#include <bunix/syscall.h>

enum {
	FIRST_PID = 1,
	AT_NULL = 0,
	AT_PHDR = 3,
	AT_PHENT = 4,
	AT_PHNUM = 5,
	AT_PAGESZ = 6,
	AT_ENTRY = 9,
	AT_EXECFN = 31,
};

struct startup_aux {
	u64 pagesz;
	u64 entry;
	u64 phdr;
	u64 phent;
	u64 phnum;
	const char *execfn;
};

static u64 stdout_handle;
static u64 stderr_handle;
static u64 time_handle;
static u64 proc_handle;

static void stdout_write(const char *text, u64 len)
{
	const struct bunix_msg message = {
		.protocol = BUNIX_PROTO_CONSOLE,
		.type = BUNIX_CONSOLE_WRITE,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { (u64)text, len, 0, 0 },
	};

	if (stdout_handle != 0) {
		bunix_ipc_send(stdout_handle, &message);
	}
}

static void time_sleep(u64 ns)
{
	const struct bunix_msg request = {
		.protocol = BUNIX_PROTO_TIME,
		.type = BUNIX_TIME_SLEEP_NS,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { ns, 0, 0, 0 },
	};
	struct bunix_msg reply;

	if (time_handle != 0) {
		bunix_ipc_call(time_handle, &request, &reply);
	}
}

static void proc_exit(u64 status)
{
	const struct bunix_msg request = {
		.protocol = BUNIX_PROTO_PROC,
		.type = BUNIX_PROC_EXIT,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { FIRST_PID, status, 0, 0 },
	};
	struct bunix_msg reply;

	if (proc_handle != 0) {
		bunix_ipc_call(proc_handle, &request, &reply);
	}
}

static void stdout_write_argv0(const char *value)
{
	const char prefix[] = "first: argv0=";
	char line[64];
	u64 cursor = 0;

	for (u64 i = 0; i < sizeof(prefix) - 1 && cursor < sizeof(line); i++) {
		line[cursor++] = prefix[i];
	}
	for (u64 i = 0; value[i] != '\0' && cursor + 1 < sizeof(line); i++) {
		line[cursor++] = value[i];
	}
	line[cursor++] = '\n';
	stdout_write(line, cursor);
}

static void append_dec(char *line, u64 *cursor, u64 value)
{
	char digits[20];
	u64 count = 0;

	if (value == 0) {
		line[(*cursor)++] = '0';
		return;
	}

	while (value != 0 && count < sizeof(digits)) {
		digits[count++] = (char)('0' + value % 10);
		value /= 10;
	}
	while (count != 0) {
		line[(*cursor)++] = digits[--count];
	}
}

static void append_hex(char *line, u64 *cursor, u64 value)
{
	const char hex[] = "0123456789abcdef";
	int started = 0;

	line[(*cursor)++] = '0';
	line[(*cursor)++] = 'x';
	for (int shift = 60; shift >= 0; shift -= 4) {
		const u64 digit = (value >> shift) & 0xf;

		if (digit != 0 || started || shift == 0) {
			line[(*cursor)++] = hex[digit];
			started = 1;
		}
	}
}

static void stdout_write_aux_dec(const char *name, u64 value)
{
	char line[64];
	u64 cursor = 0;

	for (u64 i = 0; name[i] != '\0'; i++) {
		line[cursor++] = name[i];
	}
	append_dec(line, &cursor, value);
	line[cursor++] = '\n';
	stdout_write(line, cursor);
}

static void stdout_write_aux_hex(const char *name, u64 value)
{
	char line[64];
	u64 cursor = 0;

	for (u64 i = 0; name[i] != '\0'; i++) {
		line[cursor++] = name[i];
	}
	append_hex(line, &cursor, value);
	line[cursor++] = '\n';
	stdout_write(line, cursor);
}

static void load_auxv(char **envp, struct startup_aux *aux)
{
	u64 *auxv;

	aux->pagesz = 0;
	aux->entry = 0;
	aux->phdr = 0;
	aux->phent = 0;
	aux->phnum = 0;
	aux->execfn = 0;

	if (envp == 0) {
		return;
	}

	while (*envp != 0) {
		envp++;
	}
	auxv = (u64 *)(envp + 1);

	for (;;) {
		const u64 type = auxv[0];
		const u64 value = auxv[1];

		if (type == AT_NULL) {
			break;
		}
		if (type == AT_PAGESZ) {
			aux->pagesz = value;
		} else if (type == AT_ENTRY) {
			aux->entry = value;
		} else if (type == AT_PHDR) {
			aux->phdr = value;
		} else if (type == AT_PHENT) {
			aux->phent = value;
		} else if (type == AT_PHNUM) {
			aux->phnum = value;
		} else if (type == AT_EXECFN) {
			aux->execfn = (const char *)value;
		} else if (type == BUNIX_AT_STDOUT) {
			stdout_handle = value;
		} else if (type == BUNIX_AT_STDERR) {
			stderr_handle = value;
		} else if (type == BUNIX_AT_TIME) {
			time_handle = value;
		} else if (type == BUNIX_AT_PROC) {
			proc_handle = value;
		}
		auxv += 2;
	}
}

static void inspect_auxv(const struct startup_aux *aux)
{
	stdout_write_aux_dec("first: aux pagesz=", aux->pagesz);
	stdout_write_aux_hex("first: aux entry=", aux->entry);
	stdout_write_aux_hex("first: aux phdr=", aux->phdr);
	stdout_write_aux_dec("first: aux phent=", aux->phent);
	stdout_write_aux_dec("first: aux phnum=", aux->phnum);
	stdout_write_aux_dec("first: aux stdout=", stdout_handle);
	stdout_write_aux_dec("first: aux stderr=", stderr_handle);
	stdout_write_aux_dec("first: aux time=", time_handle);
	stdout_write_aux_dec("first: aux proc=", proc_handle);
	if (aux->execfn != 0) {
		stdout_write_argv0(aux->execfn);
	}
}

int main(int argc, char **argv, char **envp)
{
	const char started[] = "first: stdout ready\n";
	const char argc_ok[] = "first: argc=1\n";
	const char exiting[] = "first: exit 0\n";
	struct startup_aux aux;

	load_auxv(envp, &aux);
	stdout_write(started, sizeof(started) - 1);
	if (argc == 1 && argv != 0 && argv[0] != 0) {
		stdout_write(argc_ok, sizeof(argc_ok) - 1);
		stdout_write_argv0(argv[0]);
	}
	inspect_auxv(&aux);
	time_sleep(100000000ULL);
	stdout_write(exiting, sizeof(exiting) - 1);
	proc_exit(0);

	return 0;
}
