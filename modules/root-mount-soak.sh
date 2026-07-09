#!/bin/sh
set -eu

i=0
while [ "$i" -lt 8 ]; do
	busybox mount -t tmpfs tmpfs /mnt && echo ROOT_MOUNT_SOAK_MOUNT_OK
	echo "ROOT_MOUNT_SOAK_PAYLOAD_$i" > /mnt/soak.txt
	busybox cat /mnt/soak.txt
	busybox mount | busybox grep /mnt >/dev/null && echo ROOT_MOUNT_SOAK_LIST_OK
	busybox umount /mnt && echo ROOT_MOUNT_SOAK_UMOUNT_OK
	busybox test ! -e /mnt/soak.txt && echo ROOT_MOUNT_SOAK_HIDE_OK
	i=$((i + 1))
done

busybox mount -t tmpfs tmpfs /mnt && echo ROOT_MOUNT_SOAK_BUSY_MOUNT_OK
echo ROOT_MOUNT_SOAK_PINNED_PAYLOAD > /mnt/pinned.txt
busybox sh -c 'cd /mnt && busybox umount /mnt || echo ROOT_MOUNT_SOAK_BUSY_OK'
busybox cat /mnt/pinned.txt
busybox umount /mnt && echo ROOT_MOUNT_SOAK_FINAL_UMOUNT_OK
busybox test ! -e /mnt/pinned.txt && echo ROOT_MOUNT_SOAK_FINAL_HIDE_OK
echo ROOT_MOUNT_SOAK_DONE
