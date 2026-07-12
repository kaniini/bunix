#!/bin/sh

run_tmpfs_basic_linux_tests() {
	send_script_sync <<'EOF_TMPFS_BASIC_LINUX_TESTS'
echo TMP_WRITE_OK > /tmp/bunix-write.txt
busybox sh -c "set -C; echo TMP_EXCL_BAD > /tmp/bunix-write.txt" || echo TMP_EXCL_DENY_OK
busybox cat /tmp/bunix-write.txt | busybox grep TMP_WRITE_OK
[ "$?" -eq 0 ] && echo TMP_EXCL_PRESERVE_OK
busybox cat /tmp/bunix-write.txt
[ "$?" -eq 0 ] && echo TMP_CAT_OK
busybox stat /tmp/bunix-write.txt
[ "$?" -eq 0 ] && echo TMP_STAT_OK
echo TMP_APPEND_ONE > /tmp/bunix-append.txt
echo TMP_APPEND_TWO >> /tmp/bunix-append.txt
busybox cat /tmp/bunix-append.txt
[ "$?" -eq 0 ] && echo TMP_APPEND_CAT_OK
busybox sh -c "echo RUN_WRITE_OK > /run/bunix-run.txt"
busybox cat /run/bunix-run.txt
[ "$?" -eq 0 ] && echo RUN_CAT_OK
echo TRUNCATE_PAYLOAD > /tmp/bunix-trunc.txt
busybox truncate -s 4 /tmp/bunix-trunc.txt
[ "$?" -eq 0 ] && echo TRUNCATE_OK
busybox cat /tmp/bunix-trunc.txt
[ "$?" -eq 0 ] && echo TRUNCATE_CAT_OK
busybox rm /tmp/bunix-trunc.txt
[ "$?" -eq 0 ] && echo UNLINK_OK
busybox test ! -e /tmp/bunix-trunc.txt
[ "$?" -eq 0 ] && echo UNLINK_GONE_OK
/bin/getdentstest
[ "$?" -eq 0 ] && echo GETDENTS64_OK
/bin/vforkstress
[ "$?" -eq 0 ] && echo VFORKSTRESS_OK
busybox test ! -e /lib/ld.so
[ "$?" -eq 0 ] && echo MUSL_LDSO_CANONICAL_OK
/bin/dyn-hello
[ "$?" -eq 0 ] && echo DYN_HELLO_OK
busybox top -b -n 1 >/dev/null
[ "$?" -eq 0 ] && echo PROC_TOP_OK
busybox ps
[ "$?" -eq 0 ] && echo PROC_PS_OK
busybox free
[ "$?" -eq 0 ] && echo PROC_FREE_OK
busybox mount
[ "$?" -eq 0 ] && echo PROC_MOUNT_OK
/bin/iovtest
[ "$?" -eq 0 ] && echo IOVTEST_OK
/bin/fchmodattest
/bin/waitpgidtest
/bin/execlongtest
/bin/auxidtest
/bin/fcntllocktest
/bin/sysracetest
/bin/schedstress
busybox mkdir -p /tmp/mkdir-p/a/b
[ "$?" -eq 0 ] && echo TMP_MKDIR_P_EXISTING_ROOT_OK
busybox test -d /tmp/mkdir-p/a/b
[ "$?" -eq 0 ] && echo TMP_MKDIR_P_NESTED_OK
busybox mkdir /tmp || echo TMP_MKDIR_EXISTING_ROOT_DENY_OK
busybox mkdir -p /union-mkdir-p/a/b
[ "$?" -eq 0 ] && echo UNION_MKDIR_P_ROOT_OK
busybox test -d /union-mkdir-p/a/b
[ "$?" -eq 0 ] && echo UNION_MKDIR_P_CHILD_OK
busybox mkdir /usr || echo UNION_MKDIR_EXISTING_LOWER_DENY_OK
/bin/execbig
[ "$?" -eq 0 ] && echo EXECBIG_OK
EOF_TMPFS_BASIC_LINUX_TESTS
}

check_tmpfs_basic_linux_tests() {
	check_exact_markers_file "$log" "$script_dir/shell-tests/tmpfs-basic-linux-tests.exact-markers.txt" \
		"tmpfs/basic Linux exact marker missing" 75 220
	check_fixed_markers_file "$log" "$script_dir/shell-tests/tmpfs-basic-linux-tests.content-markers.txt" \
		"tmpfs/basic Linux content marker missing" 45 220
	wait_for_each_fixed_count "$log" 2 "tmpfs append payload missing from file output" 45 220 \
		TMP_APPEND_ONE TMP_APPEND_TWO
	wait_for_fixed "$log" "DYN_HELLO_OK" "dynamic musl hello did not complete" 45 220
	wait_for_fixed "$log" "EXECBIG_OK" "large Linux executable did not complete" 45 220
	wait_for_fixed "$log" "sysrace ok" "Linux syscall race stress test did not complete" 75 220
	wait_for_fixed "$log" "schedstress ok" "scheduler stress test did not complete" 75 220
	require_no_fixed "$log" "top:" "busybox top reported an error" 220
}
