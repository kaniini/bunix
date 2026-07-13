#!/bin/sh
set -eu

script_dir=$(CDPATH= cd "$(dirname "$0")" && pwd)
. "$script_dir/path-safety.sh"

run_id=$(date -u +%Y%m%dT%H%M%SZ)-$$
stage=${ALPINE_ROOTFS_TEST_STAGE:-${TMPDIR:-/tmp}/bunix-alpine-rootfs-test.$run_id}
out=${ALPINE_ROOTFS_TEST_IMAGE:-$stage/rootfs.sqfs}
artifact_dir=${ALPINE_ROOTFS_ARTIFACT_DIR:-build/alpine-rootfs-test}
root=$stage/root
networking_service=${ALPINE_NETWORKING_SERVICE:-generated}

fail() {
	echo "test-alpine-rootfs: $*" >&2
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
NETDHCP_MODULE=${NETDHCP_MODULE:-build/modules/bunix-udhcpc-script.user} \
ALPINE_NETWORKING_SERVICE="$networking_service" \
	sh tools/build-alpine-rootfs.sh "$out" >/dev/null

require_file "$out"
require_file "$artifact_dir/manifest.txt"
require_file "$artifact_dir/openrc-reference-runlevels.tsv"
require_file "$artifact_dir/openrc-bunix-runlevels.tsv"
require_file "$artifact_dir/openrc-initd.tsv"
require_file "$artifact_dir/openrc-confd.tsv"
require_file "$artifact_dir/openrc-policy.tsv"
require_file "$root/var/cache/rc/deptree"

for runlevel in sysinit boot default nonetwork shutdown; do
	require_dir "$root/etc/runlevels/$runlevel"
done

require_grep "boot/networking	/etc/init.d/networking" \
	"$artifact_dir/openrc-bunix-runlevels.tsv"
require_grep "replace	inittab	getty	/bin/login" \
	"$artifact_dir/openrc-policy.tsv"

require_grep "networking" "$artifact_dir/openrc-initd.tsv"
require_grep "ifupdown-ng" "$artifact_dir/manifest.txt"
require_grep "alpine_networking_service=$networking_service" \
	"$artifact_dir/manifest.txt"

case "$networking_service" in
generated)
	require_grep "::sysinit:/sbin/bunix-openrc-bringup" "$root/etc/inittab"
	reject_grep "::sysinit:/sbin/bunix-openrc-boot" "$root/etc/inittab"
	reject_grep "::wait:/sbin/bunix-openrc-default" "$root/etc/inittab"
	require_grep "add	boot	networking	/etc/init.d/networking" \
		"$artifact_dir/openrc-policy.tsv"
	require_grep "openrc_policy=tools/alpine-generated-openrc-runlevels.policy" \
		"$artifact_dir/manifest.txt"
	require_grep "#!/bin/sh" "$root/etc/init.d/networking"
	require_grep "start_networking()" "$root/etc/init.d/networking"
	require_grep "mark_started" "$root/etc/init.d/networking"
	reject_grep 'ifup -i "$cfgfile" eth0' "$root/etc/init.d/networking"
	require_grep "/etc/init.d/networking start" "$root/sbin/bunix-openrc-bringup"
	reject_grep "sysinit/devfs	/etc/init.d/devfs" \
		"$artifact_dir/openrc-bunix-runlevels.tsv"
	reject_grep "sysinit/procfs	/etc/init.d/procfs" \
		"$artifact_dir/openrc-bunix-runlevels.tsv"
	reject_grep "sysinit/sysfs	/etc/init.d/sysfs" \
		"$artifact_dir/openrc-bunix-runlevels.tsv"
	;;
stock)
	require_grep "::sysinit:/sbin/bunix-openrc-sysinit" "$root/etc/inittab"
	require_grep "::sysinit:/sbin/bunix-openrc-step boot" "$root/etc/inittab"
	require_grep "::wait:/sbin/bunix-openrc-default" "$root/etc/inittab"
	require_grep "add	boot	networking	/etc/init.d/networking" \
		"$artifact_dir/openrc-policy.tsv"
	require_grep "suppress	boot	modules	/etc/init.d/modules" \
		"$artifact_dir/openrc-policy.tsv"
	require_grep "suppress	boot	hwdrivers	/etc/init.d/hwdrivers" \
		"$artifact_dir/openrc-policy.tsv"
	require_grep "defer	sysinit	devfs	/etc/init.d/devfs" \
		"$artifact_dir/openrc-policy.tsv"
	require_grep "openrc_policy=tools/alpine-openrc-runlevels.policy" \
		"$artifact_dir/manifest.txt"
	require_grep "#!/sbin/openrc-run" "$root/etc/init.d/networking"
	reject_grep "start_networking()" "$root/etc/init.d/networking"
	reject_grep "boot/modules	/etc/init.d/modules" \
		"$artifact_dir/openrc-bunix-runlevels.tsv"
	reject_grep "boot/hwdrivers	/etc/init.d/hwdrivers" \
		"$artifact_dir/openrc-bunix-runlevels.tsv"
	require_grep "provide dev dev-mount" "$root/etc/init.d/devfs"
	require_grep "Declare Bunix sysfs availability" "$root/etc/init.d/sysfs"
	require_grep "Declare Bunix procfs availability" "$root/etc/init.d/procfs"
	require_grep "sysinit/devfs	/etc/init.d/devfs" \
		"$artifact_dir/openrc-bunix-runlevels.tsv"
	require_grep "sysinit/procfs	/etc/init.d/procfs" \
		"$artifact_dir/openrc-bunix-runlevels.tsv"
	require_grep "sysinit/sysfs	/etc/init.d/sysfs" \
		"$artifact_dir/openrc-bunix-runlevels.tsv"
	;;
*)
	fail "unknown ALPINE_NETWORKING_SERVICE in test: $networking_service"
	;;
esac

require_grep "ttyS0::respawn:/bin/login" "$root/etc/inittab"
require_grep "::shutdown:/sbin/openrc shutdown" "$root/etc/inittab"
require_grep "/sbin/openrc" "$root/sbin/bunix-openrc-step"
require_grep "/proc/boot_timing" "$root/sbin/bunix-openrc-step"
require_grep "default >/run/openrc/softlevel" "$root/sbin/bunix-openrc-default"
require_grep "openrc-default-end" "$root/sbin/bunix-openrc-default"
require_grep "/var/cache/rc" "$root/sbin/bunix-openrc-sysinit"
require_grep "openrc-sysinit-end" "$root/sbin/bunix-openrc-sysinit"
require_grep "/var/cache/rc" "$root/sbin/bunix-openrc-bringup"
require_grep "openrc-boot-end" "$root/sbin/bunix-openrc-bringup"
require_grep "depinfo_0_service=" "$root/var/cache/rc/deptree"
require_grep "_ineed_" "$root/var/cache/rc/deptree"
reject_grep "::wait:/sbin/rc-service networking start" "$root/etc/inittab"
if grep -E '^[^#].*getty' "$root/etc/inittab" >/dev/null; then
	fail "active getty entry found in Bunix Alpine inittab"
fi

printf 'test-alpine-rootfs: openrc runlevel parity ok stage=%s artifact=%s\n' \
	"$stage" "$artifact_dir"
