/* audit: clauses B / E3n / W / A against compiler temporaries (frame slots with no declared
 * Object or an '@' name: the int<->float conversion doubleword, struct-return temps).
 * Question: does 2.0p1f+ssi with the clause removed for the temp class equal stock 2.0p1/2.5?
 * Run: python tools/compilerprobe/audit/rrun.py tools/compilerprobe/audit/repros/a_tmp.c \
 *        --abl noBtmp='^B@.*:tmp:' --abl noTmp=':tmp:' --keys
 */
extern float gF, gG;
extern int gI, gJ;
extern unsigned char gB;

float st_then_i2f(int x, float y)        /* B@1: stfs gF | stw tmp (0x4330 slot) */
{
    gF = y;
    return (float)x;
}

float st_then_u2f(unsigned x, float y)
{
    gG = y;
    return (float)x + 1.0f;
}

int st_then_f2i(float x, int y)          /* fctiwz/stfd tmp; lwz */
{
    gI = y;
    return (int)x;
}

void two_conv(int a, int b)
{
    gF = (float)a;
    gG = (float)b;
}

void conv_mix(int a, float b)
{
    gI = a;
    gF = (float)a * b;
    gB = (unsigned char)(b * 255.0f);
}

float lit_after_conv(int a)              /* E3n/W vs a temp slot */
{
    float t = (float)a;
    return t * 0.5f + 2.0f;
}

void ctl_two_statics(float a, int b)     /* control: B on two declared statics stays */
{
    gF = a;
    gI = b;
}
