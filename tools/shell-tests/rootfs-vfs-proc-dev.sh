#!/bin/sh

run_rootfs_vfs_paths() {
	send_script_marker_sync <<'EOF_ROOTFS_VFS_PATHS'
busybox cat /usr/share/bunix/nested/hello.txt && echo NESTED_CAT_OK
cd /usr/share/bunix/alpine/very/long/rootfs/path/that/exceeds
cd the/old/two-hundred-fifty-six-byte/rootfs-entry-limit
cd path-component-that-forces-the-rootfs-image-format-past-the-old-limit
cd path-component-that-forces-the-rootfs-image-format-past-the-old-limit
cd with-extra-components
busybox cat hello.txt && echo LONG_ROOTFS_PATH_OK
cd /usr/share/bunix/alpine/very/long/rootfs/path/that/exceeds
cd the/old/two-hundred-fifty-six-byte/linux-execve-path-limit
cd path-component-that-forces-the-rootfs-image-format-past-the-old-limit
cd path-component-that-forces-the-rootfs-image-format-past-the-old-limit
cd with-extra-components
P="$(pwd)/dyn-hello"
$P && echo LONG_EXEC_PATH_OK
cd /
busybox stat /hello.txt
busybox stat /usr/share
busybox stat /tmp
busybox stat /run
busybox stat /var/tmp
busybox stat /usr/bin/env
busybox ls /
busybox ls /bin
busybox readlink /bin/cat && echo SYMLINK_READLINK_OK
busybox readlink /usr/share/bunix/long-target-link > /tmp/long-target-link
busybox grep "with-extra-components/hello.txt" /tmp/long-target-link && echo LONG_SYMLINK_READLINK_OK
busybox ls -l /bin/cat && echo SYMLINK_LS_OK
busybox ls /usr/share/bunix/nested
busybox stat /bin
cd /tmp
pwd
cd /usr/share/bunix/nested
pwd
busybox ls .
busybox cat ../nested/./hello.txt && echo VFS_DOTDOT_OK
cd /usr/share/bunix/alpine/very/long/rootfs/path/that/exceeds
cd the/old/two-hundred-fifty-six-byte/rootfs-entry-limit
cd path-component-that-forces-the-rootfs-image-format-past-the-old-limit
cd path-component-that-forces-the-rootfs-image-format-past-the-old-limit
cd with-extra-components
busybox pwd | busybox grep "with-extra-components" && echo LONG_GETCWD_OK
cd /
busybox cat //usr///share/bunix/nested/hello.txt && echo VFS_SLASH_OK
busybox cat /proc/kthreads >/dev/null && echo PROCFS_STILL_OK
EOF_ROOTFS_VFS_PATHS
	wait_for_fixed "$log" "PROCFS_STILL_OK" \
		"rootfs/vfs path commands did not drain" 120 220
}

check_rootfs_vfs_paths() {
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
}

run_procfs_sysfs_surface() {
	send_script_marker_sync <<'EOF_PROCFS_SYSFS_SURFACE'
busybox cat /proc/self/status && echo PROC_STATUS_OK
busybox ls /proc/self/fd && echo PROC_FD_OK
busybox readlink /proc/self/exe && echo PROC_EXE_OK
busybox cat /proc/stat && echo PROC_STAT_OK
busybox cat /proc/ipc && echo PROC_IPC_OK
busybox cat /proc/sched && echo PROC_SCHED_OK
busybox cat /proc/sched_threads && echo PROC_SCHED_THREADS_OK
busybox cat /proc/filesystems && echo PROC_FILESYSTEMS_OK
busybox cat /proc/cpuinfo && echo PROC_CPUINFO_OK
busybox cat /proc/cmdline > /tmp/proc-cmdline
busybox grep "init=/sbin/init" /tmp/proc-cmdline && echo PROC_CMDLINE_GLOBAL_OK
busybox cat /proc/devices > /tmp/proc-devices
busybox grep "Character devices:" /tmp/proc-devices && echo PROC_DEVICES_OK
busybox cat /proc/modules >/dev/null && echo PROC_MODULES_OK
busybox cat /proc/self/cmdline && echo PROC_CMDLINE_OK
busybox cat /proc/self/cmdline > /tmp/proc-self-cmdline
busybox cat /proc/self/mounts > /tmp/proc-self-mounts
busybox grep sysfs /tmp/proc-self-mounts && echo PROC_SELF_MOUNTS_OK
busybox cat /proc/self/mountinfo > /tmp/proc-self-mountinfo
busybox grep sysfs /tmp/proc-self-mountinfo && echo PROC_PID_MOUNTINFO_OK
busybox cat /proc/self/cgroup > /tmp/proc-self-cgroup
busybox grep 0::/ /tmp/proc-self-cgroup && echo PROC_PID_CGROUP_OK
busybox dmesg > /tmp/dmesg-prefix
busybox grep -E '^\[[0-9]' /tmp/dmesg-prefix >/dev/null && echo DMESG_PREFIX_OK
busybox test -d /sys && echo SYS_DIR_OK
busybox test -d /sys/class && echo SYS_CLASS_OK
busybox test -d /sys/class/tty && echo SYS_CLASS_TTY_OK
busybox ls /sys/class/tty && echo SYS_CLASS_TTY_LS_OK
busybox test -d /sys/devices/system/cpu && echo SYS_CPU_DIR_OK
busybox cat /sys/devices/system/cpu/online && echo SYS_CPU_ONLINE_OK
busybox cat /proc/mounts > /tmp/proc-mounts
busybox grep sysfs /tmp/proc-mounts && echo PROC_MOUNTS_SYSFS_OK
busybox grep /run /tmp/proc-mounts && echo PROC_MOUNTS_RUN_OK
busybox grep /tmp /tmp/proc-mounts && echo PROC_MOUNTS_TMP_OK
busybox cat /proc/filesystems > /tmp/proc-filesystems
busybox grep -q cgroup /tmp/proc-filesystems || echo PROC_NO_CGROUP_OK
cd /tmp
busybox grep -q binfmt_misc proc-filesystems || echo PROC_NO_BINFMT_OK
cd /
busybox test -r /proc/mounts && busybox test -x /proc && echo PROC_PERMS_OK
busybox test -r /sys/devices/system/cpu/online && busybox test -x /sys && echo SYS_PERMS_OK
busybox test -w /run
busybox sh -c 'echo run-ok > /run/openrc-check'
busybox cat /run/openrc-check && echo RUN_TMPFS_WRITE_OK
busybox test -w /tmp
busybox sh -c 'echo tmp-ok > /tmp/openrc-check'
busybox cat /tmp/openrc-check && echo TMP_TMPFS_WRITE_OK
busybox test -w /var/tmp
busybox sh -c 'echo vartmp-ok > /var/tmp/openrc-check'
busybox cat /var/tmp/openrc-check && echo VARTMP_TMPFS_WRITE_OK
EOF_PROCFS_SYSFS_SURFACE
	wait_for_fixed "$log" "VARTMP_TMPFS_WRITE_OK" \
		"procfs/sysfs surface commands did not drain" 120 220
}

check_procfs_sysfs_surface() {
	wait_for_each_fixed "$log" "procfs/sysfs exact marker missing" 75 220 \
		PROC_STATUS_OK PROC_FD_OK PROC_EXE_OK PROC_STAT_OK \
		PROC_IPC_OK PROC_SCHED_THREADS_OK PROC_FILESYSTEMS_OK \
		PROC_CPUINFO_OK PROC_CMDLINE_GLOBAL_OK PROC_DEVICES_OK \
		PROC_MODULES_OK PROC_SELF_MOUNTS_OK PROC_PID_MOUNTINFO_OK \
		PROC_PID_CGROUP_OK SYS_DIR_OK SYS_CLASS_OK SYS_CLASS_TTY_OK \
		SYS_CLASS_TTY_LS_OK SYS_CPU_DIR_OK SYS_CPU_ONLINE_OK \
		PROC_MOUNTS_SYSFS_OK PROC_MOUNTS_RUN_OK PROC_MOUNTS_TMP_OK \
		PROC_NO_CGROUP_OK PROC_NO_BINFMT_OK PROC_PERMS_OK SYS_PERMS_OK \
		RUN_TMPFS_WRITE_OK TMP_TMPFS_WRITE_OK VARTMP_TMPFS_WRITE_OK \
		PROC_CMDLINE_OK
	wait_for_each_fixed "$log" "procfs/sysfs content marker missing" 45 220 \
		nodev "Bunix virtual CPU"
	wait_for_each_fixed "$log" "procfs content regression missing" 45 220 \
		"cpu  " "busybox" "direct_delivered " "direct_handoff " \
		PROC_SCHED_OK PROC_SCHED_THREADS_OK "switches " "runq_load " \
		"task tid state cpu class priority weight runtime"
	wait_for_each_fixed "$log" "openrc procfs surface missing" 45 220 \
		PROC_CMDLINE_GLOBAL_OK PROC_DEVICES_OK PROC_MODULES_OK \
		PROC_SELF_MOUNTS_OK PROC_PID_MOUNTINFO_OK PROC_PID_CGROUP_OK \
		DMESG_PREFIX_OK
	wait_for_each_fixed "$log" "sysfs regression missing" 45 220 \
		SYS_DIR_OK SYS_CLASS_OK SYS_CLASS_TTY_OK SYS_CLASS_TTY_LS_OK \
		SYS_CPU_DIR_OK SYS_CPU_ONLINE_OK PROC_MOUNTS_SYSFS_OK \
		console "0-1"
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

run_devfs_console_surface() {
	send_script_marker_sync <<'EOF_DEVFS_CONSOLE_SURFACE'
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
busybox yes a | busybox head -c 300 > /dev/console
echo DEV_CONSOLE_BIG_END > /dev/console
echo DEV_CONSOLE_BIG_WRITE_OK
EOF_DEVFS_CONSOLE_SURFACE
	wait_for_fixed "$log" "DEV_CONSOLE_BIG_WRITE_OK" \
		"devfs/console commands did not drain" 90 220
}

check_devfs_console_surface() {
	wait_for_each_fixed "$log" "devfs character-device regression missing" 45 180 \
		DEV_NULL_CHAR_OK DEV_ZERO_CHAR_OK DEV_CONSOLE_CHAR_OK \
		DEV_TTYS0_CHAR_OK DEV_CONSOLE_RAW_WRITE_OK \
		DEV_ZERO_STAT_OK DEV_URANDOM_STAT_OK DEV_ZERO_ACCESS_OK \
		DEV_ZERO_READ_OK DEV_URANDOM_READ_OK RAW_CONSOLE_UNPREFIXED_OK
	wait_for_each_fixed "$log" "devfs console regression missing" 45 180 \
		DEV_CONSOLE_BIG_END DEV_CONSOLE_BIG_WRITE_OK
}

run_rootfs_vfs_proc_dev() {
	run_rootfs_vfs_paths
	run_procfs_sysfs_surface
	run_devfs_console_surface
}

check_rootfs_vfs_proc_dev() {
	check_rootfs_vfs_paths
	check_procfs_sysfs_surface
	check_devfs_console_surface
}
