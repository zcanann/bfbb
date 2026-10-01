#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>
#include <rwsdk/rpptank.h>

/* see ptankgcntransforms.c */
extern const RwReal* _rwPNumber1;
extern const RwReal* _rwConst;
extern const RwUInt32 _rwFifo;

asm void _rpPTankGameCubeAsmVtxRender_CC_CS_NR(RwInt32 actPCount, RwUInt32 flags, RpPTankData* pTankData)
{
    nofralloc
    stwu r1, -0xe8(r1)
    mfcr r0
    stw r0, 0x8(r1)
    stw r14, 0x10(r1)
    stw r15, 0x14(r1)
    stw r16, 0x18(r1)
    stw r17, 0x1c(r1)
    stw r18, 0x20(r1)
    stw r19, 0x24(r1)
    stw r20, 0x28(r1)
    stw r21, 0x2c(r1)
    stw r22, 0x30(r1)
    stw r23, 0x34(r1)
    stw r24, 0x38(r1)
    stw r25, 0x3c(r1)
    stw r26, 0x40(r1)
    stw r27, 0x44(r1)
    stw r28, 0x48(r1)
    stw r29, 0x4c(r1)
    stw r30, 0x50(r1)
    stw r31, 0x54(r1)
    stfd f14, 0x58(r1)
    stfd f15, 0x60(r1)
    stfd f16, 0x68(r1)
    stfd f17, 0x70(r1)
    stfd f18, 0x78(r1)
    stfd f19, 0x80(r1)
    stfd f20, 0x88(r1)
    stfd f21, 0x90(r1)
    stfd f22, 0x98(r1)
    stfd f23, 0xa0(r1)
    stfd f24, 0xa8(r1)
    stfd f25, 0xb0(r1)
    stfd f26, 0xb8(r1)
    stfd f27, 0xc0(r1)
    stfd f28, 0xc8(r1)
    stfd f29, 0xd0(r1)
    stfd f30, 0xd8(r1)
    stfd f31, 0xe0(r1)
    rlwinm r8, r4, 0, 9, 9
    rlwinm r9, r4, 0, 8, 8
    rlwinm r10, r4, 0, 13, 13
    rlwinm r11, r4, 0, 13, 15
    rlwinm r12, r4, 0, 15, 15
    rlwinm r14, r4, 0, 14, 14
    rlwinm r15, r4, 0, 8, 11
    rlwinm r16, r4, 0, 11, 11
    rlwinm r17, r4, 0, 10, 10
    rlwinm r18, r4, 0, 7, 7
    cmplwi cr7, r11, 0x0
    cmplwi cr6, r15, 0x0
    cmplwi cr5, r18, 0x0
    cmplwi cr4, r16, 0x0
    cmplwi cr3, r17, 0x0
    cmplwi cr2, r12, 0x0
    cmplwi cr1, r14, 0x0
    lwz r11, 0x4c(r5)
    lwz r12, _rwPNumber1
    psq_l f11, 0x68(r5), 0, 0
    psq_l f5, 0x0(r11), 0, 0
    psq_l f4, 0x8(r11), 1, 0
    ps_neg f11, f11
    psq_l f3, 0xc(r11), 0, 0
    psq_l f2, 0x14(r11), 1, 0
    ps_muls0 f17, f5, f11
    ps_muls0 f16, f4, f11
    ps_muls1 f15, f3, f11
    ps_muls1 f14, f2, f11
    psq_l f13, 0x70(r5), 0, 0
    psq_l f1, 0x0(r12), 1, 0
    ps_add f31, f17, f15
    ps_add f30, f16, f14
    ps_muls0 f12, f13, f1
    lwz r23, 0x4(r5)
    lwz r19, 0x8(r5)
    ps_muls0 f9, f5, f12
    ps_muls0 f8, f4, f12
    ps_muls1 f7, f3, f12
    ps_muls1 f6, f2, f12
    nop
    ps_add f27, f9, f7
    ps_add f26, f8, f6
    ps_sub f29, f9, f7
    ps_sub f28, f8, f6
    ps_sub f17, f31, f27
    ps_sub f16, f30, f26
    ps_add f15, f31, f29
    ps_add f14, f30, f28
    ps_add f13, f31, f27
    ps_add f12, f30, f26
    ps_sub f11, f31, f29
    ps_sub f10, f30, f28
    beq cr7, L_000001AC
    beq cr2, L_00000184
    lwz r24, 0x24(r5)
    lwz r20, 0x28(r5)
    b L_000001AC
L_00000184:
    beq+ cr1, L_00000194
    lwz r24, 0x2c(r5)
    lwz r20, 0x30(r5)
    b L_000001AC
L_00000194:
    cmplwi r10, 0x0
    bso+ L_000001AC
    lwz r31, 0x80(r5)
    lwz r30, 0x84(r5)
    lwz r29, 0x88(r5)
    lwz r28, 0x8c(r5)
L_000001AC:
    beq+ cr6, L_0000020C
    beq+ cr4, L_000001C0
    lwz r25, 0x3c(r5)
    lwz r21, 0x40(r5)
    b L_0000020C
L_000001C0:
    beq+ cr3, L_000001D0
    lwz r25, 0x44(r5)
    lwz r21, 0x48(r5)
    b L_0000020C
L_000001D0:
    cmplwi r8, 0x0
    bso+ L_000001F4
    psq_l f23, 0x90(r5), 0, 0
    psq_l f21, 0x98(r5), 0, 0
    nop
    nop
    ps_merge01 f22, f23, f21
    ps_merge01 f20, f21, f23
    b L_0000020C
L_000001F4:
    cmplwi r9, 0x0
    bso L_0000020C
    psq_l f23, 0x90(r5), 0, 0
    psq_l f22, 0x98(r5), 0, 0
    psq_l f21, 0xa0(r5), 0, 0
    psq_l f20, 0xa8(r5), 0, 0
L_0000020C:
    beq+ cr5, L_00000218
    lwz r26, 0x14(r5)
    lwz r22, 0x18(r5)
L_00000218:
    cmpwi r3, 0x0
    lwz r27, _rwFifo
    mtctr r3
    ble L_0000036C
L_00000228:
    psq_l f9, 0x0(r23), 0, 0
    psq_l f8, 0x8(r23), 1, 0
    nop
    ps_add f31, f9, f17
    ps_add f30, f8, f16
    ps_add f29, f9, f15
    ps_add f28, f8, f14
    ps_add f27, f9, f13
    ps_add f26, f8, f12
    ps_add f25, f9, f11
    ps_add f24, f8, f10
    beq cr7, L_00000284
    beq cr2, L_00000270
    lwz r31, 0x0(r24)
    lwz r30, 0x0(r24)
    lwz r29, 0x0(r24)
    lwz r28, 0x0(r24)
    b L_00000284
L_00000270:
    beq+ cr1, L_00000284
    lwz r31, 0x0(r24)
    lwz r30, 0x4(r24)
    lwz r29, 0x8(r24)
    lwz r28, 0xc(r24)
L_00000284:
    beq+ cr6, L_000002BC
    beq+ cr4, L_000002A8
    psq_l f23, 0x0(r25), 0, 0
    psq_l f21, 0x8(r25), 0, 0
    nop
    nop
    ps_merge01 f22, f23, f21
    ps_merge01 f20, f21, f23
    b L_000002C8
L_000002A8:
    beq+ cr3, L_000002BC
    psq_l f23, 0x0(r25), 0, 0
    psq_l f22, 0x8(r25), 0, 0
    psq_l f21, 0x10(r25), 0, 0
    psq_l f20, 0x18(r25), 0, 0
L_000002BC:
    beq+ cr5, L_000002C8
    psq_l f19, 0x0(r26), 0, 0
    psq_l f18, 0x8(r26), 1, 0
L_000002C8:
    psq_st f31, 0x0(r27), 0, 6
    psq_st f30, 0x0(r27), 1, 6
    beq+ cr5, L_000002DC
    psq_st f19, 0x0(r27), 0, 7
    psq_st f18, 0x0(r27), 1, 7
L_000002DC:
    beq cr7, L_000002E4
    stw r31, 0x0(r27)
L_000002E4:
    beq+ cr6, L_000002EC
    psq_st f20, 0x0(r27), 0, 0
L_000002EC:
    psq_st f29, 0x0(r27), 0, 6
    psq_st f28, 0x0(r27), 1, 6
    beq+ cr5, L_00000300
    psq_st f19, 0x0(r27), 0, 7
    psq_st f18, 0x0(r27), 1, 7
L_00000300:
    beq cr7, L_00000308
    stw r30, 0x0(r27)
L_00000308:
    beq+ cr6, L_00000310
    psq_st f23, 0x0(r27), 0, 0
L_00000310:
    psq_st f27, 0x0(r27), 0, 6
    psq_st f26, 0x0(r27), 1, 6
    beq+ cr5, L_00000324
    psq_st f19, 0x0(r27), 0, 7
    psq_st f18, 0x0(r27), 1, 7
L_00000324:
    beq cr7, L_0000032C
    stw r29, 0x0(r27)
L_0000032C:
    beq+ cr6, L_00000334
    psq_st f22, 0x0(r27), 0, 0
L_00000334:
    psq_st f25, 0x0(r27), 0, 6
    psq_st f24, 0x0(r27), 1, 6
    add r23, r23, r19
    beq+ cr5, L_00000350
    psq_st f19, 0x0(r27), 0, 7
    psq_st f18, 0x0(r27), 1, 7
    add r26, r26, r22
L_00000350:
    beq cr7, L_0000035C
    stw r28, 0x0(r27)
    add r24, r24, r20
L_0000035C:
    beq+ cr6, L_00000368
    psq_st f21, 0x0(r27), 0, 0
    add r25, r25, r21
L_00000368:
    bdnz L_00000228
L_0000036C:
    lwz r0, 0x8(r1)
    mtcrf 255, r0
    lwz r14, 0x10(r1)
    lwz r15, 0x14(r1)
    lwz r16, 0x18(r1)
    lwz r17, 0x1c(r1)
    lwz r18, 0x20(r1)
    lwz r19, 0x24(r1)
    lwz r20, 0x28(r1)
    lwz r21, 0x2c(r1)
    lwz r22, 0x30(r1)
    lwz r23, 0x34(r1)
    lwz r24, 0x38(r1)
    lwz r25, 0x3c(r1)
    lwz r26, 0x40(r1)
    lwz r27, 0x44(r1)
    lwz r28, 0x48(r1)
    lwz r29, 0x4c(r1)
    lwz r30, 0x50(r1)
    lwz r31, 0x54(r1)
    lfd f14, 0x58(r1)
    lfd f15, 0x60(r1)
    lfd f16, 0x68(r1)
    lfd f17, 0x70(r1)
    lfd f18, 0x78(r1)
    lfd f19, 0x80(r1)
    lfd f20, 0x88(r1)
    lfd f21, 0x90(r1)
    lfd f22, 0x98(r1)
    lfd f23, 0xa0(r1)
    lfd f24, 0xa8(r1)
    lfd f25, 0xb0(r1)
    lfd f26, 0xb8(r1)
    lfd f27, 0xc0(r1)
    lfd f28, 0xc8(r1)
    lfd f29, 0xd0(r1)
    lfd f30, 0xd8(r1)
    lfd f31, 0xe0(r1)
    addi r1, r1, 0xe8
    blr
}
