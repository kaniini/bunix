#include "console.h"
#include "spinlock.h"
#include <arch/sbi.h>
#include <stdarg.h>

static struct spinlock console_lock = SPINLOCK_INIT("riscv64-console");

static void console_putc_raw(char c)
{
	if (c == '\n') {
		riscv64_sbi_putchar('\r');
	}
	riscv64_sbi_putchar(c);
}

static void console_write_raw(const char *text)
{
	while (*text != '\0') {
		console_putc_raw(*text++);
	}
}

void console_init(void)
{
}

void console_set_verbosity(const char *level)
{
	(void)level;
}

void console_putc(char c)
{
	const u64 flags = spin_lock_irqsave(&console_lock);

	console_putc_raw(c);
	spin_unlock_irqrestore(&console_lock, flags);
}

void console_write(const char *text)
{
	const u64 flags = spin_lock_irqsave(&console_lock);

	console_write_raw(text);
	spin_unlock_irqrestore(&console_lock, flags);
}

void console_write_len(const char *text, u64 len)
{
	const u64 flags = spin_lock_irqsave(&console_lock);

	for (u64 i = 0; i < len; i++) {
		console_putc_raw(text[i]);
	}
	spin_unlock_irqrestore(&console_lock, flags);
}

void console_log_write_len(const char *text, u64 len)
{
	console_write_len(text, len);
}

void console_logs_to_ring(void)
{
}

u64 console_log_size(void)
{
	return 0;
}

u64 console_log_read(char *buffer, u64 len)
{
	(void)buffer;
	(void)len;
	return 0;
}

int console_can_read(void)
{
	return 0;
}

int console_try_read_char(char *out)
{
	(void)out;
	return 0;
}

int console_poll_control_input(char *out)
{
	(void)out;
	return 0;
}

u64 console_read(char *buffer, u64 len)
{
	(void)buffer;
	(void)len;
	return 0;
}

u64 console_read_line(char *buffer, u64 len)
{
	(void)buffer;
	(void)len;
	return 0;
}

static void console_write_hex32_raw(u32 value)
{
	static const char digits[] = "0123456789abcdef";

	console_write_raw("0x");
	for (int shift = 28; shift >= 0; shift -= 4) {
		console_putc_raw(digits[(value >> shift) & 0x0f]);
	}
}

void console_write_hex32(u32 value)
{
	const u64 flags = spin_lock_irqsave(&console_lock);

	console_write_hex32_raw(value);
	spin_unlock_irqrestore(&console_lock, flags);
}

static void console_write_hex64_raw(u64 value)
{
	static const char digits[] = "0123456789abcdef";

	console_write_raw("0x");
	for (int shift = 60; shift >= 0; shift -= 4) {
		console_putc_raw(digits[(value >> shift) & 0x0f]);
	}
}

void console_write_hex64(u64 value)
{
	const u64 flags = spin_lock_irqsave(&console_lock);

	console_write_hex64_raw(value);
	spin_unlock_irqrestore(&console_lock, flags);
}

static void console_write_uint_raw(u64 value, u32 base, int is_signed)
{
	char buffer[32];
	u32 cursor = 0;

	if (is_signed && (i64)value < 0) {
		console_putc_raw('-');
		value = (u64)(-(i64)value);
	}

	if (value == 0) {
		console_putc_raw('0');
		return;
	}

	while (value != 0) {
		const u32 digit = value % base;

		buffer[cursor++] = (char)(digit < 10 ? '0' + digit :
					  'a' + digit - 10);
		value /= base;
	}

	while (cursor > 0) {
		console_putc_raw(buffer[--cursor]);
	}
}

static void console_write_pointer_raw(const void *ptr)
{
	const u64 value = (u64)ptr;
	static const char digits[] = "0123456789abcdef";

	console_write_raw("0x");
	for (int shift = 60; shift >= 0; shift -= 4) {
		console_putc_raw(digits[(value >> shift) & 0x0f]);
	}
}

static void console_vprintf_raw(const char *fmt, va_list args)
{
	while (*fmt != '\0') {
		if (*fmt != '%') {
			console_putc_raw(*fmt++);
			continue;
		}

		fmt++;
		switch (*fmt) {
		case '\0':
			console_putc_raw('%');
			return;
		case '%':
			console_putc_raw('%');
			break;
		case 'c':
			console_putc_raw((char)va_arg(args, int));
			break;
		case 's': {
			const char *text = va_arg(args, const char *);

			console_write_raw(text != 0 ? text : "(null)");
			break;
		}
		case 'd':
		case 'i':
			console_write_uint_raw((u64)va_arg(args, int), 10, 1);
			break;
		case 'u':
			console_write_uint_raw((u64)va_arg(args, unsigned int),
					       10, 0);
			break;
		case 'x':
			console_write_uint_raw((u64)va_arg(args, unsigned int),
					       16, 0);
			break;
		case 'p':
			console_write_pointer_raw(va_arg(args, const void *));
			break;
		default:
			console_putc_raw('%');
			console_putc_raw(*fmt);
			break;
		}

		fmt++;
	}
}

void console_vprintf(const char *fmt, va_list args)
{
	const u64 flags = spin_lock_irqsave(&console_lock);

	console_vprintf_raw(fmt, args);
	spin_unlock_irqrestore(&console_lock, flags);
}

void console_printf(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	console_vprintf(fmt, args);
	va_end(args);
}
