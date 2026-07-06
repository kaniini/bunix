#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <string.h>
#include <sys/socket.h>
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

static void expect_socket_type(int fd, int type, const char *label)
{
	int value = 0;
	socklen_t len = sizeof(value);

	if (getsockopt(fd, SOL_SOCKET, SO_TYPE, &value, &len) != 0 ||
	    len != sizeof(value) || value != type) {
		die(label);
	}
}

static void udp_test(void)
{
	const char payload[] = "udp-sendto";
	char buffer[32];
	struct sockaddr_in addr;
	struct sockaddr_in check;
	socklen_t check_len;
	int reuse = 1;
	int server = socket(AF_INET, SOCK_DGRAM, 0);
	int client = socket(AF_INET, SOCK_DGRAM, 0);
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
	close(client);
	close(server);
	say("nettest: udp ok\n");
}

static void tcp_test(void)
{
	const char client_payload[] = "tcp-client";
	const char server_payload[] = "tcp-server";
	char buffer[32];
	struct sockaddr_in addr;
	struct sockaddr_in check;
	struct pollfd pfd;
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
	close(accepted);
	close(client);
	close(listener);
	say("nettest: tcp ok\n");
}

int main(void)
{
	udp_test();
	tcp_test();
	say("nettest: ok\n");
	return 0;
}
