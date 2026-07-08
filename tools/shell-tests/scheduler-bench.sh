#!/bin/sh

run_scheduler_bench() {
	send_script_sync <<'EOF_SCHEDULER_BENCH'
/bin/schedbench fairness
EOF_SCHEDULER_BENCH
}

check_scheduler_bench() {
	wait_for_fixed "$log" "schedbench fairness ok" "scheduler fairness benchmark did not complete" 75 220
	wait_for_fixed "$log" "schedbench counters switches" "scheduler benchmark counters missing" 45 220
	require_no_fixed "$log" "schedbench fairness skew too high" "scheduler fairness skew exceeded threshold" 220
}
