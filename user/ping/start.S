.section .text, "ax"
.global _start
.type _start, @function
_start:
	movq $-1, %rax
	leaq one(%rip), %rdi
	movq $one_len, %rsi
	syscall

	movq $-3, %rax
	movq $0x2a, %rdi
	syscall

	movq $-4, %rax
	syscall
	movq %rax, %rbx

.wait_tick:
	movq $-4, %rax
	syscall
	cmpq %rbx, %rax
	je .wait_tick

	movq $-1, %rax
	leaq two(%rip), %rdi
	movq $two_len, %rsi
	syscall

	movq $-2, %rax
	xorq %rdi, %rdi
	syscall

.halt:
	jmp .halt

.section .rodata, "a"
one:
	.ascii "ping: one\n"
one_len = . - one

two:
	.ascii "ping: two\n"
two_len = . - two
