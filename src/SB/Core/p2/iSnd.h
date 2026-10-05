#ifndef PS2_ISND_H
#define PS2_ISND_H

#include <types.h>

struct xSndVoiceInfo;
struct tag_xFile;

struct iSndVol
{
    S16 volL;
    S16 volR;
};

struct iSndInfo
{
    U32 flags;
    iSndVol vol;
    U32 pitch;
    S32 lastStreamBuffer;
};

struct iSndFileInfo
{
    U32 ID; // offset 0x0
    U32 assetID; // offset 0x4
    U16 sample_rate; // offset 0x8
    U8 is_streamed; // offset 0xA
    union
    {
        struct
        {
            U32 address; // offset 0x0
            U32 size; // offset 0x4
        } nonstream; // offset 0xC
        struct
        {
            S32 file_index; // offset 0x0
            U32 lsn; // offset 0x4
            U32 data_size; // offset 0x8
            U16 stream_interleave_size; // offset 0xC
            U16 stream_interleave_count; // offset 0xE
        } stream; // offset 0xC
    };
};

enum isound_effect
{
    iSND_EFFECT_NONE,
    iSND_EFFECT_CAVE
};

typedef void (*iSndExternalCallback)(U32, bool);

void iSndInit();
void iSndExit();

void iSndSetEnvironmentalEffect(isound_effect);
void iSndInitSceneLoaded();

U8 iSndIsPlaying(U32 assetID);
U8 iSndIsPlaying(U32 assetID, U32 parid);
U8 iSndIsPlayingByHandle(U32 handle);
iSndFileInfo* iSndLookup(U32 id);

void iSndPause(U32 snd, U32 pause);
void iSndStop(U32 snd);
void iSndUpdate();

U32 iSndFindFreeVoice(U32 priority, U32 flags, U32 owner);

U32 iSndPlay(xSndVoiceInfo* vp);
void iSndSetVol(U32 snd, F32 vol);
void iSndSetPitch(U32 snd, F32 pitch);
void iSndStartStereo(U32 id1, U32 id2, F32 pitch);
void iSndStereo(U32 stereo);

void iSndWaitForDeadSounds();
void iSndSuspendCD(U32);


void iSndSetExternalCallback(iSndExternalCallback callback);


F32 iSndGetVol(U32 snd);

#endif
