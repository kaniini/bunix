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
send_script <<'EOF_TMPFS_BASELINE'
busybox stat -c "%u:%g %a" /tmp/bunix-write.txt
busybox mkdir /tmp/bunix-dir && echo TMP_MKDIR_OK
busybox test -d /tmp/bunix-dir && echo TMP_MKDIR_STAT_OK
LONG_TMP_NAME=bunix-readdir-name-longer-than-sixteen.txt
echo TMP_LONG_READDIR_PAYLOAD > "/tmp/$LONG_TMP_NAME"
busybox ls /tmp | busybox grep "$LONG_TMP_NAME" && echo TMP_LONG_READDIR_OK
busybox cat "/tmp/$LONG_TMP_NAME" && echo TMP_LONG_NAME_CAT_OK
EOF_TMPFS_BASELINE
wait_for_fixed "$log" "TMP_LONG_NAME_CAT_OK" "tmpfs baseline long-name commands did not drain" 240 220
send_script <<'EOF_TMPFS_EXTENDED'
TMP_NAME_MAX=bunix-tmp-name-aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.txt
echo TMP_NAME_MAX_PAYLOAD > "/tmp/$TMP_NAME_MAX"
busybox ls /tmp | busybox grep "$TMP_NAME_MAX" && echo TMP_NAME_MAX_READDIR_OK
busybox cat "/tmp/$TMP_NAME_MAX" && echo TMP_NAME_MAX_CAT_OK
busybox touch /tmp/bunix-touch.txt && echo TMP_TOUCH_CREATE_OK
busybox touch /tmp/bunix-write.txt && echo TMP_TOUCH_EXISTING_OK
busybox mkfifo /tmp/bunix-fifo && echo TMP_MKFIFO_OK
busybox test -p /tmp/bunix-fifo && echo TMP_FIFO_STAT_OK
echo TMP_LINK_ONE > /tmp/bunix-link-src.txt
busybox ln /tmp/bunix-link-src.txt /tmp/bunix-link-dst.txt && echo TMP_HARDLINK_CREATE_OK
echo TMP_LINK_TWO >> /tmp/bunix-link-dst.txt
busybox cat /tmp/bunix-link-src.txt | busybox grep TMP_LINK_TWO && echo TMP_HARDLINK_SHARE_OK
busybox mv /tmp/bunix-link-src.txt /tmp/bunix-link-renamed.txt
echo TMP_LINK_THREE >> /tmp/bunix-link-renamed.txt
busybox cat /tmp/bunix-link-dst.txt | busybox grep TMP_LINK_THREE && echo TMP_HARDLINK_RENAME_OK
busybox mkdir /tmp/bunix-rmdir
busybox rmdir /tmp/bunix-rmdir
busybox test ! -e /tmp/bunix-rmdir && echo RMDIR_OK
busybox mkdir /tmp/bunix-rm-r-dir
busybox rm -r /tmp/bunix-rm-r-dir && echo RM_R_DIR_OK
busybox rm -rf /tmp/rename-src /tmp/rename-dst /tmp/rename-parent /tmp/rename-self
busybox mkdir /tmp/rename-src
busybox mkdir /tmp/rename-src/sub
echo TMP_DIR_RENAME_PAYLOAD > /tmp/rename-src/sub/file
busybox mv /tmp/rename-src /tmp/rename-dst && echo TMP_DIR_RENAME_NONEMPTY_OK
busybox cat /tmp/rename-dst/sub/file | busybox grep TMP_DIR_RENAME_PAYLOAD && echo TMP_DIR_RENAME_SUBTREE_OK
busybox test ! -e /tmp/rename-src && echo TMP_DIR_RENAME_OLD_GONE_OK
busybox mkdir /tmp/rename-parent
busybox mkdir /tmp/rename-parent/src
busybox mkdir /tmp/rename-parent/src/sub
busybox mkdir /tmp/rename-parent/empty
echo TMP_DIR_REPLACE_PAYLOAD > /tmp/rename-parent/src/sub/file
busybox mv -T /tmp/rename-parent/src /tmp/rename-parent/empty && echo TMP_DIR_REPLACE_EMPTY_OK
busybox cat /tmp/rename-parent/empty/sub/file | busybox grep TMP_DIR_REPLACE_PAYLOAD && echo TMP_DIR_REPLACE_SUBTREE_OK
busybox mkdir /tmp/rename-parent/src2
busybox mkdir /tmp/rename-parent/nonempty
echo target > /tmp/rename-parent/nonempty/file
busybox mv -f -T /tmp/rename-parent/src2 /tmp/rename-parent/nonempty || echo TMP_DIR_REPLACE_NONEMPTY_DENY_OK
busybox mkdir /tmp/rename-self
busybox mkdir /tmp/rename-self/child
busybox mv /tmp/rename-self /tmp/rename-self/child/moved || echo TMP_DIR_RENAME_DESCENDANT_DENY_OK
busybox test -d /tmp/rename-self/child && echo TMP_DIR_RENAME_DESCENDANT_PRESERVE_OK
busybox chmod 700 /tmp/bunix-write.txt && echo TMP_CHMOD_OK
busybox stat -c "%a" /tmp/bunix-write.txt
busybox chown root:root /tmp/bunix-write.txt || echo TMP_CHOWN_DENY_OK
NAME255=
i=0
while [ "$i" -lt 255 ]; do NAME255="${NAME255}a"; i=$((i + 1)); done
echo TMP_NAME255_PAYLOAD > "/tmp/$NAME255" && echo TMP_NAME255_CREATE_OK
busybox cat "/tmp/$NAME255" && echo TMP_NAME255_CAT_OK
NAME256="${NAME255}b"
echo TMP_NAME256_PAYLOAD > "/tmp/$NAME256" || echo TMP_NAME256_DENY_OK
echo TMP_SYMLINK_PAYLOAD > /tmp/bunix-symlink-target.txt
busybox ln -s bunix-symlink-target.txt /tmp/bunix-symlink.txt && echo TMP_SYMLINK_CREATE_OK
busybox readlink /tmp/bunix-symlink.txt | busybox grep bunix-symlink-target.txt && echo TMP_SYMLINK_READLINK_OK
busybox ls -l /tmp/bunix-symlink.txt | busybox grep bunix-symlink-target.txt && echo TMP_SYMLINK_LS_OK
busybox ln -s hello.txt /union-symlink.txt && echo UNION_SYMLINK_CREATE_OK
busybox readlink /union-symlink.txt | busybox grep hello.txt && echo UNION_SYMLINK_READLINK_OK
busybox ls -l /union-symlink.txt | busybox grep hello.txt && echo UNION_SYMLINK_LS_OK
echo UNION_HARDLINK_ONE > /union-hard-src.txt
busybox ln /union-hard-src.txt /union-hard-dst.txt && echo UNION_HARDLINK_CREATE_OK
echo UNION_HARDLINK_TWO >> /union-hard-dst.txt
busybox cat /union-hard-src.txt | busybox grep UNION_HARDLINK_TWO && echo UNION_HARDLINK_SHARE_OK
EOF_TMPFS_EXTENDED
wait_for_fixed "$log" "TMP_CHOWN_DENY_OK" "tmpfs extended long-name commands did not drain" 45 220
send_script <<'EOF_LARGE_IO'
busybox head -c 8192 /dev/zero > /tmp/linux-big-write.bin
busybox test "$(busybox stat -c "%s" /tmp/linux-big-write.bin)" = "8192" && echo LINUX_BIG_WRITE_OK
/bin/mmapbig && echo MMAPBIG_OK
/bin/mmaphuge
/bin/readbig && echo READBIG_OK
EOF_LARGE_IO
wait_for_fixed "$log" "READBIG_OK" "pre-mount shell commands did not drain" 45 220
send_script <<'EOF_MOUNT_TMPFS'
mount -t tmpfs tmpfs /mnt&&echo MNT_OK
echo MNT_PAYLOAD>/mnt/linux-mount.txt
cat /mnt/linux-mount.txt&&echo MNT_CAT
mount|busybox grep /mnt&&echo MNT_LIST
umount /mnt&&echo UMNT_OK
mount|busybox grep /mnt||echo UMNT_GONE
test ! -e /mnt/linux-mount.txt&&echo UMNT_HIDE
EOF_MOUNT_TMPFS

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
wait_for_each_fixed_count "$log" 2 "append payload missing from file output" 45 220 \
	TMP_APPEND_ONE TMP_APPEND_TWO TMP_LONG_READDIR_PAYLOAD TMP_NAME_MAX_PAYLOAD \
	TMP_NAME255_PAYLOAD PATHMAX2_TMP_PAYLOAD UNION_ROOT_BASE_PAYLOAD \
	UNION_ROOT_APPEND_PAYLOAD UNION_ROOT_LONG_PAYLOAD UNION_NAME_MAX_PAYLOAD
wait_for_fixed "$log" "READBIG_OK" "large Linux read did not complete" 45 220
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
