#include "zNPCSpawner.h"

#include <types.h>

#include "zEvent.h"
#include "xDraw.h"
#include "zNPCSupport.h"
#include "zGlobals.h"
#include "xMathInlines.h"
#include "zNPCTypeRobot.h"
#include "xUtil.h"

static SMDepot g_smdepot = {};
static S32 g_drawSpawnBounds;

void zNPCSpawner_Startup()
{
}

void zNPCSpawner_Shutdown()
{
}

void zNPCSpawner_ScenePrepare()
{
    SMDepot* depot = &g_smdepot;
    XOrdInit(&depot->spawners, sizeof(g_smdepot), 0);
    for (S32 i = 0; i < 0x10; i++)
    {
        zNPCSpawner* sm = new ('SPWN', NULL) zNPCSpawner;
        XOrdAppend(&depot->spawners, sm);
    }
}

void zNPCSpawner_SceneFinish()
{
    SMDepot* depot = &g_smdepot;
    for (S32 i = 0; i < depot->spawners.cnt; i++)
    {
        RyzMemData::operator delete(depot->spawners.list[i]);
    }
    XOrdDone(&depot->spawners, 0);
}

zNPCSpawner* zNPCSpawner_GetInstance()
{
    SMDepot* depot = &g_smdepot;
    zNPCSpawner* found = NULL;

    for (S32 i = 0; i < depot->spawners.cnt; i++)
    {
        zNPCSpawner* sm = (zNPCSpawner*)depot->spawners.list[i];
        if (!(sm->flg_spawner & 1))
        {
            found = sm;
            sm->flg_spawner |= 1;
            break;
        }
    }

    return found;
}

void zNPCSpawner::Subscribe(zNPCCommon* owner)
{
    this->npc_owner = owner;
    this->tym_delay = 5.0f;
    this->max_spawn = -1;
    this->wavestat = SM_STAT_BEGIN;
    XOrdInit(&this->pendlist, 0x10, 0);
    XOrdInit(&this->actvlist, 0x10, 0);
}

void zNPCSpawner::SetWaveMode(en_SM_WAVE_MODE mode, F32 delay, S32 lifemax)
{
    this->wavemode = mode;
    this->tym_delay = delay;
    this->max_spawn = lifemax;
}

S32 zNPCSpawner::AddSpawnPoint(zMovePoint* sp)
{
    S32 ack = 0;
    for (S32 i = 0; i < 0x10; i++)
    {
        SMSPStatus* sp_stat = &this->sppool[i];
        if (sp_stat->sp == NULL)
        {
            sp_stat->sp = sp;
            sp_stat->npc_prefer = NULL;
            ack = 1;
            break;
        }
    }
    return ack;
}

S32 zNPCSpawner::AddSpawnNPC(zNPCCommon* npc)
{
    S32 ack = 0;
    for (S32 i = 0; i < 0x10; i++)
    {
        SMNPCStatus* npc_stat = &this->npcpool[i];
        if (npc_stat->npc == NULL)
        {
            npc_stat->npc = npc;
            ack = 1;
            npc_stat->status = SM_NPC_DEAD;
            npc_stat->sp_prefer = NULL;
            break;
        }
    }

    npc->DuploOwner(this->npc_owner);
    npc->DBG_Name();
    return ack;
}

void zNPCSpawner::Reset()
{
    this->cnt_spawn = 0;
    this->wavestat = SM_STAT_BEGIN;
    this->cnt_cleanup = 0;
    this->flg_spawner &= 1;
    this->flg_spawner |= 8;
    this->ClearPending();
    this->ClearActive();
    this->MapPreferred();
}

void zNPCSpawner::MapPreferred()
{
    for (S32 i = 0; i < 16; i++)
    {
        SMNPCStatus* npc_stat = &this->npcpool[i];
        if (npc_stat->npc != NULL)
        {
            zMovePoint* sp = (zMovePoint*)npc_stat->npc->FirstAssigned();
            if (sp != NULL)
            {
                SMSPStatus* sp_stat = StatForSP(sp, 0);
                if (sp_stat != NULL)
                {
                    npc_stat->sp_prefer = sp;
                    sp_stat->npc_prefer = npc_stat->npc;
                }
            }
        }
    }
}

void zNPCSpawner::Timestep(F32 dt)
{
    if (flg_spawner & 0x8)
    {
        ChildHeartbeat(dt);
    }

    if (flg_spawner & 0x10)
    {
        ChildCleanup(dt);
    }

    if (!(flg_spawner & 0x4))
    {
        tmr_wave = -1.0f > tmr_wave - dt ? -1.0f : tmr_wave - dt;

        switch (wavemode)
        {
        case SM_WAVE_DISCREET:
            UpdateDiscreet(dt);
            break;
        case SM_WAVE_CONTINUOUS:
            UpdateContinuous(dt);
            break;
        }
    }
}

void zNPCSpawner::UpdateDiscreet(F32 dt)
{
    SMSPStatus* spstat = NULL;

    switch (wavestat)
    {
    case SM_STAT_BEGIN:
        FillPending();

        if (pendlist.cnt > 0)
        {
            wavestat = SM_STAT_INPROG;
            zEntEvent(npc_owner, eEventDuploWaveBegin);

            flg_spawner &= ~0x20;
        }

        tmr_wave = tym_delay;

        break;
    case SM_STAT_INPROG:
        if (pendlist.cnt < 1)
        {
            wavestat = SM_STAT_MARKTIME;
        }
        else if (max_spawn > 0 && !(cnt_spawn < max_spawn))
        {
            wavestat = SM_STAT_ABORT;
        }
        else if (tmr_wave < 0.0f)
        {
            SMNPCStatus* npcstat = NextPendingNPC(0);
            if (npcstat != NULL)
            {
                spstat = SelectSP(npcstat);
            }

            if (!(spstat == NULL))
            {
                SpawnBeastie(npcstat, spstat);
            }

            tmr_wave = tym_delay * (0.25f * (xurand() - 0.5f)) + tym_delay;
        }

        break;
    case SM_STAT_MARKTIME:
        if (actvlist.cnt < 1)
        {
            wavestat = SM_STAT_DONE;
        }
        else if (max_spawn > 0 && !(cnt_spawn < max_spawn))
        {
            wavestat = SM_STAT_ABORT;
        }

        break;
    case SM_STAT_ABORT:
        if (actvlist.cnt < 1)
        {
            wavestat = SM_STAT_DONE;
        }

        ClearPending();

        break;
    case SM_STAT_DONE:
        if (!(flg_spawner & 0x20))
        {
            zEntEvent(npc_owner, eEventDuploWaveComplete);
        }

        if (max_spawn > 0 && !(cnt_spawn < max_spawn))
        {
            if (!(flg_spawner & 0x20))
            {
                flg_spawner |= 0x20;

                zEntEvent(npc_owner, eEventDuploExpiredMaxNPC);
                zEntEvent(npc_owner, eEventDuploSuperDuperDone);

                if (flg_spawner & 0x2)
                {
                    zEntEvent(npc_owner, eEventDuploDuperIsDoner);
                }
            }
        }
        else
        {
            if (flg_spawner & 0x2)
            {
                if (!(flg_spawner & 0x20))
                {
                    flg_spawner |= 0x20;

                    zEntEvent(npc_owner, eEventDuploSuperDuperDone);
                    zEntEvent(npc_owner, eEventDuploDuperIsDoner);
                }
            }
            else
            {
                wavestat = SM_STAT_BEGIN;
            }
        }

        break;
    default:
        break;
    }
}

void zNPCSpawner::UpdateContinuous(F32 dt)
{
    SMSPStatus* spstat = NULL;

    switch (wavestat)
    {
    case SM_STAT_BEGIN:
        FillPending();

        if (pendlist.cnt > 0)
        {
            wavestat = SM_STAT_INPROG;
            zEntEvent(npc_owner, eEventDuploWaveBegin);

            flg_spawner &= ~0x20;
        }

        tmr_wave = tym_delay;

        break;
    case SM_STAT_INPROG:
        if (pendlist.cnt < 1)
        {
            wavestat = SM_STAT_MARKTIME;
        }
        else if (max_spawn > 0 && !(cnt_spawn < max_spawn))
        {
            wavestat = SM_STAT_ABORT;
        }
        else if (tmr_wave < 0.0f)
        {
            SMNPCStatus* npcstat = NextPendingNPC(0);
            if (npcstat != NULL)
            {
                spstat = SelectSP(npcstat);
            }

            if (!(spstat == NULL))
            {
                SpawnBeastie(npcstat, spstat);
            }

            tmr_wave = tym_delay * (0.25f * (xurand() - 0.5f)) + tym_delay;
        }

        break;
    // MarkTime case is the significantly different one between this function and UpdateDiscreet
    case SM_STAT_MARKTIME:
        if (flg_spawner & 0x2)
        {
            wavestat = SM_STAT_ABORT;
        }
        else if (max_spawn > 0 && !(cnt_spawn < max_spawn))
        {
            wavestat = SM_STAT_ABORT;
        }
        else
        {
            if (tmr_wave < 0.0f)
            {
                tmr_wave = tym_delay * (0.25f * (xurand() - 0.5f)) + tym_delay;

                ReFillPending();

                if (pendlist.cnt > 0)
                {
                    wavestat = SM_STAT_INPROG;
                }
            }
            else if (pendlist.cnt > 0)
            {
                wavestat = SM_STAT_INPROG;
            }
        }

        break;
    case SM_STAT_ABORT:
        if (actvlist.cnt < 1)
        {
            wavestat = SM_STAT_DONE;
        }

        ClearPending();

        break;
    case SM_STAT_DONE:
        if (!(flg_spawner & 0x20))
        {
            zEntEvent(npc_owner, eEventDuploWaveComplete);
        }

        if (max_spawn > 0 && !(cnt_spawn < max_spawn))
        {
            if (!(flg_spawner & 0x20))
            {
                flg_spawner |= 0x20;

                zEntEvent(npc_owner, eEventDuploExpiredMaxNPC);
                zEntEvent(npc_owner, eEventDuploSuperDuperDone);

                if (flg_spawner & 0x2)
                {
                    zEntEvent(npc_owner, eEventDuploDuperIsDoner);
                }
            }
        }
        else
        {
            if (flg_spawner & 0x2)
            {
                if (!(flg_spawner & 0x20))
                {
                    flg_spawner |= 0x20;

                    zEntEvent(npc_owner, eEventDuploSuperDuperDone);
                    zEntEvent(npc_owner, eEventDuploDuperIsDoner);
                }
            }
        }

        break;
    default:
        break;
    }
}

void zNPCSpawner::Notify(en_SM_NOTICES note, void* data)
{
    zNPCCommon* npcdata = (zNPCCommon*)data;
    switch (note)
    {
    case SM_NOTE_NPCDIED:
        ToastedBeastie(npcdata);
        SetNPCStatus(npcdata, SM_NPC_DEAD);
        break;
    case SM_NOTE_NPCSTANDBY:
        SetNPCStatus(npcdata, SM_NPC_READY);
        break;
    case SM_NOTE_NPCALIVE:
        SetNPCStatus(npcdata, SM_NPC_ACTIVE);
        break;
    case SM_NOTE_DUPPAUSE:
        flg_spawner |= 0x4;
        break;
    case SM_NOTE_DUPRESUME:
        if ((flg_spawner & 0x2) == 0)
        {
            flg_spawner &= ~0x4;
        }
        break;
    case SM_NOTE_DUPSETDELAY:
        tym_delay = *(F32*)npcdata;
        tym_delay = tym_delay > 1.0f ? tym_delay : 1.0f;

        break;
    case SM_NOTE_DUPDEAD:
        if ((flg_spawner & 0x2) == 0)
        {
            ClearPending();
            flg_spawner |= 0x2;
            if ((wavestat == SM_STAT_DONE) && (flg_spawner & 0x20) == 0)
            {
                zEntEvent(npc_owner, eEventDuploDuperIsDoner);
            }
        }
        break;
    case SM_NOTE_KILLKIDS:
        ClearPending();
        flg_spawner |= 0x2;
        flg_spawner |= 0x10;
    }
}

U8 zNPCSpawner::Owned(zNPCCommon* npc) const
{
    S32 i;
    for (i = 0; i < 16; i++)
    {
        if (npcpool[i].npc == npc)
        {
            return 1;
        }
    }

    return 0;
}

U8 zNPCSpawner::Receivable(en_SM_NOTICES note, void* data) const
{
    switch (note)
    {
    case SM_NOTE_NPCDIED:
    case SM_NOTE_NPCSTANDBY:
    case SM_NOTE_NPCALIVE:
        return Owned((zNPCCommon*)data);
    case SM_NOTE_DUPPAUSE:
    case SM_NOTE_DUPRESUME:
    case SM_NOTE_DUPSETDELAY:
    case SM_NOTE_DUPDEAD:
        return 1;
    default:
        return 0;
    }
}

SMSPStatus* zNPCSpawner::SelectSP(const SMNPCStatus* npcstat)
{
    SMSPStatus* sp_stat;

    if (npcstat->sp_prefer != NULL)
    {
        sp_stat = StatForSP(npcstat->sp_prefer, 1);
        if (!sp_stat->sp->IsOn())
        {
            return NULL;
        }

        if (!IsSPLZClear(sp_stat->sp))
        {
            return NULL;
        }

        return sp_stat;
    }
    else
    {
        SMSPStatus* splist[16] = {};
        S32 cnt = 0;

        for (S32 i = 0; i < 16; i++)
        {
            SMSPStatus* tmp_stat = &sppool[i];
            if (tmp_stat->sp != NULL && tmp_stat->sp->on && tmp_stat->npc_prefer == NULL &&
                IsSPLZClear(tmp_stat->sp))
            {
                splist[cnt++] = tmp_stat;
            }
        }

        if (!cnt)
        {
            sp_stat = NULL;
        }
        else
        {
            sp_stat = xUtil_select<SMSPStatus>(splist, cnt, NULL);
        }
    }

    return sp_stat;
}

SMNPCStatus* zNPCSpawner::NextPendingNPC(S32 arg0)
{
    S32 cnt = this->pendlist.cnt;

    if (cnt < 1)
    {
        return NULL;
    }

    return xUtil_select<SMNPCStatus>((SMNPCStatus**)this->pendlist.list, cnt, NULL);
}

void zNPCSpawner::ClearActive()
{
    // NOTE: retail bug preserved deliberately - this walks actvlist.cnt entries
    // but reads them out of pendlist.list.
    for (S32 i = 0; i < actvlist.cnt; i++)
    {
        if (pendlist.list[i] != NULL)
        {
            ((SMNPCStatus*)pendlist.list[i])->status = SM_NPC_READY;
        }
    }

    XOrdReset(&actvlist);
}

void zNPCSpawner::ClearPending()
{
    for (S32 i = 0; i < pendlist.cnt; i++)
    {
        ((SMNPCStatus*)pendlist.list[i])->status = SM_NPC_READY;
    }

    XOrdReset(&pendlist);
}

st_XORDEREDARRAY* zNPCSpawner::FillPending()
{
    ClearPending();
    ReFillPending();
    return (st_XORDEREDARRAY*)this->pendlist.cnt;
}

st_XORDEREDARRAY* zNPCSpawner::ReFillPending()
{
    SMNPCStatus* npc_stat;
    S32 i;

    for (i = 0; i < 16; i++)
    {
        npc_stat = &npcpool[i];
        if (npc_stat->npc != NULL && npc_stat->status == SM_NPC_READY)
        {
            XOrdAppend(&pendlist, npc_stat);
            npc_stat->status = SM_NPC_PENDING;
        }
    }

    // NOTE: really returns pendlist.cnt; see FillPending.
    return (st_XORDEREDARRAY*)pendlist.cnt;
}

S32 zNPCSpawner::IsSPLZClear(zMovePoint* sp)
{
    xVec3 pos_sp = { 0.0f, 0.0f, 0.0f };
    S32 rc;
    xBound bnd;

    memset(&bnd, FALSE, sizeof(xBound));

    bnd.type = 0;
    xVec3Copy(&pos_sp, zMovePointGetPos(sp));

    bnd.type = 1;
    bnd.sph.r = 3.5f;
    xVec3Copy(&bnd.box.center, &pos_sp);

    xQuickCullForBound(&bnd.qcd, &bnd);

    if (g_drawSpawnBounds)
    {
        xDrawSetColor(g_CYAN);
        xBoundDraw(&bnd);
    }

    if (NPCC_chk_hitPlyr(&bnd, NULL))
    {
        return 0;
    }

    xVec3 delt = { 0.0f, 0.0f, 0.0f };

    xVec3Sub(&delt, xEntGetPos(&globals.player.ent), &pos_sp);

    if (SQ(delt.x) + SQ(delt.z) < SQ(3.5f))
    {
        return 0;
    }

    rc = IsNearbyMover(&bnd, TRUE, NULL);

    return rc ? 0 : 1;
}

S32 zNPCSpawner::IsNearbyMover(xBound* bnd, S32 usecyl, xCollis* caller_colrec)
{
    S32 hitthing = 0;
    zNPCCommon* npc;
    S32 i;
    xCollis local_colrec = {};
    xCollis* colrec;

    if (caller_colrec != NULL)
    {
        colrec = caller_colrec;
    }
    else
    {
        colrec = &local_colrec;
    }

    for (i = 0; i < globals.sceneCur->num_npcs; i++)
    {
        npc = (zNPCCommon*)globals.sceneCur->npcs[i];
        if (npc->chkby & 0x8 && npc->SelfType() != 'NTD0' && (npc->SelfType() & ~0xFF) != 'NTT\0')
        {
            xBoundHitsBound(bnd, &npc->bound, colrec);

            if (colrec->flags & 0x1)
            {
                hitthing++;
            }
            else if (usecyl)
            {
                xVec3 delt = { 0.0f, 0.0f, 0.0f };

                NPCC_pos_ofBase(npc, &delt);

                xVec3SubFrom(&delt, &bnd->sph.center);

                if (SQ(delt.x) + SQ(delt.z) < SQ(bnd->cyl.r))
                {
                    hitthing++;
                }
            }

            if (hitthing)
            {
                break;
            }
        }
    }

    return hitthing;
}

void zNPCSpawner::SetNPCStatus(zNPCCommon* npc, en_SM_NPC_STATUS status)
{
    SMNPCStatus* stat = this->StatForNPC(npc);
    if (stat != NULL)
    {
        stat->status = status;
    }
}

SMSPStatus* zNPCSpawner::StatForSP(zMovePoint* sp, S32 arg1)
{
    SMSPStatus* spstat = NULL;

    S32 i;
    for (i = 0; i < 16; i++)
    {
        SMSPStatus* tmp_stat = &sppool[i];
        if (tmp_stat->sp != NULL && tmp_stat->sp == sp)
        {
            spstat = tmp_stat;
            break;
        }
    }

    return spstat;
}

SMNPCStatus* zNPCSpawner::StatForNPC(zNPCCommon* npc)
{
    SMNPCStatus* tmp_stat;
    SMNPCStatus* npc_stat = NULL;
    S32 i;

    for (i = 0; i < 16; i++)
    {
        tmp_stat = &npcpool[i];
        if (tmp_stat->npc != NULL && tmp_stat->npc == npc)
        {
            npc_stat = tmp_stat;
            break;
        }
    }

    return npc_stat;
}

S32 zNPCSpawner::SpawnBeastie(SMNPCStatus* npcstat, SMSPStatus* spstat)
{
    zNPCCommon* npc = npcstat->npc;
    zMovePoint* sp = spstat->sp;
    xVec3 pos_sp = { 0, 0, 0 };
    zMovePoint* nav_dest = NULL;

    zMovePointGetNext(sp, sp, &nav_dest, NULL);

    if (nav_dest == NULL)
    {
        nav_dest = sp;
    }

    xVec3Copy(&pos_sp, sp->PosGet());

    npcstat->status = SM_NPC_SPAWNED;
    XOrdRemove(&pendlist, npcstat, -1);
    XOrdAppend(&actvlist, npcstat);

    npc->Respawn(&pos_sp, nav_dest, sp);

    cnt_spawn++;

    zEntEvent(npc_owner, eEventDuploNPCBorn);

    return 1;
}

SMNPCStatus* zNPCSpawner::ToastedBeastie(zNPCCommon* npc)
{
    SMNPCStatus* ret = this->StatForNPC(npc);
    XOrdRemove(&this->actvlist, ret, -1);
    zEntEvent((xBase*)this->npc_owner, eEventDuploNPCKilled);
    return ret;
}

void zNPCSpawner::ChildHeartbeat(F32 dt)
{
}

void zNPCSpawner::ChildCleanup(F32 dt)
{
    S32 i;
    SMNPCStatus* npc_stat;
    S32 cnt_know;
    S32 cnt_dead;

    if (wavestat == SM_STAT_DONE || wavestat == SM_STAT_ABORT)
    {
        for (i = actvlist.cnt - 1; i >= 0; i--)
        {
            zNPCCommon* npc = ((SMNPCStatus*)actvlist.list[i])->npc;
            npc->Damage(DMGTYP_INSTAKILL, NULL, NULL);
        }

        if (actvlist.cnt == 0 && pendlist.cnt == 0)
        {
            cnt_know = 0;
            cnt_dead = 0;
            for (i = 0; i < 16; i++)
            {
                npc_stat = &npcpool[i];
                if (npc_stat->npc != NULL)
                {
                    cnt_know = 1;
                    if (npc_stat->status == SM_NPC_READY)
                    {
                        cnt_dead = 1;
                    }
                    break;
                }
            }

            if (cnt_dead == cnt_know)
            {
                flg_spawner &= ~0x10;
            }
        }

        cnt_cleanup++;
    }
}

S32 zMovePoint::IsOn()
{
    // Cast is required by calling functions to occur in here
    // even though a single byte is moved into the return register
    return (S32)this->on;
}
