#!/bin/sh

run_tmpfs_extended() {
	send_script <<'EOF_TMPFS_BASELINE'
busybox stat -c "%u:%g %a" /tmp/bunix-write.txt
busybox mkdir /tmp/bunix-dir && echo TMP_MKDIR_OK
busybox test -d /tmp/bunix-dir && echo TMP_MKDIR_STAT_OK
LONG_TMP_NAME=bunix-readdir-name-longer-than-sixteen.txt
echo TMP_LONG_READDIR_PAYLOAD > "/tmp/$LONG_TMP_NAME"
busybox ls /tmp | busybox grep "$LONG_TMP_NAME" && echo TMP_LONG_READDIR_OK
busybox cat "/tmp/$LONG_TMP_NAME" && echo TMP_LONG_NAME_CAT_OK
EOF_TMPFS_BASELINE
	wait_for_fixed "$log" "TMP_LONG_NAME_CAT_OK" "tmpfs baseline long-name commands did not drain" 240 220

	send_script <<'EOF_TMPFS_EXTENDED'
TMP_NAME_MAX=bunix-tmp-name-aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.txt
echo TMP_NAME_MAX_PAYLOAD > "/tmp/$TMP_NAME_MAX"
busybox ls /tmp | busybox grep "$TMP_NAME_MAX" && echo TMP_NAME_MAX_READDIR_OK
busybox cat "/tmp/$TMP_NAME_MAX" && echo TMP_NAME_MAX_CAT_OK
busybox touch /tmp/bunix-touch.txt && echo TMP_TOUCH_CREATE_OK
busybox touch /tmp/bunix-write.txt && echo TMP_TOUCH_EXISTING_OK
busybox mkfifo /tmp/bunix-fifo && echo TMP_MKFIFO_OK
busybox test -p /tmp/bunix-fifo && echo TMP_FIFO_STAT_OK
echo TMP_LINK_ONE > /tmp/bunix-link-src.txt
busybox ln /tmp/bunix-link-src.txt /tmp/bunix-link-dst.txt && echo TMP_HARDLINK_CREATE_OK
echo TMP_LINK_TWO >> /tmp/bunix-link-dst.txt
busybox cat /tmp/bunix-link-src.txt | busybox grep TMP_LINK_TWO && echo TMP_HARDLINK_SHARE_OK
busybox mv /tmp/bunix-link-src.txt /tmp/bunix-link-renamed.txt
echo TMP_LINK_THREE >> /tmp/bunix-link-renamed.txt
busybox cat /tmp/bunix-link-dst.txt | busybox grep TMP_LINK_THREE && echo TMP_HARDLINK_RENAME_OK
busybox mkdir /tmp/bunix-rmdir
busybox rmdir /tmp/bunix-rmdir
busybox test ! -e /tmp/bunix-rmdir && echo RMDIR_OK
busybox mkdir /tmp/bunix-rm-r-dir
busybox rm -r /tmp/bunix-rm-r-dir && echo RM_R_DIR_OK
busybox rm -rf /tmp/rename-src /tmp/rename-dst /tmp/rename-parent /tmp/rename-self
busybox mkdir /tmp/rename-src
busybox mkdir /tmp/rename-src/sub
echo TMP_DIR_RENAME_PAYLOAD > /tmp/rename-src/sub/file
busybox mv /tmp/rename-src /tmp/rename-dst && echo TMP_DIR_RENAME_NONEMPTY_OK
busybox cat /tmp/rename-dst/sub/file | busybox grep TMP_DIR_RENAME_PAYLOAD && echo TMP_DIR_RENAME_SUBTREE_OK
busybox test ! -e /tmp/rename-src && echo TMP_DIR_RENAME_OLD_GONE_OK
busybox mkdir /tmp/rename-parent
busybox mkdir /tmp/rename-parent/src
busybox mkdir /tmp/rename-parent/src/sub
busybox mkdir /tmp/rename-parent/empty
echo TMP_DIR_REPLACE_PAYLOAD > /tmp/rename-parent/src/sub/file
busybox mv -T /tmp/rename-parent/src /tmp/rename-parent/empty && echo TMP_DIR_REPLACE_EMPTY_OK
busybox cat /tmp/rename-parent/empty/sub/file | busybox grep TMP_DIR_REPLACE_PAYLOAD && echo TMP_DIR_REPLACE_SUBTREE_OK
busybox mkdir /tmp/rename-parent/src2
busybox mkdir /tmp/rename-parent/nonempty
echo target > /tmp/rename-parent/nonempty/file
busybox mv -f -T /tmp/rename-parent/src2 /tmp/rename-parent/nonempty || echo TMP_DIR_REPLACE_NONEMPTY_DENY_OK
busybox mkdir /tmp/rename-self
busybox mkdir /tmp/rename-self/child
busybox mv /tmp/rename-self /tmp/rename-self/child/moved || echo TMP_DIR_RENAME_DESCENDANT_DENY_OK
busybox test -d /tmp/rename-self/child && echo TMP_DIR_RENAME_DESCENDANT_PRESERVE_OK
busybox chmod 700 /tmp/bunix-write.txt && echo TMP_CHMOD_OK
busybox stat -c "%a" /tmp/bunix-write.txt
busybox chown root:root /tmp/bunix-write.txt || echo TMP_CHOWN_DENY_OK
NAME255=
i=0
while [ "$i" -lt 255 ]; do NAME255="${NAME255}a"; i=$((i + 1)); done
echo TMP_NAME255_PAYLOAD > "/tmp/$NAME255" && echo TMP_NAME255_CREATE_OK
busybox cat "/tmp/$NAME255" && echo TMP_NAME255_CAT_OK
NAME256="${NAME255}b"
echo TMP_NAME256_PAYLOAD > "/tmp/$NAME256" || echo TMP_NAME256_DENY_OK
echo TMP_SYMLINK_PAYLOAD > /tmp/bunix-symlink-target.txt
busybox ln -s bunix-symlink-target.txt /tmp/bunix-symlink.txt && echo TMP_SYMLINK_CREATE_OK
busybox readlink /tmp/bunix-symlink.txt | busybox grep bunix-symlink-target.txt && echo TMP_SYMLINK_READLINK_OK
busybox ls -l /tmp/bunix-symlink.txt | busybox grep bunix-symlink-target.txt && echo TMP_SYMLINK_LS_OK
busybox ln -s hello.txt /union-symlink.txt && echo UNION_SYMLINK_CREATE_OK
busybox readlink /union-symlink.txt | busybox grep hello.txt && echo UNION_SYMLINK_READLINK_OK
busybox ls -l /union-symlink.txt | busybox grep hello.txt && echo UNION_SYMLINK_LS_OK
echo UNION_HARDLINK_ONE > /union-hard-src.txt
busybox ln /union-hard-src.txt /union-hard-dst.txt && echo UNION_HARDLINK_CREATE_OK
echo UNION_HARDLINK_TWO >> /union-hard-dst.txt
busybox cat /union-hard-src.txt | busybox grep UNION_HARDLINK_TWO && echo UNION_HARDLINK_SHARE_OK
EOF_TMPFS_EXTENDED
	wait_for_fixed "$log" "TMP_CHOWN_DENY_OK" "tmpfs extended long-name commands did not drain" 45 220
}

check_tmpfs_extended() {
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
		TMP_SYMLINK_READLINK_OK TMP_SYMLINK_LS_OK UNION_SYMLINK_CREATE_OK \
		UNION_SYMLINK_READLINK_OK UNION_SYMLINK_LS_OK UNION_HARDLINK_CREATE_OK \
		UNION_HARDLINK_SHARE_OK
}
