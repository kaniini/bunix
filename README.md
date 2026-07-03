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
The next microkernel steps are to replace bootstrap shortcuts with
capability-checked IPC and move more server functionality behind the same
user-mode ABI.

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

The VM server owns the memory authority policy, but module task spaces are now
granted through direct kernel VM calls during launch. That avoids a synchronous
scheduler re-entry path while VM remains kernel-hosted. VM still receives
event-port style IPC, currently exercised by ping.

## User Mode

User modules are built as freestanding C x86_64 ELFs with a tiny `crt0.S` and
inline syscall wrappers. Init, hello, and ping all link at the normal
`0x400000` user base and enter with the same `0x800000` user stack top.

The kernel loads each module's `PT_LOAD` segments into private frames mapped in
the target task's VM space, allocates private stack pages, enters ring 3 with
`iretq`, and exposes a small negative-number syscall namespace over
`syscall/sysret`. Current calls include console write, thread exit, timer ticks,
name lookup/name registration, service writes, VM ping-by-service, module
launch, and explicit preemption enable.

## Scheduler shape

Tasks are resource containers that will map naturally to Linux processes.
Threads are schedulable contexts that will map naturally to Linux LWPs. The
current scheduler is FIFO, with `thread_yield()`, `thread_block()`,
`thread_unblock()`, and timer-driven preemption once enabled.

The scheduler state is already split into per-CPU structures with a per-CPU run
queue. `MAX_CPUS` is still `1`, but the model avoids baking a single global run
queue into callers. Real SMP will need CPU discovery, per-CPU current-thread
lookup, locking around run queues, inter-processor wakeups, and timer
preemption.

Each task owns a `vm_space` granted by the VM server facade. On x86_64, a VM
space contains a real PML4, PDPT, and page directory, currently identity-mapping
the low 1 GiB so the kernel can keep running after CR3 switches. User ELF and
stack pages are mapped privately by splitting those 2 MiB identity mappings into
4 KiB page tables where needed. A future higher-half kernel split should remove
the broad user-visible identity map.

## Interrupts

x86_64 installs an IDT, remaps the legacy PIC, and starts the PIT at 100 Hz.
Timer interrupts drive scheduler ticks. User threads have separate trap stacks
for ring-3 interrupts, and ping explicitly enables preemption before yielding
CPU time to the VM server through the timer path.

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
started VM plus init, and init launched the hello and ping C servers. Ping
resolves VM by name, sends a VM event, and exercises timer-driven preemption
while the VM server handles the event.

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
