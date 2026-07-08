#include <arch/smp.h>

void arch_smp_init(u64 boot_info)
{
	(void)boot_info;
}

void arch_smp_release_aps(void)
{
}

u32 arch_smp_cpu_count(void)
{
	return 1;
}

u32 arch_smp_current_cpu_id(void)
{
	return 0;
}

u32 arch_smp_lapic_id(u32 cpu_index)
{
	return cpu_index == 0 ? 0 : 0xffffffffu;
}

u64 arch_smp_lapic_address(void)
{
	return 0;
}

void arch_smp_lapic_eoi(void)
{
}

int arch_smp_ioapic_for_gsi(u32 gsi, u64 *base, u32 *ioapic_id,
			    u32 *input)
{
	(void)gsi;
	(void)base;
	(void)ioapic_id;
	(void)input;
	return -1;
}

int arch_smp_irq_override(u32 source, u32 *gsi, u16 *flags)
{
	if (gsi != 0) {
		*gsi = source;
	}
	if (flags != 0) {
		*flags = 0;
	}
	return 0;
}

void arch_smp_set_syscall_stack(u64 stack)
{
	(void)stack;
}

void arch_smp_send_scheduler_ipi(u32 cpu_index)
{
	(void)cpu_index;
}

void arch_smp_handle_scheduler_ipi(void)
{
}

void arch_smp_handle_timer_interrupt(void)
{
}

u32 arch_smp_started_count(void)
{
	return 1;
}
