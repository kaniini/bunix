#ifndef BUNIXOS_IPC_H
#define BUNIXOS_IPC_H

#include "types.h"

enum {
	IPC_WORDS = 4,
};

struct ipc_port;

struct ipc_message {
	u32 type;
	u32 sender;
	u64 words[IPC_WORDS];
};

void ipc_init(void);
struct ipc_port *ipc_port_create(const char *name);
struct ipc_port *ipc_port_find(const char *name);
int ipc_send(struct ipc_port *port, const struct ipc_message *message);
int ipc_recv(struct ipc_port *port, struct ipc_message *message);

#endif
