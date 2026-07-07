#include <bunix/alloc.h>
#include <bunix/libbunix.h>
#include <bunix/tree.h>

enum {
	VFS_HANDLE_NAMES = 3,
	VFS_MAX_SYMLINKS = 40,
	VFS_MAX_PATH = 4096,
	VFS_OPEN_REMOTE = 1,
};

struct vfs_mount {
	struct bunix_tree_node node;
	char *path;
	u64 id;
	u64 service;
	u64 fstype;
	u64 pins;
};

struct vfs_open {
	struct bunix_u64_tree_node node;
	u64 id;
	u64 kind;
	u64 service;
	u64 remote_handle;
	char *path;
	u64 type;
};

static struct bunix_tree mounts;
static struct bunix_u64_tree open_files;
static u64 next_open_id = 1;
static u64 next_mount_id = 1;

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

	if (bunix_ipc_call(VFS_HANDLE_NAMES, &request, &reply) != 0) {
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

	if (bunix_ipc_call(VFS_HANDLE_NAMES, &request, &reply) != 0 ||
	    reply.words[0] != 0) {
		return 0;
	}

	return reply.cap;
}

static int path_eq(const char *left, const char *right)
{
	for (u64 i = 0; i < VFS_MAX_PATH; i++) {
		if (left[i] != right[i]) {
			return 0;
		}
		if (left[i] == '\0') {
			return 1;
		}
	}

	return 1;
}

static u64 str_len(const char *text)
{
	u64 len = 0;

	while (len < VFS_MAX_PATH && text[len] != '\0') {
		len++;
	}

	return len;
}

static char *str_dup(const char *text)
{
	const u64 len = str_len(text);
	char *copy;

	if (text == 0 || len == VFS_MAX_PATH) {
		return 0;
	}
	copy = (char *)bunix_alloc(len + 1);
	if (copy == 0) {
		return 0;
	}
	for (u64 i = 0; i <= len; i++) {
		copy[i] = text[i];
	}
	return copy;
}

static int read_path_buffer_at(u64 buffer, u64 offset, u64 len, char *path)
{
	if (buffer == 0 || len == 0 || len > VFS_MAX_PATH) {
		return -1;
	}
	for (u64 i = 0; i < VFS_MAX_PATH; i++) {
		path[i] = '\0';
	}
	if (bunix_buffer_read(buffer, offset, path, len) != 0 ||
	    path[len - 1] != '\0') {
		return -1;
	}
	return str_len(path) < VFS_MAX_PATH ? 0 : -1;
}

static int path_component_eq(const char *component, const char *literal)
{
	u64 i = 0;

	while (component[i] != '\0' && literal[i] != '\0') {
		if (component[i] != literal[i]) {
			return 0;
		}
		i++;
	}
	return component[i] == literal[i];
}

static struct vfs_open *open_from_handle(u64 handle);

static int vfs_resolve_path(const char *cwd, const char *input, char *out)
{
	char temp[VFS_MAX_PATH];
	u64 pos = 0;
	u64 in = 0;

	if (input == 0 || input[0] == '\0' ||
	    cwd == 0 || cwd[0] != '/') {
		return -1;
	}
	if (input[0] == '/') {
		temp[pos++] = '/';
	} else {
		const u64 cwd_len = str_len(cwd);

		for (u64 i = 0; i < cwd_len && pos < sizeof(temp) - 1; i++) {
			temp[pos++] = cwd[i];
		}
		if (pos == 0) {
			temp[pos++] = '/';
		}
	}

	while (input[in] != '\0') {
		char component[VFS_MAX_PATH];
		u64 comp_len = 0;

		while (input[in] == '/') {
			in++;
		}
		while (input[in] != '\0' && input[in] != '/') {
			if (comp_len + 1 >= sizeof(component)) {
				return -1;
			}
			component[comp_len++] = input[in++];
		}
		component[comp_len] = '\0';
		if (comp_len == 0 || path_component_eq(component, ".")) {
			continue;
		}
		if (path_component_eq(component, "..")) {
			if (pos > 1) {
				if (temp[pos - 1] == '/') {
					pos--;
				}
				while (pos > 1 && temp[pos - 1] != '/') {
					pos--;
				}
				if (pos > 1 && temp[pos - 1] == '/') {
					pos--;
				}
			}
			temp[pos] = '\0';
			continue;
		}
		if (pos > 1) {
			if (pos + 1 >= sizeof(temp)) {
				return -1;
			}
			temp[pos++] = '/';
		}
		if (pos + comp_len >= sizeof(temp)) {
			return -1;
		}
		for (u64 i = 0; i < comp_len; i++) {
			temp[pos++] = component[i];
		}
		temp[pos] = '\0';
	}

	if (pos == 0) {
		temp[pos++] = '/';
	}
	temp[pos] = '\0';
	for (u64 i = 0; i <= pos; i++) {
		out[i] = temp[i];
	}
	return 0;
}

static int vfs_resolve_with_base(u64 base_handle, const char *cwd,
				 const char *input, char *out, u64 *error)
{
	if (input != 0 && input[0] == '/') {
		return vfs_resolve_path(cwd, input, out);
	}
	if (base_handle == 0) {
		return vfs_resolve_path(cwd, input, out);
	}

	struct vfs_open *open = open_from_handle(base_handle);

	if (open == 0 || open->kind != VFS_OPEN_REMOTE ||
	    open->type != BUNIX_VFS_TYPE_DIRECTORY || open->path == 0) {
		if (error != 0) {
			*error = BUNIX_VFS_ERR_NOTDIR;
		}
		return -1;
	}
	return vfs_resolve_path(open->path, input, out);
}

static int path_is_at_or_under(const char *path, const char *prefix)
{
	const u64 prefix_len = str_len(prefix);

	if (prefix_len == 0) {
		return 0;
	}
	if (prefix_len == 1 && prefix[0] == '/') {
		return path[0] == '/';
	}
	for (u64 i = 0; i < prefix_len; i++) {
		if (path[i] != prefix[i]) {
			return 0;
		}
	}
	return path[prefix_len] == '\0' || path[prefix_len] == '/';
}

static struct vfs_mount *mount_for_path(const char *path)
{
	struct vfs_mount *best = 0;
	u64 best_len = 0;

	for (struct bunix_tree_node *node = bunix_tree_first_node(&mounts);
	     node != 0; node = bunix_tree_next_node(node)) {
		struct vfs_mount *mount =
			(struct vfs_mount *)node->value;
		const u64 len = mount != 0 ? str_len(mount->path) : 0;

		if (mount == 0 || len < best_len ||
		    !path_is_at_or_under(path, mount->path)) {
			continue;
		}
		best = mount;
		best_len = len;
	}
	return best;
}

static int path_dirname(const char *path, char *out)
{
	u64 len;

	if (path == 0 || out == 0 || path[0] != '/') {
		return -1;
	}
	len = str_len(path);
	while (len > 1 && path[len - 1] != '/') {
		len--;
	}
	if (len <= 1) {
		out[0] = '/';
		out[1] = '\0';
		return 0;
	}
	len--;
	for (u64 i = 0; i < len; i++) {
		out[i] = path[i];
	}
	out[len] = '\0';
	return 0;
}

static int path_copy(char *out, const char *path)
{
	const u64 len = str_len(path);

	if (out == 0 || path == 0 || len >= VFS_MAX_PATH) {
		return -1;
	}
	for (u64 i = 0; i <= len; i++) {
		out[i] = path[i];
	}
	return 0;
}

static u64 open_remote_file(u64 service, u64 remote_handle, const char *path,
			    u64 type)
{
	struct vfs_open *open;
	u64 handle;

	if (service == 0 || remote_handle == 0 || path == 0) {
		return 0;
	}
	open = (struct vfs_open *)bunix_calloc(1, sizeof(*open));
	if (open == 0) {
		return 0;
	}
	open->kind = VFS_OPEN_REMOTE;
	open->service = service;
	open->remote_handle = remote_handle;
	open->path = str_dup(path);
	open->type = type;
	if (open->path == 0) {
		bunix_free(open);
		return 0;
	}
	handle = next_open_id++;
	while (handle == 0 || bunix_u64_tree_get(&open_files, handle) != 0) {
		handle = next_open_id++;
	}
	open->id = handle;
	if (bunix_u64_tree_insert_node(&open_files, &open->node, handle,
				       (u64)open) != 0) {
		bunix_free(open->path);
		bunix_free(open);
		return 0;
	}
	return handle;
}

static struct vfs_open *open_from_handle(u64 handle)
{
	return (struct vfs_open *)bunix_u64_tree_get(&open_files, handle);
}

static void forget_open_file(u64 handle)
{
	struct vfs_open *open = open_from_handle(handle);

	if (open != 0) {
		bunix_u64_tree_remove_node(&open_files, &open->node);
		if (open->path != 0) {
			bunix_free(open->path);
		}
		bunix_free(open);
	}
}

static int forward_mount_path(struct vfs_mount *mount, struct bunix_msg *message,
			      struct bunix_msg *reply, const char *path)
{
	if (mount == 0 || mount->service == 0) {
		reply->words[0] = BUNIX_VFS_ERR_NOENT;
		return -1;
	}
	if (bunix_ipc_call(mount->service, message, reply) != 0) {
		reply->words[0] = (u64)-1;
		return -1;
	}
	if (message->type == BUNIX_VFS_OPEN_BUFFER && reply->words[0] == 0) {
		const u64 remote_handle = reply->words[1];
		const u64 local_handle =
			open_remote_file(mount->service, remote_handle, path,
					 reply->words[3]);

		if (local_handle == 0) {
			struct bunix_msg close = {
				.protocol = BUNIX_PROTO_VFS,
				.type = BUNIX_VFS_CLOSE,
				.sender = 0,
				.cap_rights = 0,
				.reply = 0,
				.cap = 0,
				.words = { remote_handle, 0, 0, 0 },
			};
			struct bunix_msg ignored;

			(void)bunix_ipc_call(mount->service, &close, &ignored);
			reply->words[0] = (u64)-1;
			return -1;
		}
		reply->words[1] = local_handle;
	}
	return 0;
}

static int forward_mount_buffer_path(struct vfs_mount *mount,
				     struct bunix_msg *message,
				     struct bunix_msg *reply,
				     const char *path)
{
	const char root[] = "/";
	const u64 cwd_len = 2;
	const u64 path_len = str_len(path) + 1;
	const u64 stat_out = message->type == BUNIX_VFS_STAT_PATH_META_BUFFER &&
			     (message->cap_rights & BUNIX_RIGHT_SEND) != 0 ?
			     BUNIX_VFS_STAT_BYTES : 0;
	const u64 out_cap = message->type == BUNIX_VFS_READLINK_BUFFER ?
			    message->words[3] >> 32 : stat_out;
	const u64 buffer_len = cwd_len + path_len + out_cap;
	const long buffer = bunix_buffer_create(buffer_len);
	struct bunix_msg forwarded = *message;
	int result;

	if (buffer < 0 ||
	    bunix_buffer_write((u64)buffer, 0, root, cwd_len) != 0 ||
	    bunix_buffer_write((u64)buffer, cwd_len, path, path_len) != 0) {
		if (buffer >= 0) {
			bunix_handle_close((u64)buffer);
		}
		reply->words[0] = (u64)-1;
		return -1;
	}

	forwarded.cap = (u64)buffer;
	forwarded.cap_rights = BUNIX_RIGHT_RECV |
			       (out_cap != 0 ? BUNIX_RIGHT_SEND : 0);
	forwarded.words[0] = cwd_len;
	forwarded.words[1] = path_len;
	forwarded.words[2] = message->type == BUNIX_VFS_CHOWN_BUFFER ?
			     message->words[2] : 0;
	if (forwarded.type == BUNIX_VFS_READLINK_BUFFER) {
		forwarded.words[3] = (message->words[3] & 0xffffffff) |
				     (out_cap << 32);
	}
	result = forward_mount_path(mount, &forwarded, reply, path);
	if (result == 0 && message->type == BUNIX_VFS_READLINK_BUFFER &&
	    out_cap != 0 && reply->words[0] == 0) {
		char target[VFS_MAX_PATH];
		const u64 offset = message->words[0] + message->words[1];
		const u64 temp_offset = cwd_len + path_len;
		const u64 written = reply->words[3];

		if (message->cap == 0 ||
		    (message->cap_rights & BUNIX_RIGHT_SEND) == 0 ||
		    written > out_cap ||
		    (written != 0 &&
		     (bunix_buffer_read((u64)buffer, temp_offset, target,
					written) != 0 ||
		      bunix_buffer_write(message->cap, offset, target,
					 written) != 0))) {
			reply->words[0] = (u64)-1;
			result = -1;
		}
	}
	if (result == 0 && message->type == BUNIX_VFS_STAT_PATH_META_BUFFER &&
	    stat_out != 0 && reply->words[0] == 0) {
		unsigned char stat[BUNIX_VFS_STAT_BYTES];
		const u64 offset = message->words[0] + message->words[1];
		const u64 temp_offset = cwd_len + path_len;

		if (message->cap == 0 ||
		    (message->cap_rights & BUNIX_RIGHT_SEND) == 0 ||
		    bunix_buffer_read((u64)buffer, temp_offset, stat,
				      sizeof(stat)) != 0 ||
		    bunix_buffer_write(message->cap, offset, stat,
				       sizeof(stat)) != 0) {
			reply->words[0] = (u64)-1;
			result = -1;
		}
	}
	bunix_handle_close((u64)buffer);
	return result;
}

static int forward_mount_symlink_path(struct vfs_mount *mount,
				      struct bunix_msg *message,
				      struct bunix_msg *reply,
				      const char *target,
				      const char *path)
{
	const char root[] = "/";
	const u64 target_len = str_len(target) + 1;
	const u64 cwd_len = 2;
	const u64 path_len = str_len(path) + 1;
	const long buffer = bunix_buffer_create(target_len + cwd_len +
						path_len);
	struct bunix_msg forwarded = *message;
	int result;

	if (buffer < 0 ||
	    bunix_buffer_write((u64)buffer, 0, target, target_len) != 0 ||
	    bunix_buffer_write((u64)buffer, target_len, root, cwd_len) != 0 ||
	    bunix_buffer_write((u64)buffer, target_len + cwd_len, path,
			       path_len) != 0) {
		if (buffer >= 0) {
			bunix_handle_close((u64)buffer);
		}
		reply->words[0] = (u64)-1;
		return -1;
	}

	forwarded.cap = (u64)buffer;
	forwarded.cap_rights = BUNIX_RIGHT_RECV;
	forwarded.words[0] = target_len;
	forwarded.words[1] = cwd_len;
	forwarded.words[2] = path_len;
	forwarded.words[3] &= 0xffffffff;
	result = forward_mount_path(mount, &forwarded, reply, path);
	bunix_handle_close((u64)buffer);
	return result;
}

static int vfs_lstat_path(const char *path, u64 task, u64 *type, u64 *status)
{
	struct vfs_mount *mount = mount_for_path(path);
	struct bunix_msg reply = {
		.protocol = BUNIX_PROTO_VFS,
		.type = BUNIX_VFS_STAT_PATH_META_BUFFER,
		.words = { 0, 0, 0, 0 },
	};

	if (type != 0) {
		*type = 0;
	}
	if (status != 0) {
		*status = (u64)-1;
	}
	if (mount != 0) {
		const char root[] = "/";
		const u64 cwd_len = 2;
		const u64 path_len = str_len(path) + 1;
		const long buffer = bunix_buffer_create(cwd_len + path_len);
		struct bunix_msg request = {
			.protocol = BUNIX_PROTO_VFS,
			.type = BUNIX_VFS_STAT_PATH_META_BUFFER,
			.sender = 0,
			.cap_rights = BUNIX_RIGHT_RECV,
			.reply = 0,
			.cap = buffer < 0 ? 0 : (u64)buffer,
			.words = { cwd_len, path_len, 0,
				   (task & 0xffffffff) | (1UL << 32) },
		};

		if (buffer < 0 ||
		    bunix_buffer_write((u64)buffer, 0, root, cwd_len) != 0 ||
		    bunix_buffer_write((u64)buffer, cwd_len, path,
				       path_len) != 0 ||
		    forward_mount_path(mount, &request, &reply, path) != 0) {
			if (buffer >= 0) {
				bunix_handle_close((u64)buffer);
			}
			return -1;
		}
		bunix_handle_close((u64)buffer);
	} else {
		reply.words[0] = BUNIX_VFS_ERR_NOENT;
	}
	if (status != 0) {
		*status = reply.words[0];
	}
	if (reply.words[0] != 0) {
		return -1;
	}
	if (type != 0) {
		*type = reply.words[2] >> 32;
	}
	return 0;
}

static int vfs_readlink_target(const char *path, u64 task, char *target,
			       u64 *status)
{
	struct vfs_mount *mount = mount_for_path(path);

	if (target != 0) {
		target[0] = '\0';
	}
	if (status != 0) {
		*status = (u64)-1;
	}
	if (target == 0) {
		return -1;
	}
	if (mount != 0) {
		const char root[] = "/";
		const u64 cwd_len = 2;
		const u64 path_len = str_len(path) + 1;
		const u64 out_cap = VFS_MAX_PATH - 1;
		const u64 out_offset = cwd_len + path_len;
		const long buffer = bunix_buffer_create(out_offset + out_cap);
		struct bunix_msg request = {
			.protocol = BUNIX_PROTO_VFS,
			.type = BUNIX_VFS_READLINK_BUFFER,
			.sender = 0,
			.cap_rights = BUNIX_RIGHT_RECV | BUNIX_RIGHT_SEND,
			.reply = 0,
			.cap = buffer < 0 ? 0 : (u64)buffer,
			.words = { cwd_len, path_len, 0,
				   (task & 0xffffffff) | (out_cap << 32) },
		};
		struct bunix_msg reply;
		u64 written;

		if (buffer < 0 ||
		    bunix_buffer_write((u64)buffer, 0, root, cwd_len) != 0 ||
		    bunix_buffer_write((u64)buffer, cwd_len, path,
				       path_len) != 0 ||
		    forward_mount_path(mount, &request, &reply, path) != 0) {
			if (buffer >= 0) {
				bunix_handle_close((u64)buffer);
			}
			return -1;
		}
		if (status != 0) {
			*status = reply.words[0];
		}
		if (reply.words[0] != 0) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
		written = reply.words[3];
		if (reply.words[1] != written || written >= VFS_MAX_PATH ||
		    bunix_buffer_read((u64)buffer, out_offset, target,
				      written) != 0) {
			bunix_handle_close((u64)buffer);
			return -1;
		}
		target[written] = '\0';
		bunix_handle_close((u64)buffer);
		return 0;
	}

	if (status != 0) {
		*status = BUNIX_VFS_ERR_NOENT;
	}
	return -1;
}

static int path_join_suffix(const char *path, const char *suffix, char *out)
{
	char combined[VFS_MAX_PATH];
	u64 pos = 0;

	if (path == 0 || suffix == 0 || out == 0) {
		return -1;
	}
	if (suffix[0] == '\0') {
		return path_copy(out, path);
	}
	if (path_eq(path, "/")) {
		combined[pos++] = '/';
		suffix++;
	} else {
		const u64 path_len = str_len(path);

		if (path_len >= sizeof(combined)) {
			return -1;
		}
		for (u64 i = 0; i < path_len; i++) {
			combined[pos++] = path[i];
		}
	}
	for (u64 i = 0; suffix[i] != '\0'; i++) {
		if (pos + 1 >= sizeof(combined)) {
			return -1;
		}
		combined[pos++] = suffix[i];
	}
	combined[pos] = '\0';
	return vfs_resolve_path("/", combined, out);
}

static int vfs_resolve_symlinks(const char *path, char *out, u64 task,
				int follow_final, u64 *error)
{
	char pending[VFS_MAX_PATH];
	u64 depth = 0;

	if (error != 0) {
		*error = (u64)-1;
	}
	if (path_copy(pending, path) != 0) {
		return -1;
	}
	for (;;) {
		u64 start = 1;

		if (path_eq(pending, "/")) {
			return path_copy(out, pending);
		}
		while (pending[start - 1] != '\0') {
			char prefix[VFS_MAX_PATH];
			char target[VFS_MAX_PATH];
			char resolved[VFS_MAX_PATH];
			char base[VFS_MAX_PATH];
			u64 end = start;
			u64 type = 0;
			u64 status = 0;

			while (pending[end] != '\0' && pending[end] != '/') {
				end++;
			}
			for (u64 i = 0; i < end; i++) {
				prefix[i] = pending[i];
			}
			prefix[end] = '\0';
			if (!follow_final && pending[end] == '\0') {
				return path_copy(out, pending);
			}
			if (vfs_lstat_path(prefix, task, &type, &status) != 0) {
				return path_copy(out, pending);
			}
			if (type == BUNIX_VFS_TYPE_SYMLINK) {
				if (depth++ >= VFS_MAX_SYMLINKS) {
					if (error != 0) {
						*error = BUNIX_VFS_ERR_LOOP;
					}
					return -1;
				}
				if (vfs_readlink_target(prefix, task, target,
							&status) != 0 ||
				    path_dirname(prefix, base) != 0 ||
				    vfs_resolve_path(base, target, resolved) != 0 ||
				    path_join_suffix(resolved, pending + end,
						     pending) != 0) {
					if (error != 0) {
						*error = status == 0 ?
							 (u64)-1 : status;
					}
					return -1;
				}
				break;
			}
			if (pending[end] == '\0') {
				return path_copy(out, pending);
			}
			start = end + 1;
		}
	}
}

static int forward_mount_two_path(struct vfs_mount *mount,
				  struct bunix_msg *message,
				  struct bunix_msg *reply,
				  const char *old_path,
				  const char *new_path)
{
	const char root[] = "/";
	const u64 cwd_len = 2;
	const u64 old_len = str_len(old_path) + 1;
	const u64 new_len = str_len(new_path) + 1;
	const long buffer = bunix_buffer_create(cwd_len + old_len +
						cwd_len + new_len);
	struct bunix_msg forwarded = *message;
	int result;

	if (buffer < 0 ||
	    bunix_buffer_write((u64)buffer, 0, root, cwd_len) != 0 ||
	    bunix_buffer_write((u64)buffer, cwd_len, old_path, old_len) != 0 ||
	    bunix_buffer_write((u64)buffer, cwd_len + old_len, root,
			       cwd_len) != 0 ||
	    bunix_buffer_write((u64)buffer, cwd_len + old_len + cwd_len,
			       new_path, new_len) != 0) {
		if (buffer >= 0) {
			bunix_handle_close((u64)buffer);
		}
		reply->words[0] = (u64)-1;
		return -1;
	}

	forwarded.cap = (u64)buffer;
	forwarded.cap_rights = BUNIX_RIGHT_RECV;
	forwarded.words[0] = 0;
	forwarded.words[1] = cwd_len | (old_len << 32);
	forwarded.words[2] = cwd_len | (new_len << 32);
	result = forward_mount_path(mount, &forwarded, reply, new_path);
	bunix_handle_close((u64)buffer);
	return result;
}

static int forward_remote_handle(struct vfs_open *open,
				 struct bunix_msg *message,
				 struct bunix_msg *reply)
{
	if (open == 0 || open->kind != VFS_OPEN_REMOTE ||
	    open->service == 0 || open->remote_handle == 0) {
		reply->words[0] = (u64)-1;
		return -1;
	}

	if (message->type == BUNIX_VFS_READDIR_BUFFER) {
		char name[VFS_MAX_PATH];
		const u64 out_cap = message->words[3];
		const u64 scratch_cap = out_cap > VFS_MAX_PATH ? VFS_MAX_PATH :
					out_cap;
		const long buffer = bunix_buffer_create(scratch_cap == 0 ? 1 :
							scratch_cap);
		struct bunix_msg forwarded = *message;
		int result = 0;

		if (message->cap == 0 ||
		    (message->cap_rights & BUNIX_RIGHT_SEND) == 0 ||
		    buffer <= 0) {
			if (buffer > 0) {
				bunix_handle_close((u64)buffer);
			}
			reply->words[0] = (u64)-1;
			return -1;
		}
		forwarded.words[0] = open->remote_handle;
		forwarded.words[2] = 0;
		forwarded.words[3] = scratch_cap;
		forwarded.cap = (u64)buffer;
		forwarded.cap_rights = BUNIX_RIGHT_SEND;
		if (bunix_ipc_call(open->service, &forwarded, reply) != 0) {
			reply->words[0] = (u64)-1;
			result = -1;
		}
		if (result == 0 && reply->words[0] == 0) {
			const u64 written = reply->words[3];

			if (written > scratch_cap ||
			    (written != 0 &&
			     (bunix_buffer_read((u64)buffer, 0, name,
						written) != 0 ||
			      bunix_buffer_write(message->cap, message->words[2],
						 name, written) != 0))) {
				reply->words[0] = (u64)-1;
				result = -1;
			}
		}
		bunix_handle_close((u64)buffer);
		return result;
	}

	message->words[0] = open->remote_handle;
	if (bunix_ipc_call(open->service, message, reply) != 0) {
		reply->words[0] = (u64)-1;
		return -1;
	}
	return 0;
}

static void vfs_open_path(struct bunix_msg *message,
			  struct bunix_msg *reply,
			  const char *path);

static struct vfs_mount *mount_by_id(u64 id)
{
	if (id == 0) {
		return 0;
	}
	for (struct bunix_tree_node *node = bunix_tree_first_node(&mounts);
	     node != 0; node = bunix_tree_next_node(node)) {
		struct vfs_mount *mount = (struct vfs_mount *)node->value;

		if (mount != 0 && mount->id == id) {
			return mount;
		}
	}
	return 0;
}

static u64 mount_translator(const char *path, u64 service, u64 fstype)
{
	struct vfs_mount *mount;

	if (path == 0 || path[0] != '/' || service == 0) {
		return BUNIX_VFS_ERR_INVAL;
	}

	mount = (struct vfs_mount *)bunix_tree_get(&mounts, path);
	if (mount != 0) {
		if (mount->pins != 0) {
			return BUNIX_VFS_ERR_BUSY;
		}
		mount->service = service;
		mount->fstype = fstype;
		return 0;
	}

	mount = (struct vfs_mount *)bunix_calloc(1, sizeof(*mount));
	if (mount == 0) {
		return (u64)-1;
	}
	mount->path = str_dup(path);
	if (mount->path == 0) {
		bunix_free(mount);
		return (u64)-1;
	}
	mount->id = next_mount_id++;
	while (mount->id == 0 || mount_by_id(mount->id) != 0) {
		mount->id = next_mount_id++;
	}
	mount->service = service;
	mount->fstype = fstype;
	if (bunix_tree_insert_node(&mounts, &mount->node, mount->path,
				   (u64)mount) != 0) {
		bunix_free(mount->path);
		bunix_free(mount);
		return (u64)-1;
	}
	return 0;
}

static u64 unmount_translator(const char *path)
{
	struct vfs_mount *mount;

	if (path == 0 || path[0] != '/' || path_eq(path, "/")) {
		return BUNIX_VFS_ERR_INVAL;
	}
	mount = (struct vfs_mount *)bunix_tree_get(&mounts, path);
	if (mount == 0) {
		return BUNIX_VFS_ERR_NOENT;
	}
	for (struct bunix_u64_tree_node *node =
		     bunix_u64_tree_first_node(&open_files);
	     node != 0; node = bunix_u64_tree_next_node(node)) {
		const struct vfs_open *open = (const struct vfs_open *)node->value;

		if (open != 0 && open->path != 0 &&
		    path_is_at_or_under(open->path, mount->path)) {
			return BUNIX_VFS_ERR_BUSY;
		}
	}
	if (mount->pins != 0) {
		return BUNIX_VFS_ERR_BUSY;
	}
	bunix_tree_remove_node(&mounts, &mount->node);
	bunix_free(mount->path);
	bunix_free(mount);
	return 0;
}

static void vfs_reply_mount_route(struct bunix_msg *reply, const char *path,
				  int pin)
{
	struct vfs_mount *mount = mount_for_path(path);

	if (mount == 0 || mount->service == 0) {
		reply->words[0] = BUNIX_VFS_ERR_NOENT;
		return;
	}
	if (mount->fstype == BUNIX_SERVICE_UNIONFS) {
		reply->words[0] = BUNIX_VFS_ERR_INVAL;
		reply->words[1] = mount->fstype;
		reply->words[2] = BUNIX_VFS_ROUTE_FLAG_RECURSIVE;
		return;
	}
	if (pin) {
		mount->pins++;
	}
	reply->words[0] = 0;
	reply->cap = mount->service;
	reply->cap_rights = BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP;
	reply->words[1] = mount->fstype;
	reply->words[2] = 0;
	reply->words[3] = mount->id;
}

static void vfs_unpin_mount_route(struct bunix_msg *reply, u64 route_id)
{
	struct vfs_mount *mount = mount_by_id(route_id);

	if (mount == 0 || mount->pins == 0) {
		reply->words[0] = BUNIX_VFS_ERR_NOENT;
		return;
	}
	mount->pins--;
	reply->words[0] = 0;
	reply->words[1] = mount->pins;
}

static void notify_procfs_mount_event(const char *path, u64 type, u64 fstype)
{
	struct vfs_mount *procfs =
		(struct vfs_mount *)bunix_tree_get(&mounts, "/proc");
	const u64 path_len = str_len(path) + 1;
	const long buffer = bunix_buffer_create(path_len);
	struct bunix_msg message = {
		.protocol = BUNIX_PROTO_PROCFS,
		.type = type,
		.sender = 0,
		.cap_rights = BUNIX_RIGHT_RECV,
		.reply = 0,
		.cap = buffer < 0 ? 0 : (u64)buffer,
		.words = { path_len, fstype, 0, 0 },
	};

	if (procfs == 0 || procfs->service == 0 || buffer < 0) {
		if (buffer >= 0) {
			bunix_handle_close((u64)buffer);
		}
		return;
	}
	if (bunix_buffer_write((u64)buffer, 0, path, path_len) == 0) {
		(void)bunix_ipc_send(procfs->service, &message);
	}
	bunix_handle_close((u64)buffer);
}

static void notify_procfs_mount(const char *path, u64 fstype)
{
	notify_procfs_mount_event(path, BUNIX_PROCFS_MOUNT_NOTIFY, fstype);
}

static void notify_procfs_unmount(const char *path)
{
	notify_procfs_mount_event(path, BUNIX_PROCFS_UNMOUNT_NOTIFY, 0);
}

static void vfs_open_path(struct bunix_msg *message, struct bunix_msg *reply,
			  const char *path)
{
	char resolved[VFS_MAX_PATH];
	u64 error = (u64)-1;
	struct vfs_mount *mount;

	if (vfs_resolve_symlinks(path, resolved, message->words[3] & 0xffffffff,
				 1, &error) != 0) {
		reply->words[0] = error;
		return;
	}
	mount = mount_for_path(resolved);
	if (mount != 0) {
		struct bunix_msg forwarded = *message;

		if (message->type == BUNIX_VFS_OPEN_BUFFER) {
			(void)forward_mount_buffer_path(mount, message, reply,
							resolved);
			return;
		}
		(void)forward_mount_path(mount, &forwarded, reply, resolved);
		return;
	}

	reply->words[0] = BUNIX_VFS_ERR_NOENT;
}

static void vfs_stat_path(struct bunix_msg *message, struct bunix_msg *reply,
			  const char *path)
{
	char resolved[VFS_MAX_PATH];
	u64 error = (u64)-1;
	struct vfs_mount *mount;
	const int nofollow = ((message->words[3] >> 32) & 1) != 0;

	if (vfs_resolve_symlinks(path, resolved, message->words[3] & 0xffffffff,
				 !nofollow, &error) != 0) {
		reply->words[0] = error;
		return;
	}
	mount = mount_for_path(resolved);
	if (mount != 0) {
		struct bunix_msg forwarded = *message;

		if (message->type == BUNIX_VFS_STAT_PATH_META_BUFFER) {
			(void)forward_mount_buffer_path(mount, message, reply,
							resolved);
			return;
		}
		(void)forward_mount_path(mount, &forwarded, reply, resolved);
		return;
	}

	reply->words[0] = BUNIX_VFS_ERR_NOENT;
}

static void vfs_access_path(struct bunix_msg *message, struct bunix_msg *reply,
			    const char *path)
{
	char resolved[VFS_MAX_PATH];
	u64 error = (u64)-1;
	struct vfs_mount *mount;
	const u64 task = message->words[3] & 0xffffffff;

	if (vfs_resolve_symlinks(path, resolved, task, 1, &error) != 0) {
		reply->words[0] = error;
		return;
	}
	mount = mount_for_path(resolved);
	if (mount != 0) {
		struct bunix_msg forwarded = *message;

		if (message->type == BUNIX_VFS_ACCESS_BUFFER) {
			(void)forward_mount_buffer_path(mount, message, reply,
							resolved);
			return;
		}
		(void)forward_mount_path(mount, &forwarded, reply, resolved);
		return;
	}

	reply->words[0] = BUNIX_VFS_ERR_NOENT;
}

static void vfs_mutate_path(struct bunix_msg *message, struct bunix_msg *reply,
			    const char *path)
{
	struct vfs_mount *mount = mount_for_path(path);

	if (mount != 0) {
		(void)forward_mount_buffer_path(mount, message, reply, path);
		return;
	}
	reply->words[0] = BUNIX_VFS_ERR_ACCESS;
}

static void vfs_symlink_path(struct bunix_msg *message,
			     struct bunix_msg *reply,
			     const char *target,
			     const char *path)
{
	struct vfs_mount *mount = mount_for_path(path);

	if (mount != 0) {
		(void)forward_mount_symlink_path(mount, message, reply,
						 target, path);
		return;
	}
	reply->words[0] = BUNIX_VFS_ERR_ACCESS;
}

static void vfs_rename_path(struct bunix_msg *message, struct bunix_msg *reply,
			    const char *old_path, const char *new_path)
{
	struct vfs_mount *old_mount = mount_for_path(old_path);
	struct vfs_mount *new_mount = mount_for_path(new_path);

	if (old_mount == 0 || new_mount == 0) {
		reply->words[0] = old_mount == new_mount ?
				  BUNIX_VFS_ERR_ACCESS : BUNIX_VFS_ERR_XDEV;
		return;
	}
	if (old_mount != new_mount) {
		reply->words[0] = BUNIX_VFS_ERR_XDEV;
		return;
	}
	(void)forward_mount_two_path(old_mount, message, reply, old_path,
				     new_path);
}

static void vfs_link_path(struct bunix_msg *message, struct bunix_msg *reply,
			  const char *old_path, const char *new_path)
{
	struct vfs_mount *old_mount = mount_for_path(old_path);
	struct vfs_mount *new_mount = mount_for_path(new_path);

	if (old_mount == 0 || new_mount == 0) {
		reply->words[0] = old_mount == new_mount ?
				  BUNIX_VFS_ERR_ACCESS : BUNIX_VFS_ERR_XDEV;
		return;
	}
	if (old_mount != new_mount) {
		reply->words[0] = BUNIX_VFS_ERR_XDEV;
		return;
	}
	(void)forward_mount_two_path(old_mount, message, reply, old_path,
				     new_path);
}

static void vfs_readlink_path(struct bunix_msg *message,
			      struct bunix_msg *reply, const char *path)
{
	char resolved[VFS_MAX_PATH];
	u64 error = (u64)-1;
	struct vfs_mount *mount;

	if (vfs_resolve_symlinks(path, resolved, message->words[3] & 0xffffffff,
				 0, &error) != 0) {
		reply->words[0] = error;
		return;
	}
	mount = mount_for_path(resolved);
	if (mount != 0) {
		(void)forward_mount_buffer_path(mount, message, reply, resolved);
		return;
	}

	reply->words[0] = BUNIX_VFS_ERR_NOENT;
}

int main(void)
{
	const char online[] = "vfs: online\n";
	const char ready[] = "vfs: ready\n";
	struct bunix_msg message;

	bunix_console_log(online, sizeof(online) - 1);
	register_service(BUNIX_SERVICE_VFS, BUNIX_HANDLE_SELF);
	bunix_tree_init(&mounts);
	bunix_u64_tree_init(&open_files);
	next_open_id = 1;
	bunix_console_log(ready, sizeof(ready) - 1);

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
		case BUNIX_VFS_OPEN_BUFFER: {
			char cwd[VFS_MAX_PATH];
			char input[VFS_MAX_PATH];
			char path[VFS_MAX_PATH];
			const u64 cwd_len = message.words[0];
			const u64 path_len = message.words[1];
			u64 error = (u64)-1;

			if ((message.cap_rights & BUNIX_RIGHT_RECV) == 0 ||
			    read_path_buffer_at(message.cap, 0, cwd_len,
						cwd) != 0 ||
			    read_path_buffer_at(message.cap, cwd_len, path_len,
						input) != 0 ||
			    vfs_resolve_with_base(message.words[2], cwd,
						  input, path, &error) != 0) {
				reply.words[0] = error;
				break;
			}
			vfs_open_path(&message, &reply, path);
			break;
		}
		case BUNIX_VFS_STAT_PATH_META_BUFFER: {
			char cwd[VFS_MAX_PATH];
			char input[VFS_MAX_PATH];
			char path[VFS_MAX_PATH];
			const u64 cwd_len = message.words[0];
			const u64 path_len = message.words[1];
			u64 error = (u64)-1;

			if ((message.cap_rights & BUNIX_RIGHT_RECV) == 0 ||
			    read_path_buffer_at(message.cap, 0, cwd_len,
						cwd) != 0 ||
			    read_path_buffer_at(message.cap, cwd_len, path_len,
						input) != 0 ||
			    vfs_resolve_with_base(message.words[2], cwd,
						  input, path, &error) != 0) {
				reply.words[0] = error;
				break;
			}
			vfs_stat_path(&message, &reply, path);
			break;
		}
		case BUNIX_VFS_ACCESS_BUFFER: {
			char cwd[VFS_MAX_PATH];
			char input[VFS_MAX_PATH];
			char path[VFS_MAX_PATH];
			const u64 cwd_len = message.words[0];
			const u64 path_len = message.words[1];
			u64 error = (u64)-1;

			if ((message.cap_rights & BUNIX_RIGHT_RECV) == 0 ||
			    read_path_buffer_at(message.cap, 0, cwd_len,
						cwd) != 0 ||
			    read_path_buffer_at(message.cap, cwd_len, path_len,
						input) != 0 ||
			    vfs_resolve_with_base(message.words[2], cwd,
						  input, path, &error) != 0) {
				reply.words[0] = error;
				break;
			}
			vfs_access_path(&message, &reply, path);
			break;
		}
		case BUNIX_VFS_CREATE_BUFFER:
		case BUNIX_VFS_CHMOD_BUFFER:
		case BUNIX_VFS_CHOWN_BUFFER:
		case BUNIX_VFS_MKDIR_BUFFER:
		case BUNIX_VFS_RMDIR_BUFFER:
		case BUNIX_VFS_UNLINK_BUFFER: {
			char cwd[VFS_MAX_PATH];
			char input[VFS_MAX_PATH];
			char path[VFS_MAX_PATH];
			const u64 cwd_len = message.words[0];
			const u64 path_len = message.words[1];
			u64 error = (u64)-1;

			if ((message.cap_rights & BUNIX_RIGHT_RECV) == 0 ||
			    read_path_buffer_at(message.cap, 0, cwd_len,
						cwd) != 0 ||
			    read_path_buffer_at(message.cap, cwd_len, path_len,
						input) != 0 ||
			    vfs_resolve_with_base(message.words[2], cwd,
						  input, path, &error) != 0) {
				reply.words[0] = error;
				break;
			}
			vfs_mutate_path(&message, &reply, path);
			break;
		}
		case BUNIX_VFS_MKNOD_BUFFER: {
			char cwd[VFS_MAX_PATH];
			char input[VFS_MAX_PATH];
			char path[VFS_MAX_PATH];
			const u64 cwd_len = message.words[0];
			const u64 path_len = message.words[1];
			u64 error = (u64)-1;

			if ((message.cap_rights & BUNIX_RIGHT_RECV) == 0 ||
			    read_path_buffer_at(message.cap, 0, cwd_len,
						cwd) != 0 ||
			    read_path_buffer_at(message.cap, cwd_len, path_len,
						input) != 0 ||
			    vfs_resolve_with_base(message.words[2], cwd,
						  input, path, &error) != 0) {
				reply.words[0] = error;
				break;
			}
			vfs_mutate_path(&message, &reply, path);
			break;
		}
		case BUNIX_VFS_SYMLINK_BUFFER: {
			char target[VFS_MAX_PATH];
			char cwd[VFS_MAX_PATH];
			char input[VFS_MAX_PATH];
			char path[VFS_MAX_PATH];
			const u64 target_len = message.words[0];
			const u64 cwd_len = message.words[1];
			const u64 path_len = message.words[2];
			const u64 base_handle = message.words[3] >> 32;
			u64 error = (u64)-1;

			if ((message.cap_rights & BUNIX_RIGHT_RECV) == 0 ||
			    read_path_buffer_at(message.cap, 0, target_len,
						target) != 0 ||
			    read_path_buffer_at(message.cap, target_len,
						cwd_len, cwd) != 0 ||
			    read_path_buffer_at(message.cap,
						target_len + cwd_len,
						path_len, input) != 0 ||
			    target[0] == '\0' ||
			    vfs_resolve_with_base(base_handle, cwd, input,
						  path, &error) != 0) {
				reply.words[0] = error;
				break;
			}
			vfs_symlink_path(&message, &reply, target, path);
			break;
		}
		case BUNIX_VFS_RENAME_BUFFER:
		case BUNIX_VFS_LINK_BUFFER: {
			char old_cwd[VFS_MAX_PATH];
			char old_input[VFS_MAX_PATH];
			char old_path[VFS_MAX_PATH];
			char new_cwd[VFS_MAX_PATH];
			char new_input[VFS_MAX_PATH];
			char new_path[VFS_MAX_PATH];
			const u64 old_base = message.words[0] & 0xffffffff;
			const u64 new_base = message.words[0] >> 32;
			const u64 old_cwd_len = message.words[1] & 0xffffffff;
			const u64 old_path_len = message.words[1] >> 32;
			const u64 new_cwd_len = message.words[2] & 0xffffffff;
			const u64 new_path_len = message.words[2] >> 32;
			u64 error = (u64)-1;
			u64 offset = 0;

			if ((message.cap_rights & BUNIX_RIGHT_RECV) == 0 ||
			    read_path_buffer_at(message.cap, offset,
						old_cwd_len, old_cwd) != 0) {
				reply.words[0] = error;
				break;
			}
			offset += old_cwd_len;
			if (read_path_buffer_at(message.cap, offset,
						old_path_len, old_input) != 0) {
				reply.words[0] = error;
				break;
			}
			offset += old_path_len;
			if (read_path_buffer_at(message.cap, offset,
						new_cwd_len, new_cwd) != 0) {
				reply.words[0] = error;
				break;
			}
			offset += new_cwd_len;
			if (read_path_buffer_at(message.cap, offset,
						new_path_len, new_input) != 0 ||
			    vfs_resolve_with_base(old_base, old_cwd,
						  old_input, old_path,
						  &error) != 0 ||
			    vfs_resolve_with_base(new_base, new_cwd,
						  new_input, new_path,
						  &error) != 0) {
				reply.words[0] = error;
				break;
			}
			if (message.type == BUNIX_VFS_RENAME_BUFFER) {
				vfs_rename_path(&message, &reply, old_path,
						new_path);
			} else {
				vfs_link_path(&message, &reply, old_path,
					      new_path);
			}
			break;
		}
		case BUNIX_VFS_CHMOD: {
			struct vfs_open *open = open_from_handle(message.words[0]);

			if (open != 0 && open->kind == VFS_OPEN_REMOTE) {
				(void)forward_remote_handle(open, &message, &reply);
			} else {
				reply.words[0] = BUNIX_VFS_ERR_ACCESS;
			}
			break;
		}
		case BUNIX_VFS_CHOWN: {
			struct vfs_open *open = open_from_handle(message.words[0]);

			if (open != 0 && open->kind == VFS_OPEN_REMOTE) {
				(void)forward_remote_handle(open, &message, &reply);
			} else {
				reply.words[0] = BUNIX_VFS_ERR_ACCESS;
			}
			break;
		}
		case BUNIX_VFS_TRUNCATE: {
			struct vfs_open *open = open_from_handle(message.words[0]);

			if (message.cap != 0) {
				char cwd[VFS_MAX_PATH];
				char input[VFS_MAX_PATH];
				char path[VFS_MAX_PATH];
				const u64 cwd_len = message.words[0];
				const u64 path_len = message.words[1];
				u64 error = (u64)-1;

				if ((message.cap_rights & BUNIX_RIGHT_RECV) == 0 ||
				    read_path_buffer_at(message.cap, 0, cwd_len,
							cwd) != 0 ||
				    read_path_buffer_at(message.cap, cwd_len,
							path_len, input) != 0 ||
				    vfs_resolve_with_base(message.words[2], cwd,
							  input, path,
							  &error) != 0) {
					reply.words[0] = error;
					break;
				}
				vfs_mutate_path(&message, &reply, path);
				break;
			}
			if (open != 0 && open->kind == VFS_OPEN_REMOTE) {
				(void)forward_remote_handle(open, &message, &reply);
			} else {
				reply.words[0] = BUNIX_VFS_ERR_ACCESS;
			}
			break;
		}
		case BUNIX_VFS_READLINK_BUFFER: {
			char cwd[VFS_MAX_PATH];
			char input[VFS_MAX_PATH];
			char path[VFS_MAX_PATH];
			const u64 cwd_len = message.words[0];
			const u64 path_len = message.words[1];
			u64 error = (u64)-1;

			if ((message.cap_rights & BUNIX_RIGHT_RECV) == 0 ||
			    read_path_buffer_at(message.cap, 0, cwd_len,
						cwd) != 0 ||
			    read_path_buffer_at(message.cap, cwd_len, path_len,
						input) != 0 ||
			    vfs_resolve_with_base(message.words[2], cwd,
						  input, path, &error) != 0) {
				reply.words[0] = error;
				break;
			}
			vfs_readlink_path(&message, &reply, path);
			break;
		}
		case BUNIX_VFS_STAT_META: {
			struct vfs_open *open = open_from_handle(message.words[0]);

			if (open != 0 && open->kind == VFS_OPEN_REMOTE) {
				(void)forward_remote_handle(open, &message, &reply);
			} else {
				reply.words[0] = (u64)-1;
			}
			break;
		}
		case BUNIX_VFS_READ_FILE_BUFFER: {
			struct vfs_open *open = open_from_handle(message.words[0]);

			if (open != 0 && open->kind == VFS_OPEN_REMOTE) {
				(void)forward_remote_handle(open, &message, &reply);
			} else {
				reply.words[0] = (u64)-1;
			}
			break;
		}
		case BUNIX_VFS_WRITE_FILE_BUFFER: {
			struct vfs_open *open = open_from_handle(message.words[0]);

			if (open != 0 && open->kind == VFS_OPEN_REMOTE) {
				(void)forward_remote_handle(open, &message, &reply);
			} else {
				reply.words[0] = BUNIX_VFS_ERR_ACCESS;
			}
			break;
		}
		case BUNIX_VFS_READDIR_BUFFER: {
			struct vfs_open *open = open_from_handle(message.words[0]);

			if (open != 0 && open->kind == VFS_OPEN_REMOTE) {
				(void)forward_remote_handle(open, &message, &reply);
			} else {
				reply.words[0] = (u64)-1;
			}
			break;
		}
		case BUNIX_VFS_CLOSE:
			if (open_from_handle(message.words[0]) != 0 &&
			    open_from_handle(message.words[0])->kind == VFS_OPEN_REMOTE) {
				const u64 local_handle = message.words[0];
				struct vfs_open *open = open_from_handle(message.words[0]);

				if (forward_remote_handle(open, &message, &reply) == 0 &&
				    reply.words[0] == 0) {
					forget_open_file(local_handle);
				}
				break;
			}
			reply.words[0] = (u64)-1;
			break;
			case BUNIX_VFS_MOUNT_BUFFER: {
				char path[VFS_MAX_PATH];
				u64 service;
				u64 status;

				service = resolve_service(message.words[1],
							  BUNIX_RIGHT_SEND |
							  BUNIX_RIGHT_DUP);
				if (service == 0 ||
				    bunix_read_path_cap(&message, path,
							sizeof(path)) != 0) {
					reply.words[0] = (u64)-1;
					break;
				}
				status = mount_translator(path, service,
							  message.words[1]);
				if (status != 0) {
					reply.words[0] = status;
				} else {
					reply.words[0] = 0;
					notify_procfs_mount(path, message.words[1]);
					bunix_console_log("vfs: mounted translator\n", 24);
				}
				break;
			}
			case BUNIX_VFS_RESOLVE_MOUNT_BUFFER: {
				char path[VFS_MAX_PATH];

				if (bunix_read_path_cap(&message, path,
							sizeof(path)) != 0) {
					reply.words[0] = (u64)-1;
					break;
				}
				vfs_reply_mount_route(&reply, path, 0);
				break;
			}
			case BUNIX_VFS_PIN_ROUTE_BUFFER: {
				char path[VFS_MAX_PATH];

				if (bunix_read_path_cap(&message, path,
							sizeof(path)) != 0) {
					reply.words[0] = (u64)-1;
					break;
				}
				vfs_reply_mount_route(&reply, path, 1);
				break;
			}
			case BUNIX_VFS_UNPIN_ROUTE: {
				vfs_unpin_mount_route(&reply, message.words[0]);
				break;
			}
			case BUNIX_VFS_UNMOUNT_BUFFER: {
				char path[VFS_MAX_PATH];
				u64 status;

				if (bunix_read_path_cap(&message, path,
							sizeof(path)) != 0) {
					reply.words[0] = (u64)-1;
					break;
				}
				status = unmount_translator(path);
				if (status != 0) {
					reply.words[0] = status;
				} else {
					reply.words[0] = 0;
					notify_procfs_unmount(path);
				}
				break;
			}
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
