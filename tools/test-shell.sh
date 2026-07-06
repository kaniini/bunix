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

check_exact_markers_file "$log" "$script_dir/test-shell-exact-markers.txt" \
	"shell regression missing" 75 220
check_fixed_markers_file "$log" "$script_dir/test-shell-content-markers.txt" \
	"shell content regression missing" 45 220
check_fixed_markers_file "$log" "$script_dir/test-shell-provisional-markers.txt" \
	"provisional shell marker missing" 45 220
check_path_limits_statfs
check_union_root_user
check_tmpfs_extended
wait_for_each_fixed_count "$log" 2 "append payload missing from file output" 45 220 \
	TMP_APPEND_ONE TMP_APPEND_TWO TMP_LONG_READDIR_PAYLOAD TMP_NAME_MAX_PAYLOAD \
	TMP_NAME255_PAYLOAD PATHMAX2_TMP_PAYLOAD UNION_ROOT_BASE_PAYLOAD \
	UNION_ROOT_APPEND_PAYLOAD UNION_ROOT_LONG_PAYLOAD UNION_NAME_MAX_PAYLOAD
check_large_io_mount
wait_for_each_fixed "$log" "procfs content regression missing" 45 220 \
	"cpu  " "/bin/sh" PROC_SHELL_PPID_OK "direct_delivered " "direct_handoff "
wait_for_each_regex "$log" "IPC fast path counter did not increase" 45 220 \
	"direct_delivered [1-9][0-9]*" "direct_handoff [1-9][0-9]*"
wait_for_each_regex "$log" "IPC per-CPU counter did not increase" 45 220 \
	"cpu[0-9][0-9]* sends [1-9][0-9]*"
check_tmpfs_basic_linux_tests

check_exec_argv_pipe

send_script <<'EOF_SECRET_DENIED'
busybox cat /secret.txt
echo POSTCAT
EOF_SECRET_DENIED

wait_for_fixed "$log" "cat: can't open '/secret.txt'" "busybox cat did not report denied /secret.txt access" 45 160
require_no_fixed "$log" "root secret" "busybox cat leaked /secret.txt to login user" 160
wait_for_fixed "$log" "POSTCAT" "shell did not continue after denied cat" 45 160

send_bytes 'ecxx\177\177ho BACKSPACE_OK\n'

wait_for_fixed "$log" "BACKSPACE_OK" "busybox backspace-edited command did not complete" 45 160

send_script <<'EOF_CAT_INTERRUPT'
cat
EOF_CAT_INTERRUPT
sleep 1
send_bytes '\003'
sleep 1
send_script <<'EOF_CAT_INTERRUPT_DONE'
echo CTRL_C_OK
EOF_CAT_INTERRUPT_DONE

wait_for_fixed "$log" "CTRL_C_OK" "foreground Ctrl-C did not return to shell" 45 180

send_script <<'EOF_WATCH'
busybox watch -n 1 busybox echo WATCH_OK & watch_pid=$!
EOF_WATCH

wait_for_awk "$log" '{ sub(/\r$/, "") } /^WATCH_OK$/ { count++ } END { exit count >= 2 ? 0 : 1 }' "busybox watch did not repeatedly run child command" 45 220

send_script <<'EOF_WATCH_DONE'
busybox kill $watch_pid
echo WATCH_DONE
EOF_WATCH_DONE

wait_for_fixed "$log" "WATCH_DONE" "busybox watch was not killed after repeated child runs" 45 220

login_prompts_before_exit=$(current_prompt_count "login: ")
send_script <<'EOF_EXIT_USER'
cd /bin
pwd
exit
EOF_EXIT_USER

wait_for_awk "$log" '{ sub(/\r$/, "") } /\/bin \$ pwd/ { prompt = NR } /^\/bin$/ && prompt { found = 1 } END { exit found ? 0 : 1 }' "busybox chdir/pwd regression failed" 45 160

wait_for_prompt_count_gt "login: " "$login_prompts_before_exit" "login prompt did not return after shell exit" 45 180

root_prompts_before_root=$(current_prompt_count "~ # ")
login_user root root "~ # " "$root_prompts_before_root"

send_script <<'EOF_ROOT'
busybox id
env
busybox cat /secret.txt && echo ROOT_SECRET_OK
EOF_ROOT

wait_for_fixed "$log" "uid=0(root)" "root login did not report uid 0" 45 180

wait_for_each_fixed "$log" "root login environment missing" 45 180 \
	"HOME=/root" "USER=root" "LOGNAME=root"

wait_for_fixed "$log" "ROOT_SECRET_OK" "root login could not read /secret.txt" 45 180
send_script <<'EOF_ROOT_UNION'
echo UNION_LOWER_PARENT_CREATE_PAYLOAD > /usr/share/bunix/nested/created.txt
busybox cat /usr/share/bunix/nested/created.txt && echo UNION_LOWER_PARENT_CREATE_OK
busybox cat /.upper/usr/share/bunix/nested/created.txt && echo UNION_LOWER_PARENT_UPPER_OK
echo UNION_LOWER_COPYUP_APPEND_PAYLOAD >> /usr/share/bunix/nested/hello.txt
busybox cat /usr/share/bunix/nested/hello.txt && echo UNION_LOWER_COPYUP_APPEND_OK
busybox cat /.upper/usr/share/bunix/nested/hello.txt && echo UNION_LOWER_COPYUP_UPPER_OK
busybox umount /.lower || echo UNION_LOWER_UMOUNT_BUSY_OK
busybox umount /.upper || echo UNION_UPPER_UMOUNT_BUSY_OK
busybox mount -t tmpfs tmpfs /.lower || echo UNION_LOWER_REPLACE_BUSY_OK
busybox mount -t tmpfs tmpfs /.upper || echo UNION_UPPER_REPLACE_BUSY_OK
busybox cat /hello.txt && echo UNION_PINNED_ROUTES_STILL_OK
EOF_ROOT_UNION
wait_for_each_fixed "$log" "root unionfs lower-parent regression missing" 45 220 \
	UNION_LOWER_PARENT_CREATE_OK UNION_LOWER_PARENT_UPPER_OK \
	UNION_LOWER_COPYUP_APPEND_OK UNION_LOWER_COPYUP_UPPER_OK \
	UNION_LOWER_UMOUNT_BUSY_OK UNION_UPPER_UMOUNT_BUSY_OK \
	UNION_LOWER_REPLACE_BUSY_OK UNION_UPPER_REPLACE_BUSY_OK \
	UNION_PINNED_ROUTES_STILL_OK
wait_for_each_fixed_count "$log" 2 "root unionfs lower-parent payload missing" 45 220 \
	UNION_LOWER_PARENT_CREATE_PAYLOAD UNION_LOWER_COPYUP_APPEND_PAYLOAD
send_script <<'EOF_ROOT_CHOWN'
busybox chown 0:0 /tmp/bunix-write.txt
busybox stat -c "%u:%g" /tmp/bunix-write.txt
exit
EOF_ROOT_CHOWN
wait_for_exact_line "$log" "0:0" "tmpfs chown did not update owner" 45 180

login_prompts_before_long=$(current_prompt_count "login: ")
wait_for_prompt_count_gt "login: " "$login_prompts_before_long" "login prompt did not return after root shell exit" 45 180

long_root_prompts_before=$(current_prompt_count "~ # ")
login_user administrator_with_long_name password_longer_than_sixteen "~ # " "$long_root_prompts_before"

send_script <<'EOF_LONG_LOGIN'
busybox id
env
exit
EOF_LONG_LOGIN
wait_for_each_fixed "$log" "long login regression missing" 45 180 \
	"uid=0(root)" "USER=administrator_with_long_name" \
	"LOGNAME=administrator_with_long_name"
echo "shell regression ok"
