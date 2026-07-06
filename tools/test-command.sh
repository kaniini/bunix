#!/bin/sh
set -eu

qemu=${QEMU:-qemu-system-x86_64}
ovmf=${OVMF_CODE:-/usr/share/OVMF/OVMF_CODE.fd}
esp=${ESP_DIR:-build/esp}
timeout_cmd=${TIMEOUT:-timeout}
qemu_timeout=${QEMU_TIMEOUT:-120s}
qemu_memory=${QEMU_MEMORY:-128M}
run_id=${BUNIX_TEST_RUN_ID:-$(date -u +%Y%m%dT%H%M%SZ)-$$}
tmp=${BUNIX_TEST_RUNTIME_DIR:-${TMPDIR:-/tmp}/bunix-command-test.$run_id}
log=$tmp/serial.log
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

command_text=${BUNIX_CMD:-${CMD:-}}
command_file=${BUNIX_CMD_FILE:-}
marker=${BUNIX_MARKER:-__BUNIX_COMMAND_OK__}
user=${BUNIX_USER:-kaniini}
password=${BUNIX_PASSWORD:-bunix}
prompt=${BUNIX_PROMPT:-~ $ }

qemu_pid=
cat_pid=
status=0

usage() {
	echo "usage: BUNIX_CMD='command' $0" >&2
	echo "       BUNIX_CMD_FILE=script.sh $0" >&2
	exit 2
}

if [ -z "$command_text" ] && [ -z "$command_file" ]; then
	usage
fi

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

fail_command() {
	label=$1
	tail_lines=${2:-160}
	out=$(save_failure_artifacts "$label" "$log" "$qemu_log" "$tail_lines")

	echo "$label" >&2
	echo "failure artifacts: $out" >&2
	cat "$qemu_log" >&2 || true
	tail -n "$tail_lines" "$log" >&2 || true
	exit 1
}

send_script() {
	cat >&3
}

wait_for_qemu_fixed() {
	expected=$1
	label=$2
	limit=${3:-45}
	tail_lines=${4:-120}
	i=0

	while ! grep -aF "$expected" "$log" >/dev/null 2>&1; do
		i=$((i + 1))
		if ! kill -0 "$qemu_pid" 2>/dev/null; then
			fail_command "qemu exited while waiting for: $expected" "$tail_lines"
		fi
		if [ "$i" -gt "$limit" ]; then
			fail_command "$label" "$tail_lines"
		fi
		sleep 1
	done
}

start_qemu() {
	mkdir -p "$tmp"
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
		-serial pipe:"$pipe" -display none -no-reboot 2>"$qemu_log" &
	qemu_pid=$!
}

start_qemu
wait_for_qemu_fixed "login: " "login prompt did not appear" 80 80

sleep 3
exec 3>"$pipe.in"
printf '%s\n%s\n' "$user" "$password" >&3
wait_for_fixed "$log" "$prompt" "shell prompt did not appear for $user" 150 120

if [ -n "$command_file" ]; then
	if [ ! -r "$command_file" ]; then
		fail_command "command file is not readable: $command_file" 40
	fi
	cat "$command_file" >&3
else
	printf '%s\n' "$command_text" >&3
fi
send_script <<EOF_COMMAND_DONE
status=\$?
echo __BUNIX_COMMAND_STATUS__=\$status
test "\$status" = 0 && echo $marker
exit
EOF_COMMAND_DONE

wait_for_fixed "$log" "__BUNIX_COMMAND_STATUS__=" "command did not report status" 90 220
if ! grep -aF "__BUNIX_COMMAND_STATUS__=0" "$log" >/dev/null 2>&1; then
	fail_command "command failed" 220
fi
wait_for_fixed "$log" "$marker" "command marker missing" 15 220

echo "command regression ok"
