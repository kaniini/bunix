#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../user/include/bunix/syscall.h"

typedef uint64_t u64;

enum {
	PROC_INIT_STACK_MAX = 512 * 1024,
	USER_STACK_TOP = 0x800000,
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
};

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

static unsigned char captured_stack[PROC_INIT_STACK_MAX];
static u64 captured_addr;
static u64 captured_len;

static u64 str_len(const char *text)
{
	return strlen(text);
}

static u64 align_down(u64 value, u64 align)
{
	return value & ~(align - 1);
}

static void mem_zero(void *ptr, u64 len)
{
	memset(ptr, 0, (size_t)len);
}

static void mem_copy(void *dst, const void *src, u64 len)
{
	memcpy(dst, src, (size_t)len);
}

static void *bunix_alloc(u64 size)
{
	return malloc((size_t)size);
}

static void bunix_free(void *ptr)
{
	free(ptr);
}

static int task_write_bytes(u64 task, u64 addr, const void *src, u64 len)
{
	(void)task;
	if (src == NULL || len > sizeof(captured_stack)) {
		return -1;
	}
	memcpy(captured_stack, src, (size_t)len);
	captured_addr = addr;
	captured_len = len;
	return 0;
}

#include "../user/proc/exec_stack.c"

static const char *guest_string(u64 addr)
{
	if (addr < captured_addr || addr >= captured_addr + captured_len) {
		return NULL;
	}
	return (const char *)captured_stack + (addr - captured_addr);
}

static u64 aux_value(const u64 *aux, u64 type)
{
	for (u64 i = 0; aux[i] != AT_NULL; i += 2) {
		if (aux[i] == type) {
			return aux[i + 1];
		}
	}
	return 0;
}

static int expect_u64(u64 got, u64 expected, const char *label)
{
	if (got != expected) {
		fprintf(stderr, "%s: got=0x%llx expected=0x%llx\n", label,
			(unsigned long long)got, (unsigned long long)expected);
		return 1;
	}
	return 0;
}

static int expect_string(const char *got, const char *expected,
			 const char *label)
{
	if (got == NULL || strcmp(got, expected) != 0) {
		fprintf(stderr, "%s: got=%s expected=%s\n", label,
			got == NULL ? "(null)" : got, expected);
		return 1;
	}
	return 0;
}

static int test_stack_layout(void)
{
	struct elf64_phdr phdrs[2] = {
		{ .type = 1, .flags = 5, .offset = 0, .vaddr = 0x400000 },
		{ .type = 1, .flags = 4, .offset = 0x1000, .vaddr = 0x401000 },
	};
	struct exec_info exec = {
		.entry = 0x401234,
		.phent = sizeof(phdrs[0]),
		.phnum = 2,
		.phdr_addr = 0x400040,
		.phdrs = phdrs,
	};
	char *argv[] = { "/bin/hello", "arg1" };
	char *envp[] = { "PATH=/bin", "TERM=bunix" };
	struct exec_strings strings = { 2, 2, argv, envp };
	struct exec_credentials creds = { 1000, 1001, 1002, 1003, 1 };
	struct exec_handles handles = { 11, 12, 13, 14, 15 };
	u64 stack = 0;
	u64 *words;
	u64 *aux;
	u64 phdr_addr;
	int failed = 0;

	memset(captured_stack, 0, sizeof(captured_stack));
	captured_addr = 0;
	captured_len = 0;
	if (build_initial_stack(99, "/bin/hello", &exec, 0x600000,
				&strings, &creds, &handles, &stack) != 0) {
		fprintf(stderr, "build_initial_stack failed\n");
		return 1;
	}
	if (captured_addr != stack || captured_len == 0 ||
	    stack < USER_STACK_TOP - PROC_INIT_STACK_MAX ||
	    stack >= USER_STACK_TOP || (stack & 7) != 0) {
		fprintf(stderr, "invalid captured stack addr=0x%llx len=%llu stack=0x%llx\n",
			(unsigned long long)captured_addr,
			(unsigned long long)captured_len,
			(unsigned long long)stack);
		return 1;
	}

	words = (u64 *)captured_stack;
	failed |= expect_u64(words[0], 2, "argc");
	failed |= expect_string(guest_string(words[1]), "/bin/hello",
				"argv0");
	failed |= expect_string(guest_string(words[2]), "arg1", "argv1");
	failed |= expect_u64(words[3], 0, "argv terminator");
	failed |= expect_string(guest_string(words[4]), "PATH=/bin", "env0");
	failed |= expect_string(guest_string(words[5]), "TERM=bunix",
				"env1");
	failed |= expect_u64(words[6], 0, "env terminator");
	aux = &words[7];
	phdr_addr = aux_value(aux, AT_PHDR);
	failed |= expect_u64(aux_value(aux, AT_PAGESZ), 4096, "AT_PAGESZ");
	failed |= expect_u64(aux_value(aux, AT_ENTRY), 0x401234, "AT_ENTRY");
	failed |= expect_u64(aux_value(aux, AT_BASE), 0x600000, "AT_BASE");
	failed |= expect_u64(phdr_addr, 0x400040, "AT_PHDR");
	failed |= expect_u64(aux_value(aux, AT_PHENT), sizeof(phdrs[0]),
			     "AT_PHENT");
	failed |= expect_u64(aux_value(aux, AT_PHNUM), 2, "AT_PHNUM");
	failed |= expect_string(guest_string(aux_value(aux, AT_EXECFN)),
				"/bin/hello", "AT_EXECFN");
	failed |= expect_u64(aux_value(aux, AT_UID), 1000, "AT_UID");
	failed |= expect_u64(aux_value(aux, AT_EUID), 1002, "AT_EUID");
	failed |= expect_u64(aux_value(aux, AT_GID), 1001, "AT_GID");
	failed |= expect_u64(aux_value(aux, AT_EGID), 1003, "AT_EGID");
	failed |= expect_u64(aux_value(aux, AT_SECURE), 1, "AT_SECURE");
	failed |= expect_string(guest_string(aux_value(aux, AT_RANDOM)),
				"bunix-musl-rand", "AT_RANDOM");
	failed |= expect_u64(aux_value(aux, AT_CLKTCK), 100, "AT_CLKTCK");
	failed |= expect_u64(aux_value(aux, BUNIX_AT_STDOUT), 11,
			     "BUNIX_AT_STDOUT");
	failed |= expect_u64(aux_value(aux, BUNIX_AT_STDERR), 12,
			     "BUNIX_AT_STDERR");
	failed |= expect_u64(aux_value(aux, BUNIX_AT_TIME), 13,
			     "BUNIX_AT_TIME");
	failed |= expect_u64(aux_value(aux, BUNIX_AT_PROC), 14,
			     "BUNIX_AT_PROC");
	failed |= expect_u64(aux_value(aux, BUNIX_AT_NAMES), 15,
			     "BUNIX_AT_NAMES");
	return failed;
}

static int test_static_pie_phdr(void)
{
	struct elf64_phdr phdrs[3] = {
		{ .type = 1, .flags = 4, .offset = 0, .vaddr = 0 },
		{ .type = 2, .flags = 6, .offset = 0x1de10,
		  .vaddr = 0x1ee10 },
		{ .type = 1, .flags = 5, .offset = 0x1000,
		  .vaddr = 0x1000 },
	};
	struct exec_info exec = {
		.entry = 0x40117c,
		.phent = sizeof(phdrs[0]),
		.phnum = 3,
		.phdr_addr = 0x400040,
		.phdrs = phdrs,
	};
	u64 stack = 0;
	u64 *words;
	u64 *aux;
	int failed = 0;

	memset(captured_stack, 0, sizeof(captured_stack));
	captured_addr = 0;
	captured_len = 0;
	if (build_initial_stack(99, "/sbin/ifquery", &exec, 0,
				NULL, NULL, NULL, &stack) != 0) {
		fprintf(stderr, "static pie build_initial_stack failed\n");
		return 1;
	}
	words = (u64 *)captured_stack;
	aux = &words[4];
	failed |= expect_u64(aux_value(aux, AT_ENTRY), 0x40117c,
			     "static pie AT_ENTRY");
	failed |= expect_u64(aux_value(aux, AT_BASE), 0,
			     "static pie AT_BASE");
	failed |= expect_u64(aux_value(aux, AT_PHDR), 0x400040,
			     "static pie AT_PHDR");
	failed |= expect_u64(aux_value(aux, AT_PHNUM), 3,
			     "static pie AT_PHNUM");
	return failed;
}

static int test_default_strings(void)
{
	struct elf64_phdr phdr = { .type = 1, .flags = 5 };
	struct exec_info exec = {
		.entry = 0x1000,
		.phent = sizeof(phdr),
		.phnum = 1,
		.phdr_addr = 0,
		.phdrs = &phdr,
	};
	u64 stack = 0;
	u64 *words;

	memset(captured_stack, 0, sizeof(captured_stack));
	if (build_initial_stack(99, "/bin/default", &exec, 0, NULL, NULL,
				NULL, &stack) != 0) {
		fprintf(stderr, "default build_initial_stack failed\n");
		return 1;
	}
	words = (u64 *)captured_stack;
	if (words[0] != 1 ||
	    expect_string(guest_string(words[1]), "/bin/default",
			  "default argv0") != 0 ||
	    words[2] != 0 || words[3] != 0) {
		fprintf(stderr, "default stack argv/env layout failed\n");
		return 1;
	}
	return 0;
}

int main(void)
{
	if (test_stack_layout() != 0 || test_static_pie_phdr() != 0 ||
	    test_default_strings() != 0) {
		return 1;
	}
	printf("proc exec stack tests ok\n");
	return 0;
}
