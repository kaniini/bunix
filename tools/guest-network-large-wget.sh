#!/bin/sh

C=/proc/net/config
URL=http://10.0.2.2:18080/large.bin
MAX_SECONDS=60
failed=0

echo "BUNIX_NET_LARGE_WGET_BEGIN"
test -e /run/openrc/started/networking || failed=1
rc-service networking status || failed=1
cat "$C" || failed=1
busybox grep -F "iface eth0" "$C" || failed=1
busybox grep -F "default_ipv4 1" "$C" || failed=1

start=$(busybox date +%s)
busybox wget -O/dev/null "$URL" || failed=1
end=$(busybox date +%s)
seconds=$((end - start))
echo "BUNIX_NET_LARGE_WGET_SECONDS=$seconds"
test "$seconds" -lt "$MAX_SECONDS" || failed=1

if [ "$failed" = 0 ]; then
	echo "BUNIX_NET_LARGE_WGET_OK"
fi
test "$failed" = 0
