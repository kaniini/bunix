#!/bin/sh
set -eu

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
apk_packages=${ALPINE_ROOTFS_PACKAGES:-alpine-baselayout busybox musl openrc}
rootfs_format=${ROOTFS_IMAGE_FORMAT:-squashfs}
login=${LOGIN_MODULE:-build/modules/login.user}
statidtest=${STATIDTEST_MODULE:-build/modules/statidtest.user}

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

rm -rf "$stage"
mkdir -p "$root" "$(dirname "$out")" "$artifact_dir" "$apk_cache"
apk_cache=$(CDPATH= cd "$apk_cache" && pwd)

# shellcheck disable=SC2086
if ! apk --root "$root" --initdb --allow-untrusted \
	--cache-dir "$apk_cache" \
	--repositories-file "$repositories" \
	add $apk_packages >"$apk_log" 2>&1; then
	mv "$apk_log" "$apk_plain_log"
	rm -rf "$root"
	mkdir -p "$root"
	# shellcheck disable=SC2086
	if ! apk --root "$root" --initdb --usermode --allow-untrusted \
		--cache-dir "$apk_cache" \
		--repositories-file "$repositories" \
		add $apk_packages >"$apk_log" 2>&1; then
		cat "$apk_plain_log" >&2
		cat "$apk_log" >&2
		exit 1
	fi
fi

chmod -R u+w "$root"
mkdir -p "$root/bin" "$root/etc" "$root/etc/init.d" "$root/etc/runlevels/default" \
	"$root/etc/runlevels/boot" "$root/proc" "$root/sys" "$root/dev" "$root/run" \
	"$root/tmp" "$root/root" "$root/var/tmp"
rm -f "$root/bin/login"
cp "$login" "$root/bin/login"
chmod 0555 "$root/bin/login"
cp "$statidtest" "$root/bin/statidtest"
chmod 0555 "$root/bin/statidtest"

merge_account_file "$root/etc/passwd" modules/passwd
merge_account_file "$root/etc/shadow" modules/shadow
merge_account_file "$root/etc/group" modules/group
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
::wait:/sbin/openrc boot
::wait:/sbin/openrc default
ttyS0::respawn:/bin/login
::shutdown:/sbin/openrc shutdown
EOF_INITTAB

cat > "$root/etc/init.d/bunix-login" <<'EOF_LOGIN_SERVICE'
#!/sbin/openrc-run
description="Bunix native login service"
command="/bin/login"
command_background="no"
EOF_LOGIN_SERVICE
chmod 0755 "$root/etc/init.d/bunix-login"
ln -sf /etc/init.d/bunix-login "$root/etc/runlevels/default/bunix-login"

find "$root/var/cache/apk" -type f -delete 2>/dev/null || true

{
	echo "source=apk"
	echo "apk_cache=$apk_cache"
	echo "repositories=$repositories"
	echo "packages=$apk_packages"
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
