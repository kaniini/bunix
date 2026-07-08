#!/bin/sh

run_scheduler_bench() {
	send_script_sync <<'EOF_SCHEDULER_BENCH'
/bin/schedbench fairness
/bin/schedbench starvation
/bin/schedbench ipc
/bin/schedbench shell
EOF_SCHEDULER_BENCH
}

check_scheduler_bench() {
	wait_for_fixed "$log" "schedbench fairness ok" "scheduler fairness benchmark did not complete" 75 220
	wait_for_fixed "$log" "schedbench counters switches" "scheduler benchmark counters missing" 45 220
	require_no_fixed "$log" "schedbench fairness skew too high" "scheduler fairness skew exceeded threshold" 220
	wait_for_fixed "$log" "schedbench starvation ok" "scheduler starvation benchmark did not complete" 75 220
	wait_for_fixed "$log" "schedbench starvation counters" "scheduler starvation counters missing" 45 220
	require_no_fixed "$log" "schedbench starvation sleeper latency too high" "scheduler starvation latency exceeded threshold" 220
	wait_for_fixed "$log" "schedbench ipc ok" "scheduler IPC benchmark did not complete" 75 220
	wait_for_fixed "$log" "schedbench ipc counters" "scheduler IPC counters missing" 45 220
	require_no_fixed "$log" "schedbench ipc latency too high" "scheduler IPC latency exceeded threshold" 220
	wait_for_fixed "$log" "schedbench shell ok" "scheduler shell latency benchmark did not complete" 75 220
	wait_for_fixed "$log" "schedbench shell counters" "scheduler shell latency counters missing" 45 220
	require_no_fixed "$log" "schedbench shell latency too high" "scheduler shell latency exceeded threshold" 220
	grep -E "^schedbench " "$log" | sed 's/^/scheduler-bench report /'
}
