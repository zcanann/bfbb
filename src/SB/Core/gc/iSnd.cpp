#include "iSnd.h"

#include "dolphin/ai.h"
#include "dolphin/os.h"
#include "dolphin/os/OSAlloc.h"
#include "dolphin/os/OSCache.h"
#include "iCutscene.h"
#include "iMemMgr.h"
#include "iTRC.h"
#include "xFile.h"
#include "xSnd.h"
#include "xString.h"
#include "xstransvc.h"
#include "xMath.h"

#include <dolphin/ar.h>
#include <dolphin/ax.h>
#include <dolphin/dvd/dvd.h>
#include <dolphin/mix.h>
#include <rwplcore.h>

#include <PowerPC_EABI_Support/MSL_C/MSL_Common/cmath>
#include <PowerPC_EABI_Support/MSL_C/MSL_Common/stdlib.h>
#include <stdio.h>
#include <string.h>
#include <types.h>

// FIXME: declared in zGame.h, which Core must not include
void zGameScreenTransitionUpdate(F32 percentComplete, char* msg, U8* rgba);

u32 aram_array[40];

// Private to this translation unit; iSndMessWithEA calls it before the
// definition, so it needs the forward declaration.
U32 SampleToNybbleAddress(U32 sample);

// Size: 0x20
// This was not in dwarf data
struct vinfo
{
    _AXVPB* voice;
    U32 flags;
    U32 aid;
    F32 xc;
    S32 x10;
    S32 x14;
    S32 x18;
    S32 x1c;
};

// The GC DSPADPCM header, with the asset ID appended. This is what iSnd.h calls
// sDSPADPCM (see iSndMessWithEA, which writes buffer[5] == loop_end).
// Size: 0x64
struct sndhdr
{
    U32 num_samples; // 0x00
    U32 num_nibbles; // 0x04
    U32 sample_rate; // 0x08
    U16 loop_flag; // 0x0C
    U16 format; // 0x0E
    U32 loop_start; // 0x10
    U32 loop_end; // 0x14
    U32 cur_addr; // 0x18
    S16 coef[16]; // 0x1C
    U16 gain; // 0x3C
    U16 pred_scale; // 0x3E
    U16 yn1; // 0x40
    U16 yn2; // 0x42
    U16 loop_pred_scale; // 0x44
    U16 loop_yn1; // 0x46
    U16 loop_yn2; // 0x48
    U16 pad[11]; // 0x4A -- pad[0] is tagged 0x63 for memory streams
    U32 assetID; // 0x60
};

// The header of a loaded SNDI asset.
struct sndinfo
{
    U32 num_sfx; // 0x00
    U32 total_size; // 0x04
    U32 num_streams; // 0x08
    U32 num_cutscene; // 0x0C
    sndhdr entry[1]; // 0x10
};

// Size: 0x11c
struct sinfo
{
    tag_xFile file; // 0x000
    U32 base; // 0x114
    sndinfo* info; // 0x118
};

// Size: 0x180
struct sndlookup
{
    sndhdr hdr; // 0x000
    U32 id; // 0x064
    tag_xFile file; // 0x068
    U32 pad; // 0x17c
};

// Size: 0x10c
// Looks like this might be a vinfo struct at the beginning here.
struct UNK_STREAM
{
    vinfo vinf;
    sndhdr hdr;
    U32 x84;
    U32 offset;
    U32 x8c;
    U32 x90;
    U32 x94;
    DVDFileInfo fileInfo;
    U32 source_a;
    u32 dest_a;
    u32 dest_b;
    U32 source_b;
    U32 xe4;
    ARQRequest request;
    U32 x108;
};

UNK_STREAM streams[6];

vinfo voices[58];

sinfo sinfo_array[12];

sndlookup snd;

U32* ua_stream_buffer = NULL; //unaligned stream buffer
U32* stream_buffer = 0;
u32 silence_buffer = 0;
volatile u32 zero_point = 0;
volatile u32 zero_end = 0;
S32 sinfo_array_max = 0;
volatile U32 SoundFlags = 0;
volatile S32 fc = 0;
static char soundInited = 0;
U32 houston_we_have_a_problem = 0;

ARQRequest* last_ar;

static void dv_callback(void* userdata)
{
    struct UNK_VOIDSTAR
    {
        U8 pad[0x14];
        U32 stream;
    };

    UNK_VOIDSTAR* data = (UNK_VOIDSTAR*)userdata;

    if (!soundInited)
    {
        return;
    }

    U32 stream = data->stream;
    xSndVoiceInfo* info = &gSnd.voice[stream];
    info->sndID = 0;
    info->flags = 0;
    if (stream < sizeof(streams) / sizeof(UNK_STREAM))
    {
        U32 idx = stream;
        if (streams[idx].vinf.voice == NULL)
        {
            return;
        }

        DVDCancelAsync(&streams[idx].fileInfo.cb, NULL);
        ARQRemoveRequest(&streams[idx].request);
        AXSetVoiceState(streams[idx].vinf.voice, 0);
        MIXReleaseChannel(streams[idx].vinf.voice);

        streams[idx].vinf.voice = NULL;
        streams[idx].vinf.flags = 0x40000;
        streams[idx].vinf.aid = 0;
    }
    else
    {
        U32 idx = stream - sizeof(streams) / sizeof(UNK_STREAM);
        if (voices[idx].voice == NULL)
        {
            return;
        }
        MIXReleaseChannel(voices[idx].voice);
        AXSetVoiceState(voices[idx].voice, 0);

        voices[idx].voice = NULL;
        voices[idx].flags = 0;
        voices[idx].aid = 0;
    }
}

static void arq_callback(u32)
{
    if (!soundInited)
    {
        return;
    }
    SoundFlags = 0;
}

static const char* dump_flags(U32 flags)
{
    static char str[0x40];

    memset(str, 0, sizeof(str));

    char* p = str;
    char cvt[5] = "-01X";
    *p++ = 'L';
    *p++ = cvt[(flags & 0x200) ? 1 : (flags & 0x400) ? 2 : 0];
    *p++ = 'D';
    *p++ = cvt[(flags & 0x1000) ? 1 : (flags & 0x2000) ? 2 : 0];
    *p++ = 'P';
    *p++ = cvt[(flags & 0x4000) ? 1 : (flags & 0x8000) ? 2 : 0];
    *p++ = (flags & 0x400000) ? 'F' : '-';
    *p++ = 'R';
    *p++ = (flags & 0x100) ? 'X' : '-';
    *p++ = 'D';
    *p++ = (flags & 0x800) ? 'X' : '-';

    return str;
}

static void arqcb(u32 pointerToARQRequest);

// FIXME: param name
static void dvdcb(s32 r3, DVDFileInfo* info)
{
    if (!soundInited)
    {
        return;
    }

    UNK_STREAM* data = (UNK_STREAM*)info->cb.userData;
    xSTAssetName(data->vinf.aid);
    dump_flags(data->vinf.flags);
    if (data->vinf.voice == NULL)
    {
        return;
    }

    if (r3 <= 0)
    {
        return;
    }

    data->offset += r3;
    data->x90 += r3;
    data->vinf.flags |= 0x800;
    data->vinf.flags &= ~0x100;
    if (data->vinf.flags & (0x200 | 0x400))
    {
        houston_we_have_a_problem = TRUE;
    }
    if ((data->vinf.flags & 0x4) == 0)
    {
        return;
    }

    U32 dest = 0;
    if ((data->vinf.flags & 0x1000) == 0)
    {
        data->vinf.flags |= 0x200;
        dest = data->dest_a;
    }
    else if ((data->vinf.flags & 0x2000) == 0)
    {
        data->vinf.flags |= 0x400;
        dest = data->dest_b;
    }

    if (dest != 0)
    {
        ARQPostRequest(&data->request, (u32)data, 0, 1, data->source_a, dest, 0x4000, arqcb);
    }
}

static void arqcb(u32 pointerToARQRequest)
{
    if (!soundInited)
    {
        return;
    }

    UNK_STREAM* data = (UNK_STREAM*)((ARQRequest*)pointerToARQRequest)->owner;

    xSTAssetName(data->vinf.aid);
    dump_flags(data->vinf.flags);
    ARQRequest* req = (ARQRequest*)pointerToARQRequest;
    last_ar = req;
    if (req->length == 0 || data->vinf.voice == NULL)
    {
        return;
    }

    if (data->vinf.flags & 0x100)
    {
        houston_we_have_a_problem = TRUE;
    }
    if (data->vinf.flags & 0x1000000)
    {
        if (data->vinf.flags & 0x200)
        {
            data->vinf.flags |= 0x1000;
            data->vinf.flags &= ~0x200;
        }
        if (data->vinf.flags & 0x400)
        {
            data->vinf.flags |= 0x2000;
            data->vinf.flags &= ~0x400;
        }

        data->x90 += data->xe4;
        data->xe4 = 0;
        data->source_b = 0;
    }
    else if (data->vinf.flags & 0x200)
    {
        data->vinf.flags |= 0x1000;
        data->vinf.flags &= ~0x200;
    }
    else if (data->vinf.flags & 0x400)
    {
        data->vinf.flags |= 0x2000;
        data->vinf.flags &= ~0x400;
    }

    if (data->x8c - data->x90 == 0)
    {
        xSTAssetName(data->vinf.aid);
        data->vinf.flags |= 0x20;
    }
    else if ((data->vinf.flags & 0x1000000) == 0)
    {
        data->vinf.flags |= 0x200000;
    }

    data->vinf.flags &= ~0x800;
    data->vinf.flags &= ~0x4000000;
    data->vinf.flags &= ~0x400000;
    if (data->vinf.flags & 0x4)
    {
        data->vinf.flags &= ~4;
        data->vinf.flags |= 0x8;
    }
}

static void iSndMyAXFree(AXVPB**);

static void fcb()
{
    if (!soundInited && iTRCDisk::IsDiskIDed())
    {
        return;
    }

    fc++;
    S32 i;
    S32 need_update = FALSE;
    for (i = 0; i < 6; i++)
    {
        U32 orig_flags = streams[i].vinf.flags;
        u32 addr;
        xSndVoiceInfo* gsnd_voice = &gSnd.voice[i];
        if (streams[i].vinf.voice == NULL)
        {
            continue;
        }

        u16 addrHi = streams[i].vinf.voice->pb.addr.currentAddressHi;
        if (orig_flags & 0xe0006)
        {
            continue;
        }
        addr = addrHi << 16;
        addr += streams[i].vinf.voice->pb.addr.currentAddressLo;
        U32 dest = streams[i].dest_b;
        dump_flags(orig_flags);

        if (addr < dest * 2 && (orig_flags & 0x4000) == 0 && (orig_flags & 0x8) == 0)
        {
            streams[i].vinf.flags |= 0x4000;
            streams[i].vinf.flags &= ~0x8000;
            streams[i].vinf.flags &= ~0x2000;
            if ((orig_flags & 0x1000000) && streams[i].source_b == 0)
            {
                streams[i].vinf.flags |= 0x2000000;
            }
            streams[i].x94 = streams[i].x90;
            orig_flags = streams[i].vinf.flags;
        }
        else if (addr >= dest * 2 && (orig_flags & 0x8000) == 0)
        {
            streams[i].vinf.flags |= 0x8000;
            streams[i].vinf.flags &= ~0x4000;
            streams[i].vinf.flags &= ~0x1000;
            if ((orig_flags & 0x1000000))
            {
                streams[i].vinf.flags |= 0x2000000;
            }
            orig_flags = streams[i].vinf.flags;
        }

        if ((orig_flags & 0x20) && (orig_flags & 0x4000))
        {
            streams[i].vinf.flags &= ~0x20;
            dest = streams[i].dest_a;
            dest *= 2;
            dest += streams[i].hdr.num_nibbles & 0xFFFF;
            AXSetVoiceLoopAddr(streams[i].vinf.voice, zero_point);
            AXSetVoiceEndAddr(streams[i].vinf.voice, dest - 1);
            AXSetVoiceLoop(streams[i].vinf.voice, 0);
            AXSetVoiceType(streams[i].vinf.voice, 0);
            streams[i].vinf.flags |= 0x40;
            orig_flags = streams[i].vinf.flags;
        }

        if (orig_flags & 0x100000)
        {
            iSndMyAXFree(&streams[i].vinf.voice);
            streams[i].vinf.flags = 0;
            streams[i].vinf.aid = 0;
            streams[i].source_b = 0;
            streams[i].xe4 = 0;
            continue;
        }

        if (addr >= zero_point && addr < zero_end && (orig_flags & 0x40))
        {
            streams[i].vinf.flags &= ~0x40;
            DVDCancelAsync(&streams[i].fileInfo.cb, NULL);
            ARQRemoveRequest(&streams[i].request);
            AXSetVoiceState(streams[i].vinf.voice, 0);
            MIXReleaseChannel(streams[i].vinf.voice);

            if (streams[i].vinf.flags & 0x10000)
            {
                u32 priority = gsnd_voice->priority;
                if (priority > 0xff)
                {
                    priority = 0xff;
                }

                iSndMyAXFree(&streams[i].vinf.voice);
                streams[i].vinf.voice = AXAcquireVoice(priority >> 3, dv_callback, i);
                if (streams[i].vinf.voice == NULL)
                {
                    streams[i].vinf.voice = NULL;
                    streams[i].vinf.flags = 0;
                    streams[i].vinf.aid = 0;
                    streams[i].source_b = 0;
                    streams[i].xe4 = 0;
                    continue;
                }

                MIXInitChannel(streams[i].vinf.voice, 0xc, 0, -0x384, -0x384, 0x40, 0x7f, -0x384);
                streams[i].vinf.flags = 0x4;
                iSndPlay(gsnd_voice);
                continue;
            }
            streams[i].vinf.flags = 0x40000;
            continue;
        }

        if (orig_flags & 0x8)
        {
            if ((streams[i].vinf.flags & 0x2) == 0)
            {
                AXSetVoiceState(streams[i].vinf.voice, 1);
            }
            MIXUnMute(streams[i].vinf.voice);
            need_update = TRUE;
            streams[i].vinf.flags |= 0x10;
            streams[i].vinf.flags |= 0x4000;
            streams[i].vinf.flags &= ~0x8;
            orig_flags = streams[i].vinf.flags;
        }

        if ((orig_flags & 0x200000) && (orig_flags & 0x200) == 0 && (orig_flags & 0x400) == 0 &&
            (orig_flags & 0x100) == 0 && (orig_flags & 0x400000) == 0 &&
            (orig_flags & 0x1000000) == 0)
        {
            streams[i].vinf.flags &= ~0x200000;
            streams[i].vinf.flags |= 0x100;
            streams[i].vinf.flags |= 0x400000;

            U32 size = streams[i].x8c - streams[i].x90;
            if (size > 0x4000)
            {
                DVDReadAsyncPrio((DVDFileInfo*)&streams[i].fileInfo.cb, (void*)streams[i].source_a,
                                 0x4000, streams[i].offset, dvdcb, 2);
            }
            else
            {
                DVDReadAsyncPrio((DVDFileInfo*)&streams[i].fileInfo.cb, (void*)streams[i].source_a,
                                 size, streams[i].offset, dvdcb, 2);
            }
        }
        if ((orig_flags & 0x800) || (orig_flags & 0x4000000))
        {
            U32 source;
            U32 length;
            if (orig_flags & 0x800)
            {
                length = 0x4000;
                if (streams[i].x90 & 0x3FFF)
                {
                    length = streams[i].x90 & 0x3FFF;
                }
                source = streams[i].source_a;
            }
            else
            {
                length = streams[i].xe4;
                source = streams[i].source_b;
            }

            U32 flags_bit_12;
            U32 flags_bit_10;
            U32 flags_bit_13;
            U32 flags_bit_9;
            flags_bit_9 = orig_flags & 0x200;
            flags_bit_12 = orig_flags & 0x1000;
            flags_bit_10 = orig_flags & 0x400;
            flags_bit_13 = orig_flags & 0x2000;
            dump_flags(orig_flags);

            if (flags_bit_12 && (orig_flags & 0x4000) && !flags_bit_10 && !flags_bit_13)
            {
                streams[i].vinf.flags |= 0x400;
                ARQPostRequest(&streams[i].request, (u32)&streams[i], 0, 1, source,
                               streams[i].dest_b, length, arqcb);
            }
            else if (flags_bit_13 && (orig_flags & 0x8000) && !flags_bit_9 && !flags_bit_12)
            {
                streams[i].vinf.flags |= 0x200;
                ARQPostRequest(&streams[i].request, (u32)&streams[i], 0, 1, source,
                               streams[i].dest_a, length, arqcb);
            }
        }

        if (streams[i].vinf.x10 == streams[i].vinf.x14 &&
            streams[i].vinf.x18 == streams[i].vinf.x1c)
        {
            continue;
        }

        streams[i].vinf.x14 = streams[i].vinf.x10;
        streams[i].vinf.x1c = streams[i].vinf.x18;
        MIXAdjustInput(streams[i].vinf.voice,
                       streams[i].vinf.x14 - MIXGetInput(streams[i].vinf.voice));
        MIXAdjustPan(streams[i].vinf.voice, streams[i].vinf.x1c - MIXGetPan(streams[i].vinf.voice));
        need_update = TRUE;
    }

    for (i = 0; i < 58; i++)
    {
        if (voices[i].voice == NULL || (voices[i].flags & 0x1))
        {
            continue;
        }

        if (voices[i].flags & 0x20000)
        {
            iSndMyAXFree(&voices[i].voice);
            voices[i].flags = NULL;
            voices[i].aid = 0;
        }
        else
        {
            U32 addr = (voices[i].voice->pb.addr.currentAddressHi << 16) +
                       voices[i].voice->pb.addr.currentAddressLo;
            if (voices[i].flags & 0x4 && !voices[i].voice->pb.addr.loopFlag && addr >= zero_point &&
                addr < zero_end)
            {
                voices[i].flags &= ~0x4;
                AXSetVoiceState(voices[i].voice, 0);
                MIXReleaseChannel(voices[i].voice);
                iSndMyAXFree(&voices[i].voice);
                voices[i].flags = NULL;
                voices[i].aid = 0;
            }
        }
    }

    if (need_update)
    {
        MIXUpdateSettings();
    }
}

void iSndInit()
{
    S32 i;

    soundInited = 1;
    AIInit(NULL);
    ARInit(aram_array, 40);
    ARQInit();
    AXInit();
    MIXInit();

    for (i = 0; i < 58; i++)
    {
        voices[i].voice = NULL;
        voices[i].flags = 0;
    }

    for (i = 0; i < 6; i++)
    {
        streams[i].vinf.voice = 0;
        streams[i].vinf.flags = 0;
        streams[i].source_a = 0;
        streams[i].dest_a = 0;
        streams[i].dest_b = 0;
        streams[i].source_b = 0;
        streams[i].xe4 = 0;
    }

    // FIXME: No idea what type this is, appears to only be used in this fuction.
    ua_stream_buffer = (U32*)OSAllocFromHeap(the_heap, 0x18100);
    stream_buffer = (U32*)(((U32)ua_stream_buffer + 0x1f) & ~0x1f);
    U32* local_buffer = stream_buffer;
    for (i = 0; i < 0x100; i++)
    {
        local_buffer[i] = NULL;
    }

    silence_buffer = ARAlloc(0x400);
    zero_point = silence_buffer;
    zero_point *= 2;
    zero_point += 2;
    zero_end = zero_point + 0x800;

    SoundFlags |= 0x1;

    BOOL enabled = OSDisableInterrupts();
    DCInvalidateRange((void*)silence_buffer, 0x400);
    ARQRequest request;
    ARQPostRequest(&request, 0, 0, 1, (u32)local_buffer, silence_buffer, 0x400, arq_callback);
    OSRestoreInterrupts(enabled);

    while (SoundFlags != 0)
        ;

    char str_buf[0x20];
    U32 offset = 0;

    for (i = 0; i < 6; i++)
    {
        u32 buf = ARAlloc(0x8000);
        sprintf(str_buf, "streaming channel %d\n", i);
        streams[i].dest_a = buf;
        streams[i].dest_b = buf + 0x4000;
        streams[i].source_a = (u32)stream_buffer + offset;
        offset += 0x4000;
    }
    AXRegisterCallback(fcb);
}

void iSndExit()
{
    soundInited = 0;
    AXQuit();
}

//not sure where this type is from.
void iSndSetEnvironmentalEffect(isound_effect)
{
    return;
}

void iSndInitSceneLoaded()
{
}

bool iSndIsPlaying(U32 assetID)
{
    if (assetID == 0)
    {
        return false;
    }

    for (S32 i = 0; i < 6; i++)
    {
        if (gSnd.voice[i].assetID == assetID)
        {
            if (streams[i].vinf.flags & 0xC000 && (streams[i].vinf.flags & 0x82) == 0)
            {
                return true;
            }
            return false;
        }
    }

    for (S32 i = 0; i < 0x3a; i++)
    {
        if (gSnd.voice[i + 6].assetID == assetID)
        {
            if (voices[i].flags & 0x4 && (voices[i].flags & 0x8) == 0)
            {
                return true;
            }
            return false;
        }
    }
    return true;
}

bool iSndIsPlaying(U32 assetID, U32 parid)
{
    for (U32 i = 0; i < 0x40; i++)
    {
        if ((assetID == 0 || gSnd.voice[i].assetID == assetID) && gSnd.voice[i].parentID == parid)
        {
            if (iSndIsPlaying(gSnd.voice[i].assetID))
            {
                return true;
            };
        }
    }
    return false;
}

bool iSndIsPlayingByHandle(U32 handle)
{
    if (handle == 0)
    {
        return false;
    }

    for (S32 i = 0; i < 6; i++)
    {
        if (gSnd.voice[i].sndID == handle)
        {
            return (streams[i].vinf.flags & 0xC000 && (streams[i].vinf.flags & 0x82) == 0);
        }
    }

    for (S32 i = 0; i < 0x3a; i++)
    {
        if (gSnd.voice[i + 6].sndID == handle)
        {
            return (voices[i].flags & 0x4 && (voices[i].flags & 0x8) == 0);
        }
    }
    return false;
}

static S32 sound_stream;

iSndFileInfo* iSndLookup(U32 id)
{
    static S32 strm_id = 1;
    static S32 snd_id = 0x1000;

    sound_stream = 0;

    if (id == 0)
    {
        return NULL;
    }

    for (S32 i = sinfo_array_max - 1; i >= 0; i--)
    {
        sndinfo* info = sinfo_array[i].info;
        if (info == NULL)
        {
            continue;
        }

        sndhdr* entry = info->entry;
        U32 n = info->num_sfx;
        U32 j = 0;

        for (; j < n; j++)
        {
            if (id == entry[j].assetID)
            {
                memcpy(&snd, &entry[j], sizeof(sndhdr));
                memcpy(&snd.file, &sinfo_array[i], sizeof(tag_xFile));
                snd.id = snd_id++;
                if (snd_id >= 0x7ffa)
                {
                    snd_id = 0x1000;
                }
                return (iSndFileInfo*)&snd;
            }
        }

        n = info->num_streams + n;
        for (; j < n; j++)
        {
            if (id == entry[j].assetID)
            {
                memcpy(&snd, &entry[j], sizeof(sndhdr));
                memcpy(&snd.file, &sinfo_array[i], sizeof(tag_xFile));
                snd.id = strm_id++;
                if (strm_id >= 0xffe)
                {
                    strm_id = 1;
                }
                if (entry[j].pad[0] == 0x63)
                {
                    snd.id = 0x1000;
                    sound_stream = 0;
                }
                else
                {
                    sound_stream = 1;
                }
                return (iSndFileInfo*)&snd;
            }
        }

        n = info->num_cutscene + n;
        for (; j < n; j++)
        {
            if (id == entry[j].assetID)
            {
                memcpy(&snd, &entry[j], sizeof(sndhdr));
                memcpy(&snd.file, &sinfo_array[i], sizeof(tag_xFile));
                snd.id = strm_id++;
                if (strm_id >= 0xffe)
                {
                    strm_id = 1;
                }
                sound_stream = 2;
                return (iSndFileInfo*)&snd;
            }
        }
    }

    return NULL;
}

void iSndPause(U32 snd, U32 pause)
{
    if (!soundInited)
    {
        return;
    }

    S32 i;
    for (i = 0; i < 64; i++)
    {
        if (gSnd.voice[i].sndID == snd)
            break;
    }

    if (i < 6)
    {
        if (streams[i].vinf.voice == NULL)
        {
            return;
        }
        if (pause)
        {
            AXSetVoiceState(streams[i].vinf.voice, 0);
            streams[i].vinf.flags |= 0x2;
        }
        else
        {
            AXSetVoiceState(streams[i].vinf.voice, 1);
            streams[i].vinf.flags &= ~0x2;
        }
    }
    else if (i < 64)
    {
        S32 j = i - 6;
        if (voices[j].voice == NULL)
        {
            return;
        }
        if (pause)
        {
            AXSetVoiceState(voices[j].voice, 0);
            voices[j].flags |= 0x8;
        }
        else
        {
            AXSetVoiceState(voices[j].voice, 1);
            voices[j].flags &= ~0x8;
        }
    }
}

void iSndStop(U32 snd)
{
    if (snd == 0)
    {
        return;
    }

    BOOL enabled = OSDisableInterrupts();

    S32 i;
    for (i = 0; i < 64; i++)
    {
        if (gSnd.voice[i].sndID == snd)
        {
            gSnd.voice[i].sndID = 0;
            gSnd.voice[i].flags = 0;
            break;
        }
    }

    if (i < 6)
    {
        if (streams[i].vinf.voice != NULL)
        {
            UNK_STREAM* pv = &streams[i];
            DVDCancel(&pv->fileInfo.cb);
            ARQRemoveRequest(&pv->request);
            AXSetVoiceState(pv->vinf.voice, 0);
            MIXReleaseChannel(pv->vinf.voice);
            iSndMyAXFree(&pv->vinf.voice);
            streams[i].vinf.flags = 0x40000;
            streams[i].vinf.aid = 0;
            streams[i].vinf.flags = 0x40000;
        }
    }
    else if (i < 64)
    {
        S32 j = i - 6;
        if (voices[j].voice != NULL)
        {
            vinfo* v = &voices[j];
            AXSetVoiceState(v->voice, 0);
            MIXReleaseChannel(v->voice);
            iSndMyAXFree(&v->voice);
            voices[j].flags = 0;
            voices[j].aid = 0;
        }
    }

    OSRestoreInterrupts(enabled);
}

U32 iVolFromX(F32 param1)
{
    float f = MAX(param1, 1e-20f);

    S32 i = 43.43f * xlog(f);
    S32 comp = MIN(i, 0);

    if (comp < -0x388)
    {
        return -0x388;
    }
    else
    {
        return MIN(i, 0);
    }
}

void iSndCalcVol(xSndVoiceInfo* vp, vinfo* info)
{
    S32 vol = iVolFromX(vp->vol * gSnd.categoryVolFader[vp->category]);

    info->x10 = vol;
    info->x14 = vol;
    info->x18 = 0x40;
    info->x1c = 0x40;
    MIXAdjustFader(info->voice, vol - MIXGetFader(info->voice));
    MIXAdjustPan(info->voice, 0x40 - MIXGetPan(info->voice));
}

void iSndCalcVol3d(xSndVoiceInfo* vp, vinfo* info)
{
    xVec3 to;

    xVec3Sub(&to, &vp->playPos, &gSnd.pos);
    F32 dist2 = xVec3Length2(&to);
    xVec3Normalize(&to, &to);
    F32 pan = xVec3Dot(&to, &gSnd.right);

    F32 volscale;
    if (dist2 > vp->outerRadius2)
    {
        volscale = 0.0f;
    }
    else if (dist2 <= vp->innerRadius2)
    {
        volscale = 1.0f;
    }
    else
    {
        F32 fadeRange = vp->outerRadius2 - vp->innerRadius2;
        volscale = std::sqrtf((fadeRange - (dist2 - vp->innerRadius2)) / fadeRange);
    }

    S32 ipan = (S32)(64.0f * pan) + 0x40;
    S32 vol = iVolFromX(volscale * (vp->vol * gSnd.categoryVolFader[vp->category]));

    if (ipan < 0)
    {
        ipan = 0;
    }
    else if (ipan > 0x7f)
    {
        ipan = 0x7f;
    }

    info->x10 = vol;
    info->x14 = vol;
    info->x18 = ipan;
    info->x1c = ipan;
    MIXAdjustFader(info->voice, vol - MIXGetFader(info->voice));
    MIXAdjustPan(info->voice, ipan - MIXGetPan(info->voice));
}

void iSndVolUpdate(xSndVoiceInfo* info, vinfo* vinfo)
{
    MIXUnMute(vinfo->voice);
    xSndInternalUpdateVoicePos(info);
    if ((info->flags & 8) != 0)
    {
        iSndCalcVol3d(info, vinfo);
    }
    else
    {
        iSndCalcVol(info, vinfo);
    }
}

U32 staticibuf;

static void iSndUpdateStreams()
{
    if (!soundInited)
    {
        return;
    }

    for (S32 i = 0; i < 6; i++)
    {
        if (streams[i].vinf.voice == NULL)
        {
            continue;
        }

        U32 flags = streams[i].vinf.flags;
        if (flags & 0x40000)
        {
            streams[i].vinf.flags = 0x80000;
            continue;
        }

        if (flags & 0x80000)
        {
            streams[i].vinf.flags = 0x100000;
            continue;
        }

        if (flags & 0x20000)
        {
            streams[i].vinf.flags = flags & ~0x20000;
            if ((streams[i].vinf.flags & 0x2) == 0)
            {
                AXSetVoiceState(streams[i].vinf.voice, 1);
            }
            continue;
        }

        xSndVoiceInfo* gv = &gSnd.voice[i];
        iSndVolUpdate(gv, &streams[i].vinf);

        flags = streams[i].vinf.flags;
        if ((flags & 0x1000000) == 0)
        {
            continue;
        }
        if ((flags & 0x2000000) == 0)
        {
            continue;
        }

        U32 ibuf = (U32)iCSSoundGetData(gv, &streams[i].xe4);
        streams[i].source_b = ibuf;
        if (ibuf == 0)
        {
            continue;
        }

        BOOL enabled = OSDisableInterrupts();
        if (streams[i].vinf.flags & 0x4)
        {
            u32 src = streams[i].source_b;
            streams[i].vinf.flags |= 0x200;
            streams[i].vinf.flags |= 0x400;
            ARQPostRequest(&streams[i].request, (u32)&streams[i], 0, 1, src, streams[i].dest_a,
                           0x8000, arqcb);
            streams[i].x108 = 0;
        }
        else
        {
            streams[i].x108++;
        }

        streams[i].vinf.flags &= ~0x2000000;
        streams[i].vinf.flags |= 0x4000000;
        staticibuf = streams[i].source_b;
        OSRestoreInterrupts(enabled);
    }
}

void iSndUpdateSounds()
{
    if (!soundInited)
    {
        return;
    }

    for (int i = 0; i < 58; i++)
    {
        if (voices[i].voice != NULL)
        {
            iSndVolUpdate(&gSnd.voice[i + 6], &voices[i]);
        }
    }
}

void iSndUpdate()
{
    if (!soundInited)
    {
        return;
    }

    BOOL enabled = OSDisableInterrupts();
    iSndUpdateStreams();
    iSndUpdateSounds();
    MIXUpdateSettings();
    OSRestoreInterrupts(enabled);

    for (S32 i = 0; i < 64; i++)
    {
        xSndVoiceInfo* vp = &gSnd.voice[i];
        U8 active;

        if (i < 6)
        {
            active = (streams[i].vinf.flags & 0xc07f) != 0;
        }
        else
        {
            U32 addr = 0;
            if (voices[i - 6].voice != NULL)
            {
                addr = (voices[i - 6].voice->pb.addr.currentAddressHi << 16) +
                       voices[i - 6].voice->pb.addr.currentAddressLo;
            }

            U32 f = voices[i - 6].flags;
            U8 done = 0;
            if ((f & 0x4) && !(f & 0x8) && addr >= zero_point && addr < zero_end)
            {
                done = 1;
            }

            active = done | (voices[i - 6].voice != NULL);
        }

        if (active)
        {
            vp->flags |= 0x1;
        }
        else
        {
            vp->flags &= ~0x1;
        }
    }
}

S32 iSndFindFreeVoice(U32 priority, U32 flags, U32 owner)
{
    if (priority > 0xff)
    {
        priority = 0xff;
    }

    if (flags & 0x4)
    {
        if (owner != 0)
        {
            xSndVoiceInfo* begin = gSnd.voice;
            xSndVoiceInfo* end = &begin[6];

            for (xSndVoiceInfo* vp = begin; vp != end; vp++)
            {
                S32 i = vp - begin;

                if (vp->lock_owner == 0)
                {
                    continue;
                }
                if (vp->lock_owner != owner)
                {
                    continue;
                }

                if ((vp->flags & 0x41) == 1)
                {
                    iSndStop(vp->sndID);
                }

                BOOL enabled = OSDisableInterrupts();
                _AXVPB* voice = AXAcquireVoice(priority >> 3, dv_callback, i);
                if (voice != NULL)
                {
                    AXSetVoiceState(voice, 0);
                    MIXInitChannel(voice, 0xc, 0, -0x384, -0x384, 0x40, 0x7f, -0x384);
                    streams[i].vinf.voice = voice;
                    streams[i].vinf.flags = 0;
                    streams[i].vinf.flags |= 0x4;
                    OSRestoreInterrupts(enabled);
                    return i;
                }

                OSRestoreInterrupts(enabled);
                return -1;
            }
        }

        for (S32 i = 0; i < 6; i++)
        {
            if (gSnd.voice[i].lock_owner != 0)
            {
                continue;
            }
            if (streams[i].vinf.voice != NULL)
            {
                continue;
            }

            BOOL enabled = OSDisableInterrupts();
            _AXVPB* voice = AXAcquireVoice(priority >> 3, dv_callback, i);
            if (voice != NULL)
            {
                AXSetVoiceState(voice, 0);
                MIXInitChannel(voice, 0xc, 0, -0x384, -0x384, 0x40, 0x7f, -0x384);
                streams[i].vinf.voice = voice;
                streams[i].vinf.flags = 0;
                streams[i].vinf.flags |= 0x4;
                OSRestoreInterrupts(enabled);
                return i;
            }

            OSRestoreInterrupts(enabled);
            return -1;
        }
    }
    else
    {
        for (S32 i = 0; i < 58; i++)
        {
            if (voices[i].voice == NULL)
            {
                voices[i].flags = 0;
                voices[i].flags |= 0x1;
                return i + 6;
            }
        }
    }

    return -1;
}

S32 iSndPrepStream(xSndVoiceInfo* vp)
{
    S32 i = vp - gSnd.voice;
    UNK_STREAM* s = &streams[i];

    if (snd.hdr.assetID != vp->assetID)
    {
        iSndLookup(vp->assetID);
        if (snd.hdr.assetID != vp->assetID)
        {
            return 0x40;
        }
    }

    memcpy(&s->hdr, &snd, sizeof(sndhdr));
    memcpy(&s->fileInfo, &snd.file.ps.fileInfo, sizeof(DVDFileInfo));
    s->fileInfo.cb.userData = s;
    s->x8c = (((s->hdr.num_nibbles + 1) >> 1) + 0x1f) & ~0x1f;
    s->vinf.flags |= 0x1;
    s->vinf.aid = snd.hdr.assetID;

    if (s->hdr.loop_flag != 0 || (vp->flags & 0x8000))
    {
        s->vinf.flags |= 0x10000;
    }

    return i;
}

S32 iSndPlayMemStream(xSndVoiceInfo* vp)
{
    S32 i = vp - gSnd.voice;
    UNK_STREAM* s = &streams[i];

    if (s->vinf.voice != NULL && (s->vinf.flags & 0x1))
    {
        BOOL enabled = OSDisableInterrupts();

        s->vinf.flags |= 0x400000;
    s->vinf.flags |= 0x1000000;
    s->vinf.flags |= 0x2000000;
    s->x90 = 0;

    AXPBADDR addr;
    memcpy(&addr, &s->hdr.loop_flag, sizeof(AXPBADDR));

    U32 start = s->dest_a * 2 + 2;
    U32 end = s->dest_a * 2 + 0xffff;

    addr.loopFlag = 1;
    addr.format = 0;
    addr.loopAddressHi = start >> 16;
    addr.loopAddressLo = start;
    addr.currentAddressHi = start >> 16;
    addr.currentAddressLo = start;
    addr.endAddressHi = end >> 16;
    addr.endAddressLo = end;

    AXSetVoiceAddr(s->vinf.voice, &addr);
    AXSetVoiceAdpcm(s->vinf.voice, (AXPBADPCM*)&s->hdr.coef);
    AXSetVoiceSrcType(s->vinf.voice, 1);
    AXSetVoiceType(s->vinf.voice, 1);

    F32 ratio = std::powf(2.0f, vp->pitch / 12.0f);
    F32 srcRatio = (s->hdr.sample_rate * ratio) / 32000.0f;

    s->vinf.voice->pb.src.ratioHi = (S32)srcRatio;
    s->vinf.voice->pb.src.ratioLo = (S32)(65536.0f * srcRatio);
    s->vinf.voice->sync |= 0x80000000;

    streams[i].vinf.x14 = 0x7fffffff;
    streams[i].vinf.x1c = 0x7fffffff;

    iSndVolUpdate(vp, &streams[i].vinf);
        OSRestoreInterrupts(enabled);

        return vp->sndID;
    }

    return 0;
}

S32 iSndPlayStream(xSndVoiceInfo* vp)
{
    S32 i = vp - gSnd.voice;
    UNK_STREAM* s = &streams[i];

    if (s->hdr.loop_start + s->x8c >= s->fileInfo.length)
    {
        xSTAssetName(vp->assetID);
        xST_xAssetID_HIPFullPath(vp->assetID);
        return 0;
    }

    if (s->vinf.voice != NULL && (s->vinf.flags & 0x1))
    {
        DVDFileInfo* fi = &s->fileInfo;

        s->x84 = s->hdr.loop_start;
        s->offset = s->hdr.loop_start;
        s->x90 = 0;

        BOOL enabled = OSDisableInterrupts();

        s->vinf.flags |= 0x400000;

        U32 len = 0x4000;
        if (s->x8c <= 0x4000)
        {
            len = s->x8c;
        }

        DVDReadAsyncPrio(fi, (void*)s->source_a, len, s->offset, dvdcb, 2);

        AXPBADDR addr;
        memcpy(&addr, &s->hdr.loop_flag, sizeof(AXPBADDR));

        U32 start = s->dest_a * 2 + 2;
        U32 end = s->dest_a * 2 + 0xffff;

        addr.loopFlag = 1;
        addr.format = 0;
        addr.loopAddressHi = start >> 16;
        addr.loopAddressLo = start;
        addr.currentAddressHi = start >> 16;
        addr.currentAddressLo = start;
        addr.endAddressHi = end >> 16;
        addr.endAddressLo = end;

        AXSetVoiceAddr(s->vinf.voice, &addr);
        AXSetVoiceAdpcm(s->vinf.voice, (AXPBADPCM*)&s->hdr.coef);
        AXSetVoiceSrcType(s->vinf.voice, 1);
        AXSetVoiceType(s->vinf.voice, 1);

        F32 ratio = std::powf(2.0f, vp->pitch / 12.0f);
        F32 srcRatio = (s->hdr.sample_rate * ratio) / 32000.0f;

        s->vinf.voice->pb.src.ratioHi = (S32)srcRatio;
        s->vinf.voice->pb.src.ratioLo = (S32)(65536.0f * srcRatio);
        s->vinf.voice->sync |= 0x80000000;

        streams[i].vinf.x14 = 0x7fffffff;
        streams[i].vinf.x1c = 0x7fffffff;

        iSndVolUpdate(vp, &streams[i].vinf);
        OSRestoreInterrupts(enabled);

        return vp->sndID;
    }

    return 0;
}

S32 iSndPlaySound(xSndVoiceInfo* vp)
{
    if (snd.hdr.assetID != vp->assetID)
    {
        iSndLookup(vp->assetID);
        if (snd.hdr.assetID != vp->assetID)
        {
            return 0;
        }
    }

    if (sound_stream != 0)
    {
        sound_stream = 0;
    }

    U32 prio = vp->priority;
    S32 i = vp - gSnd.voice;

    if (prio >= 0xff)
    {
        prio = 0xff;
    }

    S32 j = i - 6;
    vinfo* v = &voices[j];
    prio >>= 3;

    if (v->voice == NULL)
    {
        v->aid = vp->assetID;
        v->xc = 1000.0f * snd.hdr.num_samples;
        v->xc = v->xc / snd.hdr.sample_rate;

        AXPBADDR addr;
        memcpy(&addr, &snd.hdr.loop_flag, sizeof(AXPBADDR));

        if (snd.hdr.loop_flag == 0)
        {
            u32 zp = zero_point;
            addr.loopAddressHi = zp >> 16;
            addr.loopAddressLo = zp;
        }

        if (prio == 0)
        {
            prio = 1;
        }

        v->voice = AXAcquireVoice(prio, dv_callback, i);
        if (v->voice == NULL)
        {
            return 0;
        }

        BOOL enabled = OSDisableInterrupts();

        AXSetVoiceAddr(v->voice, &addr);
        AXSetVoiceType(v->voice, 0);
        AXSetVoiceAdpcm(v->voice, (AXPBADPCM*)&snd.hdr.coef);
        AXSetVoiceSrcType(v->voice, 1);
        MIXInitChannel(v->voice, 4, 0, -0x388, -0x388, 0x40, 0x7f, -0x388);
        iSndVolUpdate(vp, v);

        F32 ratio = std::powf(2.0f, vp->pitch / 12.0f);
        F32 srcRatio = (snd.hdr.sample_rate * ratio) / 32000.0f;

        v->voice->pb.src.ratioHi = (S32)srcRatio;
        v->voice->pb.src.ratioLo = (S32)(65536.0 * srcRatio);

        if (snd.hdr.loop_flag != 0)
        {
            AXSetVoiceAdpcmLoop(v->voice, (AXPBADPCMLOOP*)&snd.hdr.loop_pred_scale);
        }

        v->voice->sync |= 0x80000000;
        MIXUnMute(v->voice);
        MIXUpdateSettings();
        AXSetVoiceState(v->voice, 1);
        OSRestoreInterrupts(enabled);

        voices[j].flags &= ~0x1;
        voices[j].flags |= 0x4;

        return vp->sndID;
    }

    return 0;
}

S32 iSndPlay(xSndVoiceInfo* vp)
{
    S32 offset = (S32)vp - (S32)gSnd.voice;
    S32 div = offset / 100;

    xSTAssetName(vp->assetID);

    if ((div < 0) || (div >= 64))
    {
        return 0;
    }
    else if (div < 6)
    {
        U32 ret = iSndPrepStream(vp);
        if (ret < 0x3a)
        {
            if (vp->flags & 0x200)
            {
                return iSndPlayMemStream(vp);
            }
            else
            {
                return iSndPlayStream(vp);
            }
        }
        return ret;
    }
    else
    {
        return iSndPlaySound(vp);
    }
}

void iSndSetVol(U32 snd, F32 vol)
{
    xSndVoiceInfo* vp = &gSnd.voice[0];

    S32 i = 0;
    for (; i < 64; i++, vp++)
    {
        if (vp->sndID == snd)
            break;
    }

    if (i != 64)
    {
        vp->vol = vol;
        S32 adj = iVolFromX(vol * gSnd.categoryVolFader[vp->category]);
        vinfo* info;
        if (i < 6)
        {
            info = &streams[i].vinf;
        }
        else
        {
            info = &voices[i - 6];
        }
        if (info->voice != 0)
        {
            info->x10 = adj;
            info->x14 = adj;
            info->x18 = 0x40;
            info->x1c = 0x40;
            MIXAdjustFader(info->voice, adj - MIXGetFader(info->voice));
            MIXAdjustPan(info->voice, 0x40 - MIXGetPan(info->voice));
        }
    }
}

void iSndSetPitch(U32 snd, F32 pitch)
{
    xSndVoiceInfo* vp = &gSnd.voice[0];

    S32 i = 0;
    for (; i < 64; i++, vp++)
    {
        if (vp->sndID == snd)
            break;
    }

    if (i == 64)
    {
        return;
    }

    vinfo* info;
    if (i < 6)
    {
        info = &streams[i].vinf;
    }
    else
    {
        info = &voices[i - 6];
    }

    if (info->voice == NULL)
    {
        return;
    }

    F32 ratio = std::powf(2.0f, pitch / 12.0f);
    AXSetVoiceSrcRatio(info->voice, (3.125e-05f * vp->sample_rate) * ratio);
}

void iSndStartStereo(U32 id1, U32 id2, F32 pitch)
{
}

void iSndStereo(U32 i)
{
    if (i == 0)
    {
        OSSetSoundMode(0);
        gSnd.stereo = 0;
    }
    else
    {
        OSSetSoundMode(1);
        gSnd.stereo = 1;
    }
}

void iSndWaitForDeadSounds()
{
    fc = 0;
    for (int i = 0x8c; fc < i;)
    {
        // `i` is weird, it's stored in a saved register but never mutated. However it needs to be mutated to put it in a saved register
        i = fc;
        while (fc < i + 0xe)
            ;
        // This adds the nonmatching instruction, but get's us back to the state `i`'s register should be in.
        i = 0x8c;
        iSndUpdate();
    }
}

void iSndSuspendCD(U32)
{
}

void iSndSceneExit()
{
    BOOL enabled = OSDisableInterrupts();
    S32 done = 0;

    ARQFlushQueue();
    DVDCancelAll();

    for (S32 i = 0; i < 64; i++)
    {
        if (gSnd.voice[i].sndID != 0)
        {
            iSndStop(gSnd.voice[i].sndID);
        }
    }

    for (S32 i = 0; i < 64; i++)
    {
        if (gSnd.voice[i].sndID != 0)
        {
            iSndStop(gSnd.voice[i].sndID);
        }
    }

    OSRestoreInterrupts(enabled);

    fc = 0;
    S32 limit = 0x190;
    while (!done && fc < limit)
    {
        done = 1;
        S32 next = fc;
        while (fc < next + 0xe)
            ;
        iSndUpdate();

        for (S32 i = 0; i < 6; i++)
        {
            if (streams[i].vinf.voice != NULL)
            {
                done = 0;
            }
        }

        for (S32 i = 0; i < 58; i++)
        {
            if (voices[i].voice != NULL)
            {
                done = 0;
            }
        }
    }

    sinfo_array_max--;
    if (sinfo_array[sinfo_array_max].info != NULL)
    {
        u32 len;
        ARFree(&len);
        RwFree(sinfo_array[sinfo_array_max].info);
    }
}

void iSndMessWithEA(sDSPADPCM* param1)
{
    if (param1 != NULL)
    {
        param1->buffer[5] = SampleToNybbleAddress(param1->buffer[0] - 1);
    }
}

U32 SampleToNybbleAddress(U32 sample)
{
    U32 a = __mulhwu(0x24924925, sample);
    U32 b = (sample - a) >> 1;

    a = b + a;
    b = (a >> 3);
    a = (a << 1) & 0xfffffff0;
    a = a + (sample - (b * 0xe)) + 2;

    return a;
}

void sndloadcb(tag_xFile* tag)
{
    SoundFlags = 0;
}
S32 iSndLoadSounds(void* data)
{
    sndinfo* p = (sndinfo*)data;
    U32 i;
    U32 max = 0;
    U32 min = -1;

    if (p->num_sfx == 0 && p->num_streams == 0 && p->num_cutscene == 0)
    {
        sinfo_array[sinfo_array_max++].info = NULL;
        return 0;
    }

    if (sinfo_array_max >= 12)
    {
        exit(-1);
    }

    sndhdr* entries = p->entry;
    sinfo_array[sinfo_array_max].info = p;

    char* path = xST_xAssetID_HIPFullPath(entries[0].assetID);
    U32 hash = xStrHash(path);
    sinfo* si = &sinfo_array[sinfo_array_max];

    iFileOpen(path, 0, &si->file);
    iFileSeek(&si->file, 0, 0);

    sndhdr* e = entries;
    for (i = 0; i < p->num_sfx; i++)
    {
        st_PKR_ASSET_TOCINFO xinfo;

        xSTGetAssetInfoInHxP(e->assetID, &xinfo, hash);

        U32 off = xinfo.plus_offset + ((xinfo.sector - si->file.ps.fileInfo.startAddr) << 5);
        U32 end = off + xinfo.size;
        if (xinfo.size == 0)
        {
            xSTAssetName(xinfo.aid);
        }
        if (min > off)
        {
            min = off;
        }
        if (max < end)
        {
            max = end;
        }

        iSndMessWithEA((sDSPADPCM*)e);
        e->loop_start += off * 2;
        e->loop_end += off * 2;
        e->cur_addr += off * 2;
        e++;
    }

    U32 memstream = 0;
    for (i = 0; i < p->num_streams; i++)
    {
        st_PKR_ASSET_TOCINFO xinfo;

        xSTGetAssetInfo(entries[i + p->num_sfx].assetID, &xinfo);
        xinfo.size = (xinfo.size + 0x1f) & ~0x1f;
        xSTAssetName(entries[i + p->num_sfx].assetID);

        char* tail = path + strlen(path) - 8;
        if (xStricmp(tail, "mnu5.hip") == 0 || xStricmp(tail, "spsb.hip") == 0)
        {
            memstream = 1;
        }

        if (memstream)
        {
            entries[i + p->num_sfx].loop_start =
                xinfo.plus_offset + ((xinfo.sector - si->file.ps.fileInfo.startAddr) << 5);
        }
        else
        {
            U32 off = xinfo.plus_offset + ((xinfo.sector - si->file.ps.fileInfo.startAddr) << 5);
            U32 end = off + xinfo.size;
            if (min > off)
            {
                min = off;
            }
            if (max < end)
            {
                max = end;
            }

            iSndMessWithEA((sDSPADPCM*)&entries[i + p->num_sfx]);
            entries[i + p->num_sfx].loop_start += off * 2;
            entries[i + p->num_sfx].loop_end += off * 2;
            entries[i + p->num_sfx].cur_addr += off * 2;
        }
    }

    if (p->num_sfx != 0 || (!memstream && p->num_streams != 0))
    {
        U32 total = max - min;
        p->total_size = total;

        u32 aram = ARAlloc(total);
        S32 delta = (-min + aram) * 2;

        sndhdr* e2 = entries;
        for (i = 0; i < p->num_sfx; i++)
        {
            e2->loop_start += delta;
            e2->loop_end += delta;
            e2->cur_addr += delta;
            xSTAssetName(e2->assetID);
            e2++;
        }

        if (!memstream)
        {
            for (i = 0; i < p->num_streams; i++)
            {
                entries[i + p->num_sfx].loop_start += delta;
                entries[i + p->num_sfx].loop_end += delta;
                entries[i + p->num_sfx].cur_addr += delta;
                entries[i + p->num_sfx].pad[0] = 0x63;
                xSTAssetName(entries[i + p->num_sfx].assetID);
            }
        }

        sinfo_array[sinfo_array_max].base = min;

        U32 pos = 0;
        while (pos < total)
        {
            U32 len = 0x18000;
            if (pos + 0x18000 > total)
            {
                len = total - pos;
            }

            u32 dest = aram + pos;

            SoundFlags |= 0x1;
            iFileSeek(&si->file, min + pos, 0);
            iFileReadAsync(&si->file, (void*)stream_buffer, len, sndloadcb, 0);

            while (SoundFlags != 0)
            {
                iTRCDisk::CheckDVDAndResetState();
                if ((fc & 0xf) == 0)
                {
                    zGameScreenTransitionUpdate(0.5f, NULL, NULL);
                }
            }

            BOOL enabled = OSDisableInterrupts();
            DCStoreRange((void*)stream_buffer, len);
            ARQRequest request;
            ARQPostRequest(&request, 0, 0, 1, (u32)stream_buffer, dest, len, arq_callback);
            SoundFlags |= 0x1;
            OSRestoreInterrupts(enabled);

            while (SoundFlags != 0)
                ;

            pos += len;
        }

        while (SoundFlags != 0)
            ;
    }

    sinfo_array_max++;
    return 1;
}

void iSndDIEDIEDIE()
{
    if (!soundInited)
    {
        return;
    }

    OSDisableInterrupts();
    soundInited = 0;

    for (S32 i = 0; i < (S32)(sizeof(streams) / (sizeof(UNK_STREAM))); i++)
    {
        UNK_STREAM* pv = &streams[i];

        if (pv->vinf.voice != NULL)
        {
            MIXReleaseChannel(pv->vinf.voice);
            AXSetVoiceState(pv->vinf.voice, 0);
            iSndMyAXFree(&pv->vinf.voice);
        }
        pv++;
    }

    for (S32 i = 0; i < (S32)(sizeof(voices) / (sizeof(vinfo))); i++)
    {
        vinfo* v = &voices[i];

        if ((v->voice != NULL))
        {
            MIXReleaseChannel(v->voice);
            AXSetVoiceState(v->voice, 0);
            iSndMyAXFree(&v->voice);
        }
    }

    AXQuit();
}

void iSndSetExternalCallback(iSndExternalCallback callback)
{
}

void iSndMyAXFree(_AXVPB** param1)
{
    if (param1 != NULL && *param1 != NULL)
    {
        AXFreeVoice(*param1);
        *param1 = NULL;
    }
}

void iSndSuspend()
{
    AXRegisterCallback(0);

    for (S32 i = 0; i < (S32)(sizeof(streams) / (sizeof(UNK_STREAM))); i++)
    {
        UNK_STREAM* pv = &streams[i];

        if (pv->vinf.voice != NULL)
        {
            AXSetVoiceState(pv->vinf.voice, 0);
        }
        pv++;
    }

    for (S32 i = 0; i < (S32)(sizeof(voices) / (sizeof(vinfo))); i++)
    {
        vinfo* v = &voices[i];

        if ((v->voice != NULL))
        {
            AXSetVoiceState(v->voice, 0);
        }
    }
}

void iSndResume()
{
    for (S32 i = 0; i < (S32)(sizeof(streams) / (sizeof(UNK_STREAM))); i++)
    {
        UNK_STREAM* pv = &streams[i];

        if ((pv->vinf.voice != NULL) && !(pv->vinf.flags & 2))
        {
            AXSetVoiceState(pv->vinf.voice, 1);
        }
        pv++;
    }

    for (S32 i = 0; i < (S32)(sizeof(voices) / (sizeof(vinfo))); i++)
    {
        vinfo* v = &voices[i];

        if ((v->voice != NULL) && !(v->flags & 8))
        {
            AXSetVoiceState(v->voice, 1);
        }
    }

    AXRegisterCallback(fcb);
}

F32 iSndGetVol(U32 snd)
{
    xSndVoiceInfo* vp = &gSnd.voice[0];

    for (int i = 0; i < 0x40; i++)
    {
        if (vp->flags & 1)
        {
            if (vp->sndID == snd)
            {
                if (gSnd.categoryVolFader[vp->category] <= 0.0f)
                {
                    return 0.0f;
                }
                return (vp->vol / gSnd.categoryVolFader[vp->category]);
            }
        }
        vp++;
    }

    return 0.0f;
}