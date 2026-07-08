.section .text, "ax"
.align 2
.global riscv64_trap_entry
.type riscv64_trap_entry, @function
riscv64_trap_entry:
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

	mv a0, sp
	call arch_interrupt_dispatch

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
