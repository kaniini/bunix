#include <bunix/libbunix.h>

enum {
	NAMES_TEST_HANDLE_NAMES = 3,
	NAMES_TEST_SERVICE = ('N') | ('T' << 8) | ('S' << 16) | ('T' << 24),
	NAMES_TEST_OWNED_SERVICE =
		('N') | ('O' << 8) | ('W' << 16) | ('N' << 24),
};

static long names_register(u64 service)
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

	if (bunix_ipc_call(NAMES_TEST_HANDLE_NAMES, &request, &reply) != 0) {
		return -1;
	}
	return reply.words[0] == 0 ? 0 : -1;
}

static long names_create_namespace(void)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_NAMES,
		.type = BUNIX_NAMES_CREATE_NS,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { 0, 0, 0, 0 },
	};
	struct bunix_msg reply;

	if (bunix_ipc_call(NAMES_TEST_HANDLE_NAMES, &request, &reply) != 0) {
		return -1;
	}
	return reply.words[0] == 0 ? 0 : -1;
}

int main(void)
{
	const char spoof_denied[] = "names-test: spoof denied\n";
	const char owner_refresh_ok[] = "names-test: owner refresh ok\n";
	const char namespace_denied[] = "names-test: namespace denied\n";
	const char ok[] = "names-test: ok\n";

	if (names_register(NAMES_TEST_SERVICE) == 0) {
		return 1;
	}
	bunix_console_log(spoof_denied, sizeof(spoof_denied) - 1);

	if (names_register(NAMES_TEST_OWNED_SERVICE) != 0 ||
	    names_register(NAMES_TEST_OWNED_SERVICE) != 0) {
		return 1;
	}
	bunix_console_log(owner_refresh_ok, sizeof(owner_refresh_ok) - 1);

	if (names_create_namespace() == 0) {
		return 1;
	}
	bunix_console_log(namespace_denied, sizeof(namespace_denied) - 1);
	bunix_console_log(ok, sizeof(ok) - 1);
	return 0;
}
