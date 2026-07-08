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

	leaq child_segv(%rip), %r13
	movl $0x8b, %r14d
	call fork_wait_fault

	leaq child_sigbus(%rip), %r13
	movl $0x87, %r14d
	call fork_wait_fault

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

fork_wait_fault:
	movq $57, %rax
	syscall
	testq %rax, %rax
	js fail
	jnz 1f
	jmp *%r13

1:
	movq %rax, %r12
	movq $61, %rax
	movq %r12, %rdi
	leaq wait_status(%rip), %rsi
	xorq %rdx, %rdx
	xorq %r10, %r10
	syscall
	cmpq %r12, %rax
	jne fail

	cmpl %r14d, wait_status(%rip)
	jne fail
	ret

child_segv:
	movq $1, %rax
	movq $1, %rdi
	leaq segv_message(%rip), %rsi
	movq $segv_message_end - segv_message, %rdx
	syscall
	cmpq $segv_message_end - segv_message, %rax
	jne fail

	movabsq $0x0000700000000000, %rbx
	movl $1, (%rbx)

	movq $231, %rax
	movq $99, %rdi
	syscall

child_sigbus:
	movq $1, %rax
	movq $1, %rdi
	leaq bus_message(%rip), %rsi
	movq $bus_message_end - bus_message, %rdx
	syscall
	cmpq $bus_message_end - bus_message, %rax
	jne fail

	pushfq
	orq $0x40000, (%rsp)
	popfq
	leaq unaligned_word(%rip), %rbx
	movl 1(%rbx), %eax

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
segv_message:
	.ascii "faulttest child sigsegv\n"
segv_message_end:
bus_message:
	.ascii "faulttest child sigbus\n"
bus_message_end:
ok_message:
	.ascii "faulttest sigsegv sigbus wait status ok\n"
ok_message_end:
fail_message:
	.ascii "faulttest failed\n"
fail_message_end:
unaligned_word:
	.long 0x12345678

.section .bss, "aw"
wait_status:
	.skip 4
