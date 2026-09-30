#include "zLightning.h"

#include <stdio.h>

#include "xDebug.h"
#include "zGlobals.h"
#include "xstransvc.h"
#include "xParEmitter.h"
#include "iParMgr.h"

#include <types.h>
#include <rwcore.h>

#define NUM_LIGHTNING 48

_tagLightningAdd gLightningTweakAddInfo;

static zLightning* sLightning[NUM_LIGHTNING];
static xFuncPiece sLFuncX[10];
static xFuncPiece sLFuncY[10];
static xFuncPiece sLFuncZ[10];
static xVec3 sLFuncVal[10];
static xVec3 sLFuncSlope[10][2];
static F32 sLFuncEnd[10];
static xVec3 sTweakStart;
static xVec3 sTweakEnd;
static tweak_callback sLightningStartCB;
static tweak_callback sLightningChangeCB;
static xVec3 sPoint[5];
static F32 sSize[5];

const char* lightning_type_names[4] = { "Line", "Rotating", "Zeus", "Func" };
#define LYT_TYPE_LINE 0
#define LYT_TYPE_ROTATING 1
#define LYT_TYPE_ZEUS 2
#define LYT_TYPE_FUNC 3

static zParEmitter* sSparkEmitter;
static RwRaster* sLightningRaster;
static volatile F32 sLFuncJerkTime;
static volatile F32 sLFuncUVOffset;

static F32 sLFuncJerkFreq = 20.0f;
static F32 sLFuncShift = 15.0f;
static F32 sLFuncMaxPStep = 1.0f / 16.0f;
static F32 sLFuncMinPStep = 1.0f / 16.0f;
static F32 sLFuncMinScale = 3.0f / 10.0f;
static F32 sLFuncMaxScale = 1.0f;
static F32 sLFuncScalePerLength = 0.15f;
static F32 sLFuncMinSpan = 3.0f;
static F32 sLFuncSpanPerLength = 1.5f;
static F32 sLFuncSlopeRange = 2.0f;
static F32 sLFuncUVSpeed = 1.0f;

void lightningTweakChangeType(const tweak_info& t)
{
    xDebugRemoveTweak("Lightning|\x01Type Info");

    switch (gLightningTweakAddInfo.type)
    {
    case 0:
        break;

    case 1:
        xDebugAddTweak("Lightning|\x01Type Info|Setup Degrees",
                       &gLightningTweakAddInfo.setup_degrees, -1e9, 1e9, NULL, NULL, 2);

        xDebugAddTweak("Lightning|\x01Type Info|Move Degrees", &gLightningTweakAddInfo.move_degrees,
                       -1e9, 1e9, NULL, NULL, 2);
        break;

    case 2:
        xDebugAddTweak("Lightning|\x01Type Info|Normal Offset",
                       &gLightningTweakAddInfo.zeus_normal_offset, -100.0, 100.0, NULL, NULL, 2);

        xDebugAddTweak("Lightning|\x01Type Info|Back Offset",
                       &gLightningTweakAddInfo.zeus_back_offset, -100.0, 100.0, NULL, NULL, 2);

        xDebugAddTweak("Lightning|\x01Type Info|Side Offset",
                       &gLightningTweakAddInfo.zeus_side_offset, 0.0, 100.0, NULL, NULL, 2);
        break;
    }
}

static void lightningTweakStart(const tweak_info& t)
{
    xVec3 s, e;
    xVec3Add(&s, (xVec3*)&globals.player.ent.model->Mat->pos, &sTweakStart);
    xVec3Add(&e, (xVec3*)&globals.player.ent.model->Mat->pos, &sTweakEnd);
    gLightningTweakAddInfo.start = &s;
    gLightningTweakAddInfo.end = &e;
    zLightningAdd(&gLightningTweakAddInfo);
}

void zLightningInit()
{
    S32 i;

    for (i = 0; i < NUM_LIGHTNING; i++)
    {
        sLightning[i] = NULL;
    }

    zSceneFindObject(xStrHash("PAREMIT_EG_SPARK"));
    RwTexture* tex = (RwTexture*)xSTFindAsset(xStrHash("LIGHTNING"), NULL);
    if (tex != NULL)
    {
        sLightningRaster = tex->raster;
    }

    for (i = 0; i < 9; i++)
    {
        sLFuncX[i].next = &sLFuncX[i + 1];
        sLFuncY[i].next = &sLFuncY[i + 1];
        sLFuncZ[i].next = &sLFuncZ[i + 1];
    }

    sLFuncX[9].next = NULL;
    sLFuncY[9].next = NULL;
    sLFuncZ[9].next = NULL;

    for (i = 0; i < 10; i++) {
        xVec3Init(&sLFuncVal[i], 2.0f * (xurand() - 0.5f), 2.0f * (xurand() - 0.5f), 2.0f * (xurand() - 0.5f));
        xVec3Init(&sLFuncSlope[i][0], sLFuncSlopeRange * (2.0f * (xurand() - 0.5f)), sLFuncSlopeRange * (2.0f * (xurand() - 0.5f)), sLFuncSlopeRange * (2.0f * (xurand() - 0.5f)));
        xVec3Init(&sLFuncSlope[i][1], sLFuncSlopeRange * (2.0f * (xurand() - 0.5f)), sLFuncSlopeRange * (2.0f * (xurand() - 0.5f)), sLFuncSlopeRange * (2.0f * (xurand() - 0.5f)));

        sLFuncEnd[i] = 0.25f * (xurand() - 0.5f) + (i + 1);
    }

    sLFuncEnd[9] = 10.0f;

    for (i = 0; i < 10; i++)
    {
        S32 j;
        F32 prevEnd;
        if (i == 0)
        {
            prevEnd = 0.0f;
            j = 9;
        }
        else
        {
            j = i - 1;
            prevEnd = sLFuncEnd[j];
        }

        xFuncPiece_EndPoints(&sLFuncX[i], prevEnd, sLFuncEnd[i], sLFuncVal[j].x, sLFuncVal[i].x);
        xFuncPiece_EndPoints(&sLFuncY[i], prevEnd, sLFuncEnd[i], sLFuncVal[j].y, sLFuncVal[i].y);
        xFuncPiece_EndPoints(&sLFuncZ[i], prevEnd, sLFuncEnd[i], sLFuncVal[j].z, sLFuncVal[i].z);
    }

    sLFuncJerkTime = 0.0f;

    gLightningTweakAddInfo.type = 0x3;
    gLightningTweakAddInfo.flags = 0x1428;
    gLightningTweakAddInfo.time = 5.0f;
    xVec3Init(&sTweakStart, -5.0f, 6.0f, 0.0f);
    xVec3Init(&sTweakEnd, -5.0f, 2.0f, 0.0f);
    gLightningTweakAddInfo.color.r = 0xC8;
    gLightningTweakAddInfo.color.g = 0xC8;
    gLightningTweakAddInfo.color.b = 0xFF;
    gLightningTweakAddInfo.color.a = 0xC8;
    gLightningTweakAddInfo.arc_height = 0.5f;
    gLightningTweakAddInfo.rot_radius = 0.15f;
    gLightningTweakAddInfo.thickness = 0.25f;
    gLightningTweakAddInfo.rand_radius = 8.0f;
    gLightningTweakAddInfo.move_degrees = -2500.0f;
    gLightningTweakAddInfo.setup_degrees = 66.0f;
    gLightningTweakAddInfo.total_points = 16;
    gLightningTweakAddInfo.zeus_normal_offset = 0.75f;
    gLightningTweakAddInfo.zeus_back_offset = 0.2f;
    gLightningTweakAddInfo.zeus_side_offset = 0.0f;

    sLightningStartCB.on_change = (void (*)(tweak_info&))&lightningTweakStart;
    sLightningChangeCB.on_change = (void (*)(tweak_info&))&lightningTweakChangeType;

    xDebugAddTweak("Lightning|\01\01Go", "Start Lightning", &sLightningStartCB, NULL, 0x2);
    xDebugAddTweak("Lightning|\01Globals|\01\01JerkFrequency", &sLFuncJerkFreq, 0.0f, 1000000000.0f, NULL, NULL, 0x2);
    xDebugAddTweak("Lightning|\01Globals|\01\01ShiftSpeed", &sLFuncShift, 0.0f, 1000000000.0f, NULL, NULL, 0x2);
    xDebugAddTweak("Lightning|\01Globals|\01\02MinPStep", &sLFuncMinPStep, 0.0f, 1000000000.0f, NULL, NULL, 0x2);
    xDebugAddTweak("Lightning|\01Globals|\01\03MaxPStep", &sLFuncMaxPStep, 0.0f, 1000000000.0f, NULL, NULL, 0x2);
    xDebugAddTweak("Lightning|\01Globals|\01\03MinScale", &sLFuncMinScale, 0.0f, 1000000000.0f, NULL, NULL, 0x2);
    xDebugAddTweak("Lightning|\01Globals|\02\01MaxScale", &sLFuncMaxScale, 0.0f, 1000000000.0f, NULL, NULL, 0x2);
    xDebugAddTweak("Lightning|\01Globals|\02\01ScalePerLength", &sLFuncScalePerLength, 0.0f, 1000000000.0f, NULL, NULL, 0x2);
    xDebugAddTweak("Lightning|\01Globals|\02\02MinSpan", &sLFuncMinSpan, 0.0f, 1000000000.0f, NULL, NULL, 0x2);
    xDebugAddTweak("Lightning|\01Globals|\02\02SpanPerLength", &sLFuncSpanPerLength, 0.0f, 1000000000.0f, NULL, NULL, 0x2);
    xDebugAddTweak("Lightning|\01Globals|\02\03SlopeRange", &sLFuncSlopeRange, 0.0f, 1000000000.0f, NULL, NULL, 0x2);
    xDebugAddTweak("Lightning|\01Globals|\02\03UVSpeed", &sLFuncUVSpeed, -1000000000.0f, 1000000000.0f, NULL, NULL, 0x2);
    xDebugAddTweak("Lightning|\01Time", &gLightningTweakAddInfo.time, 0.0f, 1000000000.0f, NULL, NULL, 0x2);
    xDebugAddTweak("Lightning|\01Total Points", &gLightningTweakAddInfo.total_points, 2, 16, NULL, NULL, 0x2);
    
    xDebugAddSelectTweak("Lightning|\01Type", &gLightningTweakAddInfo.type, lightning_type_names, NULL, 4, &sLightningChangeCB, NULL, 0x2);
    
    tweak_info info;
    lightningTweakChangeType(info);

    xDebugAddFlagTweak("Lightning|\02Flag|Rot Scalar", &gLightningTweakAddInfo.flags, 0x8, NULL, NULL, 0x2);
    xDebugAddFlagTweak("Lightning|\02Flag|No Fade Out", &gLightningTweakAddInfo.flags, 0x1000, NULL, NULL, 0x2);
    xDebugAddFlagTweak("Lightning|\02Flag|Arc", &gLightningTweakAddInfo.flags, 0x20, NULL, NULL, 0x2);
    xDebugAddFlagTweak("Lightning|\02Flag|Vertical Orientation", &gLightningTweakAddInfo.flags, 0x200, NULL, NULL, 0x2);
    xDebugAddFlagTweak("Lightning|\02Flag|Taper Thickness At End", &gLightningTweakAddInfo.flags, 0x400, NULL, NULL, 0x2);
    xDebugAddFlagTweak("Lightning|\02Flag|Taper Thickness At Start", &gLightningTweakAddInfo.flags, 0x800, NULL, NULL, 0x2);
    xDebugAddTweak("Lightning|\02Start|x", &sTweakStart.x, -50.0f, 50.0f, NULL, NULL, 0x2);
    xDebugAddTweak("Lightning|\02Start|y", &sTweakStart.y, -50.0f, 50.0f, NULL, NULL, 0x2);
    xDebugAddTweak("Lightning|\02Start|z", &sTweakStart.z, -50.0f, 50.0f, NULL, NULL, 0x2);
    xDebugAddTweak("Lightning|\03End|x", &sTweakEnd.x, -50.0f, 50.0f, NULL, NULL, 0x2);
    xDebugAddTweak("Lightning|\03End|y", &sTweakEnd.y, -50.0f, 50.0f, NULL, NULL, 0x2);
    xDebugAddTweak("Lightning|\03End|z", &sTweakEnd.z, -50.0f, 50.0f, NULL, NULL, 0x2);
    xDebugAddTweak("Lightning|\04Color|\01R", &gLightningTweakAddInfo.color.r, 0x0, 0xFF, NULL, NULL, 0x2);
    xDebugAddTweak("Lightning|\04Color|\02G", &gLightningTweakAddInfo.color.g, 0x0, 0xFF, NULL, NULL, 0x2);
    xDebugAddTweak("Lightning|\04Color|\03B", &gLightningTweakAddInfo.color.b, 0x0, 0xFF, NULL, NULL, 0x2);
    xDebugAddTweak("Lightning|\04Color|\04A", &gLightningTweakAddInfo.color.a, 0x0, 0xFF, NULL, NULL, 0x2);
    xDebugAddTweak("Lightning|Lengths|Rot Radius", &gLightningTweakAddInfo.rot_radius, -1000000000.0f, 1000000000.0f, NULL, NULL, 0x2);
    xDebugAddTweak("Lightning|Lengths|Arc Height", &gLightningTweakAddInfo.arc_height, -1000000000.0f, 1000000000.0f, NULL, NULL, 0x2);
    xDebugAddTweak("Lightning|Lengths|Thickness", &gLightningTweakAddInfo.thickness, 0.0f, 100.0f, NULL, NULL, 0x2);
    xDebugAddTweak("Lightning|Randomness|Rand Radius", &gLightningTweakAddInfo.rand_radius, 0.0f, 1000000000.0f, NULL, NULL, 0x2);
}

static zLightning* FindFreeLightning()
{
    for (int i = 0; i != (sizeof(sLightning) / sizeof(zLightning*)); i++)
    {
        if (sLightning[i] != NULL)
        {
            if (!(sLightning[i]->flags & 1))
            {
                return sLightning[i];
            }
        }
        else
        {
            sLightning[i] = (zLightning*)xMemAlloc(gActiveHeap, 0x234, 0);
            return sLightning[i];
        }
    }

    return 0;
}

zLightning* zLightningAdd(_tagLightningAdd* add)
{
    zLightning* new_lightning;
    if (!(new_lightning = FindFreeLightning()))
    {
        return NULL;
    }

    new_lightning->type = add->type;
    new_lightning->flags = add->flags | 0x41;
    new_lightning->color = add->color;

    if (new_lightning->type != LYT_TYPE_FUNC)
    {
        new_lightning->legacy.total_points = add->total_points;
        new_lightning->legacy.end_points = add->end_points;        
        new_lightning->legacy.arc_height = add->arc_height;
        new_lightning->legacy.rand_radius = add->rand_radius;
        
        F32 currot = 0.0f;
        S32 zeusOnStraightPoint = TRUE;

        switch (new_lightning->type)
        {
        case LYT_TYPE_LINE:
            break;
        case LYT_TYPE_ROTATING:
            new_lightning->legacy.rot.degrees = add->move_degrees;
            new_lightning->legacy.rot.height = add->rot_radius;
            break;
        case LYT_TYPE_ZEUS:
            new_lightning->legacy.zeus.normal_offset = add->zeus_normal_offset;
            new_lightning->legacy.zeus.back_offset = add->zeus_back_offset;
            new_lightning->legacy.zeus.side_offset = add->zeus_side_offset;
            break;
        }

        xVec3 last_point;
        xVec3 dir;
        if (add->flags & 0x80)
        {
            if (add->end_points - 1 < 0)
            {
                xVec3Sub(&dir, &add->start[add->total_points - 1], add->start);
            }
            else
            {
                xVec3Sub(&dir, &add->end[add->end_points - 1], add->start);
            }
        }
        else
        {
            xVec3Sub(&dir, add->end, add->start);
        }

        xVec3Normalize(&dir, &dir);

        if (dir.y > 0.999f || dir.y < -0.999f)
        {
            xVec3Init(&new_lightning->legacy.arc_normal, 1.0f, 0.0f, 0.0f);
        }
        else
        {
            new_lightning->legacy.arc_normal.x = -(dir.x * dir.y);
            new_lightning->legacy.arc_normal.y = dir.z * dir.z + dir.x * dir.x;
            new_lightning->legacy.arc_normal.z = -(dir.z * dir.y);
            xVec3Normalize(&new_lightning->legacy.arc_normal, &new_lightning->legacy.arc_normal);
        }

        xVec3 arc_orthogonal;
        xVec3Cross(&arc_orthogonal, &new_lightning->legacy.arc_normal, &dir);

        F32 pos = 0.0f;
        F32 inc = 1.0f / (new_lightning->legacy.total_points - 1.0f);

        S32 i;
        for (i = 0; i < new_lightning->legacy.total_points; i++)
        {

            new_lightning->legacy.thickness[i] = add->thickness * 0.5f;

            if (add->flags & 0x400)
            {
                new_lightning->legacy.thickness[i] *= 1.0f - pos;
            }

            if (add->flags & 0x800)
            {
                new_lightning->legacy.thickness[i] *= pos;
            }

            if ((add->flags & 0x400) && (add->flags & 0x800))
            {
                new_lightning->legacy.thickness[i] *= 4.0f;
            }

            if (add->flags & 0x80)
            {
                S32 j = i - (add->total_points - add->end_points);
                if (j < 0)
                {
                    new_lightning->legacy.point[i] = add->start[i];
                }
                else
                {
                    new_lightning->legacy.point[i] = add->end[j];
                }
            }
            else
            {
                xVec3Lerp(&new_lightning->legacy.point[i], add->start, add->end, pos);
            }

            switch (new_lightning->type)
            {
            case LYT_TYPE_LINE:
                break;
            case LYT_TYPE_ROTATING:
                new_lightning->legacy.rot.deg[i] = currot;

                while (new_lightning->legacy.rot.deg[i] > 180.0f)
                {
                    new_lightning->legacy.rot.deg[i] -= 360.0f;
                }

                while (new_lightning->legacy.rot.deg[i] < -180.0f)
                {
                    new_lightning->legacy.rot.deg[i] += 360.0f;
                }

                currot += add->setup_degrees;
                break;
            case LYT_TYPE_ZEUS:
                if (i == 0 || i == new_lightning->legacy.total_points - 1)
                {
                    break;
                }

                if (zeusOnStraightPoint)
                {
                    xVec3Copy(&last_point, &new_lightning->legacy.point[i]);
                    zeusOnStraightPoint = FALSE;
                }
                else
                {
                    xVec3Copy(&new_lightning->legacy.point[i], &last_point);
                    xVec3AddScaled(&new_lightning->legacy.point[i], &new_lightning->legacy.arc_normal, new_lightning->legacy.zeus.normal_offset);
                    xVec3AddScaled(&new_lightning->legacy.point[i], &dir, -new_lightning->legacy.zeus.back_offset);
                    xVec3AddScaled(&new_lightning->legacy.point[i], &arc_orthogonal, -new_lightning->legacy.zeus.side_offset);

                    zeusOnStraightPoint = TRUE;
                }

                break;
            }

            if (new_lightning->flags & 0x20)
            {
                F32 scalar = 4.0f * pos + pos * pos * -4.0f;
                if (scalar > 0.0f)
                {
                    xVec3AddScaled(&new_lightning->legacy.base_point[i], &new_lightning->legacy.arc_normal, scalar * new_lightning->legacy.arc_height);
                }
            }

            new_lightning->legacy.base_point[i] = new_lightning->legacy.point[i];

            pos += inc;
        }
    }
    else
    {
        zLightningModifyEndpoints(new_lightning, add->start, add->end);

        new_lightning->func.endParam[0] = 10.0f * xurand();
        new_lightning->func.endParam[1] = 10.0f * xurand();
        new_lightning->func.endVel[0] = -1.0f;
        new_lightning->func.endVel[1] = 1.0f;
        new_lightning->func.width = add->thickness;
        new_lightning->func.arc_height = add->arc_height;
    }

    if (new_lightning->flags & 0x10)
    {
        new_lightning->time_left = new_lightning->time_total = 0.1f;
    }
    else
    {
        new_lightning->time_left = new_lightning->time_total = add->time;
    }

    return new_lightning;
}

static void UpdateLightning(zLightning* l, F32 seconds)
{
    if (!(l->flags & 0x10))
    {
        l->time_left -= seconds;
    }

    if (l->time_left <= 0.0f)
    {
        if (l->flags & 0x100)
        {
            l->time_left = 0.0f;
            l->flags &= ~0x40;
        }
        else
        {
            l->flags &= ~0x1;
        }
    }

    if (l->type != LYT_TYPE_FUNC)
    {
        if (l->type == LYT_TYPE_LINE || l->type == LYT_TYPE_ZEUS)
        {
            S32 i;
            F32 full = l->legacy.rand_radius * seconds;
            F32 half = 0.5f * full;

            for (i = 1; i < l->legacy.total_points - 1; i++)
            {
                l->legacy.point[i].x = l->legacy.base_point[i].x + (full * xurand() + -half);
                l->legacy.point[i].y = l->legacy.base_point[i].y + (full * xurand() + -half);
                l->legacy.point[i].z = l->legacy.base_point[i].z + (full * xurand() + -half);

                if (l->flags & 0x20)
                {
                    F32 sc1 = (F32)i / (F32)l->legacy.total_points;
                    F32 sc2 = 4.0f * sc1 + -4.0f * (sc1 * sc1);

                    xVec3AddScaled(&l->legacy.point[i], &l->legacy.arc_normal,
                                   sc2 * l->legacy.arc_height);
                }
            }
        }
        else if (l->type == LYT_TYPE_ROTATING)
        {
            xVec3 dir;
            xVec3Sub(&dir, &l->legacy.base_point[l->legacy.total_points - 1],
                     &l->legacy.base_point[0]);
            xVec3Normalize(&dir, &dir);

            F32 full = l->legacy.rand_radius * seconds;
            F32 half = 0.5f * full;

            for (S32 i = 1; i < l->legacy.total_points - 1; i++)
            {
                xMat3x3 mat3;
                xMat3x3Rot(&mat3, &dir, PI * l->legacy.rot.deg[i] / 180.0f);

                xVec3 vec;
                xVec3Copy(&vec, &l->legacy.arc_normal);

                F32 sc2 = 1.0f;
                if (l->flags & 0x28)
                {
                    F32 sc1 = (F32)i / (F32)l->legacy.total_points;
                    sc2 = 4.0f * sc1 + -4.0f * (sc1 * sc1);

                    if (l->flags & 0x8)
                    {
                        xVec3SMulBy(&vec, l->legacy.rot.height * sc2);
                    }
                }

                xMat3x3LMulVec(&vec, &mat3, &vec);

                l->legacy.rot.deg[i] += l->legacy.rot.degrees * seconds;

                if (l->legacy.rot.deg[i] > 180.0f)
                {
                    l->legacy.rot.deg[i] -= 360.0f;
                }
                else if (l->legacy.rot.deg[i] < -180.0f)
                {
                    l->legacy.rot.deg[i] += 360.0f;
                }

                l->legacy.point[i].x = l->legacy.base_point[i].x + (full * xurand() + -half);
                l->legacy.point[i].y = l->legacy.base_point[i].y + (full * xurand() + -half);
                l->legacy.point[i].z = l->legacy.base_point[i].z + (full * xurand() + -half);

                xVec3AddTo(&l->legacy.point[i], &vec);

                if (l->flags & 0x20)
                {
                    xVec3AddScaled(&l->legacy.point[i], &l->legacy.arc_normal,
                                   sc2 * l->legacy.arc_height);
                }
            }
        }

        if ((l->flags & 0x2) && sSparkEmitter != NULL && (xrand() & 0x3))
        {
            xParEmitterCustomSettings info;
            info.custom_flags = 0xD00;

            S32 i = xrand() % l->legacy.total_points;
            info.pos = l->legacy.point[i];
            xrand();

            xParEmitterEmitCustom(sSparkEmitter, seconds, &info);
        }
    }
    else
    {
        l->func.endParam[0] += seconds * (l->func.endVel[0] * sLFuncShift);
        while (l->func.endParam[0] > sLFuncEnd[9])
        {
            l->func.endParam[0] -= 10.0f;
        }

        while (l->func.endParam[0] < 0.0f)
        {
            l->func.endParam[0] += 10.0f;
        }

        l->func.endParam[1] += seconds * (l->func.endVel[1] * sLFuncShift);
        while (l->func.endParam[1] > sLFuncEnd[9])
        {
            l->func.endParam[1] -= 10.0f;
        }

        while (l->func.endParam[1] < 0.0f)
        {
            l->func.endParam[1] += 10.0f;
        }
    }
}


void zLightningUpdate(F32 seconds)
{
    S32 i;
    for (i = 0; i < NUM_LIGHTNING; i++)
    {
        if (sLightning[i] != NULL && sLightning[i]->flags & 0x1)
        {
            UpdateLightning(sLightning[i], seconds);
        }
    }

    sLFuncUVOffset += sLFuncUVSpeed * seconds;

    F32 uvOffset = sLFuncUVOffset;
    if (uvOffset > 1.0f)
    {
        sLFuncUVOffset = uvOffset - 1.0f;
    }

    sLFuncJerkTime = sLFuncJerkFreq * seconds + sLFuncJerkTime;
    if (!(sLFuncJerkTime > 1.0f))
    {
        return;
    }

    S32 picker = 9.0f * xurand();
    if (picker >= 9)
    {
        picker = 8;
    }
    if (picker < 0)
    {
        picker = 0;
    }

    xVec3Init(&sLFuncVal[picker], 2.0f * (xurand() - 0.5f), 2.0f * (xurand() - 0.5f), 2.0f * (xurand() - 0.5f));
    xVec3Init(&sLFuncSlope[picker][0], 4.0f * (xurand() - 0.5f), 4.0f * (xurand() - 0.5f), 4.0f * (xurand() - 0.5f));
    xVec3Init(&sLFuncSlope[picker][1], 4.0f * (xurand() - 0.5f), 4.0f * (xurand() - 0.5f), 4.0f * (xurand() - 0.5f));

    sLFuncEnd[picker] = 0.25f * (xurand() - 0.5f) + (picker + 1);

    for (i = picker; i <= picker + 1; i++)
    {
        S32 k;
        F32 prevEnd;
        if (i == 0)
        {
            k = 9;
            prevEnd = 0.0f;
        }
        else
        {
            k = i - 1;
            prevEnd = sLFuncEnd[k];
        }

        xFuncPiece_EndPoints(&sLFuncX[i], prevEnd, sLFuncEnd[i], sLFuncVal[k].x, sLFuncVal[i].x);
        xFuncPiece_EndPoints(&sLFuncY[i], prevEnd, sLFuncEnd[i], sLFuncVal[k].y, sLFuncVal[i].y);
        xFuncPiece_EndPoints(&sLFuncZ[i], prevEnd, sLFuncEnd[i], sLFuncVal[k].z, sLFuncVal[i].z);
    }

    sLFuncJerkTime = 0.0f;
}

void zLightningFunc_Render(zLightning* l)
{
    F32 t = 0.0f;
    F32 pstep = 1.0f;
    xVec3 funcVal[2];
    xVec3 side[2];
    xVec3 lastPos;
    xVec3 pos;
    xVec3 vpos;
    F32 param[2];
    xFuncPiece* pieceX[2];
    xFuncPiece* pieceY[2];
    xFuncPiece* pieceZ[2];
    RwIm3DVertex* vert[2];

    if (l->func.length > 0.00001f)
    {
        pstep = 1.0f / (10.0f * l->func.length);
    }

    if (pstep > sLFuncMaxPStep)
    {
        pstep = sLFuncMaxPStep;
    }

    if (pstep < sLFuncMinPStep)
    {
        pstep = sLFuncMinPStep;
    }

    for (S32 i = 0; i < 2; i++)
    {
        param[i] = l->func.endParam[i];
        pieceX[i] = sLFuncX;
        pieceY[i] = sLFuncY;
        pieceZ[i] = sLFuncZ;
    }

    vert[0] = gRenderArr.m_vertex;
    vert[1] = vert[0] + 240;

    if (l->func.length > 0.00001f)
    {
        side[0].x = l->func.direction.y - l->func.direction.z;
        side[0].y = l->func.direction.z - l->func.direction.x;
        side[0].z = l->func.direction.x - l->func.direction.y;

        xVec3Normalize(&side[0], &side[0]);
        xVec3Cross(&side[1], &side[0], &l->func.direction);
    }
    else
    {
        xVec3Init(&side[0], 1.0f, 0.0f, 0.0f);
        xVec3Init(&side[1], 0.0f, 0.0f, 1.0f);
    }

    xVec3Copy(&lastPos, &l->func.endPoint[0]);

    U8 alpha = xrand();
    S32 tex = 0;
    U32 nvert = 0;

    for (S32 i = 0; i < 2; i++)
    {
        xVec3Copy(&vpos, &lastPos);

        F32 vx = vpos.x;
        F32 vy = vpos.y;
        F32 vz = vpos.z;
        RwIm3DVertexSetPos(&vert[i][nvert], vx, vy, vz);
        U8 cr = l->color.r;
        U8 cg = l->color.g;
        U8 cb = l->color.b;
        RwIm3DVertexSetRGBA(&vert[i][nvert], cr, cg, cb, alpha);
        RwIm3DVertexSetUV(&vert[i][nvert], tex + sLFuncUVOffset, 0.0f);

        vx = vpos.x;
        vy = vpos.y;
        vz = vpos.z;
        RwIm3DVertexSetPos(&vert[i][nvert + 1], vx, vy, vz);
        cr = l->color.r;
        cg = l->color.g;
        cb = l->color.b;
        RwIm3DVertexSetRGBA(&vert[i][nvert + 1], cr, cg, cb, alpha);
        RwIm3DVertexSetUV(&vert[i][nvert + 1], tex + sLFuncUVOffset, 1.0f);
    }

    nvert += 2;
    tex = 1;

    while (t < 1.0f)
    {
        t += pstep;
        if (t > 1.0f)
        {
            t = 1.0f;
        }

        xVec3SMul(&pos, &l->func.endPoint[0], 1.0f - t);
        xVec3AddScaled(&pos, &l->func.endPoint[1], t);

        for (S32 i = 0; i < 2; i++)
        {
            F32 p = l->func.endParam[i] + t * l->func.paramSpan[i];

            if (p >= sLFuncEnd[9])
            {
                p -= 10 * (S32)(p / 10.0f);

                if (p < param[i])
                {
                    pieceX[i] = sLFuncX;
                    pieceY[i] = sLFuncY;
                    pieceZ[i] = sLFuncZ;
                }
            }

            param[i] = p;

            funcVal[i].x = xFuncPiece_Eval(pieceX[i], param[i], &pieceX[i]);
            funcVal[i].y = xFuncPiece_Eval(pieceY[i], param[i], &pieceY[i]);
            funcVal[i].z = xFuncPiece_Eval(pieceZ[i], param[i], &pieceZ[i]);
        }

        F32 t2 = t * t;
        F32 t3 = t2 * t;
        F32 scalar1 = 4.0f * (t + (t3 - 2.0f * t2));
        F32 scalar2 = 4.0f * (-t3 + t2);

        xVec3AddScaled(&pos, &funcVal[0], scalar1 * l->func.scale);
        xVec3AddScaled(&pos, &funcVal[1], scalar2 * l->func.scale);

        if (l->flags & 0x20)
        {
            F32 arc = 4.0f * (t - t2);
            if (arc > 0.0f)
            {
                xVec3AddScaled(&pos, &l->func.arc_normal, arc * l->func.arc_height);
            }
        }

        alpha = xrand();

        for (S32 i = 0; i < 2; i++)
        {
            xVec3Copy(&vpos, &pos);
            xVec3AddScaled(&vpos, &side[i],
                           l->func.width * (l->func.scale * (scalar1 + scalar2)));

            F32 vx = vpos.x;
            F32 vy = vpos.y;
            F32 vz = vpos.z;
            RwIm3DVertexSetPos(&vert[i][nvert], vx, vy, vz);
            U8 cr = l->color.r;
            U8 cg = l->color.g;
            U8 cb = l->color.b;
            RwIm3DVertexSetRGBA(&vert[i][nvert], cr, cg, cb, alpha);
            RwIm3DVertexSetUV(&vert[i][nvert], tex + sLFuncUVOffset, 0.0f);

            xVec3AddScaled(&vpos, &side[i],
                           (scalar1 + scalar2) * (-2.0f * l->func.scale * l->func.width));

            vx = vpos.x;
            vy = vpos.y;
            vz = vpos.z;
            RwIm3DVertexSetPos(&vert[i][nvert + 1], vx, vy, vz);
            cr = l->color.r;
            cg = l->color.g;
            cb = l->color.b;
            RwIm3DVertexSetRGBA(&vert[i][nvert + 1], cr, cg, cb, alpha);
            RwIm3DVertexSetUV(&vert[i][nvert + 1], tex + sLFuncUVOffset, 1.0f);
        }

        tex = 1 - tex;
        nvert += 2;

        xVec3Copy(&lastPos, &pos);
    }

    RwIm3DTransform(vert[0], nvert, (RwMatrix*)&g_I3,
                    rwIM3D_VERTEXUV | rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA);
    RwIm3DRenderPrimitive(rwPRIMTYPETRISTRIP);
    RwIm3DEnd();

    RwIm3DTransform(vert[1], nvert, (RwMatrix*)&g_I3,
                    rwIM3D_VERTEXUV | rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA);
    RwIm3DRenderPrimitive(rwPRIMTYPETRISTRIP);
    RwIm3DEnd();
}

void RenderLightning(zLightning* l)
{
    static RwIm3DVertex sStripVert[128];

    xMat4x3* cam = &globals.camera.mat;
    F32 fade;
    U8 alpha;
    S32 i;
    U32 nvert;
    U8 cr;
    U8 cg;
    U8 cb;
    S32 last;

    xVec3 up;
    xVec3 dir;
    xVec3 lastdir;
    xVec3 tmp;
    xVec3 pt1;
    xVec3 pt2;

    if (l->type != LYT_TYPE_FUNC)
    {
        if (l->flags & 0x1000)
        {
            fade = l->color.a;
        }
        else
        {
            fade = l->time_left / l->time_total;
            fade *= l->color.a;
        }

        alpha = 0.5f + fade;
        nvert = 2;

        if (l->flags & 0x200)
        {
            last = l->legacy.total_points;
            xVec3Init(&up, 0.0f, 1.0f, 0.0f);
        }
        else
        {
            last = l->legacy.total_points - 1;
            xVec3Copy(&up, &cam->at);
            xVec3Sub(&tmp, &l->legacy.point[1], &l->legacy.point[0]);
            xVec3AddScaled(&tmp, &cam->at,
                           -xVec3Dot(&tmp, &cam->at));
            if (xVec3Normalize(&dir, &tmp) > 0.00001f)
            {
                xVec3Cross(&up, &dir, &cam->at);
            }
        }

        xVec3Copy(&pt1, &l->legacy.point[0]);
        xVec3AddScaled(&pt1, &up, l->legacy.thickness[0]);
        xVec3Copy(&pt2, &l->legacy.point[0]);
        xVec3AddScaled(&pt2, &up, -l->legacy.thickness[0]);

        RwIm3DVertexSetPos(&sStripVert[0], pt1.x, pt1.y, pt1.z);
        RwIm3DVertexSetUV(&sStripVert[0], 0.0f, 0.0f);
        cr = l->color.r;
        cg = l->color.g;
        cb = l->color.b;
        RwIm3DVertexSetRGBA(&sStripVert[0], cr, cg, cb, alpha);
        RwIm3DVertexSetPos(&sStripVert[1], pt2.x, pt2.y, pt2.z);
        RwIm3DVertexSetUV(&sStripVert[1], 0.0f, 1.0f);
        RwIm3DVertexSetRGBA(&sStripVert[1], cr, cg, cb, alpha);

        for (i = 1; i < last; i++)
        {
            if (!(l->flags & 0x200))
            {
                xVec3Copy(&lastdir, &dir);
                xVec3Sub(&tmp, &l->legacy.point[i + 1], &l->legacy.point[i]);
                xVec3AddScaled(&tmp, &cam->at,
                               -xVec3Dot(&tmp, &cam->at));
                if (xVec3Normalize(&dir, &tmp) > 0.00001f)
                {
                    xVec3Cross(&up, &dir, &cam->at);
                }
            }
            else
            {
                lastdir = dir = up;
            }

            xVec3Copy(&pt1, &l->legacy.point[i]);
            xVec3AddScaled(&pt1, &up, l->legacy.thickness[i]);
            xVec3Copy(&pt2, &l->legacy.point[i]);
            xVec3AddScaled(&pt2, &up, -l->legacy.thickness[i]);

            S32 flip = xVec3Dot(&lastdir, &dir) < 0.0f;

            if (flip)
            {
                RwIm3DVertexSetPos(&sStripVert[nvert], pt2.x, pt2.y, pt2.z);
            }
            else
            {
                RwIm3DVertexSetPos(&sStripVert[nvert], pt1.x, pt1.y, pt1.z);
            }

            if (i & 1)
            {
                sStripVert[nvert].u = 1.0f;
            }
            else
            {
                sStripVert[nvert].u = 0.0f;
            }

            sStripVert[nvert].v = 0.0f;
            cr = l->color.r;
            cg = l->color.g;
            cb = l->color.b;
            RwIm3DVertexSetRGBA(&sStripVert[nvert], cr, cg, cb, alpha);

            nvert++;

            if (flip)
            {
                RwIm3DVertexSetPos(&sStripVert[nvert], pt1.x, pt1.y, pt1.z);
            }
            else
            {
                RwIm3DVertexSetPos(&sStripVert[nvert], pt2.x, pt2.y, pt2.z);
            }

            if (i & 1)
            {
                sStripVert[nvert].u = 1.0f;
            }
            else
            {
                sStripVert[nvert].u = 0.0f;
            }

            sStripVert[nvert].v = 1.0f;
            cr = l->color.r;
            cg = l->color.g;
            cb = l->color.b;
            RwIm3DVertexSetRGBA(&sStripVert[nvert], cr, cg, cb, alpha);

            nvert++;
            if (nvert >= 128)
            {
                nvert = 128;
                goto render;
            }
        }

        if (!(l->flags & 0x200))
        {
            xVec3Copy(&pt1, &l->legacy.point[last]);
            xVec3AddScaled(&pt1, &up, l->legacy.thickness[last]);
            xVec3Copy(&pt2, &l->legacy.point[last]);
            xVec3AddScaled(&pt2, &up, -l->legacy.thickness[last]);

            RwIm3DVertexSetPos(&sStripVert[nvert], pt1.x, pt1.y, pt1.z);

            if (i & 1)
            {
                sStripVert[nvert].u = 1.0f;
            }
            else
            {
                sStripVert[nvert].u = 0.0f;
            }

            sStripVert[nvert].v = 0.0f;
            cr = l->color.r;
            cg = l->color.g;
            cb = l->color.b;
            RwIm3DVertexSetRGBA(&sStripVert[nvert], cr, cg, cb, alpha);

            nvert++;

            RwIm3DVertexSetPos(&sStripVert[nvert], pt2.x, pt2.y, pt2.z);

            if (i & 1)
            {
                sStripVert[nvert].u = 1.0f;
            }
            else
            {
                sStripVert[nvert].u = 0.0f;
            }

            sStripVert[nvert].v = 1.0f;
            cr = l->color.r;
            cg = l->color.g;
            cb = l->color.b;
            RwIm3DVertexSetRGBA(&sStripVert[nvert], cr, cg, cb, alpha);

            nvert++;
        }

    render:
        if (RwIm3DTransform(sStripVert, nvert, NULL,
                            rwIM3D_VERTEXUV | rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA))
        {
            RwIm3DRenderPrimitive(rwPRIMTYPETRISTRIP);
            RwIm3DEnd();
        }

        fade = l->time_left / l->time_total;
        fade *= 0.5f * l->color.a;
        alpha = 0.5f + fade;

        F32 width = 1.5f + xurand();
        nvert = 2;

        if (!(l->flags & 0x200))
        {
            xVec3Copy(&up, &cam->at);
            xVec3Sub(&tmp, &l->legacy.point[1], &l->legacy.point[0]);
            xVec3AddScaled(&tmp, &cam->at,
                           -xVec3Dot(&tmp, &cam->at));
            if (xVec3Normalize(&dir, &tmp) > 0.00001f)
            {
                xVec3Cross(&up, &dir, &cam->at);
            }
        }

        xVec3Copy(&pt1, &l->legacy.point[0]);
        xVec3AddScaled(&pt1, &up, width * l->legacy.thickness[0]);
        xVec3Copy(&pt2, &l->legacy.point[0]);
        xVec3AddScaled(&pt2, &up, -width * l->legacy.thickness[0]);

        RwIm3DVertexSetPos(&sStripVert[0], pt1.x, pt1.y, pt1.z);
        RwIm3DVertexSetUV(&sStripVert[0], 0.0f, 0.0f);
        cr = l->color.r;
        cg = l->color.g;
        cb = l->color.b;
        RwIm3DVertexSetRGBA(&sStripVert[0], cr, cg, cb, alpha);
        RwIm3DVertexSetPos(&sStripVert[1], pt2.x, pt2.y, pt2.z);
        RwIm3DVertexSetUV(&sStripVert[1], 0.0f, 1.0f);
        RwIm3DVertexSetRGBA(&sStripVert[1], cr, cg, cb, alpha);

        for (i = 1; i < last; i++)
        {
            if (!(l->flags & 0x200))
            {
                xVec3Copy(&lastdir, &dir);
                xVec3Sub(&tmp, &l->legacy.point[i + 1], &l->legacy.point[i]);
                xVec3AddScaled(&tmp, &cam->at,
                               -xVec3Dot(&tmp, &cam->at));
                if (xVec3Normalize(&dir, &tmp) > 0.00001f)
                {
                    xVec3Cross(&up, &dir, &cam->at);
                }
            }

            xVec3Copy(&pt1, &l->legacy.point[i]);
            xVec3AddScaled(&pt1, &up, width * l->legacy.thickness[i]);
            xVec3Copy(&pt2, &l->legacy.point[i]);
            xVec3AddScaled(&pt2, &up, -width * l->legacy.thickness[i]);

            S32 flip = xVec3Dot(&lastdir, &dir) < 0.0f;

            if (flip)
            {
                RwIm3DVertexSetPos(&sStripVert[nvert], pt2.x, pt2.y, pt2.z);
            }
            else
            {
                RwIm3DVertexSetPos(&sStripVert[nvert], pt1.x, pt1.y, pt1.z);
            }

            if (i & 1)
            {
                sStripVert[nvert].u = 1.0f;
            }
            else
            {
                sStripVert[nvert].u = 0.0f;
            }

            sStripVert[nvert].v = 0.0f;
            cr = l->color.r;
            cg = l->color.g;
            cb = l->color.b;
            RwIm3DVertexSetRGBA(&sStripVert[nvert], cr, cg, cb, alpha);

            nvert++;

            if (flip)
            {
                RwIm3DVertexSetPos(&sStripVert[nvert], pt1.x, pt1.y, pt1.z);
            }
            else
            {
                RwIm3DVertexSetPos(&sStripVert[nvert], pt2.x, pt2.y, pt2.z);
            }

            if (i & 1)
            {
                sStripVert[nvert].u = 1.0f;
            }
            else
            {
                sStripVert[nvert].u = 0.0f;
            }

            sStripVert[nvert].v = 1.0f;
            cr = l->color.r;
            cg = l->color.g;
            cb = l->color.b;
            RwIm3DVertexSetRGBA(&sStripVert[nvert], cr, cg, cb, alpha);

            nvert++;
            if (nvert >= 128)
            {
                nvert = 128;
                goto render;
            }
        }

        if (!(l->flags & 0x200))
        {
            xVec3Copy(&pt1, &l->legacy.point[last]);
            xVec3AddScaled(&pt1, &up, width * l->legacy.thickness[last]);
            xVec3Copy(&pt2, &l->legacy.point[last]);
            xVec3AddScaled(&pt2, &up, -width * l->legacy.thickness[last]);

            RwIm3DVertexSetPos(&sStripVert[nvert], pt1.x, pt1.y, pt1.z);

            if (i & 1)
            {
                sStripVert[nvert].u = 1.0f;
            }
            else
            {
                sStripVert[nvert].u = 0.0f;
            }

            sStripVert[nvert].v = 0.0f;
            cr = l->color.r;
            cg = l->color.g;
            cb = l->color.b;
            RwIm3DVertexSetRGBA(&sStripVert[nvert], cr, cg, cb, alpha);

            nvert++;

            RwIm3DVertexSetPos(&sStripVert[nvert], pt2.x, pt2.y, pt2.z);

            if (i & 1)
            {
                sStripVert[nvert].u = 1.0f;
            }
            else
            {
                sStripVert[nvert].u = 0.0f;
            }

            sStripVert[nvert].v = 1.0f;
            cr = l->color.r;
            cg = l->color.g;
            cb = l->color.b;
            RwIm3DVertexSetRGBA(&sStripVert[nvert], cr, cg, cb, alpha);

            nvert++;
        }

        if (RwIm3DTransform(sStripVert, nvert, NULL,
                            rwIM3D_VERTEXUV | rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA))
        {
            RwIm3DRenderPrimitive(rwPRIMTYPETRISTRIP);
            RwIm3DEnd();
        }
    }
    else
    {
        zLightningFunc_Render(l);
    }
}

void zLightningRender()
{
    if (sLightningRaster != NULL)
    {
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, sLightningRaster);
    }
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
    for (S32 i = 0; i < (S32)(sizeof(sLightning) / sizeof(zLightning*)); i++)
    {
        if (sLightning[i] != NULL && (sLightning[i]->flags & 0x41) == 0x41)
        {
            RenderLightning(sLightning[i]);
        }
    }
}

void zLightningShow(zLightning* l, S32 show)
{
    if (show)
    {
        l->flags |= 0x40;
    }
    else
    {
        l->flags &= 0xffffffbf;
    }
}

void zLightningKill(zLightning* l)
{
    l->flags &= 0xfffffefe;
}

void zLightningModifyEndpoints(zLightning* l, xVec3* start, xVec3* end)
{
    xVec3 dir;
    xVec3 side;
    xVec3 hold;

    if (l->type != LYT_TYPE_FUNC)
    {
        if (l->flags & 0x80)
        {
            S32 last = l->legacy.end_points - 1;

            if (last < 0)
            {
                xVec3Sub(&dir, &start[l->legacy.total_points - 1], start);
            }
            else
            {
                xVec3Sub(&dir, &end[last], start);
            }
        }
        else
        {
            xVec3Sub(&dir, end, start);
        }

        xVec3Normalize(&dir, &dir);

        if (dir.y > 0.999f || dir.y < -0.999f)
        {
            xVec3Init(&l->legacy.arc_normal, 1.0f, 0.0f, 0.0f);
        }
        else
        {
            l->legacy.arc_normal.x = -(dir.x * dir.y);
            l->legacy.arc_normal.y = dir.z * dir.z + dir.x * dir.x;
            l->legacy.arc_normal.z = -(dir.z * dir.y);

            xVec3Normalize(&l->legacy.arc_normal, &l->legacy.arc_normal);
        }

        xVec3Cross(&side, &l->legacy.arc_normal, &dir);

        S32 i;
        S32 zeusOnStraightPoint = 1;
        F32 pos = 0.0f;
        F32 inc = 1.0f / (l->legacy.total_points - 1.0f);

        for (i = 0; i < l->legacy.total_points; i++)
        {
            if (l->flags & 0x80)
            {
                S32 j = i - (l->legacy.total_points - l->legacy.end_points);

                if (j < 0)
                {
                    l->legacy.base_point[i] = start[i];
                }
                else
                {
                    l->legacy.base_point[i] = end[j];
                }
            }
            else
            {
                xVec3Lerp(&l->legacy.base_point[i], start, end, pos);
            }

            if (l->type == LYT_TYPE_ZEUS && i != 0 && i != l->legacy.total_points - 1)
            {
                if (zeusOnStraightPoint)
                {
                    xVec3Copy(&hold, &l->legacy.base_point[i]);
                    zeusOnStraightPoint = 0;
                }
                else
                {
                    xVec3Copy(&l->legacy.base_point[i], &hold);
                    xVec3AddScaled(&l->legacy.base_point[i], &l->legacy.arc_normal,
                                   l->legacy.zeus.normal_offset);
                    xVec3AddScaled(&l->legacy.base_point[i], &dir, -l->legacy.zeus.back_offset);
                    xVec3AddScaled(&l->legacy.base_point[i], &side, -l->legacy.zeus.side_offset);
                    zeusOnStraightPoint = 1;
                }
            }

            if (l->flags & 0x20)
            {
                F32 arc = -4.0f * (pos * pos) + 4.0f * pos;

                if (arc > 0.0f)
                {
                    xVec3AddScaled(&l->legacy.base_point[i], &l->legacy.arc_normal,
                                   arc * l->legacy.arc_height);
                }
            }

            pos += inc;
        }

        S32 lastPoint = l->legacy.total_points - 1;

        l->legacy.point[0] = l->legacy.base_point[0];
        l->legacy.point[lastPoint] = l->legacy.base_point[lastPoint];
    }
    else
    {
        xVec3Copy(&l->func.endPoint[0], start);
        xVec3Copy(&l->func.endPoint[1], end);

        xVec3Sub(&l->func.direction, &l->func.endPoint[1], &l->func.endPoint[0]);

        l->func.length = xVec3Length(&l->func.direction);

        if (l->func.length > 0.00001f)
        {
            xVec3SMulBy(&l->func.direction, 1.0f / l->func.length);
        }
        else
        {
            xVec3Init(&l->func.direction, 0.0f, 0.0f, 0.0f);
        }

        l->func.scale = l->func.length * sLFuncScalePerLength;

        if (l->func.scale < sLFuncMinScale)
        {
            l->func.scale = sLFuncMinScale;
        }

        if (l->func.scale > sLFuncMaxScale)
        {
            l->func.scale = sLFuncMaxScale;
        }

        l->func.paramSpan[0] = l->func.length * sLFuncSpanPerLength;

        if (l->func.paramSpan[0] < sLFuncMinSpan)
        {
            l->func.paramSpan[0] = sLFuncMinSpan;
        }

        l->func.paramSpan[1] = l->func.paramSpan[0];

        if (l->func.direction.y > 0.999f || l->func.direction.y < -0.999f)
        {
            xVec3Init(&l->func.arc_normal, 1.0f, 0.0f, 0.0f);
        }
        else
        {
            l->func.arc_normal.x = -(l->func.direction.x * l->func.direction.y);
            l->func.arc_normal.y =
                l->func.direction.z * l->func.direction.z + l->func.direction.x * l->func.direction.x;
            l->func.arc_normal.z = -(l->func.direction.z * l->func.direction.y);

            xVec3Normalize(&l->func.arc_normal, &l->func.arc_normal);
        }

    }
}

// The target's @stringBase0 runs past ours by six strings, in this order:
// "X to test lightning\n", fifteen spaces, "1", "0", "-", "\n". Nothing left
// in the unit references them -- they are a debug overlay the linker
// dead-stripped, printing a row of per-bolt flags under a heading.
void __deadstripped_zLightning()
{
    printf("X to test lightning\n");
    printf("               ");
    printf("1");
    printf("0");
    printf("-");
    printf("\n");
}
