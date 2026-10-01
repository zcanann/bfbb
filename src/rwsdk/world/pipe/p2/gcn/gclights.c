#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

#include "rwsdk/world/pipe/p2/gcn/gcpipe.h"

#define rwID_GCNLIGHTPLUGIN 0x505

#define rwDLMAXLIGHTS 8

#define rwDLDIRECTIONALLIGHTDISTANCE (-1048576.0f)

typedef struct _rwDlLightExt _rwDlLightExt;
struct _rwDlLightExt
{
    RwUInt32 flags;
    RwReal a0;
    RwReal a1;
    RwReal a2;
    RwReal k0;
    RwReal k1;
    RwReal k2;
};

extern RwMatrix _RwDlInvCamLTM;

RwInt32 _RwDlLightExtOffset;

static GXLightObj _RwGCLightObjs[rwDLMAXLIGHTS];

#define LIGHTEXTFROMLIGHT(light) (RWPLUGINOFFSET(_rwDlLightExt, (light), _RwDlLightExtOffset))

static void _rpGCHWLightingApplyDirectionalLight(const RpLight* light, RwInt32 lightNum)
{
    RwV3d at;
    GXColor color;
    const RwRGBAReal* lightColor;
    const _rwDlLightExt* lightExt;

    lightColor = &light->color;

    RwV3dTransformVectors(&at, &RwFrameGetLTM(RpLightGetFrame(light))->at, 1, &_RwDlInvCamLTM);

    lightExt = LIGHTEXTFROMLIGHT(light);
    if (lightExt->flags == 0)
    {
        GXInitLightAttn(&_RwGCLightObjs[lightNum], 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    }
    else
    {
        GXInitLightAttn(&_RwGCLightObjs[lightNum], lightExt->a0, lightExt->a1, lightExt->a2,
                        lightExt->k0, lightExt->k1, lightExt->k2);
    }

    GXInitLightPos(&_RwGCLightObjs[lightNum], rwDLDIRECTIONALLIGHTDISTANCE * -at.x,
                   rwDLDIRECTIONALLIGHTDISTANCE * at.y, rwDLDIRECTIONALLIGHTDISTANCE * -at.z);

    color.r = (RwUInt8)(255.0f * lightColor->red);
    color.g = (RwUInt8)(255.0f * lightColor->green);
    color.b = (RwUInt8)(255.0f * lightColor->blue);
    color.a = 0;
    GXInitLightColor(&_RwGCLightObjs[lightNum], color);

    GXLoadLightObjImm(&_RwGCLightObjs[lightNum], (GXLightID)(1 << lightNum));
}

void _rwGCLightsGlobalEnable(RpLightFlag lightFlags, RxGameCubePipeData* pipeData)
{
    RpWorld* world;
    RwLLLink* cur;
    RwLLLink* end;
    const RwRGBAReal* color;
    RpLight* light;

    world = (RpWorld*)RWSRCGLOBAL(curWorld);

    cur = rwLinkListGetFirstLLLink(&world->directionalLightList);
    end = rwLinkListGetTerminator(&world->directionalLightList);
    while (cur != end)
    {
        light = rwLLLinkGetData(cur, RpLight, inWorld);

        if (light != NULL && rwObjectTestFlags(light, lightFlags))
        {
            if (RpLightGetType(light) == rpLIGHTDIRECTIONAL)
            {
                _rpGCHWLightingApplyDirectionalLight(light, pipeData->numLights);

                pipeData->lightMask |= 1 << pipeData->numLights;
                pipeData->numLights++;
            }
            else
            {
                color = &light->color;

                pipeData->ambientLightColor.red += color->red;
                pipeData->ambientLightColor.green += color->green;
                pipeData->ambientLightColor.blue += color->blue;
                pipeData->ambientLight = TRUE;
            }
        }

        cur = rwLLLinkGetNext(cur);
    }
}

void _rwGCLightsLocalEnable(const RpLight* light, RxGameCubePipeData* pipeData)
{
    RwV3d at;
    RwV3d pos;
    GXColor color;
    RwMatrix* matrix;
    const RwRGBAReal* lightColor;
    const _rwDlLightExt* lightExt;

    if (pipeData->numLights < rwDLMAXLIGHTS)
    {
        lightColor = &light->color;

        color.r = (RwUInt8)(255.0f * lightColor->red);
        color.g = (RwUInt8)(255.0f * lightColor->green);
        color.b = (RwUInt8)(255.0f * lightColor->blue);
        color.a = 0;
        GXInitLightColor(&_RwGCLightObjs[pipeData->numLights], color);

        matrix = RwFrameGetLTM(RpLightGetFrame(light));

        RwV3dTransformPoints(&pos, &matrix->pos, 1, &_RwDlInvCamLTM);
        GXInitLightPos(&_RwGCLightObjs[pipeData->numLights], -pos.x, pos.y, -pos.z);

        lightExt = LIGHTEXTFROMLIGHT(light);

        switch (RpLightGetType(light))
        {
        case rpLIGHTPOINT:
        {
            if (lightExt->flags == 0)
            {
                GXInitLightAttnA(&_RwGCLightObjs[pipeData->numLights], 1.0f, 0.0f, 0.0f);
                GXInitLightDistAttn(&_RwGCLightObjs[pipeData->numLights], 0.5f * light->radius,
                                    0.5f, GX_DA_MEDIUM);
            }
            else
            {
                GXInitLightAttn(&_RwGCLightObjs[pipeData->numLights], lightExt->a0, lightExt->a1,
                                lightExt->a2, lightExt->k0, lightExt->k1, lightExt->k2);
            }
            break;
        }
        case rpLIGHTSPOT:
        {
            RwV3dTransformVectors(&at, &matrix->at, 1, &_RwDlInvCamLTM);
            GXInitLightDir(&_RwGCLightObjs[pipeData->numLights], -at.x, at.y, -at.z);

            if (lightExt->flags == 0)
            {
                GXInitLightSpot(&_RwGCLightObjs[pipeData->numLights],
                                57.29578f * RpLightGetConeAngle(light), GX_SP_FLAT);
                GXInitLightDistAttn(&_RwGCLightObjs[pipeData->numLights], 0.5f * light->radius,
                                    0.5f, GX_DA_MEDIUM);
            }
            else
            {
                GXInitLightAttn(&_RwGCLightObjs[pipeData->numLights], lightExt->a0, lightExt->a1,
                                lightExt->a2, lightExt->k0, lightExt->k1, lightExt->k2);
            }
            break;
        }
        case rpLIGHTSPOTSOFT:
        {
            RwV3dTransformVectors(&at, &matrix->at, 1, &_RwDlInvCamLTM);
            GXInitLightDir(&_RwGCLightObjs[pipeData->numLights], -at.x, at.y, -at.z);

            if (lightExt->flags == 0)
            {
                GXInitLightSpot(&_RwGCLightObjs[pipeData->numLights],
                                57.29578f * RpLightGetConeAngle(light), GX_SP_COS);
                GXInitLightDistAttn(&_RwGCLightObjs[pipeData->numLights], 0.5f * light->radius,
                                    0.5f, GX_DA_MEDIUM);
            }
            else
            {
                GXInitLightAttn(&_RwGCLightObjs[pipeData->numLights], lightExt->a0, lightExt->a1,
                                lightExt->a2, lightExt->k0, lightExt->k1, lightExt->k2);
            }
            break;
        }
        case rpNALIGHTTYPE:
        case rpLIGHTDIRECTIONAL:
        case rpLIGHTAMBIENT:
        default:
        {
            break;
        }
        }

        GXLoadLightObjImm(&_RwGCLightObjs[pipeData->numLights],
                          (GXLightID)(1 << pipeData->numLights));

        pipeData->lightMask |= 1 << pipeData->numLights;
        pipeData->numLights++;
    }
}

static void* rwDlLightExtCnst(void* object, RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    LIGHTEXTFROMLIGHT(object)->flags = 0;

    return object;
}

static void* rwDlLightExtDest(void* object, RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    return object;
}

static void* rwDlLightExtCopy(void* dstObject, const void* srcObject, RwInt32 offsetInObject,
                              RwInt32 sizeInObject)
{
    return dstObject;
}

RwBool _rpDlLightPluginAttach(void)
{
    _RwDlLightExtOffset = RpLightRegisterPlugin(sizeof(_rwDlLightExt), rwID_GCNLIGHTPLUGIN,
                                                rwDlLightExtCnst, rwDlLightExtDest,
                                                rwDlLightExtCopy);

    return _RwDlLightExtOffset >= 0;
}
