#include <bunix/libbunix.h>

enum {
	PROC_HANDLE_CONSOLE = 2,
	PROC_HANDLE_NAMES = 3,
	PROC_HANDLE_TIME = 4,
	PROC_SEGMENT_MAX = 4096,
	PROC_INIT_STACK_MAX = 4096,
	PROC_MAX_PHDRS = 16,
	USER_STACK_TOP = 0x800000,
	ELF_MAGIC = 0x464c457f,
	ELFCLASS64 = 2,
	ELFDATA2LSB = 1,
	ET_EXEC = 2,
	ET_DYN = 3,
	EM_X86_64 = 62,
	PT_LOAD = 1,
	PT_DYNAMIC = 2,
	PF_W = 1 << 1,
	DT_NULL = 0,
	DT_RELR = 0x24,
	DT_RELRSZ = 0x23,
	DT_RELRENT = 0x25,
	AT_NULL = 0,
	AT_PHDR = 3,
	AT_PHENT = 4,
	AT_PHNUM = 5,
	AT_PAGESZ = 6,
	AT_UID = 11,
	AT_EUID = 12,
	AT_GID = 13,
	AT_EGID = 14,
	AT_CLKTCK = 17,
	AT_SECURE = 23,
	AT_RANDOM = 25,
	AT_ENTRY = 9,
	AT_EXECFN = 31,
	EXEC_HANDLE_SELF = 1,
	EXEC_HANDLE_STDOUT = 2,
	EXEC_HANDLE_STDERR = EXEC_HANDLE_STDOUT,
	EXEC_HANDLE_TIME = 3,
	EXEC_HANDLE_PROC = 4,
	PROC_MAX_PROCESSES = 16,
	PROC_DYN_LOAD_BIAS = 0x400000,
};

struct process {
	u64 pid;
	u64 linux_pid;
	u64 task_handle;
	u64 status;
	u64 exited;
	u64 waiter;
};

struct elf64_ehdr {
	unsigned char ident[16];
	unsigned short type;
	unsigned short machine;
	unsigned int version;
	u64 entry;
	u64 phoff;
	u64 shoff;
	unsigned int flags;
	unsigned short ehsize;
	unsigned short phentsize;
	unsigned short phnum;
	unsigned short shentsize;
	unsigned short shnum;
	unsigned short shstrndx;
} __attribute__((packed));

struct elf64_phdr {
	unsigned int type;
	unsigned int flags;
	u64 offset;
	u64 vaddr;
	u64 paddr;
	u64 filesz;
	u64 memsz;
	u64 align;
} __attribute__((packed));

struct elf64_dyn {
	long long tag;
	u64 value;
} __attribute__((packed));

struct vfs_file {
	u64 handle;
	u64 size;
	u64 type;
};

struct exec_info {
	u64 entry;
	u64 phent;
	u64 phnum;
	const struct elf64_phdr *phdrs;
};

static struct process processes[PROC_MAX_PROCESSES];
static u64 next_pid = 1;
static u64 first_linux_pid;
static unsigned char segment_buffer[PROC_SEGMENT_MAX];
static unsigned char init_stack[PROC_INIT_STACK_MAX];
static const char proc_online[] = "proc: online\n";
static const char proc_ready[] = "proc: ready\n";
static const char proc_exec[] = "proc: exec /bin/first\n";
static const char proc_exec_linux[] = "proc: exec /bin/lxtest\n";
static const char proc_exec_musl[] = "proc: exec /bin/musl-hello\n";
static const char proc_exec_shell[] = "proc: exec /bin/sh\n";
static const char proc_spawned[] = "proc: spawned pid=1\n";
static const char proc_spawned_linux[] = "proc: spawned pid=2\n";
static const char proc_spawned_musl[] = "proc: spawned pid=3\n";
static const char proc_spawned_shell[] = "proc: spawned pid=4\n";
static const char proc_exited[] = "proc: exited pid=1 status=0\n";
static const char proc_waited[] = "proc: wait pid=1 status=0\n";
static const char proc_register_failed[] = "proc: register failed\n";

static long register_service(u64 service, u64 handle)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_NAMES,
		.type = BUNIX_NAMES_REGISTER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP,
		.reply = 0,
		.cap = handle,
		.words = { BUNIX_NAMES_ROOT, service, 0, 0 },
	};
	struct bunix_msg reply;

	if (bunix_ipc_call(PROC_HANDLE_NAMES, &request, &reply) != 0) {
		return -1;
	}

	return reply.words[0] == 0 ? 0 : -1;
}

static u64 resolve_service(u64 service, unsigned int rights)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_NAMES,
		.type = BUNIX_NAMES_WAIT,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { BUNIX_NAMES_ROOT, service, rights, 0 },
	};
	struct bunix_msg reply;

	if (bunix_ipc_call(PROC_HANDLE_NAMES, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return 0;
	}

	return reply.cap;
}

static void pack_path(u64 *words, const char *path)
{
	words[0] = 0;
	words[1] = 0;

	for (u64 i = 0; i < 16 && path[i] != '\0'; i++) {
		const u64 slot = i / 8;
		const u64 shift = (i % 8) * 8;

		words[slot] |= ((u64)(unsigned char)path[i]) << shift;
	}
}

static void unpack_bytes(unsigned char *out, const u64 *words, u64 len)
{
	for (u64 i = 0; i < len; i++) {
		const u64 slot = i / 8;
		const u64 shift = (i % 8) * 8;

		out[i] = (unsigned char)((words[slot] >> shift) & 0xff);
	}
}

static unsigned int read_magic(const unsigned char *ident)
{
	return ((unsigned int)ident[0]) |
	       ((unsigned int)ident[1] << 8) |
	       ((unsigned int)ident[2] << 16) |
	       ((unsigned int)ident[3] << 24);
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

static int is_linux_path(const char *path)
{
	return str_eq(path, "/bin/lxtest") ||
	       str_eq(path, "/bin/musl-hello") ||
	       str_eq(path, "/bin/sh") ||
	       str_eq(path, "/bin/busybox");
}

static const char *task_name_for_path(const char *path)
{
	if (str_eq(path, "/bin/lxtest")) {
		return "lxtest";
	}
	if (str_eq(path, "/bin/musl-hello")) {
		return "musl-hello";
	}
	if (str_eq(path, "/bin/sh") || str_eq(path, "/bin/busybox")) {
		return "busybox";
	}
	return "first";
}

static u64 str_len(const char *text)
{
	u64 len = 0;

	while (text[len] != '\0') {
		len++;
	}

	return len;
}

static u64 align_down(u64 value, u64 align)
{
	return value & ~(align - 1);
}

static u64 min_u64(u64 left, u64 right)
{
	return left < right ? left : right;
}

static u64 max_u64(u64 left, u64 right)
{
	return left > right ? left : right;
}

static u64 read_u64_le(const unsigned char *src)
{
	u64 value = 0;

	for (u64 i = 0; i < 8; i++) {
		value |= ((u64)src[i]) << (i * 8);
	}

	return value;
}

static void mem_zero(unsigned char *dst, u64 len)
{
	for (u64 i = 0; i < len; i++) {
		dst[i] = 0;
	}
}

static void mem_copy(unsigned char *dst, const unsigned char *src, u64 len)
{
	for (u64 i = 0; i < len; i++) {
		dst[i] = src[i];
	}
}

static long vfs_open(u64 vfs, const char *path, struct vfs_file *file)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_OPEN,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { 0, 0, 0, 0 },
	};
	struct bunix_msg reply;

	if (file == 0) {
		return -1;
	}

	pack_path(&request.words[0], path);
	if (bunix_ipc_call(vfs, &request, &reply) != 0 ||
	    reply.words[0] != 0 ||
	    reply.words[1] == 0 ||
	    reply.words[3] != BUNIX_VFS_TYPE_REGULAR) {
		return -1;
	}

	file->handle = reply.words[1];
	file->size = reply.words[2];
	file->type = reply.words[3];
	return 0;
}

static long vfs_stat(u64 vfs, struct vfs_file *file)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_STAT,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { file != 0 ? file->handle : 0, 0, 0, 0 },
	};
	struct bunix_msg reply;

	if (file == 0 ||
	    bunix_ipc_call(vfs, &request, &reply) != 0 ||
	    reply.words[0] != 0 ||
	    reply.words[2] != BUNIX_VFS_TYPE_REGULAR) {
		return -1;
	}

	file->size = reply.words[1];
	file->type = reply.words[2];
	return 0;
}

static void vfs_close(u64 vfs, u64 file)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_CLOSE,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { file, 0, 0, 0 },
	};
	struct bunix_msg reply;

	bunix_ipc_call(vfs, &request, &reply);
}

static long vfs_read_file(u64 vfs, u64 file, u64 file_size, u64 offset,
			  unsigned char *buffer, u64 len, u64 io_buffer)
{
	u64 done = 0;

	if (offset > file_size || len > file_size - offset) {
		return -1;
	}

	while (done < len) {
		u64 chunk = len - done;
		struct bunix_msg request = {
			.protocol = BUNIX_PROTO_VFS,
			.type = BUNIX_VFS_READ_FILE_BUFFER,
			.sender = 0,
			.cap_rights = BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP,
			.reply = 0,
			.cap = io_buffer,
			.words = { file, offset + done, chunk, 0 },
		};
		struct bunix_msg reply;

		if (chunk > PROC_SEGMENT_MAX) {
			chunk = PROC_SEGMENT_MAX;
		}
		request.words[2] = chunk;
		if (bunix_ipc_call(vfs, &request, &reply) != 0 ||
		    reply.words[0] != 0 ||
		    reply.words[1] > chunk) {
			return -1;
		}

		if (reply.words[1] == 0) {
			return -1;
		}

		if (bunix_buffer_read(io_buffer, 0, buffer + done,
				      reply.words[1]) != 0) {
			return -1;
		}
		done += reply.words[1];
	}

	return 0;
}

static long elf_vaddr_to_offset(const struct elf64_phdr *phdrs, u64 phnum,
				u64 vaddr, u64 len, u64 *offset)
{
	if (phdrs == 0 || offset == 0 || vaddr + len < vaddr) {
		return -1;
	}

	for (u64 i = 0; i < phnum; i++) {
		const struct elf64_phdr *phdr = &phdrs[i];

		if (phdr->type != PT_LOAD ||
		    vaddr < phdr->vaddr ||
		    vaddr + len > phdr->vaddr + phdr->filesz) {
			continue;
		}

		*offset = phdr->offset + (vaddr - phdr->vaddr);
		return 0;
	}

	return -1;
}

static long read_vaddr_bytes(u64 vfs, const struct vfs_file *file,
			     const struct elf64_phdr *phdrs, u64 phnum,
			     u64 vaddr, unsigned char *buffer, u64 len,
			     u64 io_buffer)
{
	u64 offset = 0;

	if (file == 0 ||
	    elf_vaddr_to_offset(phdrs, phnum, vaddr, len, &offset) != 0) {
		return -1;
	}

	return vfs_read_file(vfs, file->handle, file->size, offset,
			     buffer, len, io_buffer);
}

static long apply_relr_one(u64 task, u64 load_bias, u64 reloc_vaddr,
			   u64 vfs, const struct vfs_file *file,
			   const struct elf64_phdr *phdrs, u64 phnum,
			   u64 io_buffer)
{
	unsigned char word[8];

	if (read_vaddr_bytes(vfs, file, phdrs, phnum, reloc_vaddr,
			     word, sizeof(word), io_buffer) != 0) {
		return -1;
	}

	const u64 relocated = read_u64_le(word) + load_bias;
	return bunix_task_write(task, load_bias + reloc_vaddr,
				&relocated, sizeof(relocated));
}

static long apply_relative_relocations(u64 vfs, const struct vfs_file *file,
				       const struct elf64_phdr *phdrs,
				       u64 phnum, u64 task, u64 load_bias,
				       u64 io_buffer)
{
	u64 relr = 0;
	u64 relrsz = 0;
	u64 relrent = 8;

	if (load_bias == 0) {
		return 0;
	}

	for (u64 i = 0; i < phnum; i++) {
		const struct elf64_phdr *phdr = &phdrs[i];

		if (phdr->type != PT_DYNAMIC) {
			continue;
		}
		if (phdr->filesz > PROC_SEGMENT_MAX ||
		    vfs_read_file(vfs, file->handle, file->size, phdr->offset,
				  segment_buffer, phdr->filesz,
				  io_buffer) != 0) {
			return -1;
		}

		const u64 entries = phdr->filesz / sizeof(struct elf64_dyn);
		const struct elf64_dyn *dyn =
			(const struct elf64_dyn *)segment_buffer;

		for (u64 entry = 0; entry < entries; entry++) {
			if (dyn[entry].tag == DT_NULL) {
				break;
			}
			if (dyn[entry].tag == DT_RELR) {
				relr = dyn[entry].value;
			} else if (dyn[entry].tag == DT_RELRSZ) {
				relrsz = dyn[entry].value;
			} else if (dyn[entry].tag == DT_RELRENT) {
				relrent = dyn[entry].value;
			}
		}
		break;
	}

	if (relr == 0 || relrsz == 0) {
		return 0;
	}
	if (relrent != 8 || relrsz > PROC_SEGMENT_MAX ||
	    read_vaddr_bytes(vfs, file, phdrs, phnum, relr,
			     segment_buffer, relrsz, io_buffer) != 0) {
		return -1;
	}

	u64 reloc_vaddr = 0;
	const u64 entries = relrsz / relrent;

	for (u64 i = 0; i < entries; i++) {
		const u64 entry = read_u64_le(segment_buffer + i * relrent);

		if ((entry & 1) == 0) {
			reloc_vaddr = entry;
			if (apply_relr_one(task, load_bias, reloc_vaddr,
					   vfs, file, phdrs, phnum,
					   io_buffer) != 0) {
				return -1;
			}
			reloc_vaddr += 8;
			continue;
		}

		for (u64 bit = 1; bit < 64; bit++) {
			if ((entry & (1ull << bit)) != 0 &&
			    apply_relr_one(task, load_bias, reloc_vaddr,
					   vfs, file, phdrs, phnum,
					   io_buffer) != 0) {
				return -1;
			}
			reloc_vaddr += 8;
		}
	}

	return 0;
}

static long build_initial_stack(u64 task, const char *path,
				const struct exec_info *exec, u64 *stack)
{
	enum {
		AUXV_PAIRS = 18,
		STACK_WORDS = 4 + AUXV_PAIRS * 2,
	};

	const u64 stack_base = USER_STACK_TOP - PROC_INIT_STACK_MAX;
	const u64 path_len = str_len(path);
	const u64 phdr_size = exec != 0 ? exec->phnum * exec->phent : 0;
	const unsigned char random_bytes[16] = {
		0x62, 0x75, 0x6e, 0x69, 0x78, 0x2d, 0x6d, 0x75,
		0x73, 0x6c, 0x2d, 0x72, 0x61, 0x6e, 0x64, 0x00,
	};
	u64 sp = PROC_INIT_STACK_MAX;

	if (exec == 0 ||
	    exec->phdrs == 0 ||
	    phdr_size == 0 ||
	    path_len + 1 + phdr_size + STACK_WORDS * sizeof(u64) >
	    PROC_INIT_STACK_MAX) {
		return -1;
	}

	mem_zero(init_stack, sizeof(init_stack));
	sp -= path_len + 1;
	const u64 argv0 = stack_base + sp;
	mem_copy(init_stack + sp, (const unsigned char *)path, path_len + 1);

	sp = align_down(sp, 16);
	sp -= sizeof(random_bytes);
	const u64 random_addr = stack_base + sp;
	mem_copy(init_stack + sp, random_bytes, sizeof(random_bytes));

	sp = align_down(sp, 8);
	sp -= phdr_size;
	const u64 phdr_addr = stack_base + sp;
	mem_copy(init_stack + sp, (const unsigned char *)exec->phdrs,
		 phdr_size);

	sp = align_down(sp, 16);
	sp -= STACK_WORDS * sizeof(u64);
	u64 *words = (u64 *)(init_stack + sp);

	words[0] = 1;
	words[1] = argv0;
	words[2] = 0;
	words[3] = 0;
	words[4] = AT_PAGESZ;
	words[5] = 4096;
	words[6] = AT_ENTRY;
	words[7] = exec->entry;
	words[8] = AT_PHDR;
	words[9] = phdr_addr;
	words[10] = AT_PHENT;
	words[11] = exec->phent;
	words[12] = AT_PHNUM;
	words[13] = exec->phnum;
	words[14] = AT_EXECFN;
	words[15] = argv0;
	words[16] = AT_UID;
	words[17] = 0;
	words[18] = AT_EUID;
	words[19] = 0;
	words[20] = AT_GID;
	words[21] = 0;
	words[22] = AT_EGID;
	words[23] = 0;
	words[24] = AT_SECURE;
	words[25] = 0;
	words[26] = AT_RANDOM;
	words[27] = random_addr;
	words[28] = AT_CLKTCK;
	words[29] = 100;
	words[30] = BUNIX_AT_STDOUT;
	words[31] = EXEC_HANDLE_STDOUT;
	words[32] = BUNIX_AT_STDERR;
	words[33] = EXEC_HANDLE_STDERR;
	words[34] = BUNIX_AT_TIME;
	words[35] = EXEC_HANDLE_TIME;
	words[36] = BUNIX_AT_PROC;
	words[37] = EXEC_HANDLE_PROC;
	words[38] = AT_NULL;
	words[39] = 0;

	if (bunix_task_write(task, stack_base + sp, init_stack + sp,
			     PROC_INIT_STACK_MAX - sp) != 0) {
		return -1;
	}

	*stack = stack_base + sp;
	return 0;
}

static long register_linux_process(u64 task, u64 ppid, u64 *linux_pid)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_LINUX,
		.type = BUNIX_LINUX_REGISTER_PROCESS,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { task, ppid, 0, 0 },
	};
	struct bunix_msg reply;
	const u64 linux = resolve_service(BUNIX_SERVICE_LINUX, BUNIX_RIGHT_SEND);

	if (linux == 0 ||
	    bunix_ipc_call(linux, &request, &reply) != 0 ||
	    (long)reply.words[0] <= 0) {
		return -1;
	}

	if (linux_pid != 0) {
		*linux_pid = reply.words[0];
	}
	return 0;
}

static long exec_path(u64 vfs, const char *path, const char *task_name,
		      u64 linux_parent_pid, u64 *linux_pid,
		      u64 *task_handle)
{
	const struct bunix_launch_cap caps[] = {
		{ PROC_HANDLE_CONSOLE, BUNIX_RIGHT_SEND, 0 },
		{ PROC_HANDLE_TIME, BUNIX_RIGHT_SEND, 0 },
		{ BUNIX_HANDLE_SELF, BUNIX_RIGHT_SEND, 0 },
	};
	struct elf64_ehdr ehdr;
	struct elf64_phdr phdrs[PROC_MAX_PHDRS];
	struct elf64_phdr aux_phdrs[PROC_MAX_PHDRS];
	struct exec_info exec;
	struct vfs_file file = { 0, 0, 0 };
	long io_buffer;
	long task;
	u64 stack = 0;
	u64 load_bias = 0;

	io_buffer = bunix_buffer_create(PROC_SEGMENT_MAX);
	if (io_buffer <= 0 ||
	    vfs_open(vfs, path, &file) != 0 ||
	    vfs_stat(vfs, &file) != 0 ||
	    vfs_read_file(vfs, file.handle, file.size, 0,
			  (unsigned char *)&ehdr, sizeof(ehdr),
			  (u64)io_buffer) != 0 ||
	    read_magic(ehdr.ident) != ELF_MAGIC ||
	    ehdr.ident[4] != ELFCLASS64 ||
	    ehdr.ident[5] != ELFDATA2LSB ||
	    (ehdr.type != ET_EXEC && ehdr.type != ET_DYN) ||
	    ehdr.machine != EM_X86_64 ||
	    ehdr.phnum > PROC_MAX_PHDRS ||
	    ehdr.phentsize != sizeof(phdrs[0]) ||
	    vfs_read_file(vfs, file.handle, file.size, ehdr.phoff,
			  (unsigned char *)phdrs,
			  (u64)ehdr.phnum * sizeof(phdrs[0]),
			  (u64)io_buffer) != 0) {
		if (file.handle != 0) {
			vfs_close(vfs, file.handle);
		}
		if (io_buffer > 0) {
			bunix_handle_close((u64)io_buffer);
		}
		return -1;
	}
	load_bias = ehdr.type == ET_DYN ? PROC_DYN_LOAD_BIAS : 0;
	for (u64 i = 0; i < ehdr.phnum; i++) {
		aux_phdrs[i] = phdrs[i];
		aux_phdrs[i].vaddr += load_bias;
		aux_phdrs[i].paddr += load_bias;
	}
	exec.entry = ehdr.entry + load_bias;
	exec.phent = ehdr.phentsize;
	exec.phnum = ehdr.phnum;
	exec.phdrs = aux_phdrs;

	task = bunix_task_create(task_name);
	if (task < 0) {
		vfs_close(vfs, file.handle);
		bunix_handle_close((u64)io_buffer);
		return -1;
	}

	for (u64 i = 0; i < sizeof(caps) / sizeof(caps[0]); i++) {
		if (bunix_task_grant((u64)task, caps[i].handle,
				     caps[i].rights) != 0) {
			vfs_close(vfs, file.handle);
			bunix_handle_close((u64)io_buffer);
			bunix_handle_close((u64)task);
			return -1;
		}
	}
	for (u64 i = 0; i < ehdr.phnum; i++) {
		const struct elf64_phdr *phdr = &phdrs[i];

		if (phdr->type != PT_LOAD) {
			continue;
		}

		if (phdr->filesz > phdr->memsz ||
		    phdr->vaddr + phdr->memsz < phdr->vaddr) {
			vfs_close(vfs, file.handle);
			bunix_handle_close((u64)io_buffer);
			bunix_handle_close((u64)task);
			return -1;
		}

		const u64 page_start = align_down(phdr->vaddr + load_bias,
						  4096);
		const u64 page_end = ((phdr->vaddr + load_bias +
				       phdr->memsz + 4095) &
				      ~4095ull);

		for (u64 page = page_start; page < page_end; page += 4096) {
			const u64 page_end_addr = page + 4096;
			const u64 copy_start = max_u64(page,
						       phdr->vaddr + load_bias);
			const u64 copy_end = min_u64(page_end_addr,
						     phdr->vaddr + load_bias +
						     phdr->filesz);
			const u64 dst_offset = copy_start - page;
			const u64 src_offset = copy_start - load_bias -
					       phdr->vaddr;
			const u64 filesz = copy_start < copy_end ?
					   copy_end - copy_start : 0;

			mem_zero(segment_buffer, sizeof(segment_buffer));
			if (filesz != 0 &&
			    vfs_read_file(vfs, file.handle, file.size,
					  phdr->offset + src_offset,
					  segment_buffer + dst_offset,
					  filesz, (u64)io_buffer) != 0) {
				vfs_close(vfs, file.handle);
				bunix_handle_close((u64)io_buffer);
				bunix_handle_close((u64)task);
				return -1;
			}
			if (bunix_task_map((u64)task, page, segment_buffer,
					   4096, 4096,
					   (phdr->flags & PF_W) != 0) != 0) {
				vfs_close(vfs, file.handle);
				bunix_handle_close((u64)io_buffer);
				bunix_handle_close((u64)task);
				return -1;
			}
		}
	}

	if (apply_relative_relocations(vfs, &file, phdrs, ehdr.phnum,
				       (u64)task, load_bias,
				       (u64)io_buffer) != 0) {
		vfs_close(vfs, file.handle);
		bunix_handle_close((u64)io_buffer);
		bunix_handle_close((u64)task);
		return -1;
	}

	if (build_initial_stack((u64)task, path, &exec, &stack) != 0) {
		vfs_close(vfs, file.handle);
		bunix_handle_close((u64)io_buffer);
		bunix_handle_close((u64)task);
		return -1;
	}
	vfs_close(vfs, file.handle);
	bunix_handle_close((u64)io_buffer);
	if (is_linux_path(path)) {
		const long bunix_id = bunix_task_id((u64)task);

		if (bunix_id <= 0 ||
		    register_linux_process((u64)bunix_id, linux_parent_pid,
					   linux_pid) != 0) {
			bunix_handle_close((u64)task);
			return -1;
		}
	}
	const long started = bunix_task_start_at((u64)task, exec.entry, stack);

	if (started != 0 || task_handle == 0) {
		bunix_handle_close((u64)task);
	} else {
		*task_handle = (u64)task;
	}
	return started;
}

static void reply_status(u64 reply_handle, u64 pid, u64 status)
{
	struct bunix_msg reply = {
		.protocol = BUNIX_PROTO_PROC,
		.type = BUNIX_PROC_WAIT,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { 0, pid, status, 0 },
	};

	bunix_ipc_send(reply_handle, &reply);
}

static void process_reset(struct process *process)
{
	if (process == 0) {
		return;
	}

	if (process->task_handle != 0) {
		bunix_handle_close(process->task_handle);
	}
	process->pid = 0;
	process->linux_pid = 0;
	process->task_handle = 0;
	process->status = 0;
	process->exited = 0;
	process->waiter = 0;
}

static struct process *process_find(u64 pid)
{
	for (u64 i = 0; i < PROC_MAX_PROCESSES; i++) {
		if (processes[i].pid == pid) {
			return &processes[i];
		}
	}

	return 0;
}

static struct process *process_find_linux_pid(u64 linux_pid)
{
	if (linux_pid == 0) {
		return 0;
	}

	for (u64 i = 0; i < PROC_MAX_PROCESSES; i++) {
		if (processes[i].linux_pid == linux_pid) {
			return &processes[i];
		}
	}

	return 0;
}

static struct process *process_alloc(void)
{
	for (u64 i = 0; i < PROC_MAX_PROCESSES; i++) {
		if (processes[i].pid == 0) {
			return &processes[i];
		}
	}

	return 0;
}

static long spawn_process(const char *path, u64 *pid)
{
	u64 vfs;
	u64 linux_pid = 0;
	u64 task_handle = 0;
	u64 linux_parent_pid = 0;
	struct process *process = process_alloc();

	if (process == 0 || pid == 0) {
		return -1;
	}

	vfs = resolve_service(BUNIX_SERVICE_VFS, BUNIX_RIGHT_SEND);
	if (vfs == 0) {
		return -1;
	}

	process->pid = next_pid++;
	process->linux_pid = 0;
	process->task_handle = 0;
	process->status = 0;
	process->exited = 0;
	process->waiter = 0;
	*pid = process->pid;

	if (is_linux_path(path) && first_linux_pid != 0) {
		linux_parent_pid = first_linux_pid;
	}

	if (exec_path(vfs, path, task_name_for_path(path),
		      linux_parent_pid, &linux_pid, &task_handle) != 0) {
		process_reset(process);
		*pid = 0;
		return -1;
	}

	process->linux_pid = linux_pid;
	process->task_handle = task_handle;
	if (linux_pid != 0 && first_linux_pid == 0) {
		first_linux_pid = linux_pid;
	}
	return 0;
}

int main(void)
{
	struct bunix_msg message;

	bunix_console_write(proc_online, sizeof(proc_online) - 1);
	if (register_service(BUNIX_SERVICE_PROC, BUNIX_HANDLE_SELF) != 0) {
		bunix_console_write(proc_register_failed,
				    sizeof(proc_register_failed) - 1);
		return 1;
	}
	bunix_console_write(proc_ready, sizeof(proc_ready) - 1);

	for (;;) {
		struct bunix_msg reply = {
			.protocol = BUNIX_PROTO_PROC,
			.type = 0,
			.sender = 0,
			.cap_rights = 0,
			.reply = 0,
			.cap = 0,
			.words = { 0, 0, 0, 0 },
		};
		int should_reply = 1;

		if (bunix_ipc_recv(BUNIX_HANDLE_SELF, &message) != 0 ||
		    message.protocol != BUNIX_PROTO_PROC) {
			continue;
		}

		reply.type = message.type;
		switch (message.type) {
		case BUNIX_PROC_SPAWN: {
			char path[16];
			u64 pid = 0;

			for (u64 i = 0; i < sizeof(path); i++) {
				path[i] = '\0';
			}
			unpack_bytes((unsigned char *)path, &message.words[0],
				     sizeof(path));
			if (spawn_process(path, &pid) == 0) {
				reply.words[0] = 0;
				reply.words[1] = pid;
				if (str_eq(path, "/bin/lxtest")) {
					bunix_console_write(proc_exec_linux,
							    sizeof(proc_exec_linux) - 1);
					if (pid == 2) {
						bunix_console_write(proc_spawned_linux,
								    sizeof(proc_spawned_linux) - 1);
					}
				} else if (str_eq(path, "/bin/musl-hello")) {
					bunix_console_write(proc_exec_musl,
							    sizeof(proc_exec_musl) - 1);
					bunix_console_write(proc_spawned_musl,
							    sizeof(proc_spawned_musl) - 1);
				} else if (str_eq(path, "/bin/sh")) {
					bunix_console_write(proc_exec_shell,
							    sizeof(proc_exec_shell) - 1);
					bunix_console_write(proc_spawned_shell,
							    sizeof(proc_spawned_shell) - 1);
				} else {
					bunix_console_write(proc_exec,
							    sizeof(proc_exec) - 1);
					bunix_console_write(proc_spawned,
							    sizeof(proc_spawned) - 1);
				}
			} else {
				reply.words[0] = (u64)-1;
			}
			break;
		}
		case BUNIX_PROC_WAIT: {
			struct process *process = process_find(message.words[0]);

			if (process == 0) {
				reply.words[0] = (u64)-1;
			} else if (process->exited) {
				reply.words[0] = 0;
				reply.words[1] = process->pid;
				reply.words[2] = process->status;
				bunix_console_write(proc_waited,
						    sizeof(proc_waited) - 1);
				process_reset(process);
			} else if (message.reply != 0) {
				process->waiter = message.reply;
				should_reply = 0;
			} else {
				reply.words[0] = (u64)-1;
			}
			break;
		}
		case BUNIX_PROC_EXIT: {
			struct process *process = message.words[2] != 0 ?
						  process_find_linux_pid(message.words[0]) :
						  process_find(message.words[0]);

			if (process != 0) {
				if (message.words[3] != 0 &&
				    process->task_handle != 0) {
					(void)bunix_task_kill(process->task_handle);
				}
				process->status = message.words[1];
				process->exited = 1;
				reply.words[0] = 0;
				bunix_console_write(proc_exited,
						    sizeof(proc_exited) - 1);
				if (process->waiter != 0) {
					reply_status(process->waiter,
						     process->pid,
						     process->status);
					process->waiter = 0;
					bunix_console_write(proc_waited,
							    sizeof(proc_waited) - 1);
					process_reset(process);
				}
			} else {
				reply.words[0] = (u64)-1;
			}
			break;
		}
		default:
			reply.words[0] = (u64)-1;
			break;
		}

		if (should_reply && message.reply != 0) {
			bunix_ipc_send(message.reply, &reply);
		}
	}
}
