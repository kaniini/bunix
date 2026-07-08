.section .text, "ax"
.align 2
.global riscv64_trap_entry
.type riscv64_trap_entry, @function
riscv64_trap_entry:
	/*
	 * sscratch is zero while running in S-mode and holds the kernel stack
	 * top while running in U-mode.  Swap through t0 so we can tell which
	 * kind of trap this is without losing the interrupted t0 value.
	 */
	csrrw t0, sscratch, t0
	bnez t0, .Lfrom_user

.Lfrom_supervisor:
	csrrw t0, sscratch, t0
	addi sp, sp, -320

	sd ra, 40(sp)
	sd gp, 48(sp)
	sd tp, 56(sp)
	sd t0, 64(sp)
	sd t1, 72(sp)
	sd t2, 80(sp)
	sd s0, 88(sp)
	sd s1, 96(sp)
	sd a0, 104(sp)
	sd a1, 112(sp)
	sd a2, 120(sp)
	sd a3, 128(sp)
	sd a4, 136(sp)
	sd a5, 144(sp)
	sd a6, 152(sp)
	sd a7, 160(sp)
	sd s2, 168(sp)
	sd s3, 176(sp)
	sd s4, 184(sp)
	sd s5, 192(sp)
	sd s6, 200(sp)
	sd s7, 208(sp)
	sd s8, 216(sp)
	sd s9, 224(sp)
	sd s10, 232(sp)
	sd s11, 240(sp)
	sd t3, 248(sp)
	sd t4, 256(sp)
	sd t5, 264(sp)
	sd t6, 272(sp)

	csrr t0, scause
	sd t0, 0(sp)
	csrr t0, stval
	sd t0, 8(sp)
	csrr t0, sepc
	sd t0, 16(sp)
	csrr t0, sstatus
	sd t0, 24(sp)
	addi t0, sp, 320
	sd t0, 32(sp)
	j .Ldispatch

.Lfrom_user:
	addi t0, t0, -320
	sd sp, 32(t0)
	csrr sp, sscratch
	sd sp, 64(t0)
	csrw sscratch, zero
	mv sp, t0

	sd ra, 40(sp)
	sd gp, 48(sp)
	sd tp, 56(sp)
	sd t1, 72(sp)
	sd t2, 80(sp)
	sd s0, 88(sp)
	sd s1, 96(sp)
	sd a0, 104(sp)
	sd a1, 112(sp)
	sd a2, 120(sp)
	sd a3, 128(sp)
	sd a4, 136(sp)
	sd a5, 144(sp)
	sd a6, 152(sp)
	sd a7, 160(sp)
	sd s2, 168(sp)
	sd s3, 176(sp)
	sd s4, 184(sp)
	sd s5, 192(sp)
	sd s6, 200(sp)
	sd s7, 208(sp)
	sd s8, 216(sp)
	sd s9, 224(sp)
	sd s10, 232(sp)
	sd s11, 240(sp)
	sd t3, 248(sp)
	sd t4, 256(sp)
	sd t5, 264(sp)
	sd t6, 272(sp)

	csrr t0, scause
	sd t0, 0(sp)
	csrr t0, stval
	sd t0, 8(sp)
	csrr t0, sepc
	sd t0, 16(sp)
	csrr t0, sstatus
	sd t0, 24(sp)

.Ldispatch:
	mv a0, sp
	call arch_interrupt_dispatch

	ld t0, 16(sp)
	csrw sepc, t0
	ld t0, 24(sp)
	csrw sstatus, t0

	andi t0, t0, 0x100
	beqz t0, .Lreturn_user

.Lreturn_supervisor:
	ld ra, 40(sp)
	ld gp, 48(sp)
	ld tp, 56(sp)
	ld t0, 64(sp)
	ld t1, 72(sp)
	ld t2, 80(sp)
	ld s0, 88(sp)
	ld s1, 96(sp)
	ld a0, 104(sp)
	ld a1, 112(sp)
	ld a2, 120(sp)
	ld a3, 128(sp)
	ld a4, 136(sp)
	ld a5, 144(sp)
	ld a6, 152(sp)
	ld a7, 160(sp)
	ld s2, 168(sp)
	ld s3, 176(sp)
	ld s4, 184(sp)
	ld s5, 192(sp)
	ld s6, 200(sp)
	ld s7, 208(sp)
	ld s8, 216(sp)
	ld s9, 224(sp)
	ld s10, 232(sp)
	ld s11, 240(sp)
	ld t3, 248(sp)
	ld t4, 256(sp)
	ld t5, 264(sp)
	ld t6, 272(sp)

	addi sp, sp, 320
	sret

.Lreturn_user:
	addi t0, sp, 320
	csrw sscratch, t0

	ld ra, 40(sp)
	ld gp, 48(sp)
	ld tp, 56(sp)
	ld t0, 64(sp)
	ld t1, 72(sp)
	ld t2, 80(sp)
	ld s0, 88(sp)
	ld s1, 96(sp)
	ld a0, 104(sp)
	ld a1, 112(sp)
	ld a2, 120(sp)
	ld a3, 128(sp)
	ld a4, 136(sp)
	ld a5, 144(sp)
	ld a6, 152(sp)
	ld a7, 160(sp)
	ld s2, 168(sp)
	ld s3, 176(sp)
	ld s4, 184(sp)
	ld s5, 192(sp)
	ld s6, 200(sp)
	ld s7, 208(sp)
	ld s8, 216(sp)
	ld s9, 224(sp)
	ld s10, 232(sp)
	ld s11, 240(sp)
	ld t3, 248(sp)
	ld t4, 256(sp)
	ld t5, 264(sp)
	ld t6, 272(sp)
	ld sp, 32(sp)
	sret

.global riscv64_user_enter
.type riscv64_user_enter, @function
riscv64_user_enter:
	csrw sscratch, a2
	mv sp, a1
	csrw sepc, a0
	csrr t0, sstatus
	li t1, -259
	and t0, t0, t1
	ori t0, t0, 0x20
	csrw sstatus, t0
	sret

.global riscv64_user_mode_self_test
.type riscv64_user_mode_self_test, @function
riscv64_user_mode_self_test:
	la t0, riscv64_user_test_status
	li t1, 1
	sd t1, 0(t0)
	la t0, riscv64_user_test_saved_sp
	sd sp, 0(t0)
	la t0, riscv64_user_test_saved_ra
	sd ra, 0(t0)
	la t0, riscv64_user_test_return_pc
	la t1, .Luser_test_return
	sd t1, 0(t0)
	tail riscv64_user_enter

.Luser_test_return:
	la t0, riscv64_user_test_return_pc
	sd zero, 0(t0)
	la t0, riscv64_user_test_saved_sp
	ld sp, 0(t0)
	la t0, riscv64_user_test_saved_ra
	ld ra, 0(t0)
	la t0, riscv64_user_test_status
	ld a0, 0(t0)
	ret

.global riscv64_user_ecall_probe
.type riscv64_user_ecall_probe, @function
riscv64_user_ecall_probe:
	li a7, -999
	ecall
	li t0, -1
	bne a0, t0, .Luser_probe_fail
	li a0, 0
	li a7, -998
	ecall
.Luser_probe_fail:
	li a0, 1
	li a7, -998
	ecall
.Luser_probe_halt:
	wfi
	j .Luser_probe_halt

.section .bss, "aw", @nobits
.align 3
.global riscv64_user_test_return_pc
riscv64_user_test_return_pc:
	.quad 0
riscv64_user_test_saved_sp:
	.quad 0
riscv64_user_test_saved_ra:
	.quad 0

.section .data, "aw"
.align 3
.global riscv64_user_test_status
riscv64_user_test_status:
	.quad 1
