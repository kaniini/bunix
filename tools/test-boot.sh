#!/bin/sh
set -eu

qemu=${QEMU:-qemu-system-x86_64}
ovmf=${OVMF_CODE:-/usr/share/OVMF/OVMF_CODE.fd}
esp=${ESP_DIR:-build/esp}
timeout_cmd=${TIMEOUT:-timeout}
qemu_timeout=${QEMU_TIMEOUT:-60s}
qemu_memory=${QEMU_MEMORY:-128M}
qemu_extra_args=${QEMU_EXTRA_ARGS:-}
sidecar_cmd=${BUNIX_TEST_SIDECAR_CMD:-}
sidecar_ready=${BUNIX_TEST_SIDECAR_READY_FILE:-}
sidecar_start_delay=${BUNIX_TEST_SIDECAR_START_DELAY:-1}
sidecar_ready_timeout=${BUNIX_TEST_SIDECAR_READY_TIMEOUT:-20}
rootfs_flavor=${ROOTFS_FLAVOR:-squashfs}
boot_phase=${BUNIX_BOOT_PHASE:-full}
login_wait=80
login_capture=160
login_wait_override=${BUNIX_BOOT_LOGIN_WAIT:-}
login_capture_override=${BUNIX_BOOT_LOGIN_CAPTURE:-}
run_id=${BUNIX_TEST_RUN_ID:-$(date -u +%Y%m%dT%H%M%SZ)-$$}
tmp=${BUNIX_TEST_RUNTIME_DIR:-${TMPDIR:-/tmp}/bunix-boot-test.$run_id}
log=${SERIAL_LOG:-build/serial.log}
qemu_log=$tmp/qemu.log
sidecar_log=$tmp/sidecar.log
pipe=$tmp/serial
runtime_esp=$tmp/esp
failure_dir=${FAILURE_DIR:-build/failures/$run_id}
script_dir=$(CDPATH= cd "$(dirname "$0")" && pwd)
. "$script_dir/test-lib.sh"
. "$script_dir/path-safety.sh"
BUNIX_COLLECT_FAILURES=1
BUNIX_FAILURE_DIR=$failure_dir
BUNIX_QEMU_LOG=$qemu_log
BUNIX_TEST_SIDECAR_LOG=$sidecar_log
BUNIX_TEST_HARNESS=$0
BUNIX_TEST_RUNTIME_DIR=$tmp
export BUNIX_COLLECT_FAILURES BUNIX_FAILURE_DIR BUNIX_QEMU_LOG BUNIX_TEST_SIDECAR_LOG BUNIX_TEST_HARNESS BUNIX_TEST_RUNTIME_DIR

qemu_pid=
cat_pid=
sidecar_pid=

cleanup() {
	if [ "${qemu_pid:-}" ]; then
		kill "$qemu_pid" 2>/dev/null || true
	fi
	if [ "${cat_pid:-}" ]; then
		kill "$cat_pid" 2>/dev/null || true
	fi
	if [ "${sidecar_pid:-}" ]; then
		kill "$sidecar_pid" 2>/dev/null || true
	fi
	if [ "${KEEP_TMP:-0}" != 1 ]; then
		safe_rm_rf "$tmp" "BUNIX_TEST_RUNTIME_DIR"
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

start_sidecar() {
	if [ -z "$sidecar_cmd" ]; then
		return
	fi
	(
		cd "$PWD"
		sh -c "$sidecar_cmd"
	) >"$sidecar_log" 2>&1 &
	sidecar_pid=$!
	if [ -n "$sidecar_ready" ]; then
		i=0
		while [ ! -e "$sidecar_ready" ]; do
			i=$((i + 1))
			if ! kill -0 "$sidecar_pid" 2>/dev/null; then
				fail_boot "sidecar exited before readiness: $sidecar_ready" 120
			fi
			if [ "$i" -gt "$sidecar_ready_timeout" ]; then
				fail_boot "sidecar readiness timed out: $sidecar_ready" 120
			fi
			sleep 1
		done
	else
		sleep "$sidecar_start_delay"
		if ! kill -0 "$sidecar_pid" 2>/dev/null; then
			fail_boot "sidecar exited before qemu start" 120
		fi
	fi
}

start_qemu() {
	mkdir -p "$tmp" "$(dirname "$log")"
	truncate -s 0 "$log"
	: > "$sidecar_log"
	mkfifo "$pipe.in" "$pipe.out"
	trap cleanup EXIT INT TERM

	if [ ! -d "$esp" ]; then
		echo "ESP directory is not readable: $esp" >&2
		exit 1
	fi
	safe_rm_rf "$runtime_esp" "runtime ESP directory"
	mkdir -p "$runtime_esp"
	cp -R "$esp/." "$runtime_esp/"

	cat "$pipe.out" > "$log" &
	cat_pid=$!

	start_sidecar
	TMPDIR=$tmp $timeout_cmd "$qemu_timeout" "$qemu" -enable-kvm -machine q35 -cpu host -m "$qemu_memory" \
		-smp "${SMP:-2}" \
		-drive if=pflash,format=raw,readonly=on,file="$ovmf" \
		-drive format=raw,file=fat:rw:"$runtime_esp" \
		-serial pipe:"$pipe" -display none -no-reboot $qemu_extra_args 2>"$qemu_log" &
	qemu_pid=$!
}

start_qemu
if [ "$boot_phase" = marker-poweroff ]; then
	marker=${BUNIX_BOOT_MARKER:-}
	if [ -z "$marker" ]; then
		fail_boot "BUNIX_BOOT_MARKER is required for marker-poweroff phase" 80
	fi
	wait_for_fixed_boot "$marker" "boot marker did not appear: $marker" 80 160
	if wait "$qemu_pid"; then
		qemu_pid=
		echo "boot marker poweroff ok"
		exit 0
	fi
	qemu_status=$?
	qemu_pid=
	fail_boot "qemu exited with status $qemu_status" 220
fi
if [ "$rootfs_flavor" = alpine-squashfs ] && [ "$boot_phase" = full ]; then
	login_wait=180
	login_capture=260
fi
if [ -n "$login_wait_override" ]; then
	login_wait=$login_wait_override
fi
if [ -n "$login_capture_override" ]; then
	login_capture=$login_capture_override
fi
wait_for_fixed_boot "login: " "login prompt did not appear" "$login_wait" "$login_capture"

sleep 3
exec 3>"$pipe.in"
printf '%s\n%s\n' root root >&3
wait_for_fixed_boot "~ # " "root shell prompt did not appear for boot poweroff" 60 180
if [ "$rootfs_flavor" = alpine-squashfs ] && [ "$boot_phase" = full ]; then
	printf 'busybox id | busybox grep "uid=0(root)" >/dev/null && printf "BUNIX_ALPINE_ID_%%s\\n" OK\n' >&3
	wait_for_fixed_boot "BUNIX_ALPINE_ID_OK" "alpine id smoke command did not finish" 60 180
	printf 'busybox cat /proc/self/stat | busybox cat >/dev/null && printf "BUNIX_ALPINE_PROC_PIPE_%%s\\n" OK\n' >&3
	wait_for_fixed_boot "BUNIX_ALPINE_PROC_PIPE_OK" "alpine procfs pipeline smoke command did not finish" 120 180
	printf 'busybox dmesg | busybox grep "linux-server: close" >/dev/null && printf "BUNIX_ALPINE_DMESG_%%s\\n" OK\n' >&3
	wait_for_fixed_boot "BUNIX_ALPINE_DMESG_OK" "alpine dmesg smoke command did not finish" 60 180
	printf '/bin/statidtest && printf "BUNIX_ALPINE_STATID_%%s\\n" OK\n' >&3
	wait_for_fixed_boot "BUNIX_ALPINE_STATID_OK" "alpine statid smoke command did not finish" 60 180
	printf '/sbin/openrc --help >/dev/null && printf "BUNIX_ALPINE_OPENRC_HELP_%%s\\n" OK\n' >&3
	wait_for_fixed_boot "BUNIX_ALPINE_OPENRC_HELP_OK" "alpine openrc help smoke command did not finish" 60 180
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
