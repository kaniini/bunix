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
- The riscv64 PMM can initialize from QEMU FDT memory data while reserving
  the low firmware window, kernel image, boot package/initrd, and FDT blob.
- The riscv64 generic VM hooks can create an Sv39 page-table root, map a
  user page, translate it for read/write, remove write permission, unmap it,
  and recursively tear down PMM-backed page-table pages.
- The native Bunix `ecall` entry contract can decode a synthetic user syscall
  frame, place the return value in `a0`, and advance `sepc`.
- A minimal U-mode probe can execute an `ecall`, receive the expected return
  value, and return through a test-only trap continuation on a kernel stack.
- The riscv64 boot package is visible through the FDT initrd range and starts
  with the expected `BUNIX-RV64-BOOTPKG` header.
- The packaged `abi-smoke.user` payload can be located and validated as an
  ELF64 little-endian RISC-V user image.
- Low-level riscv64 user copy helpers can copy from and to the active user
  address space with `sstatus.SUM`, validate Bunix user ranges, and reject
  invalid ranges.
- The packaged `abi-smoke.user` payload can have its load segments copied into
  backing pages, mapped at the real Bunix user base `0x400000` through the
  generic riscv64 `arch_vm_*` hooks, run in U-mode through the riscv64 crt0,
  and return via the native Bunix `exit` syscall.
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
through a generic riscv64 VM space, build a crt0-compatible stack, enter U-mode
at `0x400000`, and observe native `exit`.

The current payload launcher is still an early harness, but its address space
is built through the generic riscv64 VM hooks: it identity-maps the supervisor
RAM window needed by the early kernel/trap path and maps only a small
smoke-program image window plus one user stack page.  It is enough to prove
the real Bunix user base works, but not a general task address-space
implementation.

Riscv64 now also has initial implementations of the generic `arch_vm_*` hooks
for Sv39 page-table roots, map/protect/unmap/translate, and `satp`
activation.  These hooks allocate and free page-table pages through the
generic PMM after the emulator kernel initializes PMM from FDT memory data.
They are suitable for emulator bringup tests, but scheduler-owned userspace
still needs task lifetime integration before the early payload harness can be
removed.

Riscv64 has low-level active-address-space copy helpers for early native
syscalls: `arch_user_copy_from()` and `arch_user_copy_to()` validate the Bunix
user range, set `sstatus.SUM` only around the byte copy, restore the previous
status, and return an error for invalid ranges.  The early native console
syscall now uses those helpers instead of directly dereferencing user memory.
Full scheduler-owned task launch still needs to route normal syscall/RPC
copies through task `vm_space` objects so unmapped-page failures are reported
through `vm_read_user()`/`vm_write_user()` rather than by direct access.

The riscv64 kernel also has an SBI-backed implementation of the `console.h`
surface for early emulator bringup.  It supports output and `console_printf()`
formatting over legacy SBI putchar so generic scheduler/server code can link
without the x86 COM/VGA console.  Input and dmesg-ring reads are stubbed empty
until the real console server and TTY stack are brought up on riscv64.

Some generic kernel code on the scheduler/proc path is now architecture-neutral
enough to compile for riscv64: `kernel/sched.c` uses arch interrupt/idle hooks
instead of x86 `cli`/`sti`/`hlt`, and `kernel/vm.c` logs `arch_vm_root()`
instead of naming x86 `cr3`.  These are compile gates only; the normal
scheduler/proc/bootstrap path is not linked into the riscv64 kernel yet.

The riscv64 kernel now links the core generic service support code needed by
that path: buffer, ELF, IPC, names, scheduler, slab, server, timer, generic VM,
and the in-kernel VM server.  VM initialization is boot-protocol-neutral, with
the x86 boot path initializing Multiboot PMM before calling generic `vm_init()`.
Boot-module recording is also generic: `kernel/server.c` accepts name/start/end
module records, while x86 Multiboot enumeration lives in
`kernel/server_multiboot2.c`.  The riscv64 early emulator path now initializes
the generic boot-module registry and feeds package `module` records into
`server_record_boot_module()`.  The current smoke records `abi-smoke.user` as
a known native module and requires the `module: riscv64 registered` marker.
The packaged payload still launches through the early harness until
scheduler-owned U-mode launch is in place.

The generic ELF loader now asks the architecture for its ELF machine ID rather
than hardcoding x86_64.  The riscv64 early smoke loads `abi-smoke.user` through
`elf_load_user_image()` into a temporary generic VM server space and requires
the `elf: riscv64 loader` marker before falling back to the old early harness
to execute the payload.  Scheduler-owned U-mode launch is still the next
runtime ownership step.

The early riscv64 emulator path now initializes those generic services after
PMM bringup and proves scheduler-owned kernel thread lifetime: it creates a
kernel thread through `thread_create()`, runs it with `sched_run()`, and
observes it exit through `thread_exit()`.  This is not yet scheduler-owned
U-mode launch; the `abi-smoke.user` payload still runs through the early
payload launcher.

The PMM has been split enough to support multiple boot protocols:
`pmm_init_from_ranges()` initializes the generic page allocator from
available/reserved physical ranges, the x86_64 Multiboot2 path collects its
boot data and calls that shared initializer, and the riscv64 QEMU `virt` path
now feeds FDT memory plus firmware/kernel/initrd/FDT reservations into the
same API.
The Multiboot2 collector now lives outside the PMM core, so riscv64 can link
the generic allocator without pulling in x86 boot-protocol symbols.
Generic spinlocks have also been moved onto architecture interrupt
save/restore hooks; riscv64 provides `sstatus.SIE` save/restore semantics so
generic locking code can compile for the emulator path.
The PMM core is also console-independent now, so linking it for riscv64 does
not require the current x86-specific console implementation.

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
- Scheduler-owned U-mode task launch; only a scheduler-owned kernel-thread
  smoke runs on riscv64 today.
- VM server integration and normal task address-space lifetime wiring.
- Task-owned user-copy integration for unmapped-page fault reporting through
  `vm_read_user()`/`vm_write_user()`.
- Console input and dmesg-ring reads from the riscv64 SBI console shim.
- Linux signal frames or riscv64-specific Linux syscall parity.
- Dynamic linking and `ld-musl-riscv64.so.1`.
- VirtIO MMIO devices, block storage, networking, or Alpine rootfs boot.
- PCI, USB, ACPI, or board-specific device enumeration.
- External interrupt routing through PLIC or AIA.

These are unsupported by design for the initial port.  Each item needs its own
runtime test before the riscv64 port can claim parity with the x86_64 boot and
userspace path.
