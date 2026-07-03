.section .bss, "aw", @nobits
.align 16
syscall_stack_bottom:
	.skip 16384
syscall_stack_top:

.section .data, "aw"
.align 8
syscall_user_rsp:
	.quad 0

.section .text, "ax"
.global arch_syscall_entry
.type arch_syscall_entry, @function
arch_syscall_entry:
	movq %rsp, syscall_user_rsp(%rip)
	movq $syscall_stack_top, %rsp

	pushq %rcx
	pushq %r11

	movq %rdx, %rcx
	movq %rsi, %rdx
	movq %rdi, %rsi
	movq %rax, %rdi
	call arch_syscall_dispatch

	popq %r11
	popq %rcx
	movq syscall_user_rsp(%rip), %rsp
	sysretq
