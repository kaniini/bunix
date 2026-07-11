#!/bin/sh

C=/proc/net/config
OUT=/tmp/bunix-external-ping.out

echo "BUNIX_NET_EXT_PING_BEGIN"
rc-service networking status
cat "$C"
busybox grep -F "iface eth0" "$C"
busybox grep -F "default_ipv4 1" "$C"
busybox ping -c 4 -W 4 4.2.2.1 >"$OUT" 2>&1
cat "$OUT"
if busybox grep -F "bytes from 0.0.0.0:" "$OUT"; then
	echo "BUNIX_NET_EXT_PING_BAD_SOURCE"
	false
else
	echo "BUNIX_NET_EXT_PING_SOURCE_OK"
fi
busybox grep -F "64 bytes from 4.2.2.1:" "$OUT"
busybox grep -F "4 packets transmitted, 4 packets received" "$OUT"
ttl_count=0
first_ttl=
for ttl in $(busybox sed -n 's/.*ttl=\([0-9][0-9]*\).*/\1/p' "$OUT"); do
	ttl_count=$((ttl_count + 1))
	if [ "$ttl" -lt 1 ] || [ "$ttl" -gt 255 ]; then
		echo "BUNIX_NET_EXT_PING_BAD_TTL=$ttl"
		false
	fi
	if [ -z "$first_ttl" ]; then
		first_ttl=$ttl
	elif [ "$ttl" != "$first_ttl" ]; then
		echo "BUNIX_NET_EXT_PING_TTL_DRIFT first=$first_ttl ttl=$ttl"
		false
	fi
done
if [ "$ttl_count" != 4 ]; then
	echo "BUNIX_NET_EXT_PING_TTL_COUNT=$ttl_count"
	false
fi
echo "BUNIX_NET_EXT_PING_OK"
