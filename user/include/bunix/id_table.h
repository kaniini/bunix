#ifndef BUNIX_USER_ID_TABLE_H
#define BUNIX_USER_ID_TABLE_H

#include <bunix/alloc.h>
#include <bunix/syscall.h>
#include <bunix/tree.h>

struct bunix_id_slot {
	u64 id;
	u64 value;
};

struct bunix_id_table {
	struct bunix_id_slot *slots;
	u64 capacity;
	u64 next_id;
	u64 count;
};

struct bunix_map_entry {
	struct bunix_u64_tree_node node;
	u64 key;
	u64 value;
};

struct bunix_map {
	struct bunix_u64_tree tree;
	u64 count;
};

static inline void bunix_id_table_init(struct bunix_id_table *table)
{
	table->slots = 0;
	table->capacity = 0;
	table->next_id = 1;
	table->count = 0;
}

static inline int bunix_id_table_grow(struct bunix_id_table *table)
{
	const u64 old_capacity = table->capacity;
	const u64 new_capacity = old_capacity == 0 ? 8 : old_capacity * 2;
	struct bunix_id_slot *slots = (struct bunix_id_slot *)
		bunix_calloc(new_capacity, sizeof(slots[0]));

	if (slots == 0) {
		return -1;
	}
	for (u64 i = 0; i < old_capacity; i++) {
		slots[i] = table->slots[i];
	}
	bunix_free(table->slots);
	table->slots = slots;
	table->capacity = new_capacity;
	return 0;
}

static inline int bunix_id_table_ensure_space(struct bunix_id_table *table)
{
	if (table->count < table->capacity) {
		return 0;
	}
	return bunix_id_table_grow(table);
}

static inline u64 bunix_id_alloc(struct bunix_id_table *table, u64 value)
{
	if (table == 0 || value == 0 ||
	    bunix_id_table_ensure_space(table) != 0) {
		return 0;
	}

	for (u64 i = 0; i < table->capacity; i++) {
		struct bunix_id_slot *slot = &table->slots[i];

		if (slot->id != 0) {
			continue;
		}
		slot->id = table->next_id++;
		if (table->next_id == 0) {
			table->next_id = 1;
		}
		slot->value = value;
		table->count++;
		return slot->id;
	}

	return 0;
}

static inline u64 bunix_id_get(const struct bunix_id_table *table, u64 id)
{
	if (table == 0 || table->slots == 0 || id == 0) {
		return 0;
	}

	for (u64 i = 0; i < table->capacity; i++) {
		if (table->slots[i].id == id) {
			return table->slots[i].value;
		}
	}

	return 0;
}

static inline int bunix_id_set(struct bunix_id_table *table, u64 id, u64 value)
{
	if (table == 0 || table->slots == 0 || id == 0 || value == 0) {
		return -1;
	}

	for (u64 i = 0; i < table->capacity; i++) {
		if (table->slots[i].id == id) {
			table->slots[i].value = value;
			return 0;
		}
	}

	return -1;
}

static inline int bunix_id_remove(struct bunix_id_table *table, u64 id)
{
	if (table == 0 || table->slots == 0 || id == 0) {
		return -1;
	}

	for (u64 i = 0; i < table->capacity; i++) {
		if (table->slots[i].id == id) {
			table->slots[i].id = 0;
			table->slots[i].value = 0;
			if (table->count != 0) {
				table->count--;
			}
			return 0;
		}
	}

	return -1;
}

static inline void bunix_map_init(struct bunix_map *map)
{
	bunix_u64_tree_init(&map->tree);
	map->count = 0;
}

static inline struct bunix_map_entry *bunix_map_find_entry(
	const struct bunix_map *map, u64 key)
{
	struct bunix_u64_tree_node *node;

	if (map == 0 || key == 0) {
		return 0;
	}
	node = bunix_u64_tree_find_node(&map->tree, key);
	return node != 0 ? (struct bunix_map_entry *)node->value : 0;
}

static inline u64 bunix_map_get(const struct bunix_map *map, u64 key)
{
	struct bunix_map_entry *entry = bunix_map_find_entry(map, key);

	return entry != 0 ? entry->value : 0;
}

static inline int bunix_map_set(struct bunix_map *map, u64 key, u64 value)
{
	struct bunix_map_entry *entry;

	if (map == 0 || key == 0 || value == 0) {
		return -1;
	}

	entry = bunix_map_find_entry(map, key);
	if (entry != 0) {
		entry->value = value;
		return 0;
	}
	entry = (struct bunix_map_entry *)bunix_calloc(1, sizeof(*entry));
	if (entry == 0) {
		return -1;
	}
	entry->key = key;
	entry->value = value;
	if (bunix_u64_tree_insert_node(&map->tree, &entry->node, key,
				       (u64)entry) != 0) {
		bunix_free(entry);
		return -1;
	}
	map->count++;
	return 0;
}

static inline int bunix_map_remove(struct bunix_map *map, u64 key)
{
	struct bunix_map_entry *entry = bunix_map_find_entry(map, key);

	if (entry == 0) {
		return -1;
	}
	bunix_u64_tree_remove_node(&map->tree, &entry->node);
	bunix_free(entry);
	if (map->count != 0) {
		map->count--;
	}
	return 0;
}

static inline u64 bunix_map_at(const struct bunix_map *map, u64 index)
{
	struct bunix_u64_tree_node *node;

	if (map == 0) {
		return 0;
	}
	node = bunix_u64_tree_first_node(&map->tree);
	while (node != 0) {
		struct bunix_map_entry *entry =
			(struct bunix_map_entry *)node->value;

		if (entry != 0 && entry->value != 0) {
			if (index == 0) {
				return entry->value;
			}
			index--;
		}
		node = bunix_u64_tree_next_node(node);
	}
	return 0;
}

#endif
