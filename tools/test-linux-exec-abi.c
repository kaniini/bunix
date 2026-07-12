#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint64_t u64;

enum {
	LINUX_E2BIG = 7,
	LINUX_EFAULT = 14,
	LINUX_EINVAL = 22,
	LINUX_ENOMEM = 12,
	LINUX_MAX_PATH = 4096,
	LINUX_EXEC_MAX_STRING = 128 * 1024,
	LINUX_EXEC_MAX_STRING_BYTES = 384 * 1024,
	LINUX_EXEC_MAX_POINTERS = LINUX_EXEC_MAX_STRING_BYTES / 8,
	LINUX_SHEBANG_MAX = 256,
	LINUX_EXEC_PREPARE_PATH_OFF = 0,
	LINUX_EXEC_PREPARE_IMAGE_OFF = LINUX_MAX_PATH,
	LINUX_EXEC_PREPARE_VECTOR_OFF = LINUX_MAX_PATH + LINUX_SHEBANG_MAX,
	LINUX_EXEC_PREPARE_BUFFER_SIZE =
		LINUX_EXEC_PREPARE_VECTOR_OFF + LINUX_EXEC_MAX_STRING_BYTES +
		(LINUX_EXEC_MAX_POINTERS * sizeof(u64)) + 2 * sizeof(u64),
};

struct test_buffer {
	unsigned char *data;
	u64 size;
};

static u64 string_len(const char *text)
{
	return strlen(text);
}

static void *bunix_alloc(u64 size)
{
	return malloc((size_t)size);
}

static void *bunix_calloc(u64 count, u64 size)
{
	return calloc((size_t)count, (size_t)size);
}

static void bunix_free(void *ptr)
{
	free(ptr);
}

static int bunix_buffer_read(u64 handle, u64 offset, void *dst, u64 len)
{
	struct test_buffer *buffer = (struct test_buffer *)(uintptr_t)handle;

	if (buffer == NULL || dst == NULL || offset > buffer->size ||
	    len > buffer->size - offset) {
		return -1;
	}
	memcpy(dst, buffer->data + offset, (size_t)len);
	return 0;
}

static int bunix_buffer_write(u64 handle, u64 offset, const void *src, u64 len)
{
	struct test_buffer *buffer = (struct test_buffer *)(uintptr_t)handle;

	if (buffer == NULL || src == NULL || offset > buffer->size ||
	    len > buffer->size - offset) {
		return -1;
	}
	memcpy(buffer->data + offset, src, (size_t)len);
	return 0;
}

#include "../user/linux/exec_abi.c"

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

static int test_vector_roundtrip(void)
{
	unsigned char storage[LINUX_EXEC_PREPARE_BUFFER_SIZE];
	struct test_buffer buffer = { storage, sizeof(storage) };
	struct linux_exec_args args = { 0, 0, 0, 0, 0 };
	struct linux_exec_args out = { 0, 0, 0, 0, 0 };
	u64 argc_cap = 0;
	u64 envc_cap = 0;
	u64 len = 0;
	int failed = 0;

	memset(storage, 0, sizeof(storage));
	if (linux_exec_vector_push(&args.argv, &args.argc, &argc_cap,
				   linux_exec_dup_text("/bin/busybox")) != 0 ||
	    linux_exec_vector_push(&args.argv, &args.argc, &argc_cap,
				   linux_exec_dup_text("echo")) != 0 ||
	    linux_exec_vector_push(&args.argv, &args.argc, &argc_cap,
				   linux_exec_dup_text("hello")) != 0 ||
	    linux_exec_vector_push(&args.envp, &args.envc, &envc_cap,
				   linux_exec_dup_text("PATH=/bin")) != 0 ||
	    linux_exec_vector_push(&args.envp, &args.envc, &envc_cap,
				   linux_exec_dup_text("TERM=bunix")) != 0) {
		fprintf(stderr, "vector push failed\n");
		failed = 1;
		goto out;
	}
	if (linux_exec_serialize_args((u64)(uintptr_t)&buffer,
				      LINUX_EXEC_PREPARE_VECTOR_OFF, &args,
				      &len) != 0 ||
	    linux_exec_deserialize_args((u64)(uintptr_t)&buffer,
					LINUX_EXEC_PREPARE_VECTOR_OFF, len,
					&out) != 0) {
		fprintf(stderr, "vector roundtrip failed\n");
		failed = 1;
		goto out;
	}
	failed |= out.argc != 3 || out.envc != 2;
	failed |= expect_string(out.argv[0], "/bin/busybox", "argv0");
	failed |= expect_string(out.argv[1], "echo", "argv1");
	failed |= expect_string(out.argv[2], "hello", "argv2");
	failed |= expect_string(out.envp[0], "PATH=/bin", "env0");
	failed |= expect_string(out.envp[1], "TERM=bunix", "env1");

out:
	linux_exec_args_free(&out);
	linux_exec_args_free(&args);
	return failed;
}

static int test_shebang_rewrite(void)
{
	const unsigned char image[] = "#! /bin/busybox sh -e \n";
	struct linux_exec_args args = { 0, 0, 0, 0, 0 };
	u64 argc_cap = 0;
	char *interp = NULL;
	char *interp_arg = NULL;
	int failed = 0;

	if (linux_exec_vector_push(&args.argv, &args.argc, &argc_cap,
				   linux_exec_dup_text("/tmp/script")) != 0 ||
	    linux_exec_vector_push(&args.argv, &args.argc, &argc_cap,
				   linux_exec_dup_text("tail")) != 0) {
		fprintf(stderr, "shebang argv setup failed\n");
		failed = 1;
		goto out;
	}
	if (linux_exec_parse_shebang(image, sizeof(image), &interp,
				     &interp_arg) != 0 ||
	    linux_exec_rewrite_shebang_args(&args, "/tmp/script", interp,
					    interp_arg) != 0) {
		fprintf(stderr, "shebang rewrite failed\n");
		bunix_free(interp);
		bunix_free(interp_arg);
		failed = 1;
		goto out;
	}
	interp = NULL;
	interp_arg = NULL;
	failed |= args.argc != 4;
	failed |= expect_string(args.argv[0], "/bin/busybox", "shebang argv0");
	failed |= expect_string(args.argv[1], "sh -e", "shebang argv1");
	failed |= expect_string(args.argv[2], "/tmp/script", "shebang argv2");
	failed |= expect_string(args.argv[3], "tail", "shebang argv3");

out:
	linux_exec_args_free(&args);
	return failed;
}

static int test_invalid_shebang(void)
{
	char *interp = NULL;
	char *interp_arg = NULL;
	const unsigned char relative[] = "#!bin/sh\n";

	if (linux_exec_parse_shebang(relative, sizeof(relative), &interp,
				     &interp_arg) != -LINUX_EINVAL) {
		fprintf(stderr, "relative shebang was accepted\n");
		bunix_free(interp);
		bunix_free(interp_arg);
		return 1;
	}
	return 0;
}

int main(void)
{
	if (test_vector_roundtrip() != 0 ||
	    test_shebang_rewrite() != 0 ||
	    test_invalid_shebang() != 0) {
		return 1;
	}
	printf("linux exec abi tests ok\n");
	return 0;
}
