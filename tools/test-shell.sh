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
while ! grep -F "/ $ " "$log" >/dev/null 2>&1; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "shell prompt did not appear after login" >&2
		tail -n 120 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done

printf 'uptime\nbusybox uptime\nbusybox uname\nbusybox uname -r\nbusybox stty -a\nbusybox id\nbusybox kill -0 $$ && echo KILL_ZERO_OK\nbusybox kill -0 -$$ && echo KILL_PGRP_OK\ntrap "" INT\nbusybox kill -INT $$\necho SIGINT_IGNORE_OK\ntrap - INT\nbusybox sleep 1 && echo BUSYBOX_SLEEP_OK\nsleep 1 && echo DIRECT_SLEEP_OK\nbusybox sleep 5\n\003echo SLEEP_CTRL_C_OK\nbusybox echo PIPE_OK | busybox cat\nbusybox cat /hello.txt | busybox cat && echo PIPE_FILE_OK\nbusybox cat /usr/share/bunix/nested/hello.txt && echo NESTED_CAT_OK\nbusybox cat /proc/kthreads >/dev/null && echo PROCFS_OK\nbusybox echo BUSYBOX_ARGV_OK\nbusybox stat /hello.txt\nbusybox stat /usr/share\nbusybox ls /\nbusybox ls /bin\nbusybox readlink /bin/cat && echo SYMLINK_READLINK_OK\nbusybox ls -l /bin/cat && echo SYMLINK_LS_OK\nbusybox ls /usr/share/bunix/nested\nbusybox stat /bin\ncd /usr/share/bunix/nested\npwd\nbusybox ls .\nbusybox cat ../nested/./hello.txt && echo VFS_DOTDOT_OK\ncd /\nbusybox cat //usr///share/bunix/nested/hello.txt && echo VFS_SLASH_OK\nbusybox cat /proc/kthreads >/dev/null && echo PROCFS_STILL_OK\nbusybox cat /proc/self/status && echo PROC_STATUS_OK\nbusybox ls /proc/self/fd && echo PROC_FD_OK\nbusybox cat /proc/stat && echo PROC_STAT_OK\nbusybox cat /proc/1/cmdline && echo PROC_CMDLINE_OK\nbusybox top -b -n 1 >/dev/null && echo PROC_TOP_OK\nbusybox ps && echo PROC_PS_OK\nbusybox free && echo PROC_FREE_OK\nbusybox mount && echo PROC_MOUNT_OK\n' >&3

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

for expected in "hello.txt" "secret.txt" "musl-hello" "busybox" "cat" "stat" "File: /bin" "/bin" "File: /usr/share" "/usr/share" "nested" "SYMLINK_READLINK_OK" "SYMLINK_LS_OK" "lrwxrwxrwx"; do
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
while [ "$(grep -F -c "nested rootfs file" "$log" 2>/dev/null || true)" -lt 2 ]; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "VFS did not resolve relative dot/dotdot path" >&2
		tail -n 180 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done

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

for expected in "PROC_STATUS_OK" "PROC_FD_OK" "PROC_STAT_OK" "PROC_CMDLINE_OK" "PROC_TOP_OK" "PROC_PS_OK" "PROC_FREE_OK" "PROC_MOUNT_OK"; do
	i=0
	while ! grep -F "$expected" "$log" >/dev/null 2>&1; do
		i=$((i + 1))
		if [ "$i" -gt 45 ]; then
			echo "procfs regression missing: $expected" >&2
			tail -n 220 "$log" >&2 || true
			exit 1
		fi
		sleep 1
	done
done
for expected in "cpu  " "/bin/lxtest"; do
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

printf 'busybox watch -n 1 busybox echo WATCH_OK & watch_pid=$!\nbusybox sleep 3\nbusybox kill $watch_pid\necho WATCH_DONE\n' >&3

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

if ! awk '{ sub(/\r$/, "") } /login: session ended/ { ended = NR } /login: $/ && ended && NR > ended { found = 1 } END { exit found ? 0 : 1 }' "$log"; then
	echo "login prompt did not return after shell exit" >&2
	tail -n 180 "$log" >&2 || true
	exit 1
fi
echo "shell regression ok"
