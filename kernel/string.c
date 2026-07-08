#include "types.h"

void *memcpy(void *dst, const void *src, u64 len)
{
	u8 *out = (u8 *)dst;
	const u8 *in = (const u8 *)src;

	for (u64 i = 0; i < len; i++) {
		out[i] = in[i];
	}
	return dst;
}

void *memset(void *dst, int value, u64 len)
{
	u8 *out = (u8 *)dst;

	for (u64 i = 0; i < len; i++) {
		out[i] = (u8)value;
	}
	return dst;
}
