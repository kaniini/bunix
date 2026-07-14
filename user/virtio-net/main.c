#include <bunix/virtio.h>

#define VIRTIO_NET_HANDLE_NAMES (bunix_handle_find(BUNIX_CAP_NAME))

enum {
	VIRTIO_NET_QUEUE_SIZE = 128,
	VIRTIO_NET_RX_BUFFERS = 8,
	VIRTIO_NET_DEFAULT_MTU = 1500,
	VIRTIO_NET_MAX_MTU = 2048,
	VIRTIO_NET_RX_BUFFER_SIZE = sizeof(struct bunix_virtio_net_header) +
				    VIRTIO_NET_MAX_MTU,
	VIRTIO_NET_TX_BUFFER_SIZE = sizeof(struct bunix_virtio_net_header) +
				    VIRTIO_NET_MAX_MTU,
	VIRTIO_NET_RX_POLL_ROUNDS = 16,
	VIRTIO_NET_TX_POLL_ROUNDS = 64,
	VIRTIO_NET_IDLE_SLEEP_NS = 1000000ull,
	VIRTIO_NET_ACTIVE_SLEEP_NS = 100000ull,
	VIRTIO_NET_ACTIVE_SLEEP_ROUNDS = 256,
};

struct virtio_net_queue {
	u64 handle;
	u64 phys;
	u64 queue_size;
	u64 next_avail;
	u64 seen_used;
	struct bunix_virtio_queue_layout layout;
};

struct virtio_net_device {
	u64 device_service;
	u64 net_service;
	u64 device_index;
	u64 features;
	u64 iface_id;
	u64 rx_submit_buffer;
	u64 tx_dequeue_buffer;
	u64 tx_buffer;
	u64 tx_phys;
	u64 tx_inflight;
	u64 tx_inflight_len;
	u64 mac_hi;
	u64 mac_lo;
	u64 mtu;
	u64 link_flags;
	struct virtio_net_queue rx;
	struct virtio_net_queue tx;
	u64 rx_buffer[VIRTIO_NET_RX_BUFFERS];
	u64 rx_phys[VIRTIO_NET_RX_BUFFERS];
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
	if (bunix_buffer_write(queue->handle, queue->layout.avail_offset,
			       &zero, sizeof(zero)) != 0 ||
	    bunix_buffer_write(queue->handle, queue->layout.avail_offset + 2,
			       &zero, sizeof(zero)) != 0) {
		return -1;
	}
	queue->next_avail = 0;
	queue->seen_used = 0;
	return 0;
}

static int queue_write(u64 handle, u64 offset, const void *src, u64 len)
{
	return bunix_buffer_write(handle, offset, src, len) == 0 ? 0 : -1;
}

static int queue_read_u16(u64 handle, u64 offset, unsigned short *value)
{
	return bunix_buffer_read(handle, offset, value, sizeof(*value)) == 0 ? 0 :
									-1;
}

static int queue_read_used_elem(struct virtio_net_queue *queue, u64 index,
				struct bunix_virtio_used_elem *elem)
{
	u64 offset;

	if (queue == 0 || elem == 0 || queue->handle == 0) {
		return -1;
	}
	offset = queue->layout.used_offset + 2 * sizeof(unsigned short) +
		 (index % queue->queue_size) * sizeof(*elem);
	return bunix_buffer_read(queue->handle, offset, elem, sizeof(*elem)) == 0 ?
		       0 :
		       -1;
}

static int queue_write_desc(struct virtio_net_queue *queue, u64 index,
			    u64 addr, u64 len, u64 flags, u64 next)
{
	struct bunix_virtio_descriptor desc;

	if (queue == 0 || index >= queue->queue_size) {
		return -1;
	}
	bunix_virtio_desc_set(&desc, addr, len, flags, next);
	return queue_write(queue->handle,
			   queue->layout.desc_offset +
				   index * sizeof(desc),
			   &desc, sizeof(desc));
}

static int queue_avail_put(struct virtio_net_queue *queue, u64 head)
{
	unsigned short entry;
	unsigned short next;
	u64 ring_offset;

	if (queue == 0 || queue->handle == 0 || head >= queue->queue_size ||
	    queue->next_avail - queue->seen_used >= queue->queue_size) {
		return -1;
	}
	entry = (unsigned short)head;
	next = (unsigned short)(queue->next_avail + 1);
	ring_offset = queue->layout.avail_offset + 2 * sizeof(unsigned short) +
		      (queue->next_avail % queue->queue_size) *
			      sizeof(unsigned short);
	if (queue_write(queue->handle, ring_offset, &entry, sizeof(entry)) != 0 ||
	    queue_write(queue->handle, queue->layout.avail_offset + 2, &next,
			sizeof(next)) != 0) {
		return -1;
	}
	queue->next_avail++;
	return 0;
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
			if (mtu > VIRTIO_NET_MAX_MTU) {
				mtu = VIRTIO_NET_MAX_MTU;
			}
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

static int notify_queue(struct virtio_net_device *device, u64 queue_index)
{
	struct bunix_msg reply;

	return device_call(device->device_service, BUNIX_DEV_NOTIFY_QUEUE,
			   device->device_index, queue_index, 0, &reply);
}

static int replenish_rx(struct virtio_net_device *device)
{
	for (u64 i = 0; i < VIRTIO_NET_RX_BUFFERS; i++) {
		long handle = bunix_buffer_create(VIRTIO_NET_RX_BUFFER_SIZE);
		long phys;

		if (handle <= 0) {
			return -1;
		}
		phys = bunix_buffer_phys((u64)handle);
		if (phys <= 0) {
			bunix_handle_close((u64)handle);
			return -1;
		}
		device->rx_buffer[i] = (u64)handle;
		device->rx_phys[i] = (u64)phys;
		if (queue_write_desc(&device->rx, i, device->rx_phys[i],
				     VIRTIO_NET_RX_BUFFER_SIZE,
				     BUNIX_VIRTIO_DESC_F_WRITE, 0) != 0 ||
		    queue_avail_put(&device->rx, i) != 0) {
			return -1;
		}
	}
	return notify_queue(device, 0);
}

static int submit_rx_frame(struct virtio_net_device *device, u64 buffer,
			   u64 frame_len)
{
	struct bunix_net_packet_info info = {
		.iface = device->iface_id,
		.len = frame_len,
		.flags = 0,
		.reserved = 0,
	};
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_NET,
		.type = BUNIX_NET_PACKET_RX_SUBMIT,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_RECV,
		.reply = 0,
		.cap = device->rx_submit_buffer,
		.words = { 0, 0, 0, 0 },
	};
	struct bunix_msg reply;
	static unsigned char frame[VIRTIO_NET_MAX_MTU];

	if (frame_len == 0 || frame_len > device->mtu ||
	    frame_len > sizeof(frame) || device->rx_submit_buffer == 0) {
		return -1;
	}
	if (bunix_buffer_read(buffer, sizeof(struct bunix_virtio_net_header),
			      frame, frame_len) != 0 ||
	    bunix_buffer_write(device->rx_submit_buffer, 0, &info,
			       sizeof(info)) != 0 ||
	    bunix_buffer_write(device->rx_submit_buffer, sizeof(info), frame,
			       frame_len) != 0 ||
	    bunix_ipc_call(device->net_service, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return -1;
	}
	return 0;
}

static int complete_tx_frame(struct virtio_net_device *device)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_NET,
		.type = BUNIX_NET_PACKET_TX_COMPLETE,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { device->iface_id, device->tx_inflight_len, 0, 0 },
	};
	struct bunix_msg reply;

	if (bunix_ipc_call(device->net_service, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return -1;
	}
	device->tx_inflight = 0;
	device->tx_inflight_len = 0;
	return 0;
}

static int poll_rx_once(struct virtio_net_device *device, u64 *completed)
{
	unsigned short used_idx;

	if (queue_read_u16(device->rx.handle, device->rx.layout.used_offset + 2,
			   &used_idx) != 0) {
		return -1;
	}
	while ((unsigned short)device->rx.seen_used != used_idx) {
		struct bunix_virtio_used_elem elem;
		u64 id;
		u64 len;
		u64 frame_len;

		if (queue_read_used_elem(&device->rx, device->rx.seen_used,
					 &elem) != 0) {
			return -1;
		}
		id = elem.id;
		len = elem.len;
		device->rx.seen_used++;
		if (id >= VIRTIO_NET_RX_BUFFERS || len <=
			    sizeof(struct bunix_virtio_net_header)) {
			continue;
		}
		frame_len = len - sizeof(struct bunix_virtio_net_header);
		(void)submit_rx_frame(device, device->rx_buffer[id], frame_len);
		if (queue_avail_put(&device->rx, id) != 0) {
			return -1;
		}
		if (completed != 0) {
			*completed += 1;
		}
	}
	return 0;
}

static int poll_rx_bounded(struct virtio_net_device *device, u64 *completed_out)
{
	u64 completed = 0;

	for (u64 i = 0; i < VIRTIO_NET_RX_POLL_ROUNDS; i++) {
		if (poll_rx_once(device, &completed) != 0) {
			return -1;
		}
		if (completed != 0) {
			if (notify_queue(device, 0) != 0) {
				return -1;
			}
			if (completed_out != 0) {
				*completed_out += completed;
			}
			return 0;
		}
		bunix_sleep_ns(1000000ull);
	}
	if (completed_out != 0) {
		*completed_out += completed;
	}
	return 0;
}

static int poll_tx_completions(struct virtio_net_device *device,
			       u64 *completed)
{
	unsigned short used_idx;

	if (queue_read_u16(device->tx.handle, device->tx.layout.used_offset + 2,
			   &used_idx) != 0) {
		return -1;
	}
	while ((unsigned short)device->tx.seen_used != used_idx) {
		struct bunix_virtio_used_elem elem;

		if (queue_read_used_elem(&device->tx, device->tx.seen_used,
					 &elem) != 0) {
			return -1;
		}
		device->tx.seen_used++;
		if (device->tx_inflight != 0) {
			(void)complete_tx_frame(device);
			if (completed != 0) {
				*completed += 1;
			}
		}
	}
	return 0;
}

static int submit_tx_frame(struct virtio_net_device *device, u64 frame_len)
{
	struct bunix_virtio_net_header header = { 0 };
	static unsigned char frame[VIRTIO_NET_MAX_MTU];
	u64 len = sizeof(header) + frame_len;

	if (frame_len == 0 || frame_len > device->mtu ||
	    frame_len > sizeof(frame) || device->tx_buffer == 0 ||
	    device->tx_phys == 0 || device->tx_inflight != 0) {
		return -1;
	}
	if (bunix_buffer_read(device->tx_dequeue_buffer,
			      sizeof(struct bunix_net_packet_info), frame,
			      frame_len) != 0 ||
	    bunix_buffer_write(device->tx_buffer, 0, &header,
			       sizeof(header)) != 0 ||
	    bunix_buffer_write(device->tx_buffer, sizeof(header), frame,
			       frame_len) != 0 ||
	    queue_write_desc(&device->tx, 0, device->tx_phys, len, 0, 0) != 0 ||
	    queue_avail_put(&device->tx, 0) != 0 ||
	    notify_queue(device, 1) != 0) {
		return -1;
	}
	device->tx_inflight = 1;
	device->tx_inflight_len = frame_len;
	return 0;
}

static int poll_tx_once(struct virtio_net_device *device, u64 *activity)
{
	struct bunix_net_packet_info info;
	u64 completed = 0;
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_NET,
		.type = BUNIX_NET_PACKET_TX_DEQUEUE,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_SEND,
		.reply = 0,
		.cap = device->tx_dequeue_buffer,
		.words = { device->iface_id, device->mtu, 0, 0 },
	};
	struct bunix_msg reply;

	if (poll_tx_completions(device, &completed) != 0) {
		return -1;
	}
	if (activity != 0) {
		*activity += completed;
	}
	if (device->tx_inflight != 0 || device->tx_dequeue_buffer == 0) {
		return 0;
	}
	if (bunix_ipc_call(device->net_service, &request, &reply) != 0) {
		return -1;
	}
	if (reply.words[0] != 0) {
		return 0;
	}
	if (bunix_buffer_read(device->tx_dequeue_buffer, 0, &info,
			      sizeof(info)) != 0 ||
	    info.iface != device->iface_id || info.len == 0 ||
	    info.len > device->mtu) {
		return -1;
	}
	if (submit_tx_frame(device, info.len) != 0) {
		return -1;
	}
	if (activity != 0) {
		*activity += 1;
	}
	return 0;
}

static int poll_tx_bounded(struct virtio_net_device *device, u64 *activity)
{
	for (u64 i = 0; i < VIRTIO_NET_TX_POLL_ROUNDS; i++) {
		const u64 before = activity != 0 ? *activity : 0;

		if (poll_tx_once(device, activity) != 0) {
			return -1;
		}
		if (activity != 0 && *activity == before) {
			break;
		}
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
	{
		long rx_buffer =
			bunix_buffer_create(sizeof(struct bunix_net_packet_info) +
					    VIRTIO_NET_MAX_MTU);
		long tx_buffer =
			bunix_buffer_create(sizeof(struct bunix_net_packet_info) +
					    VIRTIO_NET_MAX_MTU);
		long tx_frame = bunix_buffer_create(VIRTIO_NET_TX_BUFFER_SIZE);
		long tx_phys;

		if (rx_buffer <= 0 || tx_buffer <= 0 || tx_frame <= 0) {
			return -1;
		}
		tx_phys = bunix_buffer_phys((u64)tx_frame);
		if (tx_phys <= 0) {
			return -1;
		}
		device->rx_submit_buffer = (u64)rx_buffer;
		device->tx_dequeue_buffer = (u64)tx_buffer;
		device->tx_buffer = (u64)tx_frame;
		device->tx_phys = (u64)tx_phys;
	}
	if (device->rx_submit_buffer == 0 || device->tx_dequeue_buffer == 0 ||
	    device->tx_buffer == 0 || device->tx_phys == 0) {
		return -1;
	}
	index = find_net_device(device->device_service);
	if (index < 0) {
		return -1;
	}
	device->device_index = (u64)index;
	if (device_call(device->device_service, BUNIX_DEV_RESET,
			device->device_index, 0, 0, &reply) != 0) {
		return -1;
	}
	log_text("virtio-net: reset\n", text_len("virtio-net: reset\n"));
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
	if (replenish_rx(device) != 0) {
		return -1;
	}
	log_iface(device->iface_id, device->mtu);
	log_text("virtio-net: rx ready\n", text_len("virtio-net: rx ready\n"));
	log_text("virtio-net: tx ready\n", text_len("virtio-net: tx ready\n"));
	{
		u64 activity = 0;

		(void)poll_rx_bounded(device, &activity);
		(void)poll_tx_bounded(device, &activity);
	}
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
	u64 active_rounds = 0;
	for (;;) {
		u64 activity = 0;

		(void)poll_rx_bounded(&device, &activity);
		(void)poll_tx_bounded(&device, &activity);
		if (activity == 0) {
			active_rounds = 0;
			bunix_sleep_ns(VIRTIO_NET_IDLE_SLEEP_NS);
		} else {
			active_rounds++;
			if (active_rounds >= VIRTIO_NET_ACTIVE_SLEEP_ROUNDS) {
				active_rounds = 0;
				bunix_sleep_ns(VIRTIO_NET_ACTIVE_SLEEP_NS);
			}
		}
	}
}
