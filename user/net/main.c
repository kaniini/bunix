#include <bunix/alloc.h>
#include <bunix/libbunix.h>

enum {
	NET_HANDLE_NAMES = 3,
	NET_IFACE_LO = 1,
	NET_PACKET_MAX = 65536,
	NET_UDP_PORT_EPHEMERAL_FIRST = 49152,
	NET_UDP_PORT_EPHEMERAL_LAST = 65535,
	NET_UDP_POLLIN = 1 << 0,
	NET_UDP_POLLOUT = 1 << 1,
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

struct net_addr {
	u64 family;
	u64 hi;
	u64 lo;
	u64 port;
};

struct udp_datagram {
	struct udp_datagram *next;
	struct net_addr source;
	u64 checksum;
	u64 len;
	unsigned char data[];
};

struct udp_socket {
	struct udp_socket *next;
	u64 id;
	u64 family;
	struct net_addr local;
	struct net_addr peer;
	u64 bound;
	u64 connected;
	u64 rx_len;
	struct udp_datagram *rx_head;
	struct udp_datagram *rx_tail;
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
static struct udp_socket *udp_sockets;
static u64 next_udp_socket_id = 1;
static u64 next_ephemeral_port = NET_UDP_PORT_EPHEMERAL_FIRST;

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

static int net_addr_is_loopback(u64 family, u64 hi, u64 lo)
{
	if (family == BUNIX_NET_ADDR_FAMILY_IPV4) {
		return hi == 0 && ((lo >> 24) & 0xff) == 127;
	}
	if (family == BUNIX_NET_ADDR_FAMILY_IPV6) {
		return hi == 0 && lo == 1;
	}
	return 0;
}

static int net_addr_is_wildcard(u64 family, u64 hi, u64 lo)
{
	if (family == BUNIX_NET_ADDR_FAMILY_IPV4 ||
	    family == BUNIX_NET_ADDR_FAMILY_IPV6) {
		return hi == 0 && lo == 0;
	}
	return 0;
}

static struct udp_socket *udp_find(u64 id)
{
	for (struct udp_socket *socket = udp_sockets; socket != 0;
	     socket = socket->next) {
		if (socket->id == id) {
			return socket;
		}
	}
	return 0;
}

static int udp_port_in_use(u64 family, u64 port)
{
	for (struct udp_socket *socket = udp_sockets; socket != 0;
	     socket = socket->next) {
		if (socket->bound && socket->family == family &&
		    socket->local.port == port) {
			return 1;
		}
	}
	return 0;
}

static u64 udp_alloc_ephemeral(u64 family)
{
	for (u64 attempt = NET_UDP_PORT_EPHEMERAL_FIRST;
	     attempt <= NET_UDP_PORT_EPHEMERAL_LAST; attempt++) {
		const u64 port = next_ephemeral_port++;

		if (next_ephemeral_port > NET_UDP_PORT_EPHEMERAL_LAST) {
			next_ephemeral_port = NET_UDP_PORT_EPHEMERAL_FIRST;
		}
		if (!udp_port_in_use(family, port)) {
			return port;
		}
	}
	return 0;
}

static u64 udp_checksum(const struct net_addr *source,
			const struct net_addr *dest,
			const unsigned char *data, u64 len)
{
	u64 sum = source->family + source->hi + source->lo + source->port +
		  dest->hi + dest->lo + dest->port + len;

	for (u64 i = 0; i < len; i++) {
		sum += data[i];
		sum = (sum & 0xffff) + (sum >> 16);
	}
	while (sum >> 16) {
		sum = (sum & 0xffff) + (sum >> 16);
	}
	return (~sum) & 0xffff;
}

static void udp_datagram_free_all(struct udp_socket *socket)
{
	struct udp_datagram *datagram = socket->rx_head;

	while (datagram != 0) {
		struct udp_datagram *next = datagram->next;

		bunix_free(datagram);
		datagram = next;
	}
	socket->rx_head = 0;
	socket->rx_tail = 0;
	socket->rx_len = 0;
}

static void reply_udp_open(struct bunix_msg *reply,
			   const struct bunix_msg *message)
{
	const u64 family = message->words[0];
	struct udp_socket *socket;

	if (family != BUNIX_NET_ADDR_FAMILY_IPV4 &&
	    family != BUNIX_NET_ADDR_FAMILY_IPV6) {
		reply->words[0] = (u64)-1;
		return;
	}
	socket = (struct udp_socket *)bunix_alloc(sizeof(*socket));
	if (socket == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	socket->next = udp_sockets;
	socket->id = next_udp_socket_id++;
	socket->family = family;
	socket->local.family = family;
	socket->local.hi = 0;
	socket->local.lo = 0;
	socket->local.port = 0;
	socket->peer.family = family;
	socket->peer.hi = 0;
	socket->peer.lo = 0;
	socket->peer.port = 0;
	socket->bound = 0;
	socket->connected = 0;
	socket->rx_len = 0;
	socket->rx_head = 0;
	socket->rx_tail = 0;
	udp_sockets = socket;
	reply->words[0] = 0;
	reply->words[1] = socket->id;
}

static long udp_bind_socket(struct udp_socket *socket, u64 hi, u64 lo,
			    u64 port)
{
	if (socket == 0 ||
	    (!net_addr_is_wildcard(socket->family, hi, lo) &&
	     !net_addr_is_loopback(socket->family, hi, lo))) {
		return -1;
	}
	if (port == 0) {
		port = udp_alloc_ephemeral(socket->family);
	}
	if (port == 0 || port > 65535 ||
	    udp_port_in_use(socket->family, port)) {
		return -1;
	}

	socket->local.hi = hi;
	socket->local.lo = lo;
	socket->local.port = port;
	socket->bound = 1;
	return 0;
}

static void reply_udp_bind(struct bunix_msg *reply,
			   const struct bunix_msg *message)
{
	struct udp_socket *socket = udp_find(message->words[0]);
	const u64 port = message->words[3];

	if (udp_bind_socket(socket, message->words[1], message->words[2],
			    port) != 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	reply->words[0] = 0;
	reply->words[1] = socket->local.port;
}

static void reply_udp_connect(struct bunix_msg *reply,
			      const struct bunix_msg *message)
{
	struct udp_socket *socket = udp_find(message->words[0]);

	if (socket == 0 ||
	    !net_addr_is_loopback(socket->family, message->words[1],
				  message->words[2]) ||
	    message->words[3] == 0 || message->words[3] > 65535) {
		reply->words[0] = (u64)-1;
		return;
	}
	if (!socket->bound && udp_bind_socket(socket, 0, 0, 0) != 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	socket->peer.hi = message->words[1];
	socket->peer.lo = message->words[2];
	socket->peer.port = message->words[3];
	socket->connected = 1;
	reply->words[0] = 0;
	reply->words[1] = socket->local.port;
}

static int udp_deliver(struct udp_socket *source, const unsigned char *data,
		       u64 len)
{
	struct udp_socket *dest = 0;
	struct udp_datagram *datagram;

	for (struct udp_socket *socket = udp_sockets; socket != 0;
	     socket = socket->next) {
		if (socket->bound &&
		    socket->family == source->family &&
		    socket->local.port == source->peer.port &&
		    (net_addr_is_wildcard(socket->family, socket->local.hi,
					  socket->local.lo) ||
		     (socket->local.hi == source->peer.hi &&
		      socket->local.lo == source->peer.lo))) {
			dest = socket;
			break;
		}
	}
	if (dest == 0) {
		return -1;
	}
	datagram = (struct udp_datagram *)bunix_alloc(sizeof(*datagram) + len);
	if (datagram == 0) {
		return -1;
	}
	datagram->next = 0;
	datagram->source = source->local;
	datagram->checksum = udp_checksum(&source->local, &source->peer,
					  data, len);
	datagram->len = len;
	for (u64 i = 0; i < len; i++) {
		datagram->data[i] = data[i];
	}
	if (dest->rx_tail != 0) {
		dest->rx_tail->next = datagram;
	} else {
		dest->rx_head = datagram;
	}
	dest->rx_tail = datagram;
	dest->rx_len++;
	return 0;
}

static void reply_udp_send(struct bunix_msg *reply,
			   const struct bunix_msg *message)
{
	struct udp_socket *socket = udp_find(message->words[0]);
	const u64 len = message->words[1];
	unsigned char *data;
	long delivered;

	if (socket == 0 || !socket->connected || len > NET_PACKET_MAX ||
	    message->cap == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	data = (unsigned char *)bunix_alloc(len == 0 ? 1 : len);
	if (data == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	if (len != 0 && bunix_buffer_read(message->cap, 0, data, len) != 0) {
		bunix_free(data);
		reply->words[0] = (u64)-1;
		return;
	}
	delivered = udp_deliver(socket, data, len);
	bunix_free(data);
	if (delivered != 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	reply->words[0] = 0;
	reply->words[1] = len;
}

static void reply_udp_recv(struct bunix_msg *reply,
			   const struct bunix_msg *message)
{
	struct udp_socket *socket = udp_find(message->words[0]);
	const u64 max_len = message->words[1];
	struct udp_datagram *datagram;
	u64 len;

	if (socket == 0 || max_len == 0 || message->cap == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	datagram = socket->rx_head;
	if (datagram == 0) {
		reply->words[0] = (u64)-1;
		reply->words[1] = 0;
		return;
	}
	len = datagram->len < max_len ? datagram->len : max_len;
	if (bunix_buffer_write(message->cap, 0, datagram->data, len) != 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	socket->rx_head = datagram->next;
	if (socket->rx_head == 0) {
		socket->rx_tail = 0;
	}
	socket->rx_len--;
	reply->words[0] = 0;
	reply->words[1] = len;
	reply->words[2] = datagram->source.port;
	reply->words[3] = datagram->checksum;
	bunix_free(datagram);
}

static void reply_udp_poll(struct bunix_msg *reply,
			   const struct bunix_msg *message)
{
	struct udp_socket *socket = udp_find(message->words[0]);

	if (socket == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	reply->words[0] = 0;
	reply->words[1] = (socket->rx_head != 0 ? NET_UDP_POLLIN : 0) |
			  NET_UDP_POLLOUT;
}

static void reply_udp_close(struct bunix_msg *reply,
			    const struct bunix_msg *message)
{
	struct udp_socket **link = &udp_sockets;

	while (*link != 0) {
		struct udp_socket *socket = *link;

		if (socket->id == message->words[0]) {
			*link = socket->next;
			udp_datagram_free_all(socket);
			bunix_free(socket);
			reply->words[0] = 0;
			return;
		}
		link = &socket->next;
	}
	reply->words[0] = (u64)-1;
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
		case BUNIX_NET_UDP_OPEN:
			reply_udp_open(&reply, &message);
			break;
		case BUNIX_NET_UDP_BIND:
			reply_udp_bind(&reply, &message);
			break;
		case BUNIX_NET_UDP_CONNECT:
			reply_udp_connect(&reply, &message);
			break;
		case BUNIX_NET_UDP_SEND:
			reply_udp_send(&reply, &message);
			break;
		case BUNIX_NET_UDP_RECV:
			reply_udp_recv(&reply, &message);
			break;
		case BUNIX_NET_UDP_POLL:
			reply_udp_poll(&reply, &message);
			break;
		case BUNIX_NET_UDP_CLOSE:
			reply_udp_close(&reply, &message);
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
