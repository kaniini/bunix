ENTRY(_start)

PHDRS
{
	text PT_LOAD FLAGS(5);
	rodata PT_LOAD FLAGS(4);
	data PT_LOAD FLAGS(6);
}

SECTIONS
{
	/* Must match RISCV64_KERNEL_LOAD_BASE in arch/riscv64/include/arch/layout.h. */
	. = 0x80200000;
	__kernel_start = .;

	.text ALIGN(4K) : {
		*(.text.boot)
		*(.text .text.*)
	} :text

	.rodata ALIGN(4K) : {
		*(.rodata .rodata.*)
	} :rodata

	.data ALIGN(4K) : {
		*(.sdata .sdata.*)
		*(.data .data.*)
	} :data

	.bss ALIGN(4K) : {
		*(.sbss .sbss.*)
		*(COMMON)
		*(.bss .bss.*)
	} :data

	. = ALIGN(4K);
	__kernel_end = .;
}
