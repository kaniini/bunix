#include "console.h"
#include "sched.h"
#include "spinlock.h"
#include <arch/io.h>
#include <stdarg.h>

enum {
	VGA_WIDTH = 80,
	VGA_HEIGHT = 25,
	VGA_TEXT_BUFFER = 0xb8000,
	VGA_ATTR = 0x0f,
	COM1 = 0x3f8,
	CONSOLE_LOG_BUFFER_SIZE = 32768,
	CONSOLE_INPUT_PENDING_SIZE = 8192,
};

static u16 *const vga = (u16 *)VGA_TEXT_BUFFER;
static u32 cursor_row;
static u32 cursor_col;
static struct spinlock console_lock = SPINLOCK_INIT("console");
static struct spinlock console_input_lock = SPINLOCK_INIT("console-input");
static char console_log_buffer[CONSOLE_LOG_BUFFER_SIZE];
static u64 console_log_start;
static u64 console_log_len;
static int console_log_ring_only;
static char console_input_pending[CONSOLE_INPUT_PENDING_SIZE];
static u64 console_input_pending_start;
static u64 console_input_pending_len;
static u32 console_cpr_state;

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
	    str_starts_with(fmt, "linux: mprotect") ||
	    str_starts_with(fmt, "linux: munmap") ||
	    str_starts_with(fmt, "linux: execve") ||
	    str_starts_with(fmt, "linux: fork") ||
	    str_starts_with(fmt, "linux-strace") ||
	    str_starts_with(fmt, "multiboot2: module") ||
	    str_starts_with(fmt, "names: lookup") ||
	    str_starts_with(fmt, "names: register") ||
	    str_starts_with(fmt, "names: update") ||
	    str_starts_with(fmt, "sched: close") ||
	    str_starts_with(fmt, "sched: grant") ||
	    str_starts_with(fmt, "sched: place") ||
	    str_starts_with(fmt, "sched: task pid") ||
	    str_starts_with(fmt, "sched: thread tid") ||
	    str_starts_with(fmt, "vfs: read file") ||
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

static int console_input_enqueue_locked(char c)
{
	if (console_input_pending_len >= sizeof(console_input_pending)) {
		return 0;
	}

	const u64 index = (console_input_pending_start +
			   console_input_pending_len) %
			  sizeof(console_input_pending);

	console_input_pending[index] = c;
	console_input_pending_len++;
	return 1;
}

static int console_input_dequeue_locked(char *out)
{
	if (console_input_pending_len == 0) {
		return 0;
	}

	*out = console_input_pending[console_input_pending_start];
	console_input_pending_start =
		(console_input_pending_start + 1) %
		sizeof(console_input_pending);
	console_input_pending_len--;
	return 1;
}

static int console_poll_input_locked(int consume_control, char *control)
{
	int saw_control = 0;

	while ((arch_inb(COM1 + 5) & 0x01) != 0) {
		const char c = (char)arch_inb(COM1);

		if (consume_control && c == 3 && !saw_control) {
			if (control != 0) {
				*control = c;
			}
			saw_control = 1;
			continue;
		}
		(void)console_input_enqueue_locked(c);
	}

	return saw_control;
}

static int serial_try_getc(char *out)
{
	const u64 flags = spin_lock_irqsave(&console_input_lock);
	int ok;

	if (console_input_pending_len == 0) {
		(void)console_poll_input_locked(0, 0);
	}
	ok = console_input_dequeue_locked(out);
	spin_unlock_irqrestore(&console_input_lock, flags);
	return ok;
}

int console_poll_control_input(char *out)
{
	const u64 flags = spin_lock_irqsave(&console_input_lock);
	const int ok = console_poll_input_locked(1, out);

	spin_unlock_irqrestore(&console_input_lock, flags);
	return ok;
}

static char serial_getc(void)
{
	char c;

	while (!serial_try_getc(&c)) {
		thread_yield();
	}

	return c;
}

int console_can_read(void)
{
	const u64 flags = spin_lock_irqsave(&console_input_lock);
	int can_read;

	if (console_input_pending_len != 0) {
		spin_unlock_irqrestore(&console_input_lock, flags);
		return 1;
	}
	(void)console_poll_input_locked(0, 0);
	can_read = console_input_pending_len != 0;
	spin_unlock_irqrestore(&console_input_lock, flags);
	return can_read;
}

static void console_queue_input_raw(const char *text)
{
	const u64 flags = spin_lock_irqsave(&console_input_lock);

	while (*text != '\0') {
		if (!console_input_enqueue_locked(*text++)) {
			break;
		}
	}
	spin_unlock_irqrestore(&console_input_lock, flags);
}

static void console_track_vt_query_raw(char c)
{
	if (console_cpr_state == 0) {
		console_cpr_state = c == '\033' ? 1 : 0;
		return;
	}
	if (console_cpr_state == 1) {
		console_cpr_state = c == '[' ? 2 : (c == '\033' ? 1 : 0);
		return;
	}
	if (console_cpr_state == 2) {
		console_cpr_state = c == '6' ? 3 : (c == '\033' ? 1 : 0);
		return;
	}
	if (console_cpr_state == 3 && c == 'n') {
		console_queue_input_raw("\033[1;1R");
		console_cpr_state = 0;
		return;
	}
	console_cpr_state = c == '\033' ? 1 : 0;
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
	console_track_vt_query_raw(c);

	if (c == '\n') {
		serial_putc('\r');
		serial_putc('\n');
		vga_newline();
		return;
	}

	if (c == '\b') {
		serial_putc('\b');
		serial_putc(' ');
		serial_putc('\b');
		if (cursor_col != 0) {
			cursor_col--;
			vga[cursor_row * VGA_WIDTH + cursor_col] =
				((u16)VGA_ATTR << 8) | ' ';
		}
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

static void console_log_putc_raw(char c)
{
	if (console_log_len < CONSOLE_LOG_BUFFER_SIZE) {
		console_log_buffer[(console_log_start + console_log_len) %
				   CONSOLE_LOG_BUFFER_SIZE] = c;
		console_log_len++;
	} else {
		console_log_buffer[console_log_start] = c;
		console_log_start =
			(console_log_start + 1) % CONSOLE_LOG_BUFFER_SIZE;
	}
}

static void console_log_write_raw(const char *text)
{
	while (*text != '\0') {
		console_log_putc_raw(*text++);
	}
}

static void console_log_emit_raw(char c)
{
	console_log_putc_raw(c);
	if (!console_log_ring_only) {
		console_putc_raw(c);
	}
}

static void console_log_emit_text_raw(const char *text)
{
	while (*text != '\0') {
		console_log_emit_raw(*text++);
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

void console_logs_to_ring(void)
{
	const u64 flags = spin_lock_irqsave(&console_lock);

	console_log_ring_only = 1;
	console_log_write_raw("kernel: console logs routed to dmesg\n");

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

void console_log_write_len(const char *text, u64 len)
{
	const u64 flags = spin_lock_irqsave(&console_lock);

	for (u64 i = 0; i < len; i++) {
		console_log_emit_raw(text[i]);
	}

	spin_unlock_irqrestore(&console_lock, flags);
}

u64 console_log_size(void)
{
	const u64 flags = spin_lock_irqsave(&console_lock);
	const u64 len = console_log_len;

	spin_unlock_irqrestore(&console_lock, flags);
	return len;
}

u64 console_log_read(char *buffer, u64 len)
{
	const u64 flags = spin_lock_irqsave(&console_lock);
	const u64 nread = len < console_log_len ? len : console_log_len;
	const u64 offset = console_log_len - nread;

	for (u64 i = 0; i < nread; i++) {
		const u64 index = (console_log_start + offset + i) %
				  CONSOLE_LOG_BUFFER_SIZE;

		buffer[i] = console_log_buffer[index];
	}

	spin_unlock_irqrestore(&console_lock, flags);
	return nread;
}

u64 console_read(char *buffer, u64 len)
{
	if (buffer == 0 || len == 0) {
		return 0;
	}
	if (len == 1) {
		buffer[0] = serial_getc();
		return 1;
	}
	return console_read_line(buffer, len);
}

u64 console_read_line(char *buffer, u64 len)
{
	u64 used = 0;

	if (buffer == 0 || len == 0) {
		return 0;
	}

	for (;;) {
		char c = serial_getc();

		if (c == '\r') {
			c = '\n';
		}

		if (c == '\b' || c == 0x7f) {
			if (used != 0) {
				used--;
				console_putc('\b');
			}
			continue;
		}

		if (c == '\n') {
			if (used < len) {
				buffer[used++] = '\n';
			}
			return used;
		}

		if ((u8)c < 0x20) {
			continue;
		}

		buffer[used++] = c;
		if (used == len) {
			return used;
		}
	}
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

static void console_log_write_uint_raw(u64 value, u32 base, int is_signed)
{
	char buffer[32];
	u32 cursor = 0;

	if (is_signed && (i64)value < 0) {
		console_log_emit_raw('-');
		value = (u64)(-(i64)value);
	}

	if (value == 0) {
		console_log_emit_raw('0');
		return;
	}

	while (value != 0) {
		const u32 digit = value % base;
		buffer[cursor++] = (char)(digit < 10 ? '0' + digit :
					  'a' + digit - 10);
		value /= base;
	}

	while (cursor > 0) {
		console_log_emit_raw(buffer[--cursor]);
	}
}

static void console_log_write_pointer_raw(const void *ptr)
{
	const u64 value = (u64)ptr;
	static const char digits[] = "0123456789abcdef";

	console_log_emit_text_raw("0x");
	for (int shift = 60; shift >= 0; shift -= 4) {
		console_log_emit_raw(digits[(value >> shift) & 0x0f]);
	}
}

static void console_vprintf_raw(const char *fmt, va_list args)
{
	while (*fmt != '\0') {
		if (*fmt != '%') {
			console_log_emit_raw(*fmt++);
			continue;
		}

		fmt++;
		switch (*fmt) {
		case '\0':
			console_log_emit_raw('%');
			return;
		case '%':
			console_log_emit_raw('%');
			break;
		case 'c':
			console_log_emit_raw((char)va_arg(args, int));
			break;
		case 's': {
			const char *text = va_arg(args, const char *);
			console_log_emit_text_raw(text != 0 ? text : "(null)");
			break;
		}
		case 'd':
		case 'i':
			console_log_write_uint_raw((u64)va_arg(args, int),
						   10, 1);
			break;
		case 'u':
			console_log_write_uint_raw(
				(u64)va_arg(args, unsigned int), 10, 0);
			break;
		case 'x':
			console_log_write_uint_raw(
				(u64)va_arg(args, unsigned int), 16, 0);
			break;
		case 'p':
			console_log_write_pointer_raw(va_arg(args,
							     const void *));
			break;
		default:
			console_log_emit_raw('%');
			console_log_emit_raw(*fmt);
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
