.section .text, "ax"

.macro ISR_NOERR vector
.global isr_stub_\vector
.type isr_stub_\vector, @function
isr_stub_\vector:
	pushq $0
	pushq $\vector
	jmp isr_common
.endm

.macro ISR_ERR vector
.global isr_stub_\vector
.type isr_stub_\vector, @function
isr_stub_\vector:
	pushq $\vector
	jmp isr_common
.endm

ISR_NOERR 0
ISR_NOERR 6
ISR_ERR 8
ISR_ERR 13
ISR_ERR 14
ISR_NOERR 32
ISR_NOERR 64
ISR_NOERR 65

.global isr_common
.type isr_common, @function
isr_common:
	pushq %rax
	pushq %rbx
	pushq %rcx
	pushq %rdx
	pushq %rsi
	pushq %rdi
	pushq %rbp
	pushq %r8
	pushq %r9
	pushq %r10
	pushq %r11
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15

	movq %rsp, %rdi
	addq $120, %rdi
	call arch_interrupt_dispatch

	popq %r15
	popq %r14
	popq %r13
	popq %r12
	popq %r11
	popq %r10
	popq %r9
	popq %r8
	popq %rbp
	popq %rdi
	popq %rsi
	popq %rdx
	popq %rcx
	popq %rbx
	popq %rax
	addq $16, %rsp
	iretq
