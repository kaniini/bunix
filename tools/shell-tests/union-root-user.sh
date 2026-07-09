#!/bin/sh

run_union_root_user() {
	send_script_sync <<'EOF_UNION_RENAME'
busybox cat /hello.txt && echo UNION_ROOT_LOWER_OK
busybox mv /rename-lower.txt /rename-upper.txt && echo UNION_LOWER_RENAME_CREATE_OK
busybox cat /rename-upper.txt | busybox grep "nested rootfs file" && echo UNION_LOWER_RENAME_READ_OK
busybox test ! -e /rename-lower.txt && echo UNION_LOWER_RENAME_OLD_GONE_OK
busybox test -e /.upper/rename-upper.txt && echo UNION_LOWER_RENAME_UPPER_OK
EOF_UNION_RENAME
	wait_for_fixed "$log" "UNION_LOWER_RENAME_UPPER_OK" "unionfs lower rename commands did not drain" 45 220

	send_script_sync <<'EOF_UNION_ROOT'
busybox ln /hello.txt /hello-hard.txt && echo UNION_LOWER_HARDLINK_CREATE_OK
busybox cat /hello-hard.txt | busybox grep "rootfs: module" && echo UNION_LOWER_HARDLINK_READ_OK
busybox test -e /.upper/hello.txt && busybox test -e /.upper/hello-hard.txt && echo UNION_LOWER_HARDLINK_COPYUP_OK
echo UNION_ROOT_BASE_PAYLOAD > /created.txt
echo UNION_ROOT_APPEND_PAYLOAD >> /created.txt
busybox cat /created.txt && echo UNION_ROOT_APPEND_CAT_OK
busybox cat /.upper/created.txt && echo UNION_ROOT_UPPER_BACKING_OK
UNION_LONG_NAME=bunix-union-root-name-longer-than-sixteen.txt
echo UNION_ROOT_LONG_PAYLOAD > "/$UNION_LONG_NAME"
busybox ls / | busybox grep "$UNION_LONG_NAME" && echo UNION_ROOT_LONG_READDIR_OK
busybox cat "/$UNION_LONG_NAME" && echo UNION_ROOT_LONG_CAT_OK
UNION_NAME_MAX=bunix-union-name-aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.txt
echo UNION_NAME_MAX_PAYLOAD > "/$UNION_NAME_MAX"
busybox ls / | busybox grep "$UNION_NAME_MAX" && echo UNION_NAME_MAX_READDIR_OK
busybox cat "/$UNION_NAME_MAX" && echo UNION_NAME_MAX_CAT_OK
busybox ls / | busybox grep created.txt && echo UNION_ROOT_READDIR_UPPER_OK
busybox ls / | busybox grep secret.txt && echo UNION_ROOT_READDIR_LOWER_OK
busybox rm /created.txt && echo UNION_ROOT_UNLINK_UPPER_OK
busybox test ! -e /created.txt && echo UNION_ROOT_UNLINK_UPPER_GONE_OK
busybox cat /hello.txt && echo UNION_ROOT_LOWER_STILL_OK
EOF_UNION_ROOT
	wait_for_fixed "$log" "UNION_ROOT_LOWER_STILL_OK" "unionfs root commands did not drain" 120 220
}

check_union_root_user() {
	check_exact_markers_file "$log" "$script_dir/shell-tests/union-root-user.exact-markers.txt" \
		"union root user exact marker missing" 75 220
	wait_for_each_fixed "$log" "unionfs user-root regression missing" 45 220 \
		UNION_ROOT_LOWER_OK UNION_LOWER_RENAME_CREATE_OK \
		UNION_LOWER_RENAME_READ_OK UNION_LOWER_RENAME_OLD_GONE_OK \
		UNION_LOWER_RENAME_UPPER_OK UNION_LOWER_HARDLINK_CREATE_OK \
		UNION_LOWER_HARDLINK_READ_OK UNION_LOWER_HARDLINK_COPYUP_OK \
		UNION_ROOT_APPEND_CAT_OK UNION_ROOT_UPPER_BACKING_OK \
		UNION_ROOT_LONG_READDIR_OK UNION_ROOT_LONG_CAT_OK \
		UNION_NAME_MAX_READDIR_OK UNION_NAME_MAX_CAT_OK \
		UNION_ROOT_READDIR_UPPER_OK UNION_ROOT_READDIR_LOWER_OK \
		UNION_ROOT_UNLINK_UPPER_OK UNION_ROOT_UNLINK_UPPER_GONE_OK \
		UNION_ROOT_LOWER_STILL_OK
	wait_for_each_fixed_count "$log" 2 "union root payload missing from file output" 45 220 \
		UNION_ROOT_BASE_PAYLOAD UNION_ROOT_APPEND_PAYLOAD \
		UNION_ROOT_LONG_PAYLOAD UNION_NAME_MAX_PAYLOAD
}
