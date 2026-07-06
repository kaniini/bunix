#!/bin/sh
set -eu

out=$1
tmp=${TMPDIR:-/tmp}/bunix-ext2-test.$$
root=$tmp/root

cleanup() {
	rm -rf "$tmp"
}
trap cleanup EXIT INT TERM

rm -rf "$tmp"
mkdir -p "$root/hard" "$root/names"

printf 'ext2 hello\n' > "$root/hello.txt"
ln "$root/hello.txt" "$root/hard/hello-hard"
ln -s /mnt/ext2/hello.txt "$root/link-to-hello"
printf 'long name ok\n' > "$root/names/long-name-abcdefghijklmnopqrstuvwxyz0123456789-abcdefghijklmnopqrstuvwxyz0123456789-abcdefghijklmnopqrstuvwxyz0123456789.txt"
truncate -s 8192 "$root/sparse-ish.bin"

rm -f "$out"
mkdir -p "$(dirname "$out")"
truncate -s 2M "$out"
/sbin/mke2fs -q -F -t ext2 -b 1024 -I 256 \
	-O filetype,sparse_super,large_file \
	-d "$root" "$out"
/sbin/e2fsck -fn "$out" >/dev/null
