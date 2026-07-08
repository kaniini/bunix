ENTRY(_start)

SECTIONS
{
	. = 0x80200000;
	__kernel_start = .;

	.text ALIGN(4K) : {
		*(.text.boot)
		*(.text .text.*)
	}

	.rodata ALIGN(4K) : {
		*(.rodata .rodata.*)
	}

	.data ALIGN(4K) : {
		*(.sdata .sdata.*)
		*(.data .data.*)
	}

	.bss ALIGN(4K) : {
		*(.sbss .sbss.*)
		*(COMMON)
		*(.bss .bss.*)
	}

	. = ALIGN(4K);
	__kernel_end = .;
}
