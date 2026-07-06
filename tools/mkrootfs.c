#include <errno.h>
#include <dirent.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

enum {
	ROOTFS_MAGIC = 0x30534652,
	ROOTFS_MAX_PATH = 4096,
	ROOTFS_INITIAL_ENTRIES = 128,
	ROOTFS_TYPE_REGULAR = 1,
	ROOTFS_TYPE_DIRECTORY = 2,
	ROOTFS_TYPE_SYMLINK = 3,
};

struct rootfs_header {
	uint32_t magic;
	uint32_t entries;
};

struct rootfs_disk_entry {
	char path[ROOTFS_MAX_PATH];
	uint64_t offset;
	uint64_t size;
	uint32_t uid;
	uint32_t gid;
	uint32_t mode;
	uint32_t type;
};

struct rootfs_entry {
	struct rootfs_disk_entry disk;
	char source[ROOTFS_MAX_PATH];
	char inline_data[ROOTFS_MAX_PATH];
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
		if (strcmp(entries->items[i].disk.path, path) == 0) {
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

	strcpy(entry->disk.path, path);
	entry->disk.uid = 0;
	entry->disk.gid = 0;
	entry->disk.mode = 0555;
	entry->disk.type = ROOTFS_TYPE_DIRECTORY;
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

static int append_tree_entry(struct rootfs_entries *entries, const char *path,
			     const char *source, const char *inline_data,
			     const struct stat *st, uint32_t type)
{
	struct rootfs_entry *entry;

	if (strlen(path) >= ROOTFS_MAX_PATH) {
		fprintf(stderr, "mkrootfs: path too long: %s\n", path);
		return -1;
	}
	if (add_parent_directories(entries, path) != 0) {
		fprintf(stderr, "mkrootfs: add parents for %s\n", path);
		return -1;
	}
	if (path_exists(entries, path)) {
		if (type == ROOTFS_TYPE_DIRECTORY) {
			for (size_t i = 0; i < entries->count; i++) {
				if (strcmp(entries->items[i].disk.path, path) == 0) {
					entries->items[i].disk.mode =
						st->st_mode & 07777;
					return 0;
				}
			}
		}
		fprintf(stderr, "mkrootfs: invalid duplicate path: %s\n", path);
		return -1;
	}

	entry = entries_append(entries, path);
	if (entry == NULL) {
		return -1;
	}

	strcpy(entry->disk.path, path);
	entry->disk.uid = 0;
	entry->disk.gid = 0;
	entry->disk.mode = st->st_mode & 07777;
	entry->disk.type = type;
	if (type == ROOTFS_TYPE_REGULAR) {
		if ((uint64_t)st->st_size > INT64_MAX) {
			fprintf(stderr, "mkrootfs: file too large: %s\n", source);
			return -1;
		}
		entry->disk.size = (uint64_t)st->st_size;
		strcpy(entry->source, source);
	} else if (type == ROOTFS_TYPE_SYMLINK) {
		entry->disk.size = strlen(inline_data);
		strcpy(entry->inline_data, inline_data);
	}
	return 0;
}

static int import_tree_path(struct rootfs_entries *entries, const char *root,
			    const char *path)
{
	char source[ROOTFS_MAX_PATH];
	struct stat st;

	if (strcmp(path, "/") == 0) {
		snprintf(source, sizeof(source), "%s", root);
	} else if (snprintf(source, sizeof(source), "%s%s", root, path) >=
		   (int)sizeof(source)) {
		fprintf(stderr, "mkrootfs: source path too long: %s%s\n", root,
			path);
		return -1;
	}

	if (lstat(source, &st) != 0) {
		fprintf(stderr, "mkrootfs: stat %s: %s\n", source,
			strerror(errno));
		return -1;
	}

	if (S_ISLNK(st.st_mode)) {
		char target[ROOTFS_MAX_PATH];
		const ssize_t len = readlink(source, target, sizeof(target) - 1);

		if (len < 0) {
			fprintf(stderr, "mkrootfs: readlink %s: %s\n", source,
				strerror(errno));
			return -1;
		}
		target[len] = '\0';
		return append_tree_entry(entries, path, source, target, &st,
					 ROOTFS_TYPE_SYMLINK);
	}

	if (S_ISREG(st.st_mode)) {
		return append_tree_entry(entries, path, source, NULL, &st,
					 ROOTFS_TYPE_REGULAR);
	}

	if (S_ISDIR(st.st_mode)) {
		struct dirent **names = NULL;
		const int count = scandir(source, &names, NULL, alphasort);

		if (strcmp(path, "/") != 0 &&
		    append_tree_entry(entries, path, source, NULL, &st,
				      ROOTFS_TYPE_DIRECTORY) != 0) {
			return -1;
		}
		if (count < 0) {
			fprintf(stderr, "mkrootfs: opendir %s: %s\n", source,
				strerror(errno));
			return -1;
		}
		for (int i = 0; i < count; i++) {
			char child[ROOTFS_MAX_PATH];
			int rc = 0;

			if (strcmp(names[i]->d_name, ".") == 0 ||
			    strcmp(names[i]->d_name, "..") == 0) {
				free(names[i]);
				continue;
			}
			if (strcmp(path, "/") == 0) {
				rc = snprintf(child, sizeof(child), "/%s",
					      names[i]->d_name);
			} else {
				rc = snprintf(child, sizeof(child), "%s/%s",
					      path, names[i]->d_name);
			}
			free(names[i]);
			if (rc < 0 || rc >= (int)sizeof(child) ||
			    import_tree_path(entries, root, child) != 0) {
				for (int j = i + 1; j < count; j++) {
					free(names[j]);
				}
				free(names);
				return -1;
			}
		}
		free(names);
		return 0;
	}

	return 0;
}

int main(int argc, char **argv)
{
	if (argc < 3) {
		fprintf(stderr,
			"usage: mkrootfs OUT PATH FILE [--symlink PATH TARGET] [--dir PATH] [--tree DIR] ...\n");
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
		const int is_tree = strcmp(argv[arg], "--tree") == 0;
		const char *path;
		const char *file;
		FILE *in;
		long size = 0;

		if (is_tree) {
			if (arg + 1 >= argc) {
				fprintf(stderr, "mkrootfs: incomplete tree\n");
				goto out;
			}
			if (import_tree_path(&entries, argv[arg + 1], "/") != 0) {
				goto out;
			}
			arg += 2;
			continue;
		}

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
		strcpy(entry->disk.path, path);
		entry->disk.size = (uint64_t)size;
		entry->disk.uid = 0;
		entry->disk.gid = 0;
		entry->disk.mode = is_dir ? 0555 :
				    (is_symlink ? 0777 :
				    (strncmp(path, "/bin/", 5) == 0 ?
				     0555 : 0444));
		if (!is_dir && !is_symlink) {
			strcpy(entry->source, file);
		}
		if (is_symlink) {
			strcpy(entry->inline_data, file);
		}
		if (strcmp(path, "/secret.txt") == 0) {
			entry->disk.mode = 0400;
		}
		if (strcmp(path, "/etc/shadow") == 0) {
			entry->disk.mode = 0400;
		}
		entry->disk.type = is_dir ? ROOTFS_TYPE_DIRECTORY :
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
			  entries.count * sizeof(entries.items[0].disk);
	for (size_t i = 0; i < entries.count; i++) {
		if (entries.items[i].disk.type == ROOTFS_TYPE_REGULAR ||
		    entries.items[i].disk.type == ROOTFS_TYPE_SYMLINK) {
			entries.items[i].disk.offset = offset;
			offset += entries.items[i].disk.size;
		}
	}

	FILE *out = fopen(out_path, "wb");
	if (out == NULL) {
		fprintf(stderr, "mkrootfs: open %s: %s\n", out_path,
			strerror(errno));
		goto out;
	}

	if (fwrite(&header, sizeof(header), 1, out) != 1) {
		fprintf(stderr, "mkrootfs: write header: %s\n", strerror(errno));
		fclose(out);
		goto out;
	}
	for (size_t i = 0; i < entries.count; i++) {
		if (fwrite(&entries.items[i].disk,
			   sizeof(entries.items[i].disk), 1, out) != 1) {
			fprintf(stderr, "mkrootfs: write entry: %s\n",
				strerror(errno));
			fclose(out);
			goto out;
		}
	}

	for (size_t i = 0; i < entries.count; i++) {
		const struct rootfs_entry *entry = &entries.items[i];

		if (entry->disk.type == ROOTFS_TYPE_DIRECTORY) {
			continue;
		}
		if (entry->disk.type == ROOTFS_TYPE_SYMLINK) {
			if (copy_bytes(out, entry->disk.path,
				       (const unsigned char *)entry->inline_data,
				       entry->disk.size) != 0) {
				fclose(out);
				goto out;
			}
			continue;
		}
		if (copy_file(out, entry->source) != 0) {
			fclose(out);
			goto out;
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
