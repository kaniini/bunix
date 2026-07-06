#include <bunix/alloc.h>
#include <bunix/libbunix.h>

enum {
	NET_HANDLE_NAMES = 3,
	NET_IFACE_LO = 1,
	NET_PACKET_MAX = 65536,
};

struct net_packet {
	struct net_packet *next;
	u64 family;
	u64 len;
	unsigned char data[];
};

struct net_interface {
	u64 id;
	u64 flags;
	u64 mtu;
	u64 ipv4_be;
	u64 ipv6_hi_be;
	u64 ipv6_lo_be;
	u64 rx_packets;
	u64 tx_packets;
	u64 rx_drops;
	u64 tx_drops;
};

static struct net_interface loopback = {
	.id = NET_IFACE_LO,
	.flags = BUNIX_NET_IFACE_FLAG_UP | BUNIX_NET_IFACE_FLAG_LOOPBACK,
	.mtu = 65536,
	.ipv4_be = 0x7f000001,
	.ipv6_hi_be = 0,
	.ipv6_lo_be = 1,
	.rx_packets = 0,
	.tx_packets = 0,
	.rx_drops = 0,
	.tx_drops = 0,
};
static struct net_packet *loopback_rx_head;
static struct net_packet *loopback_rx_tail;

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

	if (bunix_ipc_call(NET_HANDLE_NAMES, &request, &reply) != 0) {
		return -1;
	}

	return reply.words[0] == 0 ? 0 : -1;
}

static void reply_interface_count(struct bunix_msg *reply)
{
	reply->words[0] = 0;
	reply->words[1] = 1;
}

static void reply_interface_info(struct bunix_msg *reply,
				 const struct bunix_msg *message)
{
	if (message->words[0] != loopback.id) {
		reply->words[0] = (u64)-1;
		return;
	}

	reply->words[0] = 0;
	reply->words[1] = loopback.id;
	reply->words[2] = loopback.flags;
	reply->words[3] = loopback.mtu;
}

static void reply_interface_stats(struct bunix_msg *reply,
				  const struct bunix_msg *message)
{
	if (message->words[0] != loopback.id) {
		reply->words[0] = (u64)-1;
		return;
	}

	reply->words[0] = 0;
	reply->words[1] = loopback.rx_packets;
	reply->words[2] = loopback.tx_packets;
	reply->words[3] = loopback.rx_drops + loopback.tx_drops;
}

static void reply_loopback_send(struct bunix_msg *reply,
				const struct bunix_msg *message)
{
	const u64 iface = message->words[0];
	const u64 family = message->words[1];
	const u64 len = message->words[2];
	struct net_packet *packet;

	if (iface != loopback.id ||
	    (family != BUNIX_NET_ADDR_FAMILY_IPV4 &&
	     family != BUNIX_NET_ADDR_FAMILY_IPV6) ||
	    len == 0 || len > NET_PACKET_MAX || message->cap == 0) {
		reply->words[0] = (u64)-1;
		return;
	}

	packet = (struct net_packet *)bunix_alloc(sizeof(*packet) + len);
	if (packet == 0) {
		loopback.tx_drops++;
		reply->words[0] = (u64)-1;
		return;
	}
	packet->next = 0;
	packet->family = family;
	packet->len = len;
	if (bunix_buffer_read(message->cap, 0, packet->data, len) != 0) {
		bunix_free(packet);
		loopback.tx_drops++;
		reply->words[0] = (u64)-1;
		return;
	}

	if (loopback_rx_tail != 0) {
		loopback_rx_tail->next = packet;
	} else {
		loopback_rx_head = packet;
	}
	loopback_rx_tail = packet;
	loopback.tx_packets++;
	loopback.rx_packets++;
	reply->words[0] = 0;
	reply->words[1] = len;
}

static void reply_loopback_recv(struct bunix_msg *reply,
				const struct bunix_msg *message)
{
	const u64 iface = message->words[0];
	const u64 max_len = message->words[1];
	struct net_packet *packet = loopback_rx_head;
	u64 len;

	if (iface != loopback.id || max_len == 0 || message->cap == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	if (packet == 0) {
		reply->words[0] = (u64)-1;
		reply->words[1] = 0;
		return;
	}

	len = packet->len < max_len ? packet->len : max_len;
	if (bunix_buffer_write(message->cap, 0, packet->data, len) != 0) {
		reply->words[0] = (u64)-1;
		return;
	}

	loopback_rx_head = packet->next;
	if (loopback_rx_head == 0) {
		loopback_rx_tail = 0;
	}
	reply->words[0] = 0;
	reply->words[1] = packet->family;
	reply->words[2] = len;
	reply->words[3] = packet->len;
	bunix_free(packet);
}

int main(void)
{
	const char online[] = "net: online\n";
	const char loopback_online[] =
		"net: loopback lo ipv4=127.0.0.1 ipv6=::1\n";
	struct bunix_msg message;

	bunix_console_log(online, sizeof(online) - 1);
	if (register_service(BUNIX_SERVICE_NET, BUNIX_HANDLE_SELF) != 0) {
		return 1;
	}
	bunix_console_log(loopback_online, sizeof(loopback_online) - 1);

	for (;;) {
		struct bunix_msg reply = {
			.protocol = BUNIX_PROTO_NET,
			.type = 0,
			.sender = 0,
			.cap_rights = 0,
			.reply = 0,
			.cap = 0,
			.words = { 0, 0, 0, 0 },
		};

		if (bunix_ipc_recv(BUNIX_HANDLE_SELF, &message) != 0 ||
		    message.protocol != BUNIX_PROTO_NET) {
			continue;
		}

		reply.type = message.type;
		switch (message.type) {
		case BUNIX_NET_INTERFACE_COUNT:
			reply_interface_count(&reply);
			break;
		case BUNIX_NET_INTERFACE_INFO:
			reply_interface_info(&reply, &message);
			break;
		case BUNIX_NET_INTERFACE_STATS:
			reply_interface_stats(&reply, &message);
			break;
		case BUNIX_NET_LOOPBACK_SEND:
			reply_loopback_send(&reply, &message);
			break;
		case BUNIX_NET_LOOPBACK_RECV:
			reply_loopback_recv(&reply, &message);
			break;
		default:
			reply.words[0] = (u64)-1;
			break;
		}

		if (message.reply != 0) {
			bunix_ipc_send(message.reply, &reply);
		}
	}
}
