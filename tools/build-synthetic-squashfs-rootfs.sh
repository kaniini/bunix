#!/bin/sh
set -eu

out=${1:?output SquashFS image required}
run_id=$(date -u +%Y%m%dT%H%M%SZ)-$$
stage=${SQUASHFS_ROOTFS_STAGE:-${TMPDIR:-/tmp}/bunix-squashfs-rootfs.$run_id}
root=$stage/root

safe_rm_tree() {
	path=$1
	case "$path" in
	''|/|.|"$HOME")
		echo "refusing unsafe scratch path: $path" >&2
		exit 2
		;;
	esac
	rm -rf "$path"
}

install_file() {
	path=$1
	src=$2
	mode=$3

	mkdir -p "$root/$(dirname "$path")"
	cp "$src" "$root/$path"
	chmod "$mode" "$root/$path"
}

install_dir() {
	path=$1
	mkdir -p "$root/$path"
	chmod 0555 "$root/$path"
}

install_symlink() {
	path=$1
	target=$2

	mkdir -p "$root/$(dirname "$path")"
	ln -s "$target" "$root/$path"
}

busybox=${BUSYBOX:-/bin/busybox}
musl_ldso=${MUSL_LDSO:-/lib/ld-musl-x86_64.so.1}
module_dir=${MODULE_DIR:-build/modules}

long_root=/usr/share/bunix/alpine/very/long/rootfs/path/that/exceeds/the/old/two-hundred-fifty-six-byte/rootfs-entry-limit/path-component-that-forces-the-rootfs-image-format-past-the-old-limit/path-component-that-forces-the-rootfs-image-format-past-the-old-limit/with-extra-components
long_exec=/usr/share/bunix/alpine/very/long/rootfs/path/that/exceeds/the/old/two-hundred-fifty-six-byte/linux-execve-path-limit/path-component-that-forces-the-rootfs-image-format-past-the-old-limit/path-component-that-forces-the-rootfs-image-format-past-the-old-limit/with-extra-components
long_proc=/usr/share/bunix/alpine/very/long/rootfs/path/that/exceeds/the/old/two-hundred-fifty-six-byte/proc-exec-registry-limit/path-component-that-forces-the-rootfs-image-format-past-the-old-limit/path-component-that-forces-the-rootfs-image-format-past-the-old-limit/with-extra-components

safe_rm_tree "$stage"
mkdir -p "$root" "$(dirname "$out")"

install_file /hello.txt modules/hello.txt 0444
install_file /secret.txt modules/secret.txt 0400
install_file /rename-lower.txt modules/nested.txt 0444
install_file /usr/share/bunix/nested/hello.txt modules/nested.txt 0444
install_file "$long_root/hello.txt" modules/nested.txt 0444
install_file "$long_exec/dyn-hello" "$module_dir/dyn-hello.user" 0555
install_file "$long_proc/first" "$module_dir/first.user" 0555

install_file /etc/passwd modules/passwd 0444
install_file /etc/shadow modules/shadow 0400
install_file /etc/group modules/group 0444
install_file /etc/inittab modules/inittab 0444
install_file /etc/execs modules/execs 0444
install_file /etc/spawns modules/spawns 0444
mkdir -p "$root/etc/network"
cat > "$root/etc/network/interfaces" <<'EOF_INTERFACES'
auto lo
iface lo inet loopback
iface lo inet6 loopback

auto eth0
iface eth0 inet dhcp
EOF_INTERFACES
chmod 0444 "$root/etc/network/interfaces"

install_file /bin/shebangtest modules/shebangtest.sh 0555
install_file /bin/shebangloop-a modules/shebangloop-a.sh 0555
install_file /bin/shebangloop-b modules/shebangloop-b.sh 0555
install_file /bin/shebangbad modules/shebangbad.sh 0555
install_file /bin/root-mount-soak modules/root-mount-soak.sh 0555
install_file /lib/ld-musl-x86_64.so.1 "$musl_ldso" 0555

for name in first alloctest ipcstress login lxtest getdentstest vforkstress \
	execok readbig mmapbig mmaphuge execbig phdrstress musl-hello dyn-hello \
	fputest iovtest fchmodattest waitpgidtest execlongtest auxidtest \
	pathmaxtest patherrtest statidtest fcntllocktest sysracetest \
	signaltest ttytest faulttest lowmemtest schedstress schedbench uptimetest nettest \
	privtest; do
	install_file "/bin/$name" "$module_dir/$name.user" 0555
done
install_file /sbin/bunix-udhcpc-script "$module_dir/bunix-udhcpc-script.user" 0555
install_file /bin/busybox "$busybox" 0555

for dir in dev home/kaniini root tmp run mnt sys var/tmp var/run; do
	install_dir "/$dir"
done

install_symlink /usr/share/bunix/long-target-link "$long_root/hello.txt"
install_symlink /lib/libc.musl-x86_64.so.1 /lib/ld-musl-x86_64.so.1
install_symlink /sbin/init /bin/busybox
install_symlink /usr/share/udhcpc/default.script /sbin/bunix-udhcpc-script
for path in /bin/sh /usr/bin/env /bin/dmesg /bin/cat /bin/stat /bin/uptime \
	/bin/sleep /bin/ls /bin/mount /bin/umount /bin/free /bin/df /bin/ps \
	/bin/top /bin/id /bin/kill /bin/echo /bin/env /bin/stty /bin/pwd \
	/bin/ping /sbin/ifup /sbin/ifdown /sbin/ip /sbin/route /sbin/udhcpc \
	/bin/poweroff /sbin/poweroff /sbin/halt /sbin/reboot; do
	install_symlink "$path" /bin/busybox
done

mksquashfs "$root" "$out" -noappend -no-compression -no-fragments \
	-no-exports -no-xattrs -all-root -root-mode 0755 -b 128K \
	-repro-time 0 >/dev/null

echo "synthetic squashfs image ok out=$out stage=$stage"
