typedef unsigned long size_t;

void *memcpy(void *dst, const void *src, size_t len)
{
	unsigned char *out = (unsigned char *)dst;
	const unsigned char *in = (const unsigned char *)src;

	for (size_t i = 0; i < len; i++) {
		out[i] = in[i];
	}
	return dst;
}

void *memset(void *dst, int value, size_t len)
{
	unsigned char *out = (unsigned char *)dst;

	for (size_t i = 0; i < len; i++) {
		out[i] = (unsigned char)value;
	}
	return dst;
}

void *memmove(void *dst, const void *src, size_t len)
{
	unsigned char *out = (unsigned char *)dst;
	const unsigned char *in = (const unsigned char *)src;

	if (out < in) {
		for (size_t i = 0; i < len; i++) {
			out[i] = in[i];
		}
	} else if (out > in) {
		for (size_t i = len; i > 0; i--) {
			out[i - 1] = in[i - 1];
		}
	}
	return dst;
}

int memcmp(const void *left, const void *right, size_t len)
{
	const unsigned char *a = (const unsigned char *)left;
	const unsigned char *b = (const unsigned char *)right;

	for (size_t i = 0; i < len; i++) {
		if (a[i] != b[i]) {
			return (int)a[i] - (int)b[i];
		}
	}
	return 0;
}
