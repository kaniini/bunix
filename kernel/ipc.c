#include "console.h"
#include "ipc.h"
#include "sched.h"

enum {
	MAX_PORTS = 16,
	MAX_MESSAGES = 32,
};

struct ipc_message_node {
	struct ipc_message message;
	struct ipc_message_node *next;
	u32 in_use;
};

struct ipc_port {
	const char *name;
	struct ipc_message_node *head;
	struct ipc_message_node *tail;
	struct thread *receiver;
	u32 queued;
	u32 in_use;
};

static struct ipc_port ports[MAX_PORTS];
static struct ipc_message_node messages[MAX_MESSAGES];

static int str_eq(const char *left, const char *right)
{
	while (*left != '\0' && *right != '\0') {
		if (*left++ != *right++) {
			return 0;
		}
	}

	return *left == *right;
}

static struct ipc_message_node *message_alloc(void)
{
	for (u32 i = 0; i < MAX_MESSAGES; i++) {
		if (messages[i].in_use) {
			continue;
		}

		messages[i].in_use = 1;
		messages[i].next = 0;
		return &messages[i];
	}

	return 0;
}

static void message_free(struct ipc_message_node *node)
{
	node->in_use = 0;
	node->next = 0;
}

void ipc_init(void)
{
	for (u32 i = 0; i < MAX_PORTS; i++) {
		ports[i].in_use = 0;
		ports[i].head = 0;
		ports[i].tail = 0;
		ports[i].receiver = 0;
		ports[i].queued = 0;
	}

	for (u32 i = 0; i < MAX_MESSAGES; i++) {
		messages[i].in_use = 0;
		messages[i].next = 0;
	}

	console_printf("ipc: init ports=%u messages=%u\n", MAX_PORTS, MAX_MESSAGES);
}

struct ipc_port *ipc_port_create(const char *name)
{
	for (u32 i = 0; i < MAX_PORTS; i++) {
		if (!ports[i].in_use) {
			ports[i].in_use = 1;
			ports[i].name = name;
			console_printf("ipc: port create %s\n", name);
			return &ports[i];
		}
	}

	console_printf("ipc: port table full for %s\n", name);
	return 0;
}

struct ipc_port *ipc_port_find(const char *name)
{
	for (u32 i = 0; i < MAX_PORTS; i++) {
		if (ports[i].in_use && str_eq(ports[i].name, name)) {
			return &ports[i];
		}
	}

	return 0;
}

int ipc_send(struct ipc_port *port, const struct ipc_message *message)
{
	struct ipc_message_node *node;

	if (port == 0 || message == 0) {
		return -1;
	}

	node = message_alloc();
	if (node == 0) {
		console_printf("ipc: message pool exhausted port=%s\n", port->name);
		return -1;
	}

	node->message = *message;
	node->message.sender = task_id(task_current());

	if (port->tail != 0) {
		port->tail->next = node;
	} else {
		port->head = node;
	}

	port->tail = node;
	port->queued++;
	console_printf("ipc: send port=%s type=%u sender=%u queued=%u\n",
		       port->name, node->message.type, node->message.sender,
		       port->queued);

	if (port->receiver != 0) {
		struct thread *receiver = port->receiver;
		port->receiver = 0;
		thread_unblock(receiver);
	}

	return 0;
}

int ipc_recv(struct ipc_port *port, struct ipc_message *message)
{
	if (port == 0 || message == 0) {
		return -1;
	}

	while (port->head == 0) {
		port->receiver = thread_current();
		console_printf("ipc: recv block port=%s\n", port->name);
		thread_block();
	}

	struct ipc_message_node *node = port->head;
	port->head = node->next;
	if (port->head == 0) {
		port->tail = 0;
	}

	port->queued--;
	*message = node->message;
	message_free(node);

	console_printf("ipc: recv port=%s type=%u sender=%u queued=%u\n",
		       port->name, message->type, message->sender, port->queued);
	return 0;
}
