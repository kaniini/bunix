.section .text, "ax"
.global _start
.type _start, @function
_start:
	movq $9, %rax
	xorq %rdi, %rdi
	movq $20971520, %rsi
	movq $3, %rdx
	movq $0x22, %r10
	movq $-1, %r8
	xorq %r9, %r9
	syscall
	testq %rax, %rax
	js .fail
	movq %rax, %r12

	movb $0x5a, (%r12)
	movb $0xa5, 20971519(%r12)
	cmpb $0x5a, (%r12)
	jne .fail
	cmpb $0xa5, 20971519(%r12)
	jne .fail

	movq $11, %rax
	movq %r12, %rdi
	movq $20971520, %rsi
	syscall
	testq %rax, %rax
	jne .fail

	movq $1, %rax
	movq $1, %rdi
	leaq message(%rip), %rsi
	movq $message_end - message, %rdx
	syscall
	cmpq $message_end - message, %rax
	jne .fail

	xorq %rdi, %rdi
	jmp .exit

.fail:
	movq $1, %rdi
.exit:
	movq $231, %rax
	syscall

.halt:
	jmp .halt

.section .rodata, "a"
message:
	.ascii "linux mmaphuge ok\n"
message_end:
