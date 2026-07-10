#!/bin/sh
set -eu

script_dir=$(CDPATH= cd "$(dirname "$0")" && pwd)
. "$script_dir/path-safety.sh"

run_root=${BUNIX_TEST_RUN_ROOT:-build/test-runs}
keep=${BUNIX_TEST_KEEP_RUNS:-10}
dry_run=${BUNIX_TEST_PRUNE_DRY_RUN:-0}

case "$keep" in
''|*[!0-9]*)
	echo "BUNIX_TEST_KEEP_RUNS must be a non-negative integer" >&2
	exit 2
	;;
esac

case "$dry_run" in
1|yes|true|on) dry_run=1 ;;
0|no|false|off|'') dry_run=0 ;;
*)
	echo "BUNIX_TEST_PRUNE_DRY_RUN must be 0/1, yes/no, true/false, or on/off" >&2
	exit 2
	;;
esac

run_root=$(path_safety_require_generated_path "$run_root" "BUNIX_TEST_RUN_ROOT")

if [ ! -d "$run_root" ]; then
	echo "test-prune-artifacts status=ok reason=no-run-root path=$run_root"
	exit 0
fi

run_failed() {
	dir=$1

	if [ -r "$dir/summary.tsv" ] && awk -F '\t' 'NR > 1 && $2 == "fail" { found = 1 } END { exit found ? 0 : 1 }' "$dir/summary.tsv"; then
		return 0
	fi
	if grep -R '^fail$' "$dir"/*/status >/dev/null 2>&1; then
		return 0
	fi
	return 1
}

count=0
kept=0
pruned=0

for name in $(ls -1 "$run_root" 2>/dev/null | sort -r); do
	dir=$run_root/$name
	if [ ! -d "$dir" ]; then
		continue
	fi
	count=$((count + 1))
	if [ "$count" -le "$keep" ]; then
		echo "test-prune-artifacts name=$name status=keep reason=newest"
		kept=$((kept + 1))
		continue
	fi
	if run_failed "$dir"; then
		echo "test-prune-artifacts name=$name status=keep reason=failed"
		kept=$((kept + 1))
		continue
	fi

	if [ "$dry_run" -eq 1 ]; then
		echo "test-prune-artifacts name=$name status=would-prune artifact=$dir"
	else
		safe_rm_rf "$dir" "test run artifact"
		echo "test-prune-artifacts name=$name status=pruned artifact=$dir"
	fi
	pruned=$((pruned + 1))
done

echo "test-prune-artifacts status=done kept=$kept pruned=$pruned dry_run=$dry_run root=$run_root"
