#include <errno.h>
#include <elf.h>
#include <fcntl.h>
#include <stdint.h>
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

enum {
	ELF_PAGE_SIZE = 4096,
};

static uintptr_t page_down(uintptr_t value)
{
	return value & ~(uintptr_t)(ELF_PAGE_SIZE - 1);
}

static uintptr_t page_up(uintptr_t value)
{
	return (value + ELF_PAGE_SIZE - 1) & ~(uintptr_t)(ELF_PAGE_SIZE - 1);
}

static int phdr_prot(const Elf64_Phdr *ph)
{
	int prot = 0;

	if ((ph->p_flags & PF_R) != 0) {
		prot |= PROT_READ;
	}
	if ((ph->p_flags & PF_W) != 0) {
		prot |= PROT_WRITE;
	}
	if ((ph->p_flags & PF_X) != 0) {
		prot |= PROT_EXEC;
	}
	return prot;
}

static int read_exact_at(int fd, off_t offset, void *buffer, size_t len)
{
	unsigned char *cursor = buffer;

	if (lseek(fd, offset, SEEK_SET) < 0) {
		return -1;
	}
	while (len != 0) {
		ssize_t nread = read(fd, cursor, len);

		if (nread < 0) {
			if (errno == EINTR) {
				continue;
			}
			return -1;
		}
		if (nread == 0) {
			errno = EIO;
			return -1;
		}
		cursor += nread;
		len -= (size_t)nread;
	}
	return 0;
}

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

static int expect_segment_map(const char *path)
{
	Elf64_Ehdr ehdr;
	Elf64_Phdr phdrs[16];
	const Elf64_Phdr *first = NULL;
	uintptr_t addr_min = UINTPTR_MAX;
	uintptr_t addr_max = 0;
	uintptr_t map_len;
	uintptr_t off_start = 0;
	unsigned int load_count = 0;
	unsigned int i;
	void *map;
	unsigned char *base;
	int fd = open(path, O_RDONLY | O_CLOEXEC);

	if (fd < 0) {
		fprintf(stderr, "ldsopathtest: segment open %s: %s\n",
			path, strerror(errno));
		return -1;
	}
	if (read_exact_at(fd, 0, &ehdr, sizeof(ehdr)) != 0 ||
	    memcmp(ehdr.e_ident, ELFMAG, SELFMAG) != 0 ||
	    ehdr.e_ident[EI_CLASS] != ELFCLASS64 ||
	    ehdr.e_phnum > sizeof(phdrs) / sizeof(phdrs[0]) ||
	    ehdr.e_phentsize != sizeof(phdrs[0])) {
		fprintf(stderr, "ldsopathtest: bad segment ELF %s errno=%d phnum=%u phentsize=%u\n",
			path, errno, (unsigned int)ehdr.e_phnum,
			(unsigned int)ehdr.e_phentsize);
		close(fd);
		return -1;
	}
	if (read_exact_at(fd, (off_t)ehdr.e_phoff, phdrs,
			  ehdr.e_phnum * sizeof(phdrs[0])) != 0) {
		fprintf(stderr, "ldsopathtest: segment phdr read %s: %s\n",
			path, strerror(errno));
		close(fd);
		return -1;
	}
	for (i = 0; i < ehdr.e_phnum; i++) {
		const Elf64_Phdr *ph = &phdrs[i];

		if (ph->p_type != PT_LOAD) {
			continue;
		}
		load_count++;
		if (ph->p_vaddr < addr_min) {
			addr_min = ph->p_vaddr;
			off_start = ph->p_offset;
			first = ph;
		}
		if (ph->p_vaddr + ph->p_memsz > addr_max) {
			addr_max = ph->p_vaddr + ph->p_memsz;
		}
	}
	if (load_count == 0 || first == NULL) {
		fprintf(stderr, "ldsopathtest: no load segments %s\n", path);
		close(fd);
		return -1;
	}
	addr_min = page_down(addr_min);
	addr_max = page_up(addr_max);
	off_start = page_down(off_start);
	map_len = addr_max - addr_min + off_start;
	map = mmap((void *)addr_min, map_len, phdr_prot(first), MAP_PRIVATE,
		   fd, (off_t)off_start);
	if (map == MAP_FAILED) {
		fprintf(stderr, "ldsopathtest: segment initial mmap %s len=%llu off=%llu: %s\n",
			path, (unsigned long long)map_len,
			(unsigned long long)off_start, strerror(errno));
		close(fd);
		return -1;
	}
	base = (unsigned char *)map - addr_min;
	for (i = 0; i < ehdr.e_phnum; i++) {
		const Elf64_Phdr *ph = &phdrs[i];
		uintptr_t this_min;
		uintptr_t this_max;
		void *fixed;

		if (ph->p_type != PT_LOAD) {
			continue;
		}
		this_min = page_down(ph->p_vaddr);
		if (this_min == addr_min) {
			continue;
		}
		this_max = page_up(ph->p_vaddr + ph->p_memsz);
		fixed = mmap(base + this_min, this_max - this_min,
			     phdr_prot(ph), MAP_PRIVATE | MAP_FIXED, fd,
			     (off_t)page_down(ph->p_offset));
		if (fixed == MAP_FAILED) {
			fprintf(stderr, "ldsopathtest: segment fixed mmap %s vaddr=%llx len=%llu off=%llu: %s\n",
				path, (unsigned long long)this_min,
				(unsigned long long)(this_max - this_min),
				(unsigned long long)page_down(ph->p_offset),
				strerror(errno));
			munmap(map, map_len);
			close(fd);
			return -1;
		}
	}
	if (munmap(map, map_len) != 0) {
		fprintf(stderr, "ldsopathtest: segment munmap %s: %s\n",
			path, strerror(errno));
		close(fd);
		return -1;
	}
	if (close(fd) != 0) {
		fprintf(stderr, "ldsopathtest: segment close %s: %s\n",
			path, strerror(errno));
		return -1;
	}
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
	return expect_elf(path, id) == 0 && expect_segment_map(path) == 0 ? 0 : -1;
}

int main(void)
{
	struct file_id openrc;
	struct file_id librc;
	struct file_id libeinfo;

	if (expect_elf("/sbin/openrc", &openrc) != 0 ||
	    expect_segment_map("/sbin/openrc") != 0 ||
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
