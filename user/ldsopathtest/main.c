#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

struct file_id {
	dev_t dev;
	ino_t ino;
	off_t size;
};

static int expect_missing(const char *path)
{
	int fd = open(path, O_RDONLY | O_CLOEXEC);

	if (fd >= 0) {
		fprintf(stderr, "ldsopathtest: unexpected open %s\n", path);
		close(fd);
		return -1;
	}
	if (errno != ENOENT && errno != ENOTDIR && errno != EACCES) {
		fprintf(stderr, "ldsopathtest: open %s errno=%d %s\n",
			path, errno, strerror(errno));
		return -1;
	}
	return 0;
}

static int expect_elf(const char *path, struct file_id *id)
{
	unsigned char ident[4];
	void *mapping;
	struct stat st;
	ssize_t nread;
	int fd = open(path, O_RDONLY | O_CLOEXEC);

	if (fd < 0) {
		fprintf(stderr, "ldsopathtest: open %s: %s\n",
			path, strerror(errno));
		return -1;
	}
	if (fstat(fd, &st) != 0) {
		fprintf(stderr, "ldsopathtest: fstat %s: %s\n",
			path, strerror(errno));
		close(fd);
		return -1;
	}
	if (!S_ISREG(st.st_mode) || st.st_size < 4) {
		fprintf(stderr, "ldsopathtest: bad stat %s mode=%o size=%lld\n",
			path, (unsigned int)st.st_mode, (long long)st.st_size);
		close(fd);
		return -1;
	}
	nread = read(fd, ident, sizeof(ident));
	if (nread != (ssize_t)sizeof(ident) ||
	    ident[0] != 0x7f || ident[1] != 'E' ||
	    ident[2] != 'L' || ident[3] != 'F') {
		fprintf(stderr, "ldsopathtest: bad elf header %s nread=%zd\n",
			path, nread);
		close(fd);
		return -1;
	}
	mapping = mmap(NULL, (size_t)st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (mapping == MAP_FAILED) {
		fprintf(stderr, "ldsopathtest: mmap %s: %s\n",
			path, strerror(errno));
		close(fd);
		return -1;
	}
	if (((const unsigned char *)mapping)[0] != 0x7f ||
	    ((const unsigned char *)mapping)[1] != 'E' ||
	    ((const unsigned char *)mapping)[2] != 'L' ||
	    ((const unsigned char *)mapping)[3] != 'F') {
		fprintf(stderr, "ldsopathtest: bad mapped elf header %s\n",
			path);
		munmap(mapping, (size_t)st.st_size);
		close(fd);
		return -1;
	}
	if (munmap(mapping, (size_t)st.st_size) != 0) {
		fprintf(stderr, "ldsopathtest: munmap %s: %s\n",
			path, strerror(errno));
		close(fd);
		return -1;
	}
	if (close(fd) != 0) {
		fprintf(stderr, "ldsopathtest: close %s: %s\n",
			path, strerror(errno));
		return -1;
	}
	id->dev = st.st_dev;
	id->ino = st.st_ino;
	id->size = st.st_size;
	return 0;
}

static int check_library(const char *name, struct file_id *id)
{
	char path[128];

	snprintf(path, sizeof(path), "/lib/%s", name);
	if (expect_missing(path) != 0) {
		return -1;
	}
	snprintf(path, sizeof(path), "/usr/local/lib/%s", name);
	if (expect_missing(path) != 0) {
		return -1;
	}
	snprintf(path, sizeof(path), "/usr/lib/%s", name);
	return expect_elf(path, id);
}

int main(void)
{
	struct file_id openrc;
	struct file_id librc;
	struct file_id libeinfo;

	if (expect_elf("/sbin/openrc", &openrc) != 0 ||
	    check_library("librc.so.1", &librc) != 0 ||
	    check_library("libeinfo.so.1", &libeinfo) != 0) {
		return 1;
	}
	if (librc.dev == libeinfo.dev && librc.ino == libeinfo.ino) {
		fprintf(stderr, "ldsopathtest: duplicate DSO inode dev=%llu ino=%llu\n",
			(unsigned long long)librc.dev,
			(unsigned long long)librc.ino);
		return 1;
	}
	printf("linux ldso path ok openrc=%llu:%llu:%lld librc=%llu:%llu:%lld libeinfo=%llu:%llu:%lld\n",
	       (unsigned long long)openrc.dev, (unsigned long long)openrc.ino,
	       (long long)openrc.size,
	       (unsigned long long)librc.dev, (unsigned long long)librc.ino,
	       (long long)librc.size,
	       (unsigned long long)libeinfo.dev,
	       (unsigned long long)libeinfo.ino, (long long)libeinfo.size);
	return 0;
}
