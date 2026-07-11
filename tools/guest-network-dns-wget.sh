#!/bin/sh

C=/proc/net/config
DNS=/tmp/bunix-dns.out
WGET=/tmp/bunix-wget.out

echo "BUNIX_NET_DNS_WGET_BEGIN"
rc-service networking status
cat "$C"
busybox grep -F "iface eth0" "$C"
busybox grep -F "default_ipv4 1" "$C"
test -s /etc/resolv.conf
cat /etc/resolv.conf
busybox nslookup example.com >"$DNS" 2>&1
cat "$DNS"
busybox grep -F "Name:" "$DNS"
busybox grep -F "Address" "$DNS"
busybox wget -O "$WGET" http://example.com
test -s "$WGET"
busybox grep -i "example" "$WGET"
echo "BUNIX_NET_DNS_WGET_OK"
