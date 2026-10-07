#ifndef PS2_ISYSTEM_H
#define PS2_ISYSTEM_H

#include <rwplcore.h>
#include <types.h>

// PS2 DWARF (SB/Core/p2/iSystem.cpp) declares these with plain int types.
// Shared source converts time-base ticks as GET_BUS_FREQUENCY() / 4 (the GameCube
// time base). The PS2 iTime counter runs at BUSCLK / 16 = 9,216,000 Hz, which
// retail folds into constants (153600 ticks per 60 Hz frame, 184320 per 50 Hz).
#define GET_BUS_FREQUENCY() (147456000 / 4)

void iVSync();
unsigned int iRenderWareInit();
void iSystemInit(unsigned int options);
void iSystemExit();
#define JANUARY 1
#define FEBRUARY 2
#define MARCH 3
#define APRIL 4
#define MAY 5
#define JUNE 6
#define JULY 7
#define AUGUST 8
#define SEPTEMBER 9
#define OCTOBER 10
#define NOVEMBER 11
#define DECEMBER 12

U8 iGetMinute();
U8 iGetHour();
U8 iGetDay();
U8 iGetMonth();
U32 iGetCurrFormattedDate(char* str);
U32 iGetCurrFormattedTime(char* str);

#endif
