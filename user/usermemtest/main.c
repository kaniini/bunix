#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

static void write_text(int fd, const char *text)
{
	size_t len = 0;

	while (text[len] != '\0') {
		len++;
	}
	(void)write(fd, text, len);
}

static int expect_efault(const char *name, ssize_t result)
{
	if (result == -1 && errno == EFAULT) {
		return 0;
	}
	write_text(STDERR_FILENO, "usermemtest: ");
	write_text(STDERR_FILENO, name);
	write_text(STDERR_FILENO, " expected EFAULT\n");
	return -1;
}

static long raw_fstat(int fd, void *statbuf)
{
	return syscall(SYS_fstat, fd, statbuf);
}

static int test_bad_user_buffers(void)
{
	static const uintptr_t low_addr = 0x1000;
#if defined(__x86_64__)
	static const uintptr_t kernel_addr = 0xffff800000000000ull;
#else
	static const uintptr_t kernel_addr = 0xfffffffffff00000ull;
#endif
	struct stat st;

	errno = 0;
	if (expect_efault("write low",
			  write(STDOUT_FILENO, (const void *)low_addr, 1)) !=
	    0) {
		return 21;
	}

	errno = 0;
	if (expect_efault("write kernel",
			  write(STDOUT_FILENO, (const void *)kernel_addr, 1)) !=
	    0) {
		return 22;
	}

	errno = 0;
	if (expect_efault("fstat low",
			  raw_fstat(STDOUT_FILENO, (void *)low_addr)) !=
	    0) {
		return 24;
	}
	if (raw_fstat(STDOUT_FILENO, &st) != 0) {
		write_text(STDERR_FILENO, "usermemtest: fstat stdout failed\n");
		return 25;
	}
	return 0;
}

static int test_read_only_user_buffer(void)
{
	void *page = mmap(0, 4096, PROT_READ | PROT_WRITE,
			  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	if (page == MAP_FAILED) {
		write_text(STDERR_FILENO, "usermemtest: mmap ro page failed\n");
		return -1;
	}
	memset(page, 0x5a, 4096);
	if (mprotect(page, 4096, PROT_READ) != 0) {
		write_text(STDERR_FILENO, "usermemtest: mprotect ro page failed\n");
		munmap(page, 4096);
		return -1;
	}

	errno = 0;
	if (expect_efault("fstat ro", raw_fstat(STDOUT_FILENO, page)) != 0) {
		munmap(page, 4096);
		return -1;
	}

	munmap(page, 4096);
	return 0;
}

static int test_private_mapping_copy(void)
{
#if defined(__riscv)
	write_text(STDOUT_FILENO,
		   "usermemtest: skipping fork copy check on riscv64\n");
	return 0;
#else
	unsigned char *page = mmap(0, 4096, PROT_READ | PROT_WRITE,
				   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	int status = 0;
	pid_t child;

	if (page == MAP_FAILED) {
		write_text(STDERR_FILENO, "usermemtest: mmap private page failed\n");
		return -1;
	}
	page[0] = 0x31;
	page[4095] = 0x32;

	child = fork();
	if (child < 0) {
		write_text(STDERR_FILENO, "usermemtest: fork failed\n");
		munmap(page, 4096);
		return -1;
	}
	if (child == 0) {
		if (page[0] != 0x31 || page[4095] != 0x32) {
			_exit(10);
		}
		page[0] = 0x41;
		page[4095] = 0x42;
		_exit(page[0] == 0x41 && page[4095] == 0x42 ? 0 : 11);
	}
	if (waitpid(child, &status, 0) != child) {
		write_text(STDERR_FILENO, "usermemtest: waitpid failed\n");
		munmap(page, 4096);
		return -1;
	}
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
		write_text(STDERR_FILENO, "usermemtest: child failed\n");
		munmap(page, 4096);
		return -1;
	}
	if (page[0] != 0x31 || page[4095] != 0x32) {
		write_text(STDERR_FILENO,
			   "usermemtest: private mapping leaked child write\n");
		munmap(page, 4096);
		return -1;
	}

	munmap(page, 4096);
	return 0;
#endif
}

int main(void)
{
	static const char ok[] = "usermemtest user memory contract ok\n";

	const int bad_user_result = test_bad_user_buffers();
	if (bad_user_result != 0) {
		return bad_user_result;
	}
	if (test_read_only_user_buffer() != 0) {
		return 12;
	}
	if (test_private_mapping_copy() != 0) {
		return 13;
	}

	(void)write(STDOUT_FILENO, ok, sizeof(ok) - 1);
	return 0;
}
