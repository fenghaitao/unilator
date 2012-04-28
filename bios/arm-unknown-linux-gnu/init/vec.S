#include "bios/linkage.h"
	.text

	.globl	vec_init
vec_init:
	mov	r1, #0
	ldr	r2, vec_br
1:	str	r2, [r1], #4
	cmp	r1, #0x20
	blt	1b

	mrs	r0, cpsr
	bic	r1, r0, #0x1f
	orr	r1, r1, #0xd2
	msr	cpsr, r1
	mov	r0, r0
	mov	sp, #0x1000
	msr	cpsr, r0
	mov	r0, r0

	adr	r0, vec_irq
	mov	r1, #0
	str	r0, [r1, #0x38]
	mov	pc, lr

vec_br:	ldr	pc, vec_br + 0x20

vec_irq:
//	stmfd	sp!, {r0 - r3, ip, lr}
	bl	SYMBOL_NAME(time_interrupt)
//	ldmfd	sp!, {r0 - r3, ip, lr}
	subs	pc, lr, #4
