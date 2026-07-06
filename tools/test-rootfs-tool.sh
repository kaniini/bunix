#!/bin/sh
set -eu

tool=${1:?mkrootfs tool required}
tmp=${TMPDIR:-/tmp}/bunix-rootfs-tool-test.$$
image=$tmp/rootfs.img
tree_image=$tmp/tree-rootfs.img
args=

cleanup() {
	rm -rf "$tmp"
}
trap cleanup EXIT INT TERM

mkdir -p "$tmp/files"
for i in $(seq 1 150); do
	file=$tmp/files/file-$i.txt
	printf 'entry %s\n' "$i" > "$file"
	args="$args /many/file-$i.txt $file"
done
long_component=path-component-that-forces-the-rootfs-image-format-past-the-old-two-hundred-fifty-six-byte-path-field
long_path=/usr/share/bunix/alpine/very/long/rootfs/path/that/exceeds/the/old/two-hundred-fifty-six-byte/rootfs-entry-limit/$long_component/$long_component/$long_component/hello.txt
if [ "${#long_path}" -le 256 ]; then
	echo "test bug: long path is not longer than old rootfs path cap" >&2
	exit 1
fi
args="$args $long_path $tmp/files/file-1.txt"

# shellcheck disable=SC2086
"$tool" "$image" $args --dir /empty --symlink /many/current /many/file-150.txt

entries=$(od -An -j4 -N4 -tu4 "$image" | tr -d ' ')
if [ "$entries" -le 128 ]; then
	echo "rootfs entry count did not exceed old static cap: $entries" >&2
	exit 1
fi

mkdir -p "$tmp/tree/sbin" "$tmp/tree/etc" "$tmp/tree/lib"
printf '#!/bin/sh\necho tree-run\n' > "$tmp/tree/sbin/runme"
chmod 0755 "$tmp/tree/sbin/runme"
printf 'tree-conf\n' > "$tmp/tree/etc/tree.conf"
ln -s ../sbin/runme "$tmp/tree/lib/runme-link"

"$tool" "$tree_image" --tree "$tmp/tree"
tree_entries=$(od -An -j4 -N4 -tu4 "$tree_image" | tr -d ' ')
if [ "$tree_entries" -lt 6 ]; then
	echo "rootfs tree import did not add expected entries: $tree_entries" >&2
	exit 1
fi
if ! grep -aF "../sbin/runme" "$tree_image" >/dev/null 2>&1; then
	echo "rootfs tree import did not encode symlink target" >&2
	exit 1
fi

entry_size=4128
mode_offset=4120
runme_mode=
i=0
while [ "$i" -lt "$tree_entries" ]; do
	entry_offset=$((8 + i * entry_size))
	path=$(dd if="$tree_image" bs=1 skip="$entry_offset" count=4096 2>/dev/null | tr -d '\000')
	if [ "$path" = /sbin/runme ]; then
		runme_mode=$(od -An -j$((entry_offset + mode_offset)) -N4 -tu4 "$tree_image" | tr -d ' ')
		break
	fi
	i=$((i + 1))
done
if [ "$runme_mode" != 493 ]; then
	echo "rootfs tree import did not preserve executable mode: $runme_mode" >&2
	exit 1
fi

echo "rootfs tool regression ok entries=$entries tree_entries=$tree_entries long_path=${#long_path}"
