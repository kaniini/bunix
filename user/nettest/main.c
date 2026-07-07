#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

static void say(const char *text)
{
	(void)write(1, text, strlen(text));
}

static void die(const char *text)
{
	say(text);
	_exit(1);
}

static void expect_inet_port(const char *label, const struct sockaddr_in *addr,
			     unsigned short port)
{
	if (addr->sin_family != AF_INET || ntohs(addr->sin_port) != port) {
		die(label);
	}
}

static void expect_inet6_port(const char *label,
			      const struct sockaddr_in6 *addr,
			      unsigned short port)
{
	if (addr->sin6_family != AF_INET6 || ntohs(addr->sin6_port) != port) {
		die(label);
	}
}

static void expect_socket_type(int fd, int type, const char *label)
{
	int value = 0;
	socklen_t len = sizeof(value);

	if (getsockopt(fd, SOL_SOCKET, SO_TYPE, &value, &len) != 0 ||
	    len != sizeof(value) || value != type) {
		die(label);
	}
}

static int contains(const char *haystack, const char *needle)
{
	const size_t needle_len = strlen(needle);

	if (needle_len == 0) {
		return 1;
	}
	for (size_t i = 0; haystack[i] != '\0'; i++) {
		if (strncmp(haystack + i, needle, needle_len) == 0) {
			return 1;
		}
	}
	return 0;
}

static void expect_file_contains(const char *path, const char *needle,
				 const char *label)
{
	char buffer[1024];
	int fd = open(path, O_RDONLY);
	ssize_t nread;

	if (fd < 0) {
		die(label);
	}
	nread = read(fd, buffer, sizeof(buffer) - 1);
	close(fd);
	if (nread <= 0) {
		die(label);
	}
	buffer[nread] = '\0';
	if (!contains(buffer, needle)) {
		die(label);
	}
}

static void udp_test(void)
{
	const char payload[] = "udp-sendto";
	const char msg_a[] = "udp-msg";
	const char msg_b[] = "!";
	char buffer[32];
	char msg_buffer_a[8];
	char msg_buffer_b[8];
	struct sockaddr_in addr;
	struct sockaddr_in check;
	struct msghdr msg;
	struct iovec iov[2];
	socklen_t check_len;
	int reuse = 1;
	int server = socket(AF_INET, SOCK_DGRAM, 0);
	int client = socket(AF_INET, SOCK_DGRAM, 0);
	int duplicate;
	ssize_t nread;

	if (server < 0 || client < 0) {
		die("nettest: udp socket failed\n");
	}
	expect_socket_type(server, SOCK_DGRAM, "nettest: udp getsockopt failed\n");
	if (setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &reuse,
		       sizeof(reuse)) != 0) {
		die("nettest: udp setsockopt failed\n");
	}
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(23456);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(server, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
		die("nettest: udp bind failed\n");
	}
	duplicate = socket(AF_INET, SOCK_DGRAM, 0);
	if (duplicate < 0) {
		die("nettest: udp duplicate socket failed\n");
	}
	errno = 0;
	if (bind(duplicate, (struct sockaddr *)&addr, sizeof(addr)) == 0 ||
	    errno != EADDRINUSE) {
		die("nettest: udp duplicate bind failed\n");
	}
	close(duplicate);
	memset(&check, 0, sizeof(check));
	check_len = sizeof(check);
	if (getsockname(server, (struct sockaddr *)&check, &check_len) != 0 ||
	    check_len != sizeof(check)) {
		die("nettest: udp getsockname failed\n");
	}
	expect_inet_port("nettest: udp getsockname port failed\n", &check,
			 23456);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	if (sendto(client, payload, sizeof(payload), 0,
		   (struct sockaddr *)&addr, sizeof(addr)) !=
	    (ssize_t)sizeof(payload)) {
		die("nettest: udp sendto failed\n");
	}
	nread = recvfrom(server, buffer, sizeof(buffer), 0, 0, 0);
	if (nread != (ssize_t)sizeof(payload) ||
	    memcmp(buffer, payload, sizeof(payload)) != 0) {
		die("nettest: udp recv failed\n");
	}
	memset(&msg, 0, sizeof(msg));
	iov[0].iov_base = (void *)msg_a;
	iov[0].iov_len = sizeof(msg_a) - 1;
	iov[1].iov_base = (void *)msg_b;
	iov[1].iov_len = sizeof(msg_b);
	msg.msg_name = &addr;
	msg.msg_namelen = sizeof(addr);
	msg.msg_iov = iov;
	msg.msg_iovlen = 2;
	if (sendmsg(client, &msg, 0) !=
	    (ssize_t)(sizeof(msg_a) - 1 + sizeof(msg_b))) {
		die("nettest: udp sendmsg failed\n");
	}
	memset(msg_buffer_a, 0, sizeof(msg_buffer_a));
	memset(msg_buffer_b, 0, sizeof(msg_buffer_b));
	memset(&msg, 0, sizeof(msg));
	iov[0].iov_base = msg_buffer_a;
	iov[0].iov_len = sizeof(msg_a) - 1;
	iov[1].iov_base = msg_buffer_b;
	iov[1].iov_len = sizeof(msg_b);
	msg.msg_iov = iov;
	msg.msg_iovlen = 2;
	if (recvmsg(server, &msg, 0) !=
	    (ssize_t)(sizeof(msg_a) - 1 + sizeof(msg_b)) ||
	    memcmp(msg_buffer_a, msg_a, sizeof(msg_a) - 1) != 0 ||
	    memcmp(msg_buffer_b, msg_b, sizeof(msg_b)) != 0) {
		die("nettest: udp recvmsg failed\n");
	}
	say("nettest: udp msg ok\n");
	close(client);
	close(server);
	say("nettest: udp ok\n");
}

static void udp6_test(void)
{
	const char payload[] = "udp6-sendto";
	char buffer[32];
	struct sockaddr_in6 addr;
	struct sockaddr_in6 check;
	socklen_t check_len;
	int server = socket(AF_INET6, SOCK_DGRAM, 0);
	int client = socket(AF_INET6, SOCK_DGRAM, 0);
	ssize_t nread;

	if (server < 0 || client < 0) {
		die("nettest: udp6 socket failed\n");
	}
	expect_socket_type(server, SOCK_DGRAM,
			   "nettest: udp6 getsockopt failed\n");
	memset(&addr, 0, sizeof(addr));
	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons(23458);
	addr.sin6_addr = in6addr_any;
	if (bind(server, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
		die("nettest: udp6 bind failed\n");
	}
	memset(&check, 0, sizeof(check));
	check_len = sizeof(check);
	if (getsockname(server, (struct sockaddr *)&check, &check_len) != 0 ||
	    check_len != sizeof(check)) {
		die("nettest: udp6 getsockname failed\n");
	}
	expect_inet6_port("nettest: udp6 getsockname port failed\n", &check,
			  23458);
	addr.sin6_addr = in6addr_loopback;
	if (sendto(client, payload, sizeof(payload), 0,
		   (struct sockaddr *)&addr, sizeof(addr)) !=
	    (ssize_t)sizeof(payload)) {
		die("nettest: udp6 sendto failed\n");
	}
	nread = recvfrom(server, buffer, sizeof(buffer), 0, 0, 0);
	if (nread != (ssize_t)sizeof(payload) ||
	    memcmp(buffer, payload, sizeof(payload)) != 0) {
		die("nettest: udp6 recv failed\n");
	}
	close(client);
	close(server);
	say("nettest: udp6 ok\n");
}

static void tcp_test(void)
{
	const char client_payload[] = "tcp-client";
	const char server_payload[] = "tcp-server";
	char buffer[32];
	struct sockaddr_in addr;
	struct sockaddr_in check;
	struct pollfd pfd;
	fd_set readfds;
	struct timeval timeout;
	socklen_t check_len;
	int reuse = 1;
	int listener = socket(AF_INET, SOCK_STREAM, 0);
	int client = socket(AF_INET, SOCK_STREAM, 0);
	int accepted;
	ssize_t nread;

	if (listener < 0 || client < 0) {
		die("nettest: tcp socket failed\n");
	}
	expect_socket_type(listener, SOCK_STREAM,
			   "nettest: tcp getsockopt failed\n");
	if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &reuse,
		       sizeof(reuse)) != 0) {
		die("nettest: tcp setsockopt failed\n");
	}
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(23457);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) != 0 ||
	    listen(listener, 4) != 0) {
		die("nettest: tcp listen failed\n");
	}
	memset(&check, 0, sizeof(check));
	check_len = sizeof(check);
	if (getsockname(listener, (struct sockaddr *)&check, &check_len) != 0 ||
	    check_len != sizeof(check)) {
		die("nettest: tcp getsockname failed\n");
	}
	expect_inet_port("nettest: tcp getsockname port failed\n", &check,
			 23457);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	if (connect(client, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
		die("nettest: tcp connect failed\n");
	}
	memset(&check, 0, sizeof(check));
	check_len = sizeof(check);
	if (getpeername(client, (struct sockaddr *)&check, &check_len) != 0 ||
	    check_len != sizeof(check)) {
		die("nettest: tcp getpeername client failed\n");
	}
	expect_inet_port("nettest: tcp getpeername client port failed\n",
			 &check, 23457);
	pfd.fd = listener;
	pfd.events = POLLIN;
	pfd.revents = 0;
	if (poll(&pfd, 1, 0) != 1 || (pfd.revents & POLLIN) == 0) {
		die("nettest: tcp poll failed\n");
	}
	FD_ZERO(&readfds);
	FD_SET(listener, &readfds);
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	if (select(listener + 1, &readfds, 0, 0, &timeout) != 1 ||
	    !FD_ISSET(listener, &readfds)) {
		die("nettest: tcp select failed\n");
	}
	say("nettest: tcp select ok\n");
	accepted = accept(listener, 0, 0);
	if (accepted < 0) {
		die("nettest: tcp accept failed\n");
	}
	memset(&check, 0, sizeof(check));
	check_len = sizeof(check);
	if (getpeername(accepted, (struct sockaddr *)&check, &check_len) != 0 ||
	    check_len != sizeof(check) || ntohs(check.sin_port) == 0) {
		die("nettest: tcp getpeername accepted failed\n");
	}
	if (write(client, client_payload, sizeof(client_payload)) !=
	    (ssize_t)sizeof(client_payload)) {
		die("nettest: tcp client write failed\n");
	}
	nread = read(accepted, buffer, sizeof(buffer));
	if (nread != (ssize_t)sizeof(client_payload) ||
	    memcmp(buffer, client_payload, sizeof(client_payload)) != 0) {
		die("nettest: tcp server read failed\n");
	}
	if (write(accepted, server_payload, sizeof(server_payload)) !=
	    (ssize_t)sizeof(server_payload)) {
		die("nettest: tcp server write failed\n");
	}
	nread = read(client, buffer, sizeof(buffer));
	if (nread != (ssize_t)sizeof(server_payload) ||
	    memcmp(buffer, server_payload, sizeof(server_payload)) != 0) {
		die("nettest: tcp client read failed\n");
	}
	if (shutdown(accepted, SHUT_WR) != 0 || read(client, buffer,
						    sizeof(buffer)) != 0) {
		die("nettest: tcp shutdown eof failed\n");
	}
	close(accepted);
	close(client);
	close(listener);
	say("nettest: tcp ok\n");
}

static void tcp6_test(void)
{
	const char client_payload[] = "tcp6-client";
	const char server_payload[] = "tcp6-server";
	char buffer[32];
	struct sockaddr_in6 addr;
	struct sockaddr_in6 check;
	struct pollfd pfd;
	socklen_t check_len;
	int listener = socket(AF_INET6, SOCK_STREAM, 0);
	int client = socket(AF_INET6, SOCK_STREAM, 0);
	int accepted;
	ssize_t nread;

	if (listener < 0 || client < 0) {
		die("nettest: tcp6 socket failed\n");
	}
	expect_socket_type(listener, SOCK_STREAM,
			   "nettest: tcp6 getsockopt failed\n");
	memset(&addr, 0, sizeof(addr));
	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons(23459);
	addr.sin6_addr = in6addr_any;
	if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) != 0 ||
	    listen(listener, 4) != 0) {
		die("nettest: tcp6 listen failed\n");
	}
	memset(&check, 0, sizeof(check));
	check_len = sizeof(check);
	if (getsockname(listener, (struct sockaddr *)&check, &check_len) != 0 ||
	    check_len != sizeof(check)) {
		die("nettest: tcp6 getsockname failed\n");
	}
	expect_inet6_port("nettest: tcp6 getsockname port failed\n", &check,
			  23459);
	addr.sin6_addr = in6addr_loopback;
	if (connect(client, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
		die("nettest: tcp6 connect failed\n");
	}
	memset(&check, 0, sizeof(check));
	check_len = sizeof(check);
	if (getpeername(client, (struct sockaddr *)&check, &check_len) != 0 ||
	    check_len != sizeof(check)) {
		die("nettest: tcp6 getpeername client failed\n");
	}
	expect_inet6_port("nettest: tcp6 getpeername client port failed\n",
			  &check, 23459);
	pfd.fd = listener;
	pfd.events = POLLIN;
	pfd.revents = 0;
	if (poll(&pfd, 1, 0) != 1 || (pfd.revents & POLLIN) == 0) {
		die("nettest: tcp6 poll failed\n");
	}
	accepted = accept(listener, 0, 0);
	if (accepted < 0) {
		die("nettest: tcp6 accept failed\n");
	}
	if (write(client, client_payload, sizeof(client_payload)) !=
	    (ssize_t)sizeof(client_payload)) {
		die("nettest: tcp6 client write failed\n");
	}
	nread = read(accepted, buffer, sizeof(buffer));
	if (nread != (ssize_t)sizeof(client_payload) ||
	    memcmp(buffer, client_payload, sizeof(client_payload)) != 0) {
		die("nettest: tcp6 server read failed\n");
	}
	if (write(accepted, server_payload, sizeof(server_payload)) !=
	    (ssize_t)sizeof(server_payload)) {
		die("nettest: tcp6 server write failed\n");
	}
	nread = read(client, buffer, sizeof(buffer));
	if (nread != (ssize_t)sizeof(server_payload) ||
	    memcmp(buffer, server_payload, sizeof(server_payload)) != 0) {
		die("nettest: tcp6 client read failed\n");
	}
	close(accepted);
	close(client);
	close(listener);
	say("nettest: tcp6 ok\n");
}

static void proc_net_test(void)
{
	expect_file_contains("/proc/net/dev", "lo:",
			     "nettest: proc net dev failed\n");
	expect_file_contains("/proc/net/route", "0000007F",
			     "nettest: proc net route failed\n");
	expect_file_contains("/proc/net/config", "loopback 1",
			     "nettest: proc net config failed\n");
	expect_file_contains("/proc/net/sockstat", "sockets: used",
			     "nettest: proc net sockstat failed\n");
	expect_file_contains("/proc/net/udp", "local_address",
			     "nettest: proc net udp failed\n");
	expect_file_contains("/proc/net/udp6", "local_address",
			     "nettest: proc net udp6 failed\n");
	expect_file_contains("/proc/net/tcp", "local_address",
			     "nettest: proc net tcp failed\n");
	expect_file_contains("/proc/net/tcp6", "local_address",
			     "nettest: proc net tcp6 failed\n");
	say("nettest: proc net ok\n");
}

static void edge_test(void)
{
	struct sockaddr_in addr;
	int fd;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		die("nettest: refused socket failed\n");
	}
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(23999);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	errno = 0;
	if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == 0 ||
	    errno != ECONNREFUSED) {
		die("nettest: refused connect failed\n");
	}
	close(fd);

	if (setuid(1000) != 0) {
		die("nettest: setuid failed\n");
	}
	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		die("nettest: low port socket failed\n");
	}
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(80);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	errno = 0;
	if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == 0 ||
	    errno != EACCES) {
		die("nettest: low port permission failed\n");
	}
	close(fd);
	say("nettest: edges ok\n");
}

int main(void)
{
	udp_test();
	udp6_test();
	tcp_test();
	tcp6_test();
	proc_net_test();
	edge_test();
	say("nettest: ok\n");
	return 0;
}
