#!/bin/sh

run_relogin_session() {
	relogin_prompts_before=$(current_prompt_count "login: ")
	send_script_sync <<'EOF_RELOGIN_SESSION_FIRST'
busybox uptime | busybox grep " 1 users" && echo RELOGIN_FIRST_UPTIME_OK
EOF_RELOGIN_SESSION_FIRST
	send_script <<'EOF_RELOGIN_SESSION_FIRST_EXIT'
exit
EOF_RELOGIN_SESSION_FIRST_EXIT
	wait_for_prompt_count_gt "login: " "$relogin_prompts_before" "login prompt did not return after first relogin-session exit" 90 180

	relogin_shells_before=$(current_prompt_count "~ $ ")
	login_user kaniini bunix "~ $ " "$relogin_shells_before"
	relogin_prompts_before=$(current_prompt_count "login: ")
	send_script_sync <<'EOF_RELOGIN_SESSION_SECOND'
busybox uptime | busybox grep " 1 users" && echo RELOGIN_SECOND_UPTIME_OK
EOF_RELOGIN_SESSION_SECOND
	send_script <<'EOF_RELOGIN_SESSION_SECOND_EXIT'
exit
EOF_RELOGIN_SESSION_SECOND_EXIT
	wait_for_prompt_count_gt "login: " "$relogin_prompts_before" "login prompt did not return after second relogin-session exit" 90 180

	relogin_shells_before=$(current_prompt_count "~ $ ")
	login_user kaniini bunix "~ $ " "$relogin_shells_before"
	relogin_prompts_before=$(current_prompt_count "login: ")
	send_script_sync <<'EOF_RELOGIN_SESSION_THIRD'
busybox uptime | busybox grep " 1 users" && echo RELOGIN_THIRD_UPTIME_OK
EOF_RELOGIN_SESSION_THIRD
	send_script <<'EOF_RELOGIN_SESSION_THIRD_EXIT'
exit
EOF_RELOGIN_SESSION_THIRD_EXIT
	wait_for_prompt_count_gt "login: " "$relogin_prompts_before" "login prompt did not return after third relogin-session exit" 90 180
}

check_relogin_session() {
	wait_for_each_fixed "$log" "relogin session regression missing" 45 180 \
		RELOGIN_FIRST_UPTIME_OK RELOGIN_SECOND_UPTIME_OK \
		RELOGIN_THIRD_UPTIME_OK
}
