# bunixos

An experimental microkernel-oriented operating system in C.

The current slice boots a freestanding x86_64 kernel through Multiboot2,
initializes serial and VGA text consoles, parses Multiboot2 modules, starts the
kernel-hosted VM server, and enters a user-space init server which launches
other user servers. It can boot into a login prompt and run a dynamic BusyBox
shell over the serial console, with VFS-backed reads, pipes, procfs, dmesg,
basic multiuser login/session accounting, and a user-space Linux personality
server:

```text
login: root
/ # busybox echo PIPE_OK | busybox cat
PIPE_OK
/ # busybox cat /hello.txt
hello from rootfs
```

## Current shape

- `arch/x86_64/`: x86_64 Multiboot2 entry code and low-level I/O.
- `boot/`: GRUB config.
- `kernel/`: bootstrap, console/dmesg ring buffer, PMM/VM mechanisms,
  scheduler, name registry, syscall forwarding, and boot module launcher.
- `modules/`: raw Multiboot2 boot modules.
- `servers/`: kernel-hosted VM server facade.
- `user/`: freestanding ring-3 servers and test/user programs.
- `Makefile`: build, EFI ISO, and QEMU/KVM targets.

The init, names, time, user, linux, proc, procfs, block, VFS, and ping servers
are freestanding C ELF images loaded as Multiboot2 modules and entered in ring 3.
The generated rootfs includes native smoke tests, Linux-syscall tests, login,
BusyBox applet links, `/etc/passwd`, `/etc/shadow`, and small data files. Proc
loads ELF images through VFS and owns Bunix process lifetime. The Linux
personality server owns Linux PID/session/fd state and receives Linux syscalls
as `LINX` protocol messages. The VM server remains kernel-hosted for the
current performance-oriented design, but still exposes a VM IPC port for events
and the kernel exports low-level task memory primitives for allocation and range
cloning.
The host-side rootfs builder keeps the on-disk format simple but grows its entry
table dynamically, so larger test images are no longer capped by the builder's
initial table size.

Userspace servers now have a small buddy allocator backed by native task
allocation, and their object registries grow dynamically instead of using fixed
server-side pools. The remaining fixed sizes are mostly ABI/protocol-sized
buffers, per-architecture bootstrap structures, and kernel-internal tables that
have not been generalized yet.

The tree is split so future ports can add a sibling such as `arch/arm64/` with
its own boot path, interrupt setup, MMU setup, and device I/O while reusing the
common kernel and server code.

Kernel logging uses `console_printf`, a small freestanding formatter supporting
`%s`, `%c`, `%d`, `%i`, `%u`, `%x`, `%p`, and `%%`. Once Linux userspace starts,
kernel/server logs route into the kernel ring buffer and BusyBox `dmesg` can
read them without trampling the interactive shell.

## IPC shape

IPC is port-based and event-oriented. Servers create named ports, senders queue
fixed-size messages, and receivers block when a port is empty. This is intended
to grow toward lightweight kernel-thread/event-port style server communication
rather than forcing every interaction into a synchronous syscall shape.

User mode can create private ports and send/receive fixed-size
`struct bunix_msg` messages through task-local handles. Each message carries a
FourCC protocol ID plus a protocol-local type, and may transfer one attached
port capability. Handles are typed capabilities rather than global object IDs or
kernel pointers. The current handle type is `port`, with `SEND`, `RECV`, and
`DUP` rights checked by the IPC syscall path. Attached capability transfer
requires `DUP`, and the receiver gets only the requested subset of rights.
Module launch accepts an explicit list of parent handles plus requested child
rights. The parent must hold `DUP`, and the requested rights must be a subset of
the parent's rights. The child receives its own service port as handle 1, then
attenuated inherited handles in caller-specified order.

The boot policy grants init `self`, `console`, `vm`, and `names`; init registers
console and VM with the user-space names server, resolves them back as
delegable capabilities, then launches time, user, linux, proc, procfs, block,
VFS, and ping with attenuated caps. Init waits for the time server to register
`TIME`, then passes
that capability to proc so proc can delegate sleep authority to the first
process. The returned launch value is a send-capability to the child's service
port in the parent task. `recv` blocks the current thread and wakes when a
sender queues an event. `ipc_call` uses a per-task private reply port, and
received messages can carry a send-capability reply handle that the server can
use without learning anything else about the caller. A task can also close one
of its own handles, which clears only that task-local capability slot; the
underlying object and any other task's capabilities remain untouched.

The proc server is the higher-level lifetime authority above low-level tasks.
It owns Bunix PID assignment, asks VFS for executable images, parses ELF program
headers in user space, maps loadable segments through generic task syscalls,
starts tasks at the ELF entry point, and implements wait/exit over the `PROC`
FourCC protocol. Linux PID 1 is deliberately owned by the Linux personality, not
by Bunix init; Bunix task IDs and Linux PIDs remain separate identities.

Names are scoped by namespace objects. The names server starts with a root
namespace for boot services, and clients can request new namespace IDs. Register
and resolve operations include both a namespace and a service ID, so the same
object can be re-exported with different visibility and rights in another
namespace without changing the underlying port object. Init currently creates a
filesystem namespace, re-exports VFS into it, then resolves VFS from that
namespace for the root read. Callers that need a service dependency can issue a
blocking names wait, letting the names server retain the caller's reply
capability until a matching object is registered instead of polling on timer
ticks or retry loops.

The VM server owns the memory authority policy, but module task spaces are now
granted through direct kernel VM calls during launch. That avoids a synchronous
scheduler re-entry path while VM remains kernel-hosted. VM still receives
event-port style IPC, currently exercised by the ping heartbeat server.

## Filesystem Path

The filesystem path is still a server-to-server read flow. The build generates
a tiny `disk0` image containing `/hello.txt`, `/secret.txt`, `/etc/passwd`,
`/etc/shadow`, `/etc/group`, `/etc/inittab`, `/etc/execs`, `/etc/spawns`,
`/bin/first`, `/bin/lxtest`, `/bin/login`, `/bin/musl-hello`, `/bin/busybox`,
and BusyBox applet links, then GRUB loads that image as a Multiboot2 data
module. The kernel assigns that
boot module only to the block server. Init launches a block server and a VFS
server with only console and names capabilities. The block server registers
`BLK0` in the root namespace and serves read-only bytes from its assigned disk
image. The rootfs server registers `RFS0` and translates that image into
read-only VFS operations. VFS registers `VFS0` and delegates paths to
user-space filesystem translators. Procfs attaches at `/proc`, tmpfs attaches
at `/tmp`, `/run`, and `/var/tmp`, and unionfs attaches at `/` with rootfs as
its lower layer so the read-only rootfs image is visible through a writable
tmpfs-backed upper layer.

This is intentionally not a real disk filesystem yet. The useful primitive is
the capability-shaped chain `init/proc -> names -> vfs -> unionfs -> rootfs +
tmpfs -> block`, which is the path that can later point at a real disk image
and then at an Alpine root filesystem format. Runtime Linux/VFS path handling
and the generated rootfs image both follow the Linux 4096-byte `PATH_MAX`
budget.

`procfs.server` is a separate user-space server that dynamically attaches
itself to VFS as the `/proc` translator. It currently exposes `/proc/kthreads`,
showing internal Bunix tasks as kernel threads for observability from Linux
userspace, plus process metadata such as `/proc/self/status`,
`/proc/self/cmdline`, `/proc/self/fd`, `/proc/self/exe`, `/proc/stat`,
`/proc/ipc`, `/proc/filesystems`, `/proc/cpuinfo`, `/proc/meminfo`,
`/proc/mounts`, and `/proc/uptime`.
VFS sends mount notifications to procfs, so `/proc/mounts` reflects the live
translator set without procfs calling back into VFS during file reads.

## User Mode

User modules are built as freestanding C x86_64 ELFs with a tiny `crt0.S` and
the native `libbunix` header surface over inline syscall wrappers. Init, names,
time, user, linux, proc, procfs, block, VFS, first, login, and ping all link at
the normal `0x400000` user base and enter with the same `0x800000` user stack top. The
small user ABI exposes negative-number syscalls for thread exit, timer ticks,
monotonic nanosecond time, blocking nanosecond sleep, module launch with
inherited handles, private port creation, IPC send/receive, IPC call, and
generic task create/map/grant/start operations used by proc's ELF loader.
Native userspace can also allocate into its own task with `task_alloc(0, ...)`;
the server-side buddy allocator builds on that to provide growable maps and
object tables.
Shared buffer capabilities provide bulk byte transport for servers. Kernel
buffer metadata is separate from dynamically allocated byte storage, so buffers
can grow beyond the syscall scratch page; native read/write syscalls chunk
copies internally while preserving the handle API. VFS exposes the exec-facing
file operations proc needs now: open a named regular file, query its size/type,
read positioned byte ranges through a shared buffer, and close the open object.
Proc allocates a buffer, sends duplicate/write authority to VFS, VFS forwards
write-only authority to block, block fills it from the rootfs module, and proc
copies the bytes back through its read authority. Proc builds the initial exec
stack with caller-supplied `argc`, `argv[]`, `envp[]`, and auxv entries for page
size, entry point, program headers, and `AT_EXECFN`; argv/env address staging is
allocated dynamically instead of capped by fixed pointer arrays.
Bunix-private auxv entries publish startup service capabilities for stdout,
stderr, time, and proc, so a process consumes delegated handles from its initial
image instead of baking in ambient handle numbers. The kernel still has an
internal bootstrap name registry, but it is no longer exposed as a normal user
authority path.

Linux syscall numbers are kept separate from the native negative Bunix syscall
space. Nonnegative syscall numbers trap into the kernel. The kernel still owns
low-level memory operations such as `brk`, `mmap`, `munmap`, `mprotect`,
`fork`, `clone`-style task creation, and `execve` image replacement, but most
Linux identity, fd, tty, session, pipe, VFS, user, signal, wait, and process
operations are marshalled as synchronous `LINX` protocol requests to the
user-space Linux personality server. The kernel attaches user buffers as
shared-buffer capabilities and blocks the caller on a reply port, then returns
the server's Linux-style result value.

Bootstrap loads `/etc/execs` through the mounted root filesystem and uses it to
seed proc's executable metadata before spawning `/sbin/init`. It then reads
`/etc/spawns` for native smoke processes to run and wait on; extra tokens become
argv entries, and `env:NAME=value` tokens become environment entries through
proc's buffer-backed spawn request. Proc explicitly
registers Linux tasks with the Linux server before starting them, carrying the
backing Bunix task id and Linux parent PID. The Linux server owns dynamic maps
keyed by Bunix task id and Linux PID, so Linux PID 1 exists inside the
personality even though its Bunix task id is different. Each Linux
process owns a dynamically growing fd table initialized with stdin/stdout/stderr
on the console. Pipes are in-memory Linux-server objects with blocking read
semantics; `pipe`, `pipe2`, `dup`, `dup2`, `dup3`, `fcntl`, `sendfile`, and
VFS-backed file/dir fds are implemented enough for the current BusyBox shell.
The Linux server also synthesizes `/dev/tty`, `/dev/console`, `/dev/null`,
`/dev/zero`, `/dev/random`, and `/dev/urandom` for common libc and tool probes.
`exit_group` marks the Linux process exited and records its status before the
kernel tears down the backing task thread; `wait4` can block on a child and
returns the child PID with a Linux wait status.
Linux `execve` dynamically copies argv and envp strings within the current
64 KiB exec stack budget, builds the full replacement stack image, and writes it
into the new task image instead of using tiny fixed argv/env arrays. Linux
`execve` accepts executable paths up to the 4096-byte Linux `PATH_MAX` budget,
and `getcwd` returns cwd strings through shared-buffer transport instead of the
inline IPC word path.
Proc's native executable registry and bootstrap's `/etc/execs` and
`/etc/spawns` parsing use the same runtime path budget, so native task spawns
can exercise long VFS paths as well. Buffer-backed proc spawn requests and the
native initial stack image now use a 64 KiB budget, with stack writes chunked
through the lower 4 KiB native syscall transport.
The login program now execs the shell with `HOME`, `USER`, `LOGNAME`, `SHELL`,
`PATH`, and `TERM`, and changes into the account home directory before execing
the shell. Command lookup, login environment inheritance, larger argv/env
vectors, long executable paths, more than 16 supplementary login groups, and
home-directory startup are exercised by the
shell regression.

The current rootfs can run a statically linked musl hello program, a dynamic
musl hello program, and a dynamically linked BusyBox shell through `/bin/login`.
BusyBox applets currently exercised by the test loop include `cat`, `echo`,
`env`, `stat`, `ls`, `uptime`, `id`, `stty`, `kill`, `sleep`, `dmesg`, `pwd`,
and `cd` through the shell. The generated rootfs includes explicit
`/home/kaniini`, `/root`, `/tmp`, `/run`, `/mnt`, `/sys`, `/var/tmp`, and
`/var/run` directories plus `/usr/bin/env` for script-style command lookup.
Backspace, canonical tty input, Ctrl-C delivery to foreground jobs, login
prompt respawn after shell exit, `/secret.txt` permission denial for the login
user, second login as root, root access to `/secret.txt`, and applet argv
handling are covered by `make test-shell`. The same regression also verifies
that creating, copying up, and whiteouting files at `/` uses the unionfs root
overlay while preserving the read-only lower image as unionfs input.

`/bin/lxtest` contains no Bunix headers or crt0; it issues raw x86_64 Linux
syscall numbers for the compatibility path and verifies returned byte counts,
metadata, Linux PID/TID values, Linux session id and umask state, page-backed
`brk`, anonymous writable mappings, forked child creation with cloned mmap
contents, fd allocation, child waiting,
`statx`, `access`/`faccessat`/`faccessat2` permission checks, and `-EBADF` for
invalid fds, then exits successfully. Linux `mmap` is currently an
anonymous/private compatibility path implemented on top of task memory
allocation, and `munmap` removes, trims, or splits VM region records. Linux
`fork` is built on a saved syscall frame plus task VM region cloning.
Page-table teardown now clears unmapped pages and returns their frames to the
PMM; copy-on-write is still future work.

The kernel loads each module's `PT_LOAD` segments into private frames mapped in
the target task's VM space, allocates private stack pages, enters ring 3 with
`iretq`, and exposes the syscall namespace over `syscall/sysret`. The kernel
still keeps a small internal boot registry for recorded module bookkeeping, but
normal service discovery now goes through the user-space names server authority
that a task was explicitly given.

Syscall entry uses the current thread's kernel/trap stack, so a syscall that
blocks in the scheduler keeps its return frame isolated from other user threads'
syscalls. Blocking IPC marks the receiver blocked before dropping the IPC lock,
then switches after unlocking, so a fast reply cannot be lost between publishing
the receiver and entering the scheduler.

## Scheduler shape

Tasks are low-level resource containers: they own handles and VM spaces, but
they are not process lifetime objects. Threads are schedulable contexts that
will map naturally to Linux LWPs. The current scheduler is FIFO, with
`thread_yield()`, `thread_block()`, deadline-ordered sleep queues behind
`thread_sleep_ns()`, `thread_unblock()`, timer-driven preemption, and
thread-slot reaping after exit.

The scheduler state is split into per-CPU structures with per-CPU run queues,
and the shared kernel structures used by the current server path now have
spinlock/IRQ-save protection: run queues, task handles, IPC queues/message
pools, the name registry, PMM, VM space allocation, and console writes. QEMU
tests boot with `-smp 2`, and x86_64 discovers CPUs through the Multiboot2 ACPI
RSDP and MADT. The BSP maps and enables the local APIC, copies a low-memory AP
trampoline, and starts the second CPU with INIT/SIPI. APs enter 64-bit C code,
load their own IDT, GDT, TSS, syscall MSRs, and syscall stack slot, then the BSP
releases them into the common scheduler's secondary loop.

Idle CPUs mark themselves idle under the run-queue lock and then enter
`sti; hlt` only after a final locked runnable check. Remote run-queue enqueue
clears that idle state under the same lock and sends a scheduler IPI when the
target CPU was sleeping. Normal thread creation uses scheduler-owned automatic
placement over the least-loaded CPU with round-robin tie breaking. The current
boot flow lets VM, init, time, proc, block, VFS, first, and ping all use
automatic placement, which exercises user entry, syscalls, IPC wakeups, server
work, and scheduling across both CPUs.

Exited threads switch back to the per-CPU scheduler thread, which reaps them
from the scheduler stack, decrements the owning task's thread count, and returns
the thread-table slot to the empty pool. A task with no threads remains a valid
low-level container until a future higher-level process/session layer decides
what destruction means.

Each task owns a `vm_space` granted by the VM server facade. Tasks track simple
VM region metadata so fork can clone recorded ELF, stack, brk, and mmap regions
rather than hardcoded address bands. On x86_64, a VM space contains a real
PML4, PDPT, and page directory, currently identity-mapping
the low 1 GiB so the kernel can keep running after CR3 switches. User ELF and
stack pages are mapped privately by splitting those 2 MiB identity mappings into
4 KiB page tables where needed. A future higher-half kernel split should remove
the broad user-visible identity map.

## Interrupts

x86_64 installs an IDT, remaps the legacy PIC, starts the PIT at 100 Hz for the
global bootstrap clock, and programs each online CPU's local APIC timer for
periodic scheduler ticks. User threads have separate trap stacks for ring-3
interrupts, and the kernel enables preemption before normal server scheduling.
The scheduler preempts only interrupted user-mode frames, leaving kernel/syscall
critical paths non-preemptible for now.

## Build

```sh
make
```

The build uses the host `gcc` in freestanding 64-bit mode. No libc is linked.

## QEMU/KVM test

```sh
make test
```

`make test` boots through OVMF/GRUB with KVM, captures serial output in
`build/serial.log`, and checks the boot-to-login path: GRUB passed Multiboot2
modules, the kernel started VM, names, time, user, linux, proc, procfs, block,
VFS, and ping, bootstrap received its boot capabilities, services
registered/resolved through the user-space names server, VFS re-exported the
filesystem namespace, `/sbin/init` launched, procfs attached, OCAP launch
denial worked, and the login prompt appeared.

For the interactive BusyBox shell regression:

```sh
make test-shell
```

`make test-shell` builds the default dynamic-BusyBox rootfs, drives the serial
console, logs in, runs BusyBox applets,
checks pipes and file reads, verifies login/session-visible `uptime`, checks
`id`, `stat`, `ls`, `cat`, append writes, long tmpfs paths, shell `cd`/`pwd`, backspace, Ctrl-C,
permission denial for `/secret.txt`, and confirms that exiting the shell returns
to the login prompt. It also checks writable-root unionfs behavior and live
`/proc/mounts` output.

`make test-rootfs-tool` is a host-side regression for the rootfs image builder;
it creates more than 128 entries to verify the builder's dynamic entry table.

The static PIE BusyBox path remains available as a compatibility regression:

```sh
make test-shell-static
```

For an interactive serial console:

```sh
make run
```

These targets build a standalone GRUB `BOOTX64.EFI`, expose `build/esp` as an EFI
System Partition, boot through OVMF with KVM, and print to the serial console.

## EFI GRUB image

```sh
make iso
make run-iso
```

`make iso` uses `grub-mkrescue` and requires `xorriso`. The default `make run`
target does not require `xorriso`.

## Licensing note

This is 100% slop.  Is it legal to distribute?  Who knows, I certainly don't.
