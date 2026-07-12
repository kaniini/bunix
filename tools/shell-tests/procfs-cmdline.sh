#!/bin/sh

run_procfs_cmdline() {
	send_script_sync <<'EOF_PROCFS_CMDLINE'
busybox sleep 120 &
child=$!
busybox test -n "$child" && busybox test -d /proc/$child
[ "$?" -eq 0 ] && echo PROCFS_CMDLINE_PID_DIR_OK
busybox cat /proc/$child/status > /tmp/proc-child-status && busybox grep -a busybox /tmp/proc-child-status
[ "$?" -eq 0 ] && echo PROCFS_CMDLINE_STATUS_OK
busybox cat /proc/$child/cmdline > /tmp/proc-child-cmdline && busybox grep -a busybox /tmp/proc-child-cmdline && busybox grep -a sleep /tmp/proc-child-cmdline && busybox grep -a 120 /tmp/proc-child-cmdline
[ "$?" -eq 0 ] && echo PROCFS_CMDLINE_ARGV_OK
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
