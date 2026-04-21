.include "macros.inc"

.section .init, "ax"
.balign 4

.type __check_pad3, @function
__check_pad3:
    mflr r0
    lis r3, -32768
    stw r0, 4(r1)
    stwu r1, -8(r1)
    lhz r0, 0x30e4(r3)
    andi. r0, r0, 0x0eef
    cmpwi r0, 0x0eef
    bne .L__check_pad3_exit
    li r3, 0
    li r4, 0
    li r5, 0
    bl OSResetSystem
.L__check_pad3_exit:
    lwz r0, 12(r1)
    addi r1, r1, 8
    mtlr r0
    blr
.size __check_pad3, . - __check_pad3

.type __set_debug_bba, @function
__set_debug_bba:
    li r0, 1
    stb r0, Debug_BBA@sda21(0)
    blr
.size __set_debug_bba, . - __set_debug_bba

.type __get_debug_bba, @function
__get_debug_bba:
    lbz r3, Debug_BBA@sda21(0)
    blr
.size __get_debug_bba, . - __get_debug_bba

.weak __start
.type __start, @function
__start:
    bl __init_registers
    bl __init_hardware
    li r0, -1
    stwu r1, -8(r1)
    stw r0, 4(r1)
    stw r0, 0(r1)
    bl __init_data
    li r0, 0
    lis r6, -32768
    addi r6, r6, 0x44
    stw r0, 0(r6)
    lis r6, -32768
    addi r6, r6, 0xf4
    lwz r6, 0(r6)

.L__start_check_trk:
    cmplwi r6, 0
    beq .L__start_load_lomem_debug_flag
    lwz r7, 12(r6)
    b .L__start_check_debug_flag

.L__start_load_lomem_debug_flag:
    lis r5, -32768
    addi r5, r5, 0x34
    lwz r5, 0(r5)
    cmplwi r5, 0
    beq .L__start_goto_main
    lis r7, -32768
    addi r7, r7, 0x30e8
    lwz r7, 0(r7)

.L__start_check_debug_flag:
    li r5, 0
    cmplwi r7, 2
    beq .L__start_goto_inittrk
    cmplwi r7, 3
    li r5, 1
    beq .L__start_goto_inittrk
    cmplwi r7, 4
    bne .L__start_goto_main
    li r5, 2
    bl __set_debug_bba
    b .L__start_goto_main

.L__start_goto_inittrk:
    lis r6, InitMetroTRK@ha
    addi r6, r6, InitMetroTRK@l
    mtlr r6
    blrl

.L__start_goto_main:
    lis r6, -32768
    addi r6, r6, 0xf4
    lwz r5, 0(r6)
    cmplwi r5, 0
    beq+ .L__start_no_args
    lwz r6, 8(r5)
    cmplwi r6, 0
    beq+ .L__start_no_args
    add r6, r5, r6
    lwz r14, 0(r6)
    cmplwi r14, 0
    beq .L__start_no_args
    addi r15, r6, 4
    mtctr r14

.L__start_parse_args_loop:
    addi r6, r6, 4
    lwz r7, 0(r6)
    add r7, r7, r5
    stw r7, 0(r6)
    bdnz .L__start_parse_args_loop
    lis r5, -32768
    addi r5, r5, 0x34
    clrrwi r7, r15, 5
    stw r7, 0(r5)
    b .L__start_end_of_parseargs

.L__start_no_args:
    li r14, 0
    li r15, 0

.L__start_end_of_parseargs:
    bl DBInit
    bl OSInit
    lis r4, -32768
    addi r4, r4, 0x30e6
    lhz r3, 0(r4)
    andi. r5, r3, 0x8000
    beq .L__start_check_pad3
    andi. r3, r3, 0x7fff
    cmplwi r3, 1
    bne .L__start_skip_crc

.L__start_check_pad3:
    bl __check_pad3

.L__start_skip_crc:
    bl __get_debug_bba
    cmplwi r3, 1
    bne .L__start_skip_init_bba
    bl InitMetroTRK_BBA

.L__start_skip_init_bba:
    bl __init_user
    mr r3, r14
    mr r4, r15
    bl main
    b exit
.size __start, . - __start

.type __init_registers, @function
__init_registers:
    li r0, 0
    li r3, 0
    li r4, 0
    li r5, 0
    li r6, 0
    li r7, 0
    li r8, 0
    li r9, 0
    li r10, 0
    li r11, 0
    li r12, 0
    li r14, 0
    li r15, 0
    li r16, 0
    li r17, 0
    li r18, 0
    li r19, 0
    li r20, 0
    li r21, 0
    li r22, 0
    li r23, 0
    li r24, 0
    li r25, 0
    li r26, 0
    li r27, 0
    li r28, 0
    li r29, 0
    li r30, 0
    li r31, 0
    lis r1, 0x803d
    ori r1, r1, 0x8a50
    lis r2, _SDA2_BASE_@h
    ori r2, r2, _SDA2_BASE_@l
    lis r13, _SDA_BASE_@h
    ori r13, r13, _SDA_BASE_@l
    blr
.size __init_registers, . - __init_registers

.type __init_data, @function
__init_data:
    mflr r0
    stw r0, 4(r1)
    stwu r1, -24(r1)
    stw r31, 20(r1)
    stw r30, 16(r1)
    stw r29, 12(r1)
    lis r3, _rom_copy_info@ha
    addi r0, r3, _rom_copy_info@l
    mr r29, r0
    b .L__init_data_copy_step2
.L__init_data_copy_step2:
    b .L__init_data_copy_loop
.L__init_data_copy_loop:
    lwz r30, 8(r29)
    cmplwi r30, 0
    beq .L__init_data_bss_setup
    lwz r4, 0(r29)
    lwz r31, 4(r29)
    beq .L__init_data_copy_advance
    cmplw r31, r4
    beq .L__init_data_copy_advance
    mr r3, r31
    mr r5, r30
    bl memcpy
    mr r3, r31
    mr r4, r30
    bl __flush_cache
.L__init_data_copy_advance:
    addi r29, r29, 12
    b .L__init_data_copy_loop

.L__init_data_bss_setup:
    lis r3, _bss_init_info@ha
    addi r0, r3, _bss_init_info@l
    mr r29, r0
    b .L__init_data_bss_step2
.L__init_data_bss_step2:
    b .L__init_data_bss_loop
.L__init_data_bss_loop:
    lwz r5, 4(r29)
    cmplwi r5, 0
    beq .L__init_data_exit
    lwz r3, 0(r29)
    beq .L__init_data_bss_advance
    li r4, 0
    bl memset
.L__init_data_bss_advance:
    addi r29, r29, 8
    b .L__init_data_bss_loop

.L__init_data_exit:
    lwz r0, 28(r1)
    lwz r31, 20(r1)
    lwz r30, 16(r1)
    lwz r29, 12(r1)
    addi r1, r1, 24
    mtlr r0
    blr
.size __init_data, . - __init_data

.section .text, "ax"
.balign 4

.weak InitMetroTRK_BBA
.type InitMetroTRK_BBA, @function
InitMetroTRK_BBA:
    blr
.size InitMetroTRK_BBA, . - InitMetroTRK_BBA

.section .sbss, "wa", @nobits
.balign 8
.type Debug_BBA, @object
Debug_BBA:
    .skip 1
    .skip 7
.size Debug_BBA, 1
