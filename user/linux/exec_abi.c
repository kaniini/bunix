struct linux_exec_args {
	u64 argc;
	u64 envc;
	u64 bytes;
	char **argv;
	char **envp;
};

static void linux_exec_args_free(struct linux_exec_args *args)
{
	if (args == 0) {
		return;
	}
	if (args->argv != 0) {
		for (u64 i = 0; i < args->argc; i++) {
			bunix_free(args->argv[i]);
		}
		bunix_free(args->argv);
	}
	if (args->envp != 0) {
		for (u64 i = 0; i < args->envc; i++) {
			bunix_free(args->envp[i]);
		}
		bunix_free(args->envp);
	}
	args->argc = 0;
	args->envc = 0;
	args->bytes = 0;
	args->argv = 0;
	args->envp = 0;
}

static long linux_exec_vector_push(char ***values, u64 *count, u64 *capacity,
				   char *value)
{
	char **next;
	u64 new_capacity;

	if (values == 0 || count == 0 || capacity == 0 || value == 0 ||
	    *count >= LINUX_EXEC_MAX_POINTERS) {
		return -LINUX_E2BIG;
	}
	if (*count == *capacity) {
		new_capacity = *capacity == 0 ? 8 : *capacity * 2;
		if (new_capacity < *capacity ||
		    new_capacity > LINUX_EXEC_MAX_POINTERS) {
			new_capacity = LINUX_EXEC_MAX_POINTERS;
		}
		if (new_capacity == *capacity) {
			return -LINUX_E2BIG;
		}
		next = (char **)bunix_calloc(new_capacity, sizeof(next[0]));
		if (next == 0) {
			return -LINUX_ENOMEM;
		}
		if (*values != 0) {
			for (u64 i = 0; i < *count; i++) {
				next[i] = (*values)[i];
			}
			bunix_free(*values);
		}
		*values = next;
		*capacity = new_capacity;
	}
	(*values)[*count] = value;
	(*count)++;
	return 0;
}

static long linux_exec_buffer_read_u64(u64 buffer, u64 offset, u64 *value)
{
	return bunix_buffer_read(buffer, offset, value, sizeof(*value));
}

static long linux_exec_buffer_write_u64(u64 buffer, u64 offset, u64 value)
{
	return bunix_buffer_write(buffer, offset, &value, sizeof(value));
}

static long linux_exec_deserialize_one(u64 buffer, u64 limit, u64 *offset,
				       char **out)
{
	u64 len = 0;
	char *copy;

	if (buffer == 0 || offset == 0 || out == 0 ||
	    *offset + sizeof(len) < *offset ||
	    *offset + sizeof(len) > limit ||
	    linux_exec_buffer_read_u64(buffer, *offset, &len) != 0 ||
	    len == 0 || len > LINUX_EXEC_MAX_STRING ||
	    *offset + sizeof(len) + len < *offset ||
	    *offset + sizeof(len) + len > limit) {
		return -LINUX_EINVAL;
	}
	copy = (char *)bunix_alloc(len);
	if (copy == 0) {
		return -LINUX_ENOMEM;
	}
	if (bunix_buffer_read(buffer, *offset + sizeof(len), copy, len) != 0 ||
	    copy[len - 1] != '\0') {
		bunix_free(copy);
		return -LINUX_EINVAL;
	}
	*offset += sizeof(len) + len;
	*out = copy;
	return 0;
}

static long linux_exec_deserialize_args(u64 buffer, u64 offset, u64 len,
					struct linux_exec_args *args)
{
	const u64 limit = offset + len;
	u64 argc = 0;
	u64 envc = 0;
	u64 argv_capacity = 0;
	u64 env_capacity = 0;
	long result = 0;

	if (args == 0 || len < 2 * sizeof(u64) ||
	    limit < offset || limit > LINUX_EXEC_PREPARE_BUFFER_SIZE ||
	    linux_exec_buffer_read_u64(buffer, offset, &argc) != 0 ||
	    linux_exec_buffer_read_u64(buffer, offset + sizeof(u64),
				       &envc) != 0 ||
	    argc == 0 || argc > LINUX_EXEC_MAX_POINTERS ||
	    envc > LINUX_EXEC_MAX_POINTERS) {
		return -LINUX_EINVAL;
	}
	offset += 2 * sizeof(u64);
	for (u64 i = 0; i < argc; i++) {
		char *copy = 0;
		long push;

		result = linux_exec_deserialize_one(buffer, limit, &offset,
						    &copy);
		if (result != 0) {
			goto fail;
		}
		push = linux_exec_vector_push(&args->argv, &args->argc,
					      &argv_capacity, copy);
		if (push != 0) {
			bunix_free(copy);
			result = push;
			goto fail;
		}
		args->bytes += string_len(copy) + 1;
	}
	for (u64 i = 0; i < envc; i++) {
		char *copy = 0;
		long push;

		result = linux_exec_deserialize_one(buffer, limit, &offset,
						    &copy);
		if (result != 0) {
			goto fail;
		}
		push = linux_exec_vector_push(&args->envp, &args->envc,
					      &env_capacity, copy);
		if (push != 0) {
			bunix_free(copy);
			result = push;
			goto fail;
		}
		args->bytes += string_len(copy) + 1;
	}
	if (offset != limit || args->bytes > LINUX_EXEC_MAX_STRING_BYTES) {
		result = -LINUX_EINVAL;
		goto fail;
	}
	return 0;

fail:
	linux_exec_args_free(args);
	return result;
}

static long linux_exec_serialize_one(u64 buffer, u64 *offset,
				     const char *value, u64 *bytes)
{
	const u64 len = value == 0 ? 0 : string_len(value) + 1;

	if (len == 0 || len > LINUX_EXEC_MAX_STRING ||
	    *bytes + len < *bytes ||
	    *bytes + len > LINUX_EXEC_MAX_STRING_BYTES ||
	    *offset + sizeof(len) < *offset ||
	    *offset + sizeof(len) + len < *offset ||
	    *offset + sizeof(len) + len > LINUX_EXEC_PREPARE_BUFFER_SIZE) {
		return -LINUX_E2BIG;
	}
	if (linux_exec_buffer_write_u64(buffer, *offset, len) != 0 ||
	    bunix_buffer_write(buffer, *offset + sizeof(len), value,
			       len) != 0) {
		return -LINUX_EFAULT;
	}
	*offset += sizeof(len) + len;
	*bytes += len;
	return 0;
}

static long linux_exec_serialize_args(u64 buffer, u64 offset,
				      const struct linux_exec_args *args,
				      u64 *len_out)
{
	const u64 start = offset;
	u64 bytes = 0;

	if (buffer == 0 || args == 0 || len_out == 0 ||
	    args->argc == 0 ||
	    args->argc > LINUX_EXEC_MAX_POINTERS ||
	    args->envc > LINUX_EXEC_MAX_POINTERS ||
	    offset + 2 * sizeof(u64) < offset ||
	    offset + 2 * sizeof(u64) > LINUX_EXEC_PREPARE_BUFFER_SIZE) {
		return -LINUX_EINVAL;
	}
	if (linux_exec_buffer_write_u64(buffer, offset, args->argc) != 0 ||
	    linux_exec_buffer_write_u64(buffer, offset + sizeof(u64),
				       args->envc) != 0) {
		return -LINUX_EFAULT;
	}
	offset += 2 * sizeof(u64);
	for (u64 i = 0; i < args->argc; i++) {
		const long result = linux_exec_serialize_one(buffer, &offset,
							    args->argv[i],
							    &bytes);

		if (result != 0) {
			return result;
		}
	}
	for (u64 i = 0; i < args->envc; i++) {
		const long result = linux_exec_serialize_one(buffer, &offset,
							    args->envp[i],
							    &bytes);

		if (result != 0) {
			return result;
		}
	}
	*len_out = offset - start;
	return 0;
}

static char *linux_exec_dup_text(const char *text)
{
	const u64 len = text == 0 ? 0 : string_len(text) + 1;
	char *copy;

	if (len == 0 || len > LINUX_EXEC_MAX_STRING) {
		return 0;
	}
	copy = (char *)bunix_alloc(len);
	if (copy == 0) {
		return 0;
	}
	for (u64 i = 0; i < len; i++) {
		copy[i] = text[i];
	}
	return copy;
}

static long linux_exec_parse_shebang(const unsigned char *image,
				     u64 image_size, char **interp_out,
				     char **arg_out)
{
	u64 end = 0;
	u64 pos = 2;
	u64 interp_start;
	u64 interp_end;
	u64 arg_start;
	u64 arg_end;
	char *interp;
	char *arg = 0;

	if (interp_out == 0 || arg_out == 0) {
		return -LINUX_EFAULT;
	}
	*interp_out = 0;
	*arg_out = 0;
	if (image == 0 || image_size < 2 || image[0] != '#' ||
	    image[1] != '!') {
		return -LINUX_EINVAL;
	}
	while (end < image_size && end < LINUX_SHEBANG_MAX &&
	       image[end] != '\n' && image[end] != '\r' &&
	       image[end] != '\0') {
		end++;
	}
	while (pos < end && (image[pos] == ' ' || image[pos] == '\t')) {
		pos++;
	}
	interp_start = pos;
	while (pos < end && image[pos] != ' ' && image[pos] != '\t') {
		pos++;
	}
	interp_end = pos;
	if (interp_start == interp_end ||
	    image[interp_start] != '/' ||
	    interp_end - interp_start >= LINUX_MAX_PATH) {
		return -LINUX_EINVAL;
	}
	interp = (char *)bunix_alloc(interp_end - interp_start + 1);
	if (interp == 0) {
		return -LINUX_ENOMEM;
	}
	for (u64 i = 0; i < interp_end - interp_start; i++) {
		interp[i] = (char)image[interp_start + i];
	}
	interp[interp_end - interp_start] = '\0';

	while (pos < end && (image[pos] == ' ' || image[pos] == '\t')) {
		pos++;
	}
	arg_start = pos;
	arg_end = end;
	while (arg_end > arg_start &&
	       (image[arg_end - 1] == ' ' || image[arg_end - 1] == '\t')) {
		arg_end--;
	}
	if (arg_end > arg_start) {
		arg = (char *)bunix_alloc(arg_end - arg_start + 1);
		if (arg == 0) {
			bunix_free(interp);
			return -LINUX_ENOMEM;
		}
		for (u64 i = 0; i < arg_end - arg_start; i++) {
			arg[i] = (char)image[arg_start + i];
		}
		arg[arg_end - arg_start] = '\0';
	}

	*interp_out = interp;
	*arg_out = arg;
	return 0;
}

static long linux_exec_rewrite_shebang_args(struct linux_exec_args *args,
					    const char *script_path,
					    char *interp, char *interp_arg)
{
	const u64 tail = args->argc > 1 ? args->argc - 1 : 0;
	const u64 prefix = interp_arg == 0 ? 2 : 3;
	const u64 new_argc = prefix + tail;
	char **next;
	char *script_copy;
	u64 new_bytes = 0;
	u64 index = 0;

	if (args == 0 || script_path == 0 || interp == 0 ||
	    new_argc > LINUX_EXEC_MAX_POINTERS) {
		return -LINUX_EINVAL;
	}
	next = (char **)bunix_calloc(new_argc, sizeof(next[0]));
	if (next == 0) {
		return -LINUX_ENOMEM;
	}
	script_copy = linux_exec_dup_text(script_path);
	if (script_copy == 0) {
		bunix_free(next);
		return -LINUX_ENOMEM;
	}
	new_bytes += string_len(interp) + 1;
	if (interp_arg != 0) {
		new_bytes += string_len(interp_arg) + 1;
	}
	new_bytes += string_len(script_copy) + 1;
	for (u64 i = 1; i < args->argc; i++) {
		new_bytes += string_len(args->argv[i]) + 1;
	}
	for (u64 i = 0; i < args->envc; i++) {
		new_bytes += string_len(args->envp[i]) + 1;
	}
	if (new_bytes > LINUX_EXEC_MAX_STRING_BYTES) {
		bunix_free(script_copy);
		bunix_free(next);
		return -LINUX_E2BIG;
	}

	next[index++] = interp;
	if (interp_arg != 0) {
		next[index++] = interp_arg;
	}
	next[index++] = script_copy;
	for (u64 i = 1; i < args->argc; i++) {
		next[index++] = args->argv[i];
		args->argv[i] = 0;
	}
	if (args->argc != 0) {
		bunix_free(args->argv[0]);
	}
	bunix_free(args->argv);
	args->argv = next;
	args->argc = new_argc;
	args->bytes = new_bytes;
	return 0;
}
