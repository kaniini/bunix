#!/bin/sh

run_root_mount_soak() {
	login_root_if_needed

	send_script_sync <<'EOF_ROOT_MOUNT_SOAK'
busybox sh /bin/root-mount-soak
EOF_ROOT_MOUNT_SOAK
	wait_for_fixed "$log" "ROOT_MOUNT_SOAK_DONE" \
		"root mount soak commands did not drain" 180 260
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
