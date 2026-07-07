#include <bunix/libbunix.h>

enum {
	AT_NULL = 0,
	LINUX_SYSCALL_WRITE = 1,
	LINUX_SYSCALL_EXIT_GROUP = 231,
	NETDHCP_IFACE_LO = 1,
};

struct startup_aux {
	u64 names_handle;
};

static long linux_syscall4(u64 number, u64 arg0, u64 arg1, u64 arg2, u64 arg3)
{
	long result;
	register u64 r10 __asm__("r10") = arg3;

	__asm__ volatile("syscall"
			 : "=a"(result)
			 : "a"(number), "D"(arg0), "S"(arg1), "d"(arg2),
			   "r"(r10)
			 : "rcx", "r11", "memory");
	return result;
}

static u64 text_len(const char *text)
{
	u64 len = 0;

	if (text == 0) {
		return 0;
	}
	while (text[len] != '\0') {
		len++;
	}
	return len;
}

static int text_eq(const char *left, const char *right)
{
	u64 i = 0;

	if (left == 0 || right == 0) {
		return 0;
	}
	for (;;) {
		if (left[i] != right[i]) {
			return 0;
		}
		if (left[i] == '\0') {
			return 1;
		}
		i++;
	}
}

static int starts_with_key(const char *text, const char *key)
{
	u64 i = 0;

	if (text == 0 || key == 0) {
		return 0;
	}
	while (key[i] != '\0') {
		if (text[i] != key[i]) {
			return 0;
		}
		i++;
	}
	return text[i] == '=';
}

static void write_fd(u64 fd, const char *text)
{
	(void)linux_syscall4(LINUX_SYSCALL_WRITE, fd, (u64)text,
			     text_len(text), 0);
}

static void exit_group(u64 status)
{
	(void)linux_syscall4(LINUX_SYSCALL_EXIT_GROUP, status, 0, 0, 0);
	for (;;) {
	}
}

static void load_auxv(char **envp, struct startup_aux *aux)
{
	u64 *entry;

	aux->names_handle = 0;
	while (*envp != 0) {
		envp++;
	}
	entry = (u64 *)(envp + 1);
	for (;;) {
		const u64 type = entry[0];
		const u64 value = entry[1];

		if (type == AT_NULL) {
			return;
		}
		if (type == BUNIX_AT_NAMES) {
			aux->names_handle = value;
		}
		entry += 2;
	}
}

static const char *find_env(char **envp, const char *key)
{
	for (u64 i = 0; envp[i] != 0; i++) {
		if (starts_with_key(envp[i], key)) {
			return envp[i] + text_len(key) + 1;
		}
	}
	return 0;
}

static int parse_uint(const char *text, u64 *out)
{
	u64 value = 0;
	u64 i = 0;

	if (text == 0 || out == 0 || text[0] == '\0') {
		return -1;
	}
	while (text[i] >= '0' && text[i] <= '9') {
		value = value * 10 + (u64)(text[i] - '0');
		i++;
	}
	if (i == 0 || (text[i] != '\0' && text[i] != ' ' &&
		       text[i] != '\t' && text[i] != '\n')) {
		return -1;
	}
	*out = value;
	return 0;
}

static int parse_ipv4(const char *text, u64 *out)
{
	u64 parts[4] = { 0, 0, 0, 0 };
	u64 part = 0;
	u64 value = 0;
	int have_digit = 0;

	if (text == 0 || out == 0) {
		return -1;
	}
	for (u64 i = 0;; i++) {
		const char c = text[i];

		if (c >= '0' && c <= '9') {
			have_digit = 1;
			value = value * 10 + (u64)(c - '0');
			if (value > 255) {
				return -1;
			}
			continue;
		}
		if (!have_digit || part >= 4) {
			return -1;
		}
		parts[part++] = value;
		value = 0;
		have_digit = 0;
		if (c == '.') {
			continue;
		}
		if (c == '\0' || c == ' ' || c == '\t' || c == '\n') {
			break;
		}
		return -1;
	}
	if (part != 4) {
		return -1;
	}
	*out = (parts[0] << 24) | (parts[1] << 16) |
	       (parts[2] << 8) | parts[3];
	return 0;
}

static u64 prefix_from_netmask(u64 mask)
{
	u64 prefix = 0;
	int zero_seen = 0;

	for (int bit = 31; bit >= 0; bit--) {
		const int set = (mask & (1ull << (u64)bit)) != 0;

		if (set && zero_seen) {
			return 0;
		}
		if (set) {
			prefix++;
		} else {
			zero_seen = 1;
		}
	}
	return prefix;
}

static int ifreq_eth_index(const char *name, u64 *out)
{
	u64 value = 0;
	u64 i = 3;

	if (name == 0 || out == 0 ||
	    name[0] != 'e' || name[1] != 't' || name[2] != 'h' ||
	    name[3] == '\0') {
		return 0;
	}
	while (name[i] >= '0' && name[i] <= '9') {
		value = value * 10 + (u64)(name[i] - '0');
		i++;
	}
	if (name[i] != '\0') {
		return 0;
	}
	*out = value;
	return 1;
}

static u64 resolve_service(u64 names, u64 service, unsigned int rights)
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

	if (names == 0 ||
	    bunix_ipc_call(names, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return 0;
	}
	return reply.cap;
}

static int net_call(u64 net, u64 type, u64 cap, unsigned int rights,
		    u64 word0, u64 word1, u64 word2, struct bunix_msg *reply)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_NET,
		.type = (unsigned int)type,
		.sender = 0,
		.cap_rights = rights,
		.reply = 0,
		.cap = cap,
		.words = { word0, word1, word2, 0 },
	};

	return net != 0 && reply != 0 &&
		       bunix_ipc_call(net, &request, reply) == 0 &&
		       reply->words[0] == 0 ?
	       0 :
	       -1;
}

static long interface_id_by_name(u64 net, const char *name)
{
	struct bunix_msg reply;
	u64 count;
	u64 target;
	u64 eth_index = 0;

	if (text_eq(name, "lo")) {
		return NETDHCP_IFACE_LO;
	}
	if (!ifreq_eth_index(name, &target) ||
	    net_call(net, BUNIX_NET_INTERFACE_COUNT, 0, 0, 0, 0, 0,
		     &reply) != 0) {
		return -1;
	}
	count = reply.words[1];
	for (u64 i = 0; i < count; i++) {
		if (net_call(net, BUNIX_NET_INTERFACE_AT, 0, 0, i, 0, 0,
			     &reply) != 0) {
			return -1;
		}
		if (reply.words[1] != NETDHCP_IFACE_LO) {
			if (eth_index == target) {
				return (long)reply.words[1];
			}
			eth_index++;
		}
	}
	return -1;
}

static int install_lease(u64 net, const struct bunix_net_dhcp4_lease *lease)
{
	struct bunix_msg reply;
	long buffer;
	int result;

	if (lease == 0) {
		return -1;
	}
	buffer = bunix_buffer_create(sizeof(*lease));
	if (buffer <= 0) {
		return -1;
	}
	result = bunix_buffer_write((u64)buffer, 0, lease,
				    sizeof(*lease)) == 0 &&
			 net_call(net, BUNIX_NET_DHCP4_LEASE_INSTALL,
				  (u64)buffer, BUNIX_RIGHT_RECV, 0, 0, 0,
				  &reply) == 0 ?
		 0 :
		 -1;
	bunix_handle_close((u64)buffer);
	return result;
}

int main(int argc, char **argv, char **envp)
{
	struct startup_aux aux;
	struct bunix_net_dhcp4_lease lease = { 0 };
	const char *action = argc > 1 ? argv[1] : "";
	const char *interface = find_env(envp, "interface");
	const char *ip = find_env(envp, "ip");
	const char *subnet = find_env(envp, "subnet");
	const char *router = find_env(envp, "router");
	const char *dns = find_env(envp, "dns");
	const char *lease_time = find_env(envp, "lease");
	const char *serverid = find_env(envp, "serverid");
	u64 names;
	u64 net;
	u64 mask;
	long iface;

	if (text_eq(action, "deconfig") || text_eq(action, "leasefail") ||
	    text_eq(action, "nak")) {
		write_fd(1, action);
		write_fd(1, "\n");
		exit_group(0);
	}
	if (!text_eq(action, "bound") && !text_eq(action, "renew")) {
		exit_group(0);
	}
	load_auxv(envp, &aux);
	names = aux.names_handle;
	net = resolve_service(names, BUNIX_SERVICE_NET, BUNIX_RIGHT_SEND);
	if (net == 0 || interface == 0 || ip == 0 || subnet == 0 ||
	    parse_ipv4(ip, &lease.address) != 0 ||
	    parse_ipv4(subnet, &mask) != 0) {
		write_fd(2, "bunix-udhcpc: missing lease data\n");
		exit_group(1);
	}
	iface = interface_id_by_name(net, interface);
	lease.prefix_len = prefix_from_netmask(mask);
	if (iface <= 0 || lease.prefix_len == 0) {
		write_fd(2, "bunix-udhcpc: bad interface or netmask\n");
		exit_group(1);
	}
	lease.iface = (u64)iface;
	if (router != 0) {
		(void)parse_ipv4(router, &lease.gateway);
	}
	if (dns != 0) {
		const char *second = dns;

		(void)parse_ipv4(dns, &lease.dns0);
		while (*second != '\0' && *second != ' ' && *second != '\t') {
			second++;
		}
		while (*second == ' ' || *second == '\t') {
			second++;
		}
		if (*second != '\0') {
			(void)parse_ipv4(second, &lease.dns1);
		}
	}
	if (lease_time == 0 || parse_uint(lease_time,
					  &lease.lease_lifetime) != 0) {
		lease.lease_lifetime = 3600;
	}
	lease.renewal_time = lease.lease_lifetime / 2;
	if (serverid != 0) {
		(void)parse_ipv4(serverid, &lease.server);
	}
	if (install_lease(net, &lease) != 0) {
		write_fd(2, "bunix-udhcpc: lease install failed\n");
		exit_group(1);
	}
	write_fd(1, action);
	write_fd(1, "\n");
	exit_group(0);
	return 0;
}
