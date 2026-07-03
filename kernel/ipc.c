#include "console.h"
#include "ipc.h"
#include "sched.h"
#include "spinlock.h"

enum {
	MAX_PORTS = 32,
	MAX_MESSAGES = 32,
};

struct ipc_message_node {
	struct ipc_message message;
	struct ipc_message_node *next;
	u32 in_use;
};

struct ipc_port {
	const char *name;
	u64 id;
	struct ipc_message_node *head;
	struct ipc_message_node *tail;
	struct thread *receiver;
	u32 queued;
	u32 in_use;
};

static struct ipc_port ports[MAX_PORTS];
static struct ipc_message_node messages[MAX_MESSAGES];
static struct ipc_port *kernel_reply_port;
static u64 next_port_id = 1;
static struct spinlock ipc_lock = SPINLOCK_INIT("ipc");

static int str_eq(const char *left, const char *right)
{
	while (*left != '\0' && *right != '\0') {
		if (*left++ != *right++) {
			return 0;
		}
	}

	return *left == *right;
}

static struct ipc_port *ipc_port_find_locked(const char *name)
{
	for (u32 i = 0; i < MAX_PORTS; i++) {
		if (ports[i].in_use && str_eq(ports[i].name, name)) {
			return &ports[i];
		}
	}

	return 0;
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
		ports[i].id = 0;
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
	kernel_reply_port = ipc_port_create("kernel-rpc");
}

static struct ipc_port *ipc_port_alloc(const char *name, u32 reuse_named)
{
	const u64 flags = spin_lock_irqsave(&ipc_lock);
	struct ipc_port *existing = reuse_named ? ipc_port_find_locked(name) : 0;
	if (reuse_named && existing != 0) {
		console_printf("ipc: port existing %s id=%u\n", name,
			       (u32)existing->id);
		spin_unlock_irqrestore(&ipc_lock, flags);
		return existing;
	}

	for (u32 i = 0; i < MAX_PORTS; i++) {
		if (!ports[i].in_use) {
			ports[i].in_use = 1;
			ports[i].id = next_port_id++;
			ports[i].name = name;
			console_printf("ipc: port create %s id=%u\n", name,
				       (u32)ports[i].id);
			spin_unlock_irqrestore(&ipc_lock, flags);
			return &ports[i];
		}
	}

	spin_unlock_irqrestore(&ipc_lock, flags);
	console_printf("ipc: port table full for %s\n", name);
	return 0;
}

struct ipc_port *ipc_port_create(const char *name)
{
	return ipc_port_alloc(name, 1);
}

struct ipc_port *ipc_port_create_private(const char *name)
{
	return ipc_port_alloc(name, 0);
}

struct ipc_port *ipc_port_find(const char *name)
{
	const u64 flags = spin_lock_irqsave(&ipc_lock);
	struct ipc_port *port = ipc_port_find_locked(name);

	spin_unlock_irqrestore(&ipc_lock, flags);
	return port;
}

const char *ipc_port_name(const struct ipc_port *port)
{
	return port != 0 ? port->name : "(null)";
}

u64 ipc_port_id(const struct ipc_port *port)
{
	return port != 0 ? port->id : 0;
}

struct ipc_port *ipc_port_from_id(u64 id)
{
	if (id == 0) {
		return 0;
	}

	const u64 flags = spin_lock_irqsave(&ipc_lock);

	for (u32 i = 0; i < MAX_PORTS; i++) {
		if (ports[i].in_use && ports[i].id == id) {
			spin_unlock_irqrestore(&ipc_lock, flags);
			return &ports[i];
		}
	}

	spin_unlock_irqrestore(&ipc_lock, flags);
	return 0;
}

int ipc_send(struct ipc_port *port, const struct ipc_message *message)
{
	struct ipc_message_node *node;

	if (port == 0 || message == 0) {
		return -1;
	}

	const u64 flags = spin_lock_irqsave(&ipc_lock);
	node = message_alloc();
	if (node == 0) {
		console_printf("ipc: message pool exhausted port=%s\n", port->name);
		spin_unlock_irqrestore(&ipc_lock, flags);
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
	console_printf("ipc: send port=%s proto=0x%x type=%u sender=%u queued=%u\n",
		       port->name, node->message.protocol, node->message.type,
		       node->message.sender, port->queued);

	struct thread *receiver = port->receiver;
	if (port->receiver != 0) {
		port->receiver = 0;
	}

	spin_unlock_irqrestore(&ipc_lock, flags);
	thread_unblock(receiver);
	return 0;
}

int ipc_recv(struct ipc_port *port, struct ipc_message *message)
{
	if (port == 0 || message == 0) {
		return -1;
	}

	u64 flags = spin_lock_irqsave(&ipc_lock);
	while (port->head == 0) {
		port->receiver = thread_current();
		console_printf("ipc: recv block port=%s\n", port->name);
		spin_unlock_irqrestore(&ipc_lock, flags);
		thread_block();
		flags = spin_lock_irqsave(&ipc_lock);
	}

	struct ipc_message_node *node = port->head;
	port->head = node->next;
	if (port->head == 0) {
		port->tail = 0;
	}

	port->queued--;
	*message = node->message;
	message_free(node);

	console_printf("ipc: recv port=%s proto=0x%x type=%u sender=%u queued=%u\n",
		       port->name, message->protocol, message->type,
		       message->sender, port->queued);
	spin_unlock_irqrestore(&ipc_lock, flags);
	return 0;
}

int ipc_call_kernel(struct ipc_port *port, const struct ipc_message *request,
		    struct ipc_message *reply)
{
	struct ipc_message call;

	if (kernel_reply_port == 0 || port == 0 || request == 0 || reply == 0) {
		return -1;
	}

	call = *request;
	call.reply_port = kernel_reply_port;

	if (ipc_send(port, &call) != 0) {
		return -1;
	}

	sched_run();
	return ipc_recv(kernel_reply_port, reply);
}
