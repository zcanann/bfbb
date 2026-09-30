#include <types.h>
#include <rwcore.h>
#include <string.h>
#include "xBase.h"
#include "xString.h"
#include "xstransvc.h"
#include "zFeet.h"
#include "zScene.h"
#include "zSurface.h"

// zSurfaceGetName() names surface types 0 through 22, so there are 23 of them.
// No header exposes a count for them; when zSurface.h grows one, use it here.
#define SURFACE_TYPE_COUNT 23

static U32 sSurfaceSoundIDStep[SURFACE_TYPE_COUNT];
xBase* paremit_sd_pawprint;
xBase* paremit_vil_footprint;
RwRaster* sSkidMarkRaster;

void zFeetGetIDs()
{
    char type_name[128];
    char name[128];

    for (S32 i = 0; i < SURFACE_TYPE_COUNT; i++)
    {
        zSurfaceGetName(i, type_name);
        strcpy(name, "SNDFX_STEP_");
        strcat(name, type_name);
        sSurfaceSoundIDStep[i] = xStrHash(name);
    }

    paremit_sd_pawprint = zSceneFindObject(xStrHash("PAREMIT_SD_PAWPRINT"));
    paremit_vil_footprint = zSceneFindObject(xStrHash("PAREMIT_VIL_FOOTPRINT"));

    RwTexture* tex = (RwTexture*)xSTFindAsset(xStrHash("SKIDMARKTEX"), NULL);

    if (tex != NULL)
    {
        sSkidMarkRaster = tex->raster;
    }
}
