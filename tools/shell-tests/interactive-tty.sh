#!/bin/sh

run_interactive_tty() {
	send_script <<'EOF_SECRET_DENIED'
busybox cat /secret.txt
echo POSTCAT
EOF_SECRET_DENIED

	wait_for_fixed "$log" "cat: can't open '/secret.txt'" "busybox cat did not report denied /secret.txt access" 45 160
	require_no_fixed "$log" "root secret" "busybox cat leaked /secret.txt to login user" 160
	wait_for_fixed "$log" "POSTCAT" "shell did not continue after denied cat" 45 160

	send_bytes 'ecxx\177\177ho BACKSPACE_OK\n'
	wait_for_fixed "$log" "BACKSPACE_OK" "busybox backspace-edited command did not complete" 45 160

	send_script <<'EOF_CAT_INTERRUPT'
cat
EOF_CAT_INTERRUPT
	sleep 1
	prompts_before_cat_interrupt=$(current_prompt_count "~ $ ")
	send_bytes '\003'
	wait_for_prompt_count_gt "~ $ " "$prompts_before_cat_interrupt" "foreground Ctrl-C did not return to shell" 45 180
	send_script <<'EOF_CAT_INTERRUPT_DONE'
echo CTRL_C_OK
EOF_CAT_INTERRUPT_DONE
	wait_for_fixed "$log" "CTRL_C_OK" "foreground Ctrl-C marker missing" 45 180

	send_script <<'EOF_WATCH'
busybox watch -n 1 busybox echo WATCH_OK & watch_pid=$!
EOF_WATCH
	wait_for_fixed "$log" "watch_pid=" "busybox watch command was not submitted" 45 220
	wait_for_awk "$log" '{ sub(/\r$/, "") } /^WATCH_OK$/ { count++ } END { exit count >= 2 ? 0 : 1 }' "busybox watch did not repeatedly run child command" 45 220

	send_script <<'EOF_WATCH_DONE'
busybox kill $watch_pid
echo WATCH_DONE
EOF_WATCH_DONE
	wait_for_fixed "$log" "WATCH_DONE" "busybox watch was not killed after repeated child runs" 45 220

	login_prompts_before_exit=$(current_prompt_count "login: ")
	send_script <<'EOF_EXIT_USER'
cd /bin
[ "$(pwd)" = /bin ] && echo CHDIR_PWD_OK
exit
EOF_EXIT_USER

	wait_for_fixed "$log" "CHDIR_PWD_OK" "busybox chdir/pwd regression failed" 45 160
	wait_for_prompt_count_gt "login: " "$login_prompts_before_exit" "login prompt did not return after shell exit" 45 180
}
