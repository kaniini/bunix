#include <arch/fdt.h>

enum {
	FDT_MAGIC = 0xd00dfeed,
	FDT_BEGIN_NODE = 1,
	FDT_END_NODE = 2,
	FDT_PROP = 3,
	FDT_NOP = 4,
	FDT_END = 9,
	FDT_MAX_DEPTH = 16,
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

static int str_contains(const char *text, const char *needle)
{
	if (*needle == '\0') {
		return 1;
	}
	for (u32 i = 0; text[i] != '\0'; i++) {
		u32 j = 0;

		while (needle[j] != '\0' && text[i + j] == needle[j]) {
			j++;
		}
		if (needle[j] == '\0') {
			return 1;
		}
	}
	return 0;
}

static void str_copy_len(char *dst, u32 capacity, const char *src, u32 len)
{
	u32 i = 0;

	if (dst == 0 || capacity == 0) {
		return;
	}
	while (i + 1 < capacity && i < len && src[i] != '\0') {
		dst[i] = src[i];
		i++;
	}
	dst[i] = '\0';
}

static void str_copy_cstr(char *dst, u32 capacity, const char *src)
{
	u32 len = 0;

	while (src[len] != '\0') {
		len++;
	}
	str_copy_len(dst, capacity, src, len);
}

static void path_join(char *dst, u32 capacity, const char *parent,
		      const char *name)
{
	u32 out = 0;

	if (capacity == 0) {
		return;
	}
	if (parent[0] == '\0' || str_eq(parent, "/")) {
		dst[out++] = '/';
	} else {
		while (out + 1 < capacity && parent[out] != '\0') {
			dst[out] = parent[out];
			out++;
		}
		if (out + 1 < capacity) {
			dst[out++] = '/';
		}
	}
	for (u32 i = 0; out + 1 < capacity && name[i] != '\0'; i++) {
		dst[out++] = name[i];
	}
	dst[out] = '\0';
}

static int compatible_has(const u8 *data, u32 len, const char *needle)
{
	u32 cursor = 0;

	while (cursor < len) {
		const char *entry = (const char *)data + cursor;
		u32 entry_len = 0;

		while (cursor + entry_len < len && entry[entry_len] != '\0') {
			entry_len++;
		}
		if (entry_len != 0 && str_contains(entry, needle)) {
			return 1;
		}
		cursor += entry_len + 1;
	}
	return 0;
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

static u64 read_prop_cells(const u8 *data, u32 len)
{
	const u32 count = len / sizeof(u32);

	if (len == 0 || (len % sizeof(u32)) != 0 || count > 2) {
		return 0;
	}
	return read_cells((const u32 *)data, count);
}

static void platform_record_device(struct riscv64_fdt_device *devices,
				   u32 *count, const char *path,
				   const char *compatible, u64 reg_base,
				   u64 reg_size)
{
	struct riscv64_fdt_device *device;

	if (*count >= RISCV64_FDT_MAX_DEVICES) {
		return;
	}
	device = &devices[*count];
	str_copy_cstr(device->path, sizeof(device->path), path);
	str_copy_cstr(device->compatible, sizeof(device->compatible), compatible);
	device->reg_base = reg_base;
	device->reg_size = reg_size;
	(*count)++;
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

int riscv64_fdt_scan_initrd(const void *fdt,
			    struct riscv64_fdt_initrd *initrd)
{
	const struct fdt_header *header = (const struct fdt_header *)fdt;
	u32 depth = 0;
	u32 in_chosen = 0;
	u32 found_start = 0;
	u32 found_end = 0;
	u64 cursor;
	u64 struct_end;
	const char *strings;

	if (fdt == 0 || initrd == 0 || be32(&header->magic) != FDT_MAGIC) {
		return -1;
	}

	initrd->start = 0;
	initrd->end = 0;
	cursor = (u64)fdt + be32(&header->off_dt_struct);
	struct_end = cursor + be32(&header->size_dt_struct);
	strings = (const char *)fdt + be32(&header->off_dt_strings);

	while (cursor + sizeof(u32) <= struct_end) {
		const u32 token = be32((const void *)cursor);
		cursor += sizeof(u32);

		if (token == FDT_BEGIN_NODE) {
			const char *name = (const char *)cursor;

			if (depth == 1 && str_eq(name, "chosen")) {
				in_chosen = depth + 1;
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
			if (in_chosen == depth) {
				in_chosen = 0;
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

			if (in_chosen != 0 &&
			    str_eq(name, "linux,initrd-start")) {
				initrd->start = read_prop_cells(data, len);
				found_start = initrd->start != 0;
			} else if (in_chosen != 0 &&
				   str_eq(name, "linux,initrd-end")) {
				initrd->end = read_prop_cells(data, len);
				found_end = initrd->end != 0;
			}
			cursor = align4(cursor + len);
			continue;
		}

		if (token == FDT_NOP) {
			continue;
		}

		if (token == FDT_END) {
			return found_start != 0 && found_end != 0 &&
			       initrd->end > initrd->start ? 0 : -1;
		}

		return -1;
	}

	return -1;
}

int riscv64_fdt_scan_platform(const void *fdt,
			      struct riscv64_fdt_platform *platform)
{
	const struct fdt_header *header = (const struct fdt_header *)fdt;
	u32 depth = 0;
	u64 cursor;
	u64 struct_end;
	const char *strings;
	char paths[FDT_MAX_DEPTH][RISCV64_FDT_MAX_PATH];
	char compatible[FDT_MAX_DEPTH][RISCV64_FDT_MAX_COMPATIBLE];
	char device_type[FDT_MAX_DEPTH][16];
	u32 address_cells[FDT_MAX_DEPTH];
	u32 size_cells[FDT_MAX_DEPTH];
	u32 parent_address_cells[FDT_MAX_DEPTH];
	u32 parent_size_cells[FDT_MAX_DEPTH];
	u64 reg_base[FDT_MAX_DEPTH];
	u64 reg_size[FDT_MAX_DEPTH];
	u32 has_reg[FDT_MAX_DEPTH];
	u32 interrupt_controller[FDT_MAX_DEPTH];

	if (fdt == 0 || platform == 0 || be32(&header->magic) != FDT_MAGIC) {
		return -1;
	}

	for (u32 i = 0; i < sizeof(*platform); i++) {
		((u8 *)platform)[i] = 0;
	}
	for (u32 i = 0; i < FDT_MAX_DEPTH; i++) {
		paths[i][0] = '\0';
		compatible[i][0] = '\0';
		device_type[i][0] = '\0';
		address_cells[i] = 2;
		size_cells[i] = 1;
		parent_address_cells[i] = 2;
		parent_size_cells[i] = 1;
		reg_base[i] = 0;
		reg_size[i] = 0;
		has_reg[i] = 0;
		interrupt_controller[i] = 0;
	}

	cursor = (u64)fdt + be32(&header->off_dt_struct);
	struct_end = cursor + be32(&header->size_dt_struct);
	strings = (const char *)fdt + be32(&header->off_dt_strings);

	while (cursor + sizeof(u32) <= struct_end) {
		const u32 token = be32((const void *)cursor);
		cursor += sizeof(u32);

		if (token == FDT_BEGIN_NODE) {
			const char *name = (const char *)cursor;
			u32 child_depth;

			if (depth + 1 >= FDT_MAX_DEPTH) {
				return -1;
			}
			child_depth = depth + 1;
			if (depth == 0 && name[0] == '\0') {
				str_copy_cstr(paths[child_depth],
					      sizeof(paths[child_depth]), "/");
			} else {
				path_join(paths[child_depth],
					  sizeof(paths[child_depth]),
					  paths[depth], name);
			}
			compatible[child_depth][0] = '\0';
			device_type[child_depth][0] = '\0';
			address_cells[child_depth] = address_cells[depth];
			size_cells[child_depth] = size_cells[depth];
			parent_address_cells[child_depth] = address_cells[depth];
			parent_size_cells[child_depth] = size_cells[depth];
			reg_base[child_depth] = 0;
			reg_size[child_depth] = 0;
			has_reg[child_depth] = 0;
			interrupt_controller[child_depth] = 0;

			while (cursor < struct_end &&
			       *(const char *)cursor != '\0') {
				cursor++;
			}
			cursor = align4(cursor + 1);
			depth = child_depth;
			continue;
		}

		if (token == FDT_END_NODE) {
			if (depth == 0) {
				return -1;
			}
			if (str_eq(device_type[depth], "cpu") &&
			    platform->cpu_count < RISCV64_FDT_MAX_CPUS) {
				struct riscv64_fdt_cpu *cpu =
					&platform->cpus[platform->cpu_count];

				cpu->hart_id = has_reg[depth] != 0 ?
					reg_base[depth] : platform->cpu_count;
				platform->cpu_count++;
			}
			if (compatible[depth][0] != '\0' &&
			    (str_contains(compatible[depth], "uart") ||
			     str_contains(compatible[depth], "ns16550"))) {
				platform_record_device(
					platform->uarts, &platform->uart_count,
					paths[depth], compatible[depth],
					reg_base[depth], reg_size[depth]);
			}
			if (interrupt_controller[depth] != 0) {
				platform_record_device(
					platform->interrupt_controllers,
					&platform->interrupt_controller_count,
					paths[depth], compatible[depth],
					reg_base[depth], reg_size[depth]);
			}
			depth--;
			continue;
		}

		if (token == FDT_PROP) {
			u32 len;
			u32 nameoff;
			const char *name;
			const u8 *data;

			if (cursor + 2 * sizeof(u32) > struct_end ||
			    depth == 0) {
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

			if (str_eq(name, "#address-cells") && len >= sizeof(u32)) {
				address_cells[depth] = be32(data);
			} else if (str_eq(name, "#size-cells") &&
				   len >= sizeof(u32)) {
				size_cells[depth] = be32(data);
			} else if (str_eq(name, "timebase-frequency") &&
				   len >= sizeof(u32) &&
				   str_eq(paths[depth], "/cpus")) {
				platform->timebase_frequency = be32(data);
			} else if (str_eq(name, "stdout-path") &&
				   str_eq(paths[depth], "/chosen")) {
				str_copy_len(platform->stdout_path,
					     sizeof(platform->stdout_path),
					     (const char *)data, len);
			} else if (str_eq(name, "device_type")) {
				str_copy_len(device_type[depth],
					     sizeof(device_type[depth]),
					     (const char *)data, len);
			} else if (str_eq(name, "compatible")) {
				str_copy_len(compatible[depth],
					     sizeof(compatible[depth]),
					     (const char *)data, len);
				if (compatible_has(data, len, "uart") &&
				    compatible[depth][0] == '\0') {
					str_copy_cstr(compatible[depth],
						      sizeof(compatible[depth]),
						      "uart");
				}
			} else if (str_eq(name, "interrupt-controller")) {
				interrupt_controller[depth] = 1;
			} else if (str_eq(name, "reg")) {
				const u32 a = parent_address_cells[depth];
				const u32 s = parent_size_cells[depth];
				const u32 cells_per_range = a + s;

				if (cells_per_range != 0 &&
				    cells_per_range <= 4 &&
				    len >= cells_per_range * sizeof(u32)) {
					const u32 *cells = (const u32 *)data;

					reg_base[depth] = read_cells(cells, a);
					reg_size[depth] = read_cells(cells + a, s);
					has_reg[depth] = 1;
				}
			}
			cursor = align4(cursor + len);
			continue;
		}

		if (token == FDT_NOP) {
			continue;
		}

		if (token == FDT_END) {
			return 0;
		}

		return -1;
	}

	return -1;
}

u64 riscv64_fdt_total_size(const void *fdt)
{
	const struct fdt_header *header = (const struct fdt_header *)fdt;

	if (fdt == 0 || be32(&header->magic) != FDT_MAGIC) {
		return 0;
	}
	return be32(&header->totalsize);
}
