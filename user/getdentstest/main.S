.section .text, "ax"
.global _start
.type _start, @function
_start:
	movq $257, %rax
	movq $-100, %rdi
	leaq root_path(%rip), %rsi
	movq $65536, %rdx
	syscall
	testq %rax, %rax
	js fail
	movq %rax, %r12

	movq $217, %rax
	movq %r12, %rdi
	leaq getdents_buffer(%rip), %rsi
	movq $8192, %rdx
	syscall
	testq %rax, %rax
	jle close_fail
	cmpq $4096, %rax
	jg close_fail

	movq $318, %rax
	leaq getrandom_buffer(%rip), %rdi
	movq $70000, %rsi
	xorq %rdx, %rdx
	syscall
	cmpq $70000, %rax
	jne close_fail

	movq $1, %rax
	movq $1, %rdi
	leaq getrandom_message(%rip), %rsi
	movq $getrandom_message_end - getrandom_message, %rdx
	syscall
	cmpq $getrandom_message_end - getrandom_message, %rax
	jne close_fail

	movq $3, %rax
	movq %r12, %rdi
	syscall
	testq %rax, %rax
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

close_fail:
	movq $3, %rax
	movq %r12, %rdi
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
root_path:
	.asciz "/"
ok_message:
	.ascii "linux getdents64 checks ok\n"
ok_message_end:
getrandom_message:
	.ascii "linux getrandom checks ok\n"
getrandom_message_end:
fail_message:
	.ascii "linux getdents64 checks failed\n"
fail_message_end:

.section .bss, "aw"
getdents_buffer:
	.skip 8192
getrandom_buffer:
	.skip 70000
