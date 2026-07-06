#include <arch/io.h>
#include <arch/power.h>
#include "console.h"
#include "multiboot2.h"

enum {
	ACPI_FADT_SIGNATURE = 0x50434146,
	ACPI_DSDT_SIGNATURE = 0x54445344,
	ACPI_SLEEP_ENABLE = 1 << 13,
};

struct rsdp {
	char signature[8];
	u8 checksum;
	char oem_id[6];
	u8 revision;
	u32 rsdt_address;
	u32 length;
	u64 xsdt_address;
	u8 extended_checksum;
	u8 reserved[3];
} __attribute__((packed));

struct acpi_sdt_header {
	u32 signature;
	u32 length;
	u8 revision;
	u8 checksum;
	char oem_id[6];
	char oem_table_id[8];
	u32 oem_revision;
	u32 creator_id;
	u32 creator_revision;
} __attribute__((packed));

struct fadt {
	struct acpi_sdt_header header;
	u32 firmware_ctrl;
	u32 dsdt;
	u8 reserved;
	u8 preferred_pm_profile;
	u16 sci_int;
	u32 smi_cmd;
	u8 acpi_enable;
	u8 acpi_disable;
	u8 s4bios_req;
	u8 pstate_cnt;
	u32 pm1a_evt_blk;
	u32 pm1b_evt_blk;
	u32 pm1a_cnt_blk;
	u32 pm1b_cnt_blk;
} __attribute__((packed));

static u16 pm1a_cnt_blk;
static u16 pm1b_cnt_blk;
static u16 slp_typa;
static u16 slp_typb;
static int acpi_power_ready;

static int mem_eq(const char *left, const char *right, u64 len)
{
	for (u64 i = 0; i < len; i++) {
		if (left[i] != right[i]) {
			return 0;
		}
	}

	return 1;
}

static u8 checksum8(const void *data, u32 len)
{
	const u8 *bytes = (const u8 *)data;
	u8 sum = 0;

	for (u32 i = 0; i < len; i++) {
		sum += bytes[i];
	}

	return sum;
}

static const struct acpi_sdt_header *find_table_rsdt(
	const struct acpi_sdt_header *rsdt, u32 signature)
{
	const u32 entries = (rsdt->length - sizeof(*rsdt)) / sizeof(u32);
	const u32 *table = (const u32 *)((const u8 *)rsdt + sizeof(*rsdt));

	for (u32 i = 0; i < entries; i++) {
		const struct acpi_sdt_header *header =
			(const struct acpi_sdt_header *)(u64)table[i];
		if (header->signature == signature) {
			return header;
		}
	}

	return 0;
}

static const struct acpi_sdt_header *find_table_xsdt(
	const struct acpi_sdt_header *xsdt, u32 signature)
{
	const u32 entries = (xsdt->length - sizeof(*xsdt)) / sizeof(u64);
	const u64 *table = (const u64 *)((const u8 *)xsdt + sizeof(*xsdt));

	for (u32 i = 0; i < entries; i++) {
		const struct acpi_sdt_header *header =
			(const struct acpi_sdt_header *)table[i];
		if (header->signature == signature) {
			return header;
		}
	}

	return 0;
}

static u64 aml_pkg_length_size(const u8 *aml, u64 offset, u64 end)
{
	if (offset >= end) {
		return 0;
	}

	return ((aml[offset] >> 6) & 3) + 1;
}

static int aml_read_int(const u8 *aml, u64 *offset, u64 end, u8 *value)
{
	if (*offset >= end) {
		return -1;
	}

	switch (aml[*offset]) {
	case 0x00:
		*value = 0;
		*offset += 1;
		return 0;
	case 0x01:
		*value = 1;
		*offset += 1;
		return 0;
	case 0x0a:
		if (*offset + 1 >= end) {
			return -1;
		}
		*value = aml[*offset + 1];
		*offset += 2;
		return 0;
	default:
		return -1;
	}
}

static int parse_s5(const struct acpi_sdt_header *dsdt)
{
	const u8 *aml = (const u8 *)dsdt + sizeof(*dsdt);
	const u64 len = dsdt->length - sizeof(*dsdt);

	for (u64 i = 0; i + 4 < len; i++) {
		if (!((aml[i] == '_' && aml[i + 1] == 'S' &&
		       aml[i + 2] == '5' && aml[i + 3] == '_') ||
		      (aml[i] == '\\' && i + 5 < len && aml[i + 1] == '_' &&
		       aml[i + 2] == 'S' && aml[i + 3] == '5' &&
		       aml[i + 4] == '_'))) {
			continue;
		}

		u64 off = i + (aml[i] == '\\' ? 5 : 4);
		const u64 end = len;
		while (off < end && aml[off] != 0x12) {
			off++;
		}
		if (off >= end || off + 2 >= end) {
			continue;
		}

		off++;
		off += aml_pkg_length_size(aml, off, end);
		if (off >= end) {
			continue;
		}

		off++;
		u8 typa = 0;
		u8 typb = 0;
		if (aml_read_int(aml, &off, end, &typa) == 0 &&
		    aml_read_int(aml, &off, end, &typb) == 0) {
			slp_typa = ((u16)typa) << 10;
			slp_typb = ((u16)typb) << 10;
			return 0;
		}
	}

	return -1;
}

void arch_power_init(u64 multiboot_info)
{
	const struct rsdp *rsdp =
		(const struct rsdp *)multiboot2_acpi_rsdp(multiboot_info);
	const struct acpi_sdt_header *root;
	const struct acpi_sdt_header *fadt_header;
	const struct acpi_sdt_header *dsdt_header;
	const struct fadt *fadt;

	if (rsdp == 0 || !mem_eq(rsdp->signature, "RSD PTR ", 8) ||
	    checksum8(rsdp, 20) != 0) {
		console_printf("power: acpi unavailable\n");
		return;
	}

	if (rsdp->revision >= 2 && rsdp->xsdt_address != 0 &&
	    checksum8(rsdp, rsdp->length) == 0) {
		root = (const struct acpi_sdt_header *)rsdp->xsdt_address;
		fadt_header = find_table_xsdt(root, ACPI_FADT_SIGNATURE);
	} else {
		root = (const struct acpi_sdt_header *)(u64)rsdp->rsdt_address;
		fadt_header = find_table_rsdt(root, ACPI_FADT_SIGNATURE);
	}

	if (fadt_header == 0 ||
	    checksum8(fadt_header, fadt_header->length) != 0) {
		console_printf("power: fadt unavailable\n");
		return;
	}

	fadt = (const struct fadt *)fadt_header;
	dsdt_header = (const struct acpi_sdt_header *)(u64)fadt->dsdt;
	if (dsdt_header == 0 || dsdt_header->signature != ACPI_DSDT_SIGNATURE ||
	    checksum8(dsdt_header, dsdt_header->length) != 0 ||
	    parse_s5(dsdt_header) != 0) {
		console_printf("power: s5 unavailable\n");
		return;
	}

	pm1a_cnt_blk = (u16)fadt->pm1a_cnt_blk;
	pm1b_cnt_blk = (u16)fadt->pm1b_cnt_blk;
	acpi_power_ready = pm1a_cnt_blk != 0;
	console_printf("power: acpi s5 pm1a=0x%x pm1b=0x%x typa=0x%x typb=0x%x\n",
		       pm1a_cnt_blk, pm1b_cnt_blk, slp_typa, slp_typb);
}

void arch_poweroff(void)
{
	console_printf("machine: poweroff\n");
	__asm__ volatile ("cli");
	if (acpi_power_ready) {
		arch_outw(pm1a_cnt_blk, slp_typa | ACPI_SLEEP_ENABLE);
		if (pm1b_cnt_blk != 0) {
			arch_outw(pm1b_cnt_blk, slp_typb | ACPI_SLEEP_ENABLE);
		}
	}

	for (;;) {
		__asm__ volatile ("hlt");
	}
}
