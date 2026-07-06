#!/bin/sh

run_long_login() {
	long_root_prompts_before=$(current_prompt_count "~ # ")
	login_user administrator_with_long_name password_longer_than_sixteen "~ # " "$long_root_prompts_before"

	send_script <<'EOF_LONG_LOGIN'
busybox id
env
exit
EOF_LONG_LOGIN
}

check_long_login() {
	wait_for_each_fixed "$log" "long login regression missing" 45 180 \
		"uid=0(root)" "USER=administrator_with_long_name" \
		"LOGNAME=administrator_with_long_name"
}
