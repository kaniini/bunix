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
while ! grep -F "/ # " "$log" >/dev/null 2>&1; do
	i=$((i + 1))
	if ! kill -0 "$qemu_pid" 2>/dev/null; then
		echo "qemu exited before shell prompt" >&2
		cat "$qemu_log" >&2 || true
		tail -n 80 "$log" >&2 || true
		exit 1
	fi
	if [ "$i" -gt 80 ]; then
		echo "shell prompt did not appear" >&2
		cat "$qemu_log" >&2 || true
		tail -n 80 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done

sleep 3
printf 'dmesg\necho AFTER\nexit\n' > "$pipe.in"

i=0
while ! grep -F "AFTER" "$log" >/dev/null 2>&1; do
	i=$((i + 1))
	if [ "$i" -gt 45 ]; then
		echo "shell regression command did not complete" >&2
		tail -n 120 "$log" >&2 || true
		exit 1
	fi
	sleep 1
done

if ! grep -F "kernel: console logs routed to dmesg" "$log" >/dev/null; then
	echo "dmesg did not include console-routing marker" >&2
	tail -n 160 "$log" >&2 || true
	exit 1
fi
if ! grep -F "ping: heartbeat" "$log" >/dev/null; then
	echo "dmesg did not include heartbeat logs" >&2
	tail -n 160 "$log" >&2 || true
	exit 1
fi

if ! awk '
	/kernel: console logs routed to dmesg/ && !dmesg_seen { dmesg_seen = NR }
	/\/ # echo AFTER/ && !after_prompt { after_prompt = NR }
	/AFTER/ && !/echo AFTER/ && !after_seen { after_seen = NR }
	END {
		if (!dmesg_seen || !after_prompt || !after_seen) {
			exit 1
		}
		if (after_prompt < dmesg_seen || after_seen < after_prompt) {
			exit 1
		}
	}
' "$log"; then
	echo "dmesg prompt ordering assertion failed" >&2
	tail -n 200 "$log" >&2 || true
	exit 1
fi

echo "shell regression ok"
