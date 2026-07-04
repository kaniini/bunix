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
	rm -rf "$tmp"
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

printf 'uptime\nbusybox uptime\nbusybox stty -a\nbusybox id\nbusybox echo BUSYBOX_ARGV_OK\nbusybox stat /hello.txt\nbusybox ls /\nbusybox ls /bin\nbusybox stat /bin\nbusybox cat /secret.txt\necho POSTCAT\nbusybox stat /bin\nbusybox ecxx\177\177ho BACKSPACE_OK\ncat\n\003echo CTRL_C_OK\ncd /bin\npwd\nexit\n' >&3

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

for expected in "hello.txt" "secret.txt" "musl-hello" "busybox" "cat" "stat" "File: /bin" "/bin"; do
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
while ! grep -F "BACKSPACE_OK" "$log" >/dev/null 2>&1; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "busybox backspace-edited command did not complete" >&2
		tail -n 160 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done

i=0
while ! grep -F "CTRL_C_OK" "$log" >/dev/null 2>&1; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "foreground Ctrl-C did not return to shell" >&2
		tail -n 180 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done

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

if ! awk '{ sub(/\r$/, "") } /\/bin \$ pwd/ { prompt = NR } /^\/bin$/ && prompt { found = 1 } END { exit found ? 0 : 1 }' "$log"; then
	echo "busybox chdir/pwd regression failed" >&2
	tail -n 160 "$log" >&2 || true
	exit 1
fi

echo "shell regression ok"
