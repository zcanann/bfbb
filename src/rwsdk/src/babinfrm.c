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
#define E_RW_NOMEM 0x80000013

typedef struct _rwStreamFrameList _rwStreamFrameList;
struct _rwStreamFrameList
{
    RwInt32 numFrames;
};

typedef struct _rwStreamFrame _rwStreamFrame;
struct _rwStreamFrame
{
    RwV3d right;
    RwV3d up;
    RwV3d at;
    RwV3d pos;
    RwInt32 parentIndex;
    RwUInt32 data;
};

extern RwPluginRegistry frameTKList;

RwInt32 RwFrameRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB,
                                    RwPluginDataChunkWriteCallBack writeCB,
                                    RwPluginDataChunkGetSizeCallBack getSizeCB)
{
    RwInt32 plug;

    plug = _rwPluginRegistryAddPluginStream(&frameTKList, pluginID, readCB, writeCB, getSizeCB);

    return plug;
}

RwBool _rwFrameListFindFrame(const rwFrameList* frameList, const RwFrame* frame, RwInt32* npIndex)
{
    RwInt32 i;

    for (i = 0; i < frameList->numFrames; i++)
    {
        if (frameList->frames[i] == frame)
        {
            *npIndex = i;
            return TRUE;
        }
    }

    return FALSE;
}

rwFrameList* _rwFrameListDeinitialize(rwFrameList* frameList)
{
    if (frameList->numFrames)
    {
        RwFree(frameList->frames);
    }

    return frameList;
}

rwFrameList* _rwFrameListStreamRead(RwStream* stream, rwFrameList* frameList)
{
    _rwStreamFrameList fl;
    RwInt32 i;
    RwUInt32 size;
    RwUInt32 version;

    if (!RwStreamFindChunk(stream, rwID_STRUCT, &size, &version))
    {
        return NULL;
    }

    if (version >= rwLIBRARYBASEVERSION && version <= rwLIBRARYCURRENTVERSION)
    {
        if (RwStreamRead(stream, &fl, sizeof(fl)) != sizeof(fl))
        {
            return NULL;
        }

        RwMemNative32(&fl, sizeof(fl));

        frameList->numFrames = fl.numFrames;
        frameList->frames = (RwFrame**)RwMalloc(sizeof(RwFrame*) * fl.numFrames);
        if (!frameList->frames)
        {
            RWERROR((E_RW_NOMEM, sizeof(RwFrame*) * fl.numFrames));
            return NULL;
        }

        for (i = 0; i < fl.numFrames; i++)
        {
            _rwStreamFrame f;
            RwFrame* frame;
            RwMatrix* mat;

            if (RwStreamRead(stream, &f, sizeof(f)) != sizeof(f))
            {
                RwFree(frameList->frames);
                return NULL;
            }

            RwMemNative32(&f, sizeof(f));

            frame = RwFrameCreate();
            if (!frame)
            {
                RwFree(frameList->frames);
                return NULL;
            }

            mat = &frame->modelling;
            mat->right = f.right;
            mat->up = f.up;
            mat->at = f.at;
            mat->pos = f.pos;

            if (((RwReal)0.01 >= _rwMatrixNormalError(mat)) &&
                ((RwReal)0.01 >= _rwMatrixOrthogonalError(mat)) &&
                ((RwReal)0.99 <= _rwMatrixDeterminant(mat)))
            {
                rwMatrixSetFlags(mat, rwMatrixGetFlags(mat) & ~rwMATRIXINTERNALIDENTITY);
            }
            else
            {
                rwMatrixSetFlags(mat, rwMatrixGetFlags(mat) &
                                          ~(rwMATRIXINTERNALIDENTITY | rwMATRIXTYPEMASK));
            }

            frameList->frames[i] = frame;

            if (f.parentIndex >= 0)
            {
                RwFrameAddChild(frameList->frames[f.parentIndex], frame);
            }
        }
    }
    else
    {
        RWERROR((E_RW_BADVERSION));
        return NULL;
    }

    for (i = 0; i < fl.numFrames; i++)
    {
        if (!_rwPluginRegistryReadDataChunks(&frameTKList, stream, frameList->frames[i]))
        {
            RwFrameDestroyHierarchy(frameList->frames[0]);
            RwFree(frameList->frames);
            return NULL;
        }
    }

    return frameList;
}
