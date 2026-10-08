#include "xutil.h"

#include "xMath.h"

#include <ctype.h>
#if defined(PS2) || defined(XBOX)
#include <stdlib.h>
#include <stdio.h>
#else
#include <PowerPC_EABI_Support\MSL_C\MSL_Common\stdlib.h>
#include <PowerPC_EABI_Support/MSL_C/MSL_Common/ctype_api.h>
#endif

static S32 g_xutilinit;
static S32 g_crc_needinit = 1;
static U32 g_crc32_table[256] = {};

#if defined(PS2)
static U16 ascii_table[3][2] = { { 0x824f, '0' }, { 0x8260, 'A' }, { 0x8281, 'a' } };
static U16 ascii_k_table[33] = {
    0x8140, 0x8149, 0x8168, 0x8194, 0x8190, 0x8193, 0x8195, 0x8166,
    0x8169, 0x816a, 0x8196, 0x817b, 0x8143, 0x817c, 0x8144, 0x815e,
    0x8146, 0x8147, 0x8171, 0x8181, 0x8172, 0x8148, 0x8197, 0x816d,
    0x818f, 0x816e, 0x814f, 0x8151, 0x8165, 0x816f, 0x8162, 0x8170,
    0x8150,
};

void strtosjis(U8* string, U8* dest)
{
    S32 i;
    S32 sjis_code;
    S32 ascii_code;
    U8 stmp2 = '0';
    U8 stmp;
    U8* dest2 = dest;

    for (i = 0; i < 32; i++)
    {
        *dest2++ = 0;
        *dest2++ = 0;
    }

    while (*string)
    {
        stmp = 0;
        ascii_code = *string++;
        if (ascii_code >= 0x20 && ascii_code < 0x30)
            stmp = 1;
        else if (ascii_code >= 0x30 && ascii_code < 0x3a)
            stmp2 = 0;
        else if (ascii_code >= 0x3a && ascii_code < 0x41)
            stmp = 11;
        else if (ascii_code >= 0x41 && ascii_code < 0x5b)
            stmp2 = 1;
        else if (ascii_code >= 0x5b && ascii_code < 0x61)
            stmp = 37;
        else if (ascii_code >= 0x61 && ascii_code < 0x7b)
            stmp2 = 2;
        else if (ascii_code >= 0x7b && ascii_code < 0x7f)
            stmp = 63;
        else
        {
            printf("bad ASCII code 0x%x\n", ascii_code);
            exit(1);
        }

        if (stmp)
            sjis_code = ascii_k_table[(ascii_code - 0x20) - (stmp - 1)];
        else
            sjis_code = ascii_code + ascii_table[stmp2][0] - ascii_table[stmp2][1];

        *dest++ = (sjis_code & 0xff00) >> 8;
        *dest++ = sjis_code;
    }
}

U8 BCDtoi(U8 hex)
{
    char c[16];
    sprintf(c, "%x", hex);
    return atoi(c);
}

U8 itoBCD(U16 dec)
{
    S32 ones = dec % 10;
    return ones + (((dec % 100 - ones) / 10) << 4);
}

U8 itoBCD(U8 dec)
{
    S32 ones = dec % 10;
    return ones + (((dec % 100 - ones) / 10) << 4);
}
#endif

S32 xUtilStartup()
{
    if (!g_xutilinit++)
    {
        xUtil_crc_init();
    }

    return g_xutilinit;
}

S32 xUtilShutdown()
{
    g_xutilinit--;
    return g_xutilinit;
}

char* xUtil_idtag2string(U32 srctag, S32 bufidx)
{
    U32 tag = srctag;
    char* strptr;
    char* uc = (char*)&tag;
    S32 l;
    char t;
    static char buf[6][10] = {};

    if (bufidx < 0 || bufidx >= 7)
    {
        strptr = buf[0];
    }
    else
    {
        strptr = buf[bufidx];
    }

    // convert tag to big endian

    l = 1;

    if ((S32)((char*)&l)[3] != 0)
    {
        t = uc[0];
        uc[0] = uc[3];
        uc[3] = t;

        t = uc[1];
        uc[1] = uc[2];
        uc[2] = t;
    }

    switch (bufidx)
    {
    case 4:
    case 5:
        strptr[0] = isprint(uc[0]) ? uc[0] : '?';
        strptr[1] = isprint(uc[1]) ? uc[1] : '?';
        strptr[2] = isprint(uc[2]) ? uc[2] : '?';
        strptr[3] = isprint(uc[3]) ? uc[3] : '?';
        break;
    case 0:
    case 1:
    case 2:
    case 3:
    case 6:
    default:
        strptr[0] = isprint(uc[3]) ? uc[3] : '?';
        strptr[1] = isprint(uc[2]) ? uc[2] : '?';
        strptr[2] = isprint(uc[1]) ? uc[1] : '?';
        strptr[3] = isprint(uc[0]) ? uc[0] : '?';
        break;
    }

    strptr[4] = '\0';

    if (bufidx == 6)
    {
        strptr[4] = '/';
        strptr[5] = isprint(uc[0]) ? uc[0] : '?';
        strptr[6] = isprint(uc[1]) ? uc[1] : '?';
        strptr[7] = isprint(uc[2]) ? uc[2] : '?';
        strptr[8] = isprint(uc[3]) ? uc[3] : '?';
        strptr[9] = '\0';
    }

    return strptr;
}

U32 xUtil_crc_init()
{
    S32 i, j;
    U32 crc_accum;

    if (g_crc_needinit)
    {
        for (i = 0; i < 256; i++)
        {
            crc_accum = (U32)i << 24;

            for (j = 0; j < 8; j++)
            {
                if (crc_accum & 0x80000000L)
                {
                    crc_accum = (crc_accum << 1) ^ 0x04C11DB7;
                }
                else
                {
                    crc_accum = (crc_accum << 1);
                }
            }

            g_crc32_table[i] = crc_accum;
        }

        g_crc_needinit = 0;
    }

    return 0xFFFFFFFF;
}

U32 xUtil_crc_update(U32 crc_accum, char* data, S32 datasize)
{
    S32 i, j;

    if (g_crc_needinit)
    {
        xUtil_crc_init();
    }

    for (i = 0; i < datasize; i++)
    {
        j = ((crc_accum >> 24) ^ *data++) & 0xff;
        crc_accum = (crc_accum << 8) ^ g_crc32_table[j];
    }

    return crc_accum;
}

S32 xUtil_yesno(F32 wt_yes)
{
    if (0.0f == wt_yes)
    {
        return 0;
    }

    if (1.0f == wt_yes)
    {
        return 1;
    }

    return (xurand() <= wt_yes);
}

void xUtil_wtadjust(F32* wts, S32 cnt, F32 arbref)
{
    const F32 ZERO = 0.0f;

    S32 i;
    F32 sum = 0.0f, fac;

    for (i = 0; i < cnt; i++)
    {
        if (wts[i] < ZERO)
        {
            wts[i] = -wts[i];
        }

        sum += wts[i];
    }

    fac = arbref / sum;

    for (i = 0; i < cnt; i++)
    {
        wts[i] *= fac;
    }
}
