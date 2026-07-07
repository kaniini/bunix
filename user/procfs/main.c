#include <bunix/libbunix.h>
#include <bunix/id_table.h>
#include <bunix/tree.h>

enum {
	PROCFS_HANDLE_NAMES = 3,
	PROCFS_MAX_PATH = 4096,
	PROCFS_KIND_PROC = 1,
	PROCFS_KIND_KTHREADS = 2,
	PROCFS_KIND_UPTIME = 3,
	PROCFS_KIND_MEMINFO = 4,
	PROCFS_KIND_MOUNTS = 5,
	PROCFS_KIND_SELF = 6,
	PROCFS_KIND_PID = 7,
	PROCFS_KIND_PID_STAT = 8,
	PROCFS_KIND_PID_STATUS = 9,
	PROCFS_KIND_PID_CMDLINE = 10,
	PROCFS_KIND_PID_FD = 11,
	PROCFS_KIND_PID_FD_ENTRY = 12,
	PROCFS_KIND_STAT = 13,
	PROCFS_KIND_LOADAVG = 14,
	PROCFS_KIND_PID_STATM = 15,
	PROCFS_KIND_IPC = 16,
	PROCFS_KIND_PID_EXE = 17,
	PROCFS_KIND_FILESYSTEMS = 18,
	PROCFS_KIND_CPUINFO = 19,
	PROCFS_KIND_CMDLINE = 20,
	PROCFS_KIND_DEVICES = 21,
	PROCFS_KIND_MODULES = 22,
	PROCFS_KIND_PID_MOUNTS = 23,
	PROCFS_KIND_PID_MOUNTINFO = 24,
	PROCFS_KIND_PID_CGROUP = 25,
	PROCFS_KIND_NET = 26,
	PROCFS_KIND_NET_DEV = 27,
	PROCFS_KIND_NET_ROUTE = 28,
	PROCFS_KIND_NET_SOCKSTAT = 29,
	PROCFS_KIND_NET_UDP = 30,
	PROCFS_KIND_NET_UDP6 = 31,
	PROCFS_KIND_NET_TCP = 32,
	PROCFS_KIND_NET_TCP6 = 33,
	PROCFS_KIND_SCHED = 34,
	PROCFS_KIND_NET_CONFIG = 35,
};

static struct bunix_id_table open_files;
static struct bunix_tree mounts;
static char *text;
static u64 text_capacity;
static int text_failed;
static u64 proc_handle;
static u64 net_handle;

struct procfs_mount {
	struct bunix_tree_node node;
	char *path;
	u64 fstype;
};

struct proc_info {
	u64 pid;
	u64 task_id;
	u64 linux_pid;
};

struct proc_details {
	u64 ppid;
	char state;
	u64 status;
};

static u64 build_kthreads(void);
static u64 build_file_text(u64 file);
static const char *pid_cmdline(u64 pid);

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

	if (bunix_ipc_call(PROCFS_HANDLE_NAMES, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return 0;
	}
	return reply.cap;
}

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

	if (bunix_ipc_call(PROCFS_HANDLE_NAMES, &request, &reply) != 0) {
		return -1;
	}
	return reply.words[0] == 0 ? 0 : -1;
}

static int str_eq(const char *left, const char *right)
{
	while (*left != '\0' && *right != '\0') {
		if (*left++ != *right++) {
			return 0;
		}
	}
	return *left == '\0' && *right == '\0';
}

static u64 str_len(const char *text)
{
	u64 len = 0;

	while (text[len] != '\0') {
		len++;
	}
	return len;
}

static char *str_dup(const char *text)
{
	const u64 len = str_len(text);
	char *copy = (char *)bunix_alloc(len + 1);

	if (copy == 0) {
		return 0;
	}
	for (u64 i = 0; i <= len; i++) {
		copy[i] = text[i];
	}
	return copy;
}

static int path_has_prefix(const char *path, const char *prefix)
{
	u64 i = 0;

	while (prefix[i] != '\0') {
		if (path[i] != prefix[i]) {
			return 0;
		}
		i++;
	}
	return 1;
}

static u64 make_file(u64 kind, u64 arg)
{
	return kind | (arg << 8);
}

static u64 file_kind(u64 file)
{
	return file & 0xff;
}

static u64 file_arg(u64 file)
{
	return file >> 8;
}

static int parse_u64_component(const char **cursor, u64 *value)
{
	const char *p = *cursor;
	u64 out = 0;

	if (*p < '0' || *p > '9') {
		return -1;
	}
	while (*p >= '0' && *p <= '9') {
		out = out * 10 + (u64)(*p - '0');
		p++;
	}
	*cursor = p;
	*value = out;
	return 0;
}

static int read_path_buffer(u64 buffer, u64 offset, u64 len, char *path)
{
	if (buffer == 0 || len == 0 || len > PROCFS_MAX_PATH) {
		return -1;
	}
	for (u64 i = 0; i < PROCFS_MAX_PATH; i++) {
		path[i] = '\0';
	}
	if (bunix_buffer_read(buffer, offset, path, len) != 0 ||
	    path[len - 1] != '\0') {
		return -1;
	}
	return 0;
}

static u64 get_proc_handle(void)
{
	if (proc_handle == 0) {
		proc_handle = resolve_service(BUNIX_SERVICE_PROC,
					      BUNIX_RIGHT_SEND);
	}
	return proc_handle;
}

static u64 get_net_handle(void)
{
	if (net_handle == 0) {
		net_handle = resolve_service(BUNIX_SERVICE_NET,
					     BUNIX_RIGHT_SEND);
	}
	return net_handle;
}

static int proc_call(u64 type, u64 arg, struct bunix_msg *reply)
{
	const u64 proc = get_proc_handle();
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_PROC,
		.type = type,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { arg, 0, 0, 0 },
	};

	if (proc == 0 || bunix_ipc_call(proc, &request, reply) != 0 ||
	    reply->words[0] != 0) {
		return -1;
	}
	return 0;
}

static int proc_call_buffer(u64 type, u64 arg0, u64 arg1, u64 arg2,
			    u64 arg3, u64 buffer, struct bunix_msg *reply)
{
	const u64 proc = get_proc_handle();
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_PROC,
		.type = type,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_SEND | BUNIX_RIGHT_RECV,
		.reply = 0,
		.cap = buffer,
		.words = { arg0, arg1, arg2, arg3 },
	};

	if (proc == 0 || buffer == 0 ||
	    bunix_ipc_call(proc, &request, reply) != 0 ||
	    reply->words[0] != 0) {
		return -1;
	}
	return 0;
}

static int net_call(u64 type, u64 arg, struct bunix_msg *reply)
{
	const u64 net = get_net_handle();
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_NET,
		.type = type,
		.sender = 0,
		.cap_rights = 0,
		.reply = 0,
		.cap = 0,
		.words = { arg, 0, 0, 0 },
	};

	if (net == 0 || bunix_ipc_call(net, &request, reply) != 0 ||
	    reply->words[0] != 0) {
		return -1;
	}
	return 0;
}

static int net_call_buffer(u64 type, u64 arg, u64 buffer,
			   struct bunix_msg *reply)
{
	const u64 net = get_net_handle();
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_NET,
		.type = type,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_SEND,
		.reply = 0,
		.cap = buffer,
		.words = { arg, 0, 0, 0 },
	};

	if (net == 0 || buffer == 0 ||
	    bunix_ipc_call(net, &request, reply) != 0 ||
	    reply->words[0] != 0) {
		return -1;
	}
	return 0;
}

static int proc_info(u64 pid, struct proc_info *info)
{
	struct bunix_msg reply;

	if (pid == 0 || info == 0 ||
	    proc_call(BUNIX_PROC_INFO, pid, &reply) != 0) {
		return -1;
	}
	info->pid = reply.words[1];
	info->task_id = reply.words[2];
	info->linux_pid = reply.words[3];
	return 0;
}

static int proc_info_by_task(u64 task, struct proc_info *info)
{
	struct bunix_msg reply;

	if (task == 0 || info == 0 ||
	    proc_call(BUNIX_PROC_INFO_BY_TASK, task, &reply) != 0) {
		return -1;
	}
	info->pid = reply.words[1];
	info->task_id = reply.words[2];
	info->linux_pid = reply.words[3];
	return 0;
}

static int proc_info_by_linux_pid(u64 linux_pid, struct proc_info *info)
{
	struct bunix_msg reply;

	if (linux_pid == 0 || info == 0 ||
	    proc_call(BUNIX_PROC_INFO_BY_LINUX_PID, linux_pid, &reply) != 0) {
		return -1;
	}
	info->pid = reply.words[1];
	info->task_id = reply.words[2];
	info->linux_pid = reply.words[3];
	return 0;
}

static int proc_at(u64 index, struct proc_info *info)
{
	struct bunix_msg reply;

	if (info == 0 || proc_call(BUNIX_PROC_AT, index, &reply) != 0) {
		return -1;
	}
	info->pid = reply.words[1];
	info->task_id = reply.words[2];
	info->linux_pid = reply.words[3];
	return 0;
}

static int proc_cmdline_bytes(u64 pid, char *out, u64 out_size, u64 *len_out)
{
	struct bunix_msg reply;
	u64 len;
	long buffer;

	if (out == 0 || out_size == 0 || len_out == 0) {
		return -1;
	}
	for (u64 i = 0; i < out_size; i++) {
		out[i] = '\0';
	}
	*len_out = 0;
	buffer = bunix_buffer_create(out_size);
	if (buffer > 0 &&
	    proc_call_buffer(BUNIX_PROC_CMDLINE_BUFFER, pid, 0, 0,
			     out_size - 1, (u64)buffer, &reply) == 0) {
		len = reply.words[2];
		if (len >= out_size) {
			len = out_size - 1;
		}
		if (len != 0 &&
		    bunix_buffer_read((u64)buffer, 0, out, len) != 0) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
		*len_out = len;
		out[len] = '\0';
		bunix_handle_close((u64)buffer);
		return 0;
	}
	if (buffer > 0) {
		bunix_handle_close((u64)buffer);
	}
	return -1;
}

static int proc_cmdline(u64 pid, char *out, u64 out_size)
{
	u64 len = 0;

	return proc_cmdline_bytes(pid, out, out_size, &len);
}

static int proc_details(u64 pid, struct proc_details *details)
{
	struct bunix_msg reply;

	if (pid == 0 || details == 0 ||
	    proc_call(BUNIX_PROC_DETAILS, pid, &reply) != 0) {
		return -1;
	}
	details->ppid = reply.words[1];
	details->state = (char)reply.words[2];
	details->status = reply.words[3];
	return 0;
}

static u64 proc_self_pid(u64 caller_task)
{
	struct proc_info info;

	if (proc_info_by_task(caller_task, &info) != 0) {
		return 0;
	}
	return info.pid;
}

static u64 proc_path_pid(u64 pid, u64 caller_task)
{
	struct proc_info info;

	if (pid == 0) {
		pid = proc_self_pid(caller_task);
	}
	if (proc_info(pid, &info) == 0) {
		return pid;
	}
	if (proc_info_by_linux_pid(pid, &info) == 0) {
		return info.pid;
	}
	return 0;
}

static long mount_procfs(const char *path)
{
	const u64 vfs = resolve_service(BUNIX_SERVICE_VFS,
					BUNIX_RIGHT_SEND);
	struct bunix_msg reply;

	if (vfs == 0) {
		return -1;
	}
	if (bunix_ipc_call_path(vfs, BUNIX_PROTO_VFS,
				BUNIX_VFS_MOUNT_BUFFER, path,
				BUNIX_SERVICE_PROCFS, 0, 0, &reply) != 0 ||
	    reply.words[0] != 0) {
		return -1;
	}
	return 0;
}

static u64 file_for_path(const char *path, u64 caller_task)
{
	if (str_eq(path, "/proc")) {
		return make_file(PROCFS_KIND_PROC, 0);
	}
	if (str_eq(path, "/proc/kthreads")) {
		return make_file(PROCFS_KIND_KTHREADS, 0);
	}
	if (str_eq(path, "/proc/uptime")) {
		return make_file(PROCFS_KIND_UPTIME, 0);
	}
	if (str_eq(path, "/proc/stat")) {
		return make_file(PROCFS_KIND_STAT, 0);
	}
	if (str_eq(path, "/proc/ipc")) {
		return make_file(PROCFS_KIND_IPC, 0);
	}
	if (str_eq(path, "/proc/sched")) {
		return make_file(PROCFS_KIND_SCHED, 0);
	}
	if (str_eq(path, "/proc/loadavg")) {
		return make_file(PROCFS_KIND_LOADAVG, 0);
	}
	if (str_eq(path, "/proc/meminfo")) {
		return make_file(PROCFS_KIND_MEMINFO, 0);
	}
	if (str_eq(path, "/proc/filesystems")) {
		return make_file(PROCFS_KIND_FILESYSTEMS, 0);
	}
	if (str_eq(path, "/proc/cpuinfo")) {
		return make_file(PROCFS_KIND_CPUINFO, 0);
	}
	if (str_eq(path, "/proc/cmdline")) {
		return make_file(PROCFS_KIND_CMDLINE, 0);
	}
	if (str_eq(path, "/proc/devices")) {
		return make_file(PROCFS_KIND_DEVICES, 0);
	}
	if (str_eq(path, "/proc/modules")) {
		return make_file(PROCFS_KIND_MODULES, 0);
	}
	if (str_eq(path, "/proc/mounts")) {
		return make_file(PROCFS_KIND_MOUNTS, 0);
	}
	if (str_eq(path, "/proc/net")) {
		return make_file(PROCFS_KIND_NET, 0);
	}
	if (str_eq(path, "/proc/net/dev")) {
		return make_file(PROCFS_KIND_NET_DEV, 0);
	}
	if (str_eq(path, "/proc/net/route")) {
		return make_file(PROCFS_KIND_NET_ROUTE, 0);
	}
	if (str_eq(path, "/proc/net/sockstat")) {
		return make_file(PROCFS_KIND_NET_SOCKSTAT, 0);
	}
	if (str_eq(path, "/proc/net/udp")) {
		return make_file(PROCFS_KIND_NET_UDP, 0);
	}
	if (str_eq(path, "/proc/net/udp6")) {
		return make_file(PROCFS_KIND_NET_UDP6, 0);
	}
	if (str_eq(path, "/proc/net/tcp")) {
		return make_file(PROCFS_KIND_NET_TCP, 0);
	}
	if (str_eq(path, "/proc/net/tcp6")) {
		return make_file(PROCFS_KIND_NET_TCP6, 0);
	}
	if (str_eq(path, "/proc/net/config")) {
		return make_file(PROCFS_KIND_NET_CONFIG, 0);
	}
	if (str_eq(path, "/proc/self")) {
		const u64 pid = proc_path_pid(0, caller_task);

		return pid == 0 ? 0 : make_file(PROCFS_KIND_SELF, pid);
	}
	if (str_eq(path, "/proc/self/stat")) {
		const u64 pid = proc_path_pid(0, caller_task);

		return pid == 0 ? 0 : make_file(PROCFS_KIND_PID_STAT, pid);
	}
	if (str_eq(path, "/proc/self/status")) {
		const u64 pid = proc_path_pid(0, caller_task);

		return pid == 0 ? 0 : make_file(PROCFS_KIND_PID_STATUS, pid);
	}
	if (str_eq(path, "/proc/self/cmdline")) {
		const u64 pid = proc_path_pid(0, caller_task);

		return pid == 0 ? 0 : make_file(PROCFS_KIND_PID_CMDLINE, pid);
	}
	if (str_eq(path, "/proc/self/statm")) {
		const u64 pid = proc_path_pid(0, caller_task);

		return pid == 0 ? 0 : make_file(PROCFS_KIND_PID_STATM, pid);
	}
	if (str_eq(path, "/proc/self/mounts")) {
		const u64 pid = proc_path_pid(0, caller_task);

		return pid == 0 ? 0 : make_file(PROCFS_KIND_PID_MOUNTS, pid);
	}
	if (str_eq(path, "/proc/self/mountinfo")) {
		const u64 pid = proc_path_pid(0, caller_task);

		return pid == 0 ? 0 : make_file(PROCFS_KIND_PID_MOUNTINFO, pid);
	}
	if (str_eq(path, "/proc/self/cgroup")) {
		const u64 pid = proc_path_pid(0, caller_task);

		return pid == 0 ? 0 : make_file(PROCFS_KIND_PID_CGROUP, pid);
	}
	if (str_eq(path, "/proc/self/exe")) {
		const u64 pid = proc_path_pid(0, caller_task);

		return pid == 0 ? 0 : make_file(PROCFS_KIND_PID_EXE, pid);
	}
	if (str_eq(path, "/proc/self/fd")) {
		const u64 pid = proc_path_pid(0, caller_task);

		return pid == 0 ? 0 : make_file(PROCFS_KIND_PID_FD, pid);
	}
	if (path_has_prefix(path, "/proc/self/fd/")) {
		const char *cursor = path + 14;
		u64 fd;
		const u64 pid = proc_path_pid(0, caller_task);

		if (pid != 0 &&
		    parse_u64_component(&cursor, &fd) == 0 && *cursor == '\0') {
			return make_file(PROCFS_KIND_PID_FD_ENTRY, fd);
		}
	}
	if (path_has_prefix(path, "/proc/")) {
		const char *cursor = path + 6;
		u64 pid;

		if (parse_u64_component(&cursor, &pid) != 0 || pid == 0) {
			return 0;
		}
		pid = proc_path_pid(pid, caller_task);
		if (pid == 0) {
			return 0;
		}
		if (*cursor == '\0') {
			return make_file(PROCFS_KIND_PID, pid);
		}
		if (str_eq(cursor, "/stat")) {
			return make_file(PROCFS_KIND_PID_STAT, pid);
		}
		if (str_eq(cursor, "/status")) {
			return make_file(PROCFS_KIND_PID_STATUS, pid);
		}
		if (str_eq(cursor, "/cmdline")) {
			return make_file(PROCFS_KIND_PID_CMDLINE, pid);
		}
		if (str_eq(cursor, "/statm")) {
			return make_file(PROCFS_KIND_PID_STATM, pid);
		}
		if (str_eq(cursor, "/mounts")) {
			return make_file(PROCFS_KIND_PID_MOUNTS, pid);
		}
		if (str_eq(cursor, "/mountinfo")) {
			return make_file(PROCFS_KIND_PID_MOUNTINFO, pid);
		}
		if (str_eq(cursor, "/cgroup")) {
			return make_file(PROCFS_KIND_PID_CGROUP, pid);
		}
		if (str_eq(cursor, "/exe")) {
			return make_file(PROCFS_KIND_PID_EXE, pid);
		}
		if (str_eq(cursor, "/fd")) {
			return make_file(PROCFS_KIND_PID_FD, pid);
		}
		if (path_has_prefix(cursor, "/fd/")) {
			cursor += 4;
			if (parse_u64_component(&cursor, &pid) == 0 &&
			    *cursor == '\0') {
				return make_file(PROCFS_KIND_PID_FD_ENTRY, pid);
			}
		}
	}
	return 0;
}

static u64 file_size(u64 file)
{
	u64 size;

	if (file_kind(file) == PROCFS_KIND_PID_EXE) {
		return str_len(pid_cmdline(file_arg(file)));
	}
	if (file_kind(file) != PROCFS_KIND_PROC &&
	    file_kind(file) != PROCFS_KIND_SELF &&
	    file_kind(file) != PROCFS_KIND_PID &&
	    file_kind(file) != PROCFS_KIND_PID_FD) {
		size = build_file_text(file);
		bunix_free(text);
		text = 0;
		text_capacity = 0;
		text_failed = 0;
		return size;
	}
	return 0;
}

static void text_release(void)
{
	bunix_free(text);
	text = 0;
	text_capacity = 0;
	text_failed = 0;
}

static int text_reserve(u64 needed)
{
	u64 next_capacity;
	char *next;

	if (text_failed) {
		return -1;
	}
	if (needed <= text_capacity) {
		return 0;
	}
	next_capacity = text_capacity == 0 ? 256 : text_capacity;
	while (next_capacity < needed) {
		if (next_capacity > (~0ull / 2)) {
			text_failed = 1;
			return -1;
		}
		next_capacity *= 2;
	}
	next = (char *)bunix_realloc(text, text_capacity, next_capacity);
	if (next == 0) {
		text_failed = 1;
		return -1;
	}
	text = next;
	text_capacity = next_capacity;
	return 0;
}

static int text_reset(void)
{
	text_failed = 0;
	if (text_reserve(1) != 0) {
		return -1;
	}
	text[0] = '\0';
	return 0;
}

static void append_char(u64 *len, char c)
{
	if (text_reserve(*len + 2) == 0) {
		text[*len] = c;
		*len += 1;
		text[*len] = '\0';
	}
}

static void append_str(u64 *len, const char *src)
{
	while (*src != '\0') {
		append_char(len, *src++);
	}
}

static void append_bytes(u64 *len, const char *src, u64 count)
{
	if (text_reserve(*len + count + 1) == 0) {
		for (u64 i = 0; i < count; i++) {
			text[*len + i] = src[i];
		}
		*len += count;
		text[*len] = '\0';
	}
}

static void append_u64(u64 *len, u64 value)
{
	char tmp[20];
	u64 pos = 0;

	if (value == 0) {
		append_char(len, '0');
		return;
	}
	while (value != 0 && pos < sizeof(tmp)) {
		tmp[pos++] = (char)('0' + (value % 10));
		value /= 10;
	}
	while (pos != 0) {
		append_char(len, tmp[--pos]);
	}
}

static void append_hex_digit(u64 *len, u64 value)
{
	value &= 0xf;
	append_char(len, value < 10 ? (char)('0' + value) :
				     (char)('A' + value - 10));
}

static void append_hex_fixed(u64 *len, u64 value, u64 digits)
{
	while (digits != 0) {
		digits--;
		append_hex_digit(len, value >> (digits * 4));
	}
}

static void append_ipv4_route_hex(u64 *len, u64 value)
{
	for (u64 i = 0; i < 4; i++) {
		append_hex_fixed(len, (value >> (i * 8)) & 0xff, 2);
	}
}

static void append_net_iface_name(u64 *len, u64 iface, u64 flags)
{
	if ((flags & BUNIX_NET_IFACE_FLAG_LOOPBACK) != 0 || iface == 1) {
		append_str(len, "lo");
		return;
	}
	{
		struct bunix_msg count;
		u64 eth_index = 0;

		if (net_call(BUNIX_NET_INTERFACE_COUNT, 0, &count) == 0) {
			for (u64 i = 0; i < count.words[1]; i++) {
				struct bunix_msg info;
				u64 candidate;
				u64 candidate_flags;

				if (net_call(BUNIX_NET_INTERFACE_AT, i,
					     &info) != 0) {
					continue;
				}
				candidate = info.words[1];
				candidate_flags = info.words[2];
				if (candidate == 1 ||
				    (candidate_flags &
				     BUNIX_NET_IFACE_FLAG_LOOPBACK) != 0) {
					continue;
				}
				if (candidate == iface) {
					append_str(len, "eth");
					append_u64(len, eth_index);
					return;
				}
				eth_index++;
			}
		}
	}
	append_str(len, "eth");
	append_u64(len, iface >= 2 ? iface - 2 : 0);
}

static void u64_to_dec(char *out, u64 out_size, u64 value)
{
	char tmp[20];
	u64 pos = 0;
	u64 len = 0;

	if (out_size == 0) {
		return;
	}
	if (value == 0) {
		out[0] = '0';
		if (out_size > 1) {
			out[1] = '\0';
		}
		return;
	}
	while (value != 0 && pos < sizeof(tmp)) {
		tmp[pos++] = (char)('0' + (value % 10));
		value /= 10;
	}
	while (pos != 0 && len + 1 < out_size) {
		out[len++] = tmp[--pos];
	}
	out[len] = '\0';
}

static void append_task_name(u64 *len, const u64 *words)
{
	for (u64 i = 0; i < 16; i++) {
		const u64 slot = i / 8;
		const u64 shift = (i % 8) * 8;
		const char c = (char)((words[slot] >> shift) & 0xff);

		if (c == '\0') {
			break;
		}
		append_char(len, c);
	}
}

static u64 build_kthreads(void)
{
	u64 len = 0;

	append_str(&len, "tid threads flags name\n");
	text[len] = '\0';
	for (u64 index = 0;; index++) {
		u64 packed = 0;
		u64 name_words[2] = { 0, 0 };
		const long rc = bunix_task_info(index, &packed, name_words);

		if (rc != 0) {
			break;
		}
		append_u64(&len, packed & 0xffffffff);
		append_char(&len, ' ');
		append_u64(&len, (packed >> 32) & 0xffff);
		append_char(&len, ' ');
		append_u64(&len, packed >> 48);
		append_char(&len, ' ');
		append_task_name(&len, name_words);
		append_char(&len, '\n');
	}
	return len;
}

static void append_fixed2(u64 *len, u64 value)
{
	append_u64(len, value);
	append_str(len, ".00");
}

static u64 build_uptime(void)
{
	u64 len = 0;
	const u64 seconds = bunix_clock_monotonic_ns() / 1000000000ull;

	append_fixed2(&len, seconds);
	append_char(&len, ' ');
	append_fixed2(&len, seconds);
	append_char(&len, '\n');
	return len;
}

static u64 build_meminfo(void)
{
	u64 len = 0;
	struct bunix_vm_stats stats = { 0, 0 };
	u64 total_kb = 131072;
	u64 free_kb = 65536;

	if (bunix_vm_stats(&stats) == 0 && stats.total_frames != 0) {
		total_kb = stats.total_frames * 4;
		free_kb = stats.free_frames * 4;
	}

	append_str(&len, "MemTotal:       ");
	append_u64(&len, total_kb);
	append_str(&len, " kB\n");
	append_str(&len, "MemFree:        ");
	append_u64(&len, free_kb);
	append_str(&len, " kB\n");
	append_str(&len, "MemAvailable:   ");
	append_u64(&len, free_kb);
	append_str(&len, " kB\n");
	append_str(&len, "Buffers:             0 kB\n");
	append_str(&len, "Cached:              0 kB\n");
	append_str(&len, "SwapCached:          0 kB\n");
	append_str(&len, "Active:              0 kB\n");
	append_str(&len, "Inactive:            0 kB\n");
	append_str(&len, "Active(anon):        0 kB\n");
	append_str(&len, "Inactive(anon):      0 kB\n");
	append_str(&len, "Active(file):        0 kB\n");
	append_str(&len, "Inactive(file):      0 kB\n");
	append_str(&len, "Unevictable:         0 kB\n");
	append_str(&len, "Mlocked:             0 kB\n");
	append_str(&len, "SwapTotal:           0 kB\n");
	append_str(&len, "SwapFree:            0 kB\n");
	append_str(&len, "Dirty:               0 kB\n");
	append_str(&len, "Writeback:           0 kB\n");
	append_str(&len, "AnonPages:       ");
	append_u64(&len, total_kb > free_kb ? total_kb - free_kb : 0);
	append_str(&len, " kB\n");
	append_str(&len, "Mapped:              0 kB\n");
	append_str(&len, "Shmem:               0 kB\n");
	append_str(&len, "KReclaimable:        0 kB\n");
	append_str(&len, "Slab:                0 kB\n");
	append_str(&len, "SReclaimable:        0 kB\n");
	append_str(&len, "SUnreclaim:          0 kB\n");
	return len;
}

static u64 build_filesystems(void)
{
	u64 len = 0;

	append_str(&len, "nodev\tsysfs\n");
	append_str(&len, "nodev\tproc\n");
	append_str(&len, "nodev\tdevtmpfs\n");
	append_str(&len, "nodev\ttmpfs\n");
	append_str(&len, "nodev\tunionfs\n");
	append_str(&len, "\tsquashfs\n");
	append_str(&len, "\text2\n");
	return len;
}

static u64 build_cpuinfo(void)
{
	u64 len = 0;

	append_str(&len, "processor\t: 0\n");
	append_str(&len, "vendor_id\t: Bunix\n");
	append_str(&len, "cpu family\t: 6\n");
	append_str(&len, "model\t\t: 0\n");
	append_str(&len, "model name\t: Bunix virtual CPU\n");
	append_str(&len, "cpu MHz\t\t: 1000.000\n");
	append_str(&len, "flags\t\t: fpu sse sse2 syscall\n\n");
	return len;
}

static u64 build_cmdline(void)
{
	u64 len = 0;

	append_str(&len, "root=/dev/root rw init=/sbin/init\n");
	return len;
}

static u64 build_devices(void)
{
	u64 len = 0;

	append_str(&len, "Character devices:\n");
	append_str(&len, "  1 mem\n");
	append_str(&len, "  4 tty\n");
	append_str(&len, "  5 console\n");
	append_str(&len, "  5 tty\n");
	append_str(&len, "  1 random\n\n");
	append_str(&len, "Block devices:\n");
	return len;
}

static u64 build_modules(void)
{
	return 0;
}

static const char *mount_fstype_name(u64 fstype)
{
	if (fstype == BUNIX_SERVICE_PROCFS) {
		return "proc";
	}
	if (fstype == BUNIX_SERVICE_TMPFS) {
		return "tmpfs";
	}
	if (fstype == BUNIX_SERVICE_DEVFS) {
		return "devtmpfs";
	}
	if (fstype == BUNIX_SERVICE_SYSFS) {
		return "sysfs";
	}
	if (fstype == BUNIX_SERVICE_UNIONFS) {
		return "unionfs";
	}
	if (fstype == BUNIX_SERVICE_EXT2) {
		return "ext2";
	}
	return "squashfs";
}

static int mount_cache_insert(const char *path, u64 fstype)
{
	struct procfs_mount *mount;

	if (path == 0 || path[0] != '/') {
		return -1;
	}
	mount = (struct procfs_mount *)bunix_tree_get(&mounts, path);
	if (mount != 0) {
		mount->fstype = fstype;
		return 0;
	}
	mount = (struct procfs_mount *)bunix_calloc(1, sizeof(*mount));
	if (mount == 0) {
		return -1;
	}
	mount->path = str_dup(path);
	if (mount->path == 0) {
		bunix_free(mount);
		return -1;
	}
	mount->fstype = fstype;
	if (bunix_tree_insert_node(&mounts, &mount->node, mount->path,
				   (u64)mount) != 0) {
		bunix_free(mount->path);
		bunix_free(mount);
		return -1;
	}
	return 0;
}

static void mount_cache_remove(const char *path)
{
	struct procfs_mount *mount;

	if (path == 0 || path[0] != '/' || str_eq(path, "/")) {
		return;
	}
	mount = (struct procfs_mount *)bunix_tree_get(&mounts, path);
	if (mount == 0) {
		return;
	}
	bunix_tree_remove_node(&mounts, &mount->node);
	bunix_free(mount->path);
	bunix_free(mount);
}

static void handle_mount_notify(const struct bunix_msg *message)
{
	char path[PROCFS_MAX_PATH];

	if (message->cap == 0 ||
	    (message->cap_rights & BUNIX_RIGHT_RECV) == 0 ||
	    message->words[0] == 0 ||
	    message->words[0] > sizeof(path) ||
	    bunix_buffer_read(message->cap, 0, path, message->words[0]) != 0) {
		return;
	}
	path[sizeof(path) - 1] = '\0';
	(void)mount_cache_insert(path, message->words[1]);
}

static void handle_unmount_notify(const struct bunix_msg *message)
{
	char path[PROCFS_MAX_PATH];

	if (message->cap == 0 ||
	    (message->cap_rights & BUNIX_RIGHT_RECV) == 0 ||
	    message->words[0] == 0 ||
	    message->words[0] > sizeof(path) ||
	    bunix_buffer_read(message->cap, 0, path, message->words[0]) != 0) {
		return;
	}
	path[sizeof(path) - 1] = '\0';
	mount_cache_remove(path);
}

static u64 build_mounts(void)
{
	u64 len = 0;

	for (struct bunix_tree_node *node = bunix_tree_first_node(&mounts);
	     node != 0; node = bunix_tree_next_node(node)) {
		const struct procfs_mount *mount =
			(const struct procfs_mount *)node->value;
		const char *fstype;

		if (mount == 0) {
			continue;
		}
		fstype = mount_fstype_name(mount->fstype);
		append_str(&len, fstype);
		append_str(&len, " ");
		append_str(&len, mount->path);
		append_str(&len, " ");
		append_str(&len, fstype);
		append_str(&len, mount->fstype == 0 ? " ro 0 0\n" :
			   " rw 0 0\n");
	}
	return len;
}

static u64 build_mountinfo(void)
{
	u64 len = 0;
	u64 id = 1;

	for (struct bunix_tree_node *node = bunix_tree_first_node(&mounts);
	     node != 0; node = bunix_tree_next_node(node)) {
		const struct procfs_mount *mount =
			(const struct procfs_mount *)node->value;
		const char *fstype;

		if (mount == 0) {
			continue;
		}
		fstype = mount_fstype_name(mount->fstype);
		append_u64(&len, id);
		append_str(&len, id == 1 ? " 0 0:" : " 1 0:");
		append_u64(&len, id);
		append_str(&len, " / ");
		append_str(&len, mount->path);
		append_str(&len, mount->fstype == 0 ? " ro" : " rw");
		append_str(&len, " - ");
		append_str(&len, fstype);
		append_char(&len, ' ');
		append_str(&len, fstype);
		append_str(&len, mount->fstype == 0 ? " ro\n" : " rw\n");
		id++;
	}
	return len;
}

static u64 build_loadavg(void)
{
	u64 len = 0;

	append_str(&len, "0.00 0.00 0.00 1/1 1\n");
	return len;
}

static u64 build_proc_stat(void)
{
	u64 len = 0;
	const u64 ticks = bunix_timer_ticks();
	const u64 user = ticks / 4;
	const u64 system = ticks / 8;
	const u64 idle = ticks;
	struct proc_info info;
	u64 processes = 0;

	while (proc_at(processes, &info) == 0) {
		processes++;
	}

	append_str(&len, "cpu  ");
	append_u64(&len, user);
	append_str(&len, " 0 ");
	append_u64(&len, system);
	append_char(&len, ' ');
	append_u64(&len, idle);
	append_str(&len, " 0 0 0 0 0 0\n");
	append_str(&len, "cpu0 ");
	append_u64(&len, user);
	append_str(&len, " 0 ");
	append_u64(&len, system);
	append_char(&len, ' ');
	append_u64(&len, idle);
	append_str(&len, " 0 0 0 0 0 0\n");
	append_str(&len, "intr 0\n");
	append_str(&len, "ctxt ");
	append_u64(&len, ticks);
	append_char(&len, '\n');
	append_str(&len, "btime 0\n");
	append_str(&len, "processes ");
	append_u64(&len, processes);
	append_char(&len, '\n');
	append_str(&len, "procs_running ");
	append_u64(&len, processes == 0 ? 0 : 1);
	append_char(&len, '\n');
	append_str(&len, "procs_blocked 0\n");
	return len;
}

static u64 build_ipc(void)
{
	u64 len = 0;
	struct bunix_ipc_stats stats;

	if (bunix_ipc_stats(&stats) != 0) {
		return 0;
	}

	append_str(&len, "sends ");
	append_u64(&len, stats.sends);
	append_char(&len, '\n');
	append_str(&len, "queued ");
	append_u64(&len, stats.queued);
	append_char(&len, '\n');
	append_str(&len, "direct_delivered ");
	append_u64(&len, stats.direct_delivered);
	append_char(&len, '\n');
	append_str(&len, "direct_handoff ");
	append_u64(&len, stats.direct_handoff);
	append_char(&len, '\n');
	append_str(&len, "fallback_cross_cpu ");
	append_u64(&len, stats.fallback_cross_cpu);
	append_char(&len, '\n');
	append_str(&len, "fallback_nested ");
	append_u64(&len, stats.fallback_nested);
	append_char(&len, '\n');
	append_str(&len, "fallback_scheduler ");
	append_u64(&len, stats.fallback_scheduler);
	append_char(&len, '\n');
	append_str(&len, "fallback_invalid ");
	append_u64(&len, stats.fallback_invalid);
	append_char(&len, '\n');
	for (u64 cpu = 0; cpu < BUNIX_IPC_STATS_CPUS; cpu++) {
		if (stats.cpu_sends[cpu] == 0 &&
		    stats.cpu_queued[cpu] == 0 &&
		    stats.cpu_direct_delivered[cpu] == 0 &&
		    stats.cpu_direct_handoff[cpu] == 0 &&
		    stats.cpu_fallback_cross_cpu[cpu] == 0 &&
		    stats.cpu_fallback_nested[cpu] == 0 &&
		    stats.cpu_fallback_scheduler[cpu] == 0 &&
		    stats.cpu_fallback_invalid[cpu] == 0) {
			continue;
		}

		append_str(&len, "cpu");
		append_u64(&len, cpu);
		append_str(&len, " sends ");
		append_u64(&len, stats.cpu_sends[cpu]);
		append_str(&len, " queued ");
		append_u64(&len, stats.cpu_queued[cpu]);
		append_str(&len, " direct_delivered ");
		append_u64(&len, stats.cpu_direct_delivered[cpu]);
		append_str(&len, " direct_handoff ");
		append_u64(&len, stats.cpu_direct_handoff[cpu]);
		append_str(&len, " fallback_cross_cpu ");
		append_u64(&len, stats.cpu_fallback_cross_cpu[cpu]);
		append_str(&len, " fallback_nested ");
		append_u64(&len, stats.cpu_fallback_nested[cpu]);
		append_str(&len, " fallback_scheduler ");
		append_u64(&len, stats.cpu_fallback_scheduler[cpu]);
		append_str(&len, " fallback_invalid ");
		append_u64(&len, stats.cpu_fallback_invalid[cpu]);
		append_char(&len, '\n');
	}
	return len;
}

static u64 build_sched(void)
{
	u64 len = 0;
	struct bunix_sched_stats stats;

	if (bunix_sched_stats(&stats) != 0) {
		return 0;
	}

	append_str(&len, "enqueues ");
	append_u64(&len, stats.enqueues);
	append_char(&len, '\n');
	append_str(&len, "switches ");
	append_u64(&len, stats.switches);
	append_char(&len, '\n');
	append_str(&len, "wakeups ");
	append_u64(&len, stats.wakeups);
	append_char(&len, '\n');
	append_str(&len, "preemptions ");
	append_u64(&len, stats.preemptions);
	append_char(&len, '\n');
	append_str(&len, "migrations ");
	append_u64(&len, stats.migrations);
	append_char(&len, '\n');
	append_str(&len, "runtime_ticks ");
	append_u64(&len, stats.runtime_ticks);
	append_char(&len, '\n');
	append_str(&len, "wait_ticks ");
	append_u64(&len, stats.wait_ticks);
	append_char(&len, '\n');
	append_str(&len, "max_wait_ticks ");
	append_u64(&len, stats.max_wait_ticks);
	append_char(&len, '\n');
	append_str(&len, "wake_to_run_ticks ");
	append_u64(&len, stats.wake_to_run_ticks);
	append_char(&len, '\n');
	append_str(&len, "max_wake_to_run_ticks ");
	append_u64(&len, stats.max_wake_to_run_ticks);
	append_char(&len, '\n');

	for (u64 cpu = 0; cpu < BUNIX_SCHED_STATS_CPUS; cpu++) {
		if (stats.cpu_enqueues[cpu] == 0 &&
		    stats.cpu_switches[cpu] == 0 &&
		    stats.cpu_wakeups[cpu] == 0 &&
		    stats.cpu_preemptions[cpu] == 0 &&
		    stats.cpu_migrations[cpu] == 0 &&
		    stats.cpu_runtime_ticks[cpu] == 0 &&
		    stats.cpu_wait_ticks[cpu] == 0 &&
		    stats.cpu_max_wait_ticks[cpu] == 0 &&
		    stats.cpu_wake_to_run_ticks[cpu] == 0 &&
		    stats.cpu_max_wake_to_run_ticks[cpu] == 0 &&
		    stats.cpu_runq_load[cpu] == 0 &&
		    stats.cpu_min_vruntime[cpu] == 0) {
			continue;
		}

		append_str(&len, "cpu");
		append_u64(&len, cpu);
		append_str(&len, " enqueues ");
		append_u64(&len, stats.cpu_enqueues[cpu]);
		append_str(&len, " switches ");
		append_u64(&len, stats.cpu_switches[cpu]);
		append_str(&len, " wakeups ");
		append_u64(&len, stats.cpu_wakeups[cpu]);
		append_str(&len, " preemptions ");
		append_u64(&len, stats.cpu_preemptions[cpu]);
		append_str(&len, " migrations ");
		append_u64(&len, stats.cpu_migrations[cpu]);
		append_str(&len, " runtime_ticks ");
		append_u64(&len, stats.cpu_runtime_ticks[cpu]);
		append_str(&len, " wait_ticks ");
		append_u64(&len, stats.cpu_wait_ticks[cpu]);
		append_str(&len, " max_wait_ticks ");
		append_u64(&len, stats.cpu_max_wait_ticks[cpu]);
		append_str(&len, " wake_to_run_ticks ");
		append_u64(&len, stats.cpu_wake_to_run_ticks[cpu]);
		append_str(&len, " max_wake_to_run_ticks ");
		append_u64(&len, stats.cpu_max_wake_to_run_ticks[cpu]);
		append_str(&len, " runq_load ");
		append_u64(&len, stats.cpu_runq_load[cpu]);
		append_str(&len, " min_vruntime ");
		append_u64(&len, stats.cpu_min_vruntime[cpu]);
		append_char(&len, '\n');
	}

	return len;
}

static u64 build_net_dev(void)
{
	u64 len = 0;
	struct bunix_msg count;
	struct bunix_msg stats;

	append_str(&len, "Inter-|   Receive                                                |  Transmit\n");
	append_str(&len, " face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed\n");
	if (net_call(BUNIX_NET_INTERFACE_COUNT, 0, &count) != 0) {
		return len;
	}
	for (u64 i = 0; i < count.words[1]; i++) {
		struct bunix_msg info;
		u64 iface;
		u64 flags;

		if (net_call(BUNIX_NET_INTERFACE_AT, i, &info) != 0) {
			continue;
		}
		iface = info.words[1];
		flags = info.words[2];
		if (net_call(BUNIX_NET_INTERFACE_STATS, iface, &stats) != 0) {
			continue;
		}
		append_str(&len, "    ");
		append_net_iface_name(&len, iface, flags);
		append_str(&len, ": ");
		append_u64(&len, stats.words[1]);
		append_char(&len, ' ');
		append_u64(&len, stats.words[1]);
		append_str(&len, " 0 ");
		append_u64(&len, stats.words[3]);
		append_str(&len, " 0 0 0 0 ");
		append_u64(&len, stats.words[2]);
		append_char(&len, ' ');
		append_u64(&len, stats.words[2]);
		append_str(&len, " 0 ");
		append_u64(&len, stats.words[3]);
		append_str(&len, " 0 0 0 0\n");
	}
	return len;
}

static u64 build_net_route(void)
{
	u64 len = 0;
	struct bunix_msg count;
	long buffer;

	append_str(&len, "Iface\tDestination\tGateway \tFlags\tRefCnt\tUse\tMetric\tMask\t\tMTU\tWindow\tIRTT\n");
	if (net_call(BUNIX_NET_ROUTE_COUNT, 0, &count) != 0) {
		return len;
	}
	buffer = bunix_buffer_create(sizeof(struct bunix_net_route_info));
	if (buffer <= 0) {
		return len;
	}
	for (u64 i = 0; i < count.words[1]; i++) {
		struct bunix_net_route_info route;
		struct bunix_msg reply;
		struct bunix_msg iface;
		u64 mask;

		if (net_call_buffer(BUNIX_NET_ROUTE_AT, i, (u64)buffer,
				    &reply) != 0 ||
		    bunix_buffer_read((u64)buffer, 0, &route,
				      sizeof(route)) != 0 ||
		    route.family != BUNIX_NET_ADDR_FAMILY_IPV4 ||
		    net_call(BUNIX_NET_INTERFACE_INFO, route.iface,
			     &iface) != 0) {
			continue;
		}
		mask = route.prefix_len == 0 ? 0 :
		       ~((1ull << (32 - route.prefix_len)) - 1) & 0xffffffffull;
		append_net_iface_name(&len, route.iface, iface.words[2]);
		append_char(&len, '\t');
		append_ipv4_route_hex(&len, route.prefix_lo);
		append_char(&len, '\t');
		append_ipv4_route_hex(&len, route.gateway_lo);
		append_char(&len, '\t');
		append_hex_fixed(&len, route.flags == 0 ? 1 : route.flags, 4);
		append_str(&len, "\t0\t0\t");
		append_u64(&len, route.metric);
		append_char(&len, '\t');
		append_ipv4_route_hex(&len, mask);
		append_str(&len, "\t0\t0\t0\n");
	}
	bunix_handle_close((u64)buffer);
	return len;
}

static u64 build_net_config(void)
{
	u64 len = 0;
	struct bunix_net_config_status status;
	struct bunix_msg reply;
	long buffer;

	buffer = bunix_buffer_create(sizeof(status));
	if (buffer <= 0) {
		return len;
	}
	if (net_call_buffer(BUNIX_NET_CONFIG_STATUS, 0, (u64)buffer,
			    &reply) != 0 ||
	    bunix_buffer_read((u64)buffer, 0, &status, sizeof(status)) != 0) {
		bunix_handle_close((u64)buffer);
		return len;
	}
	bunix_handle_close((u64)buffer);
	append_str(&len, "loopback ");
	append_u64(&len,
		   (status.flags & BUNIX_NET_CONFIG_LOOPBACK) != 0 ? 1 : 0);
	append_char(&len, '\n');
	append_str(&len, "default_ipv4 ");
	append_u64(&len,
		   (status.flags & BUNIX_NET_CONFIG_DEFAULT_IPV4) != 0 ? 1 : 0);
	append_char(&len, '\n');
	append_str(&len, "default_ipv6 ");
	append_u64(&len,
		   (status.flags & BUNIX_NET_CONFIG_DEFAULT_IPV6) != 0 ? 1 : 0);
	append_char(&len, '\n');
	append_str(&len, "interfaces ");
	append_u64(&len, status.interface_count);
	append_char(&len, '\n');
	append_str(&len, "addresses ");
	append_u64(&len, status.address_count);
	append_char(&len, '\n');
	append_str(&len, "routes ");
	append_u64(&len, status.route_count);
	append_char(&len, '\n');
	append_str(&len, "last_error ");
	append_u64(&len, status.last_error);
	append_char(&len, '\n');
	for (u64 i = 0; i < status.interface_count; i++) {
		struct bunix_msg info;
		struct bunix_msg stats;
		u64 iface;
		u64 flags;

		if (net_call(BUNIX_NET_INTERFACE_AT, i, &info) != 0) {
			continue;
		}
		iface = info.words[1];
		flags = info.words[2];
		if (net_call(BUNIX_NET_INTERFACE_STATS, iface, &stats) != 0) {
			continue;
		}
		append_str(&len, "iface ");
		append_net_iface_name(&len, iface, flags);
		append_str(&len, " id ");
		append_u64(&len, iface);
		append_str(&len, " flags ");
		append_hex_fixed(&len, flags, 4);
		append_str(&len, " mtu ");
		append_u64(&len, info.words[3]);
		append_str(&len, " rx ");
		append_u64(&len, stats.words[1]);
		append_str(&len, " tx ");
		append_u64(&len, stats.words[2]);
		append_str(&len, " drops ");
		append_u64(&len, stats.words[3]);
		append_char(&len, '\n');
	}
	return len;
}

static int net_socket_count(u64 *count)
{
	struct bunix_msg reply;

	if (count == 0 ||
	    net_call(BUNIX_NET_OBSERVE_SOCKET_COUNT, 0, &reply) != 0) {
		return -1;
	}
	*count = reply.words[1];
	return 0;
}

static int net_socket_at(u64 index, struct bunix_net_socket_info *info)
{
	struct bunix_msg reply;
	long buffer;

	if (info == 0) {
		return -1;
	}
	buffer = bunix_buffer_create(sizeof(*info));
	if (buffer <= 0) {
		return -1;
	}
	if (net_call_buffer(BUNIX_NET_OBSERVE_SOCKET_AT, index, (u64)buffer,
			    &reply) != 0 ||
	    reply.words[1] != sizeof(*info) ||
	    bunix_buffer_read((u64)buffer, 0, info, sizeof(*info)) != 0) {
		bunix_handle_close((u64)buffer);
		return -1;
	}
	bunix_handle_close((u64)buffer);
	return 0;
}

static u64 build_net_sockstat(void)
{
	u64 len = 0;
	u64 count = 0;
	u64 tcp = 0;
	u64 udp = 0;

	if (net_socket_count(&count) == 0) {
		for (u64 i = 0; i < count; i++) {
			struct bunix_net_socket_info info;

			if (net_socket_at(i, &info) != 0) {
				continue;
			}
			if (info.protocol == 6) {
				tcp++;
			} else if (info.protocol == 17) {
				udp++;
			}
		}
	}
	append_str(&len, "sockets: used ");
	append_u64(&len, tcp + udp);
	append_char(&len, '\n');
	append_str(&len, "TCP: inuse ");
	append_u64(&len, tcp);
	append_str(&len, "\nUDP: inuse ");
	append_u64(&len, udp);
	append_str(&len, "\nUDPLITE: inuse 0\nRAW: inuse 0\nFRAG: inuse 0 memory 0\n");
	return len;
}

static u64 net_tcp_state(u64 state)
{
	if (state == 2) {
		return 0x0a;
	}
	if (state == 3) {
		return 0x01;
	}
	if (state == 4) {
		return 0x07;
	}
	return 0x07;
}

static void append_proc_net_addr(u64 *len, const struct bunix_net_socket_info *info,
				 int peer, int ipv6)
{
	const u64 hi = peer ? info->peer_hi : info->local_hi;
	const u64 lo = peer ? info->peer_lo : info->local_lo;
	const u64 port = peer ? info->peer_port : info->local_port;

	if (ipv6) {
		append_hex_fixed(len, hi, 16);
		append_hex_fixed(len, lo, 16);
	} else {
		append_hex_fixed(len, lo & 0xffffffff, 8);
	}
	append_char(len, ':');
	append_hex_fixed(len, port & 0xffff, 4);
}

static u64 build_net_socket_table(u64 protocol, u64 family)
{
	u64 len = 0;
	u64 count = 0;
	const int ipv6 = family == BUNIX_NET_ADDR_FAMILY_IPV6;

	append_str(&len, "  sl  local_address rem_address   st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n");
	if (net_socket_count(&count) != 0) {
		return len;
	}
	for (u64 i = 0, line = 0; i < count; i++) {
		struct bunix_net_socket_info info;

		if (net_socket_at(i, &info) != 0 ||
		    info.protocol != protocol || info.family != family) {
			continue;
		}
		append_char(&len, ' ');
		append_u64(&len, line);
		append_str(&len, ": ");
		append_proc_net_addr(&len, &info, 0, ipv6);
		append_char(&len, ' ');
		append_proc_net_addr(&len, &info, 1, ipv6);
		append_char(&len, ' ');
		append_hex_fixed(&len, protocol == 6 ?
				 net_tcp_state(info.state) : 0x07, 2);
		append_str(&len, " 00000000:");
		append_hex_fixed(&len, info.rx_len & 0xffffffff, 8);
		append_str(&len, " 00:00000000 00000000  1000        0 ");
		append_u64(&len, info.id);
		append_char(&len, '\n');
		line++;
	}
	return len;
}

static const char *pid_name(u64 pid)
{
	static char name[32];
	char cmdline[PROCFS_MAX_PATH];
	u64 start = 0;
	u64 len = 0;

	if (proc_cmdline(pid, cmdline, sizeof(cmdline)) != 0 ||
	    cmdline[0] == '\0') {
		return "process";
	}
	for (u64 i = 0; cmdline[i] != '\0'; i++) {
		if (cmdline[i] == '/') {
			start = i + 1;
		}
	}
	while (cmdline[start + len] != '\0' && len + 1 < sizeof(name)) {
		name[len] = cmdline[start + len];
		len++;
	}
	name[len] = '\0';
	return len == 0 ? "process" : name;
}

static const char *pid_cmdline(u64 pid)
{
	static char cmdline[PROCFS_MAX_PATH];

	if (proc_cmdline(pid, cmdline, sizeof(cmdline)) != 0 ||
	    cmdline[0] == '\0') {
		return "/bin/process";
	}
	return cmdline;
}

static u64 build_pid_stat(u64 pid)
{
	u64 len = 0;
	struct proc_details details = { 0, 'S', 0 };

	(void)proc_details(pid, &details);

	append_u64(&len, pid);
	append_str(&len, " (");
	append_str(&len, pid_name(pid));
	append_str(&len, ") ");
	append_char(&len, details.state == '\0' ? 'S' : details.state);
	append_char(&len, ' ');
	append_u64(&len, details.ppid);
	append_str(&len, " 1 1 0 -1 4194304 0 0 0 0 0 0 0 0 20 0 1 0 ");
	append_u64(&len, bunix_timer_ticks());
	append_str(&len, " 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n");
	return len;
}

static u64 build_pid_status(u64 pid)
{
	u64 len = 0;
	struct proc_info info = { 0, 0, 0 };
	struct proc_details details = { 0, 'S', 0 };

	(void)proc_info(pid, &info);
	(void)proc_details(pid, &details);
	append_str(&len, "Name:\t");
	append_str(&len, pid_name(pid));
	append_char(&len, '\n');
	append_str(&len, "State:\t");
	append_char(&len, details.state == '\0' ? 'S' : details.state);
	append_str(&len, details.state == 'Z' ? " (zombie)\n" : " (sleeping)\n");
	append_str(&len, "Pid:\t");
	append_u64(&len, pid);
	append_char(&len, '\n');
	append_str(&len, "BunixTask:\t");
	append_u64(&len, info.task_id);
	append_char(&len, '\n');
	append_str(&len, "LinuxPid:\t");
	append_u64(&len, info.linux_pid);
	append_char(&len, '\n');
	append_str(&len, "PPid:\t");
	append_u64(&len, details.ppid);
	append_char(&len, '\n');
	append_str(&len, "Uid:\t1000\t1000\t1000\t1000\n");
	append_str(&len, "Gid:\t1000\t1000\t1000\t1000\n");
	append_str(&len, "Groups:\t1000\n");
	append_str(&len, "Threads:\t1\n");
	return len;
}

static u64 build_pid_cmdline(u64 pid)
{
	u64 len = 0;
	char cmdline[PROCFS_MAX_PATH];
	u64 cmdline_len = 0;

	if (proc_cmdline_bytes(pid, cmdline, sizeof(cmdline),
			       &cmdline_len) != 0 ||
	    cmdline_len == 0) {
		append_str(&len, pid_cmdline(pid));
		append_char(&len, '\0');
	} else {
		append_bytes(&len, cmdline, cmdline_len);
		append_char(&len, '\0');
	}
	return len;
}

static u64 build_pid_statm(u64 pid)
{
	u64 len = 0;

	(void)pid;
	append_str(&len, "0 0 0 0 0 0 0\n");
	return len;
}

static u64 build_pid_cgroup(u64 pid)
{
	u64 len = 0;

	(void)pid;
	append_str(&len, "0::/\n");
	return len;
}

static u64 build_fd_entry(u64 fd)
{
	u64 len = 0;

	append_str(&len, "fd ");
	append_u64(&len, fd);
	append_char(&len, '\n');
	return len;
}

static u64 build_file_text(u64 file)
{
	u64 len;

	if (text_reset() != 0) {
		return 0;
	}
	switch (file_kind(file)) {
	case PROCFS_KIND_KTHREADS:
		len = build_kthreads();
		break;
	case PROCFS_KIND_UPTIME:
		len = build_uptime();
		break;
	case PROCFS_KIND_STAT:
		len = build_proc_stat();
		break;
	case PROCFS_KIND_IPC:
		len = build_ipc();
		break;
	case PROCFS_KIND_SCHED:
		len = build_sched();
		break;
	case PROCFS_KIND_NET_DEV:
		len = build_net_dev();
		break;
	case PROCFS_KIND_NET_ROUTE:
		len = build_net_route();
		break;
	case PROCFS_KIND_NET_CONFIG:
		len = build_net_config();
		break;
	case PROCFS_KIND_NET_SOCKSTAT:
		len = build_net_sockstat();
		break;
	case PROCFS_KIND_NET_UDP:
		len = build_net_socket_table(17, BUNIX_NET_ADDR_FAMILY_IPV4);
		break;
	case PROCFS_KIND_NET_UDP6:
		len = build_net_socket_table(17, BUNIX_NET_ADDR_FAMILY_IPV6);
		break;
	case PROCFS_KIND_NET_TCP:
		len = build_net_socket_table(6, BUNIX_NET_ADDR_FAMILY_IPV4);
		break;
	case PROCFS_KIND_NET_TCP6:
		len = build_net_socket_table(6, BUNIX_NET_ADDR_FAMILY_IPV6);
		break;
	case PROCFS_KIND_LOADAVG:
		len = build_loadavg();
		break;
	case PROCFS_KIND_MEMINFO:
		len = build_meminfo();
		break;
	case PROCFS_KIND_FILESYSTEMS:
		len = build_filesystems();
		break;
	case PROCFS_KIND_CPUINFO:
		len = build_cpuinfo();
		break;
	case PROCFS_KIND_CMDLINE:
		len = build_cmdline();
		break;
	case PROCFS_KIND_DEVICES:
		len = build_devices();
		break;
	case PROCFS_KIND_MODULES:
		len = build_modules();
		break;
	case PROCFS_KIND_MOUNTS:
	case PROCFS_KIND_PID_MOUNTS:
		len = build_mounts();
		break;
	case PROCFS_KIND_PID_MOUNTINFO:
		len = build_mountinfo();
		break;
	case PROCFS_KIND_PID_STAT:
		len = build_pid_stat(file_arg(file));
		break;
	case PROCFS_KIND_PID_STATUS:
		len = build_pid_status(file_arg(file));
		break;
	case PROCFS_KIND_PID_CMDLINE:
		len = build_pid_cmdline(file_arg(file));
		break;
	case PROCFS_KIND_PID_STATM:
		len = build_pid_statm(file_arg(file));
		break;
	case PROCFS_KIND_PID_CGROUP:
		len = build_pid_cgroup(file_arg(file));
		break;
	case PROCFS_KIND_PID_FD_ENTRY:
		len = build_fd_entry(file_arg(file));
		break;
	default:
		return 0;
	}
	return text_failed ? 0 : len;
}

static u64 open_file(u64 file)
{
	return bunix_id_alloc(&open_files, file);
}

static u64 file_from_handle(u64 handle)
{
	return bunix_id_get(&open_files, handle);
}

static void close_file(u64 handle)
{
	(void)bunix_id_remove(&open_files, handle);
}

static int file_is_dir(u64 file)
{
	const u64 kind = file_kind(file);

	return kind == PROCFS_KIND_PROC ||
	       kind == PROCFS_KIND_NET ||
	       kind == PROCFS_KIND_SELF ||
	       kind == PROCFS_KIND_PID ||
	       kind == PROCFS_KIND_PID_FD;
}

static u64 stat_buffer_offset(const struct bunix_msg *message)
{
	if (message->type == BUNIX_VFS_STAT_PATH_META_BUFFER) {
		return message->words[0] + message->words[1];
	}
	return message->words[1];
}

static void stat_reply(const struct bunix_msg *message, struct bunix_msg *reply,
		       u64 file)
{
	const int is_dir = file_is_dir(file);
	const int is_symlink = file_kind(file) == PROCFS_KIND_PID_EXE;
	const u64 size = file_size(file);

	reply->words[0] = 0;
	reply->words[1] = size;
	reply->words[2] = is_dir ?
			  (0555 | ((u64)BUNIX_VFS_TYPE_DIRECTORY << 32)) :
			  (is_symlink ?
			   (0777 | ((u64)BUNIX_VFS_TYPE_SYMLINK << 32)) :
			   (0444 | ((u64)BUNIX_VFS_TYPE_REGULAR << 32)));
	reply->words[3] = 0;
	if (message->cap != 0 &&
	    (message->cap_rights & BUNIX_RIGHT_SEND) != 0) {
		(void)bunix_vfs_stat_write(
			message->cap, stat_buffer_offset(message), size,
			reply->words[2], 0, BUNIX_VFS_DEV_PROCFS,
			file == 0 ? 1 : file, 1, 0, 4096,
			(size + 511) / 512);
	}
}

static const char *proc_dir_entry(u64 index, u64 *type)
{
	static const char *names[] = {
		"kthreads", "uptime", "stat", "ipc", "sched", "loadavg", "meminfo",
		"filesystems", "cpuinfo", "cmdline", "devices", "modules",
		"mounts", "net", "self"
	};
	static char pid_name_buf[20];
	struct proc_info info;
	const u64 static_count = sizeof(names) / sizeof(names[0]);

	if (index < static_count) {
		*type = index == 13 || index == 14 ? BUNIX_VFS_TYPE_DIRECTORY :
				     BUNIX_VFS_TYPE_REGULAR;
		return names[index];
	}
	if (proc_at(index - static_count, &info) != 0) {
		return 0;
	}
	u64_to_dec(pid_name_buf, sizeof(pid_name_buf), info.pid);
	*type = BUNIX_VFS_TYPE_DIRECTORY;
	return pid_name_buf;
}

static const char *net_dir_entry(u64 index, u64 *type)
{
	static const char *names[] = {
		"dev", "route", "config", "sockstat", "udp", "udp6", "tcp",
		"tcp6"
	};

	if (index >= sizeof(names) / sizeof(names[0])) {
		return 0;
	}
	*type = BUNIX_VFS_TYPE_REGULAR;
	return names[index];
}

static const char *pid_dir_entry(u64 index, u64 *type)
{
	static const char *names[] = {
		"stat", "status", "cmdline", "statm", "mounts",
		"mountinfo", "cgroup", "exe", "fd"
	};

	if (index >= sizeof(names) / sizeof(names[0])) {
		return 0;
	}
	*type = index == 8 ? BUNIX_VFS_TYPE_DIRECTORY :
		(index == 7 ? BUNIX_VFS_TYPE_SYMLINK :
			      BUNIX_VFS_TYPE_REGULAR);
	return names[index];
}

static const char *fd_dir_entry(u64 index, u64 *type)
{
	static const char *names[] = { "0", "1", "2" };

	if (index >= sizeof(names) / sizeof(names[0])) {
		return 0;
	}
	*type = BUNIX_VFS_TYPE_REGULAR;
	return names[index];
}

static void readlink_buffer_reply(const struct bunix_msg *message,
				  struct bunix_msg *reply, u64 file)
{
	const char *target;
	const u64 out_cap = message->words[3] >> 32;
	u64 target_len;
	u64 written;

	if (file_kind(file) != PROCFS_KIND_PID_EXE) {
		reply->words[0] = (u64)-1;
		return;
	}
	target = pid_cmdline(file_arg(file));
	target_len = str_len(target);
	written = target_len;
	reply->words[0] = 0;
	reply->words[1] = target_len;
	if (out_cap == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	if (message->cap == 0 ||
	    (message->cap_rights & BUNIX_RIGHT_SEND) == 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	if (written > out_cap) {
		written = out_cap;
	}
	if (written != 0 &&
	    bunix_buffer_write(message->cap, message->words[0] + message->words[1],
			       target, written) != 0) {
		reply->words[0] = (u64)-1;
		return;
	}
	reply->words[2] = target_len;
	reply->words[3] = written;
}

int main(void)
{
	const char online[] = "procfs: online\n";
	struct bunix_msg message;

	bunix_console_log(online, sizeof(online) - 1);
	bunix_id_table_init(&open_files);
	bunix_tree_init(&mounts);
	if (mount_cache_insert("/", 0) != 0) {
		return 1;
	}
	if (register_service(BUNIX_SERVICE_PROCFS, BUNIX_HANDLE_SELF) != 0) {
		return 1;
	}

	for (;;) {
		struct bunix_msg reply = {
			.protocol = BUNIX_PROTO_VFS,
			.type = 0,
			.sender = 0,
			.cap_rights = 0,
			.reply = 0,
			.cap = 0,
			.words = { 0, 0, 0, 0 },
		};

		if (bunix_ipc_recv(BUNIX_HANDLE_SELF, &message) != 0) {
			continue;
		}
		if (message.protocol == BUNIX_PROTO_PROCFS &&
		    message.type == BUNIX_PROCFS_MOUNT_NOTIFY) {
			handle_mount_notify(&message);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			continue;
		}
		if (message.protocol == BUNIX_PROTO_PROCFS &&
		    message.type == BUNIX_PROCFS_UNMOUNT_NOTIFY) {
			handle_unmount_notify(&message);
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			continue;
		}
		if (message.protocol == BUNIX_PROTO_PROCFS &&
		    message.type == BUNIX_PROCFS_MOUNT_PATH) {
			char path[PROCFS_MAX_PATH];

			reply.protocol = BUNIX_PROTO_PROCFS;
			reply.type = message.type;
			reply.words[0] =
				bunix_read_path_cap(&message, path,
						    sizeof(path)) == 0 ?
				(u64)mount_procfs(path) : (u64)-1;
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			bunix_ipc_send(message.reply, &reply);
			continue;
		}
		if (message.protocol != BUNIX_PROTO_VFS) {
			if (message.cap != 0) {
				bunix_handle_close(message.cap);
			}
			continue;
		}

		reply.type = message.type;
		switch (message.type) {
		case BUNIX_VFS_OPEN_BUFFER:
		case BUNIX_VFS_STAT_PATH_META_BUFFER: {
			char path[PROCFS_MAX_PATH];
			const u64 cwd_len = message.words[0];
			const u64 path_len = message.words[1];
			const u64 caller_task = message.words[3] & 0xffffffff;
			const u64 file =
				read_path_buffer(message.cap, cwd_len, path_len,
						 path) == 0 ?
				file_for_path(path, caller_task) : 0;

			if (file == 0) {
				reply.words[0] = BUNIX_VFS_ERR_NOENT;
				break;
			}
			if (message.type == BUNIX_VFS_STAT_PATH_META_BUFFER) {
				stat_reply(&message, &reply, file);
			} else {
				const u64 handle = open_file(file);

				if (handle == 0) {
					reply.words[0] = (u64)-1;
				} else {
					reply.words[0] = 0;
					reply.words[1] = handle;
					reply.words[2] = file_size(file);
					reply.words[3] = file_is_dir(file) ?
						BUNIX_VFS_TYPE_DIRECTORY :
						BUNIX_VFS_TYPE_REGULAR;
				}
			}
			break;
		}
		case BUNIX_VFS_READLINK_BUFFER: {
			char path[PROCFS_MAX_PATH];
			const u64 cwd_len = message.words[0];
			const u64 path_len = message.words[1];
			const u64 caller_task = message.words[3] & 0xffffffff;
			const u64 file =
				read_path_buffer(message.cap, cwd_len, path_len,
						 path) == 0 ?
				file_for_path(path, caller_task) : 0;

			if (file == 0) {
				reply.words[0] = BUNIX_VFS_ERR_NOENT;
				break;
			}
			readlink_buffer_reply(&message, &reply, file);
			break;
		}
		case BUNIX_VFS_STAT_META: {
			const u64 file = file_from_handle(message.words[0]);

			if (file == 0) {
				reply.words[0] = (u64)-1;
			} else {
				stat_reply(&message, &reply, file);
			}
			break;
		}
		case BUNIX_VFS_READ_FILE_BUFFER: {
			const u64 file = file_from_handle(message.words[0]);
			const u64 offset = message.words[1];
			u64 len = message.words[2];
			u64 size;

			if (file == 0 || file_is_dir(file) || message.cap == 0) {
				reply.words[0] = (u64)-1;
				break;
			}
			size = build_file_text(file);
			if (offset >= size) {
				len = 0;
			} else if (len > size - offset) {
				len = size - offset;
			}
			if (len == 0) {
				reply.words[0] = 0;
			} else {
				reply.words[0] =
					bunix_buffer_write(message.cap, 0,
							   text + offset,
							   len) == 0 ?
					0 : (u64)-1;
			}
			reply.words[1] = reply.words[0] == 0 ? len : 0;
			text_release();
			break;
		}
		case BUNIX_VFS_READDIR_BUFFER: {
			const u64 file = file_from_handle(message.words[0]);
			const char *name = 0;
			u64 type = BUNIX_VFS_TYPE_REGULAR;
			u64 name_len;
			u64 written;

			if (!file_is_dir(file)) {
				reply.words[0] = BUNIX_VFS_ERR_NOTDIR;
				break;
			}
			if (message.cap == 0 ||
			    (message.cap_rights & BUNIX_RIGHT_SEND) == 0) {
				reply.words[0] = (u64)-1;
				break;
			}
			if (file_kind(file) == PROCFS_KIND_PROC) {
				name = proc_dir_entry(message.words[1], &type);
			} else if (file_kind(file) == PROCFS_KIND_NET) {
				name = net_dir_entry(message.words[1], &type);
			} else if (file_kind(file) == PROCFS_KIND_PID_FD) {
				name = fd_dir_entry(message.words[1], &type);
			} else {
				name = pid_dir_entry(message.words[1], &type);
			}
			if (name == 0) {
				reply.words[0] = BUNIX_VFS_ERR_NOENT;
				break;
			}
			name_len = str_len(name);
			written = name_len;
			if (written > message.words[3]) {
				written = message.words[3];
			}
			if (written != 0 &&
			    bunix_buffer_write(message.cap, message.words[2],
					       name, written) != 0) {
				reply.words[0] = (u64)-1;
				break;
			}
			reply.words[0] = 0;
			reply.words[1] = (message.words[1] + 1) |
					 (type << 32);
			reply.words[2] = name_len;
			reply.words[3] = written;
			break;
		}
		case BUNIX_VFS_CLOSE:
			if (file_from_handle(message.words[0]) == 0) {
				reply.words[0] = (u64)-1;
			} else {
				close_file(message.words[0]);
				reply.words[0] = 0;
			}
			break;
		default:
			reply.words[0] = (u64)-1;
			break;
		}

		if (message.cap != 0) {
			bunix_handle_close(message.cap);
		}
		if (message.reply != 0) {
			bunix_ipc_send(message.reply, &reply);
		}
	}
}
