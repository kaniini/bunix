#!/bin/sh

run_procfs_self_pipe() {
	send_script <<'EOF_PROCFS_SELF_PIPE_OK'
/bin/procfspipetest
EOF_PROCFS_SELF_PIPE_OK
}

check_procfs_self_pipe() {
	wait_for_fixed "$log" "PROCFSPIPETEST_STAT_1" \
		"first piped /proc/self/stat read did not complete" 180 220
	wait_for_fixed "$log" "PROCFSPIPETEST_OK" \
		"repeated procfs self pipelines did not complete" 560 420
}
