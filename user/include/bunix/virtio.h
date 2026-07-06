#ifndef BUNIX_USER_VIRTIO_H
#define BUNIX_USER_VIRTIO_H

#include <bunix/driver.h>

enum {
	BUNIX_VIRTIO_PCI_VENDOR_ID = 0x1af4,
	BUNIX_VIRTIO_DEVICE_NET = 1,
	BUNIX_VIRTIO_DEVICE_BLOCK = 2,

	BUNIX_VIRTIO_STATUS_ACKNOWLEDGE = 1,
	BUNIX_VIRTIO_STATUS_DRIVER = 2,
	BUNIX_VIRTIO_STATUS_DRIVER_OK = 4,
	BUNIX_VIRTIO_STATUS_FEATURES_OK = 8,
	BUNIX_VIRTIO_STATUS_DEVICE_NEEDS_RESET = 64,
	BUNIX_VIRTIO_STATUS_FAILED = 128,

	BUNIX_VIRTIO_F_RING_INDIRECT_DESC = 28,
	BUNIX_VIRTIO_F_RING_EVENT_IDX = 29,
	BUNIX_VIRTIO_F_VERSION_1 = 32,

	BUNIX_VIRTIO_BLK_F_FLUSH = 9,

	BUNIX_VIRTIO_NET_F_CSUM = 0,
	BUNIX_VIRTIO_NET_F_GUEST_CSUM = 1,
	BUNIX_VIRTIO_NET_F_CTRL_GUEST_OFFLOADS = 2,
	BUNIX_VIRTIO_NET_F_MTU = 3,
	BUNIX_VIRTIO_NET_F_MAC = 5,
	BUNIX_VIRTIO_NET_F_GUEST_TSO4 = 7,
	BUNIX_VIRTIO_NET_F_GUEST_TSO6 = 8,
	BUNIX_VIRTIO_NET_F_GUEST_ECN = 9,
	BUNIX_VIRTIO_NET_F_GUEST_UFO = 10,
	BUNIX_VIRTIO_NET_F_HOST_TSO4 = 11,
	BUNIX_VIRTIO_NET_F_HOST_TSO6 = 12,
	BUNIX_VIRTIO_NET_F_HOST_ECN = 13,
	BUNIX_VIRTIO_NET_F_HOST_UFO = 14,
	BUNIX_VIRTIO_NET_F_MRG_RXBUF = 15,
	BUNIX_VIRTIO_NET_F_STATUS = 16,
	BUNIX_VIRTIO_NET_F_CTRL_VQ = 17,
	BUNIX_VIRTIO_NET_F_CTRL_RX = 18,
	BUNIX_VIRTIO_NET_F_CTRL_VLAN = 19,
	BUNIX_VIRTIO_NET_F_GUEST_ANNOUNCE = 21,
	BUNIX_VIRTIO_NET_F_MQ = 22,
	BUNIX_VIRTIO_NET_F_CTRL_MAC_ADDR = 23,
	BUNIX_VIRTIO_NET_F_HASH_REPORT = 57,
	BUNIX_VIRTIO_NET_F_RSS = 60,
	BUNIX_VIRTIO_NET_F_RSC_EXT = 61,
	BUNIX_VIRTIO_NET_F_STANDBY = 62,
	BUNIX_VIRTIO_NET_F_SPEED_DUPLEX = 63,

	BUNIX_VIRTIO_NET_S_LINK_UP = 1,
	BUNIX_VIRTIO_NET_S_ANNOUNCE = 2,

	BUNIX_VIRTIO_DESC_F_NEXT = 1,
	BUNIX_VIRTIO_DESC_F_WRITE = 2,
	BUNIX_VIRTIO_DESC_F_INDIRECT = 4,

	BUNIX_VIRTIO_BLK_T_IN = 0,
	BUNIX_VIRTIO_BLK_T_OUT = 1,
	BUNIX_VIRTIO_BLK_T_FLUSH = 4,
	BUNIX_VIRTIO_BLK_S_OK = 0,
	BUNIX_VIRTIO_BLK_S_IOERR = 1,
	BUNIX_VIRTIO_BLK_S_UNSUPP = 2,
	BUNIX_VIRTIO_MODERN_QUEUE_ALIGNMENT = 4,

	BUNIX_VIRTIO_TRANSPORT_PCI = BUNIX_DEV_TRANSPORT_PCI,
	BUNIX_VIRTIO_TRANSPORT_MMIO = BUNIX_DEV_TRANSPORT_MMIO,
};

struct bunix_virtio_descriptor {
	unsigned long addr;
	unsigned int len;
	unsigned short flags;
	unsigned short next;
} __attribute__((packed));

struct bunix_virtio_avail {
	unsigned short flags;
	unsigned short idx;
	unsigned short ring[];
} __attribute__((packed));

struct bunix_virtio_used_elem {
	unsigned int id;
	unsigned int len;
} __attribute__((packed));

struct bunix_virtio_used {
	unsigned short flags;
	unsigned short idx;
	struct bunix_virtio_used_elem ring[];
} __attribute__((packed));

struct bunix_virtio_blk_req_header {
	unsigned int type;
	unsigned int reserved;
	unsigned long sector;
} __attribute__((packed));

struct bunix_virtio_net_config {
	unsigned char mac[6];
	unsigned short status;
	unsigned short max_virtqueue_pairs;
	unsigned short mtu;
	unsigned int speed;
	unsigned char duplex;
	unsigned char rss_max_key_size;
	unsigned short rss_max_indirection_table_length;
	unsigned int supported_hash_types;
} __attribute__((packed));

struct bunix_virtio_net_header {
	unsigned char flags;
	unsigned char gso_type;
	unsigned short hdr_len;
	unsigned short gso_size;
	unsigned short csum_start;
	unsigned short csum_offset;
	unsigned short num_buffers;
} __attribute__((packed));

struct bunix_virtio_queue_layout {
	u64 queue_size;
	u64 desc_offset;
	u64 avail_offset;
	u64 used_offset;
	u64 total_len;
	u64 alignment;
};

struct bunix_virtio_transport {
	u64 transport;
	u64 device_index;
	u64 device_type;
	u64 vendor;
	u64 device;
	u64 common_cfg_resource;
	u64 notify_resource;
	u64 isr_resource;
	u64 device_cfg_resource;
	u64 notify_off_multiplier;
};

struct bunix_virtio_device_info {
	struct bunix_device_id id;
	struct bunix_virtio_transport transport;
	struct bunix_device_feature_set features;
};

static inline u64 bunix_virtio_align_up(u64 value, u64 alignment)
{
	if (alignment == 0) {
		return value;
	}
	return (value + alignment - 1) & ~(alignment - 1);
}

static inline int bunix_virtio_negotiate_features(
	u64 device_features, u64 required_features, u64 optional_features,
	u64 *driver_features)
{
	if (driver_features == 0 ||
	    (device_features & required_features) != required_features) {
		return -1;
	}

	*driver_features = required_features | (device_features & optional_features);
	return 0;
}

static inline u64 bunix_virtio_status_driver_start(void)
{
	return BUNIX_VIRTIO_STATUS_ACKNOWLEDGE | BUNIX_VIRTIO_STATUS_DRIVER;
}

static inline u64 bunix_virtio_status_features_ok(void)
{
	return bunix_virtio_status_driver_start() |
	       BUNIX_VIRTIO_STATUS_FEATURES_OK;
}

static inline u64 bunix_virtio_status_driver_ok(void)
{
	return bunix_virtio_status_features_ok() |
	       BUNIX_VIRTIO_STATUS_DRIVER_OK;
}

static inline u64 bunix_virtio_status_failed(u64 status)
{
	return status | BUNIX_VIRTIO_STATUS_FAILED;
}

static inline u64 bunix_virtio_queue_desc_bytes(u64 queue_size)
{
	return queue_size * sizeof(struct bunix_virtio_descriptor);
}

static inline u64 bunix_virtio_queue_avail_bytes(u64 queue_size)
{
	return 2 * sizeof(unsigned short) + queue_size * sizeof(unsigned short);
}

static inline u64 bunix_virtio_queue_used_bytes(u64 queue_size)
{
	return 2 * sizeof(unsigned short) +
	       queue_size * sizeof(struct bunix_virtio_used_elem);
}

static inline int bunix_virtio_queue_layout_init(
	struct bunix_virtio_queue_layout *layout, u64 queue_size, u64 alignment)
{
	u64 offset;

	if (layout == 0 || queue_size == 0 ||
	    (queue_size & (queue_size - 1)) != 0 ||
	    (alignment != 0 && (alignment & (alignment - 1)) != 0)) {
		return -1;
	}
	if (alignment == 0) {
		alignment = 4096;
	}

	layout->queue_size = queue_size;
	layout->alignment = alignment;
	layout->desc_offset = 0;
	layout->avail_offset = bunix_virtio_queue_desc_bytes(queue_size);
	offset = layout->avail_offset + bunix_virtio_queue_avail_bytes(queue_size);
	layout->used_offset = bunix_virtio_align_up(offset, alignment);
	layout->total_len = layout->used_offset +
			    bunix_virtio_queue_used_bytes(queue_size);
	return 0;
}

static inline void bunix_virtio_desc_set(
	struct bunix_virtio_descriptor *desc, u64 addr, u64 len, u64 flags,
	u64 next)
{
	if (desc == 0) {
		return;
	}
	desc->addr = addr;
	desc->len = (unsigned int)len;
	desc->flags = (unsigned short)flags;
	desc->next = (unsigned short)next;
}

static inline void bunix_virtio_avail_put(struct bunix_virtio_avail *avail,
					  u64 queue_size, u64 head)
{
	if (avail == 0 || queue_size == 0) {
		return;
	}
	avail->ring[avail->idx % queue_size] = (unsigned short)head;
	avail->idx++;
}

static inline struct bunix_virtio_used_elem *
bunix_virtio_used_at(struct bunix_virtio_used *used, u64 queue_size, u64 index)
{
	if (used == 0 || queue_size == 0) {
		return 0;
	}
	return &used->ring[index % queue_size];
}

#endif
