#include "console.h"
#include "elf.h"
#include "vm.h"

enum {
	ELF_MAGIC = 0x464c457f,
	ELFCLASS64 = 2,
	ELFDATA2LSB = 1,
	ET_EXEC = 2,
	EM_X86_64 = 62,
	PT_LOAD = 1,
	PF_W = 1 << 1,
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

static u64 align_down(u64 value, u64 align)
{
	return value & ~(align - 1);
}

static u64 align_up(u64 value, u64 align)
{
	return (value + align - 1) & ~(align - 1);
}

static u64 min_u64(u64 left, u64 right)
{
	return left < right ? left : right;
}

static u64 max_u64(u64 left, u64 right)
{
	return left > right ? left : right;
}

static int load_segment_page(struct vm_space *space, u64 image_start,
			     const struct elf64_phdr *phdr, u64 page_vaddr)
{
	const u64 segment_file_start = phdr->vaddr;
	const u64 segment_file_end = phdr->vaddr + phdr->filesz;
	const u64 page_end = page_vaddr + VM_PAGE_SIZE;
	const u64 copy_start = max_u64(page_vaddr, segment_file_start);
	const u64 copy_end = min_u64(page_end, segment_file_end);
	struct vm_frame frame = vm_rpc_alloc_frame();

	if (frame.addr == 0) {
		return -1;
	}

	mem_zero((u8 *)frame.addr, VM_PAGE_SIZE);

	if (copy_start < copy_end) {
		const u64 dst_offset = copy_start - page_vaddr;
		const u64 src_offset = phdr->offset + (copy_start - phdr->vaddr);
		mem_copy((u8 *)(frame.addr + dst_offset),
			 (const u8 *)(image_start + src_offset),
			 copy_end - copy_start);
	}

	if (vm_map_user_page(space, page_vaddr, frame,
			     (phdr->flags & PF_W) != 0) != 0) {
		vm_rpc_free_frame(frame);
		return -1;
	}

	return 0;
}

int elf_load_user_image(struct vm_space *space, u64 image_start, u64 image_end,
			u64 *entry)
{
	const struct elf64_ehdr *ehdr = (const struct elf64_ehdr *)image_start;
	const u64 image_size = image_end - image_start;

	if (space == 0 || image_size < sizeof(*ehdr) ||
	    read_magic(ehdr->ident) != ELF_MAGIC ||
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
		    phdr->filesz > phdr->memsz ||
		    phdr->vaddr + phdr->memsz < phdr->vaddr) {
			console_printf("elf: invalid load segment\n");
			return -1;
		}

		const u64 page_start = align_down(phdr->vaddr, VM_PAGE_SIZE);
		const u64 page_end = align_up(phdr->vaddr + phdr->memsz,
					      VM_PAGE_SIZE);

		for (u64 page = page_start; page < page_end;
		     page += VM_PAGE_SIZE) {
			if (load_segment_page(space, image_start, phdr, page) != 0) {
				console_printf("elf: failed map vaddr=%p\n",
					       (const void *)page);
				return -1;
			}
		}

		console_printf("elf: load vaddr=%p filesz=%u memsz=%u\n",
			       (const void *)phdr->vaddr,
			       (u32)phdr->filesz, (u32)phdr->memsz);
	}

	*entry = ehdr->entry;
	console_printf("elf: entry=%p\n", (const void *)*entry);
	return 0;
}
