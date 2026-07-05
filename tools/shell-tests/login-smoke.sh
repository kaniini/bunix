#!/bin/sh

run_login_smoke() {
	send_script <<'EOF_LOGIN_SMOKE'
uptime
busybox uptime
busybox uname
busybox uname -r
busybox stty -a
busybox id
env
/usr/bin/env >/dev/null && echo USR_ENV_OK
busybox test -x /bin/sh && echo ACCESS_X_OK
busybox test -r /hello.txt && echo ACCESS_R_OK
busybox test ! -r /secret.txt && echo ACCESS_DENY_OK
cd ~
pwd
cd /
busybox kill -0 $$ && echo KILL_ZERO_OK
busybox kill -0 -$$ && echo KILL_PGRP_OK
trap "" INT
busybox kill -INT $$
echo SIGINT_IGNORE_OK
trap - INT
busybox sleep 1 && echo BUSYBOX_SLEEP_OK
sleep 1 && echo DIRECT_SLEEP_OK
busybox sleep 5
EOF_LOGIN_SMOKE
	send_bytes '\003echo SLEEP_CTRL_C_OK\n'
}

check_login_smoke() {
	wait_for_fixed "$log" "load average" "busybox uptime did not run through applet argv" 45 120
	wait_for_fixed "$log" " 1 users" "busybox uptime did not report login session" 45 160
	wait_for_fixed "$log" "Bunix" "busybox uname did not report Bunix" 45 120
	wait_for_fixed "$log" "0.1" "busybox uname -r did not report 0.1" 45 120
	wait_for_fixed "$log" "uid=1000" "busybox id did not report login identity" 45 120
	wait_for_awk "$log" '{ sub(/\r$/, "") } /groups=1(\(wheel\))?,1000(\(kaniini\))?/ { found = 1 } END { exit found ? 0 : 1 }' "busybox id did not report login supplementary groups" 45 120
	wait_for_awk "$log" '{ sub(/\r$/, "") } /2018(\(g18\))?/ { found = 1 } END { exit found ? 0 : 1 }' "busybox id did not report more than 16 supplementary groups" 45 120
	wait_for_each_fixed "$log" "login environment missing" 45 160 \
		"HOME=/home/kaniini" "USER=kaniini" "LOGNAME=kaniini" \
		"SHELL=/bin/sh" "PATH=/bin:/sbin:/usr/bin:/usr/sbin" "TERM=bunix"
	wait_for_exact_line "$log" "/home/kaniini" "shell did not cd to login home directory" 45 160
	wait_for_fixed "$log" "USR_ENV_OK" "/usr/bin/env symlink did not execute" 45 160
	wait_for_each_fixed "$log" "linux access/faccessat regression missing" 45 160 \
		ACCESS_X_OK ACCESS_R_OK ACCESS_DENY_OK
	wait_for_fixed "$log" "erase = ^?" "busybox stty did not report tty erase character" 45 160

	wait_for_fixed "$log" "BUSYBOX_SLEEP_OK" "busybox sleep did not complete" 45 160
	wait_for_fixed "$log" "DIRECT_SLEEP_OK" "/bin/sleep did not complete" 45 160
	wait_for_fixed "$log" "SLEEP_CTRL_C_OK" "foreground Ctrl-C did not interrupt busybox sleep" 45 180
	wait_for_fixed "$log" "KILL_ZERO_OK" "busybox kill -0 did not report current shell" 45 160
	wait_for_fixed "$log" "KILL_PGRP_OK" "busybox kill -0 process group probe failed" 45 160
	wait_for_fixed "$log" "SIGINT_IGNORE_OK" "ignored SIGINT terminated the shell" 45 180
}
