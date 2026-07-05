#include "console.h"
#include "name.h"
#include "slab.h"
#include "spinlock.h"
#include "tree.h"

enum {
	NAME_MAX = 64,
};

struct name_entry {
	struct tree_node name_node;
	struct u64_tree_node id_node;
	u64 id;
	const char *name;
	enum name_service_kind kind;
	u64 object;
};

static struct tree names_by_name;
static struct u64_tree names_by_id;
static u64 next_name_id = 1;
static struct spinlock name_lock = SPINLOCK_INIT("name");

static char *name_copy(const char *name)
{
	u64 len = 0;
	char *copy;

	if (name == 0) {
		return 0;
	}
	while (len < NAME_MAX && name[len] != '\0') {
		len++;
	}
	if (len == 0 || len == NAME_MAX) {
		return 0;
	}
	copy = (char *)slab_alloc(len + 1);
	if (copy == 0) {
		return 0;
	}
	for (u64 i = 0; i <= len; i++) {
		copy[i] = name[i];
	}
	return copy;
}

void name_service_init(void)
{
	tree_init(&names_by_name);
	u64_tree_init(&names_by_id);

	console_printf("names: init dynamic\n");
}

u64 name_service_register(const char *name, enum name_service_kind kind,
			  u64 object)
{
	if (name == 0 || kind == NAME_SERVICE_EMPTY) {
		return 0;
	}

	const u64 flags = spin_lock_irqsave(&name_lock);
	struct name_entry *entry =
		(struct name_entry *)tree_get(&names_by_name, name);

	if (entry != 0) {
		entry->kind = kind;
		entry->object = object;
		console_printf("names: update name=%s id=%u kind=%u\n",
			       name, (u32)entry->id, kind);
		spin_unlock_irqrestore(&name_lock, flags);
		return entry->id;
	}

	entry = (struct name_entry *)slab_zalloc(sizeof(*entry));
	if (entry == 0) {
		spin_unlock_irqrestore(&name_lock, flags);
		console_printf("names: alloc failed for %s\n", name);
		return 0;
	}
	entry->name = name_copy(name);
	if (entry->name == 0) {
		slab_free(entry);
		spin_unlock_irqrestore(&name_lock, flags);
		console_printf("names: name copy failed for %s\n", name);
		return 0;
	}
	entry->id = next_name_id++;
	if (entry->id == 0) {
		entry->id = next_name_id++;
	}
	entry->kind = kind;
	entry->object = object;
	if (tree_insert_node(&names_by_name, &entry->name_node, entry->name,
			     (u64)entry) == 0 &&
	    u64_tree_insert_node(&names_by_id, &entry->id_node, entry->id,
				 (u64)entry) == 0) {
		console_printf("names: register name=%s id=%u kind=%u\n",
			       name, (u32)entry->id, kind);
		spin_unlock_irqrestore(&name_lock, flags);
		return entry->id;
	}

	if (entry->name_node.value != 0) {
		tree_remove_node(&names_by_name, &entry->name_node);
	}
	if (entry->id_node.value != 0) {
		u64_tree_remove_node(&names_by_id, &entry->id_node);
	}
	slab_free((void *)entry->name);
	slab_free(entry);
	spin_unlock_irqrestore(&name_lock, flags);
	console_printf("names: register failed for %s\n", name);
	return 0;
}

u64 name_service_lookup(const char *name)
{
	if (name == 0) {
		return 0;
	}

	const u64 flags = spin_lock_irqsave(&name_lock);
	const struct name_entry *entry =
		(const struct name_entry *)tree_get(&names_by_name, name);

	if (entry != 0) {
		console_printf("names: lookup name=%s id=%u\n",
			       name, (u32)entry->id);
		spin_unlock_irqrestore(&name_lock, flags);
		return entry->id;
	}

	spin_unlock_irqrestore(&name_lock, flags);
	console_printf("names: lookup miss name=%s\n", name);
	return 0;
}

enum name_service_kind name_service_kind(u64 id)
{
	const u64 flags = spin_lock_irqsave(&name_lock);
	const struct name_entry *entry =
		(const struct name_entry *)u64_tree_get(&names_by_id, id);

	if (entry != 0) {
		const enum name_service_kind kind = entry->kind;
		spin_unlock_irqrestore(&name_lock, flags);
		return kind;
	}

	spin_unlock_irqrestore(&name_lock, flags);
	return NAME_SERVICE_EMPTY;
}

u64 name_service_object(u64 id)
{
	const u64 flags = spin_lock_irqsave(&name_lock);
	const struct name_entry *entry =
		(const struct name_entry *)u64_tree_get(&names_by_id, id);

	if (entry != 0) {
		const u64 object = entry->object;
		spin_unlock_irqrestore(&name_lock, flags);
		return object;
	}

	spin_unlock_irqrestore(&name_lock, flags);
	return 0;
}
