.section .itcm
.include "mpeg4_header.s"

//These functions copy blocks of 16x16 or 8x8
//This needs to be improved to use 32 bit copy when possible
//Currently byte copies are used because of alignment problems
@@TODO: Half pixels are not supported yet!

//r11 = dx, r12 = dy
//This function is only used for Y, so we don't need to pass a source or destination
.global mpeg4_blockcopy_16x16
mpeg4_blockcopy_16x16:
//use the tempoarly c implementation
	//push {r0-r12,lr}
	push {r0-r3,lr}
	mov r1, r8
	mov r2, r11
	mov r3, r12
	bl mpeg4_blockcopy_16x16_tmp
	pop {r0-r3,pc}
	//pop {r0-r12,pc}



	push {r1-r7,lr}
	//calculate x and y
	mov lr, r8, lsr #STRIDE_SHIFT		//y
	sub r7, r8, lr, lsl #STRIDE_SHIFT	//x
	add lr, r12, asr #1
	add r7, r11, asr #1
	ldrh r6, [r0, #mpeg4_dec_struct__width]
	//add dx and dy and clamp to range 0-width and 0-height
	@adds r2, r12
	@	movmi r2, #0
	@ldrh r1, [r0, #mpeg4_dec_struct__height]
	@cmp r2, r1
	@	movgt r2, r1
	@add r3, r11
	@	movmi r2, #0
	@ldrh r1, [r0, #mpeg4_dec_struct__width]
	@cmp r3, r1
	@	movgt r3, r1
	//calculate min offset
	@add lr, r3, r2, lsl #STRIDE_SHIFT
	//add 16 and clamp again
	@adds r2, #16
	@	movmi r2, #0
	@ldrh r1, [r0, #mpeg4_dec_struct__height]
	@cmp r2, r1
	@	movgt r2, r1
	@add r3, #16
	@	movmi r2, #0
	@ldrh r1, [r0, #mpeg4_dec_struct__width]
	@cmp r3, r1
	@	movgt r3, r1
	//calculate max offset
	@add lr, r3, r2, lsl #STRIDE_SHIFT

	ldr r1, [r0, #mpeg4_dec_struct__pPrevY]
	add r1, r8
	add r1, r11, asr #1
	bic r4, r12, #1
	add r1, r4, lsl #(STRIDE_SHIFT - 1)
	ldr r2, [r0, #mpeg4_dec_struct__pDstY]
	add r2, r8
	mov r3, #16
	and r4, r11, #1
	orr r4, r12, lsl #1
	and r4, #3
	cmp r4, #0
	bne mpeg4_blockcopy_16x16_halfs
mpeg4_blockcopy_16x16_copyloop:
.rept 16
	cmp r7, #0
	ldrgeb r4, [r1], #1
	rsblt r4, r7, #0
	ldrltb r4, [r1, r4]
	addlt r1, #1
	strb r4, [r2], #1
	add r7, #1
.endr
	sub r7, #16
	add r1, #(STRIDE - 16)
	add r2, #(STRIDE - 16)
	subs r3, #1
	bne mpeg4_blockcopy_16x16_copyloop
	pop {r1-r7,pc}

mpeg4_blockcopy_16x16_halfs:
	cmp r4, #1
	beq mpeg4_blockcopy_16x16_halfx
	cmp r4, #2
	beq mpeg4_blockcopy_16x16_halfy
mpeg4_blockcopy_16x16_halfxy:
.rept 16
	cmp r7, #0
	ldrgeb r4, [r1], #1
	ldrgeb r5, [r1, #(STRIDE - 1)]
	addge r4, r5
	ldrgeb r5, [r1]
	addge r4, r5
	ldrgeb r5, [r1, #STRIDE]
	addge r4, r5
	addge r4, #2
	movge r4, r4, lsr #2
	rsblt r4, r7, #0
	ldrltb r4, [r1, r4]
	addlt r1, #1
	strb r4, [r2], #1
	add r7, #1
.endr
	sub r7, #16
	add r1, #(STRIDE - 16)
	add r2, #(STRIDE - 16)
	subs r3, #1
	bne mpeg4_blockcopy_16x16_halfxy
	pop {r1-r7,pc}

mpeg4_blockcopy_16x16_halfx:
.rept 16
	cmp r7, #0
	ldrgeb r4, [r1], #1
	ldrgeb r5, [r1]
	addge r4, r5
	addge r4, #1
	movge r4, r4, lsr #1
	rsblt r4, r7, #0
	ldrltb r4, [r1, r4]
	addlt r1, #1
	strb r4, [r2], #1
	add r7, #1
.endr
	sub r7, #16
	add r1, #(STRIDE - 16)
	add r2, #(STRIDE - 16)
	subs r3, #1
	bne mpeg4_blockcopy_16x16_halfx
	pop {r1-r7,pc}

mpeg4_blockcopy_16x16_halfy:
.rept 16
	cmp r7, #0
	ldrgeb r4, [r1], #1
	ldrgeb r5, [r1, #(STRIDE - 1)]
	addge r4, r5
	addge r4, #1
	movge r4, r4, lsr #1
	rsblt r4, r7, #0
	ldrltb r4, [r1, r4]
	addlt r1, #1
	strb r4, [r2], #1
	add r7, #1
.endr
	sub r7, #16
	add r1, #(STRIDE - 16)
	add r2, #(STRIDE - 16)
	subs r3, #1
	bne mpeg4_blockcopy_16x16_halfy
	pop {r1-r7,pc}

.global mpeg4_blockcopy_8x8_UV
mpeg4_blockcopy_8x8_UV:
//also use the tempoarly c implementation
	//push {r0-r12,lr}
	push {r0-r3,lr}
	mov r1, r8
	mov r2, r11
	mov r3, r12
	bl mpeg4_blockcopy_8x8_UV_tmp
	pop {r0-r3,pc}

.global mpeg4_blockcopy_8x8_Y
mpeg4_blockcopy_8x8_Y:
//also use the tempoarly c implementation
	//push {r0-r12,lr}
	push {r0-r3,lr}
	mov r1, r8
	mov r2, r11
	mov r3, r12
	bl mpeg4_blockcopy_8x8_Y_tmp
	pop {r0-r3,pc}
	//pop {r0-r12,pc}


	push {r1-r7,lr}
	ldr r1, [r0, #mpeg4_dec_struct__pPrevY]
	add r1, r8
	add r1, r11, asr #1
	mov r11, r11
	bic r4, r12, #1
	add r1, r4, lsl #(STRIDE_SHIFT - 1)
	ldr r2, [r0, #mpeg4_dec_struct__pDstY]
	add r2, r8
	mov r3, #16
	and r4, r11, #1
	orr r4, r12, lsl #1
	and r4, #3
	cmp r4, #0
	bne mpeg4_blockcopy_8x8_Y_halfs
mpeg4_blockcopy_8x8_Y_copyloop:
	ldrb r4, [r1]
	ldrb r5, [r1, #1]
	ldrb r6, [r1, #2]
	ldrb r7, [r1, #3]
	strb r4, [r2]
	strb r5, [r2, #1]
	strb r6, [r2, #2]
	strb r7, [r2, #3]
	ldrb r4, [r1, #4]
	ldrb r5, [r1, #5]
	ldrb r6, [r1, #6]
	ldrb r7, [r1, #7]
	strb r4, [r2, #4]
	strb r5, [r2, #5]
	strb r6, [r2, #6]
	strb r7, [r2, #7]
	add r1, #STRIDE
	add r2, #STRIDE
	subs r3, #1
	bne mpeg4_blockcopy_8x8_Y_copyloop
	pop {r1-r7,pc}

mpeg4_blockcopy_8x8_Y_halfs:
	cmp r4, #1
	beq mpeg4_blockcopy_8x8_Y_halfx
	cmp r4, #2
	beq mpeg4_blockcopy_8x8_Y_halfy
mpeg4_blockcopy_8x8_Y_halfxy:
.rept 8
	ldrb r4, [r1], #1
	ldrb r5, [r1, #(STRIDE - 1)]
	add r4, r5
	ldrb r5, [r1]
	add r4, r5
	ldrb r5, [r1, #STRIDE]
	add r4, r5
	add r4, #2
	mov r4, r4, lsr #2
	strb r4, [r2], #1
.endr
	add r1, #(STRIDE - 8)
	add r2, #(STRIDE - 8)
	subs r3, #1
	bne mpeg4_blockcopy_8x8_Y_halfxy
	pop {r1-r7,pc}

mpeg4_blockcopy_8x8_Y_halfx:
.rept 8
	ldrb r4, [r1], #1
	ldrb r5, [r1]
	add r4, r5
	add r4, #1
	mov r4, r4, lsr #1
	strb r4, [r2], #1
.endr
	add r1, #(STRIDE - 8)
	add r2, #(STRIDE - 8)
	subs r3, #1
	bne mpeg4_blockcopy_8x8_Y_halfx
	pop {r1-r7,pc}

mpeg4_blockcopy_8x8_Y_halfy:
.rept 8
	ldrb r4, [r1], #1
	ldrb r5, [r1, #(STRIDE - 1)]
	add r4, r5
	add r4, #1
	mov r4, r4, lsr #1
	strb r4, [r2], #1
.endr
	add r1, #(STRIDE - 8)
	add r2, #(STRIDE - 8)
	subs r3, #1
	bne mpeg4_blockcopy_8x8_Y_halfy
	pop {r1-r7,pc}