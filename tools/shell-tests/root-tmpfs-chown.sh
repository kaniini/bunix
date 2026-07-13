#!/bin/sh

run_root_tmpfs_chown() {
	login_prompts_before_root_exit=$(current_prompt_count "login: ")
	send_script_sync <<'EOF_ROOT_CHOWN'
echo ROOT_CHOWN_PAYLOAD > /tmp/bunix-write.txt
busybox chown 0:0 /tmp/bunix-write.txt
busybox stat -c "%u:%g" /tmp/bunix-write.txt
busybox chown 1000:1000 /tmp/bunix-write.txt
busybox stat -c "%u:%g" /tmp/bunix-write.txt
EOF_ROOT_CHOWN
	send_script <<'EOF_ROOT_CHOWN_EXIT'
exit
EOF_ROOT_CHOWN_EXIT
	wait_for_exact_line "$log" "0:0" "tmpfs chown did not update owner" 45 180
	wait_for_exact_line "$log" "1000:1000" "tmpfs chown did not update nonzero owner/group" 45 180
	wait_for_prompt_count_gt "login: " "$login_prompts_before_root_exit" "login prompt did not return after root shell exit" 45 180
}
