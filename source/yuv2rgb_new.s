.section .itcm

COEF_RV = 27525 //Add one V aswell //93061
COEF_GU = -22544
COEF_GV = -23396 //Mul by 2
COEF_BU = 25297 //Mul by 2 and add one U aswell //116130

.global yuv2rgb_new
yuv2rgb_new:
	stmfd sp!, {r4-r11, lr}
	ldr r10,= (YUV2RGB_ClampRangeBitTable + 256)
	ldr lr,= (144*128)//(144*128)//(192 * 128)
loop:
	ldrh r9, [r1, #128]
	ldrh r8, [r1], #2
	orr r8, r8, r9, lsl #16
	//Get U and V
	and r3, r8, #0xFF	//U
	and r4, r9, #0xFF	//V
	sub r3, #128
	sub r4, #128
	ldr r12,= (((COEF_RV & 0xFFFF) << 16) | (COEF_BU & 0xFFFF))
	ldr r11,= (((COEF_GV & 0xFFFF) << 16) | (COEF_GU & 0xFFFF))
	//Calculate Rbase (R - Y)
	smlawt r5, r4, r12, r4
	//Calculate Gbase (G - Y)
	smulwt r6, r4, r11
	mov r6, r6, lsl #1
	smlawb r6, r3, r11, r6
	//Calculate Bbase (B - Y)
	smulwb r7, r3, r12
	add r7, r3, r7, lsl #1

	ldr r3, [r0], #4
	and r4, r3, #0xFF
	sub r4, #4
	add r12, r10, r4
	ldrb r4, [r12, r5]
	ldrb r11, [r12, r6]
	ldrb r12, [r12, r7]
	orr r4, r4, r11, lsl #5
	orr r4, r4, r12, lsl #10
	orr r4, r4, #0x8000

	mov r11, r3, lsr #8
	and r11, r11, #0xFF
	add r12, r10, r11
	ldrb r9, [r12, r5]
	ldrb r11, [r12, r6]
	ldrb r12, [r12, r7]
	orr r9, r9, r11, lsl #5
	orr r9, r9, r12, lsl #10
	orr r9, r9, #0x8000
	orr r9, r4, r9, lsl #16
	str r9, [r2], #4
	//second line
	ldr r9, [r0, #252]
	and r4, r9, #0xFF
	sub r4, #1
	add r12, r10, r4
	ldrb r4, [r12, r5]
	ldrb r11, [r12, r6]
	ldrb r12, [r12, r7]
	orr r4, r4, r11, lsl #5
	orr r4, r4, r12, lsl #10
	orr r4, r4, #0x8000

	mov r11, r9, lsr #8
	and r11, r11, #0xFF
	add r11, #3
	add r12, r10, r11
	ldrb r5, [r12, r5]
	ldrb r6, [r12, r6]
	ldrb r7, [r12, r7]
	orr r5, r5, r6, lsl #5
	orr r5, r5, r7, lsl #10
	orr r5, r5, #0x8000
	orr r5, r4, r5, lsl #16
	str r5, [r2, #348]//#508]

	//loop unrolling

	//Get U and V
	mov r4, r8, lsr #24	//V
	mov r8, r8, lsr #8
	and r8, r8, #0xFF	//U
	sub r8, #128
	sub r4, #128
	ldr r12,= (((COEF_RV & 0xFFFF) << 16) | (COEF_BU & 0xFFFF))
	ldr r11,= (((COEF_GV & 0xFFFF) << 16) | (COEF_GU & 0xFFFF))
	//Calculate Rbase (R - Y)
	smlawt r5, r4, r12, r4
	//Calculate Gbase (G - Y)
	smulwt r6, r4, r11
	mov r6, r6, lsl #1
	smlawb r6, r8, r11, r6
	//Calculate Bbase (B - Y)
	smulwb r7, r8, r12
	add r7, r8, r7, lsl #1

	mov r11, r3, lsr #16
	and r11, r11, #0xFF
	sub r11, #2
	add r12, r10, r11
	ldrb r4, [r12, r5]
	ldrb r11, [r12, r6]
	ldrb r12, [r12, r7]
	orr r4, r4, r11, lsl #5
	orr r4, r4, r12, lsl #10
	orr r4, r4, #0x8000

	add r12, r10, r3, lsr #24
	add r12, #2
	ldrb r3, [r12, r5]
	ldrb r11, [r12, r6]
	ldrb r12, [r12, r7]
	orr r3, r3, r11, lsl #5
	orr r3, r3, r12, lsl #10
	orr r3, r3, #0x8000
	orr r3, r4, r3, lsl #16
	str r3, [r2], #4
	//second line
	mov r11, r9, lsr #16
	and r11, r11, #0xFF
	sub r11, #3
	add r12, r10, r11
	ldrb r4, [r12, r5]
	ldrb r11, [r12, r6]
	ldrb r12, [r12, r7]
	orr r4, r4, r11, lsl #5
	orr r4, r4, r12, lsl #10
	orr r4, r4, #0x8000

	add r12, r10, r9, lsr #24
	add r12, #1
	ldrb r9, [r12, r5]
	ldrb r11, [r12, r6]
	ldrb r12, [r12, r7]
	orr r9, r9, r11, lsl #5
	orr r9, r9, r12, lsl #10
	orr r9, r9, #0x8000
	orr r9, r4, r9, lsl #16
	str r9, [r2, #348]//#508]

	subs lr, lr, #4
	ldmlefd sp!, {r4-r11, pc}
	and r9, lr, #0xFF
	cmp r9, #80
	bne loop
	//tst lr, #0xFF
	//bne loop
	subeq lr, #80
	addeq r0, #80
	addeq r1, #40
	cmp lr, #0
	ldmlefd sp!, {r4-r11, pc}
	tst lr, #0xFF
	bne loop

	add r0, r0, #256
	add r1, r1, #128
	add r2, r2, #352 //#192 //#352 //#512

	b loop

.pool