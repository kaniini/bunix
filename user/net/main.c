#include <bunix/alloc.h>
#include <bunix/libbunix.h>

#define NET_HANDLE_NAMES (bunix_handle_find(BUNIX_CAP_NAME))

enum {
	NET_IFACE_LO = 1,
	NET_PACKET_MAX = 65536,
	NET_UDP_PORT_EPHEMERAL_FIRST = 49152,
	NET_UDP_PORT_EPHEMERAL_LAST = 65535,
	NET_UDP_POLLIN = 1 << 0,
	NET_UDP_POLLOUT = 1 << 1,
	NET_TCP_PORT_EPHEMERAL_FIRST = 49152,
	NET_TCP_PORT_EPHEMERAL_LAST = 65535,
	NET_TCP_STATE_OPEN = 1,
	NET_TCP_STATE_LISTEN = 2,
	NET_TCP_STATE_ESTABLISHED = 3,
	NET_TCP_STATE_CLOSED = 4,
	NET_TCP_POLLIN = 1 << 0,
	NET_TCP_POLLOUT = 1 << 1,
	NET_TCP_POLLHUP = 1 << 2,
	NET_TCP_SHUT_RD = 1 << 0,
	NET_TCP_SHUT_WR = 1 << 1,
	NET_PROTO_UDP = 17,
	NET_PROTO_TCP = 6,
	NET_PROTO_ICMP = 1,
	NET_PROTO_ICMPV6 = 58,
	NET_ETH_IPV4 = 0x0800,
	NET_ETH_ARP = 0x0806,
	NET_ETH_IPV6 = 0x86dd,
	NET_ICMPV6_ECHO_REQUEST = 128,
	NET_ICMPV6_ECHO_REPLY = 129,
	NET_ICMPV6_ROUTER_SOLICIT = 133,
	NET_ICMPV6_ROUTER_ADVERT = 134,
	NET_ICMPV6_NEIGHBOR_SOLICIT = 135,
	NET_ICMPV6_NEIGHBOR_ADVERT = 136,
	NET_ICMP_POLLIN = 1 << 0,
	NET_ICMP_POLLOUT = 1 << 1,
	NET_PACKET_SOCKADDR_LL_SIZE = 20,
	NET_PACKET_AF_PACKET = 17,
	NET_PACKET_ARPHRD_ETHER = 1,
	NET_PACKET_HOST = 0,
	NET_PACKET_BROADCAST = 1,
	NET_NEIGHBOR_TTL_NS = 120000000000ull,
};

struct net_packet {
	struct net_packet *next;
	u64 family;
	u64 len;
	unsigned char data[];
};

struct net_interface {
	struct net_interface *next;
	struct net_packet *tx_head;
	struct net_packet *tx_tail;
	struct net_packet *rx_head;
	struct net_packet *rx_tail;
	u64 id;
	u64 flags;
	u64 mtu;
	u64 mac_hi;
	u64 mac_lo;
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

struct net_route {
	struct net_route *next;
	u64 family;
	u64 prefix_hi;
	u64 prefix_lo;
	u64 prefix_len;
	u64 iface;
	u64 gateway_hi;
	u64 gateway_lo;
	u64 flags;
	u64 metric;
};

struct net_neighbor {
	struct net_neighbor *next;
	u64 family;
	u64 addr_hi;
	u64 addr_lo;
	u64 iface;
	u64 mac_hi;
	u64 mac_lo;
	u64 flags;
	u64 state;
	u64 updated_ns;
};

struct net_pending_frame {
	struct net_pending_frame *next;
	u64 family;
	u64 addr_hi;
	u64 addr_lo;
	u64 iface;
	u64 len;
	unsigned char data[];
};

struct net_addr_record {
	struct net_addr_record *next;
	u64 family;
	u64 addr_hi;
	u64 addr_lo;
	u64 prefix_len;
	u64 iface;
	u64 flags;
	u64 preferred_lifetime;
	u64 valid_lifetime;
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

struct tcp_segment {
	struct tcp_segment *next;
	u64 offset;
	u64 len;
	unsigned char data[];
};

struct tcp_pending {
	struct tcp_pending *next;
	u64 socket_id;
};

struct tcp_socket {
	struct tcp_socket *next;
	u64 id;
	u64 family;
	u64 state;
	struct net_addr local;
	struct net_addr peer_addr;
	u64 peer_socket;
	u64 backlog;
	u64 pending_count;
	u64 read_closed;
	u64 write_closed;
	u64 peer_write_closed;
	u64 reset;
	u64 rx_len;
	struct tcp_segment *rx_head;
	struct tcp_segment *rx_tail;
	struct tcp_pending *pending_head;
	struct tcp_pending *pending_tail;
};

struct icmp_datagram {
	struct icmp_datagram *next;
	u64 family;
	u64 source_hi;
	u64 source_lo;
	u64 len;
	unsigned char data[];
};

struct icmp_socket {
	struct icmp_socket *next;
	u64 id;
	u64 family;
	u64 protocol;
	u64 rx_len;
	struct icmp_datagram *rx_head;
	struct icmp_datagram *rx_tail;
};

static struct net_interface loopback = {
	.next = 0,
	.tx_head = 0,
	.tx_tail = 0,
	.rx_head = 0,
	.rx_tail = 0,
	.id = NET_IFACE_LO,
	.flags = BUNIX_NET_IFACE_FLAG_UP | BUNIX_NET_IFACE_FLAG_LOOPBACK,
	.mtu = 65536,
	.mac_hi = 0,
	.mac_lo = 0,
	.ipv4_be = 0x7f000001,
	.ipv6_hi_be = 0,
	.ipv6_lo_be = 1,
	.rx_packets = 0,
	.tx_packets = 0,
	.rx_drops = 0,
	.tx_drops = 0,
};
static struct net_interface *packet_ifaces;
static u64 next_packet_iface_id = 2;
static const struct net_route routes[] = {
	{
		.next = 0,
		.family = BUNIX_NET_ADDR_FAMILY_IPV4,
		.prefix_hi = 0,
		.prefix_lo = 0x7f000000,
		.prefix_len = 8,
		.iface = NET_IFACE_LO,
		.gateway_hi = 0,
		.gateway_lo = 0,
		.flags = 1,
		.metric = 0,
	},
	{
		.next = 0,
		.family = BUNIX_NET_ADDR_FAMILY_IPV6,
		.prefix_hi = 0,
		.prefix_lo = 1,
		.prefix_len = 128,
		.iface = NET_IFACE_LO,
		.gateway_hi = 0,
		.gateway_lo = 0,
		.flags = 1,
		.metric = 0,
	},
};
static struct net_route *dynamic_routes;
static const struct net_addr_record static_addrs[] = {
	{
		.next = 0,
		.family = BUNIX_NET_ADDR_FAMILY_IPV4,
		.addr_hi = 0,
		.addr_lo = 0x7f000001,
		.prefix_len = 8,
		.iface = NET_IFACE_LO,
		.flags = 1,
		.preferred_lifetime = 0,
		.valid_lifetime = 0,
	},
	{
		.next = 0,
		.family = BUNIX_NET_ADDR_FAMILY_IPV6,
		.addr_hi = 0,
		.addr_lo = 1,
		.prefix_len = 128,
		.iface = NET_IFACE_LO,
		.flags = 1,
		.preferred_lifetime = 0,
		.valid_lifetime = 0,
	},
};
static struct net_addr_record *dynamic_addrs;
static struct net_neighbor *dynamic_neighbors;
static struct net_pending_frame *pending_frames;
static u64 last_config_error;
static struct net_packet *loopback_rx_head;
static struct net_packet *loopback_rx_tail;
static struct udp_socket *udp_sockets;
static u64 next_udp_socket_id = 1;
static u64 next_ephemeral_port = NET_UDP_PORT_EPHEMERAL_FIRST;
static struct tcp_socket *tcp_sockets;
static u64 next_tcp_socket_id = 1;
static u64 next_tcp_ephemeral_port = NET_TCP_PORT_EPHEMERAL_FIRST;
static struct icmp_socket *icmp_sockets;
static u64 next_icmp_socket_id = 1;

static void neighbor_prune_expired(void);
static void neighbor_learn(u64 family, u64 hi, u64 lo, u64 iface,
			   u64 mac_hi, u64 mac_lo);
static void packet_ingress_tcp_ipv4(struct net_interface *iface,
				    const unsigned char *packet, u64 len);
static void packet_ingress_tcp_ipv6(struct net_interface *iface,
				    const unsigned char *packet, u64 len);
static u64 iface_ipv4_addr(u64 iface_id);
static int iface_ipv6_addr(u64 iface_id, u64 *hi, u64 *lo);
static int packet_tx_enqueue_copy(struct net_interface *iface,
				  const unsigned char *data, u64 len);
static long udp_bind_socket(struct udp_socket *socket, u64 hi, u64 lo,
			    u64 port);
static u64 net_checksum_sum(const unsigned char *data, u64 len, u64 sum);
static unsigned short net_checksum_finish(u64 sum);
static unsigned short icmp_ipv6_checksum(const unsigned char *data, u64 len,
					 u64 source_hi, u64 source_lo,
					 u64 dest_hi, u64 dest_lo);
static unsigned short ipv6_l4_checksum(const unsigned char *data, u64 len,
				       u64 protocol, u64 source_hi,
				       u64 source_lo, u64 dest_hi,
				       u64 dest_lo);

static const struct net_route *net_route_lookup(u64 family, u64 hi, u64 lo);
static int net_addr_is_local(u64 family, u64 hi, u64 lo);
static int icmp_enqueue(struct icmp_socket *socket, u64 family,
			u64 source_hi, u64 source_lo,
			const unsigned char *data, u64 len);

static long register_service(u64 service, u64 handle)
{
	(void)service;
	return bunix_names_register_claim(bunix_handle_find(BUNIX_CAP_CLAM),
					  handle);
}

static struct net_interface *interface_find(u64 id)
{
	if (id == loopback.id) {
		return &loopback;
	}
	for (struct net_interface *iface = packet_ifaces; iface != 0;
	     iface = iface->next) {
		if (iface->id == id) {
			return iface;
		}
	}
	return 0;
}

static u64 interface_count(void)
{
	u64 count = 1;

	for (const struct net_interface *iface = packet_ifaces; iface != 0;
	     iface = iface->next) {
		count++;
	}
	return count;
}

static struct net_interface *interface_at(u64 index)
{
	if (index == 0) {
		return &loopback;
	}
	index--;
	for (struct net_interface *iface = packet_ifaces; iface != 0;
	     iface = iface->next) {
		if (index == 0) {
			return iface;
		}
		index--;
	}
	return 0;
}

static void reply_interface_count(struct bunix_msg *reply)
{
	reply->words[0] = 0;
	reply->words[1] = interface_count();
}

static void reply_interface_info(struct bunix_msg *reply,
				 const struct bunix_msg *message)
{
	struct net_interface *iface = interface_find(message->words[0]);

	if (iface == 0) {
		reply->words[0] = (u64)-1;
		return;
	}

	reply->words[0] = 0;
	reply->words[1] = iface->id;
	reply->words[2] = iface->flags;
	reply->words[3] = iface->mtu;
}

static void packet_iface_info_store(struct bunix_net_packet_interface_info *info,
				    const struct net_interface *iface);

static u64 packet_queue_count(const struct net_packet *packet)
{
	u64 count = 0;

	while (packet != 0) {
		count++;
		packet = packet->next;
	}
	return count;
}

static void reply_interface_at(struct bunix_msg *reply,
			       const struct bunix_msg *message)
{
	struct net_interface *iface = interface_at(message->words[0]);

	if (iface == 0) {
		reply->words[0] = (u64)-1;
		return;
	}

	reply->words[0] = 0;
	reply->words[1] = iface->id;
	reply->words[2] = iface->flags;
	reply->words[3] = iface->mtu;
}

static void reply_interface_details(struct bunix_msg *reply,
				    const struct bunix_msg *message)
{
	struct net_interface *iface = interface_find(message->words[0]);
	struct bunix_net_packet_interface_info info;

	if (iface == 0 || message->cap == 0 ||
	    (message->cap_rights & BUNIX_RIGHT_SEND) == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	packet_iface_info_store(&info, iface);
	if (bunix_buffer_write(message->cap, 0, &info, sizeof(info)) != 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	reply->words[0] = 0;
	reply->words[1] = iface->id;
	reply->words[2] = iface->flags;
	reply->words[3] = iface->mtu;
}

static void reply_interface_stats(struct bunix_msg *reply,
				  const struct bunix_msg *message)
{
	struct net_interface *iface = interface_find(message->words[0]);

	if (iface == 0) {
		reply->words[0] = (u64)-1;
		return;
	}

	reply->words[0] = 0;
	reply->words[1] = iface->rx_packets;
	reply->words[2] = iface->tx_packets;
	reply->words[3] = iface->rx_drops + iface->tx_drops;
}

static void packet_iface_info_store(struct bunix_net_packet_interface_info *info,
				    const struct net_interface *iface)
{
	if (info == 0 || iface == 0) {
		return;
	}
	info->id = iface->id;
	info->flags = iface->flags;
	info->mtu = iface->mtu;
	info->mac_hi = iface->mac_hi;
	info->mac_lo = iface->mac_lo;
	info->rx_packets = iface->rx_packets;
	info->tx_packets = iface->tx_packets;
	info->rx_drops = iface->rx_drops;
	info->tx_drops = iface->tx_drops;
	info->rx_pending = packet_queue_count(iface->rx_head);
	info->tx_pending = packet_queue_count(iface->tx_head);
}

static void reply_packet_interface_attach(struct bunix_msg *reply,
					  const struct bunix_msg *message)
{
	struct bunix_net_packet_interface_info info;
	struct net_interface *iface;

	if (message->cap == 0 ||
	    (message->cap_rights & (BUNIX_RIGHT_SEND | BUNIX_RIGHT_RECV)) !=
		    (BUNIX_RIGHT_SEND | BUNIX_RIGHT_RECV) ||
	    bunix_buffer_read(message->cap, 0, &info, sizeof(info)) != 0 ||
	    info.mtu == 0 || info.mtu > NET_PACKET_MAX) {
		reply->words[0] = (u64)-1;
		return;
	}
	iface = (struct net_interface *)bunix_calloc(1, sizeof(*iface));
	if (iface == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	iface->id = next_packet_iface_id++;
	iface->flags = info.flags & (BUNIX_NET_IFACE_FLAG_UP |
				     BUNIX_NET_IFACE_FLAG_BROADCAST |
				     BUNIX_NET_IFACE_FLAG_RUNNING);
	iface->mtu = info.mtu;
	iface->mac_hi = info.mac_hi;
	iface->mac_lo = info.mac_lo;
	iface->next = packet_ifaces;
	packet_ifaces = iface;
	packet_iface_info_store(&info, iface);
	(void)bunix_buffer_write(message->cap, 0, &info, sizeof(info));
	reply->words[0] = 0;
	reply->words[1] = iface->id;
	reply->words[2] = iface->flags;
	reply->words[3] = iface->mtu;
}

static void reply_packet_interface_link(struct bunix_msg *reply,
					const struct bunix_msg *message)
{
	struct net_interface *iface = interface_find(message->words[0]);
	const u64 flags = message->words[1];

	if (iface == 0 || iface == &loopback) {
		reply->words[0] = (u64)-1;
		return;
	}
	iface->flags &= ~(BUNIX_NET_IFACE_FLAG_UP |
			  BUNIX_NET_IFACE_FLAG_RUNNING |
			  BUNIX_NET_IFACE_FLAG_BROADCAST);
	iface->flags |= flags & (BUNIX_NET_IFACE_FLAG_UP |
				 BUNIX_NET_IFACE_FLAG_RUNNING |
				 BUNIX_NET_IFACE_FLAG_BROADCAST);
	reply->words[0] = 0;
	reply->words[1] = iface->id;
	reply->words[2] = iface->flags;
	reply->words[3] = iface->mtu;
}

static void reply_interface_set_flags(struct bunix_msg *reply,
				      const struct bunix_msg *message)
{
	struct net_interface *iface = interface_find(message->words[0]);
	const u64 set = message->words[1];
	const u64 clear = message->words[2];
	const u64 mutable = BUNIX_NET_IFACE_FLAG_UP |
			    BUNIX_NET_IFACE_FLAG_BROADCAST;
	u64 preserved;

	if (iface == 0 || iface == &loopback) {
		last_config_error = BUNIX_NET_INTERFACE_SET_FLAGS;
		reply->words[0] = (u64)-1;
		return;
	}
	preserved = iface->flags & ~mutable;
	iface->flags = preserved | ((iface->flags | (set & mutable)) &
				    ~(clear & mutable));
	reply->words[0] = 0;
	reply->words[1] = iface->id;
	reply->words[2] = iface->flags;
	reply->words[3] = iface->mtu;
}

static int net_route_valid_prefix(u64 family, u64 prefix_len)
{
	if (family == BUNIX_NET_ADDR_FAMILY_IPV4) {
		return prefix_len <= 32;
	}
	if (family == BUNIX_NET_ADDR_FAMILY_IPV6) {
		return prefix_len <= 128;
	}
	return 0;
}

static int net_addr_valid_prefix(u64 family, u64 prefix_len)
{
	return net_route_valid_prefix(family, prefix_len);
}

static u64 addr_count(void)
{
	u64 count = sizeof(static_addrs) / sizeof(static_addrs[0]);

	for (const struct net_addr_record *addr = dynamic_addrs; addr != 0;
	     addr = addr->next) {
		count++;
	}
	return count;
}

static const struct net_addr_record *addr_at(u64 index)
{
	for (const struct net_addr_record *addr = dynamic_addrs; addr != 0;
	     addr = addr->next) {
		if (index == 0) {
			return addr;
		}
		index--;
	}
	if (index < sizeof(static_addrs) / sizeof(static_addrs[0])) {
		return &static_addrs[index];
	}
	return 0;
}

static int addr_equal_record(const struct net_addr_record *addr,
			     const struct bunix_net_addr_info *info)
{
	return addr != 0 && info != 0 &&
	       addr->family == info->family &&
	       addr->addr_hi == info->addr_hi &&
	       addr->addr_lo == info->addr_lo &&
	       addr->prefix_len == info->prefix_len &&
	       addr->iface == info->iface;
}

static int addr_exists(const struct bunix_net_addr_info *info)
{
	for (const struct net_addr_record *addr = dynamic_addrs; addr != 0;
	     addr = addr->next) {
		if (addr_equal_record(addr, info)) {
			return 1;
		}
	}
	for (u64 i = 0; i < sizeof(static_addrs) / sizeof(static_addrs[0]); i++) {
		if (addr_equal_record(&static_addrs[i], info)) {
			return 1;
		}
	}
	return 0;
}

static void addr_info_store(struct bunix_net_addr_info *info,
			    const struct net_addr_record *addr)
{
	if (info == 0 || addr == 0) {
		return;
	}
	info->family = addr->family;
	info->addr_hi = addr->addr_hi;
	info->addr_lo = addr->addr_lo;
	info->prefix_len = addr->prefix_len;
	info->iface = addr->iface;
	info->flags = addr->flags;
	info->preferred_lifetime = addr->preferred_lifetime;
	info->valid_lifetime = addr->valid_lifetime;
}

static void reply_addr_add(struct bunix_msg *reply,
			   const struct bunix_msg *message)
{
	struct bunix_net_addr_info info;
	struct net_addr_record *addr;

	if (message->cap == 0 ||
	    (message->cap_rights & BUNIX_RIGHT_RECV) == 0 ||
	    bunix_buffer_read(message->cap, 0, &info, sizeof(info)) != 0 ||
	    interface_find(info.iface) == 0 ||
	    !net_addr_valid_prefix(info.family, info.prefix_len)) {
		last_config_error = BUNIX_NET_ADDR_ADD;
		reply->words[0] = (u64)-1;
		return;
	}
	if (addr_exists(&info)) {
		reply->words[0] = 0;
		reply->words[1] = info.iface;
		reply->words[2] = addr_count();
		reply->words[3] = info.prefix_len;
		return;
	}
	addr = (struct net_addr_record *)bunix_calloc(1, sizeof(*addr));
	if (addr == 0) {
		last_config_error = BUNIX_NET_ADDR_ADD;
		reply->words[0] = (u64)-1;
		return;
	}
	addr->family = info.family;
	addr->addr_hi = info.addr_hi;
	addr->addr_lo = info.addr_lo;
	addr->prefix_len = info.prefix_len;
	addr->iface = info.iface;
	addr->flags = info.flags;
	addr->preferred_lifetime = info.preferred_lifetime;
	addr->valid_lifetime = info.valid_lifetime;
	addr->next = dynamic_addrs;
	dynamic_addrs = addr;
	reply->words[0] = 0;
	reply->words[1] = addr->iface;
	reply->words[2] = addr_count();
	reply->words[3] = addr->prefix_len;
}

static void reply_addr_delete(struct bunix_msg *reply,
			      const struct bunix_msg *message)
{
	struct bunix_net_addr_info info;
	struct net_addr_record **link = &dynamic_addrs;

	if (message->cap == 0 ||
	    (message->cap_rights & BUNIX_RIGHT_RECV) == 0 ||
	    bunix_buffer_read(message->cap, 0, &info, sizeof(info)) != 0) {
		last_config_error = BUNIX_NET_ADDR_DELETE;
		reply->words[0] = (u64)-1;
		return;
	}
	while (*link != 0) {
		struct net_addr_record *addr = *link;

		if (addr_equal_record(addr, &info)) {
			*link = addr->next;
			bunix_free(addr);
			reply->words[0] = 0;
			reply->words[1] = addr_count();
			return;
		}
		link = &addr->next;
	}
	last_config_error = BUNIX_NET_ADDR_DELETE;
	reply->words[0] = (u64)-1;
}

static void reply_addr_count(struct bunix_msg *reply)
{
	reply->words[0] = 0;
	reply->words[1] = addr_count();
}

static void reply_addr_at(struct bunix_msg *reply,
			  const struct bunix_msg *message)
{
	const struct net_addr_record *addr = addr_at(message->words[0]);
	struct bunix_net_addr_info info;

	if (addr == 0 || message->cap == 0 ||
	    (message->cap_rights & BUNIX_RIGHT_SEND) == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	addr_info_store(&info, addr);
	if (bunix_buffer_write(message->cap, 0, &info, sizeof(info)) != 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	reply->words[0] = 0;
	reply->words[1] = addr->iface;
	reply->words[2] = addr->family;
	reply->words[3] = addr->prefix_len;
}

static u64 route_count(void)
{
	u64 count = sizeof(routes) / sizeof(routes[0]);

	for (const struct net_route *route = dynamic_routes; route != 0;
	     route = route->next) {
		count++;
	}
	return count;
}

static const struct net_route *route_at(u64 index)
{
	for (const struct net_route *route = dynamic_routes; route != 0;
	     route = route->next) {
		if (index == 0) {
			return route;
		}
		index--;
	}
	if (index < sizeof(routes) / sizeof(routes[0])) {
		return &routes[index];
	}
	return 0;
}

static void route_info_store(struct bunix_net_route_info *info,
			     const struct net_route *route)
{
	if (info == 0 || route == 0) {
		return;
	}
	info->family = route->family;
	info->prefix_hi = route->prefix_hi;
	info->prefix_lo = route->prefix_lo;
	info->prefix_len = route->prefix_len;
	info->iface = route->iface;
	info->gateway_hi = route->gateway_hi;
	info->gateway_lo = route->gateway_lo;
	info->flags = route->flags;
	info->metric = route->metric;
}

static int route_equal_info(const struct net_route *route,
			    const struct bunix_net_route_info *info)
{
	return route != 0 && info != 0 &&
	       route->family == info->family &&
	       route->prefix_hi == info->prefix_hi &&
	       route->prefix_lo == info->prefix_lo &&
	       route->prefix_len == info->prefix_len &&
	       route->iface == info->iface &&
	       route->gateway_hi == info->gateway_hi &&
	       route->gateway_lo == info->gateway_lo &&
	       route->flags == info->flags &&
	       route->metric == info->metric;
}

static int route_exists(const struct bunix_net_route_info *info)
{
	for (const struct net_route *route = dynamic_routes; route != 0;
	     route = route->next) {
		if (route_equal_info(route, info)) {
			return 1;
		}
	}
	for (u64 i = 0; i < sizeof(routes) / sizeof(routes[0]); i++) {
		if (route_equal_info(&routes[i], info)) {
			return 1;
		}
	}
	return 0;
}

static void reply_route_add(struct bunix_msg *reply,
			    const struct bunix_msg *message)
{
	struct bunix_net_route_info info;
	struct net_route *route;

	if (message->cap == 0 ||
	    (message->cap_rights & BUNIX_RIGHT_RECV) == 0 ||
	    bunix_buffer_read(message->cap, 0, &info, sizeof(info)) != 0 ||
	    interface_find(info.iface) == 0 ||
	    !net_route_valid_prefix(info.family, info.prefix_len)) {
		last_config_error = BUNIX_NET_ROUTE_ADD;
		reply->words[0] = (u64)-1;
		return;
	}
	if (route_exists(&info)) {
		reply->words[0] = 0;
		reply->words[1] = info.iface;
		reply->words[2] = route_count();
		reply->words[3] = info.prefix_len;
		return;
	}
	route = (struct net_route *)bunix_calloc(1, sizeof(*route));
	if (route == 0) {
		last_config_error = BUNIX_NET_ROUTE_ADD;
		reply->words[0] = (u64)-1;
		return;
	}
	route->family = info.family;
	route->prefix_hi = info.prefix_hi;
	route->prefix_lo = info.prefix_lo;
	route->prefix_len = info.prefix_len;
	route->iface = info.iface;
	route->gateway_hi = info.gateway_hi;
	route->gateway_lo = info.gateway_lo;
	route->flags = info.flags;
	route->metric = info.metric;
	route->next = dynamic_routes;
	dynamic_routes = route;
	reply->words[0] = 0;
	reply->words[1] = route->iface;
	reply->words[2] = route_count();
	reply->words[3] = route->prefix_len;
}

static void reply_route_delete(struct bunix_msg *reply,
			       const struct bunix_msg *message)
{
	struct bunix_net_route_info info;
	struct net_route **link = &dynamic_routes;

	if (message->cap == 0 ||
	    (message->cap_rights & BUNIX_RIGHT_RECV) == 0 ||
	    bunix_buffer_read(message->cap, 0, &info, sizeof(info)) != 0) {
		last_config_error = BUNIX_NET_ROUTE_DELETE;
		reply->words[0] = (u64)-1;
		return;
	}
	while (*link != 0) {
		struct net_route *route = *link;

		if (route_equal_info(route, &info)) {
			*link = route->next;
			bunix_free(route);
			reply->words[0] = 0;
			reply->words[1] = route_count();
			return;
		}
		link = &route->next;
	}
	last_config_error = BUNIX_NET_ROUTE_DELETE;
	reply->words[0] = (u64)-1;
}

static void reply_route_count(struct bunix_msg *reply)
{
	reply->words[0] = 0;
	reply->words[1] = route_count();
}

static void reply_route_at(struct bunix_msg *reply,
			   const struct bunix_msg *message)
{
	const struct net_route *route = route_at(message->words[0]);
	struct bunix_net_route_info info;

	if (route == 0 || message->cap == 0 ||
	    (message->cap_rights & BUNIX_RIGHT_SEND) == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	route_info_store(&info, route);
	if (bunix_buffer_write(message->cap, 0, &info, sizeof(info)) != 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	reply->words[0] = 0;
	reply->words[1] = route->iface;
	reply->words[2] = route->family;
	reply->words[3] = route->prefix_len;
}

static int has_addr(u64 family, u64 iface)
{
	for (const struct net_addr_record *addr = dynamic_addrs; addr != 0;
	     addr = addr->next) {
		if (addr->family == family && addr->iface == iface) {
			return 1;
		}
	}
	for (u64 i = 0; i < sizeof(static_addrs) / sizeof(static_addrs[0]); i++) {
		if (static_addrs[i].family == family &&
		    static_addrs[i].iface == iface) {
			return 1;
		}
	}
	return 0;
}

static const struct net_route *default_route(u64 family)
{
	const struct net_route *best =
		net_route_lookup(family, 0, family == BUNIX_NET_ADDR_FAMILY_IPV4 ?
				 0x08080808ull : 0);

	return best != 0 && best->prefix_len == 0 ? best : 0;
}

static void reply_config_status(struct bunix_msg *reply,
				const struct bunix_msg *message)
{
	struct bunix_net_config_status status = {
		.flags = 0,
		.interface_count = interface_count(),
		.address_count = addr_count(),
		.route_count = route_count(),
		.default_ipv4_iface = 0,
		.default_ipv6_iface = 0,
		.last_error = last_config_error,
		.reserved = 0,
	};
	const struct net_route *route4 = default_route(BUNIX_NET_ADDR_FAMILY_IPV4);
	const struct net_route *route6 = default_route(BUNIX_NET_ADDR_FAMILY_IPV6);

	if (has_addr(BUNIX_NET_ADDR_FAMILY_IPV4, NET_IFACE_LO) &&
	    has_addr(BUNIX_NET_ADDR_FAMILY_IPV6, NET_IFACE_LO)) {
		status.flags |= BUNIX_NET_CONFIG_LOOPBACK;
	}
	if (route4 != 0) {
		status.flags |= BUNIX_NET_CONFIG_DEFAULT_IPV4;
		status.default_ipv4_iface = route4->iface;
	}
	if (route6 != 0) {
		status.flags |= BUNIX_NET_CONFIG_DEFAULT_IPV6;
		status.default_ipv6_iface = route6->iface;
	}
	if (message->cap != 0 &&
	    (message->cap_rights & BUNIX_RIGHT_SEND) != 0 &&
	    bunix_buffer_write(message->cap, 0, &status,
			       sizeof(status)) != 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	reply->words[0] = 0;
	reply->words[1] = status.flags;
	reply->words[2] = status.address_count;
	reply->words[3] = status.route_count;
}

static void reply_dhcp4_lease_install(struct bunix_msg *reply,
				      const struct bunix_msg *message)
{
	struct bunix_net_dhcp4_lease lease;
	struct bunix_net_addr_info addr;
	struct bunix_net_route_info route;
	long buffer;
	struct bunix_msg synthetic = *message;

	if (message->cap == 0 ||
	    (message->cap_rights & BUNIX_RIGHT_RECV) == 0 ||
	    bunix_buffer_read(message->cap, 0, &lease, sizeof(lease)) != 0 ||
	    lease.address == 0 || lease.prefix_len > 32 ||
	    interface_find(lease.iface) == 0) {
		last_config_error = BUNIX_NET_DHCP4_LEASE_INSTALL;
		reply->words[0] = (u64)-1;
		return;
	}
	buffer = bunix_buffer_create(sizeof(route) > sizeof(addr) ?
				     sizeof(route) : sizeof(addr));
	if (buffer <= 0) {
		last_config_error = BUNIX_NET_DHCP4_LEASE_INSTALL;
		reply->words[0] = (u64)-1;
		return;
	}
	addr.family = BUNIX_NET_ADDR_FAMILY_IPV4;
	addr.addr_hi = 0;
	addr.addr_lo = lease.address;
	addr.prefix_len = lease.prefix_len;
	addr.iface = lease.iface;
	addr.flags = 1;
	addr.preferred_lifetime = lease.lease_lifetime;
	addr.valid_lifetime = lease.lease_lifetime;
	synthetic.cap = (u64)buffer;
	synthetic.cap_rights = BUNIX_RIGHT_RECV;
	if (bunix_buffer_write((u64)buffer, 0, &addr, sizeof(addr)) != 0) {
		bunix_handle_close((u64)buffer);
		last_config_error = BUNIX_NET_DHCP4_LEASE_INSTALL;
		reply->words[0] = (u64)-1;
		return;
	}
	reply_addr_add(reply, &synthetic);
	if (reply->words[0] != 0) {
		bunix_handle_close((u64)buffer);
		last_config_error = BUNIX_NET_DHCP4_LEASE_INSTALL;
		return;
	}
	if (lease.gateway != 0) {
		route.family = BUNIX_NET_ADDR_FAMILY_IPV4;
		route.prefix_hi = 0;
		route.prefix_lo = 0;
		route.prefix_len = 0;
		route.iface = lease.iface;
		route.gateway_hi = 0;
		route.gateway_lo = lease.gateway;
		route.flags = BUNIX_NET_ROUTE_FLAG_UP |
			      BUNIX_NET_ROUTE_FLAG_GATEWAY;
		route.metric = 100;
		if (bunix_buffer_write((u64)buffer, 0, &route,
				       sizeof(route)) != 0) {
			bunix_handle_close((u64)buffer);
			last_config_error = BUNIX_NET_DHCP4_LEASE_INSTALL;
			reply->words[0] = (u64)-1;
			return;
		}
		reply_route_add(reply, &synthetic);
		if (reply->words[0] != 0) {
			bunix_handle_close((u64)buffer);
			last_config_error = BUNIX_NET_DHCP4_LEASE_INSTALL;
			return;
		}
	}
	bunix_handle_close((u64)buffer);
	reply->words[0] = 0;
	reply->words[1] = lease.iface;
	reply->words[2] = addr_count();
	reply->words[3] = route_count();
}

static void reply_neighbor_count(struct bunix_msg *reply)
{
	u64 count = 0;

	neighbor_prune_expired();
	for (const struct net_neighbor *neighbor = dynamic_neighbors;
	     neighbor != 0; neighbor = neighbor->next) {
		count++;
	}
	reply->words[0] = 0;
	reply->words[1] = count;
}

static void neighbor_info_store(struct bunix_net_neighbor_info *info,
				const struct net_neighbor *neighbor)
{
	if (info == 0 || neighbor == 0) {
		return;
	}
	info->family = neighbor->family;
	info->addr_hi = neighbor->addr_hi;
	info->addr_lo = neighbor->addr_lo;
	info->iface = neighbor->iface;
	info->mac_hi = neighbor->mac_hi;
	info->mac_lo = neighbor->mac_lo;
	info->flags = neighbor->flags;
	info->state = neighbor->state;
}

static void reply_neighbor_at(struct bunix_msg *reply,
			      const struct bunix_msg *message)
{
	struct net_neighbor *neighbor = dynamic_neighbors;
	u64 index = message->words[0];
	struct bunix_net_neighbor_info info;

	neighbor_prune_expired();
	if (message->cap == 0 ||
	    (message->cap_rights & BUNIX_RIGHT_SEND) == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	while (neighbor != 0 && index != 0) {
		neighbor = neighbor->next;
		index--;
	}
	if (neighbor == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	neighbor_info_store(&info, neighbor);
	if (bunix_buffer_write(message->cap, 0, &info, sizeof(info)) != 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	reply->words[0] = 0;
	reply->words[1] = sizeof(info);
}

static void reply_neighbor_add(struct bunix_msg *reply,
			       const struct bunix_msg *message)
{
	struct bunix_net_neighbor_info info;

	if (message->cap == 0 ||
	    (message->cap_rights & BUNIX_RIGHT_RECV) == 0 ||
	    bunix_buffer_read(message->cap, 0, &info, sizeof(info)) != 0 ||
	    info.family != BUNIX_NET_ADDR_FAMILY_IPV4 ||
	    info.addr_hi != 0 || info.addr_lo == 0 ||
	    info.mac_hi == 0 || interface_find(info.iface) == 0 ||
	    info.iface == NET_IFACE_LO) {
		reply->words[0] = (u64)-1;
		return;
	}
	neighbor_learn(info.family, info.addr_hi, info.addr_lo, info.iface,
		       info.mac_hi, info.mac_lo);
	reply->words[0] = 0;
	reply->words[1] = info.iface;
	reply->words[2] = info.addr_lo;
}

static void reply_neighbor_delete(struct bunix_msg *reply,
				  const struct bunix_msg *message)
{
	struct net_neighbor **link = &dynamic_neighbors;
	const u64 family = message->words[0];
	const u64 hi = message->words[1];
	const u64 lo = message->words[2];
	const u64 iface = message->words[3];

	while (*link != 0) {
		struct net_neighbor *neighbor = *link;

		if (neighbor->family == family && neighbor->addr_hi == hi &&
		    neighbor->addr_lo == lo && neighbor->iface == iface) {
			*link = neighbor->next;
			bunix_free(neighbor);
			reply->words[0] = 0;
			return;
		}
		link = &neighbor->next;
	}
	reply->words[0] = (u64)-1;
}

static void reply_observe_socket_count(struct bunix_msg *reply)
{
	u64 count = 0;

	for (const struct udp_socket *socket = udp_sockets; socket != 0;
	     socket = socket->next) {
		count++;
	}
	for (const struct tcp_socket *socket = tcp_sockets; socket != 0;
	     socket = socket->next) {
		count++;
	}
	for (const struct icmp_socket *socket = icmp_sockets; socket != 0;
	     socket = socket->next) {
		count++;
	}
	reply->words[0] = 0;
	reply->words[1] = count;
}

static void reply_observe_socket_at(struct bunix_msg *reply,
				    const struct bunix_msg *message)
{
	u64 index = message->words[0];

	for (const struct udp_socket *socket = udp_sockets; socket != 0;
	     socket = socket->next) {
		if (index == 0) {
			struct bunix_net_socket_info info;
			const struct net_addr *peer =
				socket->connected ? &socket->peer : 0;

			if (message->cap == 0 ||
			    (message->cap_rights & BUNIX_RIGHT_SEND) == 0) {
				reply->words[0] = (u64)-1;
				return;
			}
			info.id = socket->id;
			info.protocol = NET_PROTO_UDP;
			info.family = socket->family;
			info.state = socket->bound;
			info.local_hi = socket->local.hi;
			info.local_lo = socket->local.lo;
			info.local_port = socket->local.port;
			info.peer_hi = peer == 0 ? 0 : peer->hi;
			info.peer_lo = peer == 0 ? 0 : peer->lo;
			info.peer_port = peer == 0 ? 0 : peer->port;
			info.rx_len = socket->rx_len;
			info.tx_len = 0;
			if (bunix_buffer_write(message->cap, 0, &info,
					       sizeof(info)) != 0) {
				reply->words[0] = (u64)-1;
				return;
			}
			reply->words[0] = 0;
			reply->words[1] = sizeof(info);
			return;
		}
		index--;
	}
	for (const struct tcp_socket *socket = tcp_sockets; socket != 0;
	     socket = socket->next) {
		if (index == 0) {
			struct bunix_net_socket_info info;

			if (message->cap == 0 ||
			    (message->cap_rights & BUNIX_RIGHT_SEND) == 0) {
				reply->words[0] = (u64)-1;
				return;
			}
			info.id = socket->id;
			info.protocol = NET_PROTO_TCP;
			info.family = socket->family;
			info.state = socket->state;
			info.local_hi = socket->local.hi;
			info.local_lo = socket->local.lo;
			info.local_port = socket->local.port;
			info.peer_hi = socket->peer_addr.hi;
			info.peer_lo = socket->peer_addr.lo;
			info.peer_port = socket->peer_addr.port;
			info.rx_len = socket->rx_len;
			info.tx_len = 0;
			if (bunix_buffer_write(message->cap, 0, &info,
					       sizeof(info)) != 0) {
				reply->words[0] = (u64)-1;
				return;
			}
			reply->words[0] = 0;
			reply->words[1] = sizeof(info);
			return;
		}
		index--;
	}
	for (const struct icmp_socket *socket = icmp_sockets; socket != 0;
	     socket = socket->next) {
		if (index == 0) {
			struct bunix_net_socket_info info;

			if (message->cap == 0 ||
			    (message->cap_rights & BUNIX_RIGHT_SEND) == 0) {
				reply->words[0] = (u64)-1;
				return;
			}
			info.id = socket->id;
			info.protocol = socket->protocol;
			info.family = socket->family;
			info.state = 1;
			info.local_hi = 0;
			info.local_lo = 0;
			info.local_port = 0;
			info.peer_hi = 0;
			info.peer_lo = 0;
			info.peer_port = 0;
			info.rx_len = socket->rx_len;
			info.tx_len = 0;
			if (bunix_buffer_write(message->cap, 0, &info,
					       sizeof(info)) != 0) {
				reply->words[0] = (u64)-1;
				return;
			}
			reply->words[0] = 0;
			reply->words[1] = sizeof(info);
			return;
		}
		index--;
	}
	reply->words[0] = (u64)-1;
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

static u64 net_read_be16(const unsigned char *data)
{
	return ((u64)data[0] << 8) | data[1];
}

static u64 net_read_be32(const unsigned char *data)
{
	return ((u64)data[0] << 24) | ((u64)data[1] << 16) |
	       ((u64)data[2] << 8) | data[3];
}

static u64 net_read_mac48(const unsigned char *data)
{
	u64 mac = 0;

	for (u64 i = 0; i < 6; i++) {
		mac = (mac << 8) | data[i];
	}
	return mac;
}

static void net_write_be16(unsigned char *data, u64 value)
{
	data[0] = (unsigned char)((value >> 8) & 0xff);
	data[1] = (unsigned char)(value & 0xff);
}

static void net_write_be32(unsigned char *data, u64 value)
{
	data[0] = (unsigned char)((value >> 24) & 0xff);
	data[1] = (unsigned char)((value >> 16) & 0xff);
	data[2] = (unsigned char)((value >> 8) & 0xff);
	data[3] = (unsigned char)(value & 0xff);
}

static void net_write_be64(unsigned char *data, u64 value)
{
	for (u64 i = 0; i < 8; i++) {
		data[i] = (unsigned char)((value >> ((7 - i) * 8)) & 0xff);
	}
}

static u64 net_read_be64(const unsigned char *data)
{
	u64 value = 0;

	for (u64 i = 0; i < 8; i++) {
		value = (value << 8) | data[i];
	}
	return value;
}

static void neighbor_prune_expired(void)
{
	struct net_neighbor **link = &dynamic_neighbors;
	const u64 now = bunix_clock_monotonic_ns();

	while (*link != 0) {
		struct net_neighbor *neighbor = *link;

		if (neighbor->updated_ns != 0 &&
		    now - neighbor->updated_ns > NET_NEIGHBOR_TTL_NS) {
			*link = neighbor->next;
			bunix_free(neighbor);
			continue;
		}
		link = &neighbor->next;
	}
}

static struct net_neighbor *neighbor_find(u64 family, u64 hi, u64 lo,
					  u64 iface)
{
	neighbor_prune_expired();
	for (struct net_neighbor *neighbor = dynamic_neighbors; neighbor != 0;
	     neighbor = neighbor->next) {
		if (neighbor->family == family &&
		    neighbor->addr_hi == hi &&
		    neighbor->addr_lo == lo &&
		    neighbor->iface == iface) {
			return neighbor;
		}
	}
	return 0;
}

static int pending_frame_add(u64 family, u64 hi, u64 lo, u64 iface,
			     const unsigned char *data, u64 len)
{
	struct net_pending_frame *pending;

	if (data == 0 || len == 0 || len > NET_PACKET_MAX ||
	    !((family == BUNIX_NET_ADDR_FAMILY_IPV4 && hi == 0 && lo != 0) ||
	      (family == BUNIX_NET_ADDR_FAMILY_IPV6 && (hi != 0 || lo != 0)))) {
		return -1;
	}
	pending = (struct net_pending_frame *)bunix_alloc(sizeof(*pending) +
							  len);
	if (pending == 0) {
		return -1;
	}
	pending->next = pending_frames;
	pending->family = family;
	pending->addr_hi = hi;
	pending->addr_lo = lo;
	pending->iface = iface;
	pending->len = len;
	for (u64 i = 0; i < len; i++) {
		pending->data[i] = data[i];
	}
	pending_frames = pending;
	return 0;
}

static void pending_frames_flush(const struct net_neighbor *neighbor)
{
	struct net_pending_frame **link = &pending_frames;
	struct net_interface *iface;

	if (neighbor == 0 || neighbor->mac_hi == 0) {
		return;
	}
	iface = interface_find(neighbor->iface);
	if (iface == 0) {
		return;
	}
	while (*link != 0) {
		struct net_pending_frame *pending = *link;

		if (pending->family == neighbor->family &&
		    pending->addr_hi == neighbor->addr_hi &&
		    pending->addr_lo == neighbor->addr_lo &&
		    pending->iface == neighbor->iface &&
		    pending->len >= 6) {
			for (u64 i = 0; i < 6; i++) {
				pending->data[i] =
					(unsigned char)((neighbor->mac_hi >>
							 ((5 - i) * 8)) &
							0xff);
			}
			(void)packet_tx_enqueue_copy(iface, pending->data,
						     pending->len);
			*link = pending->next;
			bunix_free(pending);
			continue;
		}
		link = &pending->next;
	}
}

static void neighbor_learn(u64 family, u64 hi, u64 lo, u64 iface,
			   u64 mac_hi, u64 mac_lo)
{
	struct net_neighbor *neighbor;

	if (iface == NET_IFACE_LO || (hi == 0 && lo == 0) || mac_hi == 0) {
		return;
	}
	neighbor = neighbor_find(family, hi, lo, iface);
	if (neighbor == 0) {
		neighbor = (struct net_neighbor *)bunix_calloc(1,
							       sizeof(*neighbor));
		if (neighbor == 0) {
			return;
		}
		neighbor->next = dynamic_neighbors;
		dynamic_neighbors = neighbor;
	}
	neighbor->family = family;
	neighbor->addr_hi = hi;
	neighbor->addr_lo = lo;
	neighbor->iface = iface;
	neighbor->mac_hi = mac_hi;
	neighbor->mac_lo = mac_lo;
	neighbor->flags = 2;
	neighbor->state = 1;
	neighbor->updated_ns = bunix_clock_monotonic_ns();
	pending_frames_flush(neighbor);
}

static void packet_ingress_learn_ipv4_neighbor(const struct net_interface *iface,
					       const unsigned char *packet,
					       u64 len)
{
	const unsigned char *ip;
	u64 ihl;
	u64 src_ip;
	u64 src_mac;

	if (iface == 0 || packet == 0 || len < 14 + 20 ||
	    net_read_be16(packet + 12) != 0x0800) {
		return;
	}
	ip = packet + 14;
	if ((ip[0] >> 4) != 4) {
		return;
	}
	ihl = (ip[0] & 0x0f) * 4;
	if (ihl < 20 || len < 14 + ihl) {
		return;
	}
	src_ip = net_read_be32(ip + 12);
	src_mac = net_read_mac48(packet + 6);
	neighbor_learn(BUNIX_NET_ADDR_FAMILY_IPV4, 0, src_ip, iface->id,
		       src_mac, 0);
}

static void packet_ingress_learn_ipv6_neighbor(const struct net_interface *iface,
					       const unsigned char *packet,
					       u64 len)
{
	const unsigned char *ip;
	u64 src_hi;
	u64 src_lo;
	u64 src_mac;

	if (iface == 0 || packet == 0 || len < 14 + 40 ||
	    net_read_be16(packet + 12) != NET_ETH_IPV6) {
		return;
	}
	ip = packet + 14;
	if ((ip[0] >> 4) != 6) {
		return;
	}
	src_hi = net_read_be64(ip + 8);
	src_lo = net_read_be64(ip + 16);
	if (src_hi == 0 && src_lo == 0) {
		return;
	}
	src_mac = net_read_mac48(packet + 6);
	neighbor_learn(BUNIX_NET_ADDR_FAMILY_IPV6, src_hi, src_lo, iface->id,
		       src_mac, 0);
}

static void packet_ingress_arp_ipv4(struct net_interface *iface,
				    const unsigned char *packet, u64 len)
{
	const unsigned char *arp;
	unsigned char reply[42];
	u64 opcode;
	u64 sender_mac;
	u64 sender_ip;
	u64 target_ip;
	u64 local_ip;

	if (iface == 0 || packet == 0 || len < sizeof(reply) ||
	    net_read_be16(packet + 12) != 0x0806) {
		return;
	}
	arp = packet + 14;
	if (net_read_be16(arp) != 1 || net_read_be16(arp + 2) != 0x0800 ||
	    arp[4] != 6 || arp[5] != 4) {
		return;
	}
	opcode = net_read_be16(arp + 6);
	if (opcode != 1 && opcode != 2) {
		return;
	}
	sender_mac = net_read_mac48(arp + 8);
	sender_ip = net_read_be32(arp + 14);
	target_ip = net_read_be32(arp + 24);
	neighbor_learn(BUNIX_NET_ADDR_FAMILY_IPV4, 0, sender_ip, iface->id,
		       sender_mac, 0);
	if (opcode != 1) {
		return;
	}
	local_ip = iface_ipv4_addr(iface->id);
	if (local_ip == 0 || target_ip != local_ip || sender_mac == 0 ||
	    sender_ip == 0) {
		return;
	}
	for (u64 i = 0; i < 6; i++) {
		reply[i] = (unsigned char)((sender_mac >> ((5 - i) * 8)) &
					   0xff);
		reply[6 + i] = (unsigned char)((iface->mac_hi >>
						((5 - i) * 8)) & 0xff);
	}
	reply[12] = 0x08;
	reply[13] = 0x06;
	net_write_be16(reply + 14, 1);
	net_write_be16(reply + 16, 0x0800);
	reply[18] = 6;
	reply[19] = 4;
	net_write_be16(reply + 20, 2);
	for (u64 i = 0; i < 6; i++) {
		reply[22 + i] = (unsigned char)((iface->mac_hi >>
						 ((5 - i) * 8)) & 0xff);
		reply[32 + i] = (unsigned char)((sender_mac >>
						 ((5 - i) * 8)) & 0xff);
	}
	net_write_be32(reply + 28, local_ip);
	net_write_be32(reply + 38, sender_ip);
	(void)packet_tx_enqueue_copy(iface, reply, sizeof(reply));
}

static void arp_request_ipv4(struct net_interface *iface, u64 source_ip,
			     u64 target_ip)
{
	unsigned char frame[42];

	if (iface == 0 || iface == &loopback || source_ip == 0 ||
	    target_ip == 0 || iface->mac_hi == 0) {
		return;
	}
	for (u64 i = 0; i < 6; i++) {
		frame[i] = 0xff;
		frame[6 + i] = (unsigned char)((iface->mac_hi >>
						((5 - i) * 8)) & 0xff);
	}
	frame[12] = 0x08;
	frame[13] = 0x06;
	net_write_be16(frame + 14, 1);
	net_write_be16(frame + 16, 0x0800);
	frame[18] = 6;
	frame[19] = 4;
	net_write_be16(frame + 20, 1);
	for (u64 i = 0; i < 6; i++) {
		frame[22 + i] = (unsigned char)((iface->mac_hi >>
						 ((5 - i) * 8)) & 0xff);
		frame[32 + i] = 0;
	}
	net_write_be32(frame + 28, source_ip);
	net_write_be32(frame + 38, target_ip);
	(void)packet_tx_enqueue_copy(iface, frame, sizeof(frame));
}

static void ndp_write_ipv6_header(unsigned char *frame, u64 payload_len,
				  u64 next_header, u64 hop_limit,
				  u64 source_hi, u64 source_lo,
				  u64 dest_hi, u64 dest_lo)
{
	frame[14] = 0x60;
	frame[15] = 0;
	frame[16] = 0;
	frame[17] = 0;
	net_write_be16(frame + 18, payload_len);
	frame[20] = (unsigned char)next_header;
	frame[21] = (unsigned char)hop_limit;
	net_write_be64(frame + 22, source_hi);
	net_write_be64(frame + 30, source_lo);
	net_write_be64(frame + 38, dest_hi);
	net_write_be64(frame + 46, dest_lo);
}

static void ipv6_multicast_mac(u64 group_lo, unsigned char *mac)
{
	mac[0] = 0x33;
	mac[1] = 0x33;
	mac[2] = (unsigned char)((group_lo >> 24) & 0xff);
	mac[3] = (unsigned char)((group_lo >> 16) & 0xff);
	mac[4] = (unsigned char)((group_lo >> 8) & 0xff);
	mac[5] = (unsigned char)(group_lo & 0xff);
}

static void ndp_solicited_node_addr(u64 target_lo, u64 *hi, u64 *lo)
{
	*hi = 0xff02000000000000ull;
	*lo = 0x00000001ff000000ull | (target_lo & 0xffffffull);
}

static void ndp_request_ipv6(struct net_interface *iface, u64 source_hi,
			     u64 source_lo, u64 target_hi, u64 target_lo)
{
	unsigned char frame[86];
	unsigned char *icmp = frame + 54;
	u64 dest_hi;
	u64 dest_lo;
	unsigned short checksum;

	if (iface == 0 || iface == &loopback || iface->mac_hi == 0 ||
	    (source_hi == 0 && source_lo == 0) ||
	    (target_hi == 0 && target_lo == 0)) {
		return;
	}
	ndp_solicited_node_addr(target_lo, &dest_hi, &dest_lo);
	ipv6_multicast_mac(dest_lo, frame);
	for (u64 i = 0; i < 6; i++) {
		frame[6 + i] = (unsigned char)((iface->mac_hi >>
						((5 - i) * 8)) & 0xff);
	}
	frame[12] = 0x86;
	frame[13] = 0xdd;
	ndp_write_ipv6_header(frame, 32, NET_PROTO_ICMPV6, 255, source_hi,
			      source_lo, dest_hi, dest_lo);
	for (u64 i = 0; i < 32; i++) {
		icmp[i] = 0;
	}
	icmp[0] = NET_ICMPV6_NEIGHBOR_SOLICIT;
	net_write_be64(icmp + 8, target_hi);
	net_write_be64(icmp + 16, target_lo);
	icmp[24] = 1;
	icmp[25] = 1;
	for (u64 i = 0; i < 6; i++) {
		icmp[26 + i] = (unsigned char)((iface->mac_hi >>
						((5 - i) * 8)) & 0xff);
	}
	checksum = icmp_ipv6_checksum(icmp, 32, source_hi, source_lo, dest_hi,
				      dest_lo);
	icmp[2] = (unsigned char)((checksum >> 8) & 0xff);
	icmp[3] = (unsigned char)(checksum & 0xff);
	(void)packet_tx_enqueue_copy(iface, frame, sizeof(frame));
}

static void ndp_advertise_ipv6(struct net_interface *iface, u64 dest_mac,
			       u64 dest_hi, u64 dest_lo, u64 target_hi,
			       u64 target_lo)
{
	unsigned char frame[86];
	unsigned char *icmp = frame + 54;
	unsigned short checksum;

	if (iface == 0 || iface == &loopback || iface->mac_hi == 0 ||
	    dest_mac == 0 || (dest_hi == 0 && dest_lo == 0) ||
	    (target_hi == 0 && target_lo == 0)) {
		return;
	}
	for (u64 i = 0; i < 6; i++) {
		frame[i] = (unsigned char)((dest_mac >> ((5 - i) * 8)) &
					   0xff);
		frame[6 + i] = (unsigned char)((iface->mac_hi >>
						((5 - i) * 8)) & 0xff);
	}
	frame[12] = 0x86;
	frame[13] = 0xdd;
	ndp_write_ipv6_header(frame, 32, NET_PROTO_ICMPV6, 255, target_hi,
			      target_lo, dest_hi, dest_lo);
	for (u64 i = 0; i < 32; i++) {
		icmp[i] = 0;
	}
	icmp[0] = NET_ICMPV6_NEIGHBOR_ADVERT;
	icmp[4] = 0x60;
	net_write_be64(icmp + 8, target_hi);
	net_write_be64(icmp + 16, target_lo);
	icmp[24] = 2;
	icmp[25] = 1;
	for (u64 i = 0; i < 6; i++) {
		icmp[26 + i] = (unsigned char)((iface->mac_hi >>
						((5 - i) * 8)) & 0xff);
	}
	checksum = icmp_ipv6_checksum(icmp, 32, target_hi, target_lo, dest_hi,
				      dest_lo);
	icmp[2] = (unsigned char)((checksum >> 8) & 0xff);
	icmp[3] = (unsigned char)(checksum & 0xff);
	(void)packet_tx_enqueue_copy(iface, frame, sizeof(frame));
}

static void packet_ingress_ndp_ipv6(struct net_interface *iface,
				    const unsigned char *packet, u64 len)
{
	const unsigned char *ip;
	const unsigned char *icmp;
	u64 payload_len;
	u64 src_hi;
	u64 src_lo;
	u64 target_hi;
	u64 target_lo;
	u64 src_mac;

	if (iface == 0 || packet == 0 || len < 14 + 40 + 24 ||
	    net_read_be16(packet + 12) != NET_ETH_IPV6) {
		return;
	}
	ip = packet + 14;
	if ((ip[0] >> 4) != 6 || ip[6] != NET_PROTO_ICMPV6) {
		return;
	}
	payload_len = net_read_be16(ip + 4);
	if (payload_len < 24 || len < 14 + 40 + payload_len) {
		return;
	}
	icmp = ip + 40;
	src_hi = net_read_be64(ip + 8);
	src_lo = net_read_be64(ip + 16);
	src_mac = net_read_mac48(packet + 6);
	if (icmp[0] == NET_ICMPV6_NEIGHBOR_SOLICIT) {
		target_hi = net_read_be64(icmp + 8);
		target_lo = net_read_be64(icmp + 16);
		if (payload_len >= 32 && icmp[24] == 1 && icmp[25] == 1) {
			src_mac = net_read_mac48(icmp + 26);
		}
		if ((src_hi != 0 || src_lo != 0) && src_mac != 0) {
			neighbor_learn(BUNIX_NET_ADDR_FAMILY_IPV6, src_hi,
				       src_lo, iface->id, src_mac, 0);
		}
		if (net_addr_is_local(BUNIX_NET_ADDR_FAMILY_IPV6, target_hi,
				      target_lo) &&
		    (src_hi != 0 || src_lo != 0) && src_mac != 0) {
			ndp_advertise_ipv6(iface, src_mac, src_hi, src_lo,
					   target_hi, target_lo);
		}
		return;
	}
	if (icmp[0] == NET_ICMPV6_NEIGHBOR_ADVERT) {
		target_hi = net_read_be64(icmp + 8);
		target_lo = net_read_be64(icmp + 16);
		if (payload_len >= 32 && icmp[24] == 2 && icmp[25] == 1) {
			src_mac = net_read_mac48(icmp + 26);
		}
		if (src_mac != 0) {
			neighbor_learn(BUNIX_NET_ADDR_FAMILY_IPV6, target_hi,
				       target_lo, iface->id, src_mac, 0);
		}
	}
}

static int icmp_socket_accepts_ipv6(struct icmp_socket *socket)
{
	return socket != 0 &&
	       socket->family == BUNIX_NET_ADDR_FAMILY_IPV6 &&
	       socket->protocol == NET_PROTO_ICMPV6;
}

static void packet_ingress_icmp_ipv6(const unsigned char *packet, u64 len)
{
	const unsigned char *ip;
	const unsigned char *icmp;
	u64 payload_len;
	u64 src_hi;
	u64 src_lo;
	u64 dst_hi;
	u64 dst_lo;

	if (packet == 0 || len < 14 + 40 + 8 ||
	    net_read_be16(packet + 12) != NET_ETH_IPV6) {
		return;
	}
	ip = packet + 14;
	if ((ip[0] >> 4) != 6 || ip[6] != NET_PROTO_ICMPV6) {
		return;
	}
	payload_len = net_read_be16(ip + 4);
	if (payload_len < 8 || len < 14 + 40 + payload_len) {
		return;
	}
	icmp = ip + 40;
	if (icmp[0] == NET_ICMPV6_NEIGHBOR_SOLICIT ||
	    icmp[0] == NET_ICMPV6_NEIGHBOR_ADVERT ||
	    icmp[0] == NET_ICMPV6_ROUTER_SOLICIT ||
	    icmp[0] == NET_ICMPV6_ROUTER_ADVERT) {
		return;
	}
	src_hi = net_read_be64(ip + 8);
	src_lo = net_read_be64(ip + 16);
	dst_hi = net_read_be64(ip + 24);
	dst_lo = net_read_be64(ip + 32);
	if (!net_addr_is_local(BUNIX_NET_ADDR_FAMILY_IPV6, dst_hi, dst_lo)) {
		return;
	}
	for (struct icmp_socket *socket = icmp_sockets; socket != 0;
	     socket = socket->next) {
		if (icmp_socket_accepts_ipv6(socket)) {
			(void)icmp_enqueue(socket, BUNIX_NET_ADDR_FAMILY_IPV6,
					   src_hi, src_lo, icmp, payload_len);
		}
	}
}

static int udp_socket_accepts_ipv4(struct udp_socket *socket, u64 dst_ip,
				   u64 dst_port)
{
	if (socket == 0 || !socket->bound ||
	    socket->family != BUNIX_NET_ADDR_FAMILY_IPV4 ||
	    socket->local.port != dst_port) {
		return 0;
	}
	return (socket->local.hi == 0 && socket->local.lo == 0) ||
	       socket->local.lo == dst_ip ||
	       dst_ip == 0xffffffffu;
}

static int udp_sendto_external_ipv4(struct udp_socket *socket,
				    const struct net_addr *dest,
				    const unsigned char *data, u64 len)
{
	const struct net_route *route;
	struct net_interface *iface;
	unsigned char *frame;
	u64 source_ip;
	u64 next_hop_ip;
	u64 dest_mac = 0;
	u64 udp_len;
	u64 ip_len;
	u64 frame_len;
	unsigned short ip_checksum;

	if (socket == 0 || dest == 0 || data == 0 ||
	    socket->family != BUNIX_NET_ADDR_FAMILY_IPV4 ||
	    dest->family != BUNIX_NET_ADDR_FAMILY_IPV4 ||
	    dest->hi != 0 || dest->lo == 0 || dest->port == 0 ||
	    dest->port > 65535 || len > NET_PACKET_MAX) {
		return -1;
	}
	route = net_route_lookup(BUNIX_NET_ADDR_FAMILY_IPV4, 0, dest->lo);
	if (route == 0 || (route->flags & BUNIX_NET_ROUTE_FLAG_UP) == 0) {
		return -1;
	}
	iface = interface_find(route->iface);
	source_ip = iface_ipv4_addr(route->iface);
	next_hop_ip = route->gateway_lo != 0 ? route->gateway_lo : dest->lo;
	udp_len = 8 + len;
	ip_len = 20 + udp_len;
	frame_len = 14 + ip_len;
	if (iface == 0 || source_ip == 0 || udp_len > 0xffff ||
	    ip_len > 0xffff || frame_len > NET_PACKET_MAX) {
		return -1;
	}
	{
		struct net_neighbor *neighbor =
			neighbor_find(BUNIX_NET_ADDR_FAMILY_IPV4, 0,
				      next_hop_ip, route->iface);

		if (neighbor != 0) {
			dest_mac = neighbor->mac_hi;
		}
	}
	if (!socket->bound && udp_bind_socket(socket, 0, 0, 0) != 0) {
		return -1;
	}
	frame = (unsigned char *)bunix_alloc(frame_len);
	if (frame == 0) {
		return -1;
	}
	for (u64 i = 0; i < 6; i++) {
		frame[i] = dest_mac != 0 ?
			   (unsigned char)((dest_mac >> ((5 - i) * 8)) & 0xff) :
			   0xff;
		frame[6 + i] = (unsigned char)((iface->mac_hi >>
						((5 - i) * 8)) & 0xff);
	}
	frame[12] = 0x08;
	frame[13] = 0x00;
	frame[14] = 0x45;
	frame[15] = 0;
	net_write_be16(frame + 16, ip_len);
	net_write_be16(frame + 18, 0);
	net_write_be16(frame + 20, 0);
	frame[22] = 64;
	frame[23] = NET_PROTO_UDP;
	frame[24] = 0;
	frame[25] = 0;
	net_write_be32(frame + 26, source_ip);
	net_write_be32(frame + 30, dest->lo);
	net_write_be16(frame + 34, socket->local.port);
	net_write_be16(frame + 36, dest->port);
	net_write_be16(frame + 38, udp_len);
	net_write_be16(frame + 40, 0);
	for (u64 i = 0; i < len; i++) {
		frame[42 + i] = data[i];
	}
	ip_checksum = net_checksum_finish(net_checksum_sum(frame + 14, 20, 0));
	frame[24] = (unsigned char)((ip_checksum >> 8) & 0xff);
	frame[25] = (unsigned char)(ip_checksum & 0xff);
	if (dest_mac == 0) {
		if (pending_frame_add(BUNIX_NET_ADDR_FAMILY_IPV4, 0,
				      next_hop_ip, route->iface, frame,
				      frame_len) != 0) {
			bunix_free(frame);
			return -1;
		}
		arp_request_ipv4(iface, source_ip, next_hop_ip);
		bunix_free(frame);
		return 0;
	}
	if (packet_tx_enqueue_copy(iface, frame, frame_len) != 0) {
		bunix_free(frame);
		return -1;
	}
	bunix_free(frame);
	return 0;
}

static int udp_sendto_external_ipv6(struct udp_socket *socket,
				    const struct net_addr *dest,
				    const unsigned char *data, u64 len)
{
	const struct net_route *route;
	struct net_interface *iface;
	unsigned char *frame;
	u64 source_hi = 0;
	u64 source_lo = 0;
	u64 next_hop_hi;
	u64 next_hop_lo;
	u64 dest_mac = 0;
	u64 udp_len;
	u64 frame_len;
	unsigned short udp_checksum;

	if (socket == 0 || dest == 0 || data == 0 ||
	    socket->family != BUNIX_NET_ADDR_FAMILY_IPV6 ||
	    dest->family != BUNIX_NET_ADDR_FAMILY_IPV6 ||
	    (dest->hi == 0 && dest->lo == 0) || dest->port == 0 ||
	    dest->port > 65535 || len > NET_PACKET_MAX) {
		return -1;
	}
	route = net_route_lookup(BUNIX_NET_ADDR_FAMILY_IPV6, dest->hi,
				 dest->lo);
	if (route == 0 || (route->flags & BUNIX_NET_ROUTE_FLAG_UP) == 0) {
		return -1;
	}
	iface = interface_find(route->iface);
	next_hop_hi = (route->flags & BUNIX_NET_ROUTE_FLAG_GATEWAY) != 0 ?
		      route->gateway_hi : dest->hi;
	next_hop_lo = (route->flags & BUNIX_NET_ROUTE_FLAG_GATEWAY) != 0 ?
		      route->gateway_lo : dest->lo;
	udp_len = 8 + len;
	frame_len = 14 + 40 + udp_len;
	if (iface == 0 || iface_ipv6_addr(route->iface, &source_hi,
					  &source_lo) != 0 ||
	    iface->mac_hi == 0 || udp_len > 0xffff ||
	    frame_len > NET_PACKET_MAX ||
	    (next_hop_hi == 0 && next_hop_lo == 0)) {
		return -1;
	}
	{
		struct net_neighbor *neighbor =
			neighbor_find(BUNIX_NET_ADDR_FAMILY_IPV6, next_hop_hi,
				      next_hop_lo, route->iface);

		if (neighbor != 0) {
			dest_mac = neighbor->mac_hi;
		}
	}
	if (!socket->bound && udp_bind_socket(socket, 0, 0, 0) != 0) {
		return -1;
	}
	frame = (unsigned char *)bunix_alloc(frame_len);
	if (frame == 0) {
		return -1;
	}
	for (u64 i = 0; i < 6; i++) {
		frame[i] = dest_mac != 0 ?
			   (unsigned char)((dest_mac >> ((5 - i) * 8)) & 0xff) :
			   0xff;
		frame[6 + i] = (unsigned char)((iface->mac_hi >>
						((5 - i) * 8)) & 0xff);
	}
	frame[12] = 0x86;
	frame[13] = 0xdd;
	ndp_write_ipv6_header(frame, udp_len, NET_PROTO_UDP, 64, source_hi,
			      source_lo, dest->hi, dest->lo);
	net_write_be16(frame + 54, socket->local.port);
	net_write_be16(frame + 56, dest->port);
	net_write_be16(frame + 58, udp_len);
	net_write_be16(frame + 60, 0);
	for (u64 i = 0; i < len; i++) {
		frame[62 + i] = data[i];
	}
	udp_checksum = ipv6_l4_checksum(frame + 54, udp_len, NET_PROTO_UDP,
					source_hi, source_lo, dest->hi,
					dest->lo);
	if (udp_checksum == 0) {
		udp_checksum = 0xffff;
	}
	frame[60] = (unsigned char)((udp_checksum >> 8) & 0xff);
	frame[61] = (unsigned char)(udp_checksum & 0xff);
	if (dest_mac == 0) {
		if (pending_frame_add(BUNIX_NET_ADDR_FAMILY_IPV6, next_hop_hi,
				      next_hop_lo, route->iface, frame,
				      frame_len) != 0) {
			bunix_free(frame);
			return -1;
		}
		ndp_request_ipv6(iface, source_hi, source_lo, next_hop_hi,
				 next_hop_lo);
		bunix_free(frame);
		return 0;
	}
	if (packet_tx_enqueue_copy(iface, frame, frame_len) != 0) {
		bunix_free(frame);
		return -1;
	}
	bunix_free(frame);
	return 0;
}

static void udp_enqueue_external_ipv4(struct udp_socket *socket, u64 src_ip,
				      u64 src_port, const unsigned char *data,
				      u64 len)
{
	struct udp_datagram *datagram;

	if (socket == 0 || len > NET_PACKET_MAX) {
		return;
	}
	datagram = (struct udp_datagram *)bunix_alloc(sizeof(*datagram) + len);
	if (datagram == 0) {
		return;
	}
	datagram->next = 0;
	datagram->source.family = BUNIX_NET_ADDR_FAMILY_IPV4;
	datagram->source.hi = 0;
	datagram->source.lo = src_ip;
	datagram->source.port = src_port;
	datagram->checksum = 0;
	datagram->len = len;
	for (u64 i = 0; i < len; i++) {
		datagram->data[i] = data[i];
	}
	if (socket->rx_tail != 0) {
		socket->rx_tail->next = datagram;
	} else {
		socket->rx_head = datagram;
	}
	socket->rx_tail = datagram;
	socket->rx_len++;
}

static void packet_ingress_udp_ipv4(const unsigned char *packet, u64 len)
{
	const unsigned char *ip;
	const unsigned char *udp;
	u64 ihl;
	u64 total_len;
	u64 fragment;
	u64 udp_len;
	u64 src_ip;
	u64 dst_ip;
	u64 src_port;
	u64 dst_port;

	if (packet == 0 || len < 14 + 20 ||
	    net_read_be16(packet + 12) != 0x0800) {
		return;
	}
	ip = packet + 14;
	if ((ip[0] >> 4) != 4 || ip[9] != NET_PROTO_UDP) {
		return;
	}
	ihl = (ip[0] & 0x0f) * 4;
	if (ihl < 20 || len < 14 + ihl + 8) {
		return;
	}
	total_len = net_read_be16(ip + 2);
	if (total_len < ihl + 8 || len < 14 + total_len) {
		return;
	}
	fragment = net_read_be16(ip + 6);
	if ((fragment & 0x3fff) != 0) {
		return;
	}
	udp = ip + ihl;
	udp_len = net_read_be16(udp + 4);
	if (udp_len < 8 || udp_len > total_len - ihl) {
		return;
	}
	src_ip = net_read_be32(ip + 12);
	dst_ip = net_read_be32(ip + 16);
	src_port = net_read_be16(udp);
	dst_port = net_read_be16(udp + 2);
	for (struct udp_socket *socket = udp_sockets; socket != 0;
	     socket = socket->next) {
		if (udp_socket_accepts_ipv4(socket, dst_ip, dst_port)) {
			udp_enqueue_external_ipv4(socket, src_ip, src_port,
						  udp + 8, udp_len - 8);
		}
	}
}

static int udp_socket_accepts_ipv6(struct udp_socket *socket, u64 dst_hi,
				   u64 dst_lo, u64 dst_port)
{
	if (socket == 0 || !socket->bound ||
	    socket->family != BUNIX_NET_ADDR_FAMILY_IPV6 ||
	    socket->local.port != dst_port) {
		return 0;
	}
	return (socket->local.hi == 0 && socket->local.lo == 0) ||
	       (socket->local.hi == dst_hi && socket->local.lo == dst_lo);
}

static void udp_enqueue_external_ipv6(struct udp_socket *socket, u64 src_hi,
				      u64 src_lo, u64 src_port,
				      const unsigned char *data, u64 len)
{
	struct udp_datagram *datagram;

	if (socket == 0 || len > NET_PACKET_MAX) {
		return;
	}
	datagram = (struct udp_datagram *)bunix_alloc(sizeof(*datagram) + len);
	if (datagram == 0) {
		return;
	}
	datagram->next = 0;
	datagram->source.family = BUNIX_NET_ADDR_FAMILY_IPV6;
	datagram->source.hi = src_hi;
	datagram->source.lo = src_lo;
	datagram->source.port = src_port;
	datagram->checksum = 0;
	datagram->len = len;
	for (u64 i = 0; i < len; i++) {
		datagram->data[i] = data[i];
	}
	if (socket->rx_tail != 0) {
		socket->rx_tail->next = datagram;
	} else {
		socket->rx_head = datagram;
	}
	socket->rx_tail = datagram;
	socket->rx_len++;
}

static void packet_ingress_udp_ipv6(const unsigned char *packet, u64 len)
{
	const unsigned char *ip;
	const unsigned char *udp;
	u64 payload_len;
	u64 udp_len;
	u64 src_hi;
	u64 src_lo;
	u64 dst_hi;
	u64 dst_lo;
	u64 src_port;
	u64 dst_port;

	if (packet == 0 || len < 14 + 40 + 8 ||
	    net_read_be16(packet + 12) != NET_ETH_IPV6) {
		return;
	}
	ip = packet + 14;
	if ((ip[0] >> 4) != 6 || ip[6] != NET_PROTO_UDP) {
		return;
	}
	payload_len = net_read_be16(ip + 4);
	if (payload_len < 8 || len < 14 + 40 + payload_len) {
		return;
	}
	udp = ip + 40;
	udp_len = net_read_be16(udp + 4);
	if (udp_len < 8 || udp_len > payload_len) {
		return;
	}
	src_hi = net_read_be64(ip + 8);
	src_lo = net_read_be64(ip + 16);
	dst_hi = net_read_be64(ip + 24);
	dst_lo = net_read_be64(ip + 32);
	if (!net_addr_is_local(BUNIX_NET_ADDR_FAMILY_IPV6, dst_hi, dst_lo)) {
		return;
	}
	src_port = net_read_be16(udp);
	dst_port = net_read_be16(udp + 2);
	for (struct udp_socket *socket = udp_sockets; socket != 0;
	     socket = socket->next) {
		if (udp_socket_accepts_ipv6(socket, dst_hi, dst_lo,
					    dst_port)) {
			udp_enqueue_external_ipv6(socket, src_hi, src_lo,
						  src_port, udp + 8,
						  udp_len - 8);
		}
	}
}

static int icmp_socket_accepts_ipv4(struct icmp_socket *socket)
{
	return socket != 0 &&
	       socket->family == BUNIX_NET_ADDR_FAMILY_IPV4 &&
	       socket->protocol == NET_PROTO_ICMP;
}

static void packet_ingress_icmp_ipv4(const unsigned char *packet, u64 len)
{
	const unsigned char *ip;
	u64 ihl;
	u64 total_len;
	u64 fragment;
	u64 src_ip;
	u64 dst_ip;

	if (packet == 0 || len < 14 + 20 ||
	    net_read_be16(packet + 12) != 0x0800) {
		return;
	}
	ip = packet + 14;
	if ((ip[0] >> 4) != 4 || ip[9] != NET_PROTO_ICMP) {
		return;
	}
	ihl = (ip[0] & 0x0f) * 4;
	if (ihl < 20 || len < 14 + ihl + 8) {
		return;
	}
	total_len = net_read_be16(ip + 2);
	if (total_len < ihl + 8 || len < 14 + total_len) {
		return;
	}
	fragment = net_read_be16(ip + 6);
	if ((fragment & 0x3fff) != 0) {
		return;
	}
	src_ip = net_read_be32(ip + 12);
	dst_ip = net_read_be32(ip + 16);
	if (!net_addr_is_local(BUNIX_NET_ADDR_FAMILY_IPV4, 0, dst_ip)) {
		return;
	}
	for (struct icmp_socket *socket = icmp_sockets; socket != 0;
	     socket = socket->next) {
		if (icmp_socket_accepts_ipv4(socket)) {
			(void)icmp_enqueue(socket, BUNIX_NET_ADDR_FAMILY_IPV4,
					   0, src_ip, ip + ihl,
					   total_len - ihl);
		}
	}
}

static void reply_packet_rx_submit(struct bunix_msg *reply,
				   const struct bunix_msg *message)
{
	struct bunix_net_packet_info info;
	struct net_interface *iface;
	struct net_packet *packet;

	if (message->cap == 0 ||
	    (message->cap_rights & BUNIX_RIGHT_RECV) == 0 ||
	    bunix_buffer_read(message->cap, 0, &info, sizeof(info)) != 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	iface = interface_find(info.iface);
	if (iface == 0 || iface == &loopback || info.len == 0 ||
	    info.len > iface->mtu ||
	    (iface->flags & BUNIX_NET_IFACE_FLAG_RUNNING) == 0) {
		if (iface != 0 && iface != &loopback) {
			iface->rx_drops++;
		}
		reply->words[0] = (u64)-1;
		return;
	}
	packet = (struct net_packet *)bunix_alloc(sizeof(*packet) + info.len);
	if (packet == 0) {
		iface->rx_drops++;
		reply->words[0] = (u64)-1;
		return;
	}
	packet->next = 0;
	packet->family = 0;
	packet->len = info.len;
	if (bunix_buffer_read(message->cap, sizeof(info), packet->data,
			      info.len) != 0) {
		bunix_free(packet);
		iface->rx_drops++;
		reply->words[0] = (u64)-1;
		return;
	}
	packet_ingress_arp_ipv4(iface, packet->data, packet->len);
	packet_ingress_learn_ipv4_neighbor(iface, packet->data, packet->len);
	packet_ingress_learn_ipv6_neighbor(iface, packet->data, packet->len);
	packet_ingress_ndp_ipv6(iface, packet->data, packet->len);
	packet_ingress_udp_ipv4(packet->data, packet->len);
	packet_ingress_udp_ipv6(packet->data, packet->len);
	packet_ingress_tcp_ipv4(iface, packet->data, packet->len);
	packet_ingress_tcp_ipv6(iface, packet->data, packet->len);
	packet_ingress_icmp_ipv4(packet->data, packet->len);
	packet_ingress_icmp_ipv6(packet->data, packet->len);
	if (iface->rx_tail != 0) {
		iface->rx_tail->next = packet;
	} else {
		iface->rx_head = packet;
	}
	iface->rx_tail = packet;
	iface->rx_packets++;
	reply->words[0] = 0;
	reply->words[1] = info.iface;
	reply->words[2] = info.len;
	reply->words[3] = 0;
}

static int packet_matches_ethertype(const struct net_packet *packet,
				    u64 ethertype)
{
	if (ethertype == 0) {
		return 1;
	}
	if (packet == 0 || packet->len < 14) {
		return 0;
	}
	return ((((u64)packet->data[12] << 8) | packet->data[13]) ==
		ethertype);
}

static void reply_packet_rx_poll(struct bunix_msg *reply,
				 const struct bunix_msg *message)
{
	struct net_interface *iface = interface_find(message->words[0]);
	const u64 ethertype = message->words[1];

	reply->words[0] = 0;
	reply->words[1] = 2;
	if (iface == 0 || iface == &loopback ||
	    (iface->flags & BUNIX_NET_IFACE_FLAG_RUNNING) == 0) {
		reply->words[0] = (u64)-1;
		reply->words[1] = 0;
		return;
	}
	for (struct net_packet *packet = iface->rx_head; packet != 0;
	     packet = packet->next) {
		if (packet_matches_ethertype(packet, ethertype)) {
			reply->words[1] |= 1;
			return;
		}
	}
}

static void reply_packet_rx_dequeue(struct bunix_msg *reply,
				    const struct bunix_msg *message)
{
	struct net_interface *iface = interface_find(message->words[0]);
	const u64 max_len = message->words[1];
	const u64 ethertype = message->words[2];
	const u64 strip_len = message->words[3] & 0xffffffffu;
	const u64 sockaddr_len = message->words[3] >> 32;
	struct net_packet *prev = 0;
	struct net_packet *packet;
	u64 payload_len;
	u64 copy_len;

	if (iface == 0 || iface == &loopback ||
	    (iface->flags & BUNIX_NET_IFACE_FLAG_RUNNING) == 0 ||
	    max_len == 0 || message->cap == 0 ||
	    (message->cap_rights & BUNIX_RIGHT_SEND) == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	for (packet = iface->rx_head; packet != 0; packet = packet->next) {
		if (packet_matches_ethertype(packet, ethertype)) {
			break;
		}
		prev = packet;
	}
	if (packet == 0) {
		reply->words[0] = (u64)-1;
		reply->words[1] = iface->id;
		reply->words[2] = 0;
		reply->words[3] = 0;
		return;
	}
	if (strip_len > packet->len) {
		reply->words[0] = (u64)-1;
		reply->words[1] = iface->id;
		reply->words[2] = 0;
		reply->words[3] = packet->len;
		return;
	}
	payload_len = packet->len - strip_len;
	copy_len = payload_len < max_len ? payload_len : max_len;
	if (bunix_buffer_write(message->cap, 0, packet->data + strip_len,
			       copy_len) != 0) {
		reply->words[0] = (u64)-1;
		reply->words[1] = iface->id;
		reply->words[2] = 0;
		reply->words[3] = payload_len;
		return;
	}
	if (sockaddr_len != 0) {
		unsigned char sockaddr[NET_PACKET_SOCKADDR_LL_SIZE];
		u64 copy = sockaddr_len < sizeof(sockaddr) ?
			   sockaddr_len : sizeof(sockaddr);

		for (u64 i = 0; i < sizeof(sockaddr); i++) {
			sockaddr[i] = 0;
		}
		sockaddr[0] = NET_PACKET_AF_PACKET;
		sockaddr[1] = 0;
		sockaddr[2] = packet->len >= 14 ? packet->data[12] : 0;
		sockaddr[3] = packet->len >= 14 ? packet->data[13] : 0;
		sockaddr[4] = (unsigned char)(iface->id & 0xff);
		sockaddr[5] = (unsigned char)((iface->id >> 8) & 0xff);
		sockaddr[6] = (unsigned char)((iface->id >> 16) & 0xff);
		sockaddr[7] = (unsigned char)((iface->id >> 24) & 0xff);
		sockaddr[8] = NET_PACKET_ARPHRD_ETHER;
		sockaddr[9] = 0;
		sockaddr[10] = packet->len >= 6 &&
			       packet->data[0] == 0xff &&
			       packet->data[1] == 0xff &&
			       packet->data[2] == 0xff &&
			       packet->data[3] == 0xff &&
			       packet->data[4] == 0xff &&
			       packet->data[5] == 0xff ?
			       NET_PACKET_BROADCAST : NET_PACKET_HOST;
		sockaddr[11] = 6;
		for (u64 i = 0; i < 6 && packet->len >= 12; i++) {
			sockaddr[12 + i] = packet->data[6 + i];
		}
		if (copy != 0 &&
		    bunix_buffer_write(message->cap, max_len, sockaddr,
				       copy) != 0) {
			reply->words[0] = (u64)-1;
			reply->words[1] = iface->id;
			reply->words[2] = 0;
			reply->words[3] = payload_len;
			return;
		}
	}
	if (prev != 0) {
		prev->next = packet->next;
	} else {
		iface->rx_head = packet->next;
	}
	if (iface->rx_tail == packet) {
		iface->rx_tail = prev;
	}
	reply->words[0] = 0;
	reply->words[1] = iface->id;
	reply->words[2] = copy_len;
	reply->words[3] = payload_len;
	bunix_free(packet);
}

static void reply_packet_tx_enqueue(struct bunix_msg *reply,
				    const struct bunix_msg *message)
{
	struct bunix_net_packet_info info;
	struct net_interface *iface;
	struct net_packet *packet;

	if (message->cap == 0 ||
	    (message->cap_rights & BUNIX_RIGHT_RECV) == 0 ||
	    bunix_buffer_read(message->cap, 0, &info, sizeof(info)) != 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	iface = interface_find(info.iface);
	if (iface == 0 || iface == &loopback || info.len == 0 ||
	    info.len > iface->mtu ||
	    (iface->flags & BUNIX_NET_IFACE_FLAG_RUNNING) == 0) {
		if (iface != 0 && iface != &loopback) {
			iface->tx_drops++;
		}
		reply->words[0] = (u64)-1;
		return;
	}
	packet = (struct net_packet *)bunix_alloc(sizeof(*packet) + info.len);
	if (packet == 0) {
		iface->tx_drops++;
		reply->words[0] = (u64)-1;
		return;
	}
	packet->next = 0;
	packet->family = 0;
	packet->len = info.len;
	if (bunix_buffer_read(message->cap, sizeof(info), packet->data,
			      info.len) != 0) {
		bunix_free(packet);
		iface->tx_drops++;
		reply->words[0] = (u64)-1;
		return;
	}
	if (iface->tx_tail != 0) {
		iface->tx_tail->next = packet;
	} else {
		iface->tx_head = packet;
	}
	iface->tx_tail = packet;
	reply->words[0] = 0;
	reply->words[1] = iface->id;
	reply->words[2] = info.len;
	reply->words[3] = 0;
}

static void reply_packet_tx_dequeue(struct bunix_msg *reply,
				    const struct bunix_msg *message)
{
	struct net_interface *iface = interface_find(message->words[0]);
	const u64 max_len = message->words[1];
	struct net_packet *packet;
	struct bunix_net_packet_info info;

	if (iface == 0 || iface == &loopback ||
	    (iface->flags & BUNIX_NET_IFACE_FLAG_RUNNING) == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	packet = iface->tx_head;
	if (packet == 0) {
		reply->words[0] = (u64)-1;
		reply->words[1] = iface->id;
		reply->words[2] = 0;
		reply->words[3] = 0;
		return;
	}
	if (max_len == 0 || packet->len > max_len || message->cap == 0 ||
	    (message->cap_rights & BUNIX_RIGHT_SEND) == 0) {
		reply->words[0] = (u64)-1;
		reply->words[1] = iface->id;
		reply->words[2] = packet->len;
		reply->words[3] = 0;
		return;
	}
	info.iface = iface->id;
	info.len = packet->len;
	info.flags = 0;
	info.reserved = 0;
	if (bunix_buffer_write(message->cap, 0, &info, sizeof(info)) != 0 ||
	    bunix_buffer_write(message->cap, sizeof(info), packet->data,
			       packet->len) != 0) {
		reply->words[0] = (u64)-1;
		reply->words[1] = iface->id;
		reply->words[2] = packet->len;
		reply->words[3] = 0;
		return;
	}
	iface->tx_head = packet->next;
	if (iface->tx_head == 0) {
		iface->tx_tail = 0;
	}
	reply->words[0] = 0;
	reply->words[1] = iface->id;
	reply->words[2] = packet->len;
	reply->words[3] = 0;
	bunix_free(packet);
}

static void reply_packet_tx_complete(struct bunix_msg *reply,
				     const struct bunix_msg *message)
{
	struct net_interface *iface = interface_find(message->words[0]);

	if (iface == 0 || iface == &loopback) {
		reply->words[0] = (u64)-1;
		return;
	}
	iface->tx_packets++;
	reply->words[0] = 0;
	reply->words[1] = iface->id;
	reply->words[2] = iface->tx_packets;
	reply->words[3] = 0;
}

static int net_route_matches(const struct net_route *route, u64 family,
			     u64 hi, u64 lo)
{
	if (route == 0 || route->family != family) {
		return 0;
	}
	if (family == BUNIX_NET_ADDR_FAMILY_IPV4) {
		const u64 mask = route->prefix_len == 0 ? 0 :
				 ~((1ull << (32 - route->prefix_len)) - 1);

		return hi == 0 && route->prefix_hi == 0 &&
		       ((lo & mask) == (route->prefix_lo & mask));
	}
	if (family == BUNIX_NET_ADDR_FAMILY_IPV6) {
		if (route->prefix_len == 0) {
			return 1;
		}
		if (route->prefix_len <= 64) {
			const u64 mask = route->prefix_len == 64 ?
					 ~0ull :
					 ~((1ull << (64 - route->prefix_len)) - 1);

			return (hi & mask) == (route->prefix_hi & mask);
		}
		{
			const u64 low_bits = 128 - route->prefix_len;
			const u64 mask = low_bits == 0 ?
					 ~0ull :
					 ~((1ull << low_bits) - 1);

			return hi == route->prefix_hi &&
			       ((lo & mask) == (route->prefix_lo & mask));
		}
	}
	return 0;
}

static const struct net_route *net_route_lookup_in_list(
	const struct net_route *list, u64 family, u64 hi, u64 lo,
	const struct net_route *best)
{
	for (const struct net_route *route = list; route != 0;
	     route = route->next) {
		if (net_route_matches(route, family, hi, lo) &&
		    (best == 0 || route->prefix_len > best->prefix_len)) {
			best = route;
		}
	}
	return best;
}

static const struct net_route *net_route_lookup(u64 family, u64 hi, u64 lo)
{
	const struct net_route *best = 0;

	best = net_route_lookup_in_list(dynamic_routes, family, hi, lo, best);
	for (u64 i = 0; i < sizeof(routes) / sizeof(routes[0]); i++) {
		if (net_route_matches(&routes[i], family, hi, lo) &&
		    (best == 0 || routes[i].prefix_len > best->prefix_len)) {
			best = &routes[i];
		}
	}
	return best;
}

static int net_addr_is_local(u64 family, u64 hi, u64 lo)
{
	for (const struct net_addr_record *addr = dynamic_addrs; addr != 0;
	     addr = addr->next) {
		if (addr->family == family && addr->addr_hi == hi &&
		    addr->addr_lo == lo) {
			return 1;
		}
	}
	for (u64 i = 0; i < sizeof(static_addrs) / sizeof(static_addrs[0]); i++) {
		if (static_addrs[i].family == family &&
		    static_addrs[i].addr_hi == hi &&
		    static_addrs[i].addr_lo == lo) {
			return 1;
		}
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
	     !net_addr_is_local(socket->family, hi, lo))) {
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
	const u64 hi = message->words[1];
	const u64 lo = message->words[2];
	const u64 port = message->words[3];
	const int local = socket != 0 &&
			  net_addr_is_local(socket->family, hi, lo);
	const int routed_ipv4 = socket != 0 &&
				socket->family == BUNIX_NET_ADDR_FAMILY_IPV4 &&
				hi == 0 &&
				net_route_lookup(socket->family, hi, lo) != 0;
	const int routed_ipv6 = socket != 0 &&
				socket->family == BUNIX_NET_ADDR_FAMILY_IPV6 &&
				net_route_lookup(socket->family, hi, lo) != 0;

	if (socket == 0 || (!local && !routed_ipv4 && !routed_ipv6) ||
	    port == 0 || port > 65535) {
		reply->words[0] = (u64)-1;
		return;
	}
	if (!socket->bound && udp_bind_socket(socket, 0, 0, 0) != 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	socket->peer.hi = hi;
	socket->peer.lo = lo;
	socket->peer.port = port;
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

static int udp_deliver_to(struct udp_socket *source, const struct net_addr *dest_addr,
			  const unsigned char *data, u64 len)
{
	struct net_addr old_peer;
	u64 old_connected;
	int result;

	if (source == 0 || dest_addr == 0 ||
	    source->family != dest_addr->family ||
	    dest_addr->port == 0 || dest_addr->port > 65535) {
		return -1;
	}
	if (!net_addr_is_local(source->family, dest_addr->hi, dest_addr->lo)) {
		return source->family == BUNIX_NET_ADDR_FAMILY_IPV6 ?
		       udp_sendto_external_ipv6(source, dest_addr, data, len) :
		       udp_sendto_external_ipv4(source, dest_addr, data, len);
	}
	if (!source->bound && udp_bind_socket(source, 0, 0, 0) != 0) {
		return -1;
	}
	old_peer = source->peer;
	old_connected = source->connected;
	source->peer = *dest_addr;
	source->connected = 1;
	result = udp_deliver(source, data, len);
	source->peer = old_peer;
	source->connected = old_connected;
	return result;
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
	if (net_addr_is_local(socket->family, socket->peer.hi,
			      socket->peer.lo)) {
		delivered = udp_deliver(socket, data, len);
	} else {
		delivered = socket->family == BUNIX_NET_ADDR_FAMILY_IPV6 ?
			    udp_sendto_external_ipv6(socket, &socket->peer,
						     data, len) :
			    udp_sendto_external_ipv4(socket, &socket->peer,
						     data, len);
	}
	bunix_free(data);
	if (delivered != 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	reply->words[0] = 0;
	reply->words[1] = len;
}

static void reply_udp_sendto(struct bunix_msg *reply,
			     const struct bunix_msg *message)
{
	struct udp_socket *socket = udp_find(message->words[0]);
	const u64 len = message->words[1];
	struct net_addr dest;
	unsigned char *data;
	long delivered;

	if (socket == 0 || len > NET_PACKET_MAX || message->cap == 0 ||
	    bunix_buffer_read(message->cap, 0, &dest, sizeof(dest)) != 0 ||
	    dest.family != socket->family) {
		reply->words[0] = (u64)-1;
		return;
	}
	data = (unsigned char *)bunix_alloc(len == 0 ? 1 : len);
	if (data == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	if (len != 0 &&
	    bunix_buffer_read(message->cap, sizeof(dest), data, len) != 0) {
		bunix_free(data);
		reply->words[0] = (u64)-1;
		return;
	}
	delivered = udp_deliver_to(socket, &dest, data, len);
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

static void reply_udp_addr(struct bunix_msg *reply,
			   const struct bunix_msg *message, int peer)
{
	struct udp_socket *socket = udp_find(message->words[0]);
	const struct net_addr *addr;

	if (socket == 0 || (peer && !socket->connected)) {
		reply->words[0] = (u64)-1;
		return;
	}
	addr = peer ? &socket->peer : &socket->local;
	reply->words[0] = 0;
	reply->words[1] = addr->hi;
	reply->words[2] = addr->lo;
	reply->words[3] = addr->port;
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

static struct tcp_socket *tcp_find(u64 id)
{
	for (struct tcp_socket *socket = tcp_sockets; socket != 0;
	     socket = socket->next) {
		if (socket->id == id) {
			return socket;
		}
	}
	return 0;
}

static int tcp_port_in_use(u64 family, u64 port)
{
	for (struct tcp_socket *socket = tcp_sockets; socket != 0;
	     socket = socket->next) {
		if (socket->state != NET_TCP_STATE_CLOSED &&
		    socket->family == family &&
		    socket->local.port == port) {
			return 1;
		}
	}
	return 0;
}

static u64 tcp_alloc_ephemeral(u64 family)
{
	for (u64 attempt = NET_TCP_PORT_EPHEMERAL_FIRST;
	     attempt <= NET_TCP_PORT_EPHEMERAL_LAST; attempt++) {
		const u64 port = next_tcp_ephemeral_port++;

		if (next_tcp_ephemeral_port > NET_TCP_PORT_EPHEMERAL_LAST) {
			next_tcp_ephemeral_port = NET_TCP_PORT_EPHEMERAL_FIRST;
		}
		if (!tcp_port_in_use(family, port)) {
			return port;
		}
	}
	return 0;
}

static struct tcp_socket *tcp_alloc_socket(u64 family)
{
	struct tcp_socket *socket;

	if (family != BUNIX_NET_ADDR_FAMILY_IPV4 &&
	    family != BUNIX_NET_ADDR_FAMILY_IPV6) {
		return 0;
	}
	socket = (struct tcp_socket *)bunix_alloc(sizeof(*socket));
	if (socket == 0) {
		return 0;
	}
	socket->next = tcp_sockets;
	socket->id = next_tcp_socket_id++;
	socket->family = family;
	socket->state = NET_TCP_STATE_OPEN;
	socket->local.family = family;
	socket->local.hi = 0;
	socket->local.lo = 0;
	socket->local.port = 0;
	socket->peer_addr.family = family;
	socket->peer_addr.hi = 0;
	socket->peer_addr.lo = 0;
	socket->peer_addr.port = 0;
	socket->peer_socket = 0;
	socket->backlog = 0;
	socket->pending_count = 0;
	socket->read_closed = 0;
	socket->write_closed = 0;
	socket->peer_write_closed = 0;
	socket->reset = 0;
	socket->rx_len = 0;
	socket->rx_head = 0;
	socket->rx_tail = 0;
	socket->pending_head = 0;
	socket->pending_tail = 0;
	tcp_sockets = socket;
	return socket;
}

static void reply_tcp_open(struct bunix_msg *reply,
			   const struct bunix_msg *message)
{
	struct tcp_socket *socket = tcp_alloc_socket(message->words[0]);

	if (socket == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	reply->words[0] = 0;
	reply->words[1] = socket->id;
}

static long tcp_bind_socket(struct tcp_socket *socket, u64 hi, u64 lo,
			    u64 port)
{
	if (socket == 0 ||
	    socket->state == NET_TCP_STATE_CLOSED ||
	    (!net_addr_is_wildcard(socket->family, hi, lo) &&
	     !net_addr_is_local(socket->family, hi, lo))) {
		return -1;
	}
	if (port == 0) {
		port = tcp_alloc_ephemeral(socket->family);
	}
	if (port == 0 || port > 65535 ||
	    tcp_port_in_use(socket->family, port)) {
		return -1;
	}
	socket->local.hi = hi;
	socket->local.lo = lo;
	socket->local.port = port;
	return 0;
}

static void reply_tcp_bind(struct bunix_msg *reply,
			   const struct bunix_msg *message)
{
	struct tcp_socket *socket = tcp_find(message->words[0]);

	if (tcp_bind_socket(socket, message->words[1], message->words[2],
			    message->words[3]) != 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	reply->words[0] = 0;
	reply->words[1] = socket->local.port;
}

static void reply_tcp_listen(struct bunix_msg *reply,
			     const struct bunix_msg *message)
{
	struct tcp_socket *socket = tcp_find(message->words[0]);
	u64 backlog = message->words[1];

	if (socket == 0 || socket->state != NET_TCP_STATE_OPEN ||
	    socket->local.port == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	if (backlog == 0) {
		backlog = 1;
	}
	if (backlog > 128) {
		backlog = 128;
	}
	socket->state = NET_TCP_STATE_LISTEN;
	socket->backlog = backlog;
	reply->words[0] = 0;
	reply->words[1] = backlog;
}

static struct tcp_socket *tcp_find_listener(u64 family, u64 hi, u64 lo,
					    u64 port)
{
	for (struct tcp_socket *socket = tcp_sockets; socket != 0;
	     socket = socket->next) {
		if (socket->state == NET_TCP_STATE_LISTEN &&
		    socket->family == family &&
		    socket->local.port == port &&
		    (net_addr_is_wildcard(socket->family, socket->local.hi,
					  socket->local.lo) ||
		     (socket->local.hi == hi && socket->local.lo == lo))) {
			return socket;
		}
	}
	return 0;
}

static int tcp_pending_push(struct tcp_socket *listener, u64 socket_id)
{
	struct tcp_pending *pending;

	if (listener->pending_count >= listener->backlog) {
		return -1;
	}
	pending = (struct tcp_pending *)bunix_alloc(sizeof(*pending));
	if (pending == 0) {
		return -1;
	}
	pending->next = 0;
	pending->socket_id = socket_id;
	if (listener->pending_tail != 0) {
		listener->pending_tail->next = pending;
	} else {
		listener->pending_head = pending;
	}
	listener->pending_tail = pending;
	listener->pending_count++;
	return 0;
}

static unsigned short tcp_ipv4_checksum(const unsigned char *tcp, u64 len,
					u64 source_ip, u64 dest_ip)
{
	unsigned char pseudo[12];

	net_write_be32(pseudo, source_ip);
	net_write_be32(pseudo + 4, dest_ip);
	pseudo[8] = 0;
	pseudo[9] = NET_PROTO_TCP;
	net_write_be16(pseudo + 10, len);
	return net_checksum_finish(net_checksum_sum(tcp, len,
						    net_checksum_sum(pseudo,
								     sizeof(pseudo),
								     0)));
}

static unsigned short tcp_ipv6_checksum(const unsigned char *tcp, u64 len,
					u64 source_hi, u64 source_lo,
					u64 dest_hi, u64 dest_lo)
{
	return ipv6_l4_checksum(tcp, len, NET_PROTO_TCP, source_hi, source_lo,
				dest_hi, dest_lo);
}

static void tcp_send_synack_ipv4(struct net_interface *iface,
				 const unsigned char *request,
				 u64 source_ip, u64 dest_ip,
				 u64 source_port, u64 dest_port,
				 u64 ack_seq)
{
	unsigned char frame[54];
	unsigned short ip_checksum;
	unsigned short tcp_checksum;
	const u64 source_mac = net_read_mac48(request + 6);

	if (iface == 0 || iface == &loopback || source_mac == 0 ||
	    iface->mac_hi == 0) {
		return;
	}
	for (u64 i = 0; i < 6; i++) {
		frame[i] = (unsigned char)((source_mac >> ((5 - i) * 8)) &
					   0xff);
		frame[6 + i] = (unsigned char)((iface->mac_hi >>
						((5 - i) * 8)) & 0xff);
	}
	frame[12] = 0x08;
	frame[13] = 0x00;
	frame[14] = 0x45;
	frame[15] = 0;
	net_write_be16(frame + 16, 40);
	net_write_be16(frame + 18, 0);
	net_write_be16(frame + 20, 0);
	frame[22] = 64;
	frame[23] = NET_PROTO_TCP;
	frame[24] = 0;
	frame[25] = 0;
	net_write_be32(frame + 26, source_ip);
	net_write_be32(frame + 30, dest_ip);
	net_write_be16(frame + 34, source_port);
	net_write_be16(frame + 36, dest_port);
	net_write_be32(frame + 38, 0);
	net_write_be32(frame + 42, ack_seq);
	frame[46] = 0x50;
	frame[47] = 0x12;
	net_write_be16(frame + 48, 65535);
	net_write_be16(frame + 50, 0);
	net_write_be16(frame + 52, 0);
	ip_checksum = net_checksum_finish(net_checksum_sum(frame + 14, 20, 0));
	frame[24] = (unsigned char)((ip_checksum >> 8) & 0xff);
	frame[25] = (unsigned char)(ip_checksum & 0xff);
	tcp_checksum = tcp_ipv4_checksum(frame + 34, 20, source_ip, dest_ip);
	frame[50] = (unsigned char)((tcp_checksum >> 8) & 0xff);
	frame[51] = (unsigned char)(tcp_checksum & 0xff);
	(void)packet_tx_enqueue_copy(iface, frame, sizeof(frame));
}

static void tcp_send_synack_ipv6(struct net_interface *iface,
				 const unsigned char *request,
				 u64 source_hi, u64 source_lo,
				 u64 dest_hi, u64 dest_lo,
				 u64 source_port, u64 dest_port,
				 u64 ack_seq)
{
	unsigned char frame[74];
	unsigned short tcp_checksum;
	const u64 source_mac = net_read_mac48(request + 6);

	if (iface == 0 || iface == &loopback || source_mac == 0 ||
	    iface->mac_hi == 0) {
		return;
	}
	for (u64 i = 0; i < 6; i++) {
		frame[i] = (unsigned char)((source_mac >> ((5 - i) * 8)) &
					   0xff);
		frame[6 + i] = (unsigned char)((iface->mac_hi >>
						((5 - i) * 8)) & 0xff);
	}
	frame[12] = 0x86;
	frame[13] = 0xdd;
	ndp_write_ipv6_header(frame, 20, NET_PROTO_TCP, 64, source_hi,
			      source_lo, dest_hi, dest_lo);
	net_write_be16(frame + 54, source_port);
	net_write_be16(frame + 56, dest_port);
	net_write_be32(frame + 58, 0);
	net_write_be32(frame + 62, ack_seq);
	frame[66] = 0x50;
	frame[67] = 0x12;
	net_write_be16(frame + 68, 65535);
	net_write_be16(frame + 70, 0);
	net_write_be16(frame + 72, 0);
	tcp_checksum = tcp_ipv6_checksum(frame + 54, 20, source_hi, source_lo,
					 dest_hi, dest_lo);
	frame[70] = (unsigned char)((tcp_checksum >> 8) & 0xff);
	frame[71] = (unsigned char)(tcp_checksum & 0xff);
	(void)packet_tx_enqueue_copy(iface, frame, sizeof(frame));
}

static void packet_ingress_tcp_ipv4(struct net_interface *iface,
				    const unsigned char *packet, u64 len)
{
	const unsigned char *ip;
	const unsigned char *tcp;
	struct tcp_socket *listener;
	struct tcp_socket *server;
	u64 ihl;
	u64 total_len;
	u64 fragment;
	u64 tcp_header_len;
	u64 src_ip;
	u64 dst_ip;
	u64 src_port;
	u64 dst_port;
	u64 seq;
	u64 flags;

	if (iface == 0 || packet == 0 || len < 14 + 20 ||
	    net_read_be16(packet + 12) != 0x0800) {
		return;
	}
	ip = packet + 14;
	if ((ip[0] >> 4) != 4 || ip[9] != NET_PROTO_TCP) {
		return;
	}
	ihl = (ip[0] & 0x0f) * 4;
	if (ihl < 20 || len < 14 + ihl + 20) {
		return;
	}
	total_len = net_read_be16(ip + 2);
	if (total_len < ihl + 20 || len < 14 + total_len) {
		return;
	}
	fragment = net_read_be16(ip + 6);
	if ((fragment & 0x3fff) != 0) {
		return;
	}
	src_ip = net_read_be32(ip + 12);
	dst_ip = net_read_be32(ip + 16);
	if (!net_addr_is_local(BUNIX_NET_ADDR_FAMILY_IPV4, 0, dst_ip)) {
		return;
	}
	tcp = ip + ihl;
	tcp_header_len = (tcp[12] >> 4) * 4;
	if (tcp_header_len < 20 || total_len < ihl + tcp_header_len) {
		return;
	}
	src_port = net_read_be16(tcp);
	dst_port = net_read_be16(tcp + 2);
	flags = tcp[13];
	if (src_ip == 0 || src_port == 0 || dst_port == 0 ||
	    (flags & 0x02) == 0 || (flags & 0x10) != 0) {
		return;
	}
	listener = tcp_find_listener(BUNIX_NET_ADDR_FAMILY_IPV4, 0, dst_ip,
				     dst_port);
	if (listener == 0) {
		return;
	}
	server = tcp_alloc_socket(BUNIX_NET_ADDR_FAMILY_IPV4);
	if (server == 0) {
		return;
	}
	server->state = NET_TCP_STATE_ESTABLISHED;
	server->local.family = BUNIX_NET_ADDR_FAMILY_IPV4;
	server->local.hi = 0;
	server->local.lo = dst_ip;
	server->local.port = dst_port;
	server->peer_addr.family = BUNIX_NET_ADDR_FAMILY_IPV4;
	server->peer_addr.hi = 0;
	server->peer_addr.lo = src_ip;
	server->peer_addr.port = src_port;
	seq = net_read_be32(tcp + 4);
	if (tcp_pending_push(listener, server->id) != 0) {
		server->state = NET_TCP_STATE_CLOSED;
		return;
	}
	tcp_send_synack_ipv4(iface, packet, dst_ip, src_ip, dst_port,
			     src_port, seq + 1);
}

static void packet_ingress_tcp_ipv6(struct net_interface *iface,
				    const unsigned char *packet, u64 len)
{
	const unsigned char *ip;
	const unsigned char *tcp;
	struct tcp_socket *listener;
	struct tcp_socket *server;
	u64 payload_len;
	u64 tcp_header_len;
	u64 src_hi;
	u64 src_lo;
	u64 dst_hi;
	u64 dst_lo;
	u64 src_port;
	u64 dst_port;
	u64 seq;
	u64 flags;

	if (iface == 0 || packet == 0 || len < 14 + 40 + 20 ||
	    net_read_be16(packet + 12) != NET_ETH_IPV6) {
		return;
	}
	ip = packet + 14;
	if ((ip[0] >> 4) != 6 || ip[6] != NET_PROTO_TCP) {
		return;
	}
	payload_len = net_read_be16(ip + 4);
	if (payload_len < 20 || len < 14 + 40 + payload_len) {
		return;
	}
	src_hi = net_read_be64(ip + 8);
	src_lo = net_read_be64(ip + 16);
	dst_hi = net_read_be64(ip + 24);
	dst_lo = net_read_be64(ip + 32);
	if (!net_addr_is_local(BUNIX_NET_ADDR_FAMILY_IPV6, dst_hi, dst_lo)) {
		return;
	}
	tcp = ip + 40;
	tcp_header_len = (tcp[12] >> 4) * 4;
	if (tcp_header_len < 20 || payload_len < tcp_header_len) {
		return;
	}
	src_port = net_read_be16(tcp);
	dst_port = net_read_be16(tcp + 2);
	flags = tcp[13];
	if ((src_hi == 0 && src_lo == 0) || src_port == 0 || dst_port == 0 ||
	    (flags & 0x02) == 0 || (flags & 0x10) != 0) {
		return;
	}
	listener = tcp_find_listener(BUNIX_NET_ADDR_FAMILY_IPV6, dst_hi,
				     dst_lo, dst_port);
	if (listener == 0) {
		return;
	}
	server = tcp_alloc_socket(BUNIX_NET_ADDR_FAMILY_IPV6);
	if (server == 0) {
		return;
	}
	server->state = NET_TCP_STATE_ESTABLISHED;
	server->local.family = BUNIX_NET_ADDR_FAMILY_IPV6;
	server->local.hi = dst_hi;
	server->local.lo = dst_lo;
	server->local.port = dst_port;
	server->peer_addr.family = BUNIX_NET_ADDR_FAMILY_IPV6;
	server->peer_addr.hi = src_hi;
	server->peer_addr.lo = src_lo;
	server->peer_addr.port = src_port;
	seq = net_read_be32(tcp + 4);
	if (tcp_pending_push(listener, server->id) != 0) {
		server->state = NET_TCP_STATE_CLOSED;
		return;
	}
	tcp_send_synack_ipv6(iface, packet, dst_hi, dst_lo, src_hi, src_lo,
			     dst_port, src_port, seq + 1);
}

static int tcp_connect_external_ipv4(struct tcp_socket *client, u64 dest_ip,
				     u64 dest_port)
{
	const struct net_route *route;
	struct net_interface *iface;
	unsigned char *frame;
	u64 source_ip;
	u64 next_hop_ip;
	u64 dest_mac = 0;
	unsigned short ip_checksum;
	unsigned short tcp_checksum;

	if (client == 0 || client->state != NET_TCP_STATE_OPEN ||
	    client->family != BUNIX_NET_ADDR_FAMILY_IPV4 || dest_ip == 0 ||
	    dest_port == 0 || dest_port > 65535) {
		return -1;
	}
	route = net_route_lookup(BUNIX_NET_ADDR_FAMILY_IPV4, 0, dest_ip);
	if (route == 0 || (route->flags & BUNIX_NET_ROUTE_FLAG_UP) == 0) {
		return -1;
	}
	iface = interface_find(route->iface);
	source_ip = iface_ipv4_addr(route->iface);
	next_hop_ip = route->gateway_lo != 0 ? route->gateway_lo : dest_ip;
	if (iface == 0 || source_ip == 0 || iface->mac_hi == 0) {
		return -1;
	}
	if (client->local.port == 0 &&
	    tcp_bind_socket(client, 0, 0, 0) != 0) {
		return -1;
	}
	{
		struct net_neighbor *neighbor =
			neighbor_find(BUNIX_NET_ADDR_FAMILY_IPV4, 0,
				      next_hop_ip, route->iface);

		if (neighbor != 0) {
			dest_mac = neighbor->mac_hi;
		}
	}
	frame = (unsigned char *)bunix_alloc(54);
	if (frame == 0) {
		return -1;
	}
	for (u64 i = 0; i < 6; i++) {
		frame[i] = dest_mac != 0 ?
			   (unsigned char)((dest_mac >> ((5 - i) * 8)) & 0xff) :
			   0xff;
		frame[6 + i] = (unsigned char)((iface->mac_hi >>
						((5 - i) * 8)) & 0xff);
	}
	frame[12] = 0x08;
	frame[13] = 0x00;
	frame[14] = 0x45;
	frame[15] = 0;
	net_write_be16(frame + 16, 40);
	net_write_be16(frame + 18, 0);
	net_write_be16(frame + 20, 0);
	frame[22] = 64;
	frame[23] = NET_PROTO_TCP;
	frame[24] = 0;
	frame[25] = 0;
	net_write_be32(frame + 26, source_ip);
	net_write_be32(frame + 30, dest_ip);
	net_write_be16(frame + 34, client->local.port);
	net_write_be16(frame + 36, dest_port);
	net_write_be32(frame + 38, 0);
	net_write_be32(frame + 42, 0);
	frame[46] = 0x50;
	frame[47] = 0x02;
	net_write_be16(frame + 48, 65535);
	net_write_be16(frame + 50, 0);
	net_write_be16(frame + 52, 0);
	ip_checksum = net_checksum_finish(net_checksum_sum(frame + 14, 20, 0));
	frame[24] = (unsigned char)((ip_checksum >> 8) & 0xff);
	frame[25] = (unsigned char)(ip_checksum & 0xff);
	tcp_checksum = tcp_ipv4_checksum(frame + 34, 20, source_ip, dest_ip);
	frame[50] = (unsigned char)((tcp_checksum >> 8) & 0xff);
	frame[51] = (unsigned char)(tcp_checksum & 0xff);
	if (dest_mac == 0) {
		if (pending_frame_add(BUNIX_NET_ADDR_FAMILY_IPV4, 0,
				      next_hop_ip, route->iface, frame,
				      54) != 0) {
			bunix_free(frame);
			return -1;
		}
		arp_request_ipv4(iface, source_ip, next_hop_ip);
	} else if (packet_tx_enqueue_copy(iface, frame, 54) != 0) {
		bunix_free(frame);
		return -1;
	}
	bunix_free(frame);
	client->state = NET_TCP_STATE_ESTABLISHED;
	client->peer_addr.family = BUNIX_NET_ADDR_FAMILY_IPV4;
	client->peer_addr.hi = 0;
	client->peer_addr.lo = dest_ip;
	client->peer_addr.port = dest_port;
	client->peer_socket = 0;
	return 0;
}

static int tcp_connect_external_ipv6(struct tcp_socket *client, u64 dest_hi,
				     u64 dest_lo, u64 dest_port)
{
	const struct net_route *route;
	struct net_interface *iface;
	unsigned char *frame;
	u64 source_hi = 0;
	u64 source_lo = 0;
	u64 next_hop_hi;
	u64 next_hop_lo;
	u64 dest_mac = 0;
	unsigned short tcp_checksum;

	if (client == 0 || client->state != NET_TCP_STATE_OPEN ||
	    client->family != BUNIX_NET_ADDR_FAMILY_IPV6 ||
	    (dest_hi == 0 && dest_lo == 0) || dest_port == 0 ||
	    dest_port > 65535) {
		return -1;
	}
	route = net_route_lookup(BUNIX_NET_ADDR_FAMILY_IPV6, dest_hi,
				 dest_lo);
	if (route == 0 || (route->flags & BUNIX_NET_ROUTE_FLAG_UP) == 0) {
		return -1;
	}
	iface = interface_find(route->iface);
	next_hop_hi = (route->flags & BUNIX_NET_ROUTE_FLAG_GATEWAY) != 0 ?
		      route->gateway_hi : dest_hi;
	next_hop_lo = (route->flags & BUNIX_NET_ROUTE_FLAG_GATEWAY) != 0 ?
		      route->gateway_lo : dest_lo;
	if (iface == 0 || iface_ipv6_addr(route->iface, &source_hi,
					  &source_lo) != 0 ||
	    iface->mac_hi == 0 || (next_hop_hi == 0 && next_hop_lo == 0)) {
		return -1;
	}
	if (client->local.port == 0 &&
	    tcp_bind_socket(client, 0, 0, 0) != 0) {
		return -1;
	}
	{
		struct net_neighbor *neighbor =
			neighbor_find(BUNIX_NET_ADDR_FAMILY_IPV6, next_hop_hi,
				      next_hop_lo, route->iface);

		if (neighbor != 0) {
			dest_mac = neighbor->mac_hi;
		}
	}
	frame = (unsigned char *)bunix_alloc(74);
	if (frame == 0) {
		return -1;
	}
	for (u64 i = 0; i < 6; i++) {
		frame[i] = dest_mac != 0 ?
			   (unsigned char)((dest_mac >> ((5 - i) * 8)) & 0xff) :
			   0xff;
		frame[6 + i] = (unsigned char)((iface->mac_hi >>
						((5 - i) * 8)) & 0xff);
	}
	frame[12] = 0x86;
	frame[13] = 0xdd;
	ndp_write_ipv6_header(frame, 20, NET_PROTO_TCP, 64, source_hi,
			      source_lo, dest_hi, dest_lo);
	net_write_be16(frame + 54, client->local.port);
	net_write_be16(frame + 56, dest_port);
	net_write_be32(frame + 58, 0);
	net_write_be32(frame + 62, 0);
	frame[66] = 0x50;
	frame[67] = 0x02;
	net_write_be16(frame + 68, 65535);
	net_write_be16(frame + 70, 0);
	net_write_be16(frame + 72, 0);
	tcp_checksum = tcp_ipv6_checksum(frame + 54, 20, source_hi, source_lo,
					 dest_hi, dest_lo);
	frame[70] = (unsigned char)((tcp_checksum >> 8) & 0xff);
	frame[71] = (unsigned char)(tcp_checksum & 0xff);
	if (dest_mac == 0) {
		if (pending_frame_add(BUNIX_NET_ADDR_FAMILY_IPV6, next_hop_hi,
				      next_hop_lo, route->iface, frame,
				      74) != 0) {
			bunix_free(frame);
			return -1;
		}
		ndp_request_ipv6(iface, source_hi, source_lo, next_hop_hi,
				 next_hop_lo);
	} else if (packet_tx_enqueue_copy(iface, frame, 74) != 0) {
		bunix_free(frame);
		return -1;
	}
	bunix_free(frame);
	client->state = NET_TCP_STATE_ESTABLISHED;
	client->peer_addr.family = BUNIX_NET_ADDR_FAMILY_IPV6;
	client->peer_addr.hi = dest_hi;
	client->peer_addr.lo = dest_lo;
	client->peer_addr.port = dest_port;
	client->peer_socket = 0;
	return 0;
}

static void reply_tcp_connect(struct bunix_msg *reply,
			      const struct bunix_msg *message)
{
	struct tcp_socket *client = tcp_find(message->words[0]);
	struct tcp_socket *listener;
	struct tcp_socket *server;
	const u64 hi = message->words[1];
	const u64 lo = message->words[2];
	const u64 port = message->words[3];
	const int local = client != 0 &&
			  net_addr_is_local(client->family, hi, lo);
	const int routed_ipv4 = client != 0 &&
				client->family == BUNIX_NET_ADDR_FAMILY_IPV4 &&
				hi == 0 &&
				net_route_lookup(client->family, hi, lo) != 0;
	const int routed_ipv6 = client != 0 &&
				client->family == BUNIX_NET_ADDR_FAMILY_IPV6 &&
				net_route_lookup(client->family, hi, lo) != 0;

	if (client == 0 || client->state != NET_TCP_STATE_OPEN ||
	    (!local && !routed_ipv4 && !routed_ipv6) || port == 0 ||
	    port > 65535) {
		reply->words[0] = (u64)-1;
		return;
	}
	if (!local) {
		if ((client->family == BUNIX_NET_ADDR_FAMILY_IPV6 ?
		     tcp_connect_external_ipv6(client, hi, lo, port) :
		     tcp_connect_external_ipv4(client, lo, port)) != 0) {
			reply->words[0] = (u64)-1;
			return;
		}
		reply->words[0] = 0;
		reply->words[1] = client->local.port;
		return;
	}
	listener = tcp_find_listener(client->family, hi, lo, port);
	if (listener == 0) {
		client->reset = 1;
		reply->words[0] = (u64)-1;
		return;
	}
	if (client->local.port == 0 &&
	    tcp_bind_socket(client, 0, 0, 0) != 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	server = tcp_alloc_socket(client->family);
	if (server == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	server->state = NET_TCP_STATE_ESTABLISHED;
	server->local = listener->local;
	server->peer_addr = client->local;
	server->peer_socket = client->id;
	client->state = NET_TCP_STATE_ESTABLISHED;
	client->peer_addr.hi = hi;
	client->peer_addr.lo = lo;
	client->peer_addr.port = port;
	client->peer_socket = server->id;
	if (tcp_pending_push(listener, server->id) != 0) {
		server->state = NET_TCP_STATE_CLOSED;
		client->state = NET_TCP_STATE_OPEN;
		client->peer_socket = 0;
		reply->words[0] = (u64)-1;
		return;
	}
	reply->words[0] = 0;
	reply->words[1] = client->local.port;
}

static void reply_tcp_accept(struct bunix_msg *reply,
			     const struct bunix_msg *message)
{
	struct tcp_socket *listener = tcp_find(message->words[0]);
	struct tcp_pending *pending;

	if (listener == 0 || listener->state != NET_TCP_STATE_LISTEN ||
	    listener->pending_head == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	pending = listener->pending_head;
	listener->pending_head = pending->next;
	if (listener->pending_head == 0) {
		listener->pending_tail = 0;
	}
	listener->pending_count--;
	reply->words[0] = 0;
	reply->words[1] = pending->socket_id;
	bunix_free(pending);
}

static void tcp_segment_free_all(struct tcp_socket *socket)
{
	struct tcp_segment *segment = socket->rx_head;

	while (segment != 0) {
		struct tcp_segment *next = segment->next;

		bunix_free(segment);
		segment = next;
	}
	socket->rx_head = 0;
	socket->rx_tail = 0;
	socket->rx_len = 0;
}

static void tcp_pending_free_all(struct tcp_socket *socket)
{
	struct tcp_pending *pending = socket->pending_head;

	while (pending != 0) {
		struct tcp_pending *next = pending->next;

		bunix_free(pending);
		pending = next;
	}
	socket->pending_head = 0;
	socket->pending_tail = 0;
	socket->pending_count = 0;
}

static void reply_tcp_write(struct bunix_msg *reply,
			    const struct bunix_msg *message)
{
	struct tcp_socket *socket = tcp_find(message->words[0]);
	struct tcp_socket *peer;
	struct tcp_segment *segment;
	const u64 len = message->words[1];

	if (socket == 0 || socket->state != NET_TCP_STATE_ESTABLISHED ||
	    socket->write_closed || socket->reset || len > NET_PACKET_MAX ||
	    message->cap == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	peer = tcp_find(socket->peer_socket);
	if (peer == 0 || peer->state == NET_TCP_STATE_CLOSED ||
	    peer->read_closed) {
		socket->reset = 1;
		reply->words[0] = (u64)-1;
		return;
	}
	segment = (struct tcp_segment *)bunix_alloc(sizeof(*segment) +
						    (len == 0 ? 1 : len));
	if (segment == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	segment->next = 0;
	segment->offset = 0;
	segment->len = len;
	if (len != 0 &&
	    bunix_buffer_read(message->cap, 0, segment->data, len) != 0) {
		bunix_free(segment);
		reply->words[0] = (u64)-1;
		return;
	}
	if (peer->rx_tail != 0) {
		peer->rx_tail->next = segment;
	} else {
		peer->rx_head = segment;
	}
	peer->rx_tail = segment;
	peer->rx_len += len;
	reply->words[0] = 0;
	reply->words[1] = len;
}

static void reply_tcp_read(struct bunix_msg *reply,
			   const struct bunix_msg *message)
{
	struct tcp_socket *socket = tcp_find(message->words[0]);
	const u64 max_len = message->words[1];
	u64 done = 0;

	if (socket == 0 || socket->state != NET_TCP_STATE_ESTABLISHED ||
	    socket->reset || max_len == 0 || message->cap == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	while (done < max_len && socket->rx_head != 0) {
		struct tcp_segment *segment = socket->rx_head;
		u64 chunk = segment->len - segment->offset;

		if (chunk > max_len - done) {
			chunk = max_len - done;
		}
		if (bunix_buffer_write(message->cap, done,
				       segment->data + segment->offset,
				       chunk) != 0) {
			reply->words[0] = (u64)-1;
			return;
		}
		done += chunk;
		segment->offset += chunk;
		socket->rx_len -= chunk;
		if (segment->offset == segment->len) {
			socket->rx_head = segment->next;
			if (socket->rx_head == 0) {
				socket->rx_tail = 0;
			}
			bunix_free(segment);
		}
	}
	reply->words[0] = 0;
	reply->words[1] = done;
	reply->words[2] = done == 0 && socket->peer_write_closed;
}

static void tcp_mark_peer_write_closed(struct tcp_socket *socket)
{
	struct tcp_socket *peer = tcp_find(socket->peer_socket);

	if (peer != 0) {
		peer->peer_write_closed = 1;
	}
}

static void reply_tcp_shutdown(struct bunix_msg *reply,
			       const struct bunix_msg *message)
{
	struct tcp_socket *socket = tcp_find(message->words[0]);
	const u64 how = message->words[1];

	if (socket == 0 || socket->state != NET_TCP_STATE_ESTABLISHED) {
		reply->words[0] = (u64)-1;
		return;
	}
	if ((how & NET_TCP_SHUT_RD) != 0) {
		socket->read_closed = 1;
		tcp_segment_free_all(socket);
	}
	if ((how & NET_TCP_SHUT_WR) != 0) {
		socket->write_closed = 1;
		tcp_mark_peer_write_closed(socket);
	}
	reply->words[0] = 0;
}

static void reply_tcp_poll(struct bunix_msg *reply,
			   const struct bunix_msg *message)
{
	struct tcp_socket *socket = tcp_find(message->words[0]);
	u64 events = 0;

	if (socket == 0 || socket->state == NET_TCP_STATE_CLOSED) {
		reply->words[0] = (u64)-1;
		return;
	}
	if (socket->state == NET_TCP_STATE_LISTEN) {
		if (socket->pending_head != 0) {
			events |= NET_TCP_POLLIN;
		}
	} else if (socket->state == NET_TCP_STATE_ESTABLISHED) {
		if (socket->rx_head != 0 || socket->peer_write_closed) {
			events |= NET_TCP_POLLIN;
		}
		if (!socket->write_closed && !socket->reset) {
			events |= NET_TCP_POLLOUT;
		}
		if (socket->peer_write_closed || socket->reset) {
			events |= NET_TCP_POLLHUP;
		}
	}
	reply->words[0] = 0;
	reply->words[1] = events;
}

static void reply_tcp_addr(struct bunix_msg *reply,
			   const struct bunix_msg *message, int peer)
{
	struct tcp_socket *socket = tcp_find(message->words[0]);
	const struct net_addr *addr;

	if (socket == 0 || socket->state == NET_TCP_STATE_CLOSED ||
	    (peer && socket->state != NET_TCP_STATE_ESTABLISHED)) {
		reply->words[0] = (u64)-1;
		return;
	}
	addr = peer ? &socket->peer_addr : &socket->local;
	reply->words[0] = 0;
	reply->words[1] = addr->hi;
	reply->words[2] = addr->lo;
	reply->words[3] = addr->port;
}

static void reply_tcp_close(struct bunix_msg *reply,
			    const struct bunix_msg *message)
{
	struct tcp_socket *socket = tcp_find(message->words[0]);

	if (socket == 0 || socket->state == NET_TCP_STATE_CLOSED) {
		reply->words[0] = (u64)-1;
		return;
	}
	tcp_mark_peer_write_closed(socket);
	tcp_segment_free_all(socket);
	tcp_pending_free_all(socket);
	socket->state = NET_TCP_STATE_CLOSED;
	reply->words[0] = 0;
}

static struct icmp_socket *icmp_find(u64 id)
{
	for (struct icmp_socket *socket = icmp_sockets; socket != 0;
	     socket = socket->next) {
		if (socket->id == id) {
			return socket;
		}
	}
	return 0;
}

static u64 net_checksum_add_byte(u64 sum, unsigned char byte, int high)
{
	sum += high ? ((u64)byte << 8) : byte;
	while (sum >> 16) {
		sum = (sum & 0xffff) + (sum >> 16);
	}
	return sum;
}

static u64 net_checksum_sum(const unsigned char *data, u64 len, u64 sum)
{
	for (u64 i = 0; i < len; i++) {
		sum = net_checksum_add_byte(sum, data[i], (i & 1) == 0);
	}
	return sum;
}

static unsigned short net_checksum_finish(u64 sum)
{
	while (sum >> 16) {
		sum = (sum & 0xffff) + (sum >> 16);
	}
	return (unsigned short)(~sum & 0xffff);
}

static unsigned short icmp_ipv4_checksum(const unsigned char *data, u64 len)
{
	return net_checksum_finish(net_checksum_sum(data, len, 0));
}

static unsigned short ipv6_l4_checksum(const unsigned char *data, u64 len,
				       u64 protocol, u64 source_hi,
				       u64 source_lo, u64 dest_hi,
				       u64 dest_lo)
{
	unsigned char pseudo[40];

	for (u64 i = 0; i < sizeof(pseudo); i++) {
		pseudo[i] = 0;
	}
	for (u64 i = 0; i < 8; i++) {
		pseudo[7 - i] = (unsigned char)((source_hi >> (i * 8)) & 0xff);
		pseudo[15 - i] = (unsigned char)((source_lo >> (i * 8)) & 0xff);
		pseudo[23 - i] = (unsigned char)((dest_hi >> (i * 8)) & 0xff);
		pseudo[31 - i] = (unsigned char)((dest_lo >> (i * 8)) & 0xff);
	}
	pseudo[34] = (unsigned char)((len >> 8) & 0xff);
	pseudo[35] = (unsigned char)(len & 0xff);
	pseudo[39] = (unsigned char)protocol;
	return net_checksum_finish(net_checksum_sum(data, len,
						    net_checksum_sum(pseudo,
								     sizeof(pseudo),
								     0)));
}

static unsigned short icmp_ipv6_checksum(const unsigned char *data, u64 len,
					 u64 source_hi, u64 source_lo,
					 u64 dest_hi, u64 dest_lo)
{
	return ipv6_l4_checksum(data, len, NET_PROTO_ICMPV6, source_hi,
				source_lo, dest_hi, dest_lo);
}

static void icmp_set_checksum(unsigned char *data, u64 len, u64 family,
			      u64 source_hi, u64 source_lo,
			      u64 dest_hi, u64 dest_lo)
{
	unsigned short checksum;

	if (len < 4) {
		return;
	}
	data[2] = 0;
	data[3] = 0;
	checksum = family == BUNIX_NET_ADDR_FAMILY_IPV6 ?
		   icmp_ipv6_checksum(data, len, source_hi, source_lo,
				      dest_hi, dest_lo) :
		   icmp_ipv4_checksum(data, len);
	data[2] = (unsigned char)((checksum >> 8) & 0xff);
	data[3] = (unsigned char)(checksum & 0xff);
}

static u64 iface_ipv4_addr(u64 iface_id)
{
	for (const struct net_addr_record *addr = dynamic_addrs; addr != 0;
	     addr = addr->next) {
		if (addr->family == BUNIX_NET_ADDR_FAMILY_IPV4 &&
		    addr->iface == iface_id) {
			return addr->addr_lo;
		}
	}
	for (u64 i = 0; i < sizeof(static_addrs) / sizeof(static_addrs[0]); i++) {
		if (static_addrs[i].family == BUNIX_NET_ADDR_FAMILY_IPV4 &&
		    static_addrs[i].iface == iface_id) {
			return static_addrs[i].addr_lo;
		}
	}
	return 0;
}

static int iface_ipv6_addr(u64 iface_id, u64 *hi, u64 *lo)
{
	for (const struct net_addr_record *addr = dynamic_addrs; addr != 0;
	     addr = addr->next) {
		if (addr->family == BUNIX_NET_ADDR_FAMILY_IPV6 &&
		    addr->iface == iface_id &&
		    (addr->addr_hi != 0 || addr->addr_lo != 0)) {
			*hi = addr->addr_hi;
			*lo = addr->addr_lo;
			return 0;
		}
	}
	for (u64 i = 0; i < sizeof(static_addrs) / sizeof(static_addrs[0]); i++) {
		if (static_addrs[i].family == BUNIX_NET_ADDR_FAMILY_IPV6 &&
		    static_addrs[i].iface == iface_id &&
		    (static_addrs[i].addr_hi != 0 ||
		     static_addrs[i].addr_lo != 0)) {
			*hi = static_addrs[i].addr_hi;
			*lo = static_addrs[i].addr_lo;
			return 0;
		}
	}
	*hi = 0;
	*lo = 0;
	return -1;
}

static int packet_tx_enqueue_copy(struct net_interface *iface,
				  const unsigned char *data, u64 len)
{
	struct net_packet *packet;

	if (iface == 0 || iface == &loopback || data == 0 || len == 0 ||
	    len > iface->mtu ||
	    (iface->flags & BUNIX_NET_IFACE_FLAG_RUNNING) == 0) {
		if (iface != 0 && iface != &loopback) {
			iface->tx_drops++;
		}
		return -1;
	}
	packet = (struct net_packet *)bunix_alloc(sizeof(*packet) + len);
	if (packet == 0) {
		iface->tx_drops++;
		return -1;
	}
	packet->next = 0;
	packet->family = 0;
	packet->len = len;
	for (u64 i = 0; i < len; i++) {
		packet->data[i] = data[i];
	}
	if (iface->tx_tail != 0) {
		iface->tx_tail->next = packet;
	} else {
		iface->tx_head = packet;
	}
	iface->tx_tail = packet;
	return 0;
}

static int icmp_sendto_external_ipv4(struct icmp_socket *socket,
				     u64 dest_ip, unsigned char *data,
				     u64 len)
{
	const struct net_route *route;
	struct net_interface *iface;
	unsigned char *frame;
	u64 source_ip;
	u64 next_hop_ip;
	u64 dest_mac = 0;
	u64 frame_len;
	u64 ip_len;
	unsigned short ip_checksum;

	if (socket == 0 || socket->family != BUNIX_NET_ADDR_FAMILY_IPV4 ||
	    data == 0 || len < 8 || len > NET_PACKET_MAX) {
		return -1;
	}
	route = net_route_lookup(BUNIX_NET_ADDR_FAMILY_IPV4, 0, dest_ip);
	if (route == 0 || (route->flags & BUNIX_NET_ROUTE_FLAG_UP) == 0) {
		return -1;
	}
	iface = interface_find(route->iface);
	next_hop_ip = route->gateway_lo != 0 ? route->gateway_lo : dest_ip;
	{
		struct net_neighbor *neighbor =
			neighbor_find(BUNIX_NET_ADDR_FAMILY_IPV4, 0,
				      next_hop_ip, route->iface);

		if (neighbor != 0) {
			dest_mac = neighbor->mac_hi;
		}
	}
	source_ip = iface_ipv4_addr(route->iface);
	ip_len = 20 + len;
	frame_len = 14 + ip_len;
	if (iface == 0 || source_ip == 0 || ip_len > 0xffff ||
	    frame_len > NET_PACKET_MAX) {
		return -1;
	}
	frame = (unsigned char *)bunix_alloc(frame_len);
	if (frame == 0) {
		return -1;
	}
	for (u64 i = 0; i < 6; i++) {
		frame[i] = dest_mac != 0 ?
			   (unsigned char)((dest_mac >> ((5 - i) * 8)) & 0xff) :
			   0xff;
		frame[6 + i] = (unsigned char)((iface->mac_hi >> ((5 - i) * 8)) &
					       0xff);
	}
	frame[12] = 0x08;
	frame[13] = 0x00;
	frame[14] = 0x45;
	frame[15] = 0;
	net_write_be16(frame + 16, ip_len);
	net_write_be16(frame + 18, 0);
	net_write_be16(frame + 20, 0);
	frame[22] = 64;
	frame[23] = NET_PROTO_ICMP;
	frame[24] = 0;
	frame[25] = 0;
	net_write_be32(frame + 26, source_ip);
	net_write_be32(frame + 30, dest_ip);
	icmp_set_checksum(data, len, socket->family, 0, source_ip, 0, dest_ip);
	for (u64 i = 0; i < len; i++) {
		frame[34 + i] = data[i];
	}
	ip_checksum = net_checksum_finish(net_checksum_sum(frame + 14, 20, 0));
	frame[24] = (unsigned char)((ip_checksum >> 8) & 0xff);
	frame[25] = (unsigned char)(ip_checksum & 0xff);
	if (dest_mac == 0) {
		if (pending_frame_add(BUNIX_NET_ADDR_FAMILY_IPV4, 0,
				      next_hop_ip, route->iface, frame,
				      frame_len) != 0) {
			bunix_free(frame);
			return -1;
		}
		arp_request_ipv4(iface, source_ip, next_hop_ip);
		bunix_free(frame);
		return 0;
	}
	if (packet_tx_enqueue_copy(iface, frame, frame_len) != 0) {
		bunix_free(frame);
		return -1;
	}
	bunix_free(frame);
	return 0;
}

static int icmp_sendto_external_ipv6(struct icmp_socket *socket,
				     u64 dest_hi, u64 dest_lo,
				     unsigned char *data, u64 len)
{
	const struct net_route *route;
	struct net_interface *iface;
	unsigned char *frame;
	u64 source_hi = 0;
	u64 source_lo = 0;
	u64 next_hop_hi;
	u64 next_hop_lo;
	u64 dest_mac = 0;
	u64 frame_len;

	if (socket == 0 || socket->family != BUNIX_NET_ADDR_FAMILY_IPV6 ||
	    data == 0 || len < 8 || len > NET_PACKET_MAX ||
	    (dest_hi == 0 && dest_lo == 0)) {
		return -1;
	}
	route = net_route_lookup(BUNIX_NET_ADDR_FAMILY_IPV6, dest_hi,
				 dest_lo);
	if (route == 0 || (route->flags & BUNIX_NET_ROUTE_FLAG_UP) == 0) {
		return -1;
	}
	iface = interface_find(route->iface);
	next_hop_hi = (route->flags & BUNIX_NET_ROUTE_FLAG_GATEWAY) != 0 ?
		      route->gateway_hi : dest_hi;
	next_hop_lo = (route->flags & BUNIX_NET_ROUTE_FLAG_GATEWAY) != 0 ?
		      route->gateway_lo : dest_lo;
	frame_len = 14 + 40 + len;
	if (iface == 0 || iface_ipv6_addr(route->iface, &source_hi,
					  &source_lo) != 0 ||
	    iface->mac_hi == 0 || frame_len > NET_PACKET_MAX ||
	    (next_hop_hi == 0 && next_hop_lo == 0)) {
		return -1;
	}
	{
		struct net_neighbor *neighbor =
			neighbor_find(BUNIX_NET_ADDR_FAMILY_IPV6, next_hop_hi,
				      next_hop_lo, route->iface);

		if (neighbor != 0) {
			dest_mac = neighbor->mac_hi;
		}
	}
	frame = (unsigned char *)bunix_alloc(frame_len);
	if (frame == 0) {
		return -1;
	}
	for (u64 i = 0; i < 6; i++) {
		frame[i] = dest_mac != 0 ?
			   (unsigned char)((dest_mac >> ((5 - i) * 8)) & 0xff) :
			   0xff;
		frame[6 + i] = (unsigned char)((iface->mac_hi >>
						((5 - i) * 8)) & 0xff);
	}
	frame[12] = 0x86;
	frame[13] = 0xdd;
	ndp_write_ipv6_header(frame, len, NET_PROTO_ICMPV6, 64, source_hi,
			      source_lo, dest_hi, dest_lo);
	icmp_set_checksum(data, len, socket->family, source_hi, source_lo,
			  dest_hi, dest_lo);
	for (u64 i = 0; i < len; i++) {
		frame[54 + i] = data[i];
	}
	if (dest_mac == 0) {
		if (pending_frame_add(BUNIX_NET_ADDR_FAMILY_IPV6, next_hop_hi,
				      next_hop_lo, route->iface, frame,
				      frame_len) != 0) {
			bunix_free(frame);
			return -1;
		}
		ndp_request_ipv6(iface, source_hi, source_lo, next_hop_hi,
				 next_hop_lo);
		bunix_free(frame);
		return 0;
	}
	if (packet_tx_enqueue_copy(iface, frame, frame_len) != 0) {
		bunix_free(frame);
		return -1;
	}
	bunix_free(frame);
	return 0;
}

static void icmp_datagrams_free_all(struct icmp_socket *socket)
{
	struct icmp_datagram *datagram = socket->rx_head;

	while (datagram != 0) {
		struct icmp_datagram *next = datagram->next;

		bunix_free(datagram);
		datagram = next;
	}
	socket->rx_head = 0;
	socket->rx_tail = 0;
	socket->rx_len = 0;
}

static int icmp_enqueue(struct icmp_socket *socket, u64 family,
			u64 source_hi, u64 source_lo,
			const unsigned char *data, u64 len)
{
	struct icmp_datagram *datagram;

	if (socket == 0 || len == 0 || len > NET_PACKET_MAX) {
		return -1;
	}
	datagram = (struct icmp_datagram *)bunix_alloc(sizeof(*datagram) + len);
	if (datagram == 0) {
		return -1;
	}
	datagram->next = 0;
	datagram->family = family;
	datagram->source_hi = source_hi;
	datagram->source_lo = source_lo;
	datagram->len = len;
	for (u64 i = 0; i < len; i++) {
		datagram->data[i] = data[i];
	}
	if (socket->rx_tail != 0) {
		socket->rx_tail->next = datagram;
	} else {
		socket->rx_head = datagram;
	}
	socket->rx_tail = datagram;
	socket->rx_len++;
	return 0;
}

static void reply_icmp_open(struct bunix_msg *reply,
			    const struct bunix_msg *message)
{
	const u64 family = message->words[0];
	const u64 protocol = message->words[1];
	struct icmp_socket *socket;

	if (!((family == BUNIX_NET_ADDR_FAMILY_IPV4 &&
	       protocol == NET_PROTO_ICMP) ||
	      (family == BUNIX_NET_ADDR_FAMILY_IPV6 &&
	       protocol == NET_PROTO_ICMPV6))) {
		reply->words[0] = (u64)-1;
		return;
	}
	socket = (struct icmp_socket *)bunix_alloc(sizeof(*socket));
	if (socket == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	socket->next = icmp_sockets;
	socket->id = next_icmp_socket_id++;
	socket->family = family;
	socket->protocol = protocol;
	socket->rx_len = 0;
	socket->rx_head = 0;
	socket->rx_tail = 0;
	icmp_sockets = socket;
	reply->words[0] = 0;
	reply->words[1] = socket->id;
}

static void reply_icmp_sendto(struct bunix_msg *reply,
			      const struct bunix_msg *message)
{
	struct icmp_socket *socket = icmp_find(message->words[0]);
	const u64 len = message->words[1];
	struct net_addr dest;
	unsigned char *data;
	u64 source_hi = 0;
	u64 source_lo = socket != 0 &&
			socket->family == BUNIX_NET_ADDR_FAMILY_IPV4 ?
			0x7f000001 : 1;
	int echo_request;
	int local;

	if (socket == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	if (len < 8 || len > NET_PACKET_MAX) {
		reply->words[0] = (u64)-1;
		return;
	}
	if (message->cap == 0 ||
	    bunix_buffer_read(message->cap, 0, &dest, sizeof(dest)) != 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	if (dest.family != socket->family) {
		reply->words[0] = (u64)-1;
		return;
	}
	data = (unsigned char *)bunix_alloc(len);
	if (data == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	if (bunix_buffer_read(message->cap, sizeof(dest), data, len) != 0) {
		bunix_free(data);
		reply->words[0] = (u64)-1;
		return;
	}
	echo_request = (socket->family == BUNIX_NET_ADDR_FAMILY_IPV4 &&
			data[0] == 8) ||
		       (socket->family == BUNIX_NET_ADDR_FAMILY_IPV6 &&
			data[0] == 128);
	if (!echo_request) {
		bunix_free(data);
		reply->words[0] = (u64)-1;
		return;
	}
	local = net_addr_is_local(socket->family, dest.hi, dest.lo);
	if (!local && socket->family == BUNIX_NET_ADDR_FAMILY_IPV4) {
		if (icmp_sendto_external_ipv4(socket, dest.lo, data, len) != 0) {
			bunix_free(data);
			reply->words[0] = (u64)-1;
			return;
		}
		bunix_free(data);
		reply->words[0] = 0;
		reply->words[1] = len;
		return;
	}
	if (!local && socket->family == BUNIX_NET_ADDR_FAMILY_IPV6) {
		if (icmp_sendto_external_ipv6(socket, dest.hi, dest.lo, data,
					      len) != 0) {
			bunix_free(data);
			reply->words[0] = (u64)-1;
			return;
		}
		bunix_free(data);
		reply->words[0] = 0;
		reply->words[1] = len;
		return;
	}
	if (!local) {
		bunix_free(data);
		reply->words[0] = (u64)-1;
		return;
	}
	data[0] = socket->family == BUNIX_NET_ADDR_FAMILY_IPV4 ? 0 : 129;
	data[1] = 0;
	icmp_set_checksum(data, len, socket->family, dest.hi, dest.lo,
			  source_hi, source_lo);
	if (icmp_enqueue(socket, socket->family, dest.hi, dest.lo,
			 data, len) != 0) {
		bunix_free(data);
		reply->words[0] = (u64)-1;
		return;
	}
	bunix_free(data);
	loopback.tx_packets++;
	loopback.rx_packets++;
	reply->words[0] = 0;
	reply->words[1] = len;
}

static void reply_icmp_recv(struct bunix_msg *reply,
			    const struct bunix_msg *message)
{
	struct icmp_socket *socket = icmp_find(message->words[0]);
	const u64 max_len = message->words[1];
	struct icmp_datagram *datagram;
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
	reply->words[2] = datagram->source_hi;
	reply->words[3] = datagram->source_lo;
	bunix_free(datagram);
}

static void reply_icmp_poll(struct bunix_msg *reply,
			    const struct bunix_msg *message)
{
	struct icmp_socket *socket = icmp_find(message->words[0]);

	if (socket == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	reply->words[0] = 0;
	reply->words[1] = (socket->rx_head != 0 ? NET_ICMP_POLLIN : 0) |
			  NET_ICMP_POLLOUT;
}

static void reply_icmp_close(struct bunix_msg *reply,
			     const struct bunix_msg *message)
{
	struct icmp_socket **link = &icmp_sockets;

	while (*link != 0) {
		struct icmp_socket *socket = *link;

		if (socket->id == message->words[0]) {
			*link = socket->next;
			icmp_datagrams_free_all(socket);
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
		case BUNIX_NET_INTERFACE_AT:
			reply_interface_at(&reply, &message);
			break;
		case BUNIX_NET_INTERFACE_DETAILS:
			reply_interface_details(&reply, &message);
			break;
		case BUNIX_NET_INTERFACE_SET_FLAGS:
			reply_interface_set_flags(&reply, &message);
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
		case BUNIX_NET_UDP_SENDTO:
			reply_udp_sendto(&reply, &message);
			break;
		case BUNIX_NET_UDP_RECV:
			reply_udp_recv(&reply, &message);
			break;
		case BUNIX_NET_UDP_POLL:
			reply_udp_poll(&reply, &message);
			break;
		case BUNIX_NET_UDP_LOCAL:
			reply_udp_addr(&reply, &message, 0);
			break;
		case BUNIX_NET_UDP_PEER:
			reply_udp_addr(&reply, &message, 1);
			break;
		case BUNIX_NET_UDP_CLOSE:
			reply_udp_close(&reply, &message);
			break;
		case BUNIX_NET_TCP_OPEN:
			reply_tcp_open(&reply, &message);
			break;
		case BUNIX_NET_TCP_BIND:
			reply_tcp_bind(&reply, &message);
			break;
		case BUNIX_NET_TCP_LISTEN:
			reply_tcp_listen(&reply, &message);
			break;
		case BUNIX_NET_TCP_CONNECT:
			reply_tcp_connect(&reply, &message);
			break;
		case BUNIX_NET_TCP_ACCEPT:
			reply_tcp_accept(&reply, &message);
			break;
		case BUNIX_NET_TCP_WRITE:
			reply_tcp_write(&reply, &message);
			break;
		case BUNIX_NET_TCP_READ:
			reply_tcp_read(&reply, &message);
			break;
		case BUNIX_NET_TCP_SHUTDOWN:
			reply_tcp_shutdown(&reply, &message);
			break;
		case BUNIX_NET_TCP_CLOSE:
			reply_tcp_close(&reply, &message);
			break;
		case BUNIX_NET_TCP_POLL:
			reply_tcp_poll(&reply, &message);
			break;
		case BUNIX_NET_TCP_LOCAL:
			reply_tcp_addr(&reply, &message, 0);
			break;
		case BUNIX_NET_TCP_PEER:
			reply_tcp_addr(&reply, &message, 1);
			break;
		case BUNIX_NET_OBSERVE_SOCKET_COUNT:
			reply_observe_socket_count(&reply);
			break;
		case BUNIX_NET_OBSERVE_SOCKET_AT:
			reply_observe_socket_at(&reply, &message);
			break;
		case BUNIX_NET_PACKET_INTERFACE_ATTACH:
			reply_packet_interface_attach(&reply, &message);
			break;
		case BUNIX_NET_PACKET_INTERFACE_LINK:
			reply_packet_interface_link(&reply, &message);
			break;
		case BUNIX_NET_PACKET_RX_SUBMIT:
			reply_packet_rx_submit(&reply, &message);
			break;
		case BUNIX_NET_PACKET_RX_DEQUEUE:
			reply_packet_rx_dequeue(&reply, &message);
			break;
		case BUNIX_NET_PACKET_RX_POLL:
			reply_packet_rx_poll(&reply, &message);
			break;
		case BUNIX_NET_PACKET_TX_DEQUEUE:
			reply_packet_tx_dequeue(&reply, &message);
			break;
		case BUNIX_NET_PACKET_TX_COMPLETE:
			reply_packet_tx_complete(&reply, &message);
			break;
		case BUNIX_NET_PACKET_TX_ENQUEUE:
			reply_packet_tx_enqueue(&reply, &message);
			break;
		case BUNIX_NET_ADDR_ADD:
			reply_addr_add(&reply, &message);
			break;
		case BUNIX_NET_ADDR_DELETE:
			reply_addr_delete(&reply, &message);
			break;
		case BUNIX_NET_ADDR_COUNT:
			reply_addr_count(&reply);
			break;
		case BUNIX_NET_ADDR_AT:
			reply_addr_at(&reply, &message);
			break;
		case BUNIX_NET_ROUTE_ADD:
			reply_route_add(&reply, &message);
			break;
		case BUNIX_NET_ROUTE_DELETE:
			reply_route_delete(&reply, &message);
			break;
		case BUNIX_NET_ROUTE_COUNT:
			reply_route_count(&reply);
			break;
		case BUNIX_NET_ROUTE_AT:
			reply_route_at(&reply, &message);
			break;
		case BUNIX_NET_CONFIG_STATUS:
			reply_config_status(&reply, &message);
			break;
		case BUNIX_NET_DHCP4_LEASE_INSTALL:
			reply_dhcp4_lease_install(&reply, &message);
			break;
		case BUNIX_NET_NEIGHBOR_COUNT:
			reply_neighbor_count(&reply);
			break;
		case BUNIX_NET_NEIGHBOR_AT:
			reply_neighbor_at(&reply, &message);
			break;
		case BUNIX_NET_NEIGHBOR_ADD:
			reply_neighbor_add(&reply, &message);
			break;
		case BUNIX_NET_NEIGHBOR_DELETE:
			reply_neighbor_delete(&reply, &message);
			break;
		case BUNIX_NET_ICMP_OPEN:
			reply_icmp_open(&reply, &message);
			break;
		case BUNIX_NET_ICMP_SENDTO:
			reply_icmp_sendto(&reply, &message);
			break;
		case BUNIX_NET_ICMP_RECV:
			reply_icmp_recv(&reply, &message);
			break;
		case BUNIX_NET_ICMP_POLL:
			reply_icmp_poll(&reply, &message);
			break;
		case BUNIX_NET_ICMP_CLOSE:
			reply_icmp_close(&reply, &message);
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
