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
runlevel_policy=${ALPINE_OPENRC_POLICY:-tools/alpine-openrc-runlevels.policy}
reference_runlevels=$artifact_dir/openrc-reference-runlevels.tsv
bunix_runlevels=$artifact_dir/openrc-bunix-runlevels.tsv
initd_manifest=$artifact_dir/openrc-initd.tsv
confd_manifest=$artifact_dir/openrc-confd.tsv
policy_manifest=$artifact_dir/openrc-policy.tsv
rootfs_format=${ROOTFS_IMAGE_FORMAT:-squashfs}
login=${LOGIN_MODULE:-build/modules/login.user}
statidtest=${STATIDTEST_MODULE:-build/modules/statidtest.user}
netdhcp=${NETDHCP_MODULE:-build/modules/bunix-udhcpc-script.user}
bunix_overlay=${BUNIX_ALPINE_OVERLAY:-1}
init_command=${BUNIX_ALPINE_INIT_COMMAND:-/bin/login}
extra_dir=${BUNIX_ALPINE_EXTRA_DIR:-}
networking_service=${ALPINE_NETWORKING_SERVICE:-generated}

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

install_bunix_openrc_providers() {
	cat > "$root/etc/init.d/devfs" <<'EOF_BUNIX_DEV'
#!/sbin/openrc-run

description="Declare Bunix devfs availability to OpenRC"

depend()
{
	provide dev dev-mount
	before hwdrivers machine-id fsck
}

start()
{
	return 0
}
EOF_BUNIX_DEV
	chmod 0755 "$root/etc/init.d/devfs"

	cat > "$root/etc/init.d/sysfs" <<'EOF_BUNIX_SYS'
#!/sbin/openrc-run

description="Declare Bunix sysfs availability to OpenRC"

depend()
{
	before hwdrivers
}

start()
{
	return 0
}
EOF_BUNIX_SYS
	chmod 0755 "$root/etc/init.d/sysfs"

	cat > "$root/etc/init.d/procfs" <<'EOF_BUNIX_PROC'
#!/sbin/openrc-run

description="Declare Bunix procfs availability to OpenRC"

depend()
{
	before sysfs
}

start()
{
	return 0
}
EOF_BUNIX_PROC
	chmod 0755 "$root/etc/init.d/procfs"

	mkdir -p "$root/etc/runlevels/sysinit"
	ln -sf /etc/init.d/devfs "$root/etc/runlevels/sysinit/devfs"
	ln -sf /etc/init.d/procfs "$root/etc/runlevels/sysinit/procfs"
	ln -sf /etc/init.d/sysfs "$root/etc/runlevels/sysinit/sysfs"
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
	mkdir -p "$root/usr/share/udhcpc" "$root/sbin"
	cp "$netdhcp" "$root/sbin/bunix-udhcpc-script"
	chmod 0555 "$root/sbin/bunix-udhcpc-script"
	ln -sf /sbin/bunix-udhcpc-script "$root/usr/share/udhcpc/default.script"

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

cat > "$root/etc/inittab" <<'EOF_INITTAB'
::sysinit:/sbin/openrc sysinit
::sysinit:/sbin/openrc boot
::wait:/sbin/openrc default
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

case "$networking_service" in
generated)
	cat > "$root/etc/init.d/networking" <<'EOF_NETWORKING'
#!/bin/sh

cfgfile=${cfgfile:-/etc/network/interfaces}
started=/run/openrc/started/networking

mark_started()
{
	mkdir -p /run/openrc/started
	ln -sf /etc/init.d/networking "$started"
}

start_networking()
{
	echo " * Starting networking ..."
	ifup -i "$cfgfile" lo >/dev/null 2>&1 || true
	ifup -i "$cfgfile" eth0 || return $?
	mark_started
	return 0
}

stop_networking()
{
	echo " * Stopping networking ..."
	ifdown -i "$cfgfile" eth0 >/dev/null 2>&1 || true
	ifdown -i "$cfgfile" lo >/dev/null 2>&1 || true
	rm -f "$started"
	return 0
}

status_networking()
{
	if [ -e "$started" ]; then
		echo " * status: started"
		return 0
	fi
	echo " * status: stopped"
	return 3
}

case "$1" in
start)
	start_networking
	;;
stop)
	stop_networking
	;;
restart)
	start_networking
	;;
status)
	status_networking
	;;
*)
	echo "usage: $0 {start|stop|restart|status}" >&2
	exit 1
	;;
esac
EOF_NETWORKING
	chmod 0755 "$root/etc/init.d/networking"
	;;
stock)
	if [ ! -f "$root/etc/init.d/networking" ]; then
		echo "stock Alpine networking service is missing" >&2
		exit 2
	fi
	;;
*)
	echo "unknown ALPINE_NETWORKING_SERVICE: $networking_service" >&2
	exit 2
	;;
esac

materialize_openrc_policy
if [ "$networking_service" = stock ]; then
	install_bunix_openrc_providers
fi
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
	echo "alpine_networking_service=$networking_service"
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
