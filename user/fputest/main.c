#include <stdio.h>
#include <stdint.h>
#include <sys/wait.h>
#include <unistd.h>
#include <immintrin.h>

static int write_all(const char *buf, size_t len)
{
	while (len != 0) {
		ssize_t written = write(1, buf, len);

		if (written <= 0) {
			return -1;
		}

		buf += (size_t)written;
		len -= (size_t)written;
	}

	return 0;
}

static int str_eq(const char *left, const char *right)
{
	while (*left != '\0' && *right != '\0') {
		if (*left++ != *right++) {
			return 0;
		}
	}

	return *left == *right;
}

static uint64_t double_bits(double value)
{
	union {
		double d;
		uint64_t u;
	} bits = { .d = value };

	return bits.u;
}

static uint32_t x87_control_status(void)
{
	uint16_t control;
	uint16_t status;

	__asm__ volatile ("fnstcw %0" : "=m"(control));
	__asm__ volatile ("fnstsw %0" : "=m"(status));
	return ((uint32_t)status << 16) | control;
}

__attribute__((noinline, target("sse2")))
static uint64_t sse2_mix(uint64_t seed)
{
	uint64_t values[2];
	__m128i a = _mm_set_epi64x((long long)(seed + 0x1111111111111111ULL),
				   (long long)(seed + 0x2222222222222222ULL));
	__m128i b = _mm_set_epi64x(0x3333333333333333LL,
				   0x4444444444444444LL);
	__m128i c = _mm_add_epi64(a, b);

	_mm_storeu_si128((__m128i *)values, c);
	return values[0] ^ values[1];
}

__attribute__((noinline, target("avx2")))
static uint64_t avx2_mix(uint64_t seed)
{
	uint64_t values[4];
	__m256i a = _mm256_set_epi64x((long long)(seed + 0x1010101010101010ULL),
				      (long long)(seed + 0x2020202020202020ULL),
				      (long long)(seed + 0x3030303030303030ULL),
				      (long long)(seed + 0x4040404040404040ULL));
	__m256i b = _mm256_set1_epi64x(0x0102030405060708LL);
	__m256i c = _mm256_add_epi64(a, b);

	_mm256_storeu_si256((__m256i *)values, c);
	return values[0] ^ values[1] ^ values[2] ^ values[3];
}

int main(int argc, char **argv)
{
	char line[256];
	int do_fork = 0;
	int do_sse2 = 1;
	int do_avx2 = 1;

	for (int arg = 1; arg < argc; arg++) {
		if (str_eq(argv[arg], "--fork")) {
			do_fork = 1;
		} else if (str_eq(argv[arg], "--no-sse2")) {
			do_sse2 = 0;
		} else if (str_eq(argv[arg], "--no-avx2")) {
			do_avx2 = 0;
		}
	}

	if (write_all("fputest begin\n", 14) != 0) {
		return 1;
	}

	for (int i = 0; i < 16; i++) {
		if (i == 0 && write_all("fputest scalar\n", 15) != 0) {
			return 1;
		}

		double interval = 2.0 + ((double)i / 10.0);
		double value = (interval * 3.14159265358979323846) /
			       (double)(i + 1);

		uint64_t sse2 = 0;
		if (do_sse2) {
			if (i == 0 && write_all("fputest sse2\n", 13) != 0) {
				return 1;
			}
			sse2 = sse2_mix((uint64_t)i);
		}

		uint64_t avx2 = 0;
		uint64_t interval_bits = double_bits(interval);
		uint64_t value_bits = double_bits(value);
		uint32_t x87 = x87_control_status();
		if (do_avx2) {
			if (i == 0 && write_all("fputest avx2\n", 13) != 0) {
				return 1;
			}
			avx2 = avx2_mix((uint64_t)i);
		}

		if (i == 0 && write_all("fputest format\n", 15) != 0) {
			return 1;
		}

		int len = snprintf(line, sizeof(line),
				   "fputest iter=%d every %.1fs value=%.9f ibits=%016llx vbits=%016llx x87=%08x sse2=%016llx avx2=%016llx\n",
				   i, interval, value,
				   (unsigned long long)interval_bits,
				   (unsigned long long)value_bits,
				   x87,
				   (unsigned long long)sse2,
				   (unsigned long long)avx2);

		if (len <= 0 ||
		    write_all(line, (size_t)len < sizeof(line) ?
			      (size_t)len : sizeof(line) - 1) != 0) {
			return 1;
		}

		if (!do_fork) {
			continue;
		}

		pid_t child = fork();

		if (child < 0) {
			perror("fork");
			return 1;
		}
		if (child == 0) {
			_exit(0);
		}

		int status = 0;
		if (waitpid(child, &status, 0) != child || !WIFEXITED(status) ||
		    WEXITSTATUS(status) != 0) {
			perror("waitpid");
			return 1;
		}
	}

	return write_all("fputest ok\n", 11) == 0 ? 0 : 1;
}
