.section .text, "ax"
.global _start
.type _start, @function
_start:
	call main
	movq %rax, %rdi
	movq $-2, %rax
	syscall

.halt:
	jmp .halt
