.section .text, "ax"
.global _start
.type _start, @function
_start:
	ld a0, 0(sp)
	addi a1, sp, 8
	slli t0, a0, 3
	addi t0, t0, 16
	add a2, sp, t0
	call main
	li a7, -2
	ecall

.halt:
	wfi
	j .halt
