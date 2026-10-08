#ifndef PS2_LIBSCF_H
#define PS2_LIBSCF_H

// The subset of the Sony system configuration library interface (libscf.h)
// used by the PS2 platform layer.

#include <libcdvd.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SCE_DATE_YYYYMMDD 0
#define SCE_DATE_MMDDYYYY 1
#define SCE_DATE_DDMMYYYY 2

#define SCE_TIME_24HOUR 0
#define SCE_TIME_12HOUR 1

void sceScfGetLocalTimefromRTC(sceCdCLOCK* rtc);
int sceScfGetDateNotation(void);
int sceScfGetTimeNotation(void);

#ifdef __cplusplus
}
#endif

#endif
