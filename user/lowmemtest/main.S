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

	leaq child_low_read(%rip), %r13
	call fork_wait_segv

	leaq child_low_write(%rip), %r13
	call fork_wait_segv

	movq $1, %rax
	movq $1, %rdi
	movabsq $0x0000000000100000, %rsi
	movq $1, %rdx
	syscall
	cmpq $-14, %rax
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

fork_wait_segv:
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

	cmpl $0x8b, wait_status(%rip)
	jne fail
	ret

child_low_read:
	movabsq $0x0000000000100000, %rbx
	movl (%rbx), %eax
	movq $231, %rax
	movq $99, %rdi
	syscall

child_low_write:
	movabsq $0x0000000000300000, %rbx
	movl $0x12345678, (%rbx)
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
	.ascii "lowmemtest begin\n"
begin_message_end:
ok_message:
	.ascii "lowmemtest low identity isolation ok\n"
ok_message_end:
fail_message:
	.ascii "lowmemtest failed\n"
fail_message_end:

.section .bss, "aw"
wait_status:
	.skip 4
