#!/bin/sh

C=/proc/net/config
URL=${BUNIX_CACHEFLY_URL:-http://cachefly.cachefly.net/100mb.test}
MAX_SECONDS=${BUNIX_CACHEFLY_MAX_SECONDS:-15}
failed=0

echo "BUNIX_CACHEFLY_WGET_BEGIN"
test -e /run/openrc/started/networking || failed=1
rc-service networking status || failed=1
cat "$C" || failed=1
busybox grep -F "iface eth0" "$C" || failed=1
busybox grep -F "default_ipv4 1" "$C" || failed=1

start=$(busybox date +%s)
busybox wget -q -O/dev/null "$URL" || failed=1
end=$(busybox date +%s)
seconds=$((end - start))
echo "BUNIX_CACHEFLY_WGET_SECONDS=$seconds"
cat "$C" || failed=1
test "$seconds" -le "$MAX_SECONDS" || failed=1

if [ "$failed" = 0 ]; then
	echo "BUNIX_CACHEFLY_WGET_OK"
fi
test "$failed" = 0
