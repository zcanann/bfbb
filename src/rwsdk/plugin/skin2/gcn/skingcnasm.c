#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

#include "rwsdk/plugin/skin2/skin.h"

/*
 * Paired single skinning of the instanced vertices and normals, 2, 3 and 4
 * weights per vertex. The quantized formats are set up in the GQRs by the caller.
 */
asm void _rwDlSkinUpdate2Weights(const RpSkin* skin, const RwMatrix* matrixCache, void* vertices, void* normals, RwUInt32 numVertices, RwUInt32 vertexSize, RwUInt32 normalSize, RwUInt32 normalPad)
{
    nofralloc
    stwu r1, -0xa0(r1)
    stfd f14, 0x8(r1)
    stfd f15, 0x10(r1)
    stfd f16, 0x18(r1)
    stfd f17, 0x20(r1)
    stfd f18, 0x28(r1)
    stfd f19, 0x30(r1)
    stfd f20, 0x38(r1)
    stfd f21, 0x40(r1)
    stfd f22, 0x48(r1)
    stfd f23, 0x50(r1)
    stfd f24, 0x58(r1)
    stfd f25, 0x60(r1)
    stfd f26, 0x68(r1)
    stfd f27, 0x70(r1)
    stfd f28, 0x78(r1)
    stfd f29, 0x80(r1)
    stw r14, 0x88(r1)
    stw r15, 0x8c(r1)
    stw r18, 0x90(r1)
    stw r19, 0x94(r1)
    stw r20, 0x98(r1)
    cmpwi r7, 0x0
    mtctr r7
    ble L_000001D0
    lwz r18, 0x28(r3)
    lwz r19, 0x24(r3)
    lwz r20, 0x1c(r3)
    lwz r3, 0x20(r3)
    subi r19, r19, 0x1
    subi r18, r18, 0x2
    cmpw r20, r3
    subf r20, r8, r20
    subf r3, r9, r3
    subf r5, r8, r5
    subf r6, r9, r6
L_00000090:
    ps_sub f0, f0, f0
    psq_lu f28, 0x1(r19), 1, 5
    psq_lu f29, 0x1(r19), 1, 5
    lhzu r11, 0x2(r18)
    ps_cmpo0 cr5, f29, f0
    rlwinm r14, r11, 30, 18, 25
    clrlslwi r15, r11, 24, 6
    add r14, r14, r4
    add r15, r15, r4
    ble cr5, L_000000BC
    b L_000000C0
L_000000BC:
    addi r15, r14, 0x0
L_000000C0:
    psq_lux f16, r20, r8, 1, 6
    psq_lux f17, r20, r8, 1, 6
    psq_lux f18, r20, r8, 1, 6
    psq_lux f21, r3, r9, 1, 7
    psq_lux f22, r3, r9, 1, 7
    psq_lux f23, r3, r9, 1, 7
    add r3, r3, r10
    psq_l f0, 0x0(r14), 0, 0
    psq_l f1, 0x8(r14), 1, 0
    psq_l f2, 0x10(r14), 0, 0
    psq_l f3, 0x18(r14), 1, 0
    psq_l f4, 0x20(r14), 0, 0
    psq_l f5, 0x28(r14), 1, 0
    psq_l f6, 0x30(r14), 0, 0
    psq_l f7, 0x38(r14), 1, 0
    ps_madds0 f6, f0, f16, f6
    psq_l f8, 0x0(r15), 0, 0
    ps_madds0 f7, f1, f16, f7
    ps_muls0 f24, f0, f21
    psq_l f9, 0x8(r15), 1, 0
    ps_muls0 f25, f1, f21
    ps_madds0 f6, f2, f17, f6
    psq_l f10, 0x10(r15), 0, 0
    ps_madds0 f7, f3, f17, f7
    ps_madds0 f24, f2, f22, f24
    psq_l f11, 0x18(r15), 1, 0
    ps_madds0 f25, f3, f22, f25
    ps_madds0 f6, f4, f18, f6
    psq_l f12, 0x20(r15), 0, 0
    ps_madds0 f7, f5, f18, f7
    ps_madds0 f24, f4, f23, f24
    psq_l f13, 0x28(r15), 1, 0
    ps_madds0 f25, f5, f23, f25
    ps_muls0 f19, f6, f28
    psq_l f14, 0x30(r15), 0, 0
    ps_muls0 f20, f7, f28
    ps_muls0 f26, f24, f28
    psq_l f15, 0x38(r15), 1, 0
    ps_muls0 f27, f25, f28
    ble cr5, L_000001A0
    ps_madds0 f14, f8, f16, f14
    ps_madds0 f15, f9, f16, f15
    ps_muls0 f24, f8, f21
    ps_muls0 f25, f9, f21
    ps_madds0 f14, f10, f17, f14
    ps_madds0 f15, f11, f17, f15
    ps_madds0 f24, f10, f22, f24
    ps_madds0 f25, f11, f22, f25
    ps_madds0 f14, f12, f18, f14
    ps_madds0 f15, f13, f18, f15
    ps_madds0 f24, f12, f23, f24
    ps_madds0 f25, f13, f23, f25
    ps_madds0 f19, f14, f29, f19
    ps_madds0 f20, f15, f29, f20
    ps_madds0 f26, f24, f29, f26
    ps_madds0 f27, f25, f29, f27
L_000001A0:
    ps_merge11 f14, f19, f19
    psq_stux f19, r5, r8, 1, 6
    psq_stux f14, r5, r8, 1, 6
    psq_stux f20, r5, r8, 1, 6
    beq L_000001C8
    ps_merge11 f24, f26, f26
    psq_stux f26, r6, r9, 1, 7
    psq_stux f24, r6, r9, 1, 7
    psq_stux f27, r6, r9, 1, 7
    add r6, r6, r10
L_000001C8:
    bdnz L_00000090
    nop
L_000001D0:
    lfd f14, 0x8(r1)
    lfd f15, 0x10(r1)
    lfd f16, 0x18(r1)
    lfd f17, 0x20(r1)
    lfd f18, 0x28(r1)
    lfd f19, 0x30(r1)
    lfd f20, 0x38(r1)
    lfd f21, 0x40(r1)
    lfd f22, 0x48(r1)
    lfd f23, 0x50(r1)
    lfd f24, 0x58(r1)
    lfd f25, 0x60(r1)
    lfd f26, 0x68(r1)
    lfd f27, 0x70(r1)
    lfd f28, 0x78(r1)
    lfd f29, 0x80(r1)
    lwz r14, 0x88(r1)
    lwz r15, 0x8c(r1)
    lwz r18, 0x90(r1)
    lwz r19, 0x94(r1)
    lwz r20, 0x98(r1)
    addi r1, r1, 0xa0
    blr
}

asm void _rwDlSkinUpdate3Weights(const RpSkin* skin, const RwMatrix* matrixCache, void* vertices, void* normals, RwUInt32 numVertices, RwUInt32 vertexSize, RwUInt32 normalSize, RwUInt32 normalPad)
{
    nofralloc
    stwu r1, -0xac(r1)
    stfd f14, 0x8(r1)
    stfd f15, 0x10(r1)
    stfd f16, 0x18(r1)
    stfd f17, 0x20(r1)
    stfd f18, 0x28(r1)
    stfd f19, 0x30(r1)
    stfd f20, 0x38(r1)
    stfd f21, 0x40(r1)
    stfd f22, 0x48(r1)
    stfd f23, 0x50(r1)
    stfd f24, 0x58(r1)
    stfd f25, 0x60(r1)
    stfd f26, 0x68(r1)
    stfd f27, 0x70(r1)
    stfd f28, 0x78(r1)
    stfd f29, 0x80(r1)
    stfd f30, 0x88(r1)
    stw r14, 0x90(r1)
    stw r15, 0x94(r1)
    stw r16, 0x98(r1)
    stw r18, 0x9c(r1)
    stw r19, 0xa0(r1)
    stw r20, 0xa4(r1)
    cmpwi r7, 0x0
    mtctr r7
    ble L_00000488
    lwz r18, 0x28(r3)
    lwz r19, 0x24(r3)
    lwz r20, 0x1c(r3)
    lwz r3, 0x20(r3)
    subi r19, r19, 0x1
    subi r18, r18, 0x1
    cmpw r20, r3
    subf r20, r8, r20
    subf r3, r9, r3
    subf r5, r8, r5
    subf r6, r9, r6
L_000002C4:
    ps_sub f0, f0, f0
    psq_lu f28, 0x1(r19), 1, 5
    psq_lu f29, 0x1(r19), 1, 5
    psq_lu f30, 0x1(r19), 1, 5
    lbzu r14, 0x1(r18)
    lbzu r15, 0x1(r18)
    lbzu r16, 0x1(r18)
    clrlslwi r14, r14, 24, 6
    clrlslwi r15, r15, 24, 6
    clrlslwi r16, r16, 24, 6
    ps_cmpo0 cr5, f29, f0
    ps_cmpo0 cr6, f30, f0
    add r14, r14, r4
    add r15, r15, r4
    add r16, r16, r4
    ble cr5, L_0000030C
    ble cr6, L_00000310
    b L_00000314
L_0000030C:
    addi r15, r14, 0x0
L_00000310:
    addi r16, r14, 0x0
L_00000314:
    psq_lux f16, r20, r8, 1, 6
    psq_lux f17, r20, r8, 1, 6
    psq_lux f18, r20, r8, 1, 6
    psq_lux f21, r3, r9, 1, 7
    psq_lux f22, r3, r9, 1, 7
    psq_lux f23, r3, r9, 1, 7
    add r3, r3, r10
    psq_l f0, 0x0(r14), 0, 0
    psq_l f1, 0x8(r14), 1, 0
    psq_l f2, 0x10(r14), 0, 0
    psq_l f3, 0x18(r14), 1, 0
    psq_l f4, 0x20(r14), 0, 0
    psq_l f5, 0x28(r14), 1, 0
    psq_l f6, 0x30(r14), 0, 0
    psq_l f7, 0x38(r14), 1, 0
    ps_madds0 f6, f0, f16, f6
    psq_l f8, 0x0(r15), 0, 0
    ps_madds0 f7, f1, f16, f7
    ps_muls0 f24, f0, f21
    psq_l f9, 0x8(r15), 1, 0
    ps_muls0 f25, f1, f21
    ps_madds0 f6, f2, f17, f6
    psq_l f10, 0x10(r15), 0, 0
    ps_madds0 f7, f3, f17, f7
    ps_madds0 f24, f2, f22, f24
    psq_l f11, 0x18(r15), 1, 0
    ps_madds0 f25, f3, f22, f25
    ps_madds0 f6, f4, f18, f6
    psq_l f12, 0x20(r15), 0, 0
    ps_madds0 f7, f5, f18, f7
    ps_madds0 f24, f4, f23, f24
    psq_l f13, 0x28(r15), 1, 0
    ps_madds0 f25, f5, f23, f25
    ps_muls0 f19, f6, f28
    psq_l f14, 0x30(r15), 0, 0
    ps_muls0 f20, f7, f28
    ps_muls0 f26, f24, f28
    psq_l f15, 0x38(r15), 1, 0
    ps_muls0 f27, f25, f28
    ble cr5, L_00000458
    ps_madds0 f14, f8, f16, f14
    psq_l f0, 0x0(r16), 0, 0
    ps_madds0 f15, f9, f16, f15
    ps_muls0 f24, f8, f21
    psq_l f1, 0x8(r16), 1, 0
    ps_muls0 f25, f9, f21
    ps_madds0 f14, f10, f17, f14
    psq_l f2, 0x10(r16), 0, 0
    ps_madds0 f15, f11, f17, f15
    ps_madds0 f24, f10, f22, f24
    psq_l f3, 0x18(r16), 1, 0
    ps_madds0 f25, f11, f22, f25
    ps_madds0 f14, f12, f18, f14
    psq_l f4, 0x20(r16), 0, 0
    ps_madds0 f15, f13, f18, f15
    ps_madds0 f24, f12, f23, f24
    psq_l f5, 0x28(r16), 1, 0
    ps_madds0 f25, f13, f23, f25
    ps_madds0 f19, f14, f29, f19
    psq_l f6, 0x30(r16), 0, 0
    ps_madds0 f20, f15, f29, f20
    ps_madds0 f26, f24, f29, f26
    psq_l f7, 0x38(r16), 1, 0
    ps_madds0 f27, f25, f29, f27
    ble cr6, L_00000458
    ps_madds0 f6, f0, f16, f6
    ps_madds0 f7, f1, f16, f7
    ps_muls0 f24, f0, f21
    ps_muls0 f25, f1, f21
    ps_madds0 f6, f2, f17, f6
    ps_madds0 f7, f3, f17, f7
    ps_madds0 f24, f2, f22, f24
    ps_madds0 f25, f3, f22, f25
    ps_madds0 f6, f4, f18, f6
    ps_madds0 f7, f5, f18, f7
    ps_madds0 f24, f4, f23, f24
    ps_madds0 f25, f5, f23, f25
    ps_madds0 f19, f6, f30, f19
    ps_madds0 f20, f7, f30, f20
    ps_madds0 f26, f24, f30, f26
    ps_madds0 f27, f25, f30, f27
L_00000458:
    ps_merge11 f14, f19, f19
    psq_stux f19, r5, r8, 1, 6
    psq_stux f14, r5, r8, 1, 6
    psq_stux f20, r5, r8, 1, 6
    beq L_00000480
    ps_merge11 f24, f26, f26
    psq_stux f26, r6, r9, 1, 7
    psq_stux f24, r6, r9, 1, 7
    psq_stux f27, r6, r9, 1, 7
    add r6, r6, r10
L_00000480:
    bdnz L_000002C4
    nop
L_00000488:
    lfd f14, 0x8(r1)
    lfd f15, 0x10(r1)
    lfd f16, 0x18(r1)
    lfd f17, 0x20(r1)
    lfd f18, 0x28(r1)
    lfd f19, 0x30(r1)
    lfd f20, 0x38(r1)
    lfd f21, 0x40(r1)
    lfd f22, 0x48(r1)
    lfd f23, 0x50(r1)
    lfd f24, 0x58(r1)
    lfd f25, 0x60(r1)
    lfd f26, 0x68(r1)
    lfd f27, 0x70(r1)
    lfd f28, 0x78(r1)
    lfd f29, 0x80(r1)
    lfd f30, 0x88(r1)
    lwz r14, 0x90(r1)
    lwz r15, 0x94(r1)
    lwz r16, 0x98(r1)
    lwz r18, 0x9c(r1)
    lwz r19, 0xa0(r1)
    lwz r20, 0xa4(r1)
    addi r1, r1, 0xac
    blr
}

asm void _rwDlSkinUpdate4Weights(const RpSkin* skin, const RwMatrix* matrixCache, void* vertices, void* normals, RwUInt32 numVertices, RwUInt32 vertexSize, RwUInt32 normalSize, RwUInt32 normalPad)
{
    nofralloc
    stwu r1, -0xb8(r1)
    stfd f14, 0x8(r1)
    stfd f15, 0x10(r1)
    stfd f16, 0x18(r1)
    stfd f17, 0x20(r1)
    stfd f18, 0x28(r1)
    stfd f19, 0x30(r1)
    stfd f20, 0x38(r1)
    stfd f21, 0x40(r1)
    stfd f22, 0x48(r1)
    stfd f23, 0x50(r1)
    stfd f24, 0x58(r1)
    stfd f25, 0x60(r1)
    stfd f26, 0x68(r1)
    stfd f27, 0x70(r1)
    stfd f28, 0x78(r1)
    stfd f29, 0x80(r1)
    stfd f30, 0x88(r1)
    stfd f31, 0x90(r1)
    stw r14, 0x98(r1)
    stw r15, 0x9c(r1)
    stw r16, 0xa0(r1)
    stw r17, 0xa4(r1)
    stw r18, 0xa8(r1)
    stw r19, 0xac(r1)
    stw r20, 0xb0(r1)
    cmpwi r7, 0x0
    mtctr r7
    ble L_000007C4
    lwz r18, 0x28(r3)
    lwz r19, 0x24(r3)
    lwz r20, 0x1c(r3)
    lwz r3, 0x20(r3)
    subi r19, r19, 0x1
    subi r18, r18, 0x4
    cmpw r20, r3
    subf r20, r8, r20
    subf r3, r9, r3
    subf r5, r8, r5
    subf r6, r9, r6
L_0000058C:
    ps_sub f0, f0, f0
    psq_lu f28, 0x1(r19), 1, 5
    psq_lu f29, 0x1(r19), 1, 5
    psq_lu f30, 0x1(r19), 1, 5
    psq_lu f31, 0x1(r19), 1, 5
    lwzu r11, 0x4(r18)
    ps_cmpo0 cr5, f29, f0
    ps_cmpo0 cr6, f30, f0
    ps_cmpo0 cr7, f31, f0
    rlwinm r14, r11, 14, 18, 25
    rlwinm r15, r11, 22, 18, 25
    rlwinm r16, r11, 30, 18, 25
    clrlslwi r17, r11, 24, 6
    add r14, r14, r4
    add r15, r15, r4
    add r16, r16, r4
    add r17, r17, r4
    ble cr5, L_000005E0
    ble cr6, L_000005E4
    ble cr7, L_000005E8
    b L_000005EC
L_000005E0:
    addi r15, r14, 0x0
L_000005E4:
    addi r16, r14, 0x0
L_000005E8:
    addi r17, r14, 0x0
L_000005EC:
    psq_lux f16, r20, r8, 1, 6
    psq_lux f17, r20, r8, 1, 6
    psq_lux f18, r20, r8, 1, 6
    psq_lux f21, r3, r9, 1, 7
    psq_lux f22, r3, r9, 1, 7
    psq_lux f23, r3, r9, 1, 7
    add r3, r3, r10
    psq_l f0, 0x0(r14), 0, 0
    psq_l f1, 0x8(r14), 1, 0
    psq_l f2, 0x10(r14), 0, 0
    psq_l f3, 0x18(r14), 1, 0
    psq_l f4, 0x20(r14), 0, 0
    psq_l f5, 0x28(r14), 1, 0
    psq_l f6, 0x30(r14), 0, 0
    psq_l f7, 0x38(r14), 1, 0
    ps_madds0 f6, f0, f16, f6
    psq_l f8, 0x0(r15), 0, 0
    ps_madds0 f7, f1, f16, f7
    ps_muls0 f24, f0, f21
    psq_l f9, 0x8(r15), 1, 0
    ps_muls0 f25, f1, f21
    ps_madds0 f6, f2, f17, f6
    psq_l f10, 0x10(r15), 0, 0
    ps_madds0 f7, f3, f17, f7
    ps_madds0 f24, f2, f22, f24
    psq_l f11, 0x18(r15), 1, 0
    ps_madds0 f25, f3, f22, f25
    ps_madds0 f6, f4, f18, f6
    psq_l f12, 0x20(r15), 0, 0
    ps_madds0 f7, f5, f18, f7
    ps_madds0 f24, f4, f23, f24
    psq_l f13, 0x28(r15), 1, 0
    ps_madds0 f25, f5, f23, f25
    ps_muls0 f19, f6, f28
    psq_l f14, 0x30(r15), 0, 0
    ps_muls0 f20, f7, f28
    ps_muls0 f26, f24, f28
    psq_l f15, 0x38(r15), 1, 0
    ps_muls0 f27, f25, f28
    ble cr5, L_00000794
    ps_madds0 f14, f8, f16, f14
    psq_l f0, 0x0(r16), 0, 0
    ps_madds0 f15, f9, f16, f15
    ps_muls0 f24, f8, f21
    psq_l f1, 0x8(r16), 1, 0
    ps_muls0 f25, f9, f21
    ps_madds0 f14, f10, f17, f14
    psq_l f2, 0x10(r16), 0, 0
    ps_madds0 f15, f11, f17, f15
    ps_madds0 f24, f10, f22, f24
    psq_l f3, 0x18(r16), 1, 0
    ps_madds0 f25, f11, f22, f25
    ps_madds0 f14, f12, f18, f14
    psq_l f4, 0x20(r16), 0, 0
    ps_madds0 f15, f13, f18, f15
    ps_madds0 f24, f12, f23, f24
    psq_l f5, 0x28(r16), 1, 0
    ps_madds0 f25, f13, f23, f25
    ps_madds0 f19, f14, f29, f19
    psq_l f6, 0x30(r16), 0, 0
    ps_madds0 f20, f15, f29, f20
    ps_madds0 f26, f24, f29, f26
    psq_l f7, 0x38(r16), 1, 0
    ps_madds0 f27, f25, f29, f27
    ble cr6, L_00000794
    ps_madds0 f6, f0, f16, f6
    psq_l f8, 0x0(r17), 0, 0
    ps_madds0 f7, f1, f16, f7
    ps_muls0 f24, f0, f21
    psq_l f9, 0x8(r17), 1, 0
    ps_muls0 f25, f1, f21
    ps_madds0 f6, f2, f17, f6
    psq_l f10, 0x10(r17), 0, 0
    ps_madds0 f7, f3, f17, f7
    ps_madds0 f24, f2, f22, f24
    psq_l f11, 0x18(r17), 1, 0
    ps_madds0 f25, f3, f22, f25
    ps_madds0 f6, f4, f18, f6
    psq_l f12, 0x20(r17), 0, 0
    ps_madds0 f7, f5, f18, f7
    ps_madds0 f24, f4, f23, f24
    psq_l f13, 0x28(r17), 1, 0
    ps_madds0 f25, f5, f23, f25
    ps_madds0 f19, f6, f30, f19
    psq_l f14, 0x30(r17), 0, 0
    ps_madds0 f20, f7, f30, f20
    ps_madds0 f26, f24, f30, f26
    psq_l f15, 0x38(r17), 1, 0
    ps_madds0 f27, f25, f30, f27
    ble cr7, L_00000794
    ps_madds0 f14, f8, f16, f14
    ps_madds0 f15, f9, f16, f15
    ps_muls0 f24, f8, f21
    ps_muls0 f25, f9, f21
    ps_madds0 f14, f10, f17, f14
    ps_madds0 f15, f11, f17, f15
    ps_madds0 f24, f10, f22, f24
    ps_madds0 f25, f11, f22, f25
    ps_madds0 f14, f12, f18, f14
    ps_madds0 f15, f13, f18, f15
    ps_madds0 f24, f12, f23, f24
    ps_madds0 f25, f13, f23, f25
    ps_madds0 f19, f14, f31, f19
    ps_madds0 f20, f15, f31, f20
    ps_madds0 f26, f24, f31, f26
    ps_madds0 f27, f25, f31, f27
L_00000794:
    ps_merge11 f14, f19, f19
    psq_stux f19, r5, r8, 1, 6
    psq_stux f14, r5, r8, 1, 6
    psq_stux f20, r5, r8, 1, 6
    beq L_000007BC
    ps_merge11 f24, f26, f26
    psq_stux f26, r6, r9, 1, 7
    psq_stux f24, r6, r9, 1, 7
    psq_stux f27, r6, r9, 1, 7
    add r6, r6, r10
L_000007BC:
    bdnz L_0000058C
    nop
L_000007C4:
    lfd f14, 0x8(r1)
    lfd f15, 0x10(r1)
    lfd f16, 0x18(r1)
    lfd f17, 0x20(r1)
    lfd f18, 0x28(r1)
    lfd f19, 0x30(r1)
    lfd f20, 0x38(r1)
    lfd f21, 0x40(r1)
    lfd f22, 0x48(r1)
    lfd f23, 0x50(r1)
    lfd f24, 0x58(r1)
    lfd f25, 0x60(r1)
    lfd f26, 0x68(r1)
    lfd f27, 0x70(r1)
    lfd f28, 0x78(r1)
    lfd f29, 0x80(r1)
    lfd f30, 0x88(r1)
    lfd f31, 0x90(r1)
    lwz r14, 0x98(r1)
    lwz r15, 0x9c(r1)
    lwz r16, 0xa0(r1)
    lwz r17, 0xa4(r1)
    lwz r18, 0xa8(r1)
    lwz r19, 0xac(r1)
    lwz r20, 0xb0(r1)
    addi r1, r1, 0xb8
    blr
}

