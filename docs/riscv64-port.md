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
- Boot package: `build/riscv64/bootpkg.img`, passed with QEMU `-initrd` and
  discovered through `/chosen/linux,initrd-start` and
  `/chosen/linux,initrd-end` in the FDT.
- Test gate: `make test-boot-riscv64-early`.

The early boot gate currently verifies:

- OpenSBI reaches the Bunix riscv64 entry point.
- FDT memory discovery can read the first memory range.
- Supervisor timer interrupts fire through the riscv64 trap entry.
- The riscv64 kernel-thread context switch primitive can switch away and back.
- The riscv64 generic VM hooks can create an Sv39 page-table root, map a
  user page, translate it for read/write, remove write permission, unmap it,
  and tear down the root through an early arena-backed implementation.
- The native Bunix `ecall` entry contract can decode a synthetic user syscall
  frame, place the return value in `a0`, and advance `sepc`.
- A minimal U-mode probe can execute an `ecall`, receive the expected return
  value, and return through a test-only trap continuation on a kernel stack.
- The riscv64 boot package is visible through the FDT initrd range and starts
  with the expected `BUNIX-RV64-BOOTPKG` header.
- The packaged `abi-smoke.user` payload can be located and validated as an
  ELF64 little-endian RISC-V user image.
- The packaged `abi-smoke.user` payload can have its load segments copied into
  backing pages, mapped at the real Bunix user base `0x400000` with an early
  Sv39 page table, run in U-mode through the riscv64 crt0, and return via the
  native Bunix `exit` syscall.
- The packaged native payload can call the real riscv64 native syscall
  dispatcher for early console writes and `exit`, proving a server-shaped
  userspace payload can report through the native Bunix ABI before poweroff.
- The guest exits through SBI poweroff.

## Boot package

The first riscv64 module/rootfs carrier is a QEMU initrd image.  The host-side
builder `tools/build-riscv64-bootpkg.sh` creates a text-header package with a
module record and the current `abi-smoke.user` payload.  The early kernel
can validate the carrier magic, locate that module record, verify the payload
is an ELF64 RISC-V image, copy loadable segments into backing pages, map them
with a minimal Sv39 page table, build a crt0-compatible stack, enter U-mode at
`0x400000`, and observe native `exit`.

The current payload Sv39 setup is intentionally an early harness: it
identity-maps the supervisor RAM window and maps only a small smoke-program
image window plus one user stack page.  It is enough to prove the real Bunix
user base works, but not a general task address-space implementation.

Riscv64 now also has initial implementations of the generic `arch_vm_*` hooks
for Sv39 page-table roots, map/protect/unmap/translate, and `satp`
activation.  These hooks are backed by a fixed early page-table arena because
the riscv64 path does not yet initialize the generic PMM from FDT memory data.
They are suitable for emulator bringup tests, but scheduler-owned userspace
still needs PMM-backed table allocation, task lifetime integration, and
copyin/copyout before the early payload harness can be removed.

The PMM has been split enough to support a future riscv64 memory provider:
`pmm_init_from_ranges()` can initialize the generic page allocator from
available/reserved physical ranges, while the existing x86_64 Multiboot2 path
collects its boot data and calls that shared initializer.  Riscv64 still needs
to feed FDT memory/initrd/kernel reservations into that API before page tables
and user frames can come from PMM.
The Multiboot2 collector now lives outside the PMM core, so riscv64 can link
the generic allocator without pulling in x86 boot-protocol symbols.

This gives riscv64 a firmware-neutral package handoff separate from
Multiboot2.  Future rootfs images can ride in the same carrier or replace it
with a stricter binary table once the riscv64 bootstrap/proc path exists.

## Native Bunix ABI

Native Bunix syscalls on rv64 use `ecall`:

- `a7`: signed syscall number.  Bunix-native IDs stay negative.
- `a0` through `a3`: first four 64-bit arguments.
- `a0`: return value.
- `sepc`: advanced by 4 before returning from the syscall trap.

The build smoke target `make test-riscv64-user-abi` links a freestanding rv64
user ELF with `user/crt0-riscv64.S` and the shared Bunix syscall wrappers.  It
does not prove runtime U-mode task launch yet.

The early QEMU smoke proves that the trap path can enter U-mode, handle an
`ecall` on a supervisor stack, dispatch a small native syscall subset, and run
one packaged crt0 payload at the real Bunix user base through an early Sv39
mapping.  This is still an early harness rather than full proc/bootstrap
integration: there is no general task VM object, copyin/copyout, or
scheduler-owned user task lifetime in the riscv64 path yet.

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

The current host has riscv64 binutils and QEMU, but Alpine does not ship the
needed riscv64 Linux musl cross compiler.  Use the external musl.cc
`riscv64-linux-musl-cross.tgz` toolchain for the later static riscv64 musl
slice; until that toolchain is installed or vendored, the tree should not add a
pretend riscv64 musl binary target that cannot produce a real libc-linked ELF.

## Hardware Port Gate

Board-specific riscv64 work, including the Banana Pi BPI-F3 port, is blocked
until the QEMU `virt` emulator path has full generic riscv64 userspace bringup:
normal scheduler-owned native task launch, general user VM/copyin/copyout,
proc/bootstrap integration, and a static riscv64 musl/Linux-personality smoke.
Hardware debugging should not start while those generic emulator pieces are
still missing.

## Unsupported features

The initial riscv64 port intentionally does not support:

- SMP or secondary hart startup.
- FPU, vector, or signal context save/restore.
- Native server task launch through proc/bootstrap.
- PMM-backed user page-table management, copyin/copyout, or VM server
  integration.
- Linux signal frames or riscv64-specific Linux syscall parity.
- Dynamic linking and `ld-musl-riscv64.so.1`.
- VirtIO MMIO devices, block storage, networking, or Alpine rootfs boot.
- PCI, USB, ACPI, or board-specific device enumeration.
- External interrupt routing through PLIC or AIA.

These are unsupported by design for the initial port.  Each item needs its own
runtime test before the riscv64 port can claim parity with the x86_64 boot and
userspace path.
