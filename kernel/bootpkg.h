#ifndef BUNIX_BOOTPKG_H
#define BUNIX_BOOTPKG_H

#include "types.h"

int bootpkg_magic_ok(u64 start, u64 end);
int bootpkg_configure_cmdline(u64 start, u64 end);
int bootpkg_record_modules(u64 start, u64 end);
int bootpkg_find_module(u64 start, u64 end, const char *wanted,
			u64 *module_start, u64 *module_size);

#endif
