.set CODE64_SEL, 0x08
.set DATA64_SEL, 0x10

.section .multiboot, "a"
.align 8
multiboot2_header:
	.long 0xe85250d6
	.long 0
	.long multiboot2_header_end - multiboot2_header
	.long -(0xe85250d6 + 0 + (multiboot2_header_end - multiboot2_header))

	/* End tag. */
	.align 8
	.short 0
	.short 0
	.long 8
multiboot2_header_end:

.section .bss, "aw", @nobits
.align 4096
.global arch_boot_pml4
arch_boot_pml4:
	.skip 4096
pdpt_table:
	.skip 4096
pd_table:
	.skip 4096

.align 4
boot_magic:
	.skip 4
boot_info:
	.skip 4

.align 16
stack_bottom:
	.skip 32768
stack_top:

.section .rodata, "a"
.align 8
gdt64:
	.quad 0
	.quad 0x00af9a000000ffff
	.quad 0x00af92000000ffff
gdt64_end:

gdt64_ptr:
	.short gdt64_end - gdt64 - 1
	.long gdt64

.section .text, "ax"
.code32
.global _start
.type _start, @function
_start:
	movl $stack_top, %esp

	movl %eax, boot_magic
	movl %ebx, boot_info

	call setup_page_tables
	lgdt gdt64_ptr

	movl %cr4, %eax
	orl $(1 << 5), %eax
	movl %eax, %cr4

	movl $arch_boot_pml4, %eax
	movl %eax, %cr3

	movl $0xc0000080, %ecx
	rdmsr
	orl $(1 << 8), %eax
	wrmsr

	movl %cr0, %eax
	orl $0x80000001, %eax
	movl %eax, %cr0

	ljmp $CODE64_SEL, $long_mode_start

setup_page_tables:
	movl $(pdpt_table + 0x003), %eax
	movl %eax, arch_boot_pml4
	movl $0, arch_boot_pml4 + 4

	movl $(pd_table + 0x003), %eax
	movl %eax, pdpt_table
	movl $0, pdpt_table + 4

	movl $pd_table, %edi
	movl $0x00000083, %eax
	movl $512, %ecx
.map_pd_entry:
	movl %eax, (%edi)
	movl $0, 4(%edi)
	addl $0x200000, %eax
	addl $8, %edi
	loop .map_pd_entry
	ret

.code64
long_mode_start:
	movw $DATA64_SEL, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %ss
	movw %ax, %fs
	movw %ax, %gs

	movq $stack_top, %rsp
	xorq %rbp, %rbp

	movl boot_magic, %edi
	movl boot_info, %esi
	call kernel_main

.halt:
	cli
	hlt
	jmp .halt
