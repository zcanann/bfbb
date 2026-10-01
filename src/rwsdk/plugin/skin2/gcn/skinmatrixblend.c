#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

/* Paired single blend of the bone matrices into the skin matrix cache */
asm void _rpSkinMatrixBlendUpdateASM(RwMatrix* dstMat, const RwMatrix* matA, const RwMatrix* matB, const RwMatrix* matC, const RwUInt8* usedBoneList, RwUInt32 numUsedBones)
{
    nofralloc
    stwu r1, -0x98(r1)
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
    mtctr r8
    psq_l f0, 0x0(r6), 0, 0
    psq_l f1, 0x8(r6), 1, 0
    psq_l f2, 0x10(r6), 0, 0
    psq_l f3, 0x18(r6), 1, 0
    psq_l f4, 0x20(r6), 0, 0
    psq_l f5, 0x28(r6), 1, 0
    psq_l f6, 0x30(r6), 0, 0
    psq_l f7, 0x38(r6), 1, 0
    subi r7, r7, 0x1
L_00000074:
    lbzu r11, 0x1(r7)
    rotlwi r11, r11, 6
    add r12, r5, r11
    psq_l f8, 0x0(r12), 0, 0
    psq_l f10, 0x10(r12), 0, 0
    ps_muls0 f24, f0, f8
    psq_l f9, 0x8(r12), 1, 0
    ps_muls0 f25, f1, f8
    psq_l f11, 0x18(r12), 1, 0
    ps_muls0 f26, f0, f10
    psq_l f12, 0x20(r12), 0, 0
    ps_muls0 f27, f1, f10
    psq_l f13, 0x28(r12), 1, 0
    ps_madds1 f24, f2, f8, f24
    psq_l f14, 0x30(r12), 0, 0
    ps_madds1 f25, f3, f8, f25
    psq_l f15, 0x38(r12), 1, 0
    ps_madds1 f26, f2, f10, f26
    ps_madds1 f27, f3, f10, f27
    ps_madds0 f24, f4, f9, f24
    ps_madds0 f25, f5, f9, f25
    ps_madds0 f26, f4, f11, f26
    add r12, r4, r11
    psq_l f16, 0x0(r12), 0, 0
    ps_madds0 f27, f5, f11, f27
    psq_l f17, 0x8(r12), 1, 0
    ps_muls0 f28, f0, f12
    psq_l f18, 0x10(r12), 0, 0
    ps_muls0 f29, f1, f12
    psq_l f19, 0x18(r12), 1, 0
    ps_madds0 f30, f0, f14, f6
    psq_l f20, 0x20(r12), 0, 0
    ps_madds0 f31, f1, f14, f7
    psq_l f21, 0x28(r12), 1, 0
    ps_madds1 f28, f2, f12, f28
    psq_l f22, 0x30(r12), 0, 0
    ps_madds1 f29, f3, f12, f29
    psq_l f23, 0x38(r12), 1, 0
    ps_madds1 f30, f2, f14, f30
    ps_madds1 f31, f3, f14, f31
    ps_madds0 f28, f4, f13, f28
    ps_madds0 f29, f5, f13, f29
    ps_madds0 f30, f4, f15, f30
    ps_madds0 f31, f5, f15, f31
    ps_muls0 f8, f24, f16
    ps_muls0 f9, f25, f16
    ps_muls0 f10, f24, f18
    ps_muls0 f11, f25, f18
    ps_madds1 f8, f26, f16, f8
    ps_madds1 f9, f27, f16, f9
    ps_madds1 f10, f26, f18, f10
    ps_madds1 f11, f27, f18, f11
    ps_madds0 f8, f28, f17, f8
    ps_madds0 f9, f29, f17, f9
    ps_madds0 f10, f28, f19, f10
    add r12, r3, r11
    psq_st f8, 0x0(r12), 0, 0
    ps_madds0 f11, f29, f19, f11
    psq_st f9, 0x8(r12), 1, 0
    ps_muls0 f12, f24, f20
    psq_st f10, 0x10(r12), 0, 0
    ps_muls0 f13, f25, f20
    psq_st f11, 0x18(r12), 1, 0
    ps_madds0 f14, f24, f22, f30
    ps_madds0 f15, f25, f22, f31
    ps_madds1 f12, f26, f20, f12
    ps_madds1 f13, f27, f20, f13
    ps_madds1 f14, f26, f22, f14
    ps_madds1 f15, f27, f22, f15
    ps_madds0 f12, f28, f21, f12
    ps_madds0 f13, f29, f21, f13
    ps_madds0 f14, f28, f23, f14
    ps_madds0 f15, f29, f23, f15
    psq_st f12, 0x20(r12), 0, 0
    psq_st f13, 0x28(r12), 1, 0
    psq_st f14, 0x30(r12), 0, 0
    psq_st f15, 0x38(r12), 1, 0
    bdnz L_00000074
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
    addi r1, r1, 0x98
    blr
}
