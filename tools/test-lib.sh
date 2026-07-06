#!/bin/sh

save_failure_artifacts() {
	label=$1
	serial_log=${2:-}
	qemu_log=${3:-}
	tail_lines=${4:-160}
	failure_dir=${BUNIX_FAILURE_DIR:-${FAILURE_DIR:-build/failures}}
	id=$(date -u +%Y%m%dT%H%M%SZ)-$$
	out=$failure_dir/$id

	mkdir -p "$out"
	printf '%s\n' "$label" > "$out/reason.txt"
	printf '%s\n' "${KERNEL_CMDLINE:-}" > "$out/kernel-cmdline.txt"
	printf '%s\n' "${BUNIX_CMD:-${CMD:-}}" > "$out/command.txt"
	printf '%s\n' "${BUNIX_TEST_RUNTIME_DIR:-}" > "$out/test-runtime-dir.txt"
	printf '%s\n' "${BUNIX_SHELL_PARTS:-${BUNIX_SHELL_PART:-}}" > "$out/shell-selected-parts.txt"
	printf '%s\n' "${BUNIX_CURRENT_SHARD:-}" > "$out/shell-current-shard.txt"
	printf '%s\n' "${BUNIX_CURRENT_MARKER_FILE:-}" > "$out/shell-current-marker-file.txt"
	printf '%s\n' "${BUNIX_CURRENT_SHARD_FILE:-}" > "$out/shell-current-shard-file.txt"
	if [ -n "${BUNIX_CMD_FILE:-}" ] && [ -r "${BUNIX_CMD_FILE:-}" ]; then
		cp "$BUNIX_CMD_FILE" "$out/input-script.sh"
	fi
	if [ -n "${BUNIX_CURRENT_SHARD_FILE:-}" ] && [ -r "${BUNIX_CURRENT_SHARD_FILE:-}" ]; then
		cp "$BUNIX_CURRENT_SHARD_FILE" "$out/shell-shard.sh"
	fi
	if [ -n "${BUNIX_CURRENT_MARKER_FILE:-}" ] && [ -r "${BUNIX_CURRENT_MARKER_FILE:-}" ]; then
		cp "$BUNIX_CURRENT_MARKER_FILE" "$out/shell-marker-file.txt"
	fi
	if [ -n "${BUNIX_TEST_HARNESS:-}" ] && [ -r "${BUNIX_TEST_HARNESS:-}" ]; then
		cp "$BUNIX_TEST_HARNESS" "$out/test-harness.sh"
	fi
	if [ -n "$serial_log" ]; then
		cp "$serial_log" "$out/serial.log" 2>/dev/null || true
		tail -n "$tail_lines" "$serial_log" > "$out/serial-tail.log" 2>/dev/null || true
		grep -a 'linux-strace' "$serial_log" | tail -n "${BUNIX_STRACE_LINES:-200}" > "$out/linux-strace.log" 2>/dev/null || true
	fi
	if [ -n "$qemu_log" ]; then
		cp "$qemu_log" "$out/qemu.log" 2>/dev/null || true
	fi
	git rev-parse HEAD > "$out/git-commit.txt" 2>/dev/null || true
	printf '%s\n' "$out"
}

fail_with_log() {
	label=$1
	log=$2
	tail_lines=${3:-160}
	out=

	if [ -n "${BUNIX_CURRENT_SHARD:-}" ]; then
		label="shard=$BUNIX_CURRENT_SHARD $label"
	fi
	if [ -n "${BUNIX_CURRENT_MARKER_FILE:-}" ]; then
		label="marker_file=$BUNIX_CURRENT_MARKER_FILE $label"
	fi

	if [ -n "${BUNIX_COLLECT_FAILURES:-}" ]; then
		out=$(save_failure_artifacts "$label" "$log" "${BUNIX_QEMU_LOG:-}" "$tail_lines")
	fi

	echo "$label" >&2
	if [ -n "$out" ]; then
		echo "failure artifacts: $out" >&2
	fi
	tail -n "$tail_lines" "$log" >&2 || true
	exit 1
}

wait_for_fixed() {
	log=$1
	expected=$2
	label=$3
	limit=${4:-45}
	tail_lines=${5:-160}
	i=0

	while ! grep -aF "$expected" "$log" >/dev/null 2>&1; do
		i=$((i + 1))
		if [ "$i" -gt "$limit" ]; then
			fail_with_log "$label: $expected" "$log" "$tail_lines"
		fi
		sleep 1
	done
}

wait_for_regex() {
	log=$1
	expected=$2
	label=$3
	limit=${4:-45}
	tail_lines=${5:-160}
	i=0

	while ! grep -aE "$expected" "$log" >/dev/null 2>&1; do
		i=$((i + 1))
		if [ "$i" -gt "$limit" ]; then
			fail_with_log "$label: $expected" "$log" "$tail_lines"
		fi
		sleep 1
	done
}

wait_for_exact_line() {
	log=$1
	expected=$2
	label=$3
	limit=${4:-45}
	tail_lines=${5:-160}
	i=0

	while ! awk -v expected="$expected" '{ sub(/\r$/, "") } $0 == expected { found = 1 } END { exit found ? 0 : 1 }' "$log"; do
		i=$((i + 1))
		if [ "$i" -gt "$limit" ]; then
			fail_with_log "$label: $expected" "$log" "$tail_lines"
		fi
		sleep 1
	done
}

wait_for_marker() {
	log=$1
	expected=$2
	label=$3
	limit=${4:-45}
	tail_lines=${5:-160}

	wait_for_exact_line "$log" "$expected" "$label" "$limit" "$tail_lines"
}

wait_for_awk() {
	log=$1
	program=$2
	label=$3
	limit=${4:-45}
	tail_lines=${5:-160}
	i=0

	while ! awk "$program" "$log"; do
		i=$((i + 1))
		if [ "$i" -gt "$limit" ]; then
			fail_with_log "$label" "$log" "$tail_lines"
		fi
		sleep 1
	done
}

wait_for_fixed_count() {
	log=$1
	expected=$2
	min_count=$3
	label=$4
	limit=${5:-45}
	tail_lines=${6:-160}
	i=0

	while [ "$(grep -aF -c "$expected" "$log" 2>/dev/null || true)" -lt "$min_count" ]; do
		i=$((i + 1))
		if [ "$i" -gt "$limit" ]; then
			fail_with_log "$label: $expected" "$log" "$tail_lines"
		fi
		sleep 1
	done
}

require_no_fixed() {
	log=$1
	unexpected=$2
	label=$3
	tail_lines=${4:-160}

	if grep -aF "$unexpected" "$log" >/dev/null 2>&1; then
		fail_with_log "$label: $unexpected" "$log" "$tail_lines"
	fi
}

check_markers() {
	mode=$1
	shift
	log=$1
	label=$2
	limit=$3
	tail_lines=$4

	while IFS= read -r expected; do
		case "$expected" in
		''|\#*) continue ;;
		esac
		case "$mode" in
		fixed)
			wait_for_fixed "$log" "$expected" "$label" "$limit" "$tail_lines"
			;;
		exact)
			wait_for_marker "$log" "$expected" "$label" "$limit" "$tail_lines"
			;;
		*)
			fail_with_log "unknown marker mode: $mode" "$log" "$tail_lines"
			;;
		esac
	done
}

check_fixed_markers() {
	check_markers fixed "$@"
}

check_exact_markers() {
	check_markers exact "$@"
}

check_fixed_markers_file() {
	log=$1
	markers=$2
	label=$3
	limit=${4:-1}
	tail_lines=${5:-160}
	old_marker_file=${BUNIX_CURRENT_MARKER_FILE:-}

	BUNIX_CURRENT_MARKER_FILE=$markers
	export BUNIX_CURRENT_MARKER_FILE
	check_fixed_markers "$log" "$label" "$limit" "$tail_lines" < "$markers"
	BUNIX_CURRENT_MARKER_FILE=$old_marker_file
	export BUNIX_CURRENT_MARKER_FILE
}

check_exact_markers_file() {
	log=$1
	markers=$2
	label=$3
	limit=${4:-1}
	tail_lines=${5:-160}
	old_marker_file=${BUNIX_CURRENT_MARKER_FILE:-}

	BUNIX_CURRENT_MARKER_FILE=$markers
	export BUNIX_CURRENT_MARKER_FILE
	check_exact_markers "$log" "$label" "$limit" "$tail_lines" < "$markers"
	BUNIX_CURRENT_MARKER_FILE=$old_marker_file
	export BUNIX_CURRENT_MARKER_FILE
}

wait_for_each_fixed() {
	log=$1
	label=$2
	limit=${3:-45}
	tail_lines=${4:-160}
	shift 4

	for expected in "$@"; do
		wait_for_fixed "$log" "$expected" "$label" "$limit" "$tail_lines"
	done
}

wait_for_each_regex() {
	log=$1
	label=$2
	limit=${3:-45}
	tail_lines=${4:-160}
	shift 4

	for expected in "$@"; do
		wait_for_regex "$log" "$expected" "$label" "$limit" "$tail_lines"
	done
}

wait_for_each_fixed_count() {
	log=$1
	min_count=$2
	label=$3
	limit=${4:-45}
	tail_lines=${5:-160}
	shift 5

	for expected in "$@"; do
		wait_for_fixed_count "$log" "$expected" "$min_count" "$label" "$limit" "$tail_lines"
	done
}
