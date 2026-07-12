#!/bin/sh
set -eu

script_dir=$(CDPATH= cd "$(dirname "$0")" && pwd)
. "$script_dir/path-safety.sh"

out=${1:?output image required}
stage=${TMPDIR:-/tmp}/bunix-riscv64-rootfs.$$

safe_rm_rf "$stage" "riscv64 smoke rootfs scratch directory"
mkdir -p "$stage/bin" "$stage/etc" "$stage/lib"
printf 'hello from riscv64 squashfs\n' > "$stage/hello.txt"
printf 'riscv64-smoke\n' > "$stage/etc/hostname"

if [ -n "${RISCV64_DYN_HELLO_MODULE:-}" ]; then
	cp "$RISCV64_DYN_HELLO_MODULE" "$stage/bin/dyn-hello"
	chmod 0555 "$stage/bin/dyn-hello"
fi
if [ -n "${RISCV64_SYSCALL_SMOKE_MODULE:-}" ]; then
	cp "$RISCV64_SYSCALL_SMOKE_MODULE" "$stage/bin/rv64-syscall-smoke"
	chmod 0555 "$stage/bin/rv64-syscall-smoke"
fi
if [ -n "${RISCV64_USERMEMTEST_MODULE:-}" ]; then
	cp "$RISCV64_USERMEMTEST_MODULE" "$stage/bin/usermemtest"
	chmod 0555 "$stage/bin/usermemtest"
fi
if [ -n "${RISCV64_MUSL_HELLO_MODULE:-}" ]; then
	cp "$RISCV64_MUSL_HELLO_MODULE" "$stage/bin/musl-hello"
	chmod 0555 "$stage/bin/musl-hello"
fi
if [ -n "${RISCV64_MUSL_LDSO:-}" ]; then
	cp "$RISCV64_MUSL_LDSO" "$stage/lib/ld-musl-riscv64.so.1"
	chmod 0555 "$stage/lib/ld-musl-riscv64.so.1"
	ln -sf ld-musl-riscv64.so.1 "$stage/lib/libc.so"
fi

mkdir -p "$(dirname "$out")"
mksquashfs "$stage" "$out" -noappend -no-compression -no-fragments \
	-no-exports -no-xattrs -all-root -root-mode 0755 -b 128K \
	-repro-time 0 >/dev/null
safe_rm_rf "$stage" "riscv64 smoke rootfs scratch directory"
