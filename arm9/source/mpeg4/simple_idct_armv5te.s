/*
 * Simple IDCT
 *
 * Copyright (c) 2001 Michael Niedermayer <michaelni@gmx.at>
 * Copyright (c) 2006 Mans Rullgard <mans@mansr.com>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

 .section .itcm

#ifdef __ELF__
#   define ELF
#else
#   define ELF @
#endif

#   define A
#   define T @

#if HAVE_AS_FUNC
#   define FUNC
#else
#   define FUNC @
#endif

#   define TFUNC @

        .syntax unified
T       .thumb
ELF     .eabi_attribute 25, 1           @ Tag_ABI_align_preserved
//ELF     .section .note.GNU-stack,"",%progbits @ Mark stack as non-executable

.macro  function name, export=0, align=2
        .set            .Lpic_idx, 0
        .set            .Lpic_gp, 0
    .macro endfunc
      .if .Lpic_idx
        .align          2
        .altmacro
        put_pic         %(.Lpic_idx - 1)
        .noaltmacro
      .endif
      .if .Lpic_gp
        .unreq          gp
      .endif
//ELF     .size   \name, . - \name
FUNC    .endfunc
        .purgem endfunc
    .endm
        //.text
        .align          \align
    .if \export
        .global EXTERN_ASM\name
ELF     .type   EXTERN_ASM\name, %function
FUNC    .func   EXTERN_ASM\name
TFUNC   .thumb_func EXTERN_ASM\name
EXTERN_ASM\name:
    .else
ELF     .type   \name, %function
FUNC    .func   \name
TFUNC   .thumb_func \name
\name:
    .endif
.endm

.macro  const   name, align=2, relocate=0
    .macro endconst
ELF     .size   \name, . - \name
        .purgem endconst
    .endm
#if HAVE_SECTION_DATA_REL_RO
.if \relocate
        .section        .data.rel.ro
.else
        .section        .rodata
.endif
#elif defined(_WIN32)
        .section        .rdata
#elif !defined(__MACH__)
        .section        .rodata
#else
        .const_data
#endif
        .align          \align
\name:
.endm

#   define VFP   @
#   define NOVFP

#define GLUE(a, b) a ## b
#define JOIN(a, b) GLUE(a, b)
#define X(s) JOIN(EXTERN_ASM, s)

#define W1  22725   /* cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5 */
#define W2  21407   /* cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5 */
#define W3  19266   /* cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5 */
#define W4  16383   /* cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5 */
#define W5  12873   /* cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5 */
#define W6  8867    /* cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5 */
#define W7  4520    /* cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5 */
#define ROW_SHIFT 11
#define COL_SHIFT 20

#define W13 (W1 | (W3 << 16))
#define W26 (W2 | (W6 << 16))
#define W57 (W5 | (W7 << 16))

function idct_row_armv5te
        str    lr, [sp, #-4]!

        ldrd   r4, r5, [r0, #8]
        ldrd   r2, r3, [r0]          /* r2 = row[1:0], r3 = row[3:2] */
        orrs   r4, r4, r5
        itt    eq
        cmpeq  r4, r3
        cmpeq  r4, r2, lsr #16
        beq    row_dc_only

        mov    r4, #(1<<(ROW_SHIFT-1))
        mov    r12, #16384
        sub    r12, r12, #1            /* ip = W4 */
        smlabb r4, r12, r2, r4        /* r4 = W4*row[0]+(1<<(RS-1)) */
        ldr    r12, =W26              /* ip = W2 | (W6 << 16) */
        smultb r1, r12, r3
        smulbb lr, r12, r3
        add    r5, r4, r1
        sub    r6, r4, r1
        sub    r7, r4, lr
        add    r4, r4, lr

        ldr    r12, =W13              /* ip = W1 | (W3 << 16) */
        ldr    lr, =W57              /* lr = W5 | (W7 << 16) */
        smulbt r8, r12, r2
        smultt r9, lr, r3
        smlatt r8, r12, r3, r8
        smultt r1, r12, r2
        smulbt r10, lr, r2
        sub    r9, r9, r1
        smulbt r1, r12, r3
        smultt r11, lr, r2
        sub    r10, r10, r1
        smulbt r1, lr, r3
        ldrd   r2, r3, [r0, #8]     /* r2=row[5:4] r3=row[7:6] */
        sub    r11, r11, r1

        orrs   r1, r2, r3
        beq    1f

        smlabt r8, lr, r2, r8
        smlabt r9, r12, r2, r9
        smlatt r8, lr, r3, r8
        smlabt r9, lr, r3, r9
        smlatt r10, lr, r2, r10
        smlatt r11, r12, r2, r11
        smulbt r1, r12, r3
        smlatt r10, r12, r3, r10
        sub    r11, r11, r1

        ldr    r12, =W26              /* ip = W2 | (W6 << 16) */
        mov    r1, #16384
        sub    r1, r1, #1            /* r1 =  W4 */
        smulbb r1, r1, r2            /* r1 =  W4*row[4] */
        smultb lr, r12, r3            /* lr =  W6*row[6] */
        add    r4, r4, r1            /* r4 += W4*row[4] */
        add    r4, r4, lr            /* r4 += W6*row[6] */
        add    r7, r7, r1            /* r7 += W4*row[4] */
        sub    r7, r7, lr            /* r7 -= W6*row[6] */
        smulbb lr, r12, r3            /* lr =  W2*row[6] */
        sub    r5, r5, r1            /* r5 -= W4*row[4] */
        sub    r5, r5, lr            /* r5 -= W2*row[6] */
        sub    r6, r6, r1            /* r6 -= W4*row[4] */
        add    r6, r6, lr            /* r6 += W2*row[6] */

1:      add    r1, r4, r8
        mov    r2, r1, lsr #11
        bic    r2, r2, #0x1f0000
        sub    r1, r5, r9
        mov    r1, r1, lsr #11
        add    r2, r2, r1, lsl #16
        add    r1, r6, r10
        mov    r3, r1, lsr #11
        bic    r3, r3, #0x1f0000
        add    r1, r7, r11
        mov    r1, r1, lsr #11
        add    r3, r3, r1, lsl #16
        strd   r2, r3, [r0]

        sub    r1, r7, r11
        mov    r2, r1, lsr #11
        bic    r2, r2, #0x1f0000
        sub    r1, r6, r10
        mov    r1, r1, lsr #11
        add    r2, r2, r1, lsl #16
        add    r1, r5, r9
        mov    r3, r1, lsr #11
        bic    r3, r3, #0x1f0000
        sub    r1, r4, r8
        mov    r1, r1, lsr #11
        add    r3, r3, r1, lsl #16
        strd   r2, r3, [r0, #8]

        ldr    pc, [sp], #4

row_dc_only:
        orr    r2, r2, r2, lsl #16
        bic    r2, r2, #0xe000
        mov    r2, r2, lsl #3
        mov    r3, r2
        strd   r2, r3, [r0]
        strd   r2, r3, [r0, #8]

        ldr    pc, [sp], #4
endfunc

        .macro idct_col
        ldr    r3, [r0]              /* r3 = col[1:0] */
        mov    r12, #16384
        sub    r12, r12, #1            /* ip = W4 */
        mov    r4, #((1<<(COL_SHIFT-1))/W4) /* this matches the C version */
        add    r5, r4, r3, asr #16
        rsb    r5, r5, r5, lsl #14
        mov    r3, r3, lsl #16
        add    r4, r4, r3, asr #16
        ldr    r3, [r0, #(16*4)]
        rsb    r4, r4, r4, lsl #14

        smulbb lr, r12, r3
        smulbt r2, r12, r3
        sub    r6, r4, lr
        sub    r8, r4, lr
        add    r10, r4, lr
        add    r4, r4, lr
        sub    r7, r5, r2
        sub    r9, r5, r2
        add    r11, r5, r2
        ldr    r12, =W26
        ldr    r3, [r0, #(16*2)]
        add    r5, r5, r2

        smulbb lr, r12, r3
        smultb r2, r12, r3
        add    r4, r4, lr
        sub    r10, r10, lr
        add    r6, r6, r2
        sub    r8, r8, r2
        smulbt lr, r12, r3
        smultt r2, r12, r3
        add    r5, r5, lr
        sub    r11, r11, lr
        add    r7, r7, r2
        ldr    r3, [r0, #(16*6)]
        sub    r9, r9, r2

        smultb lr, r12, r3
        smulbb r2, r12, r3
        add    r4, r4, lr
        sub    r10, r10, lr
        sub    r6, r6, r2
        add    r8, r8, r2
        smultt lr, r12, r3
        smulbt r2, r12, r3
        add    r5, r5, lr
        sub    r11, r11, lr
        sub    r7, r7, r2
        add    r9, r9, r2

        stmfd  sp!, {r4, r5, r6, r7, r8, r9, r10, r11}

        ldr    r12, =W13
        ldr    r3, [r0, #(16*1)]
        ldr    lr, =W57
        smulbb r4, r12, r3
        smultb r6, r12, r3
        smulbb r8, lr, r3
        smultb r10, lr, r3
        smulbt r5, r12, r3
        smultt r7, r12, r3
        smulbt r9, lr, r3
        smultt r11, lr, r3
        rsb    r7, r7, #0
        ldr    r3, [r0, #(16*3)]
        rsb    r6, r6, #0

        smlatb r4, r12, r3, r4
        smlatb r6, lr, r3, r6
        smulbb r2, r12, r3
        smulbb r1, lr, r3
        sub    r8, r8, r2
        sub    r10, r10, r1
        smlatt r5, r12, r3, r5
        smlatt r7, lr, r3, r7
        smulbt r2, r12, r3
        smulbt r1, lr, r3
        sub    r9, r9, r2
        ldr    r3, [r0, #(16*5)]
        sub    r11, r11, r1

        smlabb r4, lr, r3, r4
        smlabb r6, r12, r3, r6
        smlatb r8, lr, r3, r8
        smlatb r10, r12, r3, r10
        smlabt r5, lr, r3, r5
        smlabt r7, r12, r3, r7
        smlatt r9, lr, r3, r9
        ldr    r2, [r0, #(16*7)]
        smlatt r11, r12, r3, r11

        smlatb r4, lr, r2, r4
        smlabb r6, lr, r2, r6
        smlatb r8, r12, r2, r8
        smulbb r3, r12, r2
        smlatt r5, lr, r2, r5
        sub    r10, r10, r3
        smlabt r7, lr, r2, r7
        smulbt r3, r12, r2
        smlatt r9, r12, r2, r9
        sub    r11, r11, r3
        .endm

.macro  clip   dst, src:vararg
        ldrb \dst, [r0, \src]
.endm

.macro  aclip  dst, src:vararg
        add \dst, \src
        ldrb \dst, [r0, \dst]
.endm

function idct_col_put_armv5te
        push {r0, lr}

        idct_col

        ldr    r0,= (idct_clamptable + 256)
        ldmfd  sp!, {r2, r3}
        ldr    lr, [sp, #36]
        add    r1, r2, r4
        clip   r1, r1, asr #20
        add    r12, r3, r5
        clip   r12, r12, asr #20
        sub    r2, r2, r4
        clip   r2, r2, asr #20
        sub    r3, r3, r5
        ldr    r4, [sp, #32]
        clip   r3, r3, asr #20        
        orr    r1, r1, r12, lsl #8
        strh   r1, [r4]
        add    r1, r4, #2
        str    r1, [sp, #32]
        orr    r1, r2, r3, lsl #8
        rsb    r5, lr, lr, lsl #3
        ldmfd  sp!, {r2, r3}
        strh   r1, [r5, r4]!

        sub    r1, r2, r6
        clip   r1, r1, asr #20
        sub    r12, r3, r7
        clip   r12, r12, asr #20
        add    r2, r2, r6
        add    r3, r3, r7
        orr    r1, r1, r12, lsl #8
        strh   r1, [r4, lr]!

        clip   r1, r2, asr #20
        clip   r12, r3, asr #20
        ldmfd  sp!, {r2, r3}
        orr    r1, r1, r12, lsl #8
        strh   r1, [r5, -lr]!

        add    r1, r2, r8
        clip   r1, r1, asr #20
        add    r12, r3, r9
        clip   r12, r12, asr #20
        sub    r2, r2, r8
        sub    r3, r3, r9
        clip   r7, r2, asr #20
        clip   r3, r3, asr #20
        orr    r1, r1, r12, lsl #8
        strh   r1, [r4, lr]!
        orr    r7, r7, r3, lsl #8
        ldmfd  sp!, {r2, r3}
        strh   r7, [r5, -lr]!

        add    r1, r2, r10
        clip   r1, r1, asr #20
        add    r12, r3, r11
        clip   r12, r12, asr #20
        sub    r2, r2, r10
        sub    r3, r3, r11
        clip   r7, r2, asr #20
        clip   r3, r3, asr #20
        orr    r1, r1, r12, lsl #8
        strh   r1, [r4, lr]
        orr    r7, r7, r3, lsl #8
        strh   r7, [r5, -lr]!

        pop {r0, pc}
endfunc

function idct_col_add_armv5te
        push {r0, lr}

        idct_col

        ldr    r0,= (idct_clamptable + 256)
        ldr    lr, [sp, #40]

        ldmfd  sp!, {r2, r3}
        ldrh   r12, [lr]
        add    r1, r2, r4
        sub    r2, r2, r4
        and    r4, r12, #255
        aclip  r1, r4, r1, asr #20
        add    r4, r3, r5
        add    r12, r0, r12, lsr #8
        ldrb   r4, [r12, r4, asr #20]
        sub    r3, r3, r5
        orr    r1, r1, r4, lsl #8
        ldr    r4, [sp, #36]
        rsb    r5, r4, r4, lsl #3
        ldrh   r12, [r5, lr]!
        strh   r1, [lr]
        and    r1, r12, #255
        aclip  r2, r1, r2, asr #20        
        add    r12, r0, r12, lsr #8
        ldrb   r3, [r12, r3, asr #20]
        add    r1, lr, #2
        str    r1, [sp, #32]
        orr    r1, r2, r3, lsl #8
        strh   r1, [r5]

        ldmfd  sp!, {r2, r3}
        ldrh   r12, [lr, r4]!
        sub    r1, r2, r6
        add    r2, r2, r6
        and    r6, r12, #255
        aclip  r1, r6, r1, asr #20
        sub    r6, r3, r7        
        add    r3, r3, r7
        add    r12, r0, r12, lsr #8
        ldrb   r6, [r12, r6, asr #20]
        ldrh   r12, [r5, -r4]!
        orr    r1, r1, r6, lsl #8
        strh   r1, [lr]
        and    r1, r12, #255
        aclip  r1, r1, r2, asr #20
        add    r12, r0, r12, lsr #8
        ldrb   r12, [r12, r3, asr #20]
        ldmfd  sp!, {r2, r3}
        orr    r1, r1, r12, lsl #8
        strh   r1, [r5]
        
        ldrh   r12, [lr, r4]!
        add    r1, r2, r8
        sub    r2, r2, r8
        and    r6, r12, #255
        aclip  r1, r6, r1, asr #20
        add    r6, r3, r9

        add    r12, r0, r12, lsr #8
        ldrb   r6, [r12, r6, asr #20]
        sub    r3, r3, r9
        ldrh   r12, [r5, -r4]!
        orr    r1, r1, r6, lsl #8
        strh   r1, [lr]
        and    r1, r12, #255
        aclip  r1, r1, r2, asr #20

        add    r12, r0, r12, lsr #8
        ldrb   r12, [r12, r3, asr #20]
        ldmfd  sp!, {r2, r3}
        orr    r1, r1, r12, lsl #8
        strh   r1, [r5]
        
        ldrh   r12, [lr, r4]!
        add    r1, r2, r10
        sub    r2, r2, r10
        and    r6, r12, #255
        aclip  r1, r6, r1, asr #20
        add    r6, r3, r11
        add    r12, r0, r12, lsr #8
        ldrb   r6, [r12, r6, asr #20]
        sub    r3, r3, r11
        ldrh   r12, [r5, -r4]!
        orr    r1, r1, r6, lsl #8
        strh   r1, [lr]
        and    r1, r12, #255
        aclip  r2, r1, r2, asr #20

        add    r12, r0, r12, lsr #8
        ldrb   r3, [r12, r3, asr #20]
        orr    r1, r2, r3, lsl #8
        strh   r1, [r5]

        pop {r0, pc}
endfunc

function ff_simple_idct_add_armv5te, export=1
        stmfd  sp!, {r0, r1, r4, r5, r6, r7, r8, r9, r10, r11, lr}

        mov    r0, r2

        bl     idct_row_armv5te
        add    r0, r0, #16
        bl     idct_row_armv5te
        add    r0, r0, #16
        bl     idct_row_armv5te
        add    r0, r0, #16
        bl     idct_row_armv5te
        add    r0, r0, #16
        bl     idct_row_armv5te
        add    r0, r0, #16
        bl     idct_row_armv5te
        add    r0, r0, #16
        bl     idct_row_armv5te
        add    r0, r0, #16
        bl     idct_row_armv5te

        sub    r0, r0, #(16*7)

        bl     idct_col_add_armv5te
        add    r0, r0, #4
        bl     idct_col_add_armv5te
        add    r0, r0, #4
        bl     idct_col_add_armv5te
        add    r0, r0, #4
        bl     idct_col_add_armv5te

        add    sp, sp, #8
        ldmfd  sp!, {r4, r5, r6, r7, r8, r9, r10, r11, pc}
endfunc

function ff_simple_idct_put_armv5te, export=1
        stmfd  sp!, {r0, r1, r4, r5, r6, r7, r8, r9, r10, r11, lr}

        mov    r0, r2

        bl     idct_row_armv5te
        add    r0, r0, #16
        bl     idct_row_armv5te
        add    r0, r0, #16
        bl     idct_row_armv5te
        add    r0, r0, #16
        bl     idct_row_armv5te
        add    r0, r0, #16
        bl     idct_row_armv5te
        add    r0, r0, #16
        bl     idct_row_armv5te
        add    r0, r0, #16
        bl     idct_row_armv5te
        add    r0, r0, #16
        bl     idct_row_armv5te

        sub    r0, r0, #(16*7)

        bl     idct_col_put_armv5te
        add    r0, r0, #4
        bl     idct_col_put_armv5te
        add    r0, r0, #4
        bl     idct_col_put_armv5te
        add    r0, r0, #4
        bl     idct_col_put_armv5te

        add    sp, sp, #8
        ldmfd  sp!, {r4, r5, r6, r7, r8, r9, r10, r11, pc}
endfunc