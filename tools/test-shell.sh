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

i=0
while ! grep -F "login: " "$log" >/dev/null 2>&1; do
	i=$((i + 1))
	if ! kill -0 "$qemu_pid" 2>/dev/null; then
		echo "qemu exited before login prompt" >&2
		cat "$qemu_log" >&2 || true
		tail -n 80 "$log" >&2 || true
		exit 1
	fi
	if [ "$i" -gt 80 ]; then
		echo "login prompt did not appear" >&2
		cat "$qemu_log" >&2 || true
		tail -n 80 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done

sleep 3
exec 3>"$pipe.in"
printf 'kaniini\nbunix\n' >&3

i=0
while ! grep -F "~ $ " "$log" >/dev/null 2>&1; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "shell prompt did not appear after login" >&2
		tail -n 120 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done

printf 'uptime\nbusybox uptime\nbusybox uname\nbusybox uname -r\nbusybox stty -a\nbusybox id\nenv\n/usr/bin/env >/dev/null && echo USR_ENV_OK\nbusybox test -x /bin/sh && echo ACCESS_X_OK\nbusybox test -r /hello.txt && echo ACCESS_R_OK\nbusybox test ! -r /secret.txt && echo ACCESS_DENY_OK\ncd ~\npwd\ncd /\nbusybox kill -0 $$ && echo KILL_ZERO_OK\nbusybox kill -0 -$$ && echo KILL_PGRP_OK\ntrap "" INT\nbusybox kill -INT $$\necho SIGINT_IGNORE_OK\ntrap - INT\nbusybox sleep 1 && echo BUSYBOX_SLEEP_OK\nsleep 1 && echo DIRECT_SLEEP_OK\nbusybox sleep 5\n\003echo SLEEP_CTRL_C_OK\nbusybox echo PIPE_OK | busybox cat\nbusybox cat /hello.txt | busybox cat && echo PIPE_FILE_OK\nbusybox cat /usr/share/bunix/nested/hello.txt && echo NESTED_CAT_OK\nbusybox cat /usr/share/bunix/alpine/very/long/rootfs/path/that/exceeds/the/old/two-hundred-fifty-six-byte/rootfs-entry-limit/path-component-that-forces-the-rootfs-image-format-past-the-old-limit/path-component-that-forces-the-rootfs-image-format-past-the-old-limit/with-extra-components/hello.txt && echo LONG_ROOTFS_PATH_OK\n/usr/share/bunix/alpine/very/long/rootfs/path/that/exceeds/the/old/two-hundred-fifty-six-byte/linux-execve-path-limit/path-component-that-forces-the-rootfs-image-format-past-the-old-limit/path-component-that-forces-the-rootfs-image-format-past-the-old-limit/with-extra-components/dyn-hello && echo LONG_EXEC_PATH_OK\nbusybox cat /proc/kthreads >/dev/null && echo PROCFS_OK\nbusybox echo BUSYBOX_ARGV_OK\n' >&3
printf 'busybox sh -c "test \"\\$13\" = m && echo BUSYBOX_MANY_ARGV_OK" _ a b c d e f g h i j k l m\nBUNIX_LONG_ENV=abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 busybox sh -c "test \"\\$BUNIX_LONG_ENV\" = abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 && echo BUSYBOX_LONG_ENV_OK"\n' >&3
printf 'set --\ni=1\nwhile [ "$i" -le 300 ]; do set -- "$@" "x$i"; i=$((i + 1)); done\nbusybox sh -c '"'"'test "$300" = x300 && echo BUSYBOX_ARGV300_OK'"'"' _ "$@"\ni=1\nwhile [ "$i" -le 300 ]; do eval "BUNIX_EXEC_ENV_$i=x$i; export BUNIX_EXEC_ENV_$i"; i=$((i + 1)); done\nbusybox sh -c '"'"'test "$BUNIX_EXEC_ENV_300" = x300 && echo BUSYBOX_ENV300_OK'"'"'\n' >&3
printf 'busybox stat /hello.txt\nbusybox stat /usr/share\nbusybox stat /tmp\nbusybox stat /run\nbusybox stat /var/tmp\nbusybox stat /usr/bin/env\nbusybox ls /\nbusybox ls /bin\nbusybox readlink /bin/cat && echo SYMLINK_READLINK_OK\nbusybox readlink /usr/share/bunix/long-target-link | busybox grep "/usr/share/bunix/alpine/very/long/rootfs/path/that/exceeds/the/old/two-hundred-fifty-six-byte/rootfs-entry-limit/path-component-that-forces-the-rootfs-image-format-past-the-old-limit/path-component-that-forces-the-rootfs-image-format-past-the-old-limit/with-extra-components/hello.txt" && echo LONG_SYMLINK_READLINK_OK\nbusybox ls -l /bin/cat && echo SYMLINK_LS_OK\nbusybox ls /usr/share/bunix/nested\nbusybox stat /bin\ncd /tmp\npwd\ncd /usr/share/bunix/nested\npwd\nbusybox ls .\nbusybox cat ../nested/./hello.txt && echo VFS_DOTDOT_OK\ncd /usr/share/bunix/alpine/very/long/rootfs/path/that/exceeds/the/old/two-hundred-fifty-six-byte/rootfs-entry-limit/path-component-that-forces-the-rootfs-image-format-past-the-old-limit/path-component-that-forces-the-rootfs-image-format-past-the-old-limit/with-extra-components\ntest "$(pwd)" = "/usr/share/bunix/alpine/very/long/rootfs/path/that/exceeds/the/old/two-hundred-fifty-six-byte/rootfs-entry-limit/path-component-that-forces-the-rootfs-image-format-past-the-old-limit/path-component-that-forces-the-rootfs-image-format-past-the-old-limit/with-extra-components" && echo LONG_GETCWD_OK\ncd /\nbusybox cat //usr///share/bunix/nested/hello.txt && echo VFS_SLASH_OK\nbusybox cat /proc/kthreads >/dev/null && echo PROCFS_STILL_OK\nbusybox cat /proc/self/status && echo PROC_STATUS_OK\nbusybox ls /proc/self/fd && echo PROC_FD_OK\nbusybox readlink /proc/self/exe && echo PROC_EXE_OK\nbusybox cat /proc/stat && echo PROC_STAT_OK\nbusybox cat /proc/ipc && echo PROC_IPC_OK\nbusybox cat /proc/filesystems && echo PROC_FILESYSTEMS_OK\nbusybox cat /proc/cpuinfo && echo PROC_CPUINFO_OK\nbusybox cat /proc/self/cmdline && echo PROC_CMDLINE_OK\nbusybox stat /dev/zero && echo DEV_ZERO_STAT_OK\nbusybox stat /dev/urandom && echo DEV_URANDOM_STAT_OK\nbusybox test -r /dev/zero && echo DEV_ZERO_ACCESS_OK\nbusybox head -c 4 /dev/zero >/dev/null && echo DEV_ZERO_READ_OK\nbusybox head -c 4 /dev/urandom >/dev/null && echo DEV_URANDOM_READ_OK\necho TMP_WRITE_OK > /tmp/bunix-write.txt\nbusybox cat /tmp/bunix-write.txt && echo TMP_CAT_OK\nbusybox stat /tmp/bunix-write.txt && echo TMP_STAT_OK\necho TMP_APPEND_ONE > /tmp/bunix-append.txt\necho TMP_APPEND_TWO >> /tmp/bunix-append.txt\nbusybox cat /tmp/bunix-append.txt && echo TMP_APPEND_CAT_OK\nbusybox sh -c "echo RUN_WRITE_OK > /run/bunix-run.txt"\nbusybox cat /run/bunix-run.txt && echo RUN_CAT_OK\necho TRUNCATE_PAYLOAD > /tmp/bunix-trunc.txt\nbusybox truncate -s 4 /tmp/bunix-trunc.txt && echo TRUNCATE_OK\nbusybox cat /tmp/bunix-trunc.txt && echo TRUNCATE_CAT_OK\nbusybox rm /tmp/bunix-trunc.txt && echo UNLINK_OK\nbusybox test ! -e /tmp/bunix-trunc.txt && echo UNLINK_GONE_OK\n/bin/getdentstest && echo GETDENTS64_OK\nbusybox test ! -e /lib/ld.so && echo MUSL_LDSO_CANONICAL_OK\n/bin/dyn-hello && echo DYN_HELLO_OK\nbusybox top -b -n 1 >/dev/null && echo PROC_TOP_OK\nbusybox ps && echo PROC_PS_OK\nbusybox free && echo PROC_FREE_OK\nbusybox mount && echo PROC_MOUNT_OK\n' >&3
printf '/bin/execbig && echo EXECBIG_OK\n' >&3
printf 'LONG_TMP=/tmp/bunix-pathmax2\nbusybox mkdir "$LONG_TMP"\nLONG_TMP=$LONG_TMP/segment-aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\nbusybox mkdir "$LONG_TMP"\nLONG_TMP=$LONG_TMP/segment-bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\nbusybox mkdir "$LONG_TMP"\nLONG_TMP=$LONG_TMP/segment-cccccccccccccccccccccccccccccccc\nbusybox mkdir "$LONG_TMP"\nLONG_TMP=$LONG_TMP/segment-dddddddddddddddddddddddddddddddd\nbusybox mkdir "$LONG_TMP"\nLONG_TMP=$LONG_TMP/segment-eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee\nbusybox mkdir "$LONG_TMP"\nLONG_TMP=$LONG_TMP/segment-ffffffffffffffffffffffffffffffff\nbusybox mkdir "$LONG_TMP" && echo PATHMAX2_TMP_MKDIR_OK\necho PATHMAX2_TMP_PAYLOAD > "$LONG_TMP/file.txt"\nbusybox cat "$LONG_TMP/file.txt" && echo PATHMAX2_TMP_FILE_OK\ncd "$LONG_TMP" && echo PATHMAX2_TMP_CHDIR_OK\npwd\ncd /\n' >&3
printf 'busybox df / /tmp /proc >/dev/null && echo STATFS_DF_OK\n' >&3
printf 'busybox cat /hello.txt && echo UNION_ROOT_LOWER_OK\necho UNION_ROOT_BASE_PAYLOAD > /created.txt\necho UNION_ROOT_APPEND_PAYLOAD >> /created.txt\nbusybox cat /created.txt && echo UNION_ROOT_APPEND_CAT_OK\nbusybox cat /.upper/created.txt && echo UNION_ROOT_UPPER_BACKING_OK\nUNION_LONG_NAME=bunix-union-root-name-longer-than-sixteen.txt\necho UNION_ROOT_LONG_PAYLOAD > "/$UNION_LONG_NAME"\nbusybox ls / | busybox grep "$UNION_LONG_NAME" && echo UNION_ROOT_LONG_READDIR_OK\nbusybox cat "/$UNION_LONG_NAME" && echo UNION_ROOT_LONG_CAT_OK\nbusybox ls / | busybox grep created.txt && echo UNION_ROOT_READDIR_UPPER_OK\nbusybox ls / | busybox grep secret.txt && echo UNION_ROOT_READDIR_LOWER_OK\nbusybox rm /created.txt && echo UNION_ROOT_UNLINK_UPPER_OK\nbusybox test ! -e /created.txt && echo UNION_ROOT_UNLINK_UPPER_GONE_OK\nbusybox cat /hello.txt && echo UNION_ROOT_LOWER_STILL_OK\n' >&3
printf 'busybox stat -c "%%u:%%g %%a" /tmp/bunix-write.txt\nbusybox mkdir /tmp/bunix-dir && echo TMP_MKDIR_OK\nbusybox test -d /tmp/bunix-dir && echo TMP_MKDIR_STAT_OK\nLONG_TMP_NAME=bunix-readdir-name-longer-than-sixteen.txt\necho TMP_LONG_READDIR_PAYLOAD > "/tmp/$LONG_TMP_NAME"\nbusybox ls /tmp | busybox grep "$LONG_TMP_NAME" && echo TMP_LONG_READDIR_OK\nbusybox cat "/tmp/$LONG_TMP_NAME" && echo TMP_LONG_NAME_CAT_OK\nbusybox mkdir /tmp/bunix-rmdir\nbusybox rmdir /tmp/bunix-rmdir\nbusybox test ! -e /tmp/bunix-rmdir && echo RMDIR_OK\nbusybox chmod 700 /tmp/bunix-write.txt && echo TMP_CHMOD_OK\nbusybox stat -c "%%a" /tmp/bunix-write.txt\nbusybox chown root:root /tmp/bunix-write.txt || echo TMP_CHOWN_DENY_OK\n' >&3
printf 'busybox head -c 8192 /dev/zero > /tmp/linux-big-write.bin\nbusybox test "$(busybox stat -c "%%s" /tmp/linux-big-write.bin)" = "8192" && echo LINUX_BIG_WRITE_OK\n' >&3
sleep 1
printf 'mount -t tmpfs tmpfs /mnt&&echo MNT_OK\necho MNT_PAYLOAD>/mnt/linux-mount.txt\ncat /mnt/linux-mount.txt&&echo MNT_CAT\nmount|busybox grep /mnt&&echo MNT_LIST\numount /mnt&&echo UMNT_OK\nmount|busybox grep /mnt||echo UMNT_GONE\ntest ! -e /mnt/linux-mount.txt&&echo UMNT_HIDE\n' >&3

i=0
while ! grep -F "load average" "$log" >/dev/null 2>&1; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "busybox uptime did not run through applet argv" >&2
		tail -n 120 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done

i=0
while ! grep -F " 1 users" "$log" >/dev/null 2>&1; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "busybox uptime did not report login session" >&2
		tail -n 160 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done

i=0
while ! grep -F "Bunix" "$log" >/dev/null 2>&1; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "busybox uname did not report Bunix" >&2
		tail -n 120 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done

i=0
while ! grep -F "0.1" "$log" >/dev/null 2>&1; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "busybox uname -r did not report 0.1" >&2
		tail -n 120 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done

i=0
while ! grep -F "uid=1000" "$log" >/dev/null 2>&1; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "busybox id did not report login identity" >&2
		tail -n 120 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done
if ! awk '{ sub(/\r$/, "") } /groups=1(\(wheel\))?,1000(\(kaniini\))?/ { found = 1 } END { exit found ? 0 : 1 }' "$log"; then
	echo "busybox id did not report login supplementary groups" >&2
	tail -n 120 "$log" >&2 || true
	exit 1
fi
if ! awk '{ sub(/\r$/, "") } /2018(\(g18\))?/ { found = 1 } END { exit found ? 0 : 1 }' "$log"; then
	echo "busybox id did not report more than 16 supplementary groups" >&2
	tail -n 120 "$log" >&2 || true
	exit 1
fi
for expected in "HOME=/home/kaniini" "USER=kaniini" "LOGNAME=kaniini" "SHELL=/bin/sh" "PATH=/bin:/sbin:/usr/bin:/usr/sbin" "TERM=bunix"; do
	i=0
	while ! grep -F "$expected" "$log" >/dev/null 2>&1; do
		i=$((i + 1))
		if [ "$i" -gt 45 ]; then
			echo "login environment missing: $expected" >&2
			tail -n 160 "$log" >&2 || true
			exit 1
		fi
		sleep 1
	done
done
i=0
while ! awk '{ sub(/\r$/, "") } /^\/home\/kaniini$/ { found = 1 } END { exit found ? 0 : 1 }' "$log"; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "shell did not cd to login home directory" >&2
		tail -n 160 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done
if ! awk '{ sub(/\r$/, "") } /^USR_ENV_OK$/ { found = 1 } END { exit found ? 0 : 1 }' "$log"; then
	echo "/usr/bin/env symlink did not execute" >&2
	tail -n 160 "$log" >&2 || true
	exit 1
fi
for marker in ACCESS_X_OK ACCESS_R_OK ACCESS_DENY_OK; do
	i=0
	while ! grep -aF "$marker" "$log" >/dev/null 2>&1; do
		i=$((i + 1))
		if [ "$i" -gt 45 ]; then
			echo "linux access/faccessat regression missing: $marker" >&2
			tail -n 160 "$log" >&2 || true
			exit 1
		fi
		sleep 1
	done
done
i=0
while ! grep -F "erase = ^?" "$log" >/dev/null 2>&1; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "busybox stty did not report tty erase character" >&2
		tail -n 160 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done

i=0
while ! grep -F "Size: 15" "$log" >/dev/null 2>&1; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "busybox stat did not report /hello.txt size" >&2
		tail -n 120 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done
if grep -F "can't stat '/hello.txt'" "$log" >/dev/null 2>&1; then
	echo "busybox stat failed for /hello.txt" >&2
	tail -n 120 "$log" >&2 || true
	exit 1
fi

for expected in "hello.txt" "secret.txt" "musl-hello" "busybox" "cat" "stat" "File: /bin" "/bin" "File: /usr/share" "/usr/share" "File: /tmp" "/tmp" "File: /run" "/run" "File: /var/tmp" "/var/tmp" "File: '/usr/bin/env'" "/usr/bin/env" "nested" "SYMLINK_READLINK_OK" "LONG_SYMLINK_READLINK_OK" "SYMLINK_LS_OK" "lrwxrwxrwx"; do
	i=0
	while ! grep -F "$expected" "$log" >/dev/null 2>&1; do
		i=$((i + 1))
		if [ "$i" -gt 45 ]; then
			echo "busybox directory regression missing: $expected" >&2
			tail -n 160 "$log" >&2 || true
			exit 1
		fi
		sleep 1
	done
done
if grep -F "can't stat '/bin'" "$log" >/dev/null 2>&1 ||
   grep -F "can't stat 'busybox'" "$log" >/dev/null 2>&1; then
	echo "busybox directory stat failed" >&2
	tail -n 160 "$log" >&2 || true
	exit 1
fi

i=0
while ! grep -F "NESTED_CAT_OK" "$log" >/dev/null 2>&1; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "busybox nested cat did not complete" >&2
		tail -n 180 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done
i=0
while ! grep -F "LONG_ROOTFS_PATH_OK" "$log" >/dev/null 2>&1; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "busybox cat did not read long rootfs path" >&2
		tail -n 180 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done
i=0
while ! grep -F "LONG_EXEC_PATH_OK" "$log" >/dev/null 2>&1; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "long Linux execve path did not run" >&2
		tail -n 180 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done

i=0
while ! grep -F "proc long cmdline ok" "$log" >/dev/null 2>&1; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "long proc cmdline regression failed" >&2
		tail -n 180 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done
i=0
while ! grep -F "LONG_GETCWD_OK" "$log" >/dev/null 2>&1; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "busybox long getcwd did not complete" >&2
		tail -n 180 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done
i=0
while [ "$(grep -F -c "nested rootfs file" "$log" 2>/dev/null || true)" -lt 2 ]; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "VFS did not resolve relative dot/dotdot path" >&2
		tail -n 180 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done
if ! awk '{ sub(/\r$/, "") } /^\/tmp$/ { found = 1 } END { exit found ? 0 : 1 }' "$log"; then
	echo "shell did not cd to /tmp" >&2
	tail -n 180 "$log" >&2 || true
	exit 1
fi

i=0
while [ "$(grep -F -c "nested rootfs file" "$log" 2>/dev/null || true)" -lt 3 ]; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "VFS did not resolve repeated slash path" >&2
		tail -n 180 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done

i=0
while ! grep -F "PROCFS_STILL_OK" "$log" >/dev/null 2>&1; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "procfs translator did not survive nested path tests" >&2
		tail -n 180 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done

for expected in "PROC_STATUS_OK" "PROC_FD_OK" "PROC_EXE_OK" "/bin/sh" "PROC_STAT_OK" "PROC_IPC_OK" "nodev" "PROC_FILESYSTEMS_OK" "Bunix virtual CPU" "PROC_CPUINFO_OK" "PROC_CMDLINE_OK" "DEV_ZERO_STAT_OK" "DEV_URANDOM_STAT_OK" "DEV_ZERO_ACCESS_OK" "DEV_ZERO_READ_OK" "DEV_URANDOM_READ_OK" "TMP_WRITE_OK" "TMP_CAT_OK" "TMP_STAT_OK" "TMP_APPEND_CAT_OK" "PATHMAX2_TMP_MKDIR_OK" "PATHMAX2_TMP_FILE_OK" "PATHMAX2_TMP_CHDIR_OK" "1000:1000 644" "TMP_MKDIR_OK" "TMP_MKDIR_STAT_OK" "TMP_LONG_READDIR_OK" "TMP_LONG_NAME_CAT_OK" "RMDIR_OK" "TMP_CHMOD_OK" "700" "TMP_CHOWN_DENY_OK" "LINUX_BIG_WRITE_OK" "RUN_WRITE_OK" "RUN_CAT_OK" "TRUNCATE_OK" "TRUN" "TRUNCATE_CAT_OK" "UNLINK_OK" "UNLINK_GONE_OK" "GETDENTS64_OK" "linux getdents64 checks ok" "UNION_ROOT_LOWER_OK" "UNION_ROOT_APPEND_CAT_OK" "UNION_ROOT_UPPER_BACKING_OK" "UNION_ROOT_LONG_READDIR_OK" "UNION_ROOT_LONG_CAT_OK" "UNION_ROOT_READDIR_UPPER_OK" "UNION_ROOT_READDIR_LOWER_OK" "UNION_ROOT_UNLINK_UPPER_OK" "UNION_ROOT_UNLINK_UPPER_GONE_OK" "UNION_ROOT_LOWER_STILL_OK" "MUSL_LDSO_CANONICAL_OK" "DYN_HELLO_OK" "linux exec child ok" "EXECBIG_OK" "PROC_TOP_OK" "PROC_PS_OK" "PROC_FREE_OK" "unionfs on / type unionfs (rw)" "rootfs on /.lower type rootfs (rw)" "tmpfs on /.upper type tmpfs (rw)" "tmpfs on /tmp type tmpfs (rw)" "tmpfs on /run type tmpfs (rw)" "tmpfs on /var/tmp type tmpfs (rw)" "PROC_MOUNT_OK" "MNT_OK" "MNT_PAYLOAD" "MNT_CAT" "MNT_LIST" "UMNT_OK" "UMNT_GONE" "UMNT_HIDE"; do
	i=0
	while ! grep -F "$expected" "$log" >/dev/null 2>&1; do
		i=$((i + 1))
		if [ "$i" -gt 75 ]; then
			echo "procfs regression missing: $expected" >&2
			tail -n 220 "$log" >&2 || true
			exit 1
		fi
		sleep 1
	done
done
i=0
while ! grep -F "STATFS_DF_OK" "$log" >/dev/null 2>&1; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "busybox df did not complete through statfs/fstatfs" >&2
		tail -n 220 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done
for expected in "TMP_APPEND_ONE" "TMP_APPEND_TWO" "TMP_LONG_READDIR_PAYLOAD" "PATHMAX2_TMP_PAYLOAD" "UNION_ROOT_BASE_PAYLOAD" "UNION_ROOT_APPEND_PAYLOAD" "UNION_ROOT_LONG_PAYLOAD"; do
	i=0
	while [ "$(grep -aF -c "$expected" "$log" 2>/dev/null || true)" -lt 2 ]; do
		i=$((i + 1))
		if [ "$i" -gt 45 ]; then
			echo "append payload missing from file output: $expected" >&2
			tail -n 220 "$log" >&2 || true
			exit 1
		fi
		sleep 1
	done
done
if ! awk '{ sub(/\r$/, "") } /^DYN_HELLO_OK$/ { found = 1 } END { exit found ? 0 : 1 }' "$log"; then
	echo "dynamic musl hello did not complete" >&2
	tail -n 220 "$log" >&2 || true
	exit 1
fi
if ! awk '{ sub(/\r$/, "") } /^EXECBIG_OK$/ { found = 1 } END { exit found ? 0 : 1 }' "$log"; then
	echo "large Linux executable did not complete" >&2
	tail -n 220 "$log" >&2 || true
	exit 1
fi
for expected in "cpu  " "/bin/sh" "PPid:	1" "direct_delivered " "direct_handoff "; do
	i=0
	while ! grep -F "$expected" "$log" >/dev/null 2>&1; do
		i=$((i + 1))
		if [ "$i" -gt 45 ]; then
			echo "procfs content regression missing: $expected" >&2
			tail -n 220 "$log" >&2 || true
			exit 1
		fi
		sleep 1
	done
done
for expected in "direct_delivered [1-9][0-9]*" "direct_handoff [1-9][0-9]*"; do
	i=0
	while ! grep -E "$expected" "$log" >/dev/null 2>&1; do
		i=$((i + 1))
		if [ "$i" -gt 45 ]; then
			echo "IPC fast path counter did not increase: $expected" >&2
			tail -n 220 "$log" >&2 || true
			exit 1
		fi
		sleep 1
	done
done
for expected in "cpu[0-9][0-9]* sends [1-9][0-9]*"; do
	i=0
	while ! grep -E "$expected" "$log" >/dev/null 2>&1; do
		i=$((i + 1))
		if [ "$i" -gt 45 ]; then
			echo "IPC per-CPU counter did not increase: $expected" >&2
			tail -n 220 "$log" >&2 || true
			exit 1
		fi
		sleep 1
	done
done
if grep -F "top:" "$log" >/dev/null 2>&1; then
	echo "busybox top reported an error" >&2
	tail -n 220 "$log" >&2 || true
	exit 1
fi

i=0
while ! grep -F "BUSYBOX_ARGV_OK" "$log" >/dev/null 2>&1; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "busybox argv regression command did not complete" >&2
		tail -n 160 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done

i=0
while ! grep -F "BUSYBOX_MANY_ARGV_OK" "$log" >/dev/null 2>&1; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "busybox many-argv regression command did not complete" >&2
		tail -n 160 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done

i=0
while ! grep -F "BUSYBOX_ARGV300_OK" "$log" >/dev/null 2>&1; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "busybox large-argv regression command did not complete" >&2
		tail -n 160 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done

i=0
while ! grep -F "BUSYBOX_LONG_ENV_OK" "$log" >/dev/null 2>&1; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "busybox long-env regression command did not complete" >&2
		tail -n 160 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done

i=0
while ! grep -F "BUSYBOX_ENV300_OK" "$log" >/dev/null 2>&1; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "busybox large-env regression command did not complete" >&2
		tail -n 160 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done

i=0
while ! grep -F "PIPE_OK" "$log" >/dev/null 2>&1; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "busybox echo pipe did not complete" >&2
		tail -n 180 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done

i=0
while ! grep -F "PIPE_FILE_OK" "$log" >/dev/null 2>&1; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "busybox cat pipe did not complete" >&2
		tail -n 180 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done

i=0
while ! grep -F "PROCFS_OK" "$log" >/dev/null 2>&1; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "procfs translator read did not complete" >&2
		tail -n 180 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done

i=0
while ! grep -F "BUSYBOX_SLEEP_OK" "$log" >/dev/null 2>&1; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "busybox sleep did not complete" >&2
		tail -n 160 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done

i=0
while ! grep -F "DIRECT_SLEEP_OK" "$log" >/dev/null 2>&1; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "/bin/sleep did not complete" >&2
		tail -n 160 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done

i=0
while ! grep -F "SLEEP_CTRL_C_OK" "$log" >/dev/null 2>&1; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "foreground Ctrl-C did not interrupt busybox sleep" >&2
		tail -n 180 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done

i=0
while ! grep -F "KILL_ZERO_OK" "$log" >/dev/null 2>&1; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "busybox kill -0 did not report current shell" >&2
		tail -n 160 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done

i=0
while ! grep -F "KILL_PGRP_OK" "$log" >/dev/null 2>&1; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "busybox kill -0 process group probe failed" >&2
		tail -n 160 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done

i=0
while ! grep -F "SIGINT_IGNORE_OK" "$log" >/dev/null 2>&1; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "ignored SIGINT terminated the shell" >&2
		tail -n 180 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done

printf 'busybox cat /secret.txt\necho POSTCAT\n' >&3

i=0
while ! grep -F "cat: can't open '/secret.txt'" "$log" >/dev/null 2>&1; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "busybox cat did not report denied /secret.txt access" >&2
		tail -n 160 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done
if grep -F "root secret" "$log" >/dev/null 2>&1; then
	echo "busybox cat leaked /secret.txt to login user" >&2
	tail -n 160 "$log" >&2 || true
	exit 1
fi

i=0
while ! grep -F "POSTCAT" "$log" >/dev/null 2>&1; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "shell did not continue after denied cat" >&2
		tail -n 160 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done

printf 'ecxx\177\177ho BACKSPACE_OK\n' >&3

i=0
while ! awk '{ sub(/\r$/, "") } /^BACKSPACE_OK$/ { found = 1 } END { exit found ? 0 : 1 }' "$log"; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "busybox backspace-edited command did not complete" >&2
		tail -n 160 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done

printf 'cat\n' >&3
sleep 1
printf '\003' >&3
sleep 1
printf 'echo CTRL_C_OK\n' >&3

i=0
while ! awk '{ sub(/\r$/, "") } /^CTRL_C_OK$/ { found = 1 } END { exit found ? 0 : 1 }' "$log"; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "foreground Ctrl-C did not return to shell" >&2
		tail -n 180 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done

printf 'busybox watch -n 1 busybox echo WATCH_OK & watch_pid=$!\n' >&3

i=0
while ! awk '{ sub(/\r$/, "") } /^WATCH_OK$/ { count++ } END { exit count >= 2 ? 0 : 1 }' "$log"; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "busybox watch did not repeatedly run child command" >&2
		tail -n 220 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done

printf 'busybox kill $watch_pid\necho WATCH_DONE\n' >&3

i=0
while ! awk '{ sub(/\r$/, "") } /^WATCH_DONE$/ { found = 1 } END { exit found ? 0 : 1 }' "$log"; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "busybox watch was not killed after repeated child runs" >&2
		tail -n 220 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done

login_prompts_before_exit=$(grep -F -c "login: " "$log" 2>/dev/null || true)
printf 'cd /bin\npwd\nexit\n' >&3

i=0
while ! awk '{ sub(/\r$/, "") } /\/bin \$ pwd/ { prompt = NR } /^\/bin$/ && prompt { found = 1 } END { exit found ? 0 : 1 }' "$log"; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "busybox chdir/pwd regression failed" >&2
		tail -n 160 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done

i=0
while [ "$(grep -F -c "login: " "$log" 2>/dev/null || true)" -le "$login_prompts_before_exit" ]; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "login prompt did not return after shell exit" >&2
		tail -n 180 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done

root_prompts_before_root=$(grep -F -c "~ # " "$log" 2>/dev/null || true)
printf 'root\nroot\n' >&3

i=0
while [ "$(grep -F -c "~ # " "$log" 2>/dev/null || true)" -le "$root_prompts_before_root" ]; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "root shell prompt did not appear after second login" >&2
		tail -n 180 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done

printf 'busybox id\nenv\nbusybox cat /secret.txt && echo ROOT_SECRET_OK\n' >&3

i=0
while ! grep -F "uid=0(root)" "$log" >/dev/null 2>&1; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "root login did not report uid 0" >&2
		tail -n 180 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done

for expected in "HOME=/root" "USER=root" "LOGNAME=root"; do
	i=0
	while ! grep -F "$expected" "$log" >/dev/null 2>&1; do
		i=$((i + 1))
		if [ "$i" -gt 45 ]; then
			echo "root login environment missing: $expected" >&2
			tail -n 180 "$log" >&2 || true
			exit 1
		fi
		sleep 1
	done
done

i=0
while ! awk '{ sub(/\r$/, "") } /^ROOT_SECRET_OK$/ { found = 1 } END { exit found ? 0 : 1 }' "$log"; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "root login could not read /secret.txt" >&2
		tail -n 180 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done
printf 'echo UNION_LOWER_PARENT_CREATE_PAYLOAD > /usr/share/bunix/nested/created.txt\n' >&3
printf 'busybox cat /usr/share/bunix/nested/created.txt && echo UNION_LOWER_PARENT_CREATE_OK\n' >&3
printf 'busybox cat /.upper/usr/share/bunix/nested/created.txt && echo UNION_LOWER_PARENT_UPPER_OK\n' >&3
printf 'echo UNION_LOWER_COPYUP_APPEND_PAYLOAD >> /usr/share/bunix/nested/hello.txt\n' >&3
printf 'busybox cat /usr/share/bunix/nested/hello.txt && echo UNION_LOWER_COPYUP_APPEND_OK\n' >&3
printf 'busybox cat /.upper/usr/share/bunix/nested/hello.txt && echo UNION_LOWER_COPYUP_UPPER_OK\n' >&3
for expected in UNION_LOWER_PARENT_CREATE_OK UNION_LOWER_PARENT_UPPER_OK UNION_LOWER_COPYUP_APPEND_OK UNION_LOWER_COPYUP_UPPER_OK; do
	i=0
	while ! awk -v expected="$expected" '{ sub(/\r$/, "") } $0 == expected { found = 1 } END { exit found ? 0 : 1 }' "$log"; do
		i=$((i + 1))
		if [ "$i" -gt 45 ]; then
			echo "root unionfs lower-parent regression missing: $expected" >&2
			tail -n 220 "$log" >&2 || true
			exit 1
		fi
		sleep 1
	done
done
for expected in UNION_LOWER_PARENT_CREATE_PAYLOAD UNION_LOWER_COPYUP_APPEND_PAYLOAD; do
	i=0
	while [ "$(grep -aF -c "$expected" "$log" 2>/dev/null || true)" -lt 2 ]; do
		i=$((i + 1))
		if [ "$i" -gt 45 ]; then
			echo "root unionfs lower-parent payload missing: $expected" >&2
			tail -n 220 "$log" >&2 || true
			exit 1
		fi
		sleep 1
	done
done
printf 'busybox chown 0:0 /tmp/bunix-write.txt\n' >&3
printf 'busybox stat -c "%%u:%%g" /tmp/bunix-write.txt\nexit\n' >&3
i=0
while ! awk '{ sub(/\r$/, "") } /^0:0$/ { found = 1 } END { exit found ? 0 : 1 }' "$log"; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "tmpfs chown did not update owner" >&2
		tail -n 180 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done
echo "shell regression ok"
