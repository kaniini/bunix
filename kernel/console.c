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

enum console_log_level {
	CONSOLE_LOG_ERROR,
	CONSOLE_LOG_WARN,
	CONSOLE_LOG_INFO,
	CONSOLE_LOG_DEBUG,
	CONSOLE_LOG_TRACE,
};

static u32 console_verbosity = CONSOLE_LOG_INFO;

static int str_token_eq(const char *left, const char *right)
{
	while (*left != '\0' && *left != ' ' && *right != '\0') {
		if (*left++ != *right++) {
			return 0;
		}
	}

	return (*left == '\0' || *left == ' ') && *right == '\0';
}

static int str_starts_with(const char *text, const char *prefix)
{
	while (*prefix != '\0') {
		if (*text++ != *prefix++) {
			return 0;
		}
	}

	return 1;
}

static u32 log_level_from_name(const char *level)
{
	if (level == 0) {
		return CONSOLE_LOG_INFO;
	}
	if (str_token_eq(level, "quiet") || str_token_eq(level, "error")) {
		return CONSOLE_LOG_ERROR;
	}
	if (str_token_eq(level, "warn")) {
		return CONSOLE_LOG_WARN;
	}
	if (str_token_eq(level, "debug")) {
		return CONSOLE_LOG_DEBUG;
	}
	if (str_token_eq(level, "trace")) {
		return CONSOLE_LOG_TRACE;
	}
	return CONSOLE_LOG_INFO;
}

static u32 log_level_for_format(const char *fmt)
{
	if (str_starts_with(fmt, "interrupts: vector=") ||
	    str_starts_with(fmt, "bunixos: invalid") ||
	    str_starts_with(fmt, "arch-vm: failed") ||
	    str_starts_with(fmt, "elf: invalid") ||
	    str_starts_with(fmt, "elf: failed") ||
	    str_starts_with(fmt, "kernel: failed") ||
	    str_starts_with(fmt, "kernel: invalid") ||
	    str_starts_with(fmt, "kernel: too many") ||
	    str_starts_with(fmt, "kernel: no module") ||
	    str_starts_with(fmt, "sched: refusing") ||
	    str_starts_with(fmt, "sched: thread alloc failed") ||
	    str_starts_with(fmt, "sched: task alloc failed") ||
	    str_starts_with(fmt, "sched: handle table full") ||
	    str_starts_with(fmt, "sched: vma table full") ||
	    str_starts_with(fmt, "smp: ap start timeout") ||
	    str_starts_with(fmt, "smp: lapic delivery timeout") ||
	    str_starts_with(fmt, "syscall: unknown") ||
	    str_starts_with(fmt, "linux: unknown")) {
		return CONSOLE_LOG_ERROR;
	}

	if (str_starts_with(fmt, "sched: cap denied") ||
	    str_starts_with(fmt, "sched: close denied") ||
	    str_starts_with(fmt, "sched: handle denied") ||
	    str_starts_with(fmt, "sched: inherit denied") ||
	    str_starts_with(fmt, "sched: task handle denied") ||
	    str_starts_with(fmt, "sched: buffer handle denied") ||
	    str_starts_with(fmt, "sched: vma overlap") ||
	    str_starts_with(fmt, "names: table full") ||
	    str_starts_with(fmt, "ipc: message alloc failed") ||
	    str_starts_with(fmt, "buffer: alloc failed")) {
		return CONSOLE_LOG_WARN;
	}

	if (str_starts_with(fmt, "multiboot2: mmap") ||
	    str_starts_with(fmt, "pmm: reserve") ||
	    str_starts_with(fmt, "timer: tick") ||
	    str_starts_with(fmt, "vm-server: ipc event") ||
	    str_starts_with(fmt, "sched: ipi") ||
	    str_starts_with(fmt, "sched: enqueue") ||
	    str_starts_with(fmt, "sched: switch") ||
	    str_starts_with(fmt, "sched: yield") ||
	    str_starts_with(fmt, "sched: preempt") ||
	    str_starts_with(fmt, "sched: block") ||
	    str_starts_with(fmt, "sched: sleep") ||
	    str_starts_with(fmt, "sched: wake") ||
	    str_starts_with(fmt, "sched: reap") ||
	    str_starts_with(fmt, "ipc: send") ||
	    str_starts_with(fmt, "ipc: recv") ||
	    str_starts_with(fmt, "buffer: read") ||
	    str_starts_with(fmt, "buffer: write") ||
	    str_starts_with(fmt, "kernel: task map") ||
	    str_starts_with(fmt, "kernel: task write") ||
	    str_starts_with(fmt, "kernel: task alloc") ||
	    str_starts_with(fmt, "kernel: task clone") ||
	    str_starts_with(fmt, "kernel: task start") ||
	    str_starts_with(fmt, "kernel: task fork") ||
	    str_starts_with(fmt, "elf: load") ||
	    str_starts_with(fmt, "user: enter")) {
		return CONSOLE_LOG_TRACE;
	}

	if (str_starts_with(fmt, "arch-vm: create space") ||
	    str_starts_with(fmt, "buffer: create") ||
	    str_starts_with(fmt, "buffer: destroy") ||
	    str_starts_with(fmt, "elf: entry") ||
	    str_starts_with(fmt, "ipc: port") ||
	    str_starts_with(fmt, "kernel: recorded") ||
	    str_starts_with(fmt, "linux: arch_prctl") ||
	    str_starts_with(fmt, "linux: mmap") ||
	    str_starts_with(fmt, "linux: munmap") ||
	    str_starts_with(fmt, "linux: execve") ||
	    str_starts_with(fmt, "linux: fork") ||
	    str_starts_with(fmt, "multiboot2: module") ||
	    str_starts_with(fmt, "names: lookup") ||
	    str_starts_with(fmt, "names: register") ||
	    str_starts_with(fmt, "names: update") ||
	    str_starts_with(fmt, "sched: close") ||
	    str_starts_with(fmt, "sched: grant") ||
	    str_starts_with(fmt, "sched: place") ||
	    str_starts_with(fmt, "sched: task pid") ||
	    str_starts_with(fmt, "sched: thread tid") ||
	    str_starts_with(fmt, "vm: create space") ||
	    str_starts_with(fmt, "vm: destroy space") ||
	    str_starts_with(fmt, "vm: selftest")) {
		return CONSOLE_LOG_DEBUG;
	}

	return CONSOLE_LOG_INFO;
}

static int console_should_print(const char *fmt)
{
	return log_level_for_format(fmt) <= console_verbosity;
}

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

static void console_write_token_raw(const char *text)
{
	while (*text != '\0' && *text != ' ') {
		console_putc_raw(*text++);
	}
}

void console_set_verbosity(const char *level)
{
	const u64 flags = spin_lock_irqsave(&console_lock);

	console_verbosity = log_level_from_name(level);
	console_write_raw("kernel: log level ");
	console_write_token_raw(level != 0 ? level : "info");
	console_putc_raw('\n');

	spin_unlock_irqrestore(&console_lock, flags);
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

	if (!console_should_print(fmt)) {
		return;
	}

	va_start(args, fmt);
	console_vprintf(fmt, args);
	va_end(args);
}
