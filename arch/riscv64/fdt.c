#include <arch/fdt.h>

enum {
	FDT_MAGIC = 0xd00dfeed,
	FDT_BEGIN_NODE = 1,
	FDT_END_NODE = 2,
	FDT_PROP = 3,
	FDT_NOP = 4,
	FDT_END = 9,
};

struct fdt_header {
	u32 magic;
	u32 totalsize;
	u32 off_dt_struct;
	u32 off_dt_strings;
	u32 off_mem_rsvmap;
	u32 version;
	u32 last_comp_version;
	u32 boot_cpuid_phys;
	u32 size_dt_strings;
	u32 size_dt_struct;
};

static u32 be32(const void *ptr)
{
	const u8 *p = (const u8 *)ptr;

	return ((u32)p[0] << 24) | ((u32)p[1] << 16) |
	       ((u32)p[2] << 8) | p[3];
}

static u64 read_cells(const u32 *cells, u32 count)
{
	u64 value = 0;

	for (u32 i = 0; i < count; i++) {
		value = (value << 32) | be32(&cells[i]);
	}
	return value;
}

static u64 align4(u64 value)
{
	return (value + 3) & ~3ULL;
}

static int str_eq(const char *left, const char *right)
{
	while (*left != '\0' && *right != '\0') {
		if (*left++ != *right++) {
			return 0;
		}
	}
	return *left == *right;
}

static int node_is_memory(const char *name)
{
	u32 i = 0;
	const char prefix[] = "memory";

	while (prefix[i] != '\0') {
		if (name[i] != prefix[i]) {
			return 0;
		}
		i++;
	}
	return name[i] == '\0' || name[i] == '@';
}

int riscv64_fdt_scan_memory(const void *fdt,
			    struct riscv64_fdt_memory_range *ranges,
			    u32 capacity)
{
	const struct fdt_header *header = (const struct fdt_header *)fdt;
	u32 address_cells = 2;
	u32 size_cells = 2;
	u32 depth = 0;
	u32 in_memory = 0;
	u32 found = 0;
	u64 cursor;
	u64 struct_end;
	const char *strings;

	if (fdt == 0 || ranges == 0 || capacity == 0 ||
	    be32(&header->magic) != FDT_MAGIC) {
		return -1;
	}

	cursor = (u64)fdt + be32(&header->off_dt_struct);
	struct_end = cursor + be32(&header->size_dt_struct);
	strings = (const char *)fdt + be32(&header->off_dt_strings);

	while (cursor + sizeof(u32) <= struct_end) {
		const u32 token = be32((const void *)cursor);
		cursor += sizeof(u32);

		if (token == FDT_BEGIN_NODE) {
			const char *name = (const char *)cursor;

			if (depth == 1 && node_is_memory(name)) {
				in_memory = depth + 1;
			}
			while (cursor < struct_end &&
			       *(const char *)cursor != '\0') {
				cursor++;
			}
			cursor = align4(cursor + 1);
			depth++;
			continue;
		}

		if (token == FDT_END_NODE) {
			if (in_memory == depth) {
				in_memory = 0;
			}
			if (depth > 0) {
				depth--;
			}
			continue;
		}

		if (token == FDT_PROP) {
			u32 len;
			u32 nameoff;
			const char *name;
			const u8 *data;

			if (cursor + 2 * sizeof(u32) > struct_end) {
				return -1;
			}
			len = be32((const void *)cursor);
			nameoff = be32((const void *)(cursor + sizeof(u32)));
			cursor += 2 * sizeof(u32);
			if (cursor + len > struct_end) {
				return -1;
			}
			data = (const u8 *)cursor;
			name = strings + nameoff;

			if (depth == 1 && str_eq(name, "#address-cells") &&
			    len >= sizeof(u32)) {
				address_cells = be32(data);
			} else if (depth == 1 && str_eq(name, "#size-cells") &&
				   len >= sizeof(u32)) {
				size_cells = be32(data);
			} else if (in_memory != 0 && str_eq(name, "reg")) {
				const u32 cells_per_range = address_cells + size_cells;
				u32 offset = 0;

				if (cells_per_range == 0 ||
				    cells_per_range > 4) {
					return -1;
				}
				while (offset + cells_per_range * sizeof(u32) <= len &&
				       found < capacity) {
					const u32 *cells = (const u32 *)(data + offset);

					ranges[found].base =
						read_cells(cells, address_cells);
					ranges[found].size =
						read_cells(cells + address_cells,
							   size_cells);
					found++;
					offset += cells_per_range * sizeof(u32);
				}
			}
			cursor = align4(cursor + len);
			continue;
		}

		if (token == FDT_NOP) {
			continue;
		}

		if (token == FDT_END) {
			return (int)found;
		}

		return -1;
	}

	return -1;
}
