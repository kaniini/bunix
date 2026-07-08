#!/bin/sh
set -eu

out=${1:?output image required}
stage=${TMPDIR:-/tmp}/bunix-riscv64-rootfs.$$

rm -rf "$stage"
mkdir -p "$stage/bin" "$stage/etc"
printf 'hello from riscv64 squashfs\n' > "$stage/hello.txt"
printf 'riscv64-smoke\n' > "$stage/etc/hostname"

mkdir -p "$(dirname "$out")"
mksquashfs "$stage" "$out" -noappend -no-compression -no-fragments \
	-no-exports -no-xattrs -all-root -root-mode 0755 -b 128K \
	-repro-time 0 >/dev/null
rm -rf "$stage"
