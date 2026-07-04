#include "buffer.h"
#include "console.h"
#include "ipc.h"
#include "sched.h"
#include "slab.h"
#include "spinlock.h"

enum {
	PROTO_BLOCK = ('B') | ('L' << 8) | ('K' << 16) | ('0' << 24),
	PROTO_VFS = ('V') | ('F' << 8) | ('S' << 16) | ('0' << 24),
};

struct ipc_message_node {
	struct ipc_message message;
	struct ipc_message_node *next;
};

struct ipc_port {
	const char *name;
	u64 id;
	u32 ref_count;
	u32 immortal;
	struct ipc_port *next;
	struct ipc_message_node *head;
	struct ipc_message_node *tail;
	struct thread *receiver;
	u32 queued;
};

static struct ipc_port *ports;
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

static int ipc_should_log(const struct ipc_port *port,
			  const struct ipc_message *message)
{
	if (port == 0 || message == 0) {
		return 1;
	}

	if (message->protocol == PROTO_BLOCK || message->protocol == PROTO_VFS ||
	    str_eq(port->name, "reply")) {
		return 0;
	}

	return 1;
}

static struct ipc_port *ipc_port_find_locked(const char *name)
{
	for (struct ipc_port *port = ports; port != 0; port = port->next) {
		if (str_eq(port->name, name)) {
			return port;
		}
	}

	return 0;
}

static struct ipc_message_node *message_alloc(void)
{
	struct ipc_message_node *node = slab_alloc(sizeof(*node));
	if (node != 0) {
		node->next = 0;
	}
	return node;
}

static void message_free(struct ipc_message_node *node)
{
	if (node != 0) {
		ipc_message_release(&node->message);
	}
	slab_free(node);
}

static void ipc_message_retain(struct ipc_message *message)
{
	if (message == 0) {
		return;
	}

	ipc_port_retain(message->reply_port);
	if (message->cap_type == IPC_CAP_PORT) {
		ipc_port_retain((struct ipc_port *)message->cap_object);
	} else if (message->cap_type == IPC_CAP_BUFFER) {
		buffer_retain((struct shared_buffer *)message->cap_object);
	}
}

void ipc_message_release(struct ipc_message *message)
{
	if (message != 0 && message->cap_type == IPC_CAP_PORT) {
		ipc_port_release((struct ipc_port *)message->cap_object);
	} else if (message != 0 && message->cap_type == IPC_CAP_BUFFER) {
		buffer_release((struct shared_buffer *)message->cap_object);
	}
	if (message != 0) {
		ipc_port_release(message->reply_port);
		message->reply_port = 0;
		message->cap_type = IPC_CAP_NONE;
		message->cap_object = 0;
		message->cap_rights = 0;
	}
}

void ipc_init(void)
{
	ports = 0;
	console_printf("ipc: init slab ports messages\n");
	kernel_reply_port = ipc_port_create("kernel-rpc");
}

static void ipc_port_remove_locked(struct ipc_port *port)
{
	struct ipc_port **link = &ports;

	while (*link != 0) {
		if (*link == port) {
			*link = port->next;
			port->next = 0;
			return;
		}
		link = &(*link)->next;
	}
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

	struct ipc_port *port = slab_zalloc(sizeof(*port));
	if (port != 0) {
		port->id = next_port_id++;
		port->name = name;
		port->ref_count = 0;
		port->immortal = reuse_named;
		port->next = ports;
		ports = port;
		console_printf("ipc: port create %s id=%u\n", name,
			       (u32)port->id);
		spin_unlock_irqrestore(&ipc_lock, flags);
		return port;
	}

	spin_unlock_irqrestore(&ipc_lock, flags);
	console_printf("ipc: port alloc failed for %s\n", name);
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

void ipc_port_retain(struct ipc_port *port)
{
	if (port == 0) {
		return;
	}

	const u64 flags = spin_lock_irqsave(&ipc_lock);

	port->ref_count++;
	spin_unlock_irqrestore(&ipc_lock, flags);
}

void ipc_port_release(struct ipc_port *port)
{
	if (port == 0) {
		return;
	}

	const u64 flags = spin_lock_irqsave(&ipc_lock);

	if (port->ref_count > 0) {
		port->ref_count--;
	}
	if (port->immortal || port->ref_count != 0 || port->queued != 0 ||
	    port->receiver != 0) {
		spin_unlock_irqrestore(&ipc_lock, flags);
		return;
	}

	const char *name = port->name;
	ipc_port_remove_locked(port);
	port->id = 0;
	port->name = 0;
	spin_unlock_irqrestore(&ipc_lock, flags);
	slab_free(port);
	console_printf("ipc: port destroy %s\n", name);
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

	for (struct ipc_port *port = ports; port != 0; port = port->next) {
		if (port->id == id) {
			spin_unlock_irqrestore(&ipc_lock, flags);
			return port;
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

	node = message_alloc();
	if (node == 0) {
		console_printf("ipc: message alloc failed port=%s\n", port->name);
		return -1;
	}

	node->message = *message;
	node->message.sender = task_id(task_current());
	ipc_message_retain(&node->message);

	const u64 flags = spin_lock_irqsave(&ipc_lock);
	if (port->tail != 0) {
		port->tail->next = node;
	} else {
		port->head = node;
	}

	port->tail = node;
	port->queued++;
	if (ipc_should_log(port, &node->message)) {
		console_printf("ipc: send port=%s proto=0x%x type=%u sender=%u queued=%u\n",
			       port->name, node->message.protocol,
			       node->message.type, node->message.sender,
			       port->queued);
	}

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
		if (!str_eq(port->name, "block") &&
		    !str_eq(port->name, "vfs") &&
		    !str_eq(port->name, "reply")) {
			console_printf("ipc: recv block port=%s\n", port->name);
		}
		thread_prepare_block();
		spin_unlock_irqrestore(&ipc_lock, flags);
		thread_block_prepared();
		flags = spin_lock_irqsave(&ipc_lock);
	}

	struct ipc_message_node *node = port->head;
	port->head = node->next;
	if (port->head == 0) {
		port->tail = 0;
	}

	port->queued--;
	*message = node->message;
	node->message.reply_port = 0;
	node->message.cap_type = IPC_CAP_NONE;
	node->message.cap_object = 0;
	node->message.cap_rights = 0;
	message_free(node);

	if (ipc_should_log(port, message)) {
		console_printf("ipc: recv port=%s proto=0x%x type=%u sender=%u queued=%u\n",
			       port->name, message->protocol, message->type,
			       message->sender, port->queued);
	}
	spin_unlock_irqrestore(&ipc_lock, flags);
	return 0;
}

void ipc_cancel_thread(struct thread *thread)
{
	if (thread == 0) {
		return;
	}

	const u64 flags = spin_lock_irqsave(&ipc_lock);

	for (struct ipc_port *port = ports; port != 0; port = port->next) {
		if (port->receiver == thread) {
			port->receiver = 0;
		}
	}

	spin_unlock_irqrestore(&ipc_lock, flags);
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
