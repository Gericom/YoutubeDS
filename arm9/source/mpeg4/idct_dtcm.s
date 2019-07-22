.section .dtcm

.global idct_clamptable
idct_clamptable:
.rept 256
    .byte 0
.endr
i=0
.rept 256
    .byte i
    i = i + 1
.endr
.rept 256
    .byte 255
.endr