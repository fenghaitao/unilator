	.globl	ramtest

ramtest:
	stmfd	sp!, {r4, lr}
	ldmia	r0, {r4, lr}

	mvn	r2, r1

	stmia	r0, {r1, r2}
	ldmia	r0, {r3, ip}

	teq	r1, r3
	teqeq	r2, ip

	mvn	r1, r1
	mvn	r2, r2

	stmia	r0, {r1, r2}
	ldmia	r0, {r3, ip}

	stmia	r0, {r4, lr}

	teqeq	r1, r3

	moveq	r0, ip
	movne	r0, r1
	ldmfd	sp!, {r4, pc}
