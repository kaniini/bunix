#!/bin/sh

run_smoke() {
	send_script_sync <<'EOF_SMOKE'
busybox cat /hello.txt >/dev/null && echo SMOKE_VFS_OK
busybox cat /proc/kthreads >/dev/null && echo SMOKE_PROCFS_OK
/bin/sysracetest
busybox dmesg
EOF_SMOKE
}

check_smoke() {
	wait_for_each_fixed "$log" "smoke regression missing" 45 180 \
		"bootstrap: linux init exec" \
		"first: large buffer ok" \
		SMOKE_VFS_OK SMOKE_PROCFS_OK "sysrace ok"
}
