#include "console.h"
#include "multiboot2.h"

enum {
	MULTIBOOT2_TAG_END = 0,
	MULTIBOOT2_TAG_MODULE = 3,
	MULTIBOOT2_TAG_MMAP = 6,
	MULTIBOOT2_TAG_ACPI_OLD = 14,
	MULTIBOOT2_TAG_ACPI_NEW = 15,
};

struct multiboot2_info {
	u32 total_size;
	u32 reserved;
};

struct multiboot2_tag {
	u32 type;
	u32 size;
};

struct multiboot2_tag_module {
	u32 type;
	u32 size;
	u32 mod_start;
	u32 mod_end;
	char cmdline[];
};

struct multiboot2_tag_mmap {
	u32 type;
	u32 size;
	u32 entry_size;
	u32 entry_version;
	u8 entries[];
};

struct multiboot2_tag_acpi {
	u32 type;
	u32 size;
	u8 rsdp[];
};

struct multiboot2_raw_mmap_entry {
	u64 base;
	u64 length;
	u32 type;
	u32 reserved;
};

static u64 align_up_u64(u64 value, u64 align)
{
	return (value + align - 1) & ~(align - 1);
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

u64 multiboot2_total_size(u64 info_addr)
{
	const struct multiboot2_info *info = (const struct multiboot2_info *)info_addr;

	return info->total_size;
}

void multiboot2_for_each_module(u64 info_addr, multiboot2_module_fn fn,
				void *ctx)
{
	const struct multiboot2_info *info = (const struct multiboot2_info *)info_addr;
	u64 cursor = info_addr + sizeof(*info);
	const u64 end = info_addr + info->total_size;

	while (cursor + sizeof(struct multiboot2_tag) <= end) {
		const struct multiboot2_tag *tag = (const struct multiboot2_tag *)cursor;

		if (tag->type == MULTIBOOT2_TAG_END) {
			return;
		}

		if (tag->type == MULTIBOOT2_TAG_MODULE) {
			const struct multiboot2_tag_module *raw =
				(const struct multiboot2_tag_module *)tag;
			const struct multiboot2_module module = {
				.start = raw->mod_start,
				.end = raw->mod_end,
				.cmdline = raw->cmdline,
			};
			fn(&module, ctx);
		}

		cursor = align_up_u64(cursor + tag->size, 8);
	}
}

void multiboot2_for_each_mmap(u64 info_addr, multiboot2_mmap_fn fn, void *ctx)
{
	const struct multiboot2_info *info = (const struct multiboot2_info *)info_addr;
	u64 cursor = info_addr + sizeof(*info);
	const u64 end = info_addr + info->total_size;

	while (cursor + sizeof(struct multiboot2_tag) <= end) {
		const struct multiboot2_tag *tag = (const struct multiboot2_tag *)cursor;

		if (tag->type == MULTIBOOT2_TAG_END) {
			return;
		}

		if (tag->type == MULTIBOOT2_TAG_MMAP) {
			const struct multiboot2_tag_mmap *mmap =
				(const struct multiboot2_tag_mmap *)tag;
			u64 entry_cursor = (u64)mmap->entries;
			const u64 entry_end = cursor + tag->size;

			while (entry_cursor + sizeof(struct multiboot2_raw_mmap_entry) <=
			       entry_end) {
				const struct multiboot2_raw_mmap_entry *raw =
					(const struct multiboot2_raw_mmap_entry *)entry_cursor;
				const struct multiboot2_mmap_entry entry = {
					.base = raw->base,
					.length = raw->length,
					.type = raw->type,
				};
				fn(&entry, ctx);
				entry_cursor += mmap->entry_size;
			}
		}

		cursor = align_up_u64(cursor + tag->size, 8);
	}
}

const void *multiboot2_acpi_rsdp(u64 info_addr)
{
	const struct multiboot2_info *info = (const struct multiboot2_info *)info_addr;
	u64 cursor = info_addr + sizeof(*info);
	const u64 end = info_addr + info->total_size;
	const void *old_rsdp = 0;

	while (cursor + sizeof(struct multiboot2_tag) <= end) {
		const struct multiboot2_tag *tag = (const struct multiboot2_tag *)cursor;

		if (tag->type == MULTIBOOT2_TAG_END) {
			return old_rsdp;
		}

		if (tag->type == MULTIBOOT2_TAG_ACPI_NEW) {
			const struct multiboot2_tag_acpi *acpi =
				(const struct multiboot2_tag_acpi *)tag;
			return acpi->rsdp;
		}

		if (tag->type == MULTIBOOT2_TAG_ACPI_OLD) {
			const struct multiboot2_tag_acpi *acpi =
				(const struct multiboot2_tag_acpi *)tag;
			old_rsdp = acpi->rsdp;
		}

		cursor = align_up_u64(cursor + tag->size, 8);
	}

	return old_rsdp;
}

static void dump_module(const struct multiboot2_module *module, void *ctx)
{
	(void)ctx;

	console_printf("multiboot2: module %p-%p %s\n",
		       (const void *)module->start,
		       (const void *)module->end,
		       module->cmdline);
}

static void dump_mmap(const struct multiboot2_mmap_entry *entry, void *ctx)
{
	(void)ctx;

	console_printf("multiboot2: mmap base=%p len=%p type=%u\n",
		       (const void *)entry->base,
		       (const void *)entry->length,
		       entry->type);
}

void multiboot2_dump(u64 info_addr)
{
	const struct multiboot2_info *info = (const struct multiboot2_info *)info_addr;

	console_printf("multiboot2: total_size=0x%x\n", info->total_size);

	multiboot2_for_each_mmap(info_addr, dump_mmap, 0);
	if (multiboot2_acpi_rsdp(info_addr) != 0) {
		console_printf("multiboot2: acpi rsdp=%p\n",
			       multiboot2_acpi_rsdp(info_addr));
	}
	multiboot2_for_each_module(info_addr, dump_module, 0);
}

int multiboot2_cmdline_eq(const struct multiboot2_module *module,
			  const char *cmdline)
{
	return str_eq(module->cmdline, cmdline);
}
