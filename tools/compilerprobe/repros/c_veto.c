/* (c-veto) moveinvariantsfromloop's large-loop veto.
 * GC/2.0p1 refuses to hoist anything out of a loop of more than 25
 * instructions that contains an FP load (LFS..LFDUX, opcodes 0x8e-0x95);
 * GC/2.5 tests AltiVec splats (VSPLTIS{B,H,W}, 0x161-0x163) instead, which a
 * Gekko never emits, so FP-load loops are hoisted out of at any size.
 * Expected: big_c: [2.0p1 2.0p1a 2.0p1b] != [2.5 2.6 2.7 2.0p1c 2.0p1d]; it
 *   needs BOTH R3 (the loads become invariant) and the veto move (the loop is
 *   big): a 2.0p1b+R3-only build stays in the first group.
 * big_sfield: stock 2.0p1 + the veto move alone == 2.5 (the cleanest
 *   provenance demo); the 2.0p1a family stays apart because 2.0p1a's own
 *   LICM hook never hoists a static read (an emulation clause, see the doc).
 * big_static: 2.5 also differs from 2.0p1+veto for an unrelated 2.5 change.
 * small_c / big_int: R3 only (no FP load, or under the threshold).
 * Scratch single-part builds: see docs/COMPILER_VARIANTS.md.             */
typedef struct { float x, y, z; } V;
typedef struct { float m[3][4]; } M;

/* big loop, const matrix: R3 makes the m-loads invariant, the veto decides */
void big_c(V *out, const V *in, int n, const M *mat)
{
    int i;
    for (i = 0; i < n; i++) {
        out[i].x = in[i].x * mat->m[0][0] + in[i].y * mat->m[1][0] + in[i].z * mat->m[2][0] + mat->m[0][3];
        out[i].y = in[i].x * mat->m[0][1] + in[i].y * mat->m[1][1] + in[i].z * mat->m[2][1] + mat->m[1][3];
        out[i].z = in[i].x * mat->m[0][2] + in[i].y * mat->m[1][2] + in[i].z * mat->m[2][2] + mat->m[2][3];
    }
}

/* the same with a non-const matrix: nothing is invariant (out may alias) */
void big_nc(V *out, const V *in, int n, M *mat)
{
    int i;
    for (i = 0; i < n; i++) {
        out[i].x = in[i].x * mat->m[0][0] + in[i].y * mat->m[1][0] + in[i].z * mat->m[2][0] + mat->m[0][3];
        out[i].y = in[i].x * mat->m[0][1] + in[i].y * mat->m[1][1] + in[i].z * mat->m[2][1] + mat->m[1][3];
        out[i].z = in[i].x * mat->m[0][2] + in[i].y * mat->m[1][2] + in[i].z * mat->m[2][2] + mat->m[2][3];
    }
}

/* big loop, no stores to memory the matrix could be: invariant on every
 * compiler, so only the veto decides (no R3 involved) */
float big_local(const V *in, int n, M *mat)
{
    int i;
    float s = 0.0f;
    for (i = 0; i < n; i++) {
        s += in[i].x * mat->m[0][0] + in[i].y * mat->m[1][0] + in[i].z * mat->m[2][0] + mat->m[0][3];
        s += in[i].x * mat->m[0][1] + in[i].y * mat->m[1][1] + in[i].z * mat->m[2][1] + mat->m[1][3];
        s += in[i].x * mat->m[0][2] + in[i].y * mat->m[1][2] + in[i].z * mat->m[2][2] + mat->m[2][3];
    }
    return s;
}

/* small loop: below the 25-instruction threshold */
float small_c(const float *in, int n, const M *mat)
{
    int i;
    float s = 0.0f;
    for (i = 0; i < n; i++)
        s += in[i] * mat->m[0][0];
    return s;
}

/* big loop with integer loads only: the veto never applied */
int big_int(const int *in, int n, const int *k)
{
    int i, s = 0;
    for (i = 0; i < n; i++) {
        s += in[i] * k[0] + (in[i] >> 1) * k[1] + (in[i] >> 2) * k[2] + (in[i] >> 3) * k[3];
        s ^= in[i] * k[4] + (in[i] >> 4) * k[5] + (in[i] >> 5) * k[6] + (in[i] >> 6) * k[7];
        s -= in[i] * k[8] + (in[i] >> 7) * k[9];
    }
    return s;
}

/* big FP loop reading a global scalar (a WHOLE static: stock 2.0p1 and 2.5
 * may hoist it, 2.0p1a's LICM hook never does) and a field of a global struct
 * (a SUBRANGE static: the hook does not apply) */
extern float gScale;
extern struct { float a, b; } gPair;
void big_static(V *out, const V *in, int n)
{
    int i;
    for (i = 0; i < n; i++) {
        out[i].x = in[i].x * gScale + in[i].y * gPair.b + in[i].z * 2.0f + 1.0f;
        out[i].y = in[i].y * gScale + in[i].z * gPair.b + in[i].x * 3.0f + 4.0f;
        out[i].z = in[i].z * gScale + in[i].x * gPair.b + in[i].y * 5.0f + 6.0f;
    }
}
float big_sfield(const V *in, int n)
{
    int i;
    float s = 0.0f;
    for (i = 0; i < n; i++) {
        s += in[i].x * gPair.a + in[i].y * gPair.b + in[i].z * 2.0f + 1.0f;
        s += in[i].y * gPair.a + in[i].z * gPair.b + in[i].x * 3.0f + 4.0f;
        s += in[i].z * gPair.a + in[i].x * gPair.b + in[i].y * 5.0f + 6.0f;
    }
    return s;
}
