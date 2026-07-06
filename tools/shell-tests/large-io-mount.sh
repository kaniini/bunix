#!/bin/sh

run_large_io_mount() {
	send_script <<'EOF_LARGE_IO'
busybox head -c 8192 /dev/zero > /tmp/linux-big-write.bin
busybox test "$(busybox stat -c "%s" /tmp/linux-big-write.bin)" = "8192" && echo LINUX_BIG_WRITE_OK
/bin/mmapbig && echo MMAPBIG_OK
/bin/mmaphuge
/bin/readbig && echo READBIG_OK
EOF_LARGE_IO
	wait_for_fixed "$log" "READBIG_OK" "pre-mount shell commands did not drain" 45 220

	send_script <<'EOF_MOUNT_TMPFS'
mount -t tmpfs tmpfs /mnt&&echo MNT_OK
echo MNT_PAYLOAD>/mnt/linux-mount.txt
cat /mnt/linux-mount.txt&&echo MNT_CAT
mount|busybox grep /mnt&&echo MNT_LIST
umount /mnt&&echo UMNT_OK
mount|busybox grep /mnt||echo UMNT_GONE
test ! -e /mnt/linux-mount.txt&&echo UMNT_HIDE
EOF_MOUNT_TMPFS
}

check_large_io_mount() {
	wait_for_each_fixed "$log" "large I/O or mount regression missing" 45 220 \
		LINUX_BIG_WRITE_OK "linux mmapbig ok" MMAPBIG_OK "linux mmaphuge ok" \
		"linux readbig ok" "linux readlinkbig ok" READBIG_OK MNT_OK \
		MNT_PAYLOAD MNT_CAT MNT_LIST UMNT_OK UMNT_GONE UMNT_HIDE
}
