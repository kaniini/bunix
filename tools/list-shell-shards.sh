#!/bin/sh
set -eu

manifest=${1:-tools/shell-shards.tsv}

awk -F '\t' '
BEGIN {
	printf "%-20s %-7s %-4s %-7s %-7s %-5s %-10s %-7s %-9s %-6s %s\n",
	       "name", "phase", "smp", "memory", "timeout", "cost",
	       "cleanboot", "harness", "rootfs", "boot", "description"
}
/^#/ || NF == 0 {
	next
}
NF < 8 {
	printf "invalid shard row at %s:%d\n", FILENAME, FNR > "/dev/stderr"
	exit 1
}
{
	harness = $9 == "" ? "shell" : $9
	rootfs = $10 == "" ? "synthetic" : $10
	boot = $11 == "" ? "-" : $11
	printf "%-20s %-7s %-4s %-7s %-7s %-5s %-10s %-7s %-9s %-6s %s\n",
	       $1, $2, $3, $4, $5, $6, $7, harness, rootfs, boot, $8
}
' "$manifest"
