#include <bunix/alloc.h>
#include <bunix/id_table.h>
#include <bunix/libbunix.h>

#define PROC_HANDLE_CONSOLE (bunix_handle_find(BUNIX_CAP_CONS))
#define PROC_HANDLE_NAMES (bunix_handle_find(BUNIX_CAP_NAME))
#define PROC_HANDLE_TIME (bunix_handle_find(BUNIX_CAP_TIME))
#define PROC_HANDLE_MGMT (bunix_handle_find(BUNIX_CAP_PRMG))
#define PROC_HANDLE_USER_MGMT (bunix_handle_find(BUNIX_CAP_USRM))
#define PROC_HANDLE_LINUX_MGMT (bunix_handle_find(BUNIX_CAP_LNXM))

enum {
	PROC_SEGMENT_MAX = 4096,
	PROC_INIT_STACK_MAX = 512 * 1024,
	PROC_SPAWN_BUFFER_MAX = 512 * 1024,
	USER_STACK_TOP = 0x800000,
	ELF_MAGIC = 0x464c457f,
	ELFCLASS64 = 2,
	ELFDATA2LSB = 1,
	ET_EXEC = 2,
	ET_DYN = 3,
	EM_X86_64 = 62,
	EM_RISCV = 243,
	PT_LOAD = 1,
	PT_INTERP = 3,
	PF_X = 1 << 0,
	PF_W = 1 << 1,
	PF_R = 1 << 2,
	AT_NULL = 0,
	AT_PHDR = 3,
	AT_PHENT = 4,
	AT_PHNUM = 5,
	AT_PAGESZ = 6,
	AT_BASE = 7,
	AT_UID = 11,
	AT_EUID = 12,
	AT_GID = 13,
	AT_EGID = 14,
	AT_CLKTCK = 17,
	AT_SECURE = 23,
	AT_RANDOM = 25,
	AT_ENTRY = 9,
	AT_EXECFN = 31,
	PROC_DYN_LOAD_BIAS = 0x400000,
	PROC_INTERP_LOAD_BIAS = 0x600000,
	PROC_EXEC_PATH_MAX = 4096,
	PROC_TASK_NAME_MAX = 32,
	PROC_SPAWN_SET_LOGIN = 1,
	PROC_SPAWN_LINUX = 2,
	PROC_SPAWN_FLAGS_MASK = 0xffffffff,
	PROC_SPAWN_SESSION_SHIFT = 32,
};

static unsigned short proc_elf_machine(void)
{
#if defined(__riscv)
	return EM_RISCV;
#else
	return EM_X86_64;
#endif
}

struct process {
	u64 pid;
	u64 ppid;
	u64 task_id;
	u64 linux_pid;
	u64 task_handle;
	u64 status;
	u64 exited;
	u64 waiter;
	u64 cmd_len;
	char *cmdline;
};

struct proc_exec_entry {
	struct bunix_tree_node node;
	char path[PROC_EXEC_PATH_MAX];
	char task_name[PROC_TASK_NAME_MAX];
	int linux;
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

struct vfs_file {
	u64 handle;
	u64 size;
	u64 type;
};

struct exec_info {
	u64 entry;
	u64 phent;
	u64 phnum;
	u64 phdr_addr;
	const struct elf64_phdr *phdrs;
};

struct exec_strings {
	u64 argc;
	u64 envc;
	char **argv;
	char **envp;
};

struct exec_credentials {
	u64 uid;
	u64 gid;
	u64 euid;
	u64 egid;
	u64 secure;
};

struct exec_handles {
	u64 stdout_handle;
	u64 stderr_handle;
	u64 time_handle;
	u64 proc_handle;
	u64 names_handle;
};

static int exec_handles_from_task(u64 task, struct exec_handles *handles)
{
	if (handles == 0) {
		return -1;
	}

	handles->stdout_handle = bunix_task_handle_find(task, BUNIX_CAP_CONS);
	handles->stderr_handle = handles->stdout_handle;
	handles->time_handle = bunix_task_handle_find(task, BUNIX_CAP_TIME);
	handles->proc_handle = bunix_task_handle_find(task, BUNIX_CAP_PROC);
	handles->names_handle = bunix_task_handle_find(task, BUNIX_CAP_NAME);

	return handles->stdout_handle != 0 &&
		       handles->time_handle != 0 &&
		       handles->proc_handle != 0 &&
		       handles->names_handle != 0 ?
		       0 : -1;
}

struct pending_exec_replace {
	u64 token;
	u64 task_handle;
	u64 sender;
};

static struct bunix_map processes_by_pid;
static struct bunix_map processes_by_task_id;
static struct bunix_map processes_by_linux_pid;
static struct bunix_map pending_linux_exits;
static struct bunix_map pending_exec_replaces;
static struct bunix_tree exec_registry;
static u64 next_pid = 1;
static u64 first_linux_pid;
static u64 next_exec_replace_token = 1;
static unsigned char segment_buffer[PROC_SEGMENT_MAX];
static const char proc_online[] = "proc: online\n";
static const char proc_ready[] = "proc: ready\n";
static const char proc_register_failed[] = "proc: register failed\n";

static u64 str_len(const char *text);
static int str_eq(const char *left, const char *right);

static int path_is_login(const char *path)
{
	return str_eq(path, "/bin/login") || str_eq(path, "login");
}

static int grant_login_management_caps(u64 task)
{
	if (PROC_HANDLE_USER_MGMT == 0 || PROC_HANDLE_LINUX_MGMT == 0) {
		return -1;
	}
	if (bunix_task_grant_tagged(task, PROC_HANDLE_USER_MGMT,
				    BUNIX_RIGHT_SEND, BUNIX_CAP_USRM) <= 0) {
		return -1;
	}
	if (bunix_task_grant_tagged(task, PROC_HANDLE_LINUX_MGMT,
				    BUNIX_RIGHT_SEND, BUNIX_CAP_LNXM) <= 0) {
		return -1;
	}
	return 0;
}

static u64 elf_phdr_to_bunix_prot(const struct elf64_phdr *phdr)
{
	u64 prot = 0;

	if ((phdr->flags & PF_R) != 0) {
		prot |= BUNIX_VM_PROT_READ;
	}
	if ((phdr->flags & PF_W) != 0) {
		prot |= BUNIX_VM_PROT_WRITE;
	}
	if ((phdr->flags & PF_X) != 0) {
		prot |= BUNIX_VM_PROT_EXEC;
	}
	return prot;
}

static long register_service(u64 service, u64 handle)
{
	(void)service;
	return bunix_names_register_claim(bunix_handle_find(BUNIX_CAP_CLAM),
					  handle);
}

static int recv_proc_message(struct bunix_msg *message, int *management)
{
	const u64 mgmt = PROC_HANDLE_MGMT;
	u64 ports[2];
	u64 count = 0;
	u64 index = 0;
	u64 mgmt_index = (u64)-1;

	if (message == 0 || management == 0) {
		return -1;
	}
	if (mgmt != 0) {
		mgmt_index = count;
		ports[count++] = mgmt;
	}
	ports[count++] = BUNIX_HANDLE_SELF;
	if (bunix_ipc_recv_any(ports, count, message, &index) != 0) {
		return -1;
	}
	*management = (index == mgmt_index) ? 1 : 0;
	return 0;
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

static char *string_dup_len(const char *text, u64 len)
{
	char *copy;

	if (text == 0) {
		return 0;
	}
	copy = (char *)bunix_alloc(len + 1);
	if (copy == 0) {
		return 0;
	}
	for (u64 i = 0; i < len; i++) {
		copy[i] = text[i];
	}
	copy[len] = '\0';
	return copy;
}

static int process_set_cmdline(struct process *process, const char *cmdline,
			       u64 cmd_len)
{
	char *copy;

	if (process == 0 || cmdline == 0 || cmd_len >= PROC_EXEC_PATH_MAX) {
		return -1;
	}
	copy = string_dup_len(cmdline, cmd_len);
	if (copy == 0) {
		return -1;
	}
	if (process->cmdline != 0) {
		bunix_free(process->cmdline);
	}
	process->cmdline = copy;
	process->cmd_len = cmd_len;
	return 0;
}

static int process_set_cmdline_from_buffer(struct process *process, u64 buffer,
					   u64 len)
{
	char *text;
	int result;

	if (process == 0 || buffer == 0 || len == 0 ||
	    len > PROC_EXEC_PATH_MAX) {
		return -1;
	}
	text = (char *)bunix_alloc(len);
	if (text == 0) {
		return -1;
	}
	if (bunix_buffer_read(buffer, 0, text, len) != 0 ||
	    text[len - 1] != '\0') {
		bunix_free(text);
		return -1;
	}
	result = process_set_cmdline(process, text, len - 1);
	bunix_free(text);
	return result;
}

static unsigned int read_magic(const unsigned char *ident)
{
	return ((unsigned int)ident[0]) |
	       ((unsigned int)ident[1] << 8) |
	       ((unsigned int)ident[2] << 16) |
	       ((unsigned int)ident[3] << 24);
}

static int copy_text(char *dst, u64 dst_len, const char *src)
{
	u64 i;

	if (dst == 0 || dst_len == 0 || src == 0) {
		return -1;
	}
	for (i = 0; i + 1 < dst_len && src[i] != '\0'; i++) {
		dst[i] = src[i];
	}
	if (src[i] != '\0') {
		return -1;
	}
	dst[i] = '\0';
	return 0;
}

static struct proc_exec_entry *exec_entry_for_path(const char *path)
{
	return (struct proc_exec_entry *)bunix_tree_get(&exec_registry, path);
}

static int is_linux_path(const char *path)
{
	const struct proc_exec_entry *entry = exec_entry_for_path(path);

	return entry != 0 && entry->linux;
}

static const char *task_name_for_path(const char *path)
{
	const struct proc_exec_entry *entry = exec_entry_for_path(path);

	return entry != 0 ? entry->task_name : "process";
}

static int exec_registry_add(const char *path, const char *task_name,
			     int linux)
{
	struct proc_exec_entry *entry;

	if (path == 0 || task_name == 0 ||
	    bunix_tree_get(&exec_registry, path) != 0) {
		return -1;
	}
	entry = (struct proc_exec_entry *)bunix_calloc(1, sizeof(*entry));
	if (entry == 0 ||
	    copy_text(entry->path, sizeof(entry->path), path) != 0 ||
	    copy_text(entry->task_name, sizeof(entry->task_name),
		      task_name) != 0) {
		if (entry != 0) {
			bunix_free(entry);
		}
		return -1;
	}
	entry->linux = linux;
	if (bunix_tree_insert_node(&exec_registry, &entry->node, entry->path,
				   (u64)entry) != 0) {
		bunix_free(entry);
		return -1;
	}
	return 0;
}

static int read_buffer_text(u64 buffer, u64 offset, u64 len, char *out,
			    u64 out_size)
{
	if (buffer == 0 || len == 0 || len > out_size) {
		return -1;
	}
	for (u64 i = 0; i < out_size; i++) {
		out[i] = '\0';
	}
	if (bunix_buffer_read(buffer, offset, out, len) != 0 ||
	    out[len - 1] != '\0') {
		return -1;
	}
	return 0;
}

static int register_exec_from_message(const struct bunix_msg *message)
{
	char path[PROC_EXEC_PATH_MAX];
	char task_name[PROC_TASK_NAME_MAX];

	if (message == 0 ||
	    (message->cap_rights & BUNIX_RIGHT_RECV) == 0 ||
	    read_buffer_text(message->cap, 0, message->words[0],
			     path, sizeof(path)) != 0 ||
	    read_buffer_text(message->cap, message->words[0],
			     message->words[1], task_name,
			     sizeof(task_name)) != 0) {
		return -1;
	}
	return exec_registry_add(path, task_name, message->words[2] != 0);
}

static u64 str_len(const char *text)
{
	u64 len = 0;

	while (text[len] != '\0') {
		len++;
	}

	return len;
}

static int str_eq(const char *left, const char *right)
{
	u64 i = 0;

	if (left == 0 || right == 0) {
		return 0;
	}
	while (left[i] != '\0' && right[i] != '\0') {
		if (left[i] != right[i]) {
			return 0;
		}
		i++;
	}

	return left[i] == right[i];
}

static void append_dec(char *line, u64 *cursor, u64 value)
{
	char digits[20];
	u64 count = 0;

	if (value == 0) {
		line[(*cursor)++] = '0';
		return;
	}
	while (value != 0 && count < sizeof(digits)) {
		digits[count++] = (char)('0' + (value % 10));
		value /= 10;
	}
	while (count != 0) {
		line[(*cursor)++] = digits[--count];
	}
}

static void log_pid_line(const char *prefix, u64 pid)
{
	char line[64];
	u64 cursor = 0;

	for (u64 i = 0; prefix[i] != '\0' && cursor < sizeof(line); i++) {
		line[cursor++] = prefix[i];
	}
	append_dec(line, &cursor, pid);
	if (cursor < sizeof(line)) {
		line[cursor++] = '\n';
	}
	bunix_console_log(line, cursor);
}

static void log_exec_line(const char *path)
{
	const char prefix[] = "proc: exec ";
	char line[PROC_EXEC_PATH_MAX + sizeof(prefix) + 1];
	u64 cursor = 0;

	for (u64 i = 0; prefix[i] != '\0' && cursor < sizeof(line); i++) {
		line[cursor++] = prefix[i];
	}
	if (path != 0) {
		for (u64 i = 0; path[i] != '\0' && cursor + 1 < sizeof(line);
		     i++) {
			line[cursor++] = path[i];
		}
	}
	if (cursor < sizeof(line)) {
		line[cursor++] = '\n';
	}
	bunix_console_log(line, cursor);
}

static void append_text_bounded(char *line, u64 size, u64 *cursor,
				const char *text)
{
	if (line == 0 || cursor == 0 || text == 0) {
		return;
	}
	for (u64 i = 0; text[i] != '\0' && *cursor + 1 < size; i++) {
		line[(*cursor)++] = text[i];
	}
}

static void append_dec_bounded(char *line, u64 size, u64 *cursor, u64 value)
{
	char digits[20];
	u64 count = 0;

	if (line == 0 || cursor == 0 || *cursor >= size) {
		return;
	}
	if (value == 0) {
		if (*cursor + 1 < size) {
			line[(*cursor)++] = '0';
		}
		return;
	}
	while (value != 0 && count < sizeof(digits)) {
		digits[count++] = (char)('0' + (value % 10));
		value /= 10;
	}
	while (count != 0 && *cursor + 1 < size) {
		line[(*cursor)++] = digits[--count];
	}
}

static u64 elapsed_ticks(u64 start, u64 end)
{
	return end >= start ? end - start : 0;
}

static void log_exec_timing_line(const char *path, long result,
				 u64 start, u64 headers, u64 interp,
				 u64 mapped, u64 relocated, u64 stacked,
				 u64 done)
{
	char line[512];
	u64 cursor = 0;

	if (bunix_cmdline_has("debug-proc-exec-timing") <= 0) {
		return;
	}

	append_text_bounded(line, sizeof(line), &cursor,
			    "proc-exec-timing: result=");
	if (result < 0) {
		append_text_bounded(line, sizeof(line), &cursor, "-");
		append_dec_bounded(line, sizeof(line), &cursor,
				   (u64)(-result));
	} else {
		append_dec_bounded(line, sizeof(line), &cursor, (u64)result);
	}
	append_text_bounded(line, sizeof(line), &cursor, " total=");
	append_dec_bounded(line, sizeof(line), &cursor,
			   elapsed_ticks(start, done));
	append_text_bounded(line, sizeof(line), &cursor, " headers=");
	append_dec_bounded(line, sizeof(line), &cursor,
			   elapsed_ticks(start, headers));
	append_text_bounded(line, sizeof(line), &cursor, " interp=");
	append_dec_bounded(line, sizeof(line), &cursor,
			   elapsed_ticks(headers, interp));
	append_text_bounded(line, sizeof(line), &cursor, " map=");
	append_dec_bounded(line, sizeof(line), &cursor,
			   elapsed_ticks(interp, mapped));
	append_text_bounded(line, sizeof(line), &cursor, " reloc=");
	append_dec_bounded(line, sizeof(line), &cursor,
			   elapsed_ticks(mapped, relocated));
	append_text_bounded(line, sizeof(line), &cursor, " stack=");
	append_dec_bounded(line, sizeof(line), &cursor,
			   elapsed_ticks(relocated, stacked));
	append_text_bounded(line, sizeof(line), &cursor, " path=");
	append_text_bounded(line, sizeof(line), &cursor, path);
	if (cursor < sizeof(line)) {
		line[cursor++] = '\n';
	}
	bunix_console_log(line, cursor);
}

static void log_exec_load_stage(u64 task, const char *stage, const char *path)
{
	char line[512];
	u64 cursor = 0;

	if (bunix_cmdline_has("debug-proc-exec-load") <= 0) {
		return;
	}

	append_text_bounded(line, sizeof(line), &cursor,
			    "proc-exec-load: task=");
	append_dec_bounded(line, sizeof(line), &cursor, task);
	append_text_bounded(line, sizeof(line), &cursor, " stage=");
	append_text_bounded(line, sizeof(line), &cursor, stage);
	append_text_bounded(line, sizeof(line), &cursor, " path=");
	append_text_bounded(line, sizeof(line), &cursor,
			    path != 0 ? path : "");
	if (cursor < sizeof(line)) {
		line[cursor++] = '\n';
	}
	bunix_console_log(line, cursor);
}

static void log_exit_line(const char *prefix, u64 pid, u64 status)
{
	char line[96];
	u64 cursor = 0;

	for (u64 i = 0; prefix[i] != '\0' && cursor < sizeof(line); i++) {
		line[cursor++] = prefix[i];
	}
	append_dec(line, &cursor, pid);
	for (const char *text = " status="; *text != '\0'; text++) {
		line[cursor++] = *text;
	}
	append_dec(line, &cursor, status);
	if (cursor < sizeof(line)) {
		line[cursor++] = '\n';
	}
	bunix_console_log(line, cursor);
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

static char *text_dup(const char *text)
{
	const u64 len = str_len(text) + 1;
	char *copy = (char *)bunix_alloc(len);

	if (copy == 0) {
		return 0;
	}
	for (u64 i = 0; i < len; i++) {
		copy[i] = text[i];
	}
	return copy;
}

static void exec_strings_free(struct exec_strings *strings)
{
	if (strings == 0) {
		return;
	}
	if (strings->argv != 0) {
		for (u64 i = 0; i < strings->argc; i++) {
			if (strings->argv[i] != 0) {
				bunix_free(strings->argv[i]);
			}
		}
		bunix_free(strings->argv);
	}
	if (strings->envp != 0) {
		for (u64 i = 0; i < strings->envc; i++) {
			if (strings->envp[i] != 0) {
				bunix_free(strings->envp[i]);
			}
		}
		bunix_free(strings->envp);
	}
	strings->argc = 0;
	strings->envc = 0;
	strings->argv = 0;
	strings->envp = 0;
}

static int exec_strings_default(struct exec_strings *strings, const char *path)
{
	if (strings == 0 || path == 0) {
		return -1;
	}
	strings->argc = 1;
	strings->envc = 0;
	strings->argv = (char **)bunix_calloc(1, sizeof(char *));
	strings->envp = 0;
	if (strings->argv == 0) {
		return -1;
	}
	strings->argv[0] = text_dup(path);
	if (strings->argv[0] == 0) {
		exec_strings_free(strings);
		return -1;
	}
	return 0;
}

static int spawn_buffer_string(const unsigned char *buffer, u64 *pos,
			       u64 total, char **out)
{
	u64 start;
	u64 len = 0;
	char *copy;

	if (buffer == 0 || pos == 0 || out == 0 || *pos >= total) {
		return -1;
	}
	start = *pos;
	while (*pos < total && buffer[*pos] != '\0') {
		(*pos)++;
		len++;
	}
	if (*pos >= total || len == 0) {
		return -1;
	}
	(*pos)++;
	copy = (char *)bunix_alloc(len + 1);
	if (copy == 0) {
		return -1;
	}
	for (u64 i = 0; i < len; i++) {
		copy[i] = (char)buffer[start + i];
	}
	copy[len] = '\0';
	*out = copy;
	return 0;
}

static int exec_strings_from_spawn_buffer(const struct bunix_msg *message,
					  char **path,
					  struct exec_strings *strings)
{
	unsigned char *buffer;
	u64 total;
	u64 pos = 0;
	int result = -1;

	if (message == 0 || path == 0 || strings == 0 ||
	    message->cap == 0 ||
	    (message->cap_rights & BUNIX_RIGHT_RECV) == 0 ||
	    message->words[0] == 0 ||
	    message->words[0] > PROC_SPAWN_BUFFER_MAX ||
	    message->words[1] == 0 ||
	    message->words[1] > message->words[0] ||
	    message->words[2] > message->words[0]) {
		return -1;
	}
	total = message->words[0];
	buffer = (unsigned char *)bunix_alloc(total);
	if (buffer == 0) {
		return -1;
	}
	if (bunix_buffer_read(message->cap, 0, buffer, total) != 0) {
		goto out;
	}
	if (spawn_buffer_string(buffer, &pos, total, path) != 0) {
		goto out;
	}

	strings->argc = message->words[1];
	strings->envc = message->words[2];
	strings->argv = (char **)bunix_calloc(strings->argc, sizeof(char *));
	strings->envp = strings->envc == 0 ? 0 :
		(char **)bunix_calloc(strings->envc, sizeof(char *));
	if (strings->argv == 0 || (strings->envc != 0 && strings->envp == 0)) {
		goto out;
	}
	for (u64 i = 0; i < strings->argc; i++) {
		if (spawn_buffer_string(buffer, &pos, total,
					&strings->argv[i]) != 0) {
			goto out;
		}
	}
	for (u64 i = 0; i < strings->envc; i++) {
		if (spawn_buffer_string(buffer, &pos, total,
					&strings->envp[i]) != 0) {
			goto out;
		}
	}
	result = pos == total ? 0 : -1;
out:
	bunix_free(buffer);
	if (result != 0) {
		if (*path != 0) {
			bunix_free(*path);
			*path = 0;
		}
		exec_strings_free(strings);
	}
	return result;
}

static int exec_strings_from_replace_buffer(const struct bunix_msg *message,
					    char **load_path,
					    char **execfn_path,
					    struct exec_credentials *creds,
					    struct exec_strings *strings)
{
	unsigned char *buffer;
	u64 total;
	u64 pos = sizeof(*creds);
	int result = -1;

	if (message == 0 || load_path == 0 || execfn_path == 0 ||
	    creds == 0 || strings == 0 || message->cap == 0 ||
	    (message->cap_rights & BUNIX_RIGHT_RECV) == 0 ||
	    message->words[1] <= sizeof(*creds) ||
	    message->words[1] > PROC_SPAWN_BUFFER_MAX ||
	    message->words[2] == 0 ||
	    message->words[2] > message->words[1] ||
	    message->words[3] > message->words[1]) {
		return -1;
	}
	total = message->words[1];
	buffer = (unsigned char *)bunix_alloc(total);
	if (buffer == 0) {
		return -1;
	}
	if (bunix_buffer_read(message->cap, 0, buffer, total) != 0) {
		goto out;
	}
	creds->uid = read_u64_le(buffer);
	creds->gid = read_u64_le(buffer + 8);
	creds->euid = read_u64_le(buffer + 16);
	creds->egid = read_u64_le(buffer + 24);
	creds->secure = read_u64_le(buffer + 32);
	if (spawn_buffer_string(buffer, &pos, total, load_path) != 0 ||
	    spawn_buffer_string(buffer, &pos, total, execfn_path) != 0) {
		goto out;
	}

	strings->argc = message->words[2];
	strings->envc = message->words[3];
	strings->argv = (char **)bunix_calloc(strings->argc, sizeof(char *));
	strings->envp = strings->envc == 0 ? 0 :
		(char **)bunix_calloc(strings->envc, sizeof(char *));
	if (strings->argv == 0 || (strings->envc != 0 && strings->envp == 0)) {
		goto out;
	}
	for (u64 i = 0; i < strings->argc; i++) {
		if (spawn_buffer_string(buffer, &pos, total,
					&strings->argv[i]) != 0) {
			goto out;
		}
	}
	for (u64 i = 0; i < strings->envc; i++) {
		if (spawn_buffer_string(buffer, &pos, total,
					&strings->envp[i]) != 0) {
			goto out;
		}
	}
	result = pos == total ? 0 : -1;
out:
	bunix_free(buffer);
	if (result != 0) {
		if (*load_path != 0) {
			bunix_free(*load_path);
			*load_path = 0;
		}
		if (*execfn_path != 0) {
			bunix_free(*execfn_path);
			*execfn_path = 0;
		}
		exec_strings_free(strings);
	}
	return result;
}

static long vfs_open(u64 vfs, const char *path, u64 subject,
		     struct vfs_file *file)
{
	const u64 path_len = str_len(path) + 1;
	const char cwd[] = "/";
	const u64 cwd_len = sizeof(cwd);
	long path_buffer;
	struct bunix_msg request;
	struct bunix_msg reply;

	if (file == 0) {
		return -1;
	}
	path_buffer = bunix_buffer_create(cwd_len + path_len);
	if (path_buffer <= 0 ||
	    bunix_buffer_write((u64)path_buffer, 0, cwd, cwd_len) != 0 ||
	    bunix_buffer_write((u64)path_buffer, cwd_len, path, path_len) != 0) {
		if (path_buffer > 0) {
			bunix_handle_close((u64)path_buffer);
		}
		return -1;
	}

	request = (struct bunix_msg){
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_OPEN_BUFFER,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_RECV,
		.reply = 0,
		.cap = (u64)path_buffer,
		.words = { cwd_len, path_len, 0, subject },
	};

	if (bunix_ipc_call(vfs, &request, &reply) != 0 ||
	    reply.words[0] != 0 ||
	    reply.words[1] == 0 ||
	    reply.words[3] != BUNIX_VFS_TYPE_REGULAR) {
		bunix_handle_close((u64)path_buffer);
		return -1;
	}
	bunix_handle_close((u64)path_buffer);

	file->handle = reply.words[1];
	file->size = reply.words[2];
	file->type = reply.words[3];
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

static long task_write_bytes(u64 task, u64 vaddr, const unsigned char *buffer,
			     u64 len)
{
	u64 done = 0;

	while (done < len) {
		u64 chunk = len - done;

		if (chunk > PROC_SEGMENT_MAX) {
			chunk = PROC_SEGMENT_MAX;
		}
		if (bunix_task_write(task, vaddr + done, buffer + done,
				     chunk) != 0) {
			return -1;
		}
		done += chunk;
	}
	return 0;
}

static long read_interp_path(u64 vfs, const struct vfs_file *file,
			     const struct elf64_phdr *phdrs, u64 phnum,
			     char *path, u64 path_size, u64 io_buffer)
{
	if (path == 0 || path_size == 0) {
		return -1;
	}
	path[0] = '\0';

	for (u64 i = 0; i < phnum; i++) {
		const struct elf64_phdr *phdr = &phdrs[i];

		if (phdr->type != PT_INTERP) {
			continue;
		}
		if (phdr->filesz == 0 || phdr->filesz >= path_size ||
		    vfs_read_file(vfs, file->handle, file->size,
				  phdr->offset, (unsigned char *)path,
				  phdr->filesz, io_buffer) != 0) {
			return -1;
		}
		path[phdr->filesz] = '\0';
		return 1;
	}

	return 0;
}

static long map_load_segments(u64 vfs, const struct vfs_file *file,
			      const struct elf64_phdr *phdrs, u64 phnum,
			      u64 task, u64 load_bias, u64 io_buffer)
{
	for (u64 i = 0; i < phnum; i++) {
		const struct elf64_phdr *phdr = &phdrs[i];

		if (phdr->type != PT_LOAD) {
			continue;
		}
		if (phdr->filesz > phdr->memsz ||
		    phdr->vaddr + phdr->memsz < phdr->vaddr) {
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
			    vfs_read_file(vfs, file->handle, file->size,
					  phdr->offset + src_offset,
					  segment_buffer + dst_offset,
					  filesz, io_buffer) != 0) {
				return -1;
			}
			if (bunix_task_map_prot(task, page, segment_buffer,
						4096, 4096,
						elf_phdr_to_bunix_prot(phdr))
			    != 0) {
				return -1;
			}
		}
	}

	return 0;
}

#include "exec_stack.c"

static long register_linux_process(u64 task, u64 requested_pid,
				   u64 ppid, u64 session_id, u64 *linux_pid)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_LINUX,
		.type = BUNIX_LINUX_REGISTER_PROCESS,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { task, ppid, session_id, requested_pid },
	};
	struct bunix_msg reply;
	const u64 linux = PROC_HANDLE_LINUX_MGMT;

	if (linux == 0) {
		bunix_console_log("proc: linux mgmt missing\n",
				  sizeof("proc: linux mgmt missing\n") - 1);
		return -1;
	}
	if (bunix_ipc_call(linux, &request, &reply) != 0) {
		bunix_console_log("proc: linux register call failed\n",
				  sizeof("proc: linux register call failed\n") - 1);
		return -1;
	}
	if ((long)reply.words[0] <= 0) {
		bunix_console_log("proc: linux register denied\n",
				  sizeof("proc: linux register denied\n") - 1);
		return -1;
	}

	if (linux_pid != 0) {
		*linux_pid = reply.words[0];
	}
	return 0;
}

static long register_user_process(u64 task)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_USER,
		.type = BUNIX_USER_REGISTER_PROCESS,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { task, 0, 0, 0 },
	};
	struct bunix_msg reply;
	const u64 user = PROC_HANDLE_USER_MGMT;

	if (user == 0 ||
	    bunix_ipc_call(user, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return -1;
	}

	return 0;
}

static void unregister_user_process(u64 task)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_USER,
		.type = BUNIX_USER_EXIT_PROCESS,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { task, 0, 0, 0 },
	};
	struct bunix_msg reply;
	const u64 user = PROC_HANDLE_USER_MGMT;

	if (user != 0) {
		(void)bunix_ipc_call(user, &request, &reply);
	}
}

static long apply_login_to_task(u64 task, u64 login_uid)
{
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_USER,
		.type = BUNIX_USER_APPLY_LOGIN,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { task, login_uid, 0, 0 },
	};
	struct bunix_msg reply;
	const u64 user = PROC_HANDLE_USER_MGMT;

	if (user == 0 ||
	    bunix_ipc_call(user, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return -1;
	}

	return 0;
}

static int elf_phdr_table_size(const struct elf64_ehdr *ehdr, u64 file_size,
			       u64 *bytes)
{
	if (ehdr == 0 || bytes == 0 ||
	    ehdr->phnum == 0 ||
	    ehdr->phentsize != sizeof(struct elf64_phdr)) {
		return -1;
	}

	*bytes = (u64)ehdr->phnum * sizeof(struct elf64_phdr);
	if (ehdr->phoff > file_size || *bytes > file_size - ehdr->phoff) {
		return -1;
	}

	return 0;
}

static int elf_offset_to_vaddr(const struct elf64_phdr *phdrs, u64 phnum,
			       u64 offset, u64 len, u64 load_bias,
			       u64 *vaddr)
{
	if (phdrs == 0 || vaddr == 0 || offset + len < offset) {
		return -1;
	}

	for (u64 i = 0; i < phnum; i++) {
		const struct elf64_phdr *phdr = &phdrs[i];

		if (phdr->type != PT_LOAD ||
		    phdr->offset + phdr->filesz < phdr->offset ||
		    offset < phdr->offset ||
		    offset + len > phdr->offset + phdr->filesz) {
			continue;
		}

		const u64 delta = offset - phdr->offset;
		const u64 image_vaddr = phdr->vaddr + delta;
		const u64 mapped_vaddr = load_bias + image_vaddr;

		if (image_vaddr < phdr->vaddr ||
		    mapped_vaddr < image_vaddr) {
			return -1;
		}

		*vaddr = mapped_vaddr;
		return 0;
	}

	return -1;
}

static long load_task_image(u64 vfs, u64 task, int clear_existing,
			    const char *load_path,
			    const char *execfn_path,
			    const struct exec_strings *strings,
			    const struct exec_credentials *creds,
			    const struct exec_handles *handles,
			    u64 *entry, u64 *stack)
{
	struct elf64_ehdr ehdr;
	struct elf64_ehdr interp_ehdr;
	struct elf64_phdr *phdrs = 0;
	struct elf64_phdr *interp_phdrs = 0;
	struct elf64_phdr *aux_phdrs = 0;
	struct exec_info exec = { 0, 0, 0, 0, 0 };
	struct vfs_file file = { 0, 0, 0 };
	struct vfs_file interp_file = { 0, 0, 0 };
	char interp_path[PROC_EXEC_PATH_MAX];
	u64 phdr_bytes = 0;
	u64 interp_phdr_bytes = 0;
	long io_buffer;
	u64 load_bias = 0;
	u64 interp_bias = 0;
	u64 start_entry = 0;
	long result = -1;
	const u64 timing_start = bunix_timer_ticks();
	u64 timing_headers = timing_start;
	u64 timing_interp = timing_start;
	u64 timing_mapped = timing_start;
	u64 timing_relocated = timing_start;
	u64 timing_stacked = timing_start;

	if (load_path == 0 || execfn_path == 0 || entry == 0 || stack == 0) {
		return -1;
	}

	log_exec_load_stage(bunix_task_id(task), "buffer", load_path);
	io_buffer = bunix_buffer_create(PROC_SEGMENT_MAX);
	if (io_buffer <= 0) {
		bunix_console_log("proc: exec buffer failed\n",
				  sizeof("proc: exec buffer failed\n") - 1);
		return -1;
	}
	log_exec_load_stage(bunix_task_id(task), "open-main", load_path);
	if (vfs_open(vfs, load_path, bunix_task_id(task), &file) != 0) {
		bunix_console_log("proc: exec open failed\n",
				  sizeof("proc: exec open failed\n") - 1);
		goto out;
	}
	log_exec_load_stage(bunix_task_id(task), "read-ehdr", load_path);
	if (vfs_read_file(vfs, file.handle, file.size, 0,
			  (unsigned char *)&ehdr, sizeof(ehdr),
			  (u64)io_buffer) != 0) {
		bunix_console_log("proc: exec ehdr read failed\n",
				  sizeof("proc: exec ehdr read failed\n") - 1);
		goto out;
	}
	if (read_magic(ehdr.ident) != ELF_MAGIC ||
	    ehdr.ident[4] != ELFCLASS64 ||
	    ehdr.ident[5] != ELFDATA2LSB ||
	    (ehdr.type != ET_EXEC && ehdr.type != ET_DYN) ||
	    ehdr.machine != proc_elf_machine() ||
	    elf_phdr_table_size(&ehdr, file.size, &phdr_bytes) != 0) {
		bunix_console_log("proc: exec elf invalid\n",
				  sizeof("proc: exec elf invalid\n") - 1);
		goto out;
	}
	phdrs = (struct elf64_phdr *)bunix_alloc(phdr_bytes);
	aux_phdrs = (struct elf64_phdr *)bunix_alloc(phdr_bytes);
	if (phdrs == 0 || aux_phdrs == 0) {
		bunix_console_log("proc: exec phdr alloc failed\n",
				  sizeof("proc: exec phdr alloc failed\n") - 1);
		goto out;
	}
	log_exec_load_stage(bunix_task_id(task), "read-phdrs", load_path);
	if (vfs_read_file(vfs, file.handle, file.size, ehdr.phoff,
			  (unsigned char *)phdrs, phdr_bytes,
			  (u64)io_buffer) != 0) {
		bunix_console_log("proc: exec phdr read failed\n",
				  sizeof("proc: exec phdr read failed\n") - 1);
		goto out;
	}
	timing_headers = bunix_timer_ticks();
	load_bias = ehdr.type == ET_DYN ? PROC_DYN_LOAD_BIAS : 0;
	for (u64 i = 0; i < ehdr.phnum; i++) {
		aux_phdrs[i] = phdrs[i];
		aux_phdrs[i].vaddr += load_bias;
		aux_phdrs[i].paddr += load_bias;
	}
	exec.entry = ehdr.entry + load_bias;
	exec.phent = ehdr.phentsize;
	exec.phnum = ehdr.phnum;
	if (elf_offset_to_vaddr(phdrs, ehdr.phnum, ehdr.phoff, phdr_bytes,
				load_bias, &exec.phdr_addr) != 0) {
		exec.phdr_addr = 0;
	}
	exec.phdrs = aux_phdrs;
	start_entry = exec.entry;

	log_exec_load_stage(bunix_task_id(task), "read-interp", load_path);
	const long interp = read_interp_path(vfs, &file, phdrs, ehdr.phnum,
					     interp_path, sizeof(interp_path),
					     (u64)io_buffer);
	if (interp < 0) {
		goto out;
	}
	if (interp > 0) {
		log_exec_load_stage(bunix_task_id(task), "open-interp",
				    interp_path);
		if (vfs_open(vfs, interp_path, bunix_task_id(task),
			     &interp_file) != 0 ||
		    vfs_read_file(vfs, interp_file.handle, interp_file.size, 0,
				  (unsigned char *)&interp_ehdr,
				  sizeof(interp_ehdr), (u64)io_buffer) != 0 ||
		    read_magic(interp_ehdr.ident) != ELF_MAGIC ||
		    interp_ehdr.ident[4] != ELFCLASS64 ||
		    interp_ehdr.ident[5] != ELFDATA2LSB ||
		    (interp_ehdr.type != ET_EXEC &&
		     interp_ehdr.type != ET_DYN) ||
		    interp_ehdr.machine != proc_elf_machine() ||
		    elf_phdr_table_size(&interp_ehdr, interp_file.size,
					&interp_phdr_bytes) != 0) {
			goto out;
		}
		interp_phdrs = (struct elf64_phdr *)bunix_alloc(interp_phdr_bytes);
		log_exec_load_stage(bunix_task_id(task), "read-interp-phdrs",
				    interp_path);
		if (interp_phdrs == 0 ||
		    vfs_read_file(vfs, interp_file.handle, interp_file.size,
				  interp_ehdr.phoff,
				  (unsigned char *)interp_phdrs,
				  interp_phdr_bytes,
				  (u64)io_buffer) != 0) {
			goto out;
		}
		interp_bias = interp_ehdr.type == ET_DYN ?
			      PROC_INTERP_LOAD_BIAS : 0;
		start_entry = interp_ehdr.entry + interp_bias;
	}
	timing_interp = bunix_timer_ticks();

	log_exec_load_stage(bunix_task_id(task), "clear", load_path);
	if (clear_existing && bunix_task_clear(task) != 0) {
		goto out;
	}

	log_exec_load_stage(bunix_task_id(task), "map", load_path);
	if (map_load_segments(vfs, &file, phdrs, ehdr.phnum, task,
			      load_bias, (u64)io_buffer) != 0 ||
	    (interp_file.handle != 0 &&
	     map_load_segments(vfs, &interp_file, interp_phdrs,
			       interp_ehdr.phnum, task,
			       interp_bias, (u64)io_buffer) != 0)) {
		goto out;
	}
	timing_mapped = bunix_timer_ticks();

	timing_relocated = bunix_timer_ticks();

	log_exec_load_stage(bunix_task_id(task), "stack-alloc", load_path);
	if (clear_existing &&
	    bunix_task_alloc_prot(task, USER_STACK_TOP - PROC_INIT_STACK_MAX,
				  PROC_INIT_STACK_MAX,
				  BUNIX_VM_PROT_READ |
				  BUNIX_VM_PROT_WRITE) != 0) {
		goto out;
	}

	log_exec_load_stage(bunix_task_id(task), "stack-build", load_path);
	if (build_initial_stack(task, execfn_path, &exec, interp_bias,
				strings, creds, handles, stack) != 0) {
		goto out;
	}
	timing_stacked = bunix_timer_ticks();

	log_exec_load_stage(bunix_task_id(task), "done", load_path);
	*entry = start_entry;
	result = 0;

out:
	log_exec_timing_line(load_path, result, timing_start, timing_headers,
			     timing_interp, timing_mapped, timing_relocated,
			     timing_stacked, bunix_timer_ticks());
	if (interp_file.handle != 0) {
		vfs_close(vfs, interp_file.handle);
	}
	if (file.handle != 0) {
		vfs_close(vfs, file.handle);
	}
	if (io_buffer > 0) {
		bunix_handle_close((u64)io_buffer);
	}
	bunix_free(interp_phdrs);
	bunix_free(phdrs);
	bunix_free(aux_phdrs);
	return result;
}

static long exec_path(u64 vfs, struct process *process,
		      const char *path, const char *task_name,
		      u64 linux_parent_pid, u64 login_uid, int set_login,
		      u64 session_id, int linux_personality,
		      const struct exec_strings *strings, u64 *linux_pid)
{
	struct bunix_launch_cap caps[6] = {
		{ PROC_HANDLE_CONSOLE, BUNIX_RIGHT_SEND, BUNIX_CAP_CONS },
		{ PROC_HANDLE_TIME, BUNIX_RIGHT_SEND, BUNIX_CAP_TIME },
		{ BUNIX_HANDLE_SELF, BUNIX_RIGHT_SEND, BUNIX_CAP_PROC },
		{ PROC_HANDLE_NAMES, BUNIX_RIGHT_SEND, BUNIX_CAP_NAME },
	};
	u64 cap_count = 4;
	struct exec_handles handles = { 0, 0, 0, 0, 0 };
	long task;
	u64 stack = 0;
	u64 start_entry = 0;
	long bunix_id = 0;

	task = bunix_task_create(task_name);
	if (task < 0) {
		return -1;
	}
	bunix_id = bunix_task_id((u64)task);
	if (bunix_id <= 0 ||
	    bunix_map_set(&processes_by_task_id, (u64)bunix_id,
			  (u64)process) != 0) {
		bunix_handle_close((u64)task);
		return -1;
	}
	process->task_id = (u64)bunix_id;
	process->task_handle = (u64)task;
	if (!linux_personality && register_user_process((u64)bunix_id) != 0) {
		return -1;
	}

	if (path_is_login(path) || path_is_login(task_name)) {
		caps[cap_count].handle = PROC_HANDLE_USER_MGMT;
		caps[cap_count].rights = BUNIX_RIGHT_SEND;
		caps[cap_count].tag = BUNIX_CAP_USRM;
		cap_count++;
		caps[cap_count].handle = PROC_HANDLE_LINUX_MGMT;
		caps[cap_count].rights = BUNIX_RIGHT_SEND;
		caps[cap_count].tag = BUNIX_CAP_LNXM;
		cap_count++;
	}

	for (u64 i = 0; i < cap_count; i++) {
		if (bunix_task_grant_tagged((u64)task, caps[i].handle,
					    caps[i].rights, caps[i].tag) <=
		    0) {
			return -1;
		}
	}
	if (exec_handles_from_task((u64)task, &handles) != 0) {
		return -1;
	}
	if (linux_personality) {
		if (register_linux_process((u64)bunix_id, process->pid,
					   linux_parent_pid, session_id,
					   linux_pid) != 0) {
			return -1;
		}
		if (set_login &&
		    apply_login_to_task((u64)bunix_id, login_uid) != 0) {
			return -1;
		}
	}
	if (load_task_image(vfs, (u64)task, 0, path, path, strings, 0,
			    &handles, &start_entry, &stack) != 0) {
		return -1;
	}
	const long started = bunix_task_start_at((u64)task, start_entry, stack);

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
	(void)bunix_map_remove(&processes_by_pid, process->pid);
	if (process->task_id != 0) {
		unregister_user_process(process->task_id);
		(void)bunix_map_remove(&processes_by_task_id,
				       process->task_id);
	}
	if (process->linux_pid != 0) {
		(void)bunix_map_remove(&processes_by_linux_pid,
				       process->linux_pid);
	}
	process->pid = 0;
	process->ppid = 0;
	process->task_id = 0;
	process->linux_pid = 0;
	process->task_handle = 0;
	process->status = 0;
	process->exited = 0;
	process->waiter = 0;
	process->cmd_len = 0;
	if (process->cmdline != 0) {
		bunix_free(process->cmdline);
	}
	process->cmdline = 0;
	bunix_free(process);
}

static struct process *process_find(u64 pid)
{
	return (struct process *)bunix_map_get(&processes_by_pid, pid);
}

static struct process *process_find_linux_pid(u64 linux_pid)
{
	if (linux_pid == 0) {
		return 0;
	}

	return (struct process *)
		bunix_map_get(&processes_by_linux_pid, linux_pid);
}

static struct process *process_find_task_id(u64 task_id)
{
	if (task_id == 0) {
		return 0;
	}

	return (struct process *)
		bunix_map_get(&processes_by_task_id, task_id);
}

static int linux_exit_pending(u64 linux_pid)
{
	return linux_pid != 0 &&
	       bunix_map_get(&pending_linux_exits, linux_pid) != 0;
}

static void note_pending_linux_exit(u64 linux_pid)
{
	if (linux_pid != 0) {
		(void)bunix_map_set(&pending_linux_exits, linux_pid, 1);
	}
}

static void clear_pending_linux_exit(u64 linux_pid)
{
	if (linux_pid != 0) {
		(void)bunix_map_remove(&pending_linux_exits, linux_pid);
	}
}

static struct process *process_at(u64 index)
{
	return (struct process *)bunix_map_at(&processes_by_pid, index);
}

static struct process *process_alloc(void)
{
	return (struct process *)bunix_calloc(1, sizeof(struct process));
}

static long spawn_process(const char *path, u64 login_uid, int set_login,
			  u64 session_id, int linux_personality,
			  const struct exec_strings *strings, u64 *pid)
{
	u64 vfs;
	u64 linux_pid = 0;
	u64 linux_parent_pid = 0;
	struct process *process;
	struct exec_strings default_strings = { 0, 0, 0, 0 };
	const struct exec_strings *exec_strings = strings;
	int linux_path;

	if (pid == 0) {
		return -1;
	}
	if (exec_strings == 0) {
		if (exec_strings_default(&default_strings, path) != 0) {
			return -1;
		}
		exec_strings = &default_strings;
	}

	process = process_alloc();
	if (process == 0) {
		exec_strings_free(&default_strings);
		return -1;
	}

	vfs = resolve_service(BUNIX_SERVICE_VFS, BUNIX_RIGHT_SEND);
	if (vfs == 0) {
		bunix_free(process);
		exec_strings_free(&default_strings);
		return -1;
	}

	process->pid = next_pid++;
	process->ppid = 0;
	process->task_id = 0;
	process->linux_pid = 0;
	process->task_handle = 0;
	process->status = 0;
	process->exited = 0;
	process->waiter = 0;
	if (process_set_cmdline(process, path, str_len(path)) != 0) {
		bunix_free(process);
		exec_strings_free(&default_strings);
		return -1;
	}
	*pid = process->pid;
	if (bunix_map_set(&processes_by_pid, process->pid,
			  (u64)process) != 0) {
		process_reset(process);
		*pid = 0;
		exec_strings_free(&default_strings);
		return -1;
	}

	linux_path = linux_personality || is_linux_path(path);
	if (linux_path && first_linux_pid != 0) {
		struct process *parent = process_find_linux_pid(first_linux_pid);

		linux_parent_pid = first_linux_pid;
		process->ppid = parent == 0 ? 0 : parent->pid;
	}

	if (exec_path(vfs, process, path, task_name_for_path(path),
		      linux_parent_pid, login_uid, set_login, session_id,
		      linux_path, exec_strings, &linux_pid) != 0) {
		process_reset(process);
		*pid = 0;
		exec_strings_free(&default_strings);
		return -1;
	}

	process->linux_pid = linux_pid;
	if (linux_pid != 0 &&
	    bunix_map_set(&processes_by_linux_pid, linux_pid,
			  (u64)process) != 0) {
		process_reset(process);
		*pid = 0;
		exec_strings_free(&default_strings);
		return -1;
	}
	if (linux_pid != 0 && first_linux_pid == 0) {
		first_linux_pid = linux_pid;
	}
	exec_strings_free(&default_strings);
	return 0;
}

static long register_linux_child_process(u64 linux_pid, u64 task_id, u64 ppid,
					 u64 *pid)
{
	struct process *process;
	struct process *parent = 0;

	if (linux_pid == 0 || task_id == 0 || pid == 0 ||
	    process_find_linux_pid(linux_pid) != 0) {
		return -1;
	}
	if (linux_exit_pending(linux_pid)) {
		clear_pending_linux_exit(linux_pid);
		*pid = 0;
		return 0;
	}

	process = process_alloc();
	if (process == 0) {
		return -1;
	}
	if (ppid != 0) {
		parent = process_find_linux_pid(ppid);
	}
	process->pid = next_pid++;
	process->ppid = parent == 0 ? 0 : parent->pid;
	process->task_id = task_id;
	process->linux_pid = linux_pid;
	process->task_handle = 0;
	process->status = 0;
	process->exited = 0;
	process->waiter = 0;
	if (process_set_cmdline(process, "/bin/process", 12) != 0) {
		bunix_free(process);
		return -1;
	}
	if (bunix_map_set(&processes_by_pid, process->pid,
			  (u64)process) != 0 ||
	    bunix_map_set(&processes_by_task_id, task_id,
			  (u64)process) != 0 ||
	    bunix_map_set(&processes_by_linux_pid, linux_pid,
			  (u64)process) != 0) {
		process_reset(process);
		return -1;
	}
	*pid = process->pid;
	return 0;
}

static int task_handle_matches_sender(u64 task_handle, u64 sender)
{
	const long id = bunix_task_id(task_handle);

	return sender != 0 && id > 0 && (u64)id == sender;
}

static u64 exec_replace_begin(u64 task_handle, u64 sender)
{
	struct pending_exec_replace *pending;
	u64 token;

	if (task_handle == 0) {
		return 0;
	}

	pending = (struct pending_exec_replace *)
		bunix_calloc(1, sizeof(*pending));
	if (pending == 0) {
		return 0;
	}

	token = next_exec_replace_token++;
	if (token == 0) {
		token = next_exec_replace_token++;
	}
	pending->token = token;
	pending->task_handle = task_handle;
	pending->sender = sender;
	if (bunix_map_set(&pending_exec_replaces, token, (u64)pending) != 0) {
		bunix_free(pending);
		return 0;
	}

	return token;
}

static struct pending_exec_replace *exec_replace_take(u64 token)
{
	struct pending_exec_replace *pending =
		(struct pending_exec_replace *)
			bunix_map_get(&pending_exec_replaces, token);

	if (pending != 0) {
		(void)bunix_map_remove(&pending_exec_replaces, token);
	}
	return pending;
}

static struct pending_exec_replace *exec_replace_find(u64 token)
{
	return (struct pending_exec_replace *)
		bunix_map_get(&pending_exec_replaces, token);
}

static void exec_replace_free(struct pending_exec_replace *pending)
{
	if (pending == 0) {
		return;
	}
	if (pending->task_handle != 0) {
		bunix_handle_close(pending->task_handle);
	}
	bunix_free(pending);
}

static long exec_replace_from_message(u64 vfs, const struct bunix_msg *message,
				      u64 *entry, u64 *stack)
{
	struct pending_exec_replace *pending;
	struct exec_strings strings = { 0, 0, 0, 0 };
	struct exec_credentials creds = { 0, 0, 0, 0, 0 };
	struct exec_handles handles = { 0, 0, 0, 0, 0 };
	char *load_path = 0;
	char *execfn_path = 0;
	long result = -1;

	if (message == 0 || entry == 0 || stack == 0 ||
	    message->words[0] == 0) {
		return -1;
	}

	pending = exec_replace_take(message->words[0]);
	if (pending == 0) {
		return -1;
	}

	if (exec_strings_from_replace_buffer(message, &load_path, &execfn_path,
					     &creds,
					     &strings) == 0 &&
	    exec_handles_from_task(pending->task_handle, &handles) == 0) {
		if (path_is_login(load_path) &&
		    grant_login_management_caps(pending->task_handle) != 0) {
			goto out;
		}
		result = load_task_image(vfs, pending->task_handle, 1,
					 load_path, execfn_path, &strings,
					 &creds,
					 &handles,
					 entry, stack);
	}

out:
	if (load_path != 0) {
		bunix_free(load_path);
	}
	if (execfn_path != 0) {
		bunix_free(execfn_path);
	}
	exec_strings_free(&strings);
	exec_replace_free(pending);
	return result;
}

int main(void)
{
	struct bunix_msg message;

	bunix_map_init(&processes_by_pid);
	bunix_map_init(&processes_by_task_id);
	bunix_map_init(&processes_by_linux_pid);
	bunix_map_init(&pending_linux_exits);
	bunix_map_init(&pending_exec_replaces);
	bunix_tree_init(&exec_registry);
	bunix_console_log(proc_online, sizeof(proc_online) - 1);
	if (register_service(BUNIX_SERVICE_PROC, BUNIX_HANDLE_SELF) != 0) {
		bunix_console_log(proc_register_failed,
				    sizeof(proc_register_failed) - 1);
		return 1;
	}
	bunix_console_log(proc_ready, sizeof(proc_ready) - 1);

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
		int management = 0;

		if (recv_proc_message(&message, &management) != 0 ||
		    message.protocol != BUNIX_PROTO_PROC) {
			continue;
		}

		reply.type = message.type;
		switch (message.type) {
		case BUNIX_PROC_SPAWN_BUFFER: {
			char *path = 0;
			struct exec_strings strings = { 0, 0, 0, 0 };
			u64 pid = 0;
			const u64 flags = message.words[3] &
					  PROC_SPAWN_FLAGS_MASK;
			const u64 session_id = message.words[3] >>
					       PROC_SPAWN_SESSION_SHIFT;

			if (!management && message.sender != 0) {
				reply.words[0] = (u64)-1;
				if (message.cap != 0) {
					bunix_handle_close(message.cap);
				}
				break;
			}
			if (exec_strings_from_spawn_buffer(&message, &path,
							   &strings) == 0 &&
			    spawn_process(path, 0,
					  (flags & PROC_SPAWN_SET_LOGIN) != 0,
					  session_id,
					  (flags & PROC_SPAWN_LINUX) != 0,
					  &strings, &pid) == 0) {
				reply.words[0] = 0;
				reply.words[1] = pid;
				log_exec_line(path);
				log_pid_line("proc: spawned pid=", pid);
			} else {
				reply.words[0] = (u64)-1;
				bunix_console_log("proc: spawn failed\n",
						  sizeof("proc: spawn failed\n") - 1);
			}
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			if (path != 0) {
				bunix_free(path);
			}
			exec_strings_free(&strings);
			break;
		}
		case BUNIX_PROC_REGISTER_EXEC:
			if (!management && message.sender != 0) {
				reply.words[0] = (u64)-1;
				if (message.cap != 0) {
					bunix_handle_close(message.cap);
				}
				break;
			}
			reply.words[0] = (u64)register_exec_from_message(&message);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		case BUNIX_PROC_EXEC_REPLACE_TASK: {
			u64 token = 0;

			if (!management &&
			    (message.cap == 0 ||
			     (message.cap_rights & BUNIX_RIGHT_SEND) == 0 ||
			     !task_handle_matches_sender(message.cap,
							 message.sender))) {
				reply.words[0] = (u64)-1;
				if (message.cap != 0) {
					bunix_handle_close(message.cap);
				}
				break;
			}
			if (message.cap != 0 &&
			    (message.cap_rights & BUNIX_RIGHT_SEND) != 0) {
				token = exec_replace_begin(message.cap,
							   message.sender);
			}
			if (token != 0) {
				message.cap = 0;
				reply.words[0] = 0;
				reply.words[1] = token;
			} else {
				reply.words[0] = (u64)-1;
			}
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		}
		case BUNIX_PROC_EXEC_REPLACE_BUFFER: {
			u64 entry = 0;
			u64 stack = 0;
			struct pending_exec_replace *pending =
				exec_replace_find(message.words[0]);
			u64 vfs = 0;

			if (!management &&
			    (pending == 0 || pending->sender != message.sender)) {
				reply.words[0] = (u64)-1;
				if (message.cap != 0) {
					bunix_handle_close(message.cap);
				}
				break;
			}
			vfs = resolve_service(BUNIX_SERVICE_VFS,
					      BUNIX_RIGHT_SEND);
			if (vfs != 0 &&
			    exec_replace_from_message(vfs, &message, &entry,
						      &stack) == 0) {
				reply.words[0] = 0;
				reply.words[1] = entry;
				reply.words[2] = stack;
			} else {
				reply.words[0] = (u64)-1;
			}
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			if (vfs != 0) {
				bunix_handle_close(vfs);
			}
			break;
		}
		case BUNIX_PROC_WAIT: {
			struct process *process = process_find(message.words[0]);

			if (!management && message.sender != 0) {
				reply.words[0] = (u64)-1;
				break;
			}
			if (process == 0) {
				reply.words[0] = (u64)-1;
			} else if (process->exited) {
				reply.words[0] = 0;
				reply.words[1] = process->pid;
				reply.words[2] = process->status;
				log_exit_line("proc: wait pid=", process->pid,
					      process->status);
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
			const int self_exit =
				process != 0 &&
				message.sender != 0 &&
				process->task_id == message.sender;

			if (!management && !self_exit) {
				reply.words[0] = (u64)-1;
				break;
			}
			if (process != 0) {
				if (management && message.words[3] != 0 &&
				    process->task_handle != 0) {
					(void)bunix_task_kill(process->task_handle);
				}
				process->status = message.words[1];
				process->exited = 1;
				reply.words[0] = 0;
				log_exit_line("proc: exited pid=", process->pid,
					      process->status);
				if (process->waiter != 0) {
					reply_status(process->waiter,
						     process->pid,
						     process->status);
					process->waiter = 0;
					log_exit_line("proc: wait pid=",
						      process->pid,
						      process->status);
					process_reset(process);
				} else if (message.words[2] != 0 &&
					   process->linux_pid != first_linux_pid) {
					process_reset(process);
				}
			} else {
				if (message.words[2] != 0) {
					note_pending_linux_exit(message.words[0]);
					reply.words[0] = 0;
				} else {
					reply.words[0] = (u64)-1;
				}
			}
			break;
		}
		case BUNIX_PROC_SELF: {
			struct process *process =
				process_find_task_id(message.sender);

			if (process == 0) {
				reply.words[0] = (u64)-1;
			} else {
				reply.words[0] = 0;
				reply.words[1] = process->pid;
				reply.words[2] = process->task_id;
				reply.words[3] = process->linux_pid;
			}
			break;
		}
		case BUNIX_PROC_INFO: {
			struct process *process = process_find(message.words[0]);

			if (process == 0) {
				reply.words[0] = (u64)-1;
			} else {
				reply.words[0] = 0;
				reply.words[1] = process->pid;
				reply.words[2] = process->task_id;
				reply.words[3] = process->linux_pid;
			}
			break;
		}
		case BUNIX_PROC_INFO_BY_TASK: {
			struct process *process =
				process_find_task_id(message.words[0]);

			if (process == 0) {
				reply.words[0] = (u64)-1;
			} else {
				reply.words[0] = 0;
				reply.words[1] = process->pid;
				reply.words[2] = process->task_id;
				reply.words[3] = process->linux_pid;
			}
			break;
		}
		case BUNIX_PROC_INFO_BY_LINUX_PID: {
			struct process *process =
				process_find_linux_pid(message.words[0]);

			if (process == 0) {
				reply.words[0] = (u64)-1;
			} else {
				reply.words[0] = 0;
				reply.words[1] = process->pid;
				reply.words[2] = process->task_id;
				reply.words[3] = process->linux_pid;
			}
			break;
		}
		case BUNIX_PROC_AT: {
			struct process *process = process_at(message.words[0]);

			if (process == 0) {
				reply.words[0] = (u64)-1;
			} else {
				reply.words[0] = 0;
				reply.words[1] = process->pid;
				reply.words[2] = process->task_id;
				reply.words[3] = process->linux_pid;
			}
			break;
		}
		case BUNIX_PROC_CMDLINE_BUFFER: {
			struct process *process = process_find(message.words[0]);
			const u64 buffer = message.cap;
			const u64 offset = message.words[2];
			u64 len = message.words[3];

			if (process == 0 || buffer == 0 ||
			    process->cmdline == 0 ||
			    offset > process->cmd_len) {
				reply.words[0] = (u64)-1;
			} else {
				const u64 remaining = process->cmd_len - offset;

				if (len > remaining) {
					len = remaining;
				}
				if (len != 0 &&
				    bunix_buffer_write(buffer, 0,
						       process->cmdline + offset,
						       len) != 0) {
					reply.words[0] = (u64)-1;
				} else {
					reply.words[0] = 0;
					reply.words[1] = process->cmd_len;
					reply.words[2] = len;
				}
			}
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		}
		case BUNIX_PROC_DETAILS: {
			struct process *process = process_find(message.words[0]);

			if (process == 0) {
				reply.words[0] = (u64)-1;
			} else {
				reply.words[0] = 0;
				reply.words[1] = process->ppid;
				reply.words[2] = process->exited ? 'Z' : 'S';
				reply.words[3] = process->status;
			}
			break;
		}
		case BUNIX_PROC_SET_CMDLINE_BUFFER: {
			struct process *process =
				process_find_linux_pid(message.words[0]);
			const u64 len = message.words[2];
			const int self_cmdline =
				message.sender != 0 &&
				message.words[1] == message.sender;

			if (!management && !self_cmdline) {
				reply.words[0] = (u64)-1;
				if (message.cap != 0) {
					bunix_handle_close(message.cap);
				}
				break;
			}
			if (process == 0) {
				if (linux_exit_pending(message.words[0])) {
					reply.words[0] = 0;
					reply.words[1] = 0;
				} else {
					u64 pid = 0;

					reply.words[0] =
						(u64)register_linux_child_process(
							message.words[0],
							message.words[1], 0, &pid);
					reply.words[1] = pid;
					process = pid == 0 ? 0 : process_find(pid);
					if (reply.words[0] == 0 &&
					    process != 0 &&
					    process_set_cmdline_from_buffer(process,
									    message.cap,
									    len) != 0) {
						reply.words[0] = (u64)-1;
					}
				}
			} else if (process_set_cmdline_from_buffer(process,
								   message.cap,
								   len) == 0) {
				reply.words[0] = 0;
			} else {
				reply.words[0] = (u64)-1;
			}
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			break;
		}
		case BUNIX_PROC_REGISTER_LINUX: {
			u64 pid = 0;

			if (!management) {
				reply.words[0] = (u64)-1;
				break;
			}
			reply.words[0] =
				(u64)register_linux_child_process(message.words[0],
								  message.words[1],
								  message.words[2],
								  &pid);
			reply.words[1] = pid;
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
