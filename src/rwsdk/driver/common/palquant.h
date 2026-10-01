#ifndef PALQUANT_H
#define PALQUANT_H

#include <rwsdk/rwcore.h>

typedef struct _rwPalQuantRGBABox _rwPalQuantRGBABox;
typedef union _rwPalQuantOctNode _rwPalQuantOctNode;

typedef struct RwPalQuant RwPalQuant;
struct RwPalQuant
{
    _rwPalQuantRGBABox* Mcube; /* 0x00 */
    RwReal* Mvv; /* 0x04 */
    _rwPalQuantOctNode* root; /* 0x08 */
    RwFreeList* cubefreelist; /* 0x0C */
};

extern RwBool _rwPalQuantInit(RwPalQuant* pq);
extern void _rwPalQuantTerm(RwPalQuant* pq);
extern void _rwPalQuantAddImage(RwPalQuant* pq, RwImage* img, RwReal weight);
extern RwInt32 _rwPalQuantResolvePalette(RwRGBA* palette, RwInt32 maxcols, RwPalQuant* pq);
extern void _rwPalQuantMatchImage(RwUInt8* dstpixels, RwInt32 dststride, RwInt32 dstdepth,
                                  RwBool dstPacked, RwPalQuant* pq, RwImage* img);

#endif
