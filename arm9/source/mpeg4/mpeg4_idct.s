.section .itcm

ROW_SHIFT = 11
COL_SHIFT = 6

RND0 = 65536
RND1 = 3597
RND2 = 2260
RND3 = 1203
RND4 = 0
RND5 = 120
RND6 = 512
RND7 = 512

.align 4
mpeg4_idct_row_new:
	push {r4-r11,lr}
	//load the values from the table in registers r3-r6, now we have r1 free aswell :)
	ldmia r1, {r3-r6}
	//rx(t,b)
	//r3(c2,c1), r4(c4,c3), r5(c6,c5), r6(crap,c7)
	add r0, #4
	ldmia r0, {r7-r12,lr}
	sub r0, #4
	orr r1, r7, r8
	orr r1, r1, r9		//left
	orr lr, lr, r11
	orr lr, lr, r12		//right
	orrs r11, lr, r10
	ldr r11, [r0]			//in[0]
		bne mpeg4_idct_row_new_right_or_in4
mpeg4_idct_row_new_not_right_or_in4:
	cmp r1, #0
	smlatb r11, r4, r11, r2
	//r2 is free now
		beq mpeg4_idct_row_new_not_left
	smlatb r1, r3, r8, r11		//a0 = k + c2 * in[2];
	rsb r2, r1, r11, lsl #1		//a3 = k - c2 * in[2];
	smlatb r8, r5, r8, r11		//a1 = k + c6 * in[2];
	rsb lr, r8, r11, lsl #1		//a2 = k - c6 * in[2];

	smulbb r10, r3, r7
	smlabb r10, r4, r9, r10		//b0 = c1 * in[1] + c3 * in[3];
	smulbb r11, r6, r9
	rsb r11, r11, #0
	smlabb r11, r4, r7, r11		//b1 = c3 * in[1] - c7 * in[3];
	smulbb r12, r3, r9
	rsb r12, r12, #0
	smlabb r12, r5, r7, r12		//b2 = c5 * in[1] - c1 * in[3];
	smulbb r5, r5, r9
	rsb r5, r5, #0
	smlabb r5, r6, r7, r5		//b3 = c7 * in[1] - c5 * in[3];

	add r1, r10
	mov r3, r1, asr #ROW_SHIFT
	add r8, r11
	mov r4, r8, asr #ROW_SHIFT
	add lr, r12
	mov r6, lr, asr #ROW_SHIFT
	add r2, r5
	mov r7, r2, asr #ROW_SHIFT
	stmia r0!, {r3,r4,r6,r7}

	sub r1, r10, lsl #1
	mov r7, r1, asr #ROW_SHIFT
	sub r8, r11, lsl #1
	mov r6, r8, asr #ROW_SHIFT
	sub lr, r12, lsl #1
	mov r4, lr, asr #ROW_SHIFT
	sub r2, r5, lsl #1
	mov r3, r2, asr #ROW_SHIFT
	stmia r0, {r3,r4,r6,r7}
	mov r0, #1
	pop {r4-r11,pc}

mpeg4_idct_row_new_not_left:
	movs r11, r11, asr #ROW_SHIFT
		moveq r0, #0	//if a0 == 0
		popeq {r4-r11,pc}
	mov r10, r11
	mov r12, r11
	mov lr, r11
	stmia r0!, {r10-r12,lr}
	stmia r0, {r10-r12,lr}
	mov r0, #1
	pop {r4-r11,pc}

mpeg4_idct_row_new_right_or_in4:
	orrs r1, r1, lr
		bne mpeg4_idct_row_new_left_or_right
mpeg4_idct_row_new_not_left_or_right:
	add r11, r10
	smlatb r5, r4, r11, r2	//r5 = a0
	sub r11, r10, lsl #1	//in[0] - in[4]
	smlatb r4, r4, r11, r2	//r4 = a1

	mov r10, r5, asr #ROW_SHIFT
	mov r11, r4, asr #ROW_SHIFT
	mov r12, r4, asr #ROW_SHIFT
	mov lr, r5, asr #ROW_SHIFT
	stmia r0!, {r10-r12,lr}
	stmia r0, {r10-r12,lr}
	mov r0, #1
	pop {r4-r11,pc}
	
mpeg4_idct_row_new_left_or_right:
	smlatb r11, r4, r11, r2
	smlatb r1, r3, r8, r11    //a0 = k + c2 * in[2];
	rsb r2, r1, r11, lsl #1		//a3 = k - c2 * in[2];
	smlatb r8, r5, r8, r11		//a1 = k + c6 * in[2];
	rsb lr, r8, r11, lsl #1		//a2 = k - c6 * in[2];
	smultb r11, r4, r10			//c4 * in[4]
	add r1, r11					//a0 = k + c2 * in[2] + c4 * in[4]
	sub r8, r11					//a1 = k + c6 * in[2] - c4 * in[4]
	sub lr, r11					//a2 = k - c6 * in[2] - c4 * in[4]
	add r2, r11					//a3 = k - c2 * in[2] + c4 * in[4]
	smultb r11, r5, r12			//c6 * in[6]
	add r1, r11					//a0 = k + c2 * in[2] + c4 * in[4] + c6 * in[6]
	sub r2, r11					//a3 = k - c2 * in[2] + c4 * in[4] - c6 * in[6]
	smultb r11, r3, r12			//c2 * in[6]
	sub r8, r11					//a1 = k + c6 * in[2] - c4 * in[4] - c2 * in[6]
	add lr, r11					//a2 = k - c6 * in[2] - c4 * in[4] + c2 * in[6]

	smulbb r10, r3, r7
	smlabb r10, r4, r9, r10		//b0 = c1 * in[1] + c3 * in[3];
	smulbb r11, r6, r9
	rsb r11, r11, #0
	smlabb r11, r4, r7, r11		//b1 = c3 * in[1] - c7 * in[3];
	smulbb r12, r3, r9
	rsb r12, r12, #0
	smlabb r12, r5, r7, r12		//b2 = c5 * in[1] - c1 * in[3];
	smulbb r9, r5, r9
	rsb r9, r9, #0
	smlabb r9, r6, r7, r9		//b3 = c7 * in[1] - c5 * in[3];

	ldr r7, [r0, #(5 << 2)]		//in[5]
	smlabb r10, r5, r7, r10		//b0 = c1 * in[1] + c3 * in[3] + c5 * in[5]
	smlabb r12, r6, r7, r12		//b2 = c5 * in[1] - c1 * in[3] + c7 * in[5]
	smlabb r9, r4, r7, r9		//b3 = c7 * in[1] - c5 * in[3] + c3 * in[5]
	smulbb r7, r3, r7
	sub r11, r11, r7			//b1 = c3 * in[1] - c7 * in[3] - c1 * in[5]

	ldr r7, [r0, #(7 << 2)]		//in[7]
	smlabb r10, r6, r7, r10		//b0 = c1 * in[1] + c3 * in[3] + c5 * in[5] + c7 * in[7]
	smlabb r12, r4, r7, r12		//b2 = c5 * in[1] - c1 * in[3] + c7 * in[5] + c3 * in[7]
	smulbb r4, r5, r7
	sub r11, r11, r4			//b1 = c3 * in[1] - c7 * in[3] - c1 * in[5] - c5 * in[7]
	smulbb r4, r3, r7
	sub r9, r9, r4				//b3 = c7 * in[1] - c5 * in[3] + c3 * in[5] - c1 * in[7]

	add r1, r10
	mov r3, r1, asr #ROW_SHIFT
	add r8, r11
	mov r4, r8, asr #ROW_SHIFT
	add lr, r12
	mov r6, lr, asr #ROW_SHIFT
	add r2, r9
	mov r7, r2, asr #ROW_SHIFT
	stmia r0!, {r3,r4,r6,r7}

	sub r1, r10, lsl #1
	mov r7, r1, asr #ROW_SHIFT
	sub r8, r11, lsl #1
	mov r6, r8, asr #ROW_SHIFT
	sub lr, r12, lsl #1
	mov r4, lr, asr #ROW_SHIFT
	sub r2, r9, lsl #1
	mov r3, r2, asr #ROW_SHIFT
	stmia r0, {r3,r4,r6,r7}
	mov r0, #1
	pop {r4-r11,pc}

TAN1 = 0x32EC
TAN2 = 0x6A0A
TAN3 = 0xAB0E
SQRT2 = 0x5A82

mpeg4_idct_col_8:
	push {r4-r11,lr}
	ldr r12,= TAN1
	ldr lr,= TAN3
	ldr r4, [r0, #(1 << 5)]	//mm7 = (int) in[1 * 8]
	ldr r3, [r0, #(3 << 5)]	//mm6 = (int) in[3 * 8]
	ldr r2, [r0, #(5 << 5)]	//mm5 = (int) in[5 * 8]
	ldr r1, [r0, #(7 << 5)]	//mm4 = (int) in[7 * 8]
	smlawb r5, r1, r12, r4	//mm0 = MULT(TAN1, mm4, 16) + mm7
	rsb r11, r1, #0
	smlawb r6, r4, r12, r11	//mm1 = MULT(TAN1, mm7, 16) - mm4
	smlawb r7, lr, r2, r3	//mm2 = MULT(TAN3, mm5, 16) + mm6
	rsb r11, r2, #0
	smlawb r8, lr, r3, r11	//mm3 = MULT(TAN3, mm6, 16) - mm5

	ldr r12,= (SQRT2 | (TAN2 << 16))

	add r4, r5, r7			//mm7 = mm0 + mm2
	sub r1, r6, r8			//mm4 = mm1 - mm3
	sub r5, r5, r7			//mm0 = mm0 - mm2
	add r6, r6, r8			//mm1 = mm1 + mm3
	add r3, r5, r6			//mm6 = mm0 + mm1
	sub r2, r5, r6			//mm5 = mm0 - mm1
	
	smulwb r2, r2, r12
	//mov r2, r2, lsl #1		//mm5 = 2 * MULT(SQRT2, mm5, 16)
	smulwb r3, r3, r12
	//mov r3, r3, lsl #1		//mm6 = 2 * MULT(SQRT2, mm6, 16)

	ldr r6, [r0, #(2 << 5)]	//mm1 = (int) in[2 * 8]
	ldr r7, [r0, #(6 << 5)]	//mm2 = (int) in[6 * 8]
	smlawt r8, r7, r12, r6	//mm3 = MULT(TAN2, mm2, 16) + mm1
	rsb r7, r7, #0
	smlawt r7, r6, r12, r7	//mm2 = MULT(TAN2, mm1, 16) - mm2

	//LOAD_BUTTERFLY(mm0, mm1, 0 * 8, 4 * 8, spill, in)
	ldr r5, [r0]
	ldr r6, [r0, #(4 << 5)]
	add r5, r6
	sub r6, r5, r6, lsl #1

mpeg4_idct_col_x_finish:

	//BUTTERFLY(mm0, mm3, spill)
	add r5, r8
	sub r8, r5, r8, lsl #1

	//BUTTERFLY(mm0, mm7, spill)
	add r5, r4
	sub r4, r5, r4, lsl #1
	
	str r5, [r0]
	str r4, [r0, #(7 << 5)]

	//BUTTERFLY(mm3, mm4, mm0)
	add r8, r1
	sub r1, r8, r1, lsl #1

	str r8, [r0, #(3 << 5)]
	str r1, [r0, #(4 << 5)]

	//BUTTERFLY(mm1, mm2, mm0)
	add r6, r7
	sub r7, r6, r7, lsl #1

	//BUTTERFLY(mm1, mm6, mm0)
	add r6, r3, lsl #1
	sub r3, r6, r3, lsl #2 //#1

	str r6, [r0, #(1 << 5)]
	str r3, [r0, #(6 << 5)]

	//BUTTERFLY(mm2, mm5, mm0)
	add r7, r2, lsl #1
	sub r2, r7, r2, lsl #2 //#1

	str r7, [r0, #(2 << 5)]
	str r2, [r0, #(5 << 5)]

	pop {r4-r11,pc}

mpeg4_idct_col_4:
	push {r4-r11,lr}

	ldr r12,= TAN1
	ldr lr,= TAN3

	ldr r5, [r0, #(1 << 5)]	//mm0 = (int) in[1 * 8]
	ldr r7, [r0, #(3 << 5)]	//mm2 = (int) in[3 * 8]

	smulwb r6, r12, r5		//mm1 = MULT(TAN1, mm0, 16)
	smulwb r8, lr, r7		//mm3 = MULT(TAN3, mm2, 16)

	ldr r12,= (SQRT2 | (TAN2 << 16))

	add r4, r5, r7			//mm7 = mm0 + mm2
	sub r1, r6, r8			//mm4 = mm1 - mm3
	sub r5, r5, r7			//mm0 = mm0 - mm2
	add r6, r6, r8			//mm1 = mm1 + mm3
	add r3, r5, r6			//mm6 = mm0 + mm1
	sub r2, r5, r6			//mm5 = mm0 - mm1

	smulwb r2, r2, r12
	//mov r2, r2, lsl #1		//mm5 = 2 * MULT(SQRT2, mm5, 16)
	smulwb r3, r3, r12
	//mov r3, r3, lsl #1		//mm6 = 2 * MULT(SQRT2, mm6, 16)

	ldr r5, [r0]
	ldr r8, [r0, #(2 << 5)]
	mov r6, r5
	smulwt r7, r8, r12

	b mpeg4_idct_col_x_finish

mpeg4_idct_col_3:
	push {r4-r11,lr}

	ldr r12,= (TAN1 | (SQRT2 << 16))

	ldr r4, [r0, #(1 << 5)]	//mm7 = (int) in[1 * 8]
	smulwb r1, r4, r12		//mm4 = MULT(TAN1, mm7, 16)

	add r3, r4, r1			//mm6 = mm7 + mm4
	sub r2, r4, r1			//mm5 = mm7 - mm4

	smulwt r2, r2, r12
	//mov r2, r2, lsl #1		//mm5 = 2 * MULT(SQRT2, mm5, 16)
	smulwt r3, r3, r12
	//mov r3, r3, lsl #1		//mm6 = 2 * MULT(SQRT2, mm6, 16)

	ldr r12,= TAN2
	ldr r5, [r0]
	ldr r8, [r0, #(2 << 5)]
	mov r6, r5
	smulwb r7, r12, r8

	b mpeg4_idct_col_x_finish


//int* in
.global mpeg4_idct
mpeg4_idct:
	push {r4,r5,lr}
	mov r4, r0
	mov r5, #7

	ldr r1,= TAB04
	ldr r2,= RND0
	bl mpeg4_idct_row_new //mpeg4_idct_row

	add r0, r4, #(1 << 5)
	ldr r1,= TAB17
	ldr r2,= RND1
	bl mpeg4_idct_row_new //mpeg4_idct_row

	add r0, r4, #(2 << 5)
	ldr r1,= TAB26
	ldr r2,= RND2
	bl mpeg4_idct_row_new //mpeg4_idct_row

	add r0, r4, #(3 << 5)
	ldr r1,= TAB35
	ldr r2,= RND3
	bl mpeg4_idct_row_new //mpeg4_idct_row
	orr r5, r0, lsl #3

	add r0, r4, #(4 << 5)
	ldr r1,= TAB04
	ldr r2,= RND4
	bl mpeg4_idct_row_new //mpeg4_idct_row
	orr r5, r0, lsl #4

	add r0, r4, #(5 << 5)
	ldr r1,= TAB35
	ldr r2,= RND5
	bl mpeg4_idct_row_new //mpeg4_idct_row
	orr r5, r0, lsl #5

	add r0, r4, #(6 << 5)
	ldr r1,= TAB26
	ldr r2,= RND6
	bl mpeg4_idct_row_new //mpeg4_idct_row
	orr r5, r0, lsl #6

	add r0, r4, #(7 << 5)
	ldr r1,= TAB17
	ldr r2,= RND7
	bl mpeg4_idct_row_new //mpeg4_idct_row
	orr r5, r0, lsl #7

	tst r5, #0xF0
		beq mpeg4_idct_cont1
	add r0, r4, #(0 << 2)
	bl mpeg4_idct_col_8
	add r0, r4, #(1 << 2)
	bl mpeg4_idct_col_8
	add r0, r4, #(2 << 2)
	bl mpeg4_idct_col_8
	add r0, r4, #(3 << 2)
	bl mpeg4_idct_col_8
	add r0, r4, #(4 << 2)
	bl mpeg4_idct_col_8
	add r0, r4, #(5 << 2)
	bl mpeg4_idct_col_8
	add r0, r4, #(6 << 2)
	bl mpeg4_idct_col_8
	add r0, r4, #(7 << 2)
	bl mpeg4_idct_col_8
	pop {r4,r5,pc}
mpeg4_idct_cont1:
	tst r5, #0x08
		beq mpeg4_idct_cont2
	add r0, r4, #(0 << 2)
	bl mpeg4_idct_col_4
	add r0, r4, #(1 << 2)
	bl mpeg4_idct_col_4
	add r0, r4, #(2 << 2)
	bl mpeg4_idct_col_4
	add r0, r4, #(3 << 2)
	bl mpeg4_idct_col_4
	add r0, r4, #(4 << 2)
	bl mpeg4_idct_col_4
	add r0, r4, #(5 << 2)
	bl mpeg4_idct_col_4
	add r0, r4, #(6 << 2)
	bl mpeg4_idct_col_4
	add r0, r4, #(7 << 2)
	bl mpeg4_idct_col_4
	pop {r4,r5,pc}
mpeg4_idct_cont2:
	add r0, r4, #(0 << 2)
	bl mpeg4_idct_col_3
	add r0, r4, #(1 << 2)
	bl mpeg4_idct_col_3
	add r0, r4, #(2 << 2)
	bl mpeg4_idct_col_3
	add r0, r4, #(3 << 2)
	bl mpeg4_idct_col_3
	add r0, r4, #(4 << 2)
	bl mpeg4_idct_col_3
	add r0, r4, #(5 << 2)
	bl mpeg4_idct_col_3
	add r0, r4, #(6 << 2)
	bl mpeg4_idct_col_3
	add r0, r4, #(7 << 2)
	bl mpeg4_idct_col_3
	pop {r4,r5,pc}