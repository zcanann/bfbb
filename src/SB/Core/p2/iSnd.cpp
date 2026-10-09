#include "iSnd.h"

#include "xSnd.h"
#include "xMath.h"
#include "xVec3.h"
#include "xVec3Inlines.h"
#include "xstransvc.h"
#include "xpkrsvc.h"

#include "iSystem.h"
#include "iTime.h"

#include "his/HISAPI.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <types.h>

// Where the sound code last was, for diagnosing hangs inside the IOP calls.
enum eSndWhereEnum
{
    eSndWhere_NA,
    eSndWhere_iSndSuspendCD,
    eSndWhere_iSndLoadFasterer,
    eSndWhere_UpdateVoiceStatus,
    eSndWhere_UpdateRecycleVoices,
    eSndWhere_UpdateDebugVoices,
    eSndWhere_Update3DSounds,
    eSndWhere_UpdateFireCallbacks,
    eSndWhere_UpdateFlushAsync,
    eSndWhere_DelayFrame,
    eSndWhere_PlayStart,
    eSndWhere_PlayTrigger,
    eSndWhere_PlaySndAsync,
    eSndWhere_PlaySndDone,
    eSndWhere_PlayExternalAsync,
    eSndWhere_PlayExternalWait,
    eSndWhere_PlayExternalDone,
    eSndWhere_PlayStream,
    eSndWhere_PlayStreamAsync,
    eSndWhere_PlayStreamDone,
    eSndWhere_StartStereo,
    eSndWhere_FindFreeVoice,
    eSndWhere_Pause
};

// Sound bank asset ('SNDI'): a VAG header for every effect, then every stream.
struct VAGheader
{
    char id[4];
    U32 version;
    U32 assetID;
    U32 data_size;
    U32 frequency;
    U32 streamInterleaveSize;
    U32 streamInterleaveCount;
    U32 reserved2[1];
    char name[16];
};

struct SndInfo
{
    U32 num_effects;
    U32 num_streams;
    VAGheader vagHeaders[1];
};

static char* gSndWhere_str[] = { "eSndWhere_NA",
                                 "eSndWhere_iSndSuspendCD",
                                 "eSndWhere_iSndLoadFasterer",
                                 "eSndWhere_UpdateVoiceStatus",
                                 "eSndWhere_UpdateRecycleVoices",
                                 "eSndWhere_UpdateDebugVoices",
                                 "eSndWhere_Update3DSounds",
                                 "eSndWhere_UpdateFireCallbacks",
                                 "eSndWhere_UpdateFlushAsync",
                                 "eSndWhere_DelayFrame",
                                 "eSndWhere_PlayStart",
                                 "eSndWhere_PlayTrigger",
                                 "eSndWhere_PlaySndAsync",
                                 "eSndWhere_PlaySndDone",
                                 "eSndWhere_PlayExternalAsync",
                                 "eSndWhere_PlayExternalWait",
                                 "eSndWhere_PlayExternalDone",
                                 "eSndWhere_PlayStream",
                                 "eSndWhere_PlayStreamAsync",
                                 "eSndWhere_PlayStreamDone",
                                 "eSndWhere_StartStereo",
                                 "eSndWhere_FindFreeVoice",
                                 "eSndWhere_Pause" };

static eSndWhereEnum gSndWhere;
static iSndFileInfo eeFiles[512];
static S32 eeFileCount;
static U32 currentStatus[2];
static iSndExternalCallback externalCallback;
static U32 currentSPUAddress;

// SPU2 pitch: 4096 plays a sample at the 48 kHz output rate; pitch is in semitones.
static inline U16 iSndCalcPitch(U32 sample_rate, F32 pitch)
{
    return 4096.0f * ((F32)sample_rate / 48000.0f * powf(2.0f, pitch / 12.0f));
}

static inline iSndVol iSndCalcVolLR(F32 left, F32 right)
{
    iSndVol vol;

    left = CLAMP(left, -1.0f, 1.0f);
    right = CLAMP(right, -1.0f, 1.0f);
    vol.volL = 16383.0f * left;
    vol.volR = 16383.0f * right;

    return vol;
}

iSndFileInfo* iSndLookup(U32 id)
{
    for (S32 i = 0; i < eeFileCount; i++)
    {
        if (eeFiles[i].assetID == id)
        {
            return &eeFiles[i];
        }
    }

    return NULL;
}

void iSndSuspendCD(U32)
{
    gSndWhere = eSndWhere_iSndSuspendCD;
    HISFlushHostIOHandles();
    gSndWhere = eSndWhere_NA;
}

void iSndInit()
{
}

void iSndLoadFasterer()
{
    U32 size;
    S32 index;
    SndInfo* sndInfo;
    VAGheader* vagIterator;
    st_PKR_ASSET_TOCINFO xinfo;
    char* hipname;
    S32 fileIndex;
    U32 startSPUAddress;
    U32 totalToRead;

    eeFileCount = 0;
    gSndWhere = eSndWhere_iSndLoadFasterer;
    currentSPUAddress = 0x45010;

    for (index = 0;; index++)
    {
        sndInfo = (SndInfo*)xSTFindAssetByType('SNDI', index, &size);
        if (sndInfo == NULL)
        {
            break;
        }

        if (sndInfo->num_effects == 0 && sndInfo->num_streams == 0)
        {
            continue;
        }

        vagIterator = sndInfo->vagHeaders;
        xSTGetAssetInfo(vagIterator->assetID, &xinfo);
        hipname = xST_xAssetID_HIPFullPath(xinfo.aid);
        fileIndex = HISGetFileIndex(hipname);
        startSPUAddress = currentSPUAddress;
        totalToRead = 0;

        // Effects are resident in SPU memory, packed one after another.
        for (S32 i = 0; i < sndInfo->num_effects; i++)
        {
            iSndFileInfo* file = &eeFiles[eeFileCount];

            file->ID = eeFileCount + 0x1000;
            file->assetID = vagIterator->assetID;
            file->is_streamed = 0;
            file->sample_rate = vagIterator->frequency;
            file->nonstream.address = currentSPUAddress;
            file->nonstream.size = vagIterator->data_size;
            vagIterator++;
            eeFileCount++;
            currentSPUAddress += file->nonstream.size;
            totalToRead += file->nonstream.size;
        }

        // Streams play straight off the disc.
        for (S32 i = 0; i < sndInfo->num_streams; i++)
        {
            st_PKR_ASSET_TOCINFO xinfo;
            xSTGetAssetInfo(vagIterator->assetID, &xinfo);

            iSndFileInfo* file = &eeFiles[eeFileCount];

            file->ID = eeFileCount - sndInfo->num_effects;
            file->assetID = vagIterator->assetID;
            file->is_streamed = 1;
            file->sample_rate = vagIterator->frequency;
            file->stream.file_index = fileIndex;
            file->stream.lsn = xinfo.sector;
            file->stream.data_size = vagIterator->data_size;
            file->stream.stream_interleave_size = vagIterator->streamInterleaveSize >> 11;
            file->stream.stream_interleave_count = vagIterator->streamInterleaveCount;
            eeFileCount++;
            vagIterator++;
        }

        if (totalToRead != 0)
        {
            iTimeGet();
            HISLoadBlock(fileIndex, xinfo.sector, (totalToRead + 0x7FF) >> 11,
                         (void*)startSPUAddress, HIS_MEMORY_SPU, 0, 0);
            iTimeGet();
        }
    }

    gSndWhere = eSndWhere_NA;
}

void iSndInitSceneLoaded()
{
    iTimeGet();
    iSndLoadFasterer();
    iTimeGet();
}

void iSndExit()
{
}

void iSndSetEnvironmentalEffect(isound_effect)
{
}

iSndVol iSndCalcVol(xSndVoiceInfo* vp)
{
    F32 volL;
    F32 volR;
    xVec3 to;
    F32 pan;
    F32 volscale;
    F32 fadeRange;
    F32 tmp;

    if (vp->flags & 0x8)
    {
        xVec3Sub(&to, &vp->playPos, &gSnd.pos);
        tmp = xVec3Length2(&to);
        xVec3Normalize(&to, &to);

        if (tmp <= vp->outerRadius2)
        {
            pan = 0.7853982f * (1.0f + xVec3Dot(&to, &gSnd.right));

            if (tmp <= vp->innerRadius2)
            {
                volscale = 1.0f;
            }
            else
            {
                fadeRange = vp->outerRadius2 - vp->innerRadius2;
                volscale = (fadeRange - (tmp - vp->innerRadius2)) / fadeRange;
            }

            volL = volscale * (vp->vol * cosf(pan));
            volR = volscale * (vp->vol * sinf(pan));
        }
        else
        {
            volL = volR = 0.0f;
        }
    }
    else
    {
        volL = volR = vp->vol;
    }

    volL *= gSnd.categoryVolFader[vp->category];
    volR *= gSnd.categoryVolFader[vp->category];

    if (!gSnd.stereo)
    {
        volL = volR = MAX(volL, volR);
    }

    if (vp->flags & 0x80)
    {
        volR = 0.0f;
    }

    if (vp->flags & 0x100)
    {
        volL = 0.0f;
    }

    return iSndCalcVolLR(volR, volL);
}

void iSndUpdate()
{
    gSndWhere = eSndWhere_UpdateVoiceStatus;
    HISGetVoiceStatus(currentStatus);

    gSndWhere = eSndWhere_UpdateRecycleVoices;
    {
        S32 i;
        xSndVoiceInfo* vp;
        U8 active;

        for (i = 0; i < XSND_VOICE_COUNT; i++)
        {
            vp = &gSnd.voice[i];
            active = (currentStatus[(U32)i / 32] & (1 << ((U32)i % 32))) != 0;

            if ((vp->flags & 0x1) && (vp->flags & 0x80) && active)
            {
                if (vp->flags & 0x80)
                {
                    vp->flags ^= 0x80;
                }
            }

            if ((vp->flags & 0x1) && (vp->flags & 0x40) && !active)
            {
                if (vp->flags & 0x1)
                {
                    vp->flags ^= 0x1;
                }
                if (vp->flags & 0x40)
                {
                    vp->flags ^= 0x40;
                }
                if (vp->flags & 0x80)
                {
                    vp->flags ^= 0x80;
                }
            }

            if ((vp->flags & 0x1) && !(vp->flags & 0x80) && !(vp->flags & 0x40) && !active)
            {
                if (vp->flags & 0x1)
                {
                    vp->flags ^= 0x1;
                }
            }
        }
    }

    gSndWhere = eSndWhere_Update3DSounds;
    {
        S32 i;
        xSndVoiceInfo* vp;
        iSndVol nvol;

        for (i = 0; i < XSND_VOICE_COUNT; i++)
        {
            vp = &gSnd.voice[i];
            if (!(vp->flags & 0x80) && !(vp->flags & 0x40) && (vp->flags & 0x1))
            {
                xSndInternalUpdateVoicePos(vp);
                vp->ps.vol = nvol = iSndCalcVol(vp);
                HISSetVoiceVolumeAsync(i, (U16)nvol.volL, (U16)nvol.volR);
            }
        }
    }

    gSndWhere = eSndWhere_UpdateFireCallbacks;
    {
        S32 i;
        xSndVoiceInfo* vp;
        S32 testBuffer;

        for (i = 0; i < 4; i++)
        {
            vp = &gSnd.voice[i];
            if ((vp->flags & 0x401) == 0x401 && externalCallback != NULL)
            {
                testBuffer = HISGetExternalStreamBuffer(i);
                if (testBuffer != vp->ps.lastStreamBuffer)
                {
                    externalCallback(vp->sndID, vp->ps.lastStreamBuffer == -1);
                    if (vp->ps.lastStreamBuffer == -1)
                    {
                        vp->ps.lastStreamBuffer = 1;
                    }
                    else
                    {
                        vp->ps.lastStreamBuffer = testBuffer;
                    }
                }
            }
        }
    }

    gSndWhere = eSndWhere_UpdateFlushAsync;
    HISFlushAsyncRequestsNoWait();
    gSndWhere = eSndWhere_NA;
}

U8 iSndIsPlaying(U32 assetID)
{
    S32 i;

    for (i = 0; i < XSND_VOICE_COUNT; i++)
    {
        if (gSnd.voice[i].assetID == assetID && (gSnd.voice[i].flags & 0x1))
        {
            return 1;
        }
    }

    return 0;
}

U8 iSndIsPlaying(U32 assetID, U32 parid)
{
    S32 i;

    for (i = 0; i < XSND_VOICE_COUNT; i++)
    {
        if ((assetID == 0 || gSnd.voice[i].assetID == assetID) &&
            gSnd.voice[i].parentID == parid && (gSnd.voice[i].flags & 0x1))
        {
            return 1;
        }
    }

    return 0;
}

U8 iSndIsPlayingByHandle(U32 id)
{
    S32 i;

    for (i = 0; i < XSND_VOICE_COUNT; i++)
    {
        if (gSnd.voice[i].sndID == id)
        {
            return (gSnd.voice[i].flags & 0x1) != 0;
        }
    }

    return 0;
}

U32 iSndPlay(xSndVoiceInfo* vp)
{
    iSndVol nvol;
    U32 voice;
    U32 pitch;

    gSndWhere = eSndWhere_PlayStart;
    vp->ps.lastStreamBuffer = -1;
    vp->ps.vol = nvol = iSndCalcVol(vp);
    vp->ps.pitch = pitch = iSndCalcPitch(vp->sample_rate, vp->pitch);

    gSndWhere = eSndWhere_PlayTrigger;
    voice = ((U32)vp - (U32)gSnd.voice) / sizeof(xSndVoiceInfo);

    if (vp->flags & 0x2)
    {
        xSTAssetName(vp->assetID);
        gSndWhere = eSndWhere_PlaySndAsync;

        iSndFileInfo* file = iSndLookup(vp->assetID);
        if (file == NULL)
        {
            printf("** can't find sound asset %d (%08X)\n", vp->assetID, vp->assetID);
        }
        else
        {
            HISPlaySoundAsync(voice, nvol.volL, nvol.volR, (U16)pitch,
                              file->nonstream.address, 0, 4, 0);
        }

        gSndWhere = eSndWhere_PlaySndDone;
    }
    else if (vp->flags & 0x400)
    {
        gSndWhere = eSndWhere_PlayExternalAsync;
        HISPlayExternalStreamAsync(voice, nvol.volL, nvol.volR, pitch, 0, 0, 4, 0x6B80);
        gSndWhere = eSndWhere_PlayExternalWait;
        HISFlushAsyncRequestsNoWait();
        gSndWhere = eSndWhere_PlayExternalDone;
    }
    else
    {
        xSTAssetName(vp->assetID);
        gSndWhere = eSndWhere_PlayStream;

        iSndFileInfo* file = iSndLookup(vp->assetID);
        if (file == NULL)
        {
            printf("** can't find sound asset %d (%08X)\n", vp->assetID, vp->assetID);
        }
        else
        {
            U8 loop = (vp->flags & 0x8000) != 0;
            U8 paused = (vp->flags & 0x40000) != 0;

            if (file->stream.stream_interleave_size != 0)
            {
                // Interleaved music: each track is one block of the interleave.
                U32 numTracks = file->stream.stream_interleave_count;
                U32 track = (vp->flags & 0x7800) >> 11;
                U32 offset = file->stream.stream_interleave_size;
                gSndWhere = eSndWhere_PlayStreamAsync;
                HISPlayStreamAsync(voice, nvol.volL, nvol.volR, (U16)pitch,
                                   file->stream.file_index, file->stream.lsn + track * offset,
                                   file->stream.data_size, loop | (paused ? 2 : 0), 0, 4,
                                   offset << 11, (numTracks - 1) * offset);
            }
            else
            {
                U32 streamFlags;
                if (loop)
                {
                    if (vp->flags & 0x20000)
                    {
                        streamFlags = 0;
                    }
                    else
                    {
                        streamFlags = 4;
                    }
                }
                else
                {
                    streamFlags = 0;
                }
                streamFlags |= loop | (paused ? 2 : 0);
                gSndWhere = eSndWhere_PlayStreamAsync;
                HISPlayStreamAsync(voice, nvol.volL, nvol.volR, (U16)pitch,
                                   file->stream.file_index, file->stream.lsn,
                                   file->stream.data_size, streamFlags, 0, 4, 0x8000, 0);
            }

            gSndWhere = eSndWhere_PlayStreamDone;
        }
    }

    vp->flags |= 0x80;
    vp->flags &= ~0x40000;
    gSndWhere = eSndWhere_NA;

    return vp->sndID;
}

U8 iSndIsReady(U32 id)
{
    S32 v;

    for (v = 0; v < 4; v++)
    {
        if ((gSnd.voice[v].flags & 0x5) && gSnd.voice[v].sndID == id)
        {
            return (currentStatus[(U32)v / 32] & (1 << ((U32)v % 32))) != 0;
        }
    }

    return 0;
}

void iSndStartStereo(U32 id1, U32 id2, F32 pitch)
{
    S32 v1;
    S32 v2;
    U16 ps2pitch;

    gSndWhere = eSndWhere_StartStereo;

    for (v1 = 0; v1 < XSND_VOICE_COUNT; v1++)
    {
        if ((gSnd.voice[v1].flags & 0x1) && gSnd.voice[v1].sndID == id1)
        {
            break;
        }
    }

    if (v1 == XSND_VOICE_COUNT)
    {
        return;
    }

    for (v2 = 0; v2 < XSND_VOICE_COUNT; v2++)
    {
        if ((gSnd.voice[v2].flags & 0x1) && gSnd.voice[v2].sndID == id2)
        {
            break;
        }
    }

    if (v2 == XSND_VOICE_COUNT)
    {
        return;
    }

    ps2pitch = iSndCalcPitch(gSnd.voice[v1].sample_rate, pitch);
    HISSetVoicePitchAsync(v1, ps2pitch);
    HISSetVoicePitchAsync(v2, ps2pitch);
    HISJoinStereoVoicesAsync(v1, v2);

    gSndWhere = eSndWhere_NA;
}

// Retail performs the two signed integer absolute values out of line.
#pragma inline_intrinsics off
U32 iSndFindFreeVoice(U32 priority, U32 flags, U32 owner)
{
    U32 i;
    U32 vlo;
    U32 vhi;
    U32 best;
    U32 bestpri;
    S32 bestvol;
    xSndVoiceInfo* vp;

    gSndWhere = eSndWhere_FindFreeVoice;
    best = 999;
    bestpri = 99999;
    bestvol = 99999;

    if (flags & 0x4)
    {
        // The first four voices are reserved for locked (owned) sounds.
        vlo = 0;
        vhi = 4;

        xSndVoiceInfo* begin = &gSnd.voice[vlo];
        xSndVoiceInfo* end = begin + (vhi - vlo);
        for (xSndVoiceInfo* v = begin; v != end; v++)
        {
            if (v->lock_owner != 0 && v->lock_owner == owner)
            {
                if ((v->flags & 0x41) == 0x1)
                {
                    iSndStop(v->sndID);
                }

                gSndWhere = eSndWhere_NA;
                return v - begin;
            }
        }
    }
    else
    {
        vlo = 4;
        vhi = XSND_VOICE_COUNT;
    }

    for (i = vlo, vp = &gSnd.voice[vlo]; i < vhi; i++, vp++)
    {
        if (vp->lock_owner != 0)
        {
            continue;
        }

        if (!(vp->flags & 0x1))
        {
            gSndWhere = eSndWhere_NA;
            return i;
        }

        if (vp->flags & 0xC0)
        {
            continue;
        }

        if (vp->priority > bestpri)
        {
            continue;
        }

        S32 volR = vp->ps.vol.volR;
        S32 L = abs(vp->ps.vol.volL);
        S32 R = abs(volR);
        L = MAX(L, R);

        if (L < bestvol)
        {
            bestpri = vp->priority;
            bestvol = L;
            best = i;
        }
    }

    if (best == 999)
    {
        gSndWhere = eSndWhere_NA;
        return -1;
    }

    if (priority <= bestpri)
    {
        gSndWhere = eSndWhere_NA;
        return -1;
    }

    xSTAssetName(gSnd.voice[best].assetID);
    iSndStop(gSnd.voice[best].sndID);

    gSndWhere = eSndWhere_NA;
    return best;
}

#pragma inline_intrinsics reset

void iSndPause(U32 snd, U32 pause)
{
    S32 i;

    gSndWhere = eSndWhere_Pause;

    for (i = 0; i < XSND_VOICE_COUNT; i++)
    {
        if ((gSnd.voice[i].flags & 0x1) && gSnd.voice[i].sndID == snd)
        {
            if (pause)
            {
                HISPauseVoiceAsync(i);
            }
            else
            {
                HISResumeVoiceAsync(i);
            }
            return;
        }
    }

    gSndWhere = eSndWhere_NA;
}

void iSndStop(U32 snd)
{
    S32 i;

    for (i = 0; i < XSND_VOICE_COUNT; i++)
    {
        if ((gSnd.voice[i].flags & 0x1) && gSnd.voice[i].sndID == snd)
        {
            HISStopVoiceAsync(i);

            if (gSnd.voice[i].flags & 0x80)
            {
                gSnd.voice[i].flags ^= 0x80;
            }

            if (gSnd.voice[i].flags & 0x10)
            {
                gSnd.voice[i].flags ^= 0x10;
            }

            gSnd.voice[i].flags |= 0x41;
            break;
        }
    }
}

void iSndSetVol(U32 snd, F32 vol)
{
    xSndVoiceInfo* vp;
    S32 i;

    vp = gSnd.voice;
    for (i = 0; i < XSND_VOICE_COUNT; i++, vp++)
    {
        if ((vp->flags & 0x1) && vp->sndID == snd)
        {
            vol *= gSnd.categoryVolFader[vp->category];
            vp->vol = vol;
            vp->ps.vol = iSndCalcVolLR((vp->flags & 0x80) ? 0.0f : vol,
                                       (vp->flags & 0x100) ? 0.0f : vol);
            HISSetVoiceVolumeAsync(i, vp->ps.vol.volL, vp->ps.vol.volR);
        }
    }
}

F32 iSndGetVol(U32 snd)
{
    xSndVoiceInfo* vp;
    S32 i;

    vp = gSnd.voice;
    for (i = 0; i < XSND_VOICE_COUNT; i++, vp++)
    {
        if ((vp->flags & 0x1) && vp->sndID == snd)
        {
            if (gSnd.categoryVolFader[vp->category] <= 0.0f)
            {
                return 0.0f;
            }

            return vp->vol / gSnd.categoryVolFader[vp->category];
        }
    }

    return 0.0f;
}

void iSndSetPitch(U32 snd, F32 pitch)
{
    S32 i;

    for (i = 0; i < XSND_VOICE_COUNT; i++)
    {
        if ((gSnd.voice[i].flags & 0x1) && gSnd.voice[i].sndID == snd)
        {
            HISSetVoicePitchAsync(i, iSndCalcPitch(gSnd.voice[i].sample_rate, pitch));
            break;
        }
    }
}

void iSndWaitForDeadSounds()
{
    S32 i;
    S32 numdelay = 0;
    unsigned long zombies;
    xSndVoiceInfo* vp;

    while (true)
    {
        zombies = 0;

        vp = gSnd.voice;
        for (i = 0; i < XSND_VOICE_COUNT; i++, vp++)
        {
            if ((vp->flags & 0x41) == 0x41 && vp->deadct > 1)
            {
                printf("zombie voice on channel %d:  flags %04X deadct %d\n", i, vp->flags,
                       vp->deadct);
                zombies |= 1L << i;
            }
        }

        if (zombies == 0)
        {
            break;
        }

        printf("zombie voices:  %08X\n", zombies);
        iSndUpdate();
        iVSync();
        numdelay++;
    }

    if (numdelay != 0)
    {
        printf("--- iSndWaitForDeadSounds = %d vblanks\n\n", numdelay);
    }
}

void iSndLoadExternalData(U32 snd, const void* data, S32 forceBuffer)
{
    xSndVoiceInfo* vp;
    S32 i;

    vp = gSnd.voice;
    for (i = 0; i < XSND_VOICE_COUNT; i++, vp++)
    {
        if ((vp->flags & 0x401) == 0x401 && vp->sndID == snd)
        {
            HISFlushAsyncRequests();
            HISLoadExternalStream(
                i, forceBuffer == -1 ? HISGetExternalStreamBuffer(i) : forceBuffer, (void*)data);
        }
    }
}

void iSndSetExternalCallback(iSndExternalCallback callback)
{
    externalCallback = callback;
}

void iSndStereo(U32 stereo)
{
    gSnd.stereo = stereo;
}
