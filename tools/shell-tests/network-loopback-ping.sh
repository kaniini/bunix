#!/bin/sh

run_network_loopback_ping() {
	send_script_sync <<'EOF_NETWORK_LOOPBACK_PING'
busybox ping -c 1 127.0.0.1 >/tmp/ping4.out && busybox grep -a "1 packets received" /tmp/ping4.out && echo NETWORK_PING4_LOOPBACK_OK
busybox ping -6 -c 1 ::1 >/tmp/ping6.out && busybox grep -a "1 packets received" /tmp/ping6.out && echo NETWORK_PING6_LOOPBACK_OK
EOF_NETWORK_LOOPBACK_PING
}

check_network_loopback_ping() {
	wait_for_each_fixed "$log" "network loopback ping marker missing" 45 180 \
		NETWORK_PING4_LOOPBACK_OK NETWORK_PING6_LOOPBACK_OK
}
