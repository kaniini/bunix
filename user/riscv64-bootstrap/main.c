#include <bunix/syscall.h>

static void log_line(const char *text, u64 len)
{
	(void)bunix_syscall2(BUNIX_SYSCALL_EARLY_CONSOLE_LOG, (u64)text, len);
}

static void launch_or_log(const char *name, const char *ok, u64 ok_len,
			  const char *fail, u64 fail_len)
{
	if (bunix_launch_module(name) >= 0) {
		log_line(ok, ok_len);
	} else {
		log_line(fail, fail_len);
	}
}

static void launch_with_caps_or_log(const char *name,
				    const struct bunix_launch_cap *caps,
				    u64 cap_count, const char *ok, u64 ok_len,
				    const char *fail, u64 fail_len)
{
	if (bunix_launch_module_with_caps(name, caps, cap_count) >= 0) {
		log_line(ok, ok_len);
	} else {
		log_line(fail, fail_len);
	}
}

int main(void)
{
	const char online[] = "bootstrap-riscv64: online\n";
	const char abi_ok[] = "bootstrap-riscv64: abi-smoke launched\n";
	const char abi_fail[] = "bootstrap-riscv64: abi-smoke failed\n";
	const char user_ok[] = "bootstrap-riscv64: user launched\n";
	const char user_fail[] = "bootstrap-riscv64: user failed\n";
	const char linux_ok[] = "bootstrap-riscv64: linux launched\n";
	const char linux_fail[] = "bootstrap-riscv64: linux failed\n";
	const char hello_ok[] = "bootstrap-riscv64: musl-hello launched\n";
	const char hello_fail[] = "bootstrap-riscv64: musl-hello failed\n";
	const char done[] = "bootstrap-riscv64: done\n";

	log_line(online, sizeof(online) - 1);
	launch_or_log("abi-smoke.user", abi_ok, sizeof(abi_ok) - 1,
		      abi_fail, sizeof(abi_fail) - 1);
	const struct bunix_launch_cap user_caps[] = {
		{ BUNIX_HANDLE_CONSOLE, BUNIX_RIGHT_SEND, 0 },
		{ BUNIX_HANDLE_NAMES, BUNIX_RIGHT_SEND, 0 },
	};
	launch_with_caps_or_log("user", user_caps,
				sizeof(user_caps) / sizeof(user_caps[0]),
				user_ok, sizeof(user_ok) - 1,
				user_fail, sizeof(user_fail) - 1);
	const struct bunix_launch_cap linux_caps[] = {
		{ BUNIX_HANDLE_CONSOLE, BUNIX_RIGHT_SEND, 0 },
		{ BUNIX_HANDLE_VM, BUNIX_RIGHT_SEND, 0 },
		{ BUNIX_HANDLE_NAMES, BUNIX_RIGHT_SEND, 0 },
	};
	launch_with_caps_or_log("linux", linux_caps,
				sizeof(linux_caps) / sizeof(linux_caps[0]),
				linux_ok, sizeof(linux_ok) - 1,
				linux_fail, sizeof(linux_fail) - 1);
	launch_or_log("/bin/musl-hello", hello_ok, sizeof(hello_ok) - 1,
		      hello_fail, sizeof(hello_fail) - 1);

	log_line(done, sizeof(done) - 1);
	return 0;
}
