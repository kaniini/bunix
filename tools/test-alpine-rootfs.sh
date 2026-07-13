#!/bin/sh
set -eu

script_dir=$(CDPATH= cd "$(dirname "$0")" && pwd)
. "$script_dir/path-safety.sh"

run_id=$(date -u +%Y%m%dT%H%M%SZ)-$$
stage=${ALPINE_ROOTFS_TEST_STAGE:-${TMPDIR:-/tmp}/bunix-alpine-rootfs-test.$run_id}
out=${ALPINE_ROOTFS_TEST_IMAGE:-$stage/rootfs.sqfs}
artifact_dir=${ALPINE_ROOTFS_ARTIFACT_DIR:-build/alpine-rootfs-test}
root=$stage/root

fail() {
	echo "test-alpine-rootfs: $*" >&2
	exit 1
}

require_file() {
	[ -f "$1" ] || fail "missing file: $1"
}

reject_file() {
	[ ! -e "$1" ] || fail "unexpected file: $1"
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

reject_grep() {
	pattern=$1
	file=$2
	if grep -F "$pattern" "$file" >/dev/null; then
		fail "unexpected pattern '$pattern' in $file"
	fi
}

safe_rm_rf "$stage" "ALPINE_ROOTFS_TEST_STAGE"
safe_rm_rf "$artifact_dir" "ALPINE_ROOTFS_ARTIFACT_DIR"
mkdir -p "$stage" "$artifact_dir"

ALPINE_ROOTFS_STAGE="$stage" \
ALPINE_ROOTFS_ARTIFACT_DIR="$artifact_dir" \
ROOTFS_IMAGE_FORMAT=squashfs \
LOGIN_MODULE=${LOGIN_MODULE:-build/modules/login.user} \
STATIDTEST_MODULE=${STATIDTEST_MODULE:-build/modules/statidtest.user} \
	sh tools/build-alpine-rootfs.sh "$out" >/dev/null

require_file "$out"
require_file "$artifact_dir/manifest.txt"
require_file "$artifact_dir/openrc-reference-runlevels.tsv"
require_file "$artifact_dir/openrc-bunix-runlevels.tsv"
require_file "$artifact_dir/openrc-initd.tsv"
require_file "$artifact_dir/openrc-confd.tsv"
require_file "$artifact_dir/openrc-policy.tsv"
require_file "$artifact_dir/openrc-networking.stock"
require_file "$root/var/cache/rc/deptree"
require_file "$root/fastboot"

for runlevel in sysinit boot default nonetwork shutdown; do
	require_dir "$root/etc/runlevels/$runlevel"
done

require_grep "boot/networking	/etc/init.d/networking" \
	"$artifact_dir/openrc-bunix-runlevels.tsv"
require_grep "replace	inittab	getty	/bin/login" \
	"$artifact_dir/openrc-policy.tsv"

require_grep "networking" "$artifact_dir/openrc-initd.tsv"
require_grep "ifupdown-ng" "$artifact_dir/manifest.txt"
require_grep "alpine_networking_service=stock" \
	"$artifact_dir/manifest.txt"
require_file "$root/usr/share/udhcpc/default.script"
reject_file "$root/sbin/bunix-udhcpc-script"
if [ "$(readlink "$root/usr/share/udhcpc/default.script" 2>/dev/null || true)" = \
     "/sbin/bunix-udhcpc-script" ]; then
	fail "Alpine udhcpc default.script points at Bunix helper"
fi
reject_grep "bunix-udhcpc" "$root/usr/share/udhcpc/default.script"

require_grep "add	boot	networking	/etc/init.d/networking" \
	"$artifact_dir/openrc-policy.tsv"
require_grep "add	boot	localmount	/etc/init.d/localmount" \
	"$artifact_dir/openrc-policy.tsv"
require_grep "boot/localmount	/etc/init.d/localmount" \
	"$artifact_dir/openrc-bunix-runlevels.tsv"
require_grep "suppress	boot	modules	/etc/init.d/modules" \
	"$artifact_dir/openrc-policy.tsv"
require_grep "suppress	boot	hwdrivers	/etc/init.d/hwdrivers" \
	"$artifact_dir/openrc-policy.tsv"
require_grep "defer	sysinit	devfs	/etc/init.d/devfs" \
	"$artifact_dir/openrc-policy.tsv"
require_grep "openrc_policy=tools/alpine-openrc-runlevels.policy" \
	"$artifact_dir/manifest.txt"
require_grep "openrc_networking_stock_script=$artifact_dir/openrc-networking.stock" \
	"$artifact_dir/manifest.txt"
require_grep "#!/sbin/openrc-run" "$root/etc/init.d/networking"
cmp -s "$artifact_dir/openrc-networking.stock" "$root/etc/init.d/networking" ||
	fail "Alpine networking service was modified by rootfs generation"
reject_grep "start_networking()" "$root/etc/init.d/networking"
reject_grep "boot/modules	/etc/init.d/modules" \
	"$artifact_dir/openrc-bunix-runlevels.tsv"
reject_grep "boot/hwdrivers	/etc/init.d/hwdrivers" \
	"$artifact_dir/openrc-bunix-runlevels.tsv"
require_grep "#!/sbin/openrc-run" "$root/etc/init.d/devfs"
require_grep "#!/sbin/openrc-run" "$root/etc/init.d/procfs"
require_grep "#!/sbin/openrc-run" "$root/etc/init.d/sysfs"
reject_grep "Declare Bunix" "$root/etc/init.d/devfs"
reject_grep "Declare Bunix" "$root/etc/init.d/procfs"
reject_grep "Declare Bunix" "$root/etc/init.d/sysfs"
reject_grep "sysinit/devfs	/etc/init.d/devfs" \
	"$artifact_dir/openrc-bunix-runlevels.tsv"
reject_grep "sysinit/procfs	/etc/init.d/procfs" \
	"$artifact_dir/openrc-bunix-runlevels.tsv"
reject_grep "sysinit/sysfs	/etc/init.d/sysfs" \
	"$artifact_dir/openrc-bunix-runlevels.tsv"

require_grep "::sysinit:/sbin/openrc sysinit" "$root/etc/inittab"
require_grep "::sysinit:/sbin/openrc boot" "$root/etc/inittab"
require_grep "::wait:/sbin/openrc default" "$root/etc/inittab"
require_grep "ttyS0::respawn:/bin/login" "$root/etc/inittab"
require_grep "::shutdown:/sbin/openrc shutdown" "$root/etc/inittab"
reject_file "$root/sbin/bunix-openrc-step"
reject_file "$root/sbin/bunix-openrc-bringup"
reject_file "$root/sbin/bunix-openrc-sysinit"
reject_file "$root/sbin/bunix-openrc-default"
reject_grep "bunix-openrc-bringup" "$root/etc/inittab"
reject_grep "bunix-openrc-sysinit" "$root/etc/inittab"
reject_grep "bunix-openrc-default" "$root/etc/inittab"
reject_grep "bunix-openrc-step" "$root/etc/inittab"
require_grep "depinfo_0_service=" "$root/var/cache/rc/deptree"
require_grep "_ineed_" "$root/var/cache/rc/deptree"
reject_grep "::wait:/sbin/rc-service networking start" "$root/etc/inittab"
if grep -E '^[^#].*getty' "$root/etc/inittab" >/dev/null; then
	fail "active getty entry found in Bunix Alpine inittab"
fi

printf 'test-alpine-rootfs: openrc runlevel parity ok stage=%s artifact=%s\n' \
	"$stage" "$artifact_dir"
