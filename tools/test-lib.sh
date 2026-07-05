#!/bin/sh

fail_with_log() {
	label=$1
	log=$2
	tail_lines=${3:-160}

	echo "$label" >&2
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

check_fixed_markers() {
	log=$1
	label=$2
	limit=$3
	tail_lines=$4

	while IFS= read -r expected; do
		case "$expected" in
		''|\#*) continue ;;
		esac
		wait_for_fixed "$log" "$expected" "$label" "$limit" "$tail_lines"
	done
}

check_exact_markers() {
	log=$1
	label=$2
	limit=$3
	tail_lines=$4

	while IFS= read -r expected; do
		case "$expected" in
		''|\#*) continue ;;
		esac
		wait_for_marker "$log" "$expected" "$label" "$limit" "$tail_lines"
	done
}

check_fixed_markers_file() {
	log=$1
	markers=$2
	label=$3
	limit=${4:-1}
	tail_lines=${5:-160}

	check_fixed_markers "$log" "$label" "$limit" "$tail_lines" < "$markers"
}

check_exact_markers_file() {
	log=$1
	markers=$2
	label=$3
	limit=${4:-1}
	tail_lines=${5:-160}

	check_exact_markers "$log" "$label" "$limit" "$tail_lines" < "$markers"
}
