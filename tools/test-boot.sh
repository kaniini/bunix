#!/bin/sh
set -eu

qemu=${QEMU:-qemu-system-x86_64}
ovmf=${OVMF_CODE:-/usr/share/OVMF/OVMF_CODE.fd}
esp=${ESP_DIR:-build/esp}
timeout_cmd=${TIMEOUT:-timeout}
qemu_timeout=${QEMU_TIMEOUT:-60s}
qemu_memory=${QEMU_MEMORY:-128M}
qemu_extra_args=${QEMU_EXTRA_ARGS:-}
rootfs_flavor=${ROOTFS_FLAVOR:-synthetic}
run_id=${BUNIX_TEST_RUN_ID:-$(date -u +%Y%m%dT%H%M%SZ)-$$}
tmp=${BUNIX_TEST_RUNTIME_DIR:-${TMPDIR:-/tmp}/bunix-boot-test.$run_id}
log=${SERIAL_LOG:-build/serial.log}
qemu_log=$tmp/qemu.log
pipe=$tmp/serial
runtime_esp=$tmp/esp
failure_dir=${FAILURE_DIR:-build/failures/$run_id}
script_dir=$(CDPATH= cd "$(dirname "$0")" && pwd)
. "$script_dir/test-lib.sh"
BUNIX_COLLECT_FAILURES=1
BUNIX_FAILURE_DIR=$failure_dir
BUNIX_QEMU_LOG=$qemu_log
BUNIX_TEST_HARNESS=$0
BUNIX_TEST_RUNTIME_DIR=$tmp
export BUNIX_COLLECT_FAILURES BUNIX_FAILURE_DIR BUNIX_QEMU_LOG BUNIX_TEST_HARNESS BUNIX_TEST_RUNTIME_DIR

qemu_pid=
cat_pid=

cleanup() {
	if [ "${qemu_pid:-}" ]; then
		kill "$qemu_pid" 2>/dev/null || true
	fi
	if [ "${cat_pid:-}" ]; then
		kill "$cat_pid" 2>/dev/null || true
	fi
	if [ "${KEEP_TMP:-0}" != 1 ]; then
		rm -rf "$tmp"
	else
		echo "kept test tmp: $tmp" >&2
	fi
}

fail_boot() {
	label=$1
	tail_lines=${2:-160}
	out=$(save_failure_artifacts "$label" "$log" "$qemu_log" "$tail_lines")

	echo "$label" >&2
	echo "failure artifacts: $out" >&2
	cat "$qemu_log" >&2 || true
	tail -n "$tail_lines" "$log" >&2 || true
	exit 1
}

wait_for_fixed_boot() {
	expected=$1
	label=$2
	limit=${3:-45}
	tail_lines=${4:-120}
	i=0

	while ! grep -aF "$expected" "$log" >/dev/null 2>&1; do
		i=$((i + 1))
		if ! kill -0 "$qemu_pid" 2>/dev/null; then
			fail_boot "qemu exited while waiting for: $expected" "$tail_lines"
		fi
		if [ "$i" -gt "$limit" ]; then
			fail_boot "$label" "$tail_lines"
		fi
		sleep 1
	done
}

start_qemu() {
	mkdir -p "$tmp" "$(dirname "$log")"
	truncate -s 0 "$log"
	mkfifo "$pipe.in" "$pipe.out"
	trap cleanup EXIT INT TERM

	if [ ! -d "$esp" ]; then
		echo "ESP directory is not readable: $esp" >&2
		exit 1
	fi
	rm -rf "$runtime_esp"
	mkdir -p "$runtime_esp"
	cp -R "$esp/." "$runtime_esp/"

	cat "$pipe.out" > "$log" &
	cat_pid=$!

	TMPDIR=$tmp $timeout_cmd "$qemu_timeout" "$qemu" -enable-kvm -machine q35 -cpu host -m "$qemu_memory" \
		-smp "${SMP:-2}" \
		-drive if=pflash,format=raw,readonly=on,file="$ovmf" \
		-drive format=raw,file=fat:rw:"$runtime_esp" \
		-serial pipe:"$pipe" -display none -no-reboot $qemu_extra_args 2>"$qemu_log" &
	qemu_pid=$!
}

start_qemu
wait_for_fixed_boot "login: " "login prompt did not appear" 80 160

sleep 3
exec 3>"$pipe.in"
printf '%s\n%s\n' root root >&3
wait_for_fixed_boot "~ # " "root shell prompt did not appear for boot poweroff" 60 180
if [ "$rootfs_flavor" = alpine ]; then
	printf 'busybox id | busybox grep "uid=0(root)" >/dev/null && printf "BUNIX_ALPINE_ID_%%s\\n" OK\n' >&3
	printf 'busybox ps | busybox grep "/bin/sh" >/dev/null && printf "BUNIX_ALPINE_PS_%%s\\n" OK\n' >&3
	printf 'busybox dmesg | busybox grep "linux-server: close" >/dev/null && printf "BUNIX_ALPINE_DMESG_%%s\\n" OK\n' >&3
	printf 'printf "BUNIX_ALPINE_BOOT_COMMANDS_%%s\\n" OK\n' >&3
	wait_for_fixed_boot "BUNIX_ALPINE_BOOT_COMMANDS_OK" "alpine boot smoke commands did not finish" 60 220
fi
printf '/sbin/poweroff\n' >&3

if wait "$qemu_pid"; then
	qemu_pid=
	echo "boot regression ok"
	exit 0
fi
qemu_status=$?
qemu_pid=
fail_boot "qemu exited with status $qemu_status" 220
