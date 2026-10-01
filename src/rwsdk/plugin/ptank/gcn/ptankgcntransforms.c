#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

/* Constants used by the paired single particle transforms */
const RwReal _rwConstants[8] = {
    0.0f,
    3.14159265f,  /* pi */
    0.0f,
    -1.57079633f, /* -pi / 2 */
    1.0f,
    1.0f,
    0.4052847f,   /* ~4 / pi^2 */
    1.0f
};

const RwReal _rwNnumber1[2] = { 0.5f, -0.5f };

/* Write gather pipe */
const RwUInt32 _rwFifo = 0xCC008000;

const RwReal* _rwPNumber1 = _rwNnumber1;
const RwReal* _rwConst = _rwConstants;
