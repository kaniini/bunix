#!/bin/sh

run_procfs_cmdline() {
	send_script <<'EOF_PROCFS_CMDLINE'
busybox sleep 20 &
set -- $(busybox ps | busybox grep -a '[s]leep 20')
child=$1
busybox test -d /proc/$child && echo PROCFS_CMDLINE_PID_DIR_OK
busybox cat /proc/$child/status | busybox grep -a busybox && echo PROCFS_CMDLINE_STATUS_OK
busybox cat /proc/$child/cmdline | busybox tr '\000' ' ' | busybox grep -a "busybox sleep 20" && echo PROCFS_CMDLINE_ARGV_OK
busybox kill $child
wait $child
echo PROCFS_CMDLINE_DONE
EOF_PROCFS_CMDLINE
}

check_procfs_cmdline() {
	wait_for_each_fixed "$log" "procfs live cmdline marker missing" 45 180 \
		PROCFS_CMDLINE_PID_DIR_OK PROCFS_CMDLINE_STATUS_OK \
		PROCFS_CMDLINE_ARGV_OK PROCFS_CMDLINE_DONE
}
