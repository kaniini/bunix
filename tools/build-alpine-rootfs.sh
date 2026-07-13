#!/bin/sh
set -eu

script_dir=$(CDPATH= cd "$(dirname "$0")" && pwd)
. "$script_dir/path-safety.sh"

out=${1:?output rootfs image required}
run_id=$(date -u +%Y%m%dT%H%M%SZ)-$$
stage=${ALPINE_ROOTFS_STAGE:-${TMPDIR:-/tmp}/bunix-alpine-rootfs.$run_id}
artifact_dir=${ALPINE_ROOTFS_ARTIFACT_DIR:-build/alpine-rootfs}
root=$stage/root
manifest=$artifact_dir/manifest.txt
apk_log=$artifact_dir/apk.log
apk_plain_log=$artifact_dir/apk.plain.log
apk_cache=${APK_CACHE_DIR:-$artifact_dir/apk-cache}
repositories=${APK_REPOSITORIES_FILE:-/etc/apk/repositories}
apk_packages=${ALPINE_ROOTFS_PACKAGES:-alpine-baselayout busybox musl openrc ifupdown-ng}
apk_arch=${APK_ARCH:-}
if [ -n "${ALPINE_OPENRC_POLICY:-}" ]; then
	runlevel_policy=$ALPINE_OPENRC_POLICY
else
	runlevel_policy=tools/alpine-openrc-runlevels.policy
fi
reference_runlevels=$artifact_dir/openrc-reference-runlevels.tsv
bunix_runlevels=$artifact_dir/openrc-bunix-runlevels.tsv
initd_manifest=$artifact_dir/openrc-initd.tsv
confd_manifest=$artifact_dir/openrc-confd.tsv
policy_manifest=$artifact_dir/openrc-policy.tsv
rootfs_format=${ROOTFS_IMAGE_FORMAT:-squashfs}
login=${LOGIN_MODULE:-build/modules/login.user}
statidtest=${STATIDTEST_MODULE:-build/modules/statidtest.user}
ldsopathtest=${LDSOPATHTEST_MODULE:-build/modules/ldsopathtest.user}
bunix_overlay=${BUNIX_ALPINE_OVERLAY:-1}
init_command=${BUNIX_ALPINE_INIT_COMMAND:-/bin/login}
extra_dir=${BUNIX_ALPINE_EXTRA_DIR:-}

merge_account_file() {
	base=$1
	overlay=$2
	tmp=$base.bunix-merge

	awk -F: '
		FNR == NR {
			replace[$1] = 1
			lines[++line_count] = $0
			next
		}
		!($1 in replace) {
			print
		}
		END {
			for (i = 1; i <= line_count; i++) {
				print lines[i]
			}
		}
	' "$overlay" "$base" > "$tmp"
	mv "$tmp" "$base"
}

write_runlevel_inventory() {
	root_dir=$1
	out_file=$2

	: > "$out_file"
	if [ ! -d "$root_dir/etc/runlevels" ]; then
		return
	fi
	find "$root_dir/etc/runlevels" -maxdepth 2 -type l | sort |
	while IFS= read -r link; do
		rel=${link#"$root_dir/etc/runlevels/"}
		printf '%s\t%s\n' "$rel" "$(readlink "$link")"
	done > "$out_file"
}

write_file_inventory() {
	dir=$1
	out_file=$2

	: > "$out_file"
	if [ -d "$dir" ]; then
		find "$dir" -maxdepth 1 -type f | sed 's|.*/||' | sort > "$out_file"
	fi
}

generate_openrc_cache() {
	cache_dir=$root/var/cache/rc
	work_dir=$stage/openrc-cache
	gendepends=$work_dir/gendepends.sh
	deps=$work_dir/dependencies.txt
	deptree=$cache_dir/deptree
	depconfig=$cache_dir/depconfig

	if [ ! -r "$root/usr/libexec/rc/sh/gendepends.sh" ]; then
		echo "OpenRC gendepends.sh is missing from Alpine rootfs" >&2
		exit 2
	fi

	mkdir -p "$cache_dir" "$work_dir"
	sed \
		-e 's|^\. /usr/libexec/rc/sh/functions.sh$|. "$BUNIX_OPENRC_LIBEXECDIR"/sh/functions.sh|' \
		-e 's|^\. /usr/libexec/rc/sh/rc-functions.sh$|. "$BUNIX_OPENRC_LIBEXECDIR"/sh/rc-functions.sh|' \
		-e 's|\[ -e /etc/rc.conf \] && \. /etc/rc.conf|[ -e "$BUNIX_OPENRC_SYSCONFDIR/rc.conf" ] \&\& . "$BUNIX_OPENRC_SYSCONFDIR/rc.conf"|' \
		-e 's|if \[ -d "/etc/rc.conf.d" \]; then|if [ -d "$BUNIX_OPENRC_SYSCONFDIR/rc.conf.d" ]; then|' \
		-e 's|for _f in "/etc"/rc.conf.d/\*.conf; do|for _f in "$BUNIX_OPENRC_SYSCONFDIR"/rc.conf.d/*.conf; do|' \
		"$root/usr/libexec/rc/sh/gendepends.sh" > "$gendepends"
	chmod 0555 "$gendepends"

	if ! BUNIX_OPENRC_LIBEXECDIR="$root/usr/libexec/rc" \
	     BUNIX_OPENRC_SYSCONFDIR="$root/etc" \
	     RC_SCRIPTDIRS="$root/etc" \
	     RC_UNAME=Bunix \
	     sh "$gendepends" > "$deps"; then
		if [ ! -s "$deps" ]; then
			echo "failed to generate OpenRC dependency data" >&2
			exit 1
		fi
	fi

	awk -v deptree="$deptree" -v depconfig="$depconfig" '
	function add_service(service)
	{
		if (service == "" || service_seen[service])
			return
		service_seen[service] = 1
		service_order[++service_count] = service
	}

	function add_dep(service, type, dep, key, dep_key)
	{
		if (service == "" || type == "" || dep == "" || dep == service)
			return
		add_service(service)
		dep_key = service SUBSEP type SUBSEP dep
		if (dep_seen[dep_key])
			return
		dep_seen[dep_key] = 1
		key = service SUBSEP type
		if (!type_seen[key]) {
			type_seen[key] = 1
			type_count[service]++
			type_order[service SUBSEP type_count[service]] = type
		}
		dep_count[key]++
		dep_order[key SUBSEP dep_count[key]] = dep
	}

	function reverse_type(type)
	{
		if (type == "ineed")
			return "needsme"
		if (type == "iuse")
			return "usesme"
		if (type == "iwant")
			return "wantsme"
		if (type == "iafter")
			return "ibefore"
		if (type == "ibefore")
			return "iafter"
		if (type == "iprovide")
			return "providedby"
		return ""
	}

	{
		service = $1
		type = $2
		add_service(service)
		if (NF < 3)
			next
		for (i = 3; i <= NF; i++) {
			dep = $i
			if (type == "config") {
				config_seen[dep] = 1
				continue
			}
			add_dep(service, type, dep)
			raw_service[++raw_count] = service
			raw_type[raw_count] = type
			raw_dep[raw_count] = dep
			if (type == "iprovide")
				add_service(dep)
		}
	}

	END {
		for (i = 1; i <= raw_count; i++) {
			rev = reverse_type(raw_type[i])
			if (rev == "")
				continue
			if (service_seen[raw_dep[i]])
				add_dep(raw_dep[i], rev, raw_service[i])
			else if (raw_type[i] == "ineed")
				add_dep(raw_service[i], "broken", raw_dep[i])
		}

		for (i = 1; i <= service_count; i++) {
			service = service_order[i]
			printf("depinfo_%u_service='\''%s'\''\n", i - 1,
			       service) > deptree
			for (j = 1; j <= type_count[service]; j++) {
				type = type_order[service SUBSEP j]
				key = service SUBSEP type
				for (k = 1; k <= dep_count[key]; k++)
					printf("depinfo_%u_%s_%u='\''%s'\''\n",
					       i - 1, type, k - 1,
					       dep_order[key SUBSEP k]) > deptree
			}
		}
		close(deptree)

		config_count = 0
		for (config in config_seen) {
			print config > depconfig
			config_count++
		}
		close(depconfig)
		if (config_count == 0)
			system("rm -f " depconfig)
	}
	' "$deps"

	if [ ! -s "$deptree" ]; then
		echo "OpenRC dependency cache was not generated" >&2
		exit 1
	fi
}

materialize_openrc_policy() {
	if [ ! -r "$runlevel_policy" ]; then
		echo "OpenRC runlevel policy is not readable: $runlevel_policy" >&2
		exit 2
	fi

	mkdir -p "$root/etc/runlevels/sysinit" "$root/etc/runlevels/boot" \
		"$root/etc/runlevels/default" "$root/etc/runlevels/nonetwork" \
		"$root/etc/runlevels/shutdown"
	find "$root/etc/runlevels" -maxdepth 2 -type l -exec rm -f {} \;
	cp "$runlevel_policy" "$policy_manifest"

	while IFS='	' read -r action runlevel service target reason; do
		case "$action" in
		''|\#*) continue ;;
		add|keep)
			if [ -z "$runlevel" ] || [ -z "$service" ] ||
			   [ -z "$target" ]; then
				echo "invalid OpenRC policy row: $action $runlevel $service" >&2
				exit 2
			fi
			mkdir -p "$root/etc/runlevels/$runlevel"
			ln -sf "$target" "$root/etc/runlevels/$runlevel/$service"
			;;
		replace|suppress|defer)
			: "$reason"
			;;
		*)
			echo "unknown OpenRC policy action: $action" >&2
			exit 2
			;;
		esac
	done < "$runlevel_policy"
}

apk_add_rootfs() {
	if [ -n "$apk_arch" ]; then
		# shellcheck disable=SC2086
		apk --arch "$apk_arch" "$@" add $apk_packages
	else
		# shellcheck disable=SC2086
		apk "$@" add $apk_packages
	fi
}

safe_rm_rf "$stage" "ALPINE_ROOTFS_STAGE"
mkdir -p "$root" "$(dirname "$out")" "$artifact_dir" "$apk_cache"
apk_cache=$(CDPATH= cd "$apk_cache" && pwd)

if ! apk_add_rootfs \
	--root "$root" --initdb --allow-untrusted \
	--cache-dir "$apk_cache" \
	--repositories-file "$repositories" \
	>"$apk_log" 2>&1; then
	mv "$apk_log" "$apk_plain_log"
	safe_rm_rf "$root" "Alpine rootfs retry tree"
	mkdir -p "$root"
	if ! apk_add_rootfs \
		--root "$root" --initdb --usermode --allow-untrusted \
		--cache-dir "$apk_cache" \
		--repositories-file "$repositories" \
		>"$apk_log" 2>&1; then
		cat "$apk_plain_log" >&2
		cat "$apk_log" >&2
		exit 1
	fi
fi

write_runlevel_inventory "$root" "$reference_runlevels"
write_file_inventory "$root/etc/init.d" "$initd_manifest"
write_file_inventory "$root/etc/conf.d" "$confd_manifest"

chmod -R u+w "$root"
mkdir -p "$root/bin" "$root/etc" "$root/etc/init.d" "$root/etc/runlevels/default" \
	"$root/etc/runlevels/boot" "$root/proc" "$root/sys" "$root/dev" "$root/run" \
	"$root/tmp" "$root/root" "$root/var/tmp"
mkdir -p "$root/run/openrc" "$root/run/lock" "$root/lib/rc/init.d"
if [ "$bunix_overlay" = 1 ]; then
	rm -f "$root/bin/login"
	cp "$login" "$root/bin/login"
	chmod 0555 "$root/bin/login"
	cp "$statidtest" "$root/bin/statidtest"
	chmod 0555 "$root/bin/statidtest"
	cp "$ldsopathtest" "$root/bin/ldsopathtest"
	chmod 0555 "$root/bin/ldsopathtest"

	merge_account_file "$root/etc/passwd" modules/passwd
	merge_account_file "$root/etc/shadow" modules/shadow
	merge_account_file "$root/etc/group" modules/group
fi
if [ -n "$extra_dir" ]; then
	if [ ! -d "$extra_dir" ]; then
		echo "BUNIX_ALPINE_EXTRA_DIR is not a directory: $extra_dir" >&2
		exit 2
	fi
	cp -a "$extra_dir"/. "$root"/
fi
chmod 0444 "$root/etc/passwd" "$root/etc/group"
chmod 0400 "$root/etc/shadow"

if [ ! -e "$root/etc/alpine-release" ]; then
	if [ -r /etc/alpine-release ]; then
		cp /etc/alpine-release "$root/etc/alpine-release"
	else
		printf 'edge\n' > "$root/etc/alpine-release"
	fi
fi

openrc_inittab_entries='::sysinit:/sbin/openrc sysinit
::sysinit:/sbin/openrc boot
::wait:/sbin/openrc default'

cat > "$root/etc/inittab" <<EOF_INITTAB
$openrc_inittab_entries
ttyS0::respawn:/bin/login
::shutdown:/sbin/openrc shutdown
EOF_INITTAB
sed -i "s|ttyS0::respawn:/bin/login|ttyS0::respawn:$init_command|" \
	"$root/etc/inittab"

mkdir -p "$root/etc/network"
cat > "$root/etc/network/interfaces" <<'EOF_INTERFACES'
auto lo
iface lo inet loopback
iface lo inet6 loopback

auto eth0
iface eth0 inet dhcp
EOF_INTERFACES
chmod 0444 "$root/etc/network/interfaces"
: > "$root/fastboot"
chmod 0444 "$root/fastboot"

if [ ! -f "$root/etc/init.d/networking" ]; then
	echo "stock Alpine networking service is missing" >&2
	exit 2
fi

materialize_openrc_policy
generate_openrc_cache
write_runlevel_inventory "$root" "$bunix_runlevels"

find "$root/var/cache/apk" -type f -delete 2>/dev/null || true

{
	echo "source=apk"
	echo "apk_cache=$apk_cache"
	echo "repositories=$repositories"
	echo "packages=$apk_packages"
	echo "apk_arch=${apk_arch:-$(apk --print-arch)}"
	echo "bunix_overlay=$bunix_overlay"
	echo "init_command=$init_command"
	echo "extra_dir=$extra_dir"
	echo "alpine_networking_service=stock"
	echo "openrc_policy=$runlevel_policy"
	echo "openrc_reference_runlevels=$reference_runlevels"
	echo "openrc_bunix_runlevels=$bunix_runlevels"
	apk --root "$root" info 2>/dev/null | sort
} > "$manifest"

case "$rootfs_format" in
squashfs)
	mksquashfs "$root" "$out" -noappend -no-compression -no-fragments \
		-no-exports -no-xattrs -all-root -root-mode 0755 -b 128K \
		-repro-time 0 >/dev/null
	;;
*)
	echo "unknown ROOTFS_IMAGE_FORMAT: $rootfs_format" >&2
	exit 2
	;;
esac
echo "alpine rootfs image ok format=$rootfs_format out=$out stage=$stage manifest=$manifest"
