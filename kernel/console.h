#ifndef BUNIXOS_CONSOLE_H
#define BUNIXOS_CONSOLE_H

#include "types.h"

#include <stdarg.h>

void console_init(void);
void console_putc(char c);
void console_write(const char *text);
void console_write_hex32(u32 value);
void console_write_hex64(u64 value);
void console_vprintf(const char *fmt, va_list args);
void console_printf(const char *fmt, ...);

#endif
