#ifndef PS2_EEREGS_H
#define PS2_EEREGS_H

// The subset of the Sony EE hardware register map (eeregs.h) used by the PS2
// platform layer.

#define T0_COUNT ((volatile unsigned int*)(0x10000000))
#define T0_MODE ((volatile unsigned int*)(0x10000010))
#define T0_COMP ((volatile unsigned int*)(0x10000020))
#define T0_HOLD ((volatile unsigned int*)(0x10000030))

#define T_MODE_CLKS_M 0x00000003
#define T_MODE_ZRET_M 0x00000040
#define T_MODE_CUE_M 0x00000080
#define T_MODE_CMPE_M 0x00000100
#define T_MODE_OVFE_M 0x00000200
#define T_MODE_EQUF_M 0x00000400
#define T_MODE_OVFF_M 0x00000800

#endif
