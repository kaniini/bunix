#!/bin/sh

run_root_tmpfs_chown() {
	login_prompts_before_root_exit=$(current_prompt_count "login: ")
	send_script <<'EOF_ROOT_CHOWN'
busybox chown 0:0 /tmp/bunix-write.txt
busybox stat -c "%u:%g" /tmp/bunix-write.txt
exit
EOF_ROOT_CHOWN
	wait_for_exact_line "$log" "0:0" "tmpfs chown did not update owner" 45 180
	wait_for_prompt_count_gt "login: " "$login_prompts_before_root_exit" "login prompt did not return after root shell exit" 45 180
}
