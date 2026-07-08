# Architecture Boundary

This document defines the intended boundary between `arch/*`, generic kernel
code, and userspace servers.  The point is to keep future ports cheap without
hiding real architecture differences.

## `arch/*`

Architecture directories own hardware and ABI facts:

- boot entry, firmware handoff, boot-package or Multiboot2 discovery;
- trap, interrupt, syscall, and exception entry/return code;
- low-level CPU context switching and user-mode transition mechanics;
- interrupt-controller, timer, SMP startup, and poweroff/reset primitives;
- MMU page-table formats and TLB shootdown/local flush mechanics;
- active-address-space user copy helpers;
- architecture syscall ABI decoding and Linux syscall-number translation.

Architecture code may marshal register-layout-specific Linux syscall arguments
and copy user buffers, but it should not own Linux process, file descriptor,
path, session, or credential policy.  Those belong in the userspace Linux
personality server.

## Generic Kernel

Generic kernel code owns mechanisms that are independent of CPU ISA:

- task/thread objects, scheduling policy, IPC ports/messages, handles, and
  capabilities;
- VM space and region bookkeeping above architecture page-table operations;
- PMM range management after firmware-specific range collection is complete;
- boot module records after architecture boot code has normalized the carrier;
- kernel dmesg/console routing surfaces;
- native Bunix syscall semantics that are not register-layout-specific.

Generic code should depend on small `arch_*` hooks for CPU-specific work rather
than using `#ifdef` branches for each architecture.

## Userspace Servers

Userspace servers own policy and long-lived OS personalities:

- proc owns Bunix process lifetime and executable loading policy;
- linux owns Linux PID/session/fd/credential/syscall semantics;
- VFS and mounted translators own filesystem namespace and file semantics;
- user owns user/group/session identity data;
- bus and driver servers should own hardware enumeration policy once privileged
  hardware operations are exposed through narrow kernel capabilities.

Servers should use shared native Bunix RPC protocols where possible.  If a new
architecture needs the same semantic operation, prefer extending the shared
protocol/server implementation over adding an architecture-private server.

## Accepted Differences

These differences are architecture contracts, not cleanup targets:

- x86_64 boots through EFI/GRUB Multiboot2; riscv64 QEMU `virt` boots through
  OpenSBI plus an FDT initrd boot package.
- x86_64 and riscv64 have different trap frames, syscall registers, thread
  switch code, MMU formats, interrupt controllers, timers, and shutdown paths.
- Linux syscall numbers and register ABI tables are architecture-specific even
  when the resulting `LINX` protocol operation is shared.

## Cleanup Targets

These differences are accidental bringup duplication and should be collapsed:

- duplicated boot-module sequencing after the architecture has normalized boot
  records;
- duplicated Linux syscall forwarding machinery where only the syscall-number
  table or user-buffer shape differs;
- server module build rules that differ only by target triple, crt0 object, or
  linker mode;
- one-off freestanding runtime helpers in individual userspace servers;
- QEMU harness behavior such as serial log capture, timeout handling, marker
  matching, and poweroff expectations.
