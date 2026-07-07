#!/bin/sh
set -eu

qemu=${QEMU:-qemu-system-x86_64}
ovmf=${OVMF_CODE:-/usr/share/OVMF/OVMF_CODE.fd}
esp=${ESP_DIR:-build/esp}
timeout_cmd=${TIMEOUT:-timeout}
qemu_timeout=${QEMU_TIMEOUT:-120s}
qemu_memory=${QEMU_MEMORY:-128M}
qemu_extra_args=${QEMU_EXTRA_ARGS:-}
sidecar_cmd=${BUNIX_TEST_SIDECAR_CMD:-}
sidecar_ready=${BUNIX_TEST_SIDECAR_READY_FILE:-}
sidecar_start_delay=${BUNIX_TEST_SIDECAR_START_DELAY:-1}
sidecar_ready_timeout=${BUNIX_TEST_SIDECAR_READY_TIMEOUT:-20}
run_id=${BUNIX_TEST_RUN_ID:-$(date -u +%Y%m%dT%H%M%SZ)-$$}
tmp=${BUNIX_TEST_RUNTIME_DIR:-${TMPDIR:-/tmp}/bunix-command-test.$run_id}
log=$tmp/serial.log
qemu_log=$tmp/qemu.log
sidecar_log=$tmp/sidecar.log
pipe=$tmp/serial
runtime_esp=$tmp/esp
failure_dir=${FAILURE_DIR:-build/failures/$run_id}
script_dir=$(CDPATH= cd "$(dirname "$0")" && pwd)
. "$script_dir/test-lib.sh"
BUNIX_COLLECT_FAILURES=1
BUNIX_FAILURE_DIR=$failure_dir
BUNIX_QEMU_LOG=$qemu_log
BUNIX_TEST_SIDECAR_LOG=$sidecar_log
BUNIX_TEST_HARNESS=$0
BUNIX_TEST_RUNTIME_DIR=$tmp
export BUNIX_COLLECT_FAILURES BUNIX_FAILURE_DIR BUNIX_QEMU_LOG BUNIX_TEST_SIDECAR_LOG BUNIX_TEST_HARNESS BUNIX_TEST_RUNTIME_DIR

command_text=${BUNIX_CMD:-${CMD:-}}
command_file=${BUNIX_CMD_FILE:-}
marker=${BUNIX_MARKER:-__BUNIX_COMMAND_OK__}
expect_qemu_exit=${BUNIX_EXPECT_QEMU_EXIT:-0}
user=${BUNIX_USER:-kaniini}
password=${BUNIX_PASSWORD:-bunix}
prompt=${BUNIX_PROMPT:-~ $ }
guest_poweroff=${BUNIX_GUEST_POWEROFF:-1}
login_timeout=${BUNIX_LOGIN_TIMEOUT:-80}
send_delay=${BUNIX_SEND_DELAY:-0.05}

qemu_pid=
cat_pid=
sidecar_pid=
status=0
status_injected=0

usage() {
	echo "usage: BUNIX_CMD='command' $0" >&2
	echo "       BUNIX_CMD_FILE=script.sh $0" >&2
	exit 2
}

if [ -z "$command_text" ] && [ -z "$command_file" ]; then
	usage
fi

cleanup() {
	if [ "${qemu_pid:-}" ]; then
		kill "$qemu_pid" 2>/dev/null || true
	fi
	if [ "${cat_pid:-}" ]; then
		kill "$cat_pid" 2>/dev/null || true
	fi
	if [ "${sidecar_pid:-}" ]; then
		kill "$sidecar_pid" 2>/dev/null || true
	fi
	if [ "${KEEP_TMP:-0}" != 1 ]; then
		rm -rf "$tmp"
	else
		echo "kept test tmp: $tmp" >&2
	fi
}

collect_guest_failure_probes() {
	if [ "${BUNIX_FAILURE_GUEST_PROBES:-1}" != 1 ]; then
		return
	fi
	if ! kill -0 "$qemu_pid" 2>/dev/null; then
		return
	fi
	{
		printf 'echo __BUNIX_FAILURE_PROBES_BEGIN__\n'
		printf 'echo "--- /proc/net/config ---"\n'
		printf 'cat /proc/net/config 2>&1\n'
		printf 'echo "--- /proc/net/dev ---"\n'
		printf 'cat /proc/net/dev 2>&1\n'
		printf 'echo "--- /proc/net/arp ---"\n'
		printf 'cat /proc/net/arp 2>&1\n'
		printf 'echo "--- networking status ---"\n'
		printf '/etc/init.d/networking status 2>&1\n'
		printf 'echo "--- dmesg tail ---"\n'
		printf 'dmesg 2>&1 | tail -n 80\n'
		printf 'echo __BUNIX_FAILURE_PROBES_END__\n'
	} >&3 2>/dev/null || return
	sleep "${BUNIX_FAILURE_GUEST_PROBE_DELAY:-3}"
}

fail_command() {
	label=$1
	tail_lines=${2:-160}

	collect_guest_failure_probes
	out=$(save_failure_artifacts "$label" "$log" "$qemu_log" "$tail_lines")

	echo "$label" >&2
	echo "failure artifacts: $out" >&2
	cat "$qemu_log" >&2 || true
	tail -n "$tail_lines" "$log" >&2 || true
	exit 1
}

send_script() {
	cat >&3
}

wait_for_qemu_fixed() {
	expected=$1
	label=$2
	limit=${3:-45}
	tail_lines=${4:-120}
	i=0

	while ! grep -aF "$expected" "$log" >/dev/null 2>&1; do
		i=$((i + 1))
		if ! kill -0 "$qemu_pid" 2>/dev/null; then
			fail_command "qemu exited while waiting for: $expected" "$tail_lines"
		fi
		if [ "$i" -gt "$limit" ]; then
			fail_command "$label" "$tail_lines"
		fi
		sleep 1
	done
}

current_prompt_count() {
	prompt_text=$1

	grep -F -c "$prompt_text" "$log" 2>/dev/null || true
}

wait_for_prompt_count_gt() {
	prompt_text=$1
	before=$2
	label=$3
	limit=${4:-45}
	tail_lines=${5:-180}
	i=0

	while [ "$(current_prompt_count "$prompt_text")" -le "$before" ]; do
		i=$((i + 1))
		if ! kill -0 "$qemu_pid" 2>/dev/null; then
			fail_command "qemu exited while waiting for: $prompt_text" "$tail_lines"
		fi
		if [ "$i" -gt "$limit" ]; then
			fail_command "$label" "$tail_lines"
		fi
		sleep 1
	done
}

wait_for_guest_poweroff() {
	if [ "$user" = root ]; then
		:
	else
		wait_for_prompt_count_gt "login: " "$login_prompts_before_exit" \
			"login prompt did not return before guest poweroff" 45 180
		root_prompts_before=$(current_prompt_count "~ # ")
		printf '%s\n%s\n' root root >&3
		wait_for_prompt_count_gt "~ # " "$root_prompts_before" \
			"root shell prompt did not appear for guest poweroff" 45 180
		printf '/bin/busybox poweroff -f\n' >&3
	fi

	if wait "$qemu_pid"; then
		qemu_pid=
		return 0
	fi
	qemu_status=$?
	qemu_pid=
	fail_command "qemu exited with status $qemu_status" 220
}

start_sidecar() {
	if [ -z "$sidecar_cmd" ]; then
		return
	fi
	(
		cd "$PWD"
		sh -c "$sidecar_cmd"
	) >"$sidecar_log" 2>&1 &
	sidecar_pid=$!
	if [ -n "$sidecar_ready" ]; then
		i=0
		while [ ! -e "$sidecar_ready" ]; do
			i=$((i + 1))
			if ! kill -0 "$sidecar_pid" 2>/dev/null; then
				fail_command "sidecar exited before readiness: $sidecar_ready" 120
			fi
			if [ "$i" -gt "$sidecar_ready_timeout" ]; then
				fail_command "sidecar readiness timed out: $sidecar_ready" 120
			fi
			sleep 1
		done
	else
		sleep "$sidecar_start_delay"
		if ! kill -0 "$sidecar_pid" 2>/dev/null; then
			fail_command "sidecar exited before qemu start" 120
		fi
	fi
}

start_qemu() {
	mkdir -p "$tmp"
	: > "$sidecar_log"
	mkfifo "$pipe.in" "$pipe.out"
	trap cleanup EXIT INT TERM

	if [ ! -d "$esp" ]; then
		echo "ESP directory is not readable: $esp" >&2
		exit 1
	fi
	rm -rf "$runtime_esp"
	mkdir -p "$runtime_esp"
	cp -R "$esp/." "$runtime_esp/"

	cat "$pipe.out" > "$log" &
	cat_pid=$!

	start_sidecar
	TMPDIR=$tmp $timeout_cmd "$qemu_timeout" "$qemu" -enable-kvm -machine q35 -cpu host -m "$qemu_memory" \
		-smp "${SMP:-2}" \
		-drive if=pflash,format=raw,readonly=on,file="$ovmf" \
		-drive format=raw,file=fat:rw:"$runtime_esp" \
		-serial pipe:"$pipe" -display none -no-reboot $qemu_extra_args 2>"$qemu_log" &
	qemu_pid=$!
}

start_qemu
wait_for_qemu_fixed "login: " "login prompt did not appear" "$login_timeout" 80

sleep 3
exec 3>"$pipe.in"
printf '%s\n%s\n' "$user" "$password" >&3
wait_for_fixed "$log" "$prompt" "shell prompt did not appear for $user" 150 120
login_prompts_before_exit=$(current_prompt_count "login: ")
sleep "$send_delay"

if [ -n "$command_file" ]; then
	if [ ! -r "$command_file" ]; then
		fail_command "command file is not readable: $command_file" 40
	fi
	while IFS= read -r line || [ -n "$line" ]; do
		printf '%s\n' "$line" >&3
		sleep "$send_delay"
	done < "$command_file"
else
	if [ "$expect_qemu_exit" = 1 ]; then
		printf '%s\n' "$command_text" >&3
	elif [ "$guest_poweroff" = 1 ] && [ "$user" = root ]; then
		printf '%s; status=$?; echo __BUNIX_COMMAND_STATUS__=$status; test "$status" = 0 && echo %s; test "$status" = 0 && /bin/busybox poweroff -f; exit\n' "$command_text" "$marker" >&3
		status_injected=1
	else
		printf '%s; status=$?; echo __BUNIX_COMMAND_STATUS__=$status; test "$status" = 0 && echo %s; exit\n' "$command_text" "$marker" >&3
		status_injected=1
	fi
	sleep "$send_delay"
fi

if [ "$expect_qemu_exit" = 1 ]; then
	if wait "$qemu_pid"; then
		qemu_pid=
		echo "command qemu-exit regression ok"
		exit 0
	fi
	qemu_status=$?
	qemu_pid=
	fail_command "qemu exited with status $qemu_status" 220
fi

if [ "$status_injected" = 1 ]; then
	:
elif [ "$guest_poweroff" = 1 ] && [ "$user" = root ]; then
send_script <<EOF_COMMAND_DONE_ROOT
status=\$?
echo __BUNIX_COMMAND_STATUS__=\$status
test "\$status" = 0 && echo $marker
test "\$status" = 0 && /bin/busybox poweroff -f
exit
EOF_COMMAND_DONE_ROOT
else
send_script <<EOF_COMMAND_DONE
status=\$?
echo __BUNIX_COMMAND_STATUS__=\$status
test "\$status" = 0 && echo $marker
exit
EOF_COMMAND_DONE
fi

wait_for_fixed "$log" "__BUNIX_COMMAND_STATUS__=" "command did not report status" 90 220
if ! grep -aF "__BUNIX_COMMAND_STATUS__=0" "$log" >/dev/null 2>&1; then
	fail_command "command failed" 220
fi
wait_for_fixed "$log" "$marker" "command marker missing" 15 220

if [ "$guest_poweroff" = 1 ]; then
	wait_for_guest_poweroff
fi

echo "command regression ok"
