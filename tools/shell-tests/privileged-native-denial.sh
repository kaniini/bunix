#!/bin/sh

run_privileged_native_denial() {
	send_script_sync <<'EOF_PRIVILEGED_NATIVE_DENIAL'
/bin/privtest && echo PRIVTEST_OK
EOF_PRIVILEGED_NATIVE_DENIAL
	wait_for_fixed "$log" "PRIVTEST_OK" \
		"privileged native syscall denial test did not complete" 45 160
}

check_privileged_native_denial() {
	wait_for_each_fixed "$log" "privileged native syscall denial missing" 45 160 \
		"privtest power-null denied" \
		"privtest power-public-handle denied" \
		"privtest pci-bar-null denied" \
		"privtest pci-bar-public-handle denied" \
		"privtest pci-irq-null denied" \
		"privtest pci-irq-public-handle denied" \
		"privtest ok" \
		"PRIVTEST_OK"
}
