	.text
	.globl	memcpy
memcpy:
1:	subs	r2, r2, #1
	bmi	.L1
	ldrb	r3, [r1], #1
	strb	r3, [r0], #1
	bpl	1b
.L1:
	mov	pc, lr

	.globl	memzero
memzero:
	mov	r2, r1
	mov	r1, #0

	.globl	memset
memset:
1:	subs	r2, r2, #1
	bmi	.L2
	strb	r1, [r0], #1
	bpl	1b
.L2:
	mov	pc, lr

	.globl	memeq
memeq:	teq	r2, #0
//	moveq	r0, #1
//	moveq	pc, lr
1:	ldrb	r3, [r0], #1
	ldrb	ip, [r1], #1
	teq	r3, ip
//	movne	r0, #0
//	movne	pc, lr
	subs	r2, r2, #1
	bne	1b
	mov	r0, #1
	mov	pc, lr

	.globl strcpy
strcpy:
1:      ldrb    r3, [r1], #1
        strb    r3, [r0], #1
        teq     r3, #0
        bne     1b
        mov     pc, lr

	.globl	strnlen
strnlen:
	add	r3, r0, #1
1:	ldrb	r2, [r0], #1
	teq	r2, #0
//	subnes	r1, r1, #1
	bne	1b
	sub	r0, r0, r3
	mov	pc, lr
