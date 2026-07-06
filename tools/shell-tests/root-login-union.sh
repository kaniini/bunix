#!/bin/sh

run_root_login_union() {
	root_prompts_before_root=$(current_prompt_count "~ # ")
	login_user root root "~ # " "$root_prompts_before_root"

	send_script <<'EOF_ROOT'
busybox id
env
busybox cat /secret.txt && echo ROOT_SECRET_OK
EOF_ROOT

	wait_for_fixed "$log" "uid=0(root)" "root login did not report uid 0" 45 180
	wait_for_each_fixed "$log" "root login environment missing" 45 180 \
		"HOME=/root" "USER=root" "LOGNAME=root"
	wait_for_fixed "$log" "ROOT_SECRET_OK" "root login could not read /secret.txt" 45 180

	send_script <<'EOF_ROOT_UNION'
echo UNION_LOWER_PARENT_CREATE_PAYLOAD > /usr/share/bunix/nested/created.txt
busybox cat /usr/share/bunix/nested/created.txt && echo UNION_LOWER_PARENT_CREATE_OK
busybox cat /.upper/usr/share/bunix/nested/created.txt && echo UNION_LOWER_PARENT_UPPER_OK
echo UNION_LOWER_COPYUP_APPEND_PAYLOAD >> /usr/share/bunix/nested/hello.txt
busybox cat /usr/share/bunix/nested/hello.txt && echo UNION_LOWER_COPYUP_APPEND_OK
busybox cat /.upper/usr/share/bunix/nested/hello.txt && echo UNION_LOWER_COPYUP_UPPER_OK
busybox umount /.lower || echo UNION_LOWER_UMOUNT_BUSY_OK
busybox umount /.upper || echo UNION_UPPER_UMOUNT_BUSY_OK
busybox mount -t tmpfs tmpfs /.lower || echo UNION_LOWER_REPLACE_BUSY_OK
busybox mount -t tmpfs tmpfs /.upper || echo UNION_UPPER_REPLACE_BUSY_OK
busybox cat /hello.txt && echo UNION_PINNED_ROUTES_STILL_OK
EOF_ROOT_UNION
}

check_root_login_union() {
	wait_for_each_fixed "$log" "root unionfs lower-parent regression missing" 45 220 \
		UNION_LOWER_PARENT_CREATE_OK UNION_LOWER_PARENT_UPPER_OK \
		UNION_LOWER_COPYUP_APPEND_OK UNION_LOWER_COPYUP_UPPER_OK \
		UNION_LOWER_UMOUNT_BUSY_OK UNION_UPPER_UMOUNT_BUSY_OK \
		UNION_LOWER_REPLACE_BUSY_OK UNION_UPPER_REPLACE_BUSY_OK \
		UNION_PINNED_ROUTES_STILL_OK
	wait_for_each_fixed_count "$log" 2 "root unionfs lower-parent payload missing" 45 220 \
		UNION_LOWER_PARENT_CREATE_PAYLOAD UNION_LOWER_COPYUP_APPEND_PAYLOAD
}
