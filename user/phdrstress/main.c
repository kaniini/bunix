#include <bunix/libbunix.h>

enum {
	AT_NULL = 0,
};

static u64 proc_handle;

static void load_proc_handle(char **envp)
{
	u64 *auxv;

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
			return;
		}
		if (type == BUNIX_AT_PROC) {
			proc_handle = value;
			return;
		}
		auxv += 2;
	}
}

static void proc_exit(u64 status)
{
	struct bunix_msg self_request = {
		.protocol = BUNIX_PROTO_PROC,
		.type = BUNIX_PROC_SELF,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { 0, 0, 0, 0 },
	};
	struct bunix_msg self_reply;
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_PROC,
		.type = BUNIX_PROC_EXIT,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { 0, status, 0, 1 },
	};
	struct bunix_msg reply;

	if (proc_handle != 0 &&
	    bunix_ipc_call(proc_handle, &self_request, &self_reply) == 0 &&
	    self_reply.words[0] == 0) {
		request.words[0] = self_reply.words[1];
		bunix_ipc_call(proc_handle, &request, &reply);
	}
}

int main(int argc, char **argv, char **envp)
{
	const char ok[] = "phdrstress: ok\n";

	(void)argc;
	(void)argv;
	load_proc_handle(envp);
	bunix_console_write(ok, sizeof(ok) - 1);
	proc_exit(0);
	return 0;
}
