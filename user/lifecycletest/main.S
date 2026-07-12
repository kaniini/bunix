.section .text, "ax"
.global _start
.type _start, @function
_start:
	movq $58, %rax
	syscall
	testq %rax, %rax
	js fail
	jz child

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

child:
	movq $-2, %rax
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
ok_message:
	.ascii "linux lifecycle native-exit vfork ok\n"
ok_message_end:
fail_message:
	.ascii "linux lifecycle native-exit vfork failed\n"
fail_message_end:
