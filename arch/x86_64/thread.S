.section .text, "ax"
.global arch_thread_switch
.type arch_thread_switch, @function
arch_thread_switch:
	cmpl $0, arch_thread_use_xsave(%rip)
	je 1f
	movl arch_thread_xstate_mask_low(%rip), %eax
	movl arch_thread_xstate_mask_high(%rip), %edx
	xsave64 64(%rdi)
	jmp 2f
1:
	fxsave64 64(%rdi)
2:
	pushq %rbp
	pushq %rbx
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15

	movq %rsp, (%rdi)
	movq (%rsi), %rsp

	popq %r15
	popq %r14
	popq %r13
	popq %r12
	popq %rbx
	popq %rbp
	cmpl $0, arch_thread_use_xsave(%rip)
	je 3f
	movl arch_thread_xstate_mask_low(%rip), %eax
	movl arch_thread_xstate_mask_high(%rip), %edx
	xrstor64 64(%rsi)
	jmp 4f
3:
	fxrstor64 64(%rsi)
4:
	ret
