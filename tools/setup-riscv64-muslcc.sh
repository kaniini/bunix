#!/bin/sh
set -eu

prefix=${1:-build/toolchains/riscv64-linux-musl-cross}
url=${RISCV64_MUSLCC_URL:-https://musl.cc/riscv64-linux-musl-cross.tgz}
tarball=${RISCV64_MUSLCC_TARBALL:-}
gcc=$prefix/bin/riscv64-linux-musl-gcc
parent=$(dirname "$prefix")
archive=$parent/riscv64-linux-musl-cross.tgz

if [ -x "$gcc" ]; then
	printf '%s\n' "$gcc"
	exit 0
fi

mkdir -p "$parent"
if [ -z "$tarball" ]; then
	if command -v curl >/dev/null 2>&1; then
		curl -L -o "$archive.tmp" "$url"
	elif command -v wget >/dev/null 2>&1; then
		wget -O "$archive.tmp" "$url"
	else
		echo "missing curl or wget for $url" >&2
		exit 1
	fi
	mv "$archive.tmp" "$archive"
	tarball=$archive
fi

tar -xzf "$tarball" -C "$parent"
if [ ! -x "$gcc" ]; then
	echo "missing $gcc after extracting $tarball" >&2
	exit 1
fi

printf '%s\n' "$gcc"
