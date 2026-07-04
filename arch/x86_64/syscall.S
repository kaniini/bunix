.section .text, "ax"
.global arch_syscall_entry
.type arch_syscall_entry, @function
arch_syscall_entry:
	movq %rsp, %r11
	movq %gs:8, %rsp

	subq $4288, %rsp
	andq $-64, %rsp

	movq %r8, 0(%rsp)
	movq %rdi, 8(%rsp)
	movq %rsi, 16(%rsp)
	movq %rdx, 24(%rsp)
	movq %r10, 32(%rsp)
	movq %rcx, 40(%rsp)
	movq $0x202, 48(%rsp)
	movq %r11, 56(%rsp)
	movq %rbx, 64(%rsp)
	movq %rbp, 72(%rsp)
	movq %r12, 80(%rsp)
	movq %r13, 88(%rsp)
	movq %r14, 96(%rsp)
	movq %r15, 104(%rsp)
	movq %r8, 112(%rsp)
	movq %r9, 120(%rsp)

	movq %rax, 0(%rsp)

	cmpl $0, arch_thread_use_xsave(%rip)
	je 1f
	movl arch_thread_xstate_mask_low(%rip), %eax
	movl arch_thread_xstate_mask_high(%rip), %edx
	xsave64 128(%rsp)
	jmp 2f
1:
	fxsave64 128(%rsp)
2:

	movq %rsp, %rdi
	call arch_syscall_dispatch

	movq %rax, 0(%rsp)
	cmpl $0, arch_thread_use_xsave(%rip)
	je 3f
	movl arch_thread_xstate_mask_low(%rip), %eax
	movl arch_thread_xstate_mask_high(%rip), %edx
	xrstor64 128(%rsp)
	jmp 4f
3:
	fxrstor64 128(%rsp)
4:
	movq 0(%rsp), %rax

	movq 8(%rsp), %rdi
	movq 16(%rsp), %rsi
	movq 24(%rsp), %rdx
	movq 32(%rsp), %r10
	movq 64(%rsp), %rbx
	movq 72(%rsp), %rbp
	movq 80(%rsp), %r12
	movq 88(%rsp), %r13
	movq 96(%rsp), %r14
	movq 104(%rsp), %r15
	movq 112(%rsp), %r8
	movq 120(%rsp), %r9
	movq 40(%rsp), %rcx
	movq 48(%rsp), %r11
	movq 56(%rsp), %rsp
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
	movq 112(%rax), %r8
	movq 120(%rax), %r9
	xorq %rax, %rax
	sysretq
