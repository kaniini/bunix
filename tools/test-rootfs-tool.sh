#!/bin/sh
set -eu

tool=${1:?mkrootfs tool required}
tmp=${TMPDIR:-/tmp}/bunix-rootfs-tool-test.$$
image=$tmp/rootfs.img
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

# shellcheck disable=SC2086
"$tool" "$image" $args --dir /empty --symlink /many/current /many/file-150.txt

entries=$(od -An -j4 -N4 -tu4 "$image" | tr -d ' ')
if [ "$entries" -le 128 ]; then
	echo "rootfs entry count did not exceed old static cap: $entries" >&2
	exit 1
fi

echo "rootfs tool regression ok entries=$entries"
