#!/bin/sh

C=/proc/net/config
DNS=/tmp/bunix-apk-dns.out
LOG=/tmp/bunix-apk-add.out
REPOS=/tmp/bunix-apk.repositories
PKG=${BUNIX_APK_TEST_PACKAGE:-busybox-extras}
failed=0

if [ -x /sbin/apk ]; then
	APK=/sbin/apk
elif [ -x /bin/apk ]; then
	APK=/bin/apk
else
	echo "apk binary missing"
	exit 1
fi

echo "BUNIX_APK_INSTALL_BEGIN"
test -e /run/openrc/started/networking || failed=1
rc-service networking status || failed=1
cat "$C" || failed=1
busybox grep -F "iface eth0" "$C" || failed=1
busybox grep -F "default_ipv4 1" "$C" || failed=1
test -s /etc/resolv.conf || failed=1
test -s /etc/apk/repositories || failed=1
cat /etc/apk/repositories || failed=1
if ! busybox grep -F "/edge/main" /etc/apk/repositories >"$REPOS"; then
	echo "http://dl-cdn.alpinelinux.org/alpine/edge/main" >"$REPOS"
fi
cat "$REPOS"
test -d /etc/apk/keys || failed=1
ls /etc/apk/keys || failed=1

"$APK" --version || failed=1
"$APK" info >/tmp/bunix-apk-info.out 2>&1 || failed=1
cat /tmp/bunix-apk-info.out
"$APK" info -e apk-tools || failed=1
"$APK" info -e alpine-keys || failed=1

busybox nslookup dl-cdn.alpinelinux.org >"$DNS" 2>&1 || failed=1
cat "$DNS"
busybox grep -F "Name:" "$DNS" || failed=1
busybox grep -F "Address" "$DNS" || failed=1

"$APK" add --repositories-file "$REPOS" --no-cache --timeout 20 "$PKG" </dev/null >"$LOG" 2>&1 || failed=1
cat "$LOG"
"$APK" info -e "$PKG" || failed=1
"$APK" info "$PKG" || failed=1

if [ "$PKG" = "busybox-extras" ]; then
	test -x /bin/busybox-extras || failed=1
fi

if [ "$failed" = 0 ]; then
	echo "BUNIX_APK_INSTALL_OK"
fi
test "$failed" = 0
