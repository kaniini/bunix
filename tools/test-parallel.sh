#!/bin/sh
set -eu

manifest=${BUNIX_SHELL_SHARDS:-tools/shell-shards.tsv}
ovmf=${OVMF_CODE:-/usr/share/OVMF/OVMF_CODE.fd}
test_set=${BUNIX_TEST_SET:-${BUNIX_SHELL_PART:-all}}
run_root=${BUNIX_TEST_RUN_ROOT:-build/test-runs}
run_id=${BUNIX_TEST_RUN_ID:-$(date -u +%Y%m%dT%H%M%SZ)-$$}
worker_cmd=${BUNIX_TEST_WORKER:-}
run_dir=$run_root/$run_id

default_jobs() {
	cpus=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)
	case "$cpus" in
	''|*[!0-9]*) cpus=1 ;;
	esac

	jobs=$((cpus / 2))
	if [ "$jobs" -lt 1 ]; then
		jobs=1
	fi
	if [ "$jobs" -gt 8 ]; then
		jobs=8
	fi
	echo "$jobs"
}

jobs=${BUNIX_TEST_JOBS:-$(default_jobs)}
retries=${BUNIX_TEST_RETRIES:-0}
stop_on_fail=${BUNIX_TEST_STOP_ON_FAIL:-0}
skip_host_checks=${BUNIX_TEST_SKIP_HOST_CHECKS:-0}

case "$jobs" in
''|*[!0-9]*)
	echo "BUNIX_TEST_JOBS must be a positive integer" >&2
	exit 2
	;;
0)
	echo "BUNIX_TEST_JOBS must be greater than zero" >&2
	exit 2
	;;
esac

case "$retries" in
''|*[!0-9]*)
	echo "BUNIX_TEST_RETRIES must be a non-negative integer" >&2
	exit 2
	;;
esac

case "$stop_on_fail" in
1|yes|true|on) stop_on_fail=1 ;;
0|no|false|off|'') stop_on_fail=0 ;;
*)
	echo "BUNIX_TEST_STOP_ON_FAIL must be 0/1, yes/no, true/false, or on/off" >&2
	exit 2
	;;
esac

case "$skip_host_checks" in
1|yes|true|on) skip_host_checks=1 ;;
0|no|false|off|'') skip_host_checks=0 ;;
*)
	echo "BUNIX_TEST_SKIP_HOST_CHECKS must be 0/1, yes/no, true/false, or on/off" >&2
	exit 2
	;;
esac

if [ ! -r "$manifest" ]; then
	echo "shell shard manifest is not readable: $manifest" >&2
	exit 2
fi

selected_shards() {
	awk -F '\t' -v set="$test_set" '
	function valid_uint(value) {
		return value ~ /^[0-9]+$/
	}
	function valid_memory(value) {
		return value ~ /^[0-9]+[KMG]?$/
	}
	function selected(name, phase,    list, count, i, part) {
		if (set == "" || set == "all")
			return 1
		if (set == "shell")
			return phase != "openrc"
		count = split(set, list, ",")
		for (i = 1; i <= count; i++) {
			part = list[i]
			if (part == name)
				return 1
			if ((part == "smoke" || part == "sysrace") && name == "smoke")
				return 1
			if ((part == "vfs" || part == "procfs" || part == "devfs") &&
			    name == "rootfs-vfs-proc-dev")
				return 1
			if (part == "tmpfs" &&
			    (name == "tmpfs-basic-linux-tests" ||
			     name == "tmpfs-extended" ||
			     name == "root-tmpfs-chown"))
				return 1
			if ((part == "path" || part == "statfs") && name == "path-limits-statfs")
				return 1
			if ((part == "large-io" || part == "mount") && name == "large-io-mount")
				return 1
			if (part == phase)
				return 1
		}
		return 0
	}
	/^#/ || NF == 0 { next }
	NF < 8 {
		printf "invalid shard row at %s:%d\n", FILENAME, FNR > "/dev/stderr"
		exit 1
	}
	!valid_uint($3) {
		printf "invalid smp value for shard %s: %s\n", $1, $3 > "/dev/stderr"
		exit 1
	}
	!valid_memory($4) {
		printf "invalid memory value for shard %s: %s\n", $1, $4 > "/dev/stderr"
		exit 1
	}
	!valid_uint($5) {
		printf "invalid timeout value for shard %s: %s\n", $1, $5 > "/dev/stderr"
		exit 1
	}
	!valid_uint($6) {
		printf "invalid cost value for shard %s: %s\n", $1, $6 > "/dev/stderr"
		exit 1
	}
	$7 != "yes" && $7 != "no" {
		printf "invalid clean_boot value for shard %s: %s\n", $1, $7 > "/dev/stderr"
		exit 1
	}
	selected($1, $2) {
		harness = $9 == "" ? "shell" : $9
		rootfs = $10 == "" ? "synthetic" : $10
		boot_phase = $11 == "" ? "full" : $11
		marker_file = $12
		print $1 "\t" $3 "\t" $4 "\t" $5 "\t" $6 "\t" $7 "\t" harness "\t" rootfs "\t" boot_phase "\t" marker_file
	}
	' "$manifest"
}

ordered_shards() {
	selected_shards | sort -t "$(printf '\t')" -k5,5nr -k4,4nr
}

memory_to_mib() {
	awk '
	function mib(value,    number, suffix) {
		number = value
		suffix = ""
		if (value ~ /[KMG]$/) {
			suffix = substr(value, length(value), 1)
			number = substr(value, 1, length(value) - 1)
		}
		if (suffix == "G")
			return number * 1024
		if (suffix == "K")
			return int((number + 1023) / 1024)
		return number
	}
	{ print mib($1) }
	'
}

max_selected_memory_mib() {
	if [ -n "${BUNIX_VM_MEMORY_OVERRIDE:-}" ]; then
		printf '%s\n' "$BUNIX_VM_MEMORY_OVERRIDE" | memory_to_mib
		return
	fi

	ordered_shards | awk -F '\t' '
	function mib(value,    number, suffix) {
		number = value
		suffix = ""
		if (value ~ /[KMG]$/) {
			suffix = substr(value, length(value), 1)
			number = substr(value, 1, length(value) - 1)
		}
		if (suffix == "G")
			return number * 1024
		if (suffix == "K")
			return int((number + 1023) / 1024)
		return number
	}
	{
		memory = mib($3)
		if (memory > max)
			max = memory
	}
	END {
		if (max < 1)
			max = 128
		print max
	}
	'
}

host_sanity_checks() {
	if [ "$skip_host_checks" -eq 1 ]; then
		echo "test-parallel host-checks=skipped"
		return
	fi

	if [ ! -e /dev/kvm ]; then
		echo "test-parallel host-check=fail reason=/dev/kvm-missing" >&2
		exit 2
	fi
	if [ ! -r /dev/kvm ] || [ ! -w /dev/kvm ]; then
		echo "test-parallel host-check=fail reason=/dev/kvm-not-readable-writable" >&2
		exit 2
	fi
	if [ ! -r "$ovmf" ]; then
		echo "test-parallel host-check=fail reason=ovmf-not-readable path=$ovmf" >&2
		exit 2
	fi

	fd_limit=$(ulimit -n 2>/dev/null || echo 0)
	case "$fd_limit" in
	''|*[!0-9]*) fd_limit=0 ;;
	esac
	required_fds=$((jobs * 8 + 32))
	if [ "$fd_limit" -ne 0 ] && [ "$fd_limit" -lt "$required_fds" ]; then
		echo "test-parallel host-check=fail reason=fd-limit-too-low limit=$fd_limit required=$required_fds" >&2
		exit 2
	fi

	available_kb=$(df -Pk "$run_dir" 2>/dev/null | awk 'NR == 2 { print $4 }')
	case "$available_kb" in
	''|*[!0-9]*) available_kb=0 ;;
	esac
	required_kb=$((jobs * 65536))
	if [ "$available_kb" -ne 0 ] && [ "$available_kb" -lt "$required_kb" ]; then
		echo "test-parallel host-check=fail reason=disk-space-too-low available_kb=$available_kb required_kb=$required_kb" >&2
		exit 2
	fi

	if [ -r /proc/meminfo ]; then
		available_mib=$(awk '/MemAvailable:/ { print int($2 / 1024) }' /proc/meminfo)
		case "$available_mib" in
		''|*[!0-9]*) available_mib=0 ;;
		esac
		max_memory_mib=$(max_selected_memory_mib)
		required_mib=$((jobs * max_memory_mib))
		if [ "$available_mib" -ne 0 ] && [ "$available_mib" -lt "$required_mib" ]; then
			echo "test-parallel host-check=fail reason=memory-too-low available_mib=$available_mib required_mib=$required_mib" >&2
			exit 2
		fi
	fi

	echo "test-parallel host-checks=ok"
}

mkdir -p "$run_dir"
host_sanity_checks
echo "test-parallel status=plan run_id=$run_id jobs=$jobs retries=$retries stop_on_fail=$stop_on_fail set=$test_set artifact=$run_dir"

worker_parts() {
	case "$1" in
	smoke-up)
		echo "smoke"
		;;
	root-tmpfs-chown)
		echo "tmpfs-basic-linux-tests,root-tmpfs-chown"
		;;
	*)
		echo "$1"
		;;
	esac
}

effective_timeout() {
	if [ -n "${BUNIX_QEMU_TIMEOUT_OVERRIDE:-}" ]; then
		case "$BUNIX_QEMU_TIMEOUT_OVERRIDE" in
		*[!0-9]*) echo "$BUNIX_QEMU_TIMEOUT_OVERRIDE" ;;
		*) echo "${BUNIX_QEMU_TIMEOUT_OVERRIDE}s" ;;
		esac
		return
	fi
	echo "${1}s"
}

run_worker() {
	name=$1
	smp=$2
	memory=$3
	timeout_seconds=$4
	cost=$5
	clean_boot=$6
	harness=$7
	rootfs=$8
	boot_phase=$9
	marker_file=${10:-}
	worker_smp=${BUNIX_VM_SMP_OVERRIDE:-$smp}
	worker_memory=${BUNIX_VM_MEMORY_OVERRIDE:-$memory}
	worker_timeout=$(effective_timeout "$timeout_seconds")
	parts=$(worker_parts "$name")
	worker_dir=$run_dir/$name
	start=$(date +%s)
	max_attempts=$((retries + 1))
	attempt=1
	status=fail
	rc=1
	reason=

	mkdir -p "$worker_dir"
	echo "test-parallel name=$name status=start smp=$worker_smp memory=$worker_memory timeout=$worker_timeout cost=$cost clean_boot=$clean_boot harness=$harness rootfs=$rootfs artifact=$worker_dir"

	while [ "$attempt" -le "$max_attempts" ]; do
		attempt_dir=$worker_dir/attempt-$attempt
		mkdir -p "$attempt_dir"
		echo "test-parallel name=$name status=attempt attempt=$attempt max_attempts=$max_attempts artifact=$attempt_dir"
		attempt_ok=0

		if [ "$harness" = boot ]; then
			worker_esp=$ESP_DIR
			case "$rootfs" in
			alpine|alpine-squashfs) worker_esp=${ALPINE_ESP_DIR:-build/esp-alpine} ;;
			synthetic) worker_esp=${ESP_DIR:-build/esp} ;;
			*)
				echo "unknown rootfs flavor for shard $name: $rootfs" > "$attempt_dir/stderr.log"
				worker_esp=
				;;
			esac
			if [ -n "$worker_esp" ]; then
				if BUNIX_TEST_RUN_ID="$run_id-$name-a$attempt" \
				    BUNIX_TEST_RUNTIME_DIR="$attempt_dir/runtime" \
				    FAILURE_DIR="$attempt_dir/failures" \
				    BUNIX_ROOTFS_METADATA_DIR="${BUNIX_ROOTFS_METADATA_DIR:-build/alpine-rootfs}" \
				    ESP_DIR="$worker_esp" \
				    OVMF_CODE="$ovmf" \
				    ROOTFS_FLAVOR="$rootfs" \
				    BUNIX_BOOT_PHASE="$boot_phase" \
				    SERIAL_LOG="$attempt_dir/serial.log" \
				    SMP="$worker_smp" \
				    QEMU_MEMORY="$worker_memory" \
				    QEMU_TIMEOUT="$worker_timeout" \
				    sh tools/test-boot.sh >"$attempt_dir/stdout.log" 2>"$attempt_dir/stderr.log" &&
				    { [ -z "$marker_file" ] || sh tools/check-markers.sh "$attempt_dir/serial.log" "$marker_file" >>"$attempt_dir/stdout.log" 2>>"$attempt_dir/stderr.log"; }; then
					attempt_ok=1
				else
					attempt_ok=0
				fi
			fi
		elif [ "$harness" = shell ] && [ -n "$worker_cmd" ]; then
			if BUNIX_TEST_RUN_ID="$run_id-$name-a$attempt" \
			    BUNIX_TEST_RUNTIME_DIR="$attempt_dir/runtime" \
			    FAILURE_DIR="$attempt_dir/failures" \
			    BUNIX_SHELL_PART="$parts" \
			    SMP="$worker_smp" \
			    QEMU_MEMORY="$worker_memory" \
			    QEMU_TIMEOUT="$worker_timeout" \
			    "$worker_cmd" test-shell-part >"$attempt_dir/stdout.log" 2>"$attempt_dir/stderr.log"; then
				attempt_ok=1
			else
				attempt_ok=0
			fi
		elif [ "$harness" = shell ]; then
			if BUNIX_TEST_RUN_ID="$run_id-$name-a$attempt" \
			    BUNIX_TEST_RUNTIME_DIR="$attempt_dir/runtime" \
			    FAILURE_DIR="$attempt_dir/failures" \
			    BUNIX_SHELL_PART="$parts" \
			    SMP="$worker_smp" \
			    QEMU_MEMORY="$worker_memory" \
			    QEMU_TIMEOUT="$worker_timeout" \
			    sh tools/test-shell.sh >"$attempt_dir/stdout.log" 2>"$attempt_dir/stderr.log"; then
				attempt_ok=1
			else
				attempt_ok=0
			fi
		else
			echo "unknown harness for shard $name: $harness" > "$attempt_dir/stderr.log"
			attempt_ok=0
		fi
		if [ "$attempt_ok" -eq 1 ]; then
			status=ok
			rc=0
			reason=
			break
		fi

		reason=$(sed -n '1p' "$attempt_dir/stderr.log" | tr '\t' ' ')
		attempt=$((attempt + 1))
	done

	now=$(date +%s)
	seconds=$((now - start))
	echo "$status" > "$worker_dir/status"
	echo "$seconds" > "$worker_dir/seconds"
	final_attempt=$attempt
	if [ "$final_attempt" -gt "$max_attempts" ]; then
		final_attempt=$max_attempts
	fi
	echo "$final_attempt" > "$worker_dir/attempts"
	cp "$worker_dir/attempt-$final_attempt/stdout.log" "$worker_dir/stdout.log" 2>/dev/null || : > "$worker_dir/stdout.log"
	cp "$worker_dir/attempt-$final_attempt/stderr.log" "$worker_dir/stderr.log" 2>/dev/null || : > "$worker_dir/stderr.log"
	printf '%s\n' "$reason" > "$worker_dir/reason"
	echo "test-parallel name=$name status=$status seconds=$seconds artifact=$worker_dir"
	return "$rc"
}

wait_batch() {
	status=0
	for pid in "$@"; do
		if ! wait "$pid"; then
			status=1
		fi
	done
	return "$status"
}

pids=
count=0
overall=0
launched=0
skipped=0
stopped=0

while IFS='	' read -r name smp memory timeout_seconds cost clean_boot harness rootfs boot_phase marker_file; do
	if [ -z "$name" ]; then
		continue
	fi
	if [ "$stopped" -eq 1 ]; then
		echo "test-parallel name=$name status=skip reason=stopped-after-failure"
		mkdir -p "$run_dir/$name"
		echo skip > "$run_dir/$name/status"
		echo 0 > "$run_dir/$name/seconds"
		echo stopped-after-failure > "$run_dir/$name/reason"
		echo 0 > "$run_dir/$name/attempts"
		echo "$name" >> "$run_dir/shards.txt"
		skipped=$((skipped + 1))
		continue
	fi
	echo "$name" >> "$run_dir/shards.txt"
	run_worker "$name" "$smp" "$memory" "$timeout_seconds" "$cost" "$clean_boot" "$harness" "$rootfs" "$boot_phase" "$marker_file" &
	pids="$pids $!"
	count=$((count + 1))
	launched=$((launched + 1))

	if [ "$count" -ge "$jobs" ]; then
		# shellcheck disable=SC2086
		if ! wait_batch $pids; then
			overall=1
			if [ "$stop_on_fail" -eq 1 ]; then
				stopped=1
			fi
		fi
		pids=
		count=0
	fi
done <<EOF_SHARDS
$(ordered_shards)
EOF_SHARDS

if [ "$count" -gt 0 ]; then
	# shellcheck disable=SC2086
	if ! wait_batch $pids; then
		overall=1
		if [ "$stop_on_fail" -eq 1 ]; then
			stopped=1
		fi
	fi
fi

if [ "$launched" -eq 0 ]; then
	echo "test-parallel status=fail reason=no-selected-shards set=$test_set"
	exit 2
fi

summary=$run_dir/summary.tsv
printf 'name\tstatus\tseconds\tartifact\treason\n' > "$summary"
while IFS= read -r name; do
	worker_dir=$run_dir/$name
	status=$(cat "$worker_dir/status" 2>/dev/null || echo missing)
	seconds=$(cat "$worker_dir/seconds" 2>/dev/null || echo 0)
	reason=$(cat "$worker_dir/reason" 2>/dev/null || echo '')
	printf '%s\t%s\t%s\t%s\t%s\n' "$name" "$status" "$seconds" "$worker_dir" "$reason" >> "$summary"
done < "$run_dir/shards.txt"

echo "test-parallel summary=$summary"
echo "test-parallel status=done run_id=$run_id launched=$launched skipped=$skipped artifact=$run_dir"
exit "$overall"
