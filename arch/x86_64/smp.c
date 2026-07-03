#include <arch/smp.h>
#include <arch/interrupts.h>
#include <arch/io.h>
#include <arch/user.h>
#include "console.h"
#include "multiboot2.h"
#include "sched.h"
#include "timer.h"
#include "vm.h"

enum {
	MAX_SMP_CPUS = 8,
	AP_BOOT_TIMEOUT_TICKS = 100,
	AP_STACK_SIZE = 16384,
	AP_TRAMPOLINE_PHYS = 0x7000,
	ACPI_MADT_SIGNATURE = 0x43495041,
	MADT_TYPE_LOCAL_APIC = 0,
	LAPIC_REG_ID = 0x20,
	LAPIC_REG_EOI = 0xb0,
	LAPIC_REG_SVR = 0xf0,
	LAPIC_REG_LVT_TIMER = 0x320,
	LAPIC_REG_ICR_LOW = 0x300,
	LAPIC_REG_ICR_HIGH = 0x310,
	LAPIC_REG_TIMER_INIT_COUNT = 0x380,
	LAPIC_REG_TIMER_DIVIDE = 0x3e0,
	LAPIC_ENABLE = 1 << 8,
	LAPIC_TIMER_PERIODIC = 1 << 17,
	LAPIC_ICR_DELIVERY_STATUS = 1 << 12,
	LAPIC_ICR_INIT = 5 << 8,
	LAPIC_ICR_STARTUP = 6 << 8,
	LAPIC_ICR_FIXED = 0,
	LAPIC_ICR_LEVEL_ASSERT = 1 << 14,
	LAPIC_ICR_TRIGGER_LEVEL = 1 << 15,
	MSR_GS_BASE = 0xc0000101,
	IRQ_SCHED_IPI_VECTOR = 64,
	IRQ_LAPIC_TIMER_VECTOR = 65,
	LAPIC_TIMER_INITIAL_COUNT = 1000000,
};

struct cpu_local {
	u32 cpu_id;
	u32 reserved;
	u64 syscall_stack;
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
static u32 lapic_ready;
static volatile u32 ap_started_count;
static volatile u32 ap_scheduler_count;
static volatile u32 release_aps;
static volatile u32 ap_online[MAX_SMP_CPUS];
static volatile u32 cpu_local_ready;
static struct cpu_local cpu_locals[MAX_SMP_CPUS];
u64 arch_smp_ap_stack_top;
u32 arch_smp_ap_boot_cpu;
static u8 ap_stacks[MAX_SMP_CPUS][AP_STACK_SIZE] __attribute__((aligned(16)));

extern u8 arch_smp_ap_trampoline_start[];
extern u8 arch_smp_ap_trampoline_end[];

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

static volatile u32 *lapic_reg(u32 reg)
{
	return (volatile u32 *)((u64)lapic_mmio_address + reg);
}

static u32 lapic_read(u32 reg)
{
	return *lapic_reg(reg);
}

static u32 cpu_index_from_lapic_id(u32 apic_id)
{
	for (u32 i = 0; i < cpu_count; i++) {
		if (lapic_ids[i] == apic_id) {
			return i;
		}
	}

	return 0;
}

static void set_current_cpu_id(u32 cpu_id)
{
	cpu_locals[cpu_id].cpu_id = cpu_id;
	cpu_locals[cpu_id].reserved = 0;
	cpu_locals[cpu_id].syscall_stack = 0;
	arch_wrmsr(MSR_GS_BASE, (u64)&cpu_locals[cpu_id]);
	cpu_local_ready = 1;
}

void arch_smp_set_syscall_stack(u64 stack)
{
	cpu_locals[arch_smp_current_cpu_id()].syscall_stack = stack;
}

static void lapic_write(u32 reg, u32 value)
{
	*lapic_reg(reg) = value;
	(void)lapic_read(LAPIC_REG_ID);
}

static int lapic_wait_delivery(const char *stage)
{
	u32 spins = 1000000;

	while ((lapic_read(LAPIC_REG_ICR_LOW) & LAPIC_ICR_DELIVERY_STATUS) != 0) {
		if (spins-- == 0) {
			console_printf("smp: lapic delivery timeout stage=%s\n",
				       stage);
			return -1;
		}
		__asm__ volatile ("pause");
	}

	return 0;
}

static void lapic_eoi(void)
{
	lapic_write(LAPIC_REG_EOI, 0);
}

static void lapic_enable_current_cpu(void)
{
	lapic_write(LAPIC_REG_SVR, lapic_read(LAPIC_REG_SVR) | LAPIC_ENABLE | 0xff);
}

static void lapic_timer_init_current_cpu(void)
{
	if (!lapic_ready) {
		return;
	}

	lapic_enable_current_cpu();
	lapic_write(LAPIC_REG_TIMER_DIVIDE, 0x3);
	lapic_write(LAPIC_REG_LVT_TIMER,
		    LAPIC_TIMER_PERIODIC | IRQ_LAPIC_TIMER_VECTOR);
	lapic_write(LAPIC_REG_TIMER_INIT_COUNT, LAPIC_TIMER_INITIAL_COUNT);
	console_printf("timer: lapic cpu=%u periodic\n",
		       arch_smp_current_cpu_id());
}

static void delay_ticks(u64 ticks)
{
	const u64 end = timer_ticks() + ticks;

	while (timer_ticks() < end) {
		__asm__ volatile ("pause");
	}
}

static void copy_trampoline(void)
{
	u8 *dst = (u8 *)AP_TRAMPOLINE_PHYS;
	const u8 *src = arch_smp_ap_trampoline_start;
	const u64 len = (u64)(arch_smp_ap_trampoline_end -
			      arch_smp_ap_trampoline_start);

	for (u64 i = 0; i < len; i++) {
		dst[i] = src[i];
	}
}

static int lapic_init(void)
{
	if (lapic_mmio_address == 0 ||
	    vm_map_kernel_page(lapic_mmio_address, lapic_mmio_address, 1) != 0) {
		console_printf("smp: lapic map failed addr=%p\n",
			       (const void *)(u64)lapic_mmio_address);
		return -1;
	}

	set_current_cpu_id(cpu_index_from_lapic_id(lapic_read(LAPIC_REG_ID) >> 24));
	lapic_ready = 1;
	lapic_timer_init_current_cpu();
	return 0;
}

static int start_ap(u32 cpu_index)
{
	const u8 vector = AP_TRAMPOLINE_PHYS >> 12;
	const u32 apic_id = lapic_ids[cpu_index];
	const u64 deadline = timer_ticks() + AP_BOOT_TIMEOUT_TICKS;

	arch_smp_ap_boot_cpu = cpu_index;
	arch_smp_ap_stack_top = (u64)(ap_stacks[cpu_index] + AP_STACK_SIZE);
	ap_online[cpu_index] = 0;
	__sync_synchronize();

	lapic_write(LAPIC_REG_ICR_HIGH, apic_id << 24);
	lapic_write(LAPIC_REG_ICR_LOW,
		    LAPIC_ICR_INIT | LAPIC_ICR_LEVEL_ASSERT |
		    LAPIC_ICR_TRIGGER_LEVEL);
	if (lapic_wait_delivery("init") != 0) {
		return -1;
	}
	delay_ticks(1);

	lapic_write(LAPIC_REG_ICR_HIGH, apic_id << 24);
	lapic_write(LAPIC_REG_ICR_LOW, LAPIC_ICR_STARTUP | vector);
	if (lapic_wait_delivery("sipi1") != 0) {
		return -1;
	}
	delay_ticks(1);

	lapic_write(LAPIC_REG_ICR_HIGH, apic_id << 24);
	lapic_write(LAPIC_REG_ICR_LOW, LAPIC_ICR_STARTUP | vector);
	if (lapic_wait_delivery("sipi2") != 0) {
		return -1;
	}

	while (timer_ticks() < deadline) {
		if (ap_online[cpu_index] != 0) {
			console_printf("smp: ap online cpu=%u lapic=%u\n",
				       cpu_index, apic_id);
			return 0;
		}
		__asm__ volatile ("pause");
	}

	console_printf("smp: ap start timeout cpu=%u lapic=%u\n",
		       cpu_index, apic_id);
	return -1;
}

static void start_aps(void)
{
	if (cpu_count < 2 || lapic_init() != 0) {
		return;
	}

	copy_trampoline();
	for (u32 i = 1; i < cpu_count; i++) {
		(void)start_ap(i);
	}

	console_printf("smp: started aps=%u\n", ap_started_count);
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
	start_aps();
}

u32 arch_smp_cpu_count(void)
{
	return cpu_count;
}

u32 arch_smp_current_cpu_id(void)
{
	u32 cpu_id;

	if (!cpu_local_ready) {
		return 0;
	}

	__asm__ volatile ("movl %%gs:0, %0" : "=r"(cpu_id));
	return cpu_id;
}

u32 arch_smp_lapic_id(u32 cpu_index)
{
	return cpu_index < cpu_count ? lapic_ids[cpu_index] : 0;
}

u64 arch_smp_lapic_address(void)
{
	return lapic_mmio_address;
}

void arch_smp_send_scheduler_ipi(u32 cpu_index)
{
	if (!lapic_ready || cpu_index >= cpu_count ||
	    cpu_index == arch_smp_current_cpu_id()) {
		return;
	}

	lapic_write(LAPIC_REG_ICR_HIGH, lapic_ids[cpu_index] << 24);
	lapic_write(LAPIC_REG_ICR_LOW, LAPIC_ICR_FIXED | IRQ_SCHED_IPI_VECTOR);
	(void)lapic_wait_delivery("sched");
	console_printf("sched: ipi cpu=%u\n", cpu_index);
}

void arch_smp_handle_scheduler_ipi(void)
{
	lapic_eoi();
}

void arch_smp_handle_timer_interrupt(void)
{
	lapic_eoi();
}

u32 arch_smp_started_count(void)
{
	return ap_started_count;
}

void arch_smp_release_aps(void)
{
	if (ap_started_count == 0) {
		return;
	}

	release_aps = 1;
	__sync_synchronize();

	const u64 deadline = timer_ticks() + AP_BOOT_TIMEOUT_TICKS;
	while (timer_ticks() < deadline && ap_scheduler_count < ap_started_count) {
		__asm__ volatile ("pause");
	}

	console_printf("smp: scheduler aps=%u\n", ap_scheduler_count);
}

void arch_smp_ap_entry(u32 cpu_index)
{
	ap_online[cpu_index] = 1;
	__sync_fetch_and_add(&ap_started_count, 1);
	set_current_cpu_id(cpu_index);

	while (!release_aps) {
		__asm__ volatile ("pause");
	}

	arch_interrupts_load();
	arch_user_init_cpu(cpu_index);
	lapic_timer_init_current_cpu();
	__sync_fetch_and_add(&ap_scheduler_count, 1);
	sched_secondary_start(cpu_index);
}
