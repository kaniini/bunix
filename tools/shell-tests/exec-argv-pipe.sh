#!/bin/sh

run_exec_argv_pipe() {
	send_script <<'EOF_EXEC_ARGV_PIPE'
busybox echo PIPE_OK | busybox cat
busybox cat /hello.txt | busybox cat && echo PIPE_FILE_OK
busybox cat /proc/kthreads >/dev/null && echo PROCFS_OK
busybox echo BUSYBOX_ARGV_OK
busybox sh -c "test \"\$13\" = m && echo BUSYBOX_MANY_ARGV_OK" _ a b c d e f g h i j k l m
BUNIX_LONG_ENV=abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 busybox sh -c "test \"\$BUNIX_LONG_ENV\" = abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 && echo BUSYBOX_LONG_ENV_OK"
set --
i=1
while [ "$i" -le 300 ]; do set -- "$@" "x$i"; i=$((i + 1)); done
busybox sh -c 'test "$300" = x300 && echo BUSYBOX_ARGV300_OK' _ "$@"
i=1
while [ "$i" -le 300 ]; do eval "BUNIX_EXEC_ENV_$i=x$i; export BUNIX_EXEC_ENV_$i"; i=$((i + 1)); done
busybox sh -c 'test "$BUNIX_EXEC_ENV_300" = x300 && echo BUSYBOX_ENV300_OK'
EOF_EXEC_ARGV_PIPE
}

check_exec_argv_pipe() {
	wait_for_fixed "$log" "BUSYBOX_ARGV_OK" "busybox argv regression command did not complete" 45 160
	wait_for_fixed "$log" "BUSYBOX_MANY_ARGV_OK" "busybox many-argv regression command did not complete" 45 160
	wait_for_fixed "$log" "BUSYBOX_ARGV300_OK" "busybox large-argv regression command did not complete" 45 160
	wait_for_fixed "$log" "BUSYBOX_LONG_ENV_OK" "busybox long-env regression command did not complete" 45 160
	wait_for_fixed "$log" "BUSYBOX_ENV300_OK" "busybox large-env regression command did not complete" 45 160
	wait_for_fixed "$log" "PIPE_OK" "busybox echo pipe did not complete" 45 180
	wait_for_fixed "$log" "PIPE_FILE_OK" "busybox cat pipe did not complete" 45 180
	wait_for_fixed "$log" "PROCFS_OK" "procfs translator read did not complete" 45 180
}
