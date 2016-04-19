.section .itcm
.global MI_CpuCopy8
MI_CpuCopy8:
    cmp     r2, #0
    bxeq    lr

    tst     r1, #1
    beq     _1
    ldrh    r12, [r1, #-1]
    and     r12, r12, #0x00FF
    tst     r0, #1
    ldrneh  r3, [r0, #-1]
    movne   r3, r3, lsr #8
    ldreqh  r3, [r0]
    orr     r3, r12, r3, lsl #8
    strh    r3, [r1, #-1]
    add     r0, r0, #1
    add     r1, r1, #1
    subs    r2, r2, #1
    bxeq    lr
_1:
    eor     r12, r1, r0
    tst     r12, #1
    beq     _2

    bic     r0, r0, #1
    ldrh    r12, [r0], #2
    mov     r3, r12, lsr #8
    subs    r2, r2, #2
    bcc     _3
_4:
    ldrh    r12, [r0], #2
    orr     r12, r3, r12, lsl #8
    strh    r12, [r1], #2
    mov     r3, r12, lsr #16
    subs    r2, r2, #2
    bcs     _4
    
_3:
    tst     r2, #1
    bxeq    lr
    ldrh    r12, [r1]
    and     r12, r12, #0xFF00
    orr     r12, r12, r3
    strh    r12, [r1]
    bx      lr

_2:
    tst     r12, #2
    beq     _5

    bics    r3, r2, #1
    beq     _6
    sub     r2, r2, r3
    add     r12, r3, r1
_7:
    ldrh    r3, [r0], #2
    strh    r3, [r1], #2
    cmp     r1, r12
    bcc     _7
    b       _6

_5:
    cmp     r2, #2
    bcc     _6
    tst     r1, #2
    beq     _8
    ldrh    r3, [r0], #2
    strh    r3, [r1], #2
    subs    r2, r2, #2
    bxeq    lr
_8:
    bics    r3, r2, #3
    beq     _10
    sub     r2, r2, r3
    add     r12, r3, r1
_9:
    ldr     r3, [r0], #4
    str     r3, [r1], #4
    cmp     r1, r12
    bcc     _9

_10:
    tst     r2, #2
    ldrneh  r3, [r0], #2
    strneh  r3, [r1], #2

_6:
    tst     r2, #1
    bxeq    lr
    ldrh    r2, [r1]
    ldrh    r0, [r0]
    and     r2, r2, #0xFF00
    and     r0, r0, #0x00FF
    orr     r0, r2, r0
    strh    r0, [r1]
    bx      lr

.global MI_CpuCopyFast
MI_CpuCopyFast:
    stmfd   sp!, {r4-r10}

    add     r10, r1, r2
    mov     r12, r2, lsr #5
    add     r12, r1, r12, lsl #5

_50:
    cmp     r1, r12
    ldmltia r0!, {r2-r9}
    stmltia r1!, {r2-r9}
    blt     _50
_51:
    cmp     r1, r10
    ldmltia r0!, {r2}
    stmltia r1!, {r2}
    blt     _51

    ldmfd   sp!, {r4-r10}
    bx      lr

.global MI_CpuFillFast
MI_CpuFillFast:
    stmfd   sp!, {r4-r9}

    add     r9, r1, r2
    mov     r12, r2, lsr #5
    add     r12, r1, r12, lsl #5

    mov     r2, r0
    mov     r3, r2
    mov     r4, r2
    mov     r5, r2
    mov     r6, r2
    mov     r7, r2
    mov     r8, r2

_40:
    cmp     r1, r12
    stmltia r1!, {r0, r2-r8}
    blt     _40
_41:
    cmp     r1, r9
    stmltia r1!, {r0}
    blt     _41

    ldmfd   sp!, {r4-r9}
    bx      lr