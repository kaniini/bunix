# Scheduler Policy Authority

Bunix scheduler policy is task/thread authority, not Linux PID authority.  Linux
PIDs can be mapped to Bunix tasks by the proc/Linux servers, but the scheduler
core only trusts Bunix task handles and scheduler-policy capabilities.

## Protocol

The scheduler-policy service should use FourCC protocol `SCHD`.

Initial RPC operations:

- `INFO`: return supported classes, priority range, weight range, CPU mask
  width, and online CPU mask.
- `GRANT`: derive an attenuated scheduler-policy capability for a target task
  or thread.
- `GETP`: read current class, priority, weight, and affinity mask for a target
  task or thread.
- `SETP`: apply class, priority, weight, or affinity changes allowed by the
  presented policy capability.
- `STAT`: return scheduler counters for a target task or thread when a caller
  has observation rights.

The service should be a userspace server.  Kernel scheduler syscalls should be
small primitives that validate a kernel-issued policy capability and mutate a
Bunix task/thread.  Policy decisions, Linux PID mapping, administrative defaults,
and login/session policy belong outside the kernel.

## Capability Shape

A scheduler-policy capability names:

- Target kind: task, thread, task subtree, session, or system.
- Target ID: Bunix task ID/thread ID, or zero for session/system scopes.
- Rights mask: observe, class, priority, weight, affinity, and delegate.
- Bounds: allowed classes, minimum/maximum priority, minimum/maximum weight,
  and allowed CPU mask.
- Delegation depth: zero means the capability cannot mint derived policy caps.

Attenuation is monotonic.  A derived capability can only remove rights, narrow
class sets, narrow priority/weight ranges, narrow CPU masks, and decrease
delegation depth.  No RPC may widen authority based on caller identity, UID, or
Linux PID alone.

## No Ambient Self-Promotion

Ordinary tasks start with no scheduler policy rights.  A task can ask the
scheduler-policy server for changes, but the server must require an explicit
policy capability from a trusted parent, login/session authority, or system
bootstrap authority.

Linux compatibility calls such as `sched_setscheduler(2)`, `setpriority(2)`,
and future CPU-affinity calls should RPC to the Linux server first.  The Linux
server maps PID/TID semantics to Bunix task/thread targets through proc state,
then RPCs to the scheduler-policy server with whatever policy capability the
Linux process is allowed to use.  Linux PID 1 is not special to the scheduler
unless it was explicitly granted policy authority.

## Class, Priority, Weight, And Affinity

Class is a coarse scheduler behavior bucket.  Initial classes are `KERNEL`,
`SERVER`, `USER`, `BATCH`, and `IDLE`.  Kernel and bootstrap code can seed
server tasks with `SERVER`, but runtime transitions require policy authority.

Priority is an ordering hint inside a class.  Weight is the proportional share
input used by virtual-runtime accounting.  Priority changes should not silently
change weight; callers that can tune both need both rights.

CPU affinity is a Bunix mask over scheduler CPUs.  It is not a Linux cpuset and
is not a PID property.  Linux affinity APIs are translations layered above this
mask.  Invalid masks, empty masks, and masks containing offline CPUs must be
rejected unless the policy server explicitly supports delayed/offline masks.

## Observability

`/proc/sched` remains aggregate scheduler state.  `/proc/sched_threads` exposes
thread-level scheduler snapshots for tests and tools.  A future `SCHD STAT` RPC
should provide the same data with capability-scoped filtering so user tools do
not depend on globally readable procfs forever.

Benchmarks should prefer guest-side counters and relative comparisons inside
one boot.  Host wall time is useful as a harness timeout, but it is not a
scheduler policy result.
