#include "xSnd.h"

#include <types.h>

#include "iSnd.h"
#include "xVec3.h"

extern F32 sTimeElapsed;

static _xSndDelayed sDelayedSnd[16] = { 0 };
static U32 sDelayedPaused;

xSndGlobals gSnd;

namespace
{
    class fade_data
    {
        // total size: 0x18
    public:
        U8 in; // offset 0x0, size 0x1
        U32 handle; // offset 0x4, size 0x4
        F32 start_delay; // offset 0x8, size 0x4
        F32 time; // offset 0xC, size 0x4
        F32 end_time; // offset 0x10, size 0x4
        F32 volume; // offset 0x14, size 0x4
    };

    fade_data faders[128];
    S32 faders_active;
}

void xSndInit()
{
    iSndInit();
    xSndVoiceInfo* voice = gSnd.voice;
    for (S32 i = 0; i < 64; i++, voice++)
    {
        voice->flags = 0;
        voice->lock_owner = 0;
    }

    xSndSceneInit();

    //Need this to only use f0 instead of f1 for 1.0f
    for (int i = 0; i < 5; i++)
    {
        gSnd.categoryVolFader[i] = 1.0f;
    }

    gSnd.categoryVolFader[2] = 0.7f;

    gSnd.stereo = 1;
    iSndSuspendCD(1);
    xSndDelayedInit();
    reset_faders();
}

void xSndSceneInit()
{
    gSnd.listenerMode = SND_LISTENER_MODE_PLAYER;
    for (U32 i = 0; i < 2; i++)
    {
        gSnd.listenerMat[i].at.assign(1.0f, 0.0f, 0.0f);
        gSnd.listenerMat[i].right.assign(0.0f, 1.0f, 0.0f);
        gSnd.listenerMat[i].up.assign(0.0f, 0.0f, 1.0f);
        gSnd.listenerMat[i].pos.assign(999999.0f, 999999.0f, 999999.0f);
    }
    gSnd.at.assign(1.0f, 0.0f, 0.0f);
    gSnd.right.assign(0.0f, 1.0f, 0.0f);
    gSnd.up.assign(0.0f, 0.0f, 1.0f);
    gSnd.pos.assign(999999.0f, 999999.0f, 999999.0f);
    iSndUpdate();
}

void xSndSetEnvironmentalEffect(sound_effect effectType)
{
    switch (effectType)
    {
    case SND_EFFECT_NONE:
        iSndSetEnvironmentalEffect(iSND_EFFECT_NONE);
        break;
    case SND_EFFECT_CAVE:
        iSndSetEnvironmentalEffect(iSND_EFFECT_CAVE);
        break;
    default:
        break;
    }
}

void xSndSuspend()
{
    xSndPauseAll(1, 1);
    sDelayedPaused = 1;
    xSndUpdate();
}

void xSndResume()
{
    xSndPauseAll(0, 0);
    sDelayedPaused = 0;
}

void xSndPauseAll(U32 pause_effects, U32 pause_streams)
{
    sDelayedPaused = pause_effects;

    for (U32 i = 0; i < 0x40; i++)
    {
        if (gSnd.voice[i].flags & 1)
        {
            if (gSnd.voice[i].flags & 2)
            {
                iSndPause(gSnd.voice[i].sndID, pause_effects);
            }
            else if (gSnd.voice[i].flags & 4)
            {
                iSndPause(gSnd.voice[i].sndID, pause_streams);
            }
        }
    }
}

void xSndPauseCategory(U32 mask, U32 pause)
{
    for (U32 i = 0; i < 0x40; i++)
    {
        if ((gSnd.voice[i].flags & 1) && (mask & 1 << gSnd.voice[i].category))
        {
            iSndPause(gSnd.voice[i].sndID, pause);
        }
    }
}

void xSndSetCategoryVol(sound_category category, F32 vol)
{
    gSnd.categoryVolFader[category] = vol;
}

void xSndStopAll(U32 mask)
{
    for (U32 i = 0; i < 0x40; i++)
    {
        if ((gSnd.voice[i].flags & 1) && (mask & 1 << gSnd.voice[i].category))
        {
            iSndStop(gSnd.voice[i].sndID);
        }
    }
    xSndDelayedInit();
}

void xSndDelayedInit()
{
    for (int i = 0; i < 16; i++)
    {
        //Alternates between f1 and f0
        sDelayedSnd[i].delay = 0.0f;
    }
    sDelayedPaused = 0;
}

void xSndDelayedUpdate()
{
    if (sDelayedPaused)
    {
        return;
    }

    _xSndDelayed* snd = sDelayedSnd;

    for (S32 i = 0; i < 16; i++, snd++)
    {
        if (snd->delay > 0.0f)
        {
            snd->delay -= sTimeElapsed;
            if (snd->delay < 0.0f)
            {
                snd->delay = 0.0f;
                xSndPlayInternal(snd->id, snd->vol, snd->pitch, snd->priority, snd->flags,
                                 snd->parentID, snd->parentEnt, snd->pos, snd->innerRadius,
                                 snd->outerRadius, snd->category, 0.0f);
            }
        }
    }
}

void xSndAddDelayed(U32 id, F32 vol, F32 pitch, U32 priority, U32 flags, U32 parentID, xEnt* parentEnt, xVec3* pos, F32 innerRadius, F32 outerRadius, sound_category category, F32 delay)
{
    _xSndDelayed* snd = &sDelayedSnd[0];

    for (U32 i = 0x10; i != 0; i--)
    {
        if (snd->delay <= 0.0f)
        {
            snd->id = id;
            snd->vol = vol;
            snd->pitch = pitch;
            snd->priority = priority;
            snd->flags = flags;
            snd->parentID = parentID;
            snd->parentEnt = parentEnt;
            snd->pos = pos;
            snd->innerRadius = innerRadius;
            snd->outerRadius = outerRadius;
            snd->category = category;
            snd->delay = delay;
            return;
        }
        snd++;
    }
}

void xSndCalculateListenerPosition()
{
    xMat4x3* pMat;

    switch (gSnd.listenerMode)
    {
    case SND_LISTENER_MODE_PLAYER:
        pMat = &gSnd.listenerMat[0];
        gSnd.right = pMat->right;
        gSnd.up = pMat->up;
        gSnd.at = pMat->at;
        gSnd.pos = gSnd.listenerMat[1].pos;
        gSnd.pos.y += 1.0f;
        break;
    case SND_LISTENER_MODE_CAMERA:
        pMat = &gSnd.listenerMat[0];
        gSnd.right = pMat->right;
        gSnd.up = pMat->up;
        gSnd.at = pMat->at;
        gSnd.pos = gSnd.listenerMat[0].pos;
        break;
    default:
        break;
    }
}

void xSndProcessSoundPos(const xVec3* pActual, xVec3* pProcessed) {
    xVec3 temp_f;
    xVec3 playerDelta;

    F32 factor;
    F32 inwardShift;

    switch (gSnd.listenerMode) {
    case SND_LISTENER_MODE_PLAYER:
        temp_f = *pActual - gSnd.listenerMat[1].pos;
        playerDelta = *pActual - gSnd.listenerMat[0].pos;
        factor = xVec3Length(&temp_f);
        inwardShift = xVec3Length(&playerDelta);
        if (inwardShift < factor) {
            inwardShift = factor - inwardShift;
            inwardShift /= 2.0f;
            temp_f *= (factor - inwardShift) / factor;
            *pProcessed = temp_f + gSnd.listenerMat[1].pos;
            return;
        }
        *pProcessed = *pActual;
        return;
    case SND_LISTENER_MODE_CAMERA:
        *pProcessed = *pActual;
        return;
    }
}

void xSndInternalUpdateVoicePos(xSndVoiceInfo* pVoice)
{
    if ((pVoice->flags & 1) && (pVoice->flags & 8))
    {
        if (pVoice->flags & 0x10)
        {
            if (pVoice->parentPos != NULL)
            {
                pVoice->actualPos = *pVoice->parentPos;
            }
            else if (pVoice->parentID != 0)
            {
                xEnt* ent = (xEnt*)(pVoice->parentID & 0xfffffffc); // uhh...
                if (pVoice->flags & 0x800)
                {
                    pVoice->actualPos = *(xVec3*)(ent);
                }
                else if (ent->baseFlags & 1)
                {
                    xVec3Copy(&pVoice->actualPos, xEntGetPos(ent));
                }
                else
                {
                    pVoice->flags &= 0xffffffef;
                }
            }
            // Unreachable: the enclosing `if` already required flags & 8.
            // The compiler folds it away, which is why this function still
            // matches -- do not "fix" it into a plain `else`, that would make
            // the store live and change the emitted code.
            else if (!(pVoice->flags & 8))
            {
                pVoice->actualPos = gSnd.pos;
            }
        }
        xSndProcessSoundPos(&pVoice->actualPos, &pVoice->playPos);
    }
}

void xSndUpdate()
{
    xSndCalculateListenerPosition();
    xSndDelayedUpdate();
    update_faders(sTimeElapsed);
    iSndUpdate();
}

void xSndSetListenerData(sound_listener_type listenerType, const xMat4x3* pMat)
{
    // Not out of bounds: sound_listener_type has exactly the two values
    // listenerMat[2] holds, and zGame/zSaveLoad only ever pass those two.
    //
    // The trap here is that the two listener enums are ordered OPPOSITELY --
    // sound_listener_type is CAMERA=0, PLAYER=1, while sound_listener_game_mode
    // is PLAYER=0, CAMERA=1 -- and both end up indexing this same array.
    // xSndCalculateListenerPosition and xSndProcessSoundPos therefore index it
    // with literal 0/1 rather than with a listenerMode value; keep it that way.
    int i = (int)listenerType;
    gSnd.listenerMat[i] = *pMat;
}

void xSndSelectListenerMode(sound_listener_game_mode listenerMode)
{
    gSnd.listenerMode = listenerMode;
}

void xSndExit()
{
    iSndExit();
    reset_faders();
}

U32 xSndPlay(U32 id, F32 vol, F32 pitch, U32 priority, U32 flags, U32 parentID,
             sound_category category, F32 delay)
{
    return xSndPlayInternal(id, vol, pitch, priority, flags, parentID, NULL, NULL, 0.0f, 0.0f,
                            category, delay);
}

U32 xSndPlay3D(U32 id, F32 vol, F32 pitch, U32 priority, U32 flags, xEnt* parent, F32 innerRadius,
               F32 outerRadius, sound_category category, F32 delay)
{
    return xSndPlayInternal(id, vol, pitch, priority, flags, NULL, parent, NULL, innerRadius,
                            outerRadius, category, delay);
}

U32 xSndPlay3D(U32 id, F32 vol, F32 pitch, U32 priority, U32 flags, const xVec3* pos,
               F32 innerRadius, F32 outerRadius, sound_category category, F32 delay)
{
    if (flags & 0x800)
    {
        return xSndPlayInternal(id, vol, pitch, priority, flags, NULL, (xEnt*)pos, NULL,
                                innerRadius, outerRadius, category, delay);
    }
    else
    {
        return xSndPlayInternal(id, vol, pitch, priority, flags, NULL, NULL, pos, innerRadius,
                                outerRadius, category, delay);
    }
}

// iSndLookup() on GameCube hands back a pointer to iSnd.cpp's file-scope
// `snd` object: a 0x64-byte DSP-ADPCM header followed by the internal sound
// id. src/SB/Core/gc/iSnd.h still describes iSndFileInfo with the PS2 layout
// (sample_rate is a U16 at 0x8 there, and there is nothing at 0x64), so the
// two fields this function needs are reached through the real layout instead.
struct iSndLookupInfo
{
    U32 num_samples; // 0x00
    U32 num_nibbles; // 0x04
    U32 sample_rate; // 0x08
    U8 pad0C[0x58]; // 0x0C
    S32 ID; // 0x64
};

bool xSndCategoryGetsEffects(sound_category category);

U32 xSndPlayInternal(U32 id, F32 vol, F32 pitch, U32 priority, U32 flags, U32 parentID,
                     xEnt* parentEnt, const xVec3* pos, F32 innerRadius, F32 outerRadius,
                     sound_category category, F32 delay)
{
    if (innerRadius > outerRadius)
    {
        F32 temp = innerRadius;
        innerRadius = outerRadius;
        outerRadius = temp;
    }

    vol = CLAMP(vol, 0.0f, 1.0f);

    if (delay > 0.0f)
    {
        xSndAddDelayed(id, vol, pitch, priority, flags, parentID, parentEnt, (xVec3*)pos,
                       innerRadius, outerRadius, category, delay);
        return 0;
    }

    if (flags & 0x10000)
    {
        if (parentID != 0 || parentEnt != NULL)
        {
            for (U32 i = 0; i < 64; i++)
            {
                if (gSnd.voice[i].assetID == id &&
                    (gSnd.voice[i].parentID == parentID ||
                     gSnd.voice[i].parentID == (U32)parentEnt) &&
                    (gSnd.voice[i].flags & 1))
                {
                    return 0;
                }
            }
        }
        else if (pos != NULL)
        {
            for (U32 i = 0; i < 64; i++)
            {
                if (gSnd.voice[i].assetID == id && gSnd.voice[i].parentPos == pos &&
                    (gSnd.voice[i].flags & 1))
                {
                    return 0;
                }
            }
        }
        else
        {
            for (U32 i = 0; i < 64; i++)
            {
                if (gSnd.voice[i].assetID == id && gSnd.voice[i].parentPos == NULL &&
                    gSnd.voice[i].parentID == 0 && (gSnd.voice[i].flags & 1))
                {
                    return 0;
                }
            }
        }
    }

    U32 sample_rate;
    S32 internalID;

    if (flags & 0x400)
    {
        flags |= 4;

        iSndLookupInfo* ip = (iSndLookupInfo*)iSndLookup(id);
        if (ip == NULL)
        {
            return 0;
        }

        sample_rate = ip->sample_rate;
        internalID = -1;
    }
    else
    {
        iSndLookupInfo* ip = (iSndLookupInfo*)iSndLookup(id);
        if (ip == NULL)
        {
            return 0;
        }

        if (ip->ID >= 0x1000)
        {
            flags |= 2;
        }
        else
        {
            flags |= 4;
        }

        if (xSndCategoryGetsEffects(category))
        {
            flags |= 0x20;
        }

        sample_rate = ip->sample_rate;
        internalID = ip->ID;
    }

    U32 voice = iSndFindFreeVoice(priority, flags, parentID);
    if (voice == 0xffffffff)
    {
        return 0;
    }

    xSndVoiceInfo* vp = &gSnd.voice[voice];

    vp->priority = priority;
    vp->pitch = pitch;
    vp->vol = vol;
    vp->assetID = id;
    vp->internalID = internalID;
    vp->sndID = ++gSnd.SndCount;
    vp->sample_rate = sample_rate;
    vp->flags = flags | 1;
    vp->deadct = 0;
    vp->category = category;

    if (parentEnt != NULL)
    {
        vp->flags |= 0x18;
        vp->parentID = (U32)parentEnt;
        vp->parentPos = NULL;
        vp->innerRadius2 = innerRadius * innerRadius;
        vp->outerRadius2 = (outerRadius <= 0.0f) ? 1000000.0f : outerRadius * outerRadius;

        if (flags & 0x800)
        {
            vp->actualPos = *(xVec3*)((U32)parentEnt & 0xfffffffc);
        }
        else
        {
            vp->actualPos = *xEntGetPos((xEnt*)((U32)parentEnt & 0xfffffffc));
        }
    }
    else if (pos != NULL)
    {
        vp->flags |= 8;
        vp->parentID = 0;
        vp->parentPos = (flags & 0x10) ? (xVec3*)pos : NULL;
        vp->actualPos = *pos;
        vp->innerRadius2 = innerRadius * innerRadius;
        vp->outerRadius2 = (outerRadius <= 0.0f) ? 1000000.0f : outerRadius * outerRadius;
    }
    else
    {
        vp->parentID = 0;
        vp->parentPos = NULL;
        vp->actualPos = gSnd.pos;
        vp->innerRadius2 = 16.0f;
        vp->outerRadius2 = 25.0f;
    }

    if (vp->flags & 8)
    {
        xSndInternalUpdateVoicePos(vp);
    }

    U32 played = iSndPlay(vp);
    if (!played)
    {
        vp->flags &= 0xfffffffe;
        return 0;
    }

    return vp->sndID;
}

void xSndStartStereo(U32 id1, U32 id2, F32 pitch)
{
    iSndStartStereo(id1, id2, pitch);
}

U32 xSndIDIsPlaying(U32 sndID)
{
    xSndVoiceInfo* voice = gSnd.voice;
    for (int i = 0; i < 64; i++, voice++)
    {
        if (voice->flags & 1 && voice->sndID == sndID)
        {
            return 1;
        }
    }
    return 0;
}

void xSndStop(U32 snd)
{
    iSndStop(snd);
}

void xSndParentDied(U32 pid)
{
    xSndVoiceInfo* voice = gSnd.voice;
    for (S32 i = 0; i < 64; i++, voice++)
    {
        if (voice->parentID == pid)
        {
            voice->flags = voice->flags & 0xffffffef;
        }
    }
}

void xSndStopChildren(U32 pid)
{
    U32 i = 0;
    xSndVoiceInfo* voice = gSnd.voice;
    for (; i < 64; i++, voice++)
    {
        if ((voice->flags & 1) != 0 && voice->parentID == pid)
        {
            iSndStop(voice->sndID);
            voice->flags = voice->flags & 0xffffffef;
        }
    }
}

void xSndSetVol(U32 snd, F32 vol)
{
    iSndSetVol(snd, vol);
}

void xSndSetPitch(U32 snd, F32 pitch)
{
    iSndSetPitch(snd, pitch);
}

void xSndSetExternalCallback(void (*callback)(U32))
{
    iSndSetExternalCallback(callback);
}

void reset_faders()
{
    faders_active = 0;
}

F32 xSndGetVol(U32 snd);

void update_faders(F32 dt)
{
    fade_data* it = faders;
    fade_data* end = faders + faders_active;

    while (it != end)
    {
        it->start_delay -= dt;
        if (it->start_delay > 0.0f)
        {
            it++;
            continue;
        }

        it->time += dt;
        if (it->time >= it->end_time)
        {
            if (it->in)
            {
                xSndSetVol(it->handle, it->volume);
            }
            else
            {
                xSndStop(it->handle);
            }

            faders_active--;
            end--;
            if (it == end)
            {
                break;
            }

            *it = *end;
        }
        else
        {
            F32 volume = it->time / it->end_time;
            if (!it->in)
            {
                volume = 1.0f - volume;
            }

            // Retail squares the target volume here; reproduced as shipped.
            volume *= it->volume;
            xSndSetVol(it->handle, volume * it->volume);
            it++;
        }
    }
}

U32 xSndPlay3DFade(U32 id, F32 vol, F32 pitch, U32 priority, U32 flags, const xVec3* pos,
                   F32 innerRadius, F32 outerRadius, sound_category category, F32 delay,
                   F32 fade_time)
{
    if (faders_active >= 128 || fade_time <= 0.0f)
    {
        return xSndPlay3D(id, vol, pitch, priority, flags, pos, innerRadius, outerRadius, category,
                          delay);
    }

    U32 handle = xSndPlay3D(id, 0.0f, pitch, priority, flags, pos, innerRadius, outerRadius,
                            category, delay);
    if (handle == 0)
    {
        return 0;
    }

    fade_data& f = faders[faders_active];
    f.in = 1;
    f.handle = handle;
    f.start_delay = delay;
    f.time = 0.0f;
    f.end_time = fade_time;
    f.volume = vol;
    faders_active++;

    return handle;
}

void xSndStopFade(U32 snd, F32 fade_time)
{
    if (snd == 0)
    {
        return;
    }

    if (faders_active >= 128 || fade_time <= 0.0f)
    {
        xSndStop(snd);
        return;
    }

    F32 vol = xSndGetVol(snd);
    if (vol <= 0.0f)
    {
        xSndStop(snd);

        fade_data* it = faders;
        fade_data* end = faders + faders_active;

        while (it != end)
        {
            if (it->handle == snd)
            {
                faders_active--;
                end--;
                if (it == end)
                {
                    return;
                }

                *it = *end;
            }
            else
            {
                it++;
            }
        }

        return;
    }

    fade_data* it = faders;
    fade_data* end = faders + faders_active;

    while (it != end)
    {
        if (it->handle == snd)
        {
            break;
        }

        it++;
    }

    if (it == end)
    {
        faders_active++;
    }

    it->in = 0;
    it->handle = snd;
    it->start_delay = 0.0f;
    it->time = 0.0f;
    it->end_time = fade_time;
    it->volume = vol;
}

U8 xSndStreamLock(U32 owner, sound_category kill_cat, bool kill_nonlooping)
{
    xSndVoiceInfo* begin = gSnd.voice;
    xSndVoiceInfo* end = begin + 6;

    for (xSndVoiceInfo* v = begin; v != end; v++)
    {
        if (v->lock_owner == owner)
        {
            return 1;
        }
    }

    for (xSndVoiceInfo* v = begin; v != end; v++)
    {
        if (v->lock_owner == 0 && !(v->flags & 1))
        {
            v->lock_owner = owner;
            return 1;
        }
    }

    for (xSndVoiceInfo* v = begin; v != end; v++)
    {
        if (v->lock_owner == 0 && (v->flags & 0x40))
        {
            v->lock_owner = owner;
            return 1;
        }
    }

    if (kill_cat != SND_CAT_INVALID)
    {
        for (xSndVoiceInfo* v = begin; v != end; v++)
        {
            if (v->lock_owner == 0 && v->category == kill_cat)
            {
                xSndStop(v->sndID);
                v->lock_owner = owner;
                return 1;
            }
        }
    }

    if (kill_nonlooping)
    {
        for (xSndVoiceInfo* v = begin; v != end; v++)
        {
            if (v->lock_owner == 0 && !(v->flags & 0x8000))
            {
                xSndStop(v->sndID);
                v->lock_owner = owner;
                return 1;
            }
        }
    }

    return 0;
}

U32 xSndStreamReady(U32 owner)
{
    xSndVoiceInfo* begin = gSnd.voice;
    xSndVoiceInfo* end = begin + 6;

    for (xSndVoiceInfo* v = begin; v != end; v++)
    {
        if (v->lock_owner == owner)
        {
            if (v->flags & 1)
            {
                return 0;
            }
            else
            {
                return 1;
            }
        }
    }

    return 0;
}

void xSndStreamUnlock(U32 owner)
{
    xSndVoiceInfo* begin = gSnd.voice;
    xSndVoiceInfo* end = begin + 6;

    for (xSndVoiceInfo* v = begin; v != end; v++)
    {
        if (v->lock_owner == owner)
        {
            v->lock_owner = 0;
            return;
        }
    }
}

bool xSndCategoryGetsEffects(sound_category category)
{
    return category == SND_CAT_GAME || category == SND_CAT_DIALOG;
}

F32 xSndGetVol(U32 snd)
{
    return iSndGetVol(snd);
}
