#!/bin/sh
set -eu

script_dir=$(CDPATH= cd "$(dirname "$0")" && pwd)
. "$script_dir/path-safety.sh"

out=$1
tmp=${TMPDIR:-/tmp}/bunix-ext2-test.$$
root=$tmp/root

cleanup() {
	safe_rm_rf "$tmp" "ext2 test scratch directory"
}
trap cleanup EXIT INT TERM

safe_rm_rf "$tmp" "ext2 test scratch directory"
mkdir -p "$root/hard" "$root/names"

printf 'ext2 hello\n' > "$root/hello.txt"
ln "$root/hello.txt" "$root/hard/hello-hard"
ln -s /mnt/ext2/hello.txt "$root/link-to-hello"
printf 'long name ok\n' > "$root/names/long-name-abcdefghijklmnopqrstuvwxyz0123456789-abcdefghijklmnopqrstuvwxyz0123456789-abcdefghijklmnopqrstuvwxyz0123456789.txt"
truncate -s 8192 "$root/sparse-ish.bin"

safe_rm_f "$out" "ext2 test image"
mkdir -p "$(dirname "$out")"
truncate -s 2M "$out"
/sbin/mke2fs -q -F -t ext2 -b 1024 -I 256 \
	-O filetype,sparse_super,large_file \
	-d "$root" "$out"
/sbin/e2fsck -fn "$out" >/dev/null
