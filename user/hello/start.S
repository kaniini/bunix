.section .text, "ax"
.global _start
.type _start, @function
_start:
	movq $-1, %rax
	leaq message(%rip), %rdi
	movq $message_len, %rsi
	syscall

	movq $-2, %rax
	xorq %rdi, %rdi
	syscall

.halt:
	jmp .halt

.section .rodata, "a"
message:
	.ascii "hello: world <3\n"
message_len = . - message
