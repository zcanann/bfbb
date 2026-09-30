#ifndef RPUSRDAT_H
#define RPUSRDAT_H

#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

/* C compatibility: these headers use bare tag names as types. */
typedef struct RpUserDataArray RpUserDataArray;


enum RpUserDataFormat
{
    rpNAUSERDATAFORMAT = 0,
    rpINTUSERDATA,
    rpREALUSERDATA,
    rpSTRINGUSERDATA,
    rpUSERDATAFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
typedef enum RpUserDataFormat RpUserDataFormat;

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

extern RwInt32 RpGeometryAddUserDataArray(RpGeometry* geometry, RwChar* name,
                                          RpUserDataFormat format, RwInt32 numElements);
extern RwInt32 RpGeometryGetUserDataArrayCount(const RpGeometry* geometry);
extern RpUserDataArray* RpGeometryGetUserDataArray(const RpGeometry* geometry, RwInt32 data);
extern RwInt32 RpUserDataGetFormatSize(RpUserDataFormat format);
extern RwBool RpUserDataPluginAttach(void);

#ifdef __cplusplus
}
#endif

#endif