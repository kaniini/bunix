# bunixos

An experimental microkernel-oriented operating system in C.

This first slice boots a freestanding x86_64 kernel through Multiboot2,
initializes serial and VGA text consoles, parses Multiboot2 modules, starts the
kernel-hosted VM server, and enters a user-space init server which launches
other user servers:

```text
hello: world <3
```

## Current shape

- `arch/x86_64/`: x86_64 Multiboot2 entry code and low-level I/O.
- `boot/`: GRUB config.
- `kernel/`: bootstrap, console, PMM/VM mechanisms, scheduler, name registry, and boot module launcher.
- `modules/`: raw Multiboot2 boot modules.
- `servers/`: initial server stubs, including the skeletal VM server.
- `Makefile`: build, EFI ISO, and QEMU/KVM targets.

The init, hello, and ping modules are freestanding C ELF images loaded into
their tasks and entered in ring 3. The VM server remains kernel-hosted for the
current performance-oriented design, but still exposes a VM IPC port for events.
The next microkernel steps are to move more server functionality behind the
same user-mode ABI while continuing to shrink the kernel bootstrap policy into
explicit capability handoff.

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
`struct bunix_msg` messages through task-local port handles. Handles are
capabilities rather than global object IDs or kernel pointers. Module launch
accepts an explicit list of parent handles to inherit; the child receives its
own service port as handle 1, then inherited handles in caller-specified order.
The boot policy grants init `self`, `console`, and `vm`; init launches hello
with only `console` and ping with `console` plus `vm`. The returned launch value
is a handle to the child's service port in the parent task. `recv` blocks the
current thread and wakes when a sender queues an event. `ipc_call` uses a
per-task private reply port, and received messages can carry a reply handle
that the server can use without learning anything else about the caller.

The VM server owns the memory authority policy, but module task spaces are now
granted through direct kernel VM calls during launch. That avoids a synchronous
scheduler re-entry path while VM remains kernel-hosted. VM still receives
event-port style IPC, currently exercised by ping.

## User Mode

User modules are built as freestanding C x86_64 ELFs with a tiny `crt0.S` and
inline syscall wrappers. Init, hello, and ping all link at the normal
`0x400000` user base and enter with the same `0x800000` user stack top. The
small user ABI exposes negative-number syscalls for thread exit, timer ticks,
module launch with inherited handles, private port creation, IPC send/receive,
and IPC call. The kernel still has an internal bootstrap name registry, but it
is no longer exposed as a normal user authority path.

The kernel loads each module's `PT_LOAD` segments into private frames mapped in
the target task's VM space, allocates private stack pages, enters ring 3 with
`iretq`, and exposes the syscall namespace over `syscall/sysret`.

Syscall entry uses the current thread's kernel/trap stack, so a syscall that
blocks in the scheduler keeps its return frame isolated from other user threads'
syscalls.

## Scheduler shape

Tasks are resource containers that will map naturally to Linux processes.
Threads are schedulable contexts that will map naturally to Linux LWPs. The
current scheduler is FIFO, with `thread_yield()`, `thread_block()`,
`thread_unblock()`, and timer-driven preemption once enabled.

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
placement over the least-loaded CPU, while server launch can provide a preferred
CPU that the scheduler validates. The current boot flow lets init use automatic
placement and prefers the kernel-hosted VM server, hello, and ping user modules
on CPU 1, which exercises user entry, syscalls, IPC wakeups, server work, and
timer preemption across CPUs.

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
started VM plus init, init received its boot capabilities, and init launched the
hello and ping C servers with explicit inherited handles. Init sends ping a
synchronous user IPC call through the returned service-port handle, ping
receives it through blocking `recv`, sends a VM event through its inherited VM
capability, exercises local-APIC timer preemption on CPU 1 while the VM server
handles the event, and replies to init through the granted reply capability.

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
