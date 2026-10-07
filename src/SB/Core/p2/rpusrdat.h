#ifndef PS2_RPUSRDAT_H
#define PS2_RPUSRDAT_H

#include <rwcore.h>
#include <rpworld.h>

// RenderWare user-data plugin declarations; the array layout agrees with all three debug PS2
// originals.
enum RpUserDataFormat
{
    rpNAUSERDATAFORMAT = 0,
    rpINTUSERDATA,
    rpREALUSERDATA,
    rpSTRINGUSERDATA,
    rpUSERDATAFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};

struct RpUserDataArray
{
    RwChar* name;
    RpUserDataFormat format;
    RwInt32 numElements;
    void* data;
};

#ifdef __cplusplus
extern "C" {
#endif

RwInt32 RpGeometryGetUserDataArrayCount(const RpGeometry* geometry);
RpUserDataArray* RpGeometryGetUserDataArray(const RpGeometry* geometry, RwInt32 data);

#ifdef __cplusplus
}
#endif

#endif
