#ifndef BUNIX_KERNEL_CMDLINE_H
#define BUNIX_KERNEL_CMDLINE_H

void kernel_cmdline_configure(const char *cmdline);
int kernel_cmdline_has(const char *token);

#endif
