#include "bootpkg.h"
#include "cmdline.h"
#include "server.h"

static const char bootpkg_magic[] = "BUNIX-RV64-BOOTPKG\n";
static const char bootpkg_module_prefix[] = "module ";
static const char bootpkg_cmdline_prefix[] = "cmdline ";

static int cstr_eq_len(const char *left, const char *right, u64 len)
{
	for (u64 i = 0; i < len; i++) {
		if (left[i] != right[i] || right[i] == '\0') {
			return 0;
		}
	}
	return right[len] == '\0';
}

static u64 cstr_len(const char *text)
{
	u64 len = 0;

	while (text[len] != '\0') {
		len++;
	}
	return len;
}

static u64 parse_decimal(const char *text, u64 len, u64 *used)
{
	u64 value = 0;
	u64 i = 0;

	while (i < len && text[i] >= '0' && text[i] <= '9') {
		value = value * 10 + (u64)(text[i] - '0');
		i++;
	}
	*used = i;
	return value;
}

int bootpkg_magic_ok(u64 start, u64 end)
{
	const char *image = (const char *)start;
	const u64 magic_len = sizeof(bootpkg_magic) - 1;

	if (start == 0 || end <= start || end - start < magic_len) {
		return 0;
	}
	for (u64 i = 0; i < magic_len; i++) {
		if (image[i] != bootpkg_magic[i]) {
			return 0;
		}
	}
	return 1;
}

static int bootpkg_module_line(const char *line, u64 len, char *name,
			       u64 name_capacity, u64 *size)
{
	const u64 prefix_len = cstr_len(bootpkg_module_prefix);
	u64 name_end = prefix_len;
	u64 used = 0;

	if (len <= prefix_len ||
	    !cstr_eq_len(line, bootpkg_module_prefix, prefix_len)) {
		return -1;
	}
	while (name_end < len && line[name_end] != ' ') {
		name_end++;
	}
	if (name_end == prefix_len || name_end >= len ||
	    name_end - prefix_len >= name_capacity) {
		return -1;
	}
	for (u64 i = 0; i < name_end - prefix_len; i++) {
		name[i] = line[prefix_len + i];
	}
	name[name_end - prefix_len] = '\0';
	*size = parse_decimal(line + name_end + 1, len - name_end - 1, &used);
	return used != 0 ? 0 : -1;
}

static int bootpkg_payload_offset(u64 start, u64 end, u64 *payload_offset)
{
	const char *image = (const char *)start;
	u64 cursor = sizeof(bootpkg_magic) - 1;

	while (start + cursor < end) {
		u64 line_end = cursor;

		while (start + line_end < end && image[line_end] != '\n') {
			line_end++;
		}
		if (line_end == cursor) {
			*payload_offset = line_end + 1;
			return start + *payload_offset <= end ? 0 : -1;
		}
		cursor = line_end + 1;
	}
	return -1;
}

int bootpkg_find_module(u64 start, u64 end, const char *wanted,
			u64 *module_start, u64 *module_size)
{
	const char *image = (const char *)start;
	const u64 prefix_len = cstr_len(bootpkg_module_prefix);
	const u64 wanted_len = cstr_len(wanted);
	u64 cursor = sizeof(bootpkg_magic) - 1;
	u64 payload = 0;
	u64 payload_size = 0;
	u64 found_payload_offset = 0;
	u64 found_start = 0;
	u64 found_size = 0;
	u32 matched = 0;

	while (start + cursor < end) {
		u64 line = cursor;
		u64 line_end = cursor;

		while (start + line_end < end && image[line_end] != '\n') {
			line_end++;
		}
		if (line_end == line) {
			payload = line_end + 1;
			break;
		}
		if (line_end - line > prefix_len + wanted_len + 1 &&
		    cstr_eq_len(image + line, bootpkg_module_prefix,
				prefix_len) &&
		    cstr_eq_len(image + line + prefix_len, wanted,
				wanted_len) &&
		    image[line + prefix_len + wanted_len] == ' ') {
			u64 used = 0;
			const u64 size = parse_decimal(
				image + line + prefix_len + wanted_len + 1,
				line_end - line - prefix_len - wanted_len - 1,
				&used);

			if (used != 0) {
				found_payload_offset = payload_size;
				found_size = size;
				matched = 1;
			}
		}
		if (line_end - line > prefix_len &&
		    cstr_eq_len(image + line, bootpkg_module_prefix,
				prefix_len)) {
			u64 name_end = line + prefix_len;
			u64 used = 0;
			u64 size = 0;

			while (name_end < line_end && image[name_end] != ' ') {
				name_end++;
			}
			if (name_end < line_end) {
				size = parse_decimal(image + name_end + 1,
						     line_end - name_end - 1,
						     &used);
			}
			if (used != 0) {
				if (payload_size + size < payload_size) {
					return -1;
				}
				payload_size += size;
			}
		}
		cursor = line_end + 1;
	}

	if (payload == 0 || matched == 0 ||
	    found_payload_offset + found_size < found_payload_offset ||
	    start + payload + found_payload_offset + found_size > end) {
		return -1;
	}
	found_start = start + payload + found_payload_offset;
	*module_start = found_start;
	*module_size = found_size;
	return 0;
}

int bootpkg_configure_cmdline(u64 start, u64 end)
{
	static char cmdline[256];
	const char *image = (const char *)start;
	const u64 prefix_len = cstr_len(bootpkg_cmdline_prefix);
	u64 cursor = sizeof(bootpkg_magic) - 1;

	while (start + cursor < end) {
		u64 line = cursor;
		u64 line_end = cursor;

		while (start + line_end < end && image[line_end] != '\n') {
			line_end++;
		}
		if (line_end == line) {
			break;
		}
		if (line_end - line > prefix_len &&
		    cstr_eq_len(image + line, bootpkg_cmdline_prefix,
				prefix_len)) {
			u64 len = line_end - line - prefix_len;

			if (len >= sizeof(cmdline)) {
				len = sizeof(cmdline) - 1;
			}
			for (u64 i = 0; i < len; i++) {
				cmdline[i] = image[line + prefix_len + i];
			}
			cmdline[len] = '\0';
			kernel_cmdline_configure(cmdline);
			return 0;
		}
		cursor = line_end + 1;
	}

	kernel_cmdline_configure("");
	return -1;
}

int bootpkg_record_modules(u64 start, u64 end)
{
	const char *image = (const char *)start;
	u64 cursor = sizeof(bootpkg_magic) - 1;
	u64 payload_offset = 0;
	u64 payload_cursor = 0;
	u32 recorded = 0;

	if (bootpkg_payload_offset(start, end, &payload_offset) != 0) {
		return -1;
	}
	payload_cursor = payload_offset;
	while (start + cursor < end) {
		u64 line = cursor;
		u64 line_end = cursor;
		char name[64];
		u64 size = 0;

		while (start + line_end < end && image[line_end] != '\n') {
			line_end++;
		}
		if (line_end == line) {
			break;
		}
		if (bootpkg_module_line(image + line, line_end - line,
					name, sizeof(name), &size) == 0) {
			if (size == 0 || payload_cursor + size < payload_cursor ||
			    start + payload_cursor + size > end) {
				return -1;
			}
			server_record_boot_module(name, start + payload_cursor,
						  start + payload_cursor + size);
			payload_cursor += size;
			recorded++;
		}
		cursor = line_end + 1;
	}
	return recorded != 0 ? 0 : -1;
}
