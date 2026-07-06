#include <bunix/virtio.h>

enum {
	VIRTIO_NET_HANDLE_NAMES = 3,
	VIRTIO_NET_QUEUE_SIZE = 128,
	VIRTIO_NET_DEFAULT_MTU = 1500,
};

struct virtio_net_queue {
	u64 handle;
	u64 phys;
	u64 queue_size;
	struct bunix_virtio_queue_layout layout;
};

struct virtio_net_device {
	u64 device_service;
	u64 net_service;
	u64 device_index;
	u64 features;
	u64 iface_id;
	u64 mac_hi;
	u64 mac_lo;
	u64 mtu;
	u64 link_flags;
	struct virtio_net_queue rx;
	struct virtio_net_queue tx;
};

static void log_text(const char *text, u64 len)
{
	(void)bunix_console_log(text, len);
}

static u64 text_len(const char *text)
{
	u64 len = 0;

	while (text[len] != 0) {
		len++;
	}
	return len;
}

static void append_char(char *line, u64 size, u64 *offset, char c)
{
	if (*offset + 1 >= size) {
		return;
	}
	line[*offset] = c;
	*offset += 1;
	line[*offset] = 0;
}

static void append_text(char *line, u64 size, u64 *offset, const char *text)
{
	for (u64 i = 0; text[i] != 0; i++) {
		append_char(line, size, offset, text[i]);
	}
}

static void append_u64_dec(char *line, u64 size, u64 *offset, u64 value)
{
	char tmp[20];
	u64 count = 0;

	if (value == 0) {
		append_char(line, size, offset, '0');
		return;
	}
	while (value != 0 && count < sizeof(tmp)) {
		tmp[count++] = (char)('0' + (value % 10));
		value /= 10;
	}
	while (count != 0) {
		append_char(line, size, offset, tmp[--count]);
	}
}

static void log_iface(u64 iface, u64 mtu)
{
	char line[80];
	u64 offset = 0;

	line[0] = 0;
	append_text(line, sizeof(line), &offset, "virtio-net: attached iface=");
	append_u64_dec(line, sizeof(line), &offset, iface);
	append_text(line, sizeof(line), &offset, " mtu=");
	append_u64_dec(line, sizeof(line), &offset, mtu);
	append_char(line, sizeof(line), &offset, '\n');
	log_text(line, offset);
}

static u64 service_wait(u64 service)
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
			service,
			BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP,
			0,
		},
	};
	struct bunix_msg reply;

	if (bunix_ipc_call(VIRTIO_NET_HANDLE_NAMES, &request, &reply) != 0 ||
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

static long find_net_device(u64 device_service)
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
		if ((reply.words[3] >> 32) == BUNIX_VIRTIO_DEVICE_NET) {
			return (long)i;
		}
	}
	return -1;
}

static int queue_from_reply(struct bunix_msg *reply,
			    struct virtio_net_queue *queue)
{
	long phys;

	if (reply == 0 || queue == 0 || reply->cap == 0) {
		return -1;
	}
	queue->handle = reply->cap;
	queue->queue_size = reply->words[1] & 0xffffffffull;
	if (bunix_virtio_queue_layout_init(&queue->layout, queue->queue_size,
					   BUNIX_VIRTIO_MODERN_QUEUE_ALIGNMENT) !=
		    0 ||
	    queue->layout.total_len != reply->words[2]) {
		return -1;
	}
	phys = bunix_buffer_phys(queue->handle);
	if (phys <= 0) {
		return -1;
	}
	queue->phys = (u64)phys;
	return 0;
}

static int queue_zero_avail(struct virtio_net_queue *queue)
{
	const unsigned short zero = 0;

	if (queue == 0 || queue->handle == 0) {
		return -1;
	}
	return bunix_buffer_write(queue->handle, queue->layout.avail_offset,
				  &zero, sizeof(zero)) == 0 &&
		       bunix_buffer_write(queue->handle,
					  queue->layout.avail_offset + 2,
					  &zero, sizeof(zero)) == 0 ?
		       0 :
		       -1;
}

static u64 mac_pack_hi(u64 raw_config)
{
	u64 value = 0;

	for (u64 i = 0; i < 6; i++) {
		value <<= 8;
		value |= (raw_config >> (i * 8)) & 0xff;
	}
	return value;
}

static void config_read(struct virtio_net_device *device)
{
	struct bunix_msg reply;
	u64 raw0 = 0;
	u64 raw8 = 0;

	device->mac_hi = 0x525400180001ull;
	device->mac_lo = 0;
	device->mtu = VIRTIO_NET_DEFAULT_MTU;
	device->link_flags = BUNIX_NET_IFACE_FLAG_BROADCAST;
	if (device_call(device->device_service, BUNIX_DEV_READ_CONFIG64,
			device->device_index, 0, 0, &reply) == 0) {
		raw0 = reply.words[1];
	}
	if (device_call(device->device_service, BUNIX_DEV_READ_CONFIG64,
			device->device_index, 8, 0, &reply) == 0) {
		raw8 = reply.words[1];
	}
	if ((device->features & (1ull << BUNIX_VIRTIO_NET_F_MAC)) != 0 &&
	    raw0 != 0) {
		device->mac_hi = mac_pack_hi(raw0);
	}
	if ((device->features & (1ull << BUNIX_VIRTIO_NET_F_MTU)) != 0) {
		u64 mtu = (raw8 >> 16) & 0xffff;

		if (mtu != 0) {
			device->mtu = mtu;
		}
	}
	if ((device->features & (1ull << BUNIX_VIRTIO_NET_F_STATUS)) == 0 ||
	    (((raw0 >> 48) & BUNIX_VIRTIO_NET_S_LINK_UP) != 0)) {
		device->link_flags |= BUNIX_NET_IFACE_FLAG_UP |
				      BUNIX_NET_IFACE_FLAG_RUNNING;
	}
}

static int setup_queues(struct virtio_net_device *device)
{
	struct bunix_msg reply;

	if (device_call(device->device_service, BUNIX_DEV_SETUP_QUEUE,
			device->device_index, 0, VIRTIO_NET_QUEUE_SIZE,
			&reply) != 0 ||
	    queue_from_reply(&reply, &device->rx) != 0 ||
	    queue_zero_avail(&device->rx) != 0 ||
	    device_call(device->device_service, BUNIX_DEV_SETUP_QUEUE,
			device->device_index, 1, VIRTIO_NET_QUEUE_SIZE,
			&reply) != 0 ||
	    queue_from_reply(&reply, &device->tx) != 0 ||
	    queue_zero_avail(&device->tx) != 0) {
		return -1;
	}
	return 0;
}

static int attach_net_interface(struct virtio_net_device *device)
{
	struct bunix_net_packet_interface_info info = {
		.id = 0,
		.flags = device->link_flags,
		.mtu = device->mtu,
		.mac_hi = device->mac_hi,
		.mac_lo = device->mac_lo,
	};
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_NET,
		.type = BUNIX_NET_PACKET_INTERFACE_ATTACH,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_SEND | BUNIX_RIGHT_RECV,
		.reply = 0,
		.cap = 0,
		.words = { 0, 0, 0, 0 },
	};
	struct bunix_msg reply;
	long buffer = bunix_buffer_create(sizeof(info));

	if (buffer <= 0) {
		return -1;
	}
	request.cap = (u64)buffer;
	if (bunix_buffer_write((u64)buffer, 0, &info, sizeof(info)) != 0 ||
	    bunix_ipc_call(device->net_service, &request, &reply) != 0 ||
	    reply.words[0] != 0 ||
	    bunix_buffer_read((u64)buffer, 0, &info, sizeof(info)) != 0) {
		bunix_handle_close((u64)buffer);
		return -1;
	}
	bunix_handle_close((u64)buffer);
	device->iface_id = info.id;
	return info.id >= 2 ? 0 : -1;
}

static int init_device(struct virtio_net_device *device)
{
	struct bunix_msg reply;
	long index;
	u64 required = 1ull << BUNIX_VIRTIO_F_VERSION_1;
	u64 optional = (1ull << BUNIX_VIRTIO_NET_F_MAC) |
		       (1ull << BUNIX_VIRTIO_NET_F_STATUS) |
		       (1ull << BUNIX_VIRTIO_NET_F_MTU);

	device->device_service = service_wait(BUNIX_SERVICE_DEVICE);
	device->net_service = service_wait(BUNIX_SERVICE_NET);
	if (device->device_service == 0 || device->net_service == 0) {
		return -1;
	}
	index = find_net_device(device->device_service);
	if (index < 0) {
		return -1;
	}
	device->device_index = (u64)index;
	if (device_call(device->device_service, BUNIX_DEV_NEGOTIATE_FEATURES,
			device->device_index, required, optional, &reply) != 0) {
		return -1;
	}
	device->features = reply.words[2];
	log_text("virtio-net: negotiated\n", text_len("virtio-net: negotiated\n"));
	config_read(device);
	if (setup_queues(device) != 0) {
		return -1;
	}
	log_text("virtio-net: queues ready\n",
		 text_len("virtio-net: queues ready\n"));
	if (device_call(device->device_service, BUNIX_DEV_BIND_DRIVER,
			device->device_index, 0, 0, &reply) != 0 ||
	    attach_net_interface(device) != 0) {
		return -1;
	}
	log_iface(device->iface_id, device->mtu);
	return 0;
}

int main(void)
{
	struct virtio_net_device device = { 0 };

	log_text("virtio-net: online\n", text_len("virtio-net: online\n"));
	if (init_device(&device) != 0) {
		log_text("virtio-net: init failed\n",
			 text_len("virtio-net: init failed\n"));
		return 1;
	}
	for (;;) {
		bunix_sleep_ns(1000000000ull);
	}
}
