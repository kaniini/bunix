# riscv64 Linux personality ABI

This document defines the boundary between riscv64 architecture syscall entry
and the shared Bunix Linux personality server.

## Current state

- `a7` carries the syscall number on `ecall`.
- `a0` through `a5` carry Linux syscall arguments.  The current
  `arch_syscall_frame` stores `arg0` through `arg3` as convenience fields and
  also preserves the full `a[0]` through `a[7]` register set.
- `a0` carries the return value back to userspace.
- `sepc` is advanced by 4 after syscall dispatch.
- Negative syscall numbers remain Bunix-native capability syscalls.
- Nonnegative syscall numbers are Linux personality syscalls.
- `tp` is preserved as the riscv64 thread pointer.  Future clone/TLS work must
  update `tp` through architecture state, not through Linux server policy.

The default bringup path now translates the static-musl smoke syscall subset
into shared `LINX` messages handled by `user/linux/main.c`.  The old
`BUNIX_LINUX_RISCV64_SYSCALL` action protocol remains only for the narrower
UART console smoke and should be deleted when that package no longer uses
`user/riscv64-linux/main.c`.

## Target boundary

The riscv64 kernel frontend should do only architecture-specific work:

- Decode the riscv64 syscall number from `a7`.
- Collect up to six arguments from `a0` through `a5`.
- Translate riscv64 syscall numbers to shared `BUNIX_LINUX_*` RPC message
  types.
- Build any architecture-specific user buffers needed for paths, iovecs,
  timespecs, sigsets, signal frames, clone state, or exec vectors.
- Copy scalar or buffer results back to riscv64 userspace.
- Return Linux results in `a0`, using the same negative errno convention
  already used by the x86_64 frontend.
- Terminate or block the current Bunix task only for effects that are truly
  kernel scheduling/VM responsibilities, such as final task exit or page-fault
  recovery.

The shared Linux server should own OS personality semantics:

- Linux PID/process/session/group state.
- File descriptor tables and VFS operations.
- TTY state and terminal input/output semantics.
- Signal state and process-directed signal policy.
- `wait4`, `fork`, `clone`, `execve`, credentials, cwd, umask, sockets, pipes,
  mmap bookkeeping, and proc-facing process metadata.

## Initial syscall map

The first shared-server riscv64 smoke should support at least:

- `read` -> `BUNIX_LINUX_READ`
- `write` -> `BUNIX_LINUX_WRITE`
- `openat` -> `BUNIX_LINUX_OPENAT`
- `close` -> `BUNIX_LINUX_CLOSE`
- `fstat` -> `BUNIX_LINUX_FSTAT`
- `newfstatat` -> `BUNIX_LINUX_NEWFSTATAT`
- `mmap` -> `BUNIX_LINUX_MMAP`
- `munmap` or the existing Bunix task unmap primitive once wired
- `brk` if dynamic riscv64 musl requires it
- `set_tid_address` as a no-op or shared process-thread registration result
- `getpid` -> `BUNIX_LINUX_GETPID`
- `getppid` -> `BUNIX_LINUX_GETPPID`
- `getuid` -> `BUNIX_LINUX_GETUID`
- `geteuid` -> `BUNIX_LINUX_GETEUID`
- `getgid` -> `BUNIX_LINUX_GETGID`
- `getegid` -> `BUNIX_LINUX_GETEGID`
- `gettid` -> `BUNIX_LINUX_GETTID`
- `exit` and `exit_group` -> `BUNIX_LINUX_EXIT_GROUP`

Dynamic-linker coverage must add the syscalls observed from
`ld-musl-riscv64.so.1`, especially `openat(2)` and correct `fstat(2)`
behavior.

## Refactor rule

Do not add more semantics to `user/riscv64-linux/main.c`.  New riscv64 Linux
syscall work should either:

- add a riscv64 frontend translation into the existing shared Linux protocol,
  or
- extend the shared Linux server protocol/implementation when both x86_64 and
  riscv64 need the semantic operation.
