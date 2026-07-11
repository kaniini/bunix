#include <arch/boot.h>
#include <arch/console.h>
#include <arch/fdt.h>
#include <arch/interrupts.h>
#include <arch/power.h>
#include <arch/sbi.h>
#include <arch/smp.h>
#include <arch/thread.h>
#include <arch/user.h>
#include <arch/vm.h>
#include "bootpkg.h"
#include "buffer.h"
#include "cmdline.h"
#include "console.h"
#include "elf.h"
#include "ipc.h"
#include "name.h"
#include "pmm.h"
#include "runtime.h"
#include "sched.h"
#include "server.h"
#include "slab.h"
#include "vm.h"
#include "../servers/vm/vm_server.h"

static struct riscv64_boot_info boot_info;
static struct arch_thread_context boot_context;
static struct arch_thread_context worker_context;
static volatile u32 worker_switched;
static volatile u32 sched_worker_ran;
static u8 worker_stack[4096] __attribute__((aligned(16)));
static u8 user_probe_stack[4096] __attribute__((aligned(16)));
static u8 user_probe_kernel_stack[4096] __attribute__((aligned(16)));
static u8 vm_hook_test_page[4096] __attribute__((aligned(4096)));
extern u8 __kernel_start[];
extern u8 __kernel_end[];

static const char riscv64_bootpkg_abi_smoke[] = "abi-smoke.user";

enum {
	USER_STACK_TOP = 0x800000ULL,
	USER_STACK_PAGE = USER_STACK_TOP - RISCV64_PAGE_SIZE,
	RISCV64_TIMEBASE_HZ = 10000000ULL,
	RISCV64_TIMER_HZ = 100ULL,
};

const struct riscv64_boot_info *riscv64_boot_info(void)
{
	return &boot_info;
}

static void sbi_putchar(char ch)
{
	riscv64_sbi_putchar(ch);
}

static void early_puts(const char *text)
{
	while (*text != '\0') {
		if (*text == '\n') {
			sbi_putchar('\r');
		}
		sbi_putchar(*text++);
	}
}

static void early_put_u64_dec(u64 value)
{
	char digits[20];
	u32 count = 0;

	if (value == 0) {
		sbi_putchar('0');
		return;
	}
	while (value != 0 && count < sizeof(digits)) {
		digits[count++] = (char)('0' + (value % 10));
		value /= 10;
	}
	while (count != 0) {
		sbi_putchar(digits[--count]);
	}
}

static void early_put_u64_hex(u64 value)
{
	static const char hex[] = "0123456789abcdef";
	int started = 0;

	early_puts("0x");
	for (int shift = 60; shift >= 0; shift -= 4) {
		const u8 nibble = (value >> shift) & 0xf;

		if (nibble != 0 || started != 0 || shift == 0) {
			sbi_putchar(hex[nibble]);
			started = 1;
		}
	}
}

static void early_put_kv_dec(const char *key, u64 value)
{
	early_puts(key);
	early_put_u64_dec(value);
	early_puts("\n");
}

static void early_put_kv_hex(const char *key, u64 value)
{
	early_puts(key);
	early_put_u64_hex(value);
	early_puts("\n");
}

static void early_put_kv_str(const char *key, const char *value)
{
	early_puts(key);
	early_puts(value);
	early_puts("\n");
}

static int early_str_contains(const char *text, const char *needle)
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

static void pmm_bootstrap_from_fdt(u64 fdt)
{
	struct riscv64_fdt_memory_range memory_ranges[8];
	struct pmm_memory_range available[8];
	struct pmm_reserved_range reserved[5];
	const int memory_count = riscv64_fdt_scan_memory(
		(const void *)fdt, memory_ranges,
		sizeof(memory_ranges) / sizeof(memory_ranges[0]));
	const u64 fdt_size = riscv64_fdt_total_size((const void *)fdt);
	u64 available_count = 0;
	u64 reserved_count = 0;

	if (memory_count <= 0) {
		return;
	}
	for (int i = 0; i < memory_count; i++) {
		available[available_count].base = memory_ranges[i].base;
		available[available_count].length = memory_ranges[i].size;
		available_count++;
	}

	if (boot_info.phys_base < (u64)__kernel_start) {
		reserved[reserved_count].start = boot_info.phys_base;
		reserved[reserved_count].end = (u64)__kernel_start;
		reserved_count++;
	}
	reserved[reserved_count].start = (u64)__kernel_start;
	reserved[reserved_count].end = (u64)__kernel_end;
	reserved_count++;
	if (boot_info.initrd_end > boot_info.initrd_start) {
		reserved[reserved_count].start = boot_info.initrd_start;
		reserved[reserved_count].end = boot_info.initrd_end;
		reserved_count++;
	}
	if (fdt_size != 0) {
		reserved[reserved_count].start = fdt;
		reserved[reserved_count].end = fdt + fdt_size;
		reserved_count++;
	}

	pmm_init_from_ranges(available, available_count, reserved, reserved_count);
}

static void boot_layout_smoke(u64 fdt)
{
	const u64 fdt_size = riscv64_fdt_total_size((const void *)fdt);

	early_put_kv_hex("boot: riscv64 memory-base=", boot_info.phys_base);
	early_put_kv_hex("boot: riscv64 memory-size=", boot_info.phys_size);
	early_put_kv_hex("boot: riscv64 kernel-start=", (u64)__kernel_start);
	early_put_kv_hex("boot: riscv64 kernel-end=", (u64)__kernel_end);
	if (boot_info.initrd_end > boot_info.initrd_start) {
		early_put_kv_hex("boot: riscv64 initrd-start=",
				 boot_info.initrd_start);
		early_put_kv_hex("boot: riscv64 initrd-end=",
				 boot_info.initrd_end);
		early_put_kv_hex("boot: riscv64 initrd-size=",
				 boot_info.initrd_end - boot_info.initrd_start);
	}
	if (fdt_size != 0) {
		early_put_kv_hex("boot: riscv64 fdt-start=", fdt);
		early_put_kv_hex("boot: riscv64 fdt-end=", fdt + fdt_size);
		early_put_kv_hex("boot: riscv64 fdt-size=", fdt_size);
	}
}

static void worker_thread_main(void)
{
	worker_switched = 1;
	arch_thread_switch(&worker_context, &boot_context);
	for (;;) {
		__asm__ volatile ("wfi");
	}
}

static int context_switch_self_test(void)
{
	worker_switched = 0;
	arch_thread_context_init_current(&boot_context);
	arch_thread_context_init(&worker_context,
				 worker_stack + sizeof(worker_stack),
				 worker_thread_main);
	arch_thread_switch(&boot_context, &worker_context);
	return worker_switched == 1 ? 0 : -1;
}

static void sched_worker_thread(void *arg)
{
	(void)arg;

	sched_worker_ran = 1;
	thread_exit();
}

static void generic_services_init(u64 fdt)
{
	arch_smp_init(fdt);
	kernel_runtime_memory_init();
	kernel_runtime_services_init();
	kernel_runtime_boot_modules_init();
	kernel_runtime_scheduler_init();
}

static int scheduler_self_test(void)
{
	sched_worker_ran = 0;
	if (thread_create(task_current(), "rv64-sched-smoke",
			  sched_worker_thread, 0) == 0) {
		return -1;
	}
	sched_run();
	return sched_worker_ran == 1 ? 0 : -1;
}

static void platform_discovery_smoke(u64 fdt)
{
	struct riscv64_fdt_platform platform;

	if (riscv64_fdt_scan_platform((const void *)fdt, &platform) != 0) {
		return;
	}
	if (platform.cpu_count != 0) {
		early_puts("fdt: riscv64 cpus\n");
		early_put_kv_dec("fdt: riscv64 cpu-count=", platform.cpu_count);
		early_put_kv_dec("smp: riscv64 discovered-harts=",
				 platform.cpu_count);
		early_put_kv_dec("smp: riscv64 started-harts=", 1);
		early_put_kv_dec("smp: riscv64 boot-hart=", boot_info.hart_id);
		early_put_kv_str("smp: riscv64 secondary-policy=", "parked");
	}
	if (platform.timebase_frequency != 0) {
		early_puts("fdt: riscv64 timer\n");
		early_put_kv_dec("fdt: riscv64 timebase-hz=",
				 platform.timebase_frequency);
	}
	if (platform.stdout_path[0] != '\0') {
		early_puts("fdt: riscv64 stdout\n");
		early_put_kv_str("fdt: riscv64 stdout-path=",
				 platform.stdout_path);
	}
	if (platform.stdout_uart_valid != 0) {
		early_puts("fdt: riscv64 stdout-uart\n");
		early_put_kv_str("fdt: riscv64 stdout-resolved=",
				 platform.stdout_resolved_path);
		early_put_kv_hex("fdt: riscv64 stdout-uart-base=",
				 platform.uarts[platform.stdout_uart_index].reg_base);
	}
	if (platform.uart_count != 0) {
		early_puts("fdt: riscv64 uart\n");
		early_put_kv_dec("fdt: riscv64 uart-count=",
				 platform.uart_count);
	}
	if (platform.interrupt_controller_count != 0) {
		const struct riscv64_fdt_device *routing =
			&platform.interrupt_controllers[0];

		for (u32 i = 0; i < platform.interrupt_controller_count; i++) {
			const struct riscv64_fdt_device *candidate =
				&platform.interrupt_controllers[i];

			if (!early_str_contains(candidate->compatible,
						"cpu-intc")) {
				routing = candidate;
				break;
			}
		}
		early_puts("fdt: riscv64 interrupt-controller\n");
		early_put_kv_str("fdt: riscv64 interrupt-controller-path=",
				 platform.interrupt_controllers[0].path);
		early_put_kv_str("fdt: riscv64 interrupt-controller-compatible=",
				 platform.interrupt_controllers[0].compatible);
		early_put_kv_dec("fdt: riscv64 interrupt-controller-count=",
				 platform.interrupt_controller_count);
		early_put_kv_str("fdt: riscv64 interrupt-routing-path=",
				 routing->path);
		early_put_kv_str("fdt: riscv64 interrupt-routing-compatible=",
				 routing->compatible);
	}
}

static int vm_hook_self_test(void)
{
	struct arch_vm_space space;
	const u64 vaddr = RISCV64_USER_BASE + 0x20000;
	const u64 phys = (u64)vm_hook_test_page;
	const u64 offset = 123;

	if (arch_vm_space_init(&space) != 0) {
		return -1;
	}
	if (arch_vm_map_page(&space, vaddr, phys, 1, 1) != 0) {
		arch_vm_space_destroy(&space);
		return -1;
	}
	if (arch_vm_translate(&space, vaddr + offset, 1) != phys + offset) {
		arch_vm_space_destroy(&space);
		return -1;
	}
	if (arch_vm_protect_page(&space, vaddr, 0) != 0 ||
	    arch_vm_translate(&space, vaddr + offset, 1) != 0 ||
	    arch_vm_translate(&space, vaddr + offset, 0) != phys + offset) {
		arch_vm_space_destroy(&space);
		return -1;
	}
	if (arch_vm_unmap_page(&space, vaddr) != phys ||
	    arch_vm_translate(&space, vaddr + offset, 0) != 0) {
		arch_vm_space_destroy(&space);
		return -1;
	}
	arch_vm_space_destroy(&space);
	return 0;
}

static u16 read_le16(const u8 *data)
{
	return (u16)data[0] | ((u16)data[1] << 8);
}

static u64 read_le64(const u8 *data)
{
	u64 value = 0;

	for (u32 i = 0; i < 8; i++) {
		value |= (u64)data[i] << (i * 8);
	}
	return value;
}

static int user_elf_is_riscv64(u64 start, u64 size)
{
	const u8 *elf = (const u8 *)start;
	const u16 elf_machine_riscv = 243;

	if (size < 64) {
		return 0;
	}
	if (elf[0] != 0x7f || elf[1] != 'E' || elf[2] != 'L' ||
	    elf[3] != 'F') {
		return 0;
	}
	if (elf[4] != 2 || elf[5] != 1) {
		return 0;
	}
	if (read_le16(elf + 18) != elf_machine_riscv) {
		return 0;
	}
	if (read_le64(elf + 24) == 0 || read_le64(elf + 32) == 0) {
		return 0;
	}
	if (read_le16(elf + 56) == 0) {
		return 0;
	}
	return 1;
}

static int memory_equal(const u8 *left, const u8 *right, u64 size)
{
	for (u64 i = 0; i < size; i++) {
		if (left[i] != right[i]) {
			return 0;
		}
	}
	return 1;
}

static int user_copy_self_test(void)
{
	static const u8 expected[] = { 'r', 'v', '6', '4', '-', 'c', 'o', 'p', 'y' };
	u8 actual[sizeof(expected)];
	const u64 user_addr = USER_STACK_PAGE + 128;
	struct vm_space *space = vm_server_bootstrap_space("rv64-copy-smoke");
	int ok = 0;

	for (u64 i = 0; i < sizeof(actual); i++) {
		actual[i] = 0;
	}
	if (space == 0 ||
	    vm_alloc_user_page(space, USER_STACK_PAGE, 1).addr == 0) {
		return -1;
	}
	vm_rpc_activate_space(space);
	ok = arch_user_copy_to(user_addr, expected, sizeof(expected)) == 0 &&
	     arch_user_copy_from(actual, user_addr, sizeof(actual)) == 0 &&
	     memory_equal(actual, expected, sizeof(expected)) &&
	     arch_user_copy_from(actual, 0, sizeof(actual)) != 0 &&
	     arch_user_copy_to(RISCV64_USER_LIMIT - 1, expected,
			       sizeof(expected)) != 0;
	vm_rpc_activate_space(vm_kernel_space());
	vm_rpc_destroy_space(space);
	return ok ? 0 : -1;
}

static int generic_elf_loader_self_test(u64 module_start, u64 module_size)
{
	struct vm_space *space = vm_server_bootstrap_space("rv64-elf-smoke");
	u64 entry = 0;
	int rc;

	if (space == 0) {
		return -1;
	}
	rc = elf_load_user_image(space, module_start,
				 module_start + module_size, &entry);
	vm_rpc_destroy_space(space);
	return rc == 0 && entry != 0 ? 0 : -1;
}

static int configure_bootpkg_cmdline(void)
{
	if (!bootpkg_magic_ok(boot_info.initrd_start, boot_info.initrd_end)) {
		kernel_cmdline_configure("");
		return 0;
	}
	return bootpkg_configure_cmdline(boot_info.initrd_start,
					 boot_info.initrd_end);
}

static void riscv64_run_lowlevel_self_tests(u64 fdt)
{
	(void)fdt;

	if (scheduler_self_test() == 0) {
		early_puts("sched: riscv64 thread\n");
	}
	riscv64_timer_set_relative(RISCV64_TIMEBASE_HZ / RISCV64_TIMER_HZ);
	arch_interrupts_enable();
	while (arch_timer_ticks() == 0) {
		__asm__ volatile ("wfi");
	}
	arch_interrupts_disable();
	early_puts("timer: riscv64 tick\n");
	if (context_switch_self_test() == 0) {
		early_puts("thread: riscv64 switch\n");
	}
	if (vm_hook_self_test() == 0) {
		early_puts("vm: riscv64 hooks\n");
	}
	if (riscv64_syscall_entry_self_test() == 0) {
		early_puts("syscall: riscv64 ecall\n");
	}
	if (riscv64_user_mode_self_test((u64)riscv64_user_ecall_probe,
					(u64)(user_probe_stack +
					      sizeof(user_probe_stack)),
					(u64)(user_probe_kernel_stack +
					      sizeof(user_probe_kernel_stack))) == 0) {
		early_puts("user: riscv64 mode\n");
	}
}

void riscv64_early_main(u64 hart_id, u64 fdt)
{
	struct riscv64_fdt_memory_range memory;
	struct riscv64_fdt_initrd initrd;
	u64 module_start = 0;
	u64 module_size = 0;
	int diag_enabled;
	int selftest_enabled;

	boot_info.hart_id = hart_id;
	boot_info.fdt = fdt;
	boot_info.phys_base = RISCV64_PHYS_MEM_BASE;
	boot_info.phys_size = 0;
	boot_info.kernel_load_base = RISCV64_KERNEL_LOAD_BASE;
	boot_info.direct_map_base = RISCV64_DIRECT_MAP_BASE;
	boot_info.direct_map_size = RISCV64_DIRECT_MAP_SIZE;
	boot_info.user_base = RISCV64_USER_BASE;
	boot_info.user_limit = RISCV64_USER_LIMIT;
	boot_info.initrd_start = 0;
	boot_info.initrd_end = 0;
	if (riscv64_fdt_scan_memory((const void *)fdt, &memory, 1) == 1) {
		boot_info.phys_base = memory.base;
		boot_info.phys_size = memory.size;
	}
	if (riscv64_fdt_scan_initrd((const void *)fdt, &initrd) == 0) {
		boot_info.initrd_start = initrd.start;
		boot_info.initrd_end = initrd.end;
	}

	console_init();
	early_puts("bunixos: riscv64 early bootstrap\n");
	configure_bootpkg_cmdline();
	diag_enabled = kernel_cmdline_has("riscv64-diag");
	selftest_enabled = kernel_cmdline_has("riscv64-selftest");
	pmm_bootstrap_from_fdt(fdt);
	if (pmm_total_page_count() != 0 && pmm_free_page_count() != 0) {
		early_puts("pmm: riscv64 ranges\n");
	}
	if (diag_enabled) {
		boot_layout_smoke(fdt);
		platform_discovery_smoke(fdt);
	}
	generic_services_init(fdt);
	arch_interrupts_init();
	if (selftest_enabled) {
		riscv64_run_lowlevel_self_tests(fdt);
	}
	if (bootpkg_magic_ok(boot_info.initrd_start, boot_info.initrd_end)) {
		early_puts("bootpkg: riscv64 initrd\n");
		if (kernel_cmdline_has("riscv64-bootpkg-test")) {
			early_puts("cmdline: riscv64 bootpkg\n");
			if (kernel_cmdline_has("riscv64-uart-console")) {
				riscv64_console_init_from_fdt((const void *)fdt);
			}
		}
		if (bootpkg_record_modules(boot_info.initrd_start,
					   boot_info.initrd_end) == 0 &&
		    server_boot_module_registered(riscv64_bootpkg_abi_smoke)) {
			early_puts("module: riscv64 registered\n");
		}
		if (bootpkg_find_module(boot_info.initrd_start,
					boot_info.initrd_end,
					riscv64_bootpkg_abi_smoke,
					&module_start, &module_size) == 0 &&
		    user_elf_is_riscv64(module_start, module_size)) {
			early_puts("module: riscv64 user elf\n");
			if (generic_elf_loader_self_test(module_start,
							 module_size) == 0) {
				early_puts("elf: riscv64 loader\n");
			}
			if (user_copy_self_test() == 0) {
				early_puts("copy: riscv64 user\n");
			}
			riscv64_timer_set_relative(RISCV64_TIMEBASE_HZ /
						   RISCV64_TIMER_HZ);
			arch_interrupts_enable();
			server_start_initial_boot_modules();
			sched_enable_preemption();
			sched_idle_loop();
		}
	}
	arch_poweroff();

	for (;;) {
		__asm__ volatile ("wfi");
	}
}
