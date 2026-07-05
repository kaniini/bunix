#!/bin/sh
set -eu

manifest=${1:-tools/shell-shards.tsv}

awk -F '\t' '
BEGIN {
	printf "%-16s %-7s %-4s %-7s %-7s %-5s %-10s %s\n",
	       "name", "phase", "smp", "memory", "timeout", "cost",
	       "cleanboot", "description"
}
/^#/ || NF == 0 {
	next
}
NF < 8 {
	printf "invalid shard row at %s:%d\n", FILENAME, FNR > "/dev/stderr"
	exit 1
}
{
	printf "%-16s %-7s %-4s %-7s %-7s %-5s %-10s %s\n",
	       $1, $2, $3, $4, $5, $6, $7, $8
}
' "$manifest"
