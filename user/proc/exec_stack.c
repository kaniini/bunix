static long build_initial_stack(u64 task, const char *path,
				const struct exec_info *exec,
				u64 interpreter_base,
				const struct exec_strings *strings,
				const struct exec_credentials *creds,
				const struct exec_handles *handles,
				u64 *stack)
{
	const struct exec_credentials root_creds = { 0, 0, 0, 0, 0 };
	const struct exec_handles empty_handles = { 0, 0, 0, 0, 0 };
	const u64 stack_base = USER_STACK_TOP - PROC_INIT_STACK_MAX;
	const u64 path_len = str_len(path);
	const u64 phdr_size = exec != 0 ? exec->phnum * exec->phent : 0;
	const u64 auxv_pairs = interpreter_base != 0 ? 20 : 19;
	const u64 argc = strings != 0 && strings->argc != 0 ?
			strings->argc : 1;
	const u64 envc = strings != 0 ? strings->envc : 0;
	const u64 stack_words = 1 + argc + 1 + envc + 1 + auxv_pairs * 2;
	const unsigned char random_bytes[16] = {
		0x62, 0x75, 0x6e, 0x69, 0x78, 0x2d, 0x6d, 0x75,
		0x73, 0x6c, 0x2d, 0x72, 0x61, 0x6e, 0x64, 0x00,
	};
	u64 *argv_addrs = 0;
	u64 *env_addrs = 0;
	unsigned char *init_stack = 0;
	u64 sp = PROC_INIT_STACK_MAX;
	long result = -1;

	if (creds == 0) {
		creds = &root_creds;
	}
	if (handles == 0) {
		handles = &empty_handles;
	}
	if (exec == 0 ||
	    exec->phdrs == 0 ||
	    phdr_size == 0) {
		return -1;
	}

	argv_addrs = (u64 *)bunix_alloc(argc * sizeof(u64));
	env_addrs = envc == 0 ? 0 : (u64 *)bunix_alloc(envc * sizeof(u64));
	if (argv_addrs == 0 || (envc != 0 && env_addrs == 0)) {
		goto out;
	}

	init_stack = (unsigned char *)bunix_alloc(PROC_INIT_STACK_MAX);
	if (init_stack == 0) {
		goto out;
	}
	mem_zero(init_stack, PROC_INIT_STACK_MAX);
	for (u64 i = envc; i > 0; i--) {
		const char *value = strings->envp[i - 1];
		const u64 len = str_len(value) + 1;

		if (len > sp) {
			goto out;
		}
		sp -= len;
		env_addrs[i - 1] = stack_base + sp;
		mem_copy(init_stack + sp, (const unsigned char *)value, len);
	}
	for (u64 i = argc; i > 0; i--) {
		const char *value = strings != 0 && strings->argc != 0 ?
				    strings->argv[i - 1] : path;
		const u64 len = str_len(value) + 1;

		if (len > sp) {
			goto out;
		}
		sp -= len;
		argv_addrs[i - 1] = stack_base + sp;
		mem_copy(init_stack + sp, (const unsigned char *)value, len);
	}
	if (path_len + 1 > sp) {
		goto out;
	}
	sp -= path_len + 1;
	const u64 execfn = stack_base + sp;
	mem_copy(init_stack + sp, (const unsigned char *)path, path_len + 1);

	sp = align_down(sp, 16);
	if (sizeof(random_bytes) > sp) {
		goto out;
	}
	sp -= sizeof(random_bytes);
	const u64 random_addr = stack_base + sp;
	mem_copy(init_stack + sp, random_bytes, sizeof(random_bytes));

	sp = align_down(sp, 8);
	if (phdr_size > sp) {
		goto out;
	}
	sp -= phdr_size;
	const u64 copied_phdr_addr = stack_base + sp;
	mem_copy(init_stack + sp, (const unsigned char *)exec->phdrs,
		 phdr_size);
	const u64 phdr_addr = exec->phdr_addr != 0 ?
			       exec->phdr_addr :
			       copied_phdr_addr;

	sp = align_down(sp, 16);
	if (stack_words * sizeof(u64) > sp) {
		goto out;
	}
	sp -= stack_words * sizeof(u64);
	u64 *words = (u64 *)(init_stack + sp);
	u64 word = 0;

	words[word++] = argc;
	for (u64 i = 0; i < argc; i++) {
		words[word++] = argv_addrs[i];
	}
	words[word++] = 0;
	for (u64 i = 0; i < envc; i++) {
		words[word++] = env_addrs[i];
	}
	words[word++] = 0;
	words[word++] = AT_PAGESZ;
	words[word++] = 4096;
	words[word++] = AT_ENTRY;
	words[word++] = exec->entry;
	if (interpreter_base != 0) {
		words[word++] = AT_BASE;
		words[word++] = interpreter_base;
	}
	words[word++] = AT_PHDR;
	words[word++] = phdr_addr;
	words[word++] = AT_PHENT;
	words[word++] = exec->phent;
	words[word++] = AT_PHNUM;
	words[word++] = exec->phnum;
	words[word++] = AT_EXECFN;
	words[word++] = execfn;
	words[word++] = AT_UID;
	words[word++] = creds->uid;
	words[word++] = AT_EUID;
	words[word++] = creds->euid;
	words[word++] = AT_GID;
	words[word++] = creds->gid;
	words[word++] = AT_EGID;
	words[word++] = creds->egid;
	words[word++] = AT_SECURE;
	words[word++] = creds->secure;
	words[word++] = AT_RANDOM;
	words[word++] = random_addr;
	words[word++] = AT_CLKTCK;
	words[word++] = 100;
	words[word++] = BUNIX_AT_STDOUT;
	words[word++] = handles->stdout_handle;
	words[word++] = BUNIX_AT_STDERR;
	words[word++] = handles->stderr_handle;
	words[word++] = BUNIX_AT_TIME;
	words[word++] = handles->time_handle;
	words[word++] = BUNIX_AT_PROC;
	words[word++] = handles->proc_handle;
	words[word++] = BUNIX_AT_NAMES;
	words[word++] = handles->names_handle;
	words[word++] = AT_NULL;
	words[word++] = 0;

	if (task_write_bytes(task, stack_base + sp, init_stack + sp,
			     PROC_INIT_STACK_MAX - sp) != 0) {
		goto out;
	}

	*stack = stack_base + sp;
	result = 0;

out:
	bunix_free(argv_addrs);
	bunix_free(env_addrs);
	bunix_free(init_stack);
	return result;
}
