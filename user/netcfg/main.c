#include <bunix/libbunix.h>

enum {
	NETCFG_HANDLE_NAMES = 3,
	NETCFG_MAX_CONFIG = 4096,
	NETCFG_IFACE_LO = 1,
	NETCFG_QEMU_USER_IPV4 = 0x0a00020full,
	NETCFG_QEMU_USER_GW = 0x0a000202ull,
};

static u64 str_len(const char *text)
{
	u64 len = 0;

	while (text[len] != '\0') {
		len++;
	}
	return len;
}

static int str_eq_n(const char *left, const char *right, u64 len)
{
	for (u64 i = 0; i < len; i++) {
		if (left[i] != right[i]) {
			return 0;
		}
	}
	return right[len] == '\0';
}

static void log_text(const char *text)
{
	bunix_console_log(text, str_len(text));
}

static u64 resolve_service(u64 service, unsigned int rights)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_NAMES,
		.type = BUNIX_NAMES_WAIT,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { BUNIX_NAMES_ROOT, service, rights, 0 },
	};
	struct bunix_msg reply;

	if (bunix_ipc_call(NETCFG_HANDLE_NAMES, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return 0;
	}
	return reply.cap;
}

static long register_service(u64 service, u64 handle)
{
	(void)service;
	return bunix_names_register_claim(bunix_handle_find(BUNIX_CAP_CLAM),
					  handle);
}

static long vfs_read_text(u64 vfs, const char *path, char *out, u64 out_size)
{
	const char cwd[] = "/";
	const u64 cwd_len = sizeof(cwd);
	const u64 path_len = path != 0 ? str_len(path) + 1 : 0;
	const long path_buffer =
		path_len != 0 ? bunix_buffer_create(cwd_len + path_len) : -1;
	const long io_buffer =
		out_size != 0 ? bunix_buffer_create(out_size) : -1;
	struct bunix_msg open_request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_OPEN_BUFFER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_RECV,
		.reply = 0,
		.cap = path_buffer > 0 ? (u64)path_buffer : 0,
		.words = { cwd_len, path_len, 0, 0 },
	};
	struct bunix_msg read_request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_READ_FILE_BUFFER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP,
		.reply = 0,
		.cap = io_buffer > 0 ? (u64)io_buffer : 0,
		.words = { 0, 0, out_size != 0 ? out_size - 1 : 0, 0 },
	};
	struct bunix_msg close_request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_CLOSE,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { 0, 0, 0, 0 },
	};
	struct bunix_msg reply;
	u64 read_len;

	if (vfs == 0 || path == 0 || out == 0 || out_size == 0 ||
	    path_buffer <= 0 || io_buffer <= 0 ||
	    bunix_buffer_write((u64)path_buffer, 0, cwd, cwd_len) != 0 ||
	    bunix_buffer_write((u64)path_buffer, cwd_len, path, path_len) != 0) {
		if (path_buffer > 0) {
			bunix_handle_close((u64)path_buffer);
		}
		if (io_buffer > 0) {
			bunix_handle_close((u64)io_buffer);
		}
		return -1;
	}
	if (bunix_ipc_call(vfs, &open_request, &reply) != 0 ||
	    reply.words[0] != 0 || reply.words[1] == 0 ||
	    reply.words[3] != BUNIX_VFS_TYPE_REGULAR) {
		bunix_handle_close((u64)path_buffer);
		bunix_handle_close((u64)io_buffer);
		return -1;
	}
	bunix_handle_close((u64)path_buffer);
	read_request.words[0] = reply.words[1];
	if (reply.words[2] < read_request.words[2]) {
		read_request.words[2] = reply.words[2];
	}
	if (bunix_ipc_call(vfs, &read_request, &reply) != 0 ||
	    reply.words[0] != 0 || reply.words[1] >= out_size ||
	    bunix_buffer_read((u64)io_buffer, 0, out, reply.words[1]) != 0) {
		close_request.words[0] = read_request.words[0];
		(void)bunix_ipc_call(vfs, &close_request, &reply);
		bunix_handle_close((u64)io_buffer);
		return -1;
	}
	read_len = reply.words[1];
	close_request.words[0] = read_request.words[0];
	if (bunix_ipc_call(vfs, &close_request, &reply) != 0 ||
	    reply.words[0] != 0) {
		bunix_handle_close((u64)io_buffer);
		return -1;
	}
	out[read_len] = '\0';
	bunix_handle_close((u64)io_buffer);
	return 0;
}

static int line_eq(const char *line, u64 len, const char *match)
{
	while (len != 0 && (line[0] == ' ' || line[0] == '\t')) {
		line++;
		len--;
	}
	while (len != 0 &&
	       (line[len - 1] == ' ' || line[len - 1] == '\t' ||
		line[len - 1] == '\r')) {
		len--;
	}
	return str_eq_n(line, match, len);
}

static void parse_interfaces(const char *config, int *lo4, int *lo6, int *eth0)
{
	const char *line = config;
	u64 len = 0;

	*lo4 = 0;
	*lo6 = 0;
	*eth0 = 0;
	for (u64 i = 0;; i++) {
		const char c = config[i];

		if (c != '\n' && c != '\0') {
			len++;
			continue;
		}
		if (line_eq(line, len, "iface lo inet loopback")) {
			*lo4 = 1;
		} else if (line_eq(line, len, "iface lo inet6 loopback")) {
			*lo6 = 1;
		} else if (line_eq(line, len, "iface eth0 inet dhcp")) {
			*eth0 = 1;
		}
		if (c == '\0') {
			break;
		}
		line = &config[i + 1];
		len = 0;
	}
}

static int net_call_buffer(u64 net, u64 type, u64 cap, unsigned int rights,
			   u64 word0, struct bunix_msg *reply)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_NET,
		.type = (unsigned int)type,
		.sender = 0,
		.cap_rights = rights,
		.reply = 0,
		.cap = cap,
		.words = { word0, 0, 0, 0 },
	};

	return bunix_ipc_call(net, &request, reply) == 0 &&
		       reply->words[0] == 0 ?
	       0 :
	       -1;
}

static int add_addr(u64 net, const struct bunix_net_addr_info *addr)
{
	struct bunix_msg reply;
	long buffer = bunix_buffer_create(sizeof(*addr));
	int result;

	if (buffer <= 0) {
		return -1;
	}
	result = bunix_buffer_write((u64)buffer, 0, addr, sizeof(*addr)) == 0 &&
			 net_call_buffer(net, BUNIX_NET_ADDR_ADD, (u64)buffer,
					 BUNIX_RIGHT_RECV, 0, &reply) == 0 ?
		 0 :
		 -1;
	bunix_handle_close((u64)buffer);
	return result;
}

static int configure_loopback(u64 net, int want4, int want6)
{
	const struct bunix_net_addr_info lo4 = {
		.family = BUNIX_NET_ADDR_FAMILY_IPV4,
		.addr_hi = 0,
		.addr_lo = 0x7f000001,
		.prefix_len = 8,
		.iface = NETCFG_IFACE_LO,
		.flags = 1,
	};
	const struct bunix_net_addr_info lo6 = {
		.family = BUNIX_NET_ADDR_FAMILY_IPV6,
		.addr_hi = 0,
		.addr_lo = 1,
		.prefix_len = 128,
		.iface = NETCFG_IFACE_LO,
		.flags = 1,
	};

	if ((want4 && add_addr(net, &lo4) != 0) ||
	    (want6 && add_addr(net, &lo6) != 0)) {
		return -1;
	}
	return 0;
}

static long first_packet_iface(u64 net)
{
	struct bunix_msg reply;
	u64 count;

	if (net_call_buffer(net, BUNIX_NET_INTERFACE_COUNT, 0, 0, 0,
			    &reply) != 0) {
		return -1;
	}
	count = reply.words[1];
	for (u64 i = 0; i < count; i++) {
		if (net_call_buffer(net, BUNIX_NET_INTERFACE_AT, 0, 0, i,
				    &reply) != 0) {
			return -1;
		}
		if (reply.words[1] != NETCFG_IFACE_LO &&
		    (reply.words[2] & BUNIX_NET_IFACE_FLAG_RUNNING) != 0) {
			return (long)reply.words[1];
		}
	}
	return 0;
}

static int install_qemu_user_lease(u64 net, u64 iface)
{
	struct bunix_net_dhcp4_lease lease = {
		.iface = iface,
		.address = NETCFG_QEMU_USER_IPV4,
		.prefix_len = 24,
		.gateway = NETCFG_QEMU_USER_GW,
		.dns0 = NETCFG_QEMU_USER_GW,
		.dns1 = 0,
		.lease_lifetime = 3600,
		.renewal_time = 1800,
		.server = NETCFG_QEMU_USER_GW,
	};
	struct bunix_msg reply;
	long buffer = bunix_buffer_create(sizeof(lease));
	int result;

	if (buffer <= 0) {
		return -1;
	}
	result = bunix_buffer_write((u64)buffer, 0, &lease, sizeof(lease)) == 0 &&
			 net_call_buffer(net, BUNIX_NET_DHCP4_LEASE_INSTALL,
					 (u64)buffer, BUNIX_RIGHT_RECV, 0,
					 &reply) == 0 ?
		 0 :
		 -1;
	bunix_handle_close((u64)buffer);
	return result;
}

static int log_status(u64 net)
{
	struct bunix_net_config_status status;
	struct bunix_msg reply;
	long buffer = bunix_buffer_create(sizeof(status));
	int result;

	if (buffer <= 0) {
		return -1;
	}
	result = net_call_buffer(net, BUNIX_NET_CONFIG_STATUS, (u64)buffer,
				 BUNIX_RIGHT_SEND, 0, &reply) == 0 &&
			 bunix_buffer_read((u64)buffer, 0, &status,
					   sizeof(status)) == 0 ?
		 0 :
		 -1;
	bunix_handle_close((u64)buffer);
	if (result != 0) {
		return -1;
	}
	if ((status.flags & BUNIX_NET_CONFIG_LOOPBACK) != 0) {
		log_text("netcfg: loopback configured\n");
	}
	if ((status.flags & BUNIX_NET_CONFIG_DEFAULT_IPV4) != 0) {
		log_text("netcfg: default ipv4 route configured\n");
	}
	return 0;
}

int main(void)
{
	static char config[NETCFG_MAX_CONFIG];
	u64 vfs;
	u64 net;
	int lo4;
	int lo6;
	int eth0;
	long iface;

	log_text("netcfg: online\n");
	vfs = resolve_service(BUNIX_SERVICE_VFS, BUNIX_RIGHT_SEND);
	net = resolve_service(BUNIX_SERVICE_NET, BUNIX_RIGHT_SEND);
	if (vfs == 0 || net == 0) {
		log_text("netcfg: services missing\n");
		return 1;
	}
	if (vfs_read_text(vfs, "/etc/network/interfaces", config,
			  sizeof(config)) != 0) {
		log_text("netcfg: interfaces missing\n");
		return 1;
	}
	log_text("netcfg: interfaces loaded\n");
	parse_interfaces(config, &lo4, &lo6, &eth0);
	if (configure_loopback(net, lo4, lo6) != 0) {
		log_text("netcfg: loopback failed\n");
		return 1;
	}
	if (eth0) {
		iface = first_packet_iface(net);
		if (iface < 0) {
			log_text("netcfg: interface scan failed\n");
			return 1;
		}
		if (iface == 0) {
			log_text("netcfg: eth0 absent\n");
		} else if (install_qemu_user_lease(net, (u64)iface) != 0) {
			log_text("netcfg: dhcp fallback failed\n");
			return 1;
		} else {
			log_text("netcfg: dhcp fallback lease installed\n");
		}
	}
	if (log_status(net) != 0) {
		log_text("netcfg: status failed\n");
		return 1;
	}
	if (register_service(BUNIX_SERVICE_NETCFG, BUNIX_HANDLE_SELF) != 0) {
		log_text("netcfg: register failed\n");
		return 1;
	}
	log_text("netcfg: configured\n");
	for (;;) {
		struct bunix_msg message;
		struct bunix_msg reply = {
			.protocol = BUNIX_PROTO_NETCFG,
			.type = 0,
			.words = { 0, 0, 0, 0 },
		};

		if (bunix_ipc_recv(BUNIX_HANDLE_SELF, &message) != 0) {
			continue;
		}
		reply.type = message.type;
		if (message.reply != 0) {
			bunix_ipc_send(message.reply, &reply);
		}
	}
}
