#include <string.h>
#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

#define rwPLUGIN_ID 1

#define RWERROR(errorArgs)                                                                         \
    MACRO_START                                                                                    \
    {                                                                                              \
        RwError _rwErrorCode;                                                                      \
        _rwErrorCode.pluginID = rwPLUGIN_ID;                                                       \
        _rwErrorCode.errorCode = _rwerror errorArgs;                                               \
        RwErrorSet(&_rwErrorCode);                                                                 \
    }                                                                                              \
    MACRO_STOP

#define E_RW_BADVERSION 0x80000004

typedef struct rwStreamCamera rwStreamCamera;
struct rwStreamCamera
{
    RwV2d viewWindow;
    RwV2d viewOffset;
    RwReal nearPlane;
    RwReal farPlane;
    RwReal fogPlane;
    RwUInt32 projection;
};

extern RwPluginRegistry cameraTKList;

RwInt32 RwCameraRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB,
                                     RwPluginDataChunkWriteCallBack writeCB,
                                     RwPluginDataChunkGetSizeCallBack getSizeCB)
{
    RwInt32 plug;

    plug = _rwPluginRegistryAddPluginStream(&cameraTKList, pluginID, readCB, writeCB, getSizeCB);

    return plug;
}

RwCamera* RwCameraStreamRead(RwStream* stream)
{
    RwCamera* camera;
    rwStreamCamera cam;
    RwUInt32 size;
    RwUInt32 version;

    if (!RwStreamFindChunk(stream, rwID_STRUCT, &size, &version))
    {
        return NULL;
    }

    if (version >= rwLIBRARYBASEVERSION && version <= rwLIBRARYCURRENTVERSION)
    {
        memset(&cam, 0, sizeof(cam));
        if (RwStreamRead(stream, &cam, size) != size)
        {
            return NULL;
        }

        RwMemNative32(&cam, sizeof(cam));

        camera = RwCameraCreate();
        if (!camera)
        {
            return NULL;
        }

        if (!_rwPluginRegistryReadDataChunks(&cameraTKList, stream, camera))
        {
            return NULL;
        }

        RwCameraSetViewWindow(camera, &cam.viewWindow);
        RwCameraSetViewOffset(camera, &cam.viewOffset);
        RwCameraSetNearClipPlane(camera, cam.nearPlane);
        RwCameraSetFarClipPlane(camera, cam.farPlane);
        RwCameraSetFogDistance(camera, cam.fogPlane);
        RwCameraSetProjection(camera, (RwCameraProjection)cam.projection);

        return camera;
    }

    RWERROR((E_RW_BADVERSION));
    return NULL;
}
