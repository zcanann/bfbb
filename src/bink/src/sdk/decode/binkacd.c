#include "bink.h"
#include "binkacd.h"
#include "popmal.h"
#include "../fft.h"
#include "../varbits.h"

typedef enum BINKACTransformLayout
{
    WINDOWRATIO = 16,
    BINKAC_RATE_44K = 44100,
    BINKAC_RATE_22K = 22050,
    BINKAC_TRANSFORM_44K = 2048,
    BINKAC_TRANSFORM_22K = 1024,
    BINKAC_TRANSFORM_11K = 512,
    BINKAC_MAX_CHANNELS = 2,
    MAX_TRANSFORM = BINKAC_TRANSFORM_44K * BINKAC_MAX_CHANNELS
} BINKACTransformLayout;

typedef enum BINKACBandLayout
{
    TOTBANDS = 25,
    BINKAC_BAND_SENTINEL_COUNT = 1,
    BINKAC_THRESHOLD_COUNT = TOTBANDS + BINKAC_BAND_SENTINEL_COUNT,
    BINKAC_BAND_LIMIT_SCALE = 2,
    BINKAC_THRESHOLD_BITS = 8
} BINKACBandLayout;

typedef enum BINKACRLELayout
{
    RLEBITS = 4,
    MAXRLE = 1 << RLEBITS,
    VQLENGTH = 8
} BINKACRLELayout;

typedef enum BINKACFixedPointLayout
{
    FXPBITS = 29,
    FXP_SIGN_MASK = 0x10000000,
    FXP_VALUE_MASK = (1 << FXPBITS) - 1,
    FXP_BIN_MASK = 31,
    FXP_VALUE_SHIFT = 5,
    BINKAC_INVERT_BINS = 24
} BINKACFixedPointLayout;

typedef enum BINKACSampleLayout
{
    BINKACNEWFORMAT_SKIP_BITS = 2,
    BINKAC_S16_MAX = 0x7fff,
    BINKAC_S16_MIN = -0x8000,
    BINKAC_DC_COEFF_0 = 0,
    BINKAC_DC_COEFF_1 = 1,
    BINKAC_FIRST_COEFF = 2,
    BINKAC_NYQUIST_DIVISOR = 2,
    BINKAC_TRANSFORM_HALF_DIVISOR = 2,
    BINKAC_NYQUIST_ROUNDING = 1,
    BINKAC_FFT_WORK_EXTRA = 2,
    BINKAC_DCT_COEFF_BYTES_PER_SAMPLE = 5,
    BINKAC_RDFT_COEFF_TAIL_ADJUST = 1,
    BINKAC_DCT_INVERSE = 1,
    BINKAC_RDFT_INVERSE = -1,
    BINKAC_START_FRAME = 1,
    BINKAC_MONO_CHANNELS = 1
} BINKACSampleLayout;

#define BINKAC_UNDECIBEL_BASE BINKAC_QUANT_POWER_BASE_CONST
#define BINKAC_UNDECIBEL_DB_SCALE BINKAC_QUANT_POWER_SCALE_CONST
#define BINKAC_THRESHOLD_QUANT_SCALE BINKAC_QUANT_INDEX_SCALE_CONST
#define BINKAC_BAND_LIMIT_COUNT(num_bands) ((num_bands) + BINKAC_BAND_SENTINEL_COUNT)
typedef enum BINKACSampleCountState
{
    BINKAC_SAMPLE_COUNT_UNDERFLOW = 0xffffffffU
} BINKACSampleCountState;

#define BINKAC_LOAD32(ptr) (*(const u32 PTR4*)(ptr))
#define BINKAC_BAND_SAMPLE_LIMIT(bands, band) ((bands)[band] * BINKAC_BAND_LIMIT_SCALE)
#define BINKAC_RLE_SAMPLE_RUN(index) (bink_rlelens_snd[(index)] * VQLENGTH)
#define BINKAC_WINDOW_BYTES(buffer_size) ((buffer_size) / WINDOWRATIO)
#define BINKAC_WINDOW_SAMPLES(window_size) ((window_size) / sizeof(s16))
#define BINKAC_OUTPUT_BYTES(buffer_size, window_size) ((buffer_size) - (window_size))
#define BINKAC_SAMPLE_BYTES(samples) ((samples) * sizeof(s16))
#define BINKAC_FFT_WORK_BYTES(transform_size_half, work) \
    (((u32)radfsqrt((f32)(transform_size_half)) + BINKAC_FFT_WORK_EXTRA) * sizeof(*(work)))
#define BINKAC_DCT_COEFF_BYTES(transform_size) \
    ((transform_size) * BINKAC_DCT_COEFF_BYTES_PER_SAMPLE)
#define BINKAC_RDFT_COEFF_BYTES(transform_size_half, coeffs) \
    ((transform_size_half) * sizeof(*(coeffs)) - BINKAC_RDFT_COEFF_TAIL_ADJUST)
#define BINKAC_OVERLAP_BYTES(buffer_size) ((buffer_size) / BINKAC_TRANSFORM_HALF_DIVISOR)
#define BINKAC_VARBITS_USED_BYTES(bits) \
    (((u32)((u8 PTR4*)(bits).cur - (u8 PTR4*)(bits).init)) & FXP_VALUE_MASK)
#define BINKAC_OVERLAP_SOURCE(samples, buffer_size, window_size) \
    ((u8 PTR4*)(samples) + ((buffer_size) - (window_size)))
#define BINKAC_INPUT_ADVANCE(ptr, bytes) ((u8 PTR4*)(ptr) + (bytes))
#define BINKAC_SAMPLE_ZERO 0.0f
#define BINKAC_QUANT_INDEX_SCALE_CONST 0.664f
#define BINKAC_QUANT_POWER_SCALE_CONST 0.10f
#define BINKAC_QUANT_POWER_BASE_CONST 10.0
#define BINKAC_RSQRT_ZERO 0.0f
#define BINKAC_RSQRT_NEWTON_HALF_CONST 0.5
#define BINKAC_RSQRT_NEWTON_THREE_CONST 3.0
#define BINKAC_TRANSFORM_ROOT_SCALE_CONST 2.0f

/* RLE code lengths, in VQLENGTH sample groups, for sparse audio coefficients. */
static u8 bink_rlelens_snd[MAXRLE] = {
    2, 3, 4, 5, 6, 8, 9, 10, 11, 12, 13, 14, 15, 16, 32, 64
};

/* Upper frequency for each Bink audio critical band. */
static u32 bink_bandtopfreq[TOTBANDS] = {
    0,   100,  200,  300,  400,  510,  630,  770,   920,   1080,  1270, 1480, 1720,
    2000, 2320, 2700, 3150, 3700, 4400, 5300, 6400, 7700, 9500, 12000, 15500
};

/* Reciprocals used by fxptof for the 29-bit packed fixed-point coefficients. */
static f64 bink_invertbins[BINKAC_INVERT_BINS] = {
    1.0 / (1 << 23), 1.0 / (1 << 22), 1.0 / (1 << 21), 1.0 / (1 << 20),
    1.0 / (1 << 19), 1.0 / (1 << 18), 1.0 / (1 << 17), 1.0 / (1 << 16),
    1.0 / (1 << 15), 1.0 / (1 << 14), 1.0 / (1 << 13), 1.0 / (1 << 12),
    1.0 / (1 << 11), 1.0 / (1 << 10), 1.0 / (1 << 9),  1.0 / (1 << 8),
    1.0 / (1 << 7),  1.0 / (1 << 6),  1.0 / (1 << 5),  1.0 / (1 << 4),
    1.0 / (1 << 3),  1.0 / (1 << 2),  1.0 / (1 << 1),  1.0 / (1 << 0)
};

static f32 fxptof(u32 val)
{
    f32 f;

    f = (f32)((f64)((val & ~FXP_SIGN_MASK) >> FXP_VALUE_SHIFT) *
              bink_invertbins[val & FXP_BIN_MASK]);
    return (val & FXP_SIGN_MASK) ? -f : f;
}

static inline s16 clamp_to_s16(s32 value)
{
    s32 clamped;

    if (value < BINKAC_S16_MAX) {
        if (value > BINKAC_S16_MIN) {
            clamped = value;
        } else {
            clamped = BINKAC_S16_MIN;
        }
    } else {
        clamped = BINKAC_S16_MAX;
    }

    return clamped;
}

static void quanttos16s(s16 PTR4* dest, const f32 PTR4* src, f32 scale, u32 count)
{
    if (count != 0) {
        do {
            s16 PTR4* out = dest;
            s32 value = (s32)(*src * scale);

            dest = out + 1;
            *out = clamp_to_s16(value);
            ++src;
            --count;
        } while (count != 0);
    }
}

static void quanttos16chans2(s16 PTR4* dest, const f32 PTR4* src, f32 scale, u32 count)
{
    u32 remaining;
    u32 stride;

    remaining = count - 1;
    if (remaining != BINKAC_SAMPLE_COUNT_UNDERFLOW) {
        stride = count;
        while (remaining != BINKAC_SAMPLE_COUNT_UNDERFLOW) {
            s16 PTR4* out = dest;
            s32 value = (s32)(src[0] * scale);

            dest = out + 1;
            *out = clamp_to_s16(value);
            out = dest;
            dest = out + 1;
            value = (s32)(src[stride] * scale);
            *out = clamp_to_s16(value);
            ++src;
            --remaining;
        }
    }
}

static inline u32 read_bits(VARBITS PTR4* vb, u32 count)
{
    u32 bits = vb->bitlen;

    if (bits >= count) {
        u32 value = vb->bits & GetBitsLen(count);

        vb->bitlen = bits - count;
        vb->bits >>= count;
        return value;
    } else {
        u32 word = BINKAC_LOAD32(vb->cur);
        u32 temp = vb->bits | (word << bits);
        u32 value = temp & GetBitsLen(count);

        VARBITS_ADVANCE_CUR(vb->cur);
        vb->bitlen = bits + BITSTYPELEN - count;
        vb->bits = word >> (count - bits);
        return value;
    }
}

static inline u32 read_rle_bits(VARBITS PTR4* vb)
{
    u32 bits = vb->bitlen;

    if (bits >= RLEBITS) {
        u32 value = vb->bits & GetBitsLen(RLEBITS);

        vb->bitlen = bits - RLEBITS;
        vb->bits >>= RLEBITS;
        return value;
    } else {
        u32 word = BINKAC_LOAD32(vb->cur);
        u32 temp = vb->bits | (word << bits);
        u32 value = temp & GetBitsLen(RLEBITS);

        VARBITS_ADVANCE_CUR(vb->cur);
        vb->bitlen = bits + BITSTYPELEN - RLEBITS;
        vb->bits = word >> (RLEBITS - bits);
        return value;
    }
}

static inline u32 read_bit(VARBITS PTR4* vb)
{
    u32 bitcount = vb->bitlen;

    if (bitcount != 0) {
        u32 bits = vb->bits;
        u32 value = bits & 1;

        vb->bitlen = bitcount - 1;
        vb->bits = bits >> 1;
        return value;
    } else {
        u32 word = BINKAC_LOAD32(vb->cur);
        u32 value = word & 1;

        VARBITS_ADVANCE_CUR(vb->cur);
        vb->bitlen = BITSTYPELEN - 1;
        vb->bits = word >> 1;
        return value;
    }
}

static void read_rle_samples(f32 PTR4* samples, u32 transform_size, VARBITS PTR4* vb,
                             const f32 PTR4* thresholds, const u32 PTR4* bands)
{
    u32 i;
    u32 band = 0;
    f32 scale = BINKAC_SAMPLE_ZERO;
    f32 PTR4* out;

    while (BINKAC_BAND_SAMPLE_LIMIT(bands, band) < BINKAC_FIRST_COEFF) {
        scale = thresholds[band];
        ++band;
    }

    i = BINKAC_FIRST_COEFF;
    out = samples + BINKAC_FIRST_COEFF;

    while (i < transform_size) {
        u32 end;
        u32 bitlen;

        /* Each sparse coefficient packet is either 5 bits (literal VQ run) or
           9 bits (RLE flag, 4-bit run index, 4-bit coefficient bit length). */
        if (read_bit(vb) != 0) {
            end = i + BINKAC_RLE_SAMPLE_RUN(read_rle_bits(vb));
        } else {
            end = i + VQLENGTH;
        }

        if (end > transform_size) {
            end = transform_size;
        }

        bitlen = read_rle_bits(vb);
        if (bitlen == 0) {
            memset(out, 0, (end - i) * sizeof(*out));
            out += end - i;
            i = end;

            while (i > BINKAC_BAND_SAMPLE_LIMIT(bands, band)) {
                scale = thresholds[band];
                ++band;
            }
        } else {
            while (i < end) {
                if (i == BINKAC_BAND_SAMPLE_LIMIT(bands, band)) {
                    scale = thresholds[band];
                    ++band;
                }

                {
                    s32 value = read_bits(vb, bitlen);

                    if (value) {
                        /* Bink audio 1 stores the sign bit after each nonzero coefficient. */
                        u32 sign_bit = read_bit(vb);
                        s32 sign = -(s32)sign_bit;
                        value = (value ^ sign) - sign;
                        *out = value * scale;
                    } else {
                        *out = BINKAC_SAMPLE_ZERO;
                    }
                }

                ++i;
                ++out;
            }
        }
    }
}

f64 pow(f64 x, f64 y);

static inline f32 Undecibel(f32 d)
{
    return (f32)pow(BINKAC_UNDECIBEL_BASE, d * BINKAC_UNDECIBEL_DB_SCALE);
}

static u32 Unquant(u32 transform_size, u32 chans, u32 flags, s32 PTR4* fft_work,
                   f32 PTR4* fft_coeffs, s16 PTR4* samples, void PTR4* inptr,
                   u32 num_bands, const u32 PTR4* bands,
                   f32 transform_size_root)
{
    f32 thresholds[BINKAC_THRESHOLD_COUNT];
    VARBITS vb;
    f32 decoded[MAX_TRANSFORM];
    f32 PTR4* channel;
    u32 ch;
    u32 i;
    s32 q;

    vb.init = inptr;
    vb.cur = inptr;
    vb.bitlen = 0;
    vb.bits = 0;

    if ((flags & BINKACNEWFORMAT) != 0) {
        /* New-format streams reserve two leading bits before the coefficient payload. */
        vb.bits = BINKAC_LOAD32(vb.cur) >> BINKACNEWFORMAT_SKIP_BITS;
        VARBITS_ADVANCE_CUR(vb.cur);
        vb.bitlen = BITSTYPELEN - BINKACNEWFORMAT_SKIP_BITS;
    }

    channel = decoded;
    for (ch = 0; ch < chans; ++ch) {
        {
            u32 coeff;

            VarBitsGet(coeff, u32, vb, FXPBITS);
            channel[BINKAC_DC_COEFF_0] = fxptof(coeff);
        }

        {
            u32 coeff;

            VarBitsGet(coeff, u32, vb, FXPBITS);
            channel[BINKAC_DC_COEFF_1] = fxptof(coeff);
        }

        for (i = 0; i < num_bands; ++i) {
            VarBitsGet(q, s32, vb, BINKAC_THRESHOLD_BITS);
            thresholds[i] = Undecibel((f32)q * BINKAC_THRESHOLD_QUANT_SCALE);
        }

        read_rle_samples(channel, transform_size, &vb, thresholds, bands);
        if ((flags & BINKACNEWFORMAT) != 0) {
            ddct(transform_size, BINKAC_DCT_INVERSE, channel, fft_work, fft_coeffs);
        } else {
            rdft(transform_size, BINKAC_RDFT_INVERSE, channel, fft_work, fft_coeffs);
        }

        channel += transform_size;
    }

    if (chans == BINKAC_MONO_CHANNELS) {
        quanttos16s(samples, decoded, transform_size_root, transform_size);
    } else {
        quanttos16chans2(samples, decoded, transform_size_root, transform_size);
    }

    vb.bitlen = 0;
    return BINKAC_VARBITS_USED_BYTES(vb);
}

static inline f32 radfsqrt(f32 value)
{
    if (value > BINKAC_RSQRT_ZERO) {
        f64 guess;
        f64 error;

        __asm__ volatile("frsqrte %0,%1" : "=f"(error) : "f"(value));
        guess = error;
        error = guess * guess * value;
        guess = BINKAC_RSQRT_NEWTON_HALF_CONST * guess * (BINKAC_RSQRT_NEWTON_THREE_CONST - error);
        error = guess * guess * value;
        guess = BINKAC_RSQRT_NEWTON_HALF_CONST * guess * (BINKAC_RSQRT_NEWTON_THREE_CONST - error);
        error = guess * guess * value;
        guess = BINKAC_RSQRT_NEWTON_HALF_CONST * guess * (BINKAC_RSQRT_NEWTON_THREE_CONST - error);
        return value * guess;
    }

    return value;
}

HBINKAUDIODECOMP BinkAudioDecompressOpen(u32 rate, u32 chans, u32 flags)
{
    u32 transform_size;
    u32 buffer_size;
    u32 transform_size_half;
    s32 nyq;
    u32 num_bands;
    u32 i;
    f32 transform_size_root;
    HBINKAUDIODECOMP ba;
    u32 PTR4* bands;
    s32 PTR4* fft_work;
    f32 PTR4* fft_coeffs;
    s16 PTR4* overlap;
    s16 PTR4* samples;

    if (rate >= BINKAC_RATE_44K) {
        transform_size = BINKAC_TRANSFORM_44K;
    } else if (rate >= BINKAC_RATE_22K) {
        transform_size = BINKAC_TRANSFORM_22K;
    } else {
        transform_size = BINKAC_TRANSFORM_11K;
    }

    buffer_size = BINKAC_SAMPLE_BYTES(transform_size * chans);
    if ((flags & BINKACNEWFORMAT) == 0) {
        /* Legacy RDFT streams interleave stereo by decoding one larger mono transform. */
        rate *= chans;
        transform_size *= chans;
        chans = BINKAC_MONO_CHANNELS;
    }

    nyq = (rate + BINKAC_NYQUIST_ROUNDING) / BINKAC_NYQUIST_DIVISOR;
    transform_size_half = transform_size / BINKAC_TRANSFORM_HALF_DIVISOR;
    /* Calculate the number of critical bands below Nyquist. */
    for (i = 0; i < TOTBANDS; ++i) {
        if (bink_bandtopfreq[i] >= (u32)nyq) {
            break;
        }
    }

    num_bands = i;
    pushmalloc((void PTR4* PTR4*)&bands, BINKAC_BAND_LIMIT_COUNT(num_bands) * sizeof(*bands));
    pushmalloc((void PTR4* PTR4*)&fft_work, BINKAC_FFT_WORK_BYTES(transform_size_half, fft_work));
    if ((flags & BINKACNEWFORMAT) != 0) {
        pushmalloc((void PTR4* PTR4*)&fft_coeffs, BINKAC_DCT_COEFF_BYTES(transform_size));
    } else {
        pushmalloc((void PTR4* PTR4*)&fft_coeffs,
                   BINKAC_RDFT_COEFF_BYTES(transform_size_half, fft_coeffs));
    }
    pushmalloc((void PTR4* PTR4*)&overlap, BINKAC_OVERLAP_BYTES(buffer_size));
    pushmalloc((void PTR4* PTR4*)&samples, buffer_size);

    ba = (HBINKAUDIODECOMP)popmalloc(sizeof(*ba));
    if (ba == 0) {
        return 0;
    }

    memset(ba, 0, sizeof(*ba));
    ba->bands = bands;
    ba->fft_work = fft_work;
    ba->fft_coeffs = fft_coeffs;
    ba->overlap = overlap;
    ba->samples = samples;
    ba->flags = flags;
    ba->chans = chans;
    ba->num_bands = num_bands;
    ba->transform_size = transform_size;
    ba->buffer_size = buffer_size;
    ba->window_size_in_bytes = BINKAC_WINDOW_BYTES(buffer_size);
    transform_size_root = BINKAC_TRANSFORM_ROOT_SCALE_CONST / radfsqrt((f32)transform_size);
    ba->transform_size_root = transform_size_root;

    for (i = 0; i < num_bands; ++i) {
        ba->bands[i] = (bink_bandtopfreq[i] * transform_size_half) / nyq;
        if (ba->bands[i] == 0) {
            ba->bands[i] = 1;
        }
    }
    ba->bands[i] = transform_size_half;
    ba->fft_work[0] = 0;
    ba->start_frame = BINKAC_START_FRAME;

    return ba;
}

void BinkAudioDecompress(HBINKAUDIODECOMP ba, void PTR4* PTR4* outptr, u32 PTR4* outbytes,
                         void PTR4* inptr, void PTR4* PTR4* inoutptr)
{
    u32 transform_size;
    f32 transform_size_root;
    u32 chans;
    u32 flags;
    s32 PTR4* fft_work;
    f32 PTR4* fft_coeffs;
    s16 PTR4* samples;
    u32 num_bands;
    const u32 PTR4* bands;
    u32 used;

    transform_size = ba->transform_size;
    transform_size_root = ba->transform_size_root;
    chans = ba->chans;
    flags = ba->flags;
    fft_work = ba->fft_work;
    fft_coeffs = ba->fft_coeffs;
    samples = ba->samples;
    num_bands = ba->num_bands;
    bands = ba->bands;
    used = Unquant(transform_size, chans, flags, fft_work, fft_coeffs, samples, inptr,
                   num_bands, bands, transform_size_root);

    /* Later frames overlap-add their leading window against the saved tail from the last frame. */
    if (ba->start_frame != 0) {
        ba->start_frame = 0;
    } else {
        u32 i;
        u32 count = BINKAC_WINDOW_SAMPLES(ba->window_size_in_bytes);

        for (i = 0; i < count; ++i) {
            ba->samples[i] = (ba->samples[i] * i + ba->overlap[i] * (count - i)) / count;
        }
    }

    /* Save the trailing window for the next frame's overlap blend. */
    memcpy(ba->overlap,
           BINKAC_OVERLAP_SOURCE(ba->samples, ba->buffer_size, ba->window_size_in_bytes),
           ba->window_size_in_bytes);

    if (outbytes != 0) {
        /* The public frame excludes the saved overlap tail. */
        *outbytes = BINKAC_OUTPUT_BYTES(ba->buffer_size, ba->window_size_in_bytes);
    }

    if (outptr != 0) {
        *outptr = ba->samples;
    }

    if (inoutptr != 0) {
        /* Return the compressed stream cursor after the bits consumed by Unquant. */
        *inoutptr = BINKAC_INPUT_ADVANCE(inptr, used);
    }
}

void radfree(void PTR4* ptr);

void BinkAudioDecompressClose(HBINKAUDIODECOMP handle)
{
    radfree(handle);
}
