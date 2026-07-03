.section .text, "ax"
.extern arch_current_syscall_stack
.global arch_syscall_entry
.type arch_syscall_entry, @function
arch_syscall_entry:
	movq %r10, %r9
	movq %rsp, %r10
	movq %rax, %r8
	movl %gs:0, %eax
	leaq arch_current_syscall_stack(%rip), %rsp
	movq (%rsp,%rax,8), %rsp

	pushq %r10
	pushq %rcx
	pushq %r11
	subq $8, %rsp

	movq %rdi, %rax
	movq %rsi, %r11
	movq %rdx, %rcx
	movq %r8, %rdi
	movq %rax, %rsi
	movq %r11, %rdx
	movq %r9, %r8
	call arch_syscall_dispatch

	addq $8, %rsp
	popq %r11
	popq %rcx
	popq %r10
	movq %r10, %rsp
	sysretq
