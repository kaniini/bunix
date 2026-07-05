ENTRY(_start)
PROVIDE(USER_BASE = 0x400000);

PHDRS
{
	text PT_LOAD FLAGS(5);
	pad01 PT_LOAD FLAGS(4);
	pad02 PT_LOAD FLAGS(4);
	pad03 PT_LOAD FLAGS(4);
	pad04 PT_LOAD FLAGS(4);
	pad05 PT_LOAD FLAGS(4);
	pad06 PT_LOAD FLAGS(4);
	pad07 PT_LOAD FLAGS(4);
	pad08 PT_LOAD FLAGS(4);
	pad09 PT_LOAD FLAGS(4);
	pad10 PT_LOAD FLAGS(4);
	pad11 PT_LOAD FLAGS(4);
	pad12 PT_LOAD FLAGS(4);
	pad13 PT_LOAD FLAGS(4);
	pad14 PT_LOAD FLAGS(4);
	pad15 PT_LOAD FLAGS(4);
	data PT_LOAD FLAGS(6);
}

SECTIONS
{
	. = USER_BASE;

	.text ALIGN(4K) : {
		*(.text .text.*)
		*(.rodata .rodata.*)
	} :text

	.pad01 ALIGN(4K) : { BYTE(1) } :pad01
	.pad02 ALIGN(4K) : { BYTE(2) } :pad02
	.pad03 ALIGN(4K) : { BYTE(3) } :pad03
	.pad04 ALIGN(4K) : { BYTE(4) } :pad04
	.pad05 ALIGN(4K) : { BYTE(5) } :pad05
	.pad06 ALIGN(4K) : { BYTE(6) } :pad06
	.pad07 ALIGN(4K) : { BYTE(7) } :pad07
	.pad08 ALIGN(4K) : { BYTE(8) } :pad08
	.pad09 ALIGN(4K) : { BYTE(9) } :pad09
	.pad10 ALIGN(4K) : { BYTE(10) } :pad10
	.pad11 ALIGN(4K) : { BYTE(11) } :pad11
	.pad12 ALIGN(4K) : { BYTE(12) } :pad12
	.pad13 ALIGN(4K) : { BYTE(13) } :pad13
	.pad14 ALIGN(4K) : { BYTE(14) } :pad14
	.pad15 ALIGN(4K) : { BYTE(15) } :pad15

	.data ALIGN(4K) : {
		*(.data .data.*)
		*(COMMON)
		*(.bss .bss.*)
	} :data
}
