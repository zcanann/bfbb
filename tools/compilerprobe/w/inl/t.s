.include "macros.inc"
.file "t.c"

# 0x00000000..0x00000018 | size: 0x18
.text
.balign 4

# .text:0x0 | size: 0xC
.fn f, local
/* 00000000 00000034  80 04 00 00 */	lwz r0, 0x0(r4)
/* 00000004 00000038  90 03 00 00 */	stw r0, 0x0(r3)
/* 00000008 0000003C  4E 80 00 20 */	blr
.endfn f

# .text:0xC | size: 0xC
.fn g, global
/* 0000000C 00000040  80 04 00 00 */	lwz r0, 0x0(r4)
/* 00000010 00000044  90 03 00 00 */	stw r0, 0x0(r3)
/* 00000014 00000048  4E 80 00 20 */	blr
.endfn g
