#!/bin/sh

run_tty_sigint_session() {
	send_script <<'EOF_TTY_SIGINT_SESSION'
/bin/ttysigtest
EOF_TTY_SIGINT_SESSION
	wait_for_fixed "$log" "ttysigtest ready" "ttysigtest did not block on tty read" 45 160
	send_bytes '\003'
}

check_tty_sigint_session() {
	wait_for_fixed "$log" "ttysigtest ok" "tty generated SIGINT crossed session boundary or missed reader" 45 160
}
