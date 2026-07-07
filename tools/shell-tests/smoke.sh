#!/bin/sh

run_smoke() {
	send_script <<'EOF_SMOKE'
busybox cat /hello.txt >/dev/null && echo SMOKE_VFS_OK
busybox cat /proc/kthreads >/dev/null && echo SMOKE_PROCFS_OK
/bin/sysracetest
EOF_SMOKE
}

check_smoke() {
	marker_file=$script_dir/test-boot-markers-squashfs.txt
	if [ "${SMP:-2}" = 1 ]; then
		marker_file=$script_dir/test-boot-markers-squashfs-up.txt
	fi

	check_fixed_markers_file "$log" "$marker_file" \
		"smoke boot marker missing" 45 160
	wait_for_each_fixed "$log" "smoke regression missing" 45 180 \
		SMOKE_VFS_OK SMOKE_PROCFS_OK "sysrace ok"
}
