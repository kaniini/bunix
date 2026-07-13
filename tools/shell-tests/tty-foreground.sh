#!/bin/sh

run_tty_foreground() {
	send_script_sync <<'EOF_TTY_FOREGROUND'
/bin/ttytest
EOF_TTY_FOREGROUND
}

check_tty_foreground() {
	wait_for_fixed "$log" "ttytest ok" "tty foreground process group test failed" 45 160
}
