#!/bin/sh

run_smoke() {
	send_script <<'EOF_SMOKE'
busybox cat /hello.txt >/dev/null && echo SMOKE_VFS_OK
busybox cat /proc/kthreads >/dev/null && echo SMOKE_PROCFS_OK
/bin/sysracetest
EOF_SMOKE
}

check_smoke() {
	check_fixed_markers_file "$log" "$script_dir/test-boot-markers.txt" \
		"smoke boot marker missing" 45 160
	wait_for_each_fixed "$log" "smoke regression missing" 45 180 \
		SMOKE_VFS_OK SMOKE_PROCFS_OK "sysrace ok"
}
