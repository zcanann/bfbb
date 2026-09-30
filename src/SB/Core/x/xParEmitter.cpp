#include "xParEmitter.h"

#include <types.h>
#include <zScene.h>
#include <iModel.h>
#include <xMathInlines.h>
#include <xMath.h>
#include <xGroup.h>
#include <xMovePoint.h>
#include <zGlobals.h>
#include <xDebug.h>
#include <xEvent.h>
#include <PowerPC_EABI_Support\MSL_C\MSL_Common\cmath>

static xParEmitterAsset sSaveEmmiterSettings;
static xParEmitterPropsAsset sSaveEmmiterPropSettings;
static xParEmitterPropsAsset sDummyProp;

static void add_tweaks(xParEmitter& pe)
{
}

S32 xParInterpConvertInterpMode(xParInterp* p)
{
    if (p->interp < 8)
    {
        return p->interp;
    }
    if (p->interp == xStrHash("ConstA"))
    {
        return 0;
    }
    if (p->interp == xStrHash("ConstB"))
    {
        return 1;
    }
    if (p->interp == xStrHash("Sine"))
    {
        return 4;
    }
    if (p->interp == xStrHash("Cosine"))
    {
        return 5;
    }
    if (p->interp == xStrHash("Linear"))
    {
        return 3;
    }
    if (p->interp == xStrHash("Step"))
    {
        return 7;
    }
    if (p->interp == xStrHash("Random"))
    {
        return 2;
    }
    return 0;
}

void xParEmitterInit(void* b, void* tasset)
{
    xParEmitterInit((xBase*)b, (xParEmitterAsset*)tasset);
}

void xParEmitterInit(xBase* b, xParEmitterAsset* pea)
{
    xParEmitterPropsAsset* prop;
    S32 i;
    F32 vec_length;
    F32 temp_rate;
    F32 temp_oocull;
    xParSys* temp_parSys;

    xParEmitter* t = (xParEmitter*)b;

    xBaseInit(t, (xBaseAsset*)pea);
    t->eventFunc = (xBaseEventCB)xParEmitterEventCB;
    t->tasset = pea;
    t->prop = (xParEmitterPropsAsset*)xSTFindAsset(pea->propID, NULL);
    if (t->prop == NULL)
    {
        t->prop = &sDummyProp;
    }
    if (t->linkCount != 0)
    {
        t->link = (xLinkAsset*)(t->tasset + 1);
    }
    else
    {
        t->link = NULL;
    }
    prop = t->prop;
    t->emit_flags = pea->emit_flags;
    if (t->prop->parSysID != 0)
    {
        temp_parSys = (xParSys*)zSceneFindObject(t->prop->parSysID);
    }
    else
    {
        temp_parSys = NULL;
    }
    t->parSys = temp_parSys;
    t->attachTo = NULL;
    t->emit_volume = NULL;
    for (i = 0; i < 14; i++)
    {
        if (prop->value[i].val[0] == prop->value[i].val[1])
        {
            prop->value[i].interp = 0;
        }
        else
        {
            prop->value[i].interp = xParInterpConvertInterpMode(&prop->value[i]);
        }
    }
    t->rate_mode = prop->value[0].interp;
    if ((t->rate_mode == 2) && (prop->value[0].val[0] == prop->value[0].val[1]))
    {
        t->rate_mode = 0;
    }
    if (t->rate_mode == 2)
    {
        prop->value[0].order();
    }
    if (t->rate_mode == 2)
    {
        temp_rate =
            ((prop->value[0].val[1] - prop->value[0].val[0]) * xurand()) + prop->value[0].val[0];
    }
    else
    {
        temp_rate = prop->value[0].val[0];
    }
    t->rate = temp_rate;
    t->rate_time = 0.0f;
    t->rate_fraction = 0.0f;
    t->rate_fraction_cull = 0.0f;
    if (pea->emit_flags & 2)
    {
        temp_oocull = 1.0f / pea->cull_dist_sqr;
    }
    else
    {
        temp_oocull = 0.0f;
    }
    t->oocull_distance_sqr = temp_oocull;
    t->emit_volume = NULL;
    if ((pea->emit_type == eParEmitterOCircle) || (pea->emit_type == eParEmitterOCircleEdge))
    {
        vec_length = (pea->e_circle.dir).length2();
        if (vec_length < 0.001f)
        {
            pea->e_circle.dir.assign(0.0f, 1.0f, 0.0f);
        }
        else
        {
            pea->e_circle.dir *= 1.0f / xsqrt(vec_length);
        }
    }
    t->last_attach_loc = 1e38f;
}

void xParEmitterSetup(xParEmitter* t)
{
    xEnt* ent;

    if (t->parSys != NULL)
    {
        t->group = t->parSys->group;
    }
    if ((t->tasset->attachToID != 0) && (t->tasset->emit_type == eParEmitterOffsetPoint))
    {
        ent = (xEnt*)zSceneFindObject(t->tasset->attachToID);
        iModelTagSetup(&t->tag, ent->model->Data, *(F32*)&t->tasset->e_entbound,
                       (t->tasset->e_entbound).expand, (t->tasset->e_entbound).deflection);
    }
    if (t->tasset->attachToID != 0)
    {
        ent = (xEnt*)zSceneFindObject(t->tasset->attachToID);
    }
    else
    {
        ent = NULL;
    }
    t->attachTo = ent;
    switch (t->tasset->emit_type)
    {
    case eParEmitterVolume:
        t->emit_volume = zSceneFindObject(t->tasset->e_volume.emit_volumeID);
        break;
    case eParEmitterEntityBone:
    case eParEmitterEntityBound:
        break;
    }
    add_tweaks(*t);
    return;
}

void xParEmitterReset(xParEmitter* t)
{
    xBaseReset((xBase*)t, t->tasset);
    t->emit_flags = t->tasset->emit_flags;
}

S32 xParEmitterEventCB(xBase* to, xBase* from, U32 toEvent, const F32* toParam,
                       xBase* toParamWidget)
{
    xParEmitterCustomSettings sp8;

    switch ((S32)toEvent)
    {
    case eEventReset:
        xParEmitterReset((xParEmitter*)from);
        break;
    case eEventOn:
        ((xParEmitter*)from)->emit_flags |= 1;
        break;
    case eEventOff:
        if (((xParEmitter*)from)->emit_flags & 1)
        {
            ((xParEmitter*)from)->emit_flags ^= 1;
        }
        break;
    case eEventEmit:
        memset(&sp8, 0, sizeof(sp8));
        xParEmitterEmitCustom((xParEmitter*)from, 0.033333335f, &sp8);
        break;
    }
    return 1;
}

xPar* xParEmitterEmitCustom(xParEmitter* p, F32 dt, xParEmitterCustomSettings* info)

{
    U32 custom_flags;

    custom_flags = info->custom_flags;
    xParEmitterAsset* p_tasset = p->tasset;
    if ((custom_flags & eParEmitterCustomSaveRestore) != 0)
    {
        memcpy(&sSaveEmmiterSettings, p_tasset, sizeof(xParEmitterAsset));
        memcpy(&sSaveEmmiterPropSettings, p->prop, sizeof(xParEmitterPropsAsset));
    }
    if ((custom_flags & eParEmitterCustomPos) != 0)
    {
        p_tasset->pos = info->pos;
    }
    if ((custom_flags & eParEmitterCustomRate) != 0)
    {
        p->prop->value[0] = info->value[0];
    }
    if ((custom_flags & eParEmitterCustomLife) != 0)
    {
        p->prop->life = info->life;
    }
    if ((custom_flags & eParEmitterCustomEmitVolume) != 0)
    {
        p->emit_volume = info->emit_volume;
    }
    if ((custom_flags & eParEmitterCustomSizeBirth) != 0)
    {
        p->prop->size_birth = info->size_birth;
    }
    if ((custom_flags & eParEmitterCustomSizeDeath) != 0)
    {
        p->prop->size_death = info->size_death;
    }
    if ((custom_flags & eParEmitterCustomVel) != 0)
    {
        p_tasset->vel = info->vel;
        p->prop->vel = info->vel;
    }
    if ((custom_flags & eParEmitterCustomVelAngleVariation) != 0)
    {
        p_tasset->vel_angle_variation = info->vel_angle_variation;
    }
    if ((custom_flags & eParEmitterCustomColorBirth) != 0)
    {
        p->prop->color_birth[0] = info->color_birth[0];
        p->prop->color_birth[1] = info->color_birth[1];
        p->prop->color_birth[2] = info->color_birth[2];
        p->prop->color_birth[3] = info->color_birth[3];
    }
    if ((custom_flags & eParEmitterCustomColorDeath) != 0)
    {
        p->prop->color_death[0] = info->color_death[0];
        p->prop->color_death[1] = info->color_death[1];
        p->prop->color_death[2] = info->color_death[2];
        p->prop->color_death[3] = info->color_death[3];
    }
    if ((custom_flags & eParEmitterCustomRadius) != 0)
    {
        switch (p->tasset->emit_type)
        {
        case eParEmitterCircleEdge:
        case eParEmitterCircle:
        case eParEmitterOCircleEdge:
        case eParEmitterOCircle:
            *(F32*)&p->tasset->e_entbound = info->radius;
            break;
        case eParEmitterSphereEdge1:
        case eParEmitterSphere:
        case eParEmitterSphereEdge2:
        case eParEmitterSphereEdge3:
            *(F32*)&p->tasset->e_entbound = info->radius;
            break;
        }
    }
    xPar* newpar = xParEmitterEmit(p, dt);
    if ((custom_flags & eParEmitterCustomSaveRestore) != 0)
    {
        memcpy(p_tasset, &sSaveEmmiterSettings, sizeof(xParEmitterAsset));
        memcpy(p->prop, &sSaveEmmiterPropSettings, sizeof(xParEmitterPropsAsset));
    }
    return newpar;
}

U32 xParEmitterCull(xParEmitter* t, xPar* p)

{
    F32 vec_length;
    xVec3 global_vec;
    xVec3 temp_vec;

    xParEmitterAsset* tas = t->tasset;
    if (tas->emit_flags & 2)
    {
        global_vec = xglobals->camera.mat.pos;
        xVec3Sub(&temp_vec, &global_vec, &p->m_pos);
        vec_length = xVec3Length2(&temp_vec);
        t->distance_to_cull_sqr = vec_length;
        if (tas->cull_mode == 3)
        {
            return vec_length > tas->cull_dist_sqr;
        }
        else
        {
            return vec_length < tas->cull_dist_sqr;
        }
    }
    return 0;
}

F32 xParInterpCompute(S32 interp_mode, xParInterp* r, F32 time, S32 time_has_elapsed, F32 lastVal)

{
    F32 val;

    val = time;
    switch (interp_mode)
    {
    case 0:
        val = r->val[0];
        break;
    case 1:
        val = r->val[1];
        break;
    case 2:
        if (time_has_elapsed != 0)
        {
            val = ((r->val[1] - r->val[0]) * xurand()) + r->val[0];
        }
        else
        {
            val = lastVal;
        }
        break;
    case 3:
        if (r->freq < 1e-05f)
        {
            val = r->val[0];
        }
        else
        {
            val = ((r->val[1] - r->val[0]) * (val / r->freq)) + r->val[0];
        }
        break;
    case 4:
        val = ((r->val[1] - r->val[0]) * (0.5f * isin(6.2831855f * (val * r->oofreq)) + 0.5f)) +
              r->val[0];
        break;
    case 5:
        val = ((r->val[1] - r->val[0]) * (0.5f * icos(6.2831855f * (val * r->oofreq)) + 0.5f)) +
              r->val[0];
        break;
    case 7:
        if ((val * r->freq) >= 0.5f)
        {
            val = r->val[1];
        }
        else
        {
            val = r->val[0];
        }
    }
    return val;
}

xPar* xParEmitterEmitSetTexIdxs(xPar* p, const xParSys* ps)
{
    xParCmdTex* ps_cmdTex = ps->group->m_cmdTex;

    if (ps_cmdTex == NULL)
    {
        return NULL;
    }

    switch ((S32)ps_cmdTex->birthMode)
    {
    case 1:
        p->m_texIdx[0] = (xrand() >> 0x11U) % ps_cmdTex->cols;
        p->m_texIdx[1] = (xrand() >> 0x11U) % ps_cmdTex->rows;
        break;
    case 0:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
        p->m_texIdx[0] = 0;
        p->m_texIdx[1] = 0;
        break;
    }

    return p;
}

xPar* xParEmitterEmit(xParEmitter* pe, F32 emit_dt, F32 par_dt)
{
    xParEmitterAsset* pea;
    xParEmitterPropsAsset* prop;
    S32 rate_has_elapsed;
    F32 rate;
    S32 count;
    xParSys* ps;
    xPar* p;
    F32 life;
    F32 size_birth;
    F32 size_death;
    xVec3 emitPosition;
    S32 attachGroupIndex;
    S32 attachGroupTotal;
    S32 emitAgain;
    S32 marker;
    U32 get_rnd_group_idx;
    xEnt* attach_ent;
    xMat4x3* bone_mat;
    xVec3 bone_vel;
    xVec3 emitvel;
    S32 i;
    S32 c;
    F32 fc1;
    F32 fc2;

    if (pe->parSys == NULL)
    {
        return NULL;
    }

    xPar* last_p = NULL;
    pea = pe->tasset;
    prop = pe->prop;

    pe->rate_time = pe->rate_time + emit_dt;
    rate_has_elapsed = pe->rate_time > prop->rate.freq;

    if (prop->rate.freq == 0.0f)
    {
        pe->rate_time = 0.0f;
    }

    while (pe->rate_time > prop->rate.freq)
    {
        pe->rate_time = pe->rate_time - prop->rate.freq;
    }

    rate = xParInterpCompute(pe->rate_mode, &prop->rate, pe->rate_time, rate_has_elapsed, pe->rate);
    pe->rate = rate;
    pe->rate_fraction = pe->rate_fraction + rate * emit_dt;
    pe->rate_fraction_cull = pe->rate_fraction_cull + rate * emit_dt;

    rate_has_elapsed = std::floorf(pe->rate_fraction);

    if (rate_has_elapsed > 0)
    {
        pe->rate_fraction = pe->rate_fraction - rate_has_elapsed;
    }

    if (rate_has_elapsed == 0)
    {
        return NULL;
    }

    ps = pe->parSys;

    if (ps == NULL)
    {
        return NULL;
    }

    if (ps->group == NULL)
    {
        return NULL;
    }

    if ((ps->tasset->maxPar != 0) && (ps->group->m_num_of_particles >= (S32)ps->tasset->maxPar))
    {
        return NULL;
    }

    if ((ps->tasset->maxPar != 0) && (ps->group->m_num_of_particles >= (S32)ps->tasset->maxPar))
    {
        return NULL;
    }

    if ((ps->tasset->maxPar != 0) &&
        (ps->group->m_num_of_particles + rate_has_elapsed >= (S32)ps->tasset->maxPar))
    {
        rate_has_elapsed = ps->tasset->maxPar - ps->group->m_num_of_particles;
    }

    xBase* emitObj = (xBase*)pe->attachTo;
    count = -1;
    attachGroupTotal = -1;

    do
    {
        emitAgain = 0;
        xBase* attachObject = NULL;
        marker = 0;

        if ((emitObj != NULL) && (emitObj->baseType == eBaseTypeGroup))
        {
            xGroup* g = (xGroup*)emitObj;

            if (count == -1)
            {
                attachGroupTotal = xGroupGetCount(g);
                count = 0;
            }

            if (g->asset->groupFlags & 1)
            {
                get_rnd_group_idx = xrand() % attachGroupTotal;
                attachObject = xGroupGetItemPtr(g, get_rnd_group_idx);
                emitAgain = 0;

                if (attachObject == NULL)
                {
                    attachGroupIndex = xGroupGetItem(g, get_rnd_group_idx);
                    attachObject = (xBase*)xSTFindAsset(attachGroupIndex, NULL);

                    if (attachObject != NULL)
                    {
                        marker = 1;
                    }
                }
            }
            else
            {
                attachObject = xGroupGetItemPtr(g, count);

                if (attachObject == NULL)
                {
                    attachGroupIndex = xGroupGetItem(g, count);
                    attachObject = (xBase*)xSTFindAsset(attachGroupIndex, NULL);

                    if (attachObject != NULL)
                    {
                        marker = 1;
                    }
                }

                count++;
                emitAgain = count < attachGroupTotal;
            }
        }
        else
        {
            attachObject = emitObj;
        }

        attach_ent = (xEnt*)attachObject;

        if (attachObject != NULL)
        {
            if (marker)
            {
                emitPosition = *(xVec3*)attachObject;
                attach_ent = NULL;
            }
            else
            {
                switch (attachObject->baseType)
                {
                case eBaseTypeMovePoint:
                    emitPosition = *((xMovePoint*)attachObject)->pos;
                    attach_ent = NULL;
                    break;
                default:
                    if (!xEntValidType(attachObject->baseType) ||
                        (((xEnt*)attachObject)->model == NULL))
                    {
                        emitPosition = 0.0f;
                        attach_ent = NULL;
                    }
                    else
                    {
                        emitPosition = *xEntGetPos((xEnt*)attachObject);
                    }
                    break;
                }
            }
        }
        else
        {
            emitPosition = pea->pos;
        }

        if (!(pe->emit_flags & 0x10) || (attach_ent == NULL) || xEntIsVisible(attach_ent))
        {
            bone_mat = NULL;

            if ((pea->emit_type == eParEmitterEntityBone) && (attach_ent != NULL))
            {
                bone_mat = xParEmitterTransformEntBone(emitPosition, bone_vel, *pea, *attach_ent);
                bone_vel *= par_dt;
            }

            if (pe->emit_flags & 8)
            {
                emitvel = emitPosition - pe->last_attach_loc;
                pe->last_attach_loc = emitPosition;

                if (emitvel.length2() > 25.0f)
                {
                    emitvel = 0.0f;
                }
            }
            else
            {
                emitvel = 0.0f;
            }

            for (i = 0; i < rate_has_elapsed; i++)
            {
                p = xParGroupAddPar(ps->group);

                if (p != NULL)
                {
                    last_p = p;

                    life =
                        xParInterpCompute(prop->life.interp, &prop->life, pe->rate_time, 1, 0.0f);
                    size_birth = xParInterpCompute(prop->size_birth.interp, &prop->size_birth,
                                                   pe->rate_time, 1, 0.0f);
                    size_death = xParInterpCompute(prop->size_death.interp, &prop->size_death,
                                                   pe->rate_time, 1, 0.0f);

                    p->m_lifetime = life;
                    p->totalLifespan = life;
                    p->m_size = size_birth;

                    if (size_death == size_birth)
                    {
                        p->m_sizeVel = 0.0f;
                    }
                    else
                    {
                        p->m_sizeVel = (size_death - size_birth) / life;
                    }

                    p->m_flag = 0;
                    p->m_rotdeg[0] = pe->rot[0];
                    p->m_rotdeg[1] = pe->rot[1];
                    p->m_rotdeg[2] = pe->rot[2];

                    for (c = 0; c < 4; c++)
                    {
                        fc1 = xParInterpCompute(prop->color_birth[c].interp, &prop->color_birth[c],
                                                pe->rate_time, 1, 0.0f);
                        fc2 = xParInterpCompute(prop->color_death[c].interp, &prop->color_death[c],
                                                pe->rate_time, 1, 0.0f);

                        p->m_cfl[c] = fc1;
                        p->m_c[c] = (U8)(S32)fc1;
                        p->m_cvel[c] = (fc2 - fc1) / life;
                    }

                    p->m_pos = emitPosition;
                    xParEmitterEmitSetTexIdxs(p, ps);

                    switch (pea->emit_type)
                    {
                    case eParEmitterPoint:
                        xParEmitterEmitPoint(p, pea, par_dt);
                        break;
                    case eParEmitterCircleEdge:
                        xParEmitterEmitCircleEdge(p, pea, par_dt);
                        break;
                    case eParEmitterCircle:
                        xParEmitterEmitCircle(p, pea, par_dt);
                        break;
                    case eParEmitterRectEdge:
                        xParEmitterEmitRectEdge(p, pea, par_dt);
                        break;
                    case eParEmitterRect:
                        xParEmitterEmitRect(p, pea, par_dt);
                        break;
                    case eParEmitterLine:
                        xParEmitterEmitLine(p, pea, par_dt);
                        break;
                    case eParEmitterSphereEdge1:
                        xParEmitterEmitSphereEdge(p, pea, par_dt, eParEmitterSphereEdge1);
                        break;
                    case eParEmitterSphereEdge2:
                        xParEmitterEmitSphereEdge(p, pea, par_dt, eParEmitterSphereEdge2);
                        break;
                    case eParEmitterSphereEdge3:
                        xParEmitterEmitSphereEdge(p, pea, par_dt, eParEmitterSphereEdge3);
                        break;
                    case eParEmitterSphere:
                        xParEmitterEmitSphere(p, pea, par_dt);
                        break;
                    case eParEmitterVolume:
                    {
                        xBase* obj = (xBase*)pe->emit_volume;

                        if (obj != NULL)
                        {
                            if (obj->baseType == eBaseTypeVolume)
                            {
                                xParEmitterEmitVolume(p, pea, par_dt, (xVolume*)obj);
                            }
                            else
                            {
                                xParEmitterEmitEntity(p, pea, par_dt, (xEnt*)obj);
                            }
                        }
                        break;
                    }
                    case eParEmitterOffsetPoint:
                        xParEmitterEmitOffsetPoint(pe, p, pea, par_dt, (xEnt*)attachObject);
                        break;
                    case eParEmitterVCylEdge:
                        xParEmitterEmitVCylEdge(p, pea, par_dt);
                        break;
                    case eParEmitterOCircleEdge:
                        xParEmitterEmitOCircleEdge(p, pea, par_dt);
                        break;
                    case eParEmitterOCircle:
                        xParEmitterEmitOCircle(p, pea, par_dt);
                        break;
                    case eParEmitterEntityBone:
                        if (bone_mat != NULL)
                        {
                            p->m_vel = bone_vel;
                            xParEmitterEmitEntBone(p, pea, par_dt, *bone_mat);
                        }
                        break;
                    case eParEmitterEntityBound:
                        if (attach_ent != NULL)
                        {
                            xParEmitterEmitEntBound(p, pea, par_dt, attach_ent);
                        }
                        break;
                    }

                    p->m_vel += emitvel;

                    c = xParEmitterCull(pe, p);

                    if (c != 0)
                    {
                        xParGroupKillPar(ps->group, p);
                    }
                }
                else
                {
                    emitAgain = 0;
                }
            }
        }
    } while (emitAgain);

    return last_p;
}

void xParEmitterUpdate(xBase* to, xScene*, F32 dt)
{
    xParEmitter* parTo = (xParEmitter*)to;
    if ((parTo->parSys != NULL) && (parTo->emit_flags & 1))
    {
        xParEmitterEmit(parTo, dt);
    }
}

void xParEmitterDestroy()
{
    xDebugRemoveTweak("Particle Emitters");
}

inline xPar* xParEmitterEmit(xParEmitter* pe, F32 dt)
{
    return xParEmitterEmit(pe, dt, dt);
}
