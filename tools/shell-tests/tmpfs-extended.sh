#!/bin/sh

run_tmpfs_extended() {
	tmpfs_extended_old_line_delay=$send_line_delay
	send_line_delay=${BUNIX_TMPFS_EXTENDED_LINE_DELAY:-0.35}

	send_tmpfs_extended_script() {
		while IFS= read -r line || [ -n "$line" ]; do
			tmpfs_extended_prompts_before=$(current_prompt_count "$prompt_probe")
			tmpfs_extended_cpr_before=$(current_prompt_count "$cpr_probe")
			send_line "$line"
			wait_for_shell_prompt_count_gt "$tmpfs_extended_prompts_before" \
				"$tmpfs_extended_cpr_before" \
				"tmpfs extended prompt did not return after: $line" \
				90 220
			sleep "$send_line_delay"
		done
	}

	send_tmpfs_extended_chunk() {
		tmpfs_extended_marker=$1
		shift
		send_tmpfs_extended_script
		wait_for_fixed "$log" "$tmpfs_extended_marker" \
			"tmpfs extended commands did not drain" 60 220
	}

	send_tmpfs_extended_script <<'EOF_TMPFS_BASELINE'
echo TMP_WRITE_OK > /tmp/bunix-write.txt
busybox stat -c "%u:%g %a" /tmp/bunix-write.txt
busybox mkdir /tmp/bunix-dir
[ "$?" -eq 0 ] && echo TMP_MKDIR_OK
busybox test -d /tmp/bunix-dir
[ "$?" -eq 0 ] && echo TMP_MKDIR_STAT_OK
LONG_TMP_NAME=bunix-readdir-name-longer-than-sixteen.txt
echo TMP_LONG_READDIR_PAYLOAD > "/tmp/$LONG_TMP_NAME"
busybox ls /tmp | busybox grep "$LONG_TMP_NAME"
[ "$?" -eq 0 ] && echo TMP_LONG_READDIR_OK
busybox cat "/tmp/$LONG_TMP_NAME"
[ "$?" -eq 0 ] && echo TMP_LONG_NAME_CAT_OK
EOF_TMPFS_BASELINE
	wait_for_fixed "$log" "TMP_LONG_NAME_CAT_OK" "tmpfs baseline long-name commands did not drain" 240 220

	send_tmpfs_extended_chunk TMP_NAME_MAX_CAT_OK <<'EOF_TMPFS_EXTENDED_1'
TMP_NAME_MAX=bunix-tmp-name-aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.txt
echo TMP_NAME_MAX_PAYLOAD > "/tmp/$TMP_NAME_MAX"
busybox ls /tmp | busybox grep "$TMP_NAME_MAX"
[ "$?" -eq 0 ] && echo TMP_NAME_MAX_READDIR_OK
busybox cat "/tmp/$TMP_NAME_MAX"
[ "$?" -eq 0 ] && echo TMP_NAME_MAX_CAT_OK
EOF_TMPFS_EXTENDED_1

	send_tmpfs_extended_chunk TMP_FIFO_STAT_OK <<'EOF_TMPFS_EXTENDED_2'
busybox touch /tmp/bunix-touch.txt
[ "$?" -eq 0 ] && echo TMP_TOUCH_CREATE_OK
busybox touch /tmp/bunix-write.txt
[ "$?" -eq 0 ] && echo TMP_TOUCH_EXISTING_OK
busybox mkfifo /tmp/bunix-fifo
[ "$?" -eq 0 ] && echo TMP_MKFIFO_OK
busybox test -p /tmp/bunix-fifo
[ "$?" -eq 0 ] && echo TMP_FIFO_STAT_OK
EOF_TMPFS_EXTENDED_2

	send_tmpfs_extended_chunk TMP_HARDLINK_RENAME_OK <<'EOF_TMPFS_EXTENDED_3'
echo TMP_LINK_ONE > /tmp/bunix-link-src.txt
busybox ln /tmp/bunix-link-src.txt /tmp/bunix-link-dst.txt
[ "$?" -eq 0 ] && echo TMP_HARDLINK_CREATE_OK
echo TMP_LINK_TWO >> /tmp/bunix-link-dst.txt
busybox cat /tmp/bunix-link-src.txt | busybox grep TMP_LINK_TWO
[ "$?" -eq 0 ] && echo TMP_HARDLINK_SHARE_OK
busybox mv /tmp/bunix-link-src.txt /tmp/bunix-link-renamed.txt
echo TMP_LINK_THREE >> /tmp/bunix-link-renamed.txt
busybox cat /tmp/bunix-link-dst.txt | busybox grep TMP_LINK_THREE
[ "$?" -eq 0 ] && echo TMP_HARDLINK_RENAME_OK
EOF_TMPFS_EXTENDED_3

	send_tmpfs_extended_chunk RM_R_DIR_OK <<'EOF_TMPFS_EXTENDED_4'
busybox mkdir /tmp/bunix-rmdir
busybox rmdir /tmp/bunix-rmdir
busybox test ! -e /tmp/bunix-rmdir
[ "$?" -eq 0 ] && echo RMDIR_OK
busybox mkdir /tmp/bunix-rm-r-dir
busybox rm -r /tmp/bunix-rm-r-dir
[ "$?" -eq 0 ] && echo RM_R_DIR_OK
EOF_TMPFS_EXTENDED_4

	send_tmpfs_extended_chunk TMP_DIR_RENAME_OLD_GONE_OK <<'EOF_TMPFS_EXTENDED_5'
busybox rm -rf /tmp/rename-src /tmp/rename-dst /tmp/rename-parent /tmp/rename-self
busybox mkdir /tmp/rename-src
busybox mkdir /tmp/rename-src/sub
echo TMP_DIR_RENAME_PAYLOAD > /tmp/rename-src/sub/file
busybox mv /tmp/rename-src /tmp/rename-dst
[ "$?" -eq 0 ] && echo TMP_DIR_RENAME_NONEMPTY_OK
busybox cat /tmp/rename-dst/sub/file | busybox grep TMP_DIR_RENAME_PAYLOAD
[ "$?" -eq 0 ] && echo TMP_DIR_RENAME_SUBTREE_OK
busybox test ! -e /tmp/rename-src
[ "$?" -eq 0 ] && echo TMP_DIR_RENAME_OLD_GONE_OK
EOF_TMPFS_EXTENDED_5

	send_tmpfs_extended_chunk TMP_DIR_REPLACE_SUBTREE_OK <<'EOF_TMPFS_EXTENDED_6'
busybox mkdir /tmp/rename-parent
busybox mkdir /tmp/rename-parent/src
busybox mkdir /tmp/rename-parent/src/sub
busybox mkdir /tmp/rename-parent/empty
echo TMP_DIR_REPLACE_PAYLOAD > /tmp/rename-parent/src/sub/file
busybox mv -T /tmp/rename-parent/src /tmp/rename-parent/empty
[ "$?" -eq 0 ] && echo TMP_DIR_REPLACE_EMPTY_OK
busybox cat /tmp/rename-parent/empty/sub/file | busybox grep TMP_DIR_REPLACE_PAYLOAD
[ "$?" -eq 0 ] && echo TMP_DIR_REPLACE_SUBTREE_OK
EOF_TMPFS_EXTENDED_6

	send_tmpfs_extended_chunk TMP_DIR_RENAME_DESCENDANT_PRESERVE_OK <<'EOF_TMPFS_EXTENDED_7'
busybox mkdir /tmp/rename-parent/src2
busybox mkdir /tmp/rename-parent/nonempty
echo target > /tmp/rename-parent/nonempty/file
busybox mv -f -T /tmp/rename-parent/src2 /tmp/rename-parent/nonempty || echo TMP_DIR_REPLACE_NONEMPTY_DENY_OK
busybox mkdir /tmp/rename-self
busybox mkdir /tmp/rename-self/child
busybox mv /tmp/rename-self /tmp/rename-self/child/moved || echo TMP_DIR_RENAME_DESCENDANT_DENY_OK
busybox test -d /tmp/rename-self/child
[ "$?" -eq 0 ] && echo TMP_DIR_RENAME_DESCENDANT_PRESERVE_OK
EOF_TMPFS_EXTENDED_7

	send_tmpfs_extended_chunk TMP_NAME256_DENY_OK <<'EOF_TMPFS_EXTENDED_8'
busybox chmod 700 /tmp/bunix-write.txt
[ "$?" -eq 0 ] && echo TMP_CHMOD_OK
busybox stat -c "%a" /tmp/bunix-write.txt
busybox chown root:root /tmp/bunix-write.txt || echo TMP_CHOWN_DENY_OK
NAME255=
i=0
while [ "$i" -lt 255 ]; do NAME255="${NAME255}a"; i=$((i + 1)); done
echo TMP_NAME255_PAYLOAD > "/tmp/$NAME255"
[ "$?" -eq 0 ] && echo TMP_NAME255_CREATE_OK
busybox cat "/tmp/$NAME255"
[ "$?" -eq 0 ] && echo TMP_NAME255_CAT_OK
NAME256="${NAME255}b"
echo TMP_NAME256_PAYLOAD > "/tmp/$NAME256" || echo TMP_NAME256_DENY_OK
EOF_TMPFS_EXTENDED_8

	send_tmpfs_extended_chunk TMP_SYMLINK_FOLLOW_OK <<'EOF_TMPFS_EXTENDED_9'
echo TMP_SYMLINK_PAYLOAD > /tmp/bunix-symlink-target.txt
busybox ln -s bunix-symlink-target.txt /tmp/bunix-symlink.txt
[ "$?" -eq 0 ] && echo TMP_SYMLINK_CREATE_OK
busybox readlink /tmp/bunix-symlink.txt | busybox grep bunix-symlink-target.txt
[ "$?" -eq 0 ] && echo TMP_SYMLINK_READLINK_OK
busybox ls -l /tmp/bunix-symlink.txt | busybox grep bunix-symlink-target.txt
[ "$?" -eq 0 ] && echo TMP_SYMLINK_LS_OK
busybox cat /tmp/bunix-symlink.txt | busybox grep TMP_SYMLINK_PAYLOAD
[ "$?" -eq 0 ] && echo TMP_SYMLINK_FOLLOW_OK
EOF_TMPFS_EXTENDED_9

	send_tmpfs_extended_chunk TMP_SYMLINK_ABS_FOLLOW_OK <<'EOF_TMPFS_EXTENDED_10'
busybox ln -s /hello.txt /tmp/bunix-abs-symlink.txt && busybox cat /tmp/bunix-abs-symlink.txt | busybox grep "rootfs: module"
[ "$?" -eq 0 ] && echo TMP_SYMLINK_ABS_FOLLOW_OK
EOF_TMPFS_EXTENDED_10

	send_tmpfs_extended_chunk TMP_SYMLINK_DOTDOT_FOLLOW_OK <<'EOF_TMPFS_EXTENDED_11'
busybox mkdir -p /tmp/bunix-symlink-dir/sub
echo TMP_SYMLINK_DOTDOT_PAYLOAD > /tmp/bunix-symlink-dir/target.txt
busybox ln -s ../target.txt /tmp/bunix-symlink-dir/sub/up.txt && busybox cat /tmp/bunix-symlink-dir/sub/up.txt | busybox grep TMP_SYMLINK_DOTDOT_PAYLOAD
[ "$?" -eq 0 ] && echo TMP_SYMLINK_DOTDOT_FOLLOW_OK
EOF_TMPFS_EXTENDED_11

	send_tmpfs_extended_chunk TMP_SYMLINK_CROSS_MOUNT_OK <<'EOF_TMPFS_EXTENDED_12'
busybox ln -s /proc/cmdline /tmp/bunix-proc-symlink.txt && busybox cat /tmp/bunix-proc-symlink.txt | busybox grep "init=/sbin/init"
[ "$?" -eq 0 ] && echo TMP_SYMLINK_CROSS_MOUNT_OK
EOF_TMPFS_EXTENDED_12

	send_tmpfs_extended_chunk TMP_SYMLINK_LOOP_OK <<'EOF_TMPFS_EXTENDED_13'
busybox ln -s bunix-loop-b.txt /tmp/bunix-loop-a.txt
busybox ln -s bunix-loop-a.txt /tmp/bunix-loop-b.txt
/bin/patherrtest
[ "$?" -eq 0 ] && echo TMP_SYMLINK_LOOP_OK
EOF_TMPFS_EXTENDED_13

	send_tmpfs_extended_chunk UNION_SYMLINK_FOLLOW_OK <<'EOF_TMPFS_EXTENDED_14'
busybox ln -s hello.txt /union-symlink.txt
[ "$?" -eq 0 ] && echo UNION_SYMLINK_CREATE_OK
busybox readlink /union-symlink.txt | busybox grep hello.txt
[ "$?" -eq 0 ] && echo UNION_SYMLINK_READLINK_OK
busybox ls -l /union-symlink.txt | busybox grep hello.txt
[ "$?" -eq 0 ] && echo UNION_SYMLINK_LS_OK
busybox cat /union-symlink.txt | busybox grep "rootfs: module"
[ "$?" -eq 0 ] && echo UNION_SYMLINK_FOLLOW_OK
EOF_TMPFS_EXTENDED_14

	send_tmpfs_extended_chunk UNION_HARDLINK_SHARE_OK <<'EOF_TMPFS_EXTENDED_15'
echo UNION_HARDLINK_ONE > /union-hard-src.txt
busybox ln /union-hard-src.txt /union-hard-dst.txt
[ "$?" -eq 0 ] && echo UNION_HARDLINK_CREATE_OK
echo UNION_HARDLINK_TWO >> /union-hard-dst.txt
busybox cat /union-hard-src.txt | busybox grep UNION_HARDLINK_TWO
[ "$?" -eq 0 ] && echo UNION_HARDLINK_SHARE_OK
EOF_TMPFS_EXTENDED_15
	send_line_delay=$tmpfs_extended_old_line_delay
}

check_tmpfs_extended() {
	check_exact_markers_file "$log" "$script_dir/shell-tests/tmpfs-extended.exact-markers.txt" \
		"tmpfs extended exact marker missing" 75 220
	check_fixed_markers_file "$log" "$script_dir/shell-tests/tmpfs-extended.provisional-markers.txt" \
		"tmpfs extended provisional marker missing" 45 220
	wait_for_each_fixed "$log" "tmpfs extended regression missing" 45 220 \
		TMP_MKDIR_OK TMP_MKDIR_STAT_OK TMP_LONG_READDIR_OK \
		TMP_LONG_NAME_CAT_OK TMP_NAME_MAX_READDIR_OK TMP_NAME_MAX_CAT_OK \
		TMP_TOUCH_CREATE_OK TMP_TOUCH_EXISTING_OK TMP_MKFIFO_OK \
		TMP_FIFO_STAT_OK TMP_HARDLINK_CREATE_OK TMP_HARDLINK_SHARE_OK \
		TMP_HARDLINK_RENAME_OK RMDIR_OK RM_R_DIR_OK \
		TMP_DIR_RENAME_NONEMPTY_OK TMP_DIR_RENAME_SUBTREE_OK \
		TMP_DIR_RENAME_OLD_GONE_OK TMP_DIR_REPLACE_EMPTY_OK \
		TMP_DIR_REPLACE_SUBTREE_OK TMP_DIR_REPLACE_NONEMPTY_DENY_OK \
		TMP_DIR_RENAME_DESCENDANT_DENY_OK TMP_DIR_RENAME_DESCENDANT_PRESERVE_OK \
		TMP_CHMOD_OK TMP_CHOWN_DENY_OK TMP_NAME255_CREATE_OK \
		TMP_NAME255_CAT_OK TMP_NAME256_DENY_OK TMP_SYMLINK_CREATE_OK \
		TMP_SYMLINK_READLINK_OK TMP_SYMLINK_LS_OK TMP_SYMLINK_FOLLOW_OK \
		TMP_SYMLINK_ABS_FOLLOW_OK TMP_SYMLINK_DOTDOT_FOLLOW_OK \
		TMP_SYMLINK_CROSS_MOUNT_OK TMP_SYMLINK_LOOP_OK \
		UNION_SYMLINK_CREATE_OK UNION_SYMLINK_READLINK_OK \
		UNION_SYMLINK_LS_OK UNION_SYMLINK_FOLLOW_OK UNION_HARDLINK_CREATE_OK \
		UNION_HARDLINK_SHARE_OK
	wait_for_each_fixed_count "$log" 2 "tmpfs extended payload missing from file output" 45 220 \
		TMP_LONG_READDIR_PAYLOAD TMP_NAME_MAX_PAYLOAD TMP_NAME255_PAYLOAD
}
