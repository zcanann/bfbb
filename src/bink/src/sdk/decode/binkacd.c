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
    BINKAC_QUANT_COUNT = TOTBANDS + BINKAC_BAND_SENTINEL_COUNT,
    BINKAC_BAND_LIMIT_SCALE = 2,
    BINKAC_QUANT_BITS = 8
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

#define BINKAC_BIT_MASK 1
#define BINKAC_UNDECIBEL_BASE 10.0
#define BINKAC_UNDECIBEL_DB_SCALE 0.10f
#define BINKAC_QUANT_DB_SCALE 0.664f
#define BINKAC_BAND_LIMIT_COUNT(num_bands) ((num_bands) + BINKAC_BAND_SENTINEL_COUNT)
typedef enum BINKACSampleCountState
{
    BINKAC_SAMPLE_COUNT_UNDERFLOW = 0xffffffffU
} BINKACSampleCountState;

typedef VARBITS BINKVARBITS;

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
#define BINKAC_ZERO_BYTE 0
#define BINKAC_STEREO_LEFT_COEFF(coeffs) ((coeffs)[0])
#define BINKAC_STEREO_RIGHT_COEFF(coeffs, stride) ((coeffs)[(stride)])

static const f64 BINKAC_FXP_TO_FLOAT_BIAS = 4503599627370496.0;
static const f32 BINKAC_SAMPLE_ZERO = 0.0f;
static const f64 BINKAC_VARBITS_U32_TO_F64_BIAS = 4503601774854144.0;
static const f64 BINKAC_QUANT_U32_TO_F64_BIAS = 4503601774854144.0;
static const f32 BINKAC_QUANT_INDEX_SCALE_CONST = 0.664f;
static const f32 BINKAC_QUANT_POWER_SCALE_CONST = 0.10f;
static const f64 BINKAC_QUANT_POWER_BASE_CONST = 10.0;
static const f64 BINKAC_OPEN_U32_TO_F64_BIAS = 4503599627370496.0;
static const f32 BINKAC_RSQRT_ZERO = 0.0f;
static const f64 BINKAC_RSQRT_NEWTON_HALF_CONST = 0.5;
static const f64 BINKAC_RSQRT_NEWTON_THREE_CONST = 3.0;
static const f64 BINKAC_U32_LIMIT_AS_F64 = 2147483648.0;
static const f32 BINKAC_TRANSFORM_ROOT_SCALE_CONST = 2.0f;

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

static void quanttos16s(s16 PTR4* samples, const f32 PTR4* decoded_coeffs,
                        f32 transform_size_root, u32 transform_size)
{
    if (transform_size != 0) {
        do {
            s16 PTR4* out = samples;
            s32 sample_value = (s32)(*decoded_coeffs * transform_size_root);

            samples = out + 1;
            *out = clamp_to_s16(sample_value);
            ++decoded_coeffs;
            --transform_size;
        } while (transform_size != 0);
    }
}

static void quanttos16chans2(s16 PTR4* samples, const f32 PTR4* decoded_coeffs,
                             f32 transform_size_root, u32 transform_size)
{
    u32 remaining;
    u32 stride;

    remaining = transform_size - 1;
    if (remaining != BINKAC_SAMPLE_COUNT_UNDERFLOW) {
        stride = transform_size;
        while (remaining != BINKAC_SAMPLE_COUNT_UNDERFLOW) {
            s16 PTR4* out = samples;
            s32 sample_value = (s32)(BINKAC_STEREO_LEFT_COEFF(decoded_coeffs) * transform_size_root);

            samples = out + 1;
            *out = clamp_to_s16(sample_value);
            out = samples++;
            sample_value = (s32)(BINKAC_STEREO_RIGHT_COEFF(decoded_coeffs, stride) * transform_size_root);
            *out = clamp_to_s16(sample_value);
            ++decoded_coeffs;
            --remaining;
        }
    }
}

static inline u32 read_bits(BINKVARBITS PTR4* vb, u32 count)
{
    u32 bitcount = vb->bitlen;

    if (bitcount >= count) {
        u32 result = vb->bits & GetBitsLen(count);

        vb->bitlen = bitcount - count;
        vb->bits >>= count;
        return result;
    } else {
        u32 refill = BINKAC_LOAD32(vb->cur);
        u32 reservoir = vb->bits | (refill << bitcount);
        u32 result = reservoir & GetBitsLen(count);

        VARBITS_ADVANCE_CUR(vb->cur);
        vb->bitlen = bitcount + BITSTYPELEN - count;
        vb->bits = refill >> (count - bitcount);
        return result;
    }
}

static inline u32 read_rle_bits(BINKVARBITS PTR4* vb)
{
    u32 result;
    u32 bitcount = vb->bitlen;

    if (bitcount > (RLEBITS - 1)) {
        result = vb->bits & GetBitsLen(RLEBITS);
        vb->bitlen = bitcount - RLEBITS;
        vb->bits >>= RLEBITS;
    } else {
        u32 refill = BINKAC_LOAD32(vb->cur);
        u32 reservoir = vb->bits | (refill << bitcount);

        VARBITS_ADVANCE_CUR(vb->cur);
        result = reservoir & GetBitsLen(RLEBITS);
        vb->bitlen = bitcount + BITSTYPELEN - RLEBITS;
        vb->bits = refill >> (RLEBITS - bitcount);
    }

    return result;
}

static inline u32 read_bit(BINKVARBITS PTR4* vb)
{
    u32 bitcount = vb->bitlen;
    u32 result;

    if (bitcount != 0) {
        result = vb->bits & BINKAC_BIT_MASK;
        vb->bitlen = bitcount - 1;
        vb->bits >>= 1;
    } else {
        u32 refill = BINKAC_LOAD32(vb->cur);

        VARBITS_ADVANCE_CUR(vb->cur);
        result = refill & BINKAC_BIT_MASK;
        vb->bitlen = BITSTYPELEN - 1;
        vb->bits = refill >> 1;
    }

    return result;
}

static void read_rle_samples(f32 PTR4* samps, u32 transform_size, BINKVARBITS PTR4* vbp,
                             const f32 PTR4* threshold, const u32 PTR4* bands)
{
    u32 i;
    u32 b = 0;
    f32 dequant = BINKAC_SAMPLE_ZERO;
    f32 PTR4* out;

    while (BINKAC_BAND_SAMPLE_LIMIT(bands, b) < BINKAC_FIRST_COEFF) {
        dequant = threshold[b];
        ++b;
    }

    i = BINKAC_FIRST_COEFF;
    out = samps + BINKAC_FIRST_COEFF;

    while (i < transform_size) {
        u32 end;
        u32 bitlen;

        /* Each sparse coefficient packet is either 5 bits (literal VQ run) or
           9 bits (RLE flag, 4-bit run index, 4-bit coefficient bit length). */
        {
            u32 rle_flag = read_bit(vbp);

            if (rle_flag != 0) {
                end = i + BINKAC_RLE_SAMPLE_RUN(read_rle_bits(vbp));
            } else {
                end = i + VQLENGTH;
            }
        }

        if (end > transform_size) {
            end = transform_size;
        }

        bitlen = read_rle_bits(vbp);
        if (bitlen == 0) {
            u32 zero_count = end - i;

            memset(out, BINKAC_ZERO_BYTE, zero_count * sizeof(*out));
            out += zero_count;
            i = end;

            while (i > BINKAC_BAND_SAMPLE_LIMIT(bands, b)) {
                dequant = threshold[b];
                ++b;
            }
        } else {
            while (i < end) {
                if (i == BINKAC_BAND_SAMPLE_LIMIT(bands, b)) {
                    dequant = threshold[b];
                    ++b;
                }

                {
                    s32 magnitude = read_bits(vbp, bitlen);

                    if (magnitude) {
                        /* Bink audio 1 stores the sign bit after each nonzero coefficient. */
                        u32 sign_bit = read_bit(vbp);
                        s32 sign = -(s32)sign_bit;
                        magnitude = (magnitude ^ sign) - sign;
                        *out = magnitude * dequant;
                    } else {
                        *out = 0.0f;
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
    return (f32)pow(BINKAC_QUANT_POWER_BASE_CONST, d * BINKAC_QUANT_POWER_SCALE_CONST);
}

static u32 Unquant(u32 transform_size, u32 chans, u32 flags, s32 PTR4* fft_work,
                   f32 PTR4* fft_coeffs, s16 PTR4* samples, void PTR4* inptr,
                   u32 num_bands, const u32 PTR4* bands,
                   f32 transform_size_root)
{
    f32 threshold[BINKAC_QUANT_COUNT];
    BINKVARBITS vb;
    f32 decoded_coeffs[MAX_TRANSFORM];
    u32 ch;
    f32 PTR4* channel;
    u32 i;
    u32 q;

    vb.init = inptr;
    vb.cur = inptr;
    vb.bitlen = 0;
    vb.bits = 0;
    if (flags & BINKACNEWFORMAT) {
        /* New-format streams reserve two leading bits before the coefficient payload. */
        vb.bits = BINKAC_LOAD32(vb.cur) >> BINKACNEWFORMAT_SKIP_BITS;
        VARBITS_ADVANCE_CUR(vb.cur);
        vb.bitlen = BITSTYPELEN - BINKACNEWFORMAT_SKIP_BITS;
    }

    channel = decoded_coeffs;
    for (ch = 0; ch < chans; ++ch) {
        VarBitsGet(i, u32, vb, FXPBITS);
        channel[BINKAC_DC_COEFF_0] = fxptof(i);

        VarBitsGet(i, u32, vb, FXPBITS);
        channel[BINKAC_DC_COEFF_1] = fxptof(i);

        for (i = 0; i < num_bands; ++i) {
            VarBitsGet(q, u32, vb, BINKAC_QUANT_BITS);
            threshold[i] = Undecibel((f32)(s32)q * BINKAC_QUANT_INDEX_SCALE_CONST);
        }

        read_rle_samples(channel, transform_size, &vb, threshold, bands);
        if (flags & BINKACNEWFORMAT) {
            ddct(transform_size, BINKAC_DCT_INVERSE, channel, fft_work, fft_coeffs);
        } else {
            rdft(transform_size, BINKAC_RDFT_INVERSE, channel, fft_work, fft_coeffs);
        }

        channel += transform_size;
    }

    if (chans == BINKAC_MONO_CHANNELS) {
        quanttos16s(samples, decoded_coeffs, transform_size_root, transform_size);
    } else {
        quanttos16chans2(samples, decoded_coeffs, transform_size_root, transform_size);
    }

    vb.bitlen = 0;
    return BINKAC_VARBITS_USED_BYTES(vb);
}

static inline f32 radfsqrt(f32 value)
{
    if (value > 0.0f) {
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
    u32 band_index;
    u32 transform_size;
    u32 transform_size_half;
    u32 buffer_size;
    f32 transform_size_root;
    u32 num_bands;
    s32 nyq;
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
    if (!(flags & BINKACNEWFORMAT)) {
        /* Legacy RDFT streams interleave stereo by decoding one larger mono transform. */
        rate *= chans;
        transform_size *= chans;
        chans = BINKAC_MONO_CHANNELS;
    }

    nyq = (rate + BINKAC_NYQUIST_ROUNDING) / BINKAC_NYQUIST_DIVISOR;
    transform_size_half = transform_size / BINKAC_TRANSFORM_HALF_DIVISOR;
    /* Calculate the number of critical bands below Nyquist. */
    for (band_index = 0; band_index < TOTBANDS; ++band_index) {
        if (bink_bandtopfreq[band_index] >= (u32)nyq) {
            break;
        }
    }

    num_bands = band_index;
    pushmalloc((void PTR4* PTR4*)&bands, BINKAC_BAND_LIMIT_COUNT(num_bands) * sizeof(*bands));
    pushmalloc((void PTR4* PTR4*)&fft_work, BINKAC_FFT_WORK_BYTES(transform_size_half, fft_work));
    if (flags & BINKACNEWFORMAT) {
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

    for (band_index = 0; band_index < num_bands; ++band_index) {
        ba->bands[band_index] = (bink_bandtopfreq[band_index] * transform_size_half) / nyq;
        if (ba->bands[band_index] == 0) {
            ba->bands[band_index] = 1;
        }
    }
    ba->bands[band_index] = transform_size_half;
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
    u32 used_bytes;

    transform_size = ba->transform_size;
    transform_size_root = ba->transform_size_root;
    chans = ba->chans;
    flags = ba->flags;
    fft_work = ba->fft_work;
    fft_coeffs = ba->fft_coeffs;
    samples = ba->samples;
    num_bands = ba->num_bands;
    bands = ba->bands;
    used_bytes = Unquant(transform_size, chans, flags, fft_work, fft_coeffs, samples, inptr,
                         num_bands, bands, transform_size_root);

    /* Later frames overlap-add their leading window against the saved tail from the last frame. */
    if (ba->start_frame != 0) {
        ba->start_frame = 0;
    } else {
        u32 sample_index;
        u32 window_samples = BINKAC_WINDOW_SAMPLES(ba->window_size_in_bytes);

        for (sample_index = 0; sample_index < window_samples; ++sample_index) {
            ba->samples[sample_index] =
                (ba->samples[sample_index] * sample_index +
                 ba->overlap[sample_index] * (window_samples - sample_index)) /
                window_samples;
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
        *inoutptr = BINKAC_INPUT_ADVANCE(inptr, used_bytes);
    }
}

void radfree(void PTR4* ptr);

void BinkAudioDecompressClose(HBINKAUDIODECOMP handle)
{
    radfree(handle);
}
