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
- `kernel/`: bootstrap, console, VM, cooperative scheduler, and initial server registry.
- `modules/`: raw Multiboot2 boot modules.
- `servers/hello/`: first server stub.
- `Makefile`: build, EFI ISO, and QEMU/KVM targets.

The hello implementation is still linked into the kernel for this milestone,
but it is now selected by a Multiboot2 module and run in its own kernel task and
thread. The next microkernel steps are to make those module payloads ELF server
images, add an address-space model, and wire IPC between scheduled tasks.

The tree is split so future ports can add a sibling such as `arch/arm64/` with
its own boot path, interrupt setup, MMU setup, and device I/O while reusing the
common kernel and server code.

Kernel logging uses `console_printf`, a small freestanding formatter supporting
`%s`, `%c`, `%d`, `%i`, `%u`, `%x`, `%p`, and `%%`.

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
creates tasks/threads and starts the hello and ping servers. The ping server
exercises cooperative `thread_yield()` on the FIFO run queue.

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
