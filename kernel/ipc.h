#ifndef BUNIXOS_IPC_H
#define BUNIXOS_IPC_H

#include "types.h"

enum {
	IPC_WORDS = 4,
};

struct ipc_port;
struct shared_buffer;

enum ipc_cap_type {
	IPC_CAP_NONE = 0,
	IPC_CAP_PORT,
	IPC_CAP_BUFFER,
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

void ipc_init(void);
struct ipc_port *ipc_port_create(const char *name);
struct ipc_port *ipc_port_create_private(const char *name);
struct ipc_port *ipc_port_find(const char *name);
void ipc_port_retain(struct ipc_port *port);
void ipc_port_release(struct ipc_port *port);
const char *ipc_port_name(const struct ipc_port *port);
u64 ipc_port_id(const struct ipc_port *port);
struct ipc_port *ipc_port_from_id(u64 id);
int ipc_send(struct ipc_port *port, const struct ipc_message *message);
int ipc_recv(struct ipc_port *port, struct ipc_message *message);
int ipc_call_kernel(struct ipc_port *port, const struct ipc_message *request,
		    struct ipc_message *reply);

#endif
