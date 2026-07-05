.section .text, "ax"
.global _start
.type _start, @function
_start:
	movq $2, %rax
	leaq path(%rip), %rdi
	xorq %rsi, %rsi
	xorq %rdx, %rdx
	syscall
	testq %rax, %rax
	js .fail
	movq %rax, %r12

	xorq %rax, %rax
	movq %r12, %rdi
	leaq buffer(%rip), %rsi
	movq $131072, %rdx
	syscall
	cmpq $131072, %rax
	jne .fail

	cmpb $0, buffer(%rip)
	jne .fail
	cmpb $0, buffer + 65536(%rip)
	jne .fail
	cmpb $0, buffer + 131071(%rip)
	jne .fail

	movq $267, %rax
	movq $-100, %rdi
	leaq cat_path(%rip), %rsi
	leaq readlink_buffer(%rip), %rdx
	movq $65537, %r10
	syscall
	cmpq $12, %rax
	jne .fail
	movq $0x7375622f6e69622f, %rax
	cmpq %rax, readlink_buffer(%rip)
	jne .fail
	cmpl $0x786f6279, readlink_buffer + 8(%rip)
	jne .fail

	movq $1, %rax
	movq $1, %rdi
	leaq message(%rip), %rsi
	movq $message_end - message, %rdx
	syscall
	movq $1, %rax
	movq $1, %rdi
	leaq readlink_message(%rip), %rsi
	movq $readlink_message_end - readlink_message, %rdx
	syscall

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
	.asciz "/dev/zero"
cat_path:
	.asciz "/bin/cat"
message:
	.ascii "linux readbig ok\n"
message_end:
readlink_message:
	.ascii "linux readlinkbig ok\n"
readlink_message_end:

.section .bss, "aw"
.balign 16
buffer:
	.skip 131072
readlink_buffer:
	.skip 32
