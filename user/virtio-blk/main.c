#include <bunix/virtio.h>

enum {
	VIRTIO_BLK_HANDLE_NAMES = 3,
	VIRTIO_BLK_QUEUE_SIZE = 128,
};

static u64 device_service_wait(void)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_NAMES,
		.type = BUNIX_NAMES_WAIT,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = {
			BUNIX_NAMES_ROOT,
			BUNIX_SERVICE_DEVICE,
			BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP,
			0,
		},
	};
	struct bunix_msg reply;

	if (bunix_ipc_call(VIRTIO_BLK_HANDLE_NAMES, &request, &reply) != 0 ||
	    reply.words[0] != BUNIX_DEV_OK) {
		return 0;
	}
	return reply.cap;
}

static int device_call(u64 device_service, u64 type, u64 word0, u64 word1,
		       u64 word2, struct bunix_msg *reply)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_DEVICE,
		.type = type,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { word0, word1, word2, 0 },
	};

	if (reply == 0 ||
	    bunix_ipc_call(device_service, &request, reply) != 0 ||
	    reply->words[0] != BUNIX_DEV_OK) {
		return -1;
	}
	return 0;
}

static long find_block_device(u64 device_service)
{
	struct bunix_msg reply;
	u64 count;

	if (device_call(device_service, BUNIX_DEV_ENUMERATE, 0, 0, 0,
			&reply) != 0) {
		return -1;
	}
	count = reply.words[1];
	for (u64 i = 0; i < count; i++) {
		if (device_call(device_service, BUNIX_DEV_GET_INFO, i, 0, 0,
				&reply) != 0) {
			continue;
		}
		if ((reply.words[3] >> 32) == BUNIX_VIRTIO_DEVICE_BLOCK) {
			return (long)i;
		}
	}
	return -1;
}

int main(void)
{
	const char online[] = "virtio-blk: online\n";
	const char no_device[] = "virtio-blk: no device\n";
	const char negotiated[] = "virtio-blk: negotiated\n";
	const char queue_ready[] = "virtio-blk: queue ready\n";
	const char ready[] = "virtio-blk: ready\n";
	u64 device_service;
	long device_index;
	struct bunix_msg reply;

	bunix_console_log(online, sizeof(online) - 1);
	device_service = device_service_wait();
	if (device_service == 0) {
		bunix_console_log(no_device, sizeof(no_device) - 1);
		return 0;
	}
	device_index = find_block_device(device_service);
	if (device_index < 0) {
		bunix_console_log(no_device, sizeof(no_device) - 1);
		return 0;
	}
	if (device_call(device_service, BUNIX_DEV_NEGOTIATE_FEATURES,
			(u64)device_index, 1ull << BUNIX_VIRTIO_F_VERSION_1,
			0, &reply) != 0) {
		bunix_console_log(no_device, sizeof(no_device) - 1);
		return 1;
	}
	bunix_console_log(negotiated, sizeof(negotiated) - 1);
	if (device_call(device_service, BUNIX_DEV_SETUP_QUEUE,
			(u64)device_index, 0, VIRTIO_BLK_QUEUE_SIZE,
			&reply) != 0 || reply.cap == 0) {
		bunix_console_log(no_device, sizeof(no_device) - 1);
		return 1;
	}
	bunix_console_log(queue_ready, sizeof(queue_ready) - 1);
	if (device_call(device_service, BUNIX_DEV_BIND_DRIVER,
			(u64)device_index, 0, 0, &reply) != 0) {
		bunix_console_log(no_device, sizeof(no_device) - 1);
		return 1;
	}
	bunix_console_log(ready, sizeof(ready) - 1);
	return 0;
}
