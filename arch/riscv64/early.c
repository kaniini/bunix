#include <arch/boot.h>
#include <arch/fdt.h>
#include <arch/interrupts.h>
#include <arch/sbi.h>
#include <arch/thread.h>
#include <arch/user.h>

static struct riscv64_boot_info boot_info;
static struct arch_thread_context boot_context;
static struct arch_thread_context worker_context;
static volatile u32 worker_switched;
static u8 worker_stack[4096] __attribute__((aligned(16)));
static u8 user_probe_stack[4096] __attribute__((aligned(16)));
static u8 user_probe_kernel_stack[4096] __attribute__((aligned(16)));
static u8 mapped_user_kernel_stack[4096] __attribute__((aligned(16)));
static u8 mapped_user_text[4096] __attribute__((aligned(4096)));
static u8 mapped_user_stack[4096] __attribute__((aligned(4096)));
static u64 early_root_table[512] __attribute__((aligned(4096)));
static u64 early_user_l1[512] __attribute__((aligned(4096)));
static u64 early_user_l0[512] __attribute__((aligned(4096)));

static const char riscv64_bootpkg_magic[] = "BUNIX-RV64-BOOTPKG\n";
static const char riscv64_bootpkg_module_prefix[] = "module ";
static const char riscv64_bootpkg_abi_smoke[] = "abi-smoke.user";

enum {
	SATP_MODE_SV39 = 8ULL << 60,
	USER_STACK_TOP = 0x800000ULL,
	USER_STACK_PAGE = USER_STACK_TOP - RISCV64_PAGE_SIZE,
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

static int bootpkg_magic_ok(u64 start, u64 end)
{
	const char *image = (const char *)start;
	const u64 magic_len = sizeof(riscv64_bootpkg_magic) - 1;

	if (start == 0 || end <= start || end - start < magic_len) {
		return 0;
	}
	for (u64 i = 0; i < magic_len; i++) {
		if (image[i] != riscv64_bootpkg_magic[i]) {
			return 0;
		}
	}
	return 1;
}

static int cstr_eq_len(const char *left, const char *right, u64 len)
{
	for (u64 i = 0; i < len; i++) {
		if (left[i] != right[i] || right[i] == '\0') {
			return 0;
		}
	}
	return right[len] == '\0';
}

static u64 cstr_len(const char *text)
{
	u64 len = 0;

	while (text[len] != '\0') {
		len++;
	}
	return len;
}

static u64 parse_decimal(const char *text, u64 len, u64 *used)
{
	u64 value = 0;
	u64 i = 0;

	while (i < len && text[i] >= '0' && text[i] <= '9') {
		value = value * 10 + (u64)(text[i] - '0');
		i++;
	}
	*used = i;
	return value;
}

static int bootpkg_find_module(u64 start, u64 end, const char *wanted,
			       u64 *module_start, u64 *module_size)
{
	const char *image = (const char *)start;
	const u64 prefix_len = cstr_len(riscv64_bootpkg_module_prefix);
	const u64 wanted_len = cstr_len(wanted);
	u64 cursor = sizeof(riscv64_bootpkg_magic) - 1;
	u64 payload = 0;
	u64 found_start = 0;
	u64 found_size = 0;

	while (start + cursor < end) {
		u64 line = cursor;
		u64 line_end = cursor;

		while (start + line_end < end && image[line_end] != '\n') {
			line_end++;
		}
		if (line_end == line) {
			payload = line_end + 1;
			break;
		}
		if (line_end - line > prefix_len + wanted_len + 1 &&
		    cstr_eq_len(image + line, riscv64_bootpkg_module_prefix,
				prefix_len) &&
		    cstr_eq_len(image + line + prefix_len, wanted,
				wanted_len) &&
		    image[line + prefix_len + wanted_len] == ' ') {
			u64 used = 0;
			const u64 size = parse_decimal(
				image + line + prefix_len + wanted_len + 1,
				line_end - line - prefix_len - wanted_len - 1,
				&used);

			if (used != 0) {
				found_size = size;
			}
		}
		cursor = line_end + 1;
	}

	if (payload == 0 || found_size == 0 || start + payload + found_size > end) {
		return -1;
	}
	found_start = start + payload;
	*module_start = found_start;
	*module_size = found_size;
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

static u32 read_le32(const u8 *data)
{
	return (u32)data[0] | ((u32)data[1] << 8) |
	       ((u32)data[2] << 16) | ((u32)data[3] << 24);
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

static void memory_copy(u8 *dest, const u8 *src, u64 size)
{
	for (u64 i = 0; i < size; i++) {
		dest[i] = src[i];
	}
}

static void memory_zero(u8 *dest, u64 size)
{
	for (u64 i = 0; i < size; i++) {
		dest[i] = 0;
	}
}

static u64 build_user_stack_mapped(u8 *stack_phys, u64 stack_vaddr,
				   const char *argv0)
{
	const char *src = argv0;
	const u64 argv0_len = cstr_len(argv0) + 1;
	u64 sp_offset = RISCV64_PAGE_SIZE;
	u64 string_offset;
	u64 *words;

	sp_offset -= argv0_len;
	string_offset = sp_offset;
	for (u64 i = 0; i < argv0_len; i++) {
		stack_phys[string_offset + i] = src[i];
	}
	sp_offset &= ~15ULL;
	sp_offset -= 4 * sizeof(u64);
	words = (u64 *)(stack_phys + sp_offset);
	words[0] = 1;
	words[1] = stack_vaddr + string_offset;
	words[2] = 0;
	words[3] = 0;
	return stack_vaddr + sp_offset;
}

static int load_user_elf_page(u64 start, u64 size, u64 user_vaddr,
			      u8 *backing, u64 backing_size, u64 *entry)
{
	const u8 *elf = (const u8 *)start;
	const u32 pt_load = 1;
	const u64 phoff = read_le64(elf + 32);
	const u16 phentsize = read_le16(elf + 54);
	const u16 phnum = read_le16(elf + 56);
	u32 loaded = 0;

	if (!user_elf_is_riscv64(start, size) || phentsize < 56) {
		return -1;
	}
	if (phoff + (u64)phentsize * phnum > size) {
		return -1;
	}
	memory_zero(backing, backing_size);

	for (u16 i = 0; i < phnum; i++) {
		const u8 *ph = elf + phoff + (u64)i * phentsize;
		const u32 type = read_le32(ph);
		const u64 offset = read_le64(ph + 8);
		const u64 vaddr = read_le64(ph + 16);
		const u64 filesz = read_le64(ph + 32);
		const u64 memsz = read_le64(ph + 40);
		const u64 page_offset = vaddr - user_vaddr;

		if (type != pt_load) {
			continue;
		}
		if (vaddr < user_vaddr || page_offset > backing_size ||
		    memsz < filesz || page_offset + memsz > backing_size ||
		    offset + filesz > size) {
			return -1;
		}
		memory_copy(backing + page_offset, elf + offset, filesz);
		loaded = 1;
	}

	__asm__ volatile ("fence.i" ::: "memory");
	*entry = read_le64(elf + 24);
	return loaded != 0 && *entry != 0 ? 0 : -1;
}

static u64 pte_table(u64 table)
{
	return ((table >> RISCV64_PAGE_SHIFT) << 10) | RISCV64_PTE_V;
}

static u64 pte_leaf(u64 phys, u64 flags)
{
	return ((phys >> RISCV64_PAGE_SHIFT) << 10) | flags;
}

static u64 vpn_index(u64 vaddr, u32 level)
{
	return (vaddr >> (RISCV64_PAGE_SHIFT + level * 9)) & 0x1ffULL;
}

static void early_vm_map_page(u64 vaddr, u64 phys, u64 flags)
{
	const u64 l1_index = vpn_index(vaddr, 1);
	const u64 l0_index = vpn_index(vaddr, 0);

	early_user_l1[l1_index] = pte_table((u64)early_user_l0);
	early_user_l0[l0_index] = pte_leaf(phys, flags);
}

static void early_vm_enable(void)
{
	const u64 identity_flags = RISCV64_PTE_V | RISCV64_PTE_R |
				   RISCV64_PTE_W | RISCV64_PTE_X |
				   RISCV64_PTE_A | RISCV64_PTE_D;
	const u64 user_rx = RISCV64_PTE_V | RISCV64_PTE_R |
			    RISCV64_PTE_X | RISCV64_PTE_U |
			    RISCV64_PTE_A | RISCV64_PTE_D;
	const u64 user_rw = RISCV64_PTE_V | RISCV64_PTE_R |
			    RISCV64_PTE_W | RISCV64_PTE_U |
			    RISCV64_PTE_A | RISCV64_PTE_D;
	const u64 root_ppn = ((u64)early_root_table) >> RISCV64_PAGE_SHIFT;

	memory_zero((u8 *)early_root_table, sizeof(early_root_table));
	memory_zero((u8 *)early_user_l1, sizeof(early_user_l1));
	memory_zero((u8 *)early_user_l0, sizeof(early_user_l0));

	early_root_table[vpn_index(RISCV64_PHYS_MEM_BASE, 2)] =
		pte_leaf(RISCV64_PHYS_MEM_BASE, identity_flags);
	early_root_table[0] = pte_table((u64)early_user_l1);
	early_vm_map_page(RISCV64_USER_BASE, (u64)mapped_user_text, user_rx);
	early_vm_map_page(USER_STACK_PAGE, (u64)mapped_user_stack, user_rw);

	__asm__ volatile ("sfence.vma" ::: "memory");
	__asm__ volatile ("csrw satp, %0" : : "r"(SATP_MODE_SV39 | root_ppn) :
			  "memory");
	__asm__ volatile ("sfence.vma" ::: "memory");
	__asm__ volatile ("fence.i" ::: "memory");
}

void riscv64_early_main(u64 hart_id, u64 fdt)
{
	struct riscv64_fdt_memory_range memory;
	struct riscv64_fdt_initrd initrd;
	u64 module_start = 0;
	u64 module_size = 0;
	u64 module_entry = 0;
	u64 module_stack = 0;

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

	early_puts("bunixos: riscv64 early bootstrap\n");
	arch_interrupts_init();
	riscv64_timer_set_relative(arch_timer_hz() / 100);
	arch_interrupts_enable();
	while (arch_timer_ticks() == 0) {
		__asm__ volatile ("wfi");
	}
	arch_interrupts_disable();
	early_puts("timer: riscv64 tick\n");
	if (context_switch_self_test() == 0) {
		early_puts("thread: riscv64 switch\n");
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
	if (bootpkg_magic_ok(boot_info.initrd_start, boot_info.initrd_end)) {
		early_puts("bootpkg: riscv64 initrd\n");
		if (bootpkg_find_module(boot_info.initrd_start,
					boot_info.initrd_end,
					riscv64_bootpkg_abi_smoke,
					&module_start, &module_size) == 0 &&
		    user_elf_is_riscv64(module_start, module_size)) {
			early_puts("module: riscv64 user elf\n");
			if (load_user_elf_page(module_start, module_size,
					       RISCV64_USER_BASE,
					       mapped_user_text,
					       sizeof(mapped_user_text),
					       &module_entry) == 0) {
				module_stack = build_user_stack_mapped(
					mapped_user_stack, USER_STACK_PAGE,
					"/bin/abi-smoke.user");
				early_vm_enable();
				if (riscv64_user_mode_self_test(
					    module_entry, module_stack,
					    (u64)(mapped_user_kernel_stack +
						  sizeof(mapped_user_kernel_stack))) ==
				    1) {
					early_puts("vm: riscv64 user exit\n");
				}
			}
		}
	}
	early_puts("machine: poweroff\n");
	(void)riscv64_sbi_call1(RISCV64_SBI_LEGACY_SHUTDOWN, 0);

	for (;;) {
		__asm__ volatile ("wfi");
	}
}
