#!/bin/sh
set -eu

out=$1
tmp="$out.tmp.$$"
payload_tmp="$out.payload.tmp.$$"
trap 'rm -f "$tmp" "$payload_tmp"' EXIT

shift
cmdline=
if [ "$#" -ge 2 ] && [ "$1" = "--cmdline" ]; then
	cmdline=$2
	shift 2
fi
if [ "$#" -eq 1 ]; then
	set -- "$1" abi-smoke.user
fi
if [ "$#" -eq 0 ] || [ $(( $# % 2 )) -ne 0 ]; then
	echo "usage: $0 OUT MODULE [NAME] [MODULE NAME ...]" >&2
	exit 1
fi

mkdir -p "$(dirname "$out")"
: > "$payload_tmp"
{
	printf 'BUNIX-RV64-BOOTPKG\n'
	printf 'version 1\n'
	if [ -n "$cmdline" ]; then
		printf 'cmdline %s\n' "$cmdline"
	fi
	while [ "$#" -gt 0 ]; do
		module=$1
		name=$2
		size=$(wc -c < "$module" | awk '{ print $1 }')
		printf 'module %s %s\n' "$name" "$size"
		cat "$module" >> "$payload_tmp"
		shift 2
	done
	printf '\n'
	cat "$payload_tmp"
} > "$tmp"
rm -f "$payload_tmp"
mv "$tmp" "$out"
trap - EXIT
