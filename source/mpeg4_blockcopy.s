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

	b mpeg4_blockcopy_16x16_simple
	

//use the tempoarly c implementation
mpeg4_blockcopy_16x16_complex:
	pop {r1-r7,lr}
	push {r0-r3,lr}
	mov r1, r8
	mov r2, r11
	mov r3, r12
	bl mpeg4_blockcopy_16x16_tmp
	pop {r0-r3,pc}


mpeg4_blockcopy_16x16_simple:
	//push {r1-r7,lr}
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
	ldrb r4, [r1], #1
	strb r4, [r2], #1
.endr
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
	add r4, r5
	ldrb r5, [r1]
	add r4, r5
	ldrb r5, [r1, #STRIDE]
	add r4, r5
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
	add r4, r5
	add r4, #1
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
	add r4, r5
	add r4, #1
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
	push {r0-r3,lr}
	mov r1, r8
	mov r2, r11
	mov r3, r12
	bl mpeg4_blockcopy_8x8_UV_tmp
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
	b mpeg4_blockcopy_8x8_simple
	

//use the tempoarly c implementation
mpeg4_blockcopy_8x8_complex:
	pop {r1-r7,lr}
	push {r0-r3,lr}
	mov r1, r8
	mov r2, r11
	mov r3, r12
	bl mpeg4_blockcopy_8x8_Y_tmp
	pop {r0-r3,pc}

mpeg4_blockcopy_8x8_simple:
	//push {r1-r7,lr}	
	add r1, r8
	add r1, r11, asr #1
	bic r4, r12, #1
	add r1, r4, lsl #(STRIDE_SHIFT - 1)
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