#!/bin/sh

path_safety_fail() {
	echo "path-safety: $*" >&2
	return 2
}

path_safety_repo_root() {
	git rev-parse --show-toplevel 2>/dev/null || pwd
}

path_safety_abs_for_parent() {
	path=$1
	parent=$(dirname "$path")
	base=$(basename "$path")
	missing=

	while [ ! -d "$parent" ]; do
		missing=$(basename "$parent")${missing:+/$missing}
		next=$(dirname "$parent")
		if [ "$next" = "$parent" ]; then
			path_safety_fail "cannot resolve parent for path: $path"
			return 2
		fi
		parent=$next
	done
	parent_abs=$(CDPATH= cd "$parent" && pwd -P) ||
		{ path_safety_fail "cannot resolve parent for path: $path"; return 2; }
	if [ -n "$missing" ]; then
		printf '%s/%s/%s\n' "$parent_abs" "$missing" "$base"
	else
		printf '%s/%s\n' "$parent_abs" "$base"
	fi
}

path_safety_abs_existing() {
	path=$1

	if [ ! -e "$path" ]; then
		path_safety_abs_for_parent "$path"
		return
	fi
	if [ -d "$path" ] && [ ! -L "$path" ]; then
		CDPATH= cd "$path" && pwd -P
		return
	fi
	path_safety_abs_for_parent "$path"
}

path_safety_is_temp_path() {
	path=$1
	tmp_root=${TMPDIR:-/tmp}

	case "$tmp_root" in
	''|/|.) tmp_root=/tmp ;;
	esac
	case "$path" in
	/tmp/*|/var/tmp/*|"$tmp_root"/*) return 0 ;;
	esac
	return 1
}

path_safety_is_build_path() {
	path=$1
	repo_root=$(path_safety_repo_root)

	case "$path" in
	"$repo_root"/build/*) return 0 ;;
	esac
	return 1
}

path_safety_reject_dangerous() {
	path=$1
	label=$2
	repo_root=$(path_safety_repo_root)
	home_dir=${HOME:-}

	case "$path" in
	''|/|.)
		path_safety_fail "refusing unsafe $label: $path"
		return 2
		;;
	esac
	case "$path" in
	"$repo_root"|"$repo_root"/.|"$repo_root"/..)
		path_safety_fail "refusing repository root as $label: $path"
		return 2
		;;
	esac
	if [ -n "$home_dir" ]; then
		case "$path" in
		"$home_dir"|"$home_dir"/.|"$home_dir"/..)
			path_safety_fail "refusing home directory as $label: $path"
			return 2
			;;
		esac
	fi
}

path_safety_require_generated_path() {
	path=$1
	label=${2:-generated path}

	case "$path" in
	''|/|.)
		path_safety_fail "refusing unsafe $label: $path"
		return 2
		;;
	esac
	abs=$(path_safety_abs_existing "$path") || return 2
	path_safety_reject_dangerous "$abs" "$label" || return 2
	if path_safety_is_build_path "$abs" || path_safety_is_temp_path "$abs"; then
		printf '%s\n' "$abs"
		return
	fi
	old_ifs=$IFS
	IFS=:
	for prefix in ${BUNIX_PATH_SAFETY_EXTRA_PREFIXES:-}; do
		IFS=$old_ifs
		[ -n "$prefix" ] || continue
		prefix_abs=$(path_safety_abs_existing "$prefix") || return 2
		path_safety_reject_dangerous "$prefix_abs" "extra safe path prefix" || return 2
		case "$abs" in
		"$prefix_abs"|"$prefix_abs"/*)
			IFS=$old_ifs
			printf '%s\n' "$abs"
			return
			;;
		esac
		IFS=:
	done
	IFS=$old_ifs
	path_safety_fail "refusing $label outside build/ or tmp: $path"
	return 2
}

safe_rm_rf() {
	path=$1
	label=${2:-recursive removal path}

	abs=$(path_safety_require_generated_path "$path" "$label") || exit 2
	rm -rf "$abs"
}

safe_rm_f() {
	path=$1
	label=${2:-generated file}

	abs=$(path_safety_require_generated_path "$path" "$label") || exit 2
	rm -f "$abs"
}
