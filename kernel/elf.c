#include "console.h"
#include "elf.h"

enum {
	ELF_MAGIC = 0x464c457f,
	ELFCLASS64 = 2,
	ELFDATA2LSB = 1,
	ET_EXEC = 2,
	EM_X86_64 = 62,
	PT_LOAD = 1,
};

struct elf64_ehdr {
	unsigned char ident[16];
	u16 type;
	u16 machine;
	u32 version;
	u64 entry;
	u64 phoff;
	u64 shoff;
	u32 flags;
	u16 ehsize;
	u16 phentsize;
	u16 phnum;
	u16 shentsize;
	u16 shnum;
	u16 shstrndx;
} __attribute__((packed));

struct elf64_phdr {
	u32 type;
	u32 flags;
	u64 offset;
	u64 vaddr;
	u64 paddr;
	u64 filesz;
	u64 memsz;
	u64 align;
} __attribute__((packed));

static void mem_copy(u8 *dst, const u8 *src, u64 len)
{
	for (u64 i = 0; i < len; i++) {
		dst[i] = src[i];
	}
}

static void mem_zero(u8 *dst, u64 len)
{
	for (u64 i = 0; i < len; i++) {
		dst[i] = 0;
	}
}

static u32 read_magic(const unsigned char *ident)
{
	return ((u32)ident[0]) | ((u32)ident[1] << 8) |
	       ((u32)ident[2] << 16) | ((u32)ident[3] << 24);
}

int elf_load_user_image(u64 image_start, u64 image_end, u64 *entry)
{
	const struct elf64_ehdr *ehdr = (const struct elf64_ehdr *)image_start;
	const u64 image_size = image_end - image_start;

	if (image_size < sizeof(*ehdr) || read_magic(ehdr->ident) != ELF_MAGIC ||
	    ehdr->ident[4] != ELFCLASS64 || ehdr->ident[5] != ELFDATA2LSB ||
	    ehdr->type != ET_EXEC || ehdr->machine != EM_X86_64) {
		console_printf("elf: invalid user image %p-%p\n",
			       (const void *)image_start, (const void *)image_end);
		return -1;
	}

	for (u16 i = 0; i < ehdr->phnum; i++) {
		const struct elf64_phdr *phdr =
			(const struct elf64_phdr *)(image_start + ehdr->phoff +
						    (u64)i * ehdr->phentsize);

		if (phdr->type != PT_LOAD) {
			continue;
		}

		if (phdr->offset + phdr->filesz > image_size ||
		    phdr->filesz > phdr->memsz) {
			console_printf("elf: invalid load segment\n");
			return -1;
		}

		mem_copy((u8 *)phdr->vaddr, (const u8 *)(image_start + phdr->offset),
			 phdr->filesz);
		mem_zero((u8 *)(phdr->vaddr + phdr->filesz),
			 phdr->memsz - phdr->filesz);
		console_printf("elf: load vaddr=%p filesz=%u memsz=%u\n",
			       (const void *)phdr->vaddr,
			       (u32)phdr->filesz, (u32)phdr->memsz);
	}

	*entry = ehdr->entry;
	console_printf("elf: entry=%p\n", (const void *)*entry);
	return 0;
}
