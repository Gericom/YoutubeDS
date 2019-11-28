.section .dtcm

.global ycocg_blittable
ycocg_blittable:
.rept 256
    .byte 0
.endr
i = 0
.rept 32
.rept 8
    .byte i
.endr
i = i + 1
.endr
.rept 256
    .byte 31
.endr