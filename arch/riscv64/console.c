#include "console.h"
#include "spinlock.h"
#include <arch/console.h>
#include <arch/fdt.h>
#include <arch/vm.h>
#include <arch/sbi.h>
#include <stdarg.h>

static struct spinlock console_lock = SPINLOCK_INIT("riscv64-console");
static volatile void *console_uart_base;
static u32 console_uart_reg_shift;
static u32 console_uart_reg_io_width = 1;
static int console_uart_enabled;

enum {
	UART_THR = 0,
	UART_LSR = 5,
	UART_LSR_THRE = 0x20,
};

static int console_str_contains(const char *text, const char *needle)
{
	if (*needle == '\0') {
		return 1;
	}
	for (u32 i = 0; text[i] != '\0'; i++) {
		u32 j = 0;

		while (needle[j] != '\0' && text[i + j] == needle[j]) {
			j++;
		}
		if (needle[j] == '\0') {
			return 1;
		}
	}
	return 0;
}

static int uart_is_8250_compatible(const char *compatible)
{
	return console_str_contains(compatible, "ns16550") ||
	       console_str_contains(compatible, "ns16550a") ||
	       console_str_contains(compatible, "snps,dw-apb-uart");
}

static volatile void *uart_reg(u32 reg)
{
	return (volatile u8 *)console_uart_base +
	       ((u64)reg << console_uart_reg_shift);
}

static u8 uart_read(u32 reg)
{
	if (console_uart_reg_io_width == 4) {
		return (u8)(*(volatile u32 *)uart_reg(reg));
	}
	return *(volatile u8 *)uart_reg(reg);
}

static void uart_write(u32 reg, u8 value)
{
	if (console_uart_reg_io_width == 4) {
		*(volatile u32 *)uart_reg(reg) = value;
		return;
	}
	*(volatile u8 *)uart_reg(reg) = value;
}

static int uart_wait_tx_ready(void)
{
	for (u32 i = 0; i < 1000000; i++) {
		if ((uart_read(UART_LSR) & UART_LSR_THRE) != 0) {
			return 1;
		}
	}
	return 0;
}

static int uart_putc_raw(char c)
{
	if (!console_uart_enabled || !uart_wait_tx_ready()) {
		return 0;
	}
	uart_write(UART_THR, (u8)c);
	return 1;
}

static void console_putc_raw(char c)
{
	if (c == '\n') {
		if (!uart_putc_raw('\r')) {
			riscv64_sbi_putchar('\r');
		}
	}
	if (!uart_putc_raw(c)) {
		riscv64_sbi_putchar(c);
	}
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

void riscv64_console_init_from_fdt(const void *fdt)
{
	struct riscv64_fdt_platform platform;

	if (fdt == 0 ||
	    riscv64_fdt_scan_platform(fdt, &platform) != 0 ||
	    platform.stdout_uart_valid == 0) {
		return;
	}

	const struct riscv64_fdt_device *uart =
		&platform.uarts[platform.stdout_uart_index];

	if (uart->reg_base == 0 || !uart_is_8250_compatible(uart->compatible)) {
		console_printf("uart: riscv64 console-driver=sbi\n");
		return;
	}
	if (arch_vm_register_mmio_identity(uart->reg_base, 0x1000) != 0) {
		console_printf("uart: riscv64 console-driver=sbi mmio-map-failed\n");
		return;
	}

	console_uart_base = (volatile void *)uart->reg_base;
	console_uart_reg_shift = uart->reg_shift;
	console_uart_reg_io_width = uart->reg_io_width == 4 ? 4 : 1;
	console_uart_enabled = 1;
	console_printf("uart: riscv64 console-driver=ns16550 base=%p shift=%u width=%u compatible=%s\n",
		       (const void *)uart->reg_base, console_uart_reg_shift,
		       console_uart_reg_io_width, uart->compatible);
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
