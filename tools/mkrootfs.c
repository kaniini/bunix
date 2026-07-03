#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
	ROOTFS_MAGIC = 0x30534652,
	ROOTFS_MAX_PATH = 32,
	ROOTFS_MAX_ENTRIES = 16,
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
	uint32_t reserved;
};

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
	if (argc < 4 || (argc % 2) != 0) {
		fprintf(stderr,
			"usage: mkrootfs OUT PATH0 FILE0 [PATH1 FILE1 ...]\n");
		return 1;
	}

	const size_t entry_count = (size_t)(argc - 2) / 2;
	if (entry_count > ROOTFS_MAX_ENTRIES) {
		fprintf(stderr, "mkrootfs: too many entries: %zu\n",
			entry_count);
		return 1;
	}

	const char *out_path = argv[1];
	struct rootfs_header header = {
		.magic = ROOTFS_MAGIC,
		.entries = (uint32_t)entry_count,
	};
	struct rootfs_entry entries[ROOTFS_MAX_ENTRIES];
	uint64_t offset = sizeof(header) + entry_count * sizeof(entries[0]);

	memset(entries, 0, sizeof(entries));
	for (size_t i = 0; i < entry_count; i++) {
		const char *path = argv[2 + i * 2];
		const char *file = argv[3 + i * 2];
		FILE *in;
		long size;

		if (strlen(path) >= ROOTFS_MAX_PATH) {
			fprintf(stderr, "mkrootfs: path too long: %s\n", path);
			return 1;
		}

		in = fopen(file, "rb");
		if (in == NULL) {
			fprintf(stderr, "mkrootfs: open %s: %s\n", file,
				strerror(errno));
			return 1;
		}
		size = file_size(in);
		fclose(in);
		if (size < 0) {
			fprintf(stderr, "mkrootfs: size %s: %s\n", file,
				strerror(errno));
			return 1;
		}

		strcpy(entries[i].path, path);
		entries[i].offset = offset;
		entries[i].size = (uint64_t)size;
		entries[i].uid = 0;
		entries[i].gid = 0;
		entries[i].mode = strncmp(path, "/bin/", 5) == 0 ?
				  0555 : 0444;
		if (strcmp(path, "/secret.txt") == 0) {
			entries[i].mode = 0400;
		}
		offset += (uint64_t)size;
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

	for (size_t i = 0; i < entry_count; i++) {
		if (copy_file(out, argv[3 + i * 2]) != 0) {
			fclose(out);
			return 1;
		}
	}

	if (fclose(out) != 0) {
		fprintf(stderr, "mkrootfs: close %s: %s\n", out_path,
			strerror(errno));
		return 1;
	}

	return 0;
}
