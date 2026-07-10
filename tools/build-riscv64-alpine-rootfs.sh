#!/bin/sh
set -eu

script_dir=$(CDPATH= cd "$(dirname "$0")" && pwd)
. "$script_dir/path-safety.sh"

out=${1:?output rootfs image required}
artifact_dir=${RISCV64_ALPINE_ROOTFS_ARTIFACT_DIR:-build/riscv64-alpine-rootfs}
repositories=${RISCV64_ALPINE_REPOSITORIES_FILE:-$artifact_dir/repositories}
overlay_dir=$artifact_dir/riscv64-overlay
default_extra_dir=

mkdir -p "$artifact_dir"

if [ ! -r "$repositories" ]; then
	cat > "$repositories" <<'EOF_REPOSITORIES'
https://dl-cdn.alpinelinux.org/alpine/edge/main
https://dl-cdn.alpinelinux.org/alpine/edge/community
EOF_REPOSITORIES
fi

safe_rm_rf "$overlay_dir" "riscv64 Alpine overlay directory"
if [ -n "${RISCV64_DYN_HELLO_MODULE:-}" ] ||
   [ -n "${RISCV64_MUSL_LDSO:-}" ]; then
	if [ ! -s "${RISCV64_DYN_HELLO_MODULE:-}" ]; then
		echo "missing RISCV64_DYN_HELLO_MODULE" >&2
		exit 2
	fi
	if [ ! -s "${RISCV64_MUSL_LDSO:-}" ]; then
		echo "missing RISCV64_MUSL_LDSO" >&2
		exit 2
	fi
	mkdir -p "$overlay_dir/bin" "$overlay_dir/lib"
	cp "$RISCV64_DYN_HELLO_MODULE" "$overlay_dir/bin/dyn-hello"
	cp "$RISCV64_MUSL_LDSO" "$overlay_dir/lib/ld-musl-riscv64.so.1"
	ln -sf /lib/ld-musl-riscv64.so.1 \
		"$overlay_dir/lib/libc.musl-riscv64.so.1"
	chmod 0555 "$overlay_dir/bin/dyn-hello" \
		"$overlay_dir/lib/ld-musl-riscv64.so.1"
	default_extra_dir=$overlay_dir
fi

APK_ARCH=riscv64 \
APK_REPOSITORIES_FILE="$repositories" \
ALPINE_ROOTFS_ARTIFACT_DIR="$artifact_dir" \
ALPINE_ROOTFS_PACKAGES="${RISCV64_ALPINE_ROOTFS_PACKAGES:-alpine-baselayout busybox musl openrc ifupdown-ng}" \
BUNIX_ALPINE_OVERLAY=0 \
BUNIX_ALPINE_INIT_COMMAND="${RISCV64_ALPINE_INIT_COMMAND:-/bin/sh}" \
BUNIX_ALPINE_EXTRA_DIR="${RISCV64_ALPINE_EXTRA_DIR:-$default_extra_dir}" \
ROOTFS_IMAGE_FORMAT="${RISCV64_ALPINE_ROOTFS_IMAGE_FORMAT:-squashfs}" \
	sh tools/build-alpine-rootfs.sh "$out"
