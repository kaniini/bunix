#!/bin/sh
set -eu

qemu=${RISCV64_QEMU:-qemu-system-riscv64}
kernel=${RISCV64_KERNEL:?RISCV64_KERNEL is required}
initrd=${RISCV64_INITRD:?RISCV64_INITRD is required}
serial_log=${RISCV64_SERIAL_LOG:-build/riscv64-serial.log}
timeout_value=${RISCV64_QEMU_TIMEOUT:-30s}
memory=${RISCV64_QEMU_MEMORY:-128M}
marker_file=${RISCV64_MARKERS:-}
extra_args=${RISCV64_QEMU_EXTRA_ARGS:-}

command -v "$qemu" >/dev/null 2>&1 || {
	echo "missing $qemu" >&2
	exit 1
}

mkdir -p "$(dirname "$serial_log")"
timeout "$timeout_value" "$qemu" -machine virt -m "$memory" -nographic \
	-no-reboot -kernel "$kernel" -initrd "$initrd" $extra_args >"$serial_log"

if [ -n "$marker_file" ]; then
	sh tools/check-markers.sh "$serial_log" "$marker_file"
fi
