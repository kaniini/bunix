#ifndef BUNIX_USER_VIRTIO_H
#define BUNIX_USER_VIRTIO_H

#include <bunix/driver.h>

enum {
	BUNIX_VIRTIO_PCI_VENDOR_ID = 0x1af4,
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

	BUNIX_VIRTIO_DESC_F_NEXT = 1,
	BUNIX_VIRTIO_DESC_F_WRITE = 2,
	BUNIX_VIRTIO_DESC_F_INDIRECT = 4,

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

#endif
