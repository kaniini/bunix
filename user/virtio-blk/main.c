#include <bunix/virtio.h>

enum {
	VIRTIO_BLK_HANDLE_NAMES = 3,
	VIRTIO_BLK_QUEUE_SIZE = 128,
	VIRTIO_BLK_SECTOR_SIZE = 512,
	VIRTIO_BLK_BUFFER_MAX = 128 * 1024,
	VIRTIO_BLK_REQ_HEADER_SIZE = 16,
	VIRTIO_BLK_REQ_DATA_OFFSET = VIRTIO_BLK_REQ_HEADER_SIZE,
	VIRTIO_BLK_REQ_STATUS_OFFSET = VIRTIO_BLK_REQ_DATA_OFFSET +
				       VIRTIO_BLK_BUFFER_MAX,
	VIRTIO_BLK_REQ_SIZE = VIRTIO_BLK_REQ_STATUS_OFFSET + 1,
	VIRTIO_BLK_COMPLETION_POLLS = 1000,
};

static unsigned char block_buffer[VIRTIO_BLK_BUFFER_MAX];
static unsigned char cache_buffer[VIRTIO_BLK_SECTOR_SIZE];
static unsigned char sector_buffer[VIRTIO_BLK_SECTOR_SIZE];

struct virtio_blk_queue {
	u64 handle;
	u64 phys;
	u64 queue_size;
	u64 next_avail;
	u64 seen_used;
	u64 irq_handle;
	u64 irq_port;
	u64 irq_index;
	int irq_enabled;
	struct bunix_virtio_queue_layout layout;
};

struct virtio_blk_device {
	u64 device_service;
	u64 device_index;
	u64 capacity_bytes;
	u64 req_buffer;
	u64 req_phys;
	u64 cache_sector;
	int supports_flush;
	int cache_valid;
	struct virtio_blk_queue queue;
};

static long register_service(u64 service, u64 handle)
{
	(void)service;
	return bunix_names_register_claim(bunix_handle_find(BUNIX_CAP_CLAM),
					  handle);
}

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

static u64 virtio_blk_capacity_bytes(u64 device_service, u64 device_index)
{
	struct bunix_msg reply;
	u64 sectors;

	if (device_call(device_service, BUNIX_DEV_READ_CONFIG64, device_index,
			0, 0, &reply) != 0) {
		return 0;
	}
	sectors = reply.words[1];
	return sectors * VIRTIO_BLK_SECTOR_SIZE;
}

static int virtio_blk_bind_irq(u64 device_service, u64 device_index,
			       struct virtio_blk_queue *queue)
{
	struct bunix_msg reply;
	u64 resource_count;
	long irq_port;

	if (queue == 0 ||
	    device_call(device_service, BUNIX_DEV_GET_INFO, device_index,
			0, 0, &reply) != 0) {
		return -1;
	}
	resource_count = reply.words[1] >> 32;
	for (u64 i = 0; i < resource_count; i++) {
		u64 type;
		u64 ops;

		if (device_call(device_service, BUNIX_DEV_GET_RESOURCE,
				device_index, i, 0, &reply) != 0) {
			continue;
		}
		type = reply.words[1] & 0xffffu;
		ops = (reply.words[1] >> 16) & 0xffffu;
		if (type != BUNIX_DEV_RESOURCE_IRQ ||
		    (ops & (BUNIX_DEV_OP_BIND_IRQ | BUNIX_DEV_OP_ACK_IRQ |
			    BUNIX_DEV_OP_MASK_IRQ)) !=
			    (BUNIX_DEV_OP_BIND_IRQ | BUNIX_DEV_OP_ACK_IRQ |
			     BUNIX_DEV_OP_MASK_IRQ) ||
		    reply.cap == 0) {
			if (reply.cap != 0) {
				bunix_handle_close(reply.cap);
			}
			continue;
		}
		irq_port = bunix_port_create("virtio-blk-irq");
		if (irq_port <= 0) {
			bunix_handle_close(reply.cap);
			return -1;
		}
		if (bunix_hw_irq_bind(reply.cap, 0, (u64)irq_port) != 0) {
			bunix_handle_close(reply.cap);
			bunix_handle_close((u64)irq_port);
			return -1;
		}
		queue->irq_handle = reply.cap;
		queue->irq_port = (u64)irq_port;
		queue->irq_index = 0;
		queue->irq_enabled = 1;
		return 0;
	}
	return -1;
}

static int virtio_blk_feature_failure_probe(u64 device_service,
					    u64 device_index)
{
	struct bunix_msg reply;
	u64 missing = 0;

	if (device_call(device_service, BUNIX_DEV_READ_FEATURES,
			device_index, 0, 0, &reply) != 0) {
		return -1;
	}
	for (u64 bit = 0; bit < 64; bit++) {
		const u64 feature = 1ull << bit;

		if ((reply.words[1] & feature) == 0) {
			missing = feature;
			break;
		}
	}
	if (missing == 0) {
		return -1;
	}
	return device_call(device_service, BUNIX_DEV_NEGOTIATE_FEATURES,
			   device_index, missing, 0, &reply) == 0 ? -1 : 0;
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

static int queue_write_u16(u64 handle, u64 offset, unsigned short value)
{
	return queue_write(handle, offset, &value, sizeof(value));
}

static int queue_write_desc(struct virtio_blk_queue *queue, u64 index,
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

static int virtio_blk_queue_from_reply(struct bunix_msg *reply,
				       struct virtio_blk_queue *queue)
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
	queue->next_avail = 0;
	queue->seen_used = 0;
	queue->irq_handle = 0;
	queue->irq_port = 0;
	queue->irq_index = 0;
	queue->irq_enabled = 0;
	return 0;
}

static int virtio_blk_queue_init(struct virtio_blk_queue *queue)
{
	const unsigned short zero = 0;

	if (queue == 0 || queue->handle == 0) {
		return -1;
	}
	queue->next_avail = 0;
	queue->seen_used = 0;
	return queue_write_u16(queue->handle, queue->layout.avail_offset,
			       zero) == 0 &&
	       queue_write_u16(queue->handle, queue->layout.avail_offset + 2,
			       zero) == 0 ?
		       0 :
		       -1;
}

static int virtio_blk_completion_done(struct virtio_blk_queue *queue,
				      u64 req_buffer, u64 status_offset)
{
	unsigned short used_idx;
	unsigned char status;

	if (queue_read_u16(queue->handle, queue->layout.used_offset + 2,
			   &used_idx) != 0) {
		return -1;
	}
	if (used_idx == (unsigned short)queue->seen_used) {
		return 1;
	}
	queue->seen_used = used_idx;
	if (bunix_buffer_read(req_buffer, status_offset, &status,
			      sizeof(status)) != 0) {
		return -1;
	}
	return status == BUNIX_VIRTIO_BLK_S_OK ? 0 : -1;
}

static int virtio_blk_wait_irq_completion(u64 device_service,
					  u64 device_index,
					  struct virtio_blk_queue *queue,
					  u64 req_buffer,
					  u64 status_offset)
{
	struct bunix_msg message;
	struct bunix_msg reply;
	int result;

	result = virtio_blk_completion_done(queue, req_buffer, status_offset);
	if (result != 1) {
		return result;
	}
	if (bunix_hw_irq_mask(queue->irq_handle, queue->irq_index, 0) != 0) {
		return -1;
	}
	for (;;) {
		if (bunix_ipc_recv(queue->irq_port, &message) != 0) {
			return -1;
		}
		if (message.protocol != BUNIX_PROTO_HW ||
		    message.type != BUNIX_HW_EVENT_IRQ) {
			continue;
		}
		if (device_call(device_service, BUNIX_DEV_ACK_INTERRUPT,
				device_index, 0, 0, &reply) != 0 ||
		    bunix_hw_irq_ack(queue->irq_handle, queue->irq_index) != 0) {
			return -1;
		}
		result = virtio_blk_completion_done(queue, req_buffer,
						    status_offset);
		if (result != 1) {
			return result;
		}
		if (bunix_hw_irq_mask(queue->irq_handle, queue->irq_index, 0) !=
		    0) {
			return -1;
		}
	}
}

static int virtio_blk_wait_poll_completion(struct virtio_blk_queue *queue,
					   u64 req_buffer,
					   u64 status_offset)
{
	for (u64 poll = 0; poll < VIRTIO_BLK_COMPLETION_POLLS; poll++) {
		const int result =
			virtio_blk_completion_done(queue, req_buffer,
						   status_offset);
		if (result != 1) {
			return result;
		}
		bunix_sleep_ns(1000000ull);
	}
	return -1;
}

static int virtio_blk_submit(u64 device_service, u64 device_index,
			     struct virtio_blk_queue *queue, u64 req_buffer,
			     u64 req_phys, u64 type, u64 sector, int has_data,
			     u64 data_len, int device_writes_data)
{
	const unsigned char pending = 0xff;
	const u64 status_offset = has_data ?
					  VIRTIO_BLK_REQ_DATA_OFFSET + data_len :
					  VIRTIO_BLK_REQ_DATA_OFFSET;
	struct bunix_virtio_blk_req_header header = {
		.type = (unsigned int)type,
		.reserved = 0,
		.sector = sector,
	};
	const unsigned short head = 0;
	unsigned short avail_idx;
	u64 ring_offset;
	u64 status_desc = 1;
	u64 data_flags = BUNIX_VIRTIO_DESC_F_NEXT;

	if (device_writes_data) {
		data_flags |= BUNIX_VIRTIO_DESC_F_WRITE;
	}

	if (queue == 0 || queue->handle == 0 || queue->phys == 0 ||
	    req_buffer == 0 || req_phys == 0 ||
	    data_len > VIRTIO_BLK_BUFFER_MAX ||
	    queue->next_avail - queue->seen_used >= queue->queue_size) {
		return -1;
	}
	avail_idx = (unsigned short)(queue->next_avail + 1);
	ring_offset = queue->layout.avail_offset + 4 +
		      (queue->next_avail % queue->queue_size) *
			      sizeof(unsigned short);
	if (bunix_buffer_write(req_buffer, 0, &header, sizeof(header)) != 0 ||
	    bunix_buffer_write(req_buffer, status_offset,
			       &pending, sizeof(pending)) != 0 ||
	    queue_write_desc(queue, 0, req_phys, sizeof(header),
			     BUNIX_VIRTIO_DESC_F_NEXT, 1) != 0) {
		return -1;
	}
	if (has_data) {
		status_desc = 2;
		if (queue_write_desc(queue, 1,
				     req_phys + VIRTIO_BLK_REQ_DATA_OFFSET,
				     data_len, data_flags, 2) != 0) {
			return -1;
		}
	}
	if (queue_write_desc(queue, status_desc,
			     req_phys + status_offset, 1,
			     BUNIX_VIRTIO_DESC_F_WRITE, 0) != 0 ||
	    queue_write_u16(queue->handle, ring_offset, head) != 0 ||
	    queue_write_u16(queue->handle, queue->layout.avail_offset + 2,
			    avail_idx) != 0) {
		return -1;
	}
	queue->next_avail++;
	{
		struct bunix_msg reply;

		if (device_call(device_service, BUNIX_DEV_NOTIFY_QUEUE,
				device_index, 0, 0, &reply) != 0) {
			return -1;
		}
	}
	if (queue->irq_enabled) {
		return virtio_blk_wait_irq_completion(device_service,
						      device_index, queue,
						      req_buffer,
						      status_offset);
	}
	return virtio_blk_wait_poll_completion(queue, req_buffer, status_offset);
}

static int virtio_blk_selftest(u64 device_service, u64 device_index,
			       struct virtio_blk_queue *queue, int do_flush)
{
	long req_buffer;
	long req_phys;

	if (queue == 0 || virtio_blk_queue_init(queue) != 0) {
		return -1;
	}
	req_buffer = bunix_buffer_create(VIRTIO_BLK_REQ_SIZE);
	if (req_buffer <= 0) {
		return -1;
	}
	req_phys = bunix_buffer_phys((u64)req_buffer);
	if (req_phys <= 0) {
		bunix_handle_close((u64)req_buffer);
		return -1;
	}
	if (virtio_blk_submit(device_service, device_index, queue,
			      (u64)req_buffer, (u64)req_phys,
			      BUNIX_VIRTIO_BLK_T_IN, 0, 1,
			      VIRTIO_BLK_SECTOR_SIZE, 1) != 0 ||
	    virtio_blk_submit(device_service, device_index, queue,
			      (u64)req_buffer, (u64)req_phys,
			      BUNIX_VIRTIO_BLK_T_OUT, 0, 1,
			      VIRTIO_BLK_SECTOR_SIZE, 0) != 0) {
		bunix_handle_close((u64)req_buffer);
		return -1;
	}
	if (do_flush &&
	    virtio_blk_submit(device_service, device_index, queue,
			      (u64)req_buffer, (u64)req_phys,
			      BUNIX_VIRTIO_BLK_T_FLUSH, 0, 0, 0, 0) != 0) {
		bunix_handle_close((u64)req_buffer);
		return -1;
	}
	bunix_handle_close((u64)req_buffer);
	return 0;
}

static int virtio_blk_read_sector(struct virtio_blk_device *device, u64 sector,
				  unsigned char *out)
{
	if (device == 0 || out == 0 ||
	    virtio_blk_submit(device->device_service, device->device_index,
			      &device->queue, device->req_buffer,
			      device->req_phys, BUNIX_VIRTIO_BLK_T_IN,
			      sector, 1, VIRTIO_BLK_SECTOR_SIZE, 1) != 0 ||
	    bunix_buffer_read(device->req_buffer, VIRTIO_BLK_REQ_DATA_OFFSET,
			      out, VIRTIO_BLK_SECTOR_SIZE) != 0) {
		return -1;
	}
	return 0;
}

static int virtio_blk_read_aligned(struct virtio_blk_device *device, u64 sector,
				   unsigned char *out, u64 len)
{
	if (len == 0) {
		return 0;
	}
	if ((len % VIRTIO_BLK_SECTOR_SIZE) != 0 ||
	    len > VIRTIO_BLK_BUFFER_MAX ||
	    virtio_blk_submit(device->device_service, device->device_index,
			      &device->queue, device->req_buffer,
			      device->req_phys, BUNIX_VIRTIO_BLK_T_IN,
			      sector, 1, len, 1) != 0 ||
	    bunix_buffer_read(device->req_buffer, VIRTIO_BLK_REQ_DATA_OFFSET,
			      out, len) != 0) {
		return -1;
	}
	return 0;
}

static int virtio_blk_read_bytes(struct virtio_blk_device *device, u64 offset,
				 unsigned char *out, u64 len)
{
	u64 done = 0;

	if ((offset % VIRTIO_BLK_SECTOR_SIZE) != 0) {
		const u64 sector = offset / VIRTIO_BLK_SECTOR_SIZE;
		const u64 sector_offset = offset % VIRTIO_BLK_SECTOR_SIZE;
		u64 chunk = VIRTIO_BLK_SECTOR_SIZE - sector_offset;

		if (chunk > len) {
			chunk = len;
		}
		if (virtio_blk_read_sector(device, sector, sector_buffer) != 0) {
			return -1;
		}
		for (u64 i = 0; i < chunk; i++) {
			out[i] = sector_buffer[sector_offset + i];
		}
		done += chunk;
	}

	while (len - done >= VIRTIO_BLK_SECTOR_SIZE) {
		const u64 absolute = offset + done;
		u64 chunk = len - done;

		if (chunk > VIRTIO_BLK_BUFFER_MAX) {
			chunk = VIRTIO_BLK_BUFFER_MAX;
		}
		chunk -= chunk % VIRTIO_BLK_SECTOR_SIZE;
		if (virtio_blk_read_aligned(device,
					    absolute / VIRTIO_BLK_SECTOR_SIZE,
					    out + done, chunk) != 0) {
			return -1;
		}
		done += chunk;
	}

	if (done < len) {
		const u64 absolute = offset + done;
		const u64 sector = absolute / VIRTIO_BLK_SECTOR_SIZE;
		const u64 chunk = len - done;

		if (virtio_blk_read_sector(device, sector, sector_buffer) != 0) {
			return -1;
		}
		for (u64 i = 0; i < chunk; i++) {
			out[done + i] = sector_buffer[i];
		}
	}
	return 0;
}

static int virtio_blk_read_cached(struct virtio_blk_device *device, u64 offset,
				  unsigned char *out, u64 len)
{
	const u64 sector = offset / VIRTIO_BLK_SECTOR_SIZE;
	const u64 sector_offset = offset % VIRTIO_BLK_SECTOR_SIZE;

	if (device == 0 || out == 0 ||
	    len > VIRTIO_BLK_SECTOR_SIZE ||
	    sector_offset + len > VIRTIO_BLK_SECTOR_SIZE) {
		return virtio_blk_read_bytes(device, offset, out, len);
	}
	if (!device->cache_valid || device->cache_sector != sector) {
		if (virtio_blk_read_sector(device, sector, cache_buffer) != 0) {
			device->cache_valid = 0;
			return -1;
		}
		device->cache_sector = sector;
		device->cache_valid = 1;
	}
	for (u64 i = 0; i < len; i++) {
		out[i] = cache_buffer[sector_offset + i];
	}
	return 0;
}

static int virtio_blk_write_aligned(struct virtio_blk_device *device,
				    u64 sector, const unsigned char *data,
				    u64 len)
{
	if (len == 0) {
		return 0;
	}
	if (device == 0 || data == 0 ||
	    (len % VIRTIO_BLK_SECTOR_SIZE) != 0 ||
	    len > VIRTIO_BLK_BUFFER_MAX ||
	    bunix_buffer_write(device->req_buffer, VIRTIO_BLK_REQ_DATA_OFFSET,
			       data, len) != 0 ||
	    virtio_blk_submit(device->device_service, device->device_index,
			      &device->queue, device->req_buffer,
			      device->req_phys, BUNIX_VIRTIO_BLK_T_OUT,
			      sector, 1, len, 0) != 0) {
		return -1;
	}
	device->cache_valid = 0;
	return 0;
}

static int virtio_blk_write_sector(struct virtio_blk_device *device, u64 sector,
				   const unsigned char *data)
{
	return virtio_blk_write_aligned(device, sector, data,
					VIRTIO_BLK_SECTOR_SIZE);
}

static int virtio_blk_write_bytes(struct virtio_blk_device *device, u64 offset,
				  const unsigned char *data, u64 len)
{
	u64 done = 0;

	if (device == 0 || data == 0 ||
	    offset > device->capacity_bytes ||
	    len > device->capacity_bytes - offset) {
		return -1;
	}
	if ((offset % VIRTIO_BLK_SECTOR_SIZE) != 0) {
		const u64 sector = offset / VIRTIO_BLK_SECTOR_SIZE;
		const u64 sector_offset = offset % VIRTIO_BLK_SECTOR_SIZE;
		u64 chunk = VIRTIO_BLK_SECTOR_SIZE - sector_offset;

		if (chunk > len) {
			chunk = len;
		}
		if (virtio_blk_read_sector(device, sector, sector_buffer) != 0) {
			return -1;
		}
		for (u64 i = 0; i < chunk; i++) {
			sector_buffer[sector_offset + i] = data[i];
		}
		if (virtio_blk_write_sector(device, sector, sector_buffer) != 0) {
			return -1;
		}
		done += chunk;
	}

	while (len - done >= VIRTIO_BLK_SECTOR_SIZE) {
		const u64 absolute = offset + done;
		u64 chunk = len - done;

		if (chunk > VIRTIO_BLK_BUFFER_MAX) {
			chunk = VIRTIO_BLK_BUFFER_MAX;
		}
		chunk -= chunk % VIRTIO_BLK_SECTOR_SIZE;
		if (virtio_blk_write_aligned(device,
					     absolute / VIRTIO_BLK_SECTOR_SIZE,
					     data + done, chunk) != 0) {
			return -1;
		}
		done += chunk;
	}

	if (done < len) {
		const u64 absolute = offset + done;
		const u64 sector = absolute / VIRTIO_BLK_SECTOR_SIZE;
		const u64 chunk = len - done;

		if (virtio_blk_read_sector(device, sector, sector_buffer) != 0) {
			return -1;
		}
		for (u64 i = 0; i < chunk; i++) {
			sector_buffer[i] = data[done + i];
		}
		if (virtio_blk_write_sector(device, sector, sector_buffer) != 0) {
			return -1;
		}
	}
	return 0;
}

static int virtio_blk_flush(struct virtio_blk_device *device)
{
	if (device == 0 || device->supports_flush == 0) {
		return -1;
	}
	return virtio_blk_submit(device->device_service, device->device_index,
				 &device->queue, device->req_buffer,
				 device->req_phys, BUNIX_VIRTIO_BLK_T_FLUSH,
				 0, 0, 0, 0);
}

static void pack_bytes(u64 *words, const unsigned char *data, u64 len)
{
	words[0] = 0;
	words[1] = 0;

	for (u64 i = 0; i < len && i < BUNIX_IPC_DATA_BYTES; i++) {
		const u64 slot = i / 8;
		const u64 shift = (i % 8) * 8;

		words[slot] |= ((u64)data[i]) << shift;
	}
}

static void serve_block_protocol(struct virtio_blk_device *device)
{
	const char online[] = "virtio-blk: block online\n";
	struct bunix_msg message;

	if (register_service(BUNIX_SERVICE_BLOCK, BUNIX_HANDLE_SELF) != 0) {
		return;
	}
	bunix_console_log(online, sizeof(online) - 1);

	for (;;) {
		struct bunix_msg reply = {
			.protocol = BUNIX_PROTO_BLOCK,
			.type = 0,
			.sender = 0,
			.cap_rights = 0,
			.reply = 0,
			.cap = 0,
			.words = { 0, 0, 0, 0 },
		};

		if (bunix_ipc_recv(BUNIX_HANDLE_SELF, &message) != 0 ||
		    message.protocol != BUNIX_PROTO_BLOCK) {
			continue;
		}

		reply.type = message.type;
		switch (message.type) {
		case BUNIX_BLOCK_GET_INFO:
			reply.words[0] = 0;
			reply.words[1] = device->capacity_bytes;
			reply.words[2] = VIRTIO_BLK_SECTOR_SIZE;
			break;
		case BUNIX_BLOCK_READ: {
			const u64 offset = message.words[0];
			u64 len = message.words[1];

			if (offset >= device->capacity_bytes) {
				len = 0;
			} else if (len > device->capacity_bytes - offset) {
				len = device->capacity_bytes - offset;
			}
			if (len > BUNIX_IPC_DATA_BYTES) {
				len = BUNIX_IPC_DATA_BYTES;
			}
			reply.words[0] = 0;
			reply.words[1] = len;
			if (len != 0 &&
			    virtio_blk_read_cached(device, offset, block_buffer,
						   len) != 0) {
				reply.words[0] = (u64)-1;
				reply.words[1] = 0;
			} else {
				pack_bytes(&reply.words[2], block_buffer, len);
			}
			break;
		}
		case BUNIX_BLOCK_READ_BUFFER: {
			const u64 offset = message.words[0];
			const u64 buffer_offset = message.words[2];
			u64 len = message.words[1];

			if (message.cap == 0 ||
			    (message.cap_rights & BUNIX_RIGHT_SEND) == 0) {
				reply.words[0] = (u64)-1;
				break;
			}
			if (offset >= device->capacity_bytes) {
				len = 0;
			} else if (len > device->capacity_bytes - offset) {
				len = device->capacity_bytes - offset;
			}
			if (len > sizeof(block_buffer)) {
				len = sizeof(block_buffer);
			}
			reply.words[0] = 0;
			reply.words[1] = len;
			if (len != 0 &&
			    (virtio_blk_read_bytes(device, offset, block_buffer,
						   len) != 0 ||
			     bunix_buffer_write(message.cap, buffer_offset,
						block_buffer, len) != 0)) {
				reply.words[0] = (u64)-1;
				reply.words[1] = 0;
			}
			break;
		}
		case BUNIX_BLOCK_WRITE_BUFFER: {
			const u64 offset = message.words[0];
			const u64 buffer_offset = message.words[2];
			u64 len = message.words[1];

			if (message.cap == 0 ||
			    (message.cap_rights & BUNIX_RIGHT_RECV) == 0 ||
			    offset > device->capacity_bytes ||
			    len > device->capacity_bytes - offset) {
				reply.words[0] = (u64)-1;
				break;
			}
			if (len > sizeof(block_buffer)) {
				len = sizeof(block_buffer);
			}
			reply.words[0] = 0;
			reply.words[1] = len;
			if (len != 0 &&
			    (bunix_buffer_read(message.cap, buffer_offset,
					       block_buffer, len) != 0 ||
			     virtio_blk_write_bytes(device, offset, block_buffer,
						    len) != 0)) {
				reply.words[0] = (u64)-1;
				reply.words[1] = 0;
			}
			break;
		}
		case BUNIX_BLOCK_FLUSH:
			reply.words[0] = virtio_blk_flush(device) == 0 ? 0 :
					 (u64)-1;
			break;
		default:
			reply.words[0] = (u64)-1;
			break;
		}

		if (message.cap != 0) {
			bunix_handle_close(message.cap);
		}
		if (message.reply != 0) {
			bunix_ipc_send(message.reply, &reply);
		}
	}
}

int main(void)
{
	const char online[] = "virtio-blk: online\n";
	const char no_device[] = "virtio-blk: no device\n";
	const char feature_fail_ok[] = "virtio-blk: feature fail ok\n";
	const char negotiated[] = "virtio-blk: negotiated\n";
	const char queue_ready[] = "virtio-blk: queue ready\n";
	const char irq_ready[] = "virtio-blk: irq ready\n";
	const char irq_polling[] = "virtio-blk: irq unavailable, polling\n";
	const char read_ok[] = "virtio-blk: read ok\n";
	const char write_ok[] = "virtio-blk: write ok\n";
	const char flush_ok[] = "virtio-blk: flush ok\n";
	const char ready[] = "virtio-blk: ready\n";
	u64 device_service;
	long device_index;
	long req_buffer;
	long req_phys;
	struct bunix_msg reply;
	struct virtio_blk_device device;
	u64 features = 1ull << BUNIX_VIRTIO_F_VERSION_1;
	const u64 optional_features = 1ull << BUNIX_VIRTIO_BLK_F_FLUSH;

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
	if (virtio_blk_feature_failure_probe(device_service,
					     (u64)device_index) != 0) {
		bunix_console_log(no_device, sizeof(no_device) - 1);
		return 1;
	}
	bunix_console_log(feature_fail_ok, sizeof(feature_fail_ok) - 1);
	if (device_call(device_service, BUNIX_DEV_NEGOTIATE_FEATURES,
			(u64)device_index, features,
			optional_features, &reply) != 0) {
		bunix_console_log(no_device, sizeof(no_device) - 1);
		return 1;
	}
	features = reply.words[2];
	bunix_console_log(negotiated, sizeof(negotiated) - 1);
	if (device_call(device_service, BUNIX_DEV_SETUP_QUEUE,
			(u64)device_index, 0, VIRTIO_BLK_QUEUE_SIZE,
			&reply) != 0 ||
	    virtio_blk_queue_from_reply(&reply, &device.queue) != 0) {
		bunix_console_log(no_device, sizeof(no_device) - 1);
		return 1;
	}
	bunix_console_log(queue_ready, sizeof(queue_ready) - 1);
	if (virtio_blk_bind_irq(device_service, (u64)device_index,
				&device.queue) == 0) {
		bunix_console_log(irq_ready, sizeof(irq_ready) - 1);
	} else {
		bunix_console_log(irq_polling, sizeof(irq_polling) - 1);
	}
	if (device_call(device_service, BUNIX_DEV_BIND_DRIVER,
			(u64)device_index, 0, 0, &reply) != 0) {
		bunix_console_log(no_device, sizeof(no_device) - 1);
		return 1;
	}
	bunix_console_log(ready, sizeof(ready) - 1);
	if (virtio_blk_selftest(device_service, (u64)device_index, &device.queue,
				(features &
				 (1ull << BUNIX_VIRTIO_BLK_F_FLUSH)) != 0) != 0) {
		bunix_console_log(no_device, sizeof(no_device) - 1);
		return 1;
	}
	bunix_console_log(read_ok, sizeof(read_ok) - 1);
	bunix_console_log(write_ok, sizeof(write_ok) - 1);
	if ((features & (1ull << BUNIX_VIRTIO_BLK_F_FLUSH)) != 0) {
		bunix_console_log(flush_ok, sizeof(flush_ok) - 1);
	}
	device.device_service = device_service;
	device.device_index = (u64)device_index;
	device.capacity_bytes = virtio_blk_capacity_bytes(device_service,
							  (u64)device_index);
	req_buffer = bunix_buffer_create(VIRTIO_BLK_REQ_SIZE);
	req_phys = req_buffer > 0 ? bunix_buffer_phys((u64)req_buffer) : -1;
	device.req_buffer = req_buffer > 0 ? (u64)req_buffer : 0;
	device.req_phys = req_phys > 0 ? (u64)req_phys : 0;
	device.cache_sector = 0;
	device.supports_flush =
		(features & (1ull << BUNIX_VIRTIO_BLK_F_FLUSH)) != 0;
	device.cache_valid = 0;
	if (device.capacity_bytes == 0) {
		const char capacity_fail[] = "virtio-blk: capacity unavailable\n";

		bunix_console_log(capacity_fail, sizeof(capacity_fail) - 1);
	}
	if (device.req_buffer == 0 || device.req_phys == 0) {
		const char buffer_fail[] = "virtio-blk: block buffer unavailable\n";

		bunix_console_log(buffer_fail, sizeof(buffer_fail) - 1);
	}
	if (device.queue.next_avail != device.queue.seen_used) {
		const char queue_busy[] = "virtio-blk: queue busy\n";

		bunix_console_log(queue_busy, sizeof(queue_busy) - 1);
	}
	if (device.capacity_bytes != 0 && device.req_buffer != 0 &&
	    device.req_phys != 0 &&
	    bunix_cmdline_has("virtio-blk-block-test") > 0 &&
	    device.queue.next_avail == device.queue.seen_used) {
		serve_block_protocol(&device);
	}
	return 0;
}
