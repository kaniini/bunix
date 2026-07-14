#!/bin/sh

failed=0

count_tasks() {
	set -- $(busybox wc -l < /proc/kthreads)
	echo $(($1 - 1))
}

mem_free_kb() {
	busybox awk '/MemFree:/ { print $2 }' /proc/meminfo
}

echo "BUNIX_OPENRC_FORK_RECLAIM_BEGIN"
rc-status boot || failed=1
rc-service bootmisc status || failed=1
echo "BUNIX_OPENRC_FORK_RECLAIM_BEFORE tasks=$(count_tasks) memfree_kb=$(mem_free_kb)"

for i in 1 2 3; do
	echo "BUNIX_OPENRC_BOOTMISC_RESTART_$i"
	rc-service bootmisc restart || failed=1
	rc-service bootmisc status || failed=1
done

before_tasks=$(count_tasks)
before_mem=$(mem_free_kb)
echo "BUNIX_OPENRC_FORK_STRESS_BEFORE tasks=$before_tasks memfree_kb=$before_mem"

i=0
while [ "$i" -lt 64 ]; do
	busybox sh -c 'exit 0' || failed=1
	i=$((i + 1))
done

sleep 1
after_tasks=$(count_tasks)
after_mem=$(mem_free_kb)
echo "BUNIX_OPENRC_FORK_STRESS_AFTER tasks=$after_tasks memfree_kb=$after_mem"

if [ "$after_tasks" -gt "$((before_tasks + 4))" ]; then
	echo "BUNIX_OPENRC_FORK_TASK_LEAK before=$before_tasks after=$after_tasks"
	failed=1
fi

if dmesg | busybox grep -F "can't fork: Out of memory"; then
	failed=1
fi
if dmesg | busybox grep -F "kernel: vfork failed"; then
	failed=1
fi
if dmesg | busybox grep -F "sched: vma overlap"; then
	failed=1
fi
if dmesg | busybox grep -F "fork syscall failed"; then
	failed=1
fi
if dmesg | busybox grep -F "can't create /var/run/utmp"; then
	failed=1
fi
if dmesg | busybox grep -F "ERROR: bootmisc failed"; then
	failed=1
fi
if dmesg | busybox grep -F "chmod: /var/run/utmp"; then
	failed=1
fi

if [ "$failed" = 0 ]; then
	echo "BUNIX_OPENRC_FORK_RECLAIM_OK"
fi
test "$failed" = 0
