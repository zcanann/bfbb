// Retail called the 9-argument xVec3* xSndPlay3D out of line from this TU (see
// zEntPlayerDriveUpdate), so opt out of zEnt.h's inline definition of it.
#define XSNDPLAY3D_OUT_OF_LINE

#include "xAnim.h"
#include "zFX.h"
#include <types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iAnim.h"
#include "iAnimSKB.h"
#include "iCollide.h"
#include "iMath.h"
#include "iSnd.h"
#include "iTRC.h"

#include "xDebug.h"
#include "xDraw.h"
#include "xEnt.h"
#include "xEntBoulder.h"
#include "xEvent.h"
#include "xJaw.h"
#include "xMath.h"
#include "xMathInlines.h"
#include "xMemMgr.h"
#include "xPad.h"
#include "xRay3.h"
#include "xScene.h"
#include "xShadow.h"
#include "xScrFx.h"
#include "xSnd.h"
#include "xstransvc.h"
#include "xTRC.h"
#include "xutil.h"
#include "xVec3.h"
#include "xVec3Inlines.h"

#include "zBase.h"
#include "zCamera.h"
#include "zEGenerator.h"
#include "zEntButton.h"
#include "zEntCruiseBubble.h"
#include "zEntDestructObj.h"
#include "zEntHangable.h"
#include "zEntPickup.h"
#include "zEntPlayer.h"
#include "zEntPlayerBungeeState.h"
#include "zEntPlayerOOBState.h"
#include "zEntTeleportBox.h"
#include "zEntTrigger.h"
#include "zGame.h"
#include "zGrid.h"
#include "zGameExtras.h"
#include "zGlobals.h"
#include "zGoo.h"
#include "zGust.h"
#include "zLasso.h"
#include "zMusic.h"
#include "zNPCGoals.h"
#include "zNPCTypeTiki.h"
#include "zNPCTypeTiki.h"
#include "zNPCMessenger.h"
#include "zParPTank.h"
#include "zPlatform.h"
#include "zRumble.h"
#include "zSaveLoad.h"
#include "zShrapnel.h"
#include "zSurface.h"
#include "zThrown.h"

static F32 sHackStuckTimer;
static xVec3 sHackStuckDir;
static xVec3 sHackStuckVel;
static U32 sHackStuckSetDir;
static F32 CATCH_CAPSULE_RAD = 0.45f;
static F32 CATCH_CAPSULE_BIAS = 0.3f;
static F32 sCatchCapsuleTimer;
static F32 stuck_timer;
static F32 not_stuck_timer;
static xVec3 stuck_start_loc;
S32 gSpongeBall;

// Multidimensional sound arrays for each player type
static U32 sPlayerSnd[ePlayer_MAXTYPES][ePlayerSnd_Total] = {};
static U32 sPlayerSndRand[ePlayer_MAXTYPES][ePlayerSnd_Total] = {};
static U32 sPlayerSndID[ePlayer_MAXTYPES][ePlayerSnd_Total] = {};
static _tagRumbleType sPlayerRumbleType[ePlayerSnd_Total];
static F32 sPlayerRumbleTime[ePlayerSnd_Total];
static F32 sPlayerSndFxVolume[ePlayerSnd_Total] = {};
static U32 sPlayerStreamSnd[ePlayer_MAXTYPES][ePlayerStreamSnd_Total] = {};
static U32 sPlayerStreamSndRand[ePlayer_MAXTYPES][ePlayerStreamSnd_Total] = {};
static U32 sCurrentStreamSndID;
static F32 sPlayerSndStreamVolume[ePlayerStreamSnd_Total] = {};
static F32 sPlayerSndSneakDelay;
static S32 sPlayerDiedLastTime;
static S32 sPlayerIgnoreSound;
static S32 sPlayerAttackInAir;

#define MAX_DELAYED_SOUNDS 8

// MINF asset IDs for the two swappable player characters. These are xStrHash()
// name hashes and the source names are not recoverable from the binary, but the
// role of each is unambiguous: every site that tests PATRICK_MODEL_ASSETID goes
// on to load the 'SPPA' sound pack and set the Patrick model, and every site
// that tests SANDY_MODEL_ASSETID loads 'SPSC' and sets the Sandy model.
#define PATRICK_MODEL_ASSETID 0x791025ac
#define SANDY_MODEL_ASSETID 0xc0e34b23
static zDelayedStreamSound sDelayedSound[MAX_DELAYED_SOUNDS];
static zPlayerSndTimer sPlayerStreamSndTimer[ePlayerStreamSnd_Total] = {};

// Labels for the RwBlendFunction the bubble bowl lane uses. Nothing in the
// retail object reads this array, but it is a real global there and its eleven
// string literals open @stringBase0.
const char* sBowlBlendLabels[] = { "BLENDZERO",         "BLENDONE",          "BLENDSRCCOLOR",
                                   "BLENDINVSRCCOLOR",  "BLENDSRCALPHA",     "BLENDINVSRCALPHA",
                                   "BLENDDESTALPHA",    "BLENDINVDESTALPHA", "BLENDDESTCOLOR",
                                   "BLENDINVDESTCOLOR", "BLENDSRCALPHASAT" };

F32 startJump;
F32 startDouble;
F32 startBounce;

static F32 minVelmag = 0.01f;
static F32 maxVelmag = 10.0f;
static F32 curVelmag = 0.0f;
static F32 curVelangle = 0.0f;

static S32 surfSlickness = 1;
static F32 surfFriction = 1.0f;
static F32 surfDamping = minVelmag;
static S32 lastSlickness = 1;
static xVec3 lastDeltaPos;
static xVec3 lastFloorNorm;
static xEnt* lastFloorEnt;
static U32 surfSticky;
static F32 surfSlideStart = DEG2RAD(20);
static F32 surfSlideStop = DEG2RAD(10);
F32 surfSlickRatio;
static F32 surfSlickTimer;
static F32 surfPeakRatio = 1.25f;
static F32 surfAccelWalk = 4.0f;
static F32 surfAccelRun = 8.0f;
static F32 surfDecelIdle = 2.0f;
static F32 surfDecelSkid = 8.0f;
static F32 surfMaxSpeed;
static F32 surfSlipTimer;

static xEnt* sGrabFound;
static S32 sGrabFailed;

static F32 sPlayerCollAdjust;

static zPlayerLassoInfo* sLassoInfo;
static zLasso* sLasso;
static xEnt* sHitch[32];
static S32 sNumHitches;
static F32 sHitchAngle;
static F32 sSwingTimeElapsed;
static S32 sLassoCamLinger;

static S32 sGooKnockedToSafety;
static F32 sGooKnockedTimer;
xEntBoulder* boulderVehicle;
static F32 bvTimeToIdle;
static S32 boulderRollShouldEnd;
static S32 boulderRollShouldStart;
static zParEmitter* sEmitSpinBubbles;
static zParEmitter* sEmitSundae;
static zParEmitter* sEmitStankBreath;
static class xModelTag sStankTag[3];

static RpAtomic* sReticleModel;
static F32 sReticleRot;
static F32 sReticleAlpha;
static xMat4x3 sReticleMat;
static S32 sTypeOfTarget;
static F32 sTimeToRetarget;
class xEnt* gReticleTarget;

// This struct was anonymous in the dwarf but it seemed to do better with codegen to name it
// so I can hold a pointer to it and access the members that way.
static const struct sock
{
    U32 level;
    U32 total;
} patsock_totals[] = { { 'HB\0\0', 8 },
                       { 'JF\0\0', 14 },
                       { 'BB\0\0', 9 },
                       { 'GL\0\0', 11 },
                       { 'BC\0\0', 4 },
                       { 'RB\0\0', 9 },
                       { 'SM\0\0', 10 },
                       { 'KF\0\0', 7 },
                       { 'GY\0\0', 3 },
                       { 'DB\0\0', 5 },
                       {} };

static F32 update_dt = 1.0f / 60.0f;
static F32 last_update_dt = 1.0f / 60.0f;
F32 default_player_radius = 0.5f;
_CurrentPlayer lastgCurrentPlayer = eCurrentPlayerCount;
static xVec3 update_motion;
static xVec3 req_motion;
static xVec3 precollide_motion;
static RwRaster* sBowlingLaneRast;
_CurrentPlayer gCurrentPlayer;
F32 floor_safe_tmr;
static F32 bbash_start_ht;
static F32 bbash_end_tmr;
static F32 bbash_tmr;
static F32 bbash_vel;
static S32 bbash_hit;
static S32 bbounce_hit;

static F32 idle_tmr;
static F32 inact_tmr;
static F32 stun_power_tmr;

static F32 tslide_maxspd;
static F32 tslide_maxspd_tmr;
static F32 tslide_inair_tmr;
static F32 tslide_dbl_tmr;
static U32 tslide_ground;
static xVec3 tslide_lastrealvel;

static S32 in_goo;
static S32 lin_goo;
static F32 in_goo_tmr;
static U32 player_hitlist_anim;
S32 player_hit;
static U32 player_idle_anim;
static U32 mount_type;
static xEnt* mount_object;
static F32 mount_tmr;
static S32 player_hit_anim = 1;
static U32 player_dead_anim = 1;

static xVec3 last_center;
static U32 last_frame;

static F32 sBubbleBowlLastWindupTime = -1.0f;
static F32 sBubbleBowlMultiplier = 1.0f;
static F32 sPlayerNPC_KnockBackTime = 0.5f;
static F32 sPlayerNPC_KnockBackVel = 11.5;
static U32 sShouldBubbleBowl;
static F32 sBubbleBowlTimer;
static U32 sSpatulaGrabbed;
S32 gWaitingToAutoSave;

zGustData gust_data;
xMat4x3 gPlayerAbsMat;
xMat4x3 rendermat;
xMat4x3 sCameraLastMat;
xVec3 sDriveVel;
xVec3 floor_supp[4];
F32 floor_dist[4];
F32 floor_tmr[4];
xVec3 floor_safe_vec;
xAnimTransition sandyHitTran[8];
xAnimTransition patrickHitTran[8];

static enum {
    WallJumpResult_NoJump,
    WallJumpResult_Jump,
} sWallJumpResult;
static xVec3 sWallNormal;

namespace
{
    static struct foo
    {
        S32 anim;
        U32 sndid;
        void* data;
        F32 time;
    } player_talk;
} // namespace

static xModelTag sSandyLFoot;
static xModelTag sSandyRFoot;
static xModelTag sSandyLHand;
static xModelTag sSandyRHand;
static xModelTag sSandyLKnee;
static xModelTag sSandyRKnee;
static xModelTag sSandyLElbow;
static xModelTag sSandyRElbow;
static xModelTag sSpongeBobLKnee;
static xModelTag sSpongeBobRKnee;
static xModelTag sSpongeBobLElbow;
static xModelTag sSpongeBobRElbow;
static xModelTag sSpongeBobLFoot;
static xModelTag sSpongeBobRFoot;
static xModelTag sSpongeBobLHand;
static xModelTag sSpongeBobRHand;
static xModelTag sPatrickLFoot;
static xModelTag sPatrickRFoot;
static xModelTag sPatrickLHand;
static xModelTag sPatrickRHand;
static xModelTag sPatrickLKnee;
static xModelTag sPatrickRKnee;
static xModelTag sPatrickLElbow;
static xModelTag sPatrickRElbow;
static xModelTag sPatrickMelee;
static zSurfaceProps* sWallCollisionSurface;

static void zEntPlayer_ReticleRender(zEnt* ent);
static void zEntPlayer_UpdateVelocityBlur();
void zEntPlayer_SNDPlayDelayed(F32 seconds);
S32 zEntPlayerEventCB(xBase* from, xBase* to, U32 toEvent, const F32* toParam,
                      xBase* toParamWidget);
static void PlayerRotMatchUpdateEnt(xEnt* ent, xScene* sc, F32 dt, void* fdata);
static S32 BoulderVEventCB(xBase* from, xBase* to, U32 toEvent, const F32* toParam,
                           xBase* toParamWidget);
static void zEntPlayer_SNDInit();
void zEntPlayer_RestoreSounds();

void zEntPlayerCollide(xEnt* ent, xScene* sc, F32 dt);
void zEntPlayer_CheckCritterContact(xEnt* ent, F32 dt);
static void PlayerLedgeUpdate(xEnt* ent, xScene* sc, F32 dt);
static void zEntPlayerFloorUpdate(xEnt* ent, xScene* sc, F32 dt);
static void PlayerTeeterCheck(xEnt* ent, xScene* sc, F32 dt);
static void zEntPlayerSurfDamageUpdate(xEnt* ent, xScene* sc, F32 dt);
static void zEntPlayerDriveUpdate(xEnt* ent, xScene* sc, F32 dt);
static void zEntPlayerJumpUpdate(xEnt* ent, xScene* sc, F32 dt);
static void zEntPlayerTSlideUpdate(xEnt* ent, xScene* sc, F32 dt);
void zEntPlayerCollTrigger(xEnt* ent, xScene* sc);
static void zEntPlayerVelUpdate(xEnt* ent, xScene* sc, F32 dt);

static void zEntPlayerEGenUpdate(xEnt* ent, xScene* sc, F32 dt);
static xEnt* zEntPlayer_FindGrabEnt(xEnt* ent, zScene* zsc, S32* failed);
static void PlayerSwingUpdate(xEnt* ent, F32 mag, F32 angle, F32 dt);
static S32 CheckObjectAgainstMeleeBound(xEnt* ent, void* data);
static void zEntPlayer_PredictionUpdate(xEnt* ent, F32 dt);

static void zEntPlayer_SpawnWandBubbles(xVec3* center, U32 count)
{
    if (gFrameCount - last_frame > 5)
    {
        xVec3 wand;
        xVec3ScaleC(&wand, (xVec3*)&globals.player.model_wand->Mat->at, 0.25f, 0.25f, 0.25f);
        xVec3Sub(&last_center, center, &wand);
    }

    xVec3 dir;
    xVec3Sub(&dir, center, &last_center);

    U32 num = 3;
    if (count != 0)
    {
        num = count;
    }

    xVec3* posbuf = (xVec3*)xMemPushTemp(num * 2 * sizeof(xVec3));
    xVec3* velbuf = posbuf + num;
    if (posbuf)
    {
        xVec3* pp = posbuf;
        xVec3* vp = velbuf;
        U32 j = 0;
        for (; j < num; j++, pp++, vp++)
        {
            F32 f = (F32)j / (F32)num;
            xVec3Lerp(pp, &last_center, center, f);
            pp->x += 0.125f * (xurand() - 0.5f);
            pp->y += 0.125f * (xurand() - 0.5f);
            pp->z += 0.125f * (xurand() - 0.5f);

            f = 5.0f * xurand();
            xVec3ScaleC(vp, &dir, f, f, f);
            vp->x += 0.25f * (xurand() - 0.5f);
            vp->y += 0.25f * (xurand() - 0.5f);
            vp->z += 0.25f * (xurand() - 0.5f);
        }

        zParPTankSpawnBubbles(posbuf, velbuf, num, 1.0f);
        xMemPopTemp(posbuf);
    }

    last_center = *center;
    last_frame = gFrameCount;
}

static void zEntPlayerKillCarry()
{
    if (!globals.player.carry.grabbed)
    {
        return;
    }

    if (!zThrown_KillFruit(globals.player.carry.grabbed))
    {
        if (globals.player.carry.grabbed->baseType == eBaseTypeDestructObj)
        {
            zEntEvent(globals.player.carry.grabbed, eEventDestroy);
        }
        else if (globals.player.carry.grabbed->baseType == eBaseTypeNPC &&
                 (((xNPCBasic*)globals.player.carry.grabbed)->SelfType() & 0xffffff00) == 'NTT\0')
        {
            zNPCTiki* tiki = (zNPCTiki*)globals.player.carry.grabbed;
            tiki->Damage(DMGTYP_THUNDER_TIKI_EXPLOSION, NULL, NULL);
        }
        else if (globals.player.carry.grabbed->baseType == eBaseTypeNPC)
        {
            zThrown_LaunchDir(globals.player.carry.grabbed,
                              (xVec3*)&globals.player.ent.model->Mat->at);
        }
    }
    globals.player.carry.grabbed = NULL;
}

void zEntPlayerControlOn(zControlOwner owner)
{
    U32 originalValue = globals.player.ControlOff;
    globals.player.ControlOff &= ~owner;

    if (originalValue != globals.player.ControlOff)
    {
        if (globals.player.ControlOff & 0x202)
        {
            xSndSelectListenerMode(SND_LISTENER_MODE_CAMERA);
        }
        else
        {
            xSndSelectListenerMode(SND_LISTENER_MODE_PLAYER);
        }
    }
}

// 83%, but floating point scheduling preventing match
void zEntPlayerControlOff(zControlOwner owner)
{
    U32 originalValue = globals.player.ControlOff;

    globals.player.ControlOff |= owner;
    globals.player.ControlOffTimer = 1.0f;

    if (originalValue != globals.player.ControlOff)
    {
        if (globals.player.ControlOff & 0x202)
        {
            xSndSelectListenerMode(SND_LISTENER_MODE_CAMERA);
        }
        else
        {
            xSndSelectListenerMode(SND_LISTENER_MODE_PLAYER);
        }
    }

    zEntPlayerKillCarry();
}

void TellPlayerVillainIsNear(F32 visnear)
{
    globals.player.BadGuyNearTimer = visnear;
}

void SetPlayerKillsVillainTimer(F32 time)
{
    globals.player.VictoryTimer = time;
}
static void DampenControls(F32* angle, F32* mag, F32 x, F32 y)
{
    *angle = xatan2(x, y);

    if (x > -globals.player.g.AnalogMin && x < globals.player.g.AnalogMin)
    {
        x = 0.0f;
    }

    if (y > -globals.player.g.AnalogMin && y < globals.player.g.AnalogMin)
    {
        y = 0.0f;
    }

    if (!x && !y)
    {
        *angle = 0.0f;
        *mag = 0.0f;
        return;
    }

    if (xabs(x) > xabs(y))
    {
        *mag = xabs(x);
    }
    else
    {
        *mag = xabs(y);
    }
    *mag = (*mag - globals.player.g.AnalogMin) /
           (globals.player.g.AnalogMax - globals.player.g.AnalogMin);

    if (*mag < 0.0f)
    {
        *mag = 0.0f;
        *angle = 0.0f;
    }
    else if (*mag > 1.0f)
    {
        *mag = 1.0f;
    }
}

static void CalcAnimSpeed(xEnt* ent, float f, float* pf)
{
    if (!pf[0])
    {
        return;
    }

    f = f / pf[0];
    if (f < pf[1])
    {
        f = pf[1];
    }
    else if (f > pf[2])
    {
        f = pf[2];
    }

    ent->model->Anim->Single->CurrentSpeed = f;
}

static void LeanUpdate(F32 angle, F32 dt)
{
    float abs = xabs(angle);
    float lerp;
    if (abs < 0.087266468f)
    {
        lerp = 0.0f;
    }
    else if (abs > 0.2617994f)
    {
        lerp = 1.0f;
    }
    else
    {
        lerp = 5.729578f * (abs - 0.087266468f);
    }

    if (angle > 0.0f)
    {
        lerp = -lerp;
    }
    lerp += 1.0f;

    F32 t = 6.0f * (lerp - globals.player.LeanLerp);
    globals.player.LeanLerp += t * dt;
}

static void TurnToFace(xEnt* ent, const xVec3* target, F32 speedLimit, F32 dt)
{
    xVec3 currentFacing = ent->frame->mat.at;
    xVec3Normalize(&currentFacing, &currentFacing);

    F32 angle = xVec3Dot(&currentFacing, target);
    const F32 maxAngle = 0.9999f;
    if (angle < maxAngle)
    {
        xVec3Cross(&ent->frame->drot.axis, &currentFacing, target);
        xVec3Normalize(&ent->frame->drot.axis, &ent->frame->drot.axis);

        angle = xacos(angle);
        if (angle > speedLimit * dt)
        {
            angle = speedLimit * dt;
        }

        if (ent->frame->drot.axis.y < 0.0f)
        {
            angle = -angle;
        }

        ent->frame->drot.angle = angle;
        ent->frame->mode |= 0x20;
    }
}

static void PlayerArrive(xEnt* ent, xBase* base)
{
    globals.player.AutoMoveSpeed = 0;

    zEntEvent(base, ent, 0x1f);

    if (base->baseType == 0xd)
    {
        zEntEvent(ent, base, 0x1f);
    }
}

#define CLAMP_ANGLE(a)                                                                             \
    if (a > PI)                                                                                    \
    {                                                                                              \
        a -= 2 * PI;                                                                               \
    }                                                                                              \
    else if (a < -PI)                                                                              \
    {                                                                                              \
        a = a + 2 * PI;                                                                            \
    }

static void PlayerAbsControl(xEnt* ent, F32 x, F32 z, F32 dt)
{
    U32 animUserFlag;
    U32 blendUserFlag;
    F32 mag = 1.0f;
    F32 rot = 0.0f;
    F32 angle = 0.0f;
    maxVelmag = 0.0f;

    if (gTrcPad[0].state != TRC_PadInserted)
    {
        z = x = 0.0f;
    }

    if (globals.player.ControlOff || sHackStuckTimer || sCatchCapsuleTimer > 0.15f)
    {
        z = x = 0.0f;
    }

    // F32 scalemag;
    // F32 dir_dp;
    // F32 turnfactor;
    // F32 diffAngle;
    // F32 autodist2d;
    // F32 camAngle;
    xMat4x3* m = &ent->frame->mat;
    xVec3 euler;
    xMat3x3GetEuler(m, &euler);

    ent->frame->rot.angle = euler.x >= 0.0f ? euler.x : euler.x + 2 * PI;
    surfMaxSpeed = 0.0f;

    animUserFlag = ent->model->Anim->Single->State ? ent->model->Anim->Single->State->UserFlags : 0;

    blendUserFlag = ent->model->Anim->Single->Blend && ent->model->Anim->Single->Blend->State ?
                        ent->model->Anim->Single->Blend->State->UserFlags | 0x80000000 :
                        0;

    if (globals.player.KnockIntoAirTimer && (animUserFlag & 0x1e) == 0 && (animUserFlag & 0x1) == 0)
    {
        animUserFlag |= 0x8 | 0x2;
        animUserFlag &= ~0x80;
    }

    memset(&ent->frame->dpos, 0, sizeof(xVec3));

    if (!(globals.player.KnockBackTimer || animUserFlag & 0x100))
    {
        if (globals.player.ControlOff & 0x4000)
        {
            // F32 rot;
            F32 dx = ent->frame->vel.x * ent->frame->vel.x;
            F32 dz = ent->frame->vel.z * ent->frame->vel.z;

            if (dx + dz > 0.01f)
            {
                ent->frame->mode &= ~0x2;

                angle = xatan2(ent->frame->vel.x, ent->frame->vel.z) - ent->frame->rot.angle;
                CLAMP_ANGLE(angle);

                ent->frame->drot.angle = 4.0f * angle * dt;
                ent->frame->mode |= 0x20;

                if (xabs(ent->frame->drot.angle) < 0.006f)
                {
                    if (xabs(angle) > 0.006f)
                    {
                        angle = angle > 0.0f ? 0.006f : -0.006f;
                    }
                    ent->frame->drot.angle = angle;
                }
            }
        }
        else
        {
            // TODO: figure out which variables these were
            F32 stackAng, stackMag;
            DampenControls(&stackAng, &stackMag, x, z);
            xMat4x3Copy(&gPlayerAbsMat, &globals.camera.mat);

            if (globals.player.carry.grabTarget || globals.player.carry.throwTarget)
            {
                if (!(strcmp(ent->model->Anim->Single->State->Name, "Carry_Pickup") == 0 ||
                      strcmp(ent->model->Anim->Single->State->Name, "Carry_Throw") == 0))
                {
                    globals.player.carry.grabTarget = 0;
                    globals.player.carry.throwTarget = NULL;
                    goto no_carry;
                }
                else
                {
                    ent->frame->mode &= ~0x2;

                    angle = globals.player.carry.targetRot - ent->frame->rot.angle;
                    CLAMP_ANGLE(angle);

                    if (globals.player.carry.throwTarget)
                    {
                        globals.player.carry.flyingToTarget = NULL;
                        if (angle > 0.0f)
                        {
                            ent->frame->drot.angle = globals.player.carry.throwTargetRotRate * dt;
                        }
                        else
                        {
                            ent->frame->drot.angle = -globals.player.carry.throwTargetRotRate * dt;
                        }

                        if (xabs(ent->frame->drot.angle) > angle)
                        {
                            ent->frame->drot.angle = angle;
                        }

                        F32 dx = globals.player.carry.throwTarget->model->Mat->pos.x -
                                 ent->model->Mat->pos.x;
                        F32 dz = globals.player.carry.throwTarget->model->Mat->pos.z -
                                 ent->model->Mat->pos.z;

                        // unused?
                        xsqrt(dx * dx + dz * dz);
                    }
                    else
                    {
                        ent->frame->drot.angle = 4.0f * angle * dt;
                    }

                    ent->frame->mode |= 0x20;

                    if (xabs(ent->frame->drot.angle) < 0.006f)
                    {
                        if (xabs(angle) > 0.006f)
                        {
                            angle = angle > 0.0f ? 0.006f : -0.006f;
                        }
                        ent->frame->drot.angle = angle;
                    }

                    // F32 ddot;
                    F32 atime = ent->model->Anim->Single->Time;
                    F32 lerp = globals.player.carry.grabLerpLast;

                    if (lerp < globals.player.carry.grabLerpMax &&
                        atime > globals.player.carry.grabLerpMin &&
                        globals.player.carry.grabLerpMax > globals.player.carry.grabLerpMin)
                    {
                        if (globals.player.carry.grabLerpMin > lerp)
                        {
                            lerp = globals.player.carry.grabLerpMin;
                        }

                        F32 t;
                        if (globals.player.carry.grabLerpMax < atime)
                        {
                            t = globals.player.carry.grabLerpMax;
                        }
                        else
                        {
                            t = atime;
                        }

                        lerp = -((t - lerp) / (globals.player.carry.grabLerpMax -
                                               globals.player.carry.grabLerpMin));

                        ent->frame->dpos.x = lerp * globals.player.carry.grabOffset.x;
                        ent->frame->dpos.z = lerp * globals.player.carry.grabOffset.z;
                        ent->frame->mode |= 0x2;
                        globals.player.carry.grabLerpLast = atime;
                    }
                }
            }
            else
            {
            no_carry:
                if (strcmp(ent->model->Anim->Single->State->Name, "SpatulaGrab01") == 0)
                {
                    ent->frame->mode &= ~0x2;
                    angle = xatan2(globals.camera.mat.pos.x - globals.player.ent.frame->mat.pos.x,
                                   globals.camera.mat.pos.z - globals.player.ent.frame->mat.pos.z);

                    angle -= ent->frame->rot.angle;
                    CLAMP_ANGLE(angle);

                    ent->frame->drot.angle = 4.0f * angle * dt;
                    ent->frame->mode |= 0x20;

                    if (xabs(ent->frame->drot.angle) < 0.006f)
                    {
                        if (xabs(angle) > 0.006f)
                        {
                            angle = angle > 0.0f ? 0.006f : -0.006f;
                        }

                        ent->frame->drot.angle = angle;
                    }
                }
                else if (sLassoInfo->target != NULL &&
                         (strcmp(ent->model->Anim->Single->State->Name, "LassoWindup") == 0 ||
                          strcmp(ent->model->Anim->Single->State->Name, "LassoAboutToDestroy") ==
                              0))
                {
                    ent->frame->mode &= ~2;
                    sLassoInfo->lassoRot = xatan2(sLassoInfo->target->model->Mat->pos.x -
                                                      globals.player.ent.frame->mat.pos.x,
                                                  sLassoInfo->target->model->Mat->pos.z -
                                                      globals.player.ent.frame->mat.pos.z);

                    angle = sLassoInfo->lassoRot - ent->frame->rot.angle;
                    CLAMP_ANGLE(angle);

                    ent->frame->drot.angle = 4.0f * angle * dt;
                    ent->frame->mode |= 0x20;

                    if (xabs(ent->frame->drot.angle) < 0.006f)
                    {
                        if (xabs(angle) > 0.006f)
                        {
                            angle = angle > 0.0f ? 0.006f : -0.006f;
                        }

                        ent->frame->drot.angle = angle;
                    }
                }
                else
                {
                    // F32 m;
                    if (stackMag)
                    {
                        F32 t = xatan2(gPlayerAbsMat.right.z, gPlayerAbsMat.right.x);
                        stackAng -= t;
                        if (stackAng > 2 * 3.1415927f)
                        {
                            stackAng -= 2 * 3.1415927f;
                        }
                        else if (stackAng < 0.0f)
                        {
                            stackAng += 2 * 3.1415927f;
                        }

                        if ((animUserFlag & (0x800 | 0x80)) == 0)
                        {
                            angle = stackAng - ent->frame->rot.angle;
                            CLAMP_ANGLE(angle);

                            rot = icos(angle);
                            ent->frame->drot.angle = dt * (7.0f * angle);
                            ent->frame->mode |= 0x20;
                        }

                        if (globals.player.IsBubbleBowling)
                        {
                            angle *= 0.1f;
                            rot = icos(angle);
                            ent->frame->drot.angle = dt * (7.0f * angle);
                            xMat3x3 rotY;
                            xMat3x3RotY(&rotY, ent->frame->drot.angle);
                            xMat3x3RMulVec(&ent->frame->vel, &rotY, &ent->frame->vel);
                        }

                        if (animUserFlag & 0x800)
                        {
                            rot = 1.0f;
                        }

                        if (stackMag > globals.player.s->MoveSpeed[3])
                        {
                            F32 scalemag = globals.player.SpeedMult;
                            if (stackMag < globals.player.s->MoveSpeed[4])
                            {
                                globals.player.Speed = 1;
                                maxVelmag = globals.player.s->MoveSpeed[1] * scalemag;
                                mag = (scalemag * (stackMag * globals.player.s->MoveSpeed[1])) /
                                      globals.player.s->MoveSpeed[4];
                            }
                            else
                            {
                                globals.player.Speed = 2;
                                maxVelmag = globals.player.s->MoveSpeed[2] * scalemag;
                                F32 slideVelMag = (stackMag - globals.player.s->MoveSpeed[4]) /
                                                  (globals.player.s->MoveSpeed[5] -
                                                   globals.player.s->MoveSpeed[4]);
                                if (slideVelMag > 1.0f)
                                {
                                    slideVelMag = 1.0f;
                                }
                                F32 slideAccel = globals.player.s->MoveSpeed[1] * scalemag;
                                mag = slideVelMag *
                                          (globals.player.s->MoveSpeed[2] * scalemag - slideAccel) +
                                      slideAccel;
                            }
                        }
                    }
                    else
                    {
                        rot = 0.0f;
                        globals.player.Speed = 0;
                    }

                    ent->frame->mode &= ~0x2;

                    if (sLassoInfo->swingTarget != NULL)
                    {
                        PlayerSwingUpdate(ent, stackMag, stackAng, dt);
                        return;
                    }

                    if (sLassoCamLinger)
                    {
                        F32 curFactor = zCameraGetLassoCamFactor() + dt;
                        if (curFactor > 1.0f)
                        {
                            zCameraDisableLassoCam();
                            sLassoCamLinger = 0;
                        }
                        else
                        {
                            zCameraSetLassoCamFactor(curFactor);
                        }
                    }

                    zPlayerGlobals* pg = &globals.player;
                    if (pg->HangEnt)
                    {
                        return;
                    }

                    if ((pg->SlideTrackSliding & 1 && (pg->JumpState == 0 || pg->JumpState == 1)) ||
                        pg->SlideTrackDecay && pg->JumpState && pg->JumpState != 1)
                    {
                        F32 accelX =
                            (pg->SlideTrackVel.x * pg->SlideTrackDir.x +
                             pg->SlideTrackVel.z * pg->SlideTrackDir.z - pg->g.SlideAccelVelMin) /
                            (pg->g.SlideAccelVelMax - pg->g.SlideAccelVelMin);

                        F32 accelZ;
                        if (accelX < 0.0f)
                        {
                            accelZ = pg->g.SlideAccelStart;
                        }
                        else if (accelX > 1.0f)
                        {
                            accelZ = pg->g.SlideAccelEnd;
                        }
                        else
                        {
                            accelZ = accelX * (pg->g.SlideAccelEnd - pg->g.SlideAccelStart) +
                                     pg->g.SlideAccelStart;
                        }

                        pg->SlideTrackVel.x += dt * pg->SlideTrackDir.x * accelZ;
                        pg->SlideTrackVel.z += dt * pg->SlideTrackDir.z * accelZ;

                        accelX = stackMag * isin(stackAng) * dt;
                        accelZ = stackMag * icos(stackAng) * dt;

                        F32 fwdComponent =
                            accelX * pg->SlideTrackDir.x + accelZ * pg->SlideTrackDir.z;
                        F32 sideComponent =
                            (accelZ * pg->SlideTrackDir.x - accelX * pg->SlideTrackDir.z);

                        F32 veldown = pg->SlideTrackVel.x * pg->SlideTrackDir.x +
                                      pg->SlideTrackVel.z * pg->SlideTrackDir.z;
                        if (veldown < 1.0f)
                        {
                            if (fwdComponent < 0.0f)
                            {
                                fwdComponent = 0.0f;
                            }
                        }
                        else if (veldown < 6.0f)
                        {
                            if (fwdComponent < 0.0f)
                            {
                                fwdComponent *= (veldown - 1.0f) / 5.0f;
                            }
                        }

                        if (fwdComponent > 0.0f)
                        {
                            pg->SlideTrackVel.x +=
                                pg->SlideTrackDir.x * (pg->g.SlideAccelPlayerFwd * fwdComponent);
                            pg->SlideTrackVel.z +=
                                pg->SlideTrackDir.z * (pg->g.SlideAccelPlayerFwd * fwdComponent);
                        }
                        else
                        {
                            pg->SlideTrackVel.x +=
                                pg->SlideTrackDir.x * (pg->g.SlideAccelPlayerBack * fwdComponent);
                            pg->SlideTrackVel.z +=
                                pg->SlideTrackDir.z * (pg->g.SlideAccelPlayerBack * fwdComponent);
                        }

                        pg->SlideTrackVel.x -=
                            (pg->SlideTrackDir.z * (pg->g.SlideAccelPlayerSide * sideComponent));
                        pg->SlideTrackVel.z +=
                            pg->SlideTrackDir.x * (pg->g.SlideAccelPlayerSide * sideComponent);

                        mag = xsqrt(pg->SlideTrackVel.x * pg->SlideTrackVel.x +
                                    pg->SlideTrackVel.z * pg->SlideTrackVel.z);
                        if (mag >= tslide_maxspd)
                        {
                            if (globals.player.SlideTrackDecay == globals.player.g.SlideAirHoldTime)
                            {
                                tslide_maxspd_tmr += dt;
                                if (fwdComponent > 0.0f)
                                {
                                    tslide_maxspd_tmr +=
                                        fwdComponent * dt * pg->g.SlideVelMaxIncAccel;
                                }
                            }

                            if (tslide_maxspd_tmr <= pg->g.SlideVelMaxIncTime)
                            {
                                tslide_maxspd =
                                    (tslide_maxspd_tmr / pg->g.SlideVelMaxIncTime) *
                                        (pg->g.SlideVelMaxEnd - pg->g.SlideVelMaxStart) +
                                    pg->g.SlideVelMaxStart;
                            }
                        }
                        else
                        {
                            if (mag > pg->g.SlideVelMaxStart)
                            {
                                tslide_maxspd_tmr =
                                    (pg->g.SlideVelMaxIncTime * (mag - pg->g.SlideVelMaxStart)) /
                                    (pg->g.SlideVelMaxEnd - pg->g.SlideVelMaxStart);
                                tslide_maxspd = mag;
                            }
                            else
                            {
                                tslide_maxspd_tmr = 0.0f;
                                tslide_maxspd = pg->g.SlideVelMaxStart;
                            }
                        }

                        if (mag > tslide_maxspd)
                        {
                            pg->SlideTrackVel.x *= tslide_maxspd / mag;
                            pg->SlideTrackVel.z *= tslide_maxspd / mag;
                        }

                        // F32 fwdlerp;
                        angle = xatan2(pg->SlideTrackVel.x, pg->SlideTrackVel.z);
                        F32 targetAngle = angle - ent->frame->rot.angle;
                        CLAMP_ANGLE(targetAngle);

                        ent->frame->drot.angle = 4.0f * targetAngle * dt;
                        ent->frame->mode |= 0x20;
                        if (xabs(ent->frame->drot.angle) < 0.006f)
                        {
                            if (xabs(targetAngle) > 0.006f)
                            {
                                targetAngle = targetAngle > 0.0f ? 0.006f : -0.006f;
                            }
                            ent->frame->drot.angle = targetAngle;
                        }

                        if (animUserFlag & 0x40)
                        {
                            F32 targetLean = 0.0f;
                            if (stackMag)
                            {
                                targetLean = stackAng - ent->frame->rot.angle;
                                CLAMP_ANGLE(targetLean);
                            }

                            targetLean = -targetLean / DEG2RAD(50);

                            if (targetLean < -1.0f)
                            {
                                targetLean = -1.0f;
                            }
                            else if (targetLean > 1.0f)
                            {
                                targetLean = 1.0f;
                            }

                            globals.player.SlideTrackLean +=
                                0.04f * (targetLean - globals.player.SlideTrackLean);
                            ent->model->Anim->Single->BilinearLerp[0] =
                                1.0f + globals.player.SlideTrackLean;
                        }
                        else
                        {
                            globals.player.SlideTrackLean = 0.0f;
                        }

                        ent->frame->dpos.x = dt * globals.player.SlideTrackVel.x;
                        ent->frame->dpos.z = dt * globals.player.SlideTrackVel.z;
                        ent->frame->mode |= 0x2;
                        ent->frame->vel.x = 0.0f;
                        ent->frame->vel.z = 0.0f;

                        if ((globals.player.SlideTrackSliding & 1) == 0)
                        {
                            globals.player.SlideTrackDecay -= dt;
                            if (globals.player.SlideTrackDecay < 0.0f)
                            {
                                globals.player.SlideTrackDecay = 0.0f;
                                ent->frame->vel.x = globals.player.SlideTrackVel.x;
                                ent->frame->vel.z = globals.player.SlideTrackVel.z;
                            }
                        }
                        else
                        {
                            globals.player.SlideTrackDecay = globals.player.g.SlideAirHoldTime;
                        }
                        return;
                    }

                    pg->SlideTrackDecay = 0.0f;
                    if ((animUserFlag & 0x1e) != 0 || blendUserFlag & 0x1e)
                    {
                        U32 moveFlag = animUserFlag & 0x1e;
                        if ((animUserFlag & 0x1e) == 0)
                        {
                            moveFlag = blendUserFlag & 0x1e;
                        }

                        switch (moveFlag)
                        {
                        case 0x2:
                            if (rot <= 0.0f || !stackMag)
                            {
                                break;
                            }
                            stackMag = mag * rot;
                            goto finish;

                        case 0x4:
                            if (rot <= 0.0f)
                            {
                                break;
                            }
                            stackMag = mag * rot;
                            goto finish;
                        case 0x4 | 0x2:
                            if (stackMag)
                            {
                                if (xabs(angle) >= PI / 6)
                                {
                                    if (angle > 0.0f)
                                    {
                                        stackAng = ent->frame->rot.angle + PI / 6;
                                    }
                                    else
                                    {
                                        stackAng = ent->frame->rot.angle - PI / 6;
                                    }
                                }
                            }
                            else
                            {
                                stackAng = ent->frame->rot.angle;
                            }

                            if ((animUserFlag & 0x20))
                            {
                                if (surfSlickRatio)
                                {
                                    break;
                                }
                                stackMag = globals.player.DecelRunSpeed;
                                globals.player.DecelRunSpeed =
                                    stackMag - ((4.0f / 3.0f) * (globals.player.DecelRun * dt));

                                if (globals.player.DecelRunSpeed < 0.0f)
                                {
                                    globals.player.DecelRunSpeed = 0.0f;
                                }
                                goto finish;
                            }
                            else if (stackMag)
                            {
                                stackMag = mag;
                                globals.player.DecelRunSpeed = mag;
                                globals.player.DecelRun = mag;
                                goto finish;
                            }
                            break;
                        case 0x8 | 0x2:
                            if (rot <= 0.0f && !globals.player.cheat_mode)
                            {
                                break;
                            }

                            stackMag = mag * rot;
                            goto finish;
                        case 0x8 | 0x4:
                            stackAng = ent->frame->rot.angle;
                            stackMag = globals.player.HeadbuttVel;
                            globals.player.DecelRunSpeed = stackMag;
                            globals.player.DecelRun = stackMag;
                            // fall through
                        finish:
                        default:
                            switch ((animUserFlag & 0x1e))
                            {
                            case 0x2:
                                if (strcmp(ent->model->Anim->Single->State->Name, "Walk_sneak") ==
                                        0 ||
                                    strcmp(ent->model->Anim->Single->State->Name,
                                           "Walk_blackknight") == 0)
                                {
                                    CalcAnimSpeed(ent, stackMag, globals.player.s->AnimSneak);
                                }
                                break;
                            case 0x4:
                                CalcAnimSpeed(ent, stackMag, globals.player.s->AnimWalk);
                                if (ent->model->Anim->Single->State->Speed != 1.0f)
                                {
                                    ent->model->Anim->Single->CurrentSpeed =
                                        ent->model->Anim->Single->State->Speed;
                                }
                                break;
                            case 0x2 | 0x4:
                                CalcAnimSpeed(ent, stackMag, globals.player.s->AnimRun);
                                break;
                            }

                            // xVec3* vel;
                            F32 accelMag;
                            // F32 peakLerp;
                            // F32 slickLerp;
                            if (surfSlickRatio && !globals.player.ControlOff)
                            {
                                if (!stackMag)
                                {
                                    break;
                                }

                                F32 s = (1.0f - (4.0f / 3.0f) * surfSlipTimer) * surfSlickRatio +
                                        (4.0f / 3.0f) * surfSlipTimer * 20.0f * surfSlickRatio;

                                if (moveFlag == 0x4 || moveFlag == 2)
                                {
                                    accelMag = surfAccelWalk * s;
                                    surfMaxSpeed = maxVelmag * surfPeakRatio;
                                }
                                else
                                {
                                    accelMag = surfAccelRun * s;
                                    surfMaxSpeed = maxVelmag * surfPeakRatio;
                                }

                                ent->frame->vel.x += accelMag * isin(stackAng) * dt;
                                ent->frame->vel.z += accelMag * icos(stackAng) * dt;

                                s = 2.5f * surfSlipTimer;
                                if (s >= 1.0f)
                                {
                                    surfMaxSpeed = stackMag;
                                }
                                else
                                {
                                    surfMaxSpeed = (1.0f - s) * surfMaxSpeed + s * stackMag;
                                }
                            }
                            else
                            {
                                stackMag *= dt;
                                F32 s = tslide_inair_tmr - globals.player.g.SlideAirHoldTime;
                                if (s >= 0.0f && s < globals.player.g.SlideAirSlowTime)
                                {
                                    stackMag *= s / globals.player.g.SlideAirSlowTime;
                                }

                                if (tslide_dbl_tmr > 0.0f &&
                                    tslide_dbl_tmr < globals.player.g.SlideAirDblHoldTime)
                                {
                                    stackMag *=
                                        tslide_dbl_tmr / globals.player.g.SlideAirDblHoldTime;
                                }

                                if (globals.player.cheat_mode)
                                {
                                    stackMag *= 3.0f;
                                    if (globals.pad0->on & (XPAD_BUTTON_R1 | XPAD_BUTTON_R2 |
                                                            XPAD_BUTTON_L1 | XPAD_BUTTON_L2))
                                    {
                                        stackMag *= 4.0f;
                                    }
                                }

                                ent->frame->dpos.x = stackMag * isin(stackAng);
                                ent->frame->dpos.z = stackMag * icos(stackAng);
                                ent->frame->mode |= 0x2;

                                if (globals.player.ControlOff &&
                                    globals.player.AutoMoveSpeed != 0 &&
                                    globals.player.AutoMoveSpeed != 4 &&
                                    ent->frame->dpos.x * ent->frame->dpos.x +
                                            ent->frame->dpos.z * ent->frame->dpos.z >=
                                        0.0f)
                                {
                                    ent->frame->dpos.x =
                                        globals.player.AutoMoveTarget.x - ent->model->Mat->pos.x;
                                    ent->frame->dpos.z =
                                        globals.player.AutoMoveTarget.z - ent->model->Mat->pos.z;
                                    PlayerArrive(ent, globals.player.AutoMoveObject);
                                }
                            }
                            break;
                        }
                    }
                }
            }
        }

        LeanUpdate(angle, dt);

        if (animUserFlag & 0x40)
        {
            ent->model->Anim->Single->BilinearLerp[0] = globals.player.LeanLerp;
        }

        if (blendUserFlag & 0x40)
        {
            ent->model->Anim->Single->Blend->BilinearLerp[0] = globals.player.LeanLerp;
        }
    }
}

static PlayerStreakInfo sStreakInfo[3][4] = {
    {
        { FALSE, 0xdead, &sSpongeBobLHand, &sSpongeBobLElbow, 0.7f, 1, { 255, 255, 128, 255 },
          { 196, 196, 64, 255 } },
        { FALSE, 0xdead, &sSpongeBobRHand, &sSpongeBobRElbow, 0.7f, 1, { 255, 255, 128, 255 },
          { 196, 196, 64, 255 } },
        { FALSE, 0xdead, &sSpongeBobLFoot, &sSpongeBobLKnee, 0.7f, 1, { 255, 255, 128, 255 },
          { 196, 196, 64, 255 } },
        { FALSE, 0xdead, &sSpongeBobRFoot, &sSpongeBobRKnee, 0.7f, 1, { 255, 255, 128, 255 },
          { 196, 196, 64, 255 } },
    },
    {
        { FALSE, 0xdead, &sSandyLHand, &sSandyLElbow, 0.8f, 1, { 192, 192, 255, 255 },
          { 192, 192, 255, 255 } },
        { FALSE, 0xdead, &sSandyRHand, &sSandyRElbow, 0.8f, 1, { 192, 192, 255, 255 },
          { 192, 192, 255, 255 } },
        { FALSE, 0xdead, &sSandyLFoot, &sSandyLKnee, 0.8f, 1, { 192, 192, 255, 255 },
          { 192, 192, 255, 255 } },
        { FALSE, 0xdead, &sSandyRFoot, &sSandyRKnee, 0.8f, 1, { 192, 192, 255, 255 },
          { 192, 192, 255, 255 } },
    },
    {
        { FALSE, 0xdead, &sPatrickLHand, &sPatrickLElbow, 0.6f, 1, { 255, 133, 133, 255 },
          { 255, 133, 133, 255 } },
        { FALSE, 0xdead, &sPatrickRHand, &sPatrickRElbow, 0.6f, 1, { 255, 133, 133, 255 },
          { 255, 133, 133, 255 } },
        { FALSE, 0xdead, &sPatrickLFoot, &sPatrickLKnee, 0.6f, 1, { 255, 133, 133, 255 },
          { 255, 133, 133, 255 } },
        { FALSE, 0xdead, &sPatrickRFoot, &sPatrickRKnee, 0.6f, 1, { 255, 133, 133, 255 },
          { 255, 133, 133, 255 } },
    },
};

static void HealthReset();

// WIP. Weird iterator code gen stuff isn't working
static void InvReset()
{
    globals.player.MaxHealth = 3;
    HealthReset();
    globals.player.Inv_Shiny = globals.player.g.InitialShinyCount;
    globals.player.Inv_Spatula = globals.player.g.InitialSpatulaCount;
    globals.player.Inv_PatsSock_Total = 0;

    if (globals.player.g.InitialShinyCount > SHINY_MAX)
    {
        globals.player.Inv_Shiny = SHINY_MAX;
    }

    for (U32 i = 0; i < LEVEL_COUNT; i++)
    {
        U32& maxsocks = globals.player.Inv_PatsSock_Max[i];
        globals.player.Inv_PatsSock[i] = 0;
        globals.player.Inv_LevelPickups[i] = 0;
        maxsocks = 0;
        const char* level_prefix = zSceneGetLevelPrefix(i);
        if (level_prefix == NULL)
        {
            continue;
        }

        const sock* s = patsock_totals;
        U32 level_mask = level_prefix[0] << 0x18 | level_prefix[1] << 0x10;
        for (; s->level != 0; s++)
        {
            if (level_mask == s->level)
            {
                maxsocks = s->total;
            }
        }
    }

    memcpy(&globals.player.g.PowerUp, &globals.player.g.InitialPowerUp,
           sizeof(globals.player.g.InitialPowerUp));
}

static void HealthReset()
{
    globals.player.Health = globals.player.MaxHealth;
}

static U32 RunAnyCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return (globals.player.JumpState == 0 || globals.player.JumpState == 1) &&
           globals.player.Speed == 2 && !surfSlickRatio && oob_state::oob_timer() < 0.0f;
}

static U32 RunCheck(xAnimTransition* tran, xAnimSingle* anim, void* param_3)
{
    return (globals.player.JumpState == 0 || globals.player.JumpState == 1) &&
           globals.player.Speed == 2 && !(globals.player.BadGuyNearTimer > 0.0f) &&
           !(globals.player.VictoryTimer > 0.0f) && !surfSlickRatio &&
           oob_state::oob_timer() < 0.0f;
}

static U32 RunStoicCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return (globals.player.JumpState == 0 || globals.player.JumpState == 1) &&
           globals.player.Speed == 2 && !(globals.player.BadGuyNearTimer > 0.0f) &&
           !(globals.player.VictoryTimer > 0.0f) && !surfSlickRatio &&
           oob_state::oob_timer() < 0.0f;
}

static U32 RunScaredCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return (globals.player.JumpState == 0 || globals.player.JumpState == 1) &&
           globals.player.Speed == 2 && globals.player.BadGuyNearTimer > 0.0f &&
           !(globals.player.VictoryTimer > 0.0f) && !surfSlickRatio &&
           oob_state::oob_timer() < 0.0f;
}

static U32 RunVictoryCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return (globals.player.JumpState == 0 || globals.player.JumpState == 1) &&
           globals.player.Speed == 2 && globals.player.VictoryTimer > 0.0f && !surfSlickRatio &&
           oob_state::oob_timer() < 0.0f;
}

static U32 RunSlipCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return (globals.player.JumpState == 0 || globals.player.JumpState == 1) &&
           globals.player.Speed >= 1 && surfSlickRatio;
}

static U32 RunOutOfWorldCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return (globals.player.JumpState == 0 || globals.player.JumpState == 1) &&
           globals.player.Speed >= 1 && !surfSlickRatio && oob_state::oob_timer() >= 0.0f;
}

static U32 WalkCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return (globals.player.JumpState == 0 || globals.player.JumpState == 1) &&
           globals.player.Speed == 1 && !surfSlickRatio && oob_state::oob_timer() < 0.0f;
}

static U32 WalkStoicCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return (globals.player.JumpState == 0 || globals.player.JumpState == 1) &&
           globals.player.Speed == 1 && !surfSlickRatio && !(globals.player.VictoryTimer > 0.0f) &&
           !(globals.player.BadGuyNearTimer > 0.0f) && oob_state::oob_timer() < 0.0f;
}

static U32 WalkVictoryCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return (globals.player.JumpState == 0 || globals.player.JumpState == 1) &&
           globals.player.Speed == 1 && globals.player.VictoryTimer > 0.0f && !surfSlickRatio &&
           oob_state::oob_timer() < 0.0f;
}

static U32 WalkScaredCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return (globals.player.JumpState == 0 || globals.player.JumpState == 1) &&
           globals.player.Speed == 1 && globals.player.BadGuyNearTimer > 0.0f &&
           !(globals.player.VictoryTimer > 0.0f) && !surfSlickRatio &&
           oob_state::oob_timer() < 0.0f;
}

static U32 IdleCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return (globals.player.JumpState == 0 || globals.player.JumpState == 1) &&
           globals.player.Speed == 0 && !surfSlickRatio;
}

static U32 IdleStoicCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return (globals.player.JumpState == 0 || globals.player.JumpState == 1) &&
           globals.player.Speed == 0 && !surfSlickRatio && !(globals.player.VictoryTimer > 0.0f) &&
           !(globals.player.BadGuyNearTimer > 0.0f);
}

static U32 IdleVictoryCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return (globals.player.JumpState == 0 || globals.player.JumpState == 1) &&
           globals.player.Speed == 0 && globals.player.VictoryTimer > 0.0f && !surfSlickRatio;
}

static U32 IdleScaredCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return (globals.player.JumpState == 0 || globals.player.JumpState == 1) &&
           globals.player.Speed == 0 && globals.player.BadGuyNearTimer > 0.0f &&
           !(globals.player.VictoryTimer > 0.0f) && !surfSlickRatio;
}

static U32 IdleSlipCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return (globals.player.JumpState == 0 || globals.player.JumpState == 1) &&
           globals.player.Speed == 0 && surfSlickRatio;
}

static U32 AnyMoveCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return globals.player.Speed != 0;
}

static U32 AnyStopCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return globals.player.Speed == 0;
}

static U32 SlipRunCB(xAnimTransition*, xAnimSingle*, void*)
{
    zEntPlayer_SNDPlay(ePlayerSnd_SlipLoop, 0.0f);
    return 0;
}

static U32 NoSlipCB(xAnimTransition*, xAnimSingle*, void*)
{
    zEntPlayer_SNDStop(ePlayerSnd_SlipLoop);
    return 0;
}

static U32 IdleCB(xAnimTransition*, xAnimSingle*, void*)
{
    zEntPlayer_SNDStop(ePlayerSnd_SlipLoop);
    idle_tmr = 0.0f;
    return 0;
}

static U32 InactiveCheck(xAnimTransition* tran, xAnimSingle*, void*)
{
    return idle_tmr >= 5.0f &&
           (tran->UserFlags & 0xffff) == player_idle_anim % (tran->UserFlags >> 0x10);
}

// Equivalent: sda reordering
static U32 InactiveCB(xAnimTransition*, xAnimSingle*, void*)
{
    idle_tmr = 0.0f;
    player_idle_anim = player_idle_anim + 1;
    return 0;
}

static U32 InactiveFinishedCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return idle_tmr >= 5.0f;
}

static U32 LandCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return globals.player.JumpState == 0;
}

static U32 LandTrackCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return globals.player.JumpState == 0 && globals.player.SlideTrackLand;
}

static U32 LandNoTrackCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return globals.player.JumpState == 0 && !globals.player.SlideTrackLand;
}

static U32 LandHighCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return globals.player.JumpState == 0 && globals.player.WasDJumping;
}

static U32 LandRunCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return globals.player.JumpState == 0 && globals.player.Speed >= 2;
}

static U32 LandWalkCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return globals.player.JumpState == 0 && globals.player.Speed == 1;
}

static U32 LandFastCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return globals.player.JumpState == 0 && globals.player.Speed >= 2;
}

static U32 LandNoTrackWalkCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return globals.player.JumpState == 0 && globals.player.Speed == 1 &&
           !globals.player.SlideTrackLand;
}

static U32 LandSlipIdleCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return globals.player.JumpState == 0 && globals.player.Speed == 0 && surfSlickRatio;
}

static U32 LandSlipRunCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return globals.player.JumpState == 0 && globals.player.Speed >= 1 && surfSlickRatio;
}

static U32 LandNoTrackFastCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return globals.player.JumpState == 0 && globals.player.Speed >= 2 &&
           !globals.player.SlideTrackLand;
}

static U32 LandNoTrackSlipRunCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return globals.player.JumpState == 0 && globals.player.Speed >= 1 &&
           !globals.player.SlideTrackLand && surfSlickRatio;
}

static U32 LandNoTrackSlipIdleCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return globals.player.JumpState == 0 && globals.player.Speed == 0 &&
           !globals.player.SlideTrackLand && surfSlickRatio;
}

static U32 LandCallback(xAnimTransition*, xAnimSingle*, void*)
{
    globals.player.WallJumpState = k_WALLJUMP_NOT;
    zCameraDisableWallJump(&globals.camera);
    return 0;
}

static U32 LandSlipRunCallback(xAnimTransition*, xAnimSingle*, void*)
{
    zEntPlayer_SNDPlay(ePlayerSnd_SlipLoop, 0.0f);
    globals.player.WallJumpState = k_WALLJUMP_NOT;
    zCameraDisableWallJump(&globals.camera);
    return 0;
}

static U32 SandyLandCB(xAnimTransition*, xAnimSingle*, void*)
{
    globals.player.Jump_CanDouble = 1;
    return 0;
}

static U32 BubbleSpinCheck(xAnimTransition*, xAnimSingle*, void*)
{
    if (globals.player.cheat_mode != 0)
    {
        return 0;
    }

    if (zEntTeleportBox_playerIn())
    {
        return 0;
    }

    return (!globals.player.ControlOff && globals.pad0->pressed & XPAD_BUTTON_TRIANGLE);
}

static U32 BubbleSpinCB(xAnimTransition*, xAnimSingle* anim, void*)
{
    anim->CurrentSpeed = 0.0f;
    zEntPlayer_SNDPlay(ePlayerSnd_BubbleWand, 0.0f);
    sPlayerAttackInAir++;
    zEntPlayer_SNDStop(ePlayerSnd_SlipLoop);
    return 0;
}

static U32 BubbleBashCheck(xAnimTransition*, xAnimSingle* anim, void*)
{
    if (globals.player.cheat_mode != 0)
    {
        return 0;
    }

    if (zEntTeleportBox_playerIn())
    {
        return 0;
    }

    if (sPlayerCollAdjust > 0.2f)
    {
        return 0;
    }

    return (!globals.player.ControlOff && globals.pad0->pressed & XPAD_BUTTON_SQUARE);
}

// probably equivalent: looks like sda relocation memes on sPlayerCollAdjust
static U32 BubbleBashCB(xAnimTransition*, xAnimSingle* anim, void*)
{
    zEntPlayer_SNDPlay(ePlayerSnd_BubbleBashStart, 0.0f);
    zEntPlayer_SNDStop(ePlayerSnd_SlipLoop);

    globals.player.ent.frame->vel.y = 0.0f;
    bbash_start_ht = globals.player.ent.frame->mat.pos.y + sPlayerCollAdjust;
    bbash_tmr = -globals.player.g.BBashDelay;
    bbash_end_tmr = 0.25f;
    globals.player.JumpState = 2;
    bbash_hit = 0;

    zCameraMinTargetHeightSet(bbash_start_ht);

    return 0;
}

static U32 BBashStrikeCheck(xAnimTransition*, xAnimSingle* anim, void*)
{
    return bbash_hit;
}

static U32 BBashStrikeCB(xAnimTransition*, xAnimSingle* anim, void*)
{
    globals.player.ent.frame->vel.y = 0.0f;
    return 0;
}

static U32 BBashToJumpCheck(xAnimTransition*, xAnimSingle* anim, void*)
{
    return bbash_tmr >= globals.player.g.BBashTime;
}

static U32 BubbleBounceCheck(xAnimTransition* tran, xAnimSingle* anim, void* param_3)
{
    if (globals.player.cheat_mode)
    {
        return false;
    }

    return (!globals.player.ControlOff && (globals.pad0->pressed & XPAD_BUTTON_O));
}

// equivalent: sda relocation memes
static U32 BubbleBounceCB(xAnimTransition* tran, xAnimSingle* anim, void* param_3)
{
    zCameraSetBbounce(true);

    globals.player.ent.frame->vel.x = 0.0f;
    globals.player.ent.frame->vel.y = 0.0f;
    globals.player.ent.frame->vel.z = 0.0f;

    tslide_inair_tmr = 0.0f;
    tslide_dbl_tmr = 0.0f;
    tslide_ground = 0;
    globals.player.SlideTrackDecay = 0.0f;
    bbounce_hit = 0;

    return 0;
}

static U32 BBounceAttackCB(xAnimTransition* tran, xAnimSingle* anim, void* param_3)
{
    globals.player.ent.frame->vel.y = -globals.player.g.BBounceSpeed;
    return 0;
}

static U32 BBounceStrikeCheck(xAnimTransition* tran, xAnimSingle* anim, void* param_3)
{
    return (globals.player.JumpState == 0 || globals.player.JumpState == 1);
}

static U32 BBounceStrikeCB(xAnimTransition* tran, xAnimSingle* anim, void* param_3)
{
    zEntPlayer_SNDStop(ePlayerSnd_BubbleBashStart);
    zEntPlayer_SNDPlay(ePlayerSnd_BounceStrike, 0.0f);
    zFX_SpawnBubbleSlam((xVec3*)&globals.player.ent.model->Mat->pos, 100, 0.15f, 12.0f, 2.0f);
    zCameraSetBbounce(false);
    zRumbleStart(SDR_Bounce);
    return 0;
}

static U32 BBounceToJumpCheck(xAnimTransition* tran, xAnimSingle* anim, void* param_3)
{
    return bbounce_hit;
}

static U32 BBounceToJumpCB(xAnimTransition* tran, xAnimSingle* anim, void* param_3)
{
    zEntPlayerJumpStart(&globals.player.ent, &globals.player.s->Jump);

    F32 startY = globals.player.ent.frame->mat.pos.y;
    startDouble = startY;
    startJump = startY;
    globals.player.CanJump = 0;
    globals.player.IsJumping = 1;
    globals.player.Jump_CanDouble = 1;
    globals.player.IsDJumping = 0;

    zCameraSetBbounce(false);
    zEntPlayer_SNDStop(ePlayerSnd_BubbleBashStart);
    zEntPlayer_SNDPlay(ePlayerSnd_BubbleBashHit1, 0.0f);
    return 0;
}

static U32 BbowlCheck(xAnimTransition* tran, xAnimSingle* anim, void* param_3)
{
    if (globals.player.cheat_mode)
    {
        return false;
    }

    if (zEntTeleportBox_playerIn())
    {
        return false;
    }

    return (!globals.player.ControlOff && ((globals.pad0->pressed & XPAD_BUTTON_O)) &&
            globals.player.g.PowerUp[0]);
}

static U32 BbowlCB(xAnimTransition* tran, xAnimSingle* anim, void* param_3)
{
    zEntPlayer_SNDPlay(ePlayerSnd_BowlWindup, 0.0f);
    zEntPlayer_SNDStop(ePlayerSnd_SlipLoop);

    xEntFrame* frame = globals.player.ent.frame;
    F32 x = (frame->mat.pos.x - frame->oldmat.pos.x) / last_update_dt;
    F32 z = (frame->mat.pos.z - frame->oldmat.pos.z) / last_update_dt;
    F32 speed2 = x * x + z * z;

    if (speed2 < globals.player.g.BubbleBowlMinSpeed * globals.player.g.BubbleBowlMinSpeed)
    {
        globals.player.bbowlInitVel = globals.player.g.BubbleBowlMinSpeed;
    }
    else
    {
        globals.player.bbowlInitVel = xsqrt(speed2);
    }

    frame->vel.x = globals.player.bbowlInitVel * globals.player.ent.model->Mat->at.x;
    frame->vel.z = globals.player.bbowlInitVel * globals.player.ent.model->Mat->at.z;

    sShouldBubbleBowl = 0;
    globals.player.IsBubbleBowling = 1;
    sBubbleBowlTimer = 0.0f;
    sBubbleBowlLastWindupTime = -1.0f;

    return 0;
}

static U32 BbowlWindupEndCheck(xAnimTransition* tran, xAnimSingle* anim, void* param_3)
{
    if (anim->Time < sBubbleBowlLastWindupTime && sShouldBubbleBowl)
    {
        return true;
    }
    sBubbleBowlLastWindupTime = anim->Time;
    return false;
}

static U32 BbowlTossEndCB(xAnimTransition* tran, xAnimSingle* anim, void* param_3)
{
    xEntBoulder_BubbleBowl(sBubbleBowlMultiplier);
    globals.player.IsBubbleBowling = false;
    zEntPlayer_SNDStop(ePlayerSnd_BowlWindup);
    zEntPlayer_SNDPlay(ePlayerSnd_BowlRelease, 0.0f);
    return false;
}

static U32 BbowlRecoverWalkCheck(xAnimTransition* tran, xAnimSingle* anim, void* param_3)
{
    return (anim->Time > globals.player.g.BubbleBowlMinRecoverTime &&
            WalkCheck(tran, anim, param_3));
}

static U32 BbowlRecoverRunCheck(xAnimTransition* tran, xAnimSingle* anim, void* param_3)
{
    return (anim->Time > globals.player.g.BubbleBowlMinRecoverTime &&
            RunCheck(tran, anim, param_3));
}

static U32 BbowlRecoverRunScaredCheck(xAnimTransition* tran, xAnimSingle* anim, void* param_3)
{
    return (anim->Time > globals.player.g.BubbleBowlMinRecoverTime &&
            RunScaredCheck(tran, anim, param_3));
}

static U32 BbowlRecoverRunVictoryCheck(xAnimTransition* tran, xAnimSingle* anim, void* param_3)
{
    return (anim->Time > globals.player.g.BubbleBowlMinRecoverTime &&
            RunVictoryCheck(tran, anim, param_3));
}

static U32 BbowlRecoverRunOutOfWorldCheck(xAnimTransition* tran, xAnimSingle* anim, void* param_3)
{
    return (anim->Time > globals.player.g.BubbleBowlMinRecoverTime &&
            RunOutOfWorldCheck(tran, anim, param_3));
}

static U32 BbowlRecoverRunSlipCheck(xAnimTransition* tran, xAnimSingle* anim, void* param_3)
{
    return (anim->Time > globals.player.g.BubbleBowlMinRecoverTime &&
            RunSlipCheck(tran, anim, param_3));
}

static U32 GooCheck(xAnimTransition* tran, xAnimSingle* anim, void* param_3)
{
    if (globals.player.ControlOff & 0x8000)
    {
        return false;
    }

    if (globals.player.cheat_mode)
    {
        return false;
    }

    return in_goo;
}

static U32 GooDeathCB(xAnimTransition* tran, xAnimSingle* anim, void* param_3)
{
    // Decompiled, but instructions are out of order?
    globals.player.Health = 0;
    globals.player.DamageTimer = 10.0f;
    zGooStopTide();
    sPlayerDiedLastTime = 1;
    zEntPlayerControlOff(CONTROL_OWNER_GLOBAL);
    return false;
}

static U32 Hit01Check(xAnimTransition* tran, xAnimSingle* anim, void* param_3)
{
    return (player_hit && player_hit_anim == 1);
}

static U32 Hit01CB(xAnimTransition* tran, xAnimSingle* anim, void* param_3)
{
    player_hit = 0;
    player_hit_anim = 2;
    return false;
}

static U32 Hit02Check(xAnimTransition* tran, xAnimSingle* anim, void* param_3)
{
    return (player_hit && player_hit_anim == 2);
}

static U32 Hit02CB(xAnimTransition* tran, xAnimSingle* anim, void* param_3)
{
    player_hit = 0;
    player_hit_anim = 3;
    return false;
}

static U32 Hit03Check(xAnimTransition* tran, xAnimSingle* anim, void* param_3)
{
    return (player_hit && player_hit_anim == 3);
}

static U32 Hit03CB(xAnimTransition* tran, xAnimSingle* anim, void* param_3)
{
    player_hit = 0;
    player_hit_anim = 4;
    return false;
}

static U32 Hit04Check(xAnimTransition* tran, xAnimSingle* anim, void* param_3)
{
    return (player_hit && player_hit_anim == 4);
}

static U32 Hit04CB(xAnimTransition* tran, xAnimSingle* anim, void* param_3)
{
    player_hit = 0;
    player_hit_anim = 5;
    return false;
}

static U32 Hit05Check(xAnimTransition* tran, xAnimSingle* anim, void* param_3)
{
    return (player_hit && player_hit_anim == 5);
}

static U32 Hit05CB(xAnimTransition* tran, xAnimSingle* anim, void* param_3)
{
    player_hit = 0;
    player_hit_anim = 1;
    return false;
}

static U32 Defeated01Check(xAnimTransition* tran, xAnimSingle* anim, void* param_3)
{
    // it seems like this is a useless but necessary function call
    zGameExtras_CheatFlags();
    return globals.player.Health == 0 && player_dead_anim % tran->UserFlags == 0;
}

static U32 Defeated02Check(xAnimTransition* tran, xAnimSingle* anim, void* param_3)
{
    zGameExtras_CheatFlags();
    return globals.player.Health == 0 && player_dead_anim % tran->UserFlags + 1 == 2;
}

static U32 Defeated03Check(xAnimTransition* tran, xAnimSingle* anim, void* param_3)
{
    zGameExtras_CheatFlags();
    return globals.player.Health == 0 && player_dead_anim % tran->UserFlags + 1 == 3;
}

static U32 Defeated04Check(xAnimTransition* tran, xAnimSingle* anim, void* param_3)
{
    zGameExtras_CheatFlags();
    return globals.player.Health == 0 && player_dead_anim % tran->UserFlags + 1 == 4;
}

static U32 Defeated05Check(xAnimTransition* tran, xAnimSingle* anim, void* param_3)
{
    zGameExtras_CheatFlags();
    return globals.player.Health == 0 && player_dead_anim % tran->UserFlags + 1 == 5;
}

// Equivalent: sda relocation meme
static U32 DefeatedCB(xAnimTransition* tran, xAnimSingle* anim, void* param_3)
{
    player_dead_anim++;

    if (gCurrentPlayer == eCurrentPlayerSpongeBob)
    {
        S32 cheats = zGameExtras_CheatFlags();
        if ((cheats & 0x2000000) || xurand() < 0.175f)
        {
            zShrapnelAsset* deathShrap =
                (zShrapnelAsset*)xSTFindAsset(xStrHash("spongebob_shrapnel"), NULL);
            if (deathShrap && deathShrap->initCB)
            {
                xEntHide(&globals.player.ent);
                deathShrap->initCB(deathShrap, globals.player.ent.model, NULL, NULL);
                globals.player.ent.frame->vel.x = 0.0f;
                globals.player.ent.frame->vel.y = 0.0f;
                globals.player.ent.frame->vel.z = 0.0f;
                globals.player.KnockBackTimer = 0.0f;
                globals.player.KnockIntoAirTimer = 0.0f;
            }
        }
    }

    return 0;
}

static U32 SpatulaGrabCheck(xAnimTransition*, xAnimSingle*, void*)
{
    // much different than PS2 version of this function
    return sSpatulaGrabbed;
}

S32 zEntPlayer_InBossBattle()
{
    return (globals.sceneCur->sceneID == 'B101' || // Robo Sandy
            globals.sceneCur->sceneID == 'B201' || // Robo Patrick
            globals.sceneCur->sceneID == 'B302' || // Robo Spongebob
            globals.sceneCur->sceneID == 'B303' // Brain Fight
    );
}

// Equivalent: scheduling
static U32 SpatulaGrabCB(xAnimTransition*, xAnimSingle*, void* data)
{
    sSpatulaGrabbed = 0;
    tslide_inair_tmr = 0.0f;
    tslide_dbl_tmr = 0.0f;
    globals.player.SlideTrackDecay = 0.0f;
    tslide_ground = 0;

    xEnt* ent = (xEnt*)data;
    ent->frame->vel.x = 0.0f;
    if (ent->frame->vel.y > 0.0f)
    {
        ent->frame->vel.y = 0.0f;
    }
    ent->frame->vel.z = 0.0f;

    globals.player.KnockBackTimer = 0.0f;
    globals.player.KnockIntoAirTimer = 0.0f;

    if (globals.autoSaveFeature)
    {
        if (zEntPlayer_InBossBattle())
        {
            gWaitingToAutoSave = 1;
        }
        else
        {
            zSaveLoadPreAutoSave(true);
        }
    }

    xCollis rcoll;
    xVec3 cam;
    xVec3 center;
    xRay3 r;
    rcoll.flags = 0;

    xVec3Copy(&center, &globals.player.ent.bound.cyl.center);

    xVec3Copy(&cam, &globals.camera.mat.pos);

    xVec3Copy(&r.origin, &center);
    xVec3Sub(&r.dir, &cam, &center);
    r.max_t = xVec3Length(&r.dir);
    F32 one_len = 1.0f / MAX(r.max_t, 0.00001f);
    xVec3SMul(&r.dir, &r.dir, one_len);
    r.flags = 0x800;

    xRayHitsScene(globals.sceneCur, &r, &rcoll);
    if ((rcoll.flags & 1) == 0)
    {
        zCameraSetReward(1);
    }

    zCameraDisableInput();
    F32 delay = 0.0f;
    if (gCurrentPlayer == eCurrentPlayerSpongeBob)
    {
        delay = 4.4f;
    }
    else if (gCurrentPlayer == eCurrentPlayerPatrick)
    {
        delay = 1.8f;
    }
    else if (gCurrentPlayer == eCurrentPlayerSandy)
    {
        delay = 1.43f;
    }

    zEntPlayer_SNDPlay(ePlayerSnd_PickupSpatulaComment, delay);

    return 0;
}

// equivalent: sda relocation scheduling
static U32 SpatulaGrabStopCB(xAnimTransition*, xAnimSingle*, void* data)
{
    S32 result;
    xBase* sendTo;

    idle_tmr = 0.0f;
    if (globals.autoSaveFeature)
    {
        if (zEntPlayer_InBossBattle())
        {
            gWaitingToAutoSave = 1;
        }
        else
        {
            if (zSaveLoad_DoAutoSave() < 0)
            {
                sendTo = zSceneFindObject(xStrHash("MNU4 AUTO SAVE FAILED"));
                if (sendTo)
                {
                    zEntEvent(sendTo, eEventVisible);
                }
            }

            sendTo = zSceneFindObject(xStrHash("SAVING GAME ICON UI"));
            if (sendTo)
            {
                zEntEvent(sendTo, eEventInvisible);
            }
            zSaveLoadPreAutoSave(false);
        }
    }

    zCameraEnableTracking(CO_REWARDANIM);
    xCameraSetFOV(&xglobals->camera, 75.0f);
    zCameraSetReward(0);
    zMusicSetVolume(1.0f, 0.75f);
    zCameraEnableInput();
    zEntPlayerControlOn(CONTROL_OWNER_REWARDANIM);
    return 0;
}

static U32 LCopterCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return (globals.player.JumpState && sLassoInfo->canCopter && !globals.player.ControlOff &&
            (globals.pad0->pressed & XPAD_BUTTON_X));
}

// Equivalent: sda relocation scheduling
static U32 LCopterCB(xAnimTransition*, xAnimSingle*, void* data)
{
    xEnt* ent = (xEnt*)data;

    zEntPlayer_SNDPlay(ePlayerSnd_Heli, 0.0f);
    zEntPlayer_SNDPlayStreamRandom(ePlayerStreamSnd_HeliComment1, ePlayerStreamSnd_HeliComment3,
                                   0.1f);

    globals.player.ent.frame->vel.x = 0.0f;
    globals.player.ent.frame->vel.y = -1.0f;
    globals.player.ent.frame->vel.z = 0.0f;

    sLassoInfo->copterTime = 5.0f;
    zLasso_InitTimer(sLasso, 0.25f);
    sLasso->flags = 0x1243;
    sLasso->tgRadius = 1.25f;
    xVec3AddScaled(&sLasso->crCenter, (xVec3*)&(ent->model->Mat->up), 0.25f);

    tslide_inair_tmr = 0.0f;
    tslide_dbl_tmr = 0.0f;
    tslide_ground = 0;
    globals.player.SlideTrackDecay = 0.0f;
    return 0;
}

// Equivalent: sda relocation scheduling
static U32 StopLCopterCB(xAnimTransition*, xAnimSingle*, void* data)
{
    zEntPlayer_SNDStop(ePlayerSnd_Heli);
    idle_tmr = 0.0f;
    sLassoInfo->copterTime = -1.0f;
    sLassoInfo->canCopter = 0;
    sLasso->flags = 0;
    return 0;
}

// Equivalent: static initializer scheduling (probably sda relocations?)
static void DoWallJumpCheck()
{
    sWallJumpResult = WallJumpResult_NoJump;
    xEnt* ent = &globals.player.ent;

    static F32 sAtdist = 0.65f;
    static F32 sSweptrad = 0.4f;
    static F32 sVerticalCos = 0.2588f;

    xVec3 start;
    start.x = ent->model->Mat->pos.x;
    start.y = ent->model->Mat->pos.y + ent->bound.cyl.r;
    start.z = ent->model->Mat->pos.z;

    // hack: compiler isn't calling operator=
    xVec3 end;
    end.operator=(start);
    end.x += ent->model->Mat->at.x * sAtdist;
    end.z += ent->model->Mat->at.z * sAtdist;

    xSweptSphere sws;
    xSweptSpherePrepare(&sws, &start, &end, sSweptrad);

    if (xSweptSphereToScene(&sws, globals.sceneCur, ent, 0x16))
    {
        xSweptSphereGetResults(&sws);

        xSurface* surf;
        if (sws.optr && sws.mptr)
        {
            surf = sws.mptr->Surf;
        }
        else
        {
            surf = zSurfaceGetSurface(sws.oid);
        }

        if (!surf)
        {
            return;
        }

        zSurfaceProps* surfaceProperties = (zSurfaceProps*)surf->moprops;

        if (!(surfaceProperties->asset->phys_flags & 0x20))
        {
            return;
        }

        if (xabs(sws.worldNormal.y) < sVerticalCos)
        {
            if (xVec3Dot(&sws.worldNormal, &sws.worldPolynorm) > 0.999f)
            {
                sWallNormal = sws.worldNormal;
                sWallCollisionSurface = surfaceProperties;
                sWallJumpResult = WallJumpResult_Jump;
            }
        }
    }
}

static float sTongueDblSpeedMult;

static U32 WallJumpLaunchCheck(class xAnimTransition*, class xAnimSingle*, void*)
{
    if (globals.player.ControlOff || !(globals.pad0->pressed & XPAD_BUTTON_X) ||
        !globals.player.IsJumping || globals.player.s->Wall.PeakHeight <= 0.0f)
    {
        return false;
    }
    return sWallJumpResult == WallJumpResult_Jump;
}

static U32 WallJumpLaunchCallback(class xAnimTransition*, class xAnimSingle*, void*)
{
    globals.player.WallJumpState = k_WALLJUMP_LAUNCH;
    zCameraEnableWallJump(&globals.camera, sWallNormal);
    sWallJumpResult = WallJumpResult_NoJump;
    return 0;
}

// Really strange non-matches here, seem unlike most things in this TU. Look equivalent though?
static U32 WallJumpCallback(class xAnimTransition*, class xAnimSingle*, void*)
{
    zJumpParam wallParam = globals.player.s->Wall;
    wallParam.PeakHeight *= sWallCollisionSurface->asset->walljump_scale_y;

    CalcJumpImpulse(&wallParam, NULL);
    zEntPlayerJumpStart(&globals.player.ent, &wallParam);
    zEntPlayer_SNDPlay(ePlayerSnd_Jump, 0.0f);

    xEntFrame* frame = globals.player.ent.frame;
    globals.player.IsDJumping = 0;
    globals.player.WallJumpState = k_WALLJUMP_FLIGHT;

    xVec3* velocity = &globals.player.ent.frame->vel;
    velocity->x = 0.0f;
    velocity->z = 0.0f;

    xVec3 u;
    xVec3SMul(&u, &sWallNormal,
              globals.player.s->WallJumpVelocity * sWallCollisionSurface->asset->walljump_scale_xz);
    xVec3Add(velocity, velocity, &u);
    xVec3Copy(&frame->dvel, &g_O3);
    xVec3Copy(&frame->oldvel, velocity);
    xMat3x3LookAt((xMat3x3*)globals.player.ent.model->Mat, &sWallNormal, &g_O3);
    xVec3Copy(&frame->dpos, &g_O3);

    frame->drot.angle = 0.0f;
    xVec3Copy(&frame->oldmat.pos, &frame->mat.pos);

    return 0;
}

static U32 WallJumpFlightLandCheck(class xAnimTransition*, class xAnimSingle*, void*)
{
    return sWallJumpResult == WallJumpResult_Jump;
}

static U32 WallJumpFlightLandCallback(xAnimTransition* tran, xAnimSingle* anim, void* param_3)
{
    globals.player.WallJumpState = k_WALLJUMP_LAND;
    return 0;
}

static U32 WallJumpLandFlightCheck(class xAnimTransition*, class xAnimSingle*, void*)
{
    return sWallJumpResult != WallJumpResult_Jump;
}

static U32 WallJumpLandFlightCallback(xAnimTransition* tran, xAnimSingle* anim, void* param_3)
{
    globals.player.WallJumpState = k_WALLJUMP_FLIGHT;
    return 0;
}

static U32 JumpCheck(xAnimTransition* tran, xAnimSingle* anim, void* param_3)
{
    return (globals.player.CanJump && !globals.player.ControlOff &&
            (globals.pad0->pressed & XPAD_BUTTON_X));
}

static void zEntPlayerJumpAddDriver(xEnt* ent);
static U32 JumpCB(class xAnimTransition*, class xAnimSingle*, void*)
{
    if (globals.player.cheat_mode)
    {
        return 0;
    }

    zEntPlayerJumpStart(&globals.player.ent, &globals.player.s->Jump);
    zEntPlayerJumpAddDriver(&globals.player.ent);
    zEntPlayer_SNDStop(ePlayerSnd_SlipLoop);
    zEntPlayer_SNDPlay(ePlayerSnd_Jump, 0.0f);
    F32 startY = globals.player.ent.frame->mat.pos.y;
    startDouble = startY;
    startJump = startY;
    globals.player.CanJump = 0;
    globals.player.IsJumping = 1;

    return 0;
}

static U32 JumpApexCheck(xAnimTransition*, xAnimSingle* anim, void*)
{
    return globals.player.ent.model == globals.player.model_sandy && anim->State &&
                   anim->State->Name &&
                   (strcmp(anim->State->Name, "DJumpStart01") == 0 ||
                    strcmp(anim->State->Name, "DJumpLift01") == 0) ?
               (globals.player.ent.frame->vel.y <= 5.0f) :
               (globals.player.ent.frame->vel.y <= 0.001f);
}

static U32 BounceCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return globals.player.Bounced == 1;
}

// Equivalent: sda relocation scheduling + regswap
static U32 BounceCB(xAnimTransition*, xAnimSingle*, void*)
{
    zCameraSetBbounce(true);
    globals.player.Bounced = 0;
    globals.player.Jump_CanDouble = 1;
    globals.player.Jump_CanFloat = 1;
    sLassoInfo->canCopter = 1;

    return 0;
}

static U32 BounceStopLCopterCB(xAnimTransition* tran, xAnimSingle* anim, void* param_3)
{
    StopLCopterCB(tran, anim, param_3);
    BounceCB(tran, anim, param_3);
    return 0;
}

static U32 DblJumpCheck(xAnimTransition*, xAnimSingle*, void*)
{
    if (globals.player.s->Double.PeakHeight <= 0.0f)
    {
        return 0;
    }

    if (globals.player.ControlOff || !(globals.pad0->pressed & XPAD_BUTTON_X) ||
        !globals.player.Jump_CanDouble)
    {
        return 0;
    }

    zEntPlayer_SNDPlay(ePlayerSnd_DoubleJump, 0.0f);

    return sWallJumpResult != WallJumpResult_Jump;
}

// Equivalent: regswaps and a sda relocation scheduling meme, may be cause some/all the swaps
static U32 DblJumpCB(xAnimTransition*, xAnimSingle*, void*)
{
    if (globals.player.cheat_mode)
    {
        return 0;
    }

    zEntPlayerJumpStart(&globals.player.ent, &globals.player.s->Double);
    globals.player.Jump_CanDouble = 0;
    startDouble = globals.player.ent.frame->mat.pos.y;
    globals.player.IsDJumping = 1;
    globals.player.Bounced = 0;
    sLasso->flags = NULL;

    if (tslide_inair_tmr)
    {
        F32 dirx;
        F32 dirz;
        F32 speed;
        F32 len2 = SQR(update_motion.x) + SQR(update_motion.z);

        if (xabs(len2 - 1.0f) <= 0.00001f)
        {
            dirx = update_motion.x;
            dirz = update_motion.z;
            speed = 1.0f;
        }
        else if (xabs(len2) <= 0.00001f)
        {
            dirx = 0.0f;
            dirz = 0.0f;
            speed = 0.0f;
        }
        else
        {
            speed = xsqrt(len2);
            F32 len_inv = 1.0f / speed;
            dirx = update_motion.x * len_inv;
            dirz = update_motion.z * len_inv;
        }

        speed /= update_dt;
        if (globals.player.g.SlideVelDblBoost *
                (dirx * globals.player.SlideTrackDir.x + dirz * globals.player.SlideTrackDir.z) >
            speed)
        {
            globals.player.ent.frame->vel.x += dirx * speed;
            globals.player.ent.frame->vel.z += dirz * speed;
            tslide_dbl_tmr = update_dt;
        }
    }
    return 0;
}

// equivalent: sda relocation optimization
static U32 TongueDblJumpCB(xAnimTransition* tran, xAnimSingle* anim, void* object)
{
    DblJumpCB(tran, anim, object);
    sTongueDblSpeedMult =
        (1.1f * (2.0f * anim->State->Data->Duration - anim->Time)) / anim->State->Data->Duration;
    anim->CurrentSpeed *= sTongueDblSpeedMult;
    return 0;
}

static U32 TongueDblSpinCB(xAnimTransition*, xAnimSingle* anim, void*)
{
    anim->CurrentSpeed *= sTongueDblSpeedMult;
    return 0;
}

static U32 FallCheck(xAnimTransition*, xAnimSingle* anim, void*)
{
    return !((anim && anim->State &&
              (strcmp(anim->State->Name, "LCopter01") == 0 ||
               strcmp(anim->State->Name, "LCopterHeadUp01") == 0)) &&
             (globals.player.ControlOff == 0 && (globals.pad0->on & XPAD_BUTTON_X) &&
              sLassoInfo->copterTime > 0.0f)) &&
           globals.player.JumpState != 0 && globals.player.JumpState != 1;
}

static U32 BoulderRollMoveCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return bvTimeToIdle > 0.0f && boulderVehicle;
}

static U32 BoulderRollIdleCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return bvTimeToIdle <= 0.0f && boulderVehicle;
}

static U32 BoulderRollCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return boulderRollShouldStart && boulderVehicle;
}

static U32 BoulderRollWindupCB(xAnimTransition*, xAnimSingle*, void*)
{
    zEntPlayer_SNDPlay(ePlayerSnd_BoulderStart, 0.0f);
    zEntPlayer_SNDPlayStreamRandom(ePlayerStreamSnd_SpongeBallComment1,
                                   ePlayerStreamSnd_SpongeBallComment3, 0.2f);
    zEntPlayer_SNDStop(ePlayerSnd_SlipLoop);
    return 0;
}

static void zEntPlayer_BoulderVehicleUpdate(xEnt* ent, xScene* sc, F32 dt);
static void zEntPlayer_BoulderVehicleMove(xEnt*, xScene*, F32, xEntFrame* frame);
static void zEntPlayer_BoulderVehicleRender(zEnt*);

static U32 BoulderRollCB(xAnimTransition*, xAnimSingle*, void*)
{
    xEntHide(&globals.player.ent);
    xEntBoulder_Reset(boulderVehicle, globals.sceneCur);
    xVec3Copy((xVec3*)&boulderVehicle->model->Mat->pos,
              (xVec3*)&globals.player.ent.model->Mat->pos);

    boulderVehicle->model->Mat->pos.y += boulderVehicle->bound.sph.r;
    xVec3SubFrom((xVec3*)&boulderVehicle->model->Mat->pos,
                 (xVec3*)&boulderVehicle->model->Data->boundingSphere.center);

    globals.player.ent.update = zEntPlayer_BoulderVehicleUpdate;
    globals.player.ent.move = zEntPlayer_BoulderVehicleMove;
    globals.player.ent.render = (xEntRenderCallback)zEntPlayer_BoulderVehicleRender;
    boulderVehicle->vel.y = 0.0f;
    boulderVehicle->vel.x = globals.player.PredictCurrDir.x * globals.player.PredictCurrVel;
    boulderVehicle->vel.z = globals.player.PredictCurrDir.z * globals.player.PredictCurrVel;
    boulderVehicle->rotVec.x = boulderVehicle->vel.z;
    boulderVehicle->rotVec.y = 0.0f;
    boulderVehicle->rotVec.z = -boulderVehicle->vel.x;

    xVec3Normalize(&boulderVehicle->rotVec, &boulderVehicle->rotVec);
    boulderVehicle->angVel = xVec3Length(&boulderVehicle->vel) / boulderVehicle->bound.sph.r;

    xVec3Copy((xVec3*)&boulderVehicle->model->Mat->right,
              (xVec3*)&globals.player.ent.model->Mat->right);
    xVec3Copy((xVec3*)&boulderVehicle->model->Mat->at, (xVec3*)&globals.player.ent.model->Mat->at);
    xVec3Copy((xVec3*)&boulderVehicle->model->Mat->up, (xVec3*)&globals.player.ent.model->Mat->up);

    xParEmitterCustomSettings info;
    if (gPTankDisable)
    {
        info.custom_flags = 0x35e;
        xVec3Copy(&info.pos, (xVec3*)&boulderVehicle->model->Mat->pos);
        xVec3Copy(&info.vel, (xVec3*)&boulderVehicle->vel);

        if (xVec3Normalize(&info.vel, &info.vel) < 0.00001f)
        {
            info.vel.x = 0.0f;
            info.vel.y = 3.0f;
            info.vel.z = 0.0f;
        }
        else
        {
            xVec3SMulBy(&info.vel, 3.0f);
        }

        info.vel_angle_variation = DEG2RAD(270);
        info.rate.set(3000.0f, 3000.0f, 1.0f, 0.0f);
        info.life.set(0.75f, 0.75f, 1.0f, 0.0f);
        info.size_birth.set(0.25f, 0.25f, 1.0f, 0.0f);
        info.size_death.set(0.5f, 0.5f, 1.0f, 0.0f);

        xParEmitterEmitCustom(sEmitSpinBubbles, update_dt, &info);
        xVec3AddScaled(&info.pos, &boulderVehicle->vel, 10.0f * update_dt);
        xParEmitterEmitCustom(sEmitSpinBubbles, update_dt, &info);
    }
    else
    {
        zFX_SpawnBubbleHit((xVec3*)&boulderVehicle->model->Mat->pos, 50);
    }

    boulderRollShouldEnd = 0;
    zEntEvent(&globals.player.ent, eEventSpongeballOn);
    xEntBeginUpdate(boulderVehicle, globals.sceneCur, 0.00001f);
    xEntEndUpdate(boulderVehicle, globals.sceneCur, 0.00001f);
    xEntBoulder_RealBUpdate(boulderVehicle, &boulderVehicle->frame->mat.pos);
    boulderVehicle->lightKit = globals.player.ent.lightKit;
    boulderVehicle->model->LightKit = globals.player.ent.lightKit;

    return 0;
}

static U32 BoulderRollDoneCheck()
{
    if (globals.sceneCur->sceneID == 'PG12')
    {
        return 0;
    }

    return !globals.player.ControlOff &&
               (globals.pad0->pressed &
                (XPAD_BUTTON_TRIANGLE | XPAD_BUTTON_SQUARE | XPAD_BUTTON_O | XPAD_BUTTON_X)) ||
           boulderRollShouldEnd;
}

void zEntPlayer_Update(xEnt* ent, xScene* sc, F32 dt);
static void zEntPlayer_Move(xEnt*, xScene*, F32, xEntFrame* frame);
void zEntPlayer_Render(zEnt* ent);

// Equivalent: sda relocation and some float thing before info.rate.set
static U32 BoulderRollDoneCB()
{
    xEntShow(&globals.player.ent);
    zEntPlayer_SNDPlay(ePlayerSnd_BoulderEnd, 0.0f);

    xParEmitterCustomSettings info;
    if (gPTankDisable)
    {
        info.custom_flags = 0x35e;
        xVec3Copy(&info.pos, (xVec3*)&boulderVehicle->model->Mat->pos);
        xVec3Copy(&info.vel, (xVec3*)&boulderVehicle->vel);

        if (xVec3Normalize(&info.vel, &info.vel) < 0.00001f)
        {
            info.vel.x = 0.0f;
            info.vel.y = 3.0f;
            info.vel.z = 0.0f;
        }
        else
        {
            xVec3SMulBy(&info.vel, 3.0f);
        }

        info.vel_angle_variation = DEG2RAD(270);
        info.rate.set(3000.0f, 3000.0f, 1.0f, 0.0f);
        info.life.set(0.75f, 0.75f, 1.0f, 0.0f);
        info.size_birth.set(0.25f, 0.25f, 1.0f, 0.0f);
        info.size_death.set(0.5f, 0.5f, 1.0f, 0.0f);

        xParEmitterEmitCustom(sEmitSpinBubbles, update_dt, &info);
        xVec3AddScaled(&info.pos, &boulderVehicle->vel, 5.0f * update_dt);
        xParEmitterEmitCustom(sEmitSpinBubbles, update_dt, &info);
    }
    else
    {
        zFX_SpawnBubbleHit((xVec3*)&boulderVehicle->model->Mat->pos, 50);
    }

    globals.player.ent.update = zEntPlayer_Update;
    globals.player.ent.move = zEntPlayer_Move;
    globals.player.ent.render = (xEntRenderCallback)zEntPlayer_Render;

    xEntBoulder_Kill(boulderVehicle);
    boulderRollShouldStart = 0;

    zEntEvent(&globals.player.ent, eEventSpongeballOff);
    idle_tmr = 0.0f;

    return 0;
}

static U32 SlideTrackCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return globals.player.SlideTrackSliding & 1;
}

static U32 SlideTrackCB(xAnimTransition*, xAnimSingle*, void*)
{
    sLasso->flags = 0;
    globals.player.SlideTrackLean = 0.0f;

    if (globals.player.Health != 0 && sPlayerSndID[gCurrentPlayer][ePlayerSnd_SlideLoop] == 0)
    {
        zEntPlayer_SNDPlay(ePlayerSnd_SlideLoop, 0.0f);
    }

    if (gCurrentPlayer == eCurrentPlayerSandy)
    {
        globals.player.Jump_CanDouble = 1;
    }

    zEntPlayerKillCarry();
    zEntPlayer_SNDStop(ePlayerSnd_SlipLoop);

    return 0;
}

static U32 NoslideTrackCB(xAnimTransition*, xAnimSingle*, void*)
{
    idle_tmr = 0.0f;
    return 0;
}

static U32 NoslideTrackCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return (globals.player.SlideTrackSliding & 1) == 0 && globals.player.JumpState == 0;
}

static U32 TrackFallCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return (globals.player.SlideTrackSliding & 1) == 0 && globals.player.JumpState != 0;
}

static U32 TrackFallCB(xAnimTransition*, xAnimSingle*, void*)
{
    globals.player.JumpState = 2;
    globals.player.CanJump = 1;
    return 0;
}

static U32 TrackPrefallJumpCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return globals.player.CanJump && !globals.player.ControlOff &&
           globals.pad0->pressed & XPAD_BUTTON_X && tslide_inair_tmr != 0.0f &&
           tslide_inair_tmr < 0.25f;
}

static U32 LedgeGrabCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return globals.player.s->ledge.tmr == -1.0f;
}

static U32 LedgeGrabCB(xAnimTransition*, xAnimSingle*, void* object)
{
    zEntPlayer_SNDStop(ePlayerSnd_Heli);
    globals.player.s->ledge.tmr = 0.00001f;
    globals.player.s->ledge.startrot = ((xEnt*)object)->frame->rot.angle;

    F32 endrot = globals.player.s->ledge.endrot;
    F32 startrot = globals.player.s->ledge.startrot;
    if (startrot > endrot + PI)
    {
        globals.player.s->ledge.startrot -= 2 * PI;
    }
    else if (startrot < endrot - PI)
    {
        globals.player.s->ledge.startrot += 2 * PI;
    }

    sLasso->flags = 0;
    xCameraDoCollisions(0, 2);
    return 0;
}

// Equivalent: sda relocation scheduling
static U32 LedgeFinishCB(xAnimTransition*, xAnimSingle*, void* object)
{
    idle_tmr = 0.0f;
    globals.player.JumpState = 1;
    globals.player.JumpTimer = 0.0f;
    xCameraDoCollisions(1, 2);
    return 0;
}

static U32 PatrickGrabCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return sGrabFound != 0;
}

static U32 PatrickGrabFailed(xAnimTransition*, xAnimSingle*, void*)
{
    return sGrabFailed != 0;
}

static U32 PatrickGrabKill(xAnimTransition*, xAnimSingle*, void*)
{
    return globals.player.carry.grabbed == NULL;
}

static U32 PatrickGrabThrow(xAnimTransition*, xAnimSingle*, void*)
{
    if (globals.player.cheat_mode)
    {
        return 0;
    }

    return !globals.player.ControlOff && globals.pad0->pressed & XPAD_BUTTON_O;
}

static U32 PatrickAttackCheck(xAnimTransition*, xAnimSingle*, void*)
{
    if (globals.player.cheat_mode || zEntTeleportBox_playerIn())
    {
        return 0;
    }

    return !globals.player.ControlOff && globals.pad0->pressed & XPAD_BUTTON_TRIANGLE;
}

static U32 PatrickStunCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return !globals.player.ControlOff && globals.pad0->pressed & XPAD_BUTTON_O;
}

// Equivalent: scheduling
static U32 PatrickMeleeCB(xAnimTransition*, xAnimSingle*, void*)
{
    globals.player.DoMeleeCheck = 1;
    zEntPlayer_SNDPlay(ePlayerSnd_BellyMelee, 0.0f);
    return 0;
}

static U32 PatrickGrabCB(xAnimTransition* tran, xAnimSingle*, void*)
{
    zEntPlayer_SNDPlay(ePlayerSnd_Lift1, 0.0f);
    if ((xrand() & 3) == 3)
    {
        zEntPlayer_SNDPlayStream(ePlayerStreamSnd_Lift1);
    }

    globals.player.carry.grabbed = sGrabFound;
    globals.player.carry.grabTarget = 1;
    globals.player.carry.grabYclear = 0;

    if (sGrabFound->baseType == eBaseTypeBoulder)
    {
        globals.player.carry.targetRot =
            xatan2(sGrabFound->bound.sph.center.x - globals.player.ent.frame->mat.pos.x,
                   sGrabFound->bound.sph.center.z - globals.player.ent.frame->mat.pos.z);
    }
    else
    {
        globals.player.carry.targetRot =
            xatan2(sGrabFound->model->Mat->pos.x - globals.player.ent.frame->mat.pos.x,
                   sGrabFound->model->Mat->pos.z - globals.player.ent.frame->mat.pos.z);
    }

    globals.player.carry.grabLerpLast = 0.0f;

    xAnimState* stat =
        xAnimTableGetState(globals.player.ent.model->Anim->Table, "Carry_PickupItem");
    if (stat)
    {
        xVec3 tmptran;
        xQuat tmpquat;
        iAnimEval(stat->Data->RawData[0], 0.03336667f, 1, &tmptran, &tmpquat);

        xMat4x3 objMat;
        xQuatToMat(&tmpquat, &objMat);
        xMat4x3 targetMat;
        objMat.pos = tmptran;
        xMat3x3Rot(&targetMat, &g_Y3, globals.player.carry.targetRot);
        targetMat.pos = globals.player.ent.frame->mat.pos;

        xMat4x3Mul(&objMat, &objMat, &targetMat);

        if (globals.player.carry.grabbed->baseType == '/')
        {
            globals.player.carry.grabOffset.x = objMat.pos.x - sGrabFound->bound.sph.center.x;
            globals.player.carry.grabOffset.y =
                sGrabFound->bound.sph.r + (objMat.pos.y - sGrabFound->bound.sph.center.y);
            globals.player.carry.grabOffset.z = objMat.pos.z - sGrabFound->bound.sph.center.z;
        }
        else
        {
            globals.player.carry.grabOffset.x = objMat.pos.x - sGrabFound->model->Mat->pos.x;
            globals.player.carry.grabOffset.y = objMat.pos.y - sGrabFound->model->Mat->pos.y;
            globals.player.carry.grabOffset.z = objMat.pos.z - sGrabFound->model->Mat->pos.z;
        }
    }
    else
    {
        globals.player.carry.grabOffset.x = 0.0f;
        globals.player.carry.grabOffset.y = 0.0f;
        globals.player.carry.grabOffset.z = 0.0f;
    }

    xMat3x3Rot(&globals.player.carry.spin, &g_Y3, -globals.player.carry.targetRot);
    xMat3x3Mul(&globals.player.carry.spin, (xMat3x3*)sGrabFound->model->Mat,
               &globals.player.carry.spin);
    xVec3Init(&globals.player.carry.spin.pos, 0.0f, 0.0f, 0.0f);

    sGrabFound->chkby &= ~XENT_COLLTYPE_PLYR;

    zThrown_AddFruit(sGrabFound);

    if (sGrabFound->baseType == eBaseTypeNPC)
    {
        ((zNPCCommon*)sGrabFound)->SetCarryState(zNPCCARRY_PICKUP);
    }

    return 0;
}

namespace
{
    static U32 TalkCheck(xAnimTransition* anim, xAnimSingle*, void*)
    {
        return anim->UserFlags == player_talk.anim;
    }

    static U32 TalkDoneCheck(xAnimTransition* anim, xAnimSingle*, void*)
    {
        return anim->UserFlags != player_talk.anim;
    }

    static void speak_update(F32 dt)
    {
        if (player_talk.anim == -1)
        {
            return;
        }

        if (player_talk.time < 0.2f || xSndIsPlaying(player_talk.sndid) != 0)
        {
            player_talk.time += dt;
            float jawval = xJaw_EvalData(player_talk.data, player_talk.time);
            globals.player.ent.model->Anim->Single->BilinearLerp[0] = jawval;
        }
        else
        {
            zEntPlayerSpeakStop();
        }
    }
} // namespace

// WIP, not equivalent
void zEntPlayerSpeakStart(U32 sndid, U32, S32 anim)
{
    zEntPlayerSpeakStop();

    player_talk.data = xJaw_FindData(sndid);
    if (player_talk.data)
    {
        player_talk.sndid = sndid;
        player_talk.time = 0.0f;
        if (anim < 0 || anim >= globals.player.s->talk_anims)
        {
            U8 filter_size;
            U8* filter = globals.player.s->talk_filter;
            filter_size = globals.player.s->talk_filter_size;
            U32 which = xrand() >> 13;
            player_talk.anim = filter[which % filter_size];
        }
        else
        {
            player_talk.anim = anim;
        }
    }
}

// Equiavlent: sda scheduling reorder
void zEntPlayerSpeakStop()
{
    player_talk.anim = -1;
    globals.player.ent.model->Anim->Single->BilinearLerp[0] = 0.0f;
}

// Close, some float mismatches + regswaps
static xEnt* GetPatrickTarget(xEnt* ent)
{
    xEnt* result = NULL;
    zPlatform* plat =
        ent->collis->colls[0].flags & 1 ? (zPlatform*)ent->collis->colls[0].optr : NULL;

    if (plat && plat->baseType == eBaseTypePlatform && plat->plat_flags & 2)
    {
        xCollis* coll;
        xVec3 relpos;
        xMat4x3Tolocal(&relpos, (xMat4x3*)plat->model->Mat, (xVec3*)&ent->model->Mat->pos);

        relpos.z -= 2.0f;
        if (SQR(relpos.x) + SQR(relpos.z) < 0.5625f)
        {
            xVec3 worldpos;
            worldpos.x = 0.0f;
            worldpos.y = 1.229f;
            worldpos.z = -2.0f;
            xMat4x3Toworld(&worldpos, (xMat4x3*)plat->model->Mat, &worldpos);

            if (ent->model->Mat->at.x * (worldpos.x - ent->model->Mat->pos.x) +
                    ent->model->Mat->at.z * (worldpos.z - ent->model->Mat->pos.z) >
                0.0f)
            {
                globals.player.carry.targetRot =
                    xatan2(worldpos.x - globals.player.ent.frame->mat.pos.x,
                           worldpos.z - globals.player.ent.frame->mat.pos.z);
                globals.player.carry.throwTargetRotRate =
                    globals.player.carry.targetRot - ent->frame->rot.angle;
                CLAMP_ANGLE(globals.player.carry.throwTargetRotRate);
                globals.player.carry.throwTargetRotRate /= 0.2f;
                return plat;
            }
        }
    }

    U32 i;
    F32 bestTargetDot = -1.0f;
    xVec3* bestTargetPos;
    zScene* zsc = globals.sceneCur;
    S32 grabbedIsFruit = zThrown_IsFruit(globals.player.carry.grabbed, NULL);
    for (i = 0; i < zsc->num_ents; i++)
    {
        xEnt* tgtent = zsc->ents[i];
        if (tgtent == globals.player.carry.grabbed || (tgtent->flags & 1) == 0)
        {
            continue;
        }
        F32 maxHeight = globals.player.carry.throwMaxHeight;

        if (tgtent->baseType == eBaseTypeStatic)
        {
            if (!grabbedIsFruit || !(tgtent->moreFlags & 0x8))
            {
                continue;
            }
            if (zThrown_IsFruit(tgtent, NULL) == 0)
            {
                continue;
            }
            maxHeight = globals.player.carry.throwMaxStack;
        }
        else if (tgtent->baseType == eBaseTypeDestructObj)
        {
            if (((zEntDestructObj*)tgtent)->throw_target == 0)
            {
                continue;
            }
        }
        else if (tgtent->baseType == eBaseTypeNPC)
        {
            U32 t = ((xNPCBasic*)tgtent)->SelfType();
            if (t == NPC_TYPE_JELLYPINK || t == NPC_TYPE_JELLYBLUE ||
                t == NPC_TYPE_KINGNEPTUNE || t == NPC_TYPE_MIMEFISH)
            {
                continue;
            }
        }
        else if (tgtent->baseType == eBaseTypeButton)
        {
            if (zThrown_IsFruit(globals.player.carry.grabbed, NULL))
            {
                if ((((_zEntButton*)tgtent)->basset->buttonActFlags & (0x10000 | 0x80)) == 0)
                {
                    continue;
                }
            }
            else if (globals.player.carry.grabbed->baseType == eBaseTypeNPC &&
                     ((xNPCBasic*)globals.player.carry.grabbed)->SelfType() == NPC_TYPE_TIKI_STONE)
            {
                if ((((_zEntButton*)tgtent)->basset->buttonActFlags & (0x2000 | 0x40)) == 0)
                {
                    continue;
                }
            }
            else
            {
                if ((((_zEntButton*)tgtent)->basset->buttonActFlags & 0x40) == 0)
                {
                    continue;
                }
            }
        }
        else
        {
            continue;
        }

        F32 dx = tgtent->model->Mat->pos.x - ent->model->Mat->pos.x;
        F32 dy = tgtent->model->Mat->pos.y - ent->model->Mat->pos.y;
        F32 dz = tgtent->model->Mat->pos.z - ent->model->Mat->pos.z;
        if (SQR(dx) + SQR(dz) >= SQR(globals.player.carry.throwMaxDist) ||
            dy <= globals.player.carry.throwMinHeight || dy >= maxHeight ||
            SQR(dx) + SQR(dz) <= SQR(globals.player.carry.throwMinDist))
        {
            continue;
        }

        // cos of angle between (dx, 0, dz) and at (at should already be normalized)
        F32 ddot =
            (dx * ent->model->Mat->at.x + dz * ent->model->Mat->at.z) / xsqrt(SQR(dx) + SQR(dz));

        if (ddot < globals.player.carry.throwMaxCosAngle)
        {
            continue;
        }

        if (bestTargetDot != -1.0f)
        {
            if (tgtent->model->Mat->pos.y > bestTargetPos->y)
            {
                if (ddot + 0.05f < bestTargetDot)
                {
                    continue;
                }
            }
            else
            {
                if (ddot - 0.05f < bestTargetDot)
                {
                    continue;
                }
            }
        }
        result = tgtent;

        globals.player.carry.targetRot =
            xatan2(tgtent->model->Mat->pos.x - globals.player.ent.frame->mat.pos.x,
                   tgtent->model->Mat->pos.z - globals.player.ent.frame->mat.pos.z);

        globals.player.carry.throwTargetRotRate =
            globals.player.carry.targetRot - ent->frame->rot.angle;

        CLAMP_ANGLE(globals.player.carry.throwTargetRotRate);

        globals.player.carry.throwTargetRotRate /= 0.2f;
        bestTargetPos = (xVec3*)&tgtent->model->Mat->pos;
        bestTargetDot = ddot;
    }
    return result;
}

static U32 PatrickGrabThrowCB(xAnimTransition*, xAnimSingle*, void* object)
{
    zEntPlayer_SNDPlay(ePlayerSnd_Throw, 0.0f);
    zEnt* ent = (zEnt*)object;
    if (gReticleTarget && sTypeOfTarget == 3)
    {
        globals.player.carry.throwTarget = gReticleTarget;
    }
    else
    {
        globals.player.carry.throwTarget = GetPatrickTarget(ent);
    }

    globals.player.carry.flyingToTarget = NULL;
    if (globals.player.carry.grabbed && globals.player.carry.grabbed->baseType == eBaseTypeNPC)
    {
        ((zNPCCommon*)globals.player.carry.grabbed)->SetCarryState(zNPCCARRY_THROW);
    }

    return 0;
}

static class zNPCLassoInfo* sCurrentNPCInfo;
void zEntPlayer_LassoNotify(en_LASSO_EVENT event)
{
    switch (event)
    {
    case LASS_EVNT_GRABEND:
        zLasso_SetGuide(sCurrentNPCInfo->lassoee, sCurrentNPCInfo->holdGuideAnim);
        break;
    case LASS_EVNT_ABORT:
        globals.player.lassoInfo.lasso.flags = 0;
        globals.player.lassoInfo.target = NULL;
        break;
    }
}

static unsigned int sShouldMelee;
U32 sandyHitMax;
U32 patrickHitMax;
static U32 MeleeCheck(xAnimTransition*, xAnimSingle* anim, void*)
{
    if (!sShouldMelee)
    {
        return 0;
    }

    if (strcmp(anim->State->Name, "DJumpApex01") == 0 && anim->Time < 0.3f)
    {
        return 0;
    }

    return 1;
}

static U32 LassoStartCheck(xAnimTransition*, xAnimSingle*, void*)
{
    xNPCBasic* npc = (xNPCBasic*)sLassoInfo->target;

    if (npc != NULL)
    {
        if (npc->baseType == 0x2b)
        {
            if ((npc->SelfType() & 0xffffff00) != 0x4e545400)
            {
                return ((zNPCCommon*)sLassoInfo->target)->GimmeLassInfo() != NULL;
            }
        }

        return 1;
    }

    return 0;
}

static U32 LassoLostTargetCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return !sLassoInfo->target;
}

static U32 LassoStraightToDestroyCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return sLasso->flags & (1 << 11);
}

static U32 LassoAboutToDestroyCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return 0;
}

static U32 LassoDestroyCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return sLasso->flags & (1 << 11);
}

static U32 LassoReyankCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return 0;
}

static U32 LassoFailIdleSlipCheck(xAnimTransition* tran, xAnimSingle* anim, void* data)
{
    return !sLassoInfo->target && IdleSlipCheck(tran, anim, data);
}

static U32 LassoFailIdleCheck(xAnimTransition* tran, xAnimSingle* anim, void* data)
{
    return !sLassoInfo->target && IdleCheck(tran, anim, data);
}

static U32 LassoFailWalkCheck(xAnimTransition* tran, xAnimSingle* anim, void* data)
{
    return !sLassoInfo->target && WalkCheck(tran, anim, data);
}

static U32 LassoFailRunCheck(xAnimTransition* tran, xAnimSingle* anim, void* data)
{
    return !sLassoInfo->target && RunAnyCheck(tran, anim, data);
}

static U32 LassoFailRunOutOfWorldCheck(xAnimTransition* tran, xAnimSingle* anim, void* data)
{
    return !sLassoInfo->target && RunOutOfWorldCheck(tran, anim, data);
}

static U32 LassoFailRunSlipCheck(xAnimTransition* tran, xAnimSingle* anim, void* data)
{
    return !sLassoInfo->target && RunSlipCheck(tran, anim, data);
}

// Equivalent: sda relocation scheduling
static U32 JumpMeleeCB(xAnimTransition*, xAnimSingle*, void*)
{
    zEntPlayer_SNDPlay(ePlayerSnd_Kick, 0.0f);
    if ((xrand() & 3) == 3)
    {
        zEntPlayer_SNDPlayStreamRandom(ePlayerStreamSnd_KickComment1, ePlayerStreamSnd_KickComment3,
                                       0.0f);
    }

    globals.player.ent.frame->vel.y *= 0.8f;
    sShouldMelee = 0;
    sPlayerAttackInAir++;
    return 0;
}

// Equivalent: sda relocation scheduling
static U32 MeleeCB(xAnimTransition*, xAnimSingle*, void*)
{
    zEntPlayer_SNDPlay(ePlayerSnd_Chop, 0.0f);
    if ((xrand() & 3) == 3)
    {
        zEntPlayer_SNDPlayStreamRandom(ePlayerStreamSnd_ChopComment1, ePlayerStreamSnd_ChopComment3,
                                       0.0f);
    }

    sLassoInfo->target = NULL;
    sShouldMelee = 0;
    sPlayerAttackInAir++;
    return 0;
}

static U32 LassoStartCB(xAnimTransition*, xAnimSingle*, void* object)
{
    zEntPlayer_SNDPlay(ePlayerSnd_Heli, 0.0f);
    sLassoInfo->swingTarget = NULL;

    xEnt* ent = (xEnt*)object;
    zNPCCommon* npc = (zNPCCommon*)sLassoInfo->target;
    if (sLassoInfo->target->baseType == eBaseTypeNPC && (npc->SelfType() & 0xffffff00) != 'NTT\0')
    {
        sLassoInfo->targetGuide = 1;
        sCurrentNPCInfo = npc->GimmeLassInfo();
        npc->LassoNotify(LASS_EVNT_BEGIN);
        zLasso_SetGuide(npc, sCurrentNPCInfo->grabGuideAnim);
    }
    else
    {
        sLassoInfo->targetGuide = NULL;
    }

    zLasso_InitTimer(sLasso, 0.125f);
    sLasso->flags = 0x12c3;
    sLasso->tgRadius = 1.25f;

    xVec3AddScaled(&sLasso->crCenter, (xVec3*)&ent->model->Mat->up, 0.5f);
    sLassoInfo->lassoRot =
        xatan2(sLassoInfo->target->model->Mat->pos.x - globals.player.ent.frame->mat.pos.x,
               sLassoInfo->target->model->Mat->pos.z - globals.player.ent.frame->mat.pos.z);

    return 0;
}

// Equivalent
static U32 LassoThrowCB(xAnimTransition*, xAnimSingle*, void* object)
{
    xEnt* ent = (xEnt*)object;

    zEntPlayer_SNDStop(ePlayerSnd_Heli);
    zLasso_ResetTimer(sLasso, 0.4f);

    sLasso->flags = 0x11;
    sLasso->tgRadius = 0.75f * sLasso->crRadius;

    xVec3SMul(&sLasso->tgNormal, (xVec3*)&ent->model->Mat->at, -sLassoInfo->dist);
    sLasso->tgNormal.y += 5.0f - 4.0f * sLassoInfo->dist;
    xVec3Normalize(&sLasso->tgNormal, &sLasso->tgNormal);
    xVec3Copy(&sLasso->tgCenter, &sLasso->stCenter);
    xVec3AddScaled(&sLasso->tgCenter, (xVec3*)&ent->model->Mat->at, 0.5f * -sLassoInfo->dist);

    sLasso->tgCenter.y += 0.7f * sLassoInfo->dist + 0.3f;

    return 0;
}

// Equivalent
static U32 LassoFlyCB(xAnimTransition*, xAnimSingle*, void* object)
{
    xEnt* ent = (xEnt*)object;

    zEntPlayer_SNDPlay(ePlayerSnd_LassoThrow, 0.0f);
    zLasso_ResetTimer(sLasso, 0.4f * sLassoInfo->dist);

    if (sLassoInfo->targetGuide == 0)
    {
        sLasso->flags = 1;
        xVec3Copy(&sLasso->tgCenter, xBoundCenter(&sLassoInfo->target->bound));
        xVec3AddScaled(&sLasso->tgCenter, (xVec3*)&ent->model->Mat->at,
                       sLassoInfo->target->model->Data->boundingSphere.radius * sLassoInfo->dist);
        xVec3AddScaled(&sLasso->tgCenter, (xVec3*)&ent->model->Mat->up,
                       sLassoInfo->target->model->Data->boundingSphere.radius * sLassoInfo->dist);
        sLasso->tgRadius = 1.5f * sLassoInfo->target->model->Data->boundingSphere.radius;

        xVec3SMul(&sLasso->tgNormal, (xVec3*)&ent->model->Mat->at, 1.0f);
        sLasso->tgNormal.y += 5.0f - 4.0f * sLassoInfo->dist;
        xVec3Normalize(&sLasso->tgNormal, &sLasso->tgNormal);
    }
    else
    {
        sLasso->flags = 1;
        zLasso_InterpToGuide(sLasso);
    }

    sLasso->tgSlack = 0.5f;
    return 0;
}

static U32 LassoDestroyCB(xAnimTransition*, xAnimSingle*, void*)
{
    zEntPlayer_SNDPlay(ePlayerSnd_LassoYank, 0.17f);
    zEntPlayer_SNDPlayStreamRandom(ePlayerStreamSnd_RopingComment1, ePlayerStreamSnd_RopingComment3,
                                   0.2f);

    if (sLassoInfo->targetGuide == 0)
    {
        zLasso_ResetTimer(sLasso, 0.5f);
        sLassoInfo->destroy = 1;
        sLasso->flags = 0x521;

        xVec3Copy(&sLasso->tgCenter, xBoundCenter(&sLassoInfo->target->bound));
        sLasso->tgRadius = 0.75f * sLassoInfo->target->model->Data->boundingSphere.radius;
        xVec3Init(&sLasso->tgNormal, 0.0f, 1.0f, 0.0f);
        return 0;
    }
    else
    {
        sLassoInfo->zeroAnim = sLassoInfo->target->model->Anim->Single->State;
        ((zNPCCommon*)sLassoInfo->target)->LassoNotify(LASS_EVNT_GRABSTART);
        sLasso->flags = 0x4c01;
        return 0;
    }
}

static U32 LassoYankCB(xAnimTransition*, xAnimSingle*, void*)
{
    zEntPlayer_SNDPlay(ePlayerSnd_LassoYank, 0.17f);
    zEntPlayer_SNDPlayStreamRandom(ePlayerStreamSnd_RopingComment1, ePlayerStreamSnd_RopingComment3,
                                   0.2f);

    if (sLassoInfo->targetGuide && sLassoInfo->target)
    {
        ((zNPCCommon*)sLassoInfo->target)->LassoNotify(LASS_EVNT_YANK);
    }

    return 0;
}

// Equivalent: sda relocation scheduling
static U32 MeleeStopCB(xAnimTransition*, xAnimSingle*, void*)
{
    idle_tmr = 0.0f;

    if (globals.player.SundaeTimer < 0.0f)
    {
        globals.player.SpeedMult = 1.0f;
    }
    else
    {
        globals.player.SpeedMult = globals.player.g.SundaeMult;
    }

    sShouldMelee = 0;
    return 0;
}

static U32 SpatulaMeleeStopCB(xAnimTransition* tran, xAnimSingle* anim, void* object)
{
    MeleeStopCB(tran, anim, object);
    SpatulaGrabCB(tran, anim, object);
    return 0;
}

// Equivalent: sda relocation scheduling
static U32 LassoStopCB(xAnimTransition*, xAnimSingle*, void*)
{
    idle_tmr = 0.0f;
    sLasso->flags = 0;

    if (sLassoInfo->targetGuide)
    {
        if (sLassoInfo->target)
        {
            ((zNPCCommon*)sLassoInfo->target)->LassoNotify(LASS_EVNT_YANK);
        }
    }
    else if (sLassoInfo->destroy && sLassoInfo->target)
    {
        zEntEvent(sLassoInfo->target, eEventHit);
    }

    sLassoInfo->destroy = 0;
    sLassoInfo->target = NULL;
    zLasso_SetGuide(NULL, NULL);
    zRumbleStart(SDR_LassoDestroy);
    return 0;
}

static U32 LassoSwingGroundedBeginCheck(xAnimTransition*, xAnimSingle*, void*)
{
    if (globals.sceneCur->sceneID == 'B201' && globals.player.JumpState == 0)
    {
        sLassoInfo->swingTarget = NULL;
        gReticleTarget = NULL;
        sTypeOfTarget = 0;
    }

    return sLassoInfo->swingTarget && globals.player.JumpState == 0;
}

static U32 LassoSwingBeginCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return sLassoInfo->swingTarget && globals.player.JumpState != 0;
}

static U32 LassoSwingReleaseCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return (!globals.player.ControlOff && (globals.pad0->pressed & XPAD_BUTTON_X) &&
            sSwingTimeElapsed > 0.5f) ||
           sLassoInfo->swingTarget == NULL || !(sLassoInfo->swingTarget->flags & 1);
}

static U32 LassoSwingBeginCB(xAnimTransition*, xAnimSingle*, void* object)
{
    xEnt* ent = (xEnt*)object;

    sLassoInfo->target = NULL;
    gReticleTarget = NULL;

    if (sLasso->flags & 1)
    {
        sLasso->flags = 1;
        zLasso_ResetTimer(sLasso, 0.133f);
        xVec3Copy(&sLasso->tgCenter, &sLasso->stCenter);
    }
    else
    {
        sLasso->flags = 0x1043;
        zLasso_InitTimer(sLasso, 0.133f);
        xVec3Copy(&sLasso->stNormal, (xVec3*)&ent->model->Mat->right);
        zEntPlayer_SNDPlay(ePlayerSnd_Heli, 0.0f);
    }

    sLasso->tgSlack = 0.5f;
    sLasso->tgRadius = sLassoInfo->swingTarget->model->Data->boundingSphere.radius;
    xVec3Copy(&sLasso->tgNormal, (xVec3*)&ent->model->Mat->right);
    xVec3Copy(&globals.player.HangVel, (xVec3*)&ent->frame->vel);
    sSwingTimeElapsed = 0.0f;

    zCameraSetBbounce(false);
    zCameraSetLongbounce(false);
    zCameraSetHighbounce(false);
    return 0;
}

static U32 LassoSwingGroundedBeginCB(xAnimTransition* tran, xAnimSingle* anim, void* object)
{
    xEnt* ent = (xEnt*)object;
    JumpCB(tran, anim, object);

    ent->frame->vel.y *= 0.5f;
    LassoSwingBeginCB(tran, anim, object);
    return 0;
}

static U32 LassoSwingTossCB(xAnimTransition*, xAnimSingle*, void*)
{
    zEntPlayer_SNDStop(ePlayerSnd_Heli);
    zEntPlayer_SNDPlay(ePlayerSnd_LassoThrow, 0.0f);

    sLasso->flags = 1;
    zLasso_ResetTimer(sLasso, 0.117f);
    xVec3Copy(&sLasso->tgCenter, xBoundCenter(&sLassoInfo->swingTarget->bound));
    xVec3AddScaled(&sLasso->tgCenter, (xVec3*)&sLassoInfo->swingTarget->model->Mat->up, -0.6f);
    xVec3Copy(&sLasso->tgNormal, (xVec3*)&sLassoInfo->swingTarget->model->Mat->up);

    if (xVec3Dot(&sLasso->tgNormal, &sLasso->stNormal) < 0.0f)
    {
        xVec3Inv(&sLasso->tgNormal, &sLasso->tgNormal);
    }

    sLasso->tgRadius = 0.1f;
    return 0;
}

static U32 LassoSwingCB(xAnimTransition*, xAnimSingle* anim, void*)
{
    sLasso->flags = 0xc21;
    zLasso_ResetTimer(sLasso, 0.0f);

    anim->BilinearLerp[0] = 1.0f;
    anim->Blend->BilinearLerp[0] = 1.0f;

    zCameraEnableLassoCam();
    zCameraSetLassoCamFactor(1.0f);
    return 0;
}

// Equivalent: sda/float scheduling crap
static U32 LassoSwingGroundedCB(xAnimTransition*, xAnimSingle*, void*)
{
    zEntPlayer_SNDStop(ePlayerSnd_Heli);
    idle_tmr = 0.0f;
    sTimeToRetarget = 0.5f;
    sLassoInfo->swingTarget = NULL;
    sLasso->flags = 0;
    sLassoCamLinger = 1;

    return 0;
}

// Really odd scheduling. Maybe equivalent?
static U32 LassoSwingReleaseCB(xAnimTransition* tran, xAnimSingle* anim, void* object)
{
    zEntPlayer_SNDStop(ePlayerSnd_Heli);
    idle_tmr = 0.0f;
    sTimeToRetarget = 0.5f;
    sLassoInfo->canCopter = 1;
    globals.player.Jump_CanDouble = 1;
    globals.player.IsDJumping = 0;
    sLassoCamLinger = 1;
    sLassoInfo->swingTarget = NULL;
    sLasso->flags = 0;

    JumpCB(tran, anim, object);

    return 0;
}

static U8 StunBubbleTrail(xAnimSingle* single)
{
    S32 ret = 0;
    xAnimState* astate = single->State;
    if ((strcmp(astate->Name, "StunFall") == 0) ||
        ((strcmp(astate->Name, "StunJump") == 0) && (single->Time >= 0.6f) && (single->Time <= 1.0f)))
    {
        ret = 1;
    }
    return ret;
}

static U8 BubbleBashContrails(xAnimSingle* single)
{
    S32 ret = 0;
    xAnimState* astate = single->State;
    if (((strcmp(astate->Name, "BbashStart01") == 0) && (single->Time >= 0.3f)) ||
        (strcmp(astate->Name, "BbashAttack01") == 0) ||
        (strcmp(astate->Name, "BbashMiss01") == 0) && (single->Time <= 0.125f))
    {
        ret = 1;
    }
    return ret;
}

static U8 BubbleBounceContrails(xAnimSingle* single)
{
    S32 ret = 0;
    xAnimState* astate = single->State;
    if (

        ((strcmp(astate->Name, "BbounceStart01") == 0) && (single->Time >= 0.9f)) ||
        (strcmp(astate->Name, "BbounceAttack01") == 0))
    {
        ret = 1;
    }
    return ret;
}

xAnimTable* zSandy_AnimTable()
{
    xAnimTable* animTable = xAnimTableNew("Sandy", NULL, 0);

    xAnimTableNewState(animTable, "Idle01", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Idle01b", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Idle01c", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Idle02", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Idle04", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Idle05", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Inactive_sleep", 0x20, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "SlipIdle01", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Inactive01", 0x20, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Inactive02", 0x20, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Hit01", 0x20, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Defeat01", 0x00, 0x0480, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Defeat02", 0x00, 0x0480, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Defeat03", 0x00, 0x0480, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Defeat04", 0x00, 0x0480, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "DefeatGoo", 0x00, 0x0480, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Walk01", 0x10, 0x0044, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Run01", 0x10, 0x0046, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "RunOutOfWorld01", 0x10, 0x0046, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "SlipRun01", 0x10, 0x0046, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "JumpStart01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "JumpLift01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "JumpApex01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Fall01", 0x10, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Land01", 0x20, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "LandRun01", 0x20, 0x0006, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "BounceStart01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "BounceLift01", 0x10, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "BounceApex01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "DJumpApex01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "FallHigh01", 0x10, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "LandHigh01", 0x20, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "LCopterHeadUp01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "LCopter01", 0x10, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "LedgeGrab01", 0x20, 0x0080, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "TailSlide01", 0x10, 0x1840, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "TailSlideJumpStart01", 0x20, 0x100a, 1.0, NULL, NULL, 0.0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "TailSlideJumpApex01", 0x20, 0x100a, 1.0, NULL, NULL, 0.0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "TailSlideFall01", 0x10, 0x100a, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "TailSlideLand01", 0x20, 0x100a, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "TailSlideDJumpApex01", 0x20, 0x100a, 1.0, NULL, NULL, 0.0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Talk04", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Talk03", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Talk02", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Talk01", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "LassoSwingCatch01", 0x20, 0x0000, 1.0, NULL, NULL, 0.0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "LassoSwingCatch02", 0x20, 0x0000, 1.0, NULL, NULL, 0.0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "LassoSwing", 0x10, 0x0040, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "LassoSwingRelease", 0x20, 0x0000, 1.0, NULL, NULL, 0.0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "SpatulaGrab01", 0x20, 0x0080, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "JumpMelee01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Melee01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "LassoWindup", 0x20, 0x0080, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "LassoThrow", 0x20, 0x0080, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "LassoFly", 0x10, 0x0080, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "LassoDestroy", 0x20, 0x0080, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "LassoAboutToDestroy", 0x10, 0x0080, 1.0, NULL, NULL, 0.0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "LassoEnemyRope", 0x20, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "LassoEnemyFight", 0x10, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "LassoEnemyWin", 0x20, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "LassoEnemyLose", 0x20, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);

    xAnimTableNewTransition(animTable, "LandRun01", "Run01", NULL, NULL, 0x10, 0, 0.0, 0.0, 0, 0,
                            0.1, NULL);

    xAnimTableNewTransition(
        animTable,
        "Idle01b Idle01c Idle02 Idle04 Idle05 Inactive_sleep Inactive01 Inactive02 Land01 LandHigh01",
        "Idle01", NULL, IdleCB, 0x10, 0, 0.0, 0.0, 0, 0, 0.1, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle01b Idle01c Idle02 Idle04 Idle05 SlipIdle01 Inactive01 Inactive02 Inactive_sleep Walk01 LandRun01 Run01 SlipRun01",
        "LassoSwingCatch01", LassoSwingGroundedBeginCheck, LassoSwingGroundedBeginCB, 0, 0, 0.0,
        0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "JumpStart01 JumpApex01 Fall01 DJumpStart01 DJumpApex01 TailSlideJumpStart01 TailSlideJumpApex01 TailSlideFall01 TailSlideDJumpApex01 LCopter01 LCopterHeadUp01 BounceStart01 BounceLift01 BounceApex01",
        "LassoSwingCatch01", LassoSwingBeginCheck, LassoSwingBeginCB, 0, 0, 0.0, 0.0, 1, 0, 0.15,
        NULL);

    xAnimTableNewTransition(animTable, "LassoSwingCatch01", "LassoSwingCatch02", NULL,
                            LassoSwingTossCB, 0x10, 0, 0.0, 0.0, 0, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LassoSwingCatch02", "LassoSwing", NULL, LassoSwingCB, 0x10,
                            0, 0.0, 0.0, 0, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "LassoSwingCatch01 LassoSwingCatch02 LassoSwing",
                            "SlipIdle01", IdleSlipCheck, LassoSwingGroundedCB, 0, 0, 0.0, 0.0, 1, 0,
                            0.15, NULL);
    xAnimTableNewTransition(animTable, "LassoSwingCatch01 LassoSwingCatch02 LassoSwing", "Idle01",
                            IdleCheck, LassoSwingGroundedCB, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LassoSwingCatch01 LassoSwingCatch02 LassoSwing", "Walk01",
                            WalkCheck, LassoSwingGroundedCB, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LassoSwingCatch01 LassoSwingCatch02 LassoSwing", "Run01",
                            RunAnyCheck, LassoSwingGroundedCB, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LassoSwingCatch01 LassoSwingCatch02 LassoSwing",
                            "RunOutOfWorld01", RunOutOfWorldCheck, LassoSwingGroundedCB, 0, 0, 0.0,
                            0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LassoSwingCatch01 LassoSwingCatch02 LassoSwing",
                            "SlipRun01", RunSlipCheck, LassoSwingGroundedCB, 0, 0, 0.0, 0.0, 1, 0,
                            0.15, NULL);
    xAnimTableNewTransition(animTable, "LassoSwingCatch01 LassoSwingCatch02 LassoSwing", "Fall01",
                            LassoSwingReleaseCheck, LassoSwingReleaseCB, 0, 0, 0.0, 0.0, 1, 0, 0.15,
                            NULL);

    xAnimTableNewTransition(animTable, "LassoWindup", "LassoThrow", NULL, LassoThrowCB, 0x10, 0,
                            0.0, 0.0, 0, 0, 0.0, NULL);
    xAnimTableNewTransition(animTable, "LassoThrow", "LassoFly", NULL, LassoFlyCB, 0x10, 0, 0.0,
                            0.0, 0, 0, 0.0, NULL);

    xAnimTableNewTransition(animTable, "LassoEnemyRope", "LassoEnemyFight", NULL, NULL, 0x10, 0,
                            0.0, 0.0, 0, 0, 0.0, NULL);

    xAnimTableNewTransition(animTable, "Melee01 JumpMelee01", "SlipIdle01", IdleSlipCheck,
                            MeleeStopCB, 0x10, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Melee01 JumpMelee01", "Idle01", IdleCheck, MeleeStopCB,
                            0x10, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Melee01 JumpMelee01", "Walk01", WalkCheck, MeleeStopCB,
                            0x10, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Melee01 JumpMelee01", "Run01", RunAnyCheck, MeleeStopCB,
                            0x10, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Melee01 JumpMelee01", "RunOutOfWorld01", RunOutOfWorldCheck,
                            MeleeStopCB, 0x10, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Melee01 JumpMelee01", "SlipRun01", RunSlipCheck,
                            MeleeStopCB, 0x10, 0, 0.0, 0.0, 1, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "JumpMelee01", "Fall01", NULL, NULL, 0x10, 0, 0.0, 0.0, 1, 0,
                            0.15, NULL);

    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle01b Idle01c Idle02 Idle04 Idle05 Inactive_sleep SlipIdle01 Inactive01 Inactive02 Walk01 Run01 Land01 LandRun01 ",
        "SpatulaGrab01", SpatulaGrabCheck, SpatulaGrabCB, 0, 0, 0.0, 0.0, 5, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "Melee01", "SpatulaGrab01", SpatulaGrabCheck,
                            SpatulaMeleeStopCB, 0x00, 0, 0.0, 0.0, 5, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "SpatulaGrab01", "Idle01", NULL, SpatulaGrabStopCB, 0x10, 0,
                            0.0, 0.0, 1, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "LassoEnemyWin LassoEnemyLose LassoDestroy", "SlipIdle01",
                            IdleSlipCheck, LassoStopCB, 0x10, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LassoEnemyWin LassoEnemyLose LassoDestroy", "Idle01",
                            IdleCheck, LassoStopCB, 0x10, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LassoEnemyWin LassoEnemyLose LassoDestroy", "Walk01",
                            WalkCheck, LassoStopCB, 0x10, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LassoEnemyWin LassoEnemyLose LassoDestroy", "Run01",
                            RunAnyCheck, LassoStopCB, 0x10, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LassoEnemyWin LassoEnemyLose LassoDestroy",
                            "RunOutOfWorld01", RunOutOfWorldCheck, LassoStopCB, 0x10, 0, 0.0, 0.0,
                            1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LassoEnemyWin LassoEnemyLose LassoDestroy", "SlipRun01",
                            RunSlipCheck, LassoStopCB, 0x10, 0, 0.0, 0.0, 1, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "DJumpApex01", "Fall01", NULL, NULL, 0x10, 0, 0.0, 0.0, 0, 0,
                            0.20, NULL);
    xAnimTableNewTransition(animTable, "JumpApex01", "Fall01", NULL, NULL, 0x10, 0, 0.0, 0.0, 0, 0,
                            0.08, NULL);
    xAnimTableNewTransition(animTable, "JumpStart01", "JumpApex01", NULL, NULL, 0x10, 0, 0.0, 0.0,
                            0, 0, 0.08, NULL);

    xAnimTableNewTransition(
        animTable,
        "Idle01 SlipIdle01 Walk01 Run01 RunOutOfWorld01 SlipRun01 Land01 LandHigh01 LandRun01 JumpStart01 JumpApex01 DJumpApex01 Fall01",
        "BounceStart01", BounceCheck, BounceCB, 0, 0, 0.0, 0.0, 0x0f, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LCopterHeadUp01 LCopter01", "BounceStart01", BounceCheck,
                            BounceStopLCopterCB, 0x00, 0, 0.0, 0.0, 0x0f, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BounceStart01", "BounceLift01", NULL, NULL, 0x10, 0, 0.0,
                            0.0, 0x00, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BounceLift01", "BounceApex01", JumpApexCheck, NULL, 0x00, 0,
                            0.0, 0.0, 0x01, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BounceApex01", "Fall01", NULL, NULL, 0x10, 0, 0.0, 0.0,
                            0x00, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle01b Idle01c Idle02 Idle04 Idle05 SlipIdle01 Inactive01 Inactive02 Inactive_sleep Walk01 LandRun01 Run01 SlipRun01",
        "LassoWindup", LassoStartCheck, LassoStartCB, 0, 0, 0.0, 0.0, 2, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "JumpStart01 JumpLift01 JumpApex01 DJumpApex01 Fall01 BounceStart01 BounceLift01 BounceApex01",
        "JumpMelee01", MeleeCheck, JumpMeleeCB, 0, 0, 0.0, 0.0, 2, 0, 0.06, NULL);
    xAnimTableNewTransition(animTable, "DJumpApex01", "JumpMelee01", MeleeCheck, JumpMeleeCB, 0, 0,
                            0.0, 0.0, 2, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle01b Idle01c Idle02 Idle04 Idle05 SlipIdle01 Inactive01 Inactive02 Inactive_sleep Walk01 LandRun01 Land01 Run01 SlipRun01",
        "Melee01", MeleeCheck, MeleeCB, 0, 0, 0.0, 0.0, 2, 0, 0.00, NULL);
    xAnimTableNewTransition(
        animTable,
        "LassoWindup LassoThrow LassoFly LassoDestroy LassoAboutToDestroy LassoEnemyRope LassoEnemyFight LassoEnemyWin LassoEnemyLose",
        "Idle01", LassoLostTargetCheck, LassoStopCB, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "LassoFly", "LassoDestroy", LassoStraightToDestroyCheck,
                            LassoDestroyCB, 0, 0, 0.0, 0.0, 0, 0, 0.08, NULL);
    xAnimTableNewTransition(animTable, "LassoDestroy", "LassoDestroy", LassoReyankCheck,
                            LassoYankCB, 0, 0, 0.0, 0.0, 0, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LassoFly", "LassoAboutToDestroy", LassoAboutToDestroyCheck,
                            LassoDestroyCB, 0, 0, 0.0, 0.0, 0, 0, 0.08, NULL);
    xAnimTableNewTransition(animTable, "LassoAboutToDestroy", "LassoDestroy", LassoDestroyCheck,
                            LassoYankCB, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LassoAboutToDestroy", "SlipIdle01", LassoFailIdleSlipCheck,
                            LassoStopCB, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LassoAboutToDestroy", "Idle01", LassoFailIdleCheck,
                            LassoStopCB, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LassoAboutToDestroy", "Walk01", LassoFailWalkCheck,
                            LassoStopCB, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LassoAboutToDestroy", "Run01", LassoFailRunCheck,
                            LassoStopCB, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LassoAboutToDestroy", "RunOutOfWorld01",
                            LassoFailRunOutOfWorldCheck, LassoStopCB, 0, 0, 0.0, 0.0, 1, 0, 0.15,
                            NULL);
    xAnimTableNewTransition(animTable, "LassoAboutToDestroy", "SlipRun01", LassoFailRunSlipCheck,
                            LassoStopCB, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "LCopter01 LCopterHeadUp01", "SlipIdle01", IdleSlipCheck,
                            StopLCopterCB, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LCopter01 LCopterHeadUp01", "Idle01", IdleCheck,
                            StopLCopterCB, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LCopter01 LCopterHeadUp01", "Walk01", WalkCheck,
                            StopLCopterCB, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LCopter01 LCopterHeadUp01", "Run01", RunAnyCheck,
                            StopLCopterCB, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LCopter01 LCopterHeadUp01", "RunOutOfWorld01",
                            RunOutOfWorldCheck, StopLCopterCB, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LCopter01 LCopterHeadUp01", "SlipRun01", RunSlipCheck,
                            StopLCopterCB, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "Walk01 Run01 RunOutOfWorld01 LandRun01 Idle01 SlipRun01",
                            "SlipIdle01", IdleSlipCheck, IdleCB, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable,
                            "Walk01 Run01 RunOutOfWorld01 LandRun01 SlipIdle01 SlipRun01", "Idle01",
                            IdleCheck, IdleCB, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);

    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle01b Idle01c Idle02 Idle04 Idle05 SlipIdle01 Inactive01 Inactive02 Inactive_sleep Run01 RunOutOfWorld01 SlipRun01 LandRun01 Land01 LandHigh01",
        "Walk01", WalkCheck, NULL, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle01b Idle01c Idle02 Idle04 Idle05 SlipIdle01 Inactive01 Inactive02 Inactive_sleep RunOutOfWorld01 SlipRun01 Walk01 Land01 LandHigh01",
        "Run01", RunAnyCheck, NULL, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle01b Idle01c Idle02 Idle04 Idle05 SlipIdle01 Inactive01 Inactive02 Inactive_sleep Run01 SlipRun01 Walk01 Land01 LandHigh01",
        "RunOutOfWorld01", RunOutOfWorldCheck, NULL, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle01b Idle01c Idle02 Idle04 Idle05 SlipIdle01 Inactive01 Inactive02 Inactive_sleep Run01 RunOutOfWorld01 Walk01 Land01 LandHigh01",
        "SlipRun01", RunSlipCheck, NULL, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);

    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle01b Idle01c Idle02 Idle04 Idle05 SlipIdle01 Inactive01 Inactive02 Inactive_sleep Walk01 LandRun01 Run01 RunOutOfWorld01 SlipRun01 Land01 LandHigh01",
        "JumpStart01", JumpCheck, JumpCB, 0, 0, 0.0, 0.0, 0x0A, 0, 0.0, NULL);

    xAnimTableNewTransition(animTable, "JumpStart01", "JumpApex01", JumpApexCheck, NULL, 0, 0, 0.0,
                            0.0, 1, 0, 0.1, NULL);
    xAnimTableNewTransition(animTable,
                            "JumpStart01 JumpApex01 Fall01 BounceStart01 BounceLift01 BounceApex01",
                            "DJumpApex01", DblJumpCheck, DblJumpCB, 0, 0, 0.0, 0.0, 0x0A, 0, 0.1,
                            NULL);
    xAnimTableNewTransition(animTable, "LCopter01 LCopterHeadUp01", "Fall01", FallCheck,
                            StopLCopterCB, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle01b Idle01c Idle02 Idle04 Idle05 SlipIdle01 Inactive01 Inactive02 Inactive_sleep Walk01 LandRun01 RunOutOfWorld01 Run01 SlipRun01 Land01 LandHigh01",
        "Fall01", FallCheck, NULL, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "JumpApex01 DJumpApex01 Fall01", "Land01", LandCheck,
                            SandyLandCB, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "JumpApex01 DJumpApex01 Fall01", "SlipIdle01",
                            LandSlipIdleCheck, SandyLandCB, 0, 0, 0.0, 0.0, 2, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "JumpApex01 DJumpApex01 Fall01", "LandRun01", LandFastCheck,
                            SandyLandCB, 0, 0, 0.0, 0.0, 3, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "JumpApex01 DJumpApex01 Fall01", "Walk01", LandWalkCheck,
                            SandyLandCB, 0, 0, 0.0, 0.0, 3, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "JumpApex01 DJumpApex01 Fall01", "SlipRun01",
                            LandSlipRunCheck, SandyLandCB, 0, 0, 0.0, 0.0, 4, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "Fall01 DJumpApex01 TailSlideFall01 TailSlideDJumpApex01",
                            "LCopterHeadUp01", LCopterCheck, LCopterCB, 0, 0, 0.0, 0.0, 8, 0, 0.15,
                            NULL);
    xAnimTableNewTransition(animTable, "LCopterHeadUp01", "LCopter01", NULL, NULL, 0x10, 0, 0.0,
                            0.0, 0, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "JumpStart01 JumpLift01 JumpApex01 Fall01 DJumpApex01 TailSlideJumpStart01 TailSlideJumpApex01 TailSlideFall01 TailSlideDJumpApex01 LCopter01 LCopterHeadUp01",
        "LedgeGrab01", LedgeGrabCheck, LedgeGrabCB, 0, 0, 0.0, 0.0, 0x0B, 0, 0.1, NULL);
    xAnimTableNewTransition(animTable, "LedgeGrab01", "Idle01", NULL, LedgeFinishCB, 0x10, 0, 0.0,
                            0.0, 0, 0, 0.1, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle01b Idle01c Idle02 Idle04 Idle05 SlipIdle01 Inactive01 Inactive02 Inactive_sleep Walk01 LandRun01 Run01 RunOutOfWorld01 SlipRun01 JumpLift01 JumpApex01 Fall01 Land01 DJumpApex01 FallHigh01 LandHigh01 LCopter01 LCopterHeadUp01",
        "TailSlide01", SlideTrackCheck, SlideTrackCB, 0, 0, 0.0, 0.0, 9, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "TailSlide01", "Idle01", NoslideTrackCheck, NoslideTrackCB,
                            0x00, 0, 0.0, 0.0, 0x09, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "TailSlide01", "TailSlideFall01", TrackFallCheck,
                            TrackFallCB, 0x00, 0, 0.0, 0.0, 0x09, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "TailSlideDJumpApex01", "TailSlideFall01", NULL, NULL, 0x10,
                            0, 0.0, 0.0, 0x00, 0, 0.20, NULL);
    xAnimTableNewTransition(animTable, "TailSlideJumpApex01", "TailSlideFall01", NULL, NULL, 0x10,
                            0, 0.0, 0.0, 0x00, 0, 0.08, NULL);
    xAnimTableNewTransition(animTable, "TailSlideLand01", "TailSlide01", NULL, NULL, 0x10, 0, 0.0,
                            0.0, 0x00, 0, 0.08, NULL);
    xAnimTableNewTransition(animTable, "TailSlideJumpStart01", "TailSlideJumpApex01", NULL, NULL,
                            0x10, 0, 0.0, 0.0, 0x00, 0, 0.08, NULL);
    xAnimTableNewTransition(animTable, "TailSlide01 TailSlideLand01", "TailSlideJumpStart01",
                            JumpCheck, JumpCB, 0x00, 0, 0.0, 0.0, 0x0A, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "TailSlideJumpStart01 TailSlideJumpApex01 TailSlideFall01",
                            "TailSlideJumpStart01", TrackPrefallJumpCheck, JumpCB, 0x00, 0, 0.0,
                            0.0, 0x0F, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "TailSlideJumpStart01", "TailSlideJumpApex01", JumpApexCheck,
                            NULL, 0x00, 0, 0.0, 0.0, 0x01, 0, 0.10, NULL);
    xAnimTableNewTransition(animTable, "TailSlideJumpStart01 TailSlideJumpApex01 TailSlideFall01",
                            "TailSlideDJumpApex01", DblJumpCheck, DblJumpCB, 0x00, 0, 0.0, 0.0,
                            0x0A, 0, 0.10, NULL);

    xAnimTableNewTransition(
        animTable, "TailSlideJumpStart01 TailSlideJumpApex01 TailSlideDJumpApex01 TailSlideFall01",
        "TailSlideLand01", LandTrackCheck, SlideTrackCB, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable, "TailSlideJumpStart01 TailSlideJumpApex01 TailSlideDJumpApex01 TailSlideFall01",
        "Land01", LandNoTrackCheck, NoslideTrackCB, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable, "TailSlideJumpStart01 TailSlideJumpApex01 TailSlideDJumpApex01 TailSlideFall01",
        "LandRun01", LandNoTrackFastCheck, NoslideTrackCB, 0, 0, 0.0, 0.0, 3, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable, "TailSlideJumpStart01 TailSlideJumpApex01 TailSlideDJumpApex01 TailSlideFall01",
        "Walk01", LandNoTrackWalkCheck, NoslideTrackCB, 0, 0, 0.0, 0.0, 3, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable, "TailSlideJumpStart01 TailSlideJumpApex01 TailSlideDJumpApex01 TailSlideFall01",
        "SlipIdle01", LandNoTrackSlipIdleCheck, NoslideTrackCB, 0, 0, 0.0, 0.0, 4, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable, "TailSlideJumpStart01 TailSlideJumpApex01 TailSlideDJumpApex01 TailSlideFall01",
        "SlipRun01", LandNoTrackSlipRunCheck, NoslideTrackCB, 0, 0, 0.0, 0.0, 4, 0, 0.15, NULL);

    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle01b Idle01c Idle02 Idle04 Idle05 SlipIdle01 Inactive01 Inactive02 Inactive_sleep Walk01 LandRun01 Run01 RunOutOfWorld01 SlipRun01 JumpLift01 JumpApex01 Fall01 Land01 DJumpApex01 FallHigh01 LandHigh01 LCopter01 LCopterHeadUp01 TailSlide01 TailSlideJumpStart01 TailSlideJumpApex01 TailSlideDJumpApex01 TailSlideFall01 TailSlideLand01",
        "Defeat01", Defeated01Check, DefeatedCB, 0, 4, 0.0, 0.0, 0x09, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle01b Idle01c Idle02 Idle04 Idle05 SlipIdle01 Inactive01 Inactive02 Inactive_sleep Walk01 LandRun01 Run01 RunOutOfWorld01 SlipRun01 JumpLift01 JumpApex01 Fall01 Land01 DJumpApex01 FallHigh01 LandHigh01 LCopter01 LCopterHeadUp01 TailSlide01 TailSlideJumpStart01 TailSlideJumpApex01 TailSlideDJumpApex01 TailSlideFall01 TailSlideLand01",
        "Defeat02", Defeated02Check, DefeatedCB, 0, 4, 0.0, 0.0, 0x09, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle01b Idle01c Idle02 Idle04 Idle05 SlipIdle01 Inactive01 Inactive02 Inactive_sleep Walk01 LandRun01 Run01 RunOutOfWorld01 SlipRun01 JumpLift01 JumpApex01 Fall01 Land01 DJumpApex01 FallHigh01 LandHigh01 LCopter01 LCopterHeadUp01 TailSlide01 TailSlideJumpStart01 TailSlideJumpApex01 TailSlideDJumpApex01 TailSlideFall01 TailSlideLand01",
        "Defeat03", Defeated03Check, DefeatedCB, 0, 4, 0.0, 0.0, 0x09, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle01b Idle01c Idle02 Idle04 Idle05 SlipIdle01 Inactive01 Inactive02 Inactive_sleep Walk01 LandRun01 Run01 RunOutOfWorld01 SlipRun01 JumpLift01 JumpApex01 Fall01 Land01 DJumpApex01 FallHigh01 LandHigh01 LCopter01 LCopterHeadUp01 TailSlide01 TailSlideJumpStart01 TailSlideJumpApex01 TailSlideDJumpApex01 TailSlideFall01 TailSlideLand01",
        "Defeat04", Defeated04Check, DefeatedCB, 0, 4, 0.0, 0.0, 0x09, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle01b Idle01c Idle02 Idle04 Idle05 SlipIdle01 Inactive01 Inactive02 Inactive_sleep Walk01 LandRun01 Run01 RunOutOfWorld01 SlipRun01 JumpLift01 JumpApex01 Fall01 Land01 DJumpApex01 FallHigh01 LandHigh01 LCopter01 LCopterHeadUp01 TailSlide01 TailSlideJumpStart01 TailSlideJumpApex01 TailSlideDJumpApex01 TailSlideFall01 TailSlideLand01",
        "DefeatGoo", GooCheck, GooDeathCB, 0, 0, 0.0, 0.0, 0x1e, 0, 0.15, NULL);

    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle01b Idle01c Idle02 Idle04 Idle05 SlipIdle01 Inactive_sleep Inactive01 Inactive02 ",
        "Talk01", TalkCheck, NULL, 0, 0, 0.0, 0.0, 0x14, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle01b Idle01c Idle02 Idle04 Idle05 SlipIdle01 Inactive_sleep Inactive01 Inactive02 ",
        "Talk02", TalkCheck, NULL, 0, 1, 0.0, 0.0, 0x14, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle01b Idle01c Idle02 Idle04 Idle05 SlipIdle01 Inactive_sleep Inactive01 Inactive02 ",
        "Talk03", TalkCheck, NULL, 0, 2, 0.0, 0.0, 0x14, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle01b Idle01c Idle02 Idle04 Idle05 SlipIdle01 Inactive_sleep Inactive01 Inactive02 ",
        "Talk04", TalkCheck, NULL, 0, 3, 0.0, 0.0, 0x14, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "Talk01", "Idle01", TalkDoneCheck, IdleCB, 0x00, 0x00000,
                            0.0, 0.0, 0x14, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Talk02", "Idle01", TalkDoneCheck, IdleCB, 0x00, 0x00001,
                            0.0, 0.0, 0x14, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Talk03", "Idle01", TalkDoneCheck, IdleCB, 0x00, 0x00002,
                            0.0, 0.0, 0x14, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Talk04", "Idle01", TalkDoneCheck, IdleCB, 0x00, 0x00003,
                            0.0, 0.0, 0x14, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Idle01b", InactiveCheck, InactiveCB, 0x00,
                            0x80000, 0.0, 0.0, 0x01, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Idle01c", InactiveCheck, InactiveCB, 0x00,
                            0x80001, 0.0, 0.0, 0x01, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Idle02", InactiveCheck, InactiveCB, 0x00, 0x80002,
                            0.0, 0.0, 0x01, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Idle04", InactiveCheck, InactiveCB, 0x00, 0x80003,
                            0.0, 0.0, 0x01, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Idle05", InactiveCheck, InactiveCB, 0x00, 0x80004,
                            0.0, 0.0, 0x01, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Inactive_sleep", InactiveCheck, InactiveCB, 0x00,
                            0x80005, 0.0, 0.0, 0x01, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Inactive01", InactiveCheck, InactiveCB, 0x00,
                            0x80006, 0.0, 0.0, 0x01, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Inactive02", InactiveCheck, InactiveCB, 0x00,
                            0x80007, 0.0, 0.0, 0x01, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01b", "Idle01", InactiveFinishedCheck, IdleCB, 0x00,
                            0x00000, 0.0, 0.0, 0x00, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01c", "Idle01", InactiveFinishedCheck, IdleCB, 0x00,
                            0x00000, 0.0, 0.0, 0x00, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle02", "Idle01", InactiveFinishedCheck, IdleCB, 0x00,
                            0x00000, 0.0, 0.0, 0x00, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle04", "Idle01", InactiveFinishedCheck, IdleCB, 0x00,
                            0x00000, 0.0, 0.0, 0x00, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle05", "Idle01", InactiveFinishedCheck, IdleCB, 0x00,
                            0x00000, 0.0, 0.0, 0x00, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Inactive01", "Idle01", NULL, IdleCB, 0x10, 0x00000, 0.0,
                            0.0, 0x00, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Inactive02", "Idle01", NULL, IdleCB, 0x10, 0x00000, 0.0,
                            0.0, 0x00, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Hit01", "Idle01", NULL, IdleCB, 0x10, 0x00000, 0.0, 0.0,
                            0x00, 0, 0.15, NULL);

    return animTable;
}

static U32 StunStartFallCB(xAnimTransition*, xAnimSingle*, void*)
{
    stun_power_tmr = 0;
    return 0;
}

static U32 StunRadiusCB(xAnimTransition*, xAnimSingle*, void*)
{
    zEntPlayer_SNDPlay(ePlayerSnd_BellySmash, 0.0f);
    if ((xrand() & 0x3) == 3)
    {
        zEntPlayer_SNDPlayStreamRandom(ePlayerStreamSnd_BellySmashComment1,
                                       ePlayerStreamSnd_BellySmashComment3, 0.0f);
    }

    if (tslide_lastrealvel.y > -1.0f)
    {
        zPadAddRumble(eRumble_Medium, 0.1f, 0, 0x0);
    }
    else if (stun_power_tmr < 0.4f)
    {
        zPadAddRumble(eRumble_Heavy, 0.15f, 0, 0x0);
    }
    else if (stun_power_tmr < 1.0f)
    {
        zPadAddRumble(eRumble_VeryHeavy, 0.3f, 0, 0x0);
    }
    else
    {
        zPadAddRumble(eRumble_VeryHeavyHi, 0.6f, 0, 0x0);
    }

    return 0;
}

static S32 MeleeAttackBoundCollide(xEnt* ent, zScene* zsc, xBound* meleeB)
{
    Melee_cbData cbdata;
    xVec3 pos;

    cbdata.ent = ent;
    cbdata.zsc = zsc;
    cbdata.meleeB = meleeB;
    cbdata.hitsomething = 0;
    xVec3Copy(&pos, xBoundCenter(meleeB));

    xGridCheckPosition(&colls_grid, &pos, &meleeB->qcd, CheckObjectAgainstMeleeBound, &cbdata);
    xGridCheckPosition(&colls_oso_grid, &pos, &meleeB->qcd, CheckObjectAgainstMeleeBound, &cbdata);
    xGridCheckPosition(&npcs_grid, &pos, &meleeB->qcd, CheckObjectAgainstMeleeBound, &cbdata);

    return cbdata.hitsomething;
}

void xEntBoulder_AddForce(xEntBoulder* ent, xVec3* force);

static S32 CheckObjectAgainstMeleeBound(xEnt* cbent, void* cbdata)
{
    Melee_cbData* data = (Melee_cbData*)cbdata;
    S32 hitsomething = data->hitsomething;
    xEnt* ent = data->ent;
    xBound* meleeB = data->meleeB;
    xCollis meleeColl;
    xRay3 tempray;
    xVec3 dir;
    xVec3 tmp;
    S32 worldSpaceNorm;

    if (!(cbent->baseFlags & 0x20) ||
        (!(cbent->moreFlags & 0x10) && cbent->baseType != eBaseTypeStatic) ||
        !(cbent->flags & 1) || cbent->model == NULL)
    {
        return 1;
    }

    if (cbent->baseType == eBaseTypeStatic)
    {
        if (zThrown_IsFruit(cbent, NULL))
        {
            zThrown_AddFruit(cbent);
            zThrown_KillFruit(cbent);
        }
        else if (!(cbent->moreFlags & 0x10))
        {
            return 1;
        }
    }

    xBoundHitsBound(meleeB, &cbent->bound, &meleeColl);

    if (!(meleeColl.flags & 1))
    {
        return 1;
    }

    if (cbent->baseType != eBaseTypeNPC)
    {
        xFXShineStart((xVec3*)&cbent->model->Mat->pos, 1.0f, 0.3f, 0.01f, 0.1f, 0, NULL, NULL, 0.1f,
                      0);
        zEntEvent(cbent, 0x163);
    }

    if (cbent->baseType == eBaseTypeNPC)
    {
        zNPCCommon* npc = (zNPCCommon*)cbent;

        if ((((xNPCBasic*)cbent)->SelfType() & 0xffffff00) == 'NTR\0')
        {
            iColor_tag c_inside;
            iColor_tag c_outside;

            c_inside.r = 255;
            c_inside.g = 255;
            c_inside.b = 255;
            c_outside.r = 255;
            c_outside.g = 255;
            c_outside.b = 0;
            xFXShineStart((xVec3*)&cbent->model->Mat->pos, 1.0f, 0.5f, 0.01f, 0.1f, 0, &c_inside, &c_outside,
                          0.1f, 0);
        }

        if (globals.player.SundaeTimer > 0.0f)
        {
            npc->Damage(DMGTYP_INSTAKILL, &globals.player.ent, NULL);
        }
        else
        {
            npc->Damage(DMGTYP_SIDE, &globals.player.ent, NULL);
        }

        hitsomething = 1;
    }
    else if (cbent->baseType == eBaseTypeBoulder)
    {
        xEntBoulder* boul = (xEntBoulder*)cbent;

        if (boul->basset->flags & 0x100)
        {
            zEntEvent(cbent, 0x3a);
        }

        xVec3Sub(&dir, (xVec3*)&boul->model->Mat->pos, (xVec3*)&ent->model->Mat->pos);
        xVec3Normalize(&dir, &dir);
        xVec3SMulBy(&dir, 10.0f);
        xEntBoulder_AddForce(boul, &dir);
        hitsomething = 1;
    }
    else if (cbent->baseType == eBaseTypeButton)
    {
        if (gCurrentPlayer == eCurrentPlayerSpongeBob)
        {
            zEntButton_Press((_zEntButton*)cbent, 1);
        }

        if (gCurrentPlayer == eCurrentPlayerSandy)
        {
            zEntButton_Press((_zEntButton*)cbent, 0x4000);
        }

        if (gCurrentPlayer == eCurrentPlayerPatrick)
        {
            zEntButton_Press((_zEntButton*)cbent, 0x8000);
        }

        hitsomething = 1;
    }
    else if (cbent->baseType == eBaseTypeDestructObj)
    {
        zEntDestructObj_Hit((zEntDestructObj*)cbent, 0x1000);
        hitsomething = 1;
    }
    else if (cbent->baseType == eBaseTypePlatform && cbent->subType == ZPLATFORM_SUBTYPE_PADDLE)
    {
        {
            U32 paddleFlags = ((zPlatform*)cbent)->passet->paddle.paddleFlags;

            if ((gCurrentPlayer == eCurrentPlayerSpongeBob && (paddleFlags & 0x8)) ||
                (gCurrentPlayer == eCurrentPlayerPatrick && (paddleFlags & 0x80)) ||
                (gCurrentPlayer == eCurrentPlayerSandy && (paddleFlags & 0x40)))
            {
                worldSpaceNorm = 0;

                if (gCurrentPlayer == eCurrentPlayerSpongeBob)
                {
                    tempray.origin.x = ent->model->Mat->pos.x;
                    tempray.origin.y = ent->model->Mat->pos.y + ent->bound.sph.r;
                    tempray.origin.z = ent->model->Mat->pos.z;
                    tempray.dir.x = meleeB->sph.center.x - tempray.origin.x;
                    tempray.dir.y = meleeB->sph.center.y - tempray.origin.y;
                    tempray.dir.z = meleeB->sph.center.z - tempray.origin.z;
                    tempray.min_t = 0.0f;
                    tempray.max_t = meleeB->sph.r + xVec3Normalize(&tempray.dir, &tempray.dir);
                }
                else
                {
                    tempray.dir = *(xVec3*)&ent->model->Mat->at;
                    tempray.origin.x = meleeB->sph.center.x;
                    tempray.origin.y = meleeB->sph.center.y;
                    tempray.origin.z = meleeB->sph.center.z;
                    tmp.x = ent->model->Mat->pos.x - tempray.origin.x;
                    tmp.y = ent->model->Mat->pos.y - tempray.origin.y;
                    tmp.z = ent->model->Mat->pos.z - tempray.origin.z;
                    tempray.min_t = xVec3Dot(&tmp, &tempray.dir);
                    tempray.max_t = 0.0f;

                    if (tempray.min_t > 0.0f)
                    {
                        tempray.min_t = -meleeB->sph.r;
                    }
                }

                tempray.flags = 0xc00;
                meleeColl.flags = 0x200;
                iRayHitsModel(&tempray, cbent->model, &meleeColl);
                xDrawSetColor(0xff, 0x00, 0xff, 0xff);
                xDrawLine(&tempray.origin, &meleeB->sph.center);

                if (!(meleeColl.flags & 1))
                {
                    meleeColl.flags = 0x200;
                    worldSpaceNorm = 1;
                    xSphereHitsModel(&meleeB->sph, cbent->model, &meleeColl);
                }

                if (meleeColl.flags & 1)
                {
                    meleeColl.optr = cbent;

                    if (zPlatform_PaddleCollide(&meleeColl, (xVec3*)&ent->model->Mat->pos, &tempray.dir,
                                                worldSpaceNorm))
                    {
                        hitsomething = 1;
                    }
                }
            }
        }
    }
    else
    {
        zEntEvent(cbent, 0x3a);
        hitsomething = 1;
    }

    if (hitsomething)
    {
        zFX_SpawnBubbleHit(&meleeB->sph.center, 10);
    }

    data->hitsomething = hitsomething;

    return 1;
}

S32 zEntPlayer_IsSneaking()
{
    if (gCurrentPlayer != eCurrentPlayerSpongeBob)
    {
        return false;
    }

    U32 flags = globals.player.ent.model->Anim->Single->State->UserFlags;
    if ((flags & 1) != 0 || (flags & 0x1e) == 2 || (flags & 0x1e) == 4)
    {
        return true;
    }
    else
    {
        return false;
    }
}

xAnimTable* zPatrick_AnimTable()
{
    xAnimTable* animTable = xAnimTableNew("Patrick", NULL, 0);

    xAnimTableNewState(animTable, "Idle01", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Idle02", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Idle03", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Idle04", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "SlipIdle01", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Inactive01", 0x20, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Inactive02", 0x20, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Inactive03", 0x20, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Inactive04", 0x20, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Inactive05", 0x20, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Inactive06", 0x20, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Inactive07", 0x20, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Walk01", 0x10, 0x0044, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Walk02", 0x10, 0x0044, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Walk03", 0x10, 0x0044, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Run01", 0x10, 0x0046, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Run02", 0x10, 0x0046, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Run03", 0x10, 0x0046, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "RunOutOfWorld01", 0x10, 0x0046, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "SlipRun01", 0x10, 0x0046, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "JumpStart01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Fall01", 0x10, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Land01", 0x20, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "BounceStart01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "BounceLift01", 0x10, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "BounceApex01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "DJumpStart01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "FallHigh01", 0x10, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "LandHigh01", 0x20, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "LedgeGrab01", 0x20, 0x0080, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "StunJump", 0x20, 0x400a, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "StunFall", 0x10, 0x400a, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "StunLand", 0x20, 0x0080, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Hit01", 0x20, 0x0080, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Hit02", 0x20, 0x0080, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Hit03", 0x20, 0x0080, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Melee01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Defeated01", 0x00, 0x0480, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Defeated02", 0x00, 0x0480, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "DefeatedGoo01", 0x00, 0x0480, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "DefeatedProjectile01", 0x00, 0x0480, 1.0, NULL, NULL, 0.0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "ButtSlide01", 0x10, 0x1840, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Talk04", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Talk03", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Talk02", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Talk01", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Carry_Pickup", 0x20, 0x0080, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Carry_PickupFail", 0x20, 0x0080, 1.0, NULL, NULL, 0.0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Carry_Idle", 0x10, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Carry_Walk", 0x10, 0x0004, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Carry_Throw", 0x20, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Carry_PickupItem", 0x10, 0x0000, 1.0, NULL, NULL, 0.0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Carry_IdleItem", 0x10, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Carry_WalkItem", 0x10, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Carry_ThrowItem", 0x10, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "SpatulaGrab01", 0x20, 0x0080, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);

    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Walk01 Walk02 Walk03 Run01 Run02 Run03 Land01 LandHigh01 Melee01 Carry_Idle Carry_Walk Carry_IdleItem Carry_WalkItem",
        "SpatulaGrab01", SpatulaGrabCheck, SpatulaGrabCB, 0, 0, 0.0, 0.0, 5, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "SpatulaGrab01", "Idle01", NULL, SpatulaGrabStopCB, 0x10, 0,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Carry_Pickup", "Carry_Idle", NULL, NULL, 0x10, 0, 0.0, 0.0,
                            0, 0, 0.10, NULL);
    xAnimTableNewTransition(animTable, "Carry_Throw", "Idle01", NULL, NULL, 0x10, 0, 0.0, 0.0, 0, 0,
                            0.10, NULL);
    xAnimTableNewTransition(animTable, "Carry_PickupFail", "Idle01", NULL, NULL, 0x10, 0, 0.0, 0.0,
                            0, 0, 0.10, NULL);
    xAnimTableNewTransition(animTable, "Carry_Idle", "Carry_Walk", AnyMoveCheck, NULL, 0x00, 0, 0.0,
                            0.0, 0, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Carry_Walk", "Carry_Idle", AnyStopCheck, NULL, 0x00, 0, 0.0,
                            0.0, 0, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "Carry_Idle Carry_Walk", "Carry_Throw", PatrickGrabThrow,
                            PatrickGrabThrowCB, 0, 0, 0.0, 0.0, 0, 0, 0.15, NULL);

    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Walk01 Walk02 Walk03 Run01 Run02 Run03 SlipRun01",
        "Carry_Pickup", PatrickGrabCheck, PatrickGrabCB, 0, 0, 0.0, 0.0, 0, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Walk01 Walk02 Walk03 Run01 Run02 Run03",
        "Carry_PickupFail", PatrickGrabFailed, NULL, 0, 0, 0.0, 0.0, 0, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "Carry_Idle Carry_Walk Carry_Pickup", "Idle01",
                            PatrickGrabKill, NULL, 0, 0, 0.0, 0.0, 0, 0, 0.15, NULL);

    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Walk01 Walk02 Walk03 Run01 Run02 Run03 SlipRun01",
        "Melee01", PatrickAttackCheck, PatrickMeleeCB, 0, 0, 0.0, 0.0, 0, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "Melee01", "SlipIdle01", IdleSlipCheck, NULL, 0x10, 0, 0.0,
                            0.0, 1, 0, 0.00, NULL);
    xAnimTableNewTransition(animTable, "Melee01", "Idle01", IdleCheck, NULL, 0x10, 0, 0.0, 0.0, 1,
                            0, 0.00, NULL);
    xAnimTableNewTransition(animTable, "Melee01", "Walk01", WalkStoicCheck, NULL, 0x10, 0, 0.0, 0.0,
                            1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Melee01", "Walk02", WalkVictoryCheck, NULL, 0x10, 0, 0.0,
                            0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Melee01", "Walk03", WalkScaredCheck, NULL, 0x10, 0, 0.0,
                            0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Melee01", "Run01", RunStoicCheck, NULL, 0x10, 0, 0.0, 0.0,
                            1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Melee01", "Run02", RunVictoryCheck, NULL, 0x10, 0, 0.0, 0.0,
                            1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Melee01", "Run03", RunScaredCheck, NULL, 0x10, 0, 0.0, 0.0,
                            1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Melee01", "RunOutOfWorld01", RunOutOfWorldCheck, NULL, 0x10,
                            0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Melee01", "SlipRun01", RunSlipCheck, NULL, 0x10, 0, 0.0,
                            0.0, 1, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "Inactive01 Land01 LandHigh01", "Idle01", NULL, NULL, 0x10,
                            0, 0.0, 0.0, 0, 0, 0.1, NULL);

    xAnimTableNewTransition(animTable, "JumpStart01", "Fall01", NULL, NULL, 0x10, 0, 0.0, 0.0, 0, 0,
                            0.0, NULL);
    xAnimTableNewTransition(animTable, "DJumpStart01", "Fall01", NULL, NULL, 0x10, 0, 0.0, 0.0, 0,
                            0, 0.0, NULL);

    xAnimTableNewTransition(
        animTable,
        "Idle01 SlipIdle01 Walk01 Walk02 Walk03 Run01 Run02 Run03 RunOutOfWorld01 SlipRun01 Land01 LandHigh01 JumpStart01 JumpApex01 DJumpStart01 Fall01 StunJump StunFall StunLand",
        "BounceStart01", BounceCheck, BounceCB, 0, 0, 0.0, 0.0, 0x0F, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "BounceStart01", "BounceLift01", NULL, NULL, 0x10, 0, 0.0,
                            0.0, 0, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BounceLift01", "BounceApex01", JumpApexCheck, NULL, 0x00, 0,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BounceApex01", "Fall01", NULL, NULL, 0x10, 0, 0.0, 0.0, 0,
                            0, 0.15, NULL);

    xAnimTableNewTransition(
        animTable, "Idle01 Walk01 Walk02 Walk03 Run01 Run02 Run03 RunOutOfWorld01 SlipRun01",
        "SlipIdle01", IdleSlipCheck, NULL, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "SlipIdle01 Walk01 Run01 RunOutOfWorld01 SlipRun01",
                            "Idle01", IdleCheck, NULL, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "Idle01 Walk02 Run02", "Idle02", IdleVictoryCheck, IdleCB, 0,
                            0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle02 Run02", "Walk02", WalkVictoryCheck, IdleCB, 0, 0,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle02 Walk02", "Run02", RunVictoryCheck, IdleCB, 0, 0, 0.0,
                            0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle02 Walk02 Run02", "Idle01", IdleStoicCheck, IdleCB, 0,
                            0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Walk03 Run03 Idle01", "Idle04", IdleScaredCheck, IdleCB, 0,
                            0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle04 Run03", "Walk03", WalkScaredCheck, IdleCB, 0, 0, 0.0,
                            0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle04 Walk03", "Run03", RunScaredCheck, IdleCB, 0, 0, 0.0,
                            0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle04 Walk03 Run03", "Idle01", IdleStoicCheck, IdleCB, 0,
                            0, 0.0, 0.0, 1, 0, 0.15, NULL);

    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Run01 Run02 Run03 RunOutOfWorld01 SlipRun01 Land01 LandHigh01",
        "Walk01", WalkStoicCheck, NULL, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Run01 Run02 Run03 RunOutOfWorld01 SlipRun01 Land01 LandHigh01",
        "Walk02", WalkVictoryCheck, NULL, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Run01 Run02 Run03 RunOutOfWorld01 SlipRun01 Land01 LandHigh01",
        "Walk03", WalkScaredCheck, NULL, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);

    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Walk01 Walk02 Walk03 Land01 LandHigh01 Run02 Run03 RunOutOfWorld01 SlipRun01",
        "Run01", RunStoicCheck, NULL, 0, 0, 0.0, 0.0, 0x01, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Walk01 Walk02 Walk03 Land01 LandHigh01 Run01 Run03 RunOutOfWorld01 SlipRun01",
        "Run02", RunVictoryCheck, NULL, 0, 0, 0.0, 0.0, 0x01, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Walk01 Walk02 Walk03 Land01 LandHigh01 Run01 Run02 RunOutOfWorld01 SlipRun01",
        "Run03", RunScaredCheck, NULL, 0, 0, 0.0, 0.0, 0x01, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Walk01 Walk02 Walk03 Land01 LandHigh01 Run01 Run02 Run03 SlipRun01",
        "RunOutOfWorld01", RunOutOfWorldCheck, NULL, 0, 0, 0.0, 0.0, 0x01, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Walk01 Walk02 Walk03 Land01 LandHigh01 Run01 Run02 Run03 RunOutOfWorld01",
        "SlipRun01", RunSlipCheck, NULL, 0, 0, 0.0, 0.0, 0x01, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Walk01 Walk02 Walk03 Run01 Run02 Run03 RunOutOfWorld01 SlipRun01 Land01 LandHigh01",
        "JumpStart01", JumpCheck, JumpCB, 0, 0, 0.0, 0.0, 0x0A, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable,
                            "JumpStart01 Fall01 FallHigh01 BounceStart01 BounceLift01 BounceApex01",
                            "DJumpStart01", DblJumpCheck, DblJumpCB, 0, 0, 0.0, 0.0, 0x0A, 0, 0.15,
                            NULL);

    xAnimTableNewTransition(animTable, "Fall01 FallHigh01 JumpStart01 DJumpStart01", "Land01",
                            LandCheck, NULL, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Fall01 FallHigh01 JumpStart01 DJumpStart01", "SlipRun01",
                            LandSlipRunCheck, NULL, 0, 0, 0.0, 0.0, 2, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Fall01 FallHigh01 JumpStart01 DJumpStart01", "SlipIdle01",
                            LandSlipIdleCheck, NULL, 0, 0, 0.0, 0.0, 2, 0, 0.15, NULL);

    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Walk01 Walk02 Walk03 Run01 Run02 Run03 RunOutOfWorld01 SlipRun01 Land01 LandHigh01",
        "Fall01", FallCheck, NULL, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);

    xAnimTableNewTransition(
        animTable,
        "JumpStart01 Fall01 FallHigh01 DJumpStart01 BounceStart01 BounceLift01 BounceApex01",
        "StunJump", PatrickStunCheck, NULL, 0, 0, 0.0, 0.0, 0x0A, 0, 0.125, NULL);

    xAnimTableNewTransition(animTable, "StunJump", "StunFall", NULL, StunStartFallCB, 0x10, 0, 0.0,
                            0.0, 0, 0, 0.0, NULL);
    xAnimTableNewTransition(animTable, "StunFall", "StunLand", LandCheck, StunRadiusCB, 0x00, 0,
                            0.0, 0.0, 1, 0, 0.1, NULL);
    xAnimTableNewTransition(animTable, "StunLand", "Idle01", NULL, NULL, 0x10, 0, 0.0, 0.0, 0, 0,
                            0.0, NULL);

    xAnimTableNewTransition(animTable, "JumpStart01 Fall01 FallHigh01 DJumpStart01", "LedgeGrab01",
                            LedgeGrabCheck, LedgeGrabCB, 0, 0, 0.0, 0.0, 0x0B, 0, 0.1, NULL);

    xAnimTableNewTransition(animTable, "LedgeGrab01", "Idle01", NULL, LedgeFinishCB, 0x10, 0, 0.0,
                            0.0, 0, 0, 0.1, NULL);

    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Walk01 Walk02 Walk03 Run01 Run02 Run03 RunOutOfWorld01 SlipRun01 Fall01 FallHigh01 Land01 LandHigh01",
        "ButtSlide01", SlideTrackCheck, SlideTrackCB, 0, 0, 0.0, 0.0, 9, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "Carry_PickupFail Carry_Idle Carry_Walk Carry_Throw",
                            "ButtSlide01", SlideTrackCheck, SlideTrackCB, 0, 0, 0.0, 0.0, 9, 0,
                            0.15, NULL);

    xAnimTableNewTransition(animTable, "ButtSlide01", "Idle01", NoslideTrackCheck, NoslideTrackCB,
                            0, 0, 0.0, 0.0, 0x09, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "ButtSlide01", "Fall01", TrackFallCheck, TrackFallCB, 0, 0,
                            0.0, 0.0, 0x09, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "ButtSlide01", "JumpStart01", JumpCheck, JumpCB, 0, 0, 0.0,
                            0.0, 0x0A, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "JumpStart01 Fall01 FallHigh01", "JumpStart01",
                            TrackPrefallJumpCheck, JumpCB, 0, 0, 0.0, 0.0, 0x0F, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "Idle01", "Idle03", InactiveCheck, InactiveCB, 0, 0x080000,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Inactive01", InactiveCheck, InactiveCB, 0,
                            0x080001, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Inactive02", InactiveCheck, InactiveCB, 0,
                            0x080002, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Inactive03", InactiveCheck, InactiveCB, 0,
                            0x080003, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Inactive04", InactiveCheck, InactiveCB, 0,
                            0x080004, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Inactive05", InactiveCheck, InactiveCB, 0,
                            0x080005, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Inactive06", InactiveCheck, InactiveCB, 0,
                            0x080006, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Inactive07", InactiveCheck, InactiveCB, 0,
                            0x080007, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle03", "Idle01", InactiveFinishedCheck, IdleCB, 0,
                            0x000000, 0.0, 0.0, 0, 0, 0.15, NULL);

    xAnimTableNewTransition(
        animTable, "Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 ",
        "Idle01", NULL, IdleCB, 0x10, 0, 0.0, 0.0, 0, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "Hit01 Hit02 Hit03 Hit04", "Idle01", NULL, IdleCB, 0x10, 0,
                            0.0, 0.0, 0, 0, 0.15, NULL);

    xAnimTransition* pTran = xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Walk01 Walk02 Walk03 Run01 Run02 Run03 RunOutOfWorld01 SlipRun01",
        "Defeated01", Defeated01Check, DefeatedCB, 0, 2, 0.0, 0.0, 0x0F, 0, 0.15, NULL);

    xAnimTableAddTransition(
        animTable, pTran,
        "JumpStart01 Fall01 Land01 DJumpStart01 FallHigh01 LandHigh01 LedgeGrab01 StunJump");
    xAnimTableAddTransition(animTable, pTran,
                            "StunFall StunLand Hit01 Hit02 Hit03 Hit04 Melee01 ButtSlide01");
    xAnimTableAddTransition(
        animTable, pTran,
        "Carry_Pickup Carry_PickupFail Carry_Idle Carry_Walk Carry_Throw Carry_PickupItem");
    xAnimTableAddTransition(animTable, pTran, "Carry_IdleItem Carry_WalkItem Carry_ThrowItem");

    pTran = xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Walk01 Walk02 Walk03 Run01 Run02 Run03 RunOutOfWorld01 SlipRun01",
        "Defeated02", Defeated02Check, DefeatedCB, 0, 2, 0.0, 0.0, 0x0F, 0, 0.15, NULL);

    xAnimTableAddTransition(
        animTable, pTran,
        "JumpStart01 Fall01 Land01 DJumpStart01 FallHigh01 LandHigh01 LedgeGrab01 StunJump");
    xAnimTableAddTransition(animTable, pTran,
                            "StunFall StunLand Hit01 Hit02 Hit03 Hit04 Melee01 ButtSlide01");
    xAnimTableAddTransition(
        animTable, pTran,
        "Carry_Pickup Carry_PickupFail Carry_Idle Carry_Walk Carry_Throw Carry_PickupItem");
    xAnimTableAddTransition(animTable, pTran, "Carry_IdleItem Carry_WalkItem Carry_ThrowItem");

    pTran = xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Walk01 Walk02 Walk03 Run01 Run02 Run03 RunOutOfWorld01 SlipRun01",
        "DefeatedGoo01", GooCheck, GooDeathCB, 0, 0, 0.0, 0.0, 0x10, 0, 0.15, NULL);

    xAnimTableAddTransition(
        animTable, pTran,
        "JumpStart01 Fall01 Land01 DJumpStart01 FallHigh01 LandHigh01 LedgeGrab01 StunJump");
    xAnimTableAddTransition(animTable, pTran,
                            "StunFall StunLand Hit01 Hit02 Hit03 Hit04 Melee01 ButtSlide01");
    xAnimTableAddTransition(
        animTable, pTran,
        "Carry_Pickup Carry_PickupFail Carry_Idle Carry_Walk Carry_Throw Carry_PickupItem");
    xAnimTableAddTransition(animTable, pTran, "Carry_IdleItem Carry_WalkItem Carry_ThrowItem");

    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 ",
        "Talk01", TalkCheck, NULL, 0, 0, 0.0, 0.0, 0x14, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 ",
        "Talk02", TalkCheck, NULL, 0, 1, 0.0, 0.0, 0x14, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 ",
        "Talk03", TalkCheck, NULL, 0, 2, 0.0, 0.0, 0x14, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 ",
        "Talk04", TalkCheck, NULL, 0, 3, 0.0, 0.0, 0x14, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "Talk01", "Idle01", TalkDoneCheck, IdleCB, 0, 0, 0.0, 0.0,
                            0x14, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Talk02", "Idle01", TalkDoneCheck, IdleCB, 0, 1, 0.0, 0.0,
                            0x14, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Talk03", "Idle01", TalkDoneCheck, IdleCB, 0, 2, 0.0, 0.0,
                            0x14, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Talk04", "Idle01", TalkDoneCheck, IdleCB, 0, 3, 0.0, 0.0,
                            0x14, 0, 0.15, NULL);

    return animTable;
}

xAnimTable* zEntPlayer_AnimTable()
{
    static const char* const STANDARD_STATES[33] = {
        "Idle01",     "Walk01",     "Run01",      "Run02",      "Run03",      "RunOutOfWorld01",
        "SlipRun01",  "SlipIdle01", "Land01",     "LandHigh01", "Idle02",     "Idle03",
        "Idle04",     "Idle05",     "Idle06",     "Idle07",     "Idle08",     "Idle09",
        "Idle10",     "Idle11",     "Idle12",     "Idle13",     "Inactive01", "Inactive02",
        "Inactive03", "Inactive04", "Inactive05", "Inactive06", "Inactive07", "Inactive08",
        "Inactive09", "Inactive10", NULL
    };

    static const char* const HIT_STATES[64] = { "Idle01",
                                          "SlipIdle01",
                                          "Walk01",
                                          "Run01",
                                          "Run02",
                                          "Run03",
                                          "SlipRun01",
                                          "Land01",
                                          "LandHigh01",
                                          "LandRun01",
                                          "Idle02",
                                          "Idle03",
                                          "Idle04",
                                          "Idle05",
                                          "Idle06",
                                          "Idle07",
                                          "Idle08",
                                          "Idle09",
                                          "Idle10",
                                          "Idle11",
                                          "Idle12",
                                          "Idle13",
                                          "Inactive01",
                                          "Inactive02",
                                          "Inactive03",
                                          "Inactive04",
                                          "Inactive05",
                                          "Inactive06",
                                          "Inactive07",
                                          "Inactive08",
                                          "Inactive09",
                                          "Inactive10",
                                          "TongueStart01",
                                          "TongueSlide01",
                                          "TongueJump01",
                                          "TongueJumpXtra01",
                                          "TongueDJumpApex01",
                                          "TongueLand01",
                                          "JumpStart01",
                                          "JumpApex01",
                                          "DJumpStart01",
                                          "DJumpLift01",
                                          "Fall01",
                                          "Bspin01",
                                          "BbashStart01",
                                          "BbashAttack01",
                                          "BbashStrike01",
                                          "BbounceStart01",
                                          "BbounceAttack01",
                                          "BbounceStrike01",
                                          "BounceStart01",
                                          "BounceLift01",
                                          "BounceApex01",
                                          "Bbowl01",
                                          "BbowlStart01",
                                          "BbowlWindup01",
                                          "BbowlToss01",
                                          "BbowlRecover01",
                                          "WallLaunch01",
                                          "WallFlight01",
                                          "WallFlight02",
                                          "WallLand01",
                                          "WallFall01",
                                          NULL };

    xAnimTable* animTable = xAnimTableNew("SB", NULL, 0);

    xAnimTableNewState(animTable, "Idle01", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Idle02", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Idle03", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Idle04", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Idle05", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Idle06", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Idle07", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Idle08", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Idle09", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Idle10", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Idle11", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Idle12", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Idle13", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "SlipIdle01", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Inactive01", 0x20, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Inactive02", 0x20, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Inactive03", 0x20, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Inactive04", 0x20, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Inactive05", 0x20, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Inactive06", 0x20, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Inactive07", 0x20, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Inactive08", 0x20, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Inactive09", 0x20, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Inactive10", 0x20, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Walk01", 0x10, 0x0044, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Run01", 0x10, 0x0046, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Run02", 0x10, 0x0046, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Run03", 0x10, 0x0046, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "RunOutOfWorld01", 0x10, 0x0046, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "SlipRun01", 0x10, 0x0046, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "JumpStart01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "JumpLift01", 0x10, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "JumpApex01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Fall01", 0x10, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Land01", 0x20, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "LandRun01", 0x20, 0x0006, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "BounceStart01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "BounceLift01", 0x10, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "BounceApex01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "DJumpStart01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "DJumpLift01", 0x10, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "FallHigh01", 0x20, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "LandHigh01", 0x20, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Bspin01", 0x20, 0x080A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "BbashAttack01", 0x10, 0x4000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "BbashStart01", 0x20, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "BbashStrike01", 0x20, 0x4000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "BbashMiss01", 0x20, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "BbounceAttack01", 0x10, 0x4000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "BbounceStart01", 0x20, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "BbounceStrike01", 0x20, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Bbowl01", 0x20, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "BbowlStart01", 0x20, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "BbowlWindup01", 0x10, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "BbowlToss01", 0x20, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "BbowlRecover01", 0x20, 0x0080, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "LedgeGrab01", 0x20, 0x0080, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Hit01", 0x20, 0x0080, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Hit02", 0x20, 0x0080, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Hit03", 0x20, 0x0080, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Hit04", 0x20, 0x0080, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Hit05", 0x20, 0x0080, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Defeated01", 0x00, 0x0480, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Defeated02", 0x00, 0x0480, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Defeated03", 0x00, 0x0480, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Defeated04", 0x00, 0x0480, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Defeated05", 0x00, 0x0480, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "TongueSlide01", 0x10, 0x1840, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "TongueStart01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "TongueJump01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "TongueJumpXtra01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "TongueDJumpApex01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "TongueFall01", 0x10, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "TongueLand01", 0x20, 0x1800, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "TongueTumble01", 0x20, 0x1800, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Goo01", 0x10, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Goo02", 0x20, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "GooDefeated", 0x00, 0x0480, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "WallLaunch01", 0x20, 0x008A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "WallFlight01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "WallFlight02", 0x10, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "WallLand01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "WallFall01", 0x10, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "BoulderRoll01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "BoulderRoll02", 0x10, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Talk04", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Talk03", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Talk02", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Talk01", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "SpatulaGrab01", 0x20, 0x0080, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);

    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 Idle05 Idle06 Idle07 Idle08 Idle09 Idle10 Idle11 Idle12 Idle13 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Inactive08 Inactive09 Inactive10 Walk01 Run01 Run02 Run03 Land01 LandRun01",
        "SpatulaGrab01", SpatulaGrabCheck, SpatulaGrabCB, 0, 0, 0.0, 0.0, 2, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "SpatulaGrab01", "Idle01", NULL, SpatulaGrabStopCB, 0x10, 0,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle02", "Idle01", InactiveFinishedCheck, IdleCB, 0x00, 0,
                            0.0, 0.0, 0, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle03", "Idle01", InactiveFinishedCheck, IdleCB, 0x00, 0,
                            0.0, 0.0, 0, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle04", "Idle01", InactiveFinishedCheck, IdleCB, 0x00, 0,
                            0.0, 0.0, 0, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle05", "Idle01", InactiveFinishedCheck, IdleCB, 0x00, 0,
                            0.0, 0.0, 0, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle06", "Idle01", InactiveFinishedCheck, IdleCB, 0x00, 0,
                            0.0, 0.0, 0, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle07", "Idle01", InactiveFinishedCheck, IdleCB, 0x00, 0,
                            0.0, 0.0, 0, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle08", "Idle01", InactiveFinishedCheck, IdleCB, 0x00, 0,
                            0.0, 0.0, 0, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle09", "Idle01", InactiveFinishedCheck, IdleCB, 0x00, 0,
                            0.0, 0.0, 0, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle11", "Idle01", InactiveFinishedCheck, IdleCB, 0x00, 0,
                            0.0, 0.0, 0, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle12", "Idle01", InactiveFinishedCheck, IdleCB, 0x00, 0,
                            0.0, 0.0, 0, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Inactive01", "Idle01", NULL, IdleCB, 0x10, 0, 0.0, 0.0, 0,
                            0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Inactive02", "Idle01", NULL, IdleCB, 0x10, 0, 0.0, 0.0, 0,
                            0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Inactive03", "Idle01", NULL, IdleCB, 0x10, 0, 0.0, 0.0, 0,
                            0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Inactive04", "Idle01", NULL, IdleCB, 0x10, 0, 0.0, 0.0, 0,
                            0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Inactive05", "Idle01", NULL, IdleCB, 0x10, 0, 0.0, 0.0, 0,
                            0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Inactive06", "Idle01", NULL, IdleCB, 0x10, 0, 0.0, 0.0, 0,
                            0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Inactive07", "Idle01", NULL, IdleCB, 0x10, 0, 0.0, 0.0, 0,
                            0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Inactive08", "Idle01", NULL, IdleCB, 0x10, 0, 0.0, 0.0, 0,
                            0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Inactive09", "Idle01", NULL, IdleCB, 0x10, 0, 0.0, 0.0, 0,
                            0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Inactive10", "Idle01", NULL, IdleCB, 0x10, 0, 0.0, 0.0, 0,
                            0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Land01", "Idle01", NULL, IdleCB, 0x10, 0, 0.0, 0.0, 0, 0,
                            0.10, NULL);
    xAnimTableNewTransition(animTable, "LandHigh01", "Idle01", NULL, IdleCB, 0x10, 0, 0.0, 0.0, 0,
                            0, 0.10, NULL);

    xAnimTableNewTransition(animTable, "Idle01 Walk01 Run01 Run02 Run03 RunOutOfWorld01 SlipRun01",
                            "SlipIdle01", IdleSlipCheck, IdleCB, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "LandRun01", "SlipIdle01", IdleSlipCheck, IdleCB, 0,
                            0x00000000, 0.0, 0.0, 1, 0, 0.10, NULL);
    xAnimTableNewTransition(animTable, "SlipIdle01", "Idle01", IdleCheck, IdleCB, 0, 0x00000000,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Walk01", "Idle01", IdleCheck, IdleCB, 0, 0x00000000, 0.0,
                            0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Run01", "Idle01", IdleCheck, IdleCB, 0, 0x00000000, 0.0,
                            0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Run02", "Idle10", IdleVictoryCheck, IdleCB, 0, 0x00000000,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Run02", "Idle01", IdleStoicCheck, IdleCB, 0, 0x00000000,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle10", "Idle01", IdleStoicCheck, IdleCB, 0, 0x00000000,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Idle10", IdleVictoryCheck, IdleCB, 0, 0x00000000,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Run03", "Idle13", IdleScaredCheck, IdleCB, 0, 0x00000000,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Run03", "Idle01", IdleStoicCheck, IdleCB, 0, 0x00000000,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle13", "Idle01", IdleStoicCheck, IdleCB, 0, 0x00000000,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Idle13", IdleScaredCheck, IdleCB, 0, 0x00000000,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LandRun01", "Idle01", IdleCheck, IdleCB, 0, 0x00000000, 0.0,
                            0.0, 1, 0, 0.10, NULL);
    xAnimTableNewTransition(animTable, "SlipRun01", "Idle01", IdleCheck, IdleCB, 0, 0x00000000, 0.0,
                            0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Idle02", InactiveCheck, InactiveCB, 0, 0x00140000,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Idle03", InactiveCheck, InactiveCB, 0, 0x00140001,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Idle04", InactiveCheck, InactiveCB, 0, 0x00140002,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Idle05", InactiveCheck, InactiveCB, 0, 0x00140003,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Idle06", InactiveCheck, InactiveCB, 0, 0x00140004,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Idle07", InactiveCheck, InactiveCB, 0, 0x00140005,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Idle08", InactiveCheck, InactiveCB, 0, 0x00140006,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Idle09", InactiveCheck, InactiveCB, 0, 0x00140007,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Idle11", InactiveCheck, InactiveCB, 0, 0x00140008,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Idle12", InactiveCheck, InactiveCB, 0, 0x00140009,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Inactive01", InactiveCheck, InactiveCB, 0,
                            0x0014000a, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Inactive02", InactiveCheck, InactiveCB, 0,
                            0x0014000b, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Inactive03", InactiveCheck, InactiveCB, 0,
                            0x0014000c, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Inactive04", InactiveCheck, InactiveCB, 0,
                            0x0014000d, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Inactive05", InactiveCheck, InactiveCB, 0,
                            0x0014000e, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Inactive06", InactiveCheck, InactiveCB, 0,
                            0x0014000f, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Inactive07", InactiveCheck, InactiveCB, 0,
                            0x00140010, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Inactive08", InactiveCheck, InactiveCB, 0,
                            0x00140011, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Inactive09", InactiveCheck, InactiveCB, 0,
                            0x00140012, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Inactive10", InactiveCheck, InactiveCB, 0,
                            0x00140013, 0.0, 0.0, 1, 0, 0.15, NULL);

    xAnimTransition* tranTbl1[8];

    tranTbl1[0] =
        xAnimTableNewTransition(animTable, STANDARD_STATES[0], "Walk01", WalkCheck, NoSlipCB, 0,
                                0x00000000, 0.0f, 0.0f, 0x01, 0, 0.15f, NULL);
    tranTbl1[1] =
        xAnimTableNewTransition(animTable, STANDARD_STATES[0], "Run01", RunStoicCheck, NoSlipCB, 0,
                                0x00000000, 0.0f, 0.0f, 0x01, 0, 0.15f, NULL);
    tranTbl1[2] =
        xAnimTableNewTransition(animTable, STANDARD_STATES[0], "Run02", RunVictoryCheck, NoSlipCB,
                                0, 0x00000000, 0.0f, 0.0f, 0x03, 0, 0.15f, NULL);
    tranTbl1[3] =
        xAnimTableNewTransition(animTable, STANDARD_STATES[0], "Run03", RunScaredCheck, NoSlipCB, 0,
                                0x00000000, 0.0f, 0.0f, 0x02, 0, 0.15f, NULL);
    tranTbl1[4] =
        xAnimTableNewTransition(animTable, STANDARD_STATES[0], "SlipRun01", RunSlipCheck, SlipRunCB,
                                0, 0x00000000, 0.0f, 0.0f, 0x02, 0, 0.15f, NULL);
    tranTbl1[5] = xAnimTableNewTransition(animTable, STANDARD_STATES[0], "RunOutOfWorld01",
                                          RunOutOfWorldCheck, NoSlipCB, 0, 0x00200100, 0.0f, 0.0f,
                                          0x02, 0, 0.15f, NULL);
    tranTbl1[6] =
        xAnimTableNewTransition(animTable, STANDARD_STATES[0], "Fall01", FallCheck, NoSlipCB, 0,
                                0x00000000, 0.0f, 0.0f, 0x01, 0, 0.15f, NULL);
    tranTbl1[7] =
        xAnimTableNewTransition(animTable, STANDARD_STATES[0], "BbashStart01", BubbleBashCheck,
                                BubbleBashCB, 0, 0x00000000, 0.0f, 0.0f, 0x0A, 0, 0.00f, NULL);

    for (int i = 1; STANDARD_STATES[i] != NULL; i++)
    {
        for (U32 a = 0; a < 8; a++)
        {
            if ((strcmp(STANDARD_STATES[i], tranTbl1[a]->Dest->Name) != 0) &&
                ((i != 5 || ((S32)a != 7))))
            {
                xAnimTableAddTransition(animTable, tranTbl1[a], STANDARD_STATES[i]);
            }
        }
    }

    xAnimTableAddTransition(animTable, tranTbl1[4], "LandRun01");
    xAnimTableAddTransition(animTable, tranTbl1[5], "LandRun01");
    xAnimTableAddTransition(animTable, tranTbl1[6], "LandRun01");
    xAnimTableAddTransition(animTable, tranTbl1[7], "LandRun01");

    xAnimTableNewTransition(animTable, "LandRun01", "Walk01", WalkCheck, NULL, 0x10, 0x00000000,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LandRun01", "Run01", RunStoicCheck, NULL, 0x10, 0x00000000,
                            0.0, 0.0, 1, 0, 0.10, NULL);
    xAnimTableNewTransition(animTable, "LandRun01", "Run02", RunVictoryCheck, NULL, 0x10,
                            0x00000000, 0.0, 0.0, 3, 0, 0.10, NULL);
    xAnimTableNewTransition(animTable, "LandRun01", "Run03", RunScaredCheck, NULL, 0x10, 0x00000000,
                            0.0, 0.0, 2, 0, 0.10, NULL);
    xAnimTableNewTransition(animTable, "LandRun01", "RunOutOfWorld01", RunOutOfWorldCheck, NULL,
                            0x10, 0x00200100, 0.0, 0.0, 3, 0, 0.10, NULL);
    xAnimTableNewTransition(animTable, "LandRun01", "SlipRun01", RunSlipCheck, SlipRunCB, 0x10,
                            0x00000000, 0.0, 0.0, 2, 0, 0.10, NULL);

    xAnimTransition* pTran = xAnimTableNewTransition(animTable, "Idle01", "JumpStart01", JumpCheck,
                                                     JumpCB, 0, 0, 0.0, 0.0, 10, 0, 0.15, NULL);

    xAnimTableAddTransition(animTable, pTran, "Walk01");
    xAnimTableAddTransition(animTable, pTran, "Run01");
    xAnimTableAddTransition(animTable, pTran, "Run02");
    xAnimTableAddTransition(animTable, pTran, "Run03");
    xAnimTableAddTransition(animTable, pTran, "RunOutOfWorld01");
    xAnimTableAddTransition(animTable, pTran, "SlipRun01");
    xAnimTableAddTransition(animTable, pTran, "SlipIdle01");
    xAnimTableAddTransition(animTable, pTran, "Land01");
    xAnimTableAddTransition(animTable, pTran, "LandHigh01");
    xAnimTableAddTransition(animTable, pTran, "LandRun01");
    xAnimTableAddTransition(animTable, pTran, "Idle02");
    xAnimTableAddTransition(animTable, pTran, "Idle03");
    xAnimTableAddTransition(animTable, pTran, "Idle04");
    xAnimTableAddTransition(animTable, pTran, "Idle05");
    xAnimTableAddTransition(animTable, pTran, "Idle06");
    xAnimTableAddTransition(animTable, pTran, "Idle07");
    xAnimTableAddTransition(animTable, pTran, "Idle08");
    xAnimTableAddTransition(animTable, pTran, "Idle09");
    xAnimTableAddTransition(animTable, pTran, "Idle10");
    xAnimTableAddTransition(animTable, pTran, "Idle11");
    xAnimTableAddTransition(animTable, pTran, "Idle12");
    xAnimTableAddTransition(animTable, pTran, "Idle13");
    xAnimTableAddTransition(animTable, pTran, "Inactive01");
    xAnimTableAddTransition(animTable, pTran, "Inactive02");
    xAnimTableAddTransition(animTable, pTran, "Inactive03");
    xAnimTableAddTransition(animTable, pTran, "Inactive04");
    xAnimTableAddTransition(animTable, pTran, "Inactive05");
    xAnimTableAddTransition(animTable, pTran, "Inactive06");
    xAnimTableAddTransition(animTable, pTran, "Inactive07");
    xAnimTableAddTransition(animTable, pTran, "Inactive08");
    xAnimTableAddTransition(animTable, pTran, "Inactive09");
    xAnimTableAddTransition(animTable, pTran, "Inactive10");
    xAnimTableAddTransition(animTable, pTran, "Goo01");

    pTran = xAnimTableNewTransition(animTable, "Idle01", "BounceStart01", BounceCheck, BounceCB, 0,
                                    0, 0.0, 0.0, 0x0f, 0, 0.15, NULL);

    xAnimTableAddTransition(animTable, pTran, "SlipIdle01");
    xAnimTableAddTransition(animTable, pTran, "Walk01");
    xAnimTableAddTransition(animTable, pTran, "Run01");
    xAnimTableAddTransition(animTable, pTran, "Run02");
    xAnimTableAddTransition(animTable, pTran, "Run03");
    xAnimTableAddTransition(animTable, pTran, "RunOutOfWorld01");
    xAnimTableAddTransition(animTable, pTran, "SlipRun01");
    xAnimTableAddTransition(animTable, pTran, "Land01");
    xAnimTableAddTransition(animTable, pTran, "LandHigh01");
    xAnimTableAddTransition(animTable, pTran, "LandRun01");
    xAnimTableAddTransition(animTable, pTran, "JumpStart01");
    xAnimTableAddTransition(animTable, pTran, "JumpApex01");
    xAnimTableAddTransition(animTable, pTran, "DJumpStart01");
    xAnimTableAddTransition(animTable, pTran, "DJumpLift01");
    xAnimTableAddTransition(animTable, pTran, "Fall01");
    xAnimTableAddTransition(animTable, pTran, "BbounceAttack01");
    xAnimTableAddTransition(animTable, pTran, "BbounceStart01");
    xAnimTableAddTransition(animTable, pTran, "BbashMiss01");

    xAnimTransition* tranTbl2[11];
    tranTbl2[0] = xAnimTableNewTransition(animTable, HIT_STATES[0], "Hit01", Hit01Check, Hit01CB, 0,
                                          0, 0.0, 0.0, 0x14, 0, 0.15, NULL);
    tranTbl2[1] = xAnimTableNewTransition(animTable, HIT_STATES[0], "Hit02", Hit02Check, Hit02CB, 0,
                                          0, 0.0, 0.0, 0x14, 0, 0.15, NULL);
    tranTbl2[2] = xAnimTableNewTransition(animTable, HIT_STATES[0], "Hit03", Hit03Check, Hit03CB, 0,
                                          0, 0.0, 0.0, 0x14, 0, 0.15, NULL);
    tranTbl2[3] = xAnimTableNewTransition(animTable, HIT_STATES[0], "Hit04", Hit04Check, Hit04CB, 0,
                                          0, 0.0, 0.0, 0x14, 0, 0.15, NULL);
    tranTbl2[4] = xAnimTableNewTransition(animTable, HIT_STATES[0], "Hit05", Hit05Check, Hit05CB, 0,
                                          0, 0.0, 0.0, 0x14, 0, 0.15, NULL);
    tranTbl2[5] = xAnimTableNewTransition(animTable, HIT_STATES[0], "Defeated01", Defeated01Check,
                                          DefeatedCB, 0, 5, 0.0, 0.0, 0x1e, 0, 0.15, NULL);
    tranTbl2[6] = xAnimTableNewTransition(animTable, HIT_STATES[0], "Defeated02", Defeated02Check,
                                          DefeatedCB, 0, 5, 0.0, 0.0, 0x1e, 0, 0.15, NULL);
    tranTbl2[7] = xAnimTableNewTransition(animTable, HIT_STATES[0], "Defeated03", Defeated03Check,
                                          DefeatedCB, 0, 5, 0.0, 0.0, 0x1e, 0, 0.15, NULL);
    tranTbl2[8] = xAnimTableNewTransition(animTable, HIT_STATES[0], "Defeated04", Defeated04Check,
                                          DefeatedCB, 0, 5, 0.0, 0.0, 0x1e, 0, 0.15, NULL);
    tranTbl2[9] = xAnimTableNewTransition(animTable, HIT_STATES[0], "Defeated05", Defeated05Check,
                                          DefeatedCB, 0, 5, 0.0, 0.0, 0x1e, 0, 0.15, NULL);
    tranTbl2[10] = xAnimTableNewTransition(animTable, HIT_STATES[0], "GooDefeated", GooCheck,
                                           GooDeathCB, 0, 0, 0.0, 0.0, 0x1e, 0, 0.15, NULL);

    for (U32 i = 1; HIT_STATES[i] != NULL; i++)
    {
        for (U32 a = 0; a < 11; a++)
        {
            xAnimTableAddTransition(animTable, tranTbl2[a], HIT_STATES[i]);
        }
    }

    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 Idle05 Idle06 Idle07 Idle08 Idle09 Idle10 Idle11 Idle12 Idle13 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Inactive08 Inactive09 Inactive10 Walk01 Run01 Run02 Run03 RunOutOfWorld01 Land01 LandHigh01 LandRun01 SlipIdle01 SlipRun01",
        "TongueStart01", SlideTrackCheck, SlideTrackCB, 0, 0, 0.0, 0.0, 9, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "TongueSlide01", "Idle01", NoslideTrackCheck, NoslideTrackCB,
                            0x00, 0, 0.0, 0.0, 9, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "TongueStart01", "TongueSlide01", NULL, NULL, 0x10, 0, 0.0,
                            0.0, 0, 0, 0.00, NULL);

    xAnimTableNewTransition(animTable, "TongueSlide01 TongueLand01 TongueStart01", "TongueFall01",
                            TrackFallCheck, TrackFallCB, 0, 0, 0.0, 0.0, 9, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "TongueDJumpApex01", "TongueFall01", NULL, NULL, 0x10, 0,
                            0.0, 0.0, 0x00, 0, 0.20, NULL);
    xAnimTableNewTransition(animTable, "TongueJump01", "TongueFall01", NULL, NULL, 0x10, 0, 0.0,
                            0.0, 0x00, 0, 0.08, NULL);
    xAnimTableNewTransition(animTable, "TongueLand01", "TongueSlide01", NULL, NULL, 0x10, 0, 0.0,
                            0.0, 0x00, 0, 0.08, NULL);
    xAnimTableNewTransition(animTable, "TongueSlide01 TongueLand01", "TongueJump01", JumpCheck,
                            JumpCB, 0x00, 0, 0.0, 0.0, 0x0A, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "TongueJump01 TongueFall01", "TongueJump01",
                            TrackPrefallJumpCheck, JumpCB, 0x00, 0, 0.0, 0.0, 0x0F, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "TongueFall01", "TongueDJumpApex01", DblJumpCheck, DblJumpCB,
                            0, 0, 0.0, 0.0, 0x0A, 0, 0.2, NULL);

    xAnimTableNewTransition(animTable,
                            "TongueJump01 TongueJumpXtra01 TongueDJumpApex01 TongueFall01",
                            "TongueLand01", LandTrackCheck, SlideTrackCB, 0, 0, 0.0, 0.0, 1, 0,
                            0.15, NULL);
    xAnimTableNewTransition(animTable,
                            "TongueJump01 TongueJumpXtra01 TongueDJumpApex01 TongueFall01",
                            "Land01", LandNoTrackCheck, NoslideTrackCB, 0, 0, 0.0, 0.0, 1, 0, 0.15,
                            NULL);
    xAnimTableNewTransition(animTable,
                            "TongueJump01 TongueJumpXtra01 TongueDJumpApex01 TongueFall01",
                            "LandRun01", LandNoTrackFastCheck, NULL, 0, 0, 0.0, 0.0, 3, 0, 0.15,
                            NULL);
    xAnimTableNewTransition(animTable,
                            "TongueJump01 TongueJumpXtra01 TongueDJumpApex01 TongueFall01",
                            "Walk01", LandNoTrackWalkCheck, NULL, 0, 0, 0.0, 0.0, 3, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable,
                            "TongueJump01 TongueJumpXtra01 TongueDJumpApex01 TongueFall01",
                            "SlipRun01", LandNoTrackSlipRunCheck, SlipRunCB, 0, 0, 0.0, 0.0, 4, 0,
                            0.15, NULL);
    xAnimTableNewTransition(animTable,
                            "TongueJump01 TongueJumpXtra01 TongueDJumpApex01 TongueFall01",
                            "SlipIdle01", LandNoTrackSlipIdleCheck, NULL, 0, 0, 0.0, 0.0, 4, 0,
                            0.15, NULL);

    xAnimTableNewTransition(animTable, "TongueJump01", "TongueJumpXtra01", DblJumpCheck,
                            TongueDblJumpCB, 0x20, 0, 0.0, 0.0, 0x0A, 0, 0.00, NULL);

    xAnimTableNewTransition(animTable, "TongueJumpXtra01", "TongueDJumpApex01", NULL,
                            TongueDblSpinCB, 0x10, 0, 0.0, 0.0, 1, 0, 0.0, NULL);

    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 Idle05 Idle06 Idle07 Idle08 Idle09 Idle10 Idle11 Idle12 Idle13 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Inactive08 Inactive09 Inactive10 Walk01 Run01 Run02 Run03 RunOutOfWorld01 SlipRun01 SlipIdle01 Land01 LandRun01",
        "BoulderRoll01", BoulderRollCheck, BoulderRollWindupCB, 0, 0, 0.0, 0.0, 0x0A, 0, 0.5, NULL);

    xAnimTableNewTransition(animTable, "BoulderRoll01", "BoulderRoll02", NULL, BoulderRollCB, 0x10,
                            0x00000000, 0.0, 0.0, 0x00, 0, 0.00, NULL);
    xAnimTableNewTransition(animTable, "BoulderRoll02", "SlipIdle01", IdleSlipCheck, NULL, 0x00,
                            0x00000000, 0.0, 0.0, 0x0A, 0, 0.25, NULL);
    xAnimTableNewTransition(animTable, "BoulderRoll02", "Idle01", IdleCheck, NULL, 0x00, 0x00000000,
                            0.0, 0.0, 0x0A, 0, 0.25, NULL);
    xAnimTableNewTransition(animTable, "BoulderRoll02", "Walk01", WalkCheck, NULL, 0x00, 0x00000000,
                            0.0, 0.0, 0x0A, 0, 0.25, NULL);
    xAnimTableNewTransition(animTable, "BoulderRoll02", "Run01", RunStoicCheck, NULL, 0x00,
                            0x00000000, 0.0, 0.0, 0x0A, 0, 0.25, NULL);
    xAnimTableNewTransition(animTable, "BoulderRoll02", "Run02", RunVictoryCheck, NULL, 0x00,
                            0x00000000, 0.0, 0.0, 0x0A, 0, 0.25, NULL);
    xAnimTableNewTransition(animTable, "BoulderRoll02", "Run03", RunScaredCheck, NULL, 0x00,
                            0x00000000, 0.0, 0.0, 0x0A, 0, 0.25, NULL);
    xAnimTableNewTransition(animTable, "BoulderRoll02", "RunOutOfWorld01", RunOutOfWorldCheck, NULL,
                            0x00, 0x00200100, 0.0, 0.0, 0x0A, 0, 0.25, NULL);
    xAnimTableNewTransition(animTable, "BoulderRoll02", "SlipRun01", RunSlipCheck, SlipRunCB, 0x00,
                            0x00000000, 0.0, 0.0, 0x0A, 0, 0.25, NULL);
    xAnimTableNewTransition(animTable, "BoulderRoll02", "Fall01", FallCheck, NULL, 0x00, 0x00000000,
                            0.0, 0.0, 0x0A, 0, 0.25, NULL);

    xAnimTableNewTransition(animTable, "JumpStart01", "JumpApex01", NULL, NULL, 0x10, 0, 0.0, 0.0,
                            0, 0, 0.08, NULL);
    xAnimTableNewTransition(animTable, "JumpApex01", "Fall01", NULL, NULL, 0x10, 0, 0.0, 0.0, 0, 0,
                            0.08, NULL);
    xAnimTableNewTransition(animTable, "BounceStart01", "BounceLift01", NULL, NULL, 0x10, 0, 0.0,
                            0.0, 0, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BounceLift01", "BounceApex01", JumpApexCheck, NULL, 0x00, 0,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BounceApex01", "Fall01", NULL, NULL, 0x10, 0, 0.0, 0.0, 0,
                            0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "DJumpStart01", "DJumpLift01", NULL, NULL, 0x10, 0, 0.0, 0.0,
                            0, 0, 0.00, NULL);
    xAnimTableNewTransition(animTable, "JumpLift01", "JumpApex01", JumpApexCheck, NULL, 0x00, 0,
                            0.0, 0.0, 1, 0, 0.15, NULL);

    xAnimTableNewTransition(
        animTable,
        "JumpStart01 JumpLift01 JumpApex01 BounceStart01 BounceLift01 BounceApex01 Fall01",
        "DJumpStart01", DblJumpCheck, DblJumpCB, 0, 0, 0.0, 0.0, 0x0A, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "JumpStart01 JumpLift01 JumpApex01 Fall01 WallFlight01 WallFlight02 WallLand01 WallFall01 DJumpStart01 DJumpLift01 DJumpApex01",
        "WallLaunch01", WallJumpLaunchCheck, WallJumpLaunchCallback, 0, 0, 0.0, 0.0, 0x0A, 0, 0.15,
        NULL);

    xAnimTableNewTransition(animTable, "WallLaunch01", "WallFlight01", NULL, WallJumpCallback, 0x10,
                            0, 0.0, 0.0, 0, 0, 0.0, NULL);
    xAnimTableNewTransition(animTable, "WallFlight01", "WallFlight02", NULL, NULL, 0x10, 0, 0.0,
                            0.0, 0, 0, 0.0, NULL);

    xAnimTableNewTransition(animTable, "WallFlight01 WallFlight02", "WallLand01",
                            WallJumpFlightLandCheck, WallJumpFlightLandCallback, 0x00, 0, 0.0, 0.0,
                            0, 0, 0.08, NULL);
    xAnimTableNewTransition(animTable, "WallLand01", "WallFlight02", WallJumpLandFlightCheck,
                            WallJumpLandFlightCallback, 0x00, 0, 0.0, 0.0, 0, 0, 0.08, NULL);
    xAnimTableNewTransition(animTable, "WallLand01", "WallFall01", NULL, NULL, 0x10, 0, 0.0, 0.0, 0,
                            0, 0.15, NULL);

    xAnimTableNewTransition(
        animTable,
        "JumpApex01 Fall01 DJumpLift01 WallFlight01 WallFlight02 WallLand01 WallFall01 BbashMiss01",
        "Land01", LandCheck, LandCallback, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "JumpApex01 Fall01 DJumpLift01 WallFlight01 WallFlight02 WallLand01 WallFall01 BbashMiss01",
        "LandHigh01", LandHighCheck, LandCallback, 0, 0, 0.0, 0.0, 2, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "JumpApex01 Fall01 DJumpLift01 WallFlight01 WallFlight02 WallLand01 WallFall01 BbashMiss01",
        "LandRun01", LandRunCheck, LandCallback, 0, 0, 0.0, 0.0, 3, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "JumpApex01 Fall01 DJumpLift01 WallFlight01 WallFlight02 WallLand01 WallFall01 BbashMiss01",
        "SlipRun01", LandSlipRunCheck, LandSlipRunCallback, 0, 0, 0.0, 0.0, 4, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "JumpApex01 Fall01 DJumpLift01 WallFlight01 WallFlight02 WallLand01 WallFall01 BbashMiss01",
        "SlipIdle01", LandSlipIdleCheck, LandCallback, 0, 0, 0.0, 0.0, 4, 0, 0.15, NULL);

    pTran = xAnimTableNewTransition(animTable, "Idle01", "Bspin01", BubbleSpinCheck, BubbleSpinCB,
                                    0, 0, 0.0, 0.0, 0x0A, 0, 0.05, NULL);

    xAnimTableAddTransition(animTable, pTran, "Idle02");
    xAnimTableAddTransition(animTable, pTran, "Idle03");
    xAnimTableAddTransition(animTable, pTran, "Idle04");
    xAnimTableAddTransition(animTable, pTran, "Idle05");
    xAnimTableAddTransition(animTable, pTran, "Idle06");
    xAnimTableAddTransition(animTable, pTran, "Idle07");
    xAnimTableAddTransition(animTable, pTran, "Idle08");
    xAnimTableAddTransition(animTable, pTran, "Idle09");
    xAnimTableAddTransition(animTable, pTran, "Idle10");
    xAnimTableAddTransition(animTable, pTran, "Idle11");
    xAnimTableAddTransition(animTable, pTran, "Idle12");
    xAnimTableAddTransition(animTable, pTran, "Idle13");
    xAnimTableAddTransition(animTable, pTran, "SlipIdle01");
    xAnimTableAddTransition(animTable, pTran, "Inactive01");
    xAnimTableAddTransition(animTable, pTran, "Inactive02");
    xAnimTableAddTransition(animTable, pTran, "Inactive03");
    xAnimTableAddTransition(animTable, pTran, "Inactive04");
    xAnimTableAddTransition(animTable, pTran, "Inactive05");
    xAnimTableAddTransition(animTable, pTran, "Inactive06");
    xAnimTableAddTransition(animTable, pTran, "Inactive07");
    xAnimTableAddTransition(animTable, pTran, "Inactive09");
    xAnimTableAddTransition(animTable, pTran, "Inactive10");
    xAnimTableAddTransition(animTable, pTran, "Walk01");
    xAnimTableAddTransition(animTable, pTran, "Run01");
    xAnimTableAddTransition(animTable, pTran, "Run02");
    xAnimTableAddTransition(animTable, pTran, "Run03");
    xAnimTableAddTransition(animTable, pTran, "SlipRun01");
    xAnimTableAddTransition(animTable, pTran, "Land01");
    xAnimTableAddTransition(animTable, pTran, "LandHigh01");
    xAnimTableAddTransition(animTable, pTran, "LandRun01");
    xAnimTableAddTransition(animTable, pTran, "JumpStart01");
    xAnimTableAddTransition(animTable, pTran, "JumpLift01");
    xAnimTableAddTransition(animTable, pTran, "JumpApex01");
    xAnimTableAddTransition(animTable, pTran, "BounceStart01");
    xAnimTableAddTransition(animTable, pTran, "BounceLift01");
    xAnimTableAddTransition(animTable, pTran, "BounceApex01");
    xAnimTableAddTransition(animTable, pTran, "DJumpStart01");
    xAnimTableAddTransition(animTable, pTran, "DJumpLift01");
    xAnimTableAddTransition(animTable, pTran, "Fall01");

    pTran = xAnimTableNewTransition(animTable, "JumpStart01", "BbounceStart01", BubbleBounceCheck,
                                    BubbleBounceCB, 0, 0, 0.0, 0.0, 0x0A, 0, 0.15, NULL);

    xAnimTableAddTransition(animTable, pTran, "JumpLift01");
    xAnimTableAddTransition(animTable, pTran, "JumpApex01");
    xAnimTableAddTransition(animTable, pTran, "BounceStart01");
    xAnimTableAddTransition(animTable, pTran, "BounceLift01");
    xAnimTableAddTransition(animTable, pTran, "BounceApex01");
    xAnimTableAddTransition(animTable, pTran, "DJumpStart01");
    xAnimTableAddTransition(animTable, pTran, "DJumpLift01");
    xAnimTableAddTransition(animTable, pTran, "Fall01");
    xAnimTableAddTransition(animTable, pTran, "BbashMiss01");

    xAnimTableNewTransition(animTable, "Bspin01", "SlipIdle01", IdleSlipCheck, IdleCB, 0x10,
                            0x00000000, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Bspin01", "Idle01", IdleCheck, IdleCB, 0x10, 0x00000000,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Bspin01", "Walk01", WalkCheck, NULL, 0x10, 0x00000000, 0.0,
                            0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Bspin01", "Run01", RunStoicCheck, NULL, 0x10, 0x00000000,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Bspin01", "Run03", RunScaredCheck, NULL, 0x10, 0x00000000,
                            0.0, 0.0, 2, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Bspin01", "Run02", RunVictoryCheck, NULL, 0x10, 0x00000000,
                            0.0, 0.0, 3, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Bspin01", "RunOutOfWorld01", RunOutOfWorldCheck, NULL, 0x10,
                            0x00200100, 0.0, 0.0, 3, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Bspin01", "SlipRun01", RunSlipCheck, SlipRunCB, 0x10,
                            0x00000000, 0.0, 0.0, 4, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Bspin01", "Fall01", NULL, NULL, 0x10, 0x00000000, 0.0, 0.0,
                            1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BbashStart01", "BbashAttack01", NULL, NULL, 0x10,
                            0x00000000, 0.0, 0.0, 1, 0, 0.00, NULL);
    xAnimTableNewTransition(animTable, "BbashStart01", "BbashStrike01", BBashStrikeCheck,
                            BBashStrikeCB, 0x00, 0x00000000, 0.0, 0.0, 2, 0, 0.10, NULL);
    xAnimTableNewTransition(animTable, "BbashAttack01", "BbashStrike01", BBashStrikeCheck,
                            BBashStrikeCB, 0x00, 0x00000000, 0.0, 0.0, 2, 0, 0.10, NULL);
    xAnimTableNewTransition(animTable, "BbashAttack01", "BbashMiss01", BBashToJumpCheck, NULL, 0x00,
                            0x00000000, 0.0, 0.0, 1, 0, 0.10, NULL);
    xAnimTableNewTransition(animTable, "BbashStrike01", "Fall01", NULL, NULL, 0x10, 0x00000000, 0.0,
                            0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BbashMiss01", "Fall01", NULL, NULL, 0x10, 0x00000000, 0.0,
                            0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BbounceStart01", "BbounceAttack01", NULL, BBounceAttackCB,
                            0x10, 0x00000000, 0.0, 0.0, 1, 0, 0.10, NULL);
    xAnimTableNewTransition(animTable, "BbounceAttack01", "BbounceStrike01", BBounceStrikeCheck,
                            BBounceStrikeCB, 0x00, 0x00000000, 0.0, 0.0, 1, 0, 0.00, NULL);
    xAnimTableNewTransition(animTable, "BbounceAttack01", "JumpLift01", BBounceToJumpCheck,
                            BBounceToJumpCB, 0x00, 0x00000000, 0.0, 0.0, 1, 0, 0.10, NULL);
    xAnimTableNewTransition(animTable, "BbounceStrike01", "SlipIdle01", IdleSlipCheck, IdleCB, 0x10,
                            0x00000000, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BbounceStrike01", "Idle01", IdleCheck, IdleCB, 0x10,
                            0x00000000, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BbounceStrike01", "Walk01", WalkCheck, NULL, 0x10,
                            0x00000000, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BbounceStrike01", "Run01", RunStoicCheck, NULL, 0x10,
                            0x00000000, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BbounceStrike01", "Run03", RunScaredCheck, NULL, 0x10,
                            0x00000000, 0.0, 0.0, 2, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BbounceStrike01", "Run02", RunVictoryCheck, NULL, 0x10,
                            0x00000000, 0.0, 0.0, 3, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BbounceStrike01", "RunOutOfWorld01", RunOutOfWorldCheck,
                            NULL, 0x10, 0x00200100, 0.0, 0.0, 3, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BbounceStrike01", "SlipRun01", RunSlipCheck, SlipRunCB,
                            0x10, 0x00000000, 0.0, 0.0, 4, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable,
                            "JumpStart01 JumpLift01 JumpApex01 Fall01 DJumpStart01 DJumpLift01",
                            "LedgeGrab01", LedgeGrabCheck, LedgeGrabCB, 0, 0, 0.0, 0.0, 0xb, 0, 0.1,
                            NULL);

    xAnimTableNewTransition(animTable, "LedgeGrab01", "Idle01", NULL, LedgeFinishCB, 0x10, 0, 0.0,
                            0.0, 0, 0, 0.1, NULL);

    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 Idle05 Idle06 Idle07 Idle08 Idle09 Idle10 Idle11 Idle12 Idle13 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Inactive09 Inactive10 Walk01 Land01 LandRun01 Run01 Run02 Run03 SlipRun01",
        "BbowlStart01", BbowlCheck, BbowlCB, 0, 0, 0.0, 0.0, 0x0A, 0, 0.1, NULL);

    xAnimTableNewTransition(animTable, "BbowlStart01", "BbowlWindup01", NULL, NULL, 0x10,
                            0x00000000, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BbowlWindup01", "BbowlToss01", BbowlWindupEndCheck, NULL,
                            0x00, 0x00000000, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BbowlToss01", "BbowlRecover01", NULL, BbowlTossEndCB, 0x10,
                            0x00000000, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BbowlRecover01", "Walk01", BbowlRecoverWalkCheck, NULL,
                            0x00, 0x00000000, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BbowlRecover01", "Run01", BbowlRecoverRunCheck, NULL, 0x00,
                            0x00000000, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BbowlRecover01", "Run03", BbowlRecoverRunScaredCheck, NULL,
                            0x00, 0x00000000, 0.0, 0.0, 2, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BbowlRecover01", "Run02", BbowlRecoverRunVictoryCheck, NULL,
                            0x00, 0x00000000, 0.0, 0.0, 3, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BbowlRecover01", "RunOutOfWorld01",
                            BbowlRecoverRunOutOfWorldCheck, NULL, 0x00, 0x00200100, 0.0, 0.0, 3, 0,
                            0.15, NULL);
    xAnimTableNewTransition(animTable, "BbowlRecover01", "SlipRun01", BbowlRecoverRunSlipCheck,
                            SlipRunCB, 0x00, 0x00000000, 0.0, 0.0, 4, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BbowlRecover01", "Idle01", NULL, IdleCB, 0x10, 0x00000000,
                            0.0, 0.0, 1, 0, 0.15, NULL);

    bungee_state::insert_animations(*animTable);
    cruise_bubble::insert_player_animations(*animTable);

    xAnimTableNewTransition(animTable, "Hit01", "Idle01", NULL, IdleCB, 0x10, 0, 0.0, 0.0, 0, 0,
                            0.15, NULL);
    xAnimTableNewTransition(animTable, "Hit02", "Idle01", NULL, IdleCB, 0x10, 0, 0.0, 0.0, 0, 0,
                            0.15, NULL);
    xAnimTableNewTransition(animTable, "Hit03", "Idle01", NULL, IdleCB, 0x10, 0, 0.0, 0.0, 0, 0,
                            0.15, NULL);
    xAnimTableNewTransition(animTable, "Hit04", "Idle01", NULL, IdleCB, 0x10, 0, 0.0, 0.0, 0, 0,
                            0.15, NULL);
    xAnimTableNewTransition(animTable, "Hit05", "Idle01", NULL, IdleCB, 0x10, 0, 0.0, 0.0, 0, 0,
                            0.15, NULL);

    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 Idle05 Idle06 Idle07 Idle08 Idle09 Idle10 Idle11 Idle12 Idle13 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Inactive08 Inactive09 Inactive10 ",
        "Talk01", TalkCheck, NULL, 0, 0, 0.0, 0.0, 0x14, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 Idle05 Idle06 Idle07 Idle08 Idle09 Idle10 Idle11 Idle12 Idle13 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Inactive08 Inactive09 Inactive10 ",
        "Talk02", TalkCheck, NULL, 0, 1, 0.0, 0.0, 0x14, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 Idle05 Idle06 Idle07 Idle08 Idle09 Idle10 Idle11 Idle12 Idle13 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Inactive08 Inactive09 Inactive10 ",
        "Talk03", TalkCheck, NULL, 0, 2, 0.0, 0.0, 0x14, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 Idle05 Idle06 Idle07 Idle08 Idle09 Idle10 Idle11 Idle12 Idle13 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Inactive08 Inactive09 Inactive10 ",
        "Talk04", TalkCheck, NULL, 0, 3, 0.0, 0.0, 0x14, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "Talk01", "Idle01", TalkDoneCheck, NULL, 0, 0, 0.0, 0.0,
                            0x14, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Talk02", "Idle01", TalkDoneCheck, NULL, 0, 1, 0.0, 0.0,
                            0x14, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Talk03", "Idle01", TalkDoneCheck, NULL, 0, 2, 0.0, 0.0,
                            0x14, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Talk04", "Idle01", TalkDoneCheck, NULL, 0, 3, 0.0, 0.0,
                            0x14, 0, 0.15, NULL);

    return animTable;
}

xAnimTable* zSpongeBobTongue_AnimTable()
{
    xAnimTable* animTable = xAnimTableNew("SBTongue", NULL, 0);

    xAnimTableNewState(animTable, "TongueSlide01", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "TongueStart01", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "TongueJump01", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "TongueJumpXtra01", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "TongueDJumpApex01", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "TongueFall01", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "TongueLand01", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);

    return animTable;
}

xAnimTable* zEntPlayer_BoulderVehicleAnimTable()
{
    xAnimTable* table = xAnimTableNew("BoulderVehicleTable", NULL, 0x0);

    xAnimTableNewState(table, "Idle01", 0x10, 0x0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Move01", 0x10, 0x0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);

    xAnimTableNewTransition(table, "Idle01", "Move01", BoulderRollMoveCheck, NULL, 0x0, 0x0, 0.0f,
                            0.0f, 1, 0, 0.45f, NULL);
    xAnimTableNewTransition(table, "Move01", "Idle01", BoulderRollIdleCheck, NULL, 0x0, 0x0, 0.0f,
                            0.0f, 1, 0, 0.45f, NULL);

    return table;
}

static S32 load_talk_filter(U8* filter, xModelAssetParam* params, U32 params_size, S32 max_size)
{
    // Not sure about these variable names.
    F32* non_choice; // Not in DWARF.
    S32 size = 0;
    F32* non_choices = (F32*)xMemPushTemp(max_size * sizeof(F32));
    S32 found = zParamGetFloatList(params, params_size, "NonRandomTalkAnims", max_size, non_choices,
                                   non_choices);

    for (S32 i = 0; i < max_size; i++)
    {
        bool skip = false;
        non_choice = non_choices;
        for (S32 j = 0; j < found; j++)
        {
            if ((S32)*non_choice - 1 == i)
            {
                skip = true;
                break;
            }
            non_choice++;
        }
        if (!skip)
        {
            filter[size] = (U8)i;
            size++;
        }
    }
    if (size <= 0)
    {
        *filter = '\0';
        size = 1;
    }
    xMemPopTemp(non_choices);
    return size;
}

static U32 count_talk_anims(xAnimTable* anims)
{
    xAnimFile* firstData = anims->StateList->Data;
    char talkAnimName[20];
    S32 talkAnimCount = 0;

    sprintf(talkAnimName, "Talk%02d", 1);

    for (xAnimState* state = anims->StateList; state != NULL; state = state->Next)
    {
        if (stricmp(state->Name, talkAnimName) == 0)
        {
            if (state->Data == firstData || ++talkAnimCount >= 4)
            {
                break;
            }
            sprintf(talkAnimName, "Talk%02d", talkAnimCount + 1);
        }
    }

    return talkAnimCount;
}

static void load_player_ini(zPlayerSettings& ps, xModelInstance& model, xModelAssetParam* modelass,
                            U32 params_size)
{
    U32 count;
    count = count_talk_anims(model.Anim->Table);
    ps.talk_anims = count;
    count = load_talk_filter(ps.talk_filter, modelass, params_size, ps.talk_anims);
    ps.talk_filter_size = count;
}

static void load_player_ini()
{
    xModelAssetParam* modelass;
    U32 size[3];

    if (globals.player.model_spongebob != NULL)
    {
        modelass = zEntGetModelParams(globals.player.ent.asset->modelInfoID, &size[2]);
        load_player_ini(globals.player.sb, *globals.player.model_spongebob, modelass, size[2]);
    }

    if (globals.player.model_patrick != NULL)
    {
        modelass = zEntGetModelParams(PATRICK_MODEL_ASSETID, &size[1]);
        load_player_ini(globals.player.patrick, *globals.player.model_patrick, modelass, size[1]);
    }

    if (globals.player.model_sandy != NULL)
    {
        modelass = zEntGetModelParams(SANDY_MODEL_ASSETID, &size[0]);
        load_player_ini(globals.player.sandy, *globals.player.model_sandy, modelass, size[0]);
    }
}

zParEmitter* gEmitBFX;
static F32 sLastBubbleEmit;
static F32 sLastInvulnEmit;

void zEntPlayer_Init(xEnt* ent, xEntAsset* asset)
{
    U8 index;
    xModelInstance* m;
    F32 bbncvtm;
    U32 bufsize;
    xAnimTable* wettbl;
    xAnimState* drystate;
    xAnimFile* wetfile;
    xAnimFile* dryfile;
    S32 aa;
    S32 numa;
    U32 trailerHash;
    static S32 drybob_anim_count;
    static void** drybob_chgData[64];
    static void* drybob_oldData[64];
    static F32* drybob_chgTime[64];
    static F32 drybob_oldTime[64];

    zEntInit((zEnt*)ent, asset, 'PLYR');
    xEntInitShadow(*ent, (xEntShadow&)globals.player.entShadow_embedded);
    ent->simpShadow = &globals.player.simpShadow_embedded;
    xShadowSimple_CacheInit(ent->simpShadow, ent, 80);

    // The light kit ID is stored immediately after the asset's link array.
    if (*(U32*)((xLinkAsset*)(asset + 1) + ent->linkCount) != 0)
    {
        ent->lightKit =
            (xLightKit*)xSTFindAsset(*(U32*)((xLinkAsset*)(asset + 1) + ent->linkCount), NULL);
        if ((ent->lightKit != NULL) && (ent->lightKit->tagID != 'TIKL'))
        {
            ent->lightKit = NULL;
        }
    }

    globals.player.model_spongebob = ent->model;
    memset(&globals.player.sb_models, 0, sizeof(globals.player.sb_models));
    index = 0;

    for (m = globals.player.model_spongebob; m != NULL; m = m->Next)
    {
        for (S32 i = 0; i < 14; i++)
        {
            if (globals.player.sb_model_indices[i] == index)
            {
                globals.player.sb_models[i] = m;
            }
        }
        index++;
    }

    PlayerHackFixBbashMiss(globals.player.model_spongebob);
    globals.player.sb_models[10]->Parent = globals.player.model_spongebob;
    globals.player.sb_models[11]->Parent = globals.player.model_spongebob;
    globals.player.sb_models[12]->Parent = globals.player.model_spongebob;
    globals.player.sb_models[13]->Parent = globals.player.model_spongebob;
    globals.player.sb_models[10]->Flags = 0x2000;
    globals.player.sb_models[11]->Flags = 0x2000;
    globals.player.sb_models[12]->Flags = 0x2000;
    globals.player.sb_models[13]->Flags = 0x2000;
    globals.player.sb_models[10]->Mat = globals.player.model_spongebob->Mat;
    globals.player.sb_models[11]->Mat = globals.player.model_spongebob->Mat;
    globals.player.sb_models[12]->Mat = globals.player.model_spongebob->Mat;
    globals.player.sb_models[13]->Mat = globals.player.model_spongebob->Mat;

    globals.player.model_wand = globals.player.sb_models[5];
    iModelTagSetup(&globals.player.BubbleWandTag[0], globals.player.model_wand->Data, -0.604f,
                   0.46f, 0.63f);
    iModelTagSetup(&globals.player.BubbleWandTag[1], globals.player.model_wand->Data, -0.563f,
                   0.427f, 0.294f);
    iModelTagSetup(&sSpongeBobLFoot, globals.player.sb_models[8]->Data, 0.119f, 0.043f, -0.032f);
    iModelTagSetup(&sSpongeBobRFoot, globals.player.sb_models[9]->Data, -0.119f, 0.043f, -0.032f);
    iModelTagSetup(&sSpongeBobLKnee, globals.player.sb_models[8]->Data, 0.119f, 0.161f, 0.024f);
    iModelTagSetup(&sSpongeBobRKnee, globals.player.sb_models[9]->Data, -0.118f, 0.161f, 0.024f);
    iModelTagSetup(&sSpongeBobLHand, globals.player.sb_models[1]->Data, 0.55f, 0.496f, 0.02f);
    iModelTagSetup(&sSpongeBobRHand, globals.player.sb_models[2]->Data, -0.55f, 0.496f, -0.011f);
    iModelTagSetup(&sSpongeBobRElbow, globals.player.sb_models[2]->Data, -0.442f, 0.458f, -0.007f);
    iModelTagSetup(&sSpongeBobLElbow, globals.player.sb_models[1]->Data, 0.43f, 0.458f, -0.004f);
    iModelTagSetup(sStankTag, globals.player.ent.model->Data, -0.014f, 0.546f, 0.168f);
    sEmitSpinBubbles = zParEmitterFind("PAREMIT_GRAB_BUBBLES");
    sEmitSundae = zParEmitterFind("PAREMIT_CLOUD");
    sEmitStankBreath = zParEmitterFind("PAREMIT_STANK");
    gEmitBFX = zParEmitterFind("PAREMIT_BFX");
    sLassoInfo = &globals.player.lassoInfo;
    sLasso = &sLassoInfo->lasso;

    bbncvtm = globals.player.g.BBashTime - globals.player.g.BBashCVTime;
    bbash_vel =
        (globals.player.g.BBashHeight + 0.5f * globals.player.g.Gravity * bbncvtm * bbncvtm) /
        globals.player.g.BBashTime;

    void* info = xSTFindAsset(PATRICK_MODEL_ASSETID, &bufsize);

    if (info != 0)
    {
        globals.player.model_patrick = zEntRecurseModelInfo(info, ent);
        iModelTagSetup(&sPatrickMelee, globals.player.model_patrick->Data, 0.0f, 0.475f, 0.252f);
        iModelTagSetup(&sPatrickLFoot, globals.player.model_patrick->Data, 0.187f, 0.0f, -0.068f);
        iModelTagSetup(&sPatrickRFoot, globals.player.model_patrick->Data, -0.187f, 0.0f, -0.068f);
        iModelTagSetup(&sPatrickLKnee, globals.player.model_patrick->Data, 0.19f, 0.099f, -0.138f);
        iModelTagSetup(&sPatrickRKnee, globals.player.model_patrick->Data, -0.19f, 0.099f, -0.138f);
        iModelTagSetup(&sPatrickRHand, globals.player.model_patrick->Data, -0.632f, 0.711f,
                       -0.235f);
        iModelTagSetup(&sPatrickLHand, globals.player.model_patrick->Data, 0.684f, 0.694f, -0.215f);
        iModelTagSetup(&sPatrickRElbow, globals.player.model_patrick->Data, -0.475f, 0.733f,
                       -0.269f);
        iModelTagSetup(&sPatrickLElbow, globals.player.model_patrick->Data, 0.475f, 0.733f,
                       -0.269f);
    }
    else
    {
        globals.player.model_patrick = NULL;
    }

    info = xSTFindAsset(-0x3f1cb4dd, &bufsize);

    if (info != 0)
    {
        globals.player.model_sandy = zEntRecurseModelInfo(info, ent);
        zLasso_Init(sLasso, globals.player.model_sandy, -0.599f, 0.645f, 0.051f);
        iModelTagSetup(&sSandyLFoot, globals.player.model_sandy->Data, 0.159f, 0.0f, 0.045f);
        iModelTagSetup(&sSandyRFoot, globals.player.model_sandy->Data, -0.012f, 0.0f, 0.258f);
        iModelTagSetup(&sSandyLKnee, globals.player.model_sandy->Data, 0.129f, 0.287f, 0.089f);
        iModelTagSetup(&sSandyRKnee, globals.player.model_sandy->Data, -0.071f, -0.071f, 0.089f);
        iModelTagSetup(&sSandyRHand, globals.player.model_sandy->Data, -0.642f, 0.747f, 0.006f);
        iModelTagSetup(&sSandyLHand, globals.player.model_sandy->Data, 0.641f, 0.747f, 0.006f);
        iModelTagSetup(&sSandyRElbow, globals.player.model_sandy->Data, -0.37f, 0.661f, 0.086f);
        iModelTagSetup(&sSandyLElbow, globals.player.model_sandy->Data, 0.37f, 0.661f, 0.086f);
    }
    else
    {
        globals.player.model_sandy = NULL;
    }

    for (aa = 0; aa < drybob_anim_count; aa++)
    {
        *drybob_chgData[aa] = drybob_oldData[aa];
        *drybob_chgTime[aa] = drybob_oldTime[aa];
    }

    drybob_anim_count = 0;

    info = xSTFindAsset(xStrHash("spongebob_bind_treedome.dff"), NULL);

    if (info != NULL)
    {
        RpAtomic* treedome0 = (RpAtomic*)info;
        RpAtomic* treedome1 = iModelFile_RWMultiAtomic(treedome0);
        RpAtomic* treedome2 = iModelFile_RWMultiAtomic(treedome1);
        RpAtomic* treedome3 = iModelFile_RWMultiAtomic(treedome2);

        globals.player.sb_models[1]->Data = treedome0;
        globals.player.sb_models[2]->Data = treedome1;
        globals.player.sb_models[0]->Data = treedome3;

        wettbl = (xAnimTable*)xSTFindAsset(xStrHash("spongebob_bind.ATBL"), NULL);
        xAnimTable* drytbl = (xAnimTable*)xSTFindAsset(xStrHash("spongebob_bind_treedome.ATBL"), NULL);

        if (wettbl != NULL && drytbl != NULL)
        {
            for (drystate = drytbl->StateList; drystate != NULL; drystate = drystate->Next)
            {
                if (!(drystate->UserFlags & 0x40000000))
                {
                    xAnimState* wetstate = xAnimTableGetState(wettbl, drystate->Name);
                    wetfile = wetstate->Data;
                    dryfile = drystate->Data;
                    numa = wetfile->NumAnims[0] * wetfile->NumAnims[1];

                    for (aa = 0; aa < numa; aa++)
                    {
                        drybob_chgData[drybob_anim_count] = &wetfile->RawData[aa];
                        drybob_oldData[drybob_anim_count] = wetfile->RawData[aa];
                        drybob_chgTime[drybob_anim_count] = &wetfile->Duration;
                        drybob_oldTime[drybob_anim_count] = wetfile->Duration;
                        wetfile->RawData[aa] = dryfile->RawData[aa];
                        drybob_anim_count++;
                    }

                    wetfile->Duration = dryfile->Duration;
                }
            }
        }
    }

    cruise_bubble::init();
    load_player_ini();
    xEntEnable(ent);
    xEntShow(ent);
    globals.player.Visible = 1;
    globals.player.AutoMoveSpeed = 0;
    ent->pflags &= (U8)~XENT_PFLAGS_HAS_GRAVITY;
    ent->collis->chk &= (U8)~0x1;
    ent->update = zEntPlayer_Update;
    ent->move = zEntPlayer_Move;
    ent->render = (xEntRenderCallback)zEntPlayer_Render;
    ent->eventFunc = zEntPlayerEventCB;

    if (ent->linkCount != 0)
    {
        ent->link = (xLinkAsset*)(asset + 1);
    }
    else
    {
        ent->link = NULL;
    }

    if (globals.sceneCur->baseCount[eBaseTypeGust] != 0)
    {
        xFFXAddEffect(ent, zGustUpdateEnt, &gust_data);
    }

    xFFXRotMatchState* rms = xFFXRotMatchAlloc();

    if (rms != NULL)
    {
        rms->max_decl = globals.player.g.RotMatchMaxAngle;
        rms->tmatch = globals.player.g.RotMatchMatchTime;
        rms->trelax = globals.player.g.RotMatchRelaxTime;
        rms->tmr = 0.0f;
        xFFXAddEffect(ent, PlayerRotMatchUpdateEnt, rms);
    }

    bungee_state::init();
    oob_state::init();

    boulderVehicle = NULL;
    boulderVehicle = (xEntBoulder*)zSceneFindObject(xStrHash("BOULDER_VEHICLE"));

    if (boulderVehicle != NULL)
    {
        boulderVehicle->eventFunc = BoulderVEventCB;
    }

    sNumHitches = 0;
    sHitchAngle = 0.0f;
    trailerHash = xStrHash("trailer_hitch");

    for (aa = 0; aa < globals.sceneCur->num_base; aa++)
    {
        xEnt* hitch = (xEnt*)globals.sceneCur->base[aa];

        if (hitch->baseType == eBaseTypeDestructObj || hitch->baseType == eBaseTypePlatform ||
            hitch->baseType == eBaseTypeStatic)
        {
            if (hitch->asset->modelInfoID == trailerHash)
            {
                sHitch[sNumHitches] = hitch;
                sNumHitches++;
            }
        }
    }

    lastgCurrentPlayer = eCurrentPlayerCount;
    zEntPlayerPreReset();
    zEntPlayerReset(ent);
    zEntPlayer_SNDInit();
    sPlayerDiedLastTime = 0;
    zEntPlayer_RestoreSounds();
}

void zEntPlayer_RestoreSounds()
{
    sPlayerIgnoreSound--;
    if (sPlayerIgnoreSound < 0)
    {
        sPlayerIgnoreSound = 0;
    }
}

void zEntPlayer_Load(xEnt* ent, xSerial* serial)
{
    return;
}

static void zEntPlayer_StreakFX(xEnt* ent, F32)
{
    S32 i;
    S32 p;
    S32 cp = 0;

    for (i = 0; i < 3; i++)
    {
        for (p = 0; p < 4; p++)
        {
            sStreakInfo[i][p].activated = FALSE;
        }
    }

    if (ent->model == globals.player.model_sandy)
    {
        cp = 1;
    }
    else if (ent->model == globals.player.model_patrick)
    {
        cp = 2;
    }

    if (globals.player.SlideTrackSliding & 0x1)
    {
        if (cp == 1)
        {
            sStreakInfo[cp][0].activated = TRUE;
            sStreakInfo[cp][1].activated = TRUE;
        }
        else
        {
            sStreakInfo[cp][2].activated = TRUE;
            sStreakInfo[cp][3].activated = TRUE;
        }
    }
    else if (cp == 1)
    {
        if (strstr(ent->model->Anim->Single->State->Name, "Tail") != NULL)
        {
            sStreakInfo[cp][0].activated = TRUE;
            sStreakInfo[cp][1].activated = TRUE;
        }
    }
    else if (cp == 0)
    {
        if (strcmp(ent->model->Anim->Single->State->Name, "TongueJump01") == 0 ||
            strcmp(ent->model->Anim->Single->State->Name, "TongueJumpXtra01") == 0 ||
            strcmp(ent->model->Anim->Single->State->Name, "TongueDJumpApex01") == 0)
        {
            sStreakInfo[cp][2].activated = TRUE;
            sStreakInfo[cp][3].activated = TRUE;
        }
    }

    if (globals.player.Jump_Springboard != NULL)
    {
        sStreakInfo[cp][0].activated = TRUE;
        sStreakInfo[cp][1].activated = TRUE;
    }

    if (cp == 1)
    {
        if (strcmp(ent->model->Anim->Single->State->Name, "Melee01") == 0)
        {
            sStreakInfo[cp][1].activated = TRUE;
        }
        else if (strcmp(ent->model->Anim->Single->State->Name, "JumpMelee01") == 0)
        {
            sStreakInfo[cp][2].activated = TRUE;
        }
        else if (strcmp(ent->model->Anim->Single->State->Name, "LassoSwing") == 0)
        {
            sStreakInfo[cp][2].activated = TRUE;
            sStreakInfo[cp][3].activated = TRUE;
        }
        else if (globals.player.IsCoptering)
        {
            sStreakInfo[cp][1].activated = TRUE;
        }
    }
    else if (cp == 0)
    {
        if (strcmp(ent->model->Anim->Single->State->Name, "BbowlWindup01") == 0)
        {
            sStreakInfo[cp][1].activated = TRUE;
        }
    }
    else if (cp == 2)
    {
        if (strcmp(ent->model->Anim->Single->State->Name, "Stunfall") == 0 ||
            strcmp(ent->model->Anim->Single->State->Name, "Stunjump") == 0)
        {
            sStreakInfo[cp][1].activated = TRUE;
            sStreakInfo[cp][0].activated = TRUE;
            sStreakInfo[cp][3].activated = TRUE;
            sStreakInfo[cp][2].activated = TRUE;
        }
    }

    for (i = 0; i < 3; i++)
    {
        for (p = 0; p < 4; p++)
        {
            if (sStreakInfo[i][p].activated && sStreakInfo[i][p].streakID == 0xdead)
            {
                sStreakInfo[i][p].streakID =
                    xFXStreakStart(0.0f, 4.0f, sStreakInfo[i][p].alphaStart, 0x0,
                                   &sStreakInfo[i][p].colA, &sStreakInfo[i][p].colB,
                                   sStreakInfo[i][p].streakTaper);
            }
            else
            {
                if (!sStreakInfo[i][p].activated && sStreakInfo[i][p].streakID != 0xdead)
                {
                    xFXStreakStop(sStreakInfo[i][p].streakID);
                    sStreakInfo[i][p].streakID = 0xdead;
                }
            }

            if (sStreakInfo[i][p].streakID == 0xdead)
            {
                continue;
            }

            if (i == 0)
            {
                iModelTagEval(ent->model->Data, sStreakInfo[i][p].tagA,
                              globals.player.model_spongebob->Mat, &sStreakInfo[i][p].a);
                iModelTagEval(ent->model->Data, sStreakInfo[i][p].tagB,
                              globals.player.model_spongebob->Mat, &sStreakInfo[i][p].b);
            }
            else
            {
                iModelTagEval(ent->model->Data, sStreakInfo[i][p].tagA, ent->model->Mat,
                              &sStreakInfo[i][p].a);
                iModelTagEval(ent->model->Data, sStreakInfo[i][p].tagB, ent->model->Mat,
                              &sStreakInfo[i][p].b);
            }

            xFXStreakUpdate(sStreakInfo[i][p].streakID, &sStreakInfo[i][p].a, &sStreakInfo[i][p].b);
        }
    }
}

static void zEntPlayer_SpringboardFX(xEnt* ent, F32 dt)
{
    xVec3 temp1;
    xVec3 temp2;
    xParEmitterCustomSettings info;

    if (globals.player.Jump_Springboard != NULL && ent->frame->vel.y >= 0.0f)
    {
        xVec3Copy(&temp1, (xVec3*)&ent->model->Mat->pos);
        temp1.y += 1.5f;

        xVec3Copy(&temp2, (xVec3*)&ent->model->Mat->right);

        temp2 *= 0.15f;
        xVec3Add(&temp1, &temp1, &temp2);

        if (gPTankDisable)
        {
            static F32 sLastSpringboardBubbleEmit = 0.0f;
            sLastSpringboardBubbleEmit += dt;

            if (!(sLastSpringboardBubbleEmit > 0.02f))
            {
                return;
            }

            sLastSpringboardBubbleEmit = 0.0f;

            info.custom_flags = 0x35E;
            info.pos = temp1;
            info.vel.x = 1.0f;
            info.vel.z = 0.0f;
            info.vel.y = 0.0f;
            info.vel_angle_variation = PI * 2.0f;
            info.rate.set(100.0f, 100.0f, 1.0f, 0);
            info.life.set(1.0f, 1.0f, 1.0f, 0);

            F32 size = 0.15f * xurand() + 0.05f;

            info.size_birth.set(size, size, 1.0f, 0);

            size *= 1.2f;
            info.size_death.set(size, size, 1.0f, 0);

            xParEmitterEmitCustom(sEmitSpinBubbles, dt, &info);
        }
        else
        {
            zFX_SpawnBubbleTrail((xVec3*)&globals.player.ent.model->Mat->pos, 3);
        }
    }
}

F32 sRingDelay;

static void getPadDefl(_tagPadAnalog* stick, class xVec2* v)
{
    v->x = 0.0f;
    v->y = 0.0f;

    if (stick->x > 45.0f || stick->x < -45.0f)
    {
        if (stick->x > 0.0f)
        {
            v->x = 1.0f;
        }
        else
        {
            v->x = -1.0f;
        }

        if (stick->x < globals.player.g.AnalogMax && stick->x > 0)
        {
            v->x = (stick->x - 45.0f) / (127.0f - globals.player.g.AnalogMin);
        }
        else if (stick->x > -127.0f && stick->x < 0)
        {
            v->x = (45.0f + stick->x) / (127.0f - globals.player.g.AnalogMin);
        }
    }

    if (stick->y > 45.0f || stick->y < -45.0f)
    {
        if (stick->y > 0.0f)
        {
            v->y = 1.0f;
        }
        else
        {
            v->y = -1.0f;
        }

        if (stick->y < 127.0f && stick->y > 0)
        {
            v->y = (stick->y - 45.0f) / 82.0f;
        }
        else if (stick->y > -127.0f && stick->y < 0)
        {
            v->y = (45.0f + stick->y) / 82.0f;
        }
    }
}

static S32 BoulderVEventCB(xBase* from, xBase* to, U32 toEvent, const F32* toParam,
                           xBase* toParamWidget)
{
    if (toEvent == eEventKill || toEvent == eEventHit)
    {
        boulderRollShouldEnd = 1;
    }

    return xEntBoulderEventCB(from, to, toEvent, toParam, toParamWidget);
}

static void zEntPlayer_BoulderVehicleRender(zEnt* ent)
{
    xShadow_ListAdd(boulderVehicle);
}

static void zEntPlayer_BoulderVehicleMove(xEnt* ent, xScene* scn, F32, xEntFrame* frame)
{
    frame->mode = 0x30000;
}

static void zEntPlayer_BoulderVehicleUpdate(xEnt* ent, xScene* sc, F32 dt)
{
    xEntCollis collis;
    xMat3x3 rotM;
    xVec3 tempRight;
    xVec3 tempAt;
    xVec3 axis;
    xVec3 heading;
    xVec2 inputDefl;
    F32 ang;
    F32 mag;
    S32 i;

    gSpongeBall = TRUE;
    xEntBoulder* shouldDamagePlayer = NULL;
    zEntPlayer_PredictionUpdate(ent, dt);

    xVec3Copy(&ent->frame->oldmat.pos, &ent->frame->mat.pos);

    U8 chkBackup = ent->collis->chk;
    U8 penBackup = ent->collis->pen;
    ent->collis->chk = 0x0;
    ent->collis->pen = 0x0;

    if (boulderVehicle != NULL && globals.player.ControlOff == FALSE)
    {
        bvTimeToIdle -= dt;

        getPadDefl(&globals.pad0->analog1, &inputDefl);

        if (globals.pad0->on & 0x20)
        {
            inputDefl.x = 1.0f;
        }

        if (globals.pad0->on & 0x80)
        {
            inputDefl.x = -1.0f;
        }

        if (globals.pad0->on & 0x40)
        {
            inputDefl.y = 1.0f;
        }

        if (globals.pad0->on & 0x10)
        {
            inputDefl.y = -1.0f;
        }

        xVec3Copy(&tempRight, &globals.camera.mat.right);
        tempRight.y = 0.0f;

        F32 rightLen = xVec3Length2(&tempRight);
        if (rightLen > 0.0001f)
        {
            xVec3SMulBy(&tempRight, 1.0f / xsqrt(rightLen));
        }
        else
        {
            xVec3Copy(&tempRight, (xVec3*)&globals.player.ent.model->Mat->right);
        }

        xVec3Copy(&tempAt, &globals.camera.mat.at);
        tempAt.y = 0.0f;

        F32 atLen = xVec3Length2(&tempAt);
        if (atLen > 0.0001f)
        {
            xVec3SMulBy(&tempAt, 1.0f / xsqrt(atLen));
        }
        else
        {
            xVec3Copy(&tempAt, &globals.camera.mat.up);
        }

        xVec3AddScaled(&boulderVehicle->vel, &tempRight, 60.0f * (-0.15f * inputDefl.x) * dt);
        xVec3AddScaled(&boulderVehicle->vel, &tempAt, 60.0f * (-0.15f * inputDefl.y) * dt);

        if (globals.player.Visible)
        {
            boulderVehicle->baseFlags |= 0x1;
        }
        else
        {
            boulderVehicle->baseFlags &= ~0x1;
        }

        boulderVehicle->collis = &collis;
        xEntBoulder_Update(boulderVehicle, sc, dt);

        for (i = collis.dyn_sidx; i < collis.dyn_eidx; i++)
        {
            if ((collis.colls[i].flags & 0x1) && collis.colls[i].optr &&
                ((xEntBoulder*)collis.colls[i].optr)->baseType == eBaseTypeBoulder &&
                (((xEntBoulder*)collis.colls[i].optr)->basset->flags & 0x2))
            {
                shouldDamagePlayer = (xEntBoulder*)collis.colls[i].optr;
            }
        }

        boulderVehicle->collis = NULL;

        if (!shouldDamagePlayer)
        {
            if (xVec3Length2(&boulderVehicle->vel) > 4.0f)
            {
                bvTimeToIdle = 2.0f;
            }

            F32 deflMag = inputDefl.x * inputDefl.x + inputDefl.y * inputDefl.y;

            if (boulderVehicle->angVel > 4.0f && deflMag > 0.25f)
            {
                xVec3Cross(&axis, (xVec3*)&boulderVehicle->model->Mat->right,
                           &boulderVehicle->rotVec);
                xMat3x3Rot(&rotM, &axis, 0.01f * xasin(xVec3Normalize(&axis, &axis)));
                xMat3x3Mul((xMat3x3*)boulderVehicle->model->Mat,
                           (xMat3x3*)boulderVehicle->model->Mat, &rotM);
            }

            if (deflMag > 0.1f)
            {
                xVec3Init(&heading, -globals.camera.mat.right.z, 0.0f, globals.camera.mat.right.x);
                xVec3SMulBy(&heading, -1.0f * inputDefl.y);
                xVec3AddScaled(&heading, &globals.camera.mat.right, -1.0f * inputDefl.x);
                heading.y = 0.0f;

                if (xVec3Normalize(&heading, &heading) > 0.001f)
                {
                    xVec3Copy((xVec3*)&ent->model->Mat->at, &heading);
                    xVec3Init((xVec3*)&ent->model->Mat->up, 0.0f, 1.0f, 0.0f);
                    xVec3Init((xVec3*)&ent->model->Mat->right, ent->model->Mat->at.z, 0.0f,
                              -ent->model->Mat->at.x);
                }
            }

            xVec3Copy((xVec3*)&ent->model->Mat->pos, &boulderVehicle->bound.sph.center);
            ent->model->Mat->pos.y -= ent->bound.sph.r;

            xCameraSetTargetMatrix(&globals.camera, xEntGetFrame(&globals.player.ent));
            xCameraSetTargetOMatrix(&globals.camera, xEntGetFrame(&globals.player.ent));

            zEntPlayerCollTrigger(ent, sc);

            DampenControls(&ang, &mag, -globals.pad0->analog1.x, -globals.pad0->analog1.y);

            if (mag)
            {
                if (mag > globals.player.s->MoveSpeed[3])
                {
                    if (mag < globals.player.s->MoveSpeed[4])
                    {
                        globals.player.Speed = 1;
                    }
                    else
                    {
                        globals.player.Speed = 2;
                    }
                }
            }
            else
            {
                globals.player.Speed = 0;
            }

            xVec3Copy(&ent->frame->mat.pos, (xVec3*)&ent->model->Mat->pos);
            ent->move = zEntPlayer_BoulderVehicleMove;

            if (ent->bupdate)
            {
                ent->bupdate(ent, (xVec3*)&ent->model->Mat->pos);
            }

            zCameraSetPlayerVel(&boulderVehicle->vel);
            sCameraLastMat = *(xMat4x3*)ent->model->Mat;
        }
        else
        {
            goto boulder_roll_continue;
        }
    }

    if (boulderVehicle == NULL || globals.player.ControlOff != FALSE)
    {
        ent->collis->chk = chkBackup;
        ent->collis->pen = penBackup;

        BoulderRollDoneCB();
        zEntPlayer_Update(ent, sc, dt);

        if (shouldDamagePlayer)
        {
            zEntPlayer_DamageNPCKnockBack(shouldDamagePlayer, 1,
                                          &shouldDamagePlayer->bound.sph.center);
        }
    }
    else
    {
    boulder_roll_continue:
        ent->collis->chk = chkBackup;
        ent->collis->pen = penBackup;

        if (shouldDamagePlayer)
        {
            boulderRollShouldEnd = TRUE;
        }

        if (BoulderRollDoneCheck())
        {
            BoulderRollDoneCB();
            zEntPlayer_Update(ent, sc, dt);

            if (shouldDamagePlayer)
            {
                zEntPlayer_DamageNPCKnockBack(shouldDamagePlayer, 1,
                                              &shouldDamagePlayer->bound.sph.center);
            }
        }
        else
        {
            zEntPickup_CheckAllPickupsAgainstPlayer(sc, dt);
        }
    }
}

static void zEntPlayer_PredictionUpdate(xEnt* ent, F32 dt)
{
    zPlayerGlobals* g = &globals.player;
    xVec3 lastDir;
    F32 lastVel;

    xVec3Copy(&lastDir, &g->PredictCurrDir);
    lastVel = g->PredictCurrVel;

    xVec3Sub(&g->PredictCurrDir, &ent->frame->mat.pos, &ent->frame->oldmat.pos);
    xVec3SMulBy(&g->PredictCurrDir, 1.0f / last_update_dt);
    g->PredictCurrDir.y = 0.0f;

    g->PredictCurrVel = xVec3Length(&g->PredictCurrDir);

    if (g->PredictCurrVel > 0.05f)
    {
        xVec3SMulBy(&g->PredictCurrDir, 1.0f / g->PredictCurrVel);
    }
    else
    {
        xVec3Copy(&g->PredictCurrDir, (xVec3*)&ent->model->Mat->at);
    }

    g->PredictCurrVel = 0.9f * lastVel + (1.0f - 0.9f) * g->PredictCurrVel;

    if (g->PredictCurrVel > 0.05f)
    {
        xVec3 cross;
        F32 newAngV;

        xVec3Cross(&cross, &lastDir, &g->PredictCurrDir);
        newAngV = xasin(cross.y);

        if (xVec3Dot(&lastDir, &g->PredictCurrDir) < 0.0f)
        {
            if (newAngV > 0.0f)
            {
                newAngV = PI - newAngV;
            }
            else
            {
                newAngV = -PI - newAngV;
            }
        }

        newAngV /= dt;
        g->PredictAngV = 0.9f * g->PredictAngV + (1.0f - 0.9f) * newAngV;

        if (g->PredictAngV > 0.05f || g->PredictAngV < -0.05f)
        {
            F32 r = g->PredictCurrVel / g->PredictAngV;

            xVec3Init(&g->PredictRotate, -g->PredictCurrDir.z, 0.0f, g->PredictCurrDir.x);
            xVec3SMulBy(&g->PredictRotate, r);
            xVec3Sub(&g->PredictTranslate, (xVec3*)&ent->model->Mat->pos, &g->PredictRotate);
        }
    }
    else
    {
        g->PredictAngV = 0.0f;
    }
}

void zEntPlayer_PredictPos(xVec3* pos, F32 timeIntoFuture, F32 leadFactor, S32 useTurn)
{
    zPlayerGlobals* g = &globals.player;
    F32 useVel;
    F32 useAngV;
    xMat3x3 rotMat;

    useVel = g->PredictCurrVel * leadFactor;
    useAngV = g->PredictAngV;

    if ((useTurn != 0) && (useAngV > 0.05f || useAngV < -0.05f))
    {
        xMat3x3RotY(&rotMat, leadFactor * (timeIntoFuture * useAngV));
        xMat3x3RMulVec(pos, &rotMat, &g->PredictRotate);
        xVec3AddTo(pos, &g->PredictTranslate);
    }
    else
    {
        xVec3Copy(pos, (xVec3*)&g->ent.model->Mat->pos);
        if (useVel > 0.05f)
        {
            xVec3AddScaled(pos, &g->PredictCurrDir, (timeIntoFuture * useVel));
        }
    }
}

static S32 zEntPlayerKnockToSafety(xEnt* ent)
{
    F32 diffX;
    F32 diffY;
    F32 diffZ;
    F32 popheight;
    F32 ttot;
    F32 velXZ;

    if (globals.player.Health == 0)
    {
        return 0;
    }
    else
    {
        diffZ = floor_safe_vec.z - ent->model->Mat->pos.z;
        diffX = floor_safe_vec.x - ent->model->Mat->pos.x;
        diffY = floor_safe_vec.y - ent->model->Mat->pos.y;
        velXZ = xsqrt(diffX * diffX + diffZ * diffZ);
        if (diffY < -3.0f || diffY > 5.0f || velXZ > 9.0f)
        {
            return 0;
        }
        else
        {
            popheight = ent->model->Mat->pos.y;
            F32 highest = floor_safe_vec.y;
            if (popheight > highest)
            {
                highest = popheight;
            }
            ttot = highest + 2.65f;
            diffY = xsqrt(2.0f * (ttot - popheight) / globals.player.g.Gravity);
            popheight = xsqrt(2.0f * (ttot - floor_safe_vec.y) / globals.player.g.Gravity);
            ttot = diffY + popheight;
            ent->frame->vel.y = diffY * globals.player.g.Gravity;
            popheight = velXZ / ttot;
            if (velXZ < 1e-5f)
            {
                ent->frame->vel.x = 0.0f;
                ent->frame->vel.z = 0.0f;
            }
            else
            {
                velXZ = popheight / velXZ;
                ent->frame->vel.x = diffX * velXZ;
                ent->frame->vel.z = diffZ * velXZ;
            }
            globals.player.KnockBackTimer = ttot;
            globals.player.KnockIntoAirTimer = 0.0f;
            return 1;
        }
    }

    return 0;
}

static xEnt* zEntPlayer_FindGrabEnt(xEnt* ent, zScene* zsc, S32* failed)
{
    U32 i;
    F32 dx, dy, dz;

    for (i = 0; i < zsc->num_ents; i++)
    {
        xEnt* e = (xEnt*)zsc->ents[i];

        if (!(e->baseFlags & 0x20))
        {
            continue;
        }

        if (!(e->flags & 0x1))
        {
            continue;
        }

        if (!e->model)
        {
            continue;
        }

        if (!(e->chkby & 0x10))
        {
            continue;
        }

        if (e->baseType != 0x2f)
        {
            dx = e->model->Mat->pos.x - ent->model->Mat->pos.x;
            dy = e->model->Mat->pos.y - ent->model->Mat->pos.y;
            dz = e->model->Mat->pos.z - ent->model->Mat->pos.z;
        }
        else
        {
            xEntBoulder* boul = (xEntBoulder*)e;

            dx = boul->bound.sph.center.x - ent->model->Mat->pos.x;
            dy = (boul->bound.sph.center.y - boul->bound.sph.r) - ent->model->Mat->pos.y;
            dz = boul->bound.sph.center.z - ent->model->Mat->pos.z;
        }

        F32 dist2 = dx * dx + dz * dz;

        if (dist2 >= globals.player.carry.maxDist * globals.player.carry.maxDist ||
            dy >= globals.player.carry.maxHeight || dy <= globals.player.carry.minHeight ||
            dist2 <= globals.player.carry.minDist * globals.player.carry.minDist)
        {
            continue;
        }

        F32 dist = xsqrt(dist2);

        if ((dx * ent->model->Mat->at.x + dz * ent->model->Mat->at.z) / dist <
            globals.player.carry.maxCosAngle)
        {
            continue;
        }

        if (e->model->Scale.x)
        {
            continue;
        }

        if (zThrown_IsStacked(e))
        {
            continue;
        }

        if (e->baseType == 0x2b && !((zNPCCommon*)e)->SetCarryState(zNPCCARRY_ATTEMPTPICKUP))
        {
            continue;
        }

        if (!((e->moreFlags & 0x8) || e->baseType == 0x2b) ||
            !(e->baseType == 0xb || e->baseType == 0x2b || e->baseType == 0x2f ||
              e->baseType == 0x1b))
        {
            if (failed)
            {
                *failed = 1;
            }
        }
        else
        {
            if (failed)
            {
                *failed = 0;
            }

            return e;
        }
    }

    return NULL;
}

static const U8 SBBBashBones[8] = { 22, 30, 38, 42 };
static const U8 SBBBounceBones[8] = { 22, 30, 38, 42 };

void zEntPlayer_Update(xEnt* ent, xScene* sc, F32 dt)
{
    if ((gCurrentPlayer == eCurrentPlayerPatrick && !globals.player.model_patrick) ||
        (gCurrentPlayer == eCurrentPlayerSandy && !globals.player.model_sandy))
    {
        gCurrentPlayer = eCurrentPlayerSpongeBob;
    }

    if (dt < 1e-5f)
    {
        return;
    }

    last_update_dt = update_dt;
    update_dt = dt;

    zEntPlayer_SNDPlayDelayed(dt);

    for (S32 j = 0; j < ePlayerStreamSnd_Total; j++)
    {
        if (sPlayerStreamSndTimer[j].timer > 0.0f)
        {
            sPlayerStreamSndTimer[j].timer -= dt;
            if (sPlayerStreamSndTimer[j].timer < 0.0f)
            {
                sPlayerStreamSndTimer[j].timer = 0.0f;
            }
        }
    }

    gSpongeBall = 0;

    if (!sLassoInfo->swingTarget)
    {
        zEntPlayer_UpdateVelocityBlur();
    }

    zEntPlayer_PredictionUpdate(ent, dt);
    zEntPlayerEGenUpdate(ent, sc, dt);

    if (gReticleTarget)
    {
        sReticleRot += 8.0f * dt;
        if (sReticleRot > 2.0f * PI)
        {
            sReticleRot -= 2.0f * PI;
        }

        sReticleAlpha += 3.0f * dt;
        if (sReticleAlpha > 1.0f)
        {
            sReticleAlpha = 1.0f;
        }
    }

    if (globals.player.ControlOff || !globals.player.Health || in_goo)
    {
        xEntBoulder* boul = globals.player.bubblebowl;
        if (boul && boul->update)
        {
            zEntEvent(&globals.player.ent, boul, eEventKill);
        }
    }

    if (bungee_state::update(sc, dt))
    {
        return;
    }

    if (cruise_bubble::update(sc, dt))
    {
        return;
    }

    if (oob_state::update(*sc, dt))
    {
        return;
    }

    zEntPlayer_SpringboardFX(ent, dt);
    speak_update(dt);

    if (gCurrentPlayer == eCurrentPlayerSpongeBob)
    {
        U32 total = globals.player.ent.model->Anim->Single->State->UserFlags & 0x1e;
        if (total == 2 || total == 4)
        {
            if (!sPlayerSndID[gCurrentPlayer][ePlayerSnd_Sneak] && sPlayerSndSneakDelay == 0.0f)
            {
                zEntPlayer_SNDPlay(ePlayerSnd_Sneak, 0.0f);
            }
        }
        else
        {
            zEntPlayer_SNDStop(ePlayerSnd_Sneak);
            sPlayerSndSneakDelay = 0.3f;
        }
    }
    else
    {
        xSndStop(sPlayerSndID[0][ePlayerSnd_Sneak]);
        sPlayerSndID[0][ePlayerSnd_Sneak] = 0;
        sPlayerSndSneakDelay = 0.3f;
    }

    if (strcmp(ent->model->Anim->Single->State->Name, "BbowlWindup01") == 0)
    {
        ent->model->Anim->Single->CurrentSpeed =
            1.0f + sBubbleBowlTimer / globals.player.g.BubbleBowlTimeDelay;
        if (ent->model->Anim->Single->CurrentSpeed > 2.0f)
        {
            ent->model->Anim->Single->CurrentSpeed = 2.0f;
        }

        sBubbleBowlTimer += dt;

        if (!sShouldBubbleBowl)
        {
            if (globals.player.ControlOff || !(globals.pad0->on & XPAD_BUTTON_O) ||
                sBubbleBowlTimer > globals.player.g.BubbleBowlTimeDelay)
            {
                if (sBubbleBowlTimer > globals.player.g.BubbleBowlTimeDelay)
                {
                    sBubbleBowlTimer = globals.player.g.BubbleBowlTimeDelay;
                }

                sShouldBubbleBowl = 1;
                sBubbleBowlMultiplier =
                    globals.player.g.BubbleBowlPercentIncrease *
                        (sBubbleBowlTimer / globals.player.g.BubbleBowlTimeDelay) +
                    1.0f;
            }
        }
    }

    zEntPickup_CheckAllPickupsAgainstPlayer(sc, dt);

    if (globals.player.g.CheatPlayerSwitch && !globals.player.carry.grabbed &&
        globals.player.SundaeTimer < 0.0f)
    {
        if (!globals.player.ControlOff && (globals.pad0->pressed & XPAD_BUTTON_UP))
        {
            gCurrentPlayer = eCurrentPlayerSpongeBob;
        }
        else if (!globals.player.ControlOff && (globals.pad0->pressed & XPAD_BUTTON_LEFT) &&
                 globals.player.model_patrick)
        {
            gCurrentPlayer = eCurrentPlayerPatrick;
        }
        else if (!globals.player.ControlOff && (globals.pad0->pressed & XPAD_BUTTON_RIGHT) &&
                 globals.player.model_sandy)
        {
            gCurrentPlayer = eCurrentPlayerSandy;
        }
    }

    if (gCurrentPlayer != lastgCurrentPlayer)
    {
        switch (gCurrentPlayer)
        {
        case eCurrentPlayerSpongeBob:
            *globals.player.model_spongebob->Mat = *ent->model->Mat;
            ent->model = globals.player.model_spongebob;
            globals.player.s = &globals.player.sb;
            gReticleTarget = NULL;
            globals.player.IsCoptering = 0;
            break;
        case eCurrentPlayerPatrick:
            if (globals.player.model_patrick)
            {
                *globals.player.model_patrick->Mat = *ent->model->Mat;
                ent->model = globals.player.model_patrick;
                globals.player.s = &globals.player.patrick;
                gReticleTarget = NULL;
                globals.player.IsBubbleBowling = 0;
                globals.player.IsCoptering = 0;
            }
            break;
        case eCurrentPlayerSandy:
            if (globals.player.model_sandy)
            {
                *globals.player.model_sandy->Mat = *ent->model->Mat;
                ent->model = globals.player.model_sandy;
                globals.player.s = &globals.player.sandy;
                gReticleTarget = NULL;
                globals.player.IsBubbleBowling = 0;
            }
            break;
        }
    }

    lastgCurrentPlayer = gCurrentPlayer;

    globals.player.Inv_PatsSock_CurrentLevel = globals.player.Inv_PatsSock[zSceneGetLevelIndex()];
    globals.player.Inv_LevelPickups_CurrentLevel =
        globals.player.Inv_LevelPickups[zSceneGetLevelIndex()];

    xCameraSetTargetMatrix(&globals.camera, xEntGetFrame(&globals.player.ent));
    xCameraSetTargetOMatrix(&globals.camera, &sCameraLastMat);
    zCameraSetPlayerVel(&globals.player.ent.frame->vel);

    sGrabFound = NULL;
    sGrabFailed = 0;

    if (ent->model == globals.player.model_patrick)
    {
        if (globals.player.carry.grabbed)
        {
            xEnt* oldTarget = gReticleTarget;
            sTimeToRetarget -= dt;
            if (sTimeToRetarget < 0.0f)
            {
                sTimeToRetarget = 0.25f;
                gReticleTarget = GetPatrickTarget(ent);
                sTypeOfTarget = 3;
                if (gReticleTarget != oldTarget)
                {
                    sReticleAlpha = 0.0f;
                }
            }
        }
        else
        {
            xEnt* oldTarget = gReticleTarget;
            sTimeToRetarget -= dt;
            if (sTimeToRetarget < 0.0f)
            {
                sTimeToRetarget = 0.25f;
                gReticleTarget = zEntPlayer_FindGrabEnt(ent, (zScene*)sc, NULL);
                sTypeOfTarget = 2;
                if (gReticleTarget != oldTarget)
                {
                    sReticleAlpha = 0.0f;
                }
            }
        }

        if (!globals.player.carry.grabbed && !globals.player.ControlOff &&
            (globals.pad0->pressed & XPAD_BUTTON_O))
        {
            if (gReticleTarget && sTypeOfTarget == 2)
            {
                sGrabFound = gReticleTarget;
            }
            else
            {
                sGrabFound = zEntPlayer_FindGrabEnt(ent, (zScene*)sc, &sGrabFailed);
            }
        }

        xVec3 pos = *(xVec3*)&ent->model->Mat->pos;

        xAnimSingle* single = globals.player.ent.model->Anim->Single;
        xAnimState* astate = single->State;

        if (!gPTankDisable && StunBubbleTrail(single))
        {
            zFX_SpawnBubbleTrail((xVec3*)&globals.player.ent.model->Mat->pos, 1);
        }

        if (sRingDelay > 0.0f)
        {
            F32 s;
            if (dt > sRingDelay)
            {
                s = sRingDelay;
            }
            else
            {
                s = dt;
            }
            sRingDelay -= s;
        }

        if (strcmp(astate->Name, "StunLand") == 0 &&
            globals.player.ent.model->Anim->Single->Time < 0.25f)
        {
            if (sRingDelay == 0.0f)
            {
                if (ent->collis->colls[0].dist > 2.0f)
                {
                    return;
                }

                pos.x += ent->collis->colls[0].tohit.x * ent->collis->colls[0].dist;
                pos.y += ent->collis->colls[0].tohit.y * ent->collis->colls[0].dist;
                pos.z += ent->collis->colls[0].tohit.z * ent->collis->colls[0].dist;
                pos.y += 0.2f;

                zFXPatrickStun(&pos);
                sRingDelay = 2.0f;
            }

            if (gPTankDisable)
            {
                xParEmitterCustomSettings info;
                info.custom_flags = 0x30e;
                info.pos = pos;

                for (S32 j = 0; j < 100U; j++)
                {
                    F32 ang = 2.0f * PI * j / 100.0f;
                    info.vel.x = 4.0f * icos(ang);
                    info.vel.y = 1.25f;
                    info.vel.z = 4.0f * isin(ang);
                    info.life.set(1.0f, 1.0f, 1.0f, 0);
                    info.size_birth.set(0.05f, 0.05f, 1.0f, 0);
                    info.size_death.set(0.25f, 0.25f, 1.0f, 0);
                    xParEmitterEmitCustom(gEmitBFX, 1.0f / 60.0f, &info);
                }
            }
            else
            {
                zFX_SpawnBubbleSlam(&pos, 0x18, 0.15f, 12.0f, 2.0f);
            }
        }
    }

    if (globals.player.SundaeTimer >= 0.0f)
    {
        globals.player.SundaeTimer -= dt;
        if (globals.player.SundaeTimer <= 0.0f)
        {
            globals.player.SpeedMult = 1.0f;
            globals.player.SundaeTimer = -1.0f;
        }
        else
        {
            sLastInvulnEmit += dt;
            if (sLastInvulnEmit > 0.02f)
            {
                sLastInvulnEmit = 0.0f;

                xParEmitterCustomSettings info;
                info.custom_flags = 0x352;
                xVec3Copy(&info.pos, xBoundCenter(&ent->bound));

                xVec3 vel;
                xVec3 tmp;
                xVec3SMul(&vel, &globals.player.PredictCurrDir, globals.player.PredictCurrVel);
                xVec3SMul(&tmp, (xVec3*)&globals.player.ent.model->Mat->right, 1.0f);
                xVec3Add(&vel, &vel, &tmp);
                xVec3SMul(&tmp, (xVec3*)&globals.player.ent.model->Mat->at, 1.0f);
                xVec3Add(&info.vel, &vel, &tmp);

                info.vel_angle_variation = 0.075f;
                info.rate.set(150.0f, 150.0f, 1.0f, 0);
                info.life.set(1.5f, 2.0f, 1.0f, 2);

                xParEmitterEmitCustom(sEmitStankBreath, 1.0f / 60.0f, &info);
            }
        }
    }

    if ((ent->model->Anim->Single->State->UserFlags & 0x1000) || tslide_ground)
    {
        xVec3 normvel;
        normvel = globals.player.SlideTrackVel;
        normvel.y = -1e-07f;

        xBound slideB;
        slideB.type = XBOUND_TYPE_SPHERE;
        slideB.sph.center.x =
            ent->bound.sph.center.x + 1.25f * dt * globals.player.SlideTrackVel.x;
        slideB.sph.center.y = ent->bound.sph.center.y + 1.25f * dt * ent->frame->vel.y;
        slideB.sph.center.z =
            ent->bound.sph.center.z + 1.25f * dt * globals.player.SlideTrackVel.z;
        slideB.sph.r = ent->bound.sph.r + 0.3f;

        xQuickCullForBound(&slideB.qcd, &slideB);
        MeleeAttackBoundCollide(ent, (zScene*)sc, &slideB);
    }

    const U8* bonelist = NULL;

    if (ent->model == globals.player.model_spongebob)
    {
        xAnimSingle* single = globals.player.ent.model->Anim->Single;
        if (BubbleBashContrails(single))
        {
            bonelist = SBBBashBones;
        }
        else if (BubbleBounceContrails(single))
        {
            bonelist = SBBBounceBones;
        }
    }

    if (bonelist)
    {
        U32 num = 0;
        const U8* bp = bonelist;
        while (*bp)
        {
            num++;
            bp++;
        }

        xVec3* posbuf = (xVec3*)xMemPushTemp(num * 2 * sizeof(xVec3));
        xVec3* velbuf = posbuf + num;
        if (posbuf)
        {
            xVec3* pp = posbuf;
            xVec3* vp = velbuf;
            U32 j = 0;
            for (; j < num; j++, pp++, vp++, bonelist++)
            {
                xMat4x3 mat;
                xMat4x3Mul(&mat, (xMat4x3*)(ent->model->Mat + *bonelist),
                           (xMat4x3*)ent->model->Mat);
                *pp = mat.pos;
                pp->x += 0.1f * (xurand() - 0.5f);
                pp->y += 0.1f * (xurand() - 0.5f);
                pp->z += 0.1f * (xurand() - 0.5f);
                vp->x = 0.2f * (xurand() - 0.5f);
                vp->y = 0.2f * (xurand() - 0.5f);
                vp->z = 0.2f * (xurand() - 0.5f);
            }

            zParPTankSpawnBubbles(posbuf, velbuf, num, 1.0f);
            xMemPopTemp(posbuf);
        }
    }

    if ((globals.player.ControlOff & ~(CONTROL_OWNER_FROZEN | CONTROL_OWNER_SPRINGBOARD)) ||
        globals.cmgr)
    {
        globals.player.ControlOffTimer = 1.0f;
    }
    else if (globals.sceneCur->sceneID == 'PG12')
    {
        zEntPlayer_setBoulderMode(1);
    }

    globals.player.KnockBackTimer -= dt;
    if (globals.player.KnockBackTimer < 0.0f)
    {
        globals.player.KnockBackTimer = 0.0f;
    }

    globals.player.KnockIntoAirTimer -= dt;
    if (globals.player.KnockIntoAirTimer < 0.0f)
    {
        globals.player.KnockIntoAirTimer = 0.0f;
    }

    globals.player.Sneak = 0;

    xAnimSingle* single = globals.player.ent.model->Anim->Single;

    if (!globals.player.ControlOff && globals.pad0->on == 0 &&
        abs(globals.pad0->analog1.x) <= globals.player.g.AnalogMin &&
        abs(globals.pad0->analog1.y) <= globals.player.g.AnalogMin)
    {
        xAnimState* astate = globals.player.ent.model->Anim->Single->State;
        if (astate && (astate->UserFlags & 1))
        {
            globals.player.IdleMinorTimer += dt;
            globals.player.IdleMajorTimer += dt;
            if (!surfSticky)
            {
                globals.player.IdleSitTimer += dt;
            }

            if (globals.player.IdleMinorTimer == 0.0f)
            {
                globals.player.IdleMinorTimer = 0.0001f;
            }
        }
    }
    else
    {
        globals.player.IdleMinorTimer = 0.0f;
        globals.player.IdleMajorTimer = -3.0f;
        globals.player.IdleSitTimer = 0.0f;
    }

    if (globals.player.ScareTimer > 0.0f)
    {
        globals.player.ScareTimer -= dt;
        if (globals.player.ScareTimer < 0.0f)
        {
            globals.player.ScareTimer = 0.0f;
        }
    }

    if (globals.player.VictoryTimer > 0.0f)
    {
        globals.player.VictoryTimer -= dt;
        if (globals.player.VictoryTimer < 0.0f)
        {
            globals.player.VictoryTimer = 0.0f;
        }
    }

    if (globals.player.BadGuyNearTimer > 0.0f)
    {
        globals.player.BadGuyNearTimer -= dt;
        if (globals.player.BadGuyNearTimer < 0.0f)
        {
            globals.player.BadGuyNearTimer = 0.0f;
        }
    }

    if (globals.player.DamageTimer > 0.0f)
    {
        globals.player.DamageTimer -= dt;
        if (globals.player.DamageTimer < 0.0f)
        {
            globals.player.DamageTimer = 0.0f;
        }
    }

    if (globals.player.ControlOffTimer > 0.0f)
    {
        globals.player.ControlOffTimer -= dt;
        if (globals.player.ControlOffTimer < 0.0f)
        {
            globals.player.ControlOffTimer = 0.0f;
        }
    }

    if (globals.player.CowerTimer > 0.0f)
    {
        globals.player.CowerTimer -= dt;
        if (globals.player.CowerTimer < 0.0f)
        {
            globals.player.CowerTimer = 0.0f;
        }
    }

    if (globals.player.FallDeathTimer > 0.0f)
    {
        globals.player.FallDeathTimer -= dt;
        if (globals.player.FallDeathTimer < 0.0f)
        {
            globals.player.FallDeathTimer = 0.0f;
        }
    }

    if (globals.player.HelmetTimer > 0.0f)
    {
        globals.player.HelmetTimer -= dt;
        if (globals.player.HelmetTimer < 0.0f)
        {
            globals.player.HelmetTimer = 0.0f;
        }
    }

    if (sPlayerSndSneakDelay > 0.0f)
    {
        sPlayerSndSneakDelay -= dt;
        if (sPlayerSndSneakDelay < 0.0f)
        {
            sPlayerSndSneakDelay = 0.0f;
        }
    }

    idle_tmr += dt;
    if (globals.player.ControlOff & CONTROL_OWNER_TALK_BOX)
    {
        idle_tmr = 0.0f;
    }

    if (inact_tmr > 0.0f)
    {
        inact_tmr -= dt;
        if (inact_tmr < 0.0f)
        {
            inact_tmr = 0.0f;
        }
    }

    stun_power_tmr += dt;

    iColor_tag black = { 0, 0, 0, 255 };
    iColor_tag clear = { 0, 0, 0, 0 };

    if (!globals.player.Health)
    {
        if (globals.player.DamageTimer >= 0.3f && single->CurrentSpeed == 0.0f &&
            !xScrFxIsFading() && (single->State->UserFlags & 0x400))
        {
            xScrFxFade(&clear, &black, 0.3f, NULL, 1);
            globals.player.DamageTimer = 0.333333f;
        }

        if (globals.player.DamageTimer <= 0.0f)
        {
            zGameStateSwitch(2);
            HealthReset();
        }
    }

    sCameraLastMat = *(xMat4x3*)ent->model->Mat;
    sWallJumpResult = WallJumpResult_NoJump;

    if ((globals.player.ent.model->Anim->Single->State->UserFlags & 0x1e) == 0xa)
    {
        DoWallJumpCheck();
    }

    if (gCurrentPlayer == eCurrentPlayerSpongeBob &&
        ent->model->Anim->Single->CurrentSpeed == 0.0f &&
        !ent->model->Anim->Single->Blend->State &&
        strcmp(ent->model->Anim->Single->State->Name, "Bspin01") == 0)
    {
        ent->model->Anim->Single->CurrentSpeed = ent->model->Anim->Single->State->Speed;
    }

    xEntBeginUpdate(ent, sc, dt);

    xAnimState* astate = globals.player.ent.model->Anim->Single->State;

    if (bbash_end_tmr != 0.0f && strncmp(astate->Name, "Bbash", 5) != 0)
    {
        bbash_end_tmr -= dt;
        if (bbash_end_tmr < 0.0f)
        {
            bbash_end_tmr = 0.0f;
            zCameraMinTargetHeightClear();
        }
    }

    U8 hitting_floor = 0;
    U8 hitting_wall = 0;

    if (strcmp(astate->Name, "Bspin01") == 0 ? single->CurrentSpeed != 0.0f : 0)
    {
        if (single->Time >= globals.player.g.BSpinMinFrame)
        {
            hitting_wall = 1;
        }
    }

    if (hitting_wall && single->Time <= globals.player.g.BSpinMaxFrame)
    {
        hitting_floor = 1;
    }

    globals.player.IsBubbleSpinning = hitting_floor;

    if (!tslide_ground &&
        (strcmp(ent->model->Anim->Single->State->Name, "BbashAttack01") == 0 ||
         strcmp(ent->model->Anim->Single->State->Name, "BbashStart01") == 0 ||
         strcmp(ent->model->Anim->Single->State->Name, "BbashStrike01") == 0 ||
         (strcmp(ent->model->Anim->Single->State->Name, "BbashMiss01") == 0 &&
          ent->frame->vel.y > 0.0f)))
    {
        if (sPlayerCollAdjust <= 0.5f)
        {
            sPlayerCollAdjust += 3.0f * dt;
            if (sPlayerCollAdjust > 0.5f)
            {
                sPlayerCollAdjust = 0.5f;
            }
        }
        else
        {
            sPlayerCollAdjust -= 5.0f * dt;
            if (sPlayerCollAdjust < 0.5f)
            {
                sPlayerCollAdjust = 0.5f;
            }
        }
    }
    else if (!tslide_ground && ent->frame->vel.y > 0.5f)
    {
        if (sPlayerCollAdjust <= 0.25f)
        {
            sPlayerCollAdjust += 3.0f * dt;
            if (sPlayerCollAdjust > 0.25f)
            {
                sPlayerCollAdjust = 0.25f;
            }
        }
        else
        {
            sPlayerCollAdjust -= 5.0f * dt;
            if (sPlayerCollAdjust < 0.25f)
            {
                sPlayerCollAdjust = 0.25f;
            }
        }
    }
    else
    {
        sPlayerCollAdjust -= 5.0f * dt;
        if (sPlayerCollAdjust < 0.0f)
        {
            sPlayerCollAdjust = 0.0f;
        }
    }

    ent->frame->oldmat.pos.y += sPlayerCollAdjust;
    ent->frame->mat.pos.y += sPlayerCollAdjust;

    if (globals.player.ShockRadius != 0.0f)
    {
        globals.player.ShockRadius += 6.0f * dt;
        if (globals.player.ShockRadius > 4.0f)
        {
            globals.player.ShockRadius = 4.0f;
        }

        for (U32 i = 0; i < sc->num_npcs; i++)
        {
        }

        if (4.0f == globals.player.ShockRadius)
        {
            globals.player.ShockRadius = 0.0f;
        }
        else
        {
            globals.player.ShockRadiusOld = globals.player.ShockRadius;
        }
    }

    zGooCollsBegin();

    if (globals.player.WallJumpState == k_WALLJUMP_LAUNCH ||
        globals.player.WallJumpState == k_WALLJUMP_FLIGHT ||
        strncmp(astate->Name, "Bbash", 5) == 0)
    {
        sHackStuckTimer = 0.0f;
    }
    else
    {
        F32 mvelx = xVec3Length2(&ent->frame->vel);
        F32 mvelz = xVec3Length2(&tslide_lastrealvel);

        if (ent->collis->colls[0].flags & 1)
        {
            if (sHackStuckTimer != 0.0f)
            {
                ent->frame->vel = sHackStuckVel;
                ent->frame->oldvel = sHackStuckVel;
            }

            sHackStuckTimer = 0.0f;
        }
        else if (mvelx >= 36.0f && mvelz <= 0.25f && !(ent->collis->colls[0].flags & 1) &&
                 ((ent->collis->colls[2].flags & 1) || (ent->collis->colls[3].flags & 1) ||
                  (ent->collis->colls[4].flags & 1) || (ent->collis->colls[5].flags & 1)))
        {
            if (sHackStuckTimer == 0.0f)
            {
                sHackStuckVel = ent->frame->vel;
            }

            sHackStuckTimer = 0.15f;
        }
        else if (sHackStuckTimer != 0.0f)
        {
            sHackStuckTimer -= dt;
            if (sHackStuckTimer < 0.0f)
            {
                sHackStuckTimer = 0.0f;
                ent->frame->vel = sHackStuckVel;
                ent->frame->oldvel = sHackStuckVel;
            }
        }

        if (sHackStuckTimer != 0.0f)
        {
            if (!sHackStuckSetDir)
            {
                xRay3 testRay;
                xCollis testColl;

                testRay.origin = ent->frame->oldmat.pos;
                testRay.origin.y += 0.5f;
                testRay.dir.x = 0.0f;
                testRay.dir.y = -1.0f;
                testRay.dir.z = 0.0f;
                testRay.min_t = 0.0f;
                testRay.max_t = 1.5f;
                testRay.flags = 0xc00;
                testColl.flags = 0x200;

                iRayHitsEnv(&testRay, globals.sceneCur->env, &testColl);

                if ((testColl.flags & 1) && xabs(testColl.norm.y) < 0.99985f)
                {
                    sHackStuckDir.x = testColl.norm.x * testColl.norm.y;
                    sHackStuckDir.y =
                        -testColl.norm.x * testColl.norm.x - testColl.norm.z * testColl.norm.z;
                    sHackStuckDir.z = testColl.norm.z * testColl.norm.y;
                    xVec3Normalize(&sHackStuckDir, &sHackStuckDir);
                    sHackStuckSetDir = 1;
                }
            }

            if (sHackStuckSetDir)
            {
                F32 s = xsqrt(mvelx);
                if (s > 10.0f)
                {
                    s = 10.0f;
                }

                ent->frame->vel.x = s * sHackStuckDir.x;
                ent->frame->vel.y = s * sHackStuckDir.y;
                ent->frame->vel.z = s * sHackStuckDir.z;
                ent->frame->oldvel = ent->frame->vel;
            }
            else
            {
                sHackStuckDir.x = 0.0f;
                sHackStuckDir.y = 0.0f;
                sHackStuckDir.z = 0.0f;
            }
        }
    }

    F32 old_yvel = ent->frame->vel.y;
    S32 num_updates = 1;

    sDriveVel.z = 0.0f;
    sDriveVel.y = 0.0f;
    sDriveVel.x = 0.0f;

    if (!globals.player.HangEnt && !sLassoInfo->swingTarget)
    {
        if (dt > 0.1f)
        {
            num_updates = 6;
        }
        else if (dt > 1.0f / 12.0f)
        {
            num_updates = 5;
        }
        else if (dt > 1.0f / 15.0f)
        {
            num_updates = 4;
        }
        else if (dt > 0.05f)
        {
            num_updates = 3;
        }
        else
        {
            num_updates = 2;
        }
    }

    dt = dt / num_updates;

    static xEntCollis old_collis;

    for (S32 updidx = 0; updidx < num_updates; updidx++)
    {
        xVec3 suboldpos = ent->frame->mat.pos;

        xEntApplyPhysics(ent, sc, dt);
        xEntMove(ent, sc, dt);

        req_motion = ent->frame->dpos;

        xVec3 predrive_pos = ent->frame->mat.pos;

        xEntDriveUpdate(&globals.player.drv, sc, dt, NULL);

        sDriveVel.x += ent->frame->mat.pos.x - predrive_pos.x;
        sDriveVel.y += ent->frame->mat.pos.y - predrive_pos.y;
        sDriveVel.z += ent->frame->mat.pos.z - predrive_pos.z;

        if (globals.sceneCur->baseCount[eBaseTypeGust] != 0)
        {
            xFFX* gust_fkt = ent->ffx;
            while (gust_fkt)
            {
                if (gust_fkt->doEffect == zGustUpdateEnt)
                {
                    if (globals.player.JumpState == 3 || surfSlickRatio != 0.0f)
                    {
                        xFFXTurnOn(gust_fkt);
                    }
                    else
                    {
                        xFFXTurnOff(gust_fkt);
                        if (gust_data.gust_on)
                        {
                            memset(&gust_data, 0, sizeof(zGustData));
                        }
                    }
                }

                gust_fkt = gust_fkt->next;
            }
        }
        else
        {
            gust_data.gust_on = 0;
        }

        xFFXApply(ent, sc, dt);

        xVec3 motion;
        xVec3Sub(&motion, &ent->frame->mat.pos, &suboldpos);

        if (!globals.player.cheat_mode &&
            (globals.player.JumpState == 0 || globals.player.JumpState == 1) &&
            globals.player.FallDeathTimer == 0.0f)
        {
            ent->frame->mat.pos.y -= 0.5f * dt;

            F32 ndotm = globals.player.floor_norm.x * motion.x +
                        globals.player.floor_norm.z * motion.z;

            if (ndotm > 0.0f &&
                (req_motion.x != 0.0f || req_motion.z != 0.0f || surfSlickRatio != 0.0f))
            {
                globals.player.slope = -1;
                ent->frame->mat.pos.y -= xsqrt(ndotm);
            }
            else if (ndotm < 0.0f)
            {
                globals.player.slope = 1;
            }
            else
            {
                globals.player.slope = 0;
            }
        }

        if (lastFloorEnt)
        {
            if (surfSlickRatio != 0.0f)
            {
                ent->frame->mat.pos.x += 13.0f * dt * globals.player.floor_norm.x;
                ent->frame->mat.pos.y -=
                    13.0f * dt *
                    xsqrt(globals.player.floor_norm.x * globals.player.floor_norm.x +
                          globals.player.floor_norm.z * globals.player.floor_norm.z);
                ent->frame->mat.pos.z += 13.0f * dt * globals.player.floor_norm.z;
            }
            else if (globals.player.SlipFadeTimer > 0.0f)
            {
                F32 sett = 30.0f * (globals.player.SlipFadeTimer / 0.8f * dt);
                ent->frame->mat.pos.x += sett * lastFloorNorm.x;
                ent->frame->mat.pos.z += sett * lastFloorNorm.z;
            }
        }

        xVec3Sub(&precollide_motion, &ent->frame->mat.pos, &ent->frame->oldmat.pos);

        zEntPlayerCollide(ent, sc, dt);

        U32 stuckforce =
            (ent->collis->colls[2].flags & 1) || (ent->collis->colls[3].flags & 1) ||
            (ent->collis->colls[4].flags & 1) || (ent->collis->colls[5].flags & 1);

        if ((ent->collis->colls[0].flags & 1) ||
            (globals.player.ControlOff & ~CONTROL_OWNER_SPRINGBOARD))
        {
            stuck_timer = 0.0f;
            not_stuck_timer = 0.0f;
        }
        else if (!stuckforce)
        {
            not_stuck_timer += dt;
            if (not_stuck_timer > 0.2f)
            {
                stuck_timer = 0.0f;
                not_stuck_timer = 0.0f;
            }
        }
        else
        {
            xVec3& player_loc = *(xVec3*)&globals.player.ent.model->Mat->pos;

            if (stuck_timer == 0.0f)
            {
                not_stuck_timer = 0.0f;
                stuck_start_loc = player_loc;
            }

            F32 dist2 = (player_loc - stuck_start_loc).length2();

            if (dist2 > 1.0f)
            {
                stuck_timer = 0.0f;
                not_stuck_timer = 0.0f;
            }
            else
            {
                stuck_timer += dt;
                if (stuck_timer > 1.5f && globals.player.Health && !in_goo)
                {
                    oob_state::force_start();
                }
            }

            xprintf("Stuck: %.2f %.2f %.2f\n", stuck_timer, not_stuck_timer, xsqrt(dist2));
        }

        memcpy(&old_collis, ent->collis, sizeof(xEntCollis));
        zEntPlayerCollide(ent, sc, dt);
        memcpy(ent->collis, &old_collis, sizeof(xEntCollis));

        zEntPlayer_CheckCritterContact(ent, dt);
        xEntBoulder_ApplyForces(ent->collis);
    }

    xVec3 driveDist = sDriveVel;

    dt = update_dt;

    if (update_dt > 0.0001f)
    {
        sDriveVel.x = sDriveVel.x / update_dt;
        sDriveVel.y = sDriveVel.y / update_dt;
        sDriveVel.z = sDriveVel.z / update_dt;
    }

    xVec3Sub(&update_motion, &ent->frame->mat.pos, &ent->frame->oldmat.pos);

    if (!globals.player.cheat_mode)
    {
        xCollis bbc;
        bbc.flags = 0;

        xBox bbox = *(xBox*)xEnvGetBBox(globals.sceneCur->env);

        if (globals.sceneCur->zen)
        {
            bbox.upper.y += globals.sceneCur->zen->easset->loldHeight;
        }

        xSphereHitsBox(&ent->bound.sph, &bbox, &bbc);

        if (!(bbc.flags & 0x11))
        {
            zEntEvent(ent, 0x11);
        }
    }

    if (sCatchCapsuleTimer != 0.0f)
    {
        sCatchCapsuleTimer -= dt;
        if (sCatchCapsuleTimer <= 0.0f)
        {
            sCatchCapsuleTimer = 0.0f;
        }
    }

    if (!globals.player.cheat_mode)
    {
        sHackStuckSetDir = 0;

        if (update_motion.length() > 1e-5f)
        {
            U32 redo_catchtunnel = 1;
            xVec3 vstart;
            xVec3 vend;
            xSweptSphere sws;
            U32 iter;
            xSweptSphere swsredo[3];
            xSweptSphere* swscurr;
            xVec3 totalTan;
            xVec3 deltaremain;

            while (1)
            {
                vstart = ent->frame->oldmat.pos;
                vstart.y += ent->bound.sph.r - ent->bound.sph.r * CATCH_CAPSULE_BIAS;
                vend = vstart + update_motion;

                xSweptSpherePrepare(&sws, &vstart, &vend,
                                    ent->bound.sph.r * CATCH_CAPSULE_RAD);

                if (!xSweptSphereToNonMoving(&sws, globals.sceneCur, ent, ent->collType))
                {
                    goto catchtunnel_done;
                }

                xSweptSphereGetResults(&sws);

                if (sCatchCapsuleTimer == 0.0f)
                {
                    sCatchCapsuleTimer = 0.3f;
                }

                if (!redo_catchtunnel)
                {
                    goto catchtunnel_reset;
                }

                vend = vstart + precollide_motion;
                swscurr = swsredo;

                for (iter = 0; iter < 3; iter++, swscurr++)
                {
                    xSweptSpherePrepare(swscurr, &vstart, &vend,
                                        ent->bound.sph.r * CATCH_CAPSULE_RAD);

                    if (!xSweptSphereToNonMoving(swscurr, globals.sceneCur, ent,
                                                 ent->collType))
                    {
                        if (iter >= 1)
                        {
                            vstart = vend;
                            break;
                        }

                        vend = vstart + update_motion;
                        *swscurr = sws;
                    }
                    else
                    {
                        xSweptSphereGetResults(swscurr);
                    }

                    if (iter == 0)
                    {
                        totalTan = swscurr->worldTangent;
                        sHackStuckDir = totalTan;
                        sHackStuckSetDir = 1;
                    }
                    else
                    {
                        xVec3Cross(&totalTan, &swscurr->worldNormal,
                                   &swsredo[iter - 1].worldNormal);

                        if (totalTan.length2() < 1e-07f)
                        {
                            goto catchtunnel_reset;
                        }

                        totalTan.normalize();

                        F32 catchdot = totalTan.x * swscurr->basis.xm.at.x +
                                       totalTan.y * swscurr->basis.xm.at.y +
                                       totalTan.z * swscurr->basis.xm.at.z;

                        if (catchdot < 0.0f)
                        {
                            totalTan.x = -totalTan.x;
                            totalTan.y = -totalTan.y;
                            totalTan.z = -totalTan.z;
                        }

                        if (iter == 1)
                        {
                            sHackStuckDir = totalTan;
                            sHackStuckSetDir = 1;
                        }
                    }

                    F32 distremain = swscurr->dist - swscurr->curdist;

                    deltaremain.x = distremain * swscurr->basis.xm.at.x;
                    deltaremain.y = distremain * swscurr->basis.xm.at.y;
                    deltaremain.z = distremain * swscurr->basis.xm.at.z;

                    F32 tandot = xVec3Dot(&deltaremain, &totalTan);

                    vstart = swscurr->worldPos;

                    vend.x = tandot * totalTan.x + vstart.x;
                    vend.y = tandot * totalTan.y + vstart.y;
                    vend.z = tandot * totalTan.z + vstart.z;
                }

                ent->frame->mat.pos.x = vstart.x;
                ent->frame->mat.pos.y =
                    vstart.y - (ent->bound.sph.r - ent->bound.sph.r * CATCH_CAPSULE_BIAS);
                ent->frame->mat.pos.z = vstart.z;

                zEntPlayerCollide(ent, sc, dt);

                xVec3Sub(&update_motion, &ent->frame->mat.pos, &ent->frame->oldmat.pos);

                redo_catchtunnel = 0;
            }

        catchtunnel_reset:
            ent->frame->mat.pos = ent->frame->oldmat.pos;
        }
    }

catchtunnel_done:

    F32 fg = 1.5f * dt;

    for (xCollis* wcoll = &ent->collis->colls[2]; wcoll < &ent->collis->colls[6]; wcoll++)
    {
        if (wcoll->flags & 1)
        {
            F32 hdotm = wcoll->hdng.x * req_motion.x + wcoll->hdng.z * req_motion.z;
            xSurface* wsurf = zSurfaceGetSurface(wcoll);

            if (hdotm > 0.0f && wcoll->hdng.y < 0.0f && zSurfaceGetStep(wsurf))
            {
                ent->frame->mat.pos.y += fg;
                ent->collis->colls[0] = *wcoll;
                wcoll->flags = 0;
            }
        }
    }

    zGooCollsEnd();

    if (globals.player.JumpState == 0 && !in_goo)
    {
        sGooKnockedTimer -= dt;
        if (sGooKnockedTimer < 0.0f)
        {
            sGooKnockedToSafety = 0;
            sGooKnockedTimer = 0.0f;
        }
    }

    lin_goo = in_goo;

    xEntCollis* fcoll = ent->collis;

    if (fcoll->colls[0].flags & 1)
    {
        xBase* b = (xBase*)fcoll->colls[0].optr;
        if (b)
        {
            if (b->baseType == 0x18)
            {
                zEntButton_Hold((_zEntButton*)b, 0x400);
            }

            F32 dummy;
            if (zGooIs((xEnt*)fcoll->colls[0].optr, dummy, 1))
            {
                if (!sGooKnockedToSafety && zEntPlayerKnockToSafety(ent))
                {
                    globals.player.DamageTimer = 0.0f;
                    zEntPlayer_Damage((xBase*)fcoll->colls[0].optr, 0);
                    sGooKnockedToSafety = 1;
                    sGooKnockedTimer = 0.25f;
                }
                else
                {
                    in_goo = 1;
                }
            }
            else
            {
                in_goo = 0;
            }

            globals.player.CanJump = 1;
            globals.player.Jump_CanDouble = 1;
        }
        else
        {
            in_goo = 0;
        }
    }

    if (in_goo)
    {
        if (fcoll->colls[0].flags & 1)
        {
            in_goo_tmr += dt;
            if ((U8)xrand() > 0x40)
            {
                zFX_SpawnBubbleTrail((xVec3*)&ent->model->Mat->pos, 1);
            }
        }
    }
    else
    {
        in_goo_tmr = 0.0f;
    }

    if (globals.player.drv.driver)
    {
        zCameraTranslate(&globals.camera, &driveDist);
    }

    if (zcam_bbounce && ent->frame->mat.pos.y > ent->frame->oldmat.pos.y)
    {
        zCameraTranslate(&globals.camera, 0.0f,
                         ent->frame->mat.pos.y - ent->frame->oldmat.pos.y, 0.0f);
    }

    PlayerLedgeUpdate(ent, sc, dt);

    if (strcmp(astate->Name, "BbashAttack01") == 0 || strcmp(astate->Name, "BbashStart01") == 0)
    {
        xCollis* ceil = &ent->collis->colls[1];
        xEnt* destructent = (xEnt*)ent->collis->colls[1].optr;
        U8 destroyed = (destructent == NULL);

        if (strcmp(astate->Name, "BbashStrike01") != 0 && (ceil->flags & 1) && destructent &&
            (destructent->moreFlags & 0x10))
        {
            if (destructent->baseType == eBaseTypePlatform &&
                destructent->subType == ZPLATFORM_SUBTYPE_PADDLE &&
                (((zPlatform*)destructent)->passet->paddle.paddleFlags & 0x100))
            {
                xVec3 bashRay = { 0.0f, 1.0f, 0.0f };
                zPlatform_PaddleCollide(ceil, &ent->bound.sph.center, &bashRay, 1);
            }

            zEntEvent(destructent, 0x165);
        }

        if ((ceil->flags & 1) && destructent)
        {
            if (destructent->baseType == eBaseTypeBoulder)
            {
                xEntBoulder* boul = (xEntBoulder*)destructent;
                if (boul->basset->flags & 0x100)
                {
                    zEntEvent(destructent, 0x3a);
                }

                if (boul->basset->flags & 0x100)
                {
                    zEntEvent(destructent, 0x3a);
                }

                xVec3 f;
                xVec3Init(&f, 0.0f, 25.0f, 0.0f);
                xEntBoulder_AddForce(boul, &f);
            }
            else if (destructent->baseType == eBaseTypeButton)
            {
                zEntButton_Press((_zEntButton*)destructent, 4);
            }
            else if (destructent->baseType == eBaseTypeNPC)
            {
                zNPCCommon* npc = (zNPCCommon*)destructent;
                npc->Damage(DMGTYP_BELOW, &globals.player.ent, &ceil->tohit);
                if (!npc->IsHealthy())
                {
                    destroyed = 1;
                }
            }
            else if (destructent->baseType == eBaseTypeDestructObj)
            {
                if (!(destructent->chkby & 0x10))
                {
                    destroyed = 1;
                }
            }
        }

        if (destructent)
        {
            if (destroyed)
            {
                ceil->flags &= ~1;
                ent->frame->vel.y = old_yvel;
            }
            else
            {
                bbash_hit = 1;
                ent->frame->vel.y = 0.0f;
            }
        }
    }

    if (strcmp(astate->Name, "BbounceAttack01") == 0 ||
        strcmp(astate->Name, "BbounceStart01") == 0 || strcmp(astate->Name, "StunJump") == 0 ||
        strcmp(astate->Name, "StunFall") == 0 || strcmp(astate->Name, "StunLand") == 0)
    {
        xCollis* floor = &ent->collis->colls[0];
        xEnt* destructent = (xEnt*)ent->collis->colls[0].optr;
        U8 destroyed = (destructent == NULL);

        if ((floor->flags & 1) && destructent && (destructent->moreFlags & 0x10))
        {
            if (destructent->baseType == eBaseTypePlatform &&
                destructent->subType == ZPLATFORM_SUBTYPE_PADDLE &&
                (((zPlatform*)destructent)->passet->paddle.paddleFlags & 0x200))
            {
                xVec3 bounceRay = { 0.0f, -1.0f, 0.0f };
                zPlatform_PaddleCollide(floor, &ent->bound.sph.center, &bounceRay, 1);
            }

            if (strcmp(astate->Name, "StunJump") == 0 || strcmp(astate->Name, "StunFall") == 0 ||
                strcmp(astate->Name, "StunLand") == 0)
            {
                zEntEvent(&globals.player.ent, destructent, 0x167);
            }
            else
            {
                zEntEvent(&globals.player.ent, destructent, 0x164);
            }
        }

        if ((floor->flags & 1) && destructent)
        {
            if (destructent->baseType == eBaseTypeButton)
            {
                if (gCurrentPlayer == eCurrentPlayerSpongeBob)
                {
                    zEntButton_Press((_zEntButton*)destructent, 2);
                }
                else if (gCurrentPlayer == eCurrentPlayerPatrick)
                {
                    zEntButton_Press((_zEntButton*)destructent, 0x100);
                }
            }
            else if (destructent->baseType == eBaseTypeNPC)
            {
                zNPCCommon* npc = (zNPCCommon*)destructent;
                npc->Damage(DMGTYP_ABOVE, &globals.player.ent, &floor->tohit);
                if (!npc->IsHealthy())
                {
                    destroyed = 1;
                }
            }
            else if (destructent->baseType == eBaseTypeDestructObj)
            {
                if (!(destructent->chkby & 0x10))
                {
                    destroyed = 1;
                }
            }
        }

        if (destructent)
        {
            if (destroyed)
            {
                floor->flags &= ~1;
            }
            else
            {
                bbounce_hit = 1;
                ent->frame->vel.y = 0.0f;
            }
        }
    }

    if (sRingDelay >= 1.1f)
    {
        F32 stunlerp;
        F32 mag = 1.25f * (2.0f - sRingDelay);

        if (1.0f < mag)
        {
            stunlerp = 1.0f;
        }
        else
        {
            stunlerp = mag;
        }

        zNPCMsg_AreaPlayerStun(5.0f, 5.0f * stunlerp + (1.0f - stunlerp), NULL);
    }

    zEntPlayerFloorUpdate(ent, sc, dt);
    PlayerTeeterCheck(ent, sc, dt);
    zEntPlayerSurfDamageUpdate(ent, sc, dt);
    zEntPlayerDriveUpdate(ent, sc, dt);
    zEntPlayerJumpUpdate(ent, sc, dt);
    zEntPlayerTSlideUpdate(ent, sc, dt);
    zEntPlayerCollTrigger(ent, sc);

    if (globals.player.RootUp.y != globals.player.RootUpTarget.y ||
        1.0f != globals.player.RootUp.y)
    {
        xVec3 ax;
        xVec3Sub(&ax, &globals.player.RootUpTarget, &globals.player.RootUp);

        F32 rads = xVec3Dot(&ax, &ax);
        F32 crs;

        if (globals.player.HangElapsed <= 0.25f)
        {
            crs = 0.07f;
        }
        else if (globals.player.HangElapsed >= 0.5f)
        {
            crs = 0.5f;
        }
        else
        {
            crs = 1.72f * (globals.player.HangElapsed - 0.25f);
        }

        if (rads < crs * crs)
        {
            globals.player.RootUp = globals.player.RootUpTarget;
        }
        else
        {
            xVec3SMul(&ax, &ax, 1.0f / xsqrt(rads) * crs);
            xVec3Add(&globals.player.RootUp, &globals.player.RootUp, &ax);
            xVec3Normalize(&globals.player.RootUp, &globals.player.RootUp);
        }
    }

    zEntPlayerVelUpdate(ent, sc, dt);

    ent->frame->oldmat.pos.y -= sPlayerCollAdjust;
    ent->frame->mat.pos.y -= sPlayerCollAdjust;

    xEntEndUpdate(ent, sc, dt);

    static U32 streakBubbleSpinID = 0xdead;

    if (ent->model == globals.player.model_spongebob && globals.player.IsBubbleSpinning &&
        globals.player.model_wand)
    {
        xBound wandB;
        wandB.type = XBOUND_TYPE_SPHERE;

        xSphere* wand = &wandB.sph;
        xVec3 a;
        xVec3 b;

        iModelTagEval(ent->model->Data, &globals.player.BubbleWandTag[0],
                      globals.player.model_wand->Mat, &a);
        iModelTagEval(ent->model->Data, &globals.player.BubbleWandTag[1],
                      globals.player.model_wand->Mat, &b);

        wandB.sph.center.x = 0.5f * (a.x + b.x);
        wandB.sph.center.y = 0.5f * (a.y + b.y);
        wandB.sph.center.z = 0.5f * (a.z + b.z);
        wandB.sph.r = globals.player.g.BSpinRadius;
        wandB.sph.center.y = 0.5f * (a.y + b.y) - 0.15f;

        if (streakBubbleSpinID == 0xdead)
        {
            iColor_tag streakWandCol;
            iColor_tag streakWandCol2;

            streakWandCol.r = 255;
            streakWandCol.g = 255;
            streakWandCol.b = 128;
            streakWandCol2.r = 255;
            streakWandCol2.g = 128;
            streakWandCol2.b = 128;

            streakBubbleSpinID =
                xFXStreakStart(0.0f, 5.0f, 1.0f, 0, &streakWandCol, &streakWandCol2, 0);
        }

        xFXStreakUpdate(streakBubbleSpinID, &a, &b);

        if (gPTankDisable)
        {
            sLastBubbleEmit += dt;
            if (sLastBubbleEmit > 0.02f)
            {
                sLastBubbleEmit = 0.0f;

                xParEmitterCustomSettings info;
                info.custom_flags = 0x35e;
                info.pos = wand->center;

                xVec3Sub(&info.vel, &wand->center, (xVec3*)&ent->model->Mat->pos);
                info.vel.y = 0.0f;
                xVec3SMulBy(&info.vel, 0.4f);

                info.vel_angle_variation = 0.15f * PI;
                info.rate.set(100.0f, 100.0f, 1.0f, 0);
                info.life.set(1.0f, 1.0f, 1.0f, 0);

                F32 size = 0.15f * xurand() + 0.05f;
                info.size_birth.set(size, size, 1.0f, 0);
                size *= 1.2f;
                info.size_death.set(size, size, 1.0f, 0);

                xParEmitterEmitCustom(sEmitSpinBubbles, dt, &info);
            }
        }
        else
        {
            zEntPlayer_SpawnWandBubbles(&wand->center, 3);
        }

        wandB.sph.center.x = 0.333f * (ent->model->Mat->pos.x + (a.x + b.x));
        wandB.sph.center.y =
            0.05f + (ent->model->Mat->pos.y + globals.player.g.BSpinRadius);
        wandB.sph.center.z = 0.333f * (ent->model->Mat->pos.z + (a.z + b.z));

        xQuickCullForBound(&wandB.qcd, &wandB);
        MeleeAttackBoundCollide(ent, (zScene*)sc, &wandB);
    }
    else
    {
        if (streakBubbleSpinID != 0xdead)
        {
            xFXStreakStop(streakBubbleSpinID);
        }

        streakBubbleSpinID = 0xdead;
    }

    xVec3Sub(&lastDeltaPos, &ent->frame->mat.pos, &ent->frame->oldmat.pos);

    if (surfSlickRatio != 0.0f)
    {
        lastFloorNorm = globals.player.floor_norm;
        lastFloorEnt = (xEnt*)ent->collis->colls[0].optr;

        if (lastFloorEnt && lastFloorEnt->id != xStrHash("slipdeck01") &&
            lastFloorEnt->id != xStrHash("slipdeck02") &&
            lastFloorEnt->id != xStrHash("slipdeck03") &&
            lastFloorEnt->id != xStrHash("slipdeck04"))
        {
            lastFloorEnt = NULL;
        }
    }

    if (globals.player.carry.grabbed)
    {
        if (globals.player.carry.grabbed->baseType == eBaseTypeBoulder)
        {
            xEntBoulder* boul = (xEntBoulder*)globals.player.carry.grabbed;
            xVec3Init(&boul->vel, 0.0f, 0.0f, 0.0f);
            boul->angVel = 0.0f;
            boul->chkby = 0;
        }

        char tmpStateName[256];
        xAnimSingle* playerAnim = ent->model->Anim->Single;

        strcpy(tmpStateName, playerAnim->State->Name);
        strcat(tmpStateName, "Item");

        xAnimState* itemAnim = xAnimTableGetState(ent->model->Anim->Table, tmpStateName);

        if (itemAnim)
        {
            xVec3 tmptran;
            xQuat tmpquat;

            iAnimEval(itemAnim->Data->RawData[0],
                      ent->model->Anim->Single->Time + 0.03336667f, 1, &tmptran, &tmpquat);

            xMat4x3 objMat;
            xQuatToMat(&tmpquat, (xMat3x3*)&objMat);
            objMat.pos = tmptran;

            xVec3 rotatedLC;
            xEntBoulder* boul;

            if (globals.player.carry.grabbed->baseType == eBaseTypeBoulder)
            {
                boul = (xEntBoulder*)globals.player.carry.grabbed;
                xMat3x3RMulVec(&rotatedLC, (xMat3x3*)boul->model->Mat, &boul->localCenter);
            }

            if (globals.player.carry.grabTarget)
            {
                xMat4x3 targetMat;
                xMat3x3Rot((xMat3x3*)&targetMat, &g_Y3, globals.player.carry.targetRot);
                targetMat.pos = ent->frame->mat.pos;
                xMat4x3Mul(&objMat, &objMat, &targetMat);

                if (globals.player.carry.grabLerpMin < globals.player.carry.grabLerpMax &&
                    globals.player.carry.grabLerpLast < globals.player.carry.grabLerpMax)
                {
                    F32 lerp =
                        (globals.player.carry.grabLerpLast - globals.player.carry.grabLerpMin) /
                        (globals.player.carry.grabLerpMax - globals.player.carry.grabLerpMin);

                    if (lerp <= 0.0f)
                    {
                        objMat.pos.x = objMat.pos.x - globals.player.carry.grabOffset.x;
                        objMat.pos.z = objMat.pos.z - globals.player.carry.grabOffset.z;
                    }
                    else
                    {
                        objMat.pos.x =
                            objMat.pos.x - globals.player.carry.grabOffset.x * (1.0f - lerp);
                        objMat.pos.z =
                            objMat.pos.z - globals.player.carry.grabOffset.z * (1.0f - lerp);
                    }
                }

                if (!globals.player.carry.grabYclear)
                {
                    F32 bottom;

                    if (globals.player.carry.grabbed->baseType == eBaseTypeBoulder)
                    {
                        bottom = boul->model->Mat->pos.y + rotatedLC.y - boul->bound.sph.r;
                    }
                    else
                    {
                        bottom = globals.player.carry.grabbed->model->Mat->pos.y;
                    }

                    if (bottom > objMat.pos.y)
                    {
                        objMat.pos.y = bottom;
                    }
                    else
                    {
                        globals.player.carry.grabYclear = 1;
                    }
                }
            }
            else
            {
                xMat4x3Mul(&objMat, &objMat, &ent->frame->mat);
            }

            xMat4x3Mul(&objMat, &globals.player.carry.spin, &objMat);

            if (globals.player.carry.grabbed->baseType == eBaseTypeBoulder)
            {
                objMat.pos.y = objMat.pos.y + boul->bound.sph.r;
                xVec3Copy(&boul->bound.sph.center, &objMat.pos);
                xMat3x3RMulVec(&rotatedLC, (xMat3x3*)&objMat, &boul->localCenter);
                xVec3SubFrom(&objMat.pos, &rotatedLC);
            }

            *globals.player.carry.grabbed->model->Mat = *(RwMatrixTag*)&objMat;

            if (globals.player.carry.grabbed->frame)
            {
                globals.player.carry.grabbed->frame->mat = objMat;
            }
        }
        else
        {
            if (globals.player.carry.grabbed->baseType == eBaseTypeNPC)
            {
                ((zNPCCommon*)globals.player.carry.grabbed)
                    ->SetCarryState((en_NPC_CARRY_STATE)0);
            }
            else if (!zThrown_KillFruit(globals.player.carry.grabbed))
            {
                zThrown_Remove(globals.player.carry.grabbed);
                xEntReset(globals.player.carry.grabbed);
            }

            globals.player.carry.grabbed->chkby |= 0x10;

            if (globals.player.carry.grabbed->baseType == eBaseTypeNPC)
            {
                zNPCCommon* npc = (zNPCCommon*)globals.player.carry.grabbed;
                if ((((xNPCBasic*)npc)->SelfType() & 0xffffff00) == 'NTT\0' && !npc->IsHealthy())
                {
                    globals.player.carry.grabbed->chkby = 0;
                }
            }

            globals.player.carry.grabbed = NULL;
        }

        if (strcmp(playerAnim->State->Name, "Carry_Throw") == 0 && playerAnim->Time >= 0.3f)
        {
            if (globals.player.carry.throwTarget)
            {
                if (zThrown_IsFruit(globals.player.carry.grabbed, NULL) &&
                    zThrown_IsFruit(globals.player.carry.throwTarget, NULL))
                {
                    zThrown_LaunchStack(globals.player.carry.grabbed,
                                        globals.player.carry.throwTarget);
                }
                else if (globals.player.carry.throwTarget->baseType == eBaseTypePlatform &&
                         (((zPlatform*)globals.player.carry.throwTarget)->plat_flags & 2))
                {
                    xVec3 tgtpos;
                    tgtpos.x = 0.0f;
                    tgtpos.y = 1.229f;
                    tgtpos.z = -2.0f;

                    xMat4x3Toworld(&tgtpos,
                                   (xMat4x3*)globals.player.carry.throwTarget->model->Mat,
                                   &tgtpos);

                    if (zThrown_LaunchPos(globals.player.carry.grabbed, &tgtpos,
                                          (xVec3*)&globals.player.ent.model->Mat->at))
                    {
                        globals.player.carry.flyingToTarget = globals.player.carry.throwTarget;
                    }

                    zThrown_PatrickLauncher(globals.player.carry.grabbed,
                                            globals.player.carry.throwTarget);
                }
                else
                {
                    xVec3 tempPos;

                    if (globals.player.carry.throwTarget->baseType == eBaseTypeButton)
                    {
                        tempPos.x =
                            0.95f * globals.player.carry.throwTarget->model->Mat->up.x +
                            globals.player.carry.throwTarget->model->Mat->pos.x;
                        tempPos.y =
                            0.95f * globals.player.carry.throwTarget->model->Mat->up.y +
                            globals.player.carry.throwTarget->model->Mat->pos.y - 0.5f;
                        tempPos.z =
                            0.95f * globals.player.carry.throwTarget->model->Mat->up.z +
                            globals.player.carry.throwTarget->model->Mat->pos.z;
                    }
                    else
                    {
                        tempPos.x = globals.player.carry.throwTarget->model->Mat->pos.x;
                        tempPos.y =
                            globals.player.carry.throwTarget->model->Mat->pos.y + 1.12f;
                        tempPos.z = globals.player.carry.throwTarget->model->Mat->pos.z;
                    }

                    if (zThrown_LaunchPos(globals.player.carry.grabbed, &tempPos,
                                          (xVec3*)&globals.player.ent.model->Mat->at))
                    {
                        globals.player.carry.flyingToTarget = globals.player.carry.throwTarget;
                    }
                    else
                    {
                        globals.player.carry.flyingToTarget = NULL;
                    }
                }
            }
            else
            {
                zThrown_LaunchDir(globals.player.carry.grabbed,
                                  (xVec3*)&globals.player.ent.model->Mat->at);
                globals.player.carry.flyingToTarget = NULL;
            }

            globals.player.carry.grabbed = NULL;
            globals.player.carry.throwTarget = NULL;
        }
    }

    if (globals.player.ControlOff && globals.player.ControlOnEvent)
    {
        zEntPlayerControlOn(CONTROL_OWNER_EVENT);
        globals.player.ControlOnEvent = 0;
    }

    RwMatrixTag rootOldMat = *ent->model->Mat;

    if (1.0f != globals.player.RootUp.y)
    {
        xMat4x3 tmpMat;
        tmpMat.up = globals.player.RootUp;

        xVec3 ax;
        xVec3Cross(&ax, &globals.player.RootUp, (xVec3*)&ent->model->Mat->up);

        F32 crs = xVec3Normalize(&ax, &ax);
        F32 dot = xVec3Dot((xVec3*)&ent->model->Mat->up, &globals.player.RootUp);

        if (0.0f == crs)
        {
            if (dot < 0.0f)
            {
                xVec3Inv(&tmpMat.at, (xVec3*)&ent->model->Mat->at);
            }
        }
        else
        {
            F32 rads = xasin(crs);
            if (dot < 0.0f)
            {
                rads = PI - rads;
            }

            xMat3x3 rotMat;
            xMat3x3Rot(&rotMat, &ax, rads);
            xMat3x3LMulVec(&tmpMat.right, &rotMat, (xVec3*)ent->model->Mat);
            xMat3x3LMulVec(&tmpMat.at, &rotMat, (xVec3*)&ent->model->Mat->at);
        }

        tmpMat.pos.x = 0.0f;

        if (globals.player.HangEnt)
        {
            tmpMat.pos.y = -ent->bound.sph.r;
        }
        else
        {
            tmpMat.pos.y = 0.0f;
        }

        tmpMat.pos.z = 0.0f;

        xMat3x3RMulVec(&tmpMat.pos, &tmpMat, &tmpMat.pos);

        tmpMat.pos.x = tmpMat.pos.x + ent->model->Mat->pos.x;

        if (globals.player.HangEnt)
        {
            tmpMat.pos.y =
                tmpMat.pos.y + (ent->model->Mat->pos.y + ent->bound.sph.r);
        }
        else
        {
            tmpMat.pos.y = tmpMat.pos.y + ent->model->Mat->pos.y;
        }

        tmpMat.pos.z = tmpMat.pos.z + ent->model->Mat->pos.z;

        if (globals.player.HangEnt && globals.player.HangStartLerp < 1.0f)
        {
            tmpMat.pos.z = globals.player.HangStartLerp *
                               (tmpMat.pos.z - globals.player.HangStartPos.z) +
                           globals.player.HangStartPos.z;
            tmpMat.pos.x = globals.player.HangStartLerp *
                               (tmpMat.pos.x - globals.player.HangStartPos.x) +
                           globals.player.HangStartPos.x;
            tmpMat.pos.y = globals.player.HangStartLerp *
                               (tmpMat.pos.y - globals.player.HangStartPos.y) +
                           globals.player.HangStartPos.y;
        }

        tmpMat.flags = 0;

        *(xMat4x3*)ent->model->Mat = tmpMat;
        rendermat = tmpMat;
    }
    else
    {
        rendermat = *(xMat4x3*)ent->model->Mat;
    }

    sHitchAngle += 3.14f * dt;
    if (sHitchAngle > 2.0f * PI)
    {
        sHitchAngle -= 2.0f * PI;
    }

    xMat3x3 hitchMat;
    xMat3x3RotY(&hitchMat, sHitchAngle);

    for (S32 hitch = 0; hitch < sNumHitches; hitch++)
    {
        if (sHitch[hitch]->flags & 1)
        {
            xMat3x3Copy((xMat3x3*)sHitch[hitch]->model->Mat, &hitchMat);
        }
    }

    if (ent->model == globals.player.model_sandy)
    {
        S32 wasCoptering = globals.player.IsCoptering;
        S32 hitch = 0;

        if (strcmp(ent->model->Anim->Single->State->Name, "LCopter01") == 0 ||
            strcmp(ent->model->Anim->Single->State->Name, "LCopterHeadUp01") == 0)
        {
            hitch = 1;
        }

        globals.player.IsCoptering = (U8)hitch;

        if (globals.player.IsCoptering)
        {
            globals.player.ent.frame->vel.x = 0.0f;
            globals.player.ent.frame->vel.y = -1.0f;
            globals.player.ent.frame->vel.z = 0.0f;
        }

        if (wasCoptering && !globals.player.IsCoptering)
        {
            ent->model->Anim->Single->CurrentSpeed = 1.0f;
        }
        else if (globals.player.IsCoptering && sLassoInfo->copterTime < 2.0f)
        {
            F32 speed = 1.0f - 0.375f * (2.0f - sLassoInfo->copterTime);
            ent->model->Anim->Single->CurrentSpeed = speed;

            U32 heliSnd = sPlayerSndID[gCurrentPlayer][ePlayerSnd_Heli];
            if (heliSnd)
            {
                xSndSetPitch(heliSnd, 5.0f * (-1.0f + speed));
            }
        }

        if (globals.player.JumpState == 0)
        {
            sLassoInfo->canCopter = 1;
        }

        if (sLasso->flags & 1)
        {
            if (sLassoInfo->swingTarget)
            {
                if (strcmp(ent->model->Anim->Single->State->Name, "LassoSwingCatch02") == 0)
                {
                    if (sLasso->flags & 0x800)
                    {
                        xVec3Sub(&sLasso->crCenter, xBoundCenter(&sLassoInfo->swingTarget->bound),
                                 (xVec3*)&ent->model->Mat->pos);
                        xVec3AddScaled(&sLasso->crCenter,
                                       (xVec3*)&sLassoInfo->swingTarget->model->Mat->up, -0.6f);
                    }
                    else
                    {
                        xVec3Copy(&sLasso->tgCenter,
                                  xBoundCenter(&sLassoInfo->swingTarget->bound));
                        xVec3AddScaled(&sLasso->tgCenter,
                                       (xVec3*)&sLassoInfo->swingTarget->model->Mat->up, -0.6f);
                    }
                }
                else if (strcmp(ent->model->Anim->Single->State->Name, "LassoSwing") == 0)
                {
                    xVec3Sub(&sLasso->crCenter, xBoundCenter(&sLassoInfo->swingTarget->bound),
                             (xVec3*)&ent->model->Mat->pos);
                    xVec3AddScaled(&sLasso->crCenter,
                                   (xVec3*)&sLassoInfo->swingTarget->model->Mat->up, -0.6f);
                }
            }

            if (sLassoInfo->target)
            {
                if (strcmp(ent->model->Anim->Single->State->Name, "LassoFly") == 0)
                {
                    if (!sLassoInfo->targetGuide && !(sLasso->flags & 0x800))
                    {
                        xVec3Copy(&sLasso->tgCenter, xBoundCenter(&sLassoInfo->target->bound));
                        xVec3AddScaled(&sLasso->tgCenter, (xVec3*)&ent->model->Mat->at,
                                       sLassoInfo->target->model->Data->boundingSphere.radius *
                                           sLassoInfo->dist);
                        xVec3AddScaled(&sLasso->tgCenter, (xVec3*)&ent->model->Mat->up,
                                       sLassoInfo->target->model->Data->boundingSphere.radius *
                                           sLassoInfo->dist);
                    }
                }
                else if (strcmp(ent->model->Anim->Single->State->Name, "LassoDestroy") == 0 &&
                         !(sLasso->flags & 0x800))
                {
                    xVec3Copy(&sLasso->tgCenter, xBoundCenter(&sLassoInfo->target->bound));
                }
            }

            zLasso_Update(sLasso, ent, dt);

            if (sLassoInfo->destroy && (sLasso->flags & 0x800))
            {
                if (sLassoInfo->target)
                {
                    zEntEvent(sLassoInfo->target, 0x3a);
                }

                sLassoInfo->destroy = 0;
            }

            if (sLassoInfo->copterTime > 0.0f)
            {
                sLassoInfo->copterTime -= dt;
                if (sLassoInfo->copterTime < 0.0f)
                {
                    sLassoInfo->canCopter = 0;
                }
            }
        }

        xModelTag* meleeTag = NULL;

        if (strcmp(ent->model->Anim->Single->State->Name, "Melee01") == 0 &&
            ent->model->Anim->Single->Time >= globals.player.g.SandyMeleeMinFrame &&
            ent->model->Anim->Single->Time <= globals.player.g.SandyMeleeMaxFrame)
        {
            meleeTag = &sSandyRHand;
        }
        else if (strcmp(ent->model->Anim->Single->State->Name, "JumpMelee01") == 0 ||
                 (sShouldMelee &&
                  strcmp(ent->model->Anim->Single->State->Name, "DJumpApex01") == 0))
        {
            meleeTag = &sSandyLFoot;
        }

        if (meleeTag)
        {
            xBound meleeB;
            meleeB.type = XBOUND_TYPE_SPHERE;

            iModelTagEval(ent->model->Data, meleeTag, ent->model->Mat, &meleeB.sph.center);

            meleeB.sph.r = globals.player.g.SandyMeleeRadius;

            xQuickCullForBound(&meleeB.qcd, &meleeB);
            MeleeAttackBoundCollide(ent, (zScene*)sc, &meleeB);
        }

        if (gReticleTarget && gReticleTarget->baseType == eBaseTypeNPC &&
            !((zNPCCommon*)gReticleTarget)->CanRope())
        {
            gReticleTarget = NULL;
            sTimeToRetarget = 0.0f;
        }

        if (gReticleTarget)
        {
            xVec3 disp;
            xVec3Sub(&disp, (xVec3*)&gReticleTarget->model->Mat->pos,
                     (xVec3*)&ent->model->Mat->pos);

            if (xVec3Length2(&disp) > 100.0f)
            {
                gReticleTarget = NULL;
                sTimeToRetarget = 0.0f;
            }
        }

        if (!globals.player.ControlOff && (globals.pad0->pressed & XPAD_BUTTON_O) &&
            !gReticleTarget)
        {
            sTimeToRetarget = 0.0f;
        }

        sTimeToRetarget -= dt;

        U32 sliding = ent->model->Anim->Single->State->UserFlags & 0x1000;

        if (sliding && gReticleTarget && sTypeOfTarget != 1)
        {
            gReticleTarget = NULL;
            sTimeToRetarget = 0.25f;
        }

        if (sTimeToRetarget < 0.0f)
        {
            if (((sLasso->flags & 1) == 0 && sLassoInfo->target == NULL && meleeTag == NULL) ||
                globals.player.IsCoptering || sLassoInfo->swingTarget)
            {
                xEnt* oldTarget = gReticleTarget;
                xEnt* closest = NULL;

                sTimeToRetarget = 0.25f;

                F32 closestDist_sqr = 100.0f;
                xVec3 toTarget;

                if (globals.player.JumpState == 0 && !sliding)
                {
                    xRay3 ray;
                    xCollis rayCollis;
                    U32 i = 0;

                    for (; i < ((zScene*)sc)->num_base; i++)
                    {
                        xEnt* targent = (xEnt*)((zScene*)sc)->base[i];

                        if (targent->baseType != eBaseTypeDestructObj &&
                            targent->baseType != eBaseTypeNPC)
                        {
                            continue;
                        }

                        if (!(targent->flags & 1))
                        {
                            continue;
                        }

                        if (!targent->model)
                        {
                            continue;
                        }

                        xVec3Sub(&toTarget, (xVec3*)&targent->model->Mat->pos,
                                 (xVec3*)&ent->model->Mat->pos);

                        F32 currDist_sqr = xVec3Length2(&toTarget);

                        if (currDist_sqr >= 100.0f)
                        {
                            continue;
                        }

                        if (currDist_sqr >= closestDist_sqr)
                        {
                            continue;
                        }

                        if (targent->baseType == eBaseTypeNPC)
                        {
                            if ((((xNPCBasic*)targent)->SelfType() & 0xffffff00) == 'NTT\0' &&
                                !((zNPCCommon*)targent)->flg_vuln)
                            {
                                continue;
                            }

                            if (((xNPCBasic*)targent)->SelfType() == 'NTT4')
                            {
                                continue;
                            }
                        }

                        if (targent->baseType == eBaseTypeDestructObj &&
                            !zEntDestructObj_GetHit((zEntDestructObj*)targent, 0x200))
                        {
                            continue;
                        }

                        if (targent->baseType == eBaseTypeNPC &&
                            !((zNPCCommon*)targent)->CanRope())
                        {
                            continue;
                        }

                        rayCollis.flags = 0;

                        F32 dist = xsqrt(currDist_sqr);

                        ray.max_t = dist;
                        ray.min_t = 0.5f;

                        xVec3Copy(&ray.origin, (xVec3*)&ent->model->Mat->pos);
                        ray.origin.y += 0.5f;

                        xVec3SMul(&ray.dir, &toTarget, 1.0f / dist);

                        ray.flags = 0xc00;

                        xRayHitsSceneFlags(globals.sceneCur, &ray, &rayCollis, 0x10, 0x2e);

                        if ((rayCollis.flags & 1) && rayCollis.optr != targent)
                        {
                            continue;
                        }

                        if (xVec3Dot(&toTarget, (xVec3*)&ent->model->Mat->at) <= 0.0f)
                        {
                            continue;
                        }

                        closestDist_sqr = currDist_sqr;
                        closest = targent;
                    }

                    gReticleTarget = closest;
                    sTypeOfTarget = 0;
                }

                if (!gReticleTarget || globals.player.JumpState)
                {
                    F32 maxDist_sqr = 100.0f;
                    xEnt* targent = NULL;

                    for (S32 i = 0; i < sNumHitches; i++)
                    {
                        xEnt* hitchent = sHitch[i];

                        if (sLassoInfo->swingTarget == hitchent)
                        {
                            continue;
                        }

                        if (!(hitchent->flags & 1))
                        {
                            continue;
                        }

                        xVec3Sub(&toTarget, (xVec3*)&hitchent->model->Mat->pos,
                                 (xVec3*)&ent->model->Mat->pos);

                        if (globals.player.JumpState == 0 && toTarget.y <= 1.0f)
                        {
                            continue;
                        }

                        F32 currDist_sqr = xVec3Length2(&toTarget);

                        if (currDist_sqr >= maxDist_sqr)
                        {
                            continue;
                        }

                        if (xVec3Dot(&toTarget, (xVec3*)&ent->model->Mat->at) <= 0.0f)
                        {
                            continue;
                        }

                        maxDist_sqr = currDist_sqr;
                        targent = hitchent;
                    }

                    if (targent)
                    {
                        gReticleTarget = targent;
                        sTypeOfTarget = 1;
                    }
                }

                if (gReticleTarget != oldTarget)
                {
                    sReticleAlpha = 0.0f;
                }
            }
        }

        if (strcmp(ent->model->Anim->Single->State->Name, "DJumpApex01") != 0)
        {
            sShouldMelee = 0;
        }

        if (((sLasso->flags & 1) == 0 && sLassoInfo->target == NULL && meleeTag == NULL) ||
            globals.player.IsCoptering)
        {
            sLassoInfo->target = NULL;
            sLassoInfo->swingTarget = NULL;

            if (!globals.player.ControlOff && (globals.pad0->pressed & XPAD_BUTTON_O))
            {
                if (gReticleTarget)
                {
                    xVec3 disp;
                    xVec3Sub(&disp, (xVec3*)&gReticleTarget->model->Mat->pos,
                             (xVec3*)&ent->model->Mat->pos);

                    F32 dist = xVec3Length(&disp);

                    switch (sTypeOfTarget)
                    {
                    case 0:
                        sLassoInfo->target = gReticleTarget;
                        sLassoInfo->dist = dist / 10.0f;
                        break;
                    case 1:
                        sLassoInfo->swingTarget = gReticleTarget;
                        sLassoInfo->dist = dist * 0.25f;
                        break;
                    }
                }
            }
            else if (!globals.player.ControlOff && (globals.pad0->pressed & XPAD_BUTTON_TRIANGLE) &&
                     strcmp(ent->model->Anim->Single->State->Name, "LedgeGrab01") != 0 &&
                     !globals.player.IsCoptering && !zEntTeleportBox_playerIn())
            {
                sShouldMelee = 1;
            }
        }
    }

    if (ent->model == globals.player.model_patrick &&
        strcmp(ent->model->Anim->Single->State->Name, "Melee01") == 0 &&
        ent->model->Anim->Single->Time >= 0.1333f &&
        ent->model->Anim->Single->Time < 0.4f && globals.player.DoMeleeCheck)
    {
        xBound meleeB;
        meleeB.type = XBOUND_TYPE_SPHERE;

        iModelTagEval(ent->model->Data, &sPatrickMelee, ent->model->Mat, &meleeB.sph.center);

        meleeB.sph.r = 0.3f;

        xQuickCullForBound(&meleeB.qcd, &meleeB);
        zFX_SpawnBubbleTrail(&meleeB.sph.center, 1);

        if (MeleeAttackBoundCollide(ent, (zScene*)sc, &meleeB))
        {
            globals.player.DoMeleeCheck = 0;
        }
    }

    zEntPlayer_StreakFX(ent, dt);

    *ent->model->Mat = rootOldMat;

    if (dt)
    {
        tslide_lastrealvel.x =
            (ent->model->Mat->pos.x - ent->frame->oldmat.pos.x) / dt;
        tslide_lastrealvel.y =
            (ent->model->Mat->pos.y - ent->frame->oldmat.pos.y) / dt;
        tslide_lastrealvel.z =
            (ent->model->Mat->pos.z - ent->frame->oldmat.pos.z) / dt;
    }
}

xVec3* NPCC_rightDir(xEnt* ent);

void zEntPlayer_CheckCritterContact(xEnt* player, F32 dt)
{
    S32 i;
    xEntCollis* plyrcol = player->collis;

    if (globals.player.DamageTimer > 0.0f)
    {
        return;
    }

    for (i = plyrcol->npc_sidx; i < plyrcol->npc_eidx; i++)
    {
        xCollis* colrec = &plyrcol->colls[i];

        if (colrec->optr == NULL)
        {
            continue;
        }

        zNPCCommon* npc = (zNPCCommon*)(xEnt*)(colrec->optr);

        if (npc->baseType != eBaseTypeNPC)
        {
            continue;
        }

        S32 npctype = npc->SelfType();

        if (npctype == NPC_TYPE_CRITTER)
        {
            zEntPlayer_DamageNPCKnockBack(npc, 1, npc->Pos());
            return;
        }

        if (npctype == NPC_TYPE_GLOVE)
        {
            if (xVec3Dot(&g_NY3, &colrec->hdng) < 0.86f)
            {
                continue;
            }

            xPsyche* psy = npc->psy_instinct;

            if (psy->GIDOfActive() != NPC_GOAL_ALERTGLOVE)
            {
                continue;
            }

            F32 tym_inGoal = psy->TimerGet(XPSY_TYMR_CURGOAL);

            if (tym_inGoal < 0.35f)
            {
                continue;
            }

            static U32 hashes_ss[3] = { xStrHash("BBounceStrike01"), xStrHash("BBounceAttack01"),
                                        xStrHash("BBounceStrike01") };
            static U32 hashes_pa[3] = { xStrHash("StunJump"), xStrHash("StunFall"),
                                        xStrHash("StunLand") };

            U32 anid_player = globals.player.ent.model->Anim->Single->State->ID;
            S32 found = 0;

            if (gCurrentPlayer == eCurrentPlayerSpongeBob)
            {
                for (S32 k = 0; k < 3; k++)
                {
                    if (anid_player == hashes_ss[k])
                    {
                        found = 1;
                        break;
                    }
                }
            }
            else if (gCurrentPlayer == eCurrentPlayerPatrick)
            {
                for (S32 k = 0; k < 3; k++)
                {
                    if (anid_player == hashes_pa[k])
                    {
                        found = 1;
                        break;
                    }
                }
            }

            if (found)
            {
                continue;
            }

            zEntPlayer_DamageNPCKnockBack(npc, 1, npc->Pos());
            return;
        }

        if ((npctype & 0xffffff00) == 'NTR\0' || (npctype & 0xffffff00) == 'NTF\0' ||
            (npctype & 0xffffff00) == 'NTD\0' || (npctype & 0xffffff00) == 'NTA\0')
        {
            if (xVec3Dot(&g_NY3, &colrec->hdng) < 0.65f)
            {
                continue;
            }

            U32 mvinf = zEntPlayer_MoveInfo();

            if (!(mvinf & 0x1))
            {
                continue;
            }

            xVec3 dir_push;
            F32 dsq = npc->XZDstSqToPlayer(&dir_push, NULL);

            if (dsq > 1e-5f)
            {
                dir_push /= xsqrt(dsq);
            }
            else
            {
                dir_push = *NPCC_rightDir(&globals.player.ent);
            }

            xVec3 vel_push = dir_push * 3.0f * dt;

            player->frame->mat.pos += vel_push;
            player->frame->mode |= 0x1;
        }
    }
}

void zEntPlayer_PatrickLaunch(xEnt* patLauncher)
{
    globals.player.carry.patLauncher = patLauncher;
}

void zEntPlayer_ShadowModelEnable()
{
    if (globals.player.ent.model == globals.player.model_spongebob)
    {
        globals.player.sb_models[10]->Flags = globals.player.sb_models[10]->Flags | 1;
        globals.player.sb_models[11]->Flags = globals.player.sb_models[11]->Flags | 1;
        globals.player.sb_models[12]->Flags = globals.player.sb_models[12]->Flags | 1;
        globals.player.sb_models[13]->Flags =
            globals.player.sb_models[13]->Flags | globals.player.sb_models[5]->Flags & 1;
        globals.player.sb_models[0]->Flags &= 0xfffe;
        globals.player.sb_models[1]->Flags &= 0xfffe;
        globals.player.sb_models[2]->Flags &= 0xfffe;
        globals.player.sb_models[5]->Flags &= 0xfffe;
    }
    if (globals.player.ent.model == globals.player.model_sandy)
    {
        globals.player.model_sandy->Next->Flags &= 0xfffe;
    }
}

void zEntPlayer_ShadowModelDisable()
{
    if (globals.player.ent.model == globals.player.model_spongebob)
    {
        globals.player.sb_models[0]->Flags = globals.player.sb_models[0]->Flags | 1;
        globals.player.sb_models[1]->Flags = globals.player.sb_models[1]->Flags | 1;
        globals.player.sb_models[2]->Flags = globals.player.sb_models[2]->Flags | 1;
        globals.player.sb_models[5]->Flags =
            globals.player.sb_models[5]->Flags | globals.player.sb_models[13]->Flags & 1;
        globals.player.sb_models[10]->Flags &= 0xfffe;
        globals.player.sb_models[11]->Flags &= 0xfffe;
        globals.player.sb_models[12]->Flags &= 0xfffe;
        globals.player.sb_models[13]->Flags &= 0xfffe;
    }
    if (globals.player.ent.model == globals.player.model_sandy)
    {
        globals.player.model_sandy->Next->Flags |= 1;
    }
}

static void zEntPlayer_BubbleBowlLaneRender(zEnt* ent)
{
    xShadowCache cache;
    xVec3 center;
    F32 factor;
    xMat4x3 matrix;
    U32 i;

    factor = 1.75f * sBubbleBowlTimer;

    if (factor < 0.0f)
    {
        factor = 0.0f;
    }

    if (factor > 1.0f)
    {
        factor = 1.0f;
    }

    xVec3Copy(&center, (xVec3*)&ent->model->Mat->pos);
    xVec3AddScaled(&center, (xVec3*)&ent->model->Mat->up, 2.0f);
    xVec3AddScaled(&center, (xVec3*)&ent->model->Mat->at, 4.5f + factor);

    xShadowVertical_FillCache(&cache, &center, 0.5f * factor + 0.7f, 6.0f, 0.65f);

    xVec3Copy(&matrix.pos, &center);
    xVec3Copy(&matrix.right, (xVec3*)&ent->model->Mat->right);
    xVec3Copy(&matrix.up, (xVec3*)&ent->model->Mat->at);
    xVec3Inv(&matrix.at, (xVec3*)&ent->model->Mat->up);
    xMat3x3SMul(&matrix, &matrix, 1.0f);

    gShadowObjectRadius = 0.5f * factor + 0.7f;
    xShadowVertical_DrawCache(&cache, factor, 0.0f, 1, (RwMatrixTag*)&matrix, sBowlingLaneRast);

    for (i = 0; i < cache.entCount; i++)
    {
        xEnt* ep = cache.ent[i];

        if (xShadowReceiveShadowSetup(ep))
        {
            xShadowReceiveShadow(ep, factor, 1, (RwMatrixTag*)&matrix, sBowlingLaneRast);
        }
    }
}

struct zNPCDutchman;

extern zNPCDutchman* dutchman_reticle_ent;
extern xVec3 dutchman_reticle_center;
extern F32 dutchman_reticle_radius;

xVec3* NPCC_rightDir(xEnt* ent);
xVec3* NPCC_faceDir(xEnt* ent);
xVec3* NPCC_upDir(xEnt* ent);

inline void get_reticle_bound(xVec3& center, F32& radius);

static void zEntPlayer_ReticleRender(zEnt* ent)
{
    if (gReticleTarget && sReticleModel && sReticleAlpha >= 1.0f / 255.0f)
    {
        iModelSetMaterialAlpha(sReticleModel, 255.0f * sReticleAlpha);

        F32 scale = 4.0f * (1.0f - sReticleAlpha) + 1.0f;

        xVec3 fromcam;
        xVec3Sub(&fromcam, (xVec3*)&gReticleTarget->model->Mat->pos, &globals.camera.mat.pos);

        scale = scale * (0.2f * xVec3Length(&fromcam));

        if (scale < 1.0f)
        {
            scale = 1.0f;
        }

        sReticleMat.up.y = scale;

        F32 bob = 0.3f * isin(sReticleRot);

        if (bob < 0.0f)
        {
            bob = -bob;
        }

        sReticleMat.at.x = -globals.camera.mat.right.z;
        sReticleMat.at.y = 0.0f;
        sReticleMat.at.z = globals.camera.mat.right.x;

        xVec3SMulBy(&sReticleMat.at, scale / xVec3Length(&sReticleMat.at));

        sReticleMat.right.x = sReticleMat.at.z;
        sReticleMat.right.y = 0.0f;
        sReticleMat.right.z = -sReticleMat.at.x;

        F32 radius;
        get_reticle_bound(sReticleMat.pos, radius);

        sReticleMat.pos.y = sReticleMat.pos.y + (radius + bob);

        if (!iModelCull(sReticleModel, (RwMatrixTag*)&sReticleMat))
        {
            iModelRender(sReticleModel, (RwMatrixTag*)&sReticleMat);
        }

        sReticleMat.up.y = -scale;
        sReticleMat.right.x = -sReticleMat.right.x;
        sReticleMat.right.z = -sReticleMat.right.z;
        sReticleMat.pos.y -= 2.0f * (radius + bob);

        if (!iModelCull(sReticleModel, (RwMatrixTag*)&sReticleMat))
        {
            iModelRender(sReticleModel, (RwMatrixTag*)&sReticleMat);
        }

        sReticleMat.up.y = 1.0f;
    }
}

inline void get_reticle_bound(xVec3& center, F32& radius)
{
    if (gReticleTarget == (xEnt*)dutchman_reticle_ent)
    {
        center = dutchman_reticle_center;
        radius = dutchman_reticle_radius;
    }
    else if (gReticleTarget->baseType == 0x2b)
    {
        zNPCCommon* npc = (zNPCCommon*)gReticleTarget;
        S32 type = npc->SelfType();

        if (type == NPC_TYPE_SLEEPY)
        {
            xBox* box = &npc->bound.box.box;

            center = npc->bound.box.center;
            radius = box->upper.y - box->lower.y;
            radius *= 0.5f;
        }
        else if (type == NPC_TYPE_CHUCK)
        {
            static const xVec3 offsetChuck = { 0.3f, -0.55f, 0.5f };

            radius = 1.2f * npc->bound.sph.r;
            center = npc->bound.sph.center;

            center += *NPCC_rightDir(npc) * offsetChuck.x;
            center += *NPCC_upDir(npc) * offsetChuck.y;
            center += *NPCC_faceDir(npc) * offsetChuck.z;
        }
        else
        {
            center = *(xVec3*)&gReticleTarget->model->Mat->pos;
            center.y = center.y + gReticleTarget->model->Data->boundingSphere.center.y;
            radius = gReticleTarget->model->Data->boundingSphere.radius;
            radius = radius * xVec3Length((xVec3*)&gReticleTarget->model->Mat->up);
        }
    }
    else
    {
        center = *(xVec3*)&gReticleTarget->model->Mat->pos;
        center.y = center.y + gReticleTarget->model->Data->boundingSphere.center.y;
        radius = gReticleTarget->model->Data->boundingSphere.radius;
        radius = radius * xVec3Length((xVec3*)&gReticleTarget->model->Mat->up);
    }
}

static void zEntPlayerUpdateModelSB()
{
    xAnimSingle* single = globals.player.ent.model->Anim->Single;
    S32 i;

    xModelInstance* m = globals.player.sb_models[0];
    m->Flags |= 3;
    *m->Mat = *globals.player.ent.model->Mat;

    m = globals.player.sb_models[1];
    m->Flags |= 3;
    *m->Mat = *globals.player.ent.model->Mat;

    m = globals.player.sb_models[2];
    m->Flags |= 3;
    *m->Mat = *globals.player.ent.model->Mat;

    m = globals.player.sb_models[3];

    if (zGameExtras_CheatFlags() & 0x10000000)
    {
        m->Flags |= 3;
    }
    else
    {
        m->Flags &= 0xfffc;
    }

    m = globals.player.sb_models[4];
    m->Flags &= 0xfffc;

    m = globals.player.sb_models[5];

    if (globals.player.IsBubbleSpinning || strcmp(single->State->Name, "BbashStart01") == 0 ||
        strcmp(single->State->Name, "BbounceStrike01") == 0 ||
        strcmp(single->State->Name, "BbounceStart01") == 0 ||
        strcmp(single->State->Name, "BbounceAttack01") == 0)
    {
        m->Flags |= 1;
    }
    else
    {
        m->Flags &= 0xfffe;
    }

    m = globals.player.sb_models[6];

    xAnimState* state = xAnimTableGetState(m->Anim->Table, single->State->Name);

    if (state)
    {
        m->Flags = 1;
        m->Anim->Single->State = state;
        m->Anim->Single->Time = single->Time;

        if (m->Anim->Single->State->Data->NumAnims[0] == 3)
        {
            m->Anim->Single->BilinearLerp[0] = 1.0f + globals.player.SlideTrackLean;
        }

        xAnimPlayUpdate(m->Anim, 0.0f);
        xAnimPlayEval(m->Anim);

        xMat4x3Mul((xMat4x3*)m->Mat, (xMat4x3*)&globals.player.ent.model->Mat[3],
                   (xMat4x3*)globals.player.ent.model->Mat);
    }
    else
    {
        m->Flags = 0;
    }

    m = globals.player.sb_models[7];

    if ((strcmp(single->State->Name, "BbashStart01") == 0 && single->Time >= 0.1f) ||
        strcmp(single->State->Name, "BbashAttack01") == 0)
    {
        xMat4x3Mul((xMat4x3*)m->Mat, (xMat4x3*)&globals.player.ent.model->Mat[5],
                   (xMat4x3*)globals.player.ent.model->Mat);

        RpAtomicSetRenderCallBack(m->Data, xFXBubbleRender);

        if (m->Flags & 2)
        {
            m->Flags |= 1;
            globals.player.IsBubbleBashing = 1;
        }
        else
        {
            m->Flags |= 2;
        }
    }
    else
    {
        m->Flags &= 0xfffe;
    }

    S32 bone_index[2] = { 38, 42 };
    xModelInstance* model_index[2] = { globals.player.sb_models[8], globals.player.sb_models[9] };

    for (i = 0; i < 2; i++)
    {
        F32 startTime = 0.075f * i + 0.4f;
        F32 strikeTime = 0.04f * i + 0.4f;
        S32 bone = bone_index[i];

        m = model_index[i];

        if ((strcmp(single->State->Name, "BbounceStart01") == 0 &&
             single->Time >= startTime) ||
            strcmp(single->State->Name, "BbounceAttack01") == 0 ||
            (strcmp(single->State->Name, "BbounceStrike01") == 0 &&
             single->Time <= strikeTime))
        {
            xMat4x3Mul((xMat4x3*)m->Mat, (xMat4x3*)&globals.player.ent.model->Mat[bone],
                       (xMat4x3*)globals.player.ent.model->Mat);

            RpAtomicSetRenderCallBack(m->Data, xFXBubbleRender);

            if (m->Flags & 2)
            {
                m->Flags |= 1;
                globals.player.IsBubbleBouncing = 1;
            }
            else
            {
                m->Flags |= 2;
            }
        }
        else
        {
            m->Flags &= 0xfffe;
        }
    }
}

void zEntPlayerUpdateModel()
{
    zPlayerGlobals* pg = &globals.player;

    if (pg->ent.model == pg->model_spongebob)
    {
        zEntPlayerUpdateModelSB();
    }
}

static void zEntPlayerEmitTongueBubbles()
{
    xModelInstance* model = globals.player.sb_models[6];
    U8 rand;
    xVec3 vec;

    if ((model->Flags & 1) && (rand = xrand(), rand < 0x80))
    {
        xVec3Copy(&vec, (xVec3*)&model->Mat->pos);
        vec.y -= 0.5f;
        zFX_SpawnBubbleTrail(&vec, 1);
    }
}

static void zEntPlayerEmitSlideBubbles()
{
    xModelInstance* model = globals.player.ent.model;
    U8 rand;
    xVec3 vec;

    if ((model->Flags & 1) && (rand = xrand(), rand < 0x80))
    {
        xVec3Copy(&vec, (xVec3*)&model->Mat->pos);
        vec.y -= 0.5f;
        zFX_SpawnBubbleTrail(&vec, 1);
    }
}

static void zEntPlayerCheckHelmetPop()
{
    xVec3 vec;
    xModelInstance* model = globals.player.sb_models[7];
    if ((globals.player.IsBubbleBashing == 0) || (globals.player.sb_models[7]->Flags & 1))
    {
        return;
    }
    xMat4x3Mul((xMat4x3*)globals.player.sb_models[7]->Mat,
               (xMat4x3*)(&globals.player.ent.model->Mat[5]),
               (xMat4x3*)(globals.player.ent.model)->Mat);
    xVec3Copy(&vec, (xVec3*)&model->Mat->pos);
    vec.y += 0.35f;
    zFX_SpawnBubbleHit(&vec, 0x32);
    globals.player.IsBubbleBashing = 0;
}

static void zEntPlayerCheckShoePop()
{
    xEnt& ent = globals.player.ent;
    xModelInstance** mlist;
    S32 i;
    S32 bone;

    if (globals.player.IsBubbleBouncing != 0)
    {
        S32 bone_index[2] = { 38, 42 };
        xModelInstance* model_index[2] = { globals.player.sb_models[8],
                                           globals.player.sb_models[9] };

        for (i = 0; i < 2; i++)
        {
            bone = bone_index[i];
            xModelInstance* m = model_index[i];

            if (!(m->Flags & 1))
            {
                xMat4x3Mul((xMat4x3*)m->Mat, (xMat4x3*)&ent.model->Mat[bone],
                           (xMat4x3*)ent.model->Mat);
                zFX_SpawnBubbleHit((xVec3*)&m->Mat->pos, 10);
                globals.player.IsBubbleBouncing = 0;
            }
        }
    }
}

void zEntPlayer_Render(zEnt* ent)
{
    if (bungee_state::render())
    {
        return;
    }

    if (cruise_bubble::render())
    {
        return;
    }

    if (!globals.player.Visible)
    {
        xShadowManager_Remove(ent);
        return;
    }

    F32 lerp = 0.0f;

    RwMatrixTag rootOldMat = *ent->model->Mat;

    xAnimSingle* single = ent->model->Anim->Single;
    xAnimSingle* blend = single->Blend;

    if (strcmp(single->State->Name, "SD0_SetDown") == 0 ||
        strcmp(single->State->Name, "SD1_SetDown") == 0)
    {
        if (single->Time >= 0.85f)
        {
            lerp = -0.6f;
        }
        else if (single->Time > 0.65f)
        {
            lerp = -0.6f * (single->Time - 0.65f) / (0.85f - 0.65f);
        }
    }
    else if (blend->State &&
             (strcmp(blend->State->Name, "SD0_SetDown") == 0 ||
              strcmp(blend->State->Name, "SD1_SetDown") == 0))
    {
        lerp = -0.6f * (1.0f - single->BlendFactor * single->Tran->BlendRecip);
    }

    if (lerp)
    {
        xVec3 f = { 0.0f, 0.0f, 0.0f };
        f.z = lerp;

        xMat3x3RMulVec(&f, (xMat3x3*)ent->model->Mat, &f);

        ent->model->Mat->pos.x = ent->model->Mat->pos.x + f.x;
        ent->model->Mat->pos.z = ent->model->Mat->pos.z + f.z;
    }

    *(xMat4x3*)ent->model->Mat = rendermat;

    if (1.0f != globals.player.RootUp.y || lerp)
    {
        xModelInstance* m = ent->model->Next;
        while (m)
        {
            *m->Mat = *ent->model->Mat;
            m = m->Next;
        }
    }

    xAnimSingle* playerAnim = ent->model->Anim->Single;

    zEntPlayerUpdateModel();

    if (ent->model == globals.player.model_spongebob)
    {
        zEntPlayerEmitTongueBubbles();

        xVec3 a;
        xVec3 b;

        iModelTagEval(ent->model->Data, &globals.player.BubbleWandTag[0],
                      globals.player.model_wand->Mat, &a);
        iModelTagEval(ent->model->Data, &globals.player.BubbleWandTag[1],
                      globals.player.model_wand->Mat, &b);

        xVec3 center;
        center.x = (a.x + b.x) / 2.0f;
        center.y = (a.y + b.y) / 2.0f;
        center.y = (a.y + b.y) / 2.0f - 0.15f;
        center.z = (a.z + b.z) / 2.0f;

        if (!gPTankDisable && strcmp(playerAnim->State->Name, "BbashStart01") == 0)
        {
            zEntPlayer_SpawnWandBubbles(&center, 1);
        }

        zEntPlayerCheckHelmetPop();

        if (!gPTankDisable && strcmp(playerAnim->State->Name, "BbounceStart01") == 0)
        {
            zEntPlayer_SpawnWandBubbles(&center, 1);
        }

        zEntPlayerCheckShoePop();
    }
    else if (ent->model == globals.player.model_sandy)
    {
        U32 sliding = tslide_ground;
        xModelInstance* m = ent->model;
        S32 i = 0;

        while (m)
        {
            switch (i)
            {
            case 0:
            case 1:
                m->Flags |= 3;
                break;
            case 2:
                if (sliding)
                {
                    if (m->Flags & 2)
                    {
                        m->Flags |= 1;
                    }
                    else
                    {
                        m->Flags |= 2;
                    }

                    *m->Mat = *ent->model->Mat;
                }
                else
                {
                    m->Flags &= ~3;
                }
                break;
            }

            m = m->Next;
            i++;
        }

        if (globals.player.SlideTrackSliding)
        {
            zEntPlayerEmitSlideBubbles();
        }

        if (sLasso->flags & 1)
        {
            if (sLasso->flags & 0x1000)
            {
                sLasso->flags &= ~0x1000;
            }
            else
            {
                zLasso_Render(sLasso);
            }
        }
    }
    else if (ent->model == globals.player.model_patrick && globals.player.SlideTrackSliding)
    {
        zEntPlayerEmitSlideBubbles();
    }

    F32 dot = 0.0f;

    if (gReticleTarget)
    {
        xVec3 toCam;
        xVec3Sub(&toCam, &globals.camera.mat.pos, (xVec3*)&ent->model->Mat->pos);

        xVec3 toTarget;
        xVec3Sub(&toTarget, (xVec3*)&gReticleTarget->model->Mat->pos,
                 (xVec3*)&ent->model->Mat->pos);

        dot = xVec3Dot(&toCam, &toTarget);

        if (dot <= 0.0f)
        {
            zEntPlayer_ReticleRender(ent);
        }
    }

    if (gCurrentPlayer == eCurrentPlayerSandy)
    {
        ent->model->Next->Flags &= ~1;
    }

    xEntRender(&globals.player.ent);

    if (gCurrentPlayer == eCurrentPlayerSandy)
    {
        xModelInstance* m = ent->model->Next;
        m->Flags |= 1;

        RwRenderStateSet((RwRenderState)0x14, (void*)3);
        xModelRenderSingle(m);
        RwRenderStateSet((RwRenderState)0x14, (void*)2);
        xModelRenderSingle(m);
        RwRenderStateSet((RwRenderState)0x14, (void*)1);
    }

    if (dot > 0.0f)
    {
        zEntPlayer_ReticleRender(ent);
    }

    *ent->model->Mat = rootOldMat;

    if (globals.player.IsBubbleBowling)
    {
        zEntPlayer_BubbleBowlLaneRender(ent);
    }
}

static void dampen_velocity(xVec3& v1, const xVec3& v2, F32 f);

static void zEntPlayer_Move(xEnt* ent, xScene*, F32 dt, xEntFrame* frame)
{
    frame->mode = 0;
    frame->mode &= ~0x400;
    frame->mode &= ~0x800;

    if (globals.player.Health)
    {
        _tagxPad* pad = globals.pad0;
        S32 xval = -pad->analog1.x;
        S32 yval = -pad->analog1.y;

        if (!globals.player.g.CheatPlayerSwitch)
        {
            if (pad->on & XPAD_BUTTON_LEFT)
            {
                xval = globals.player.g.AnalogMax;
            }
            else if (pad->on & XPAD_BUTTON_RIGHT)
            {
                xval = -globals.player.g.AnalogMax;
            }

            if (pad->on & XPAD_BUTTON_UP)
            {
                yval = globals.player.g.AnalogMax;
            }
            else if (pad->on & XPAD_BUTTON_DOWN)
            {
                yval = -globals.player.g.AnalogMax;
            }
        }

        PlayerAbsControl(ent, xval, yval, dt);
    }

    if (globals.player.cheat_mode)
    {
        if (!globals.player.ControlOff && (globals.pad0->on & XPAD_BUTTON_SQUARE))
        {
            frame->dpos.y +=
                dt * (7.0f * ((globals.pad0->on & (XPAD_BUTTON_L1 | XPAD_BUTTON_L2 |
                                                   XPAD_BUTTON_R1 | XPAD_BUTTON_R2)) ?
                                  4.68f :
                                  1.56f));
            frame->mode |= 0x2;
        }
        else if (!globals.player.ControlOff && (globals.pad0->on & XPAD_BUTTON_TRIANGLE))
        {
            frame->dpos.y -=
                dt * (7.0f * ((globals.pad0->on & (XPAD_BUTTON_L1 | XPAD_BUTTON_L2 |
                                                   XPAD_BUTTON_R1 | XPAD_BUTTON_R2)) ?
                                  4.68f :
                                  1.56f));
            frame->mode |= 0x2;
        }
    }

    if (globals.player.WallJumpState == k_WALLJUMP_LAUNCH)
    {
        xVec3 wallnorm;

        frame->mode |= 0x10000;

        xVec3Inv(&wallnorm, &sWallNormal);
        TurnToFace(ent, &wallnorm, 10.0f, dt);
    }

    if (!globals.player.JumpState)
    {
        sPlayerAttackInAir = 0;
    }

    if (sPlayerAttackInAir > 1)
    {
        return;
    }

    if (surfSlickRatio)
    {
        return;
    }

    if (globals.player.IsBubbleSpinning ||
        strcmp(ent->model->Anim->Single->State->Name, "Melee01") == 0 ||
        strcmp(ent->model->Anim->Single->State->Name, "JumpMelee01") == 0)
    {
        const zPlayerSettings& sb = globals.player.sb;
        const xVec3 damp = { sb.spin_damp_xz, sb.spin_damp_y, sb.spin_damp_xz };

        dampen_velocity(ent->frame->vel, damp, dt);
        dampen_velocity(ent->frame->dpos, damp, dt);
    }
}

void zEntPlayer_setBoulderMode(U32 mode)
{
    if (mode != 0)
    {
        boulderRollShouldStart = 1;
        boulderRollShouldEnd = 0;
    }
    else
    {
        boulderRollShouldStart = 0;
        boulderRollShouldEnd = 1;
    }
}

S32 zEntPlayer_Damage(xBase* src, U32 damage, const xVec3* knockback)
{
    S32 newDamage = zEntPlayer_Damage(src, damage);

    if (!newDamage)
    {
        return false;
    }

    if (knockback)
    {
        globals.player.ent.frame->vel.x = knockback->x;
        globals.player.ent.frame->vel.y = knockback->y;
        globals.player.ent.frame->vel.z = knockback->z;
    }

    return true;
}

S32 zEntPlayer_DamageNPCKnockBack(xBase* src, U32 damage, xVec3* npcPos)
{
    S32 tmpRtn;
    F32 dx;
    F32 dz;
    F32 mag;
    F32 tmpVel;

    if (zEntPlayer_Damage(src, damage) == 0)
    {
        tmpRtn = 0;
    }
    else
    {
        dx = globals.player.ent.model->Mat->pos.x - npcPos->x;
        dz = globals.player.ent.model->Mat->pos.z - npcPos->z;
        mag = xsqrt(dx * dx + dz * dz);

        if (mag < 0.01f)
        {
            mag = 1.5f;
            dx = -globals.player.ent.model->Mat->at.x;
            dz = -globals.player.ent.model->Mat->at.z;
        }

        tmpVel = sPlayerNPC_KnockBackVel / mag;
        tmpRtn = 1;

        globals.player.ent.frame->vel.x = tmpVel * dx;
        globals.player.ent.frame->vel.y = 0.0f;
        globals.player.ent.frame->vel.z = tmpVel * dz;

        globals.player.KnockBackTimer = sPlayerNPC_KnockBackTime;
        globals.player.KnockIntoAirTimer = 0.0f;
    }

    return tmpRtn;
}

void zEntPlayer_DamageKnockIntoAir(F32 height)
{
    zJumpParam jump;
    jump.PeakHeight = height;
    jump.TimeHold = 0.0f;
    jump.TimeGravChange = 0.3f;
    CalcJumpImpulse(&jump, NULL);
    zEntPlayerJumpStart(&globals.player.ent, &jump);
    zEntPlayerJumpAddDriver(&globals.player.ent);
    globals.player.KnockIntoAirTimer = 0.75f;
    globals.player.Jump_CanDouble = 1;
    globals.player.Jump_CanFloat = 1;
}

S32 zEntPlayer_Damage(xBase* src, U32 damage)
{
    iColor_tag c_inside;
    iColor_tag c_outside;

    if (globals.player.cheat_mode != 0)
    {
        if (!(globals.player.DamageTimer > 0.0f))
        {
            globals.player.DamageTimer = globals.player.g.DamageTimeHit;
        }
        return 0;
    }
    else
    {
        if (globals.player.ent.update == zEntPlayer_BoulderVehicleUpdate)
        {
            BoulderRollDoneCB();
        }

        if (!(((damage == 0x29a) && (damage = 1, 1 < globals.player.Health)) ||
              (!(globals.player.DamageTimer > 0.0f) && !(globals.player.ControlOffTimer > 0.0f) &&
               (zEntTeleportBox_playerIn() == 0))))
        {
            return 0;
        }

        zEntPlayerControlOn(CONTROL_OWNER_SPRINGBOARD);
        zEntPlayer_SNDStop(ePlayerSnd_BowlWindup);
        zEntPlayer_SNDStop(ePlayerSnd_CruiseNavigate);
        zEntPlayer_SNDStop(ePlayerSnd_BubbleWand);
        zEntPlayer_SNDStop(ePlayerSnd_Heli);
        zEntPlayer_SNDStop(ePlayerSnd_SlideLoop);
        zEntPlayer_SNDStop(ePlayerSnd_SlipLoop);
        zEntPlayer_SNDPlayRandom(ePlayerSnd_OuchStart, ePlayerSnd_OuchEnd, 0.0f);

        // NOTE: the alpha channel of both colours is left uninitialised in retail.
        c_inside.r = 255;
        c_inside.g = 255;
        c_inside.b = 255;
        c_outside.r = 255;
        c_outside.g = 0;
        c_outside.b = 0;

        xFXShineStart((xVec3*)&globals.player.ent.model->Mat->pos, 2.0f, 0.5f, 0.0f, 0.1f, 0,
                      &c_inside, &c_outside, 0.1f, 1);

        globals.player.ScareSource = src;

        if (damage > globals.player.Health)
        {
            damage = globals.player.Health;
        }

        if (globals.player.g.TakeDamage)
        {
            globals.player.Health -= damage;
        }

        if (globals.player.Health == 0)
        {
            sPlayerDiedLastTime = 1;
            zEntPlayer_SNDPlay(ePlayerSnd_Death, 0.0f);

            if ((xrand() & 3) == 3)
            {
                zEntPlayer_SNDPlayStreamRandom(ePlayerStreamSnd_Exclaim1, ePlayerStreamSnd_Exclaim4,
                                               0.6f);
            }

            globals.player.DamageTimer = 10.0f;
            zEntPlayerControlOff(CONTROL_OWNER_GLOBAL);

            if (globals.player.ent.driver)
            {
                globals.player.ent.driver = NULL;
                xVec3Copy(&globals.player.ent.frame->vel, &g_O3);
                xVec3Copy(&globals.player.ent.frame->oldvel, &g_O3);
            }

            zEntPickup_GiveAllRewardsNow();
            gEnableRewardsQueue = 0;
            zEntEvent(&globals.player.ent, &globals.player.ent, eEventPlayerDeath);
        }
        else
        {
            globals.player.DamageTimer = globals.player.g.DamageTimeHit;
        }

        if (globals.player.HangEnt)
        {
            globals.player.HangEnt->grabTimer = globals.player.HangEnt->hangInfo->grabDelay;
            globals.player.HangEnt = NULL;
            globals.player.RootUpTarget.x = 0.0f;
            globals.player.RootUpTarget.y = 1.0f;
            globals.player.RootUpTarget.z = 0.0f;
            globals.player.HangElapsed = 0.0f;
        }

        if (globals.player.ent.model == globals.player.model_sandy)
        {
            if (globals.player.Health != 0)
            {
                player_hitlist_anim = (player_hitlist_anim + 1) % sandyHitMax;
                xAnimPlayStartTransition(globals.player.model_sandy->Anim,
                                         &sandyHitTran[player_hitlist_anim]);
            }
        }
        else if (globals.player.ent.model == globals.player.model_patrick)
        {
            if (globals.player.Health != 0)
            {
                player_hitlist_anim = (player_hitlist_anim + 1) % patrickHitMax;
                xAnimPlayStartTransition(globals.player.model_patrick->Anim,
                                         &patrickHitTran[player_hitlist_anim]);
            }
        }
        else if (globals.player.ent.model == globals.player.model_spongebob)
        {
            if (globals.player.Health != 0)
            {
                player_hit = 1;
            }
        }

        idle_tmr = 0.0f;

        if (sLassoInfo->target && sLassoInfo->targetGuide)
        {
            ((zNPCCommon*)sLassoInfo->target)->LassoNotify(LASS_EVNT_ENDED);
        }

        globals.player.lassoInfo.lasso.flags = 0;
        globals.player.lassoInfo.target = NULL;
        zRumbleStart(globals.currentActivePad, SDR_Damage);

        globals.player.SpeedMult = 1.0f;
        globals.player.SundaeTimer = -1.0f;
        globals.player.IsBubbleBowling = 0;
    }

    return 1;
}

S32 zEntPlayer_MoveInfo()
{
    U32 animflags = globals.player.ent.model->Anim->Single->State->UserFlags & 0x1e;
    U32 infoflags = 0;
    const char* nam_ast = globals.player.ent.model->Anim->Single->State->Name;

    if (animflags == 0 || globals.player.ent.model->Anim->Single->State->UserFlags & 1)
    {
        infoflags |= 1;
    }

    if (animflags == 4)
    {
        infoflags |= 2;
    }

    if ((animflags == 6) || (animflags == 8))
    {
        infoflags |= 4;
    }

    if (globals.player.IsBubbleSpinning || strcmp(nam_ast, "Bspin01") == 0)
    {
        infoflags |= 0x20;
    }

    if (strcmp(nam_ast, "BbashStart01") == 0)
    {
        infoflags |= 8;
    }

    if ((strcmp(nam_ast, "BbounceStrike01") == 0) || (strcmp(nam_ast, "BbounceStart01") == 0) ||
        (strcmp(nam_ast, "BbounceAttack01") == 0))
    {
        infoflags |= 0x10;
    }

    if (animflags == 0xe)
    {
        infoflags |= 0x10;
    }

    if (animflags == 0xc)
    {
        infoflags |= 0x20;
    }

    if (animflags == 2 || infoflags & 1 || infoflags & 2)
    {
        infoflags |= 0x40;
    }

    return infoflags;
}

void zEntPlayer_GiveHealth(S32 quantity)
{
    if (quantity < 0 && -quantity > (S32)globals.player.Health)
    {
        globals.player.Health = 0;
        return;
    }

    U32 sum = globals.player.Health + quantity;
    U32 maxHealth = globals.player.MaxHealth;
    globals.player.Health = sum;

    if (sum > maxHealth)
    {
        globals.player.Health = maxHealth;
    }
}

void zEntPlayer_GiveSpatula(S32)
{
    sSpatulaGrabbed = 1;

    if (globals.player.ControlOffTimer < 0.1f)
    {
        globals.player.ControlOffTimer = 0.1f;
    }

    zNPCMsg_AreaNotify(NULL, NPC_MID_PLYRSPATULA, 30.0f, 0x104, NPC_TYPE_UNKNOWN);
    zMusicSetVolume(0.5f, 0.2f);
}

void zEntPlayer_GiveShinyObject(S32 quantity)
{
    if (quantity < 0 && -quantity > (S32)globals.player.Inv_Shiny)
    {
        globals.player.Inv_Shiny = 0;
        return;
    }

    U32 sum = globals.player.Inv_Shiny + quantity;
    U32 maxShinies = SHINY_MAX;
    globals.player.Inv_Shiny = sum;

    if (sum > maxShinies)
    {
        globals.player.Inv_Shiny = maxShinies;
    }
}

void zEntPlayer_GivePatsSocksCurrentLevel(S32 quantity)
{
    U32 level = zSceneGetLevelIndex();

    if (quantity < 0 && -quantity > (S32)globals.player.Inv_PatsSock_Total)
    {
        globals.player.Inv_PatsSock_Total = 0;
    }
    else
    {
        globals.player.Inv_PatsSock_Total += quantity;
    }

    if (quantity < 0 && -quantity > (S32)globals.player.Inv_PatsSock[level])
    {
        globals.player.Inv_PatsSock[level] = 0;
    }
    else
    {
        globals.player.Inv_PatsSock[level] += quantity;
    }

    globals.player.Inv_PatsSock_CurrentLevel = globals.player.Inv_PatsSock[level];

    if (quantity > 0)
    {
        zNPCMsg_AreaNotify(NULL, NPC_MID_PLYRSPATULA, 30.0f, 0x104, NPC_TYPE_UNKNOWN);
    }
}

void zEntPlayer_GiveLevelPickupCurrentLevel(S32 quantity)
{
    U32 level = zSceneGetLevelIndex();

    if (quantity < 0 && -quantity > (S32)globals.player.Inv_LevelPickups[level])
    {
        globals.player.Inv_LevelPickups[level] = 0;
    }
    else
    {
        globals.player.Inv_LevelPickups[level] += quantity;
    }

    globals.player.Inv_LevelPickups_CurrentLevel = globals.player.Inv_LevelPickups[level];

    if (quantity > 0)
    {
        zNPCMsg_AreaNotify(NULL, NPC_MID_PLYRSPATULA, 30.0f, 0x104, NPC_TYPE_UNKNOWN);
    }
}

static F32 CalcJumpImpulse_Smooth(F32 g, F32 j, F32 h, F32 Tgc, F32 Tgs)
{
    F32 Tm[3];
    U32 solcnt;
    U32 i;
    F32 Tmfound;

    F32 b0 = 0.0f;
    F32 b1 = 0.0f;
    F32 b2 = -j / 2.0f;
    F32 T1 = Tgc + Tgs;
    F32 A3;
    F32 B2;
    F32 A = (j - g) / (6.0f * Tgs);
    F32 B = (g * Tgc - j * Tgc - j * Tgs) / (2.0f * Tgs);
    F32 Tgc2 = Tgc * Tgc;
    F32 T12 = T1 * T1;
    A3 = 3.0f * A;
    B2 = 2.0f * B;

    F32 Kc = -(A3 * Tgc2 + j * Tgc + B2 * Tgc);
    F32 D = b2 * Tgc2 - A * (Tgc * Tgc2) - B * Tgc2 - Kc * Tgc;
    F32 c2 = -g / 2.0f;
    F32 v1 = A3 * T12 + B2 * T1 + g * T1;
    F32 c0 = D + (A * (T1 * T12) + B * T12) - c2 * T12 - v1 * T1;

    F32 t1 = xsqrt((b0 - h) / b2);
    F32 t2 = xsqrt((c0 - h) / c2);

    solcnt = xMathSolveCubic(-2.0f * A, -B, 0.0f, D - h, &Tm[0], &Tm[1], &Tm[2]);

    if (t1 <= Tgc && t1 >= 0.0f)
    {
        return -b1 - 2.0f * b2 * t1;
    }

    if (t2 >= T1)
    {
        return -(v1 + Kc) - 2.0f * c2 * t2;
    }

    Tmfound = -1.0f;

    for (i = 0; i < solcnt; i++)
    {
        if (Tm[i] >= Tgc && Tm[i] <= T1)
        {
            if (Tmfound < 0.0f || Tm[i] < Tmfound)
            {
                Tmfound = Tm[i];
            }
        }
    }

    if (Tmfound != -1.0f)
    {
        return -Kc - B2 * Tmfound - A3 * Tmfound * Tmfound;
    }

    return 1.0f;
}

void CalcJumpImpulse(zJumpParam* param, const zPlayerSettings* settings)
{
    if (settings == 0)
    {
        settings = globals.player.s;
    }
    param->ImpulseVel =
        CalcJumpImpulse_Smooth(globals.player.g.Gravity, settings->JumpGravity, param->PeakHeight,
                               param->TimeGravChange, settings->GravSmooth);
}

void zEntPlayerJumpStart(xEnt* ent, zJumpParam* jump)
{
    globals.player.Jump_CurrGravity = globals.player.s->JumpGravity;
    globals.player.Jump_HoldTimer = jump->TimeHold;
    globals.player.Jump_ChangeTimer = jump->TimeGravChange;

    if ((ent->frame->vel.y > 0.0f) && (globals.player.JumpState == 2))
    {
        ent->frame->vel.y = jump->ImpulseVel;
    }
    else
    {
        ent->frame->vel.y = jump->ImpulseVel;
    }

    globals.player.JumpState = 2;

    if (((ent->model->Anim->Single->State->UserFlags & 30) != 14) &&
        ((ent->model->Anim->Single->State->UserFlags & 256) == 0))
    {
        return;
    }

    globals.player.Bounced = 1;
}

void zEntPlayerJumpAddDriver(xEnt* ent)
{
    if (sDriveVel.y > 0.0f)
    {
        ent->frame->vel.y += sDriveVel.y;
    }
}

static void zEntPlayerJumpLand(xEnt* ent)
{
    F32 diff;
    F32 vol;
    F32 tempFloat;

    globals.player.JumpState = 0;
    globals.player.SlideNotGroundedSinceSlide = 0;
    zEntPlayerControlOn(CONTROL_OWNER_SPRINGBOARD);

    diff = -ent->frame->vel.y;
    vol = diff - 5.0f;

    if (vol <= 0.0f)
    {
        tempFloat = 0.0f;
    }
    else if (vol >= 10.0f)
    {
        tempFloat = 1.0f;
    }
    else
    {
        tempFloat = vol / 10.0f;
    }

    if (tempFloat > 0.0f && globals.sceneCur->sceneID != 'MNU3')
    {
        zEntPlayer_SNDPlay(ePlayerSnd_Land, 0.0f);
        zEntPlayer_SNDSetVol(ePlayerSnd_Land, tempFloat);
        if (vol > 12.0f)
        {
            zPadAddRumble(eRumble_VeryLight, 0.008f * vol, 0, 0);
        }
    }
}

static void zEntPlayerJumpUpdate(xEnt* ent, xScene* sc, F32 dt)
{
    F32 lerp;

    if (strcmp(ent->model->Anim->Single->State->Name, "BbashStrike01") == 0 &&
        ent->model->Anim->Single->Time < 13.0f / 30.0f)
    {
        return;
    }

    {
        if (strcmp(ent->model->Anim->Single->State->Name, "BbashAttack01") == 0 ||
            strcmp(ent->model->Anim->Single->State->Name, "BbashStart01") == 0)
        {
            if (bbash_tmr < 0.0f)
            {
                bbash_tmr += dt;

                if (bbash_tmr >= 0.0f)
                {
                    ent->frame->vel.y = bbash_vel;
                }
            }
            else
            {
                bbash_tmr += dt;
            }

            if (bbash_tmr > 0.0f && bbash_tmr > globals.player.g.BBashCVTime)
            {
                ent->frame->vel.y -= globals.player.g.Gravity * dt;
            }
        }
        else if (globals.player.cheat_mode == 0)
        {
            globals.player.LastJumpState = globals.player.JumpState;

            if (globals.player.Jump_HoldTimer)
            {
                globals.player.Jump_HoldTimer -= dt;

                if (globals.player.Jump_HoldTimer <= 0.0f)
                {
                    globals.player.Jump_HoldTimer = 0.0f;
                }
                else if (globals.player.ControlOff || !(globals.pad0->on & XPAD_BUTTON_X))
                {
                    globals.player.Jump_HoldTimer = 0.0f;
                    globals.player.Jump_ChangeTimer = 0.0f;
                    globals.player.Jump_CurrGravity = globals.player.g.Gravity;
                }
            }

            if (globals.player.Jump_ChangeTimer)
            {
                globals.player.Jump_ChangeTimer -= dt;

                if (0.0f == globals.player.Jump_ChangeTimer)
                {
                    globals.player.Jump_ChangeTimer = 1e-07f;
                }

                if (globals.player.Jump_ChangeTimer <= -globals.player.s->GravSmooth)
                {
                    globals.player.Jump_HoldTimer = 0.0f;
                    globals.player.Jump_ChangeTimer = 0.0f;
                    globals.player.Jump_CurrGravity = globals.player.g.Gravity;
                }
                else if (globals.player.Jump_ChangeTimer < 0.0f)
                {
                    lerp = -globals.player.Jump_ChangeTimer / globals.player.s->GravSmooth;
                    globals.player.Jump_CurrGravity = (1.0f - lerp) * globals.player.s->JumpGravity +
                                                      lerp * globals.player.g.Gravity;
                }
            }

            if ((ent->collis->colls[0].flags & 0x1) &&
                !(ent->collis->colls[0].optr &&
                  ((xBase*)ent->collis->colls[0].optr)->baseType == eBaseTypeVillain) &&
                ent->frame->vel.y <= 0.0f)
            {
                if (globals.player.JumpState)
                {
                    zEntPlayerJumpLand(ent);

                    if (gCurrentPlayer != eCurrentPlayerSandy ||
                        strcmp(ent->model->Anim->Single->State->Name, "Fall01") != 0)
                    {
                        globals.player.Jump_CanDouble = 1;
                    }

                    globals.player.Jump_CanFloat = 1;
                    globals.player.Jump_Springboard = NULL;
                    globals.player.Jump_SpringboardStart = 0;
                    globals.player.Bounced = 0;
                    zCameraSetBbounce(0);
                    zCameraSetLongbounce(0);
                    zCameraSetHighbounce(0);
                }
            }
            else
            {
                if (globals.player.JumpState == 0)
                {
                    globals.player.JumpState = 1;
                    globals.player.JumpTimer = 0.0f;
                }
                else if (globals.player.JumpState == 1)
                {
                    globals.player.JumpTimer += dt;

                    if (globals.player.JumpTimer >= 0.25f)
                    {
                        globals.player.JumpState = 2;
                    }
                }
                else if (globals.player.JumpState == 2 && ent->frame->vel.y < 16.0f &&
                         globals.player.Jump_SpringboardStart)
                {
                    globals.player.Jump_SpringboardStart = 0;

                    if (0.0f == globals.player.Jump_Springboard->passet->sb.jmpdir.x &&
                        0.0f == globals.player.Jump_Springboard->passet->sb.jmpdir.z)
                    {
                        globals.player.Jump_CanDouble = 1;
                    }
                }

                if (globals.player.JumpState == 3)
                {
                    ent->frame->vel.x *= 0.96f;
                    ent->frame->vel.z *= 0.96f;
                }
            }

            if (globals.player.Jump_CurrGravity &&
                (globals.player.JumpState == 2 || globals.player.JumpState == 1))
            {
                if (strcmp(globals.player.ent.model->Anim->Single->State->Name, "JumpMelee01") ==
                        0 &&
                    globals.player.ent.model->Anim->Single->Time < 0.25f)
                {
                    ent->frame->vel.y -= 0.8f * 0.8f * globals.player.Jump_CurrGravity * dt;
                }
                else if (!globals.player.IsCoptering &&
                         strcmp(globals.player.ent.model->Anim->Single->State->Name,
                                "BbounceStart01") != 0 &&
                         strcmp(globals.player.ent.model->Anim->Single->State->Name,
                                "BbounceAttack01") != 0)
                {
                    ent->frame->vel.y -= globals.player.Jump_CurrGravity * dt;
                }
            }

            if (globals.player.JumpState == 0 || globals.player.JumpState == 1)
            {
                globals.player.CanJump = 1;
                globals.player.IsJumping = 0;
                globals.player.WasDJumping = globals.player.IsDJumping;
                globals.player.IsDJumping = 0;
            }
        }
    }
}

static void zEntPlayerEGenUpdate(xEnt* ent, xScene* sc, F32 dt)
{
    U32 i;
    xIsect isx;
    F32 rad;

    globals.player.earc_coll.flags &= ~0x1;

    for (i = 0; i < ((zScene*)sc)->baseCount[eBaseTypeEGenerator]; i++)
    {
        zEGenerator* egen = (zEGenerator*)((zScene*)sc)->baseList[eBaseTypeEGenerator] + i;

        if (egen->flags & 0x1)
        {
            xLine3VecDist2(&egen->src_pos, &egen->dst_pos, &ent->bound.sph.center, &isx);

            rad = ent->bound.sph.r;

            if (isx.dist < rad * rad + 0.1f * (2.0f * rad) + 0.1f * 0.1f)
            {
                globals.player.earc_coll.flags |= 0x1;

                if (zEntPlayer_Damage(egen, 1, NULL))
                {
                    if (globals.player.Health)
                    {
                        globals.player.DamageTimer = globals.player.g.DamageTimeEGen;
                    }

                    zRumbleStart(SDR_DamageByEGen);
                }
            }
        }
    }
}

static void zEntPlayerVelUpdate(xEnt* ent, xScene* sc, F32 dt)
{
    F32 min;
    F32 interp;
    F32 speedMult;
    F32 gft;
    F32 s;
    xEnt* flent;
    F32 sft;
    F32 velen2;
    xCollis* colls;
    xCollis* coll;
    S32 i;
    F32 dh;
    F32 h_dot_v;
    F32 v_dot_n;
    xVec3 boost;

    if (ent->model->Anim->Single->State->UserFlags & 0x100)
    {
        return;
    }

    if (globals.player.SlideTrackSliding)
    {
        return;
    }

    xVec3* v = &ent->frame->vel;

    if (strcmp(ent->model->Anim->Single->State->Name, "BoulderRoll01") == 0)
    {
        min = 2.5f * globals.player.ent.model->Mat->at.x;

        if (min > 0.0f)
        {
            if (v->x < min)
            {
                v->x = min;
            }
        }
        else if (v->x > min)
        {
            v->x = min;
        }

        min = 2.5f * globals.player.ent.model->Mat->at.z;

        if (min > 0.0f)
        {
            if (v->z < min)
            {
                v->z = min;
            }
        }
        else if (v->z > min)
        {
            v->z = min;
        }
    }
    else if (globals.player.IsBubbleBowling)
    {
        interp = 1.0f + sBubbleBowlTimer;
        interp *= interp;
        interp *= interp;
        interp = 1.0f / interp;
        speedMult = globals.player.g.BubbleBowlMinSpeed * (1.0f - interp) +
                    interp * globals.player.bbowlInitVel;
        v->x = globals.player.ent.model->Mat->at.x * speedMult;
        v->z = globals.player.ent.model->Mat->at.z * speedMult;
    }
    else if (!(globals.player.SlideTimer > 0.0f) && !surfSlickRatio &&
             !(globals.player.SlipFadeTimer > 0.0f))
    {
        if (globals.player.JumpState == 0)
        {
            gft = globals.player.KnockBackTimer;

            if (gft)
            {
                s = gft / (gft + dt);
                v->x *= s;
                v->z *= s;
            }
            else
            {
                v->x = 0.0f;
                v->z = 0.0f;
            }

            if (globals.player.ent.collis->colls[0].flags & 0x1)
            {
                flent = (xEnt*)globals.player.ent.collis->colls[0].optr;
            }
            else
            {
                flent = NULL;
            }

            if (flent == NULL)
            {
                v->y = 0.0f;
            }
            else if (flent->baseType == eBaseTypePlatform &&
                     flent->subType == ZPLATFORM_SUBTYPE_TEETER)
            {
                if (sDriveVel.y < 0.0f)
                {
                    v->y = sDriveVel.y;
                }
                else
                {
                    v->y = 0.0f;
                }
            }
            else
            {
                v->y = 0.0f;
            }
        }
    }
    else
    {
        if (globals.player.Slide == 0 && globals.player.SlideTimer > 0.0f)
        {
            sft = 2.15f - globals.player.SlideTimer;

            if (globals.player.JumpState == 0 || globals.player.JumpState == 1)
            {
                if (globals.player.JumpState == 0)
                {
                    v->y = 0.0f;
                }

                if (sft < 0.4f)
                {
                    s = (0.4f - sft) / ((0.4f - sft) + dt);
                    v->x *= s;
                    v->z *= s;
                }
                else
                {
                    v->x = 0.0f;
                    v->z = 0.0f;
                    return;
                }
            }
            else if (sft < 2.15f)
            {
                s = (2.15f - sft) / ((2.15f - sft) + dt);
                v->x *= s;
                v->z *= s;
            }
            else
            {
                v->x = 0.0f;
                v->z = 0.0f;
                return;
            }
        }

        velen2 = xVec3Length2(v);

        if (xabs(velen2) < 0.0001f)
        {
            v->x = 0.0f;
            v->y = 0.0f;
            v->z = 0.0f;
        }
        else
        {
            i = 0;
            colls = ent->collis->colls;
            coll = colls;

            if (surfSlickRatio)
            {
                if (colls[0].optr)
                {
                    dh = ent->frame->mat.pos.y - ent->frame->oldmat.pos.y;

                    if (dh > 0.0f)
                    {
                        v->x *= 0.97f;
                        v->z *= 0.97f;
                    }
                }
                else
                {
                    v->y = 0.0f;
                }

                coll = colls + 2;
                i = 2;
            }
            else if (!(globals.player.SlideTimer > 0.0f))
            {
                if (globals.player.SlipFadeTimer > 0.0f)
                {
                    s = globals.player.SlipFadeTimer / (globals.player.SlipFadeTimer + dt);
                    v->x *= s;
                    v->z *= s;
                }
                else
                {
                    v->x = 0.0f;
                    v->z = 0.0f;
                    return;
                }
            }

            for (; i < 6; i++, coll++)
            {
                if ((coll->flags & 0x1) && coll->dist < 0.5f &&
                    (!coll->optr || ((xBase*)coll->optr)->baseType != eBaseTypeVillain))
                {
                    h_dot_v = xVec3Dot(v, &coll->hdng);

                    if (h_dot_v < 0.0f)
                    {
                        continue;
                    }

                    v_dot_n = xVec3Dot(v, &coll->norm);

                    if (v_dot_n > 0.0f)
                    {
                        continue;
                    }

                    if (-v_dot_n >= 0.965926f * velen2)
                    {
                        v->x = 0.0f;
                        v->y = 0.0f;
                        v->z = 0.0f;
                        return;
                    }

                    xVec3SMul(&boost, &coll->norm, 0.5f * -v_dot_n);
                    xVec3AddTo(v, &boost);
                    velen2 = xVec3Length2(v);
                }
            }
        }
    }
}

struct TrackPolyData
{
    xVec3 center;
    xMat4x3* mat;
    xEnt* testEnt;
    S32 triIndex;
    xVec3 vert[3];
    F32 neardist;
    xVec3 nearpt;
    S32 nearvert;
    S32 nearedge;
    xEnt* foundEnt;
};

static RpCollisionTriangle* nearestTrackCB(RpIntersection*, RpCollisionTriangle* collTriangle,
                                           RwReal, void* data)
{
    TrackPolyData* tpd = (TrackPolyData*)data;
    F32 currnear = tpd->neardist;
    xVec3 currpt;
    S32 currvert, curredge;
    xVec3 xformVert[4], xformNorm;
    S32 i;
    F32 pdx[3], pdz[3];
    F32 numer, denom, t, testdist2;
    xVec3 edgevec, centvec, testpt;

    xMat4x3Toworld(&xformVert[0], tpd->mat, (xVec3*)collTriangle->vertices[0]);
    xMat4x3Toworld(&xformVert[1], tpd->mat, (xVec3*)collTriangle->vertices[1]);
    xMat4x3Toworld(&xformVert[2], tpd->mat, (xVec3*)collTriangle->vertices[2]);
    xMat3x3RMulVec(&xformNorm, tpd->mat, (xVec3*)&collTriangle->normal);

    for (i = 0; i < 3; i++)
    {
        pdx[i] = tpd->center.x - xformVert[i].x;
        pdz[i] = tpd->center.z - xformVert[i].z;
    }

    // `zero` is a matching device, not recovered source. pdx/pdz are written
    // in a loop and so cannot be const; binding the literal to a local is the
    // only way left to stop the scheduler treating those stores as aliasing
    // this load, which retail issues ahead of them.
    const F32 zero = 0.0f;
    F32 f3 = pdx[0] * pdz[1] - pdz[0] * pdx[1];
    F32 f2 = pdx[1] * pdz[2] - pdz[1] * pdx[2];
    F32 f1 = pdx[2] * pdz[0] - pdz[2] * pdx[0];

    if ((f3 >= zero && f2 >= zero && f1 >= zero) || (f3 <= zero && f2 <= zero && f1 <= zero))
    {
        tpd->neardist = 0.0f;
        tpd->vert[0] = xformVert[0];
        tpd->vert[1] = xformVert[1];
        tpd->vert[2] = xformVert[2];
        tpd->triIndex = collTriangle->index;
        tpd->foundEnt = tpd->testEnt;

        return NULL;
    }

    xformVert[3] = xformVert[0];

    for (i = 0; i < 3; i++)
    {
        edgevec.x = xformVert[i + 1].x - xformVert[i].x;
        edgevec.y = xformVert[i + 1].y - xformVert[i].y;
        edgevec.z = xformVert[i + 1].z - xformVert[i].z;
        centvec.x = tpd->center.x - xformVert[i].x;
        centvec.y = tpd->center.y - xformVert[i].y;
        centvec.z = tpd->center.z - xformVert[i].z;

        numer = centvec.x * edgevec.x + centvec.y * edgevec.y + centvec.z * edgevec.z;
        denom = edgevec.x * edgevec.x + edgevec.y * edgevec.y + edgevec.z * edgevec.z;

        if (denom < 0.000001f)
        {
            return collTriangle;
        }

        if (numer <= 0.0f)
        {
            testdist2 = SQR(centvec.x) + SQR(centvec.y) + SQR(centvec.z);
            if (testdist2 < currnear)
            {
                currnear = testdist2;
                currpt = xformVert[i];
                currvert = i;
                curredge = -1;
            }
        }
        else if (numer < denom)
        {
            t = numer / denom;
            testpt.x = t * edgevec.x + xformVert[i].x;
            testpt.y = t * edgevec.y + xformVert[i].y;
            testpt.z = t * edgevec.z + xformVert[i].z;
            testdist2 = SQR(tpd->center.x - testpt.x) + SQR(tpd->center.y - testpt.y) +
                        SQR(tpd->center.z - testpt.z);

            if (testdist2 < currnear)
            {
                currnear = testdist2;
                currpt = testpt;
                currvert = -1;
                curredge = i;
            }
        }
    }

    if (currnear != tpd->neardist)
    {
        tpd->vert[0] = xformVert[0];
        tpd->vert[1] = xformVert[1];
        tpd->vert[2] = xformVert[2];
        tpd->neardist = currnear;
        tpd->nearpt = currpt;
        tpd->nearvert = currvert;
        tpd->nearedge = curredge;
        tpd->triIndex = collTriangle->index;
        tpd->foundEnt = tpd->testEnt;
    }

    return collTriangle;
}

static F32 det3x3top1(F32 a, F32 b, F32 c, F32 d, F32 e, F32 f)
{
    F32 ret = -((a * f) - ((b * f) - (e * c)));
    return -((d * b) - ((a * e) + ((d * c) + ret)));
}

void xQuickCullForSphere(xQCData* q, const xSphere* s);

static void SlideTrackUpdate(xEnt* p)
{
    xQCData qcd;
    RpIntersection isx;
    xSphere sph;
    xCollis coll;
    TrackPolyData tpd;
    U32 i;

    sph.center = *(xVec3*)&p->model->Mat->pos;
    sph.r = 2.0f * p->bound.sph.r;
    xQuickCullForSphere(&qcd, &sph);

    S32 triIndex = -1;

    tpd.neardist = 1e38f;
    tpd.triIndex = -1;
    tpd.center = *(xVec3*)&p->model->Mat->pos;

    isx.type = rpINTERSECTSPHERE;
    isx.t.sphere.center = p->model->Mat->pos;
    isx.t.sphere.radius = 2.0f * p->bound.sph.r;

    for (i = 0; i < globals.player.SlideTrackCount; i++)
    {
        xEnt* tent = globals.player.SlideTrackEnt[i];

        if (!xQuickCullIsects(&qcd, &tent->bound.qcd))
        {
            continue;
        }

        coll.flags = 0;
        xSphereHitsBound(&sph, &tent->bound, &coll);

        if (!(coll.flags & k_HIT_IT))
        {
            continue;
        }

        RpAtomicGetFrame(tent->model->Data)->ltm = *tent->model->Mat;

        tpd.mat = (xMat4x3*)tent->model->Mat;
        tpd.testEnt = tent;

        RpAtomicForAllIntersections(tent->model->Data, &isx, nearestTrackCB, &tpd);
    }

    if (tpd.neardist < 1.1f * p->bound.sph.r)
    {
        triIndex = tpd.triIndex;
    }

    if (triIndex == -1)
    {
        return;
    }

    if (globals.player.JumpState)
    {
        return;
    }

    if (!globals.player.JumpState && globals.player.LastJumpState)
    {
        globals.player.SlideTrackLand = 1.0f;
        return;
    }

    RpGeometry* geom = tpd.foundEnt->model->Data->geometry;
    RpTriangle* tri = &geom->triangles[triIndex];
    RwTexCoords* uvs = geom->texCoords[0];
    RwV3d* verts = geom->morphTarget->verts;

    F32 det = det3x3top1(verts[tri->vertIndex[0]].x, verts[tri->vertIndex[1]].x,
                         verts[tri->vertIndex[2]].x, verts[tri->vertIndex[0]].z,
                         verts[tri->vertIndex[1]].z, verts[tri->vertIndex[2]].z);

    if (xabs(det) < 1e-5f)
    {
        return;
    }

    F32 val = (-uvs[tri->vertIndex[0]].v *
                   det3x3top1(isx.t.sphere.center.x, verts[tri->vertIndex[1]].x,
                              verts[tri->vertIndex[2]].x, isx.t.sphere.center.z,
                              verts[tri->vertIndex[1]].z, verts[tri->vertIndex[2]].z) +
               -uvs[tri->vertIndex[1]].v *
                   det3x3top1(verts[tri->vertIndex[0]].x, isx.t.sphere.center.x,
                              verts[tri->vertIndex[2]].x, verts[tri->vertIndex[0]].z,
                              isx.t.sphere.center.z, verts[tri->vertIndex[2]].z) +
               -uvs[tri->vertIndex[2]].v *
                   det3x3top1(verts[tri->vertIndex[0]].x, verts[tri->vertIndex[1]].x,
                              isx.t.sphere.center.x, verts[tri->vertIndex[0]].z,
                              verts[tri->vertIndex[1]].z, isx.t.sphere.center.z)) /
              det;

    F32 valx = (-uvs[tri->vertIndex[0]].v *
                    det3x3top1(1.0f + isx.t.sphere.center.x, verts[tri->vertIndex[1]].x,
                               verts[tri->vertIndex[2]].x, isx.t.sphere.center.z,
                               verts[tri->vertIndex[1]].z, verts[tri->vertIndex[2]].z) +
                -uvs[tri->vertIndex[1]].v *
                    det3x3top1(verts[tri->vertIndex[0]].x, 1.0f + isx.t.sphere.center.x,
                               verts[tri->vertIndex[2]].x, verts[tri->vertIndex[0]].z,
                               isx.t.sphere.center.z, verts[tri->vertIndex[2]].z) +
                -uvs[tri->vertIndex[2]].v *
                    det3x3top1(verts[tri->vertIndex[0]].x, verts[tri->vertIndex[1]].x,
                               1.0f + isx.t.sphere.center.x, verts[tri->vertIndex[0]].z,
                               verts[tri->vertIndex[1]].z, isx.t.sphere.center.z)) /
               det;

    F32 valz = (-uvs[tri->vertIndex[0]].v *
                    det3x3top1(isx.t.sphere.center.x, verts[tri->vertIndex[1]].x,
                               verts[tri->vertIndex[2]].x, 1.0f + isx.t.sphere.center.z,
                               verts[tri->vertIndex[1]].z, verts[tri->vertIndex[2]].z) +
                -uvs[tri->vertIndex[1]].v *
                    det3x3top1(verts[tri->vertIndex[0]].x, isx.t.sphere.center.x,
                               verts[tri->vertIndex[2]].x, verts[tri->vertIndex[0]].z,
                               1.0f + isx.t.sphere.center.z, verts[tri->vertIndex[2]].z) +
                -uvs[tri->vertIndex[2]].v *
                    det3x3top1(verts[tri->vertIndex[0]].x, verts[tri->vertIndex[1]].x,
                               isx.t.sphere.center.x, verts[tri->vertIndex[0]].z,
                               verts[tri->vertIndex[1]].z, 1.0f + isx.t.sphere.center.z)) /
               det;

    valx -= val;
    valz -= val;

    F32 len = xsqrt(valx * valx + valz * valz);

    if (len < 1e-5f)
    {
        return;
    }

    globals.player.SlideTrackDir.x = valx / len;
    globals.player.SlideTrackDir.z = valz / len;
    globals.player.SlideTrackSliding |= 1;
    globals.player.SlideNotGroundedSinceSlide = 1;
}

static void zEntPlayerTSlideUpdate(xEnt* ent, xScene* sc, F32 dt)
{
    globals.player.SlideTrackSliding = (globals.player.SlideTrackSliding & 1) << 1;
    globals.player.SlideTrackLand = 0.0f;

    if (globals.player.SlideTrackCount)
    {
        SlideTrackUpdate(ent);
    }

    if ((globals.player.SlideTrackSliding & 1) && !(globals.player.SlideTrackSliding & 2))
    {
        globals.player.SlideTrackVel.x = tslide_lastrealvel.x;
        globals.player.SlideTrackVel.y = 0.0f;
        globals.player.SlideTrackVel.z = tslide_lastrealvel.z;

        ent->frame->vel.x = 0.0f;
        ent->frame->vel.y = 0.0f;
        ent->frame->vel.z = 0.0f;

        tslide_inair_tmr = 0.0f;
        tslide_dbl_tmr = 0.0f;
        tslide_ground = 1;
    }
    else if (!(globals.player.SlideTrackSliding & 1) && (globals.player.SlideTrackSliding & 2))
    {
        if (globals.player.JumpState)
        {
            tslide_inair_tmr = dt;
            tslide_dbl_tmr = 0.0f;
        }

        zEntPlayer_SNDStop(ePlayerSnd_SlideLoop);
    }

    if (!(globals.player.SlideTrackSliding & 1) && !globals.player.JumpState)
    {
        tslide_ground = 0;
    }

    if (globals.player.JumpState)
    {
        if (tslide_inair_tmr)
        {
            if (tslide_inair_tmr > globals.player.g.SlideAirHoldTime)
            {
                static F32 tmax =
                    globals.player.g.SlideAirHoldTime + globals.player.g.SlideAirSlowTime;

                if (tslide_inair_tmr >= tmax)
                {
                    ent->frame->vel.x = 0.0f;
                    ent->frame->vel.z = 0.0f;
                }
                else
                {
                    F32 decay = ((tmax - tslide_inair_tmr) - dt) / (tmax - tslide_inair_tmr);
                    ent->frame->vel.x *= decay;
                    ent->frame->vel.z *= decay;
                }
            }

            tslide_inair_tmr += dt;
        }

        if (tslide_dbl_tmr)
        {
            if (tslide_dbl_tmr > globals.player.g.SlideAirDblHoldTime)
            {
                static F32 tmax =
                    globals.player.g.SlideAirDblHoldTime + globals.player.g.SlideAirDblSlowTime;

                if (tslide_dbl_tmr >= tmax)
                {
                    ent->frame->vel.x = 0.0f;
                    ent->frame->vel.z = 0.0f;
                }
                else
                {
                    F32 decay = ((tmax - tslide_dbl_tmr) - dt) / (tmax - tslide_dbl_tmr);
                    ent->frame->vel.x *= decay;
                    ent->frame->vel.z *= decay;
                }
            }

            tslide_dbl_tmr += dt;
        }
    }

    if (!globals.player.JumpState)
    {
        tslide_inair_tmr = 0.0f;
    }
}

static U32 cchkButtbounce;
static S32 cchkSquish;

static void zEntPlayerFloorUpdate(xEnt* ent, xScene* sc, F32 dt)
{
    static xVec3 lorigins[4] = { { -0.3f, 0.0f, 0.3f },
                                 { 0.3f, 0.0f, 0.3f },
                                 { 0.3f, 0.0f, -0.3f },
                                 { -0.3f, 0.0f, -0.3f } };

    xRay3 ray;
    xCollis rcoll;
    xMat3x3 rotm;
    xVec3 lastvel;
    xVec3 e0;
    xVec3 e1;
    xVec3 n0;
    xVec3 n1;
    xVec3 axis;
    xVec3 downhill;
    xVec3 slidedir;
    xEnt* oent;
    S32 i;
    F32 total;
    S32 count;

    xCollis* coll = &ent->collis->colls[0];
    xSurface* surf = zSurfaceGetSurface(coll);
    globals.player.floor_surf = surf;

    surfSlickTimer -= dt;
    if (surfSlickTimer < 0.0f)
    {
        surfSlickTimer = 0.0f;
    }

    surfSticky = 0;
    surfSlickRatio = 0.0f;

    if ((globals.player.JumpState == 0 || globals.player.JumpState == 1) && surf && !surf->state)
    {
        surfSticky = zSurfaceGetSticky(surf);
    }

    if (surf && !surf->state)
    {
        surfFriction = zSurfaceGetFriction(surf);
        surfSlickness = zSurfaceGetSlickness(surf);
        surfDamping = zSurfaceGetDamping(surf, minVelmag);
        surfSlipTimer = 0.00001f;
    }
    else
    {
        surfFriction = 1.0f;
        surfDamping = minVelmag;

        if (globals.player.JumpState != 0)
        {
            surfSlipTimer = 0.0f;
        }

        if (surfSlipTimer)
        {
            surfSlipTimer += dt;
            if (surfSlipTimer >= 0.75f)
            {
                surfSlipTimer = 0.0f;
            }
        }

        if (!surfSlipTimer)
        {
            surfSlickness = 1;
        }
    }

    if (globals.player.ForceSlipperyTimer > 0.0f)
    {
        if ((globals.player.JumpState == 0 || globals.player.JumpState == 1) &&
            globals.player.WallJumpState == k_WALLJUMP_NOT)
        {
            surfFriction = globals.player.ForceSlipperyFriction;
            surfSlickness = (S32)(1.0f / surfFriction);
            surfDamping = xpow(minVelmag, globals.player.ForceSlipperyFriction);
            surfSlipTimer = 0.00001f;
        }

        if (globals.player.JumpState != 1)
        {
            globals.player.ForceSlipperyTimer -= dt;
        }
    }

    if (surfSlickness != 1)
    {
        if (surfSlickness > 1)
        {
            surfSlickRatio = 150.0f / surfSlickness;
        }
        else
        {
            surfSlickRatio = 1.0f;
        }

        surfSlickTimer = 0.1f;
    }
    else
    {
        surfSlickRatio = 0.0f;
    }

    xEntFrame* frame = ent->frame;

    if (lastSlickness != surfSlickness)
    {
        if (surfSlickness == 1)
        {
            if (lastFloorEnt)
            {
                globals.player.SlipFadeTimer = 0.8f;
            }
            else
            {
                frame->vel.x = 0.0f;
                frame->vel.z = 0.0f;
            }
        }
        else
        {
            xVec3SMul(&lastvel, &lastDeltaPos, 1.0f / dt);
            frame->vel.x = lastvel.x;
            frame->vel.z = lastvel.z;
        }
    }

    if (surfSlickRatio)
    {
        F32 speed2 = frame->vel.x * frame->vel.x + frame->vel.z * frame->vel.z;

        if (surfMaxSpeed)
        {
            if (speed2 > surfMaxSpeed * surfMaxSpeed)
            {
                F32 scale = surfMaxSpeed / xsqrt(speed2);
                frame->vel.x *= scale;
                frame->vel.z *= scale;
            }
        }
        else
        {
            F32 decel;

            if (xStricmp(ent->model->Anim->Single[0].State->Name, "Skid") != 0)
            {
                decel = surfDecelIdle * dt;
            }
            else
            {
                decel = surfDecelSkid * dt;
            }

            F32 drop = (1.0f - (4.0f / 3.0f) * surfSlipTimer) * decel +
                       (4.0f / 3.0f) * surfSlipTimer * 20.0f * decel;
            F32 speed = xsqrt(speed2);

            if (speed <= drop)
            {
                frame->vel.x = 0.0f;
                frame->vel.z = 0.0f;
            }
            else
            {
                F32 scale = (speed - drop) / speed;
                frame->vel.x *= scale;
                frame->vel.z *= scale;
            }
        }
    }
    else if (globals.player.SlipFadeTimer > 0.0f)
    {
        globals.player.SlipFadeTimer -= dt;
        if (globals.player.SlipFadeTimer < 0.0f)
        {
            globals.player.SlipFadeTimer = 0.0f;
        }
    }

    xVec3* fnorm = &coll->norm;

    lastSlickness = surfSlickness;

    if (coll->flags & 0x1)
    {
        xVec3Copy(&globals.player.floor_norm, fnorm);
    }
    else
    {
        xVec3Copy(&globals.player.floor_norm, &g_Y3);
        xVec3Copy(&ray.origin, xBoundCenter(&ent->bound));
        ray.dir.x = 0.0f;
        ray.dir.y = -1.0f;
        ray.dir.z = 0.0f;
        ray.min_t = 0.0f;
        ray.max_t = 10.0f;
        ray.flags = 0x800;
        rcoll.flags = 0;
        rcoll.optr = NULL;
        rcoll.mptr = NULL;
        rcoll.dist = 1e38f;
        xRayHitsScene(globals.sceneCur, &ray, &rcoll);
        coll->dist = rcoll.dist;
    }

    ray.dir.x = 0.0f;
    ray.dir.y = -1.0f;
    ray.dir.z = 0.0f;
    ray.min_t = -0.7f;
    ray.max_t = 1.0f;
    ray.flags = 0xc00;

    total = 0.0f;
    count = 0;

    oent = (xEnt*)coll->optr;
    if (oent)
    {
        for (i = 0; i < 4; i++)
        {
            xMat4x3Toworld(&ray.origin, &ent->frame->mat, &lorigins[i]);
            rcoll.flags = 0;
            rcoll.optr = NULL;
            rcoll.mptr = NULL;
            rcoll.dist = 1e38f;

            if (oent->collLev == 5)
            {
                iRayHitsModel(&ray, oent->model, &rcoll);
            }
            else
            {
                xRayHitsBound(&ray, &oent->bound, &rcoll);
            }

            floor_supp[i] = ray.origin;
            floor_dist[i] = rcoll.dist;

            if (rcoll.flags & 0x1)
            {
                total += rcoll.dist;
                if (!zGooIs(oent))
                {
                    count++;
                }
            }
        }
    }
    else
    {
        for (i = 0; i < 4; i++)
        {
            xMat4x3Toworld(&ray.origin, &ent->frame->mat, &lorigins[i]);
            rcoll.flags = 0;
            rcoll.optr = NULL;
            rcoll.mptr = NULL;
            rcoll.dist = 1e38f;
            iRayHitsEnv(&ray, globals.sceneCur->env, &rcoll);

            floor_supp[i] = ray.origin;
            floor_dist[i] = rcoll.dist;

            if (rcoll.flags & 0x1)
            {
                total += rcoll.dist;
                if (!(rcoll.flags & 0x20000))
                {
                    count++;
                }
            }
        }
    }

    if (count == 4 && globals.player.JumpState == 0)
    {
        floor_safe_vec = *(xVec3*)&ent->model->Mat->pos;
        floor_safe_tmr = 0.0f;
        sGooKnockedToSafety = 0;
        sGooKnockedTimer = 0.0f;
    }
    else
    {
        floor_safe_tmr += dt;
    }

    total *= 0.25f;
    F32 lo = total - 0.17320508f;
    F32 hi = 0.17320508f + total;

    for (i = 0; i < 4; i++)
    {
        floor_supp[i].y -= MAX(lo, MIN(floor_dist[i], hi));
    }

    xVec3Sub(&e0, &floor_supp[1], &floor_supp[0]);
    xVec3Sub(&e1, &floor_supp[2], &floor_supp[0]);
    xVec3Cross(&n0, &e0, &e1);
    xVec3Sub(&e0, &floor_supp[2], &floor_supp[0]);
    xVec3Sub(&e1, &floor_supp[3], &floor_supp[0]);
    xVec3Cross(&n1, &e0, &e1);
    xVec3Add(&globals.player.floor_norm, &n0, &n1);
    xVec3Normalize(&globals.player.floor_norm, &globals.player.floor_norm);

    if (globals.player.Slide == 1 || globals.player.Slide == 2)
    {
        frame = ent->frame;

        if (((coll->flags & 0x1) &&
             (fnorm->y > icos(surfSlideStop) ||
              (surf && (surf->state || !zSurfaceGetSlide(surf))))) ||
            frame->vel.y > 0.0f || surfSlickRatio)
        {
            if (surfSlickRatio)
            {
                globals.player.SlideTimer = 0.0f;
            }

            globals.player.Slide = 0;
        }
    }
    else if ((coll->flags & 0x1) && fnorm->y < icos(zSurfaceGetSlideStartAngle(surf)) && surf &&
             !surf->state && zSurfaceGetSlide(surf))
    {
        axis.x = -fnorm->z;
        axis.y = 0.0f;
        axis.z = fnorm->x;
        xVec3Normalize(&axis, &axis);
        xVec3Cross(&downhill, fnorm, &axis);
        xVec3Normalize(&downhill, &downhill);
        xMat3x3Rot(&rotm, &axis, xacos(fnorm->y));
        xMat3x3RMulVec(&slidedir, &rotm, &downhill);

        F32 dot = slidedir.x * req_motion.x + slidedir.z * req_motion.z;

        if (dot < 0.0f &&
            (globals.player.LastJumpState == 0 || globals.player.LastJumpState == 1))
        {
            globals.player.Slide = 3;

            F32 scale = dot / xsqrt(slidedir.x * slidedir.x + slidedir.z * slidedir.z);
            F32 dx = slidedir.x * scale;
            F32 dz = slidedir.z * scale;
            ent->frame->mat.pos.x -= dx;
            ent->frame->mat.pos.z -= dz;

            if (update_motion.y > 0.0f)
            {
                ent->frame->mat.pos.y = ent->frame->oldmat.pos.y;
            }
        }
        else
        {
            globals.player.Slide = 1;
            surfSlideStart = zSurfaceGetSlideStartAngle(surf);
            surfSlideStop = zSurfaceGetSlideStopAngle(surf);
            ent->pflags |= 0x80;

            F32 boost = 0.5f / update_dt;
            ent->frame->vel.x += req_motion.x * boost;
            ent->frame->vel.z += req_motion.z * boost;
            ent->frame->vel.y = -4.0f;
        }

        globals.player.SlideTimer = 0.0f;
    }
    else
    {
        globals.player.Slide = 0;
    }

    if (globals.player.Slide == 1)
    {
        globals.player.SlideTimer += dt;
    }
    else if (globals.player.Slide == 0)
    {
        if (globals.player.SlideTimer > 0.0f)
        {
            globals.player.SlideTimer -= dt;
            if (globals.player.SlideTimer < 0.0f)
            {
                globals.player.SlideTimer = 0.0f;
            }
        }
    }
}

static U32 PlayerDepenQuery(xEnt* ent, xEnt* other, xScene* scene, F32 dt, xCollis* coll)
{
    if (globals.player.DamageTimer > 0.0f)
    {
        xSurface* surf = zSurfaceGetSurface(coll);
        if (surf && !surf->state)
        {
            zSurfaceProps* prop = (zSurfaceProps*)surf->moprops;
            if (prop && prop->asset)
            {
                switch (prop->asset->game_damage_type)
                {
                case 0:
                    return 1;
                case 1:
                case 2:
                case 3:
                case 4:
                case 5:
                case 6:
                    if (prop->asset->game_damage_flags & 1)
                    {
                        return 0;
                    }
                    break;
                }
            }
        }
    }

    return 1;
}

static void PlayerBoundUpdate(xEnt* ent, xVec3* pos)
{
    xEntDefaultBoundUpdate(ent, pos);
    ent->bound.sph.r = 0.5f;
    ent->bound.sph.center.y -= ent->bound.sph.r;
    xVec3AddScaled(&ent->bound.sph.center, &globals.player.RootUp, ent->bound.sph.r - 0.2f);
}

static void zEntPlayerSurfDamageUpdate(xEnt* ent, xScene* sc, F32 dt)
{
    xCollis* coll;
    xCollis* cend;
    xSurface* surf;
    S32 damaged;
    F32 dx;
    F32 dz;
    F32 mag;
    F32 kvel;

    if (globals.player.DamageTimer > 0.0f)
    {
        return;
    }

    coll = ent->collis->colls;
    cend = coll + ent->collis->idx;

    for (; coll < cend; coll++)
    {
        if ((coll->flags & 0x1) && (surf = zSurfaceGetSurface(coll)) != NULL && !surf->state)
        {
            zSurfaceProps* prop = (zSurfaceProps*)surf->moprops;
            damaged = 0;

            if (prop && prop->asset)
            {
                switch (prop->asset->game_damage_type)
                {
                case 0:
                    break;
                case 4:
                case 6:
                    if (zEntPlayer_Damage(surf, 1, NULL))
                    {
                        if (globals.player.Health)
                        {
                            globals.player.DamageTimer = prop->asset->damage_timer
                                                             ? prop->asset->damage_timer
                                                             : globals.player.g.DamageTimeSurface;
                        }

                        if (globals.player.Health)
                        {
                            if (tslide_ground || coll == ent->collis->colls ||
                                prop->asset->damage_bounce)
                            {
                                zEntPlayer_DamageKnockIntoAir(prop->asset->damage_bounce
                                                                  ? prop->asset->damage_bounce
                                                                  : globals.player.g.DamageSurfKnock);
                            }
                            else
                            {
                                dx = -coll->tohit.x;
                                dz = -coll->tohit.z;
                                mag = xsqrt(dx * dx + dz * dz);

                                if (mag > 0.01f)
                                {
                                    kvel = sPlayerNPC_KnockBackVel / mag;
                                    globals.player.ent.frame->vel.x = kvel * dx;
                                    globals.player.ent.frame->vel.y = 0.0f;
                                    globals.player.ent.frame->vel.z = kvel * dz;
                                    globals.player.KnockBackTimer = sPlayerNPC_KnockBackTime;
                                    globals.player.KnockIntoAirTimer = 0.0f;
                                }
                            }
                        }
                    }

                    damaged = 1;
                    break;
                case 1:
                {
                    xEnt* cent = (xEnt*)coll->optr;

                    if (cent && cent->baseType == eBaseTypeEGenerator &&
                        !(((zEGenerator*)cent)->flags & 0x1))
                    {
                        break;
                    }
                }
                    // fall through
                case 2:
                case 3:
                case 5:
                    if (globals.player.Health)
                    {
                        globals.player.DamageTimer = 0.0f;
                        zEntPlayer_Damage(surf, globals.player.Health, NULL);
                        damaged = 1;
                    }
                    break;
                case 7:
                    if (globals.player.Health)
                    {
                        globals.player.DamageTimer = 0.0f;
                        zEntPlayer_Damage(surf, 1);
                        damaged = 1;
                    }
                    break;
                }

                if (damaged)
                {
                    return;
                }
            }
        }
    }
}

// Equivalent; scheduling.
static void PlayerMountHackUpdate(F32 delta)
{
    mount_tmr = mount_tmr + delta;
    if ((mount_tmr > 0.1f) && (mount_object != NULL))
    {
        zEntEvent(mount_object, mount_type);
        mount_object = NULL;
        mount_type = 0;
    }
}

static void PlayerMountHackTakeAction(xEnt* ent, U32 type)
{
    if (mount_tmr > 0.1f)
    {
        zEntEvent(ent, type);
    }
    else
    {
        mount_object = ent;
        mount_type = type;
    }
    mount_tmr = 0.0f;
}

static void zEntPlayerDriveUpdate(xEnt* ent, xScene* sc, F32 dt)
{
    xCollis* coll;
    zPlatform* plat;
    xEntDrive* drv;
    zJumpParam jump;
    F32 jmph;
    U32 superbounce;
    xVec3* jmpdir;
    xAnimState* spring_state;

    if (globals.player.carry.patLauncher)
    {
        plat = (zPlatform*)globals.player.carry.patLauncher;
        globals.player.carry.patLauncher = NULL;

        if (globals.player.Health)
        {
            goto do_bounce;
        }
    }

    PlayerMountHackUpdate(dt);

    drv = &globals.player.drv;

    if (drv->odriver)
    {
        if (drv->odriver->baseType != eBaseTypePlatform)
        {
            if (!drv->os)
            {
                drv->otm = 0.0f;
                drv->odriver = NULL;
            }
        }
        else
        {
            zPlatform* oplat = (zPlatform*)drv->odriver;

            if (!drv->os)
            {
                drv->otm = 0.0f;
                drv->odriver = NULL;

                if (oplat->subType == ZPLATFORM_SUBTYPE_BREAKAWAY)
                {
                    zPlatform_Dismount(oplat);
                }

                if (oplat->passet->flags & 0x1)
                {
                    zPlatform_Shake(oplat, dt, 0.2f, 12.0f * PI);
                }
            }
        }
    }

    coll = ent->collis->colls;

    if (coll->flags & 0x1)
    {
        plat = (zPlatform*)coll->optr;
    }
    else
    {
        plat = NULL;
    }

    if (!plat)
    {
        if (!drv->driver)
        {
            return;
        }

        PlayerMountHackTakeAction(drv->driver, 0x21);

        if (globals.player.JumpState != 0 && globals.player.JumpState != 1)
        {
            xEntDriveDismount(drv, 0.0001f);
            return;
        }

        xEntDriveDismount(drv, 0.3f);
        return;
    }

    xSurface* surf;
    surf = zSurfaceGetSurface(coll);

    if (surf && !surf->state && zSurfaceGetDamageType(surf))
    {
        return;
    }

    if (plat != (zPlatform*)drv->driver)
    {
        if (drv->driver)
        {
            PlayerMountHackTakeAction(drv->driver, 0x21);
            xEntDriveDismount(drv, 0.3f);
        }

        if ((plat->baseType == eBaseTypeNPC &&
             (((xNPCBasic*)(xEnt*)plat)->SelfType() & 0xffffff00) == 'NTT\0') ||
            plat->baseType == eBaseTypeBoulder)
        {
            xEntDriveMount(drv, (xEnt*)plat, 0.15f, coll);
            return;
        }

        if (plat->baseType != eBaseTypePlatform && (plat->moreFlags & 0x2) &&
            (plat->moreFlags & 0x20))
        {
            xEntDriveMount(drv, (xEnt*)plat, 0.15f, coll);
            return;
        }

        if (plat->baseType == eBaseTypePendulum)
        {
            xEntDriveMount(drv, (xEnt*)plat, 0.3f, coll);
            PlayerMountHackTakeAction((xEnt*)plat, 0x20);
            return;
        }

        if (plat->baseType == eBaseTypeStatic && plat->driver)
        {
            xEntDriveMount(drv, (xEnt*)plat, -1.0f, coll);
            return;
        }

        if (plat->baseType != eBaseTypePlatform)
        {
            return;
        }

        if (!(plat->plat_flags & 0x1))
        {
            return;
        }

        if (plat->subType != ZPLATFORM_SUBTYPE_SPRINGBOARD)
        {
            xEntDriveMount(drv, (xEnt*)plat, -1.0f, coll);
            PlayerMountHackTakeAction((xEnt*)plat, 0x20);

            if (plat->subType == ZPLATFORM_SUBTYPE_BREAKAWAY)
            {
                zPlatform_Mount(plat);
            }

            if (plat->passet->flags & 0x1)
            {
                zPlatform_Shake(plat, dt, 0.2f, 12.0f * PI);
            }
        }
    }
    else
    {
        plat = (zPlatform*)drv->driver;
    }

    if (plat->subType != ZPLATFORM_SUBTYPE_SPRINGBOARD)
    {
        return;
    }

    if (!globals.player.Health)
    {
        return;
    }

    if ((plat->passet->flags & 0x2) &&
        (globals.player.JumpState == 0 || globals.player.JumpState == 1))
    {
        return;
    }

    if (plat->plat_flags & 0x2)
    {
        return;
    }

do_bounce:

    if (plat->tmr <= 0.0f)
    {
        plat->ctr = 0;
    }
    else if (plat->ctr < 2)
    {
        plat->ctr++;
    }

    superbounce = 0;

    if (strcmp(ent->model->Anim->Single->State->Name, "BbounceAttack01") == 0 &&
        plat->passet->sb.jmpbounce)
    {
        jmph = plat->passet->sb.jmpbounce;
        superbounce = 1;
    }
    else
    {
        F32* jmphs = plat->passet->sb.jmph;
        jmph = MAX(MAX(jmphs[0], jmphs[1]), jmphs[2]);
    }

    jmpdir = &plat->passet->sb.jmpdir;
    jump.PeakHeight = jmph;
    jump.TimeHold = 0.0f;
    jump.TimeGravChange = 0.3f;
    CalcJumpImpulse(&jump, NULL);

    startBounce = ent->frame->mat.pos.y;
    zEntPlayerJumpStart(ent, &jump);

    if (plat->asset->modelInfoID == xStrHash("tree_bounce_bind_top"))
    {
        xSndPlay3D(xStrHash("springboard_big"), 0.77f, 0.0f, 0x80, 0,
                   (const xVec3*)&plat->model->Mat->pos, 0.0f, (sound_category)0, 0.0f);
    }
    else
    {
        xSndPlay3D(xStrHash("springboard"), 0.77f, 0.0f, 0x80, 0, (const xVec3*)&plat->model->Mat->pos, 0.0f,
                   (sound_category)0, 0.0f);
    }

    xVec3SMul(&ent->frame->vel, jmpdir, ent->frame->vel.y);

    globals.player.Jump_CanDouble = 0;
    globals.player.Jump_CanFloat = 1;
    globals.player.Jump_Springboard = plat;
    globals.player.Jump_SpringboardStart = 1;

    if (plat->passet->sb.springflags & 0x4)
    {
        zEntPlayerControlOff((zControlOwner)0x4000);
    }
    else
    {
        zEntPlayerControlOn((zControlOwner)0x4000);
    }

    xAnimPlay* aplay = plat->model->Anim;

    if (aplay && (spring_state = xAnimTableGetState(aplay->Table, "Spring")) != NULL)
    {
        xAnimPlaySetState(aplay->Single, spring_state, 0.0f);
    }

    globals.player.Bounced = 1;
    zCameraSetBbounce(1);

    if ((plat->passet->sb.springflags & 0x1) &&
        (!(plat->passet->sb.springflags & 0x2) || superbounce))
    {
        zCameraSetHighbounce(1);
    }
    else
    {
        if (jmpdir->x * jmpdir->x + jmpdir->z * jmpdir->z > jmpdir->y * jmpdir->y)
        {
            zCameraSetLongbounce(1);
        }
        else
        {
            zCameraSetHighbounce(0);
        }
    }

    tslide_inair_tmr = 0.0f;
    tslide_dbl_tmr = 0.0f;
    tslide_ground = 0;
    globals.player.SlideTrackDecay = 0.0f;
}

void zEntPlayerExit(xEnt* ent)
{
    bungee_state::destroy();
}

static void PlayerHitAnimInit(xModelInstance* model, xAnimTransition* tran, U32* index)
{
    *index = 0;
    xAnimState* state = model->Anim->Table->StateList;
    while ((state != NULL) && (*index < 8))
    {
        if (strncmp(state->Name, "Hit0", 4) == 0)
        {
            tran[*index].Dest = state;
            tran[*index].Callback = NULL;
            tran[*index].SrcTime = 0.0;
            tran[*index].DestTime = 0.0;
            tran[*index].BlendRecip = 5.0f;
            tran[*index].BlendOffset = NULL;
            (*index)++;
        }
        state = state->Next;
    }
}

static U32 sTrackHash[10][2] = { { 0x5904a48f, 0x5904a498 }, { 0x5904a512, 0x5904a51b },
                                 { 0x5904a595, 0x5904a59e }, { 0x5904a618, 0x5904a621 },
                                 { 0x5904a69b, 0x5904a6a4 }, { 0x5904a71e, 0x5904a727 },
                                 { 0x5904a7a1, 0x5904a7aa }, { 0x5904a824, 0x5904a82d },
                                 { 0x5904a8a7, 0x5904a8b0 }, { 0x5904a92a, 0x5904a933 } };

S32 zEntPlayer_ObjIDIsTrack(U32 objID)
{
    S32 i;

    if (objID == 0xcd8dd617)
    {
        return 1;
    }

    if (objID >= 0x2f948df5 && objID <= 0x2f948dfe)
    {
        return 1;
    }

    if (objID >= 0x5904a48f && objID <= 0x5904a933)
    {
        for (i = 0; i < 10; i++)
        {
            if (objID >= sTrackHash[i][0] && objID <= sTrackHash[i][1])
            {
                return 1;
            }
        }
    }

    return 0;
}

void zEntPlayerPreReset()
{
    globals.player.ControlOff = 0;
    if (!oob_state::IsPlayerInControl())
    {
        zEntPlayerControlOff(CONTROL_OWNER_OOB);
        globals.player.ControlOffTimer = 1e38;
    }
}

static void PlayerLedgeInit(zLedgeGrabParams* ledge, xModelInstance* model);

void zEntPlayerReset(xEnt* ent)
{
    xVec3Init(&globals.player.PredictCurrDir, 0.0f, 1.0f, 0.0f);
    globals.player.PredictCurrVel = 0.0f;

    if (globals.player.bubblebowl)
    {
        xEntBoulder_Kill(globals.player.bubblebowl);
    }

    if (boulderVehicle)
    {
        boulderVehicle->flags |= 0x40;
        boulderVehicle->asset->flags |= 0x40;
        xEntBoulder_Kill(boulderVehicle);

        if (boulderVehicle->model->Anim)
        {
            xAnimPlaySetState(boulderVehicle->model->Anim->Single,
                              boulderVehicle->model->Anim->Table->StateList, 0.0f);
        }
    }

    ent->update = zEntPlayer_Update;
    ent->move = zEntPlayer_Move;
    ent->render = (xEntRenderCallback)zEntPlayer_Render;

    if (zGameModeGet() == eGameMode_Game &&
        (zGameStateGet() == eGameState_SceneSwitch || zGameStateGet() == eGameState_Play ||
         zGameStateGet() == eGameState_GameStats))
    {
    }
    else if (zGameModeGet() == eGameMode_Game && zGameStateGet() == eGameState_LoseChance)
    {
        HealthReset();
    }
    else
    {
        InvReset();
    }

    globals.player.ControlOnEvent = 0;
    globals.player.Speed = 0;
    globals.player.SpeedMult = 1.0f;
    globals.player.Sneak = 0;
    globals.player.Teeter = 0;
    globals.player.Slide = 0;
    globals.player.SlideTimer = 0.0f;
    globals.player.SlideNotGroundedSinceSlide = 0;
    globals.player.Stepping = 0;
    globals.player.JumpState = 0;
    globals.player.LastJumpState = 0;
    globals.player.IdleMinorTimer = 0.0f;
    globals.player.IdleMajorTimer = 0.0f;
    globals.player.IdleSitTimer = 0.0f;
    globals.player.cheat_mode = 0;
    globals.player.FireTarget = NULL;
    globals.player.LeanLerp = 1.0f;
    globals.player.ScareTimer = 0.0f;
    globals.player.DamageTimer = 0.0f;
    globals.player.SundaeTimer = -1.0f;
    globals.player.ControlOffTimer = 0.0f;
    globals.player.HelmetTimer = 0.0f;
    globals.player.Bounced = 0;
    globals.player.FallDeathTimer = 0.0f;
    globals.player.Diggable = NULL;
    globals.player.Visible = 1;
    globals.player.AutoMoveSpeed = 0;
    globals.player.Face_ScareTimer = 0.0f;
    globals.player.Face_EventTimer = 0.0f;
    globals.player.Face_PantTimer = 0.0f;
    globals.player.SpecialReceived = 0;
    globals.player.Jump_CanFloat = 1;
    globals.player.Jump_CanDouble = 1;
    globals.player.Jump_Springboard = NULL;
    globals.player.Jump_SpringboardStart = 0;
    globals.player.CanJump = 1;
    globals.player.CanBubbleSpin = 1;
    globals.player.CanBubbleBounce = 1;
    globals.player.CanBubbleBash = 1;
    globals.player.IsJumping = 0;
    globals.player.IsDJumping = 0;
    globals.player.WasDJumping = 0;
    globals.player.IsBubbleSpinning = 0;
    globals.player.IsBubbleBouncing = 0;
    globals.player.IsBubbleBashing = 0;
    globals.player.IsBubbleBowling = 0;
    globals.player.WallJumpState = k_WALLJUMP_NOT;
    globals.player.ForceSlipperyTimer = -1.0f;
    globals.player.carry.grabbed = NULL;
    globals.player.carry.grabTarget = 0;
    globals.player.carry.throwTarget = NULL;
    globals.player.carry.patLauncher = NULL;
    globals.player.lassoInfo.lasso.flags = 0;
    globals.player.lassoInfo.target = NULL;
    globals.player.lassoInfo.destroy = 0;

    if (globals.player.lassoInfo.swingTarget)
    {
        globals.player.lassoInfo.swingTarget = NULL;
        zCameraDisableLassoCam();
    }

    ent->collis->depenq = PlayerDepenQuery;
    ent->bupdate = PlayerBoundUpdate;
    surfSlickRatio = 0.0f;
    surfSlickTimer = 0.0f;
    globals.player.floor_surf = NULL;
    globals.player.floor_norm = g_Y3;
    ent->collis->chk = 0x2e;

    xEntReset(ent);
    xVec3Copy(&ent->frame->rot.axis, &g_Y3);
    ent->frame->rot.angle = ent->asset->ang.x;

    xEntDriveInit(&globals.player.drv, ent);
    globals.player.drv.flags = 1;

    xCameraSetTargetMatrix(&globals.camera, xEntGetFrame(&globals.player.ent));
    xCameraSetTargetOMatrix(&globals.camera, &sCameraLastMat);
    zCameraReset(&globals.camera);
    xMat4x3Copy(&gPlayerAbsMat, &globals.camera.mat);

    globals.player.Jump_CurrGravity = globals.player.g.Gravity;

    for (xAnimState* state = globals.player.ent.model->Anim->Table->StateList; state;
         state = state->Next)
    {
        if ((state->UserFlags & 0x40) &&
            !(state->Data->NumAnims[0] == 3 && state->Data->NumAnims[1] == 1))
        {
            state->UserFlags &= ~0x40;
        }
    }

    xModelInstance* model = globals.player.ent.model;

    for (U32 i = 0; model; model = model->Next, i++)
    {
        if (i >= 2)
        {
            model->Flags &= 0xfffc;
        }
    }

    globals.player.RootUp.x = 0.0f;
    globals.player.RootUp.y = 1.0f;
    globals.player.RootUp.z = 0.0f;
    globals.player.RootUpTarget.x = 0.0f;
    globals.player.RootUpTarget.y = 1.0f;
    globals.player.RootUpTarget.z = 0.0f;
    globals.player.HangElapsed = 0.0f;
    globals.player.HangEnt = NULL;
    globals.player.HangEntLast = NULL;
    globals.player.KnockBackTimer = 0.0f;
    globals.player.KnockIntoAirTimer = 0.0f;
    sGooKnockedToSafety = 0;
    sGooKnockedTimer = 0.0f;

    xEntBeginCollide(&globals.player.ent, globals.sceneCur, 0.0f);
    xEntSetNostepNormAngle(surfSlideStart);

    curVelmag = 0.0f;
    curVelangle = 0.0f;
    surfSlickness = 1;
    surfFriction = 1.0f;
    surfDamping = minVelmag;
    lastSlickness = 1;
    surfSticky = 0;
    sPlayerCollAdjust = 0.0f;
    tslide_maxspd_tmr = 0.0f;
    tslide_maxspd = globals.player.g.SlideVelMaxStart;
    tslide_lastrealvel.x = 0.0f;
    globals.player.SlideTrackDecay = 0.0f;
    tslide_lastrealvel.y = 0.0f;
    tslide_lastrealvel.z = 0.0f;
    tslide_inair_tmr = 0.0f;
    tslide_ground = 0;
    sRingDelay = 0.0f;
    player_hit = 0;
    globals.player.SlideTrackSliding = 0;
    globals.player.SlideTrackVel.x = 0.0f;
    globals.player.SlideTrackVel.y = 0.0f;
    globals.player.SlideTrackVel.z = 0.0f;
    mount_type = 0;
    mount_object = NULL;
    mount_tmr = 0.0f;
    sHackStuckTimer = 0.0f;

    if (globals.player.model_spongebob)
    {
        xAnimPlaySetState(globals.player.model_spongebob->Anim->Single,
                          globals.player.model_spongebob->Anim->Table->StateList, 0.0f);
    }

    if (globals.player.model_patrick)
    {
        xAnimPlaySetState(globals.player.model_patrick->Anim->Single,
                          globals.player.model_patrick->Anim->Table->StateList, 0.0f);
    }

    if (globals.player.model_sandy)
    {
        xAnimPlaySetState(globals.player.model_sandy->Anim->Single,
                          globals.player.model_sandy->Anim->Table->StateList, 0.0f);
    }

    xAnimPlaySetState(ent->model->Anim->Single, ent->model->Anim->Table->StateList, 0.0f);

    if (globals.sceneCur->sceneID == 'MNU3')
    {
        globals.firstStartPressed = 1;
        globals.autoSaveFeature = 0;
        globals.pad0 = mPad;
        globals.currentActivePad = 0;
        zEntPlayerControlOff(CONTROL_OWNER_EVENT);
        globals.player.ControlOnEvent = 0;
    }

    iColor_tag fade_from = { 0, 0, 0, 255 };
    iColor_tag fade_to = { 0, 0, 0, 0 };

    xScrFxFade(&fade_from, &fade_to, 1.0f, NULL, 0);

    if (globals.player.model_spongebob)
    {
        PlayerLedgeInit(&globals.player.sb.ledge, globals.player.model_spongebob);
    }

    if (globals.player.model_patrick)
    {
        PlayerLedgeInit(&globals.player.patrick.ledge, globals.player.model_patrick);
        PlayerHitAnimInit(globals.player.model_patrick, patrickHitTran, &patrickHitMax);
    }

    if (globals.player.model_sandy)
    {
        PlayerLedgeInit(&globals.player.sandy.ledge, globals.player.model_sandy);
        PlayerHitAnimInit(globals.player.model_sandy, sandyHitTran, &sandyHitMax);
    }

    globals.player.SlideTrackCount = 0;

    for (U32 j = 0; j < globals.sceneCur->num_base; j++)
    {
        xBase* base = globals.sceneCur->base[j];

        if ((base->baseType == eBaseTypeStatic || base->baseType == eBaseTypePlatform) &&
            zEntPlayer_ObjIDIsTrack(base->id))
        {
            xEnt* track = (xEnt*)globals.sceneCur->base[j];

            globals.player.SlideTrackEnt[globals.player.SlideTrackCount++] = track;
            track->baseFlags &= 0xffe7;
            track->chkby = 0;
            track->penby = 0;
        }
    }

    cruise_bubble::reset();
    bungee_state::reset();

    in_goo_tmr = 0.0f;
    in_goo = 0;

    sReticleModel = (RpAtomic*)xSTFindAsset(xStrHash("target_reticle_hand"), NULL);
    xMat4x3Identity(&sReticleMat);
    gReticleTarget = NULL;
    sTimeToRetarget = 0.0f;

    void* lane = xSTFindAsset(xStrHash("bowling_lane"), NULL);
    sBowlingLaneRast = lane ? *(RwRaster**)lane : NULL;

    if (globals.sceneCur->sceneID == 'PG12')
    {
        zEntPlayer_setBoulderMode(1);
    }
    else
    {
        zEntPlayer_setBoulderMode(0);
    }

    stuck_timer = 0.0f;
    not_stuck_timer = 0.0f;

    zEntPlayerSpeakStop();
}

static xEnt* PlayerCollCheckOneEnt(xEnt* ent, xScene* sc, void* data);
static xEnt* PlayerCollCheckOneVillain(xEnt* ent, xScene* sc, void* data);
static void PlayerCollsSelectDepen(xEnt* ent, xScene* sc, F32 dt);
static U32 CollidePyramidBoxTop(xCollis* coll, xBox* box, F32 height, xSphere* sph);

static void PlayerCollisBuildFromDepen(xCollis* coll)
{
    F32 len = xVec3Length(&coll->depen);

    xVec3SMul(&coll->hdng, &coll->depen, -1.0f / len);
    coll->dist = 0.5f - len;
    xVec3SMul(&coll->tohit, &coll->hdng, coll->dist);
}

static xEnt* PlayerCollCheckOneEnt(xEnt* ent, xScene* sc, void* data)
{
    xent_entent = 1;

    xEnt* p = (xEnt*)data;
    xCollis* coll;
    U32 modl_coll = 0;

    if (p->collis->idx >= 15)
    {
        xent_entent = 0;
        return NULL;
    }

    if ((ent->chkby & p->collType) == 0)
    {
        xent_entent = 0;
        return ent;
    }

    if (ent->id == p->id)
    {
        xent_entent = 0;
        return ent;
    }

    coll = &p->collis->colls[p->collis->idx];

    if (ent->miscflags & 0x8)
    {
        if (p->frame->mat.pos.y + p->bound.sph.r > ent->bound.box.box.upper.y)
        {
            xSphere tmpsph;

            tmpsph.center.x = p->frame->mat.pos.x;
            tmpsph.center.y = p->frame->mat.pos.y + p->bound.sph.r;
            tmpsph.center.z = p->frame->mat.pos.z;
            tmpsph.r = p->bound.sph.r;

            CollidePyramidBoxTop(coll, &ent->bound.box.box, 1.0f, &tmpsph);
        }
        else
        {
            coll->flags = 0x1F00;
            xBoundHitsBound(&p->bound, &ent->bound, coll);
        }
    }
    else
    {
        if (ent->collLev == 5 && p->collType & (XENT_COLLTYPE_NPC | XENT_COLLTYPE_PLYR))
        {
            modl_coll = 1;
        }

        if (modl_coll)
        {
            coll->flags = 0;
        }
        else
        {
            coll->flags = 0x1F00;
        }

        xBoundHitsBound(&p->bound, &ent->bound, coll);
    }

    if (coll->flags & 0x1)
    {
        if (modl_coll)
        {
            coll->flags = 0x1F00;

            xModelInstance* m = (ent->collModel) ? ent->collModel : ent->model;

            if (m->Flags & 0x800)
            {
                coll->flags |= 0x2000;
            }

            U8 ncolls = 15 - p->collis->idx;
            U8 idx = iSphereHitsModel3(&p->bound.sph, m, coll, ncolls, 0.78539819f);

            for (U8 i = 0; i < idx; i++)
            {
                coll[i].optr = ent;
                coll[i].mptr = ent->model;

                p->collis->idx++;
            }

            xent_entent = 0;
            return ent;
        }
        else
        {
            coll->oid = 0;
            coll->optr = ent;
            coll->mptr = ent->model;

            p->collis->idx++;

            if (coll->flags & 0x10)
            {
                xVec3Sub(&coll->tohit, xBoundCenter(&ent->bound), xEntGetCenter(p));
                xVec3SMul(&coll->depen, &coll->tohit, -0.25f * (1.0f / xVec3Length(&coll->tohit)));
                coll->depen.y = 0.0f;

                PlayerCollisBuildFromDepen(coll);
            }

            if (ent->pflags & 0x20 && ent->bound.type == XBOUND_TYPE_SPHERE &&
                p->bound.type == XBOUND_TYPE_SPHERE && coll->hdng.y < -0.866025f)
            {
                F32 rsum = p->bound.sph.r + ent->bound.sph.r;
                F32 dx = p->bound.sph.center.x - ent->bound.sph.center.x;
                F32 dy = p->bound.sph.center.y - ent->bound.sph.center.y;
                F32 dz = p->bound.sph.center.z - ent->bound.sph.center.z;

                F32 hsqr = SQR(rsum) - (SQR(dx) + SQR(dz));

                if (hsqr >= 0.0f)
                {
                    coll->depen.x = 0.0f;
                    coll->depen.y = xsqrt(hsqr) - dy;
                    coll->depen.z = 0.0f;
                    coll->dist = p->bound.sph.r - coll->depen.y;
                    coll->hdng.x = 0.0f;
                    coll->hdng.y = -1.0f;
                    coll->hdng.z = 0.0f;
                }
            }
        }
    }

    xent_entent = 0;
    return ent;
}

static U32 CollidePyramidBoxTop(xCollis* coll, xBox* box, F32 height, xSphere* sph)
{
    xVec3 point;
    xVec3 corner[2];
    F32 quaddirX;
    F32 quaddirZ;
    xSweptSphere sws;
    xVec3 start;
    xVec3 end;

    point.x = 0.5f * (box->lower.x + box->upper.x);
    point.y = box->upper.y + height;
    point.z = 0.5f * (box->lower.z + box->upper.z);

    F32 dx = sph->center.x - point.x;
    F32 dz = sph->center.z - point.z;

    if (sph->center.x <= box->lower.x - sph->r || sph->center.x >= box->upper.x + sph->r ||
        sph->center.z <= box->lower.z - sph->r || sph->center.z >= box->upper.z + sph->r ||
        sph->center.y >= point.y + sph->r)
    {
        return 0;
    }

    if (xabs(dx) < 0.001f && xabs(dz) < 0.001f)
    {
        dz = 0.001f;
    }

    corner[0].y = box->upper.y;
    corner[1].y = box->upper.y;

    if (xabs(dx) > xabs(dz))
    {
        quaddirZ = 0.0f;

        if (dx > 0.0f)
        {
            corner[0].x = box->upper.x;
            corner[0].z = box->upper.z;
            corner[1].x = box->upper.x;
            corner[1].z = box->lower.z;
            quaddirX = 1.0f;
        }
        else
        {
            corner[0].x = box->lower.x;
            corner[0].z = box->lower.z;
            corner[1].x = box->lower.x;
            corner[1].z = box->upper.z;
            quaddirX = -1.0f;
        }
    }
    else
    {
        quaddirX = 0.0f;

        if (dz > 0.0f)
        {
            corner[0].x = box->lower.x;
            corner[0].z = box->upper.z;
            corner[1].x = box->upper.x;
            corner[1].z = box->upper.z;
            quaddirZ = 1.0f;
        }
        else
        {
            corner[0].x = box->upper.x;
            corner[0].z = box->lower.z;
            corner[1].x = box->lower.x;
            corner[1].z = box->lower.z;
            quaddirZ = -1.0f;
        }
    }

    start.x = sph->center.x;
    start.y = sph->r + (sph->center.y + height);
    start.z = sph->center.z;
    end = sph->center;

    xSweptSpherePrepare(&sws, &start, &end, sph->r);

    if (xSweptSphereToTriangle(&sws, &point, &corner[0], &corner[1]))
    {
        xSweptSphereGetResults(&sws);

        F32 normX = sws.worldNormal.x;
        F32 normZ = sws.worldNormal.z;

        if (xabs(normX) < 1e-5f && xabs(normZ) < 1e-5f)
        {
            normX = quaddirX;
            normZ = quaddirZ;
        }
        else
        {
            F32 normMag = xsqrt(normX * normX + normZ * normZ);

            normX = normX * (1.0f / normMag);
            normZ = normZ * (1.0f / normMag);

            if (normX * quaddirX + normZ * quaddirZ < 0.70710678f)
            {
                normX = quaddirX;
                normZ = quaddirZ;
            }
        }

        F32 boxMaxSize = xsqrt(SQR(0.5f * (box->upper.x - box->lower.x)) +
                               SQR(0.5f * (box->upper.z - box->lower.z)));

        start.x = normX * (boxMaxSize + sph->r) + sph->center.x;
        start.y = sph->center.y;
        start.z = normZ * (boxMaxSize + sph->r) + sph->center.z;

        xSweptSpherePrepare(&sws, &start, &end, sph->r);

        if (xSweptSphereToTriangle(&sws, &point, &corner[0], &corner[1]))
        {
            xSweptSphereGetResults(&sws);

            coll->flags |= k_HIT_IT | k_HIT_0x20000;
            coll->oid = 0;
            coll->optr = NULL;
            coll->mptr = NULL;
            coll->dist = sph->r - (sws.dist - sws.curdist);
            coll->norm = sws.worldPolynorm;

            coll->tohit.x = -normX * coll->dist;
            coll->tohit.y = 0.0f;
            coll->tohit.z = -normZ * coll->dist;

            coll->hdng.x = -normX;
            coll->hdng.y = 0.0f;
            coll->hdng.z = -normZ;

            coll->depen.x = normX * (sws.dist - sws.curdist);
            coll->depen.y = 0.0f;
            coll->depen.z = normZ * (sws.dist - sws.curdist);

            return 1;
        }
    }

    return 0;
}

static xEnt* PlayerCollCheckOneVillain(xEnt* ent, xScene* sc, void* data)
{
    xEnt* p = (xEnt*)data;

    if (p->collis->idx >= 15)
    {
        return NULL;
    }

    if ((ent->chkby & p->collType) == 0)
    {
        return ent;
    }

    if (ent->id == p->id)
    {
        return ent;
    }

    if (!((zNPCCommon*)ent)->IsHealthy())
    {
        return ent;
    }

    xCollis* coll = &p->collis->colls[p->collis->idx];

    if (cchkButtbounce &&
        (p->frame->oldmat.pos.y >= ent->frame->mat.pos.y + ent->bound.sph.r ||
         p->frame->oldmat.pos.y >= ent->frame->oldmat.pos.y + ent->bound.sph.r) &&
        p->frame->mat.pos.y <= ent->frame->mat.pos.y + ent->bound.sph.r)
    {
        F32 playerOldRad = p->bound.sph.r;

        p->bound.sph.r *= 1.5f;
        xBoundHitsBound(&p->bound, &ent->bound, coll);
        p->bound.sph.r = playerOldRad;

        if (coll->flags & k_HIT_IT)
        {
            coll->norm.x = 0.0f;
            coll->norm.y = 1.0f;
            coll->norm.z = 0.0f;

            coll->depen.x = 0.0f;
            coll->depen.y =
                ent->frame->mat.pos.y + ent->bound.sph.r - p->frame->mat.pos.y;
            coll->depen.z = 0.0f;

            coll->dist = p->bound.sph.r - coll->depen.y;

            coll->hdng.x = 0.0f;
            coll->hdng.y = -1.0f;
            coll->hdng.z = 0.0f;

            coll->tohit.x = coll->hdng.x * coll->dist;
            coll->tohit.y = coll->hdng.y * coll->dist;
            coll->tohit.z = coll->hdng.z * coll->dist;

            coll->oid = ent->id;
            coll->optr = ent;
            coll->mptr = ent->model;

            p->collis->idx++;

            return ent;
        }
    }

    if (ent->bound.type == XBOUND_TYPE_BOX && ent->baseType == eBaseTypeNPC &&
        (((xNPCBasic*)ent)->SelfType() & 0xffffff00) != 'NTT\0' &&
        (((xNPCBasic*)ent)->SelfType() & 0xffffff00) != 'NTR\0' &&
        p->frame->mat.pos.y + p->bound.sph.r > ent->bound.box.box.upper.y)
    {
        xSphere tmpsph;

        tmpsph.center.x = p->frame->mat.pos.x;
        tmpsph.center.y = p->frame->mat.pos.y + p->bound.sph.r;
        tmpsph.center.z = p->frame->mat.pos.z;
        tmpsph.r = p->bound.sph.r;

        CollidePyramidBoxTop(coll, &ent->bound.box.box, 1.0f, &tmpsph);
    }
    else
    {
        xBoundHitsBound(&p->bound, &ent->bound, coll);
    }

    if (coll->flags & k_HIT_IT)
    {
        if (!(coll->flags & k_HIT_0x20000) && coll->hdng.y < 0.0f && coll->hdng.y > -0.93969f)
        {
            coll->flags |= k_HIT_0x20000;
        }

        coll->oid = 0;
        coll->optr = ent;
        coll->mptr = ent->model;

        p->collis->idx++;

        if (coll->mptr->Flags & 0x800)
        {
            coll->flags |= 0x2000;
        }

        if (coll->flags & 0x10)
        {
            xVec3Sub(&coll->tohit, xBoundCenter(&ent->bound), xEntGetCenter(p));
            xVec3SMul(&coll->depen, &coll->tohit, -0.25f * (1.0f / xVec3Length(&coll->tohit)));
            coll->depen.y = 0.0f;

            PlayerCollisBuildFromDepen(coll);
        }
    }

    return ent;
}

static void PlayerCollisTranslate(xCollis* c, F32 x, F32 y, F32 z)
{
    if (c->depen.x != 0.0f || c->depen.y != 0.0f || c->depen.z != 0.0f)
    {
        F32 dx = c->tohit.x - x;
        F32 dy = c->tohit.y - y;
        F32 dz = c->tohit.z - z;

        c->tohit.x = dx;
        c->tohit.y = dy;
        c->tohit.z = dz;

        F32 dist2 = xsqrt(dz * dz + (dx * dx + dy * dy));
        c->dist = dist2;

        F32 s = 1.0f / dist2;

        c->hdng.x = dx * s;
        c->hdng.y = dy * s;
        c->hdng.z = dz * s;

        F32 amt = MIN(0.0f, s * (dist2 - 0.5f));

        c->depen.x = dx * amt;
        c->depen.y = dy * amt;
        c->depen.z = dz * amt;
    }
}

static void PlayerCollsAllTranslate(xCollis* colls, F32 x, F32 y, F32 z)
{
    xCollis* end = colls + k_XCOLLS_IDX_COUNT;
    xCollis* c = colls;

    for (; c < end; c++)
    {
        if (c->dist < 1e38f)
        {
            PlayerCollisTranslate(c, x, y, z);
        }
    }
}

static void PlayerCollsWallsTranslate(xCollis* colls, F32 x, F32 y, F32 z)
{
    xCollis* end = colls + k_XCOLLS_IDX_COUNT;
    xCollis* c = colls + k_XCOLLS_IDX_FRONT;

    for (; c < end; c++)
    {
        if (c->dist < 1e38f)
        {
            PlayerCollisTranslate(c, x, y, z);
        }
    }
}

static void PlayerCollsWallsTranslate(xCollis* colls, const xVec3* v)
{
    PlayerCollsWallsTranslate(colls, v->x, v->y, v->z);
}

static void PlayerCollsSidesTranslate(xCollis* colls, F32 x, F32 y, F32 z)
{
    xCollis* left = &colls[k_XCOLLS_IDX_LEFT];
    if (left->dist < 1e38f)
    {
        PlayerCollisTranslate(left, x, y, z);
    }

    xCollis* right = &colls[k_XCOLLS_IDX_RIGHT];
    if (right->dist < 1e38f)
    {
        PlayerCollisTranslate(right, x, y, z);
    }
}

static void PlayerCollCheckEnv(xEnt* ent, xScene* sc)
{
    ent->collis->env_sidx = ent->collis->idx;

    ent->bound.sph.r = 0.65f;
    iSphereHitsEnv4(&ent->bound.sph, sc->env, (xMat3x3*)ent->frame, ent->collis->colls);
    ent->bound.sph.r = 0.5f;

    PlayerCollsAllTranslate(ent->collis->colls, 0.0f, 0.0f, 0.0f);

    ent->collis->env_eidx = ent->collis->idx;
}

static F32 ComputeFudge(F32 a, F32 b)
{
    F32 min = MIN(a, b);
    a = (min - -0.175f) / 0.074999996f; // Will not match with 0.075f.

    if (0.0f > MIN(a, 1.0f))
    {
        return 0.0f;
    }

    return MIN(a, 1.0f);
}

static void CalcCombinedDepen(F32& dx, F32& dz, F32 ax, F32 az, F32 bx, F32 bz, F32 fudge)
{
    F32 la = xsqrt(ax * ax + az * az);
    F32 lb = xsqrt(bx * bx + bz * bz);

    if (la < 1e-5f || lb < 1e-5f)
    {
        dx = 0.5f * (ax + bx);
        dz = 0.5f * (az + bz);
    }
    else
    {
        F32 normX = ax / la;
        F32 normZ = -az / la;
        F32 ubx = bx / lb;
        F32 nddot = normZ * ubx + normX * (bz / lb);
        F32 say = normX;

        if (nddot < 0.0f)
        {
            nddot = -nddot;
            normZ = -normZ;
            say = -normX;
        }

        if (nddot < 0.0001f)
        {
            dx = 0.5f * (ax + bx);
            dz = 0.5f * (az + bz);
        }
        else
        {
            F32 d = MAX(nddot, 0.25f);
            dx = (lb * normZ) / d;
            dz = (lb * say) / d;

            F32 nby = -bz / lb;
            F32 dot2 = nby * normX + ubx * (az / la);

            if (dot2 < 0.0f)
            {
                dot2 = -dot2;
                nby = -nby;
                ubx = -ubx;
            }

            if (dot2 < 0.0001f)
            {
                dx = 0.5f * (ax + bx);
                dz = 0.5f * (az + bz);
            }

            dot2 = MAX(0.25f, dot2);
            dx = dx + (la * nby) / dot2;
            dz = dz + (la * ubx) / dot2;
            dx = dx * fudge + 0.5f * (ax + bx) * (1.0f - fudge);
            dz = dz * fudge + 0.5f * (az + bz) * (1.0f - fudge);
        }
    }
}

static void PlayerCollsSelectDepen(xEnt* ent, xScene* sc, F32 dt)
{
    xCollis* colls = ent->collis->colls;
    xMat4x3* mat = &ent->frame->mat;
    xCollis* c = colls + k_XCOLLS_IDX_COUNT;
    xCollis* cend = colls + ent->collis->idx;
    xVec3 motion_delta = mat->pos - ent->frame->oldmat.pos;

    for (; c < cend; c++)
    {
        U8 idx = xCollideGetCollsIdx(c, &c->tohit, mat);

        if (idx == k_XCOLLS_IDX_FLOOR)
        {
            xSurface* surface = zSurfaceGetSurface(c);
            zSurfaceProps* surfaceProperties = (zSurfaceProps*)surface->moprops;

            if (surfaceProperties->asset->phys_flags & 0x20)
            {
                xVec3 vec;
                xVec3Init(&vec, c->tohit.x, 0.0001f, c->tohit.z);
                xVec3Normalize(&vec, &vec);
                idx = xCollideGetCollsIdx(c, &vec, mat);
            }
        }

        xCollis* curr = colls + idx;

        if (c->dist < curr->dist)
        {
            *curr = *c;
        }
    }

    if (ent->pflags & 0x80)
    {
        xCollis* coll = ent->collis->colls;

        if ((coll->flags & k_HIT_IT) && coll->dist < 0.5f)
        {
            F32 h_dot_n = xVec3Dot(&coll->hdng, &coll->norm);

            if (h_dot_n > 0.0f)
            {
                xVec3Inv(&coll->norm, &coll->norm);
                h_dot_n = -h_dot_n;
            }

            if (xabs(xacos(xVec3Dot(&coll->norm, &update_motion))) < 0.78539819f)
            {
                F32 depen_len = h_dot_n * coll->dist + 0.5f;

                if (depen_len < 0.0f || depen_len > 0.5f)
                {
                    depen_len = CLAMP(depen_len, 0.0f, 0.5f);
                }

                xVec3SMul(&coll->depen, &coll->norm, depen_len);
            }
        }
    }

    c = colls;
    cend = colls + k_XCOLLS_IDX_COUNT;

    for (; c < cend; c++)
    {
        xEnt* cent = (xEnt*)c->optr;

        if (!cent)
        {
            continue;
        }

        if (!(cent->penby & ent->collType) || !(ent->collis->pen & cent->collType))
        {
            c->depen.x = 0.0f;
            c->depen.y = 0.0f;
            c->depen.z = 0.0f;
            continue;
        }

        if (globals.player.DamageTimer > 0.0f)
        {
            xSurface* surf = zSurfaceGetSurface(c);

            if (surf && !surf->state && zSurfaceGetDamageType(surf) &&
                zSurfaceGetDamagePassthrough(surf))
            {
                c->depen.x = 0.0f;
                c->depen.y = 0.0f;
                c->depen.z = 0.0f;
                continue;
            }
        }

        if (cent->baseType == eBaseTypeVillain)
        {
            if (c == &colls[k_XCOLLS_IDX_FLOOR])
            {
                c->depen.y = 0.0f;
            }
            else if (c != &colls[k_XCOLLS_IDX_FRONT])
            {
                c->depen.x = 0.0f;
                c->depen.y = 0.0f;
                c->depen.z = 0.0f;
            }
        }
    }

    xCollis* cfloor = &colls[k_XCOLLS_IDX_FLOOR];
    xCollis* cceil = &colls[k_XCOLLS_IDX_CEIL];
    xSurface* sfloor = zSurfaceGetSurface(cfloor);

    if ((cfloor->flags & k_HIT_IT) && ent->frame->vel.y > 0.0f && globals.player.JumpState != 0 &&
        globals.player.JumpState != 1)
    {
        F32 floordot = xVec3Dot(&motion_delta, &cfloor->tohit);

        if (floordot < 0.0f)
        {
            cfloor->flags &= ~k_HIT_IT;
        }
    }

    if ((cfloor->flags & k_HIT_IT) && cfloor->dist < 0.5f)
    {
        if (sfloor && (globals.player.Slide == 1 || globals.player.Slide == 3))
        {
            xVec3AddTo(&mat->pos, &cfloor->depen);
            PlayerCollsWallsTranslate(colls, &cfloor->depen);
        }
        else
        {
            mat->pos.y += cfloor->depen.y;
            PlayerCollsWallsTranslate(colls, 0.0f, cfloor->depen.y, 0.0f);
        }
    }
    else if ((cceil->flags & k_HIT_IT) && cceil->dist < 0.5f)
    {
        if (cceil->hdng.y < icos(PI / 6))
        {
            xVec3AddTo(&mat->pos, &cceil->depen);
            PlayerCollsWallsTranslate(colls, &cceil->depen);
        }
        else
        {
            mat->pos.y += cceil->depen.y;
            PlayerCollsWallsTranslate(colls, 0.0f, cceil->depen.y, 0.0f);
            ent->frame->vel.y = 0.0f;
        }
    }

    S32 num_walls = 0;
    xCollis* first_wall = NULL;
    xCollis* inside_wall = NULL;

    for (c = &colls[k_XCOLLS_IDX_FRONT], cend = &colls[k_XCOLLS_IDX_COUNT]; c < cend; c++)
    {
        if (c->flags & k_HIT_0x10)
        {
            inside_wall = c;
        }

        if (c->flags & k_HIT_IT)
        {
            if (c->optr)
            {
                num_walls++;

                if (!first_wall)
                {
                    first_wall = c;
                }
            }
            else
            {
                num_walls++;

                if (!first_wall)
                {
                    first_wall = c;
                }
            }
        }
    }

    if (inside_wall)
    {
        inside_wall->flags &= ~k_HIT_IT;
    }

    if (num_walls)
    {
        if (num_walls == 1)
        {
            mat->pos.x += first_wall->depen.x;
            mat->pos.z += first_wall->depen.z;
        }
        else
        {
            xCollis* cfront = &colls[k_XCOLLS_IDX_FRONT];
            xCollis* crear = &colls[k_XCOLLS_IDX_REAR];

            if ((cfront->flags & k_HIT_IT) && cfront->dist < 0.5f)
            {
                if ((crear->flags & k_HIT_IT) && crear->dist < 0.5f)
                {
                    F32 dx;
                    F32 dz;

                    CalcCombinedDepen(dx, dz, crear->depen.x, crear->depen.z, cfront->depen.x,
                                      cfront->depen.z,
                                      ComputeFudge(crear->tohit.y, cfront->tohit.y));

                    mat->pos.x += dx;
                    mat->pos.z += dz;

                    PlayerCollsSidesTranslate(colls, dx, 0.0f, dz);
                }
                else
                {
                    mat->pos.x += cfront->depen.x;
                    mat->pos.z += cfront->depen.z;

                    PlayerCollsSidesTranslate(colls, cfront->depen.x, 0.0f, cfront->depen.z);
                }
            }
            else if ((crear->flags & k_HIT_IT) && crear->dist < 0.5f)
            {
                mat->pos.x += crear->depen.x;
                mat->pos.z += crear->depen.z;

                PlayerCollsSidesTranslate(colls, crear->depen.x, 0.0f, crear->depen.z);
            }

            xCollis* cleft = &colls[k_XCOLLS_IDX_LEFT];
            xCollis* cright = &colls[k_XCOLLS_IDX_RIGHT];

            if ((cleft->flags & k_HIT_IT) && cleft->dist < 0.5f)
            {
                if ((cright->flags & k_HIT_IT) && cright->dist < 0.5f)
                {
                    F32 dx;
                    F32 dz;

                    CalcCombinedDepen(dx, dz, cleft->depen.x, cleft->depen.z, cright->depen.x,
                                      cright->depen.z,
                                      ComputeFudge(cleft->tohit.y, cright->tohit.y));

                    mat->pos.x += dx;
                    mat->pos.z += dz;
                }
                else
                {
                    mat->pos.x += cleft->depen.x;
                    mat->pos.z += cleft->depen.z;
                }
            }
            else if ((cright->flags & k_HIT_IT) && cright->dist < 0.5f)
            {
                mat->pos.x += cright->depen.x;
                mat->pos.z += cright->depen.z;
            }
        }
    }
}

void zEntPlayerCollide(xEnt* ent, xScene* sc, F32 dt)
{
    cchkButtbounce =
        globals.player.Health && (ent->model->Anim->Single->State->UserFlags & 0x1e) == 0xe;

    cchkSquish = globals.player.Health &&
                 (ent->model->Anim->Single->State->UserFlags & 0x1e) == 0xa &&
                 ent->frame->mat.pos.y < ent->frame->oldmat.pos.y;

    xEntBeginCollide(ent, sc, dt);

    if (ent->collis->chk & XENT_COLLTYPE_ENV)
    {
        PlayerCollCheckEnv(ent, sc);
    }

    if (ent->collis->chk & XENT_COLLTYPE_NPC)
    {
        xEntCollCheckNPCsByGrid(ent, sc, PlayerCollCheckOneVillain);
    }

    if (ent->collis->chk & (XENT_COLLTYPE_STAT | XENT_COLLTYPE_DYN))
    {
        xEntCollCheckByGrid(ent, sc, PlayerCollCheckOneEnt);
    }

    if (ent->collis->chk & 0x2e)
    {
        PlayerCollsSelectDepen(ent, sc, dt);

        xCollis* colls = ent->collis->colls;
        xCollis* c = &colls[k_XCOLLS_IDX_FRONT];
        xCollis* cend = &colls[ent->collis->idx];
        for (; c < cend; c++)
        {
            if (c->dist > 0.5f)
            {
                c->flags &= ~k_HIT_IT;
            }
        }
    }

    xEntEndCollide(ent, sc, dt);
}

void zEntPlayerCollTrigger(xEnt* ent, xScene* sc)
{
    U32 i;
    U32 inside;
    zEntTrigger* trig;

    for (i = 0; i < sc->num_trigs; i++)
    {
        trig = (zEntTrigger*)sc->trigs[i];

        if (!xBaseIsEnabled(trig))
        {
            continue;
        }

        inside = 0;
        xTriggerAsset* tasset = (xTriggerAsset*)(trig->asset + 1);

        switch (trig->subType)
        {
        case ZENTTRIGGER_TYPE_BOX:
        {
            xVec3 v;
            xIsect isect;

            xMat4x3Tolocal(&v, &trig->triggerMatrix, &ent->bound.sph.center);
            iBoxIsectVec(&trig->triggerBox, &v, &isect);

            if (isect.penned <= 0.0f)
            {
                inside = 1;
            }
            break;
        }
        case ZENTTRIGGER_TYPE_SPHERE:
        {
            xSphere sph;
            xIsect isect;

            sph.center = tasset->p[0];
            sph.r = tasset->p[1].x;

            iSphereIsectVec(&sph, &ent->bound.sph.center, &isect);

            if (isect.penned <= 0.0f)
            {
                inside = 1;
            }
            break;
        }
        case ZENTTRIGGER_TYPE_VCYLINDER:
        {
            xCylinder cyl;
            xIsect isect;

            cyl.center = tasset->p[0];
            cyl.r = tasset->p[1].x;
            cyl.h = tasset->p[1].y;

            iCylinderIsectVec(&cyl, &ent->bound.sph.center, &isect);

            if (isect.penned <= 0.0f)
            {
                inside = 1;
            }
            break;
        }
        case ZENTTRIGGER_TYPE_VSPHERE:
        {
            xSphere sph;
            xIsect isect;

            sph.center = tasset->p[0];
            sph.r = tasset->p[1].x;

            iSphereIsectVec(&sph, &ent->bound.sph.center, &isect);

            if (isect.penned <= 0.0f)
            {
                inside = 1;
            }
            break;
        }
        case ZENTTRIGGER_TYPE_4:
        case ZENTTRIGGER_TYPE_5:
            break;
        }

        if (inside && !(trig->entered & 0x1))
        {
            if (tasset->flags & 0x1)
            {
                if (xVec3Dot(&tasset->direction, (xVec3*)&ent->model->Mat->at) <= 0.0f)
                {
                    zEntEvent(trig, eEventEnterPlayer);

                    switch (gCurrentPlayer)
                    {
                    case eCurrentPlayerSpongeBob:
                        zEntEvent(trig, eEventEnterSpongeBob);
                        break;
                    case eCurrentPlayerPatrick:
                        zEntEvent(trig, eEventEnterPatrick);
                        break;
                    case eCurrentPlayerSandy:
                        zEntEvent(trig, eEventEnterSandy);
                        break;
                    }
                }
            }
            else
            {
                zEntEvent(trig, eEventEnterPlayer);

                switch (gCurrentPlayer)
                {
                case eCurrentPlayerSpongeBob:
                    zEntEvent(trig, eEventEnterSpongeBob);
                    break;
                case eCurrentPlayerPatrick:
                    zEntEvent(trig, eEventEnterPatrick);
                    break;
                case eCurrentPlayerSandy:
                    zEntEvent(trig, eEventEnterSandy);
                    break;
                }
            }
        }

        if (!inside && (trig->entered & 0x1))
        {
            if (tasset->flags & 0x1)
            {
                if (xVec3Dot(&tasset->direction, (xVec3*)&ent->model->Mat->at) <= 0.0f)
                {
                    zEntEvent(trig, eEventExitPlayer);

                    switch (gCurrentPlayer)
                    {
                    case eCurrentPlayerSpongeBob:
                        zEntEvent(trig, eEventExitSpongeBob);
                        break;
                    case eCurrentPlayerPatrick:
                        zEntEvent(trig, eEventExitPatrick);
                        break;
                    case eCurrentPlayerSandy:
                        zEntEvent(trig, eEventExitSandy);
                        break;
                    }
                }
            }
            else
            {
                zEntEvent(trig, eEventExitPlayer);

                switch (gCurrentPlayer)
                {
                case eCurrentPlayerSpongeBob:
                    zEntEvent(trig, eEventExitSpongeBob);
                    break;
                case eCurrentPlayerPatrick:
                    zEntEvent(trig, eEventExitPatrick);
                    break;
                case eCurrentPlayerSandy:
                    zEntEvent(trig, eEventExitSandy);
                    break;
                }
            }
        }

        if (inside && !globals.player.ControlOff && (globals.pad0->pressed & XPAD_BUTTON_O))
        {
            zEntEvent(trig, eEventButtonPressAction);
        }

        if (inside)
        {
            trig->entered |= 0x1;
        }
        else
        {
            trig->entered &= ~0x1;
        }
    }
}

static xVec3* GetPosVec(xBase* base)
{
    xVec3* vec = (xVec3*)&g_O3;

    switch (base->baseType)
    {
    case eBaseTypeMovePoint:
        vec = ((xMovePoint*)(base))->pos;
        break;
    case eBaseTypeVillain:
    case eBaseTypePlayer:
    case eBaseTypePickup:
    case eBaseTypePlatform:
    case eBaseTypeDoor:
    case eBaseTypeStatic:
    case eBaseTypeDynamic:
    case eBaseTypePendulum:
    case eBaseTypeHangable:
    case eBaseTypeButton:
    case eBaseTypeDestructObj:
        vec = (xVec3*)&(((xEnt*)(base))->model->Mat->pos);
        break;
    }

    return vec;
}

S32 zEntPlayerEventCB(xBase* from, xBase* to, U32 toEvent, const F32* toParam, xBase* toParamWidget)
{
    if (cruise_bubble::event_handler(from, toEvent, toParam, toParamWidget))
    {
        return 1;
    }

    switch (toEvent)
    {
    case eEventControlOff:
        if (globals.player.ent.update == (xEntUpdateCallback)zEntPlayer_BoulderVehicleUpdate)
        {
            BoulderRollDoneCB();
        }

        zEntPlayerControlOff(CONTROL_OWNER_EVENT);
        globals.player.ControlOnEvent = 0;
        globals.player.ControlOffTimer = 1.0f;
        globals.player.AutoMoveSpeed = 0;
        xScrFxLetterbox(1);
        break;
    case eEventControlOn:
        globals.player.ControlOnEvent = 1;
        globals.player.AutoMoveSpeed = 0;
        xScrFxLetterbox(0);
        break;
    case eEventGiveHealth:
    {
        S32 amount = (S32)toParam[0];

        if (amount == 0)
        {
            amount = 1;
        }

        if (amount < 0)
        {
            if (zEntPlayer_Damage(from, -amount) && globals.player.Health != 0)
            {
                if (0.0f == toParam[1])
                {
                    if (amount != -666 && globals.player.g.DamageGiveHealthKnock)
                    {
                        zEntPlayer_DamageKnockIntoAir(globals.player.g.DamageGiveHealthKnock);
                    }
                }
                else if (toParam[1] > 0.0f)
                {
                    zEntPlayer_DamageKnockIntoAir(toParam[1]);
                }
            }
        }
        else
        {
            zEntPlayer_GiveHealth(amount);
        }

        break;
    }
    case eEventGiveShinyObjects:
        zEntPlayer_GiveShinyObject((S32)(toParam[0] + (toParam[0] > 0.0f ? 0.5f : -0.5f)));
        break;
    case eEventGivePowerUp:
        globals.player.g.PowerUp[(S32)toParam[0]] = 1;
        break;
    case eEventTakeSocks:
    {
        S32 amount = (S32)(toParam[0] + (toParam[0] > 0.0f ? 0.5f : -0.5f));

        if (amount < 0)
        {
            amount = 0;
        }

        if (amount > (S32)globals.player.Inv_PatsSock_Total)
        {
            globals.player.Inv_PatsSock_Total = 0;
        }
        else
        {
            globals.player.Inv_PatsSock_Total -= amount;
        }

        break;
    }
    case eEventGiveCurrLevelSocks:
        zEntPlayer_GivePatsSocksCurrentLevel((S32)toParam[0]);
        break;
    case eEventGiveCurrLevelPickup:
        zEntPlayer_GiveLevelPickupCurrentLevel((S32)toParam[0]);
        break;
    case eEventSetCurrLevelSocks:
    {
        U32 had = globals.player.Inv_PatsSock[zSceneGetLevelIndex()];

        globals.player.Inv_PatsSock_Total += (U32)toParam[0] - had;
        globals.player.Inv_PatsSock[zSceneGetLevelIndex()] = (U32)toParam[0];
        break;
    }
    case eEventSetCurrLevelPickup:
        globals.player.Inv_LevelPickups[zSceneGetLevelIndex()] = (U32)toParam[0];
        break;
    case eEventKill:
        if (globals.player.ent.update == (xEntUpdateCallback)zEntPlayer_BoulderVehicleUpdate)
        {
            BoulderRollDoneCB();
        }

        if (globals.player.Health != 0)
        {
            globals.player.DamageTimer = 0.0f;
            zEntPlayer_Damage(from, globals.player.Health, NULL);
        }

        break;
    case eEventHit:
        if (globals.player.ent.update == (xEntUpdateCallback)zEntPlayer_BoulderVehicleUpdate)
        {
            BoulderRollDoneCB();
        }

        if (globals.player.Health != 0)
        {
            zEntPlayer_Damage(from, 1, NULL);
        }

        break;
    case eEventOutOfBounds:
        if (to == (xBase*)&globals.player.ent)
        {
            globals.dontShowPadMessageDuringLoadingOrCutScene = 1;

            if (globals.player.ent.update == (xEntUpdateCallback)zEntPlayer_BoulderVehicleUpdate)
            {
                BoulderRollDoneCB();
            }

            oob_state::force_start();
        }
        else if (((xEnt*)to)->asset)
        {
            xVec3Copy(&((xEnt*)to)->frame->mat.pos, &((xEnt*)to)->asset->pos);
            xVec3Copy((xVec3*)&((xEnt*)to)->model->Mat->pos, &((xEnt*)to)->asset->pos);
            xVec3Copy(&((xEnt*)to)->frame->vel, &g_O3);
            xVec3Copy(&((xEnt*)to)->frame->rot.axis, &g_Y3);
            ((xEnt*)to)->frame->rot.angle = 0.0f;
        }

        break;
    case eEventFallToDeath:
    {
        if (globals.player.ent.update == (xEntUpdateCallback)zEntPlayer_BoulderVehicleUpdate)
        {
            BoulderRollDoneCB();
        }

        const char* anim = ((xEnt*)to)->model->Anim->Single[0].State->Name;

        if (xStricmp(anim, "Fall_losechance") != 0 && xStricmp(anim, "Fall_losedrop") != 0)
        {
            globals.player.FallDeathTimer = 1.2f;
            globals.player.Health = 0;
            globals.player.DamageTimer = 15.0f;
            globals.player.ent.frame->vel.x = 0.0f;
            globals.player.ent.frame->vel.z = 0.0f;
        }

        break;
    }
    case eEventVisible:
        globals.player.Visible = 1;
        break;
    case eEventInvisible:
        globals.player.Visible = 0;
        xShadowManager_Remove(&globals.player.ent);
        break;
    case eEventMoveToTarget:
        if (globals.player.ControlOff && toParamWidget)
        {
            globals.player.AutoMoveDist = toParam[0];
            globals.player.AutoMoveSpeed = (U32)toParam[1] + 1;
            globals.player.AutoMoveTarget = *GetPosVec(toParamWidget);
            globals.player.AutoMoveObject = toParamWidget;

            if (globals.player.AutoMoveSpeed > 3 || globals.player.AutoMoveSpeed < 1)
            {
                globals.player.AutoMoveSpeed = 1;
            }
        }

        break;
    case eEventFaceTarget:
        if (globals.player.ControlOff && toParamWidget)
        {
            globals.player.AutoMoveSpeed = 4;
            globals.player.AutoMoveTarget = *GetPosVec(toParamWidget);
            globals.player.AutoMoveObject = toParamWidget;
        }

        break;
    case eEventNPCPatrolOff:
        globals.player.AutoMoveSpeed = 0;
        break;
    case eEventPlayerRumbleTest:
        zRumbleStart((_tagSDRumbleType)1);
        break;
    case eEventPlayerRumbleLight:
        if (toParam[0])
        {
            zPadAddRumble((_tagRumbleType)5, toParam[0], 0, 0);
        }
        else
        {
            zRumbleStart((_tagSDRumbleType)4);
        }

        break;
    case eEventPlayerRumbleMedium:
        if (toParam[0])
        {
            zPadAddRumble((_tagRumbleType)7, toParam[0], 0, 0);
        }
        else
        {
            zRumbleStart((_tagSDRumbleType)5);
        }

        break;
    case eEventPlayerRumbleHeavy:
        if (toParam[0])
        {
            zPadAddRumble((_tagRumbleType)9, toParam[0], 0, 0);
        }
        else
        {
            zRumbleStart((_tagSDRumbleType)6);
        }

        break;
    case eEventLaunchShrapnel:
        if (toParamWidget)
        {
            zShrapnelAsset* shrap = (zShrapnelAsset*)toParamWidget;

            if (shrap->initCB)
            {
                xVec3 vel;

                xVec3SMul(&vel, &globals.player.PredictCurrDir, globals.player.PredictCurrVel);
                shrap->initCB(shrap, globals.player.ent.model, &vel, NULL);
            }
        }

        break;
    case eEventPlrSwitchCharacter:
        if (1.0f == toParam[0])
        {
            gCurrentPlayer = eCurrentPlayerPatrick;
        }
        else if (2.0f == toParam[0])
        {
            gCurrentPlayer = eCurrentPlayerSandy;
        }
        else
        {
            gCurrentPlayer = eCurrentPlayerSpongeBob;
        }

        break;
    case eEventSituationDestroyedTiki:
        if ((xrand() & 3) == 3)
        {
            zEntPlayer_SNDPlayStreamRandom(0, 20, ePlayerStreamSnd_DestroyTiki1,
                                           ePlayerStreamSnd_DestroyTiki3, 0.5f);
        }

        break;
    case eEventSituationDestroyedRobot:
        if ((xrand() & 3) == 3)
        {
            zEntPlayer_SNDPlayStreamRandom(0, 20, ePlayerStreamSnd_DestroyRobot1,
                                           ePlayerStreamSnd_DestroyRobot3, 0.5f);
        }

        break;
    case eEventSituationSeeFodder:
        zEntPlayer_SNDPlayStreamRandom(ePlayerStreamSnd_SeeFodder, ePlayerStreamSnd_SeeFodder, 0.5f);
        break;
    case eEventSituationSeeHammer:
        zEntPlayer_SNDPlayStreamRandom(ePlayerStreamSnd_SeeHammer, ePlayerStreamSnd_SeeHammer, 0.5f);
        break;
    case eEventSituationSeeTarTar:
        zEntPlayer_SNDPlayStreamRandom(ePlayerStreamSnd_SeeTarTar, ePlayerStreamSnd_SeeTarTar, 0.5f);
        break;
    case eEventSituationSeeGLove:
        zEntPlayer_SNDPlayStreamRandom(ePlayerStreamSnd_SeeGLove, ePlayerStreamSnd_SeeGLove, 0.5f);
        break;
    case eEventSituationSeeMonsoon:
        zEntPlayer_SNDPlayStreamRandom(ePlayerStreamSnd_SeeMonsoon, ePlayerStreamSnd_SeeMonsoon,
                                       0.5f);
        break;
    case eEventSituationSeeSleepyTime:
        zEntPlayer_SNDPlayStreamRandom(ePlayerStreamSnd_SeeSleepyTime,
                                       ePlayerStreamSnd_SeeSleepyTime, 0.5f);
        break;
    case eEventSituationSeeArf:
        zEntPlayer_SNDPlayStreamRandom(ePlayerStreamSnd_SeeArf, ePlayerStreamSnd_SeeArf, 0.5f);
        break;
    case eEventSituationSeeTubelets:
        zEntPlayer_SNDPlayStreamRandom(ePlayerStreamSnd_SeeTubelets, ePlayerStreamSnd_SeeTubelets,
                                       0.5f);
        break;
    case eEventSituationSeeSlick:
        zEntPlayer_SNDPlayStreamRandom(ePlayerStreamSnd_SeeSlick, ePlayerStreamSnd_SeeSlick, 0.5f);
        break;
    case eEventSituationSeeKingJellyfish:
        zEntPlayer_SNDPlayStreamRandom(ePlayerStreamSnd_SeeKingJellyfish,
                                       ePlayerStreamSnd_SeeKingJellyfish, 0.5f);
        break;
    case eEventSituationSeePrawn:
        zEntPlayer_SNDPlayStreamRandom(ePlayerStreamSnd_SeePrawn, ePlayerStreamSnd_SeePrawn, 0.5f);
        break;
    case eEventSituationSeeDutchman:
        zEntPlayer_SNDPlayStreamRandom(ePlayerStreamSnd_SeeDutchman, ePlayerStreamSnd_SeeDutchman,
                                       0.5f);
        break;
    case eEventSituationSeeSandyBoss:
        zEntPlayer_SNDPlayStreamRandom(ePlayerStreamSnd_SeeSandyBoss, ePlayerStreamSnd_SeeSandyBoss,
                                       0.5f);
        break;
    case eEventSituationSeePatrickBoss:
        zEntPlayer_SNDPlayStreamRandom(ePlayerStreamSnd_SeePatrickBoss1,
                                       ePlayerStreamSnd_SeePatrickBoss2, 0.5f);
        break;
    case eEventSituationSeeSpongeBobBoss:
        zEntPlayer_SNDPlayStreamRandom(ePlayerStreamSnd_SeeSpongeBobBoss,
                                       ePlayerStreamSnd_SeeSpongeBobBoss, 0.5f);
        break;
    case eEventSituationSeeRobotPlankton:
        zEntPlayer_SNDPlayStreamRandom(ePlayerStreamSnd_SeeRobotPlankton,
                                       ePlayerStreamSnd_SeeRobotPlankton, 0.5f);
        break;
    case eEventPlayerDeath:
    case eEventGiveChance:
    case eEventSituationPlayerSuccess:
    case eEventSituationPlayerFailure:
        break;
    }

    return 1;
}

static void PlayerSwingUpdate(xEnt* ent, F32 mag, F32 angle, F32 dt)
{
    xVec3 pos;
    xVec3 unitHang;
    xVec3 accel;
    xVec3 unitAccel;
    xVec3 unitVel;
    xVec3 unitDefl;

    pos = *(xVec3*)&ent->model->Mat->pos;
    pos.y += ent->bound.sph.r;

    accel.x = mag * (2.0f * isin(angle));
    accel.y = 0.0f;
    accel.z = mag * (2.0f * icos(angle));

    xVec3Normalize(&unitAccel, &accel);

    for (S32 i = k_XCOLLS_IDX_FRONT; i < k_XCOLLS_IDX_COUNT; i++)
    {
        if (ent->collis->colls[i].flags & k_HIT_IT)
        {
            xVec3AddScaled(&globals.player.HangVel, &ent->collis->colls[i].norm,
                           -xVec3Dot(&globals.player.HangVel, &ent->collis->colls[i].norm));
        }
    }

    xVec3Normalize(&unitVel, &globals.player.HangVel);

    xVec3Sub(&unitHang, &pos, (xVec3*)&sLassoInfo->swingTarget->model->Mat->pos);

    F32 hangDist = xVec3Normalize(&unitHang, &unitHang);

    F32 hangDot = xVec3Dot(&unitHang, &accel);

    accel.x -= hangDot * unitHang.x;
    accel.y -= hangDot * unitHang.y;
    accel.z -= hangDot * unitHang.z;

    if (xVec3Normalize(&unitDefl, &accel) > 1e-5f)
    {
        F32 velDot = xVec3Dot(&unitDefl, &globals.player.HangVel);

        if (velDot < 0.0f)
        {
            xVec3Inv(&accel, &accel);
        }

        xVec3 velPerp;
        xVec3 velAlong;

        xVec3SMul(&velAlong, &unitDefl, velDot);
        xVec3Sub(&velPerp, &globals.player.HangVel, &velAlong);
        xVec3AddScaled(&accel, &velPerp, -1.5f);
    }

    accel.y -= 15.0f;

    globals.player.HangVel.x += accel.x * dt;
    globals.player.HangVel.y += accel.y * dt;
    globals.player.HangVel.z += accel.z * dt;

    F32 hangVelDot = xVec3Dot(&unitHang, &globals.player.HangVel);

    if (hangDist > 3.96f && hangVelDot > 0.0f)
    {
        globals.player.HangVel.x -= hangVelDot * unitHang.x;
        globals.player.HangVel.y -= hangVelDot * unitHang.y;
        globals.player.HangVel.z -= hangVelDot * unitHang.z;
    }

    pos.x += globals.player.HangVel.x * dt;
    pos.y += globals.player.HangVel.y * dt;
    pos.z += globals.player.HangVel.z * dt;

    xVec3Sub(&unitHang, &pos, (xVec3*)&sLassoInfo->swingTarget->model->Mat->pos);

    hangDist = xVec3Normalize(&unitHang, &unitHang);

    if (hangDist > 3.96f)
    {
        hangDist = 0.95f * hangDist + 0.2f;
    }

    globals.player.RootUpTarget = unitHang;
    xVec3Inv(&globals.player.RootUpTarget, &globals.player.RootUpTarget);

    globals.player.HangElapsed += dt;

    xVec3Copy(&ent->frame->mat.pos, (xVec3*)&sLassoInfo->swingTarget->model->Mat->pos);
    xVec3AddScaled(&ent->frame->mat.pos, &unitHang, hangDist);

    ent->frame->mat.pos.y -= ent->bound.sph.r;
    ent->frame->mode |= 0x1;

    if (globals.player.HangStartLerp < 1.0f)
    {
        globals.player.HangStartLerp = MIN(1.0f, 5.0f * dt + globals.player.HangStartLerp);
    }

    F32 lerpDiff = 1.0f - mag * xVec3Dot(&unitAccel, &unitVel);
    F32 lerp = ent->model->Anim->Single->BilinearLerp[0];

    lerpDiff -= lerp;
    lerpDiff = dt * lerpDiff;
    lerpDiff = 6.0f * lerpDiff;

    ent->model->Anim->Single->BilinearLerp[0] += lerpDiff;
    ent->model->Anim->Single->Blend->BilinearLerp[0] += lerpDiff;

    F32 newLerp = ent->model->Anim->Single->BilinearLerp[0];

    if ((lerp <= 1.8f && newLerp > 1.8f) || (lerp >= 0.2f && newLerp < 0.2f))
    {
        zEntPlayer_SNDPlay(ePlayerSnd_LassoYank, 0.0f);
    }

    sSwingTimeElapsed += dt;

    F32 curFactor = zCameraGetLassoCamFactor();

    F32 camDot = xVec3Dot(&globals.camera.mat.at, &unitHang);

    zCameraSetLassoCamFactor(0.8f * curFactor + 0.2f * (0.5f - 0.5f * camDot));
}

static void PlayerTeeterCheck(xEnt* ent, xScene* sc, F32 dt)
{
    S32 i;

    if (globals.player.MountChimney)
    {
        globals.player.Teeter = 4;
        return;
    }

    if (globals.player.Teeter)
    {
        globals.player.Teeter--;
    }

    if (globals.player.JumpState)
    {
        return;
    }

    for (i = 0; i < 4; i++)
    {
        if (floor_dist[i] > 0.424264f)
        {
            if (floor_tmr[i] >= 0.2f)
            {
                floor_tmr[i] = 0.2f;
                globals.player.Teeter = 1;
                return;
            }

            floor_tmr[i] += dt;
        }
        else
        {
            floor_tmr[i] = 0.0f;
        }
    }
}

static void PlayerRotMatchUpdateEnt(xEnt* ent, xScene* sc, F32 dt, void* fdata)
{
    xFFXRotMatchState* rms = (xFFXRotMatchState*)fdata;

    if (!ent->collis)
    {
        return;
    }

    if (!ent->frame)
    {
        return;
    }

    xCollis* coll = ent->collis->colls;
    S32 hit_it = coll->flags & 0x1;
    xSurface* surf = zSurfaceGetSurface(coll);
    S32 grounded = 0;

    if (hit_it && surf && !surf->state && zSurfaceGetMatchOrient(surf))
    {
        grounded = 1;
    }

    if (grounded)
    {
        if (!rms->lgrounded)
        {
            rms->tmr = 0.0f;
        }

        xVec3* eup = &globals.player.RootUpTarget;
        xVec3* fup = &globals.player.floor_norm;
        xVec3 nfup;
        xVec3 neup;

        F32 fup_len = xVec3Normalize(&nfup, fup);
        F32 eup_len = xVec3Normalize(&neup, eup);
        F32 fdecl = xacos(nfup.y);
        F32 edecl = xacos(neup.y);

        if (edecl < rms->max_decl || fdecl < rms->max_decl)
        {
            xVec3 raxis;

            xVec3Cross(&raxis, eup, fup);
            xVec3Normalize(&raxis, &raxis);

            F32 rang = xVec3Dot(&nfup, &neup);

            if (rang > 1.0f)
            {
                rang = 1.0f;
            }

            rang = xacos(rang);

            if (rang)
            {
                F32 s = MIN(1.0f, dt / rms->tmatch);

                if (fdecl >= rms->max_decl)
                {
                    s = MIN(s, (rms->max_decl - edecl) / (fdecl - edecl));
                }

                if (s)
                {
                    s = s * rang;
                    s -= 0.001f;

                    xMat4x3 rot;

                    xMat4x3Rot(&rot, &raxis, s, xEntGetPos(ent));
                    xMat3x3RMulVec(eup, &rot, &neup);

                    globals.player.HangElapsed = 0.0f;
                }
            }
        }
    }
    else if (!globals.player.HangEnt)
    {
        if (rms->lgrounded)
        {
            rms->tmr = -0.1f;
        }

        if (rms->tmr > 0.0f)
        {
            xVec3* eup = &globals.player.RootUpTarget;
            xVec3 neup;

            F32 eup_len = xVec3Normalize(&neup, eup);

            xVec3 raxis;

            xVec3Init(&raxis, -neup.z, 0.0f, neup.x);

            F32 rang = xacos(neup.y);

            if (rang)
            {
                F32 s = MIN(1.0f, dt / rms->trelax);

                s = s * rang;

                xMat4x3 rot;

                xMat4x3Rot(&rot, &raxis, s, xEntGetPos(ent));
                xMat3x3RMulVec(eup, &rot, &neup);

                globals.player.HangElapsed = 0.0f;
            }
        }
    }

    if (xabs(globals.player.RootUpTarget.y - 1.0f) < 1e-5f)
    {
        globals.player.RootUpTarget.x = 0.0f;
        globals.player.RootUpTarget.y = 1.0f;
        globals.player.RootUpTarget.z = 0.0f;

        globals.player.HangElapsed = 0.0f;
    }

    rms->tmr += dt;
    rms->lgrounded = grounded;
}

void zEntPlayer_StoreCheckPoint(xVec3* pos, F32 rot, U32 initCamID)
{
    if (pos != NULL)
    {
        globals.player.cp.pos = *pos;
        globals.player.cp.rot = rot;
        globals.player.cp.initCamID = initCamID;
    }
}

void zEntPlayer_LoadCheckPoint()
{
    xEnt& p = globals.player.ent;
    xModelInstance& m = *p.model;
    xEntFrame& f = *p.frame;
    zCheckPoint& cp = globals.player.cp;

    f.mat.pos = cp.pos;
    f.oldmat.pos = cp.pos;
    f.rot.angle = cp.rot;
    f.rot.axis = xVec3::create(0.0f, 1.0f, 0.0f);

    xMat3x3Euler(&f.mat, f.rot.angle, 0, 0);
    *(xMat4x3*)(m.Mat) = f.mat;
    xCameraSetTargetMatrix(&globals.camera, xEntGetFrame(&globals.player.ent));
    xCameraSetTargetOMatrix(&globals.camera, &sCameraLastMat);

    if (sPlayerDiedLastTime != 0)
    {
        sPlayerDiedLastTime = 0;
        zEntPlayer_SNDPlayStreamRandom(0x00, 0x01, ePlayerStreamSnd_EnterScene1,
                                       ePlayerStreamSnd_EnterScene3, 0.3f);
        zEntPlayer_SNDPlayStreamRandom(0x02, 0x04, ePlayerStreamSnd_EnterScene1,
                                       ePlayerStreamSnd_EnterScene4, 0.3f);
        zEntPlayer_SNDPlayStreamRandom(0x05, 0x0A, ePlayerStreamSnd_EnterScene1,
                                       ePlayerStreamSnd_EnterScene5, 0.3f);
        zEntPlayer_SNDPlayStreamRandom(0x0B, 0x19, ePlayerStreamSnd_EnterScene1,
                                       ePlayerStreamSnd_EnterScene6, 0.3f);
        zEntPlayer_SNDPlayStreamRandom(0x1A, 0x64, ePlayerStreamSnd_EnterScene1,
                                       ePlayerStreamSnd_EnterScene7, 0.3f);
    }

    zCameraReset(&globals.camera);
}

static void _SetupRumble(_tagePlayerSnd player_snd, _tagRumbleType type, float time)
{
    sPlayerRumbleType[player_snd] = type;
    sPlayerRumbleTime[player_snd] = time;
}

// Close. missing redundant float loads, maybe equivalent.
static void zEntPlayer_SNDInit()
{
    for (S32 player = 0; player < ePlayer_MAXTYPES; player++)
    {
        for (S32 snd = 0; snd < ePlayerSnd_Total; snd++)
        {
            sPlayerSndID[player][snd] = 0;
            sPlayerSndRand[player][snd] = 0;
            sPlayerStreamSndRand[player][snd] = 0;
        }
    }
    for (S32 snd = 0; snd < ePlayerStreamSnd_Total; snd++)
    {
        sPlayerStreamSndTimer[snd].timer = 0.0f;
        sPlayerStreamSndTimer[snd].time = 0.0f;
    }

    for (S32 i = 0; i < MAX_DELAYED_SOUNDS; i++)
    {
        sDelayedSound[i].start = ePlayerStreamSnd_Invalid;
        sDelayedSound[i].end = ePlayerStreamSnd_Invalid;
        sDelayedSound[i].delay = 0.0f;
    }

    for (S32 snd = 0; snd < ePlayerSnd_Total; snd++)
    {
        sPlayerRumbleType[snd] = eRumble_Off;
        sPlayerRumbleTime[snd] = 0.0f;
    }

    for (S32 snd = 0; snd < ePlayerSnd_Total; snd++)
    {
        sPlayerSndFxVolume[snd] = 0.77f;
    }
    sPlayerSndFxVolume[2] *= 0.23f;
    sPlayerSndFxVolume[3] *= 0.25f;
    sPlayerSndFxVolume[0x2d] *= 0.4f;
    sPlayerSndFxVolume[0x18] *= 0.4f;
    sPlayerSndFxVolume[0x2e] *= 0.4f;
    sPlayerSndFxVolume[4] *= 0.7f;
    sPlayerSndFxVolume[5] *= 0.7f;
    sPlayerSndFxVolume[7] *= 0.7f;
    sPlayerSndFxVolume[6] *= 0.7f;
    sPlayerSndFxVolume[0xd] *= 0.7f;
    sPlayerSndFxVolume[0x11] *= 0.7f;
    sPlayerSndFxVolume[0x17] *= 0.7f;
    sPlayerSndFxVolume[9] *= 0.45f;
    sPlayerSndFxVolume[8] *= 0.5f;
    sPlayerSndFxVolume[0x15] *= 0.6f;
    sPlayerSndFxVolume[0x16] *= 0.6f;

    for (S32 snd = 0; snd < ePlayerStreamSnd_Total; snd++)
    {
        sPlayerSndStreamVolume[snd] = 0.65f;
    }
    sPlayerSndStreamVolume[ePlayerStreamSnd_SpatulaComment1] *= 0.8f;

    sPlayerSnd[eCurrentPlayerSpongeBob][1] = xStrHash("generic_land");
    sPlayerSnd[eCurrentPlayerSpongeBob][2] = xStrHash("SB_jump_sngl");
    sPlayerSnd[eCurrentPlayerSpongeBob][3] = xStrHash("SB_jump_dub");
    sPlayerSnd[eCurrentPlayerSpongeBob][4] = xStrHash("SB_bowl_windup_loop");
    sPlayerSnd[eCurrentPlayerSpongeBob][5] = xStrHash("SB_bowl_release");
    sPlayerSnd[eCurrentPlayerSpongeBob][6] = xStrHash("SB_bounce_start");
    sPlayerSnd[eCurrentPlayerSpongeBob][7] = xStrHash("SB_bounce_hit1");
    sPlayerSnd[eCurrentPlayerSpongeBob][8] = xStrHash("SB_bounce_hit2");
    sPlayerSnd[eCurrentPlayerSpongeBob][9] = xStrHash("SB_Bubble_wand");
    sPlayerSnd[eCurrentPlayerSpongeBob][10] = xStrHash("SB_cruise_start");
    sPlayerSnd[eCurrentPlayerSpongeBob][0xb] = xStrHash("SB_cruise_nav_loop");
    sPlayerSnd[eCurrentPlayerSpongeBob][0xc] = xStrHash("SB_cruise_hit");
    sPlayerSnd[eCurrentPlayerSpongeBob][0xd] = xStrHash("SB_bounce_hit2");
    sPlayerSnd[eCurrentPlayerSpongeBob][0xe] = xStrHash("SB_wetball_start");
    sPlayerSnd[eCurrentPlayerSpongeBob][0xf] = 0;
    sPlayerSnd[eCurrentPlayerSpongeBob][0x10] = xStrHash("SB_wetball_end");
    sPlayerSnd[eCurrentPlayerSpongeBob][0x11] = 0;
    sPlayerSnd[eCurrentPlayerSpongeBob][0x12] = 0;
    sPlayerSnd[eCurrentPlayerSpongeBob][0x13] = 0;
    sPlayerSnd[eCurrentPlayerSpongeBob][0x14] = 0;
    sPlayerSnd[eCurrentPlayerSpongeBob][0x15] = 0;
    sPlayerSnd[eCurrentPlayerSpongeBob][0x16] = 0;
    sPlayerSnd[eCurrentPlayerSpongeBob][0x17] = 0;
    sPlayerSnd[eCurrentPlayerSpongeBob][0x18] = 0;
    sPlayerSnd[eCurrentPlayerSpongeBob][0x19] = 0;
    sPlayerSnd[eCurrentPlayerSpongeBob][0x1b] = xStrHash("SB_Ouch1");
    sPlayerSnd[eCurrentPlayerSpongeBob][0x1c] = xStrHash("SB_Ouch2");
    sPlayerSnd[eCurrentPlayerSpongeBob][0x1d] = xStrHash("SB_Ouch3");
    sPlayerSnd[eCurrentPlayerSpongeBob][0x1e] = xStrHash("SB_Ouch4");
    sPlayerSnd[eCurrentPlayerSpongeBob][0x1f] = xStrHash("SB_Death");
    sPlayerSnd[eCurrentPlayerSpongeBob][0x20] = 0;
    sPlayerSnd[eCurrentPlayerSpongeBob][0x21] = 0;
    sPlayerSnd[eCurrentPlayerSpongeBob][0x22] = xStrHash("GSG_pickup");
    sPlayerSnd[eCurrentPlayerSpongeBob][0x23] = xStrHash("SB_undies");
    sPlayerSnd[eCurrentPlayerSpongeBob][0x24] = xStrHash("Bus_all");
    sPlayerSnd[eCurrentPlayerSpongeBob][0x25] = xStrHash("Bus_whistle");
    sPlayerSnd[eCurrentPlayerSpongeBob][0x26] = xStrHash("SB_tb_loop");
    sPlayerSnd[eCurrentPlayerSpongeBob][0x2a] = xStrHash("gspatula_sb");
    sPlayerSnd[eCurrentPlayerSpongeBob][0x2d] = xStrHash("Xylo_sneaky_loop");
    sPlayerSnd[eCurrentPlayerSpongeBob][0x2e] = xStrHash("SB_slip_idle_loop");

    sPlayerStreamSnd[eCurrentPlayerSpongeBob][1] = xStrHash("SBG01079");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][2] = xStrHash("SBG01080");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][3] = xStrHash("SBG01081");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][4] = xStrHash("SBG01022");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][5] = xStrHash("SBG01022");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][6] = xStrHash("SBG01023");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][7] = xStrHash("SBG01066_c");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][8] = xStrHash("SBG01066_a");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][9] = xStrHash("SBG01067");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][10] = xStrHash("SBG01068_a");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0xb] = xStrHash("SBG01068_d");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0xc] = xStrHash("SBG01070");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0xd] = xStrHash("SBG01069");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0xe] = xStrHash("B101_SB_win");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0xf] = xStrHash("SBG01017_a");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x10] = xStrHash("SBG01017_b");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x11] = xStrHash("SBG01018");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x12] = xStrHash("SBG01016");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x13] = xStrHash("SBG01019");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x14] = xStrHash("SBG01076");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x15] = xStrHash("SBG01077");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x16] = xStrHash("SBG01078");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x17] = xStrHash("SBG01060_c");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x18] = xStrHash("SBG01058_b");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x19] = xStrHash("SBG01054_b");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x1a] = xStrHash("SBG01055_d");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x1b] = xStrHash("SBG01056_d");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x1c] = xStrHash("SBG01057_c");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x1d] = xStrHash("SBG01057_a");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x1e] = xStrHash("SBG01056_a");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x1f] = xStrHash("SBG01030");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x20] = xStrHash("SBG01030");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x21] = xStrHash("SBG01030");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x22] = 0;
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x23] = 0;
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x24] = 0;
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x25] = 0;
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x26] = 0;
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x27] = 0;
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x28] = 0;
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x29] = 0;
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x2a] = 0;
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x2b] = 0;
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x2c] = 0;
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x2d] = 0;
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x2e] = 0;
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x2f] = 0;
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x30] = 0;
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x31] = 0;
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x32] = 0;
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x33] = 0;
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x34] = 0;
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x35] = 0;
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x36] = 0;
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x37] = xStrHash("SBG01091");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x38] = xStrHash("SBG01092");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x39] = xStrHash("SBG01093_a");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x3a] = xStrHash("SBG01094_b");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x3b] = xStrHash("SBG01095");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x3c] = xStrHash("SBG01096");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x3d] = xStrHash("SBG01097");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x3e] = xStrHash("SBG01098");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x3f] = xStrHash("SBG01099");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x40] = xStrHash("SBG01100");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x41] = xStrHash("SBG01101");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x42] = xStrHash("SBG01102");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x43] = xStrHash("SBG01103");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x44] = xStrHash("SBG01104");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x45] = xStrHash("SBG01105");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x46] = xStrHash("SBG01106");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x47] = xStrHash("SBG01107");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x48] = xStrHash("SBG01108");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x49] = xStrHash("SBG01109");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x4a] = xStrHash("SBG01109");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x4b] = xStrHash("SBG01110");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x4c] = xStrHash("SBG01111");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x4d] = xStrHash("SBG01021");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x4e] = xStrHash("SBG01021");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x4f] = xStrHash("SBG01024");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x50] = xStrHash("SBG01025");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x51] = xStrHash("SBG01026");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x52] = xStrHash("SBG01086_a");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x53] = xStrHash("SBG01086_b");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x54] = xStrHash("SBG01085_a");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x55] = xStrHash("SBG01084_a");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x56] = xStrHash("SBG01082_a");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x57] = xStrHash("SBG01059_a");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x58] = xStrHash("SBG01083_a");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x5d] = xStrHash("SBG01041_b");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x5e] = xStrHash("SBG01042_c");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x5f] = xStrHash("SBG01043_c");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x60] = xStrHash("SBG01044_b");

    sPlayerSnd[eCurrentPlayerPatrick][1] = xStrHash("generic_land");
    sPlayerSnd[eCurrentPlayerPatrick][2] = xStrHash("Pat_jump_sngl");
    sPlayerSnd[eCurrentPlayerPatrick][3] = xStrHash("Pat_jump_dub");
    sPlayerSnd[eCurrentPlayerPatrick][4] = 0;
    sPlayerSnd[eCurrentPlayerPatrick][5] = 0;
    sPlayerSnd[eCurrentPlayerPatrick][6] = 0;
    sPlayerSnd[eCurrentPlayerPatrick][7] = 0;
    sPlayerSnd[eCurrentPlayerPatrick][8] = 0;
    sPlayerSnd[eCurrentPlayerPatrick][9] = 0;
    sPlayerSnd[eCurrentPlayerPatrick][10] = 0;
    sPlayerSnd[eCurrentPlayerPatrick][0xb] = 0;
    sPlayerSnd[eCurrentPlayerPatrick][0xc] = 0;
    sPlayerSnd[eCurrentPlayerPatrick][0xd] = 0;
    sPlayerSnd[eCurrentPlayerPatrick][0xe] = 0;
    sPlayerSnd[eCurrentPlayerPatrick][0xf] = 0;
    sPlayerSnd[eCurrentPlayerPatrick][0x10] = 0;
    sPlayerSnd[eCurrentPlayerPatrick][0x11] = xStrHash("Pat_belly");
    sPlayerSnd[eCurrentPlayerPatrick][0x12] = xStrHash("Pat_smash_belly");
    sPlayerSnd[eCurrentPlayerPatrick][0x13] = xStrHash("Pat_lift3B");
    sPlayerSnd[eCurrentPlayerPatrick][0x14] = xStrHash("Pat_throw");
    sPlayerSnd[eCurrentPlayerPatrick][0x15] = 0;
    sPlayerSnd[eCurrentPlayerPatrick][0x16] = 0;
    sPlayerSnd[eCurrentPlayerPatrick][0x17] = 0;
    sPlayerSnd[eCurrentPlayerPatrick][0x18] = 0;
    sPlayerSnd[eCurrentPlayerPatrick][0x19] = 0;
    sPlayerSnd[eCurrentPlayerPatrick][0x1b] = xStrHash("Pat_Ouch1");
    sPlayerSnd[eCurrentPlayerPatrick][0x1c] = xStrHash("Pat_Ouch2");
    sPlayerSnd[eCurrentPlayerPatrick][0x1d] = xStrHash("Pat_Ouch3");
    sPlayerSnd[eCurrentPlayerPatrick][0x1e] = xStrHash("Pat_Ouch4");
    sPlayerSnd[eCurrentPlayerPatrick][0x1f] = xStrHash("SB_Death");
    sPlayerSnd[eCurrentPlayerPatrick][0x20] = xStrHash("Ffruit_crackle");
    sPlayerSnd[eCurrentPlayerPatrick][0x21] = 0;
    sPlayerSnd[eCurrentPlayerPatrick][0x22] = xStrHash("GSG_pickup");
    sPlayerSnd[eCurrentPlayerPatrick][0x23] = xStrHash("SB_undies");
    sPlayerSnd[eCurrentPlayerPatrick][0x24] = xStrHash("bus_all");
    sPlayerSnd[eCurrentPlayerPatrick][0x25] = xStrHash("Bus_whistle");
    sPlayerSnd[eCurrentPlayerPatrick][0x26] = xStrHash("Pat_slide_loop");
    sPlayerSnd[eCurrentPlayerPatrick][0x2a] = xStrHash("gspatula_pat");
    sPlayerSnd[eCurrentPlayerPatrick][0x2d] = 0;

    sPlayerStreamSnd[eCurrentPlayerPatrick][1] = xStrHash("PSGB01028_a");
    sPlayerStreamSnd[eCurrentPlayerPatrick][2] = xStrHash("PSGB01029_a");
    sPlayerStreamSnd[eCurrentPlayerPatrick][3] = xStrHash("PSGB01030_a");
    sPlayerStreamSnd[eCurrentPlayerPatrick][4] = xStrHash("PSGB01013_b");
    sPlayerStreamSnd[eCurrentPlayerPatrick][5] = xStrHash("PSGB01014_b");
    sPlayerStreamSnd[eCurrentPlayerPatrick][6] = xStrHash("PSGB01015_b");
    sPlayerStreamSnd[eCurrentPlayerPatrick][7] = xStrHash("PSGB01077_c");
    sPlayerStreamSnd[eCurrentPlayerPatrick][8] = xStrHash("PSGB01076_a");
    sPlayerStreamSnd[eCurrentPlayerPatrick][9] = xStrHash("PSGB01078_c");
    sPlayerStreamSnd[eCurrentPlayerPatrick][10] = xStrHash("PSGB01078_b");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0xb] = xStrHash("PSGB01077_b");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0xc] = xStrHash("PSGB01075_a");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0xd] = xStrHash("PSGB01074_d");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0xe] = xStrHash("B101_Pat_win");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0xf] = xStrHash("PSGB01010_a");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x10] = xStrHash("PSGB01011_a");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x11] = xStrHash("PSGB01011_c");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x12] = xStrHash("PSGB01012_c");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x13] = xStrHash("PSGB01036_c");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x14] = 0;
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x15] = 0;
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x16] = 0;
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x17] = 0;
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x18] = 0;
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x19] = 0;
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x1a] = 0;
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x1b] = 0;
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x1c] = 0;
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x1f] = xStrHash("PSGB01016_a");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x20] = xStrHash("PSGB01017_b");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x21] = xStrHash("PSGB01018_a");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x22] = xStrHash("PSGB01066_a");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x23] = xStrHash("PSGB01067_a");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x24] = xStrHash("PSGB01068_a");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x25] = 0;
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x26] = 0;
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x27] = 0;
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x28] = 0;
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x29] = 0;
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x2a] = 0;
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x2b] = 0;
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x2c] = 0;
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x2d] = 0;
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x2e] = 0;
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x2f] = 0;
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x30] = 0;
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x31] = xStrHash("PSGB01037_a");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x32] = xStrHash("PSGB01038_a");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x33] = xStrHash("PSGB01039_a");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x34] = xStrHash("PSGB01040_a");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x35] = xStrHash("PSGB01041_a");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x36] = xStrHash("PSGB01042_a");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x37] = xStrHash("PSGB01088_a");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x38] = xStrHash("PSGB01089_a");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x39] = xStrHash("PSGB01090_a");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x3a] = xStrHash("PSGB01091");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x3b] = xStrHash("PSGB01092");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x3c] = xStrHash("PSGB01093");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x3d] = xStrHash("PSGB01094");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x3e] = xStrHash("PSGB01095_a");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x3f] = xStrHash("PSGB01096");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x40] = xStrHash("PSGB01097");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x41] = xStrHash("PSGB01098");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x42] = xStrHash("PSGB01099");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x43] = xStrHash("PSGB01100");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x44] = xStrHash("PSGB01101");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x45] = xStrHash("PSGB01102");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x46] = xStrHash("PSGB01103");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x47] = xStrHash("PSGB01104");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x48] = xStrHash("PSGB01105");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x49] = xStrHash("PSGB01106");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x4a] = xStrHash("PSGB01107");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x4b] = 0;
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x4c] = xStrHash("PSGB01108");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x4d] = xStrHash("PSGB01029B");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x4e] = xStrHash("PSGB01034_a");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x4f] = 0;
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x50] = 0;
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x51] = 0;
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x52] = xStrHash("PSGB01037_a");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x53] = xStrHash("PSGB01037_d");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x54] = xStrHash("PSGB01040_a");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x55] = xStrHash("PSGB01040_b");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x56] = xStrHash("PSGB01040_a");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x57] = xStrHash("PSGB01039_a");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x58] = xStrHash("PSGB01041_a");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x5c] = xStrHash("PSGB01033_c");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x5d] = xStrHash("PSGB01052_b");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x5e] = xStrHash("PSGB01053_b");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x5f] = xStrHash("PSGB01054_b");
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x60] = xStrHash("PSGB01055_c");

    sPlayerSnd[eCurrentPlayerSandy][1] = xStrHash("generic_land");
    sPlayerSnd[eCurrentPlayerSandy][2] = xStrHash("SC_jump_sngl");
    sPlayerSnd[eCurrentPlayerSandy][3] = xStrHash("SC_jump_dub");
    sPlayerSnd[eCurrentPlayerSandy][4] = 0;
    sPlayerSnd[eCurrentPlayerSandy][5] = 0;
    sPlayerSnd[eCurrentPlayerSandy][6] = 0;
    sPlayerSnd[eCurrentPlayerSandy][7] = 0;
    sPlayerSnd[eCurrentPlayerSandy][8] = 0;
    sPlayerSnd[eCurrentPlayerSandy][9] = 0;
    sPlayerSnd[eCurrentPlayerSandy][10] = 0;
    sPlayerSnd[eCurrentPlayerSandy][0xb] = 0;
    sPlayerSnd[eCurrentPlayerSandy][0xc] = 0;
    sPlayerSnd[eCurrentPlayerSandy][0xd] = 0;
    sPlayerSnd[eCurrentPlayerSandy][0xe] = 0;
    sPlayerSnd[eCurrentPlayerSandy][0xf] = 0;
    sPlayerSnd[eCurrentPlayerSandy][0x10] = 0;
    sPlayerSnd[eCurrentPlayerSandy][0x11] = 0;
    sPlayerSnd[eCurrentPlayerSandy][0x12] = 0;
    sPlayerSnd[eCurrentPlayerSandy][0x13] = 0;
    sPlayerSnd[eCurrentPlayerSandy][0x14] = 0;
    sPlayerSnd[eCurrentPlayerSandy][0x15] = xStrHash("SC_chop");
    sPlayerSnd[eCurrentPlayerSandy][0x16] = xStrHash("SC_kick");
    sPlayerSnd[eCurrentPlayerSandy][0x17] = xStrHash("SC_heli_loop");
    sPlayerSnd[eCurrentPlayerSandy][0x18] = xStrHash("SC_lasso_throw");
    sPlayerSnd[eCurrentPlayerSandy][0x19] = xStrHash("SC_lasso_stretch");
    sPlayerSnd[eCurrentPlayerSandy][0x1b] = xStrHash("SC_Ouch1");
    sPlayerSnd[eCurrentPlayerSandy][0x1c] = xStrHash("SC_Ouch2");
    sPlayerSnd[eCurrentPlayerSandy][0x1d] = xStrHash("SC_Ouch3");
    sPlayerSnd[eCurrentPlayerSandy][0x1e] = xStrHash("SC_Ouch4");
    sPlayerSnd[eCurrentPlayerSandy][0x1f] = xStrHash("SB_Death");
    sPlayerSnd[eCurrentPlayerSandy][0x20] = 0;
    sPlayerSnd[eCurrentPlayerSandy][0x21] = 0;
    sPlayerSnd[eCurrentPlayerSandy][0x22] = xStrHash("GSG_pickup");
    sPlayerSnd[eCurrentPlayerSandy][0x23] = xStrHash("SC_undies");
    sPlayerSnd[eCurrentPlayerSandy][0x24] = xStrHash("bus_all");
    sPlayerSnd[eCurrentPlayerSandy][0x25] = xStrHash("Bus_whistle");
    sPlayerSnd[eCurrentPlayerSandy][0x26] = xStrHash("SC_slide_loop");
    sPlayerSnd[eCurrentPlayerSandy][0x2a] = xStrHash("gspatula_sandy");
    sPlayerSnd[eCurrentPlayerSandy][0x2d] = 0;

    sPlayerStreamSnd[eCurrentPlayerSandy][1] = xStrHash("SCGB01028A");
    sPlayerStreamSnd[eCurrentPlayerSandy][2] = xStrHash("SCGB01029B");
    sPlayerStreamSnd[eCurrentPlayerSandy][3] = xStrHash("SCGB01029B");
    sPlayerStreamSnd[eCurrentPlayerSandy][4] = xStrHash("SCGB01013C");
    sPlayerStreamSnd[eCurrentPlayerSandy][5] = xStrHash("SCGB01014A");
    sPlayerStreamSnd[eCurrentPlayerSandy][6] = xStrHash("SCGB01015A");
    sPlayerStreamSnd[eCurrentPlayerSandy][7] = xStrHash("SCGB01071B");
    sPlayerStreamSnd[eCurrentPlayerSandy][8] = xStrHash("SCGB01072A");
    sPlayerStreamSnd[eCurrentPlayerSandy][9] = xStrHash("SCGB01073A");
    sPlayerStreamSnd[eCurrentPlayerSandy][10] = xStrHash("SCGB01069B");
    sPlayerStreamSnd[eCurrentPlayerSandy][0xb] = xStrHash("SCGB01074A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0xc] = xStrHash("SCGB01067A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0xd] = xStrHash("SCGB01070B");
    sPlayerStreamSnd[eCurrentPlayerSandy][0xe] = xStrHash("B101_San_win");
    sPlayerStreamSnd[eCurrentPlayerSandy][0xf] = xStrHash("SCGB01010A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x10] = xStrHash("SCGB01011A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x11] = xStrHash("SCGB01011B");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x12] = xStrHash("SCGB01012C");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x13] = xStrHash("SCGB01012A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x14] = 0;
    sPlayerStreamSnd[eCurrentPlayerSandy][0x15] = 0;
    sPlayerStreamSnd[eCurrentPlayerSandy][0x16] = 0;
    sPlayerStreamSnd[eCurrentPlayerSandy][0x17] = 0;
    sPlayerStreamSnd[eCurrentPlayerSandy][0x18] = 0;
    sPlayerStreamSnd[eCurrentPlayerSandy][0x19] = 0;
    sPlayerStreamSnd[eCurrentPlayerSandy][0x1a] = 0;
    sPlayerStreamSnd[eCurrentPlayerSandy][0x1b] = 0;
    sPlayerStreamSnd[eCurrentPlayerSandy][0x1c] = 0;
    sPlayerStreamSnd[eCurrentPlayerSandy][0x1f] = xStrHash("SCGB01016A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x20] = xStrHash("SCGB01017A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x21] = xStrHash("SCGB01018A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x22] = 0;
    sPlayerStreamSnd[eCurrentPlayerSandy][0x23] = 0;
    sPlayerStreamSnd[eCurrentPlayerSandy][0x24] = 0;
    sPlayerStreamSnd[eCurrentPlayerSandy][0x25] = xStrHash("SCGB01022A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x26] = xStrHash("SCGB01023A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x27] = xStrHash("SCGB01024A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x28] = xStrHash("SCGB01025A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x29] = xStrHash("SCGB01026A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x2a] = xStrHash("SCGB01027A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x2b] = xStrHash("SCGB01059A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x2c] = xStrHash("SCGB01060A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x2d] = xStrHash("SCGB01061A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x2e] = xStrHash("SCGB01062A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x2f] = xStrHash("SCGB01063A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x30] = xStrHash("SCGB01064A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x31] = xStrHash("SCGB01040A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x32] = xStrHash("SCGB01041A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x33] = xStrHash("SCGB01042A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x34] = xStrHash("SCGB01043A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x35] = xStrHash("SCGB01044A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x36] = xStrHash("SCGB01045A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x37] = xStrHash("SCGB01079A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x38] = xStrHash("SCGB01080A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x39] = xStrHash("SCGB01081A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x3a] = xStrHash("SCGB01082A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x3b] = xStrHash("SCGB01083A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x3c] = xStrHash("SCGB01084A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x3d] = xStrHash("SCGB01085A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x3e] = xStrHash("SCGB01086A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x3f] = xStrHash("SCGB01087A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x40] = xStrHash("SCGB01088A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x41] = xStrHash("SCGB01089A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x42] = xStrHash("SCGB01090A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x43] = xStrHash("SCGB01091A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x44] = xStrHash("SCGB01092A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x45] = xStrHash("SCGB01093A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x46] = xStrHash("SCGB01094A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x47] = xStrHash("SCGB01095A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x48] = xStrHash("SCGB01096A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x49] = xStrHash("SCGB01097A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x4a] = xStrHash("SCGB01097A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x4b] = xStrHash("SCGB01098A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x4c] = xStrHash("SCGB01099A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x4d] = xStrHash("SCGB01030A");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x4e] = xStrHash("SCGB01030B");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x4f] = 0;
    sPlayerStreamSnd[eCurrentPlayerSandy][0x50] = 0;
    sPlayerStreamSnd[eCurrentPlayerSandy][0x51] = 0;
    sPlayerStreamSnd[eCurrentPlayerSandy][0x52] = xStrHash("SCGB01040a");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x53] = xStrHash("SCGB01040b");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x54] = xStrHash("SCGB01044a");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x55] = xStrHash("SCGB01045a");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x56] = xStrHash("SCGB01045b");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x57] = xStrHash("SCGB01066C");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x58] = xStrHash("SCGB01043a");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x5d] = xStrHash("SCGB01054a");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x5e] = xStrHash("SCGB01056b");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x5f] = xStrHash("SCGB01057a");
    sPlayerStreamSnd[eCurrentPlayerSandy][0x60] = xStrHash("SCGB01058a");

    sPlayerSnd[eCurrentPlayerSpongeBob][0x27] = xStrHash("wind2loud_loop");
    sPlayerSnd[eCurrentPlayerSpongeBob][0x28] = xStrHash("SB_bungee1");
    sPlayerSnd[eCurrentPlayerSpongeBob][0x29] = xStrHash("SB_bungee2");

    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x61] = xStrHash("SBG01008");

    sPlayerSnd[eCurrentPlayerSpongeBob][0x2b] = xStrHash("SBG01003_a");
    sPlayerSnd[eCurrentPlayerSpongeBob][0x2c] = xStrHash("SBG01003_b");

    sPlayerStreamSnd[eCurrentPlayerSpongeBob][100] = xStrHash("SBG01041_a");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x65] = xStrHash("SBG01041_b");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x66] = xStrHash("SBG01041_c");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x67] = xStrHash("SBG01042_a");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x68] = xStrHash("SBG01042_b");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x69] = xStrHash("SBG01042_c");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x6a] = xStrHash("SBG01043_a");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x6b] = xStrHash("SBG01043_b");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x6c] = xStrHash("SBG01043_c");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x6d] = xStrHash("SBG01044_a");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x6e] = xStrHash("SBG01044_b");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x6f] = xStrHash("SBG01044_c");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x70] = xStrHash("SBG01045_a");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x71] = xStrHash("SBG01045_b");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x72] = xStrHash("SBG01045_c");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x73] = xStrHash("SBG01046_a");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x74] = xStrHash("SBG01046_b");
    sPlayerStreamSnd[eCurrentPlayerSpongeBob][0x75] = xStrHash("SBG01046_c");

    sPlayerSnd[eCurrentPlayerPatrick][0x27] = 0;
    sPlayerSnd[eCurrentPlayerPatrick][0x28] = 0;
    sPlayerSnd[eCurrentPlayerPatrick][0x29] = 0;

    sPlayerStreamSnd[eCurrentPlayerPatrick][0x27] = 0;
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x28] = 0;
    sPlayerStreamSnd[eCurrentPlayerPatrick][0x29] = 0;

    sPlayerSnd[eCurrentPlayerSandy][0x27] = 0;
    sPlayerSnd[eCurrentPlayerSandy][0x28] = 0;
    sPlayerSnd[eCurrentPlayerSandy][0x29] = 0;

    sPlayerStreamSnd[eCurrentPlayerSandy][0x27] = 0;
    sPlayerStreamSnd[eCurrentPlayerSandy][0x28] = 0;
    sPlayerStreamSnd[eCurrentPlayerSandy][0x29] = 0;

    sPlayerStreamSndTimer[1].time = 240.0f;
    sPlayerStreamSndTimer[2].time = 240.0f;
    sPlayerStreamSndTimer[3].time = 240.0f;
    sPlayerStreamSndTimer[4].time = 240.0f;
    sPlayerStreamSndTimer[5].time = 240.0f;
    sPlayerStreamSndTimer[6].time = 240.0f;
    sPlayerStreamSndTimer[7].time = 120.0f;
    sPlayerStreamSndTimer[8].time = 120.0f;
    sPlayerStreamSndTimer[9].time = 120.0f;
    sPlayerStreamSndTimer[10].time = 120.0f;
    sPlayerStreamSndTimer[0xb].time = 120.0f;
    sPlayerStreamSndTimer[0xc].time = 120.0f;
    sPlayerStreamSndTimer[0xe].time = 0.0f;
    sPlayerStreamSndTimer[0xf].time = 240.0f;
    sPlayerStreamSndTimer[0x10].time = 240.0f;
    sPlayerStreamSndTimer[0x11].time = 240.0f;
    sPlayerStreamSndTimer[0x12].time = 240.0f;
    sPlayerStreamSndTimer[0x13].time = 30.0f;
    sPlayerStreamSndTimer[0x14].time = 240.0f;
    sPlayerStreamSndTimer[0x15].time = 240.0f;
    sPlayerStreamSndTimer[0x16].time = 240.0f;
    sPlayerStreamSndTimer[0x17].time = 0.0f;
    sPlayerStreamSndTimer[0x18].time = 0.0f;
    sPlayerStreamSndTimer[0x19].time = 0.0f;
    sPlayerStreamSndTimer[0x1a].time = 240.0f;
    sPlayerStreamSndTimer[0x1b].time = 240.0f;
    sPlayerStreamSndTimer[0x1c].time = 240.0f;
    sPlayerStreamSndTimer[0x1d].time = 240.0f;
    sPlayerStreamSndTimer[0x1e].time = 240.0f;
    sPlayerStreamSndTimer[0x1f].time = 90.0f;
    sPlayerStreamSndTimer[0x20].time = 90.0f;
    sPlayerStreamSndTimer[0x21].time = 90.0f;
    sPlayerStreamSndTimer[0x22].time = 30.0f;
    sPlayerStreamSndTimer[0x23].time = 30.0f;
    sPlayerStreamSndTimer[0x24].time = 30.0f;
    sPlayerStreamSndTimer[0x25].time = 20.0f;
    sPlayerStreamSndTimer[0x26].time = 20.0f;
    sPlayerStreamSndTimer[0x27].time = 20.0f;
    sPlayerStreamSndTimer[0x28].time = 20.0f;
    sPlayerStreamSndTimer[0x29].time = 20.0f;
    sPlayerStreamSndTimer[0x2a].time = 20.0f;
    sPlayerStreamSndTimer[0x2b].time = 20.0f;
    sPlayerStreamSndTimer[0x2c].time = 20.0f;
    sPlayerStreamSndTimer[0x2d].time = 20.0f;
    sPlayerStreamSndTimer[0x2e].time = 60.0f;
    sPlayerStreamSndTimer[0x2f].time = 60.0f;
    sPlayerStreamSndTimer[0x30].time = 60.0f;
    sPlayerStreamSndTimer[0x31].time = 360.0f;
    sPlayerStreamSndTimer[0x32].time = 360.0f;
    sPlayerStreamSndTimer[0x33].time = 360.0f;
    sPlayerStreamSndTimer[0x34].time = 360.0f;
    sPlayerStreamSndTimer[0x35].time = 360.0f;
    sPlayerStreamSndTimer[0x36].time = 360.0f;
    sPlayerStreamSndTimer[0x37].time = 0.0f;
    sPlayerStreamSndTimer[0x38].time = 0.0f;
    sPlayerStreamSndTimer[0x39].time = 0.0f;
    sPlayerStreamSndTimer[0x3a].time = 0.0f;
    sPlayerStreamSndTimer[0x3b].time = 0.0f;
    sPlayerStreamSndTimer[0x3c].time = 240.0f;
    sPlayerStreamSndTimer[0x3d].time = 240.0f;
    sPlayerStreamSndTimer[0x3e].time = 240.0f;
    sPlayerStreamSndTimer[0x3f].time = 240.0f;
    sPlayerStreamSndTimer[0x40].time = 240.0f;
    sPlayerStreamSndTimer[0x41].time = 240.0f;
    sPlayerStreamSndTimer[0x42].time = 240.0f;
    sPlayerStreamSndTimer[0x43].time = 240.0f;
    sPlayerStreamSndTimer[0x44].time = 240.0f;
    sPlayerStreamSndTimer[0x45].time = 0.0f;
    sPlayerStreamSndTimer[0x46].time = 0.0f;
    sPlayerStreamSndTimer[0x47].time = 0.0f;
    sPlayerStreamSndTimer[0x48].time = 0.0f;
    sPlayerStreamSndTimer[0x49].time = 0.0f;
    sPlayerStreamSndTimer[0x4a].time = 0.0f;
    sPlayerStreamSndTimer[0x4b].time = 0.0f;
    sPlayerStreamSndTimer[0x4c].time = 0.0f;
    sPlayerStreamSndTimer[0x4d].time = 0.0f;
    sPlayerStreamSndTimer[0x4e].time = 0.0f;
    sPlayerStreamSndTimer[0x52].time = 30.0f;
    sPlayerStreamSndTimer[0x53].time = 30.0f;
    sPlayerStreamSndTimer[0x54].time = 30.0f;
    sPlayerStreamSndTimer[0x55].time = 30.0f;
    sPlayerStreamSndTimer[0x56].time = 30.0f;
    sPlayerStreamSndTimer[0x57].time = 30.0f;
    sPlayerStreamSndTimer[0x58].time = 30.0f;
    sPlayerStreamSndTimer[0x5c].time = 520.0f;
    sPlayerStreamSndTimer[0x5d].time = 360.0f;
    sPlayerStreamSndTimer[0x5e].time = 360.0f;
    sPlayerStreamSndTimer[0x5f].time = 360.0f;
    sPlayerStreamSndTimer[0x60].time = 360.0f;

    F32 minutes =
        globals.player.Inv_Spatula ? 2.0f * xlog(globals.player.Inv_Spatula) - 3.5f : 0.0f;
    if (minutes <= 0.0f)
    {
        minutes = 0.0f;
    }

    minutes *= 60.0f;

    for (S32 i = 0; i < ePlayerStreamSnd_Total; i++)
    {
        if (i >= ePlayerStreamSnd_Combo4 && i <= ePlayerStreamSnd_BigCombo2)
        {
            break;
        }
        if (sPlayerStreamSndTimer[i].time > 0.0f)
        {
            sPlayerStreamSndTimer[i].time += minutes;
        }
        sPlayerStreamSndTimer[i].timer = 0.3f * sPlayerStreamSndTimer[i].time;
    }

    _SetupRumble(ePlayerSnd_Jump, eRumble_Off, 0.0f);
    _SetupRumble(ePlayerSnd_DoubleJump, eRumble_Off, 0.0f);
    _SetupRumble(ePlayerSnd_BowlWindup, eRumble_Off, 0.0f);
    _SetupRumble(ePlayerSnd_BowlRelease, eRumble_Light, 0.1f);
    _SetupRumble(ePlayerSnd_BubbleBashStart, eRumble_Off, 0.0f);
    _SetupRumble(ePlayerSnd_BubbleBashHit1, eRumble_Medium, 0.15f);
    _SetupRumble(ePlayerSnd_BubbleBashHit2, eRumble_Medium, 0.15f);
    _SetupRumble(ePlayerSnd_BubbleWand, eRumble_Off, 0.0f);
    _SetupRumble(ePlayerSnd_CruiseStart, eRumble_Off, 0.0f);
    _SetupRumble(ePlayerSnd_CruiseNavigate, eRumble_Light, 0.0f);
    _SetupRumble(ePlayerSnd_CruiseHit, eRumble_VeryHeavy, 0.5f);
    _SetupRumble(ePlayerSnd_BounceStrike, eRumble_Heavy, 0.2f);
    _SetupRumble(ePlayerSnd_BoulderStart, eRumble_Off, 0.0f);
    _SetupRumble(ePlayerSnd_BoulderRoll, eRumble_Off, 0.0f);
    _SetupRumble(ePlayerSnd_BoulderEnd, eRumble_Off, 0.0f);
    _SetupRumble(ePlayerSnd_BellyMelee, eRumble_Medium, 0.2f);
    _SetupRumble(ePlayerSnd_Lift1, eRumble_Light, 0.15f);
    _SetupRumble(ePlayerSnd_Throw, eRumble_VeryLight, 0.15f);
    _SetupRumble(ePlayerSnd_Chop, eRumble_Off, 0.15f);
    _SetupRumble(ePlayerSnd_Kick, eRumble_Off, 0.15f);
    _SetupRumble(ePlayerSnd_Heli, eRumble_Off, 0.0f);
    _SetupRumble(ePlayerSnd_LassoThrow, eRumble_VeryLight, 0.2f);
    _SetupRumble(ePlayerSnd_LassoYank, eRumble_VeryLight, 0.2f);
    _SetupRumble(ePlayerSnd_Ouch1, eRumble_Medium, 0.4f);
    _SetupRumble(ePlayerSnd_Ouch2, eRumble_Medium, 0.4f);
    _SetupRumble(ePlayerSnd_Ouch3, eRumble_Medium, 0.4f);
    _SetupRumble(ePlayerSnd_Ouch4, eRumble_Medium, 0.4f);
    _SetupRumble(ePlayerSnd_Death, eRumble_Heavy, 1.0f);
    _SetupRumble(ePlayerSnd_FruitCrackle, eRumble_Off, 0.0f);
    _SetupRumble(ePlayerSnd_CheckPoint, eRumble_VeryHeavy, 0.3f);
    _SetupRumble(ePlayerSnd_PickupSpatula, eRumble_Off, 0.0f);
    _SetupRumble(ePlayerSnd_PickupUnderwear, eRumble_Off, 0.0f);
    _SetupRumble(ePlayerSnd_Bus, eRumble_Light, 2.3f);
    _SetupRumble(ePlayerSnd_Taxi, eRumble_VeryLight, 2.3f);
}

// Equivalent: scheduling
void zEntPlayer_SNDPlay(_tagePlayerSnd player_snd, F32 delay)
{
    if (globals.cmgr == NULL && !sPlayerIgnoreSound)
    {
        if (sPlayerSnd[gCurrentPlayer][player_snd])
        {
            sPlayerSndID[gCurrentPlayer][player_snd] =
                xSndPlay(sPlayerSnd[gCurrentPlayer][player_snd], sPlayerSndFxVolume[player_snd],
                         0.0f, 0x80, 0, 0, SND_CAT_GAME, delay);
            if (sPlayerRumbleTime[player_snd] > 0.0f)
            {
                xPadAddRumble(globals.currentActivePad, sPlayerRumbleType[player_snd],
                              sPlayerRumbleTime[player_snd], 1, 0);
            }
        }
    }
}

void zEntPlayer_SNDPlayStream(_tagePlayerStreamSnd player_snd)
{
    zEntPlayer_SNDPlayStream(player_snd, 0);
}

// Not very close, compiler isn't generating data accesses as offsets to a common base of sPlayerSnd like it should
// Possibly equivalent?
void zEntPlayer_SNDPlayStream(_tagePlayerStreamSnd player_snd, U32 flags)
{
    if (globals.cmgr == NULL && !sPlayerIgnoreSound)
    {
        if (!(sPlayerStreamSndTimer[player_snd].timer > 0.0f ||
              xSndIsPlayingByHandle(sCurrentStreamSndID)))
        {
            sCurrentStreamSndID = xSndPlay(sPlayerStreamSnd[gCurrentPlayer][player_snd],
                                           sPlayerSndStreamVolume[player_snd], 0.0f, 0x80, flags, 0,
                                           SND_CAT_GAME, 0.0f);
            sPlayerStreamSndTimer[player_snd].timer = sPlayerStreamSndTimer[player_snd].time;
        }
    }
}

// equivalent: regswap, likely caused by not reloading 0.0f for second compare
void zEntPlayer_SNDPlayDelayed(F32 seconds)
{
    if (globals.cmgr == NULL && !sPlayerIgnoreSound)
    {
        for (S32 i = 0; i < MAX_DELAYED_SOUNDS; i++)
        {
            if (sDelayedSound[i].delay > 0.0f)
            {
                sDelayedSound[i].delay -= seconds;
                if (sDelayedSound[i].delay <= 0.0f)
                {
                    zEntPlayer_SNDPlayStreamRandom(sDelayedSound[i].start, sDelayedSound[i].end,
                                                   0.0f);
                    sDelayedSound[i].start = ePlayerStreamSnd_Invalid;
                    sDelayedSound[i].end = ePlayerStreamSnd_Invalid;
                    sDelayedSound[i].delay = 0.0f;
                }
            }
        }
    }
}

void zEntPlayer_SNDPlayStream(U32 lower, U32 upper, _tagePlayerStreamSnd player_snd, U32 flags)
{
    if (globals.player.Inv_Spatula >= lower && globals.player.Inv_Spatula <= upper)
    {
        zEntPlayer_SNDPlayStream(player_snd, flags);
    }
}

void zEntPlayer_SNDPlayStreamRandom(U32 lower, U32 upper, _tagePlayerStreamSnd player_snd_start,
                                    _tagePlayerStreamSnd player_snd_end, F32 delay)

{
    if (globals.player.Inv_Spatula >= lower && globals.player.Inv_Spatula <= upper)
    {
        zEntPlayer_SNDPlayStreamRandom(player_snd_start, player_snd_end, delay);
    }
}

void zEntPlayer_SNDPlayStreamRandom(_tagePlayerStreamSnd player_snd_start,
                                    _tagePlayerStreamSnd player_snd_end, F32 delay)
{
    if (globals.cmgr == NULL && !NPCC_NPCIsConversing() && !sPlayerIgnoreSound &&
        !xSndIsPlayingByHandle(sCurrentStreamSndID))
    {
        if (delay > 0.0f)
        {
            for (S32 i = 0; i < MAX_DELAYED_SOUNDS; i++)
            {
                if (sDelayedSound[i].delay <= 0.0f)
                {
                    sDelayedSound[i].start = player_snd_start;
                    sDelayedSound[i].end = player_snd_end;
                    sDelayedSound[i].delay = delay;
                    return;
                }
            }
        }
        else
        {
            S32 diff = player_snd_end - player_snd_start + 1;
            for (S32 i = 0; i < diff; i++)
            {
                if (sPlayerStreamSndTimer[player_snd_start + i].timer > 0.0f)
                {
                    return;
                }
            }

            // Unrolls very differently, not sure if correct
            for (S32 i = 0; i < diff; i++)
            {
                sPlayerStreamSndTimer[player_snd_start + i].timer =
                    sPlayerStreamSndTimer[player_snd_start + i].time;
            }

            S32 rand_array[32];
            for (S32 i = 0; i < diff; i++)
            {
                rand_array[i] = player_snd_start + i;
            }

            for (S32 i = 0; i < diff; i++)
            {
                U32 j = (U32)rand() % diff;
                S32 swap = rand_array[i];
                rand_array[i] = rand_array[j];
                rand_array[j] = swap;
            }

            S32 pick_sound = -1;
            if (player_snd_start != player_snd_end)
            {
                S32 first_valid;
                S32 num_valid = 0;
                for (S32 i = 0; i < diff; i++)
                {
                    S32 possible = rand_array[i];
                    if (sPlayerStreamSnd[gCurrentPlayer][possible])
                    {
                        num_valid++;
                        first_valid = possible;
                        if (possible == sPlayerStreamSndRand[gCurrentPlayer][player_snd_start])
                        {
                            continue;
                        }
                        pick_sound = possible;
                        break;
                    }
                }

                if (num_valid == 1)
                {
                    pick_sound = first_valid;
                }
            }
            else
            {
                if (sPlayerStreamSnd[gCurrentPlayer][rand_array[0]])
                {
                    pick_sound = rand_array[0];
                }
            }

            if (pick_sound > 0)
            {
                sPlayerStreamSndRand[gCurrentPlayer][player_snd_start] = pick_sound;
                sCurrentStreamSndID = xSndPlay(sPlayerStreamSnd[gCurrentPlayer][pick_sound],
                                               sPlayerSndStreamVolume[pick_sound], 0.0f, 0x80, 0, 0,
                                               SND_CAT_GAME, 0.0f);
            }
        }
    }
}

void zEntPlayer_SNDPlayRandom(_tagePlayerSnd player_snd_start, _tagePlayerSnd player_snd_end,
                              float delay)
{
    if (globals.cmgr == NULL && !sPlayerIgnoreSound)
    {
        S32 diff = player_snd_end - player_snd_start + 1;
        S32 rand_array[32];
        for (S32 i = 0; i < diff; i++)
        {
            rand_array[i] = player_snd_start + i;
        }

        for (S32 i = 0; i < diff; i++)
        {
            U32 j = xrand() % diff;
            S32 swap = rand_array[i];
            rand_array[i] = rand_array[j];
            rand_array[j] = swap;
        }

        S32 pick_sound = 0;
        for (S32 i = 0; i < diff; i++)
        {
            S32 possible = rand_array[i];
            if (sPlayerSnd[gCurrentPlayer][possible] &&
                possible != sPlayerSndRand[gCurrentPlayer][player_snd_start])
            {
                pick_sound = possible;
                break;
            }
        }

        if (pick_sound > 0)
        {
            sPlayerSndRand[gCurrentPlayer][player_snd_start] = pick_sound;
            U32 returned_snd_id =
                xSndPlay(sPlayerSnd[gCurrentPlayer][pick_sound], sPlayerSndFxVolume[pick_sound],
                         0.0f, 0x80, 0, 0, SND_CAT_GAME, delay);

            if (sPlayerRumbleTime[pick_sound] > 0.0f)
            {
                xPadAddRumble(globals.currentActivePad, sPlayerRumbleType[pick_sound],
                              sPlayerRumbleTime[pick_sound], 1, 0);
            }

            for (S32 i = player_snd_start; i <= player_snd_end; i++)
            {
                sPlayerSndID[gCurrentPlayer][i] = returned_snd_id;
            }
        }
    }
}

void zEntPlayer_SNDSetVol(_tagePlayerSnd player_snd, F32 new_vol)
{
    if (sPlayerSnd[gCurrentPlayer][player_snd])
    {
        xSndSetVol(sPlayerSndID[gCurrentPlayer][player_snd], new_vol);
    }
}

void zEntPlayer_SNDSetPitch(_tagePlayerSnd player_snd, F32 new_pitch)
{
    if (sPlayerSnd[gCurrentPlayer][player_snd])
    {
        xSndSetPitch(sPlayerSndID[gCurrentPlayer][player_snd], new_pitch);
    }
}

void zEntPlayer_SNDStop(_tagePlayerSnd player_snd)
{
    if (sPlayerSnd[gCurrentPlayer][player_snd] && sPlayerSndID[gCurrentPlayer][player_snd])
    {
        xSndStop(sPlayerSndID[gCurrentPlayer][player_snd]);
        sPlayerSndID[gCurrentPlayer][player_snd] = 0;
        if (globals.sceneCur->sceneID != 'MNU3')
        {
            xPadDestroyRumbleChain(globals.currentActivePad);
        }
    }
}

void zEntPlayer_SNDStopStream()
{
    if (xSndIsPlayingByHandle(sCurrentStreamSndID))
    {
        xSndStop(sCurrentStreamSndID);
    }
}

void zEntPlayer_SNDNotifyPlaying(U32 id)
{
    sCurrentStreamSndID = id;
}

static void PlayerBeginCollideNoBupdate(xEnt* ent, xScene*, F32)
{
    U8 idx;

    for (idx = 0; idx < 18; idx++)
    {
        xCollis* coll = &ent->collis->colls[idx];

        coll->flags = 0x1F00;
        coll->optr = NULL;
        coll->mptr = NULL;
        coll->dist = 1e38f;
    }

    ent->collis->idx = 6;
    ent->collis->stat_sidx = 6;
    ent->collis->stat_eidx = 6;
    ent->collis->dyn_sidx = 6;
    ent->collis->dyn_eidx = 6;
    ent->collis->npc_sidx = 6;
    ent->collis->npc_eidx = 6;
    ent->collis->env_sidx = 6;
    ent->collis->env_eidx = 6;
}

static void PlayerCollsDetect(xEnt* ent, xScene* sc, F32 dt)
{
    PlayerBeginCollideNoBupdate(ent, sc, dt);

    if (ent->collis->chk & XENT_COLLTYPE_ENV)
    {
        xEntCollCheckEnv(ent, sc);
    }

    if (ent->collis->chk & XENT_COLLTYPE_NPC)
    {
        xEntCollCheckNPCs(ent, sc, PlayerCollCheckOneVillain);
    }

    if (ent->collis->chk & XENT_COLLTYPE_DYN)
    {
        xEntCollCheckDyns(ent, sc, PlayerCollCheckOneEnt);
    }

    if (ent->collis->chk & XENT_COLLTYPE_STAT)
    {
        xEntCollCheckStats(ent, sc, PlayerCollCheckOneEnt);
    }
}

// checkVec was used in a deadstripped function.
// This function is here to force the symbol to be linked.
void __deadstripped_zEntPlayer()
{
    static xVec3 checkVec[2] = { { 0.0f, 0.0f, 0.3f }, { 0.0f, 0.0f, -0.3f } };
}

S32 _iAnimSKBExtractTranslate(iAnimSKBHeader* skb, U32 bone, xVec3* tran, S32 maxTran);
void _iAnimSKBAdjustTranslate(iAnimSKBHeader* skb, U32 bone, F32* tranStart, F32* tranEnd);

static void PlayerHackFixBbashMiss(xModelInstance* model)
{
    static char* bbstate[4] = { "BbashStart01", "BbashAttack01", "BbashStrike01",
                                "BbashMiss01" };
    static F32 bbadjust[4][2] = { { 0.0f, -0.55f },
                                  { -0.55f, -0.55f },
                                  { -0.55f, -0.55f },
                                  { -0.55f, -0.55f } };
    static F32 bbspeed[4] = { 1.0f, 0.96f, 0.96f, 0.92f };

    S32 i;
    xVec3 tran[2];
    xVec3 tranList[128];
    S32 tranCount;

    for (i = 0; i < 4; i++)
    {
        xAnimState* astate = xAnimTableGetState(model->Anim->Table, bbstate[i]);

        if (astate)
        {
            iAnimSKBHeader* skb = (iAnimSKBHeader*)astate->Data->RawData[0];

            if (!(skb->Flags & 0x80000000))
            {
                if (i == 3)
                {
                    tranCount = _iAnimSKBExtractTranslate(skb, 1, tranList, 128);
                    tranList[0].y += bbadjust[i][0];
                    tranList[tranCount - 1].y += bbadjust[i][1];
                    _iAnimSKBAdjustTranslate(skb, 1, (F32*)tranList,
                                             (F32*)&tranList[tranCount - 1]);
                }
                else
                {
                    tran[0].x = 0.0f;
                    tran[0].y = bbadjust[i][0];
                    tran[0].z = 0.0f;
                    tran[1].x = 0.0f;
                    tran[1].y = bbadjust[i][1];
                    tran[1].z = 0.0f;
                    _iAnimSKBAdjustTranslate(skb, 1, (F32*)&tran[0], (F32*)&tran[1]);
                }

                skb->Flags |= 0x80000000;
            }

            astate->Speed = bbspeed[i];
        }
    }
}

static void PlayerLedgeInit(zLedgeGrabParams* ledge, xModelInstance* model)
{
    S32 i;
    xVec3 tran[64];
    xQuat quat[64];

    xAnimState* idle = model->Anim->Table->StateList;
    xAnimState* grab = xAnimTableGetState(model->Anim->Table, "LedgeGrab01");

    if (grab && !(grab->UserFlags & 0x40000000))
    {
        if (!(((iAnimSKBHeader*)grab->Data->RawData[0])->Flags & 0x80000000))
        {
            ledge->tranCount = _iAnimSKBExtractTranslate(
                (iAnimSKBHeader*)grab->Data->RawData[0], 1, ledge->tranTable, 60);

            iAnimEval(idle->Data->RawData[0], 0.0f, 0, tran, quat);
            _iAnimSKBAdjustTranslate((iAnimSKBHeader*)grab->Data->RawData[0], 1, (F32*)&tran[1],
                                     (F32*)&tran[1]);

            for (i = 0; i < ledge->tranCount; i++)
            {
                ledge->tranTable[i].x -= tran[1].x;
                ledge->tranTable[i].y -= tran[1].y;
                ledge->tranTable[i].z -= tran[1].z;
            }

            ledge->zdist = ledge->tranTable[ledge->tranCount - 1].z;
            ((iAnimSKBHeader*)grab->Data->RawData[0])->Flags |= 0x80000000;
        }

        grab->Data->FileFlags &= ~0x777;
        xAnimFileSetTime(grab->Data, grab->Data->Duration, grab->Data->TimeOffset);
        ledge->ttime = grab->Data->Duration / grab->Speed;
    }
    else
    {
        ledge->ttime = 0.5f;
        ledge->tranCount = 0;
        ledge->zdist = 0.1f;
    }

    ledge->optr = NULL;
    ledge->y0det = 0.0f;
    ledge->dydet = 1.2f;
    ledge->r0det = 0.5f;
    ledge->drdet = 0.7f;
    ledge->thdet = PI / 2;
    ledge->rtime = 0.2f;
    ledge->tmr = 0.0f;
    ledge->spos.x = 0.0f;
    ledge->spos.y = 0.0f;
    ledge->spos.z = 0.0f;
    ledge->tpos.x = 0.0f;
    ledge->tpos.y = 0.0f;
    ledge->tpos.z = 0.0f;
    ledge->nrays = 3;
    ledge->rrand = 0;
}

static void PlayerLedgeUpdate(xEnt* ent, xScene* sc, F32 dt)
{
    zLedgeGrabParams* ledge = &globals.player.s->ledge;
    F32 dx;
    F32 dz;

    if (ledge->tmr > 0.0f)
    {
        F32 tmr = ledge->tmr + dt;

        if (ledge->ttime - ledge->tmr < dt)
        {
            ledge->tmr = 0.0f;
        }
        else
        {
            ledge->tmr = tmr;
        }

        F32 lerp = MIN(tmr, ledge->ttime) / ledge->ttime;

        if (ledge->optr)
        {
            xMat4x3 delta;

            RwMatrixInvert((RwMatrixTag*)&delta, (RwMatrixTag*)&ledge->omat);
            xMat4x3Mul(&delta, &delta, (xMat4x3*)ledge->optr->model->Mat);
            xMat4x3Toworld(&ledge->spos, &delta, &ledge->spos);
            xMat4x3Toworld(&ledge->epos, &delta, &ledge->epos);
            xMat4x3Toworld(&ledge->tpos, &delta, &ledge->tpos);
            ledge->omat = *(xMat4x3*)ledge->optr->model->Mat;
            ledge->endrot = xatan2(ledge->tpos.x - ledge->spos.x, ledge->tpos.z - ledge->spos.z);

            if (ledge->startrot > PI + ledge->endrot)
            {
                ledge->startrot -= 2.0f * PI;
            }
            else if (ledge->startrot < ledge->endrot - PI)
            {
                ledge->startrot += 2.0f * PI;
            }
        }

        if (ledge->tranCount != 0)
        {
            F32 frame;

            if (xStricmp(ent->model->Anim->Single[0].State->Name, "LedgeGrab01") != 0)
            {
                frame = 1000.0f;
            }
            else
            {
                frame = ent->model->Anim->Single->Time;
            }

            dz = ledge->epos.z - ledge->spos.z;
            dx = ledge->epos.x - ledge->spos.x;
            F32 len = xsqrt(dx * dx + dz * dz);

            dx /= len;
            dz /= len;

            F32 drop = len + ledge->tranTable[0].z;
            F32 blend;

            if (frame < ledge->animGrab)
            {
                blend = 1.0f - frame / ledge->animGrab;
            }
            else
            {
                blend = 0.0f;
            }

            F32 fidx = lerp * (ledge->tranCount - 1);
            F32 flr = std::floorf(fidx);
            S32 idx = (S32)flr;
            F32 frac = fidx - flr;

            if (idx >= ledge->tranCount - 1)
            {
                frac = 1.0f;
                idx--;
            }

            xVec3 tran;

            xVec3Lerp(&tran, &ledge->tranTable[idx], &ledge->tranTable[idx + 1], frac);

            F32 back = blend * drop;

            tran.x -= ledge->tranTable[ledge->tranCount - 1].x;
            tran.y -= ledge->tranTable[ledge->tranCount - 1].y;
            tran.z -= ledge->tranTable[ledge->tranCount - 1].z;

            ent->frame->mat.pos.x = tran.x * dz + dx * (tran.z - back);
            ent->frame->mat.pos.y = tran.y;
            ent->frame->mat.pos.z = -tran.x * dx + dz * (tran.z - back);
            ent->frame->mat.pos.x += ledge->tpos.x;
            ent->frame->mat.pos.y += ledge->tpos.y;
            ent->frame->mat.pos.z += ledge->tpos.z;
        }
        else
        {
            xVec3Lerp(&ent->frame->mat.pos, &ledge->spos, &ledge->tpos, lerp);
            ent->frame->mat.pos.y += isin(PI * lerp);
        }

        ent->frame->vel.x = 0.0f;
        ent->frame->vel.y = 0.0f;
        ent->frame->vel.z = 0.0f;
        tslide_inair_tmr = 0.0f;
        tslide_dbl_tmr = 0.0f;
        globals.player.SlideTrackDecay = 0.0f;
        tslide_ground = 0;

        F32 rot = MIN(tmr / ledge->rtime, 1.0f);

        ent->frame->rot.angle =
            globals.player.s->ledge.startrot +
            rot * (globals.player.s->ledge.endrot - globals.player.s->ledge.startrot);
        xMat3x3Rot(&ent->frame->mat, &ent->frame->rot.axis, ent->frame->rot.angle);
    }
    else
    {
        ledge->tmr = 0.0f;

        if (ent->frame->vel.y >= 0.0f)
        {
            return;
        }

        if (tslide_ground)
        {
            return;
        }

        xNearFloorPoly nfp;

        nfp.box.upper.x = 1.0f + ent->frame->mat.pos.x;
        nfp.box.upper.y = 1.0f + ent->frame->mat.pos.y;
        nfp.box.upper.z = 1.0f + ent->frame->mat.pos.z;
        nfp.box.lower.x = ent->frame->mat.pos.x - 1.0f;
        nfp.box.lower.y = 0.5f + ent->frame->mat.pos.y;
        nfp.box.lower.z = ent->frame->mat.pos.z - 1.0f;
        nfp.floorDot = 0.866f;
        nfp.facingDot = -1.0f;
        nfp.facingVec = ent->frame->mat.at;
        xVec3Lerp(&nfp.center, &nfp.box.upper, &nfp.box.lower, 0.5f);

        for (S32 pass = 0; pass <= 2; pass++)
        {
            if (!xSceneNearestFloorPoly(sc, &nfp, 0x10, 0x26))
            {
                return;
            }

            if (nfp.neardist < 0.4f)
            {
                return;
            }

            xSurface* surf;

            if (nfp.optr)
            {
                surf = nfp.mptr->Surf;
            }
            else
            {
                surf = zSurfaceGetSurface(nfp.oid);
            }

            if (surf && (((zSurfaceProps*)surf->moprops)->asset->phys_flags & 0x40))
            {
                return;
            }

            F32 dx = nfp.nearpt.x - ent->frame->mat.pos.x;
            F32 dz = nfp.nearpt.z - ent->frame->mat.pos.z;
            F32 dist = xsqrt(dx * dx + dz * dz);

            if (dist > 1e-07f && dist < 0.8f &&
                (dx * nfp.facingVec.x + dz * nfp.facingVec.z) / dist > 0.7071f)
            {
                xRay3 ray;
                xCollis coll;

                ray.origin.x = ent->frame->mat.pos.x;
                ray.origin.y = 1.0f + ent->frame->mat.pos.y;
                ray.origin.z = ent->frame->mat.pos.z;
                ray.dir.x = 0.0f;
                ray.dir.y = -1.0f;
                ray.dir.z = 0.0f;
                ray.min_t = 0.0f;
                ray.max_t = ray.origin.y - (-1.25f + nfp.nearpt.y);
                ray.flags = 0xc00;
                coll.flags = 0;
                coll.optr = NULL;
                coll.mptr = NULL;
                coll.dist = 1e38f;
                xRayHitsSceneFlags(sc, &ray, &coll, 0x10, 0x26);

                if (coll.dist < 1e38f)
                {
                    return;
                }

                ledge->tpos.x = (dx / dist) * ledge->zdist + nfp.nearpt.x;
                ledge->tpos.y = 0.2f + nfp.nearpt.y;
                ledge->tpos.z = (dz / dist) * ledge->zdist + nfp.nearpt.z;

                xVec3 oldcenter = ent->bound.sph.center;

                ent->bound.sph.center = ledge->tpos;
                ent->bound.sph.center.y += 0.51f;

                xEntCollis oldcollis = *ent->collis;

                PlayerCollsDetect(ent, sc, dt);

                if (ent->collis->idx > 6)
                {
                    ent->bound.sph.center = oldcenter;
                    *ent->collis = oldcollis;
                    return;
                }

                ent->bound.sph.center = oldcenter;
                *ent->collis = oldcollis;

                ledge->tmr = -1.0f;
                ledge->spos = *xEntGetPos(ent);
                ledge->epos = nfp.nearpt;
                ledge->tpos.y -= 0.2f;
                ledge->endrot =
                    xatan2(ledge->tpos.x - ledge->spos.x, ledge->tpos.z - ledge->spos.z);
                ledge->optr = (xEnt*)nfp.optr;

                if (ledge->optr)
                {
                    ledge->omat = *(xMat4x3*)ledge->optr->model->Mat;
                }

                return;
            }

            if (pass == 0)
            {
                nfp.box.upper.x += 0.2f * ent->frame->mat.at.x - 0.2f;
                nfp.box.lower.x += 0.2f * ent->frame->mat.at.x + 0.2f;
                nfp.center.x += 0.2f * ent->frame->mat.at.x;
                nfp.box.upper.z += 0.2f * ent->frame->mat.at.z - 0.2f;
                nfp.box.lower.z += 0.2f * ent->frame->mat.at.z + 0.2f;
                nfp.center.z += 0.2f * ent->frame->mat.at.z;
            }
        }
    }
}

xAnimTable* zEntPlayer_TreeDomeSBAnimTable()
{
    xAnimTable* table = xAnimTableNew("SB", NULL, 0x0);

    xAnimTableNewState(table, "Idle01", 0x10, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Idle02", 0x10, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Idle03", 0x10, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Idle04", 0x10, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Idle05", 0x10, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Idle06", 0x10, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Idle07", 0x10, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Idle08", 0x10, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Idle09", 0x10, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Idle10", 0x10, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Idle11", 0x10, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Idle12", 0x10, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Idle13", 0x10, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "SlipIdle01", 0x10, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Inactive01", 0x20, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Inactive02", 0x20, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Inactive03", 0x20, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Inactive04", 0x20, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Inactive05", 0x20, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Inactive06", 0x20, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Inactive07", 0x20, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Inactive08", 0x20, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Inactive09", 0x20, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Inactive10", 0x20, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Walk01", 0x10, 0x44, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Run01", 0x10, 0x46, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Run02", 0x10, 0x46, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Run03", 0x10, 0x46, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "RunOutOfWorld01", 0x10, 0x46, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "SlipRun01", 0x10, 0x46, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "JumpStart01", 0x20, 0xA, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "JumpLift01", 0x10, 0xA, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "JumpApex01", 0x20, 0xA, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Fall01", 0x10, 0xA, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Land01", 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "LandRun01", 0x20, 0x6, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "BounceStart01", 0x20, 0xA, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "BounceLift01", 0x10, 0xA, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "BounceApex01", 0x20, 0xA, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "DJumpStart01", 0x20, 0xA, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "DJumpLift01", 0x10, 0xA, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "FallHigh01", 0x20, 0x0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "LandHigh01", 0x20, 0x0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Bspin01", 0x20, 0x80A, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "BbashAttack01", 0x10, 0x4000, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "BbashStart01", 0x20, 0x0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "BbashStrike01", 0x20, 0x4000, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "BbashMiss01", 0x20, 0x0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "BbounceAttack01", 0x10, 0x4000, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "BbounceStart01", 0x20, 0x0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "BbounceStrike01", 0x20, 0x0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Bbowl01", 0x20, 0x0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "BbowlStart01", 0x20, 0x0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "BbowlWindup01", 0x10, 0x0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "BbowlToss01", 0x20, 0x0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "BbowlRecover01", 0x20, 0x80, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "LedgeGrab01", 0x20, 0x80, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Hit01", 0x20, 0x80, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Hit02", 0x20, 0x80, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Hit03", 0x20, 0x80, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Hit04", 0x20, 0x80, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Hit05", 0x20, 0x80, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Defeated01", 0x0, 0x480, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Defeated02", 0x0, 0x480, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Defeated03", 0x0, 0x480, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Defeated04", 0x0, 0x480, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Defeated05", 0x0, 0x480, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "TongueSlide01", 0x10, 0x1840, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "TongueStart01", 0x20, 0xA, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "TongueJump01", 0x20, 0xA, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "TongueJumpXtra01", 0x20, 0xA, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "TongueDJumpApex01", 0x20, 0xA, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "TongueFall01", 0x10, 0xA, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "TongueLand01", 0x20, 0x1800, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "TongueTumble01", 0x20, 0x1800, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Goo01", 0x10, 0x0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Goo02", 0x20, 0x0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "GooDefeated", 0x0, 0x480, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "WallLaunch01", 0x20, 0x8A, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "WallFlight01", 0x20, 0xA, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "WallFlight02", 0x10, 0xA, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "WallLand01", 0x20, 0xA, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "WallFall01", 0x10, 0xA, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "BoulderRoll01", 0x20, 0xA, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "BoulderRoll02", 0x10, 0xA, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Talk04", 0x10, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Talk03", 0x10, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Talk02", 0x10, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Talk01", 0x10, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "SpatulaGrab01", 0x20, 0x80, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);

    return table;
}

void iCameraSetBlurriness(F32 amount);

// Equivalent; scheduling issues.
static void zEntPlayer_UpdateVelocityBlur()
{
    F32 start_vel2;
    F32 peak_vel2;
    F32 vel; // not in DWARF

    static F32 start_vel = 10.0f;
    static F32 peak_vel = 50.0f;
    static F32 max_blur = 0.5f;

    if (start_vel >= peak_vel)
    {
        return;
    }

    start_vel2 = start_vel * start_vel;
    peak_vel2 = peak_vel * peak_vel;
    vel = globals.player.ent.frame->vel.length2();
    if (vel < start_vel2)
    {
        iCameraSetBlurriness(0);
    }
    else
    {
        start_vel2 = (vel / (peak_vel2 - start_vel2));
        if (start_vel2 > 1.0f)
        {
            start_vel2 = 1.0f;
        }
        iCameraSetBlurriness(start_vel2 * max_blur);
    }
}

static void dampen_velocity(xVec3& v1, const xVec3& v2, F32 f)
{
    F32 f0 = v1.x * v2.x;

    // `v1_old` is const so that the scheduler does not treat the stores that
    // fill it as possibly aliasing the 0.0f literal load below; retail issues
    // that load before the last two stores.
    const xVec3 v1_old = v1;

    F32 f3 = v2.y;
    F32 f4 = -((f * f0) - v1.x);

    v1.x = f4;
    f4 = v1.y;
    f3 = f4 * f3;
    f3 = -((f * f3) - f4);

    v1.y = f3;
    f3 = v1.z;
    f0 = f3 * v2.z;
    v1.z = -((f * f0) - f3);

    S32 oldV1Neg = (v1_old.x < 0.0f) ? 1 : 0;
    S32 newV1Neg = (v1.x < 0.0f) ? 1 : 0;

    if (newV1Neg != oldV1Neg)
    {
        v1.x = 0.0f;
    }

    oldV1Neg = (v1_old.y < 0.0f) ? 1 : 0;
    newV1Neg = (v1.y < 0.0f) ? 1 : 0;

    if (newV1Neg != oldV1Neg)
    {
        v1.y = 0.0f;
    }

    oldV1Neg = (v1_old.z < 0.0f) ? 1 : 0;
    newV1Neg = (v1.z < 0.0f) ? 1 : 0;

    if (newV1Neg != oldV1Neg)
    {
        v1.z = 0.0f;
    }
}

static void player_sound_hop_load(U32 hopid, S32 hip_or_hop)
{
    S64 t;

    xMemPushBase();
    t = iTimeGet();
    xUtil_idtag2string(hopid, 0);
    iTimeDiffSec(t);
    xSTPreLoadScene(hopid, NULL, hip_or_hop);
    t = iTimeGet();
    xUtil_idtag2string(hopid, 0);
    iTimeDiffSec(t);
    xSTQueueSceneAssets(hopid, hip_or_hop);
    t = iTimeGet();
    xUtil_idtag2string(hopid, 0);
    iTimeDiffSec(t);
    while (xSTLoadStep(hopid) < 1.0f)
    {
        iTRCDisk::CheckDVDAndResetState();
    }
    xSTDisconnect(hopid, hip_or_hop);
    t = iTimeGet();
    xUtil_idtag2string(hopid, 0);
    iTimeDiffSec(t);
}

static S32 g_flg_loaded;

void zEntPlayer_LoadSounds()
{
    U32 bufsize;

    player_sound_hop_load('SPSB', 2);
    g_flg_loaded |= 2;

    void* info = xSTFindAsset(PATRICK_MODEL_ASSETID, &bufsize);
    if (info != NULL)
    {
        player_sound_hop_load('SPPA', 2);
        g_flg_loaded |= 0x08;
    }

    info = xSTFindAsset(SANDY_MODEL_ASSETID, &bufsize);
    if (info != NULL)
    {
        player_sound_hop_load('SPSC', 2);
        g_flg_loaded |= 0x20;
    }

    zEntPlayer_SNDInit();
}

void zEntPlayer_UnloadSounds()
{
    if (g_flg_loaded == 0)
    {
        return;
    }
    if (g_flg_loaded & 0x10)
    {
        xSTUnLoadScene('SPSC', 1);
    }
    if (g_flg_loaded & 0x20)
    {
        xSTUnLoadScene('SPSC', 2);
        iSndSceneExit();
    }
    if (g_flg_loaded & 0x04)
    {
        xSTUnLoadScene('SPPA', 1);
    }
    if (g_flg_loaded & 0x08)
    {
        xSTUnLoadScene('SPPA', 2);
        iSndSceneExit();
    }
    if (g_flg_loaded & 0x01)
    {
        xSTUnLoadScene('SPSB', 1);
    }
    if (g_flg_loaded & 0x02)
    {
        xSTUnLoadScene('SPSB', 2);
        iSndSceneExit();
    }
    g_flg_loaded = 0;
}

static void dont_move(xEnt* ent, xScene* scene, F32 dt, xEntFrame* frame)
{
    PlayerAbsControl(ent, 0.0, 0.0, dt);
}

U8 zEntPlayer_MinimalUpdate(xEnt* ent, xScene* sc, F32 dt, xVec3& drive_motion)
{
    U8 hit_goo = 0;
    xEntCollis* fcoll;

    if (oob_state::update(*sc, dt))
    {
        return 1;
    }

    zEntPickup_CheckAllPickupsAgainstPlayer(sc, dt);
    xEntBeginUpdate(ent, sc, dt);
    zGooCollsBegin();

    xVec3 oldpos = ent->frame->mat.pos;

    xEntApplyPhysics(ent, sc, dt);

    xEntMoveCallback oldmove = ent->move;
    ent->move = dont_move;

    xEntMove(ent, sc, dt);

    ent->move = oldmove;

    const xVec3 req_motion = ent->frame->dpos;
    const xVec3 predrive_pos = ent->frame->mat.pos;

    xEntDriveUpdate(&globals.player.drv, sc, dt, NULL);

    drive_motion = ent->frame->mat.pos - predrive_pos;

    xVec3 motion;
    xVec3Sub(&motion, &ent->frame->mat.pos, &oldpos);

    if ((globals.player.JumpState == 0 || globals.player.JumpState == 1) &&
        !globals.player.FallDeathTimer)
    {
        F32 sink = 0.5f * dt;

        ent->frame->mat.pos.y -= sink;

        F32 ndotm = globals.player.floor_norm.x * motion.x +
                    globals.player.floor_norm.z * motion.z;

        if (ndotm > 0.0f && (req_motion.x != 0.0f || req_motion.z != 0.0f || surfSlickRatio))
        {
            globals.player.slope = -1;
            ent->frame->mat.pos.y -= xsqrt(ndotm);
        }
        else if (ndotm < 0.0f)
        {
            globals.player.slope = 1;
        }
        else
        {
            globals.player.slope = 0;
        }
    }

    zEntPlayerCollide(ent, sc, dt);
    zEntPlayer_CheckCritterContact(ent, dt);
    xEntBoulder_ApplyForces(ent->collis);
    zGooCollsEnd();

    fcoll = ent->collis;

    if (fcoll->colls[0].flags & 1)
    {
        xBase* b = (xBase*)fcoll->colls[0].optr;

        if (b && b->baseType == 0x18)
        {
            zEntButton_Hold((_zEntButton*)b, 0x400);
        }
    }

    if (fcoll->colls[0].flags & 1)
    {
        xBase* b = (xBase*)fcoll->colls[0].optr;

        if (b && zGooIs((xEnt*)b))
        {
            hit_goo = 1;
        }
    }

    zEntPlayerFloorUpdate(ent, sc, dt);
    zEntPlayerSurfDamageUpdate(ent, sc, dt);
    zEntPlayerDriveUpdate(ent, sc, dt);
    zEntPlayerJumpUpdate(ent, sc, dt);
    zEntPlayerCollTrigger(ent, sc);
    zEntPlayerVelUpdate(ent, sc, dt);
    xEntEndUpdate(ent, sc, dt);

    return hit_goo;
}

void zEntPlayer_MinimalRender(zEnt* ent)
{
    F32 dot = 0.0f;

    if (gReticleTarget)
    {
        const xVec3& pos = *(xVec3*)&ent->model->Mat->pos;
        const xVec3& tpos = *(xVec3*)&gReticleTarget->model->Mat->pos;
        xVec3 toCam = globals.camera.mat.pos - pos;
        xVec3 toTarget = tpos - pos;

        dot = toCam.dot(toTarget);
        if (dot <= 0.0f)
        {
            zEntPlayer_ReticleRender(ent);
        }
    }

    xEntRender(&globals.player.ent);

    if (dot > 0.0f)
    {
        zEntPlayer_ReticleRender(ent);
    }
}

S32 zEntPlayerDyingInGoo()
{
    return in_goo != 0;
}

// TODO: Move these to their headers

WEAK void xVec3ScaleC(xVec3* o, const xVec3* v, F32 x, F32 y, F32 z)
{
    o->x = v->x * x;
    o->y = v->y * y;
    o->z = v->z * z;
}

WEAK void xMat3x3SMul(xMat3x3* o, const xMat3x3* m, F32 s)
{
    xVec3SMul(&o->right, &m->right, s);
    xVec3SMul(&o->up, &m->up, s);
    xVec3SMul(&o->at, &m->at, s);
    o->flags = 0;
}

WEAK U32 xSndIsPlaying(U32 assetID)
{
    return iSndIsPlaying(assetID);
}

WEAK U8 xSndIsPlayingByHandle(U32 sndID)
{
    return iSndIsPlayingByHandle(sndID);
}

WEAK S32 zNPCTiki::IsHealthy()
{
    return flg_vuln != 0;
}

WEAK void zCameraTranslate(xCamera* cam, xVec3* pos)
{
    zCameraTranslate(cam, pos->x, pos->y, pos->z);
}

// TODO: This belongs in zNPCSupport.h
// but the compiler put it here for some reason?
// The casts below are not papering over anything. Mat is a RenderWare matrix,
// so its axes are RwV3d; the game's own vector type is xVec3. The two are
// layout-identical (three consecutive F32) but are genuinely distinct types
// owned by different SDKs, and neither can be made an alias of the other
// without dragging RenderWare into xMath3.h or vice versa. Casting at the
// boundary is what retail did, and it costs nothing at runtime.
WEAK xVec3* NPCC_rightDir(xEnt* ent)
{
    return (xVec3*)&ent->model->Mat->right;
}

WEAK xVec3* NPCC_faceDir(xEnt* ent)
{
    return (xVec3*)&ent->model->Mat->at;
}

WEAK xVec3* NPCC_upDir(xEnt* ent)
{
    return (xVec3*)&ent->model->Mat->up;
}

WEAK S32 zNPCCommon::SetCarryState(en_NPC_CARRY_STATE)
{
    return 0;
}

WEAK S32 zNPCCommon::CanRope()
{
    return flg_vuln & 0x1000000;
}

WEAK F32 zNPCCommon::XZDstSqToPlayer(xVec3* dir, F32* dy)
{
    return XZDstSqToPos(xEntGetPos(&globals.player.ent), dir, dy);
}

WEAK F32 zNPCCommon::XZDstSqToPos(const xVec3* pos, xVec3* dir, F32* dy)
{
    xVec3 tmp;

    if (!dir)
    {
        dir = &tmp;
    }

    XZVecToPos(dir, pos, dy);

    return xVec3Length2(dir);
}

WEAK void zNPCCommon::XZVecToPos(xVec3* dir, const xVec3* pos, F32* dy)
{
    xVec3Sub(dir, pos, Pos());

    if (dy)
    {
        *dy = dir->y;
    }

    dir->y = 0.0f;
}

WEAK xVec3* zNPCCommon::Pos()
{
    return (xVec3*)&model->Mat->pos;
}
