	.file	"a_new.c"
	.option nopic
	.attribute arch, "rv64i2p1_m2p0_f2p2_d2p2_zicsr2p0_zmmul1p0"
	.attribute unaligned_access, 0
	.attribute stack_align, 16
	.text
	.section	.rodata
	.align	3
.LC1:
	.string	"%d\n"
	.align	3
.LC0:
	.word	2
	.word	6
	.word	3
	.word	1
	.word	2
	.word	4
	.word	6
	.word	78
	.word	123
	.word	-51
	.text
	.align	2
	.globl	main
	.type	main, @function
main:
.LFB0:
	.cfi_startproc
	addi	sp,sp,-80
	.cfi_def_cfa_offset 80
	sd	ra,72(sp)
	sd	s0,64(sp)
	.cfi_offset 1, -8
	.cfi_offset 8, -16
	addi	s0,sp,80
	.cfi_def_cfa 8, 0
	lui	a5,%hi(.LC0)
	addi	a5,a5,%lo(.LC0)
	ld	a1,0(a5)
	ld	a2,8(a5)
	ld	a3,16(a5)
	ld	a4,24(a5)
	sd	a1,-72(s0)
	sd	a2,-64(s0)
	sd	a3,-56(s0)
	sd	a4,-48(s0)
	ld	a5,32(a5)
	sd	a5,-40(s0)
	sw	zero,-20(s0)
	j	.L2
.L5:
	lw	a4,-24(s0)
	addi	a5,s0,-72
	slli	a4,a4,2
	add	a5,a4,a5
	lw	a4,0(a5)
	lw	a5,-24(s0)
	addiw	a5,a5,1
	sext.w	a3,a5
	addi	a5,s0,-72
	slli	a3,a3,2
	add	a5,a3,a5
	lw	a5,0(a5)
	bge	a4,a5,.L4
	lw	a4,-24(s0)
	addi	a5,s0,-72
	slli	a4,a4,2
	add	a5,a4,a5
	lw	a5,0(a5)
	sw	a5,-32(s0)
	lw	a5,-24(s0)
	addiw	a5,a5,1
	sext.w	a4,a5
	addi	a5,s0,-72
	slli	a4,a4,2
	add	a5,a4,a5
	lw	a4,0(a5)
	lw	a3,-24(s0)
	addi	a5,s0,-72
	slli	a3,a3,2
	add	a5,a3,a5
	sw	a4,0(a5)
	lw	a5,-24(s0)
	addiw	a5,a5,1
	sext.w	a4,a5
	addi	a5,s0,-72
	slli	a4,a4,2
	add	a5,a4,a5
	lw	a4,-32(s0)
	sw	a4,0(a5)
.L4:
	lw	a5,-24(s0)
	addiw	a5,a5,1
	sw	a5,-24(s0)
.L3:
	lw	a5,-20(s0)
	addiw	a5,a5,-1
	sext.w	a5,a5
	lw	a4,-24(s0)
	sext.w	a4,a4
	blt	a4,a5,.L5
	lw	a5,-20(s0)
	addiw	a5,a5,1
	sw	a5,-20(s0)
.L2:
	lw	a5,-20(s0)
	sext.w	a4,a5
	li	a5,9
	ble	a4,a5,.L3
	sw	zero,-28(s0)
	j	.L7
.L8:
	lw	a4,-28(s0)
	addi	a5,s0,-72
	slli	a4,a4,2
	add	a5,a4,a5
	lw	a5,0(a5)
	mv	a1,a5
	lui	a5,%hi(.LC1)
	addi	a0,a5,%lo(.LC1)
	call	printf
	lw	a5,-28(s0)
	addiw	a5,a5,1
	sw	a5,-28(s0)
.L7:
	lw	a5,-28(s0)
	sext.w	a4,a5
	li	a5,9
	ble	a4,a5,.L8
	li	a5,0
	mv	a0,a5
	ld	ra,72(sp)
	.cfi_restore 1
	ld	s0,64(sp)
	.cfi_restore 8
	.cfi_def_cfa 2, 80
	addi	sp,sp,80
	.cfi_def_cfa_offset 0
	jr	ra
	.cfi_endproc
.LFE0:
	.size	main, .-main
	.ident	"GCC: () 15.1.0"
	.section	.note.GNU-stack,"",@progbits
