#!/bin/sh

C=/proc/net/config
OUT=/tmp/bunix-wget-readv.out
URL=http://10.0.2.2:18080/readv.bin
EXPECTED=262144
failed=0

echo "BUNIX_NET_WGET_READV_BEGIN"
test -e /run/openrc/started/networking || failed=1
rc-service networking status || failed=1
cat "$C" || failed=1
busybox grep -F "iface eth0" "$C" || failed=1
busybox grep -F "default_ipv4 1" "$C" || failed=1
busybox wget -O/dev/null "$URL" || failed=1
busybox wget -O "$OUT" "$URL" || failed=1
size=$(busybox wc -c < "$OUT")
echo "BUNIX_NET_WGET_READV_SIZE=$size"
test "$size" = "$EXPECTED" || failed=1
busybox grep -aF "BUNIX_READV_HTTP_BEGIN" "$OUT" || failed=1
busybox grep -aF "BUNIX_READV_HTTP_END" "$OUT" || failed=1
if [ "$failed" = 0 ]; then echo "BUNIX_NET_WGET_READV_OK"; fi
test "$failed" = 0

