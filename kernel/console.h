#ifndef BUNIXOS_CONSOLE_H
#define BUNIXOS_CONSOLE_H

#include "types.h"

#include <stdarg.h>

void console_init(void);
void console_set_verbosity(const char *level);
void console_putc(char c);
void console_write(const char *text);
void console_write_len(const char *text, u64 len);
void console_log_write_len(const char *text, u64 len);
void console_logs_to_ring(void);
u64 console_log_size(void);
u64 console_log_read(char *buffer, u64 len);
int console_can_read(void);
u64 console_read(char *buffer, u64 len);
u64 console_read_line(char *buffer, u64 len);
void console_write_hex32(u32 value);
void console_write_hex64(u64 value);
void console_vprintf(const char *fmt, va_list args);
void console_printf(const char *fmt, ...);

#endif
