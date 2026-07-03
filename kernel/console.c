#include "console.h"
#include "spinlock.h"
#include <arch/io.h>
#include <stdarg.h>

enum {
	VGA_WIDTH = 80,
	VGA_HEIGHT = 25,
	VGA_TEXT_BUFFER = 0xb8000,
	VGA_ATTR = 0x0f,
	COM1 = 0x3f8,
};

static u16 *const vga = (u16 *)VGA_TEXT_BUFFER;
static u32 cursor_row;
static u32 cursor_col;
static struct spinlock console_lock = SPINLOCK_INIT("console");

static void serial_init(void)
{
	arch_outb(COM1 + 1, 0x00);
	arch_outb(COM1 + 3, 0x80);
	arch_outb(COM1 + 0, 0x03);
	arch_outb(COM1 + 1, 0x00);
	arch_outb(COM1 + 3, 0x03);
	arch_outb(COM1 + 2, 0xc7);
	arch_outb(COM1 + 4, 0x0b);
}

static void serial_putc(char c)
{
	while ((arch_inb(COM1 + 5) & 0x20) == 0) {
	}
	arch_outb(COM1, (u8)c);
}

static void vga_newline(void)
{
	cursor_col = 0;
	if (++cursor_row < VGA_HEIGHT) {
		return;
	}

	for (u32 row = 1; row < VGA_HEIGHT; row++) {
		for (u32 col = 0; col < VGA_WIDTH; col++) {
			vga[(row - 1) * VGA_WIDTH + col] = vga[row * VGA_WIDTH + col];
		}
	}

	cursor_row = VGA_HEIGHT - 1;
	for (u32 col = 0; col < VGA_WIDTH; col++) {
		vga[cursor_row * VGA_WIDTH + col] = ((u16)VGA_ATTR << 8) | ' ';
	}
}

void console_init(void)
{
	serial_init();
	cursor_row = 0;
	cursor_col = 0;

	for (u32 row = 0; row < VGA_HEIGHT; row++) {
		for (u32 col = 0; col < VGA_WIDTH; col++) {
			vga[row * VGA_WIDTH + col] = ((u16)VGA_ATTR << 8) | ' ';
		}
	}
}

static void console_putc_raw(char c)
{
	if (c == '\n') {
		serial_putc('\r');
		serial_putc('\n');
		vga_newline();
		return;
	}

	serial_putc(c);
	vga[cursor_row * VGA_WIDTH + cursor_col] = ((u16)VGA_ATTR << 8) | (u8)c;
	if (++cursor_col == VGA_WIDTH) {
		vga_newline();
	}
}

static void console_write_raw(const char *text)
{
	while (*text != '\0') {
		console_putc_raw(*text++);
	}
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
		buffer[cursor++] = (char)(digit < 10 ? '0' + digit : 'a' + digit - 10);
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
