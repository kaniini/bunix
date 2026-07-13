#!/bin/sh

C=/proc/net/config
OUT=/tmp/bunix-external-ping.out
failed=0

echo "BUNIX_NET_EXT_PING_BEGIN"
test -e /run/openrc/started/networking || failed=1
rc-service networking status || failed=1
cat "$C" || failed=1
busybox grep -F "iface eth0" "$C" || failed=1
busybox grep -F "default_ipv4 1" "$C" || failed=1
busybox ping -c 4 -W 4 4.2.2.1 >"$OUT" 2>&1 || failed=1
cat "$OUT"
if busybox grep -F "bytes from 0.0.0.0:" "$OUT"; then echo "BUNIX_NET_EXT_PING_BAD_SOURCE"; failed=1; else echo "BUNIX_NET_EXT_PING_SOURCE_OK"; fi
busybox grep -F "64 bytes from 4.2.2.1:" "$OUT" || failed=1
busybox grep -F "4 packets transmitted, 4 packets received" "$OUT" || failed=1
busybox sed -n 's/.*ttl=\([0-9][0-9]*\).*/\1/p' "$OUT" >/tmp/bunix-external-ping.ttl
ttl_count=$(busybox wc -l </tmp/bunix-external-ping.ttl)
first_ttl=$(busybox sed -n '1p' /tmp/bunix-external-ping.ttl)
bad_ttl=$(busybox awk '$1 < 1 || $1 > 255 { print $1; exit }' /tmp/bunix-external-ping.ttl)
drift_ttl=$(busybox awk -v first="$first_ttl" '$1 != first { print $1; exit }' /tmp/bunix-external-ping.ttl)
if [ -n "$bad_ttl" ]; then echo "BUNIX_NET_EXT_PING_BAD_TTL=$bad_ttl"; failed=1; fi
if [ -n "$drift_ttl" ]; then echo "BUNIX_NET_EXT_PING_TTL_DRIFT first=$first_ttl ttl=$drift_ttl"; failed=1; fi
if [ "$ttl_count" != 4 ]; then echo "BUNIX_NET_EXT_PING_TTL_COUNT=$ttl_count"; failed=1; fi
if [ "$failed" = 0 ]; then echo "BUNIX_NET_EXT_PING_OK"; fi
test "$failed" = 0
