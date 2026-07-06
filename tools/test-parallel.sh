#!/bin/sh
set -eu

manifest=${BUNIX_SHELL_SHARDS:-tools/shell-shards.tsv}
test_set=${BUNIX_TEST_SET:-${BUNIX_SHELL_PART:-all}}
run_root=${BUNIX_TEST_RUN_ROOT:-build/test-runs}
run_id=${BUNIX_TEST_RUN_ID:-$(date -u +%Y%m%dT%H%M%SZ)-$$}
make_cmd=${MAKE:-make}

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

if [ ! -r "$manifest" ]; then
	echo "shell shard manifest is not readable: $manifest" >&2
	exit 2
fi

mkdir -p "$run_root/$run_id"
echo "test-parallel status=plan run_id=$run_id jobs=$jobs set=$test_set artifact=$run_root/$run_id"

selected_shards() {
	awk -F '\t' -v set="$test_set" '
	function valid_uint(value) {
		return value ~ /^[0-9]+$/
	}
	function valid_memory(value) {
		return value ~ /^[0-9]+[KMG]?$/
	}
	function selected(name, phase,    list, count, i, part) {
		if (set == "" || set == "all" || set == "shell")
			return 1
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
	selected($1, $2) { print $1 "\t" $3 "\t" $4 "\t" $5 "\t" $6 "\t" $7 }
	' "$manifest"
}

worker_parts() {
	case "$1" in
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
	worker_smp=${BUNIX_VM_SMP_OVERRIDE:-$smp}
	worker_memory=${BUNIX_VM_MEMORY_OVERRIDE:-$memory}
	worker_timeout=$(effective_timeout "$timeout_seconds")
	parts=$(worker_parts "$name")
	worker_dir=$run_root/$run_id/$name
	start=$(date +%s)

	mkdir -p "$worker_dir"
	echo "test-parallel name=$name status=start smp=$worker_smp memory=$worker_memory timeout=$worker_timeout cost=$cost clean_boot=$clean_boot artifact=$worker_dir"

	if BUNIX_TEST_RUN_ID="$run_id-$name" \
	    BUNIX_TEST_RUNTIME_DIR="$worker_dir/runtime" \
	    FAILURE_DIR="$worker_dir/failures" \
	    BUNIX_SHELL_PART="$parts" \
	    SMP="$worker_smp" \
	    QEMU_MEMORY="$worker_memory" \
	    QEMU_TIMEOUT="$worker_timeout" \
	    "$make_cmd" test-shell-part >"$worker_dir/stdout.log" 2>"$worker_dir/stderr.log"; then
		status=ok
		rc=0
	else
		status=fail
		rc=1
	fi

	now=$(date +%s)
	seconds=$((now - start))
	echo "$status" > "$worker_dir/status"
	echo "$seconds" > "$worker_dir/seconds"
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

while IFS='	' read -r name smp memory timeout_seconds cost clean_boot; do
	if [ -z "$name" ]; then
		continue
	fi
	if [ "$name" = root-mount-soak ]; then
		echo "test-parallel name=$name status=skip reason=unimplemented"
		skipped=$((skipped + 1))
		continue
	fi

	run_worker "$name" "$smp" "$memory" "$timeout_seconds" "$cost" "$clean_boot" &
	pids="$pids $!"
	count=$((count + 1))
	launched=$((launched + 1))

	if [ "$count" -ge "$jobs" ]; then
		# shellcheck disable=SC2086
		if ! wait_batch $pids; then
			overall=1
		fi
		pids=
		count=0
	fi
done <<EOF_SHARDS
$(selected_shards)
EOF_SHARDS

if [ "$count" -gt 0 ]; then
	# shellcheck disable=SC2086
	if ! wait_batch $pids; then
		overall=1
	fi
fi

if [ "$launched" -eq 0 ]; then
	echo "test-parallel status=fail reason=no-selected-shards set=$test_set"
	exit 2
fi

echo "test-parallel status=done run_id=$run_id launched=$launched skipped=$skipped artifact=$run_root/$run_id"
exit "$overall"
