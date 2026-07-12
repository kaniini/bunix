# User Memory Contract

This document defines the current contract between architecture page-table
code, the generic VM layer, kernel task operations, and userspace servers.
It is the target for the user-copy/VM API cleanup in exploration 067.

## Address Space Types

`struct vm_space` represents one page-table root plus generic metadata.
`vm_kernel_space()` is the kernel address space.  Task address spaces are
created by `vm_rpc_create_space()` and are attached to tasks through
`task_vm_space()`.

The kernel may inspect physical frames through the direct physical mapping, but
generic user-copy APIs must only translate pages that are explicitly mapped as
user pages in the target `vm_space`.

## Translation APIs

Architecture code exposes two translation families:

- `arch_vm_translate()` translates any present page and is for kernel-owned
  mappings or low-level architecture checks.
- `arch_vm_translate_user()` requires user-accessible PTE bits and rejects
  kernel-only mappings.  Generic user-copy and user-range operations must use
  this family.

The user translation contract is:

- return physical address plus page offset on success;
- return zero for unmapped pages, missing user permission, missing write
  permission on write probes, unsupported large user pages, or malformed page
  tables;
- never allow a user-copy path to access a kernel-only mapping by accident.

## Generic User VM APIs

The `vm_*_user*` APIs in `kernel/vm.h` operate on explicit user mappings:

- `vm_map_user_page()` maps one physical frame into a task/user address space.
- `vm_alloc_user_page()` and `vm_alloc_user_range()` allocate zeroed frames and
  map them as user pages.
- `vm_read_user()` copies bytes from explicit user mappings into kernel memory.
- `vm_write_user()` copies bytes from kernel memory into explicit writable user
  mappings.
- `vm_protect_user_range()` changes user page protections.
- `vm_unmap_user_range()` removes user pages and releases their frames.
- `vm_clone_user_range()` creates private copies of user mappings.
- `vm_share_user_range()` shares retained frames between user address spaces.
- `vm_share_cow_user_range()` shares retained frames read-only and marks the
  source read-only for copy-on-write.
- `vm_cow_user_page()` replaces one shared user page with a writable private
  copy after a COW fault.

These APIs accept a `struct vm_space *`, not a task ID.  They do not update
task VM-region metadata by themselves unless explicitly documented otherwise.
Callers that create, clone, protect, or remove mappings must keep
`task_vm_region` bookkeeping in sync through the task helpers.

## Kernel Task Operations

`server_task_*` helpers are the privileged kernel-side task authority exposed to
userspace servers through Bunix syscalls:

- `server_task_map()` maps loader bytes into another task and records an ELF
  VM region.
- `server_task_write()` writes kernel-owned bytes into another task's existing
  writable user mappings.
- `server_task_alloc()` and `server_task_alloc_kind()` allocate user mappings
  and record task VM regions.
- `server_task_clone_range()` copies user mappings between two authorized
  tasks.

The user-facing wrappers are `bunix_task_map*()`, `bunix_task_write()`,
`bunix_task_alloc*()`, and `bunix_task_clone_range*()`.  The proc server uses
these to build process images and stacks.  These calls grant authority over a
task through task handles; they must not imply access to kernel mappings.

## Linux Frontend User Copy

Architecture Linux syscall frontends copy syscall buffers through helpers such
as `read_current_user()` and `write_current_user()`.  Those helpers target the
current task's `task_vm_space()` and use generic user-copy operations.

On x86_64, `write_current_user()` retries after invoking
`task_handle_cow_fault()` for covered pages so Linux-style writes into COW
mappings can succeed.  This retry behavior is a frontend policy layered on top
of `vm_write_user()`; the generic write helper still only writes to explicit
writable user pages.

## Current Audit Map

- Generic VM implementation: `kernel/vm.c`, `kernel/vm.h`.
- x86_64 translation and syscall copy path: `arch/x86_64/vm.c`,
  `arch/x86_64/user.c`.
- riscv64 translation and syscall copy path: `arch/riscv64/vm.c`,
  `arch/riscv64/user.c`.
- Kernel server task authority: `kernel/server.c`, `kernel/server.h`.
- Task VM-region metadata: `kernel/sched.c`, `kernel/sched.h`.
- Userspace task-memory clients: `user/include/bunix/syscall.h`,
  `user/proc/main.c`, `user/proc/exec_stack.c`.

## Cleanup Targets

Exploration 067 should make the contract harder to misuse by:

- keeping user and kernel translation names explicit at every layer;
- adding failed-copy diagnostics that include task, address, length, operation,
  and reason without dumping user data;
- auditing all server and test-program call sites against this document;
- adding cross-architecture regressions for low-address faults, unmapped
  pointers, read-only pages, copied mappings, and denied kernel-only mappings.
