#include <bunix/virtio.h>

enum {
	VIRTIO_BLK_HANDLE_NAMES = 3,
	VIRTIO_BLK_QUEUE_SIZE = 128,
	VIRTIO_BLK_SECTOR_SIZE = 512,
	VIRTIO_BLK_REQ_HEADER_SIZE = 16,
	VIRTIO_BLK_REQ_DATA_OFFSET = VIRTIO_BLK_REQ_HEADER_SIZE,
	VIRTIO_BLK_REQ_STATUS_OFFSET = VIRTIO_BLK_REQ_DATA_OFFSET +
				       VIRTIO_BLK_SECTOR_SIZE,
	VIRTIO_BLK_REQ_SIZE = VIRTIO_BLK_REQ_STATUS_OFFSET + 1,
	VIRTIO_BLK_COMPLETION_POLLS = 1000,
};

struct virtio_blk_queue {
	u64 handle;
	u64 phys;
	u64 queue_size;
	u64 next_avail;
	u64 seen_used;
	struct bunix_virtio_queue_layout layout;
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

static int virtio_blk_submit(u64 device_service, u64 device_index,
			     struct virtio_blk_queue *queue, u64 req_buffer,
			     u64 req_phys, u64 type, u64 sector, int has_data,
			     int device_writes_data)
{
	const unsigned char pending = 0xff;
	struct bunix_virtio_blk_req_header header = {
		.type = (unsigned int)type,
		.reserved = 0,
		.sector = sector,
	};
	unsigned short used_idx;
	const unsigned short head = 0;
	unsigned short avail_idx;
	u64 ring_offset;
	unsigned char status;
	u64 status_desc = 1;
	u64 data_flags = BUNIX_VIRTIO_DESC_F_NEXT;

	if (device_writes_data) {
		data_flags |= BUNIX_VIRTIO_DESC_F_WRITE;
	}

	if (queue == 0 || queue->handle == 0 || queue->phys == 0 ||
	    req_buffer == 0 || req_phys == 0 ||
	    queue->next_avail - queue->seen_used >= queue->queue_size) {
		return -1;
	}
	avail_idx = (unsigned short)(queue->next_avail + 1);
	ring_offset = queue->layout.avail_offset + 4 +
		      (queue->next_avail % queue->queue_size) *
			      sizeof(unsigned short);
	if (bunix_buffer_write(req_buffer, 0, &header, sizeof(header)) != 0 ||
	    bunix_buffer_write(req_buffer, VIRTIO_BLK_REQ_STATUS_OFFSET,
			       &pending, sizeof(pending)) != 0 ||
	    queue_write_desc(queue, 0, req_phys, sizeof(header),
			     BUNIX_VIRTIO_DESC_F_NEXT, 1) != 0) {
		return -1;
	}
	if (has_data) {
		status_desc = 2;
		if (queue_write_desc(queue, 1,
				     req_phys + VIRTIO_BLK_REQ_DATA_OFFSET,
				     VIRTIO_BLK_SECTOR_SIZE, data_flags,
				     2) != 0) {
			return -1;
		}
	}
	if (queue_write_desc(queue, status_desc,
			     req_phys + VIRTIO_BLK_REQ_STATUS_OFFSET, 1,
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
	for (u64 poll = 0; poll < VIRTIO_BLK_COMPLETION_POLLS; poll++) {
		if (queue_read_u16(queue->handle, queue->layout.used_offset + 2,
				   &used_idx) != 0) {
			return -1;
		}
		if (used_idx != (unsigned short)queue->seen_used) {
			queue->seen_used = used_idx;
			if (bunix_buffer_read(req_buffer,
					      VIRTIO_BLK_REQ_STATUS_OFFSET,
					      &status, sizeof(status)) != 0) {
				return -1;
			}
			return status == BUNIX_VIRTIO_BLK_S_OK ? 0 : -1;
		}
		bunix_sleep_ns(1000000ull);
	}
	return -1;
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
			      BUNIX_VIRTIO_BLK_T_IN, 0, 1, 1) != 0 ||
	    virtio_blk_submit(device_service, device_index, queue,
			      (u64)req_buffer, (u64)req_phys,
			      BUNIX_VIRTIO_BLK_T_OUT, 0, 1, 0) != 0) {
		bunix_handle_close((u64)req_buffer);
		return -1;
	}
	if (do_flush &&
	    virtio_blk_submit(device_service, device_index, queue,
			      (u64)req_buffer, (u64)req_phys,
			      BUNIX_VIRTIO_BLK_T_FLUSH, 0, 0, 0) != 0) {
		bunix_handle_close((u64)req_buffer);
		return -1;
	}
	bunix_handle_close((u64)req_buffer);
	return 0;
}

int main(void)
{
	const char online[] = "virtio-blk: online\n";
	const char no_device[] = "virtio-blk: no device\n";
	const char negotiated[] = "virtio-blk: negotiated\n";
	const char queue_ready[] = "virtio-blk: queue ready\n";
	const char read_ok[] = "virtio-blk: read ok\n";
	const char write_ok[] = "virtio-blk: write ok\n";
	const char flush_ok[] = "virtio-blk: flush ok\n";
	const char ready[] = "virtio-blk: ready\n";
	u64 device_service;
	long device_index;
	struct bunix_msg reply;
	struct virtio_blk_queue queue;
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
	    virtio_blk_queue_from_reply(&reply, &queue) != 0) {
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
	if (virtio_blk_selftest(device_service, (u64)device_index, &queue,
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
	return 0;
}
