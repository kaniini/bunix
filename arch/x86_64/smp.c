#include <arch/smp.h>
#include "console.h"
#include "multiboot2.h"

enum {
	MAX_SMP_CPUS = 8,
	ACPI_MADT_SIGNATURE = 0x43495041,
	MADT_TYPE_LOCAL_APIC = 0,
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

struct madt {
	struct acpi_sdt_header header;
	u32 lapic_address;
	u32 flags;
	u8 entries[];
} __attribute__((packed));

struct madt_entry {
	u8 type;
	u8 length;
} __attribute__((packed));

struct madt_local_apic {
	u8 type;
	u8 length;
	u8 processor_id;
	u8 apic_id;
	u32 flags;
} __attribute__((packed));

static u32 cpu_count;
static u32 lapic_ids[MAX_SMP_CPUS];
static u32 lapic_mmio_address;

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

static const struct acpi_sdt_header *find_table_rsdt(const struct acpi_sdt_header *rsdt,
						     u32 signature)
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

static const struct acpi_sdt_header *find_table_xsdt(const struct acpi_sdt_header *xsdt,
						     u32 signature)
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

static void record_lapic(u8 apic_id, u32 flags)
{
	if ((flags & 1) == 0 || cpu_count >= MAX_SMP_CPUS) {
		return;
	}

	lapic_ids[cpu_count++] = apic_id;
}

static void parse_madt(const struct madt *madt)
{
	u64 cursor = (u64)madt->entries;
	const u64 end = (u64)madt + madt->header.length;

	lapic_mmio_address = madt->lapic_address;

	while (cursor + sizeof(struct madt_entry) <= end) {
		const struct madt_entry *entry = (const struct madt_entry *)cursor;

		if (entry->length == 0 || cursor + entry->length > end) {
			break;
		}

		if (entry->type == MADT_TYPE_LOCAL_APIC &&
		    entry->length >= sizeof(struct madt_local_apic)) {
			const struct madt_local_apic *lapic =
				(const struct madt_local_apic *)entry;
			record_lapic(lapic->apic_id, lapic->flags);
		}

		cursor += entry->length;
	}
}

void arch_smp_init(u64 multiboot_info)
{
	const struct rsdp *rsdp = (const struct rsdp *)multiboot2_acpi_rsdp(multiboot_info);
	const struct acpi_sdt_header *root;
	const struct acpi_sdt_header *madt_header = 0;

	cpu_count = 1;
	lapic_ids[0] = 0;

	if (rsdp == 0 || !mem_eq(rsdp->signature, "RSD PTR ", 8) ||
	    checksum8(rsdp, 20) != 0) {
		console_printf("smp: acpi unavailable cpus=1\n");
		return;
	}

	if (rsdp->revision >= 2 && rsdp->xsdt_address != 0 &&
	    checksum8(rsdp, rsdp->length) == 0) {
		root = (const struct acpi_sdt_header *)rsdp->xsdt_address;
		madt_header = find_table_xsdt(root, ACPI_MADT_SIGNATURE);
	} else {
		root = (const struct acpi_sdt_header *)(u64)rsdp->rsdt_address;
		madt_header = find_table_rsdt(root, ACPI_MADT_SIGNATURE);
	}

	if (madt_header == 0 || checksum8(madt_header, madt_header->length) != 0) {
		console_printf("smp: madt unavailable cpus=1\n");
		return;
	}

	cpu_count = 0;
	parse_madt((const struct madt *)madt_header);
	if (cpu_count == 0) {
		cpu_count = 1;
		lapic_ids[0] = 0;
	}

	console_printf("smp: discovered cpus=%u lapic=%p bsp_apic=%u\n",
		       cpu_count, (const void *)(u64)lapic_mmio_address,
		       lapic_ids[0]);
}

u32 arch_smp_cpu_count(void)
{
	return cpu_count;
}

u32 arch_smp_lapic_id(u32 cpu_index)
{
	return cpu_index < cpu_count ? lapic_ids[cpu_index] : 0;
}
