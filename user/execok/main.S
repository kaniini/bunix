.section .text, "ax"
.global _start
.type _start, @function
_start:
	movq $1, %rax
	movq $1, %rdi
	leaq message(%rip), %rsi
	movq $message_end - message, %rdx
	syscall

	movq $231, %rax
	xorq %rdi, %rdi
	syscall

.halt:
	jmp .halt

.section .rodata, "a"
message:
	.ascii "linux exec child ok\n"
message_end:
