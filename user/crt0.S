.section .text, "ax"
.global _start
.type _start, @function
_start:
	movq (%rsp), %rdi
	leaq 8(%rsp), %rsi
	leaq 16(%rsp,%rdi,8), %rdx
	call main
	movq %rax, %rdi
	movq $-2, %rax
	syscall

.halt:
	jmp .halt
