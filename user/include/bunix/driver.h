#ifndef BUNIX_USER_DRIVER_H
#define BUNIX_USER_DRIVER_H

#include <bunix/syscall.h>

enum {
	BUNIX_DRIVER_RESOURCE_HW = 1,
};

struct bunix_driver_resource {
	const char *name;
	u64 handle;
	unsigned int kind;
	unsigned int ops;
	u64 base;
	u64 len;
};

struct bunix_driver {
	const char *name;
	unsigned int service;
	u64 service_handle;
	u64 names_handle;
	const struct bunix_driver_resource *resources;
	u64 resource_count;
};

static inline int bunix_str_eq(const char *left, const char *right)
{
	if (left == 0 || right == 0) {
		return 0;
	}
	while (*left != '\0' && *right != '\0') {
		if (*left++ != *right++) {
			return 0;
		}
	}
	return *left == '\0' && *right == '\0';
}

static inline u64 bunix_driver_str_len(const char *text)
{
	u64 len = 0;

	if (text == 0) {
		return 0;
	}
	while (text[len] != '\0') {
		len++;
	}
	return len;
}

static inline const struct bunix_driver_resource *
bunix_driver_resource_named(const struct bunix_driver *driver,
			    const char *name)
{
	if (driver == 0 || name == 0) {
		return 0;
	}
	for (u64 i = 0; i < driver->resource_count; i++) {
		if (bunix_str_eq(driver->resources[i].name, name)) {
			return &driver->resources[i];
		}
	}
	return 0;
}

static inline int bunix_driver_register(const struct bunix_driver *driver)
{
	struct bunix_msg request;
	struct bunix_msg reply;

	if (driver == 0 || driver->names_handle == 0 ||
	    driver->service_handle == 0 || driver->service == 0) {
		return -1;
	}

	request.protocol = BUNIX_PROTO_NAMES;
	request.type = BUNIX_NAMES_REGISTER;
	request.sender = 0;
	request.cap_rights = BUNIX_RIGHT_SEND | BUNIX_RIGHT_DUP;
	request.reply = 0;
	request.cap = driver->service_handle;
	request.words[0] = BUNIX_NAMES_ROOT;
	request.words[1] = driver->service;
	request.words[2] = 0;
	request.words[3] = 0;

	return bunix_ipc_call(driver->names_handle, &request, &reply);
}

static inline void bunix_driver_log_lifecycle(const struct bunix_driver *driver,
					      const char *state)
{
	char line[128];
	u64 offset = 0;
	u64 len;

	if (driver == 0 || driver->name == 0 || state == 0) {
		return;
	}

	len = bunix_driver_str_len(driver->name);
	for (u64 i = 0; i < len && offset + 1 < sizeof(line); i++) {
		line[offset++] = driver->name[i];
	}
	if (offset + 2 < sizeof(line)) {
		line[offset++] = ':';
		line[offset++] = ' ';
	}
	len = bunix_driver_str_len(state);
	for (u64 i = 0; i < len && offset + 1 < sizeof(line); i++) {
		line[offset++] = state[i];
	}
	if (offset < sizeof(line)) {
		line[offset++] = '\n';
	}
	(void)bunix_console_log(line, offset);
}

#endif
