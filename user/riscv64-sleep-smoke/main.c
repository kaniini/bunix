#include <bunix/syscall.h>

enum {
	SLEEP_NS = 200000000ULL,
};

static void log_text(const char *text, u64 len)
{
	(void)bunix_early_console_log(text, len);
}

static void poweroff(void)
{
	const u64 power = bunix_handle_find(BUNIX_CAP_POWR);

	if (power != 0) {
		(void)bunix_machine_poweroff(power);
	}
}

int main(void)
{
	const char before[] = "native: riscv64 sleep before\n";
	const char after[] = "native: riscv64 sleep after\n";
	const char ok[] = "native: riscv64 sleep ok\n";
	const char fail[] = "native: riscv64 sleep failed\n";
	const u64 before_ticks = bunix_timer_ticks();
	const u64 before_ns = bunix_clock_monotonic_ns();
	long result;
	u64 after_ticks;
	u64 after_ns;

	log_text(before, sizeof(before) - 1);
	result = bunix_sleep_ns(SLEEP_NS);
	after_ticks = bunix_timer_ticks();
	after_ns = bunix_clock_monotonic_ns();
	log_text(after, sizeof(after) - 1);

	if (result == 0 && after_ticks > before_ticks && after_ns > before_ns) {
		log_text(ok, sizeof(ok) - 1);
	} else {
		log_text(fail, sizeof(fail) - 1);
	}
	poweroff();
	return 1;
}
