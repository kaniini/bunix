#!/bin/sh

run_rootfs_vfs_proc_dev() {
	send_script_sync <<'EOF_ROOTFS_VFS_PROC_DEV'
busybox cat /usr/share/bunix/nested/hello.txt && echo NESTED_CAT_OK
busybox cat /usr/share/bunix/alpine/very/long/rootfs/path/that/exceeds/the/old/two-hundred-fifty-six-byte/rootfs-entry-limit/path-component-that-forces-the-rootfs-image-format-past-the-old-limit/path-component-that-forces-the-rootfs-image-format-past-the-old-limit/with-extra-components/hello.txt && echo LONG_ROOTFS_PATH_OK
/usr/share/bunix/alpine/very/long/rootfs/path/that/exceeds/the/old/two-hundred-fifty-six-byte/linux-execve-path-limit/path-component-that-forces-the-rootfs-image-format-past-the-old-limit/path-component-that-forces-the-rootfs-image-format-past-the-old-limit/with-extra-components/dyn-hello && echo LONG_EXEC_PATH_OK
busybox stat /hello.txt
busybox stat /usr/share
busybox stat /tmp
busybox stat /run
busybox stat /var/tmp
busybox stat /usr/bin/env
busybox ls /
busybox ls /bin
busybox readlink /bin/cat && echo SYMLINK_READLINK_OK
busybox readlink /usr/share/bunix/long-target-link | busybox grep "/usr/share/bunix/alpine/very/long/rootfs/path/that/exceeds/the/old/two-hundred-fifty-six-byte/rootfs-entry-limit/path-component-that-forces-the-rootfs-image-format-past-the-old-limit/path-component-that-forces-the-rootfs-image-format-past-the-old-limit/with-extra-components/hello.txt" && echo LONG_SYMLINK_READLINK_OK
busybox ls -l /bin/cat && echo SYMLINK_LS_OK
busybox ls /usr/share/bunix/nested
busybox stat /bin
cd /tmp
pwd
cd /usr/share/bunix/nested
pwd
busybox ls .
busybox cat ../nested/./hello.txt && echo VFS_DOTDOT_OK
cd /usr/share/bunix/alpine/very/long/rootfs/path/that/exceeds/the/old/two-hundred-fifty-six-byte/rootfs-entry-limit/path-component-that-forces-the-rootfs-image-format-past-the-old-limit/path-component-that-forces-the-rootfs-image-format-past-the-old-limit/with-extra-components
test "$(pwd)" = "/usr/share/bunix/alpine/very/long/rootfs/path/that/exceeds/the/old/two-hundred-fifty-six-byte/rootfs-entry-limit/path-component-that-forces-the-rootfs-image-format-past-the-old-limit/path-component-that-forces-the-rootfs-image-format-past-the-old-limit/with-extra-components" && echo LONG_GETCWD_OK
cd /
busybox cat //usr///share/bunix/nested/hello.txt && echo VFS_SLASH_OK
busybox cat /proc/kthreads >/dev/null && echo PROCFS_STILL_OK
busybox cat /proc/self/status && echo PROC_STATUS_OK
busybox ls /proc/self/fd && echo PROC_FD_OK
busybox readlink /proc/self/exe && echo PROC_EXE_OK
busybox cat /proc/stat && echo PROC_STAT_OK
busybox cat /proc/ipc && echo PROC_IPC_OK
busybox cat /proc/sched && echo PROC_SCHED_OK
busybox cat /proc/filesystems && echo PROC_FILESYSTEMS_OK
busybox cat /proc/cpuinfo && echo PROC_CPUINFO_OK
busybox cat /proc/cmdline > /tmp/proc-cmdline && busybox grep "init=/sbin/init" /tmp/proc-cmdline && echo PROC_CMDLINE_GLOBAL_OK
busybox cat /proc/devices > /tmp/proc-devices && busybox grep "Character devices:" /tmp/proc-devices && echo PROC_DEVICES_OK
busybox cat /proc/modules >/dev/null && echo PROC_MODULES_OK
busybox cat /proc/self/cmdline && echo PROC_CMDLINE_OK
/bin/cat /proc/self/cmdline > /tmp/proc-self-cmdline && busybox grep -a /bin/cat /tmp/proc-self-cmdline && echo PROC_SELF_CMDLINE_CALLER_OK
busybox cat /proc/self/mounts > /tmp/proc-self-mounts && busybox grep "sysfs /sys sysfs" /tmp/proc-self-mounts && echo PROC_SELF_MOUNTS_OK
busybox cat /proc/self/mountinfo > /tmp/proc-self-mountinfo && busybox grep " - sysfs sysfs " /tmp/proc-self-mountinfo && echo PROC_PID_MOUNTINFO_OK
busybox cat /proc/self/cgroup > /tmp/proc-self-cgroup && busybox grep "0::/" /tmp/proc-self-cgroup && echo PROC_PID_CGROUP_OK
busybox dmesg > /tmp/dmesg-prefix && busybox grep -E '^\[[0-9][0-9]*\.[0-9][0-9][0-9][0-9][0-9][0-9]\] kernel:' /tmp/dmesg-prefix >/dev/null && echo DMESG_PREFIX_OK
busybox test -d /sys && echo SYS_DIR_OK
busybox test -d /sys/class && echo SYS_CLASS_OK
busybox test -d /sys/class/tty && echo SYS_CLASS_TTY_OK
busybox ls /sys/class/tty && echo SYS_CLASS_TTY_LS_OK
busybox test -d /sys/devices/system/cpu && echo SYS_CPU_DIR_OK
busybox cat /sys/devices/system/cpu/online && echo SYS_CPU_ONLINE_OK
busybox cat /proc/mounts > /tmp/proc-mounts && busybox grep "sysfs /sys sysfs" /tmp/proc-mounts && echo PROC_MOUNTS_SYSFS_OK
busybox grep "tmpfs /run tmpfs" /tmp/proc-mounts && echo PROC_MOUNTS_RUN_OK
busybox grep "tmpfs /tmp tmpfs" /tmp/proc-mounts && echo PROC_MOUNTS_TMP_OK
busybox cat /proc/filesystems > /tmp/proc-filesystems && { busybox grep -q cgroup /tmp/proc-filesystems || echo PROC_NO_CGROUP_OK; }
busybox test -s /tmp/proc-filesystems && { busybox grep -q binfmt_misc /tmp/proc-filesystems || echo PROC_NO_BINFMT_OK; }
busybox test -r /proc/mounts && busybox test -x /proc && echo PROC_PERMS_OK
busybox test -r /sys/devices/system/cpu/online && busybox test -x /sys && echo SYS_PERMS_OK
busybox test -w /run && busybox sh -c 'echo run-ok > /run/openrc-check && cat /run/openrc-check' && echo RUN_TMPFS_WRITE_OK
busybox test -w /tmp && busybox sh -c 'echo tmp-ok > /tmp/openrc-check && cat /tmp/openrc-check' && echo TMP_TMPFS_WRITE_OK
busybox test -w /var/tmp && busybox sh -c 'echo vartmp-ok > /var/tmp/openrc-check && cat /var/tmp/openrc-check' && echo VARTMP_TMPFS_WRITE_OK
busybox stat /dev/zero && echo DEV_ZERO_STAT_OK
busybox stat /dev/urandom && echo DEV_URANDOM_STAT_OK
busybox test -r /dev/zero && echo DEV_ZERO_ACCESS_OK
busybox head -c 4 /dev/zero >/dev/null && echo DEV_ZERO_READ_OK
busybox head -c 4 /dev/urandom >/dev/null && echo DEV_URANDOM_READ_OK
busybox test -c /dev/null && echo DEV_NULL_CHAR_OK
busybox test -c /dev/zero && echo DEV_ZERO_CHAR_OK
busybox test -c /dev/console && echo DEV_CONSOLE_CHAR_OK
busybox test -c /dev/ttyS0 && echo DEV_TTYS0_CHAR_OK
busybox sh -c 'printf "RAW_CONSOLE_UNPREFIXED_OK\n" > /dev/console' && echo DEV_CONSOLE_RAW_WRITE_OK
busybox sh -c 'i=0; while [ "$i" -lt 300 ]; do printf a; i=$((i + 1)); done; echo DEV_CONSOLE_BIG_END' > /dev/console && echo DEV_CONSOLE_BIG_WRITE_OK
EOF_ROOTFS_VFS_PROC_DEV
	wait_for_fixed "$log" "DEV_CONSOLE_BIG_WRITE_OK" \
		"rootfs/vfs/proc/dev commands did not drain" 160 220
}

check_rootfs_vfs_proc_dev() {
	check_exact_markers_file "$log" "$script_dir/shell-tests/rootfs-vfs-proc-dev.exact-markers.txt" \
		"rootfs/vfs/proc/dev exact marker missing" 75 220
	check_fixed_markers_file "$log" "$script_dir/shell-tests/rootfs-vfs-proc-dev.content-markers.txt" \
		"rootfs/vfs/proc/dev content marker missing" 45 220
	check_fixed_markers_file "$log" "$script_dir/shell-tests/rootfs-vfs-proc-dev.provisional-markers.txt" \
		"rootfs/vfs/proc/dev provisional marker missing" 45 220
	wait_for_fixed "$log" "Size: 15" "busybox stat did not report /hello.txt size" 45 120
	require_no_fixed "$log" "can't stat '/hello.txt'" "busybox stat failed for /hello.txt" 120

	wait_for_each_fixed "$log" "busybox directory regression missing" 45 160 \
		"hello.txt" "secret.txt" "musl-hello" "busybox" "cat" "stat" \
		"File: /bin" "/bin" "File: /usr/share" "/usr/share" \
		"File: /tmp" "/tmp" "File: /run" "/run" "File: /var/tmp" \
		"/var/tmp" "File: '/usr/bin/env'" "/usr/bin/env" "nested" \
		SYMLINK_READLINK_OK LONG_SYMLINK_READLINK_OK SYMLINK_LS_OK lrwxrwxrwx
	require_no_fixed "$log" "can't stat '/bin'" "busybox directory stat failed" 160
	require_no_fixed "$log" "can't stat 'busybox'" "busybox directory stat failed" 160

	wait_for_fixed "$log" "NESTED_CAT_OK" "busybox nested cat did not complete" 45 180
	wait_for_fixed "$log" "LONG_ROOTFS_PATH_OK" "busybox cat did not read long rootfs path" 45 180
	wait_for_fixed "$log" "LONG_EXEC_PATH_OK" "long Linux execve path did not run" 45 180
	wait_for_fixed "$log" "proc long cmdline ok" "long proc cmdline regression failed" 45 180
	wait_for_fixed "$log" "LONG_GETCWD_OK" "busybox long getcwd did not complete" 45 180
	wait_for_fixed_count "$log" "nested rootfs file" 2 "VFS did not resolve relative dot/dotdot path" 45 180
	wait_for_exact_line "$log" "/tmp" "shell did not cd to /tmp" 45 180
	wait_for_fixed_count "$log" "nested rootfs file" 3 "VFS did not resolve repeated slash path" 45 180
	wait_for_fixed "$log" "PROCFS_STILL_OK" "procfs translator did not survive nested path tests" 45 180
	wait_for_fixed "$log" "PROC_SELF_CMDLINE_CALLER_OK" "procfs self cmdline did not resolve caller" 45 180

	wait_for_each_fixed "$log" "devfs character-device regression missing" 45 180 \
		DEV_NULL_CHAR_OK DEV_ZERO_CHAR_OK DEV_CONSOLE_CHAR_OK \
		DEV_TTYS0_CHAR_OK DEV_CONSOLE_RAW_WRITE_OK
	wait_for_each_fixed "$log" "procfs content regression missing" 45 220 \
		"cpu  " "busybox" "direct_delivered " "direct_handoff " \
		PROC_SCHED_OK "switches " "runq_load "
	wait_for_each_fixed "$log" "openrc procfs surface missing" 45 220 \
		PROC_CMDLINE_GLOBAL_OK PROC_DEVICES_OK PROC_MODULES_OK \
		PROC_SELF_MOUNTS_OK PROC_PID_MOUNTINFO_OK PROC_PID_CGROUP_OK \
		DMESG_PREFIX_OK
	wait_for_each_fixed "$log" "sysfs regression missing" 45 220 \
		SYS_DIR_OK SYS_CLASS_OK SYS_CLASS_TTY_OK SYS_CLASS_TTY_LS_OK \
		SYS_CPU_DIR_OK SYS_CPU_ONLINE_OK PROC_MOUNTS_SYSFS_OK \
		"ttyS0" "0-1"
	wait_for_each_fixed "$log" "openrc mount/permission surface missing" 45 220 \
		PROC_MOUNTS_RUN_OK PROC_MOUNTS_TMP_OK PROC_NO_CGROUP_OK \
		PROC_NO_BINFMT_OK PROC_PERMS_OK SYS_PERMS_OK \
		RUN_TMPFS_WRITE_OK TMP_TMPFS_WRITE_OK VARTMP_TMPFS_WRITE_OK \
		run-ok tmp-ok vartmp-ok
	wait_for_each_regex "$log" "IPC fast path counter did not increase" 45 220 \
		"direct_delivered [1-9][0-9]*" "direct_handoff [1-9][0-9]*"
	wait_for_each_regex "$log" "IPC per-CPU counter did not increase" 45 220 \
		"cpu[0-9][0-9]* sends [1-9][0-9]*"
}
