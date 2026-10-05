#include "xClimate.h"

#include <types.h>
#include <string.h>
#include "xMath.h"
#include "xMath3.h"
#include "xString.h"
#include "xVec3.h"
#include "zGlobals.h"
#include "zParEmitter.h"
#include "zParPTank.h"

extern U32 gPTankDisable;

_tagClimate* sClimate;

float snow_life = 3.0f;
const xVec3 snow_vel = { 0.0f, -2.0f, 0.0f };
const xVec3 snow_dvel = { 0.1f, 0.1f, 0.1f };

void xClimateVecFromAngle(F32 angleDegrees, xVec3* vec)
{
    xMat3x3 Mat;

    xMat3x3Identity(&Mat);
    xMat3x3Euler(&Mat, (PI * angleDegrees) / ONEEIGHTY, 0.0f, 0.0f);
    xVec3Init(vec, 0.0f, 0.0f, 1.0f);
    xMat3x3LMulVec(vec, &Mat, vec);
}

void xClimateInit(_tagClimate* climate)
{
    climate->rain.strength = 0.0f;

    climate->rain.rain_emitter = zParEmitterFind(xStrHash("PAREMIT_RAIN"));
    climate->rain.rain_emitter->emit_flags &= 0xfe;

    climate->rain.snow_emitter = zParEmitterFind(xStrHash("PAREMIT_SNOW"));
    climate->rain.snow_emitter->emit_flags &= 0xfe;
}

// Equivalent
// float ops are being optimized more aggressively
void xClimateInitAsset(_tagClimate* climate, xEnvAsset* easset)
{
    sClimate = climate;
    climate->wind.strength = 0.0f;
    xClimateVecFromAngle(climate->wind.angle, &climate->wind.dir);

    if (easset->climateFlags == 0)
    {
        climate->wind.strength = 0.0f;
        climate->rain.strength = 0.0f;
        return;
    }
    if (easset->climateFlags & 1)
    {
        climate->rain.rain = 1.0f;
        climate->rain.strength = 0.5f * (easset->climateStrengthMax - easset->climateStrengthMin);
        climate->rain.strength += easset->climateStrengthMin;
    }
    else if (easset->climateFlags & 2)
    {
        climate->rain.rain = 0.0f;
        climate->rain.strength = 0.5f * (easset->climateStrengthMax - easset->climateStrengthMin);
        climate->rain.strength += easset->climateStrengthMin;
    }
}

void xClimateSetSnow(F32 stre)
{
    sClimate->rain.rain = 0;
    sClimate->rain.strength = stre;
}

void xClimateSetRain(F32 stre)
{
    sClimate->rain.rain = 1;
    sClimate->rain.strength = stre;
}

// Equivalent
// Float literal is being loaded three separate times in the original code.
static void GetPosBigDogWhattupFool(xVec3* vec)
{
    xCamera* camera = &xglobals->camera;
    vec->x = 10.0f * camera->mat.at.x + camera->mat.pos.x;
    vec->y = 10.0f * camera->mat.at.y + camera->mat.pos.y;
    vec->z = 10.0f * camera->mat.at.z + camera->mat.pos.z;
}

static void UpdateRain(_tagClimate* climate, float seconds)
{
    _tagRain* r = &climate->rain;
    xParEmitterCustomSettings info;
    memset(&info, 0, sizeof(xParEmitterCustomSettings));
    info.custom_flags = eParEmitterCustomPos;

    if (r->rain != 0)
    {
        S32 total_rain_drops = 25.0f * r->strength;
        for (S32 i = 0; i < total_rain_drops; i++)
        {
            GetPosBigDogWhattupFool(&info.pos);
            info.pos.x += 25.0f * xurand() - 12.5f;
            info.pos.y += 8.0f;
            info.pos.z += 25.0f * xurand() - 12.5f;
            xParEmitterEmitCustom(r->rain_emitter, seconds, &info);
        }
        return;
    }

    xVec3 fool;
    S32 total_snow_flakes = 25.0f * r->strength;
    info.custom_flags |= eParEmitterCustomVel | eParEmitterCustomLife;
    GetPosBigDogWhattupFool(&fool);
    if (gPTankDisable)
    {
        for (S32 i = 0; i < total_snow_flakes; i++)
        {
            info.pos = fool;
            info.pos.x += 45.0f * xurand() - 22.5f;
            info.pos.z += 45.0f * xurand() - 22.5f;

            F32 xx = info.pos.x - fool.x;
            F32 zz = info.pos.z - fool.z;
            F32 perc = xx * zz / 506.25f;
            perc = 1.0f - perc;

            info.vel.x = snow_dvel.x * xurand() + snow_vel.x;
            info.vel.y = snow_dvel.y * xurand() + snow_vel.y;
            info.vel.z = snow_dvel.z * xurand() + snow_vel.z;

            info.pos.y += 4.0f * perc + 4.0f;
            info.life.val[0] = snow_life * perc + snow_life;
            xParEmitterEmitCustom(r->snow_emitter, seconds, &info);
        }
        return;
    }

    S32 num = (F32)total_snow_flakes * 0.1f;
    if (num > 0)
    {
        xVec3* pos = (xVec3*)xMemPushTemp(num * 2 * sizeof(xVec3));
        xVec3* vel = pos + num;
        if (pos != NULL)
        {
            xVec3* p = pos;
            xVec3* v = vel;

            for (S32 i = 0; i < num; i++, p++, v++)
            {
                *p = fool;
                p->x += 45.0f * xurand() - 22.5f;
                p->z += 45.0f * xurand() - 22.5f;

                F32 xx = p->x - fool.x;
                F32 zz = p->z - fool.z;
                float perc = (xx * xx + zz * zz) / 506.25f;
                perc = 1.0f - perc;

                p->y += 4.0f * perc + 4.0f;

                v->x = snow_dvel.x * xurand() + snow_vel.x;
                v->y = snow_dvel.y * xurand() + snow_vel.y;
                v->z = snow_dvel.z * xurand() + snow_vel.z;
            }

            zParPTankSpawnSnow(pos, vel, num);
            xMemPopTemp(pos);
        }
    }
}

static void UpdateWind(_tagClimate* climate, F32 seconds)
{
    return;
}

void xClimateUpdate(_tagClimate* climate, F32 seconds)
{
    UpdateRain(climate, seconds);
    UpdateWind(climate, seconds);
}

#if !defined(XBOX)
void xVec3Init(xVec3* vec, F32 x, F32 y, F32 z)
{
    vec->x = x;
    vec->y = y;
    vec->z = z;
}
#endif

#if !defined(XBOX)
void xMat3x3Identity(xMat3x3* matrix)
{
    xMat3x3Copy(matrix, &g_I3);
}
#endif

#if !defined(XBOX)
void xMat3x3Copy(xMat3x3* m1, const xMat3x3* m2)
{
    memcpy(m1, m2, sizeof(xMat3x3));
}
#endif
