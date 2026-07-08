.section .text, "ax"
.global _start
.type _start, @function
_start:
	movq $1, %rax
	movq $1, %rdi
	leaq begin_message(%rip), %rsi
	movq $begin_message_end - begin_message, %rdx
	syscall
	cmpq $begin_message_end - begin_message, %rax
	jne fail

	movq $57, %rax
	syscall
	testq %rax, %rax
	js fail
	jz child_fault

	movq %rax, %r12
	movq $61, %rax
	movq %r12, %rdi
	leaq wait_status(%rip), %rsi
	xorq %rdx, %rdx
	xorq %r10, %r10
	syscall
	cmpq %r12, %rax
	jne fail

	cmpl $0x8b, wait_status(%rip)
	jne fail

	movq $1, %rax
	movq $1, %rdi
	leaq ok_message(%rip), %rsi
	movq $ok_message_end - ok_message, %rdx
	syscall
	cmpq $ok_message_end - ok_message, %rax
	jne fail

	movq $231, %rax
	xorq %rdi, %rdi
	syscall

child_fault:
	movq $1, %rax
	movq $1, %rdi
	leaq child_message(%rip), %rsi
	movq $child_message_end - child_message, %rdx
	syscall
	cmpq $child_message_end - child_message, %rax
	jne fail

	movabsq $0x0000700000000000, %rbx
	movl $1, (%rbx)

	movq $231, %rax
	movq $99, %rdi
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
begin_message:
	.ascii "faulttest begin\n"
begin_message_end:
child_message:
	.ascii "faulttest child fault\n"
child_message_end:
ok_message:
	.ascii "faulttest sigsegv wait status ok\n"
ok_message_end:
fail_message:
	.ascii "faulttest failed\n"
fail_message_end:

.section .bss, "aw"
wait_status:
	.skip 4
