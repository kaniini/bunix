#ifndef BUNIXOS_IPC_H
#define BUNIXOS_IPC_H

#include "types.h"

enum {
	IPC_WORDS = 4,
	IPC_STATS_CPUS = 8,
};

struct ipc_port;
struct shared_buffer;
struct task_hw_resource;
struct thread;

enum ipc_cap_type {
	IPC_CAP_NONE = 0,
	IPC_CAP_PORT,
	IPC_CAP_BUFFER,
	IPC_CAP_TASK,
	IPC_CAP_HW_RESOURCE,
};

struct ipc_message {
	u32 protocol;
	u32 type;
	u32 sender;
	u32 cap_rights;
	struct ipc_port *reply_port;
	enum ipc_cap_type cap_type;
	void *cap_object;
	u64 words[IPC_WORDS];
};

struct ipc_stats {
	u64 sends;
	u64 queued;
	u64 direct_delivered;
	u64 direct_handoff;
	u64 fallback_cross_cpu;
	u64 fallback_nested;
	u64 fallback_scheduler;
	u64 fallback_invalid;
	u64 cpu_sends[IPC_STATS_CPUS];
	u64 cpu_queued[IPC_STATS_CPUS];
	u64 cpu_direct_delivered[IPC_STATS_CPUS];
	u64 cpu_direct_handoff[IPC_STATS_CPUS];
	u64 cpu_fallback_cross_cpu[IPC_STATS_CPUS];
	u64 cpu_fallback_nested[IPC_STATS_CPUS];
	u64 cpu_fallback_scheduler[IPC_STATS_CPUS];
	u64 cpu_fallback_invalid[IPC_STATS_CPUS];
};

void ipc_init(void);
struct ipc_port *ipc_port_create(const char *name);
struct ipc_port *ipc_port_create_private(const char *name);
struct ipc_port *ipc_port_find(const char *name);
void ipc_port_retain(struct ipc_port *port);
void ipc_port_release(struct ipc_port *port);
const char *ipc_port_name(const struct ipc_port *port);
u64 ipc_port_id(const struct ipc_port *port);
struct ipc_port *ipc_port_from_id(u64 id);
int ipc_port_affinity_cpu(struct ipc_port *port, u32 *cpu_id);
int ipc_send(struct ipc_port *port, const struct ipc_message *message);
int ipc_send_interrupt(struct ipc_port *port, const struct ipc_message *message);
int ipc_recv(struct ipc_port *port, struct ipc_message *message);
int ipc_try_recv(struct ipc_port *port, struct ipc_message *message);
void ipc_message_release(struct ipc_message *message);
void ipc_stats_snapshot(struct ipc_stats *stats);
int ipc_call_kernel(struct ipc_port *port, const struct ipc_message *request,
		    struct ipc_message *reply);
void ipc_cancel_thread(struct thread *thread);

#endif
