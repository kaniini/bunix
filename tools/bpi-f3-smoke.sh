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
  --classify-log FILE
                      summarize which exploration hardware tasks FILE supports
  --summarize-log FILE
                      extract board-relevant values from a captured log
  --review-log FILE  run preboot check, smoke check, classifier, and summary
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

fail_sanity() {
	message=$1

	echo "invalid serial evidence: $message" >&2
	exit 1
}

has_marker() {
	log=$1
	marker=$2

	grep -aF "$marker" "$log" >/dev/null
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
	require_marker "$log" "boot: riscv64 memory-base="
	require_marker "$log" "boot: riscv64 memory-size="
	require_marker "$log" "boot: riscv64 kernel-start="
	require_marker "$log" "boot: riscv64 kernel-end="
	require_marker "$log" "boot: riscv64 initrd-start="
	require_marker "$log" "boot: riscv64 initrd-end="
	require_marker "$log" "boot: riscv64 initrd-size="
	require_marker "$log" "boot: riscv64 fdt-start="
	require_marker "$log" "boot: riscv64 fdt-end="
	require_marker "$log" "boot: riscv64 fdt-size="
	check_layout_sanity "$log"
	require_marker "$log" "fdt: riscv64 cpus"
	require_marker "$log" "fdt: riscv64 cpu-count="
	require_marker "$log" "smp: riscv64 discovered-harts="
	require_marker "$log" "smp: riscv64 started-harts="
	require_marker "$log" "smp: riscv64 boot-hart="
	require_marker "$log" "smp: riscv64 secondary-policy=parked"
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
	require_marker "$log" "fdt: riscv64 interrupt-controller-compatible="
	require_marker "$log" "fdt: riscv64 interrupt-controller-count="
	require_marker "$log" "fdt: riscv64 interrupt-routing-path="
	require_marker "$log" "fdt: riscv64 interrupt-routing-compatible="
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

classify_line() {
	label=$1
	status=$2
	detail=$3

	printf '%s\t%s\t%s\n' "$label" "$status" "$detail"
}

marker_value() {
	log=$1
	marker=$2

	grep -aF "$marker" "$log" | sed -n '1s/^[^=]*=//p' | tr -d '\r'
}

marker_number() {
	log=$1
	marker=$2
	value=$(marker_value "$log" "$marker")

	if [ -z "$value" ]; then
		fail_sanity "missing numeric marker $marker"
	fi
	printf '%s\n' "$((value))"
}

require_nonzero_marker() {
	log=$1
	marker=$2
	value=$(marker_number "$log" "$marker")

	if [ "$value" -eq 0 ]; then
		fail_sanity "$marker is zero"
	fi
}

require_ordered_range() {
	log=$1
	label=$2
	start_marker=$3
	end_marker=$4
	start=$(marker_number "$log" "$start_marker")
	end=$(marker_number "$log" "$end_marker")

	if [ "$start" -ge "$end" ]; then
		fail_sanity "$label range is not ordered"
	fi
}

require_exact_size() {
	log=$1
	label=$2
	start_marker=$3
	end_marker=$4
	size_marker=$5
	start=$(marker_number "$log" "$start_marker")
	end=$(marker_number "$log" "$end_marker")
	size=$(marker_number "$log" "$size_marker")

	if [ $((end - start)) -ne "$size" ]; then
		fail_sanity "$label size does not match start/end"
	fi
}

check_layout_sanity() {
	log=$1

	require_nonzero_marker "$log" "boot: riscv64 memory-size="
	require_ordered_range "$log" "kernel" \
		"boot: riscv64 kernel-start=" "boot: riscv64 kernel-end="
	require_ordered_range "$log" "initrd" \
		"boot: riscv64 initrd-start=" "boot: riscv64 initrd-end="
	require_ordered_range "$log" "fdt" \
		"boot: riscv64 fdt-start=" "boot: riscv64 fdt-end="
	require_exact_size "$log" "initrd" \
		"boot: riscv64 initrd-start=" "boot: riscv64 initrd-end=" \
		"boot: riscv64 initrd-size="
	require_exact_size "$log" "fdt" \
		"boot: riscv64 fdt-start=" "boot: riscv64 fdt-end=" \
		"boot: riscv64 fdt-size="
}

expect_check_log_failure() {
	source_log=$1
	bad_log=$2
	error_log=$3
	match=$4
	replacement=$5
	expected=$6

	sed "s|$match.*|$replacement|" "$source_log" > "$bad_log"
	if "$0" --artifact-dir "$artifact_dir" --check-log "$bad_log" \
	    >/dev/null 2>"$error_log"; then
		echo "expected BPI-F3 smoke log check to fail: $match" >&2
		exit 1
	fi
	grep -aF "$expected" "$error_log" >/dev/null
}

summary_line() {
	key=$1
	value=$2

	if [ -n "$value" ]; then
		printf '%s\t%s\n' "$key" "$value"
	else
		printf '%s\tmissing\n' "$key"
	fi
}

summarize_log() {
	log=$1

	require_file "$log"
	summary_line "memory-base" "$(marker_value "$log" "boot: riscv64 memory-base=")"
	summary_line "memory-size" "$(marker_value "$log" "boot: riscv64 memory-size=")"
	summary_line "kernel-start" "$(marker_value "$log" "boot: riscv64 kernel-start=")"
	summary_line "kernel-end" "$(marker_value "$log" "boot: riscv64 kernel-end=")"
	summary_line "initrd-start" "$(marker_value "$log" "boot: riscv64 initrd-start=")"
	summary_line "initrd-end" "$(marker_value "$log" "boot: riscv64 initrd-end=")"
	summary_line "initrd-size" "$(marker_value "$log" "boot: riscv64 initrd-size=")"
	summary_line "fdt-start" "$(marker_value "$log" "boot: riscv64 fdt-start=")"
	summary_line "fdt-end" "$(marker_value "$log" "boot: riscv64 fdt-end=")"
	summary_line "fdt-size" "$(marker_value "$log" "boot: riscv64 fdt-size=")"
	summary_line "cpu-count" "$(marker_value "$log" "fdt: riscv64 cpu-count=")"
	summary_line "smp-discovered-harts" \
		"$(marker_value "$log" "smp: riscv64 discovered-harts=")"
	summary_line "smp-started-harts" \
		"$(marker_value "$log" "smp: riscv64 started-harts=")"
	summary_line "smp-boot-hart" "$(marker_value "$log" "smp: riscv64 boot-hart=")"
	summary_line "smp-secondary-policy" \
		"$(marker_value "$log" "smp: riscv64 secondary-policy=")"
	summary_line "timebase-hz" "$(marker_value "$log" "fdt: riscv64 timebase-hz=")"
	summary_line "stdout-path" "$(marker_value "$log" "fdt: riscv64 stdout-path=")"
	summary_line "stdout-resolved" "$(marker_value "$log" "fdt: riscv64 stdout-resolved=")"
	summary_line "stdout-uart-base" "$(marker_value "$log" "fdt: riscv64 stdout-uart-base=")"
	summary_line "uart-count" "$(marker_value "$log" "fdt: riscv64 uart-count=")"
	summary_line "interrupt-controller-path" \
		"$(marker_value "$log" "fdt: riscv64 interrupt-controller-path=")"
	summary_line "interrupt-controller-compatible" \
		"$(marker_value "$log" "fdt: riscv64 interrupt-controller-compatible=")"
	summary_line "interrupt-controller-count" \
		"$(marker_value "$log" "fdt: riscv64 interrupt-controller-count=")"
	summary_line "interrupt-routing-path" \
		"$(marker_value "$log" "fdt: riscv64 interrupt-routing-path=")"
	summary_line "interrupt-routing-compatible" \
		"$(marker_value "$log" "fdt: riscv64 interrupt-routing-compatible=")"
}

review_log() {
	log=$1

	check_preboot_log "$log"
	check_log "$log"
	printf 'bpi-f3 classification:\n'
	classify_log "$log"
	printf 'bpi-f3 summary:\n'
	summarize_log "$log"
}

classify_log() {
	log=$1

	require_file "$log"
	if has_marker "$log" "fdt: riscv64 stdout-uart" &&
	    has_marker "$log" "fdt: riscv64 stdout-uart-base=" &&
	    has_marker "$log" "bunixos: riscv64 early bootstrap"; then
		classify_line "early-serial" "evidence" \
			"firmware stdout resolves to a UART and Bunix prints on serial"
	else
		classify_line "early-serial" "missing" \
			"need Bunix boot banner plus stdout UART diagnostics"
	fi

	if has_marker "$log" "sbi: system reset poweroff" &&
	    has_marker "$log" "machine: poweroff"; then
		classify_line "sbi-poweroff" "evidence" \
			"System Reset poweroff path reached firmware"
	else
		classify_line "sbi-poweroff" "missing" \
			"need machine poweroff plus SBI System Reset marker"
	fi

	if has_marker "$log" "timer: riscv64 tick" &&
	    has_marker "$log" "fdt: riscv64 timebase-hz="; then
		classify_line "timer" "evidence" \
			"timer tick observed with firmware timebase diagnostic"
	else
		classify_line "timer" "missing" \
			"need timer tick plus timebase diagnostic"
	fi

	if has_marker "$log" "fdt: riscv64 interrupt-controller" &&
	    has_marker "$log" "fdt: riscv64 interrupt-controller-path=" &&
	    has_marker "$log" "fdt: riscv64 interrupt-routing-compatible="; then
		classify_line "interrupt-routing" "evidence" \
			"firmware interrupt-controller node discovered with routing-compatible diagnostic"
	else
		classify_line "interrupt-routing" "missing" \
			"need interrupt-controller discovery and routing-compatible diagnostics"
	fi

	if has_marker "$log" "fdt: riscv64 cpu-count=" &&
	    has_marker "$log" "smp: riscv64 started-harts=" &&
	    has_marker "$log" "smp: riscv64 secondary-policy=parked"; then
		classify_line "smp-policy" "evidence" \
			"firmware hart count discovered and first-milestone secondary policy is parked"
	else
		classify_line "smp-policy" "missing" \
			"need hart count, started hart, and secondary policy diagnostics"
	fi

	if has_marker "$log" "bootstrap-riscv64: online" &&
	    has_marker "$log" "native: riscv64 server argc=1 argv0=/bin/abi-smoke.user" &&
	    has_marker "$log" "musl hello argc=1 argv0=/bin/musl-hello"; then
		classify_line "userspace-smoke" "evidence" \
			"bootstrap, native smoke, linux server, and static musl hello ran"
	else
		classify_line "userspace-smoke" "missing" \
			"need bootstrap plus native and musl hello markers"
	fi
}

self_test() {
	tmp=${TMPDIR:-/tmp}/bunix-bpi-f3-smoke.$$
	preboot=${TMPDIR:-/tmp}/bunix-bpi-f3-preboot.$$
	review=${TMPDIR:-/tmp}/bunix-bpi-f3-review.$$
	bad=${TMPDIR:-/tmp}/bunix-bpi-f3-bad.$$
	err=${TMPDIR:-/tmp}/bunix-bpi-f3-err.$$

	trap 'rm -f "$tmp" "$preboot" "$review" "$bad" "$err"' EXIT HUP INT TERM
	cat >"$tmp" <<EOF
bunixos: riscv64 early bootstrap
pmm: riscv64 ranges
boot: riscv64 memory-base=0x80000000
boot: riscv64 memory-size=0x8000000
boot: riscv64 kernel-start=0x80200000
boot: riscv64 kernel-end=0x80300000
boot: riscv64 initrd-start=0x84000000
boot: riscv64 initrd-end=0x84010000
boot: riscv64 initrd-size=0x10000
boot: riscv64 fdt-start=0x87e00000
boot: riscv64 fdt-end=0x87e01000
boot: riscv64 fdt-size=0x1000
fdt: riscv64 cpus
fdt: riscv64 cpu-count=1
smp: riscv64 discovered-harts=1
smp: riscv64 started-harts=1
smp: riscv64 boot-hart=0
smp: riscv64 secondary-policy=parked
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
fdt: riscv64 interrupt-controller-compatible=sifive,plic-1.0.0
fdt: riscv64 interrupt-controller-count=1
fdt: riscv64 interrupt-routing-path=/soc/interrupt-controller@c000000
fdt: riscv64 interrupt-routing-compatible=sifive,plic-1.0.0
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
	cat "$preboot" "$tmp" > "$review"
	review_log "$review" | grep -aF "bpi-f3 summary:" >/dev/null
	expect_check_log_failure "$tmp" "$bad" "$err" \
		"boot: riscv64 memory-size=" \
		"boot: riscv64 memory-size=0x0" \
		"invalid serial evidence: boot: riscv64 memory-size= is zero"
	expect_check_log_failure "$tmp" "$bad" "$err" \
		"boot: riscv64 kernel-end=" \
		"boot: riscv64 kernel-end=0x80100000" \
		"invalid serial evidence: kernel range is not ordered"
	expect_check_log_failure "$tmp" "$bad" "$err" \
		"boot: riscv64 initrd-size=" \
		"boot: riscv64 initrd-size=0x20000" \
		"invalid serial evidence: initrd size does not match start/end"
	classify_log "$tmp" >/dev/null
	summarize_log "$tmp" | grep -aF "initrd-size	0x10000" >/dev/null
	summarize_log "$tmp" | grep -aF "smp-secondary-policy	parked" >/dev/null
	summarize_log "$tmp" | grep -aF "stdout-resolved	/soc/serial@10000000" >/dev/null
	summarize_log "$tmp" | grep -aF "interrupt-controller-path	/soc/interrupt-controller@c000000" >/dev/null
	summarize_log "$tmp" | grep -aF "interrupt-routing-compatible	sifive,plic-1.0.0" >/dev/null
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
	--classify-log)
		if [ $# -lt 2 ]; then
			usage >&2
			exit 2
		fi
		classify_log "$2"
		exit 0
		;;
	--summarize-log)
		if [ $# -lt 2 ]; then
			usage >&2
			exit 2
		fi
		summarize_log "$2"
		exit 0
		;;
	--review-log)
		if [ $# -lt 2 ]; then
			usage >&2
			exit 2
		fi
		review_log "$2"
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
