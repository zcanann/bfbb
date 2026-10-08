#ifndef PS2_LIBCDVD_H
#define PS2_LIBCDVD_H

// The subset of the Sony CD/DVD library interface (libcdvd.h) used by the PS2
// platform layer.

typedef struct
{
    unsigned char stat;
    unsigned char second;
    unsigned char minute;
    unsigned char hour;
    unsigned char pad;
    unsigned char day;
    unsigned char month;
    unsigned char year;
} sceCdCLOCK;

#ifdef __cplusplus
extern "C" {
#endif

#define SCECdINIT 0x00
#define SCECdINoD 0x02
#define SCECdEXIT 0x05

#define SCECdCD 1
#define SCECdDVD 2

#define SCECdPS2CD 0x12
#define SCECdPS2CDDA 0x13
#define SCECdPS2DVD 0x14

int sceCdInit(int init_mode);
int sceCdGetDiskType(void);
int sceCdMmode(int media);
int sceCdReadClock(sceCdCLOCK* rtc);

#ifdef __cplusplus
}
#endif

#endif
