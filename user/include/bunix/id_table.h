#ifndef BUNIX_USER_ID_TABLE_H
#define BUNIX_USER_ID_TABLE_H

#include <bunix/alloc.h>
#include <bunix/syscall.h>

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

struct bunix_map_slot {
	u64 key;
	u64 value;
};

struct bunix_map {
	struct bunix_map_slot *slots;
	u64 capacity;
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
	map->slots = 0;
	map->capacity = 0;
	map->count = 0;
}

static inline int bunix_map_grow(struct bunix_map *map)
{
	const u64 old_capacity = map->capacity;
	const u64 new_capacity = old_capacity == 0 ? 8 : old_capacity * 2;
	struct bunix_map_slot *slots = (struct bunix_map_slot *)
		bunix_calloc(new_capacity, sizeof(slots[0]));

	if (slots == 0) {
		return -1;
	}
	for (u64 i = 0; i < old_capacity; i++) {
		slots[i] = map->slots[i];
	}
	bunix_free(map->slots);
	map->slots = slots;
	map->capacity = new_capacity;
	return 0;
}

static inline int bunix_map_ensure_space(struct bunix_map *map)
{
	if (map->count < map->capacity) {
		return 0;
	}
	return bunix_map_grow(map);
}

static inline u64 bunix_map_get(const struct bunix_map *map, u64 key)
{
	if (map == 0 || map->slots == 0 || key == 0) {
		return 0;
	}

	for (u64 i = 0; i < map->capacity; i++) {
		if (map->slots[i].key == key) {
			return map->slots[i].value;
		}
	}

	return 0;
}

static inline int bunix_map_set(struct bunix_map *map, u64 key, u64 value)
{
	struct bunix_map_slot *free_slot = 0;

	if (map == 0 || key == 0 || value == 0 ||
	    bunix_map_ensure_space(map) != 0) {
		return -1;
	}

	for (u64 i = 0; i < map->capacity; i++) {
		struct bunix_map_slot *slot = &map->slots[i];

		if (slot->key == key) {
			slot->value = value;
			return 0;
		}
		if (slot->key == 0 && free_slot == 0) {
			free_slot = slot;
		}
	}

	if (free_slot == 0) {
		return -1;
	}
	free_slot->key = key;
	free_slot->value = value;
	map->count++;
	return 0;
}

static inline int bunix_map_remove(struct bunix_map *map, u64 key)
{
	if (map == 0 || map->slots == 0 || key == 0) {
		return -1;
	}

	for (u64 i = 0; i < map->capacity; i++) {
		if (map->slots[i].key == key) {
			map->slots[i].key = 0;
			map->slots[i].value = 0;
			if (map->count != 0) {
				map->count--;
			}
			return 0;
		}
	}

	return -1;
}

#endif
