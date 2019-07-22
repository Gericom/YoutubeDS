.section .itcm
.include "mpeg4_header.s"

DITHER_COEF_00 = 0
DITHER_COEF_10 = 4
DITHER_COEF_20 = 2
DITHER_COEF_30 = 6
DITHER_COEF_01 = 3
DITHER_COEF_11 = 7
DITHER_COEF_21 = 1
DITHER_COEF_31 = 5

//r0 = y
//r1 = uv
//r2 = out
.macro gen_yog2rgb fullwidth
    stmfd sp!, {r4-r11, lr}
    ldr lr,= (144*128)
    ldr r12,= (ycocg_blittable + 256)
1:
    ldrh r4, [r1, #(STRIDE >> 1)] //2 co samples
    ldrh r5, [r1], #2   //2 cg samples

    ldr r3, [r0], #4    //4 y samples

    orr r4, r4, r5, lsl #16
  
    and r6, r4, #0xFF
    and r7, r4, #0xFF0000
    mov r7, r7, lsr #16
    sub r8, r6, r7 //co - cg

    sub r7, #128 //cg
    add r6, r8, r7, lsl #1 //co + cg
    
    and r9, r3, #0xFF
    add r9, r12
    ldrb r10, [r9, r8]//r
    ldrb r11, [r9, -r6]//b
    ldrb r9, [r9, r7]//g    
    //orr r10, #0x8000
    and r5, r3, #0xFF00

    orr r10, r11, lsl #10
    orr r10, r9, lsl #5

    add r5, r12, r5, lsr #8
    add r5, #DITHER_COEF_10
    ldrb r9, [r5, r8]//r
    ldrb r11, [r5, -r6]//b
    ldrb r5, [r5, r7]//g
    //orr r10, #0x80000000
    orr r10, r9, lsl #16
    orr r10, r11, lsl #(16 + 10)
    orr r10, r5, lsl #(16 + 5)

    ldr r5, [r0, #(STRIDE - 4)]   //4 y samples

    str r10, [r2], #8

    and r9, r5, #0xFF
    add r9, r12
    add r9, #DITHER_COEF_01
    ldrb r10, [r9, r8]//r
    ldrb r11, [r9, -r6]//b
    ldrb r9, [r9, r7]//g
    //orr r10, #0x8000
    orr r10, r11, lsl #10
    orr r10, r9, lsl #5

    and r11, r5, #0xFF00
    add r11, r12, r11, lsr #8
    add r11, #DITHER_COEF_11
    ldrb r8, [r11, r8]//r
    ldrb r6, [r11, -r6]//b
    ldrb r7, [r11, r7]//g
    //orr r10, #0x80000000
    orr r10, r8, lsl #16
    orr r10, r6, lsl #(16 + 10)
    orr r10, r7, lsl #(16 + 5)
    str r10, [r2, #((STRIDE * 2) - 8)]

    //second part
    and r6, r4, #0xFF00
    mov r7, r4, lsr #24
    rsb r8, r7, r6, lsr #8 //co - cg

    sub r7, #128 //cg
    add r6, r8, r7, lsl #1 //co + cg

    and r11, r3, #0xFF0000
    add r9, r12, r11, lsr #16
    add r9, #DITHER_COEF_20
    ldrb r10, [r9, r8]//r
    ldrb r11, [r9, -r6]//b
    ldrb r9, [r9, r7]//g    
    //orr r10, #0x8000

    add r3, r12, r3, lsr #24

    orr r10, r11, lsl #10
    orr r10, r9, lsl #5
    
    add r3, #DITHER_COEF_30
    ldrb r9, [r3, r8]//r
    ldrb r11, [r3, -r6]//b
    ldrb r3, [r3, r7]//g
    //orr r10, #0x80000000
    orr r10, r9, lsl #16
    orr r10, r11, lsl #(16 + 10)
    orr r10, r3, lsl #(16 + 5)
    str r10, [r2, #-4]

    and r11, r5, #0xFF0000
    add r9, r12, r11, lsr #16
    add r9, #DITHER_COEF_21
    ldrb r10, [r9, r8]//r
    ldrb r11, [r9, -r6]//b
    ldrb r9, [r9, r7]//g    
    //orr r10, #0x8000

    add r3, r12, r5, lsr #24

    orr r10, r11, lsl #10
    orr r10, r9, lsl #5
    
    add r3, #DITHER_COEF_31
    ldrb r9, [r3, r8]//r
    ldrb r11, [r3, -r6]//b
    ldrb r3, [r3, r7]//g
    //orr r10, #0x80000000
    orr r10, r9, lsl #16
    orr r10, r11, lsl #(16 + 10)
    orr r10, r3, lsl #(16 + 5)
    str r10, [r2, #((STRIDE * 2) - 4)]

    sub lr, #4
.if \fullwidth
    tst lr, #0xFF
    bne 1b
    add r0, r0, #256
	add r1, r1, #128
	add r2, r2, #512
    cmp lr, #0
        bne 1b    
    ldmfd sp!, {r4-r11, pc}
.else
    and r9, lr, #0xFF
	cmp r9, #80
        bne 1b
	add r0, #(256 + 80)
	add r1, #(128 + 40)
    add r2, #(512 + 160)    
    subs lr, #80
        bne 1b 
    ldmfd sp!, {r4-r11, pc}
.endif
.endm

.global yog2rgb_convert176
yog2rgb_convert176:
	gen_yog2rgb 0

.global yog2rgb_convert256
yog2rgb_convert256:
	gen_yog2rgb 1