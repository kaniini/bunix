#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
	ROOTFS_MAGIC = 0x30534652,
	ROOTFS_MAX_PATH = 128,
	ROOTFS_INITIAL_ENTRIES = 128,
	ROOTFS_TYPE_REGULAR = 1,
	ROOTFS_TYPE_DIRECTORY = 2,
	ROOTFS_TYPE_SYMLINK = 3,
};

struct rootfs_header {
	uint32_t magic;
	uint32_t entries;
};

struct rootfs_entry {
	char path[ROOTFS_MAX_PATH];
	uint64_t offset;
	uint64_t size;
	uint32_t uid;
	uint32_t gid;
	uint32_t mode;
	uint32_t type;
};

struct rootfs_entries {
	struct rootfs_entry *items;
	size_t count;
	size_t capacity;
};

static int entries_init(struct rootfs_entries *entries)
{
	entries->items = calloc(ROOTFS_INITIAL_ENTRIES,
				sizeof(entries->items[0]));
	if (entries->items == NULL) {
		fprintf(stderr, "mkrootfs: alloc entries: %s\n",
			strerror(errno));
		return -1;
	}
	entries->count = 0;
	entries->capacity = ROOTFS_INITIAL_ENTRIES;
	return 0;
}

static void entries_free(struct rootfs_entries *entries)
{
	free(entries->items);
	entries->items = NULL;
	entries->count = 0;
	entries->capacity = 0;
}

static int entries_reserve_one(struct rootfs_entries *entries,
			       const char *path)
{
	if (entries->count < entries->capacity) {
		return 0;
	}

	const size_t next_capacity = entries->capacity == 0 ?
				     ROOTFS_INITIAL_ENTRIES :
				     entries->capacity * 2;
	struct rootfs_entry *next =
		realloc(entries->items, next_capacity * sizeof(next[0]));
	if (next == NULL) {
		fprintf(stderr, "mkrootfs: grow entries for %s: %s\n", path,
			strerror(errno));
		return -1;
	}
	memset(next + entries->capacity, 0,
	       (next_capacity - entries->capacity) * sizeof(next[0]));
	entries->items = next;
	entries->capacity = next_capacity;
	return 0;
}

static struct rootfs_entry *entries_append(struct rootfs_entries *entries,
					   const char *path)
{
	if (entries_reserve_one(entries, path) != 0) {
		return NULL;
	}

	return &entries->items[entries->count++];
}

static int path_exists(const struct rootfs_entries *entries,
		       const char *path)
{
	for (size_t i = 0; i < entries->count; i++) {
		if (strcmp(entries->items[i].path, path) == 0) {
			return 1;
		}
	}

	return 0;
}

static int add_directory(struct rootfs_entries *entries,
			 const char *path)
{
	struct rootfs_entry *entry;

	if (path_exists(entries, path)) {
		return 0;
	}
	entry = entries_append(entries, path);
	if (entry == NULL) {
		return -1;
	}

	strcpy(entry->path, path);
	entry->uid = 0;
	entry->gid = 0;
	entry->mode = 0555;
	entry->type = ROOTFS_TYPE_DIRECTORY;
	return 0;
}

static int add_parent_directories(struct rootfs_entries *entries,
				  const char *path)
{
	char current[ROOTFS_MAX_PATH];
	size_t len = 0;

	current[len++] = '/';
	current[len] = '\0';
	for (size_t i = 1; path[i] != '\0'; i++) {
		if (path[i] != '/') {
			if (len + 1 >= sizeof(current)) {
				return -1;
			}
			current[len++] = path[i];
			current[len] = '\0';
			continue;
		}
		if (len > 1 && add_directory(entries, current) != 0) {
			return -1;
		}
		if (len + 1 >= sizeof(current)) {
			return -1;
		}
		current[len++] = '/';
		current[len] = '\0';
	}
	return 0;
}

static long file_size(FILE *file)
{
	if (fseek(file, 0, SEEK_END) != 0) {
		return -1;
	}

	const long size = ftell(file);
	if (size < 0 || fseek(file, 0, SEEK_SET) != 0) {
		return -1;
	}

	return size;
}

static int copy_bytes(FILE *out, const char *path, const unsigned char *data,
		      size_t len)
{
	if (len != 0 && fwrite(data, 1, len, out) != len) {
		fprintf(stderr, "mkrootfs: write %s: %s\n", path,
			strerror(errno));
		return -1;
	}
	return 0;
}

static int copy_file(FILE *out, const char *path)
{
	FILE *in = fopen(path, "rb");
	unsigned char buffer[4096];

	if (in == NULL) {
		fprintf(stderr, "mkrootfs: open %s: %s\n", path, strerror(errno));
		return -1;
	}

	for (;;) {
		const size_t n = fread(buffer, 1, sizeof(buffer), in);

		if (n != 0 && fwrite(buffer, 1, n, out) != n) {
			fprintf(stderr, "mkrootfs: write image: %s\n",
				strerror(errno));
			fclose(in);
			return -1;
		}

		if (n < sizeof(buffer)) {
			if (ferror(in)) {
				fprintf(stderr, "mkrootfs: read %s: %s\n", path,
					strerror(errno));
				fclose(in);
				return -1;
			}
			break;
		}
	}

	fclose(in);
	return 0;
}

int main(int argc, char **argv)
{
	if (argc < 4) {
		fprintf(stderr,
			"usage: mkrootfs OUT PATH FILE [--symlink PATH TARGET] [--dir PATH] ...\n");
		return 1;
	}

	const char *out_path = argv[1];
	struct rootfs_header header = {
		.magic = ROOTFS_MAGIC,
		.entries = 0,
	};
	struct rootfs_entries entries;
	int result = 1;

	if (entries_init(&entries) != 0) {
		return 1;
	}
	if (add_directory(&entries, "/") != 0 ||
	    add_directory(&entries, "/dev") != 0) {
		goto out;
	}
	for (int arg = 2; arg < argc;) {
		const int is_symlink = strcmp(argv[arg], "--symlink") == 0;
		const int is_dir = strcmp(argv[arg], "--dir") == 0;
		const char *path;
		const char *file;
		FILE *in;
		long size = 0;

		if (is_dir) {
			if (arg + 1 >= argc) {
				fprintf(stderr, "mkrootfs: incomplete dir\n");
				goto out;
			}
			path = argv[arg + 1];
			file = NULL;
			arg += 2;
		} else if (is_symlink) {
			if (arg + 2 >= argc) {
				fprintf(stderr, "mkrootfs: incomplete symlink\n");
				goto out;
			}
			path = argv[arg + 1];
			file = argv[arg + 2];
			arg += 3;
		} else {
			if (arg + 1 >= argc) {
				fprintf(stderr, "mkrootfs: missing file for %s\n",
					argv[arg]);
				goto out;
			}
			path = argv[arg];
			file = argv[arg + 1];
			arg += 2;
		}

		if (strlen(path) >= ROOTFS_MAX_PATH) {
			fprintf(stderr, "mkrootfs: path too long: %s\n", path);
			goto out;
		}
		if (path[0] != '/' ||
		    (!is_dir && path_exists(&entries, path))) {
			fprintf(stderr, "mkrootfs: invalid duplicate path: %s\n",
				path);
			goto out;
		}
		if (add_parent_directories(&entries, path) != 0) {
			fprintf(stderr, "mkrootfs: add parents for %s\n", path);
			goto out;
		}
		if (is_dir && path_exists(&entries, path)) {
			continue;
		}

		if (is_dir) {
			size = 0;
		} else if (is_symlink) {
			if (strlen(file) >= ROOTFS_MAX_PATH) {
				fprintf(stderr,
					"mkrootfs: symlink target too long: %s\n",
					file);
				goto out;
			}
			size = (long)strlen(file);
		} else {
			in = fopen(file, "rb");
			if (in == NULL) {
				fprintf(stderr, "mkrootfs: open %s: %s\n",
					file, strerror(errno));
				goto out;
			}
			size = file_size(in);
			fclose(in);
			if (size < 0) {
				fprintf(stderr, "mkrootfs: size %s: %s\n",
					file, strerror(errno));
				goto out;
			}
		}

		struct rootfs_entry *entry = entries_append(&entries, path);
		if (entry == NULL) {
			goto out;
		}
		strcpy(entry->path, path);
		entry->size = (uint64_t)size;
		entry->uid = 0;
		entry->gid = 0;
		entry->mode = is_dir ? 0555 :
				  (is_symlink ? 0777 :
				  (strncmp(path, "/bin/", 5) == 0 ?
				   0555 : 0444));
		if (strcmp(path, "/secret.txt") == 0) {
			entry->mode = 0400;
		}
		if (strcmp(path, "/etc/shadow") == 0) {
			entry->mode = 0400;
		}
		entry->type = is_dir ? ROOTFS_TYPE_DIRECTORY :
			      (is_symlink ?
			       ROOTFS_TYPE_SYMLINK :
			       ROOTFS_TYPE_REGULAR);
	}

	if (entries.count > UINT32_MAX) {
		fprintf(stderr, "mkrootfs: too many entries for image\n");
		goto out;
	}
	header.entries = (uint32_t)entries.count;
	uint64_t offset = sizeof(header) +
			  entries.count * sizeof(entries.items[0]);
	for (size_t i = 0; i < entries.count; i++) {
		if (entries.items[i].type == ROOTFS_TYPE_REGULAR ||
		    entries.items[i].type == ROOTFS_TYPE_SYMLINK) {
			entries.items[i].offset = offset;
			offset += entries.items[i].size;
		}
	}

	FILE *out = fopen(out_path, "wb");
	if (out == NULL) {
		fprintf(stderr, "mkrootfs: open %s: %s\n", out_path,
			strerror(errno));
		goto out;
	}

	if (fwrite(&header, sizeof(header), 1, out) != 1 ||
	    fwrite(entries.items, sizeof(entries.items[0]), entries.count, out) !=
	    entries.count) {
		fprintf(stderr, "mkrootfs: write header: %s\n", strerror(errno));
		fclose(out);
		goto out;
	}

	for (int arg = 2; arg < argc;) {
		if (strcmp(argv[arg], "--dir") == 0) {
			arg += 2;
		} else if (strcmp(argv[arg], "--symlink") == 0) {
			const char *path = argv[arg + 1];
			const char *target = argv[arg + 2];

			if (copy_bytes(out, path,
				       (const unsigned char *)target,
				       strlen(target)) != 0) {
				fclose(out);
				goto out;
			}
			arg += 3;
		} else {
			if (copy_file(out, argv[arg + 1]) != 0) {
				fclose(out);
				goto out;
			}
			arg += 2;
		}
	}

	if (fclose(out) != 0) {
		fprintf(stderr, "mkrootfs: close %s: %s\n", out_path,
			strerror(errno));
		goto out;
	}

	result = 0;

out:
	entries_free(&entries);
	return result;
}
