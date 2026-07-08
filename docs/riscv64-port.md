# riscv64 port notes

This document tracks the initial riscv64 bringup contract.  It is deliberately
not a parity promise with the x86_64 port yet; it names the current boot path,
the next Linux-personality slice, and the unsupported areas that must remain
explicit while the port is young.

## Current boot target

- Machine: `qemu-system-riscv64 -machine virt`.
- Firmware handoff: QEMU's default OpenSBI path loading the Bunix supervisor
  ELF with `-kernel`.
- Kernel image: `build/riscv64/bunixos.kernel`.
- Test gate: `make test-boot-riscv64-early`.

The early boot gate currently verifies:

- OpenSBI reaches the Bunix riscv64 entry point.
- FDT memory discovery can read the first memory range.
- Supervisor timer interrupts fire through the riscv64 trap entry.
- The riscv64 kernel-thread context switch primitive can switch away and back.
- The native Bunix `ecall` entry contract can decode a synthetic user syscall
  frame, place the return value in `a0`, and advance `sepc`.
- The guest exits through SBI poweroff.

## Native Bunix ABI

Native Bunix syscalls on rv64 use `ecall`:

- `a7`: signed syscall number.  Bunix-native IDs stay negative.
- `a0` through `a3`: first four 64-bit arguments.
- `a0`: return value.
- `sepc`: advanced by 4 before returning from the syscall trap.

The build smoke target `make test-riscv64-user-abi` links a freestanding rv64
user ELF with `user/crt0-riscv64.S` and the shared Bunix syscall wrappers.  It
does not prove runtime U-mode task launch yet.

## Linux personality slice

The first riscv64 Linux-personality target is a static rv64 musl hello program,
not BusyBox or dynamic linking.  That slice should prove the loader and Linux
syscall ABI with the smallest executable that still uses libc startup:

1. Build or import a static `riscv64` musl ELF for `user/musl-hello/main.c`.
2. Teach the riscv64 module/rootfs packaging path to carry that ELF as
   `/bin/musl-hello`.
3. Load the ELF through the existing proc/VFS/server path once native user task
   launch is available on riscv64.
4. Dispatch non-negative riscv64 Linux syscall numbers through the Linux
   personality server.  Do not reuse x86_64 Linux syscall numbers.
5. Support only the static-hello syscall set at first: process exit, write,
   brk or mmap as required by musl startup, aux vector setup, and the minimal
   file/proc reads used by the existing hello test.
6. Add a QEMU smoke that runs `/bin/musl-hello`, checks the expected serial or
   dmesg marker, and ends in guest poweroff.

Dynamic linking, Alpine rootfs parity, and BusyBox are later riscv64 slices.

The current host has riscv64 binutils and QEMU, but no riscv64 musl compiler
wrapper or libc headers/libraries in the cross sysroot.  Until those inputs are
installed or vendored, the tree should not add a pretend riscv64 musl binary
target that cannot produce a real libc-linked ELF.

## Unsupported features

The initial riscv64 port intentionally does not support:

- SMP or secondary hart startup.
- FPU, vector, or signal context save/restore.
- Native U-mode task entry with a kernel-stack trap transition.
- User page-table activation, copyin/copyout, or VM server integration.
- Linux signal frames or riscv64-specific Linux syscall parity.
- Dynamic linking and `ld-musl-riscv64.so.1`.
- VirtIO MMIO devices, block storage, networking, or Alpine rootfs boot.
- PCI, USB, ACPI, or board-specific device enumeration.
- External interrupt routing through PLIC or AIA.

These are unsupported by design for the initial port.  Each item needs its own
runtime test before the riscv64 port can claim parity with the x86_64 boot and
userspace path.
