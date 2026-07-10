#!/bin/sh
set -eu

script_dir=$(CDPATH= cd "$(dirname "$0")" && pwd)
. "$script_dir/path-safety.sh"

arch=${ARCH_USER_C:-arch/x86_64/user.c}
server=${LINUX_SERVER_C:-user/linux/main.c}
proto=${BUNIX_SYSCALL_H:-user/include/bunix/syscall.h}
test_paths=${LINUX_AUDIT_TEST_PATHS:-"tools user"}
tmp=${TMPDIR:-/tmp}/bunix-linux-audit.$$

cleanup() {
	safe_rm_rf "$tmp" "Linux syscall audit scratch directory"
}
trap cleanup EXIT INT TERM

mkdir -p "$tmp"

enum_file=$tmp/enum
names_file=$tmp/names
arch_file=$tmp/arch
scalar_file=$tmp/scalar
server_file=$tmp/server
tested_file=$tmp/tested
table_file=$tmp/table

awk '
	/^[ \t]*LINUX_SYSCALL_[A-Z0-9_]+[ \t]*=/ {
		name = $1
		gsub(",", "", name)
		value = $0
		sub(/.*= */, "", value)
		sub(/,.*/, "", value)
		print value "\t" name
	}
' "$arch" | sort -n > "$enum_file"

awk '
	/static const char \*linux_syscall_name/ { in_names = 1 }
	in_names && /^[ \t]*case LINUX_SYSCALL_/ {
		name = $2
		gsub(":", "", name)
	}
	in_names && /return "/ && name != "" {
		text = $0
		sub(/.*return "/, "", text)
		sub(/".*/, "", text)
		print name "\t" text
		name = ""
	}
	in_names && /^[ \t]*default:/ { in_names = 0 }
' "$arch" > "$names_file"

awk '
	function flush(	i, class, risk) {
		if (count == 0) {
			return
		}
		class = "local"
		if (body ~ /linux_forward/ || body ~ /request.type[ \t]*=[ \t]*LINUX_SYSCALL_/ ||
		    body ~ /linux_forward_message/) {
			class = "forwarded"
		}
		risk = "-"
		if (body ~ /LINUX_ENOSYS/) {
			risk = risk == "-" ? "ENOSYS" : risk ",ENOSYS"
		}
		if (body ~ /LINUX_EINVAL/ || body ~ /linux_einval_u64/) {
			risk = risk == "-" ? "EINVAL" : risk ",EINVAL"
		}
		for (i = 1; i <= count; i++) {
			print cases[i] "\t" class "\t" risk "\t" first_line
		}
		count = 0
		body = ""
		first_line = 0
	}
	/static u64 linux_syscall_handle/ { in_handle = 1 }
	in_handle && /^[ \t]*case LINUX_SYSCALL_/ {
		if (body != "") {
			flush()
		}
		name = $2
		gsub(":", "", name)
		cases[++count] = name
		if (first_line == 0) {
			first_line = FNR
		}
		next
	}
	in_handle && /^static u64 linux_syscall_dispatch/ {
		flush()
		in_handle = 0
	}
	in_handle && count > 0 {
		body = body $0 "\n"
	}
' "$arch" > "$arch_file"

awk '
	/static int linux_syscall_forwards_scalar/ { in_scalar = 1 }
	in_scalar && /^[ \t]*case LINUX_SYSCALL_/ {
		name = $2
		gsub(":", "", name)
		print name "\tscalar-forwarded\t-\t" FNR
	}
	in_scalar && /^[ \t]*default:/ { in_scalar = 0 }
' "$arch" > "$scalar_file"

awk '
	/^[ \t]*case BUNIX_LINUX_/ {
		name = $2
		gsub(":", "", name)
		sub(/^BUNIX_LINUX_/, "LINUX_SYSCALL_", name)
		print name "\tserver\t" FNR
	}
' "$server" | sort -u > "$server_file"

while read -r number const; do
	token=$(printf '%s\n' "$const" |
		sed 's/^LINUX_SYSCALL_//' |
		tr 'ABCDEFGHIJKLMNOPQRSTUVWXYZ_' 'abcdefghijklmnopqrstuvwxyz-')
	found=no
	for path in $test_paths; do
		if [ -e "$path" ] && grep -R -i -q "$token" "$path" 2>/dev/null; then
			found=yes
			break
		fi
	done
	printf '%s\t%s\n' "$const" "$found"
done < "$enum_file" > "$tested_file"

awk -v names="$names_file" -v archf="$arch_file" -v scalarf="$scalar_file" \
    -v serverf="$server_file" -v tested="$tested_file" '
	FILENAME == names {
		human[$1] = $2
		next
	}
	FILENAME == archf {
		arch_class[$1] = $2
		arch_risk[$1] = $3
		arch_line[$1] = $4
		next
	}
	FILENAME == scalarf {
		arch_class[$1] = $2
		arch_risk[$1] = $3
		arch_line[$1] = $4
		next
	}
	FILENAME == serverf {
		server_class[$1] = "server"
		server_line[$1] = $3
		next
	}
	FILENAME == tested {
		tested_class[$1] = $2
		next
	}
	{
		number = $1
		const = $2
		name = human[const] != "" ? human[const] : const
		a = arch_class[const] != "" ? arch_class[const] : "missing"
		s = server_class[const] != "" ? server_class[const] : "-"
		t = tested_class[const] != "" ? tested_class[const] : "no"
		r = arch_risk[const] != "" ? arch_risk[const] : "-"
		printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\n", number, name, const, a, s, t, r
	}
' "$names_file" "$arch_file" "$scalar_file" "$server_file" "$tested_file" "$enum_file" |
	sort -n > "$table_file"

defined=$(wc -l < "$enum_file" | tr -d ' ')
handled=$(awk '$4 != "missing" { count++ } END { print count + 0 }' "$table_file")
forwarded=$(awk '$4 == "forwarded" || $4 == "scalar-forwarded" { count++ } END { print count + 0 }' "$table_file")
server_handled=$(awk '$5 == "server" { count++ } END { print count + 0 }' "$table_file")
missing=$(awk '$4 == "missing" { count++ } END { print count + 0 }' "$table_file")
untested=$(awk '$6 != "yes" { count++ } END { print count + 0 }' "$table_file")

cat <<EOF
Linux syscall audit
===================
arch:   $arch
server: $server

defined:        $defined
arch handled:   $handled
arch forwarded: $forwarded
server handled: $server_handled
missing arch:   $missing
no marker hint: $untested

Syscall table
-------------
nr	name	enum	arch	server	marker	risk
EOF
cat "$table_file"

echo
echo "Missing arch dispatch"
echo "---------------------"
awk '$4 == "missing" { print $1 "\t" $2 "\t" $3 }' "$table_file"

echo
echo "Forwarded without server case"
echo "-----------------------------"
awk '($4 == "forwarded" || $4 == "scalar-forwarded") && $5 != "server" { print $1 "\t" $2 "\t" $3 }' "$table_file"

echo
echo "Risky fallback sites"
echo "--------------------"
if command -v rg >/dev/null 2>&1; then
	rg -n "return .*LINUX_(ENOSYS|EINVAL)|linux_einval_u64|unknown syscall" "$arch" "$server" || true
else
	grep -n -E "return .*LINUX_(ENOSYS|EINVAL)|linux_einval_u64|unknown syscall" "$arch" "$server" || true
fi

echo
echo "Marker hints are heuristic string matches in: $test_paths"
