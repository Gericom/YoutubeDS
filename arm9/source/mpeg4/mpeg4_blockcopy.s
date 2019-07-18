.section .itcm
.include "mpeg4_header.s"

//use the tempoarly c implementation
mpeg4_blockcopy_16x16_complex:
	pop {r1-r7,lr}
	push {r0-r3,lr}
	mov r1, r8
	mov r2, r11
	mov r3, r12
	bl mpeg4_blockcopy_16x16_tmp
	pop {r0-r3,pc}

//r11 = dx, r12 = dy
//This function is only used for Y, so we don't need to pass a source or destination
.global mpeg4_blockcopy_16x16
mpeg4_blockcopy_16x16:
	//find out if the motion vector is out of bounds:
	push {r1-r7,lr}
	mov lr, r8, lsr #STRIDE_SHIFT		//y
	sub r7, r8, lr, lsl #STRIDE_SHIFT	//x
	cmn r7, r11, asr #1
		bmi mpeg4_blockcopy_16x16_complex
	add r6, r11, #(1 + 32)
	add r7, r6, asr #1
	ldrh r6, [r0, #mpeg4_dec_struct__width]
	cmp r7, r6
		bge mpeg4_blockcopy_16x16_complex
	cmn lr, r12, asr #1
		bmi mpeg4_blockcopy_16x16_complex
	add r6, r12, #(1 + 32)
	add lr, r6, asr #1
	ldrh r6, [r0, #mpeg4_dec_struct__height]
	cmp lr, r6
		bge mpeg4_blockcopy_16x16_complex

mpeg4_blockcopy_16x16_simple:
	//push {r1-r7,lr}
	ldr r1, [r0, #mpeg4_dec_struct__pPrevY]
	bic r4, r12, #1
	add r1, r8
	add r1, r11, asr #1	
	add r1, r4, lsl #(STRIDE_SHIFT - 1)
	ldr r2, [r0, #mpeg4_dec_struct__pDstY]	
	mov r3, #16
	add r2, r8
	and r4, r11, #1
	orr r4, r12, lsl #1
	ands r4, #3
	bne mpeg4_blockcopy_16x16_halfs
	tst r1, #3
	beq mpeg4_blockcopy_16x16_copyloop2
mpeg4_blockcopy_16x16_copyloop:
	ldrb r4, [r1], #1
	ldrb r5, [r1], #1
	ldrb r6, [r1], #1
	ldrb r7, [r1], #1
	
	orr r4, r5, lsl #8
	orr r4, r6, lsl #16
	orr r4, r7, lsl #24
	str r4, [r2], #4

	ldrb r4, [r1], #1
	ldrb r5, [r1], #1
	ldrb r6, [r1], #1
	ldrb r7, [r1], #1

	orr r4, r5, lsl #8
	orr r4, r6, lsl #16
	orr r4, r7, lsl #24
	str r4, [r2], #4

	ldrb r4, [r1], #1
	ldrb r5, [r1], #1
	ldrb r6, [r1], #1
	ldrb r7, [r1], #1
	
	orr r4, r5, lsl #8
	orr r4, r6, lsl #16
	orr r4, r7, lsl #24
	str r4, [r2], #4

	ldrb r4, [r1], #1
	ldrb r5, [r1], #1
	ldrb r6, [r1], #1
	ldrb r7, [r1], #(STRIDE - 15)

	orr r4, r5, lsl #8
	orr r4, r6, lsl #16
	orr r4, r7, lsl #24
	str r4, [r2], #(STRIDE - 12)

	subs r3, #1
	bne mpeg4_blockcopy_16x16_copyloop
	pop {r1-r7,pc}

mpeg4_blockcopy_16x16_copyloop2:
	ldmia r1!, {r4, r5, r6, r7}
	stmia r2!, {r4, r5, r6, r7}
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
	ldrb r4, [r1], #1
	ldrb r5, [r1, #(STRIDE - 1)]	
	ldrb r6, [r1]	
	ldrb r7, [r1, #STRIDE]
	add r4, r5
	add r4, r6
	add r4, r7
	add r4, #2
	mov r4, r4, lsr #2
	strb r4, [r2], #1
.endr
	add r1, #(STRIDE - 16)
	add r2, #(STRIDE - 16)
	subs r3, #1
	bne mpeg4_blockcopy_16x16_halfxy
	pop {r1-r7,pc}

mpeg4_blockcopy_16x16_halfx:
.rept 16
	ldrb r4, [r1], #1
	ldrb r5, [r1]	
	add r4, #1
	add r4, r5
	mov r4, r4, lsr #1
	strb r4, [r2], #1
.endr
	sub r7, #16
	add r1, #(STRIDE - 16)
	add r2, #(STRIDE - 16)
	subs r3, #1
	bne mpeg4_blockcopy_16x16_halfx
	pop {r1-r7,pc}

mpeg4_blockcopy_16x16_halfy:
.rept 16
	ldrb r4, [r1], #1
	ldrb r5, [r1, #(STRIDE - 1)]	
	add r4, #1
	add r4, r5
	mov r4, r4, lsr #1
	strb r4, [r2], #1
.endr
	add r1, #(STRIDE - 16)
	add r2, #(STRIDE - 16)
	subs r3, #1
	bne mpeg4_blockcopy_16x16_halfy
	pop {r1-r7,pc}

.global mpeg4_blockcopy_8x8_UV
mpeg4_blockcopy_8x8_UV:
	//find out if the motion vector is out of bounds:
	push {r1-r7,lr}
	mov lr, r8, lsr #STRIDE_SHIFT		//y
	and r7, r8, #((1 << (STRIDE_SHIFT - 1)) - 1)
	//sub r7, r8, lr, lsl #STRIDE_SHIFT	//x
	cmn r7, r11, asr #1
		bmi mpeg4_blockcopy_8x8_UV_complex
	add r6, r11, #(1 + 16)
	add r7, r6, asr #1
	ldrh r6, [r0, #mpeg4_dec_struct__width]
	cmp r7, r6, lsr #1
		bge mpeg4_blockcopy_8x8_UV_complex
	cmn lr, r12, asr #1
		bmi mpeg4_blockcopy_8x8_UV_complex
	add r6, r12, #(1 + 16)
	add lr, r6, asr #1
	ldrh r6, [r0, #mpeg4_dec_struct__height]
	cmp lr, r6, lsr #1
		bge mpeg4_blockcopy_8x8_UV_complex

	ldr r1, [r0, #mpeg4_dec_struct__pPrevUV]
	ldr r2, [r0, #mpeg4_dec_struct__pDstUV]
	b mpeg4_blockcopy_8x8_simple

	@ push {r0-r3,lr}
	@ mov r1, r8
	@ mov r2, r11
	@ mov r3, r12
	@ bl mpeg4_blockcopy_8x8_UV_tmp
	@ pop {r0-r3,pc}

//use the tempoarly c implementation
mpeg4_blockcopy_8x8_UV_complex:
	pop {r1-r7,lr}
	push {r0-r3,lr}
	mov r1, r8
	mov r2, r11
	mov r3, r12
	bl mpeg4_blockcopy_8x8_UV_tmp
	pop {r0-r3,pc}

//use the tempoarly c implementation
mpeg4_blockcopy_8x8_complex:
	pop {r1-r7,lr}
	push {r0-r3,lr}
	mov r1, r8
	mov r2, r11
	mov r3, r12
	bl mpeg4_blockcopy_8x8_Y_tmp
	pop {r0-r3,pc}

.global mpeg4_blockcopy_8x8_Y
mpeg4_blockcopy_8x8_Y:
	//find out if the motion vector is out of bounds:
	push {r1-r7,lr}
	mov lr, r8, lsr #STRIDE_SHIFT		//y
	sub r7, r8, lr, lsl #STRIDE_SHIFT	//x
	cmn r7, r11, asr #1
		bmi mpeg4_blockcopy_8x8_complex
	add r6, r11, #(1 + 16)
	add r7, r6, asr #1
	ldrh r6, [r0, #mpeg4_dec_struct__width]
	cmp r7, r6
		bge mpeg4_blockcopy_8x8_complex
	cmn lr, r12, asr #1
		bmi mpeg4_blockcopy_8x8_complex
	add r6, r12, #(1 + 16)
	add lr, r6, asr #1
	ldrh r6, [r0, #mpeg4_dec_struct__height]
	cmp lr, r6
		bge mpeg4_blockcopy_8x8_complex

	ldr r1, [r0, #mpeg4_dec_struct__pPrevY]
	ldr r2, [r0, #mpeg4_dec_struct__pDstY]

mpeg4_blockcopy_8x8_simple:
	//push {r1-r7,lr}	
	add r1, r8
	add r1, r11, asr #1
	bic r4, r12, #1
	add r1, r4, lsl #(STRIDE_SHIFT - 1)
	add r2, r8
	mov r3, #8
	and r4, r11, #1
	orr r4, r12, lsl #1
	ands r4, #3
	bne mpeg4_blockcopy_8x8_Y_halfs
	tst r1, #3
		beq mpeg4_blockcopy_8x8_copyloop2
mpeg4_blockcopy_8x8_Y_copyloop:
	ldrb r4, [r1], #1
	ldrb r5, [r1], #1
	ldrb r6, [r1], #1
	ldrb r7, [r1], #1
	orr r4, r5, lsl #8
	orr r4, r6, lsl #16
	orr r4, r7, lsl #24
	str r4, [r2], #4
	ldrb r4, [r1], #1
	ldrb r5, [r1], #1
	ldrb r6, [r1], #1
	ldrb r7, [r1], #(STRIDE - 7)
	orr r4, r5, lsl #8
	orr r4, r6, lsl #16
	orr r4, r7, lsl #24
	str r4, [r2], #(STRIDE - 4)
	subs r3, #1
	bne mpeg4_blockcopy_8x8_Y_copyloop
	pop {r1-r7,pc}

mpeg4_blockcopy_8x8_copyloop2:
	ldmia r1!, {r4, r5}
	stmia r2!, {r4, r5}
	add r1, #(STRIDE - 8)
	add r2, #(STRIDE - 8)	
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