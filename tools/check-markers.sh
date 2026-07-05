#!/bin/sh
set -eu

if [ "$#" -ne 2 ]; then
	echo "usage: $0 LOG MARKERS" >&2
	exit 2
fi

script_dir=$(CDPATH= cd "$(dirname "$0")" && pwd)
. "$script_dir/test-lib.sh"

check_fixed_markers_file "$1" "$2" "test marker missing" 1 120
