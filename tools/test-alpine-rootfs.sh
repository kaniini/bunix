#!/bin/sh
set -eu

image=${1:?alpine rootfs image required}

entry_size=4128
path_size=4096
offset_offset=4096
size_offset=4104
type_offset=4124

entry_count() {
	od -An -j4 -N4 -tu4 "$image" | tr -d ' '
}

entry_path() {
	i=$1
	dd if="$image" bs=1 skip=$((8 + i * entry_size)) count=$path_size 2>/dev/null |
		tr -d '\000'
}

entry_type() {
	i=$1
	od -An -j$((8 + i * entry_size + type_offset)) -N4 -tu4 "$image" |
		tr -d ' '
}

entry_offset() {
	i=$1
	od -An -j$((8 + i * entry_size + offset_offset)) -N8 -tu8 "$image" |
		tr -d ' '
}

entry_size_bytes() {
	i=$1
	od -An -j$((8 + i * entry_size + size_offset)) -N8 -tu8 "$image" |
		tr -d ' '
}

find_entry() {
	want=$1
	entries=$(entry_count)
	i=0
	while [ "$i" -lt "$entries" ]; do
		if [ "$(entry_path "$i")" = "$want" ]; then
			echo "$i"
			return 0
		fi
		i=$((i + 1))
	done
	return 1
}

require_entry() {
	path=$1
	type=$2
	index=$(find_entry "$path") || {
		echo "missing alpine rootfs entry: $path" >&2
		exit 1
	}
	actual=$(entry_type "$index")
	if [ "$actual" != "$type" ]; then
		echo "wrong alpine rootfs type for $path: $actual" >&2
		exit 1
	fi
}

require_symlink_target() {
	path=$1
	target=$2
	index=$(find_entry "$path") || {
		echo "missing alpine rootfs symlink: $path" >&2
		exit 1
	}
	actual_type=$(entry_type "$index")
	if [ "$actual_type" != 3 ]; then
		echo "wrong alpine rootfs symlink type for $path: $actual_type" >&2
		exit 1
	fi
	offset=$(entry_offset "$index")
	size=$(entry_size_bytes "$index")
	actual=$(dd if="$image" bs=1 skip="$offset" count="$size" 2>/dev/null)
	if [ "$actual" != "$target" ]; then
		echo "wrong alpine rootfs symlink target for $path: $actual" >&2
		exit 1
	fi
}

require_entry /etc/alpine-release 1
require_entry /bin/busybox 1
require_entry /bin/login 1
require_entry /sbin/openrc 1
require_entry /etc/rc.conf 1
require_entry /etc/inittab 1
require_entry /sbin/init 3
require_entry /etc/runlevels/default/bunix-login 3
require_symlink_target /sbin/init /bin/busybox
require_symlink_target /lib/libc.musl-x86_64.so.1 ld-musl-x86_64.so.1

if ! grep -aF "/bin/login" "$image" >/dev/null 2>&1; then
	echo "alpine rootfs inittab/login service does not reference Bunix login" >&2
	exit 1
fi
if ! grep -aF "/sbin/openrc" "$image" >/dev/null 2>&1; then
	echo "alpine rootfs does not reference OpenRC init commands" >&2
	exit 1
fi

echo "alpine rootfs smoke ok entries=$(entry_count)"
