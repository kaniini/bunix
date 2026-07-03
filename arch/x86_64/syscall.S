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

	subq $112, %rsp

	movq %r8, 0(%rsp)
	movq %rdi, 8(%rsp)
	movq %rsi, 16(%rsp)
	movq %rdx, 24(%rsp)
	movq %r9, 32(%rsp)
	movq %rcx, 40(%rsp)
	movq %r11, 48(%rsp)
	movq %r10, 56(%rsp)
	movq %rbx, 64(%rsp)
	movq %rbp, 72(%rsp)
	movq %r12, 80(%rsp)
	movq %r13, 88(%rsp)
	movq %r14, 96(%rsp)
	movq %r15, 104(%rsp)

	movq %rsp, %rdi
	call arch_syscall_dispatch

	movq 40(%rsp), %rcx
	movq 48(%rsp), %r11
	movq 56(%rsp), %r10
	addq $112, %rsp
	movq %r10, %rsp
	sysretq

.global arch_user_resume
.type arch_user_resume, @function
arch_user_resume:
	movq %rdi, %rax
	movq 64(%rax), %rbx
	movq 72(%rax), %rbp
	movq 80(%rax), %r12
	movq 88(%rax), %r13
	movq 96(%rax), %r14
	movq 104(%rax), %r15
	movq 40(%rax), %rcx
	movq 48(%rax), %r11
	movq 56(%rax), %rsp
	movq 16(%rax), %rsi
	movq 24(%rax), %rdx
	movq 32(%rax), %r10
	movq 8(%rax), %rdi
	xorq %rax, %rax
	sysretq
