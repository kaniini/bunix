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
set -- $(busybox cat /proc/$child/stat)
stat_pid=$1
stat_comm=$2
stat_ppid=$4
busybox test "$stat_pid" = "$child" && busybox test "$stat_comm" != "(process)"
[ "$?" -eq 0 ] && echo PROCFS_CMDLINE_STAT_ID_OK
busybox test "$stat_ppid" = "$$"
[ "$?" -eq 0 ] && echo PROCFS_CMDLINE_STAT_PPID_OK
busybox readlink /proc/$child/exe > /tmp/proc-child-exe && ! busybox grep -a /bin/process /tmp/proc-child-exe
[ "$?" -eq 0 ] && echo PROCFS_CMDLINE_EXE_OK
busybox top -b -n 1 > /tmp/proc-top && ! busybox grep -a /bin/process /tmp/proc-top && ! busybox grep -a '{process}' /tmp/proc-top
[ "$?" -eq 0 ] && echo PROCFS_CMDLINE_TOP_OK
busybox kill $child
wait $child
echo PROCFS_CMDLINE_DONE
EOF_PROCFS_CMDLINE
}

check_procfs_cmdline() {
	wait_for_each_fixed "$log" "procfs live cmdline marker missing" 45 180 \
		PROCFS_CMDLINE_PID_DIR_OK PROCFS_CMDLINE_STATUS_OK \
		PROCFS_CMDLINE_ARGV_OK PROCFS_CMDLINE_STAT_ID_OK \
		PROCFS_CMDLINE_STAT_PPID_OK PROCFS_CMDLINE_EXE_OK \
		PROCFS_CMDLINE_TOP_OK PROCFS_CMDLINE_DONE
}
