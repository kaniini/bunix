#!/bin/sh
set -eu

script_dir=$(CDPATH= cd "$(dirname "$0")" && pwd)
. "$script_dir/path-safety.sh"

fail() {
	echo "test-path-safety: $*" >&2
	exit 1
}

expect_fail() {
	label=$1
	shift

	if sh -c 'set -e; . tools/path-safety.sh; "$@"' test-path-safety "$@" \
		>/tmp/bunix-path-safety.out 2>/tmp/bunix-path-safety.err; then
		cat /tmp/bunix-path-safety.out >&2 || true
		cat /tmp/bunix-path-safety.err >&2 || true
		fail "$label unexpectedly succeeded"
	fi
	if ! grep -F "path-safety: refusing" /tmp/bunix-path-safety.err >/dev/null; then
		cat /tmp/bunix-path-safety.err >&2 || true
		fail "$label did not report path-safety refusal"
	fi
}

expect_ok() {
	label=$1
	shift

	if ! "$@" >/tmp/bunix-path-safety.out 2>/tmp/bunix-path-safety.err; then
		cat /tmp/bunix-path-safety.out >&2 || true
		cat /tmp/bunix-path-safety.err >&2 || true
		fail "$label failed"
	fi
}

run_id=$(date -u +%Y%m%dT%H%M%SZ)-$$
tmp_dir=${TMPDIR:-/tmp}/bunix-path-safety-test.$run_id
build_dir=build/path-safety-test.$run_id
trap 'safe_rm_rf "$tmp_dir" "path safety test tmp"; safe_rm_rf "$build_dir" "path safety test build"' EXIT INT TERM

mkdir -p "$tmp_dir/sub" "$build_dir/sub"
: > "$tmp_dir/sub/file"
: > "$build_dir/sub/file"

expect_ok "tmp recursive removal" safe_rm_rf "$tmp_dir/sub" "tmp removal"
expect_ok "build recursive removal" safe_rm_rf "$build_dir/sub" "build removal"
expect_ok "build generated file removal" safe_rm_f "$build_dir/file.img" "build generated file"

expect_fail "empty path" path_safety_require_generated_path "" "empty path"
expect_fail "root path" safe_rm_rf / "root removal"
expect_fail "current directory path" safe_rm_rf . "current directory removal"
expect_fail "repository root path" safe_rm_rf "$(path_safety_repo_root)" "repo root removal"
if [ -n "${HOME:-}" ]; then
	expect_fail "home directory path" safe_rm_rf "$HOME" "home removal"
fi
expect_fail "non-generated relative path" safe_rm_rf modules "module removal"

if [ -e tools/mkrootfs.c ]; then
	fail "retired tools/mkrootfs.c source returned"
fi
if grep 'tools/mkrootfs\.c\|mkrootfs --tree' Makefile tools/build-*.sh \
	tools/test-boot.sh tools/test-command.sh tools/test-shell.sh \
	tools/test-prune-artifacts.sh tools/test-alpine-rootfs.sh \
	tools/test-riscv64-alpine-rootfs.sh tools/audit-linux-syscalls.sh \
	>/tmp/bunix-path-safety.out; then
	cat /tmp/bunix-path-safety.out >&2
	fail "retired mkrootfs importer is referenced by live build/tooling"
fi
if grep -R 'rm -rf "\$' tools/*.sh | grep -v 'tools/path-safety.sh:' \
	>/tmp/bunix-path-safety.out; then
	cat /tmp/bunix-path-safety.out >&2
	fail "unguarded rm -rf remains in host shell tools"
fi

safe_rm_f /tmp/bunix-path-safety.out "path safety stdout"
safe_rm_f /tmp/bunix-path-safety.err "path safety stderr"
echo "test-path-safety: ok"
