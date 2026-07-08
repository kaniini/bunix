#!/bin/sh
set -eu

out=$1
module=$2
name=${3:-abi-smoke.user}
tmp="$out.tmp"
size=$(wc -c < "$module" | awk '{ print $1 }')

mkdir -p "$(dirname "$out")"
{
	printf 'BUNIX-RV64-BOOTPKG\n'
	printf 'version 1\n'
	printf 'module %s %s\n' "$name" "$size"
	printf '\n'
	cat "$module"
} > "$tmp"
mv "$tmp" "$out"
