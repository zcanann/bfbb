#include "bink.h"
#include "binkngc.h"
#include "ngcsnd.h"
#include <dolphin/ar.h>
#include <dolphin/ax.h>
#include <dolphin/os/OSCache.h>

f32 powf(f32 x, f32 y);

typedef enum NGCSoundChannelLayout
{
    NGC_SOUND_MONO_CHANNELS = 1,
    NGC_SOUND_STEREO_CHANNELS = 2,
    NGC_SOUND_LOCK_BUFFER_COUNT = 2,
    NGC_SOUND_ARQ_TASK_COUNT = NGC_SOUND_LOCK_BUFFER_COUNT * NGC_SOUND_STEREO_CHANNELS,
    NGC_SOUND_NO_LOCK_INDEX = -1,
    NGC_SOUND_LOCK_INDEX_MASK = NGC_SOUND_LOCK_BUFFER_COUNT - 1,
    NGC_SOUND_LAST_LOCK_INDEX = NGC_SOUND_LOCK_INDEX_MASK,
    NGC_SOUND_RIGHT_TASK_OFFSET = NGC_SOUND_STEREO_CHANNELS
} NGCSoundChannelLayout;

/* ARQRequest.owner keeps the NGCSoundState pointer with bit 0 as an in-flight latch. */
typedef enum NGCSoundTaskFlag
{
    NGC_TASK_BUSY_FLAG = 1,
    NGC_TASK_OWNER_MASK = ~NGC_TASK_BUSY_FLAG
} NGCSoundTaskFlag;

#define NGC_TASK_OWNER(task) ((NGCSoundState PTR4*)((task)->owner & NGC_TASK_OWNER_MASK))
#define NGC_TASK_OWNER_BUSY(owner) (((owner) & NGC_TASK_BUSY_FLAG) != 0)
#define NGC_TASK_BUSY(task) (((task)->owner & NGC_TASK_BUSY_FLAG) != 0)
#define NGC_TASK_MARK_BUSY(task) ((task)->owner |= NGC_TASK_BUSY_FLAG)
#define NGC_TASK_CLEAR_BUSY(task) ((task)->owner &= NGC_TASK_OWNER_MASK)
#define NGC_TASK_SET_OWNER(task, state) ((task)->owner = (u32)(state))

typedef enum NGCSoundTiming
{
    NGC_SOUND_BITS_16 = 16,
    NGC_SOUND_FRAME_ALIGN = ARQ_DMA_ALIGNMENT,
    NGC_SOUND_FRAME_ALIGN_MASK = NGC_SOUND_FRAME_ALIGN - 1,
    NGC_SOUND_BUFFER_ALIGN = 0x40,
    NGC_SOUND_BUFFER_ALIGN_MASK = NGC_SOUND_BUFFER_ALIGN - 1,
    NGC_SOUND_FRAMES_PER_SECOND = 25,
    NGC_SOUND_BUFFER_MILLISECONDS = 800,
    NGC_SOUND_MILLISECONDS_PER_SECOND = 1000,
    NGC_SOUND_PERCENT_SCALE = 100,
    NGC_SOUND_STARVATION_PERCENT = 90,
    NGC_DEFAULT_STARVATION_MILLISECONDS = 720,
    NGC_SOUND_MAX_BUSY_POLLS = 99999,
    NGC_SOUND_BITS_TO_BYTES_SHIFT = 3,
    NGC_SOUND_BEST_SIZE_SHIFT = 5
} NGCSoundTiming;

typedef enum BINKNGCSoundLimits
{
    BINK_NGC_VOLUME_MAX = 0x7fff,
    BINK_NGC_PAN_MAX = 0x10000
} BINKNGCSoundLimits;

typedef enum NGCAXVoiceConstants
{
    AX_VOICE_PRIORITY_BINK = 0x1f,
    AX_SAMPLE_RATE = 32000,
    AX_PB_STATE_STOP = 0,
    AX_PB_STATE_RUN = 1,
    AX_PB_FORMAT_PCM16 = 10,
    AX_PB_FORMAT_PCM8 = 25,
    AX_MIX_MODE_DEFAULT = 3,
    AX_ADDR_LOOP_ON = 1,
    AX_ADDR_HIGH_SHIFT = 16
} NGCAXVoiceConstants;

#define AX_SYNC_VOLUME_MIX (AX_SYNC_FLAG_COPYVOL | AX_SYNC_FLAG_COPYAXPBMIX | AX_SYNC_FLAG_COPYMXRCTRL)
typedef enum NGCAXSRCLastSamples
{
    AX_SRC_LAST_SAMPLE_0 = 0,
    AX_SRC_LAST_SAMPLE_1 = 1,
    AX_SRC_LAST_SAMPLE_2 = 2,
    AX_SRC_LAST_SAMPLE_3 = 3
} NGCAXSRCLastSamples;

typedef enum NGCSamplePacking
{
    NGC_SAMPLE_HALF_SHIFT = 16,
    NGC_SAMPLE_BYTE_SHIFT = 8,
    NGC_SAMPLE_HIGH_HALF_MASK = 0xffff0000,
    NGC_SAMPLE_LOW_HALF_MASK = 0xffff,
    NGC_SAMPLE_HIGH_BYTE_MASK = 0xff00,
    NGC_SAMPLE_LOW_BYTE_MASK = 0xff
} NGCSamplePacking;

#define NGC_SAMPLE_HIGH_HALF(value) ((value) >> NGC_SAMPLE_HALF_SHIFT)
#define NGC_SAMPLE_LOW_HALF(value) ((value) & NGC_SAMPLE_LOW_HALF_MASK)
#define NGC_SAMPLE_LEFT_8_PAIR(value) \
    ((((value) >> NGC_SAMPLE_HALF_SHIFT) & NGC_SAMPLE_HIGH_BYTE_MASK) | \
     (((value) >> NGC_SAMPLE_BYTE_SHIFT) & NGC_SAMPLE_LOW_BYTE_MASK))
#define NGC_SAMPLE_RIGHT_8_PAIR(value) \
    ((((value) >> NGC_SAMPLE_BYTE_SHIFT) & NGC_SAMPLE_HIGH_BYTE_MASK) | \
     ((value) & NGC_SAMPLE_LOW_BYTE_MASK))
#define NGC_SAMPLE_LEFT_16_PAIR(first, second) \
    (NGC_SAMPLE_HIGH_HALF(first) | ((second) & NGC_SAMPLE_HIGH_HALF_MASK))
#define NGC_SAMPLE_RIGHT_16_PAIR(first, second) \
    (NGC_SAMPLE_LOW_HALF(first) | ((second) << NGC_SAMPLE_HALF_SHIFT))
typedef enum NGCSoundCopyLayout
{
    NGC_SOUND_HALF_BUFFER_SHIFT = 1,
    NGC_STEREO16_GROUP_SHIFT = 3,
    NGC_STEREO16_TAIL_SHIFT = 2,
    NGC_STEREO8_GROUP_SHIFT = 2,
    NGC_STEREO8_TAIL_SHIFT = 1
} NGCSoundCopyLayout;

typedef struct NGCSoundState NGCSoundState;

typedef enum NGCPlayState
{
    NGC_PLAY_STATE_STOPPED,
    NGC_PLAY_STATE_STARVED,
    NGC_PLAY_STATE_RUNNING
} NGCPlayState;

typedef enum NGCSoundOnOff
{
    NGC_SOUND_OFF,
    NGC_SOUND_ON
} NGCSoundOnOff;

typedef enum NGCSoundPauseState
{
    NGC_SOUND_UNPAUSED,
    NGC_SOUND_PAUSED
} NGCSoundPauseState;

struct NGCSoundState
{
    s32 volume;
    f32 pan;
    NGCPlayState play_state;
    NGCSoundPauseState paused;
    s32 lock_index;
    u32 play_cursor; /* ARAM write cursor for the next decoded frame */
    u32 pending_end; /* delayed voice end address when the cursor wraps */
    u32 frame_size; /* bytes submitted per Bink audio frame */
    u32 channel_stride; /* bytes reserved per channel in ARAM */
    u32 starvation_threshold;
    u8 PTR4* decode_buffer; /* MRAM staging buffers for ARQ uploads */
    u8 PTR4* stereo_buffer; /* temporary split buffer for interleaved stereo */
    u32 last_ready_time;
    u32 starvation_time;
    ARQRequest tasks[NGC_SOUND_ARQ_TASK_COUNT]; /* two lock buffers by left/right ARQ uploads */
    AXVPB PTR4* left_voice;
    AXVPB PTR4* right_voice;
    u8 PTR4* audio_buffer; /* ARAM ring buffer base */
    u32 address_shift; /* AX addresses are samples for 16-bit, bytes for 8-bit */
};

typedef BINKSND NGCBinkSound;
typedef char NGCSoundStateFitsInBinkSndData
    [(sizeof(NGCSoundState) <= sizeof(((BINKSND PTR4*)0)->snddata)) ? 1 : -1];

#define NGC_STATE(ptr) ((NGCSoundState PTR4*)(ptr))
#define NGC_SND(snd) ((NGCBinkSound PTR4*)(snd))
#define NGC_SOUND_STATE(snd) ((NGCSoundState PTR4*)NGC_SND(snd)->snddata)
#define NGC_TASK(state, index) (&NGC_STATE(state)->tasks[(index)])
#define NGC_TASK_FOR_INDEX(state, index, side) NGC_TASK(state, (side) + ((index) << 1))
#define NGC_TASK_FOR_LOCK_CHANNEL(state, index, channel) NGC_TASK(state, ((index) + (channel)) + (channel))
#define NGC_RIGHT_LOCK_TASK_INDEX(index) ((index) + NGC_SOUND_RIGHT_TASK_OFFSET)
#define NGC_LEFT_LOCK_TASK(state, index) NGC_TASK(state, index)
#define NGC_RIGHT_LOCK_TASK_BASE(state) NGC_TASK(state, NGC_SOUND_RIGHT_TASK_OFFSET)
#define NGC_RIGHT_LOCK_TASK(state, index) NGC_TASK(state, NGC_RIGHT_LOCK_TASK_INDEX(index))
#define NGC_TASK_SOURCE(task) ((u8 PTR4*)((task)->source))
#define NGC_TASK_SOURCE_AT(task, offset) (NGC_TASK_SOURCE(task) + (offset))
#define NGC_LEFT_VOICE(ptr) (NGC_STATE(ptr)->left_voice)
#define NGC_RIGHT_VOICE(ptr) (NGC_STATE(ptr)->right_voice)
#define NGC_CHANNEL_STRIDE(ptr) (NGC_STATE(ptr)->channel_stride)
#define NGC_ADDRESS_SHIFT(ptr) (NGC_STATE(ptr)->address_shift)
#define NGC_ADVANCE_U32_BYTES(ptr, bytes) ((u32 PTR4*)((u8 PTR4*)(ptr) + (bytes)))
#define NGC_ADVANCE_U16_BYTES(ptr, bytes) ((u16 PTR4*)((u8 PTR4*)(ptr) + (bytes)))
#define NGC_ALIGN_UP(value, mask) (((value) + (mask)) & ~(mask))
#define NGC_SOUND_RATE_BYTES(sound) (((sound)->freq * (sound)->bits) >> NGC_SOUND_BITS_TO_BYTES_SHIFT)
#define NGC_SOUND_BEST_SIZE_MASK(chans) (-((chans) << NGC_SOUND_BEST_SIZE_SHIFT))
#define NGC_SOUND_PLAY_LIMIT(state) \
    (((u32)(state)->audio_buffer + (state)->channel_stride) - (state)->frame_size)
#define NGC_SOUND_RIGHT_CURSOR(state) ((state)->play_cursor + (state)->channel_stride)
#define NGC_AX_ADDR(addr, shift) ((addr) >> (shift))
#define NGC_AX_END_ADDR(addr, shift) (NGC_AX_ADDR(addr, shift) - 1)
#define NGC_AX_RIGHT_ADDR(state, addr) \
    (((addr) + NGC_CHANNEL_STRIDE(state)) >> NGC_ADDRESS_SHIFT(state))
#define NGC_AX_RIGHT_END_ADDR(state, addr) (NGC_AX_RIGHT_ADDR(state, addr) - 1)
#define NGC_AX_CURRENT_CURSOR(voice, shift) (AX_VOICE_CURRENT_ADDR(voice) << (shift))
#define NGC_AX_END_CURSOR(voice, shift) (AX_VOICE_END_ADDR(voice) << (shift))

#define AX_VOICE_END_ADDR(voice) (*(u32 PTR4*)&(voice)->pb.addr.endAddressHi)
#define AX_VOICE_CURRENT_ADDR(voice) (*(u32 PTR4*)&(voice)->pb.addr.currentAddressHi)
#define AX_VOICE_MIX(voice) (&(voice)->pb.mix)
#define AX_VOICE_VOLUME(voice) ((voice)->pb.ve.currentVolume)
#define AX_VOICE_VOLUME_DELTA(voice) ((voice)->pb.ve.currentDelta)
#define AX_VOICE_MIX_MODE(voice) ((voice)->pb.mixerCtrl)
#define AX_VOICE_SYNC_FLAGS(voice) ((voice)->sync)

#define NGC_SOUND_PAN_ONE BINK_NGC_PAN_ONE
#define NGC_SOUND_PAN_EXPONENT BINK_NGC_PAN_EXPONENT
#define NGC_SOUND_MIX_SCALE BINK_NGC_MIX_SCALE
#define NGC_SOUND_U32_TO_F64_BIAS BINK_NGC_U32_TO_F64_BIAS
#define NGC_SOUND_AX_SAMPLE_RATE BINK_NGC_AX_SAMPLE_RATE
#define NGC_SOUND_S32_TO_F64_BIAS BINK_NGC_S32_TO_F64_BIAS[0]
#define NGC_SOUND_PAN_TO_FLOAT BINK_NGC_PAN_TO_FLOAT[0]
#define NGC_SOUND_PAN_CENTER BINK_NGC_PAN_CENTER

static const f32 BINK_NGC_PAN_ONE = 1.0f;
static const f32 BINK_NGC_PAN_EXPONENT = 0.3f;
static const f32 BINK_NGC_MIX_SCALE = 65535.0f;
static const f64 BINK_NGC_U32_TO_F64_BIAS = 4503599627370496.0;
static const f32 BINK_NGC_AX_SAMPLE_RATE = 32000.0f;
static const f64 BINK_NGC_S32_TO_F64_BIAS[] = {
    4503601774854144.0,
};
static const f32 BINK_NGC_PAN_TO_FLOAT[] = {
    0.0000152587890625f,
};
static const f32 BINK_NGC_PAN_CENTER = 0.5f;

static void NGC_SoundPlay(BINKSND PTR4* snd, u32 index, u32 upload_bytes);
static void NGC_StarvedClear(BINKSND PTR4* snd);
static void NGC_SoundVolume(BINKSND PTR4* snd);

const char BINK_ERROR_OPENING_FILE[] = "Error opening file.";
const char BINK_ERROR_NOT_BINK[20] = "Not a Bink file.";
const char BINK_ERROR_NO_COMPRESSED_FRAMES[] = "The file doesn't contain any compressed frames yet.";
const char BINK_ERROR_OUT_OF_MEMORY[20] = "Out of memory.";

static void startVoices(u32 task)
{
    ARQRequest PTR4* arq_task = (ARQRequest PTR4*)task;
    NGCSoundState PTR4* state = NGC_TASK_OWNER(arq_task);

    if (state != 0 && state->play_state == NGC_PLAY_STATE_STOPPED) {
        AXVPB PTR4* voice = NGC_LEFT_VOICE(state);

        if (voice != 0) {
            AXSetVoiceState(voice, AX_PB_STATE_RUN);
        }

        voice = NGC_RIGHT_VOICE(state);
        if (voice != 0) {
            AXSetVoiceState(voice, AX_PB_STATE_RUN);
        }

        state->play_state = NGC_PLAY_STATE_RUNNING;
    }

    NGC_TASK_CLEAR_BUSY(arq_task); /* clear the in-flight latch after the left upload */
}
static void NGC_SoundPlay(BINKSND PTR4* snd, u32 index, u32 upload_bytes)
{
    NGCSoundState PTR4* state;
    u32 play_end;
    ARQRequest PTR4* task;
    ARQRequest PTR4* right_task;

    state = NGC_SOUND_STATE(snd);
    play_end = NGC_SOUND_STATE(snd)->play_cursor + upload_bytes;
    task = NGC_LEFT_LOCK_TASK(state, index);

    if (NGC_SOUND_STATE(snd)->right_voice != 0) {
        right_task = NGC_RIGHT_LOCK_TASK_BASE(state);
        right_task += index;

        DCFlushRange((void PTR4*)right_task->source, upload_bytes);
        ARQPostRequest(right_task, 0, ARQ_TYPE_MRAM_TO_ARAM, ARQ_PRIORITY_HIGH,
                       (u32)right_task->source, NGC_SOUND_RIGHT_CURSOR(NGC_SOUND_STATE(snd)),
                       upload_bytes, 0);
    }

    DCFlushRange((void PTR4*)task->source, upload_bytes);
    ARQPostRequest(task, task->owner, ARQ_TYPE_MRAM_TO_ARAM, ARQ_PRIORITY_HIGH, (u32)task->source,
                   NGC_SOUND_STATE(snd)->play_cursor, upload_bytes, startVoices);

    if (play_end > NGC_SOUND_PLAY_LIMIT(NGC_SOUND_STATE(snd))) {
        AXVPB PTR4* voice = NGC_LEFT_VOICE(state);
        u32 shift = NGC_ADDRESS_SHIFT(state);

        if (NGC_AX_CURRENT_CURSOR(voice, shift) > play_end) {
            NGC_SOUND_STATE(snd)->pending_end = play_end;
        } else {
            AXSetVoiceEndAddr(voice, NGC_AX_END_ADDR(play_end, shift));

            voice = NGC_RIGHT_VOICE(state);
            if (voice != 0) {
                AXSetVoiceEndAddr(voice, NGC_AX_RIGHT_END_ADDR(state, play_end));
            }

            play_end = (u32)NGC_SOUND_STATE(snd)->audio_buffer;
        }
    }

    NGC_SOUND_STATE(snd)->play_cursor = play_end;
    NGC_SOUND_STATE(snd)->last_ready_time = RADTimerRead();
}

static s32 NGC_SoundReinit(BINKSND PTR4* snd)
{
    NGCSoundState PTR4* state;
    AXVPB PTR4* voice;
    u32 ring_start;
    u32 ax_addr;
    u32 ring_end;
    u32 owner;
    u32 i;

    state = NGC_SOUND_STATE(snd);
    voice = NGC_LEFT_VOICE(state);
    if (voice != 0) {
        AXSetVoiceState(voice, AX_PB_STATE_STOP);
    }

    voice = NGC_RIGHT_VOICE(state);
    if (voice != 0) {
        AXSetVoiceState(voice, AX_PB_STATE_STOP);
    }

    ring_start = (u32)NGC_SOUND_STATE(snd)->audio_buffer;
    NGC_SOUND_STATE(snd)->lock_index = NGC_SOUND_NO_LOCK_INDEX;
    NGC_SOUND_STATE(snd)->play_state = NGC_PLAY_STATE_STOPPED;
    NGC_SOUND_STATE(snd)->play_cursor = ring_start;
    voice = NGC_LEFT_VOICE(state);
    ax_addr = NGC_AX_ADDR(ring_start, NGC_SOUND_STATE(snd)->address_shift);

    AXSetVoiceCurrentAddr(voice, ax_addr);
    voice = NGC_RIGHT_VOICE(state);
    if (voice != 0) {
        AXSetVoiceCurrentAddr(voice, NGC_AX_RIGHT_ADDR(state, ring_start));
    }

    ring_start = NGC_SOUND_STATE(snd)->play_cursor;
    AXSetVoiceLoopAddr(NGC_LEFT_VOICE(state), NGC_AX_ADDR(ring_start, NGC_SOUND_STATE(snd)->address_shift));
    voice = NGC_RIGHT_VOICE(state);
    if (voice != 0) {
        AXSetVoiceLoopAddr(voice, NGC_AX_RIGHT_ADDR(state, ring_start));
    }

    ring_end = NGC_SOUND_STATE(snd)->play_cursor + NGC_SOUND_STATE(snd)->channel_stride;
    AXSetVoiceEndAddr(NGC_LEFT_VOICE(state), NGC_AX_END_ADDR(ring_end, NGC_SOUND_STATE(snd)->address_shift));
    voice = NGC_RIGHT_VOICE(state);
    if (voice != 0) {
        AXSetVoiceEndAddr(voice, NGC_AX_RIGHT_END_ADDR(state, ring_end));
    }

    state->pending_end = 0;
    owner = (u32)state;
    for (i = 0; i < NGC_SOUND_ARQ_TASK_COUNT; ++i) {
        if ((state->tasks[i].owner & NGC_TASK_BUSY_FLAG) != 0) {
            ARQRemoveRequest(&state->tasks[i]);
        }

        state->tasks[i].owner = owner; /* reset owner and clear the busy latch */
    }

    return 1;
}

static void SetStreamVolumePan(AXVPB PTR4* voice, u32 volume, u32 left, u32 right)
{
    AXPBMIX PTR4* mix = AX_VOICE_MIX(voice);

    memset(mix, 0, sizeof(*mix));
    mix->vL = left;
    mix->vR = right;
    AX_VOICE_VOLUME(voice) = volume;
    AX_VOICE_VOLUME_DELTA(voice) = 0;
    AX_VOICE_MIX_MODE(voice) = AX_MIX_MODE_DEFAULT;
    AX_VOICE_SYNC_FLAGS(voice) |= AX_SYNC_VOLUME_MIX;
}

static void NGC_SoundVolume(BINKSND PTR4* snd)
{
    s32 left = (s32)(powf(NGC_SOUND_PAN_ONE - NGC_SOUND_STATE(snd)->pan, NGC_SOUND_PAN_EXPONENT) * NGC_SOUND_MIX_SCALE);
    s32 right = (s32)(powf(NGC_SOUND_STATE(snd)->pan, NGC_SOUND_PAN_EXPONENT) * NGC_SOUND_MIX_SCALE);
    AXVPB PTR4* voice = NGC_SOUND_STATE(snd)->left_voice;

    if (voice != 0) {
        SetStreamVolumePan(voice, NGC_SOUND_STATE(snd)->volume, left,
                           NGC_SND(snd)->chans == NGC_SOUND_MONO_CHANNELS ? right : 0);
    }

    voice = NGC_SOUND_STATE(snd)->right_voice;
    if (voice != 0) {
        SetStreamVolumePan(voice, NGC_SOUND_STATE(snd)->volume, 0, right);
    }
}

static s32 NGC_SoundInit(BINKSND PTR4* snd)
{
    u32 bytes_per_second;
    u32 frame_size;
    u32 channel_stride;
    u32 i;
    u32 start;
    u32 end;
    AXPBADDR addr;
    AXPBSRC src;
    AXVPB PTR4** voices;
    NGCSoundState PTR4* state;

    NGC_SOUND_STATE(snd)->starvation_time = NGC_DEFAULT_STARVATION_MILLISECONDS;
    NGC_SOUND_STATE(snd)->lock_index = NGC_SOUND_NO_LOCK_INDEX;
    NGC_SOUND_STATE(snd)->play_state = NGC_PLAY_STATE_STOPPED;

    bytes_per_second = NGC_SOUND_RATE_BYTES(NGC_SND(snd));
    frame_size = NGC_ALIGN_UP(bytes_per_second / NGC_SOUND_FRAMES_PER_SECOND, NGC_SOUND_FRAME_ALIGN_MASK);
    channel_stride = NGC_ALIGN_UP((bytes_per_second * NGC_SOUND_BUFFER_MILLISECONDS) / NGC_SOUND_MILLISECONDS_PER_SECOND,
                                  NGC_SOUND_BUFFER_ALIGN_MASK);

    NGC_SOUND_STATE(snd)->frame_size = frame_size;
    NGC_SOUND_STATE(snd)->channel_stride = channel_stride;
    NGC_SND(snd)->BestSizeIn16 = frame_size * NGC_SND(snd)->chans;
    NGC_SOUND_STATE(snd)->starvation_threshold =
        channel_stride - (channel_stride * NGC_SOUND_STARVATION_PERCENT) / NGC_SOUND_PERCENT_SCALE;
    if (NGC_SND(snd)->bits != NGC_SOUND_BITS_16) {
        NGC_SND(snd)->BestSizeIn16 = frame_size * NGC_SND(snd)->chans * sizeof(s16);
    }

    NGC_SND(snd)->BestSizeMask = NGC_SOUND_BEST_SIZE_MASK(NGC_SND(snd)->chans);
    NGC_SOUND_STATE(snd)->decode_buffer = (u8 PTR4*)radmalloc(NGC_SOUND_STATE(snd)->channel_stride * NGC_SND(snd)->chans);
    if (NGC_SOUND_STATE(snd)->decode_buffer == 0) {
        return 0;
    }

    if (NGC_SND(snd)->chans == NGC_SOUND_STEREO_CHANNELS) {
        NGC_SOUND_STATE(snd)->stereo_buffer = (u8 PTR4*)radmalloc(NGC_SOUND_STATE(snd)->channel_stride);
        if (NGC_SOUND_STATE(snd)->stereo_buffer == 0) {
            return 0;
        }
    }

    NGC_SOUND_STATE(snd)->audio_buffer = (u8 PTR4*)radaudiomalloc(NGC_SOUND_STATE(snd)->channel_stride * NGC_SND(snd)->chans);
    if (NGC_SOUND_STATE(snd)->audio_buffer == 0) {
        return 0;
    }

    state = NGC_SOUND_STATE(snd);
    NGC_SOUND_STATE(snd)->address_shift = (NGC_SND(snd)->bits == NGC_SOUND_BITS_16);

    for (i = 0; i < NGC_SOUND_ARQ_TASK_COUNT; ++i) {
        NGC_TASK_SET_OWNER(state->tasks + i, state); /* seed owner and clear the busy latch */
    }

    NGC_SOUND_STATE(snd)->play_cursor = (u32)NGC_SOUND_STATE(snd)->audio_buffer;
    NGC_SOUND_STATE(snd)->pending_end = 0;

    for (i = 0; i < NGC_SND(snd)->chans; ++i) {
        voices = &NGC_SOUND_STATE(snd)->left_voice;
        voices[i] = AXAcquireVoice(AX_VOICE_PRIORITY_BINK, 0, 0);
        if (voices[i] == 0) {
            return 0;
        }

        start = ((u32)NGC_SOUND_STATE(snd)->audio_buffer + (i * NGC_SOUND_STATE(snd)->channel_stride)) >>
                NGC_SOUND_STATE(snd)->address_shift;
        end = (((u32)NGC_SOUND_STATE(snd)->audio_buffer + ((i + 1) * NGC_SOUND_STATE(snd)->channel_stride)) >>
               NGC_SOUND_STATE(snd)->address_shift) -
              1;

        addr.loopFlag = AX_ADDR_LOOP_ON;
        addr.format = (NGC_SND(snd)->bits == NGC_SOUND_BITS_16) ? AX_PB_FORMAT_PCM16 : AX_PB_FORMAT_PCM8;
        addr.loopAddressHi = start >> AX_ADDR_HIGH_SHIFT;
        addr.loopAddressLo = start;
        addr.endAddressHi = end >> AX_ADDR_HIGH_SHIFT;
        addr.endAddressLo = end;
        addr.currentAddressHi = addr.loopAddressHi;
        addr.currentAddressLo = addr.loopAddressLo;
        AXSetVoiceAddr(voices[i], &addr);

        if (NGC_SND(snd)->freq == AX_SAMPLE_RATE) {
            AXSetVoiceSrcType(voices[i], AX_SRC_TYPE_NONE);
        } else {
            AXSetVoiceSrcType(voices[i], AX_SRC_TYPE_LINEAR);
            src.ratioHi = 1;
            src.ratioLo = 0;
            src.currentAddressFrac = 0;
            src.last_samples[AX_SRC_LAST_SAMPLE_0] = 0;
            src.last_samples[AX_SRC_LAST_SAMPLE_1] = 0;
            src.last_samples[AX_SRC_LAST_SAMPLE_2] = 0;
            src.last_samples[AX_SRC_LAST_SAMPLE_3] = 0;
            AXSetVoiceSrc(voices[i], &src);
            AXSetVoiceSrcRatio(voices[i], (f32)NGC_SND(snd)->freq / NGC_SOUND_AX_SAMPLE_RATE);
        }
    }

    NGC_SoundVolume(snd);
    return 1;
}

static void NGC_SoundPause(BINKSND PTR4* snd)
{
    NGCSoundState PTR4* state = NGC_SOUND_STATE(snd);
    AXVPB PTR4* voice = NGC_LEFT_VOICE(state);

    if (voice != 0) {
        AXSetVoiceState(voice, AX_PB_STATE_STOP);
    }

    voice = NGC_RIGHT_VOICE(state);
    if (voice != 0) {
        AXSetVoiceState(voice, AX_PB_STATE_STOP);
    }
}

static void NGC_SoundResume(BINKSND PTR4* snd)
{
    NGCSoundState PTR4* state = NGC_SOUND_STATE(snd);
    AXVPB PTR4* voice = NGC_LEFT_VOICE(state);

    if (voice != 0) {
        AXSetVoiceState(voice, AX_PB_STATE_RUN);
    }

    voice = NGC_RIGHT_VOICE(state);
    if (voice != 0) {
        AXSetVoiceState(voice, AX_PB_STATE_RUN);
    }
}

static void NGC_SoundShutdown(BINKSND PTR4* snd)
{
    u32 i;
    void PTR4* allocation;
    AXVPB PTR4* voice;
    AXVPB PTR4** voices;
    NGCSoundState PTR4* state;

    NGC_SOUND_STATE(snd)->paused = NGC_SOUND_PAUSED;

    state = NGC_SOUND_STATE(snd);
    voice = NGC_LEFT_VOICE(state);
    if (voice != 0) {
        AXSetVoiceState(voice, AX_PB_STATE_STOP);
    }

    voice = NGC_RIGHT_VOICE(state);
    if (voice != 0) {
        AXSetVoiceState(voice, AX_PB_STATE_STOP);
    }

    i = 0;
    voices = &state->left_voice;
    for (; i < NGC_SOUND_STEREO_CHANNELS; ++i) {
        voice = voices[i];
        if (voice != 0) {
            AXFreeVoice(voice);
            voices[i] = 0;
        }
    }

    allocation = NGC_SOUND_STATE(snd)->decode_buffer;
    if (allocation != 0) {
        radfree(allocation);
        NGC_SOUND_STATE(snd)->decode_buffer = 0;
    }

    allocation = NGC_SOUND_STATE(snd)->stereo_buffer;
    if (allocation != 0) {
        radfree(allocation);
        NGC_SOUND_STATE(snd)->stereo_buffer = 0;
    }

    allocation = NGC_SOUND_STATE(snd)->audio_buffer;
    if (allocation != 0) {
        radaudiofree(allocation);
        NGC_SOUND_STATE(snd)->audio_buffer = 0;
    }
}

static s32 Lock(BINKSND PTR4* snd, u8 PTR4* PTR4* addr, u32 PTR4* len)
{
    NGCSoundState PTR4* state;
    u32 writable_bytes;
    u32 voice_cursor;
    u32 half_size;
    u32 channel_stride;
    ARQRequest PTR4* task;
    ARQRequest PTR4* right_task;
    AXVPB PTR4* voice;
    u32 shift;
    u8 PTR4* left_buffer;
    u8 PTR4* decode_buffer;

    if (NGC_SOUND_STATE(snd)->lock_index >= 0) {
        state = NGC_SOUND_STATE(snd);
        writable_bytes = NGC_SOUND_STATE(snd)->play_cursor;
        voice = NGC_LEFT_VOICE(state);
        shift = NGC_ADDRESS_SHIFT(state);
        voice_cursor = NGC_AX_CURRENT_CURSOR(voice, shift);

        if (writable_bytes >= voice_cursor) {
            channel_stride = NGC_SOUND_STATE(snd)->channel_stride;
            writable_bytes = ((u32)NGC_SOUND_STATE(snd)->audio_buffer + channel_stride) - writable_bytes;
        } else {
            channel_stride = NGC_SOUND_STATE(snd)->channel_stride;
            writable_bytes = (voice_cursor - writable_bytes) - 1;
        }

        half_size = channel_stride >> NGC_SOUND_HALF_BUFFER_SHIFT;
        if (writable_bytes > half_size) {
            writable_bytes = half_size;
        } else if (writable_bytes < NGC_SOUND_STATE(snd)->frame_size) {
            writable_bytes = NGC_SOUND_STATE(snd)->frame_size;
        }

        /* The lock index selects one half of the MRAM staging buffer. */
        task = NGC_LEFT_LOCK_TASK(state, NGC_SOUND_STATE(snd)->lock_index);
        left_buffer = NGC_SOUND_STATE(snd)->decode_buffer +
                      ((NGC_SOUND_STATE(snd)->lock_index * channel_stride) >>
                       NGC_SOUND_HALF_BUFFER_SHIFT);
        writable_bytes *= snd->chans;
        task->source = (u32)left_buffer;
        decode_buffer = left_buffer;
        left_buffer += NGC_SOUND_STATE(snd)->channel_stride;
        right_task = NGC_RIGHT_LOCK_TASK(state, NGC_SOUND_STATE(snd)->lock_index);
        right_task->source = (u32)left_buffer;

        if (snd->chans == NGC_SOUND_STEREO_CHANNELS) {
            /* Bink decodes interleaved stereo; AX/ARAM playback uses split left/right channels. */
            decode_buffer = NGC_SOUND_STATE(snd)->stereo_buffer;
        }

        *addr = decode_buffer;
        *len = writable_bytes;

        return 1;
    }

    return 0;
}

static s32 Unlock(BINKSND PTR4* snd, u32 filled_bytes)
{
    NGCSoundState PTR4* state;
    ARQRequest PTR4* task;
    u32 padded_bytes;

    if (NGC_SOUND_STATE(snd)->lock_index == NGC_SOUND_NO_LOCK_INDEX) {
        return 0;
    }

    state = NGC_SOUND_STATE(snd);
    task = NGC_TASK(state, state->lock_index);
    NGC_TASK_MARK_BUSY(task);

    if (NGC_SND(snd)->chans == NGC_SOUND_STEREO_CHANNELS) {
        /* Split the temporary interleaved stereo buffer into the two ARQ upload buffers. */
        u8 PTR4* left_buffer = NGC_TASK_SOURCE(task);
        u8 PTR4* right_buffer =
            NGC_TASK_SOURCE(NGC_RIGHT_LOCK_TASK(state, state->lock_index));
        u8 PTR4* stereo_src = state->stereo_buffer;

        if (NGC_SND(snd)->bits == NGC_SOUND_BITS_16) {
            u32 i;
            u32 PTR4* src32 = (u32 PTR4*)stereo_src;
            u32 PTR4* left32 = (u32 PTR4*)left_buffer;
            u32 PTR4* right32 = (u32 PTR4*)right_buffer;
            u32 groups = filled_bytes >> NGC_STEREO16_GROUP_SHIFT;

            for (i = 0; i < groups; ++i) {
                u32 first = src32[0];
                u32 second = src32[1];
                src32 += 2;

                *left32++ = NGC_SAMPLE_LEFT_16_PAIR(first, second);
                *right32++ = NGC_SAMPLE_RIGHT_16_PAIR(first, second);
            }

            groups = (filled_bytes - (groups << NGC_STEREO16_GROUP_SHIFT)) >> NGC_STEREO16_TAIL_SHIFT;
            for (i = 0; i < groups; ++i) {
                u32 stereo_pair = *src32++;

                *(u16 PTR4*)left32 = (u16)NGC_SAMPLE_HIGH_HALF(stereo_pair);
                *(u16 PTR4*)right32 = (u16)stereo_pair;
                left32 = NGC_ADVANCE_U32_BYTES(left32, 2);
                right32 = NGC_ADVANCE_U32_BYTES(right32, 2);
            }
        } else {
            u32 i;
            u32 PTR4* src32 = (u32 PTR4*)stereo_src;
            u16 PTR4* left16 = (u16 PTR4*)left_buffer;
            u16 PTR4* right16 = (u16 PTR4*)right_buffer;
            u32 groups = filled_bytes >> NGC_STEREO8_GROUP_SHIFT;

            for (i = 0; i < groups; ++i) {
                u32 packed_samples = *src32++;

                *left16++ = (u16)NGC_SAMPLE_LEFT_8_PAIR(packed_samples);
                *right16++ = (u16)NGC_SAMPLE_RIGHT_8_PAIR(packed_samples);
            }

            groups = (filled_bytes - (groups << NGC_STEREO8_GROUP_SHIFT)) >> NGC_STEREO8_TAIL_SHIFT;
            for (i = 0; i < groups; ++i) {
                u16 stereo_pair = *(u16 PTR4*)src32;

                src32 = NGC_ADVANCE_U32_BYTES(src32, 2);
                *(u8 PTR4*)left16 = (u8)(stereo_pair >> NGC_SAMPLE_BYTE_SHIFT);
                *(u8 PTR4*)right16 = (u8)stereo_pair;
                left16 = NGC_ADVANCE_U16_BYTES(left16, 1);
                right16 = NGC_ADVANCE_U16_BYTES(right16, 1);
            }
        }

        filled_bytes >>= NGC_SOUND_HALF_BUFFER_SHIFT;
    }

    if ((filled_bytes & NGC_SOUND_FRAME_ALIGN_MASK) != 0) {
        u32 i;

        padded_bytes = NGC_ALIGN_UP(filled_bytes, NGC_SOUND_FRAME_ALIGN_MASK);
        for (i = 0; i < NGC_SND(snd)->chans; ++i) {
            memset(NGC_TASK_SOURCE_AT(NGC_TASK_FOR_LOCK_CHANNEL(state, state->lock_index, i),
                                      filled_bytes),
                   0, padded_bytes - filled_bytes);
        }
        filled_bytes = padded_bytes;
    }

    if (state->play_state == NGC_PLAY_STATE_STOPPED) {
        NGC_TASK(state, state->lock_index)->length = filled_bytes;
        if (state->lock_index == NGC_SOUND_LAST_LOCK_INDEX) {
            NGC_SoundPlay(snd, 0, NGC_TASK(state, 0)->length);
            NGC_SoundPlay(snd, 1, NGC_TASK(state, 1)->length);
        }
    } else {
        NGC_SoundPlay(snd, state->lock_index, filled_bytes);
    }

    return 1;
}

static void NGC_StarvedClear(BINKSND PTR4* snd)
{
    u32 poll_count;
    u32 lock_side;
    u32 buffer_side;
    u32 right_task_index;
    u32 silent_start;
    u32 silent_end;
    NGCSoundState PTR4* state;
    ARQRequest PTR4* task;
    u8 PTR4* silence_buffer;
    AXVPB PTR4* voice;

    state = NGC_SOUND_STATE(snd);
    poll_count = 0;
check_busy:
    /* Wait for one staging half to be free before injecting silence. */
    lock_side = poll_count & NGC_SOUND_LOCK_INDEX_MASK;
    task = NGC_LEFT_LOCK_TASK(state, lock_side);
    if (NGC_TASK_BUSY(task)) {
        goto busy;
    }
    buffer_side = lock_side;
    silent_start = NGC_SOUND_STATE(snd)->play_cursor;
    silent_end = silent_start + NGC_SOUND_STATE(snd)->frame_size;
    if (NGC_SND(snd)->chans == NGC_SOUND_STEREO_CHANNELS) {
        silence_buffer = NGC_SOUND_STATE(snd)->stereo_buffer;
    } else {
        silence_buffer = NGC_SOUND_STATE(snd)->decode_buffer +
                         ((buffer_side * NGC_SOUND_STATE(snd)->channel_stride) >>
                          NGC_SOUND_HALF_BUFFER_SHIFT);
    }

    memset(silence_buffer, 0, NGC_SOUND_STATE(snd)->frame_size);
    task = NGC_LEFT_LOCK_TASK(state, lock_side);
    task->source = (u32)silence_buffer;
    right_task_index = NGC_RIGHT_LOCK_TASK_INDEX(lock_side);
    NGC_TASK(state, right_task_index)->source = (u32)silence_buffer;
    NGC_SoundPlay(snd, lock_side, NGC_SOUND_STATE(snd)->frame_size);

    /* Re-anchor playback to the silent frame if AX has already passed it. */
    voice = NGC_LEFT_VOICE(state);
    if (NGC_AX_CURRENT_CURSOR(voice, NGC_ADDRESS_SHIFT(state)) > silent_end) {
        AXSetVoiceCurrentAddr(voice, NGC_AX_ADDR(silent_start, NGC_ADDRESS_SHIFT(state)));
        voice = NGC_RIGHT_VOICE(state);
        if (voice != 0) {
            AXSetVoiceCurrentAddr(voice, NGC_AX_RIGHT_ADDR(state, silent_start));
        }
    }

    AXSetVoiceLoopAddr(NGC_LEFT_VOICE(state), NGC_AX_ADDR(silent_start, NGC_ADDRESS_SHIFT(state)));
    voice = NGC_RIGHT_VOICE(state);
    if (voice != 0) {
        AXSetVoiceLoopAddr(voice, NGC_AX_RIGHT_ADDR(state, silent_start));
    }

    AXSetVoiceEndAddr(NGC_LEFT_VOICE(state), NGC_AX_END_ADDR(silent_end, NGC_ADDRESS_SHIFT(state)));
    voice = NGC_RIGHT_VOICE(state);
    if (voice != 0) {
        AXSetVoiceEndAddr(voice, NGC_AX_RIGHT_END_ADDR(state, silent_end));
    }
    return;

busy:
    ++poll_count;
    if ((s32)poll_count <= NGC_SOUND_MAX_BUSY_POLLS) {
        goto check_busy;
    }
}

static s32 Ready(BINKSND PTR4* snd)
{
    s32 lock_index;
    u32 now;
    NGCSoundState PTR4* state;
    AXVPB PTR4* voice;
    ARQRequest PTR4* task;
    u32 voice_cursor;
    u32 end_cursor;
    u32 address_shift;

    lock_index = NGC_SOUND_NO_LOCK_INDEX;
    if (NGC_SOUND_STATE(snd)->paused != 0 || NGC_SND(snd)->OnOff == NGC_SOUND_OFF ||
        NGC_SOUND_STATE(snd)->left_voice == 0) {
        return 0;
    }

    state = NGC_SOUND_STATE(snd);
    now = RADTimerRead();
    address_shift = NGC_ADDRESS_SHIFT(state);
    voice = NGC_SOUND_STATE(snd)->left_voice;
    voice_cursor = NGC_AX_CURRENT_CURSOR(voice, address_shift);
    if (NGC_SOUND_STATE(snd)->play_state == NGC_PLAY_STATE_RUNNING) {
        u32 buffered_bytes;

        end_cursor = NGC_AX_END_CURSOR(voice, address_shift);
        buffered_bytes = 0;
        /* A pending wrapped end address becomes valid once playback crosses the wrap. */
        if (voice_cursor < NGC_SOUND_STATE(snd)->pending_end) {
            end_cursor = NGC_SOUND_STATE(snd)->pending_end;
            AXSetVoiceEndAddr(voice, NGC_AX_END_ADDR(end_cursor, address_shift));
            voice = NGC_RIGHT_VOICE(state);
            if (voice != 0) {
                AXSetVoiceEndAddr(voice, NGC_AX_RIGHT_END_ADDR(state, end_cursor));
            }
            NGC_SOUND_STATE(snd)->pending_end = 0;
            NGC_SOUND_STATE(snd)->play_cursor = (u32)NGC_SOUND_STATE(snd)->audio_buffer;
        }

        if ((now - NGC_SOUND_STATE(snd)->last_ready_time) < NGC_SOUND_STATE(snd)->starvation_time) {
            buffered_bytes = NGC_SOUND_STATE(snd)->play_cursor;
            if (voice_cursor < buffered_bytes) {
                buffered_bytes -= voice_cursor;
            } else {
                buffered_bytes =
                    (end_cursor - voice_cursor) +
                    (buffered_bytes - (u32)NGC_SOUND_STATE(snd)->audio_buffer);
            }
            if (buffered_bytes >= NGC_SOUND_STATE(snd)->starvation_threshold) {
                goto check_tasks;
            }
        }

        NGC_StarvedClear(snd);
        snd->SoundDroppedOut = 1;
        NGC_SOUND_STATE(snd)->play_state = NGC_PLAY_STATE_STARVED;
    }

check_tasks:
    if (NGC_SOUND_STATE(snd)->pending_end == 0) {
        /* Only offer a lock when the writer is safely ahead of the AX cursor. */
        if (NGC_SOUND_STATE(snd)->play_cursor >= voice_cursor ||
            voice_cursor - NGC_SOUND_STATE(snd)->play_cursor > NGC_SOUND_STATE(snd)->frame_size) {
            lock_index = 0;
            task = state->tasks;
            for (;;) {
                u32 owner = task->owner;

                ++task;
                if (!NGC_TASK_OWNER_BUSY(owner)) {
                    break;
                }
                ++lock_index;
                if (lock_index > NGC_SOUND_LAST_LOCK_INDEX) {
                    lock_index = NGC_SOUND_NO_LOCK_INDEX;
                    break;
                }
            }
        }
    }

    NGC_SOUND_STATE(snd)->last_ready_time = now;
    NGC_SOUND_STATE(snd)->lock_index = lock_index;
    return lock_index != NGC_SOUND_NO_LOCK_INDEX;
}

static void Volume(BINKSND PTR4* snd, s32 volume)
{
    if (volume < 0) {
        volume = 0;
    }
    if (volume > BINK_NGC_VOLUME_MAX) {
        volume = BINK_NGC_VOLUME_MAX;
    }

    NGC_SOUND_STATE(snd)->volume = volume;
    NGC_SoundVolume(snd);
}

static void Pan(BINKSND PTR4* snd, s32 pan)
{
    if (pan < 0) {
        pan = 0;
    }
    if (pan > BINK_NGC_PAN_MAX) {
        pan = BINK_NGC_PAN_MAX;
    }

    NGC_SOUND_STATE(snd)->pan = (f32)pan * NGC_SOUND_PAN_TO_FLOAT;
    NGC_SoundVolume(snd);
}

static s32 SetOnOff(BINKSND PTR4* snd, s32 status)
{
    if (status == NGC_SOUND_ON && NGC_SND(snd)->OnOff == NGC_SOUND_OFF) {
        if (NGC_SoundReinit(snd) == 0) {
            return NGC_SND(snd)->OnOff;
        }
        NGC_SND(snd)->OnOff = status;
    } else if (status == NGC_SOUND_OFF && NGC_SND(snd)->OnOff == NGC_SOUND_ON) {
        AXVPB PTR4* voice;
        NGCSoundState PTR4* state = NGC_SOUND_STATE(snd);

        voice = NGC_LEFT_VOICE(state);
        if (voice != 0) {
            AXSetVoiceState(voice, AX_PB_STATE_STOP);
        }

        voice = NGC_RIGHT_VOICE(state);
        if (voice != 0) {
            AXSetVoiceState(voice, AX_PB_STATE_STOP);
        }

        NGC_SND(snd)->OnOff = status;
    }

    return NGC_SND(snd)->OnOff;
}

static s32 Pause(BINKSND PTR4* snd, s32 status)
{
    if (status) {
        NGC_SoundPause(snd);
        NGC_SOUND_STATE(snd)->paused = NGC_SOUND_PAUSED;
    } else {
        NGC_SoundResume(snd);
        NGC_SOUND_STATE(snd)->paused = NGC_SOUND_UNPAUSED;
    }

    return NGC_SOUND_STATE(snd)->paused;
}

static void Close(BINKSND PTR4* snd)
{
    NGC_SoundShutdown(snd);
}

static s32 Open(BINKSND PTR4* snd, u32 freq, s32 bits, s32 chans, u32 flags, HBINK bink)
{
    s32 result;

    memset(snd, 0, sizeof(*snd));

    NGC_SND(snd)->freq = freq;
    NGC_SND(snd)->bits = bits;
    NGC_SND(snd)->chans = chans;
    NGC_SND(snd)->SoundDroppedOut = 0;
    NGC_SOUND_STATE(snd)->paused = NGC_SOUND_UNPAUSED;
    NGC_SOUND_STATE(snd)->volume = BINK_NGC_VOLUME_MAX;
    NGC_SOUND_STATE(snd)->pan = NGC_SOUND_PAN_CENTER;
    NGC_SND(snd)->OnOff = NGC_SOUND_ON;

    snd->Ready = Ready;
    snd->Lock = Lock;
    snd->Unlock = Unlock;
    snd->Volume = Volume;
    snd->Pan = Pan;
    snd->Pause = Pause;
    snd->SetOnOff = SetOnOff;
    snd->Close = Close;

    if (NGC_SoundInit(snd) != 0) {
        NGC_SND(snd)->NoThreadService = 0;
        result = 1;
    } else {
        result = 0;
    }

    return result;
}

BINKSNDOPEN BinkOpenNGCSound(u32 param)
{
    (void)param;
    return Open;
}

void MyAXSetVoiceCurrentAddr(NGCSoundState PTR4* state, u32 addr)
{
    u32 shift = NGC_ADDRESS_SHIFT(state);
    AXVPB PTR4* voice = NGC_LEFT_VOICE(state);

    AXSetVoiceCurrentAddr(voice, NGC_AX_ADDR(addr, shift));

    voice = NGC_RIGHT_VOICE(state);
    if (voice != 0) {
        AXSetVoiceCurrentAddr(voice, NGC_AX_RIGHT_ADDR(state, addr));
    }
}

void MyAXSetVoiceLoopAddr(NGCSoundState PTR4* state, u32 addr)
{
    u32 shift = NGC_ADDRESS_SHIFT(state);
    AXVPB PTR4* voice = NGC_LEFT_VOICE(state);

    AXSetVoiceLoopAddr(voice, NGC_AX_ADDR(addr, shift));

    voice = NGC_RIGHT_VOICE(state);
    if (voice != 0) {
        AXSetVoiceLoopAddr(voice, NGC_AX_RIGHT_ADDR(state, addr));
    }
}

void MyAXSetVoiceEndAddr(NGCSoundState PTR4* state, u32 addr)
{
    u32 shift = NGC_ADDRESS_SHIFT(state);
    AXVPB PTR4* voice = NGC_LEFT_VOICE(state);

    AXSetVoiceEndAddr(voice, NGC_AX_END_ADDR(addr, shift));

    voice = NGC_RIGHT_VOICE(state);
    if (voice != 0) {
        AXSetVoiceEndAddr(voice, NGC_AX_RIGHT_END_ADDR(state, addr));
    }
}

void MyAXSetVoiceState(NGCSoundState PTR4* state, u16 voice_state)
{
    AXVPB PTR4* voice = NGC_LEFT_VOICE(state);

    if (voice != 0) {
        AXSetVoiceState(voice, voice_state);
    }

    voice = NGC_RIGHT_VOICE(state);
    if (voice != 0) {
        AXSetVoiceState(voice, voice_state);
    }
}

u32 get_play_pos(NGCSoundState PTR4* state)
{
    AXVPB PTR4* voice = NGC_LEFT_VOICE(state);
    u32 shift = NGC_ADDRESS_SHIFT(state);

    return NGC_AX_CURRENT_CURSOR(voice, shift);
}

u32 get_end_pos(NGCSoundState PTR4* state)
{
    AXVPB PTR4* voice = NGC_LEFT_VOICE(state);
    u32 shift = NGC_ADDRESS_SHIFT(state);

    return NGC_AX_END_CURSOR(voice, shift);
}

ARQRequest PTR4* get_task(NGCSoundState PTR4* state, u32 index, u32 side)
{
    return NGC_TASK_FOR_INDEX(state, index, side);
}

void ConvDataToStereo8(u32 PTR4* src, u16 PTR4* left, u16 PTR4* right, u32 bytes)
{
    u32 i;
    u32 total = bytes;
    u32 count;

    count = total >> NGC_STEREO8_GROUP_SHIFT;

    for (i = 0; i < count; ++i) {
        u32 packed_samples = *src++;

        *left++ = (u16)NGC_SAMPLE_LEFT_8_PAIR(packed_samples);
        *right++ = (u16)NGC_SAMPLE_RIGHT_8_PAIR(packed_samples);
    }

    count = (total - (count << NGC_STEREO8_GROUP_SHIFT)) >> NGC_STEREO8_TAIL_SHIFT;
    for (i = 0; i < count; ++i) {
        u16 packed_samples = *(u16 PTR4*)src;
        src = NGC_ADVANCE_U32_BYTES(src, 2);
        *(u8 PTR4*)left = (u8)(packed_samples >> NGC_SAMPLE_BYTE_SHIFT);
        *(u8 PTR4*)right = (u8)packed_samples;
        left = NGC_ADVANCE_U16_BYTES(left, 1);
        right = NGC_ADVANCE_U16_BYTES(right, 1);
    }
}

void ConvDataToStereo16(u32 PTR4* src, u32 PTR4* left, u32 PTR4* right, u32 bytes)
{
    u32 i;
    u32 total = bytes;
    u32 count;

    count = total >> NGC_STEREO16_GROUP_SHIFT;

    for (i = 0; i < count; ++i) {
        u32 first = src[0];
        u32 second = src[1];
        src += 2;

        *left++ = NGC_SAMPLE_LEFT_16_PAIR(first, second);
        *right++ = NGC_SAMPLE_RIGHT_16_PAIR(first, second);
    }

    count = (total - (count << NGC_STEREO16_GROUP_SHIFT)) >> NGC_STEREO16_TAIL_SHIFT;
    for (i = 0; i < count; ++i) {
        u32 packed_samples = *src++;
        *(u16 PTR4*)left = (u16)NGC_SAMPLE_HIGH_HALF(packed_samples);
        *(u16 PTR4*)right = (u16)packed_samples;
        left = NGC_ADVANCE_U32_BYTES(left, 2);
        right = NGC_ADVANCE_U32_BYTES(right, 2);
    }
}
