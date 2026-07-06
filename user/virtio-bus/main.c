#include <bunix/virtio.h>

enum {
	VIRTIO_BUS_HANDLE_NAMES = 3,
};

static long register_service(u64 service, u64 handle)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_NAMES,
		.type = BUNIX_NAMES_REGISTER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP,
		.reply = 0,
		.cap = handle,
		.words = { BUNIX_NAMES_ROOT, service, 0, 0 },
	};
	struct bunix_msg reply;

	if (bunix_ipc_call(VIRTIO_BUS_HANDLE_NAMES, &request, &reply) != 0) {
		return -1;
	}
	return reply.words[0] == BUNIX_DEV_OK ? 0 : -1;
}

static void reply_empty_enumeration(struct bunix_msg *reply)
{
	reply->words[0] = BUNIX_DEV_OK;
	reply->words[1] = 0;
	reply->words[2] = 0;
	reply->words[3] = 0;
}

static void reply_no_device(struct bunix_msg *reply)
{
	reply->words[0] = BUNIX_DEV_ERR_NOENT;
	reply->words[1] = 0;
	reply->words[2] = 0;
	reply->words[3] = 0;
}

int main(void)
{
	const char online[] = "virtio-bus: online\n";
	const char ready[] = "virtio-bus: ready devices=0\n";
	struct bunix_msg message;

	bunix_console_log(online, sizeof(online) - 1);
	if (register_service(BUNIX_SERVICE_DEVICE, BUNIX_HANDLE_SELF) != 0) {
		return 1;
	}
	bunix_console_log(ready, sizeof(ready) - 1);

	for (;;) {
		struct bunix_msg reply = {
			.protocol = BUNIX_PROTO_DEVICE,
			.type = 0,
			.sender = 0,
			.cap_rights = 0,
			.reply = 0,
			.cap = 0,
			.words = { BUNIX_DEV_ERR_UNSUPPORTED, 0, 0, 0 },
		};

		if (bunix_ipc_recv(BUNIX_HANDLE_SELF, &message) != 0 ||
		    message.protocol != BUNIX_PROTO_DEVICE) {
			continue;
		}

		reply.type = message.type;
		switch (message.type) {
		case BUNIX_DEV_ENUMERATE:
			reply_empty_enumeration(&reply);
			break;
		case BUNIX_DEV_GET_INFO:
		case BUNIX_DEV_GET_RESOURCE:
		case BUNIX_DEV_BIND_DRIVER:
		case BUNIX_DEV_RESET:
		case BUNIX_DEV_READ_FEATURES:
		case BUNIX_DEV_NEGOTIATE_FEATURES:
		case BUNIX_DEV_SETUP_QUEUE:
		case BUNIX_DEV_NOTIFY_QUEUE:
		case BUNIX_DEV_ACK_INTERRUPT:
			reply_no_device(&reply);
			break;
		default:
			reply.words[0] = BUNIX_DEV_ERR_UNSUPPORTED;
			break;
		}

		if (message.reply != 0) {
			bunix_ipc_send(message.reply, &reply);
		}
	}
}
