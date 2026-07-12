#!/bin/sh
set -eu

script_dir=$(CDPATH= cd "$(dirname "$0")" && pwd)
. "$script_dir/path-safety.sh"

qemu=${QEMU:-qemu-system-x86_64}
ovmf=${OVMF_CODE:-/usr/share/OVMF/OVMF_CODE.fd}
rootfs=${RUN_PROFILE_ROOTFS_FLAVOR:-${ROOTFS_FLAVOR:-alpine-squashfs}}
esp=${RUN_PROFILE_ESP_DIR:-${ESP_DIR:-build/esp-virtio-net}}
target=${RUN_PROFILE_BUILD_TARGET:-$esp/EFI/BOOT/BOOTX64.EFI}
rootfs_target=${RUN_PROFILE_ROOTFS_TARGET:-}
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
phase_log=$out_dir/serial-phases.tsv
category_log=$out_dir/serial-categories.tsv
level_log=$out_dir/serial-levels.tsv
phase_prefix_log=$out_dir/serial-phase-prefixes.tsv
slowpath_log=$out_dir/serial-slowpath.log
summary_log=$out_dir/summary.txt
timeout_cmd=${TIMEOUT:-timeout}
qemu_pid=
cat_pid=
start_ms=

if [ -z "$rootfs_target" ]; then
	case "$rootfs" in
	alpine-squashfs)
		rootfs_target=build/modules/alpine-disk0.sqfs
		;;
	squashfs)
		rootfs_target=build/modules/disk0.sqfs
		;;
	esac
fi

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

serial_output_reports() {
	: > "$prefix_log"
	: > "$phase_log"
	: > "$category_log"
	: > "$level_log"
	: > "$phase_prefix_log"
	: > "$slowpath_log"

	awk '
	function starts(text, prefix)
	{
		return index(text, prefix) == 1
	}

	function strip_timestamp(text, out)
	{
		out = text
		sub(/^\[[^]]+\] /, "", out)
		return out
	}

	function line_prefix(text, out)
	{
		out = strip_timestamp(text)
		if (starts(out, "BdsDxe:"))
			return "firmware"
		if (out ~ /^   OpenRC /)
			return "openrc"
		if (out ~ /^ \* /)
			return "openrc"
		if (starts(out, "Service "))
			return "openrc-service"
		if (starts(out, "login:"))
			return "login"
		if (starts(out, "password:"))
			return "login"
		if (starts(out, "~ # "))
			return "shell"
		if (match(out, /^[A-Za-z0-9_-]+:/))
			return substr(out, RSTART, RLENGTH - 1)
		return "-"
	}

	function line_category(prefix, text)
	{
		if (prefix == "firmware")
			return "firmware"
		if (prefix == "openrc" || prefix == "openrc-service")
			return "openrc"
		if (prefix == "login" || prefix == "shell")
			return "login-shell"
		if (prefix == "bootstrap")
			return "bootstrap"
		if (prefix == "kernel" || prefix == "bunixos" ||
		    prefix == "multiboot2" || prefix == "elf" ||
		    prefix == "slab" || prefix == "pmm")
			return "kernel"
		if (prefix == "sched" || prefix == "smp" ||
		    prefix == "timer" || prefix == "irq" ||
		    prefix == "interrupts")
			return "sched-irq"
		if (prefix == "ipc" || prefix == "buffer")
			return "ipc-buffer"
		if (prefix == "vfs" || prefix == "squashfs" ||
		    prefix == "unionfs" || prefix == "procfs" ||
		    prefix == "tmpfs" || prefix == "devfs" ||
		    prefix == "sysfs" || prefix == "utmpfs" ||
		    prefix == "block")
			return "vfs-fs"
		if (prefix == "net" || prefix == "netcfg" ||
		    prefix == "virtio-net" || text ~ /udhcpc|eth0|networking/)
			return "networking"
		if (prefix == "pci" || prefix == "virtio-bus" ||
		    prefix == "usb-bus" || prefix == "xhci" ||
		    prefix == "input")
			return "drivers-bus"
		if (prefix == "linux" || prefix == "linux-server" ||
		    prefix == "linux-einval" || prefix == "linux-strace" ||
		    prefix == "linux-strace-enter" ||
		    prefix == "linux-debug" || prefix == "user-syscall" ||
		    prefix == "syscall" || prefix == "user")
			return "linux-compat"
		if (prefix == "names")
			return "names"
		if (prefix == "vm-server" || prefix == "vm" ||
		    prefix == "arch-vm")
			return "vm"
		if (prefix == "console")
			return "console"
		if (prefix == "power" || prefix == "machine")
			return "power"
		return "other"
	}

	function console_level(text)
	{
		text = strip_timestamp(text)
		if (starts(text, "interrupts: vector=") ||
		    starts(text, "bunixos: invalid") ||
		    starts(text, "arch-vm: failed") ||
		    starts(text, "elf: invalid") ||
		    starts(text, "elf: failed") ||
		    starts(text, "kernel: failed") ||
		    starts(text, "kernel: invalid") ||
		    starts(text, "kernel: too many") ||
		    starts(text, "kernel: no module") ||
		    starts(text, "sched: refusing") ||
		    starts(text, "sched: thread alloc failed") ||
		    starts(text, "sched: task alloc failed") ||
		    starts(text, "sched: handle table full") ||
		    starts(text, "sched: vma table full") ||
		    starts(text, "smp: ap start timeout") ||
		    starts(text, "smp: lapic delivery timeout") ||
		    starts(text, "syscall: unknown") ||
		    starts(text, "linux: unknown"))
			return "error"
		if (starts(text, "sched: cap denied") ||
		    starts(text, "sched: close denied") ||
		    starts(text, "sched: handle denied") ||
		    starts(text, "sched: inherit denied") ||
		    starts(text, "sched: task handle denied") ||
		    starts(text, "sched: buffer handle denied") ||
		    starts(text, "sched: vma overlap") ||
		    starts(text, "names: table full") ||
		    starts(text, "ipc: message alloc failed") ||
		    starts(text, "buffer: alloc failed"))
			return "warn"
		if (starts(text, "BdsDxe:"))
			return "firmware"
		if (text ~ /^   OpenRC / || text ~ /^ \* / ||
		    starts(text, "Service ") || starts(text, "login:") ||
		    starts(text, "password:") || starts(text, "~ # "))
			return "guest-stdout"
		if (text ~ /^[A-Za-z0-9_-]+:/)
			return "info"
		return "other"
	}

	{
		line = $0
		gsub(/\r/, "", line)
		if (phase == "")
			phase = "firmware"
		if (phase == "firmware" && index(line, "bunixos:") != 0)
			phase = "kernel-servers"

		line_phase = phase
		prefix = line_prefix(line)
		category = line_category(prefix, line)
		level = console_level(line)
		bytes = length(line) + 1

		prefix_count[prefix]++
		phase_lines[line_phase]++
		phase_bytes[line_phase] += bytes
		category_key = line_phase SUBSEP category
		category_lines[category_key]++
		category_bytes[category_key] += bytes
		level_key = line_phase SUBSEP level
		level_lines[level_key]++
		level_bytes[level_key] += bytes
		phase_prefix_key = line_phase SUBSEP prefix SUBSEP category SUBSEP level
		phase_prefix_lines[phase_prefix_key]++
		phase_prefix_bytes[phase_prefix_key] += bytes

		if (line_phase == "linux-init-to-login")
			print line > slowpath_log

		if (index(line, "bootstrap: linux init exec") != 0)
			phase = "linux-init-to-login"
		else if (starts(strip_timestamp(line), "login: "))
			phase = "post-login"
	}
	END {
		for (prefix in prefix_count)
			printf "%d\t%s\n", prefix_count[prefix], prefix > prefix_log
		for (phase in phase_lines)
			printf "%d\t%d\t%s\n", phase_lines[phase],
			    phase_bytes[phase], phase > phase_log
		for (key in category_lines) {
			split(key, parts, SUBSEP)
			printf "%d\t%d\t%s\t%s\n", category_lines[key],
			    category_bytes[key], parts[1], parts[2] > category_log
		}
		for (key in level_lines) {
			split(key, parts, SUBSEP)
			printf "%d\t%d\t%s\t%s\n", level_lines[key],
			    level_bytes[key], parts[1], parts[2] > level_log
		}
		for (key in phase_prefix_lines) {
			split(key, parts, SUBSEP)
			printf "%d\t%d\t%s\t%s\t%s\t%s\n",
			    phase_prefix_lines[key], phase_prefix_bytes[key],
			    parts[1], parts[2], parts[3], parts[4] > phase_prefix_log
		}
	}
	' prefix_log="$prefix_log" phase_log="$phase_log" \
		category_log="$category_log" level_log="$level_log" \
		phase_prefix_log="$phase_prefix_log" \
		slowpath_log="$slowpath_log" "$serial_log" >/dev/null

	sort -nr "$prefix_log" -o "$prefix_log"
	sort -k3,3 -nr "$phase_log" -o "$phase_log"
	sort -k3,3 -k1,1nr "$category_log" -o "$category_log"
	sort -k3,3 -k1,1nr "$level_log" -o "$level_log"
	sort -k3,3 -k1,1nr "$phase_prefix_log" -o "$phase_prefix_log"
}

mkdir -p "$out_dir" "$tmp"
: > "$events_log"
: > "$qemu_log"
: > "$serial_log"
start_ms=$(now_ms)
trap cleanup EXIT INT TERM

event profile-start "rootfs=$rootfs"
if [ -n "$rootfs_target" ]; then
	event rootfs-build-start "$rootfs_target"
	"${MAKE:-make}" ROOTFS_FLAVOR="$rootfs" "$rootfs_target"
	event rootfs-build-done "$rootfs_target"
fi
event esp-build-start "$target"
"${MAKE:-make}" ROOTFS_FLAVOR="$rootfs" "$target"
event esp-build-done "$target"

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

serial_output_reports
serial_lines=$(wc -l < "$serial_log" | tr -d ' ')
serial_bytes=$(wc -c < "$serial_log" | tr -d ' ')
total_ms=$(elapsed_ms)
{
	printf 'run_id=%s\n' "$run_id"
	printf 'rootfs=%s\n' "$rootfs"
	printf 'rootfs_target=%s\n' "$rootfs_target"
	printf 'esp=%s\n' "$esp"
	printf 'total_ms=%s\n' "$total_ms"
	printf 'serial_lines=%s\n' "$serial_lines"
	printf 'serial_bytes=%s\n' "$serial_bytes"
	printf 'events=%s\n' "$events_log"
	printf 'serial_log=%s\n' "$serial_log"
	printf 'qemu_log=%s\n' "$qemu_log"
	printf 'prefixes=%s\n' "$prefix_log"
	printf 'phases=%s\n' "$phase_log"
	printf 'categories=%s\n' "$category_log"
	printf 'levels=%s\n' "$level_log"
	printf 'phase_prefixes=%s\n' "$phase_prefix_log"
	printf 'slowpath_log=%s\n' "$slowpath_log"
} > "$summary_log"

printf 'run-profile summary=%s total_ms=%s serial_lines=%s serial_bytes=%s\n' \
	"$summary_log" "$total_ms" "$serial_lines" "$serial_bytes"
awk 'NR <= 20 { printf "run-profile prefix_count=%s prefix=%s\n", $1, $2 }' \
	"$prefix_log"
awk 'NR <= 20 { printf "run-profile category_count=%s bytes=%s phase=%s category=%s\n", $1, $2, $3, $4 }' \
	"$category_log"
