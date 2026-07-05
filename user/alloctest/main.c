#include <bunix/alloc.h>
#include <bunix/syscall.h>

enum {
	AT_NULL = 0,
	LARGE_SIZE = 1024 * 1024 + 4096,
	SMALL_COUNT = 128,
	SMALL_SIZE = 96,
};

static u64 proc_handle;

static void write_text(const char *text)
{
	u64 len = 0;

	while (text[len] != '\0') {
		len++;
	}
	(void)bunix_early_console_write(text, len);
}

static void load_auxv(char **envp)
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
			break;
		}
		if (type == BUNIX_AT_PROC) {
			proc_handle = value;
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
		(void)bunix_ipc_call(proc_handle, &request, &reply);
	}
}

static void exit_now(u64 status)
{
	proc_exit(status);
	(void)bunix_syscall1(BUNIX_SYSCALL_EXIT, status);
	for (;;) {
	}
}

static int check_large(unsigned char *ptr, unsigned char seed)
{
	if (ptr == 0) {
		return -1;
	}
	for (u64 i = 0; i < LARGE_SIZE; i += 4096) {
		ptr[i] = (unsigned char)(seed + i / 4096);
	}
	ptr[LARGE_SIZE - 1] = (unsigned char)(seed ^ 0x5a);
	for (u64 i = 0; i < LARGE_SIZE; i += 4096) {
		if (ptr[i] != (unsigned char)(seed + i / 4096)) {
			return -1;
		}
	}
	return ptr[LARGE_SIZE - 1] == (unsigned char)(seed ^ 0x5a) ? 0 : -1;
}

int main(int argc, char **argv, char **envp)
{
	unsigned char *large;
	unsigned char *again;
	unsigned char *small[SMALL_COUNT];

	(void)argc;
	(void)argv;
	load_auxv(envp);

	large = (unsigned char *)bunix_alloc(LARGE_SIZE);
	if (check_large(large, 0x31) != 0) {
		write_text("alloctest: large failed\n");
		exit_now(1);
	}
	bunix_free(large);
	again = (unsigned char *)bunix_alloc(LARGE_SIZE);
	if (again != large || check_large(again, 0x73) != 0) {
		write_text("alloctest: reuse failed\n");
		exit_now(1);
	}
	bunix_free(again);

	for (u64 i = 0; i < SMALL_COUNT; i++) {
		small[i] = (unsigned char *)bunix_alloc(SMALL_SIZE);
		if (small[i] == 0) {
			write_text("alloctest: small failed\n");
			exit_now(1);
		}
		small[i][0] = (unsigned char)i;
		small[i][SMALL_SIZE - 1] = (unsigned char)(i ^ 0xa5);
	}
	for (u64 i = 0; i < SMALL_COUNT; i++) {
		if (small[i][0] != (unsigned char)i ||
		    small[i][SMALL_SIZE - 1] != (unsigned char)(i ^ 0xa5)) {
			write_text("alloctest: small corrupt\n");
			exit_now(1);
		}
		bunix_free(small[i]);
	}

	write_text("alloctest: ok\n");
	exit_now(0);
}
