#!/bin/sh
set -eu

qemu=${QEMU:-qemu-system-x86_64}
ovmf=${OVMF_CODE:-/usr/share/OVMF/OVMF_CODE.fd}
esp=${ESP_DIR:-build/esp}
timeout_cmd=${TIMEOUT:-timeout}
tmp=${TMPDIR:-/tmp}/bunix-shell-test.$$
log=$tmp/serial.log
qemu_log=$tmp/qemu.log
pipe=$tmp/serial
script_dir=$(CDPATH= cd "$(dirname "$0")" && pwd)
. "$script_dir/test-lib.sh"

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

wait_for_qemu_fixed() {
	expected=$1
	label=$2
	limit=${3:-45}
	tail_lines=${4:-120}
	i=0

	while ! grep -aF "$expected" "$log" >/dev/null 2>&1; do
		i=$((i + 1))
		if ! kill -0 "$qemu_pid" 2>/dev/null; then
			echo "qemu exited while waiting for: $expected" >&2
			cat "$qemu_log" >&2 || true
			tail -n "$tail_lines" "$log" >&2 || true
			exit 1
		fi
		if [ "$i" -gt "$limit" ]; then
			echo "$label" >&2
			cat "$qemu_log" >&2 || true
			tail -n "$tail_lines" "$log" >&2 || true
			exit 1
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

mkdir -p "$tmp"
mkfifo "$pipe.in" "$pipe.out"
trap cleanup EXIT INT TERM

cat "$pipe.out" > "$log" &
cat_pid=$!

TMPDIR=$tmp $timeout_cmd 90s "$qemu" -enable-kvm -machine q35 -cpu host -m 128M \
	-smp "${SMP:-2}" \
	-drive if=pflash,format=raw,readonly=on,file="$ovmf" \
	-drive format=raw,file=fat:rw:"$esp" \
	-serial pipe:"$pipe" -display none -no-reboot 2>"$qemu_log" &
qemu_pid=$!

wait_for_qemu_fixed "login: " "login prompt did not appear" 80 80

sleep 3
exec 3>"$pipe.in"
printf 'kaniini\nbunix\n' >&3

wait_for_fixed "$log" "~ $ " "shell prompt did not appear after login" 150 120

printf 'uptime\nbusybox uptime\nbusybox uname\nbusybox uname -r\nbusybox stty -a\nbusybox id\nenv\n/usr/bin/env >/dev/null && echo USR_ENV_OK\nbusybox test -x /bin/sh && echo ACCESS_X_OK\nbusybox test -r /hello.txt && echo ACCESS_R_OK\nbusybox test ! -r /secret.txt && echo ACCESS_DENY_OK\ncd ~\npwd\ncd /\nbusybox kill -0 $$ && echo KILL_ZERO_OK\nbusybox kill -0 -$$ && echo KILL_PGRP_OK\ntrap "" INT\nbusybox kill -INT $$\necho SIGINT_IGNORE_OK\ntrap - INT\nbusybox sleep 1 && echo BUSYBOX_SLEEP_OK\nsleep 1 && echo DIRECT_SLEEP_OK\nbusybox sleep 5\n\003echo SLEEP_CTRL_C_OK\nbusybox echo PIPE_OK | busybox cat\nbusybox cat /hello.txt | busybox cat && echo PIPE_FILE_OK\nbusybox cat /usr/share/bunix/nested/hello.txt && echo NESTED_CAT_OK\nbusybox cat /usr/share/bunix/alpine/very/long/rootfs/path/that/exceeds/the/old/two-hundred-fifty-six-byte/rootfs-entry-limit/path-component-that-forces-the-rootfs-image-format-past-the-old-limit/path-component-that-forces-the-rootfs-image-format-past-the-old-limit/with-extra-components/hello.txt && echo LONG_ROOTFS_PATH_OK\n/usr/share/bunix/alpine/very/long/rootfs/path/that/exceeds/the/old/two-hundred-fifty-six-byte/linux-execve-path-limit/path-component-that-forces-the-rootfs-image-format-past-the-old-limit/path-component-that-forces-the-rootfs-image-format-past-the-old-limit/with-extra-components/dyn-hello && echo LONG_EXEC_PATH_OK\nbusybox cat /proc/kthreads >/dev/null && echo PROCFS_OK\nbusybox echo BUSYBOX_ARGV_OK\n' >&3
printf 'busybox sh -c "test \"\\$13\" = m && echo BUSYBOX_MANY_ARGV_OK" _ a b c d e f g h i j k l m\nBUNIX_LONG_ENV=abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 busybox sh -c "test \"\\$BUNIX_LONG_ENV\" = abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 && echo BUSYBOX_LONG_ENV_OK"\n' >&3
printf 'set --\ni=1\nwhile [ "$i" -le 300 ]; do set -- "$@" "x$i"; i=$((i + 1)); done\nbusybox sh -c '"'"'test "$300" = x300 && echo BUSYBOX_ARGV300_OK'"'"' _ "$@"\ni=1\nwhile [ "$i" -le 300 ]; do eval "BUNIX_EXEC_ENV_$i=x$i; export BUNIX_EXEC_ENV_$i"; i=$((i + 1)); done\nbusybox sh -c '"'"'test "$BUNIX_EXEC_ENV_300" = x300 && echo BUSYBOX_ENV300_OK'"'"'\n' >&3
printf 'busybox stat /hello.txt\nbusybox stat /usr/share\nbusybox stat /tmp\nbusybox stat /run\nbusybox stat /var/tmp\nbusybox stat /usr/bin/env\nbusybox ls /\nbusybox ls /bin\nbusybox readlink /bin/cat && echo SYMLINK_READLINK_OK\nbusybox readlink /usr/share/bunix/long-target-link | busybox grep "/usr/share/bunix/alpine/very/long/rootfs/path/that/exceeds/the/old/two-hundred-fifty-six-byte/rootfs-entry-limit/path-component-that-forces-the-rootfs-image-format-past-the-old-limit/path-component-that-forces-the-rootfs-image-format-past-the-old-limit/with-extra-components/hello.txt" && echo LONG_SYMLINK_READLINK_OK\nbusybox ls -l /bin/cat && echo SYMLINK_LS_OK\nbusybox ls /usr/share/bunix/nested\nbusybox stat /bin\ncd /tmp\npwd\ncd /usr/share/bunix/nested\npwd\nbusybox ls .\nbusybox cat ../nested/./hello.txt && echo VFS_DOTDOT_OK\ncd /usr/share/bunix/alpine/very/long/rootfs/path/that/exceeds/the/old/two-hundred-fifty-six-byte/rootfs-entry-limit/path-component-that-forces-the-rootfs-image-format-past-the-old-limit/path-component-that-forces-the-rootfs-image-format-past-the-old-limit/with-extra-components\ntest "$(pwd)" = "/usr/share/bunix/alpine/very/long/rootfs/path/that/exceeds/the/old/two-hundred-fifty-six-byte/rootfs-entry-limit/path-component-that-forces-the-rootfs-image-format-past-the-old-limit/path-component-that-forces-the-rootfs-image-format-past-the-old-limit/with-extra-components" && echo LONG_GETCWD_OK\ncd /\nbusybox cat //usr///share/bunix/nested/hello.txt && echo VFS_SLASH_OK\nbusybox cat /proc/kthreads >/dev/null && echo PROCFS_STILL_OK\nbusybox cat /proc/self/status && echo PROC_STATUS_OK\nbusybox ls /proc/self/fd && echo PROC_FD_OK\nbusybox readlink /proc/self/exe && echo PROC_EXE_OK\nbusybox cat /proc/stat && echo PROC_STAT_OK\nbusybox cat /proc/ipc && echo PROC_IPC_OK\nbusybox cat /proc/filesystems && echo PROC_FILESYSTEMS_OK\nbusybox cat /proc/cpuinfo && echo PROC_CPUINFO_OK\nbusybox cat /proc/self/cmdline && echo PROC_CMDLINE_OK\nbusybox stat /dev/zero && echo DEV_ZERO_STAT_OK\nbusybox stat /dev/urandom && echo DEV_URANDOM_STAT_OK\nbusybox test -r /dev/zero && echo DEV_ZERO_ACCESS_OK\nbusybox head -c 4 /dev/zero >/dev/null && echo DEV_ZERO_READ_OK\nbusybox head -c 4 /dev/urandom >/dev/null && echo DEV_URANDOM_READ_OK\necho TMP_WRITE_OK > /tmp/bunix-write.txt\nbusybox sh -c "set -C; echo TMP_EXCL_BAD > /tmp/bunix-write.txt" || echo TMP_EXCL_DENY_OK\nbusybox cat /tmp/bunix-write.txt | busybox grep TMP_WRITE_OK && echo TMP_EXCL_PRESERVE_OK\nbusybox cat /tmp/bunix-write.txt && echo TMP_CAT_OK\nbusybox stat /tmp/bunix-write.txt && echo TMP_STAT_OK\necho TMP_APPEND_ONE > /tmp/bunix-append.txt\necho TMP_APPEND_TWO >> /tmp/bunix-append.txt\nbusybox cat /tmp/bunix-append.txt && echo TMP_APPEND_CAT_OK\nbusybox sh -c "echo RUN_WRITE_OK > /run/bunix-run.txt"\nbusybox cat /run/bunix-run.txt && echo RUN_CAT_OK\necho TRUNCATE_PAYLOAD > /tmp/bunix-trunc.txt\nbusybox truncate -s 4 /tmp/bunix-trunc.txt && echo TRUNCATE_OK\nbusybox cat /tmp/bunix-trunc.txt && echo TRUNCATE_CAT_OK\nbusybox rm /tmp/bunix-trunc.txt && echo UNLINK_OK\nbusybox test ! -e /tmp/bunix-trunc.txt && echo UNLINK_GONE_OK\n/bin/getdentstest && echo GETDENTS64_OK\n/bin/vforkstress && echo VFORKSTRESS_OK\nbusybox test ! -e /lib/ld.so && echo MUSL_LDSO_CANONICAL_OK\n/bin/dyn-hello && echo DYN_HELLO_OK\nbusybox top -b -n 1 >/dev/null && echo PROC_TOP_OK\nbusybox ps && echo PROC_PS_OK\nbusybox free && echo PROC_FREE_OK\nbusybox mount && echo PROC_MOUNT_OK\n' >&3
printf '/bin/iovtest && echo IOVTEST_OK\n/bin/fchmodattest\n/bin/waitpgidtest\n/bin/execlongtest\n/bin/auxidtest\n' >&3
printf 'busybox mkdir -p /tmp/mkdir-p/a/b && echo TMP_MKDIR_P_EXISTING_ROOT_OK\nbusybox test -d /tmp/mkdir-p/a/b && echo TMP_MKDIR_P_NESTED_OK\nbusybox mkdir /tmp || echo TMP_MKDIR_EXISTING_ROOT_DENY_OK\nbusybox mkdir -p /union-mkdir-p/a/b && echo UNION_MKDIR_P_ROOT_OK\nbusybox test -d /union-mkdir-p/a/b && echo UNION_MKDIR_P_CHILD_OK\nbusybox mkdir /usr || echo UNION_MKDIR_EXISTING_LOWER_DENY_OK\n' >&3
printf 'busybox grep "PPid:\t1" /proc/$$/status && echo PROC_SHELL_PPID_OK\n' >&3
printf '/bin/cat /proc/self/cmdline | busybox grep -a /bin/cat && echo PROC_SELF_CMDLINE_CALLER_OK\n' >&3
printf 'busybox sh -c '"'"'/bin/cat /proc/$$/cmdline | busybox grep -a PROC_ARGV_SENTINEL && echo PROC_ARGV_CMDLINE_OK'"'"' PROC_ARGV_SENTINEL\n' >&3
printf 'busybox test -c /dev/null && echo DEV_NULL_CHAR_OK\nbusybox test -c /dev/zero && echo DEV_ZERO_CHAR_OK\nbusybox test -c /dev/console && echo DEV_CONSOLE_CHAR_OK\n' >&3
printf '/bin/execbig && echo EXECBIG_OK\n' >&3
printf 'busybox sh -c '"'"'i=0; while [ "$i" -lt 300 ]; do printf a; i=$((i + 1)); done; echo DEV_CONSOLE_BIG_END'"'"' > /dev/console && echo DEV_CONSOLE_BIG_WRITE_OK\n' >&3
printf 'LONG_TMP=/tmp/bunix-pathmax2\nbusybox mkdir "$LONG_TMP"\nLONG_TMP=$LONG_TMP/segment-aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\nbusybox mkdir "$LONG_TMP"\nLONG_TMP=$LONG_TMP/segment-bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\nbusybox mkdir "$LONG_TMP"\nLONG_TMP=$LONG_TMP/segment-cccccccccccccccccccccccccccccccc\nbusybox mkdir "$LONG_TMP"\nLONG_TMP=$LONG_TMP/segment-dddddddddddddddddddddddddddddddd\nbusybox mkdir "$LONG_TMP"\nLONG_TMP=$LONG_TMP/segment-eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee\nbusybox mkdir "$LONG_TMP"\nLONG_TMP=$LONG_TMP/segment-ffffffffffffffffffffffffffffffff\nbusybox mkdir "$LONG_TMP" && echo PATHMAX2_TMP_MKDIR_OK\necho PATHMAX2_TMP_PAYLOAD > "$LONG_TMP/file.txt"\nbusybox cat "$LONG_TMP/file.txt" && echo PATHMAX2_TMP_FILE_OK\ncd "$LONG_TMP" && echo PATHMAX2_TMP_CHDIR_OK\npwd\ncd /\n/bin/pathmaxtest\n' >&3
printf 'busybox df / /tmp /proc >/dev/null && echo STATFS_DF_OK\n' >&3
printf 'busybox cat /hello.txt && echo UNION_ROOT_LOWER_OK\nbusybox mv /rename-lower.txt /rename-upper.txt && echo UNION_LOWER_RENAME_CREATE_OK\nbusybox cat /rename-upper.txt | busybox grep "nested rootfs file" && echo UNION_LOWER_RENAME_READ_OK\nbusybox test ! -e /rename-lower.txt && echo UNION_LOWER_RENAME_OLD_GONE_OK\nbusybox test -e /.upper/rename-upper.txt && echo UNION_LOWER_RENAME_UPPER_OK\n' >&3
wait_for_fixed "$log" "UNION_LOWER_RENAME_UPPER_OK" "unionfs lower rename commands did not drain" 45 220
printf 'busybox ln /hello.txt /hello-hard.txt && echo UNION_LOWER_HARDLINK_CREATE_OK\nbusybox cat /hello-hard.txt | busybox grep "rootfs: module" && echo UNION_LOWER_HARDLINK_READ_OK\nbusybox test -e /.upper/hello.txt && busybox test -e /.upper/hello-hard.txt && echo UNION_LOWER_HARDLINK_COPYUP_OK\necho UNION_ROOT_BASE_PAYLOAD > /created.txt\necho UNION_ROOT_APPEND_PAYLOAD >> /created.txt\nbusybox cat /created.txt && echo UNION_ROOT_APPEND_CAT_OK\nbusybox cat /.upper/created.txt && echo UNION_ROOT_UPPER_BACKING_OK\nUNION_LONG_NAME=bunix-union-root-name-longer-than-sixteen.txt\necho UNION_ROOT_LONG_PAYLOAD > "/$UNION_LONG_NAME"\nbusybox ls / | busybox grep "$UNION_LONG_NAME" && echo UNION_ROOT_LONG_READDIR_OK\nbusybox cat "/$UNION_LONG_NAME" && echo UNION_ROOT_LONG_CAT_OK\nUNION_NAME_MAX=bunix-union-name-aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.txt\necho UNION_NAME_MAX_PAYLOAD > "/$UNION_NAME_MAX"\nbusybox ls / | busybox grep "$UNION_NAME_MAX" && echo UNION_NAME_MAX_READDIR_OK\nbusybox cat "/$UNION_NAME_MAX" && echo UNION_NAME_MAX_CAT_OK\nbusybox ls / | busybox grep created.txt && echo UNION_ROOT_READDIR_UPPER_OK\nbusybox ls / | busybox grep secret.txt && echo UNION_ROOT_READDIR_LOWER_OK\nbusybox rm /created.txt && echo UNION_ROOT_UNLINK_UPPER_OK\nbusybox test ! -e /created.txt && echo UNION_ROOT_UNLINK_UPPER_GONE_OK\nbusybox cat /hello.txt && echo UNION_ROOT_LOWER_STILL_OK\n' >&3
wait_for_fixed "$log" "UNION_ROOT_LOWER_STILL_OK" "unionfs root commands did not drain" 120 220
printf 'busybox stat -c "%%u:%%g %%a" /tmp/bunix-write.txt\nbusybox mkdir /tmp/bunix-dir && echo TMP_MKDIR_OK\nbusybox test -d /tmp/bunix-dir && echo TMP_MKDIR_STAT_OK\nLONG_TMP_NAME=bunix-readdir-name-longer-than-sixteen.txt\necho TMP_LONG_READDIR_PAYLOAD > "/tmp/$LONG_TMP_NAME"\nbusybox ls /tmp | busybox grep "$LONG_TMP_NAME" && echo TMP_LONG_READDIR_OK\nbusybox cat "/tmp/$LONG_TMP_NAME" && echo TMP_LONG_NAME_CAT_OK\n' >&3
wait_for_fixed "$log" "TMP_LONG_NAME_CAT_OK" "tmpfs baseline long-name commands did not drain" 240 220
printf 'TMP_NAME_MAX=bunix-tmp-name-aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.txt\necho TMP_NAME_MAX_PAYLOAD > "/tmp/$TMP_NAME_MAX"\nbusybox ls /tmp | busybox grep "$TMP_NAME_MAX" && echo TMP_NAME_MAX_READDIR_OK\nbusybox cat "/tmp/$TMP_NAME_MAX" && echo TMP_NAME_MAX_CAT_OK\nbusybox touch /tmp/bunix-touch.txt && echo TMP_TOUCH_CREATE_OK\nbusybox touch /tmp/bunix-write.txt && echo TMP_TOUCH_EXISTING_OK\nbusybox mkfifo /tmp/bunix-fifo && echo TMP_MKFIFO_OK\nbusybox test -p /tmp/bunix-fifo && echo TMP_FIFO_STAT_OK\necho TMP_LINK_ONE > /tmp/bunix-link-src.txt\nbusybox ln /tmp/bunix-link-src.txt /tmp/bunix-link-dst.txt && echo TMP_HARDLINK_CREATE_OK\necho TMP_LINK_TWO >> /tmp/bunix-link-dst.txt\nbusybox cat /tmp/bunix-link-src.txt | busybox grep TMP_LINK_TWO && echo TMP_HARDLINK_SHARE_OK\nbusybox mv /tmp/bunix-link-src.txt /tmp/bunix-link-renamed.txt\necho TMP_LINK_THREE >> /tmp/bunix-link-renamed.txt\nbusybox cat /tmp/bunix-link-dst.txt | busybox grep TMP_LINK_THREE && echo TMP_HARDLINK_RENAME_OK\nbusybox mkdir /tmp/bunix-rmdir\nbusybox rmdir /tmp/bunix-rmdir\nbusybox test ! -e /tmp/bunix-rmdir && echo RMDIR_OK\nbusybox mkdir /tmp/bunix-rm-r-dir\nbusybox rm -r /tmp/bunix-rm-r-dir && echo RM_R_DIR_OK\nbusybox rm -rf /tmp/rename-src /tmp/rename-dst /tmp/rename-parent /tmp/rename-self\nbusybox mkdir /tmp/rename-src\nbusybox mkdir /tmp/rename-src/sub\necho TMP_DIR_RENAME_PAYLOAD > /tmp/rename-src/sub/file\nbusybox mv /tmp/rename-src /tmp/rename-dst && echo TMP_DIR_RENAME_NONEMPTY_OK\nbusybox cat /tmp/rename-dst/sub/file | busybox grep TMP_DIR_RENAME_PAYLOAD && echo TMP_DIR_RENAME_SUBTREE_OK\nbusybox test ! -e /tmp/rename-src && echo TMP_DIR_RENAME_OLD_GONE_OK\nbusybox mkdir /tmp/rename-parent\nbusybox mkdir /tmp/rename-parent/src\nbusybox mkdir /tmp/rename-parent/src/sub\nbusybox mkdir /tmp/rename-parent/empty\necho TMP_DIR_REPLACE_PAYLOAD > /tmp/rename-parent/src/sub/file\nbusybox mv -T /tmp/rename-parent/src /tmp/rename-parent/empty && echo TMP_DIR_REPLACE_EMPTY_OK\nbusybox cat /tmp/rename-parent/empty/sub/file | busybox grep TMP_DIR_REPLACE_PAYLOAD && echo TMP_DIR_REPLACE_SUBTREE_OK\nbusybox mkdir /tmp/rename-parent/src2\nbusybox mkdir /tmp/rename-parent/nonempty\necho target > /tmp/rename-parent/nonempty/file\nbusybox mv -f -T /tmp/rename-parent/src2 /tmp/rename-parent/nonempty || echo TMP_DIR_REPLACE_NONEMPTY_DENY_OK\nbusybox mkdir /tmp/rename-self\nbusybox mkdir /tmp/rename-self/child\nbusybox mv /tmp/rename-self /tmp/rename-self/child/moved || echo TMP_DIR_RENAME_DESCENDANT_DENY_OK\nbusybox test -d /tmp/rename-self/child && echo TMP_DIR_RENAME_DESCENDANT_PRESERVE_OK\nbusybox chmod 700 /tmp/bunix-write.txt && echo TMP_CHMOD_OK\nbusybox stat -c "%%a" /tmp/bunix-write.txt\nbusybox chown root:root /tmp/bunix-write.txt || echo TMP_CHOWN_DENY_OK\n' >&3
printf 'NAME255=\ni=0\nwhile [ "$i" -lt 255 ]; do NAME255="${NAME255}a"; i=$((i + 1)); done\necho TMP_NAME255_PAYLOAD > "/tmp/$NAME255" && echo TMP_NAME255_CREATE_OK\nbusybox cat "/tmp/$NAME255" && echo TMP_NAME255_CAT_OK\nNAME256="${NAME255}b"\necho TMP_NAME256_PAYLOAD > "/tmp/$NAME256" || echo TMP_NAME256_DENY_OK\n' >&3
printf 'echo TMP_SYMLINK_PAYLOAD > /tmp/bunix-symlink-target.txt\nbusybox ln -s bunix-symlink-target.txt /tmp/bunix-symlink.txt && echo TMP_SYMLINK_CREATE_OK\nbusybox readlink /tmp/bunix-symlink.txt | busybox grep bunix-symlink-target.txt && echo TMP_SYMLINK_READLINK_OK\nbusybox ls -l /tmp/bunix-symlink.txt | busybox grep bunix-symlink-target.txt && echo TMP_SYMLINK_LS_OK\n' >&3
printf 'busybox ln -s hello.txt /union-symlink.txt && echo UNION_SYMLINK_CREATE_OK\nbusybox readlink /union-symlink.txt | busybox grep hello.txt && echo UNION_SYMLINK_READLINK_OK\nbusybox ls -l /union-symlink.txt | busybox grep hello.txt && echo UNION_SYMLINK_LS_OK\necho UNION_HARDLINK_ONE > /union-hard-src.txt\nbusybox ln /union-hard-src.txt /union-hard-dst.txt && echo UNION_HARDLINK_CREATE_OK\necho UNION_HARDLINK_TWO >> /union-hard-dst.txt\nbusybox cat /union-hard-src.txt | busybox grep UNION_HARDLINK_TWO && echo UNION_HARDLINK_SHARE_OK\n' >&3
wait_for_fixed "$log" "TMP_CHOWN_DENY_OK" "tmpfs extended long-name commands did not drain" 45 220
printf 'busybox head -c 8192 /dev/zero > /tmp/linux-big-write.bin\nbusybox test "$(busybox stat -c "%%s" /tmp/linux-big-write.bin)" = "8192" && echo LINUX_BIG_WRITE_OK\n' >&3
printf '/bin/mmapbig && echo MMAPBIG_OK\n/bin/mmaphuge\n/bin/readbig && echo READBIG_OK\n' >&3
wait_for_fixed "$log" "READBIG_OK" "pre-mount shell commands did not drain" 45 220
printf 'mount -t tmpfs tmpfs /mnt&&echo MNT_OK\necho MNT_PAYLOAD>/mnt/linux-mount.txt\ncat /mnt/linux-mount.txt&&echo MNT_CAT\nmount|busybox grep /mnt&&echo MNT_LIST\numount /mnt&&echo UMNT_OK\nmount|busybox grep /mnt||echo UMNT_GONE\ntest ! -e /mnt/linux-mount.txt&&echo UMNT_HIDE\n' >&3

wait_for_fixed "$log" "load average" "busybox uptime did not run through applet argv" 45 120
wait_for_fixed "$log" " 1 users" "busybox uptime did not report login session" 45 160
wait_for_fixed "$log" "Bunix" "busybox uname did not report Bunix" 45 120
wait_for_fixed "$log" "0.1" "busybox uname -r did not report 0.1" 45 120
wait_for_fixed "$log" "uid=1000" "busybox id did not report login identity" 45 120
wait_for_awk "$log" '{ sub(/\r$/, "") } /groups=1(\(wheel\))?,1000(\(kaniini\))?/ { found = 1 } END { exit found ? 0 : 1 }' "busybox id did not report login supplementary groups" 45 120
wait_for_awk "$log" '{ sub(/\r$/, "") } /2018(\(g18\))?/ { found = 1 } END { exit found ? 0 : 1 }' "busybox id did not report more than 16 supplementary groups" 45 120
for expected in "HOME=/home/kaniini" "USER=kaniini" "LOGNAME=kaniini" "SHELL=/bin/sh" "PATH=/bin:/sbin:/usr/bin:/usr/sbin" "TERM=bunix"; do
	wait_for_fixed "$log" "$expected" "login environment missing" 45 160
done
wait_for_exact_line "$log" "/home/kaniini" "shell did not cd to login home directory" 45 160
wait_for_exact_line "$log" "USR_ENV_OK" "/usr/bin/env symlink did not execute" 45 160
for marker in ACCESS_X_OK ACCESS_R_OK ACCESS_DENY_OK; do
	wait_for_fixed "$log" "$marker" "linux access/faccessat regression missing" 45 160
done
wait_for_fixed "$log" "erase = ^?" "busybox stty did not report tty erase character" 45 160

wait_for_fixed "$log" "Size: 15" "busybox stat did not report /hello.txt size" 45 120
require_no_fixed "$log" "can't stat '/hello.txt'" "busybox stat failed for /hello.txt" 120

for expected in "hello.txt" "secret.txt" "musl-hello" "busybox" "cat" "stat" "File: /bin" "/bin" "File: /usr/share" "/usr/share" "File: /tmp" "/tmp" "File: /run" "/run" "File: /var/tmp" "/var/tmp" "File: '/usr/bin/env'" "/usr/bin/env" "nested" "SYMLINK_READLINK_OK" "LONG_SYMLINK_READLINK_OK" "SYMLINK_LS_OK" "lrwxrwxrwx"; do
	wait_for_fixed "$log" "$expected" "busybox directory regression missing" 45 160
done
require_no_fixed "$log" "can't stat '/bin'" "busybox directory stat failed" 160
require_no_fixed "$log" "can't stat 'busybox'" "busybox directory stat failed" 160

wait_for_fixed "$log" "NESTED_CAT_OK" "busybox nested cat did not complete" 45 180
wait_for_fixed "$log" "LONG_ROOTFS_PATH_OK" "busybox cat did not read long rootfs path" 45 180
wait_for_fixed "$log" "LONG_EXEC_PATH_OK" "long Linux execve path did not run" 45 180
wait_for_fixed "$log" "proc long cmdline ok" "long proc cmdline regression failed" 45 180
wait_for_fixed "$log" "LONG_GETCWD_OK" "busybox long getcwd did not complete" 45 180
wait_for_fixed_count "$log" "nested rootfs file" 2 "VFS did not resolve relative dot/dotdot path" 45 180
wait_for_exact_line "$log" "/tmp" "shell did not cd to /tmp" 45 180
wait_for_fixed_count "$log" "nested rootfs file" 3 "VFS did not resolve repeated slash path" 45 180
wait_for_fixed "$log" "PROCFS_STILL_OK" "procfs translator did not survive nested path tests" 45 180
wait_for_fixed "$log" "PROC_SELF_CMDLINE_CALLER_OK" "procfs self cmdline did not resolve caller" 45 180
wait_for_fixed "$log" "PROC_ARGV_CMDLINE_OK" "procfs cmdline did not expose argv bytes" 45 180

for expected in DEV_NULL_CHAR_OK DEV_ZERO_CHAR_OK DEV_CONSOLE_CHAR_OK; do
	wait_for_fixed "$log" "$expected" "devfs character-device regression missing" 45 180
done

check_fixed_markers "$log" "shell regression missing" 75 220 <<'EOF_MARKERS'
PROC_STATUS_OK
PROC_FD_OK
PROC_EXE_OK
/bin/sh
PROC_STAT_OK
PROC_IPC_OK
nodev
PROC_FILESYSTEMS_OK
Bunix virtual CPU
PROC_CPUINFO_OK
PROC_CMDLINE_OK
DEV_ZERO_STAT_OK
DEV_URANDOM_STAT_OK
DEV_ZERO_ACCESS_OK
DEV_ZERO_READ_OK
DEV_URANDOM_READ_OK
DEV_CONSOLE_BIG_END
DEV_CONSOLE_BIG_WRITE_OK
TMP_WRITE_OK
TMP_EXCL_DENY_OK
TMP_EXCL_PRESERVE_OK
TMP_CAT_OK
TMP_STAT_OK
TMP_APPEND_CAT_OK
PATHMAX2_TMP_MKDIR_OK
PATHMAX2_TMP_FILE_OK
PATHMAX2_TMP_CHDIR_OK
linux pathmax ok
TMP_MKDIR_P_EXISTING_ROOT_OK
TMP_MKDIR_P_NESTED_OK
TMP_MKDIR_EXISTING_ROOT_DENY_OK
UNION_MKDIR_P_ROOT_OK
UNION_MKDIR_P_CHILD_OK
UNION_MKDIR_EXISTING_LOWER_DENY_OK
1000:1000 644
TMP_MKDIR_OK
TMP_MKDIR_STAT_OK
TMP_LONG_READDIR_OK
TMP_LONG_NAME_CAT_OK
TMP_NAME_MAX_READDIR_OK
TMP_NAME_MAX_CAT_OK
TMP_NAME255_CREATE_OK
TMP_NAME255_CAT_OK
TMP_NAME256_DENY_OK
TMP_TOUCH_CREATE_OK
TMP_TOUCH_EXISTING_OK
TMP_MKFIFO_OK
TMP_FIFO_STAT_OK
TMP_HARDLINK_CREATE_OK
TMP_HARDLINK_SHARE_OK
TMP_HARDLINK_RENAME_OK
RMDIR_OK
RM_R_DIR_OK
TMP_DIR_RENAME_NONEMPTY_OK
TMP_DIR_RENAME_SUBTREE_OK
TMP_DIR_RENAME_OLD_GONE_OK
TMP_DIR_REPLACE_EMPTY_OK
TMP_DIR_REPLACE_SUBTREE_OK
TMP_DIR_REPLACE_NONEMPTY_DENY_OK
TMP_DIR_RENAME_DESCENDANT_DENY_OK
TMP_DIR_RENAME_DESCENDANT_PRESERVE_OK
TMP_CHMOD_OK
700
TMP_CHOWN_DENY_OK
TMP_SYMLINK_CREATE_OK
TMP_SYMLINK_READLINK_OK
TMP_SYMLINK_LS_OK
LINUX_BIG_WRITE_OK
linux mmapbig ok
MMAPBIG_OK
linux mmaphuge ok
linux readbig ok
READBIG_OK
RUN_WRITE_OK
RUN_CAT_OK
TRUNCATE_OK
TRUN
TRUNCATE_CAT_OK
UNLINK_OK
UNLINK_GONE_OK
GETDENTS64_OK
linux vforkstress ok
VFORKSTRESS_OK
linux getdents64 checks ok
linux getdents64 large checks ok
linux getrandom checks ok
linux readlinkbig ok
linux iovtest ok
IOVTEST_OK
linux fchmodat checks ok
linux waitpgid checks ok
linux execlong ok
linux auxid ok
UNION_ROOT_LOWER_OK
UNION_LOWER_RENAME_CREATE_OK
UNION_LOWER_RENAME_READ_OK
UNION_LOWER_RENAME_OLD_GONE_OK
UNION_LOWER_RENAME_UPPER_OK
UNION_LOWER_HARDLINK_CREATE_OK
UNION_LOWER_HARDLINK_READ_OK
UNION_LOWER_HARDLINK_COPYUP_OK
UNION_ROOT_APPEND_CAT_OK
UNION_ROOT_UPPER_BACKING_OK
UNION_ROOT_LONG_READDIR_OK
UNION_ROOT_LONG_CAT_OK
UNION_NAME_MAX_READDIR_OK
UNION_NAME_MAX_CAT_OK
UNION_ROOT_READDIR_UPPER_OK
UNION_ROOT_READDIR_LOWER_OK
UNION_ROOT_UNLINK_UPPER_OK
UNION_ROOT_UNLINK_UPPER_GONE_OK
UNION_ROOT_LOWER_STILL_OK
UNION_SYMLINK_CREATE_OK
UNION_SYMLINK_READLINK_OK
UNION_SYMLINK_LS_OK
UNION_HARDLINK_CREATE_OK
UNION_HARDLINK_SHARE_OK
MUSL_LDSO_CANONICAL_OK
DYN_HELLO_OK
linux exec child ok
EXECBIG_OK
PROC_TOP_OK
PROC_PS_OK
PROC_FREE_OK
unionfs on / type unionfs (rw)
rootfs on /.lower type rootfs (rw)
tmpfs on /.upper type tmpfs (rw)
tmpfs on /run type tmpfs (rw)
tmpfs on /tmp type tmpfs (rw)
tmpfs on /var/tmp type tmpfs (rw)
PROC_MOUNT_OK
MNT_OK
MNT_PAYLOAD
MNT_CAT
MNT_LIST
UMNT_OK
UMNT_GONE
UMNT_HIDE
EOF_MARKERS
wait_for_fixed "$log" "STATFS_DF_OK" "busybox df did not complete through statfs/fstatfs" 45 220
for expected in "TMP_APPEND_ONE" "TMP_APPEND_TWO" "TMP_LONG_READDIR_PAYLOAD" "TMP_NAME_MAX_PAYLOAD" "TMP_NAME255_PAYLOAD" "PATHMAX2_TMP_PAYLOAD" "UNION_ROOT_BASE_PAYLOAD" "UNION_ROOT_APPEND_PAYLOAD" "UNION_ROOT_LONG_PAYLOAD" "UNION_NAME_MAX_PAYLOAD"; do
	wait_for_fixed_count "$log" "$expected" 2 "append payload missing from file output" 45 220
done
wait_for_exact_line "$log" "DYN_HELLO_OK" "dynamic musl hello did not complete" 45 220
wait_for_exact_line "$log" "EXECBIG_OK" "large Linux executable did not complete" 45 220
wait_for_exact_line "$log" "READBIG_OK" "large Linux read did not complete" 45 220
for expected in "cpu  " "/bin/sh" "PROC_SHELL_PPID_OK" "direct_delivered " "direct_handoff "; do
	wait_for_fixed "$log" "$expected" "procfs content regression missing" 45 220
done
for expected in "direct_delivered [1-9][0-9]*" "direct_handoff [1-9][0-9]*"; do
	wait_for_regex "$log" "$expected" "IPC fast path counter did not increase" 45 220
done
for expected in "cpu[0-9][0-9]* sends [1-9][0-9]*"; do
	wait_for_regex "$log" "$expected" "IPC per-CPU counter did not increase" 45 220
done
require_no_fixed "$log" "top:" "busybox top reported an error" 220

wait_for_fixed "$log" "BUSYBOX_ARGV_OK" "busybox argv regression command did not complete" 45 160
wait_for_fixed "$log" "BUSYBOX_MANY_ARGV_OK" "busybox many-argv regression command did not complete" 45 160
wait_for_fixed "$log" "BUSYBOX_ARGV300_OK" "busybox large-argv regression command did not complete" 45 160
wait_for_fixed "$log" "BUSYBOX_LONG_ENV_OK" "busybox long-env regression command did not complete" 45 160
wait_for_fixed "$log" "BUSYBOX_ENV300_OK" "busybox large-env regression command did not complete" 45 160
wait_for_fixed "$log" "PIPE_OK" "busybox echo pipe did not complete" 45 180
wait_for_fixed "$log" "PIPE_FILE_OK" "busybox cat pipe did not complete" 45 180
wait_for_fixed "$log" "PROCFS_OK" "procfs translator read did not complete" 45 180

wait_for_fixed "$log" "BUSYBOX_SLEEP_OK" "busybox sleep did not complete" 45 160
wait_for_fixed "$log" "DIRECT_SLEEP_OK" "/bin/sleep did not complete" 45 160
wait_for_fixed "$log" "SLEEP_CTRL_C_OK" "foreground Ctrl-C did not interrupt busybox sleep" 45 180
wait_for_fixed "$log" "KILL_ZERO_OK" "busybox kill -0 did not report current shell" 45 160
wait_for_fixed "$log" "KILL_PGRP_OK" "busybox kill -0 process group probe failed" 45 160
wait_for_fixed "$log" "SIGINT_IGNORE_OK" "ignored SIGINT terminated the shell" 45 180

printf 'busybox cat /secret.txt\necho POSTCAT\n' >&3

wait_for_fixed "$log" "cat: can't open '/secret.txt'" "busybox cat did not report denied /secret.txt access" 45 160
require_no_fixed "$log" "root secret" "busybox cat leaked /secret.txt to login user" 160
wait_for_fixed "$log" "POSTCAT" "shell did not continue after denied cat" 45 160

printf 'ecxx\177\177ho BACKSPACE_OK\n' >&3

wait_for_exact_line "$log" "BACKSPACE_OK" "busybox backspace-edited command did not complete" 45 160

printf 'cat\n' >&3
sleep 1
printf '\003' >&3
sleep 1
printf 'echo CTRL_C_OK\n' >&3

wait_for_exact_line "$log" "CTRL_C_OK" "foreground Ctrl-C did not return to shell" 45 180

printf 'busybox watch -n 1 busybox echo WATCH_OK & watch_pid=$!\n' >&3

wait_for_awk "$log" '{ sub(/\r$/, "") } /^WATCH_OK$/ { count++ } END { exit count >= 2 ? 0 : 1 }' "busybox watch did not repeatedly run child command" 45 220

printf 'busybox kill $watch_pid\necho WATCH_DONE\n' >&3

wait_for_exact_line "$log" "WATCH_DONE" "busybox watch was not killed after repeated child runs" 45 220

login_prompts_before_exit=$(grep -F -c "login: " "$log" 2>/dev/null || true)
printf 'cd /bin\npwd\nexit\n' >&3

wait_for_awk "$log" '{ sub(/\r$/, "") } /\/bin \$ pwd/ { prompt = NR } /^\/bin$/ && prompt { found = 1 } END { exit found ? 0 : 1 }' "busybox chdir/pwd regression failed" 45 160

wait_for_prompt_count_gt "login: " "$login_prompts_before_exit" "login prompt did not return after shell exit" 45 180

root_prompts_before_root=$(grep -F -c "~ # " "$log" 2>/dev/null || true)
printf 'root\nroot\n' >&3

wait_for_prompt_count_gt "~ # " "$root_prompts_before_root" "root shell prompt did not appear after second login" 45 180

printf 'busybox id\nenv\nbusybox cat /secret.txt && echo ROOT_SECRET_OK\n' >&3

wait_for_fixed "$log" "uid=0(root)" "root login did not report uid 0" 45 180

for expected in "HOME=/root" "USER=root" "LOGNAME=root"; do
	wait_for_fixed "$log" "$expected" "root login environment missing" 45 180
done

wait_for_exact_line "$log" "ROOT_SECRET_OK" "root login could not read /secret.txt" 45 180
printf 'echo UNION_LOWER_PARENT_CREATE_PAYLOAD > /usr/share/bunix/nested/created.txt\n' >&3
printf 'busybox cat /usr/share/bunix/nested/created.txt && echo UNION_LOWER_PARENT_CREATE_OK\n' >&3
printf 'busybox cat /.upper/usr/share/bunix/nested/created.txt && echo UNION_LOWER_PARENT_UPPER_OK\n' >&3
printf 'echo UNION_LOWER_COPYUP_APPEND_PAYLOAD >> /usr/share/bunix/nested/hello.txt\n' >&3
printf 'busybox cat /usr/share/bunix/nested/hello.txt && echo UNION_LOWER_COPYUP_APPEND_OK\n' >&3
printf 'busybox cat /.upper/usr/share/bunix/nested/hello.txt && echo UNION_LOWER_COPYUP_UPPER_OK\n' >&3
for expected in UNION_LOWER_PARENT_CREATE_OK UNION_LOWER_PARENT_UPPER_OK UNION_LOWER_COPYUP_APPEND_OK UNION_LOWER_COPYUP_UPPER_OK; do
	wait_for_exact_line "$log" "$expected" "root unionfs lower-parent regression missing" 45 220
done
for expected in UNION_LOWER_PARENT_CREATE_PAYLOAD UNION_LOWER_COPYUP_APPEND_PAYLOAD; do
	wait_for_fixed_count "$log" "$expected" 2 "root unionfs lower-parent payload missing" 45 220
done
printf 'busybox chown 0:0 /tmp/bunix-write.txt\n' >&3
printf 'busybox stat -c "%%u:%%g" /tmp/bunix-write.txt\nexit\n' >&3
wait_for_exact_line "$log" "0:0" "tmpfs chown did not update owner" 45 180

login_prompts_before_long=$(grep -F -c "login: " "$log" 2>/dev/null || true)
wait_for_prompt_count_gt "login: " "$login_prompts_before_long" "login prompt did not return after root shell exit" 45 180

long_root_prompts_before=$(grep -F -c "~ # " "$log" 2>/dev/null || true)
printf 'administrator_with_long_name\npassword_longer_than_sixteen\n' >&3
wait_for_prompt_count_gt "~ # " "$long_root_prompts_before" "long login shell prompt did not appear" 45 180

printf 'busybox id\nenv\nexit\n' >&3
for expected in "uid=0(root)" "USER=administrator_with_long_name" "LOGNAME=administrator_with_long_name"; do
	wait_for_fixed "$log" "$expected" "long login regression missing" 45 180
done
echo "shell regression ok"
