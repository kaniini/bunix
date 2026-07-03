.section .text, "ax"
.extern arch_current_syscall_stack
.global arch_syscall_entry
.type arch_syscall_entry, @function
arch_syscall_entry:
	movq %rsp, %r10
	movq arch_current_syscall_stack(%rip), %rsp

	pushq %r10
	pushq %rcx
	pushq %r11
	subq $8, %rsp

	movq %rdx, %rcx
	movq %rsi, %rdx
	movq %rdi, %rsi
	movq %rax, %rdi
	call arch_syscall_dispatch

	addq $8, %rsp
	popq %r11
	popq %rcx
	popq %r10
	movq %r10, %rsp
	sysretq
