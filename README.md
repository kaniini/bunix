# bunixos

An experimental microkernel-oriented operating system in C.

This first slice boots a freestanding x86_64 kernel through Multiboot2,
initializes serial and VGA text consoles, parses Multiboot2 modules, creates a
task/thread for each requested boot server, and starts a hello server requested
by a boot module:

```text
hello: world <3
```

## Current shape

- `arch/x86_64/`: x86_64 Multiboot2 entry code and low-level I/O.
- `boot/`: GRUB config.
- `kernel/`: bootstrap, console, PMM/VM mechanisms, cooperative scheduler, and initial server registry.
- `modules/`: raw Multiboot2 boot modules.
- `servers/`: initial server stubs, including the skeletal VM server.
- `Makefile`: build, EFI ISO, and QEMU/KVM targets.

The hello module is now a standalone ELF image loaded into its task and entered
in ring 3. The VM server is also module-backed and owns memory policy shape
through RPC hooks. The next microkernel steps are to move the remaining
kernel-hosted servers to ELF images and replace bootstrap shortcuts with
capability-checked IPC.

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

VM space grants now travel through VM server RPC messages. The VM server still
uses a bootstrap space for itself, then receives `VM_RPC_CREATE_SPACE` messages
for other module-backed servers and replies over the caller's reply port.

## User Mode

The `hello` module is built as a freestanding x86_64 ELF at `0x400000`. The
kernel loads its `PT_LOAD` segments, enters ring 3 with `iretq`, and exposes a
small negative-number syscall namespace over `syscall/sysret`: `-1` writes to
the console and `-2` exits the thread.

## Scheduler shape

Tasks are resource containers that will map naturally to Linux processes.
Threads are schedulable contexts that will map naturally to Linux LWPs. The
current scheduler is cooperative and FIFO, with `thread_yield()`,
`thread_block()`, and `thread_unblock()`.

The scheduler state is already split into per-CPU structures with a per-CPU run
queue. `MAX_CPUS` is still `1`, but the model avoids baking a single global run
queue into callers. Real SMP will need CPU discovery, per-CPU current-thread
lookup, locking around run queues, inter-processor wakeups, and timer
preemption.

Each task owns a `vm_space` granted by the VM server facade. On x86_64, a VM
space contains a real PML4, PDPT, and page directory, currently identity-mapping
the low 1 GiB so the existing kernel-thread servers can run while CR3 switching
is exercised.

## Interrupts

x86_64 installs an IDT, remaps the legacy PIC, and starts the PIT at 100 Hz.
Timer interrupts drive scheduler ticks, and the cooperative scheduler can now
preempt a running thread when another thread is ready on the current CPU's run
queue.

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
`build/serial.log`, and checks that GRUB passed a Multiboot2 module which
creates tasks/threads and starts the hello, ping, and VM servers. The ping
server exercises cooperative `thread_yield()` on the FIFO run queue, and the VM
server exercises the temporary `vm_rpc_*` frame allocation boundary.

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
