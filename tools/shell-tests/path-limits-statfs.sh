#!/bin/sh

run_path_limits_statfs() {
	send_script_sync <<'EOF_PATH_LIMITS_STATFS'
LONG_TMP=/tmp/bunix-pathmax2
busybox mkdir "$LONG_TMP"
LONG_TMP=$LONG_TMP/segment-aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
busybox mkdir "$LONG_TMP"
LONG_TMP=$LONG_TMP/segment-bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb
busybox mkdir "$LONG_TMP"
LONG_TMP=$LONG_TMP/segment-cccccccccccccccccccccccccccccccc
busybox mkdir "$LONG_TMP"
LONG_TMP=$LONG_TMP/segment-dddddddddddddddddddddddddddddddd
busybox mkdir "$LONG_TMP"
LONG_TMP=$LONG_TMP/segment-eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee
busybox mkdir "$LONG_TMP"
LONG_TMP=$LONG_TMP/segment-ffffffffffffffffffffffffffffffff
busybox mkdir "$LONG_TMP" && echo PATHMAX2_TMP_MKDIR_OK
echo PATHMAX2_TMP_PAYLOAD > "$LONG_TMP/file.txt"
busybox cat "$LONG_TMP/file.txt" && echo PATHMAX2_TMP_FILE_OK
cd "$LONG_TMP" && echo PATHMAX2_TMP_CHDIR_OK
pwd
cd /
/bin/pathmaxtest
/bin/patherrtest
/bin/statidtest
busybox df / /tmp /proc >/dev/null && echo STATFS_DF_OK
EOF_PATH_LIMITS_STATFS
}

check_path_limits_statfs() {
	check_exact_markers_file "$log" "$script_dir/shell-tests/path-limits-statfs.exact-markers.txt" \
		"path/statfs exact marker missing" 75 220
	wait_for_each_fixed "$log" "tmpfs path limit regression missing" 45 220 \
		PATHMAX2_TMP_MKDIR_OK PATHMAX2_TMP_FILE_OK PATHMAX2_TMP_CHDIR_OK \
		PATHMAX2_TMP_PAYLOAD
	wait_for_each_fixed_count "$log" 2 "path limit payload missing from file output" 45 220 \
		PATHMAX2_TMP_PAYLOAD
	wait_for_fixed "$log" "linux pathmax ok" "linux pathmax regression failed" 75 220
	wait_for_fixed "$log" "linux patherr ok" "empty Linux path errno regression failed" 45 220
	wait_for_fixed "$log" "linux statid ok" "Linux stat identity regression failed" 45 220
	wait_for_fixed "$log" "STATFS_DF_OK" "busybox df did not complete through statfs/fstatfs" 45 220
}
