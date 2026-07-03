.set AP_TRAMPOLINE_PHYS, 0x7000
.set AP_CR0_PE, 0x00000001
.set AP_CR0_PG, 0x80000000
.set AP_CR4_PAE, 0x00000020
.set AP_MSR_EFER, 0xc0000080
.set AP_EFER_LME, 0x00000100

.section .text, "ax"
.code16
.global arch_smp_ap_trampoline_start
.global arch_smp_ap_trampoline_end
.type arch_smp_ap_trampoline_start, @function
arch_smp_ap_trampoline_start:
	cli
	cld
	xorw %ax, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %ss
	movw $AP_TRAMPOLINE_PHYS, %sp

	data32 lgdt AP_TRAMPOLINE_PHYS + (ap_gdt_ptr - arch_smp_ap_trampoline_start)

	movl %cr0, %eax
	orl $AP_CR0_PE, %eax
	movl %eax, %cr0

	data32 ljmp $0x08, $AP_TRAMPOLINE_PHYS + (ap_protected - arch_smp_ap_trampoline_start)

.code32
ap_protected:
	movw $0x10, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %ss

	movl %cr4, %eax
	orl $AP_CR4_PAE, %eax
	movl %eax, %cr4

	movl $arch_boot_pml4, %eax
	movl %eax, %cr3

	movl $AP_MSR_EFER, %ecx
	rdmsr
	orl $AP_EFER_LME, %eax
	wrmsr

	movl %cr0, %eax
	orl $(AP_CR0_PE | AP_CR0_PG), %eax
	movl %eax, %cr0

	ljmpl $0x18, $AP_TRAMPOLINE_PHYS + (ap_long - arch_smp_ap_trampoline_start)

.code64
ap_long:
	movw $0x10, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %ss

	movabsq $arch_smp_ap_stack_top, %rax
	movq (%rax), %rsp
	xorq %rbp, %rbp

	movabsq $arch_smp_ap_boot_cpu, %rax
	movl (%rax), %edi
	movabsq $arch_smp_ap_entry, %rax
	call *%rax

ap_halt:
	cli
	hlt
	jmp ap_halt

.align 8
ap_gdt:
	.quad 0
	.quad 0x00cf9a000000ffff
	.quad 0x00cf92000000ffff
	.quad 0x00af9a000000ffff
ap_gdt_end:

ap_gdt_ptr:
	.short ap_gdt_end - ap_gdt - 1
	.long AP_TRAMPOLINE_PHYS + (ap_gdt - arch_smp_ap_trampoline_start)

.align 16
arch_smp_ap_trampoline_end:
