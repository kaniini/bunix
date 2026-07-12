#!/bin/sh
set -eu

script_dir=$(CDPATH= cd "$(dirname "$0")" && pwd)
. "$script_dir/path-safety.sh"

qemu=${QEMU:-qemu-system-x86_64}
ovmf=${OVMF_CODE:-/usr/share/OVMF/OVMF_CODE.fd}
rootfs=${RUN_PROFILE_ROOTFS_FLAVOR:-${ROOTFS_FLAVOR:-alpine-squashfs}}
esp=${RUN_PROFILE_ESP_DIR:-${ESP_DIR:-build/esp-virtio-net}}
target=${RUN_PROFILE_BUILD_TARGET:-$esp/EFI/BOOT/BOOTX64.EFI}
qemu_memory=${RUN_PROFILE_QEMU_MEMORY:-${QEMU_MEMORY:-128M}}
qemu_timeout=${RUN_PROFILE_QEMU_TIMEOUT:-${QEMU_TIMEOUT:-180s}}
qemu_extra_args=${RUN_PROFILE_QEMU_ARGS:-${RUN_QEMU_NET_ARGS:-}}
run_id=${RUN_PROFILE_RUN_ID:-$(date -u +%Y%m%dT%H%M%SZ)-$$}
out_dir=${RUN_PROFILE_OUT_DIR:-build/run-profile/$run_id}
tmp=${RUN_PROFILE_RUNTIME_DIR:-${TMPDIR:-/tmp}/bunix-run-profile.$run_id}
runtime_esp=$tmp/esp
pipe=$tmp/serial
serial_log=$out_dir/serial.log
qemu_log=$out_dir/qemu.log
events_log=$out_dir/events.tsv
prefix_log=$out_dir/serial-prefixes.tsv
summary_log=$out_dir/summary.txt
timeout_cmd=${TIMEOUT:-timeout}
qemu_pid=
cat_pid=
start_ms=

now_ms() {
	date +%s%3N
}

elapsed_ms() {
	now=$(now_ms)
	printf '%s\n' "$((now - start_ms))"
}

event() {
	name=$1
	detail=${2:-}
	now=$(now_ms)
	elapsed=$((now - start_ms))

	printf '%s\t%s\t%s\n' "$elapsed" "$name" "$detail" >> "$events_log"
	printf 'run-profile event=%s elapsed_ms=%s' "$name" "$elapsed"
	if [ -n "$detail" ]; then
		printf ' detail=%s' "$detail"
	fi
	printf '\n'
}

cleanup() {
	if [ "${qemu_pid:-}" ]; then
		kill "$qemu_pid" 2>/dev/null || true
	fi
	if [ "${cat_pid:-}" ]; then
		kill "$cat_pid" 2>/dev/null || true
	fi
	if [ "${RUN_PROFILE_KEEP_RUNTIME:-0}" != 1 ]; then
		safe_rm_rf "$tmp" "run-profile runtime directory"
	else
		printf 'run-profile runtime=%s\n' "$tmp"
	fi
}

fail_profile() {
	label=$1

	printf 'run-profile failed: %s\n' "$label" >&2
	printf 'run-profile artifacts=%s\n' "$out_dir" >&2
	if [ -r "$qemu_log" ]; then
		cat "$qemu_log" >&2 || true
	fi
	if [ -r "$serial_log" ]; then
		tail -n 160 "$serial_log" >&2 || true
	fi
	exit 1
}

wait_fixed() {
	pattern=$1
	name=$2
	limit=${3:-90}
	i=0

	while ! grep -aF "$pattern" "$serial_log" >/dev/null 2>&1; do
		i=$((i + 1))
		if [ "${qemu_pid:-}" ] && ! kill -0 "$qemu_pid" 2>/dev/null; then
			fail_profile "qemu exited while waiting for $name"
		fi
		if [ "$i" -gt "$limit" ]; then
			fail_profile "timed out waiting for $name"
		fi
		sleep 1
	done
	event "$name" "$pattern"
}

fixed_count() {
	grep -aF -c "$1" "$serial_log" 2>/dev/null || true
}

wait_count_gt() {
	pattern=$1
	before=$2
	name=$3
	limit=${4:-90}
	i=0

	while [ "$(fixed_count "$pattern")" -le "$before" ]; do
		i=$((i + 1))
		if [ "${qemu_pid:-}" ] && ! kill -0 "$qemu_pid" 2>/dev/null; then
			fail_profile "qemu exited while waiting for $name"
		fi
		if [ "$i" -gt "$limit" ]; then
			fail_profile "timed out waiting for $name"
		fi
		sleep 1
	done
	event "$name" "$pattern"
}

serial_prefix_counts() {
	awk '
	{
		line = $0
		gsub(/\r/, "", line)
		sub(/^\[[^]]+\] /, "", line)
		if (match(line, /^[A-Za-z0-9_-]+:/)) {
			prefix = substr(line, RSTART, RLENGTH - 1)
			count[prefix]++
		}
	}
	END {
		for (prefix in count)
			printf "%d\t%s\n", count[prefix], prefix
	}
	' "$serial_log" | sort -nr > "$prefix_log"
}

mkdir -p "$out_dir" "$tmp"
: > "$events_log"
: > "$qemu_log"
: > "$serial_log"
start_ms=$(now_ms)
trap cleanup EXIT INT TERM

event profile-start "rootfs=$rootfs"
event build-start "$target"
"${MAKE:-make}" ROOTFS_FLAVOR="$rootfs" "$target"
event build-done "$target"

if [ ! -d "$esp" ]; then
	fail_profile "ESP directory missing: $esp"
fi
if [ ! -r "$ovmf" ]; then
	fail_profile "OVMF image missing: $ovmf"
fi
mkfifo "$pipe.in" "$pipe.out"
safe_rm_rf "$runtime_esp" "run-profile runtime ESP"
mkdir -p "$runtime_esp"
cp -R "$esp/." "$runtime_esp/"
event runtime-esp-ready "$runtime_esp"

cat "$pipe.out" > "$serial_log" &
cat_pid=$!

event qemu-start "$qemu"
TMPDIR=$tmp "$timeout_cmd" "$qemu_timeout" "$qemu" -enable-kvm \
	-machine q35 -cpu host -m "$qemu_memory" \
	-smp "${SMP:-2}" \
	-drive if=pflash,format=raw,readonly=on,file="$ovmf" \
	-drive format=raw,file=fat:rw:"$runtime_esp" \
	$qemu_extra_args \
	-serial pipe:"$pipe" -display none -no-reboot 2>"$qemu_log" &
qemu_pid=$!

wait_fixed "bunixos:" kernel-start 60
wait_fixed "bootstrap: fs ready" fs-ready 120
if grep -aF "netcfg: configured" "$serial_log" >/dev/null 2>&1; then
	event netcfg-configured "netcfg: configured"
fi
wait_fixed "bootstrap: linux init exec" linux-init-exec 120
wait_fixed "login: " login-prompt 180

exec 3>"$pipe.in"
root_prompts_before=$(fixed_count "~ # ")
printf '%s\n%s\n' root root >&3
wait_count_gt "~ # " "$root_prompts_before" root-shell 120

shell_marker=__BUNIX_RUN_PROFILE_SHELL_OK__
printf 'echo %s\n' "$shell_marker" >&3
wait_fixed "$shell_marker" first-command 30

net_marker=__BUNIX_RUN_PROFILE_NET_DONE__
printf '/sbin/ip addr show eth0 >/dev/null 2>&1; echo %s=$?\n' "$net_marker" >&3
wait_fixed "$net_marker" network-command 30

printf '/bin/busybox poweroff -f\n' >&3
event poweroff-sent
if wait "$qemu_pid"; then
	qemu_pid=
	event qemu-exit
else
	status=$?
	qemu_pid=
	fail_profile "qemu exited with status $status"
fi

serial_prefix_counts
serial_lines=$(wc -l < "$serial_log" | tr -d ' ')
serial_bytes=$(wc -c < "$serial_log" | tr -d ' ')
total_ms=$(elapsed_ms)
{
	printf 'run_id=%s\n' "$run_id"
	printf 'rootfs=%s\n' "$rootfs"
	printf 'esp=%s\n' "$esp"
	printf 'total_ms=%s\n' "$total_ms"
	printf 'serial_lines=%s\n' "$serial_lines"
	printf 'serial_bytes=%s\n' "$serial_bytes"
	printf 'events=%s\n' "$events_log"
	printf 'serial_log=%s\n' "$serial_log"
	printf 'qemu_log=%s\n' "$qemu_log"
	printf 'prefixes=%s\n' "$prefix_log"
} > "$summary_log"

printf 'run-profile summary=%s total_ms=%s serial_lines=%s serial_bytes=%s\n' \
	"$summary_log" "$total_ms" "$serial_lines" "$serial_bytes"
awk 'NR <= 20 { printf "run-profile prefix_count=%s prefix=%s\n", $1, $2 }' \
	"$prefix_log"
