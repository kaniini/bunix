#include <bunix/libbunix.h>

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

	if (bunix_ipc_call(bunix_handle_find(BUNIX_CAP_NAME),
			   &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return 0;
	}
	return reply.cap;
}

static int call_denied(u64 port, unsigned int protocol, unsigned int type,
		       u64 word0, u64 word1, u64 word2, u64 word3)
{
	struct bunix_msg request = {
		.protocol = protocol,
		.type = type,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { word0, word1, word2, word3 },
	};
	struct bunix_msg reply;

	if (port == 0 ||
	    bunix_ipc_call(port, &request, &reply) != 0) {
		return -1;
	}
	return reply.words[0] != 0 ? 0 : -1;
}

int main(void)
{
	const char user_denied[] = "mgmt-test: user denied\n";
	const char proc_denied[] = "mgmt-test: proc denied\n";
	const char linux_denied[] = "mgmt-test: linux denied\n";
	const char ok[] = "mgmt-test: ok\n";
	const u64 user = resolve_service(BUNIX_SERVICE_USER, BUNIX_RIGHT_SEND);
	const u64 proc = resolve_service(BUNIX_SERVICE_PROC, BUNIX_RIGHT_SEND);
	const u64 linux = resolve_service(BUNIX_SERVICE_LINUX,
					  BUNIX_RIGHT_SEND);

	if (call_denied(user, BUNIX_PROTO_USER,
			BUNIX_USER_REGISTER_PROCESS, 0x55550001, 0, 0, 0) != 0) {
		return 1;
	}
	bunix_console_log(user_denied, sizeof(user_denied) - 1);

	if (call_denied(proc, BUNIX_PROTO_PROC,
			BUNIX_PROC_EXIT, 0x55550002, 0, 1, 0) != 0) {
		return 1;
	}
	bunix_console_log(proc_denied, sizeof(proc_denied) - 1);

	if (call_denied(linux, BUNIX_PROTO_LINUX,
			BUNIX_LINUX_REGISTER_PROCESS, 0x55550003, 0, 0, 0) != 0) {
		return 1;
	}
	bunix_console_log(linux_denied, sizeof(linux_denied) - 1);

	bunix_console_log(ok, sizeof(ok) - 1);
	return 0;
}
