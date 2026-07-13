#!/bin/sh

C=/proc/net/config
DNS=/tmp/bunix-dns.out
WGET=/tmp/bunix-wget.out
failed=0

echo "BUNIX_NET_DNS_WGET_BEGIN"
test -e /run/openrc/started/networking || failed=1
rc-service networking status || failed=1
cat "$C" || failed=1
busybox grep -F "iface eth0" "$C" || failed=1
busybox grep -F "default_ipv4 1" "$C" || failed=1
test -s /etc/resolv.conf || failed=1
cat /etc/resolv.conf || failed=1
busybox nslookup example.com >"$DNS" 2>&1 || failed=1
cat "$DNS"
busybox grep -F "Name:" "$DNS" || failed=1
busybox grep -F "Address" "$DNS" || failed=1
busybox wget -O "$WGET" http://example.com || failed=1
test -s "$WGET" || failed=1
busybox grep -i "example" "$WGET" || failed=1
if [ "$failed" = 0 ]; then echo "BUNIX_NET_DNS_WGET_OK"; fi
test "$failed" = 0
