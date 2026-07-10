#!/bin/sh
set -eu

script_dir=$(CDPATH= cd "$(dirname "$0")" && pwd)
. "$script_dir/path-safety.sh"

run_id=$(date -u +%Y%m%dT%H%M%SZ)-$$
stage=${RISCV64_ALPINE_ROOTFS_TEST_STAGE:-${TMPDIR:-/tmp}/bunix-riscv64-alpine-rootfs-test.$run_id}
out=${RISCV64_ALPINE_ROOTFS_TEST_IMAGE:-$stage/rootfs.sqfs}
artifact_dir=${RISCV64_ALPINE_ROOTFS_ARTIFACT_DIR:-${TMPDIR:-/tmp}/bunix-riscv64-alpine-rootfs-artifacts.$run_id}
root=$stage/root

fail() {
	echo "test-riscv64-alpine-rootfs: $*" >&2
	exit 1
}

require_file() {
	[ -f "$1" ] || fail "missing file: $1"
}

require_dir() {
	[ -d "$1" ] || fail "missing directory: $1"
}

require_grep() {
	pattern=$1
	file=$2
	grep -F "$pattern" "$file" >/dev/null ||
		fail "missing pattern '$pattern' in $file"
}

reject_file() {
	[ ! -e "$1" ] || fail "unexpected file: $1"
}

safe_rm_rf "$stage" "RISCV64_ALPINE_ROOTFS_TEST_STAGE"
safe_rm_rf "$artifact_dir" "RISCV64_ALPINE_ROOTFS_ARTIFACT_DIR"
mkdir -p "$stage" "$artifact_dir"

ALPINE_ROOTFS_STAGE="$stage" \
RISCV64_ALPINE_ROOTFS_ARTIFACT_DIR="$artifact_dir" \
	sh tools/build-riscv64-alpine-rootfs.sh "$out" >/dev/null

require_file "$out"
require_file "$artifact_dir/manifest.txt"
require_file "$artifact_dir/openrc-reference-runlevels.tsv"
require_file "$artifact_dir/openrc-bunix-runlevels.tsv"
require_file "$artifact_dir/openrc-initd.tsv"
require_file "$artifact_dir/openrc-confd.tsv"
require_file "$artifact_dir/openrc-policy.tsv"
require_file "$artifact_dir/repositories"

require_dir "$root/bin"
require_dir "$root/sbin"
require_dir "$root/etc/runlevels/default"
require_grep "apk_arch=riscv64" "$artifact_dir/manifest.txt"
require_grep "bunix_overlay=0" "$artifact_dir/manifest.txt"
require_grep "init_command=/bin/sh" "$artifact_dir/manifest.txt"
require_grep "busybox" "$artifact_dir/manifest.txt"
require_grep "openrc" "$artifact_dir/manifest.txt"
require_grep "ttyS0::respawn:/bin/sh" "$root/etc/inittab"
reject_file "$root/bin/statidtest"
reject_file "$root/sbin/bunix-udhcpc-script"
if [ -n "${RISCV64_DYN_HELLO_MODULE:-}" ] ||
   [ -n "${RISCV64_MUSL_LDSO:-}" ]; then
	require_file "$root/bin/dyn-hello"
	require_file "$root/lib/ld-musl-riscv64.so.1"
	require_grep "extra_dir=$artifact_dir/riscv64-overlay" \
		"$artifact_dir/manifest.txt"
else
	reject_file "$root/bin/dyn-hello"
fi

printf 'test-riscv64-alpine-rootfs: artifact ok stage=%s artifact=%s\n' \
	"$stage" "$artifact_dir"
