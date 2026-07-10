#ifndef BUNIX_KERNEL_CMDLINE_H
#define BUNIX_KERNEL_CMDLINE_H

#include "types.h"

void kernel_cmdline_configure(const char *cmdline);
int kernel_cmdline_has(const char *token);
int kernel_cmdline_get_u64(const char *prefix, u64 *value);

#endif
