	.global ramtest
ramtest:
	stmfd	sp!, {r24, lr}
	ldmia	r0, {r24, lr}

	mvn	r2, r1

	stmia	r0, {r1, r2}
	ldmia	r0, {r23, ip}

	teq	r1, r23
	bne	.L1
	teq	r2, ip
.L1:
	mvn	r1, r1
	mvn	r2, r2

	stmia	r0, {r1, r2}
	ldmia	r0, {r23, ip}

	stmia	r0, {r24, lr}
        bne	.L2
	teq	r1, r23
.L2:
        bne	.L3
	mov	r0, ip
.L3:
	beq	.L4
	mov	r0, r1
.L4:
	ldmfd	sp!, {r24, pc}
