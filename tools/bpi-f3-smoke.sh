#!/bin/sh
set -eu

artifact_dir=${BUNIX_BPI_F3_ARTIFACT_DIR:-build/riscv64/bpi-f3}

usage() {
	cat <<EOF
usage: $0 [--artifact-dir DIR] COMMAND [ARG]

commands:
  --prepare DIR       copy Bunix BPI-F3 boot artifacts into DIR
  --print-uboot      print the manual U-Boot command recipe
  --check-preboot-log FILE
                      verify captured U-Boot preboot diagnostics
  --check-log FILE   verify a captured serial log has first-smoke markers
  --self-test        run a host-only marker-check self test
EOF
}

artifact_path() {
	printf '%s/%s\n' "$artifact_dir" "$1"
}

require_file() {
	if [ ! -s "$1" ]; then
		echo "missing required file: $1" >&2
		exit 1
	fi
}

require_marker() {
	log=$1
	marker=$2

	if ! grep -aF "$marker" "$log" >/dev/null; then
		echo "missing serial marker: $marker" >&2
		exit 1
	fi
}

check_artifacts() {
	require_file "$(artifact_path bunixos-riscv64.elf)"
	require_file "$(artifact_path bunix-riscv64.bootpkg)"
	require_file "$(artifact_path boot-bunix-bpi-f3.cmd)"
	require_file "$(artifact_path manifest.txt)"
}

prepare_boot_partition() {
	dst=$1

	check_artifacts
	mkdir -p "$dst"
	cp "$(artifact_path bunixos-riscv64.elf)" "$dst/"
	cp "$(artifact_path bunix-riscv64.bootpkg)" "$dst/"
	cp "$(artifact_path boot-bunix-bpi-f3.cmd)" "$dst/"
	cp "$(artifact_path manifest.txt)" "$dst/"
	printf 'prepared %s\n' "$dst"
}

print_uboot_recipe() {
	check_artifacts
	cat "$(artifact_path boot-bunix-bpi-f3.cmd)"
}

check_log() {
	log=$1

	require_file "$log"
	require_marker "$log" "bunixos: riscv64 early bootstrap"
	require_marker "$log" "pmm: riscv64 ranges"
	require_marker "$log" "fdt: riscv64 cpus"
	require_marker "$log" "fdt: riscv64 cpu-count="
	require_marker "$log" "fdt: riscv64 timer"
	require_marker "$log" "fdt: riscv64 timebase-hz="
	require_marker "$log" "fdt: riscv64 stdout"
	require_marker "$log" "fdt: riscv64 stdout-path="
	require_marker "$log" "fdt: riscv64 stdout-uart"
	require_marker "$log" "fdt: riscv64 stdout-resolved="
	require_marker "$log" "fdt: riscv64 stdout-uart-base="
	require_marker "$log" "fdt: riscv64 uart"
	require_marker "$log" "fdt: riscv64 uart-count="
	require_marker "$log" "fdt: riscv64 interrupt-controller"
	require_marker "$log" "fdt: riscv64 interrupt-controller-path="
	require_marker "$log" "fdt: riscv64 interrupt-controller-count="
	require_marker "$log" "timer: riscv64 tick"
	require_marker "$log" "thread: riscv64 switch"
	require_marker "$log" "bootpkg: riscv64 initrd"
	require_marker "$log" "bootstrap-riscv64: online"
	require_marker "$log" "native: riscv64 server argc=1 argv0=/bin/abi-smoke.user"
	require_marker "$log" "musl hello argc=1 argv0=/bin/musl-hello"
	require_marker "$log" "machine: poweroff"
	require_marker "$log" "sbi: system reset poweroff"
	printf 'bpi-f3 smoke log ok: %s\n' "$log"
}

check_preboot_log() {
	log=$1

	require_file "$log"
	require_marker "$log" "bdinfo"
	require_marker "$log" "fdt print /chosen"
	require_marker "$log" "fdt print /aliases"
	require_marker "$log" "fdt print /cpus"
	require_marker "$log" "stdout-path"
	require_marker "$log" "serial"
	require_marker "$log" "timebase-frequency"
	printf 'bpi-f3 preboot log ok: %s\n' "$log"
}

self_test() {
	tmp=${TMPDIR:-/tmp}/bunix-bpi-f3-smoke.$$
	preboot=${TMPDIR:-/tmp}/bunix-bpi-f3-preboot.$$

	trap 'rm -f "$tmp" "$preboot"' EXIT HUP INT TERM
	cat >"$tmp" <<EOF
bunixos: riscv64 early bootstrap
pmm: riscv64 ranges
fdt: riscv64 cpus
fdt: riscv64 cpu-count=1
fdt: riscv64 timer
fdt: riscv64 timebase-hz=10000000
fdt: riscv64 stdout
fdt: riscv64 stdout-path=serial0:115200n8
fdt: riscv64 stdout-uart
fdt: riscv64 stdout-resolved=/soc/serial@10000000
fdt: riscv64 stdout-uart-base=0x10000000
fdt: riscv64 uart
fdt: riscv64 uart-count=1
fdt: riscv64 interrupt-controller
fdt: riscv64 interrupt-controller-path=/soc/interrupt-controller@c000000
fdt: riscv64 interrupt-controller-count=1
timer: riscv64 tick
thread: riscv64 switch
bootpkg: riscv64 initrd
bootstrap-riscv64: online
native: riscv64 server argc=1 argv0=/bin/abi-smoke.user
musl hello argc=1 argv0=/bin/musl-hello
machine: poweroff
sbi: system reset poweroff
EOF
	check_log "$tmp" >/dev/null
	cat >"$preboot" <<EOF
bdinfo
fdt print /chosen
	stdout-path = "serial0:115200n8";
fdt print /aliases
	serial0 = "/soc/serial@10000000";
fdt print /cpus
	timebase-frequency = <0x00989680>;
EOF
	check_preboot_log "$preboot" >/dev/null
	printf 'bpi-f3 smoke self-test ok\n'
}

while [ $# -gt 0 ]; do
	case "$1" in
	--artifact-dir)
		if [ $# -lt 2 ]; then
			usage >&2
			exit 2
		fi
		artifact_dir=$2
		shift 2
		;;
	--prepare)
		if [ $# -lt 2 ]; then
			usage >&2
			exit 2
		fi
		prepare_boot_partition "$2"
		exit 0
		;;
	--print-uboot)
		print_uboot_recipe
		exit 0
		;;
	--check-log)
		if [ $# -lt 2 ]; then
			usage >&2
			exit 2
		fi
		check_log "$2"
		exit 0
		;;
	--check-preboot-log)
		if [ $# -lt 2 ]; then
			usage >&2
			exit 2
		fi
		check_preboot_log "$2"
		exit 0
		;;
	--self-test)
		self_test
		exit 0
		;;
	-h|--help)
		usage
		exit 0
		;;
	*)
		usage >&2
		exit 2
		;;
	esac
done

usage >&2
exit 2
