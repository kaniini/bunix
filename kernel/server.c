#include "console.h"
#include "elf.h"
#include "ipc.h"
#include "multiboot2.h"
#include "name.h"
#include "pmm.h"
#include "sched.h"
#include "server.h"
#include "slab.h"
#include "tree.h"
#include "types.h"
#include "vm.h"
#include <arch/user.h>
#include "../servers/vm/vm_server.h"

static const struct server boot_servers[] = {
	{ "names", 0 },
	{ "consoled", 0 },
	{ "bootstrap", 0 },
	{ "time", 0 },
	{ "user", 0 },
	{ "linux", 0 },
	{ "proc", 0 },
	{ "procfs", 0 },
	{ "tmpfs", 0 },
	{ "devfs", 0 },
	{ "sysfs", 0 },
	{ "utmpfs", 0 },
	{ "rootfs", 0 },
	{ "unionfs", 0 },
	{ "block", 0 },
	{ "virtio-bus", 0 },
	{ "virtio-blk", 0 },
	{ "net", 0 },
	{ "vfs", 0 },
	{ "ping", 0 },
	{ "vm", vm_server_start },
};

struct module_server_start {
	struct tree_node node;
	const struct server *server;
	u64 image_start;
	u64 image_end;
	u64 data_start;
	u64 data_end;
	struct vm_space *space;
	u64 stack;
	u32 launched;
};

struct boot_data_module {
	const char *name;
	u64 start;
	u64 end;
};

static struct tree module_starts_by_name;
static struct boot_data_module disk0_module;

struct task_start {
	u64 entry;
	u64 stack;
};

struct fork_start {
	struct arch_syscall_frame frame;
};

static int str_eq(const char *left, const char *right);
static const struct server *find_boot_server(const char *name);
static struct module_server_start *module_start_find(const char *name);
static void grant_bootstrap_caps(struct task *task, const char *server_name);
static u64 align_up(u64 value, u64 align);

void server_start_all(void)
{
	const u32 count = sizeof(boot_servers) / sizeof(boot_servers[0]);

	for (u32 i = 0; i < count; i++) {
		console_printf("kernel: starting server %s\n", boot_servers[i].name);
		boot_servers[i].start();
	}
}

static void module_server_thread(void *arg)
{
	const struct module_server_start *start =
		(const struct module_server_start *)arg;

	console_printf("kernel: starting module server %s image=%p-%p\n",
		       start->server->name,
		       (const void *)start->image_start,
		       (const void *)start->image_end);

	if (start->server->start == 0) {
		u64 entry = 0;
		if (elf_load_user_image(start->space, start->image_start,
					start->image_end, &entry) == 0) {
			arch_user_enter(entry, start->stack);
		}
		return;
	}

	if (start->server->start != 0) {
		start->server->start();
	}
}

static int validate_inherited_caps(const char *name, struct task *parent,
				   const struct task_launch_cap *caps,
				   u64 cap_count)
{
	enum {
		MAX_INHERITED_HANDLES = 8,
	};

	if (cap_count > MAX_INHERITED_HANDLES) {
		console_printf("kernel: too many inherited caps for %s count=%u\n",
			       name, (u32)cap_count);
		return -1;
	}

	for (u64 cap = 0; cap < cap_count; cap++) {
		if (caps == 0 ||
		    caps[cap].reserved != 0 ||
		    task_can_inherit_handle(parent, caps[cap].handle,
					    caps[cap].rights) != 0) {
			console_printf("kernel: invalid inherited cap handle=%u rights=0x%x for %s\n",
				       caps != 0 ? (u32)caps[cap].handle : 0,
				       caps != 0 ? caps[cap].rights : 0,
				       name);
			return -1;
		}
	}

	return 0;
}

static u64 launch_user_image(const char *name, struct task *parent,
			     const struct task_launch_cap *caps, u64 cap_count,
			     struct module_server_start *start)
{
	enum {
		USER_STACK_TOP = 0x800000,
		USER_STACK_PAGES = 16,
	};

	struct vm_space *space = vm_server_bootstrap_space(name);
	if (space == 0) {
		return (u64)-1;
	}

	struct task *task = task_create(name, space);
	if (task == 0) {
		return (u64)-1;
	}

	struct ipc_port *service_port = ipc_port_create(name);
	const u64 child_self =
		task_grant_port(task, service_port,
				TASK_RIGHT_SEND | TASK_RIGHT_RECV |
				TASK_RIGHT_DUP);
	if (child_self == 0) {
		return (u64)-1;
	}

	for (u64 cap = 0; cap < cap_count; cap++) {
		if (task_grant_inherited_handle(task, parent,
						caps[cap].handle,
						caps[cap].rights) == 0) {
			console_printf("kernel: invalid inherited cap handle=%u rights=0x%x for %s\n",
				       (u32)caps[cap].handle,
				       caps[cap].rights, name);
			return (u64)-1;
		}
	}

	grant_bootstrap_caps(task, name);

	for (u64 page = 0; page < USER_STACK_PAGES; page++) {
		const u64 vaddr = USER_STACK_TOP - (page + 1) * VM_PAGE_SIZE;
		if (vm_alloc_user_page(space, vaddr, 1).addr == 0) {
			console_printf("kernel: failed to map stack for %s\n",
				       name);
			return (u64)-1;
		}
	}
	(void)task_add_vm_region(task, USER_STACK_TOP -
				 USER_STACK_PAGES * VM_PAGE_SIZE,
				 USER_STACK_PAGES * VM_PAGE_SIZE, 1,
				 TASK_VM_REGION_STACK);

	const u64 argc = 0;
	if (vm_write_user(space, USER_STACK_TOP - sizeof(argc), &argc,
			  sizeof(argc)) != 0) {
		console_printf("kernel: failed to seed stack for %s\n", name);
		return (u64)-1;
	}

	start->space = space;
	start->stack = USER_STACK_TOP - sizeof(argc);

	if (thread_create(task, name, module_server_thread, start) == 0) {
		console_printf("kernel: failed to create server thread %s\n",
			       name);
		return (u64)-1;
	}

	if (!str_eq(name, "vm")) {
		name_service_register(name, NAME_SERVICE_TASK, task_id(task));
	}

	return parent != 0 ?
	       task_grant_port(parent, service_port,
			       TASK_RIGHT_SEND | TASK_RIGHT_DUP) : 0;
}

static const struct server *find_boot_server(const char *name)
{
	const u32 count = sizeof(boot_servers) / sizeof(boot_servers[0]);

	for (u32 i = 0; i < count; i++) {
		if (str_eq(name, boot_servers[i].name)) {
			return &boot_servers[i];
		}
	}

	return 0;
}

static struct module_server_start *module_start_find(const char *name)
{
	return (struct module_server_start *)
		tree_get(&module_starts_by_name, name);
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

static void mem_copy(u8 *dst, const u8 *src, u64 len)
{
	for (u64 i = 0; i < len; i++) {
		dst[i] = src[i];
	}
}

static void mem_zero(u8 *dst, u64 len)
{
	for (u64 i = 0; i < len; i++) {
		dst[i] = 0;
	}
}

static u64 copy_boot_module(const struct multiboot2_module *module,
			    const char *kind)
{
	const u64 size = module->end - module->start;
	const u64 pages = align_up(size, PMM_PAGE_SIZE) / PMM_PAGE_SIZE;
	const u64 copy = pmm_pages_alloc_contiguous(pages);

	if (copy == 0) {
		console_printf("kernel: failed to copy %s module %s size=%u\n",
			       kind, module->cmdline, (u32)size);
		return 0;
	}

	mem_copy((u8 *)copy, (const u8 *)module->start, size);
	console_printf("kernel: copied %s module %s image=%p-%p size=%u\n",
		       kind, module->cmdline, (const void *)copy,
		       (const void *)(copy + size), (u32)size);
	return copy;
}

static u64 align_down(u64 value, u64 align)
{
	return value & ~(align - 1);
}

static u64 align_up(u64 value, u64 align)
{
	return (value + align - 1) & ~(align - 1);
}

static u64 min_u64(u64 left, u64 right)
{
	return left < right ? left : right;
}

static u64 max_u64(u64 left, u64 right)
{
	return left > right ? left : right;
}

static void task_entry_thread(void *arg)
{
	struct task_start *start = (struct task_start *)arg;
	const u64 entry = start->entry;
	const u64 stack = start->stack;

	slab_free(start);
	arch_user_enter(entry, stack);
}

static void fork_entry_thread(void *arg)
{
	struct fork_start *start = (struct fork_start *)arg;
	const struct arch_syscall_frame frame = start->frame;

	slab_free(start);
	arch_user_resume(&frame);
}

int server_launch_module(const char *name)
{
	return server_launch_module_with_caps(name, 0, 0, 0) == (u64)-1 ? -1 : 0;
}

static void grant_bootstrap_caps(struct task *task, const char *server_name)
{
	static const struct task_hw_resource com1_port = {
		.type = TASK_HW_RESOURCE_PORT,
		.ops = TASK_HW_OP_READ | TASK_HW_OP_WRITE,
		.base = 0x3f8,
		.len = 8,
	};
	static const struct task_hw_resource pci_config_ports = {
		.type = TASK_HW_RESOURCE_PORT,
		.ops = TASK_HW_OP_READ | TASK_HW_OP_WRITE,
		.base = 0xcf8,
		.len = 8,
	};

	if (str_eq(server_name, "names")) {
		task_grant_port(task, ipc_port_find("console"),
				TASK_RIGHT_SEND | TASK_RIGHT_DUP);
		return;
	}

	if (str_eq(server_name, "consoled")) {
		task_grant_port(task, ipc_port_find("console"),
				TASK_RIGHT_SEND | TASK_RIGHT_RECV |
				TASK_RIGHT_DUP);
		task_grant_port(task, ipc_port_find("names"),
				TASK_RIGHT_SEND | TASK_RIGHT_DUP);
		task_grant_hw_resource(task, &com1_port, TASK_RIGHT_SEND);
		return;
	}

	if (str_eq(server_name, "virtio-bus")) {
		task_grant_hw_resource(task, &pci_config_ports, TASK_RIGHT_SEND);
		return;
	}

	if (!str_eq(server_name, "bootstrap")) {
		return;
	}

	task_grant_port(task, ipc_port_find("console"),
			TASK_RIGHT_SEND | TASK_RIGHT_DUP);
	task_grant_port(task, ipc_port_find("vm"),
			TASK_RIGHT_SEND | TASK_RIGHT_DUP);
	task_grant_port(task, ipc_port_find("names"),
			TASK_RIGHT_SEND | TASK_RIGHT_DUP);
}

u64 server_launch_module_with_caps(const char *name, struct task *parent,
				   const struct task_launch_cap *caps,
				   u64 cap_count)
{
	struct module_server_start *start = module_start_find(name);

	if (start != 0) {
		const char *server_name = start->server->name;

		if (start->launched) {
			console_printf("kernel: module server %s already launched\n",
				       server_name);
			return -1;
		}
		if (validate_inherited_caps(name, parent, caps, cap_count) != 0) {
			return (u64)-1;
		}
		console_printf("kernel: launching module server %s\n",
			       server_name);
		start->launched = 1;
		return launch_user_image(server_name, parent, caps, cap_count,
					 start);
	}

	console_printf("kernel: no module server named %s\n", name);
	return (u64)-1;
}

u64 server_task_create(struct task *parent, const char *name)
{
	enum {
		USER_STACK_TOP = 0x800000,
		USER_STACK_PAGES = 16,
	};

	if (parent == 0 || name == 0) {
		return (u64)-1;
	}

	struct vm_space *space = vm_server_bootstrap_space(name);
	if (space == 0) {
		return (u64)-1;
	}

	struct task *task = task_create(name, space);
	if (task == 0) {
		return (u64)-1;
	}
	const char *task_label = task_name(task);

	struct ipc_port *service_port = ipc_port_create_private(task_label);
	if (task_grant_port(task, service_port,
			    TASK_RIGHT_SEND | TASK_RIGHT_RECV |
			    TASK_RIGHT_DUP) == 0) {
		return (u64)-1;
	}

	for (u64 page = 0; page < USER_STACK_PAGES; page++) {
		const u64 vaddr = USER_STACK_TOP - (page + 1) * VM_PAGE_SIZE;
		if (vm_alloc_user_page(space, vaddr, 1).addr == 0) {
			console_printf("kernel: failed to map stack for %s\n",
				       task_label);
			return (u64)-1;
		}
	}
	(void)task_add_vm_region(task, USER_STACK_TOP -
				 USER_STACK_PAGES * VM_PAGE_SIZE,
				 USER_STACK_PAGES * VM_PAGE_SIZE, 1,
				 TASK_VM_REGION_STACK);

	const u64 argc = 0;
	if (vm_write_user(space, USER_STACK_TOP - sizeof(argc), &argc,
			  sizeof(argc)) != 0) {
		console_printf("kernel: failed to seed stack for %s\n",
			       task_label);
		return (u64)-1;
	}

	name_service_register(task_label, NAME_SERVICE_TASK, task_id(task));
	return task_grant_task(parent, task, TASK_RIGHT_SEND | TASK_RIGHT_DUP);
}

int server_task_map(struct task *parent, u64 task_handle, u64 vaddr,
		    const void *src, u64 filesz, u64 memsz, u32 writable)
{
	struct task *task = task_from_handle(parent, task_handle,
					    TASK_RIGHT_SEND);
	if (task == 0 || src == 0 || filesz > memsz ||
	    vaddr + memsz < vaddr) {
		return -1;
	}

	struct vm_space *space = task_vm_space(task);
	const u64 page_start = align_down(vaddr, VM_PAGE_SIZE);
	const u64 page_end = align_up(vaddr + memsz, VM_PAGE_SIZE);

	for (u64 page = page_start; page < page_end; page += VM_PAGE_SIZE) {
		struct vm_frame frame = vm_rpc_alloc_frame();
		const u64 page_end_addr = page + VM_PAGE_SIZE;
		const u64 copy_start = max_u64(page, vaddr);
		const u64 copy_end = min_u64(page_end_addr, vaddr + filesz);

		if (frame.addr == 0) {
			return -1;
		}

		mem_zero((u8 *)frame.addr, VM_PAGE_SIZE);
		if (copy_start < copy_end) {
			const u64 dst_offset = copy_start - page;
			const u64 src_offset = copy_start - vaddr;

			mem_copy((u8 *)(frame.addr + dst_offset),
				 (const u8 *)src + src_offset,
				 copy_end - copy_start);
		}
		if (vm_map_user_page(space, page, frame, writable) != 0) {
			vm_rpc_free_frame(frame);
			return -1;
		}
	}

	console_printf("kernel: task map task=%u vaddr=%p filesz=%u memsz=%u\n",
		       task_id(task), (const void *)vaddr, (u32)filesz,
		       (u32)memsz);
	if (task_add_vm_region(task, page_start, page_end - page_start,
			       writable, TASK_VM_REGION_ELF) != 0) {
		return -1;
	}
	return 0;
}

int server_task_write(struct task *parent, u64 task_handle, u64 vaddr,
		      const void *src, u64 len)
{
	struct task *task = task_from_handle(parent, task_handle,
					    TASK_RIGHT_SEND);
	if (task == 0 || src == 0 || vaddr + len < vaddr) {
		return -1;
	}

	if (vm_write_user(task_vm_space(task), vaddr, src, len) != 0) {
		return -1;
	}

	console_printf("kernel: task write task=%u vaddr=%p len=%u\n",
		       task_id(task), (const void *)vaddr, (u32)len);
	return 0;
}

int server_task_alloc(struct task *parent, u64 task_handle, u64 vaddr,
		      u64 len, u32 writable)
{
	return server_task_alloc_kind(parent, task_handle, vaddr, len, writable,
				      TASK_VM_REGION_MMAP);
}

int server_task_alloc_kind(struct task *parent, u64 task_handle, u64 vaddr,
			   u64 len, u32 writable, u32 kind)
{
	struct task *task = task_handle == 0 ? parent :
			    task_from_handle(parent, task_handle,
					     TASK_RIGHT_SEND);
	if (task == 0 || len == 0 || vaddr + len < vaddr) {
		return -1;
	}

	if (vm_alloc_user_range(task_vm_space(task), vaddr, len, writable) != 0) {
		return -1;
	}

	console_printf("kernel: task alloc task=%u vaddr=%p len=%u writable=%u\n",
		       task_id(task), (const void *)vaddr, (u32)len, writable);
	if (task_add_vm_region(task, align_down(vaddr, VM_PAGE_SIZE),
			       align_up(vaddr + len, VM_PAGE_SIZE) -
			       align_down(vaddr, VM_PAGE_SIZE),
			       writable, kind) != 0) {
		return -1;
	}
	return 0;
}

int server_task_clone_range(struct task *parent, u64 dst_handle,
			    u64 src_handle, u64 vaddr, u64 len, u32 writable)
{
	struct task *dst = task_from_handle(parent, dst_handle, TASK_RIGHT_SEND);
	struct task *src = task_from_handle(parent, src_handle, TASK_RIGHT_SEND);

	if (dst == 0 || src == 0 || len == 0 || vaddr + len < vaddr) {
		return -1;
	}

	if (vm_clone_user_range(task_vm_space(dst), task_vm_space(src),
				vaddr, len, writable) != 0) {
		return -1;
	}

	console_printf("kernel: task clone dst=%u src=%u vaddr=%p len=%u writable=%u\n",
		       task_id(dst), task_id(src), (const void *)vaddr,
		       (u32)len, writable);
	return 0;
}

int server_task_grant(struct task *parent, u64 task_handle, u64 handle,
		      u32 rights)
{
	struct task *task = task_from_handle(parent, task_handle,
					    TASK_RIGHT_SEND);

	if (task == 0) {
		return -1;
	}

	return task_grant_inherited_handle(task, parent, handle, rights) == 0 ?
	       -1 : 0;
}

int server_task_start(struct task *parent, u64 task_handle, u64 entry)
{
	enum {
		USER_STACK_TOP = 0x800000,
	};

	struct task *task = task_from_handle(parent, task_handle,
					    TASK_RIGHT_SEND);
	if (task == 0) {
		return -1;
	}

	struct task_start *start = slab_alloc(sizeof(*start));
	if (start == 0) {
		return -1;
	}
	start->entry = entry;
	start->stack = USER_STACK_TOP - sizeof(u64);

	console_printf("kernel: task start task=%u entry=%p\n", task_id(task),
		       (const void *)entry);
	if (thread_create(task, task_name(task), task_entry_thread, start) == 0) {
		slab_free(start);
		return -1;
	}
	return 0;
}

int server_task_start_at(struct task *parent, u64 task_handle, u64 entry,
			 u64 stack)
{
	struct task *task = task_from_handle(parent, task_handle,
					    TASK_RIGHT_SEND);
	if (task == 0 || stack == 0) {
		return -1;
	}

	struct task_start *start = slab_alloc(sizeof(*start));
	if (start == 0) {
		return -1;
	}
	start->entry = entry;
	start->stack = stack;

	console_printf("kernel: task start task=%u entry=%p stack=%p\n",
		       task_id(task), (const void *)entry,
		       (const void *)stack);
	if (thread_create(task, task_name(task), task_entry_thread, start) == 0) {
		slab_free(start);
		return -1;
	}
	return 0;
}

int server_task_kill(struct task *parent, u64 task_handle)
{
	struct task *task = task_from_handle(parent, task_handle,
					    TASK_RIGHT_SEND);
	if (task == 0) {
		return -1;
	}

	return task_kill(task);
}

int server_task_clear(struct task *parent, u64 task_handle)
{
	struct task *task = task_from_handle(parent, task_handle,
					    TASK_RIGHT_SEND);

	if (task == 0) {
		return -1;
	}

	while (task_vm_region_count(task) != 0) {
		const struct task_vm_region *region = task_vm_region_at(task, 0);

		if (region == 0) {
			return -1;
		}

		const u64 base = region->base;
		const u64 len = region->len;

		if (vm_unmap_user_range(task_vm_space(task), base, len) != 0) {
			return -1;
		}
		if (task_remove_vm_region(task, base, len) != 0) {
			return -1;
		}
	}

	task_clear_vm_regions(task);
	return 0;
}

struct task *server_task_fork_current_stopped(
	const struct arch_syscall_frame *frame)
{
	struct task *parent = task_current();
	if (parent == 0 || frame == 0) {
		return 0;
	}

	struct vm_space *space = vm_server_bootstrap_space(task_name(parent));
	if (space == 0) {
		return 0;
	}

	struct task *child = task_create(task_name(parent), space);
	if (child == 0) {
		return 0;
	}
	if (task_clone_handles(child, parent) != 0) {
		(void)task_kill(child);
		return 0;
	}

	const u64 region_count = task_vm_region_count(parent);
	for (u64 i = 0; i < region_count; i++) {
		const struct task_vm_region *region =
			task_vm_region_at(parent, i);

		if (region == 0 ||
		    vm_clone_user_range(task_vm_space(child),
					task_vm_space(parent),
					region->base, region->len,
					region->writable) != 0 ||
		    task_add_vm_mapping(child, region->base, region->len,
					region->prot, region->flags,
					region->kind, region->object_type,
					region->object_id,
					region->offset) != 0) {
			(void)task_kill(child);
			return 0;
		}
	}

	const u64 brk = task_linux_brk(parent);
	const u64 mmap_next = task_linux_mmap_next(parent);
	const u64 fs_base = task_linux_fs_base(parent);
	task_set_linux_brk(child, brk);
	task_set_linux_mmap_next(child, mmap_next);
	task_set_linux_fs_base(child, fs_base);

	return child;
}

int server_task_start_fork(struct task *child,
			   const struct arch_syscall_frame *frame)
{
	struct task *parent = task_current();
	if (parent == 0 || child == 0 || frame == 0) {
		return -1;
	}

	struct fork_start *start = slab_alloc(sizeof(*start));
	if (start == 0) {
		return -1;
	}
	start->frame = *frame;

	console_printf("kernel: task fork parent=%u child=%u rip=%p rsp=%p\n",
		       task_id(parent), task_id(child),
		       (const void *)frame->user_rip,
		       (const void *)frame->user_rsp);
	if (thread_create(child, task_name(child), fork_entry_thread, start) == 0) {
		slab_free(start);
		return -1;
	}
	return 0;
}

struct task *server_task_fork_current(const struct arch_syscall_frame *frame)
{
	struct task *child = server_task_fork_current_stopped(frame);
	if (child == 0) {
		return 0;
	}

	if (server_task_start_fork(child, frame) != 0) {
		(void)task_kill(child);
		return 0;
	}

	return child;
}

static struct module_server_start *current_module_start(void)
{
	const char *name = task_name(task_current());

	return module_start_find(name);
}

u64 server_boot_module_size(void)
{
	const struct module_server_start *start = current_module_start();

	return start != 0 && start->data_end > start->data_start ?
	       start->data_end - start->data_start : 0;
}

int server_boot_module_read(u64 offset, void *buffer, u64 len)
{
	const struct module_server_start *start = current_module_start();
	const u64 size = server_boot_module_size();

	if (start == 0 || start->data_start == 0 || buffer == 0 ||
	    offset > size || len > size - offset) {
		return -1;
	}

	mem_copy((u8 *)buffer, (const u8 *)(start->data_start + offset), len);
	return 0;
}

static void record_boot_module(const struct multiboot2_module *module, void *ctx)
{
	(void)ctx;

	const struct server *server = find_boot_server(module->cmdline);
	if (server == 0) {
		if (str_eq(module->cmdline, "disk0")) {
			const u64 copy = copy_boot_module(module, "data");
			const u64 size = module->end - module->start;

			disk0_module.name = module->cmdline;
			disk0_module.start = copy != 0 ? copy : module->start;
			disk0_module.end = disk0_module.start + size;
			console_printf("kernel: recorded data module disk0 image=%p-%p size=%u\n",
				       (const void *)disk0_module.start,
				       (const void *)disk0_module.end,
				       (u32)size);
			struct module_server_start *block =
				module_start_find("block");
			if (block != 0) {
				block->data_start = disk0_module.start;
				block->data_end = disk0_module.end;
			}
			return;
		}

		console_printf("kernel: ignoring unknown module %s\n", module->cmdline);
		return;
	}

	const u64 size = module->end - module->start;
	const u64 copy = copy_boot_module(module, "server");
	struct module_server_start *start =
		(struct module_server_start *)slab_zalloc(sizeof(*start));
	if (start == 0) {
		console_printf("kernel: module server alloc failed\n");
		return;
	}
	start->server = server;
	start->image_start = copy != 0 ? copy : module->start;
	start->image_end = start->image_start + size;
	start->data_start = 0;
	start->data_end = 0;
	if (str_eq(server->name, "block")) {
		start->data_start = disk0_module.start;
		start->data_end = disk0_module.end;
	}
	start->space = 0;
	start->stack = 0;
	start->launched = 0;
	if (tree_insert_node(&module_starts_by_name, &start->node,
			     server->name, (u64)start) != 0) {
		console_printf("kernel: duplicate module server %s\n",
			       server->name);
		slab_free(start);
		return;
	}

	console_printf("kernel: recorded module server %s image=%p-%p\n",
		       server->name, (const void *)start->image_start,
		       (const void *)start->image_end);
}

void server_start_boot_modules(u64 multiboot_info)
{
	struct ipc_port *console_port = ipc_port_create("console");

	tree_init(&module_starts_by_name);
	name_service_register("console", NAME_SERVICE_IPC_PORT,
			      (u64)console_port);
	multiboot2_for_each_module(multiboot_info, record_boot_module, 0);
	server_launch_module("vm");
	sched_run();
	server_launch_module("names");
	server_launch_module("consoled");
	sched_run();
	server_launch_module("bootstrap");
}
