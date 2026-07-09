#!/bin/sh

run_linux_signaltest() {
	send_script_sync <<'EOF_LINUX_SIGNALTEST'
/bin/signaltest && echo SIGNALTEST_OK
EOF_LINUX_SIGNALTEST
}

check_linux_signaltest() {
	wait_for_each_fixed "$log" "Linux signaltest marker missing" 75 220 \
		"linux sigchld self-pipe ok" "linux signaltest ok" SIGNALTEST_OK
	require_no_fixed "$log" "signaltest poll ready=" \
		"Linux signaltest SIGCHLD self-pipe poll timed out" 220
}
