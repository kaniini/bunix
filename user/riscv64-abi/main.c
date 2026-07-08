#include <bunix/syscall.h>

static u64 cstr_len(const char *text)
{
	u64 len = 0;

	while (text[len] != '\0') {
		len++;
	}
	return len;
}

int main(int argc, char **argv, char **envp)
{
	(void)envp;
	const char prefix[] = "native: riscv64 server argc=";
	const char one[] = "1 argv0=";
	const char syscalls[] = "native: riscv64 syscalls\n";
	const char fail[] = "native: riscv64 syscall check failed\n";
	const char newline[] = "\n";
	int ok = 1;

	bunix_early_console_write(prefix, sizeof(prefix) - 1);
	bunix_early_console_write(argc == 1 ? one : "? argv0=",
				  argc == 1 ? sizeof(one) - 1 : 8);
	if (argc > 0 && argv != 0 && argv[0] != 0) {
		bunix_early_console_write(argv[0], cstr_len(argv[0]));
	}
	bunix_early_console_write(newline, sizeof(newline) - 1);

	if (bunix_task_id(0) == 0) {
		ok = 0;
	}
	if (bunix_timer_ticks() == (u64)-1) {
		ok = 0;
	}
	if (bunix_clock_monotonic_ns() == (u64)-1) {
		ok = 0;
	}
	if (ok) {
		bunix_early_console_log(syscalls, sizeof(syscalls) - 1);
	} else {
		bunix_early_console_write(fail, sizeof(fail) - 1);
	}
	return (int)bunix_syscall1(BUNIX_SYSCALL_EXIT, (u64)argc);
}
