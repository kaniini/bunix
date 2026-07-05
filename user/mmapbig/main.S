.section .text, "ax"
.global _start
.type _start, @function
_start:
	movq $257, %rax
	movq $-100, %rdi
	leaq path(%rip), %rsi
	xorq %rdx, %rdx
	syscall
	testq %rax, %rax
	js .fail
	movq %rax, %r12

	movq $9, %rax
	xorq %rdi, %rdi
	movq $131072, %rsi
	movq $1, %rdx
	movq $2, %r10
	movq %r12, %r8
	xorq %r9, %r9
	syscall
	testq %rax, %rax
	js .fail
	movq %rax, %r13

	cmpl $0x464c457f, (%r13)
	jne .fail
	movb 70000(%r13), %al
	movb 131071(%r13), %al

	movq $11, %rax
	movq %r13, %rdi
	movq $131072, %rsi
	syscall
	testq %rax, %rax
	jne .fail

	movq $3, %rax
	movq %r12, %rdi
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
path:
	.asciz "/bin/execbig"
message:
	.ascii "linux mmapbig ok\n"
message_end:
