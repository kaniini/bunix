#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
	ROOTFS_MAGIC = 0x30534652,
	ROOTFS_MAX_PATH = 32,
};

struct rootfs_header {
	uint32_t magic;
	uint32_t entries;
};

struct rootfs_entry {
	char path[ROOTFS_MAX_PATH];
	uint64_t offset;
	uint64_t size;
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
	if (argc != 6) {
		fprintf(stderr,
			"usage: mkrootfs OUT PATH0 FILE0 PATH1 FILE1\n");
		return 1;
	}

	const char *out_path = argv[1];
	const char *paths[] = { argv[2], argv[4] };
	const char *files[] = { argv[3], argv[5] };
	struct rootfs_header header = {
		.magic = ROOTFS_MAGIC,
		.entries = 2,
	};
	struct rootfs_entry entries[2];
	uint64_t offset = sizeof(header) + sizeof(entries);

	memset(entries, 0, sizeof(entries));
	for (size_t i = 0; i < 2; i++) {
		FILE *in;
		long size;

		if (strlen(paths[i]) >= ROOTFS_MAX_PATH) {
			fprintf(stderr, "mkrootfs: path too long: %s\n", paths[i]);
			return 1;
		}

		in = fopen(files[i], "rb");
		if (in == NULL) {
			fprintf(stderr, "mkrootfs: open %s: %s\n", files[i],
				strerror(errno));
			return 1;
		}
		size = file_size(in);
		fclose(in);
		if (size < 0) {
			fprintf(stderr, "mkrootfs: size %s: %s\n", files[i],
				strerror(errno));
			return 1;
		}

		strcpy(entries[i].path, paths[i]);
		entries[i].offset = offset;
		entries[i].size = (uint64_t)size;
		offset += (uint64_t)size;
	}

	FILE *out = fopen(out_path, "wb");
	if (out == NULL) {
		fprintf(stderr, "mkrootfs: open %s: %s\n", out_path,
			strerror(errno));
		return 1;
	}

	if (fwrite(&header, sizeof(header), 1, out) != 1 ||
	    fwrite(entries, sizeof(entries), 1, out) != 1) {
		fprintf(stderr, "mkrootfs: write header: %s\n", strerror(errno));
		fclose(out);
		return 1;
	}

	for (size_t i = 0; i < 2; i++) {
		if (copy_file(out, files[i]) != 0) {
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
