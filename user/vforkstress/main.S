.section .text, "ax"
.global _start
.type _start, @function
_start:
	xorq %r12, %r12

fork_loop:
	cmpq $40, %r12
	je wait_loop

	movq $57, %rax
	syscall
	testq %rax, %rax
	js fork_fail
	jz worker_child

	incq %r12
	jmp fork_loop

worker_child:
	movq $58, %rax
	syscall
	testq %rax, %rax
	js child_fail
	jz vfork_child

	movq $231, %rax
	xorq %rdi, %rdi
	syscall

vfork_child:
	movq $35, %rax
	leaq sleep_time(%rip), %rdi
	xorq %rsi, %rsi
	syscall

	movq $231, %rax
	xorq %rdi, %rdi
	syscall

child_fail:
	movq $1, %rax
	movq $2, %rdi
	leaq child_fail_message(%rip), %rsi
	movq $child_fail_message_end - child_fail_message, %rdx
	syscall

	movq $231, %rax
	movq $2, %rdi
	syscall

wait_loop:
	xorq %r13, %r13

wait_one:
	cmpq $40, %r13
	je ok

	movq $61, %rax
	movq $-1, %rdi
	leaq wait_status(%rip), %rsi
	xorq %rdx, %rdx
	xorq %r10, %r10
	syscall
	testq %rax, %rax
	jle wait_fail
	cmpl $0, wait_status(%rip)
	jne status_fail

	incq %r13
	jmp wait_one

ok:
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

fork_fail:
	movq $1, %rax
	movq $2, %rdi
	leaq fork_fail_message(%rip), %rsi
	movq $fork_fail_message_end - fork_fail_message, %rdx
	syscall
	jmp fail

wait_fail:
	movq $1, %rax
	movq $2, %rdi
	leaq wait_fail_message(%rip), %rsi
	movq $wait_fail_message_end - wait_fail_message, %rdx
	syscall
	jmp fail

status_fail:
	movq $1, %rax
	movq $2, %rdi
	leaq status_fail_message(%rip), %rsi
	movq $status_fail_message_end - status_fail_message, %rdx
	syscall
	jmp fail

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
ok_message:
	.ascii "linux vforkstress ok\n"
ok_message_end:
fail_message:
	.ascii "linux vforkstress failed\n"
fail_message_end:
fork_fail_message:
	.ascii "linux vforkstress fork failed\n"
fork_fail_message_end:
child_fail_message:
	.ascii "linux vforkstress child vfork failed\n"
child_fail_message_end:
wait_fail_message:
	.ascii "linux vforkstress wait failed\n"
wait_fail_message_end:
status_fail_message:
	.ascii "linux vforkstress child status failed\n"
status_fail_message_end:
sleep_time:
	.quad 0
	.quad 200000000

.section .bss, "aw"
wait_status:
	.skip 4
