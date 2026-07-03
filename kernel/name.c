#include "console.h"
#include "name.h"
#include "spinlock.h"

enum {
	MAX_NAMES = 32,
};

struct name_entry {
	u64 id;
	const char *name;
	enum name_service_kind kind;
	u64 object;
	u32 owner;
};

static struct name_entry names[MAX_NAMES];
static u64 next_name_id = 1;
static struct spinlock name_lock = SPINLOCK_INIT("name");

static int str_eq(const char *left, const char *right)
{
	while (*left != '\0' && *right != '\0') {
		if (*left++ != *right++) {
			return 0;
		}
	}

	return *left == *right;
}

void name_service_init(void)
{
	for (u32 i = 0; i < MAX_NAMES; i++) {
		names[i].id = 0;
		names[i].name = 0;
		names[i].kind = NAME_SERVICE_EMPTY;
		names[i].object = 0;
		names[i].owner = 0;
	}

	console_printf("names: init entries=%u\n", MAX_NAMES);
}

u64 name_service_register(const char *name, enum name_service_kind kind,
			  u64 object)
{
	if (name == 0 || kind == NAME_SERVICE_EMPTY) {
		return 0;
	}

	const u64 flags = spin_lock_irqsave(&name_lock);

	for (u32 i = 0; i < MAX_NAMES; i++) {
		if (names[i].id != 0 && str_eq(names[i].name, name)) {
			names[i].kind = kind;
			names[i].object = object;
			console_printf("names: update name=%s id=%u kind=%u\n",
				       name, (u32)names[i].id, kind);
			spin_unlock_irqrestore(&name_lock, flags);
			return names[i].id;
		}
	}

	for (u32 i = 0; i < MAX_NAMES; i++) {
		if (names[i].id != 0) {
			continue;
		}

		names[i].id = next_name_id++;
		names[i].name = name;
		names[i].kind = kind;
		names[i].object = object;
		console_printf("names: register name=%s id=%u kind=%u\n",
			       name, (u32)names[i].id, kind);
		spin_unlock_irqrestore(&name_lock, flags);
		return names[i].id;
	}

	spin_unlock_irqrestore(&name_lock, flags);
	console_printf("names: table full for %s\n", name);
	return 0;
}

u64 name_service_lookup(const char *name)
{
	if (name == 0) {
		return 0;
	}

	const u64 flags = spin_lock_irqsave(&name_lock);

	for (u32 i = 0; i < MAX_NAMES; i++) {
		if (names[i].id != 0 && str_eq(names[i].name, name)) {
			console_printf("names: lookup name=%s id=%u\n",
				       name, (u32)names[i].id);
			spin_unlock_irqrestore(&name_lock, flags);
			return names[i].id;
		}
	}

	spin_unlock_irqrestore(&name_lock, flags);
	console_printf("names: lookup miss name=%s\n", name);
	return 0;
}

enum name_service_kind name_service_kind(u64 id)
{
	const u64 flags = spin_lock_irqsave(&name_lock);

	for (u32 i = 0; i < MAX_NAMES; i++) {
		if (names[i].id == id) {
			const enum name_service_kind kind = names[i].kind;
			spin_unlock_irqrestore(&name_lock, flags);
			return kind;
		}
	}

	spin_unlock_irqrestore(&name_lock, flags);
	return NAME_SERVICE_EMPTY;
}

u64 name_service_object(u64 id)
{
	const u64 flags = spin_lock_irqsave(&name_lock);

	for (u32 i = 0; i < MAX_NAMES; i++) {
		if (names[i].id == id) {
			const u64 object = names[i].object;
			spin_unlock_irqrestore(&name_lock, flags);
			return object;
		}
	}

	spin_unlock_irqrestore(&name_lock, flags);
	return 0;
}
