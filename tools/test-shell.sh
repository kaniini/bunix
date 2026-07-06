#!/bin/sh
set -eu

qemu=${QEMU:-qemu-system-x86_64}
ovmf=${OVMF_CODE:-/usr/share/OVMF/OVMF_CODE.fd}
esp=${ESP_DIR:-build/esp}
timeout_cmd=${TIMEOUT:-timeout}
qemu_timeout=${QEMU_TIMEOUT:-180s}
qemu_memory=${QEMU_MEMORY:-128M}
qemu_extra_args=${QEMU_EXTRA_ARGS:-}
guest_poweroff=${BUNIX_GUEST_POWEROFF:-1}
send_line_delay=${BUNIX_SEND_LINE_DELAY:-0.35}
run_id=${BUNIX_TEST_RUN_ID:-$(date -u +%Y%m%dT%H%M%SZ)-$$}
tmp=${BUNIX_TEST_RUNTIME_DIR:-${TMPDIR:-/tmp}/bunix-shell-test.$run_id}
log=$tmp/serial.log
qemu_log=$tmp/qemu.log
pipe=$tmp/serial
runtime_esp=$tmp/esp
failure_dir=${FAILURE_DIR:-build/failures/$run_id}
script_dir=$(CDPATH= cd "$(dirname "$0")" && pwd)
shell_parts=${BUNIX_SHELL_PART:-all}
. "$script_dir/test-lib.sh"
. "$script_dir/shell-tests/smoke.sh"
. "$script_dir/shell-tests/login-smoke.sh"
. "$script_dir/shell-tests/relogin-session.sh"
. "$script_dir/shell-tests/exec-argv-pipe.sh"
. "$script_dir/shell-tests/procfs-cmdline.sh"
. "$script_dir/shell-tests/rootfs-vfs-proc-dev.sh"
. "$script_dir/shell-tests/tmpfs-basic-linux-tests.sh"
. "$script_dir/shell-tests/path-limits-statfs.sh"
. "$script_dir/shell-tests/union-root-user.sh"
. "$script_dir/shell-tests/tmpfs-extended.sh"
. "$script_dir/shell-tests/large-io-mount.sh"
. "$script_dir/shell-tests/interactive-tty.sh"
. "$script_dir/shell-tests/root-login-union.sh"
. "$script_dir/shell-tests/root-tmpfs-chown.sh"
. "$script_dir/shell-tests/long-login.sh"
. "$script_dir/shell-tests/root-mount-soak.sh"
BUNIX_COLLECT_FAILURES=1
BUNIX_FAILURE_DIR=$failure_dir
BUNIX_QEMU_LOG=$qemu_log
BUNIX_TEST_HARNESS=$0
BUNIX_TEST_RUNTIME_DIR=$tmp
BUNIX_SHELL_PARTS=$shell_parts
export BUNIX_COLLECT_FAILURES BUNIX_FAILURE_DIR BUNIX_QEMU_LOG BUNIX_TEST_HARNESS BUNIX_TEST_RUNTIME_DIR BUNIX_SHELL_PARTS

cleanup() {
	if [ "${qemu_pid:-}" ]; then
		kill "$qemu_pid" 2>/dev/null || true
	fi
	if [ "${cat_pid:-}" ]; then
		kill "$cat_pid" 2>/dev/null || true
	fi
	if [ "${KEEP_TMP:-0}" != 1 ]; then
		rm -rf "$tmp"
	else
		echo "kept test tmp: $tmp" >&2
	fi
}

fail_with_qemu_log() {
	label=$1
	tail_lines=${2:-120}
	out=$(save_failure_artifacts "$label" "$log" "$qemu_log" "$tail_lines")

	echo "$label" >&2
	echo "failure artifacts: $out" >&2
	cat "$qemu_log" >&2 || true
	tail -n "$tail_lines" "$log" >&2 || true
	exit 1
}

send_script() {
	while IFS= read -r line || [ -n "$line" ]; do
		printf '%s\n' "$line" >&3
		sleep "$send_line_delay"
	done
}

send_script_sync() {
	prompt_probe=$(printf '\033[6n')

	while IFS= read -r line || [ -n "$line" ]; do
		before=$(grep -aF -c "$prompt_probe" "$log" 2>/dev/null || true)
		printf '%s\n' "$line" >&3
		wait_for_prompt_count_gt "$prompt_probe" "$before" \
			"shell prompt did not return after: $line" 90 220
		sleep "$send_line_delay"
	done
}

send_bytes() {
	printf '%b' "$1" >&3
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
			fail_with_qemu_log "qemu exited while waiting for: $expected" "$tail_lines"
		fi
		if [ "$i" -gt "$limit" ]; then
			fail_with_qemu_log "$label" "$tail_lines"
		fi
		sleep 1
	done
}

wait_for_prompt_count_gt() {
	prompt=$1
	before=$2
	label=$3
	limit=${4:-45}
	tail_lines=${5:-180}
	i=0

	while [ "$(grep -aF -c "$prompt" "$log" 2>/dev/null || true)" -le "$before" ]; do
		i=$((i + 1))
		if [ "$i" -gt "$limit" ]; then
			fail_with_log "$label" "$log" "$tail_lines"
		fi
		sleep 1
	done
}

start_qemu() {
	mkdir -p "$tmp"
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

	TMPDIR=$tmp $timeout_cmd "$qemu_timeout" "$qemu" -enable-kvm -machine q35 -cpu host -m "$qemu_memory" \
		-smp "${SMP:-2}" \
		-drive if=pflash,format=raw,readonly=on,file="$ovmf" \
		-drive format=raw,file=fat:rw:"$runtime_esp" \
		-serial pipe:"$pipe" -display none -no-reboot $qemu_extra_args 2>"$qemu_log" &
	qemu_pid=$!
}

login_user() {
	user=$1
	password=$2
	prompt=$3
	before=${4:-}

	if [ -n "$before" ]; then
		printf '%s\n%s\n' "$user" "$password" >&3
		wait_for_prompt_count_gt "$prompt" "$before" "shell prompt did not appear for $user" 45 180
		return
	fi

	printf '%s\n%s\n' "$user" "$password" >&3
	wait_for_fixed "$log" "$prompt" "shell prompt did not appear for $user" 150 120
}

current_prompt_count() {
	prompt=$1

	grep -F -c "$prompt" "$log" 2>/dev/null || true
}

begin_shard() {
	name=$1

	BUNIX_CURRENT_SHARD=$name
	BUNIX_CURRENT_SHARD_FILE=$script_dir/shell-tests/$name.sh
	export BUNIX_CURRENT_SHARD
	export BUNIX_CURRENT_SHARD_FILE
	mkdir -p "$tmp/shard-times"
	date +%s > "$tmp/shard-times/$name"
}

finish_shard() {
	name=$1
	start=$(cat "$tmp/shard-times/$name" 2>/dev/null || echo 0)
	now=$(date +%s)
	seconds=$((now - start))

	echo "test-shell-part name=$name status=ok seconds=$seconds"
	unset BUNIX_CURRENT_SHARD
	unset BUNIX_CURRENT_SHARD_FILE
}

part_selected() {
	part=$1

	case ",$shell_parts," in
	*,all,*|*,"$part",*) return 0 ;;
	esac

	case "$part" in
	rootfs-vfs-proc-dev)
		case ",$shell_parts," in
		*,vfs,*|*,procfs,*|*,devfs,*) return 0 ;;
		esac
		;;
	tmpfs-basic-linux-tests|tmpfs-extended)
		case ",$shell_parts," in
		*,tmpfs,*) return 0 ;;
		esac
		;;
	path-limits-statfs)
		case ",$shell_parts," in
		*,path,*|*,statfs,*) return 0 ;;
		esac
		;;
	large-io-mount)
		case ",$shell_parts," in
		*,large-io,*|*,mount,*) return 0 ;;
		esac
		;;
	esac

	return 1
}

require_supported_parts() {
	old_ifs=$IFS
	IFS=,
	set -- $shell_parts
	IFS=$old_ifs

	for part do
		case "$part" in
		all|vfs|procfs|devfs|tmpfs|path|statfs|large-io|mount|\
		smoke|login-smoke|relogin-session|exec-argv-pipe|procfs-cmdline|rootfs-vfs-proc-dev|\
		tmpfs-basic-linux-tests|path-limits-statfs|union-root-user|\
		tmpfs-extended|large-io-mount|interactive-tty|root-login-union|\
		root-tmpfs-chown|long-login|root-mount-soak)
			;;
		*)
			fail_with_log "unknown shell shard: $part" "$log" 80
			;;
		esac
	done
}

exit_user_shell_if_active() {
	if [ "${user_shell_active:-0}" = 0 ]; then
		return
	fi

	login_prompts_before_auto_exit=$(current_prompt_count "login: ")
	send_script <<'EOF_AUTO_EXIT_USER'
exit
EOF_AUTO_EXIT_USER
	wait_for_prompt_count_gt "login: " "$login_prompts_before_auto_exit" "login prompt did not return after selected user shards" 45 180
	user_shell_active=0
}

login_user_if_needed() {
	if [ "${user_shell_active:-0}" = 1 ]; then
		return
	fi

	user_prompts_before=$(current_prompt_count "~ $ ")
	login_user kaniini bunix "~ $ " "$user_prompts_before"
	user_shell_active=1
}

login_root_if_needed() {
	if [ "${root_shell_active:-0}" = 1 ]; then
		return
	fi

	exit_user_shell_if_active
	root_prompts_before_auto_root=$(current_prompt_count "~ # ")
	login_user root root "~ # " "$root_prompts_before_auto_root"
	root_shell_active=1
}

exit_root_shell_if_active() {
	if [ "${root_shell_active:-0}" = 0 ]; then
		return
	fi

	login_prompts_before_auto_root_exit=$(current_prompt_count "login: ")
	send_script <<'EOF_AUTO_EXIT_ROOT'
exit
EOF_AUTO_EXIT_ROOT
	wait_for_prompt_count_gt "login: " "$login_prompts_before_auto_root_exit" "login prompt did not return after selected root shards" 45 180
	root_shell_active=0
}

wait_for_guest_poweroff() {
	if [ "$guest_poweroff" != 1 ]; then
		return
	fi

	login_root_if_needed
	send_script <<'EOF_GUEST_POWEROFF'
/sbin/poweroff
EOF_GUEST_POWEROFF

	if wait "$qemu_pid"; then
		qemu_pid=
		return
	fi
	qemu_status=$?
	qemu_pid=
	fail_with_qemu_log "qemu exited with status $qemu_status" 220
}

start_qemu
wait_for_qemu_fixed "login: " "login prompt did not appear" 80 80
require_supported_parts

sleep 3
exec 3>"$pipe.in"
login_user kaniini bunix "~ $ "
user_shell_active=1
root_shell_active=0

if part_selected smoke; then
	begin_shard smoke
	run_smoke
	check_smoke
	finish_shard smoke
fi

if part_selected login-smoke; then
	begin_shard login-smoke
	run_login_smoke
fi

if part_selected relogin-session; then
	begin_shard relogin-session
	run_relogin_session
	user_shell_active=0
fi

if part_selected exec-argv-pipe; then
	begin_shard exec-argv-pipe
	login_user_if_needed
	run_exec_argv_pipe
fi

if part_selected procfs-cmdline; then
	begin_shard procfs-cmdline
	login_user_if_needed
	run_procfs_cmdline
fi

if part_selected rootfs-vfs-proc-dev; then
	begin_shard rootfs-vfs-proc-dev
	login_user_if_needed
	run_rootfs_vfs_proc_dev
fi

if part_selected tmpfs-basic-linux-tests; then
	begin_shard tmpfs-basic-linux-tests
	login_user_if_needed
	run_tmpfs_basic_linux_tests
fi

if part_selected path-limits-statfs; then
	begin_shard path-limits-statfs
	login_user_if_needed
	run_path_limits_statfs
fi

if part_selected union-root-user; then
	begin_shard union-root-user
	login_user_if_needed
	run_union_root_user
fi

if part_selected tmpfs-extended; then
	begin_shard tmpfs-extended
	login_user_if_needed
	run_tmpfs_extended
fi

if part_selected large-io-mount; then
	begin_shard large-io-mount
	login_user_if_needed
	run_large_io_mount
fi

if part_selected login-smoke; then
	check_login_smoke
	finish_shard login-smoke
fi

if part_selected relogin-session; then
	check_relogin_session
	finish_shard relogin-session
fi

if part_selected rootfs-vfs-proc-dev; then
	check_rootfs_vfs_proc_dev
	finish_shard rootfs-vfs-proc-dev
fi

if part_selected procfs-cmdline; then
	check_procfs_cmdline
	finish_shard procfs-cmdline
fi

if part_selected path-limits-statfs; then
	check_path_limits_statfs
	finish_shard path-limits-statfs
fi

if part_selected union-root-user; then
	check_union_root_user
	finish_shard union-root-user
fi

if part_selected tmpfs-extended; then
	check_tmpfs_extended
	finish_shard tmpfs-extended
fi

if part_selected large-io-mount; then
	check_large_io_mount
	finish_shard large-io-mount
fi

if part_selected tmpfs-basic-linux-tests; then
	check_tmpfs_basic_linux_tests
	finish_shard tmpfs-basic-linux-tests
fi

if part_selected exec-argv-pipe; then
	check_exec_argv_pipe
	finish_shard exec-argv-pipe
fi

if part_selected interactive-tty; then
	begin_shard interactive-tty
	login_user_if_needed
	run_interactive_tty
	user_shell_active=0
	finish_shard interactive-tty
fi

if part_selected root-login-union; then
	begin_shard root-login-union
	exit_user_shell_if_active
	run_root_login_union
	check_root_login_union
	root_shell_active=1
	finish_shard root-login-union
fi

if part_selected root-tmpfs-chown; then
	begin_shard root-tmpfs-chown
	login_root_if_needed
	run_root_tmpfs_chown
	root_shell_active=0
	user_shell_active=0
	finish_shard root-tmpfs-chown
fi

if part_selected root-mount-soak; then
	begin_shard root-mount-soak
	run_root_mount_soak
	check_root_mount_soak
	root_shell_active=1
	finish_shard root-mount-soak
fi

if part_selected long-login; then
	begin_shard long-login
	exit_root_shell_if_active
	exit_user_shell_if_active
	run_long_login
	check_long_login
	finish_shard long-login
fi
echo "shell regression ok"
wait_for_guest_poweroff
