.section .text, "ax"
.global _start
.type _start, @function
_start:
	movq $1, %rax
	movq $1, %rdi
	leaq message(%rip), %rsi
	movq $message_end - message, %rdx
	syscall
	cmpq $message_end - message, %rax
	jne fail

	movq $1, %rax
	movq $99, %rdi
	leaq badfd_message(%rip), %rsi
	movq $badfd_message_end - badfd_message, %rdx
	syscall
	cmpq $-9, %rax
	jne fail

	movq $1, %rax
	movq $1, %rdi
	leaq ok_message(%rip), %rsi
	movq $ok_message_end - ok_message, %rdx
	syscall
	cmpq $ok_message_end - ok_message, %rax
	jne fail

	movq $257, %rax
	movq $-100, %rdi
	leaq hello_path(%rip), %rsi
	xorq %rdx, %rdx
	syscall
	cmpq $3, %rax
	jl fail
	movq %rax, %r12

	xorq %rax, %rax
	movq %r12, %rdi
	leaq read_buffer(%rip), %rsi
	movq $15, %rdx
	syscall
	cmpq $15, %rax
	jne fail

	movq $1, %rax
	movq $1, %rdi
	leaq read_buffer(%rip), %rsi
	movq $15, %rdx
	syscall
	cmpq $15, %rax
	jne fail

	movq $3, %rax
	movq %r12, %rdi
	syscall
	testq %rax, %rax
	jne fail

	xorq %rax, %rax
	movq %r12, %rdi
	leaq read_buffer(%rip), %rsi
	movq $1, %rdx
	syscall
	cmpq $-9, %rax
	jne fail

	movq $231, %rax
	xorq %rdi, %rdi
	syscall

fail:
	movq $1, %rax
	movq $2, %rdi
	leaq fail_message(%rip), %rsi
	movq $fail_message_end - fail_message, %rdx
	syscall

	movq $231, %rax
	movq $1, %rdi
	syscall

.halt:
	jmp .halt

.section .rodata, "a"
message:
	.ascii "linux syscall shared buffer\n"
message_end:
badfd_message:
	.ascii "linux bad fd should not print\n"
badfd_message_end:
ok_message:
	.ascii "linux return checks ok\n"
ok_message_end:
fail_message:
	.ascii "linux return checks failed\n"
fail_message_end:
hello_path:
	.asciz "/hello.txt"

.section .bss, "aw"
read_buffer:
	.skip 32
