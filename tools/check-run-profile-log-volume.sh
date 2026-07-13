#!/bin/sh
set -eu

profile=${1:-}
if [ -z "$profile" ]; then
	printf 'usage: %s build/run-profile/<run-id>\n' "$0" >&2
	exit 2
fi

summary=$profile/summary.txt
prefixes=$profile/serial-prefixes.tsv
phases=$profile/serial-phases.tsv

if [ ! -r "$summary" ] || [ ! -r "$prefixes" ] || [ ! -r "$phases" ]; then
	printf 'missing run-profile artifacts under %s\n' "$profile" >&2
	exit 1
fi

summary_value() {
	key=$1
	awk -F= -v key="$key" '$1 == key { print $2 }' "$summary"
}

prefix_count() {
	prefix=$1
	awk -v prefix="$prefix" '$2 == prefix { print $1; found = 1 } END { if (!found) print 0 }' "$prefixes"
}

phase_lines() {
	phase=$1
	awk -v phase="$phase" '$3 == phase { print $1; found = 1 } END { if (!found) print 0 }' "$phases"
}

check_le() {
	name=$1
	value=$2
	limit=$3

	if [ "$value" -gt "$limit" ]; then
		printf 'log-volume: %s=%s exceeds %s\n' "$name" "$value" "$limit" >&2
		exit 1
	fi
	printf 'log-volume: %s=%s limit=%s\n' "$name" "$value" "$limit"
}

serial_lines=$(summary_value serial_lines)
serial_bytes=$(summary_value serial_bytes)
kernel_count=$(prefix_count kernel)
names_count=$(prefix_count names)
vm_count=$(prefix_count vm-server)
user_syscall_count=$(prefix_count user-syscall)
kernel_server_lines=$(phase_lines kernel-servers)

check_le serial_lines "$serial_lines" 320
check_le serial_bytes "$serial_bytes" 14000
check_le kernel_prefix "$kernel_count" 60
check_le names_prefix "$names_count" 5
check_le vm_server_prefix "$vm_count" 5
check_le user_syscall_prefix "$user_syscall_count" 0
check_le kernel_server_phase_lines "$kernel_server_lines" 275
