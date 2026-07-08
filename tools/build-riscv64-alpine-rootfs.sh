#!/bin/sh
set -eu

out=${1:?output rootfs image required}
artifact_dir=${RISCV64_ALPINE_ROOTFS_ARTIFACT_DIR:-build/riscv64-alpine-rootfs}
repositories=${RISCV64_ALPINE_REPOSITORIES_FILE:-$artifact_dir/repositories}

mkdir -p "$artifact_dir"

if [ ! -r "$repositories" ]; then
	cat > "$repositories" <<'EOF_REPOSITORIES'
https://dl-cdn.alpinelinux.org/alpine/edge/main
https://dl-cdn.alpinelinux.org/alpine/edge/community
EOF_REPOSITORIES
fi

APK_ARCH=riscv64 \
APK_REPOSITORIES_FILE="$repositories" \
ALPINE_ROOTFS_ARTIFACT_DIR="$artifact_dir" \
ALPINE_ROOTFS_PACKAGES="${RISCV64_ALPINE_ROOTFS_PACKAGES:-alpine-baselayout busybox musl openrc ifupdown-ng}" \
BUNIX_ALPINE_OVERLAY=0 \
BUNIX_ALPINE_INIT_COMMAND="${RISCV64_ALPINE_INIT_COMMAND:-/bin/sh}" \
ROOTFS_IMAGE_FORMAT="${RISCV64_ALPINE_ROOTFS_IMAGE_FORMAT:-squashfs}" \
	sh tools/build-alpine-rootfs.sh "$out"
