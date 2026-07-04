#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
	ROOTFS_MAGIC = 0x30534652,
	ROOTFS_MAX_PATH = 128,
	ROOTFS_MAX_ENTRIES = 128,
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

static int path_exists(const struct rootfs_entry *entries, size_t count,
		       const char *path)
{
	for (size_t i = 0; i < count; i++) {
		if (strcmp(entries[i].path, path) == 0) {
			return 1;
		}
	}

	return 0;
}

static int add_directory(struct rootfs_entry *entries, size_t *count,
			 const char *path)
{
	if (path_exists(entries, *count, path)) {
		return 0;
	}
	if (*count >= ROOTFS_MAX_ENTRIES) {
		fprintf(stderr, "mkrootfs: too many entries adding %s\n", path);
		return -1;
	}

	strcpy(entries[*count].path, path);
	entries[*count].uid = 0;
	entries[*count].gid = 0;
	entries[*count].mode = 0555;
	entries[*count].type = ROOTFS_TYPE_DIRECTORY;
	(*count)++;
	return 0;
}

static int add_parent_directories(struct rootfs_entry *entries, size_t *count,
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
		if (len > 1 && add_directory(entries, count, current) != 0) {
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
	struct rootfs_entry entries[ROOTFS_MAX_ENTRIES];
	size_t entry_count = 0;

	memset(entries, 0, sizeof(entries));
	if (add_directory(entries, &entry_count, "/") != 0 ||
	    add_directory(entries, &entry_count, "/dev") != 0) {
		return 1;
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
				return 1;
			}
			path = argv[arg + 1];
			file = NULL;
			arg += 2;
		} else if (is_symlink) {
			if (arg + 2 >= argc) {
				fprintf(stderr, "mkrootfs: incomplete symlink\n");
				return 1;
			}
			path = argv[arg + 1];
			file = argv[arg + 2];
			arg += 3;
		} else {
			if (arg + 1 >= argc) {
				fprintf(stderr, "mkrootfs: missing file for %s\n",
					argv[arg]);
				return 1;
			}
			path = argv[arg];
			file = argv[arg + 1];
			arg += 2;
		}

		if (entry_count >= ROOTFS_MAX_ENTRIES) {
			fprintf(stderr, "mkrootfs: too many entries\n");
			return 1;
		}
		if (strlen(path) >= ROOTFS_MAX_PATH) {
			fprintf(stderr, "mkrootfs: path too long: %s\n", path);
			return 1;
		}
		if (path[0] != '/' ||
		    (!is_dir && path_exists(entries, entry_count, path))) {
			fprintf(stderr, "mkrootfs: invalid duplicate path: %s\n",
				path);
			return 1;
		}
		if (add_parent_directories(entries, &entry_count, path) != 0) {
			fprintf(stderr, "mkrootfs: add parents for %s\n", path);
			return 1;
		}
		if (is_dir && path_exists(entries, entry_count, path)) {
			continue;
		}
		if (entry_count >= ROOTFS_MAX_ENTRIES) {
			fprintf(stderr, "mkrootfs: too many entries\n");
			return 1;
		}

		if (is_dir) {
			size = 0;
		} else if (is_symlink) {
			if (strlen(file) >= ROOTFS_MAX_PATH) {
				fprintf(stderr,
					"mkrootfs: symlink target too long: %s\n",
					file);
				return 1;
			}
			size = (long)strlen(file);
		} else {
			in = fopen(file, "rb");
			if (in == NULL) {
				fprintf(stderr, "mkrootfs: open %s: %s\n",
					file, strerror(errno));
				return 1;
			}
			size = file_size(in);
			fclose(in);
			if (size < 0) {
				fprintf(stderr, "mkrootfs: size %s: %s\n",
					file, strerror(errno));
				return 1;
			}
		}

		strcpy(entries[entry_count].path, path);
		entries[entry_count].size = (uint64_t)size;
		entries[entry_count].uid = 0;
		entries[entry_count].gid = 0;
		entries[entry_count].mode = is_dir ? 0555 :
				  (is_symlink ? 0777 :
				  (strncmp(path, "/bin/", 5) == 0 ?
				   0555 : 0444));
		if (strcmp(path, "/secret.txt") == 0) {
			entries[entry_count].mode = 0400;
		}
		if (strcmp(path, "/etc/shadow") == 0) {
			entries[entry_count].mode = 0400;
		}
		entries[entry_count].type = is_dir ? ROOTFS_TYPE_DIRECTORY :
					     (is_symlink ?
					      ROOTFS_TYPE_SYMLINK :
					      ROOTFS_TYPE_REGULAR);
		entry_count++;
	}

	header.entries = (uint32_t)entry_count;
	uint64_t offset = sizeof(header) + entry_count * sizeof(entries[0]);
	for (size_t i = 0; i < entry_count; i++) {
		if (entries[i].type == ROOTFS_TYPE_REGULAR ||
		    entries[i].type == ROOTFS_TYPE_SYMLINK) {
			entries[i].offset = offset;
			offset += entries[i].size;
		}
	}

	FILE *out = fopen(out_path, "wb");
	if (out == NULL) {
		fprintf(stderr, "mkrootfs: open %s: %s\n", out_path,
			strerror(errno));
		return 1;
	}

	if (fwrite(&header, sizeof(header), 1, out) != 1 ||
	    fwrite(entries, sizeof(entries[0]), entry_count, out) != entry_count) {
		fprintf(stderr, "mkrootfs: write header: %s\n", strerror(errno));
		fclose(out);
		return 1;
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
				return 1;
			}
			arg += 3;
		} else {
			if (copy_file(out, argv[arg + 1]) != 0) {
				fclose(out);
				return 1;
			}
			arg += 2;
		}
	}

	if (fclose(out) != 0) {
		fprintf(stderr, "mkrootfs: close %s: %s\n", out_path,
			strerror(errno));
		return 1;
	}

	return 0;
}
