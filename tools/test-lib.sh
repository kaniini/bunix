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

check_fixed_markers_file() {
	log=$1
	markers=$2
	label=$3
	limit=${4:-1}
	tail_lines=${5:-160}

	check_fixed_markers "$log" "$label" "$limit" "$tail_lines" < "$markers"
}
