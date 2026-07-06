#!/bin/sh
set -eu

qemu=${QEMU:-qemu-system-x86_64}
ovmf=${OVMF_CODE:-/usr/share/OVMF/OVMF_CODE.fd}
esp=${ESP_DIR:-build/esp}
timeout_cmd=${TIMEOUT:-timeout}
qemu_timeout=${QEMU_TIMEOUT:-180s}
tmp=${TMPDIR:-/tmp}/bunix-shell-test.$$
log=$tmp/serial.log
qemu_log=$tmp/qemu.log
pipe=$tmp/serial
failure_dir=${FAILURE_DIR:-build/failures}
script_dir=$(CDPATH= cd "$(dirname "$0")" && pwd)
. "$script_dir/test-lib.sh"
. "$script_dir/shell-tests/login-smoke.sh"
. "$script_dir/shell-tests/exec-argv-pipe.sh"
. "$script_dir/shell-tests/rootfs-vfs-proc-dev.sh"
. "$script_dir/shell-tests/tmpfs-basic-linux-tests.sh"
. "$script_dir/shell-tests/path-limits-statfs.sh"
. "$script_dir/shell-tests/union-root-user.sh"
. "$script_dir/shell-tests/tmpfs-extended.sh"
. "$script_dir/shell-tests/large-io-mount.sh"
. "$script_dir/shell-tests/interactive-tty.sh"
. "$script_dir/shell-tests/root-login-union.sh"
. "$script_dir/shell-tests/root-tmpfs-chown.sh"
. "$script_dir/shell-tests/long-login.sh"
BUNIX_COLLECT_FAILURES=1
BUNIX_FAILURE_DIR=$failure_dir
BUNIX_QEMU_LOG=$qemu_log
BUNIX_TEST_HARNESS=$0
export BUNIX_COLLECT_FAILURES BUNIX_FAILURE_DIR BUNIX_QEMU_LOG BUNIX_TEST_HARNESS

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

fail_with_qemu_log() {
	label=$1
	tail_lines=${2:-120}
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

send_bytes() {
	printf '%b' "$1" >&3
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
			fail_with_qemu_log "qemu exited while waiting for: $expected" "$tail_lines"
		fi
		if [ "$i" -gt "$limit" ]; then
			fail_with_qemu_log "$label" "$tail_lines"
		fi
		sleep 1
	done
}

wait_for_prompt_count_gt() {
	prompt=$1
	before=$2
	label=$3
	limit=${4:-45}
	tail_lines=${5:-180}
	i=0

	while [ "$(grep -aF -c "$prompt" "$log" 2>/dev/null || true)" -le "$before" ]; do
		i=$((i + 1))
		if [ "$i" -gt "$limit" ]; then
			fail_with_log "$label" "$log" "$tail_lines"
		fi
		sleep 1
	done
}

start_qemu() {
	mkdir -p "$tmp"
	mkfifo "$pipe.in" "$pipe.out"
	trap cleanup EXIT INT TERM

	cat "$pipe.out" > "$log" &
	cat_pid=$!

	TMPDIR=$tmp $timeout_cmd "$qemu_timeout" "$qemu" -enable-kvm -machine q35 -cpu host -m 128M \
		-smp "${SMP:-2}" \
		-drive if=pflash,format=raw,readonly=on,file="$ovmf" \
		-drive format=raw,file=fat:rw:"$esp" \
		-serial pipe:"$pipe" -display none -no-reboot 2>"$qemu_log" &
	qemu_pid=$!
}

login_user() {
	user=$1
	password=$2
	prompt=$3
	before=${4:-}

	if [ -n "$before" ]; then
		printf '%s\n%s\n' "$user" "$password" >&3
		wait_for_prompt_count_gt "$prompt" "$before" "shell prompt did not appear for $user" 45 180
		return
	fi

	printf '%s\n%s\n' "$user" "$password" >&3
	wait_for_fixed "$log" "$prompt" "shell prompt did not appear for $user" 150 120
}

current_prompt_count() {
	prompt=$1

	grep -F -c "$prompt" "$log" 2>/dev/null || true
}

start_qemu
wait_for_qemu_fixed "login: " "login prompt did not appear" 80 80

sleep 3
exec 3>"$pipe.in"
login_user kaniini bunix "~ $ "

run_login_smoke
run_exec_argv_pipe
run_rootfs_vfs_proc_dev
run_tmpfs_basic_linux_tests
run_path_limits_statfs
run_union_root_user
run_tmpfs_extended
run_large_io_mount

check_login_smoke

check_rootfs_vfs_proc_dev

check_path_limits_statfs
check_union_root_user
check_tmpfs_extended
check_large_io_mount
check_tmpfs_basic_linux_tests

check_exec_argv_pipe

run_interactive_tty

run_root_login_union
check_root_login_union
run_root_tmpfs_chown
run_long_login
check_long_login
echo "shell regression ok"
