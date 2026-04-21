.include "macros.inc"

.section .init, "ax"
.balign 4

.global __init_hardware
.type __init_hardware, @function
__init_hardware:
    mfmsr r0
    ori r0, r0, 0x2000
    mtmsr r0
    mflr r31
    bl __OSPSInit
    bl __OSFPRInit
    bl __OSCacheInit
    mtlr r31
    blr
.size __init_hardware, . - __init_hardware

.global __flush_cache
.type __flush_cache, @function
__flush_cache:
    lis r5, -1
    ori r5, r5, 0xfff1
    and r5, r5, r3
    subf r3, r5, r3
    add r4, r4, r3
.L__flush_cache_loop:
    dcbst 0, r5
    sync
    icbi 0, r5
    addic r5, r5, 8
    addic. r4, r4, -8
    bge .L__flush_cache_loop
    isync
    blr
.size __flush_cache, . - __flush_cache

.section .text, "ax"
.balign 4

.global __init_user
.type __init_user, @function
__init_user:
    mflr r0
    stw r0, 4(r1)
    stwu r1, -8(r1)
    bl __init_cpp
    lwz r0, 12(r1)
    addi r1, r1, 8
    mtlr r0
    blr
.size __init_user, . - __init_user

.type __init_cpp, @function
__init_cpp:
    mflr r0
    stw r0, 4(r1)
    stwu r1, -16(r1)
    stw r31, 12(r1)
    lis r3, _ctors@ha
    addi r0, r3, _ctors@l
    mr r31, r0
    b .L__init_cpp_step2
.L__init_cpp_step2:
    b .L__init_cpp_step3
.L__init_cpp_step3:
    b .L__init_cpp_load
.L__init_cpp_call:
    mtlr r12
    blrl
    addi r31, r31, 4
.L__init_cpp_load:
    lwz r12, 0(r31)
    cmplwi r12, 0
    bne .L__init_cpp_call
    lwz r0, 20(r1)
    lwz r31, 12(r1)
    addi r1, r1, 16
    mtlr r0
    blr
.size __init_cpp, . - __init_cpp

.global _ExitProcess
.type _ExitProcess, @function
_ExitProcess:
    mflr r0
    stw r0, 4(r1)
    stwu r1, -8(r1)
    bl PPCHalt
    lwz r0, 12(r1)
    addi r1, r1, 8
    mtlr r0
    blr
.size _ExitProcess, . - _ExitProcess
