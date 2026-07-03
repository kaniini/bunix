# bunixos

An experimental microkernel-oriented operating system in C.

This first slice boots a freestanding x86_64 kernel through Multiboot2,
initializes serial and VGA text consoles, parses Multiboot2 modules, starts the
kernel-hosted VM server, and enters a user-space init server which launches
other user servers. The current boot proves a synthetic filesystem read, a
VFS-backed first process with stdout/exit status, and a user-space heartbeat
server:

```text
rootfs: module
first: stdout ready
first: exit 0
ping: heartbeat
```

## Current shape

- `arch/x86_64/`: x86_64 Multiboot2 entry code and low-level I/O.
- `boot/`: GRUB config.
- `kernel/`: bootstrap, console, PMM/VM mechanisms, scheduler, name registry, and boot module launcher.
- `modules/`: raw Multiboot2 boot modules.
- `servers/`: initial server stubs, including the skeletal VM server.
- `Makefile`: build, EFI ISO, and QEMU/KVM targets.

The init, names, time, linux, proc, block, VFS, and ping servers are
freestanding C ELF images loaded as Multiboot2 modules and entered in ring 3.
The first native process and a tiny Linux-syscall smoke test are packaged into
the generated rootfs and loaded by proc through VFS as `/bin/first` and
`/bin/lxtest`. The VM server remains kernel-hosted for the current
performance-oriented design, but still exposes a VM IPC port for events and
the kernel exports low-level task memory primitives for allocation and range
cloning. Tasks now track simple VM region metadata so fork can clone recorded
ELF, stack, brk, and mmap regions rather than hardcoded address bands. The
next microkernel steps are to replace the toy rootfs with a real filesystem
format while continuing to shrink the kernel bootstrap policy into explicit
capability handoff.

The tree is split so future ports can add a sibling such as `arch/arm64/` with
its own boot path, interrupt setup, MMU setup, and device I/O while reusing the
common kernel and server code.

Kernel logging uses `console_printf`, a small freestanding formatter supporting
`%s`, `%c`, `%d`, `%i`, `%u`, `%x`, `%p`, and `%%`.

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
delegable capabilities, then launches time, proc, block, VFS, ping, and the
Linux personality server with
attenuated caps. Init waits for the time server to register `TIME`, then passes
that capability to proc so proc can delegate sleep authority to the first
process. The returned launch value is a send-capability to the child's service
port in the parent task. `recv` blocks the current thread and wakes when a
sender queues an event. `ipc_call` uses a per-task private reply port, and
received messages can carry a send-capability reply handle that the server can
use without learning anything else about the caller. A task can also close one
of its own handles, which clears only that task-local capability slot; the
underlying object and any other task's capabilities remain untouched.

The proc server is the first higher-level lifetime authority above low-level
tasks. It owns PID assignment for the first process, asks VFS for `/bin/first`,
parses the ELF program headers in user space, maps loadable segments through
generic task syscalls, starts the task at the ELF entry point, and implements
wait/exit over the `PROC` FourCC protocol. The first process starts with
fd-like stdout mapped to a console send capability plus time and proc
capabilities.

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

The first filesystem slice is a server-to-server read flow. The build generates
a tiny `disk0` image containing `/hello.txt`, `/bin/first`, and `/bin/lxtest`,
then GRUB loads that image as a Multiboot2 data module. The kernel assigns that
boot module only to the block server. Init launches a block server and a VFS
server with only console and names capabilities. The block server registers
`BLK0` in the root namespace and serves read-only bytes from its assigned disk
image. VFS resolves `BLK0`, registers `VFS0`, parses the generated rootfs entry
table, and serves pathname reads. Init creates a filesystem namespace,
re-exports `VFS0` there, resolves it from that namespace, reads `/hello.txt`,
and prints `rootfs: module`.

This is intentionally not a real disk filesystem yet. The useful primitive is
the capability-shaped chain `init/proc -> names -> vfs -> block -> disk0`,
which is the path that can later point at a real disk image and then at an
Alpine root filesystem format.

## User Mode

User modules are built as freestanding C x86_64 ELFs with a tiny `crt0.S` and
the native `libbunix` header surface over inline syscall wrappers. Init, names,
time, linux, proc, block, VFS, first, and ping all link at the normal
`0x400000` user base and enter with the same `0x800000` user stack top. The
small user ABI exposes negative-number syscalls for thread exit, timer ticks,
monotonic nanosecond time, blocking nanosecond sleep, module launch with
inherited handles, private port creation, IPC send/receive, IPC call, and
generic task create/map/grant/start operations used by proc's ELF loader.
Shared buffer capabilities provide bulk byte transport for servers. VFS exposes
the exec-facing file operations proc needs now: open a named regular file,
query its size/type, read positioned byte ranges through a shared buffer, and
close the open object. Proc allocates a buffer, sends duplicate/write authority
to VFS, VFS forwards write-only authority to block, block fills it from the
rootfs module, and proc copies the bytes back through its read authority. Proc
also builds a minimal exec stack for the new task: `argc`, `argv[]`, null
`envp`, and auxv entries for page size, entry point, program headers, and
`AT_EXECFN`. Bunix-private auxv entries publish startup service capabilities
for stdout, stderr, time, and proc, so a process consumes delegated handles from
its initial image instead of baking in ambient handle numbers. The kernel still
has an internal bootstrap name registry, but it is no longer exposed as a normal
user authority path.

Linux syscall numbers are kept separate from the native negative Bunix syscall
space. For the current MVP, nonnegative syscall numbers trap into the kernel.
The minimal tracked `brk` is still answered directly by the kernel, while
Linux identity and file-oriented calls are marshalled as synchronous `LINX`
protocol requests to the user-space linux personality server. Proc explicitly
registers Linux tasks with that server before starting them, carrying the
backing Bunix task id and Linux parent PID. The server owns a small Linux PID
namespace table keyed by the backing Bunix task id, so the first Linux process
sees PID/TID 1 even though its Bunix task id is different. Each Linux process
object also owns its fd table, initialized with stdout/stderr and allocating
VFS-backed file fds from 3 independently. `exit_group` marks the Linux process
exited and records its status before the kernel tears down the backing task
thread; `wait4(-1, &status, 0, NULL)` can block on a child and returns the
child PID with a Linux wait status. This keeps Bunix task identity available for
observability without conflating it with Linux process identity. The kernel
attaches user buffers as shared-buffer capabilities and blocks the caller on a
reply port, then returns the server's Linux-style result value. `/bin/lxtest`
contains no Bunix headers or crt0; it issues raw x86_64 Linux syscall numbers
for `write`, `getpid`, `gettid`, `openat`, `fstat`, `newfstatat`, `read`,
`close`, `mmap`, `munmap`, `wait4`, and `exit_group`, verifies returned byte
counts, metadata, Linux PID/TID values, page-backed `brk`, anonymous writable
mappings, forked child creation with cloned mmap contents, fd allocation, child
waiting, and `-EBADF` for invalid fds, then exits successfully. Init launches
two `/bin/lxtest` instances to prove their Linux PID and fd namespaces are
separate; the first also forks and waits for its child. Console writes use the
server's delegated console capability, while
`openat(AT_FDCWD, path, O_RDONLY)`, `fstat`, `newfstatat`, `read`, and `close`
proxy to VFS using shared-buffer capabilities. Linux `mmap` is currently an
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

Each task owns a `vm_space` granted by the VM server facade. On x86_64, a VM
space contains a real PML4, PDPT, and page directory, currently identity-mapping
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
`build/serial.log`, and checks that GRUB passed Multiboot2 modules, the kernel
started VM, names, time, and init, init received its boot capabilities, and init
registered and resolved services through the user-space names server before
launching time, proc, block, VFS, and ping with attenuated inherited handles.
The test checks namespace creation, VFS re-export into the filesystem namespace,
the `VFS0 -> BLK0 -> disk0` read, and the resulting `rootfs: module` console
output. Init also proves the OCAP launch rule by asking for a receive right it
does not hold, which the kernel rejects before the module is marked launched.
Init asks proc to spawn `/bin/first`; proc reads that ELF through VFS, maps its
loadable segments, starts the task, and init waits for its exit status before
launching ping with console, VM, and time capabilities. Ping asks the time
server to sleep for two-second intervals, logs `ping: heartbeat`, and forwards
an incrementing heartbeat value plus the time server's monotonic timestamp as a
VMEM FourCC event through its inherited VM capability.

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

No third-party source has been imported yet. Keep imported components
permissively licensed only, for example MIT, BSD, ISC, Apache-2.0, or similarly
permissive terms.
