#include "bios/linkage.h"
#define CSR_BASE	0x42000000
#define SDRAM_TIMING	0x10c
#define SDRAM_ADDR_SZ0	0x110
#define SDRAM_ADDR_SZ1	0x114
#define SDRAM_ADDR_SZ2	0x118
#define SDRAM_ADDR_SZ3	0x11c

#define SDRAM_BASE	0x40000000
#define SDRAM_NRARRAYS	4
#define SDRAM_ARRAYOFF	0x00004000

	.text
	.global _entry
_entry:
#if 0	
	bic	r11, lr, #3
	ldr	sp, [r11, #8]
	mrs	r0, cpsr
	bic	r0, r0, #0x1f
	orr	r0, r0, #0xd3
	msr	cpsr, r0

	mov	r0, #0
/*	mcr	p15, 0, r0, c7, c7, 0	@ flush all caches */

/*	mrc	p15, 0, r0, c1, c0	@ Control reg */
	bic	r0, r0, #0x0d		@ D-cache, wback & MMU off
	orr	r0, r0, #1 << 12	@ I-cache on
/*	mcr	p15, 0, r0, c1, c0	@ Control reg */

	bl	ser_init
/*
 * Initialise SDRAM.  This copes with the SDRAM in any state -
 * it does a complete initialisation and allocation of all
 * banks.
 *
 * Turn off SDRAM refresh
 */
	mov	r0, #0
	mov	ip, #CSR_BASE
	str	r0, [ip, #SDRAM_TIMING]
/* 
 * Wait for any refresh cycles to complete
 */
	mov	r0, #64
1:	subs	r0, r0, #1
	bgt	1b
/*
 * All-banks precharge SDRAM arrays
 */
	mov	r0, #SDRAM_BASE
	mov	r1, #SDRAM_NRARRAYS
1:	ldr	r2, [r0]
	ldr	r2, [r0]
	add	r0, r0, #SDRAM_ARRAYOFF
	subs	r1, r1, #1
	bgt	1b
/*
 * Write mode registers
 */
	mov	r0, #SDRAM_BASE
	orr	r0, r0, #0x88		@ Tcas
	mov	r1, #SDRAM_NRARRAYS
1:	str	r0, [r0]
	add	r0, r0, #SDRAM_ARRAYOFF
	subs	r1, r1, #1
	bgt	1b
/*
 * Turn on minimum SDRAM refresh
 */
	mov	r0, #0x00010000
	orr	r0, r0, #0x00000b00
	orr	r0, r0, #0x000000a5
	str	r0, [ip, #SDRAM_TIMING]
/*
 * Set size to maximum
 */
	mov	r1, #SDRAM_NRARRAYS
	add	r2, ip, #SDRAM_ADDR_SZ0
	mov	r0, #0x4f
1:	str	r0, [r2], #4
	add	r0, r0, #64*1024*1024
	subs	r1, r1, #1
	bgt	1b
/*
 * Wait for banks to be refreshed
 */
	mov	r0, #8
	mov	r1, #32
1:	subs	r1, r1, #2
	bgt	1b
	subs	r0, r0, #1
	bgt	1b
/*
 * Set SDRAM refresh to normal
 */
	mov	r0, #0x001a0000
	orr	r0, r0, #0x00000b00
	orr	r0, r0, #0x000000a5
	str	r0, [ip, #SDRAM_TIMING]

	mov	r4, #0x00000000
1:	mov	r5, #0
/*
 * Detect RAM multiplexer settings
 */
	mov	r0, r4			@ check for presence
	add	r1, r4, #64
	bl	testram
	bne	4f

	mov	r0, r4			@ check bit 18 (row 9)
	orr	r1, r0, #1 << 18
	bl	testram
//	orreq	r5, r5, #4

	mov	r0, r4			@ check bit 20 (BA1)
	orr	r1, r0, #1 << 20
	bl	testram
//	orreq	r5, r5, #2

	mov	r0, r4			@ check bit 22 (row 11)
	orr	r1, r0, #1 << 22
	bl	testram
//	orreq	r5, r5, #1

	adr	r1, ram_modes		@ convert test -> mux
	ldrb	r6, [r1, r5]
	orr	r6, r6, r4

	add	r0, ip, #SDRAM_ADDR_SZ0	@ set mux correctly
	orr	r5, r6, #7		@ leave size at 64MB
	str	r5, [r0, r4, lsr #24]
/*
 * Detect RAM array size
 */
	mov	r5, #1 << 20
	mov	r7, #0

2:	add	r0, r4, r5
	add	r1, r4, r5, lsr #1
	bl	testram
	bne	3f
	mov	r5, r5, lsl #1
	cmp	r7, #7
//	addne	r7, r7, #1
	bne	2b

3:	orr	r5, r6, r7
	add	r0, ip, #SDRAM_ADDR_SZ0
	str	r5, [r0, r4, lsr #24]	@ set array size

4:	add	r4, r4, #0x04000000
	teq	r4, #0x10000000
	bne	1b
/*
 * Allocate RAM addresses
 */
	mov	r3, #7			@ start at largest size
	mov	r4, #0x00000000
1:	mov	r5, #SDRAM_NRARRAYS
	add	r2, ip, #SDRAM_ADDR_SZ0
2:	ldr	r0, [r2], #4		@ Read SDRAM size & addr
	and	r1, r0, #7
	teq	r1, r3
	bne	3f

	and	r0, r0, #127		@ Preserve size etc
	orr	r0, r0, r4		@ Add base address
	str	r0, [r2, #-4]		@ Write SDRAM size & addr
	mov	r0, #524288
	add	r4, r4, r0, lsl r1	@ Next address

3:	subs	r5, r5, #1
	bgt	2b
	subs	r3, r3, #1
	bgt	1b

	mov	r0, #0
//	mcr	p15, 0, r0, c15, c1, 2

	mov	r1, #0x1000
	ldr	r2, =_etext
1:	ldr	r0, [r2], #4
	str	r0, [r1], #4
	subs	sp, sp, #4
	bgt	1b

	mov	r0, #0
	ldr	r1, =SYMBOL_NAME(_bss_start)
	ldr	r2, =SYMBOL_NAME(_end)
1:	str	r0, [r1], #4
	cmp	r1, r2
	blt	1b
	sub	sp, r4, #1048576
	bl	vec_init

	ldr	r0, =SYMBOL_NAME(ram_size)
	str	r4, [r0]
#endif
	bl	SYMBOL_NAME(start_main)
l:	b	l

ram_modes:
	.byte	0x08
	.byte	0x08
	.byte	0x00
	.byte	0x00
	.byte	0x10
	.byte	0x20
	.byte	0x30
	.byte	0x40

testram:
	mov	r10, #0x55
	orr	r10, r10, #0xaa00
	orr	r10, r10, r10, lsl #16
	str	r10, [r0]
	mvn	r10, r10
	str	r10, [r1]
	mvn	r10, r10
	ldr	r11, [r0]
	teq	r10, r11
	bne	1f
	mvn	r10, r10
	str	r10, [r0]
	mvn	r10, r10
	str	r10, [r1]
	mvn	r10, r10
	ldr	r11, [r0]
	teq	r10, r11
1:	mov	pc, lr

ser_init:
	mov	r0, #0x42000000
	orr	r0, r0, #0x160
	mov	r1, #0
	str	r1, [r0, #20]
	mov	r2, #16
1:	str	r1, [r0, #0]
	subs	r2, r2, #1
	bne	1b
	mov	r1, #1
	str	r1, [r0, #20]
	mov	r1, #19
	str	r1, [r0, #16]
	mov	r1, #0
	str	r1, [r0, #12]
	mov	r1, #3 << 5
	str	r1, [r0, #8]
	mov	r2, #16
1:	ldr	r1, [r0, #4]
	subs	r2, r2, #1
	bne	1b
	mov	pc, lr

ser_prints:
	mov	r4, #0x42000000
	orr	r4, r4, #0x160
1:	ldrb	r5, [r0], #1
	teq	r5, #0
//	moveq	pc, lr
2:	str	r5, [r4]
3:	ldr	r6, [r4, #24]
	tst	r6, #1 << 3
	bne	3b
	teq	r5, #'\n'
//	moveq	r5, #'\r'
	beq	2b
	b	1b

ser_printc:
	mov	r4, #0x42000000
	orr	r4, r4, #0x160
1:	str	r0, [r4]
2:	ldr	r6, [r4, #24]
	tst	r6, #1 << 3
	bne	2b
	teq	r0, #'\n'
//	moveq	r0, #'\r'
	beq	1b
	mov	pc, lr

ser_printhex:
	mov	r4, #0x42000000
	orr	r4, r4, #0x160
	mov	r6, #8
1:	and	r5, r0, #0xf0000000
	mov	r5, r5, lsr #28
	cmp	r5, #10
//	adccs	r5, r5, #6
	add	r5, r5, #'0'
	str	r5, [r4]
2:	ldr	r5, [r4, #24]
	tst	r5, #1 << 3
	bne	2b
	mov	r0, r0, lsl #4
	subs	r6, r6, #1
	bne	1b
	mov	pc, lr

	.globl	ser_msg
ser_msg:
//	stmfd	sp!, {r4 - r6, lr}
	bl	ser_prints
//	ldmfd	sp!, {r4 - r6, pc}

	.globl	ser_getc
ser_getc:
	mov	r3, #0x42000000
	orr	r3, r3, #0x160
1:	ldr	r2, [r3, #24]
	tst	r2, #1 << 4
	bne	1b
	ldr	r0, [r3, #0]
	mov	pc, lr
