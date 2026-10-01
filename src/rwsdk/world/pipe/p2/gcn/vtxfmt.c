#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

#include "rwsdk/world/pipe/p2/gcn/gcpipe.h"

#define rwID_GCNVTXFMTPLUGIN 0x511

RwInt32 _rpDlGeomVtxFmtOffset = 0;
RwInt32 _rpDlWorldVtxFmtOffset = 0;

static RwModuleInfo _RpVtxFmtModule;
static RpGameCubeVtxFmt _RpDlVtxFmtDefault;

void RpGameCubeVtxFmtInit(RpGameCubeVtxFmt* fmt);
void RpGameCubeVtxFmtDestroy(RpGameCubeVtxFmt* fmt);

void _rwDlVtxFmtSetup(RpGameCubeVtxFmt* fmt, RxGameCubePipeData* pipeData)
{
    RwUInt32 count;
    RxGameCubeVertexBuffer* vbHeader;
    GXAttr texCoord;

    if (fmt == NULL)
    {
        fmt = &_RpDlVtxFmtDefault;
    }

    vbHeader = (RxGameCubeVertexBuffer*)(pipeData->resEntry + 1);

    GXClearVtxDesc();

    /* Positions */
    count = 0;
    GXSetVtxDesc(GX_VA_POS, (GXAttrType)vbHeader->attr[count].indexType);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, (GXCompType)fmt->pos, fmt->posFrac);
    GXSetArray(GX_VA_POS, vbHeader->attr[count].array, vbHeader->attr[count].stride);
    count++;

    /* Normals */
    if (pipeData->flags & rpGEOMETRYNORMALS)
    {
        if (fmt->nbt)
        {
            GXSetVtxDesc(GX_VA_NBT, (GXAttrType)vbHeader->attr[count].indexType);
            GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_NBT, GX_NRM_NBT, (GXCompType)fmt->norm, 0);
            GXSetArray(GX_VA_NBT, vbHeader->attr[count].array, vbHeader->attr[count].stride);
            count++;
        }
        else
        {
            GXSetVtxDesc(GX_VA_NRM, (GXAttrType)vbHeader->attr[count].indexType);
            GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_NRM, GX_NRM_XYZ, (GXCompType)fmt->norm, 0);
            GXSetArray(GX_VA_NRM, vbHeader->attr[count].array, vbHeader->attr[count].stride);
            count++;
        }
    }

    /* Prelight colors */
    if (pipeData->flags & rpGEOMETRYPRELIT)
    {
        GXSetVtxDesc(GX_VA_CLR0, (GXAttrType)vbHeader->attr[count].indexType);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, (fmt->preLight > rpRGBX8) ? GX_CLR_RGBA : GX_CLR_RGB,
                        (GXCompType)fmt->preLight, 0);
        GXSetArray(GX_VA_CLR0, vbHeader->attr[count].array, vbHeader->attr[count].stride);
        count++;
    }

    /* Texture coordinates */
    if (pipeData->flags & (rpGEOMETRYTEXTURED | rpGEOMETRYTEXTURED2))
    {
        for (texCoord = GX_VA_TEX0; count < vbHeader->numAttrArrays; count++, texCoord++)
        {
            GXSetVtxDesc(texCoord, (GXAttrType)vbHeader->attr[count].indexType);
            GXSetVtxAttrFmt(GX_VTXFMT0, texCoord, GX_TEX_ST,
                            (GXCompType)fmt->texCoord[texCoord - GX_VA_TEX0],
                            fmt->texCoordFrac[texCoord - GX_VA_TEX0]);
            GXSetArray(texCoord, vbHeader->attr[count].array, vbHeader->attr[count].stride);
        }
    }
}

RpGameCubeVtxFmt* _rpGameCubeVtxFmtGetDefault(void)
{
    return &_RpDlVtxFmtDefault;
}

static void* _rxDlVertexFmtConst(void* object, RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    *RWPLUGINOFFSET(RpGameCubeVtxFmt*, object, offsetInObject) = NULL;

    return object;
}

static void* _rxDlVertexFmtDest(void* object, RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    RpGameCubeVtxFmt** vtxFmt;

    vtxFmt = RWPLUGINOFFSET(RpGameCubeVtxFmt*, object, offsetInObject);
    if (*vtxFmt != NULL)
    {
        RpGameCubeVtxFmtDestroy(*vtxFmt);
    }

    return object;
}

static void* _rpDlVtxFmtOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    if (++_RpVtxFmtModule.numInstances == 1)
    {
        RpGameCubeVtxFmtInit(&_RpDlVtxFmtDefault);
    }

    return instance;
}

static void* _rpDlVtxFmtClose(void* instance, RwInt32 offset, RwInt32 size)
{
    _RpVtxFmtModule.numInstances--;

    return instance;
}

RwBool _rpDlVtxFmtPluginAttach(void)
{
    RwInt32 rpDlEngineVtxFmtOffset;

    rpDlEngineVtxFmtOffset =
        RwEngineRegisterPlugin(0, rwID_GCNVTXFMTPLUGIN, _rpDlVtxFmtOpen, _rpDlVtxFmtClose);
    if (rpDlEngineVtxFmtOffset < 0)
    {
        return FALSE;
    }

    _rpDlGeomVtxFmtOffset =
        RpGeometryRegisterPlugin(sizeof(RpGameCubeVtxFmt*), rwID_GCNVTXFMTPLUGIN,
                                 _rxDlVertexFmtConst, _rxDlVertexFmtDest, NULL);
    if (_rpDlGeomVtxFmtOffset < 0)
    {
        return FALSE;
    }

    _rpDlWorldVtxFmtOffset =
        RpWorldRegisterPlugin(sizeof(RpGameCubeVtxFmt*), rwID_GCNVTXFMTPLUGIN, _rxDlVertexFmtConst,
                              _rxDlVertexFmtDest, NULL);

    return _rpDlWorldVtxFmtOffset >= 0;
}

void RpGameCubeVtxFmtSetPosition(RpGameCubeVtxFmt* fmt, RpGameCubeCompType type, RwUInt8 frac)
{
    fmt->pos = type;
    fmt->posFrac = frac;
}

void RpGameCubeVtxFmtSetNormal(RpGameCubeVtxFmt* fmt, RpGameCubeCompType type, RwBool nbt)
{
    fmt->norm = type;
    fmt->nbt = nbt;
}

void RpGameCubeVtxFmtSetTexCoord(RpGameCubeVtxFmt* fmt, RwTextureCoordinateIndex index,
                                 RpGameCubeCompType type, RwUInt8 frac)
{
    fmt->texCoord[index - rwTEXTURECOORDINATEINDEX0] = type;
    fmt->texCoordFrac[index - rwTEXTURECOORDINATEINDEX0] = frac;
}

void RpGameCubeVtxFmtSetPreLight(RpGameCubeVtxFmt* fmt, RpGameCubeColorCompType type)
{
    fmt->preLight = type;
}

void RpGameCubeVtxFmtInit(RpGameCubeVtxFmt* fmt)
{
    RwInt32 i;

    fmt->pos = rpF32;
    fmt->norm = rpF32;
    for (i = 0; i < 8; i++)
    {
        fmt->texCoord[i] = rpF32;
    }
    fmt->preLight = rpRGBA8;
    fmt->format = 1;
    fmt->posFrac = 0;
    for (i = 0; i < 8; i++)
    {
        fmt->texCoordFrac[i] = 0;
    }
    fmt->nbt = FALSE;
    fmt->refCnt = 1;
}

RpGameCubeVtxFmt* RpGameCubeVtxFmtCreate(void)
{
    RpGameCubeVtxFmt* fmt;

    fmt = (RpGameCubeVtxFmt*)RwMalloc(sizeof(RpGameCubeVtxFmt));
    RpGameCubeVtxFmtInit(fmt);

    return fmt;
}

void RpGameCubeVtxFmtDestroy(RpGameCubeVtxFmt* fmt)
{
    if (fmt->refCnt == 1)
    {
        RwFree(fmt);
    }
    else
    {
        fmt->refCnt--;
    }
}

void RpGameCubeGeometrySetVtxFmt(RpGeometry* geometry, RpGameCubeVtxFmt* fmt)
{
    RpGameCubeVtxFmt** vtxFmt;

    vtxFmt = RWPLUGINOFFSET(RpGameCubeVtxFmt*, geometry, _rpDlGeomVtxFmtOffset);
    if (*vtxFmt != NULL)
    {
        RpGameCubeVtxFmtDestroy(*vtxFmt);
    }

    *vtxFmt = fmt;
    (*vtxFmt)->refCnt++;
}
