#include <bunix/libbunix.h>
#include <bunix/id_table.h>

enum {
	PROCFS_HANDLE_NAMES = 3,
	PROCFS_MAX_TEXT = 2048,
	PROCFS_MAX_PATH = 256,
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
};

static struct bunix_id_table open_files;
static char text[PROCFS_MAX_TEXT];
static u64 proc_handle;

struct proc_info {
	u64 pid;
	u64 task_id;
	u64 linux_pid;
};

static u64 build_kthreads(void);

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

static void unpack_path(char *path, const u64 *words)
{
	for (u64 i = 0; i < PROCFS_MAX_PATH; i++) {
		path[i] = '\0';
	}
	for (u64 i = 0; i < 16; i++) {
		const u64 slot = i / 8;
		const u64 shift = (i % 8) * 8;

		path[i] = (char)((words[slot] >> shift) & 0xff);
	}
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

static void pack_name(u64 *words, const char *name)
{
	words[0] = 0;
	words[1] = 0;
	for (u64 i = 0; i < 16 && name[i] != '\0'; i++) {
		const u64 slot = i / 8;
		const u64 shift = (i % 8) * 8;

		words[slot] |= ((u64)(unsigned char)name[i]) << shift;
	}
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

static u64 get_proc_handle(void)
{
	if (proc_handle == 0) {
		proc_handle = resolve_service(BUNIX_SERVICE_PROC,
					      BUNIX_RIGHT_SEND);
	}
	return proc_handle;
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

static int proc_cmdline(u64 pid, char *out, u64 out_size)
{
	struct bunix_msg reply;
	u64 len;

	if (out == 0 || out_size == 0 ||
	    proc_call(BUNIX_PROC_CMDLINE, pid, &reply) != 0) {
		return -1;
	}
	for (u64 i = 0; i < out_size; i++) {
		out[i] = '\0';
	}
	len = reply.words[1];
	if (len >= out_size) {
		len = out_size - 1;
	}
	for (u64 i = 0; i < len && i < 16; i++) {
		const u64 slot = 2 + (i / 8);
		const u64 shift = (i % 8) * 8;

		out[i] = (char)((reply.words[slot] >> shift) & 0xff);
	}
	return 0;
}

static u64 proc_self_pid(void)
{
	struct proc_info info;
	u64 fallback = 0;

	for (u64 index = 0; proc_at(index, &info) == 0; index++) {
		char cmdline[32];

		if (fallback == 0 && info.linux_pid != 0) {
			fallback = info.pid;
		}
		if (proc_cmdline(info.pid, cmdline, sizeof(cmdline)) == 0 &&
		    str_eq(cmdline, "/bin/sh")) {
			return info.pid;
		}
	}
	return fallback;
}

static u64 proc_path_pid(u64 pid)
{
	struct proc_info info;

	if (pid == 0) {
		pid = proc_self_pid();
	}
	return proc_info(pid, &info) == 0 ? pid : 0;
}

static long mount_procfs(void)
{
	const u64 vfs = resolve_service(BUNIX_SERVICE_VFS,
					BUNIX_RIGHT_SEND);
	struct bunix_msg request = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_MOUNT,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP,
		.reply = 0,
		.cap = BUNIX_HANDLE_SELF,
		.words = { 0, 0, 0, 0 },
	};
	struct bunix_msg reply;

	if (vfs == 0) {
		return -1;
	}
	pack_path(&request.words[0], "/proc");
	if (bunix_ipc_call(vfs, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return -1;
	}
	return 0;
}

static u64 file_for_path(const char *path)
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
	if (str_eq(path, "/proc/loadavg")) {
		return make_file(PROCFS_KIND_LOADAVG, 0);
	}
	if (str_eq(path, "/proc/meminfo")) {
		return make_file(PROCFS_KIND_MEMINFO, 0);
	}
	if (str_eq(path, "/proc/mounts")) {
		return make_file(PROCFS_KIND_MOUNTS, 0);
	}
	if (str_eq(path, "/proc/self")) {
		const u64 pid = proc_path_pid(0);

		return pid == 0 ? 0 : make_file(PROCFS_KIND_SELF, pid);
	}
	if (str_eq(path, "/proc/self/stat")) {
		const u64 pid = proc_path_pid(0);

		return pid == 0 ? 0 : make_file(PROCFS_KIND_PID_STAT, pid);
	}
	if (str_eq(path, "/proc/self/status")) {
		const u64 pid = proc_path_pid(0);

		return pid == 0 ? 0 : make_file(PROCFS_KIND_PID_STATUS, pid);
	}
	if (str_eq(path, "/proc/self/cmdline")) {
		const u64 pid = proc_path_pid(0);

		return pid == 0 ? 0 : make_file(PROCFS_KIND_PID_CMDLINE, pid);
	}
	if (str_eq(path, "/proc/self/statm")) {
		const u64 pid = proc_path_pid(0);

		return pid == 0 ? 0 : make_file(PROCFS_KIND_PID_STATM, pid);
	}
	if (str_eq(path, "/proc/self/fd")) {
		const u64 pid = proc_path_pid(0);

		return pid == 0 ? 0 : make_file(PROCFS_KIND_PID_FD, pid);
	}
	if (path_has_prefix(path, "/proc/self/fd/")) {
		const char *cursor = path + 14;
		u64 fd;
		const u64 pid = proc_path_pid(0);

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
		if (proc_path_pid(pid) == 0) {
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
	if (file_kind(file) != PROCFS_KIND_PROC &&
	    file_kind(file) != PROCFS_KIND_SELF &&
	    file_kind(file) != PROCFS_KIND_PID &&
	    file_kind(file) != PROCFS_KIND_PID_FD) {
		return PROCFS_MAX_TEXT;
	}
	return 0;
}

static void append_char(u64 *len, char c)
{
	if (*len + 1 < sizeof(text)) {
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

	append_str(&len, "MemTotal:       131072 kB\n");
	append_str(&len, "MemFree:         65536 kB\n");
	append_str(&len, "MemAvailable:    65536 kB\n");
	append_str(&len, "Buffers:             0 kB\n");
	append_str(&len, "Cached:              0 kB\n");
	append_str(&len, "SwapTotal:           0 kB\n");
	append_str(&len, "SwapFree:            0 kB\n");
	return len;
}

static u64 build_mounts(void)
{
	u64 len = 0;

	append_str(&len, "rootfs / rootfs ro 0 0\n");
	append_str(&len, "proc /proc proc rw 0 0\n");
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

static const char *pid_name(u64 pid)
{
	static char name[32];
	char cmdline[32];
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
	static char cmdline[32];

	if (proc_cmdline(pid, cmdline, sizeof(cmdline)) != 0 ||
	    cmdline[0] == '\0') {
		return "/bin/process";
	}
	return cmdline;
}

static u64 build_pid_stat(u64 pid)
{
	u64 len = 0;

	append_u64(&len, pid);
	append_str(&len, " (");
	append_str(&len, pid_name(pid));
	append_str(&len, ") S 0 1 1 0 -1 4194304 0 0 0 0 0 0 0 0 20 0 1 0 ");
	append_u64(&len, bunix_timer_ticks());
	append_str(&len, " 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n");
	return len;
}

static u64 build_pid_status(u64 pid)
{
	u64 len = 0;

	append_str(&len, "Name:\t");
	append_str(&len, pid_name(pid));
	append_char(&len, '\n');
	append_str(&len, "State:\tS (sleeping)\n");
	append_str(&len, "Pid:\t");
	append_u64(&len, pid);
	append_char(&len, '\n');
	append_str(&len, "PPid:\t0\n");
	append_str(&len, "Uid:\t1000\t1000\t1000\t1000\n");
	append_str(&len, "Gid:\t1000\t1000\t1000\t1000\n");
	append_str(&len, "Groups:\t1000\n");
	append_str(&len, "Threads:\t1\n");
	return len;
}

static u64 build_pid_cmdline(u64 pid)
{
	u64 len = 0;

	append_str(&len, pid_cmdline(pid));
	append_char(&len, '\0');
	return len;
}

static u64 build_pid_statm(u64 pid)
{
	u64 len = 0;

	(void)pid;
	append_str(&len, "0 0 0 0 0 0 0\n");
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
	switch (file_kind(file)) {
	case PROCFS_KIND_KTHREADS:
		return build_kthreads();
	case PROCFS_KIND_UPTIME:
		return build_uptime();
	case PROCFS_KIND_STAT:
		return build_proc_stat();
	case PROCFS_KIND_IPC:
		return build_ipc();
	case PROCFS_KIND_LOADAVG:
		return build_loadavg();
	case PROCFS_KIND_MEMINFO:
		return build_meminfo();
	case PROCFS_KIND_MOUNTS:
		return build_mounts();
	case PROCFS_KIND_PID_STAT:
		return build_pid_stat(file_arg(file));
	case PROCFS_KIND_PID_STATUS:
		return build_pid_status(file_arg(file));
	case PROCFS_KIND_PID_CMDLINE:
		return build_pid_cmdline(file_arg(file));
	case PROCFS_KIND_PID_STATM:
		return build_pid_statm(file_arg(file));
	case PROCFS_KIND_PID_FD_ENTRY:
		return build_fd_entry(file_arg(file));
	default:
		return 0;
	}
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
	       kind == PROCFS_KIND_SELF ||
	       kind == PROCFS_KIND_PID ||
	       kind == PROCFS_KIND_PID_FD;
}

static void stat_reply(struct bunix_msg *reply, u64 file)
{
	const int is_dir = file_is_dir(file);

	reply->words[0] = 0;
	reply->words[1] = file_size(file);
	reply->words[2] = is_dir ?
			  (0555 | ((u64)BUNIX_VFS_TYPE_DIRECTORY << 32)) :
			  (0444 | ((u64)BUNIX_VFS_TYPE_REGULAR << 32));
	reply->words[3] = 0;
}

static const char *proc_dir_entry(u64 index, u64 *type)
{
	static const char *names[] = {
		"kthreads", "uptime", "stat", "ipc", "loadavg", "meminfo",
		"mounts", "self"
	};
	static char pid_name_buf[20];
	struct proc_info info;
	const u64 static_count = sizeof(names) / sizeof(names[0]);

	if (index < static_count) {
		*type = index == 7 ? BUNIX_VFS_TYPE_DIRECTORY :
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

static const char *pid_dir_entry(u64 index, u64 *type)
{
	static const char *names[] = { "stat", "status", "cmdline", "statm", "fd" };

	if (index >= sizeof(names) / sizeof(names[0])) {
		return 0;
	}
	*type = index == 4 ? BUNIX_VFS_TYPE_DIRECTORY :
			     BUNIX_VFS_TYPE_REGULAR;
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

int main(void)
{
	const char online[] = "procfs: online\n";
	struct bunix_msg message;

	bunix_console_log(online, sizeof(online) - 1);
	bunix_id_table_init(&open_files);
	if (register_service(BUNIX_SERVICE_PROCFS, BUNIX_HANDLE_SELF) != 0) {
		return 1;
	}
	if (mount_procfs() != 0) {
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

		if (bunix_ipc_recv(BUNIX_HANDLE_SELF, &message) != 0 ||
		    message.protocol != BUNIX_PROTO_VFS) {
			continue;
		}

		reply.type = message.type;
		switch (message.type) {
		case BUNIX_VFS_OPEN:
		case BUNIX_VFS_STAT_PATH_META: {
			char path[PROCFS_MAX_PATH];
			const u64 file = (unpack_path(path, &message.words[0]),
					  file_for_path(path));

			if (file == 0) {
				reply.words[0] = BUNIX_VFS_ERR_NOENT;
				break;
			}
			if (message.type == BUNIX_VFS_STAT_PATH_META) {
				stat_reply(&reply, file);
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
		case BUNIX_VFS_OPEN_BUFFER:
		case BUNIX_VFS_STAT_PATH_META_BUFFER: {
			char path[PROCFS_MAX_PATH];
			const u64 cwd_len = message.words[0];
			const u64 path_len = message.words[1];
			const u64 file =
				read_path_buffer(message.cap, cwd_len, path_len,
						 path) == 0 ?
				file_for_path(path) : 0;

			if (file == 0) {
				reply.words[0] = BUNIX_VFS_ERR_NOENT;
				break;
			}
			if (message.type == BUNIX_VFS_STAT_PATH_META_BUFFER) {
				stat_reply(&reply, file);
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
		case BUNIX_VFS_STAT_META: {
			const u64 file = file_from_handle(message.words[0]);

			if (file == 0) {
				reply.words[0] = (u64)-1;
			} else {
				stat_reply(&reply, file);
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
			reply.words[0] = bunix_buffer_write(message.cap, 0,
							    text + offset,
							    len) == 0 ? 0 :
					 (u64)-1;
			reply.words[1] = reply.words[0] == 0 ? len : 0;
			break;
		}
		case BUNIX_VFS_READDIR: {
			const u64 file = file_from_handle(message.words[0]);
			const char *name = 0;
			u64 type = BUNIX_VFS_TYPE_REGULAR;

			if (!file_is_dir(file)) {
				reply.words[0] = BUNIX_VFS_ERR_NOTDIR;
				break;
			}
			if (file_kind(file) == PROCFS_KIND_PROC) {
				name = proc_dir_entry(message.words[1], &type);
			} else if (file_kind(file) == PROCFS_KIND_PID_FD) {
				name = fd_dir_entry(message.words[1], &type);
			} else {
				name = pid_dir_entry(message.words[1], &type);
			}
			if (name == 0) {
				reply.words[0] = BUNIX_VFS_ERR_NOENT;
				break;
			}
			reply.words[0] = 0;
			reply.words[1] = (message.words[1] + 1) |
					 (type << 32);
			pack_name(&reply.words[2], name);
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
