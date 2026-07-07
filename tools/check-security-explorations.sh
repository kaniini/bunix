#!/bin/sh
set -eu

parent=${1:-explorations/022-security-vulnerability-audit.txt}
shift || true

if [ "$#" -eq 0 ]; then
	set -- explorations/03*-*.txt explorations/04*-*.txt
fi

status=0

for file in "$@"; do
	case "$file" in
	explorations/034-*|explorations/035-*|explorations/036-*|\
	explorations/037-*|explorations/038-*|explorations/039-*|\
	explorations/040-*|explorations/041-*|explorations/042-*|\
	explorations/043-*)
		;;
	*)
		continue
		;;
	esac

	if [ ! -f "$file" ]; then
		continue
	fi

	name=${file#explorations/}
	if ! grep -F "$name" "$parent" >/dev/null 2>&1; then
		echo "security exploration missing parent reference: $file" >&2
		status=1
	fi
	for section in "Problem:" "Threat model:" "Evidence:" "Root cause:" "Fix plan:" "Tasks:"; do
		if ! grep -F "$section" "$file" >/dev/null 2>&1; then
			echo "security exploration missing $section $file" >&2
			status=1
		fi
	done
	if ! grep -F -- "- [ ]" "$file" >/dev/null 2>&1; then
		echo "security exploration has no open tasks: $file" >&2
		status=1
	fi
done

if [ "$status" -eq 0 ]; then
	echo "security exploration audit ok"
fi
exit "$status"
