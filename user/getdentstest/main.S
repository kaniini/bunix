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

	movq $83, %rax
	leaq tmp_path(%rip), %rdi
	movq $0755, %rsi
	syscall
	testq %rax, %rax
	js fail

	movq $3, %rax
	movq %r12, %rdi
	syscall
	testq %rax, %rax
	jne fail

	movq $260, %r13
create_loop:
	movq $257, %rax
	movq $-100, %rdi
	leaq create_path(%rip), %rsi
	movq $66, %rdx
	movq $0644, %r10
	syscall
	testq %rax, %rax
	js fail
	movq %rax, %r12

	movq $3, %rax
	movq %r12, %rdi
	syscall
	testq %rax, %rax
	jne fail

	addb $1, create_digits + 3(%rip)
	cmpb $'9', create_digits + 3(%rip)
	jle create_next
	movb $'0', create_digits + 3(%rip)
	addb $1, create_digits + 2(%rip)
	cmpb $'9', create_digits + 2(%rip)
	jle create_next
	movb $'0', create_digits + 2(%rip)
	addb $1, create_digits + 1(%rip)
	cmpb $'9', create_digits + 1(%rip)
	jle create_next
	movb $'0', create_digits + 1(%rip)
	addb $1, create_digits(%rip)
create_next:
	decq %r13
	jnz create_loop

	movq $257, %rax
	movq $-100, %rdi
	leaq tmp_path(%rip), %rsi
	movq $65536, %rdx
	syscall
	testq %rax, %rax
	js fail
	movq %rax, %r12

	movq $217, %rax
	movq %r12, %rdi
	leaq large_getdents_buffer(%rip), %rsi
	movq $131072, %rdx
	syscall
	testq %rax, %rax
	jle close_fail

	movq $3, %rax
	movq %r12, %rdi
	syscall
	testq %rax, %rax
	jne fail

	movq $1, %rax
	movq $1, %rdi
	leaq large_message(%rip), %rsi
	movq $large_message_end - large_message, %rdx
	syscall
	cmpq $large_message_end - large_message, %rax
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
tmp_path:
	.asciz "/tmp/getdents-large"
ok_message:
	.ascii "linux getdents64 checks ok\n"
ok_message_end:
getrandom_message:
	.ascii "linux getrandom checks ok\n"
getrandom_message_end:
large_message:
	.ascii "linux getdents64 large checks ok\n"
large_message_end:
fail_message:
	.ascii "linux getdents64 checks failed\n"
fail_message_end:

.section .data, "aw"
create_path:
	.ascii "/tmp/getdents-large/gd-aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa-"
create_digits:
	.asciz "0000"

.section .bss, "aw"
getdents_buffer:
	.skip 8192
large_getdents_buffer:
	.skip 131072
getrandom_buffer:
	.skip 70000
