#!/bin/sh

run_scheduler_bench() {
	send_script_sync <<'EOF_SCHEDULER_BENCH'
/bin/schedbench fairness
/bin/schedbench starvation
/bin/schedbench ipc
/bin/schedbench shell
/bin/schedbench affinity
EOF_SCHEDULER_BENCH
}

run_scheduler_migration() {
	send_script_sync <<'EOF_SCHEDULER_MIGRATION'
/bin/schedbench migration
EOF_SCHEDULER_MIGRATION
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
	wait_for_fixed "$log" "schedbench affinity" "scheduler affinity benchmark did not complete" 45 220
	wait_for_fixed "$log" "mask=1 ok" "scheduler affinity mask check failed" 45 220
	grep -E "^schedbench " "$log" | sed 's/^/scheduler-bench report /'
}

check_scheduler_migration() {
	wait_for_fixed "$log" "schedbench migration ok" "scheduler migration benchmark did not complete" 75 220
	wait_for_fixed "$log" "idle_migrations" "scheduler idle migration counters missing" 45 220
	require_no_fixed "$log" "schedbench migration did not observe idle pull" "scheduler migration did not observe idle-pull behavior" 220
	require_no_fixed "$log" "schedbench migration counters did not advance" "scheduler migration counters did not advance" 220
	require_no_fixed "$log" "schedbench migration deadline delta too high" "scheduler migration deadline skew exceeded threshold" 220
	grep -E "^schedbench migration" "$log" | sed 's/^/scheduler-migration report /'
}
