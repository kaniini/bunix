#!/bin/sh

run_root_mount_soak() {
	login_root_if_needed

	send_script <<'EOF_ROOT_MOUNT_SOAK'
i=0
while [ "$i" -lt 8 ]; do
	target="/mnt/soak-$i"
	busybox mkdir "$target"
	busybox mount -t tmpfs tmpfs "$target" && echo ROOT_MOUNT_SOAK_MOUNT_OK
	echo "ROOT_MOUNT_SOAK_PAYLOAD_$i" > "$target/soak.txt"
	busybox cat "$target/soak.txt"
	busybox mount | busybox grep "$target" >/dev/null && echo ROOT_MOUNT_SOAK_LIST_OK
	busybox umount "$target" && echo ROOT_MOUNT_SOAK_UMOUNT_OK
	busybox test ! -e "$target/soak.txt" && echo ROOT_MOUNT_SOAK_HIDE_OK
	i=$((i + 1))
done
busybox mkdir /mnt/pinned-mount
busybox mount -t tmpfs tmpfs /mnt/pinned-mount && echo ROOT_MOUNT_SOAK_BUSY_MOUNT_OK
echo ROOT_MOUNT_SOAK_PINNED_PAYLOAD > /mnt/pinned-mount/pinned.txt
busybox sh -c 'cd /mnt/pinned-mount && busybox umount /mnt/pinned-mount || echo ROOT_MOUNT_SOAK_BUSY_OK'
busybox cat /mnt/pinned-mount/pinned.txt
busybox umount /mnt/pinned-mount && echo ROOT_MOUNT_SOAK_FINAL_UMOUNT_OK
busybox test ! -e /mnt/pinned-mount/pinned.txt && echo ROOT_MOUNT_SOAK_FINAL_HIDE_OK
echo ROOT_MOUNT_SOAK_DONE
EOF_ROOT_MOUNT_SOAK
}

check_root_mount_soak() {
	wait_for_each_fixed_count "$log" 8 "root mount soak repeated marker missing" 120 260 \
		ROOT_MOUNT_SOAK_MOUNT_OK ROOT_MOUNT_SOAK_LIST_OK \
		ROOT_MOUNT_SOAK_UMOUNT_OK ROOT_MOUNT_SOAK_HIDE_OK
	wait_for_each_fixed "$log" "root mount soak final marker missing" 45 260 \
		ROOT_MOUNT_SOAK_PAYLOAD_0 ROOT_MOUNT_SOAK_PAYLOAD_7 \
		ROOT_MOUNT_SOAK_BUSY_MOUNT_OK ROOT_MOUNT_SOAK_BUSY_OK \
		ROOT_MOUNT_SOAK_PINNED_PAYLOAD ROOT_MOUNT_SOAK_FINAL_UMOUNT_OK \
		ROOT_MOUNT_SOAK_FINAL_HIDE_OK ROOT_MOUNT_SOAK_DONE
}
