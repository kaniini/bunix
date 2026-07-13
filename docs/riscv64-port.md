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
- Alpine shell gate: `make test-boot-riscv64-alpine`, using
  `build/riscv64/bootpkg-alpine.img`.

The early boot gate currently verifies required boot evidence plus opt-in
diagnostic/self-test coverage.  The default `test-boot-riscv64-early`
package carries `riscv64-diag riscv64-selftest`; the Alpine package does not,
so Alpine boot is not forced to print hardware diagnostics or run destructive
bringup self-tests.

- OpenSBI reaches the Bunix riscv64 entry point.
- FDT memory discovery can read the first memory range.
- Under `riscv64-selftest`, supervisor timer interrupts fire through the
  riscv64 trap entry.
- Under `riscv64-selftest`, the riscv64 kernel-thread context switch
  primitive can switch away and back.
- The riscv64 PMM can initialize from QEMU FDT memory data while reserving
  the low firmware window, kernel image, boot package/initrd, and FDT blob.
- Under `riscv64-selftest`, the riscv64 generic VM hooks can create an Sv39 page-table root, map a
  user page, translate it for read/write, remove write permission, unmap it,
  and recursively tear down PMM-backed page-table pages.
- Under `riscv64-selftest`, the native Bunix `ecall` entry contract can decode a synthetic user syscall
  frame, place the return value in `a0`, and advance `sepc`.
- Under `riscv64-selftest`, a minimal U-mode probe can execute an `ecall`, receive the expected return
  value, and return through a test-only trap continuation on a kernel stack.
- The riscv64 boot package is visible through the FDT initrd range and starts
  with the expected `BUNIX-RV64-BOOTPKG` header.
- The packaged `abi-smoke.user` payload can be located and validated as an
  ELF64 little-endian RISC-V user image.
- Under `riscv64-selftest`, low-level riscv64 user copy helpers can copy from and to the active user
  address space with `sstatus.SUM`, validate Bunix user ranges, and reject
  invalid ranges.
- The packaged `abi-smoke.user` payload can be loaded by the generic ELF
  loader, launched through the normal module-server path as a scheduler-owned
  task, run in U-mode through the riscv64 crt0, and exit by terminating and
  reaping its scheduler thread and task.
- The packaged native payload can call the real riscv64 native syscall
  dispatcher for early console writes and `exit`, proving a server-shaped
  userspace payload can report through the native Bunix ABI before poweroff.
- A static riscv64 musl hello payload installed in the smoke squashfs as
  `/bin/musl-hello` can be spawned and waited through proc, preserve absolute
  `argv[0]`, use an initial stack with argv/envp plus an `AT_NULL` aux vector
  terminator, send riscv64 Linux-number syscalls through a riscv64 syscall
  frontend to the shared userspace `linux` server, register as a Linux
  process, write through the shared Linux server, and exit cleanly.
- A static rv64 syscall smoke payload installed in the smoke squashfs as
  `/bin/rv64-syscall-smoke` can be spawned and waited through proc, issue raw
  Linux `ecall`s for process identity and credential syscalls, write its
  result through the shared Linux server, and exit cleanly before bootstrap
  powers off the test VM.
- A dynamic rv64 musl hello payload can run from the mounted squashfs root as
  `/bin/dyn-hello`, loading `/lib/ld-musl-riscv64.so.1` through proc, VFS,
  squashfs, and the shared Linux server.  The gate requires
  `musl hello argc=1 argv0=/bin/dyn-hello` and
  `bootstrap-riscv64: dyn-hello ok`.
- The Alpine boot gate mounts a riscv64 Alpine squashfs, runs `/bin/dyn-hello`,
  spawns Alpine `/bin/sh -c 'echo BUNIX_RISCV64_ALPINE_SH_OK'` through
  proc/Linux, and exits through SBI poweroff after the marker appears.
- The guest exits through SBI poweroff.

## Boot package

The first riscv64 module/rootfs carrier is a QEMU initrd image.  The host-side
builder `tools/build-riscv64-bootpkg.sh` creates a text-header package with
module records and payloads.  The old `OUT MODULE [NAME]` invocation remains
valid, and the builder also accepts repeated `MODULE NAME` pairs for ordered
module/data payloads plus an optional `cmdline` header.  The default QEMU boot
carrier contains the `disk0` smoke squashfs plus `names`, `user`,
`bootstrap`, `abi-smoke.user`, and `linux`, while
`test-riscv64-bootpkg` also builds a multi-record carrier to prove host-side
ordering.  The early kernel can parse the package command line through the
shared `kernel_cmdline_configure()` path used by x86_64, validate the carrier
magic, locate module records, verify RISC-V user ELF payloads, register them
with the generic module registry, launch them with `server_launch_module()`,
build a crt0/libc-compatible stack, enter U-mode, and observe native or
Linux-personality-server-approved `exit` through scheduler task teardown.

Riscv64 now also has initial implementations of the generic `arch_vm_*` hooks
for Sv39 page-table roots, map/protect/unmap/translate, and `satp`
activation.  These hooks allocate and free page-table pages through the
generic PMM after the emulator kernel initializes PMM from FDT memory data.
They are suitable for emulator bringup tests, but scheduler-owned userspace
still needs broader proc/bootstrap integration before it can match the x86_64
server graph.

Riscv64 has low-level active-address-space copy helpers for early native
syscalls: `arch_user_copy_from()` and `arch_user_copy_to()` validate the Bunix
user range, set `sstatus.SUM` only around the byte copy, restore the previous
status, and return an error for invalid ranges.  The early native console
syscall now uses those helpers instead of directly dereferencing user memory.
The copy smoke now runs against a temporary generic VM server space; broader
server/RPC syscall paths still need to route normal copies through task
`vm_space` objects so unmapped-page failures are reported through
`vm_read_user()`/`vm_write_user()` rather than by direct access.

The riscv64 kernel also has an SBI-backed implementation of the `console.h`
surface for early emulator bringup.  It supports output and `console_printf()`
formatting over legacy SBI putchar so generic scheduler/server code can link
without the x86 COM/VGA console.  Input and dmesg-ring reads are stubbed empty
until the real console server and TTY stack are brought up on riscv64.

Some generic kernel code on the scheduler/proc path is now architecture-neutral
enough to run the native module smoke on riscv64: `kernel/sched.c` uses arch
interrupt/idle hooks instead of x86 `cli`/`sti`/`hlt`, and `kernel/vm.c` logs
`arch_vm_root()` instead of naming x86 `cr3`.  Full proc/bootstrap server graph
parity remains open.

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
The packaged `abi-smoke.user` payload now launches through the normal generic
module path: the riscv64 early boot path records the boot-package module,
calls `server_launch_module("abi-smoke.user")`, runs the scheduler, enters
U-mode from the module server thread, handles native `exit` by terminating the
thread, reaps the task, and then powers off.  The old static mapped-image
early harness for that payload has been removed.

The default riscv64 boot package now carries the shared `names` and `user`
servers, and the generic initial boot-module starter launches `names` before
the riscv64 bootstrap.  The bootstrap then launches `user` with console and
names send capabilities before launching the shared Linux server as module
`linux`.
The current smoke checks the `names` launch marker plus real
`names: register name=bootstrap` and `names: register name=user` service
effects.  The package still omits the normal console server, so ordinary
`bunix_console_log()` messages queued by `names` and `user` are not drained as
visible `names: online` or `user: online` text yet.  The riscv64 native
syscall frontend implements the shared-buffer create/read/write syscalls
needed by normal userspace servers on that path.

The UART boot package remains a narrower SBI/UART smoke carrier for now.
Including the shared `user` server there exposed a UART-only timeout while the
names service was replying to the user service registration.  That debugging
belongs with the boot-test/server-graph split, not with the main emulator path
that is preparing for Alpine userspace.

The generic ELF loader now asks the architecture for its ELF machine ID rather
than hardcoding x86_64.  The riscv64 early smoke loads `abi-smoke.user` through
`elf_load_user_image()` into a temporary generic VM server space and requires
the `elf: riscv64 loader` marker before launching the same module through the
normal scheduler-owned path.

Riscv64 task address spaces currently include supervisor-only identity
mappings for the QEMU RAM window.  This preserves the same kernel execution
model as the no-MMU early path while a task VM space is active, allowing the
scheduler, module-launch thread, trap entry, and syscall handling to run from
inside a user task address space.  This should later become a tighter shared
kernel mapping policy, but it is enough for emulator userspace parity.

The early riscv64 emulator path now initializes those generic services after
PMM bringup and proves scheduler-owned kernel thread lifetime: it creates a
kernel thread through `thread_create()`, runs it with `sched_run()`, and
observes it exit through `thread_exit()`.  The same smoke later launches
`abi-smoke.user` as a scheduler-owned U-mode task through the generic module
path.

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
Multiboot2 while preserving the same kernel command-line semantics.  Future
rootfs images can ride in the same carrier or replace it with a stricter
binary table once the riscv64 bootstrap/proc path exists.

## Native Bunix ABI

Native Bunix syscalls on rv64 use `ecall`:

- `a7`: signed syscall number.  Bunix-native IDs stay negative.
- `a0` through `a3`: first four 64-bit arguments.
- `a0`: return value.
- `sepc`: advanced by 4 before returning from the syscall trap.

The current riscv64 native dispatcher is table-driven and uses the same
negative Bunix IDs as x86_64 for the implemented subset: `exit`,
`timer_ticks`, `launch_module`, `port_create`, `ipc_send`, `ipc_recv`,
`ipc_call`, `handle_close`, `boot_module_read`, `clock_monotonic_ns`,
`sleep_ns`, the low-level task exec primitives used by proc, buffer
create/read/write, `cmdline_has`, `task_id`, `early_console_write`,
`early_console_log`, and `machine_power`.  The QEMU smoke proves the U-mode
`abi-smoke.user` payload
can call `task_id(0)`, timer ticks, monotonic clock, early console write/log,
and `exit` through that path.  `boot_module_read` is present for modules with
attached data payloads, but `abi-smoke.user` does not currently have one.  The
IPC subset is enough for the riscv64 `linux` server to receive scalar `LINX`
messages and reply through a reply port; capability-bearing messages remain a
broader server-graph requirement.

The build smoke target `make test-riscv64-user-abi` links a freestanding rv64
user ELF with `user/crt0-riscv64.S` and the shared Bunix syscall wrappers.  It
does not boot QEMU by itself; runtime launch is covered by
`make test-boot-riscv64-early`.

The early QEMU smoke proves that the trap path can enter U-mode, handle an
`ecall` on a supervisor stack, dispatch a small native syscall subset, and run
one packaged crt0 payload at the real Bunix user base through a scheduler-owned
task and generic VM space.  The main QEMU path now also launches the shared
time, user, proc, VFS, squashfs, and Linux servers; fuller boot packaging and
cleanup are tracked by the follow-on riscv64 refactor explorations.

## Linux personality slice

The first riscv64 Linux-personality target began with static rv64 musl
userspace and now reaches a controlled dynamic-linker and Alpine shell
milestone.  The emulator builds the static
`user/musl-hello/main.c` ELF and the syscall-heavy
`user/riscv64-syscall-smoke/main.c` ELF with host clang against the musl.cc
sysroot, stages them in the smoke squashfs as `/bin/musl-hello` and
`/bin/rv64-syscall-smoke`, spawns and waits them through proc, and
sends their nonnegative Linux syscalls through a riscv64 syscall frontend to
the shared userspace Linux server built from `user/linux/main.c`.
`make test-boot-riscv64-early` now proves the shared server path by requiring
`names: register name=linux`, `bootstrap-riscv64: syscall-smoke ok`,
`rv64 syscall smoke ok`, `bootstrap-riscv64: musl-hello ok`, the musl hello
serial marker, and
`linux-riscv64: exit_group status=0`.

The riscv64 syscall frontend maps the observed static hello surface to shared
Linux server messages: riscv64 `write` becomes shared `BUNIX_LINUX_WRITE`,
`exit` and `exit_group` become shared `BUNIX_LINUX_EXIT_GROUP`, and
`set_tid_address` is answered by the current scheduler thread ID.  The
syscall-heavy static smoke also maps riscv64 `getpid`, `getppid`, `getuid`,
`geteuid`, `getgid`, `getegid`, and `gettid` to the shared Linux server's
process and credential messages.  During the `riscv64-bootpkg-test` smoke,
stdout/stderr writes are also mirrored to the early SBI console because the
package still omits the real console server.  That early-console mirror is a
smoke-test aid, not the future Alpine console model.

The old `user/riscv64-linux/main.c` stub and its private action protocol have
been removed.  The narrower UART smoke package no longer launches Linux at
all; it proves UART early console output plus native user launch and exits
through the native machine poweroff syscall.  All riscv64 Linux-personality
boot paths use the shared Linux server built from `user/linux/main.c`.

The shared `proc` server now has initial riscv64 build coverage.  When built
for riscv64, it accepts `EM_RISCV` ELF images instead of x86_64 images, and
the riscv64 native syscall frontend exposes the low-level task primitives
that proc uses to create, map, write, clear, and start exec images.  The
default boot package now carries shared `time` and `proc` modules; the
riscv64 bootstrap launches time, waits for `BUNIX_SERVICE_TIME` through
names, and then launches proc with console, names, and time capabilities.
Proc now executes `/bin/dyn-hello`, `/bin/rv64-syscall-smoke`, and
`/bin/musl-hello` from a mounted squashfs root.  The dynamic musl loader
exercises the Linux/VFS `openat(2)` and `fstat(2)` path, and the static smoke
programs use proc wait as the test completion primitive instead of a
kernel-side poweroff latch.

The first riscv64 VFS/rootfs service graph is now present in the default boot
package.  The host builds shared-server riscv64 variants for block, VFS,
tmpfs, squashfs, and unionfs:

```
make test-riscv64-fs-server-build
```

The smoke boot package carries a tiny squashfs image as `disk0`, plus `block`,
`vfs`, and `squashfs` modules.  The image contains `/bin/dyn-hello`,
`/bin/rv64-syscall-smoke`, `/bin/musl-hello`, `/lib/ld-musl-riscv64.so.1`,
and a small static `/etc` payload.  The riscv64 bootstrap launches those
servers, waits for names registrations, sets the squashfs mount path to `/`,
and mounts it at `/` through VFS.  `make test-boot-riscv64-early` now requires
`bootstrap-riscv64: block ready`, `bootstrap-riscv64: vfs ready`,
`bootstrap-riscv64: squashfs ready`, and
`bootstrap-riscv64: rootfs mounted` before the static Linux smoke tasks run.
The default early target still uses a synthetic rootfs smoke.  The separate
Alpine target swaps in the riscv64 Alpine squashfs artifact and proves a shell
command under Alpine userspace.

The generic initial stack seeds `argc`, `argv[0]`, a null argv terminator, a
null envp terminator, and an `AT_NULL` auxv terminator.  The auxv terminator is
required even for the simple static musl smoke because musl startup walks past
envp into auxv before reaching `main()`.

Full OpenRC boot parity and interactive login are later riscv64 slices.  The
host can now prepare a minimal riscv64 Alpine squashfs artifact with:

```
make test-riscv64-alpine-rootfs
```

That target uses `APK_ARCH=riscv64`, builds a pure Alpine rootfs without
x86_64 Bunix user overlays, and records `ttyS0::respawn:/bin/sh` as a
temporary init command.

The host can also prepare the first dynamic-linker artifacts:

```
make test-riscv64-dynamic-linker-artifacts
```

That target builds `build/riscv64/modules/dyn-hello.user` with interpreter
`/lib/ld-musl-riscv64.so.1` and stages the musl loader content as
`build/riscv64/modules/ld-musl-riscv64.so.1`.  The riscv64 Alpine rootfs
builder overlays those files into `/bin` and `/lib` when they are supplied.
Runtime dynamic-linker coverage is now part of `make test-boot-riscv64-early`
and `make test-boot-riscv64-alpine`.  The Alpine target additionally proves:

```sh
make test-boot-riscv64-alpine
```

The required serial markers are `bootstrap-riscv64: rootfs mounted`,
`bootstrap-riscv64: linux ready`,
`musl hello argc=1 argv0=/bin/dyn-hello`,
`BUNIX_RISCV64_ALPINE_SH_OK`, `bootstrap-riscv64: alpine sh ok`, and SBI
poweroff.

The current host has riscv64 binutils and QEMU, but Alpine does not ship the
needed riscv64 Linux musl cross compiler.  Use `make
riscv64-muslcc-toolchain` to install the external musl.cc
`riscv64-linux-musl-cross.tgz` toolchain under ignored
`build/toolchains/riscv64-linux-musl-cross`, or set
`RISCV64_MUSLCC_TARBALL=/path/to/riscv64-linux-musl-cross.tgz` to use a
predownloaded archive.  Until that toolchain is installed or configured, the
tree should not add a pretend riscv64 musl binary target that cannot produce a
real libc-linked ELF.  The musl.cc driver in that archive is an i386 binary
on this host, so the Bunix build uses host clang plus the musl.cc sysroot,
crt objects, libc, and libgcc support directory to produce the static
`build/riscv64/modules/musl-hello.user` ELF.  The riscv64 boot package carries
that payload under the package name `/bin/musl-hello`; the current static
hello execution path goes through the userspace riscv64 Linux server described
above.

## Hardware Port Scope

Board-specific riscv64 work is out of scope for the current project plan.
The emulator now has proc/bootstrap, shared-Linux, dynamic-loader, and Alpine
shell smoke coverage, but full OpenRC, interactive login, VirtIO, and the
generic driver model still need enough quality work that hardware-specific
artifact generation would be premature.

## Unsupported features

The initial riscv64 port intentionally does not support:

- SMP or secondary hart startup.
- FPU, vector, or signal context save/restore.
- Full OpenRC boot and interactive login on riscv64.
- VM server integration beyond bootstrap-created task address spaces.
- Task-owned user-copy integration for unmapped-page fault reporting through
  `vm_read_user()`/`vm_write_user()`.
- Console input and dmesg-ring reads from the riscv64 SBI console shim.
- Linux signal frames or riscv64-specific Linux syscall parity beyond the
  current smoke syscall surface.
- VirtIO MMIO devices, block storage beyond the boot-package-backed block
  server, networking, or persistent disks.
- PCI, USB, ACPI, or board-specific device enumeration.
- External interrupt routing through PLIC or AIA.

These are unsupported by design for the initial port.  Each item needs its own
runtime test before the riscv64 port can claim parity with the x86_64 boot and
userspace path.
