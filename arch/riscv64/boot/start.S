.section .text.boot, "ax"
.global _start
.type _start, @function
_start:
	.option push
	.option norelax
	la sp, boot_stack_top
	.option pop

	call riscv64_early_main

.halt:
	wfi
	j .halt

.section .bss, "aw", @nobits
.align 12
boot_stack:
	.skip 32768
boot_stack_top:
