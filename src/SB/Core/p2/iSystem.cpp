#include "iSystem.h"

#include "iFile.h"
#include "iTime.h"

#include "xDebug.h"
#include "xFX.h"
#include "xMath.h"
#include "xMath3.h"
#include "xMemMgr.h"
#include "xPad.h"
#include "xShadow.h"
#include "xSnd.h"
#include "xString.h"
#include "xstransvc.h"
#include "xutil.h"

#include "zScene.h"

#include <rwcore.h>
#include <rpworld.h>
#include <rphanim.h>
#include <rpusrdat.h>
#include <rppds.h>

#include <eekernel.h>
#include <libcdvd.h>
#include <libgraph.h>
#include <libscf.h>
#include <sifdev.h>
#include <sifrpc.h>
#include <stdio.h>
#include <string.h>
#include <types.h>

// RenderWare plugins attached by the PS2 build (RpWorld, RpCollision, RpSkin,
// RpHAnim, RpMatFX, RpUserData, RpPTank and the PS2 ADC plugin).
extern "C" {
RwBool RpWorldPluginAttach(void);
RwBool RpCollisionPluginAttach(void);
RwBool RpSkinPluginAttach(void);
RwBool RpHAnimPluginAttach(void);
RwBool RpMatFXPluginAttach(void);
RwBool RpUserDataPluginAttach(void);
RwBool RpPTankPluginAttach(void);
RwBool RpADCPluginAttach(void);
}

void iVU0Reset();
void p2EnableFastIm3D();
void xMath3Exit();

S32 DVD;
RwVideoMode sVideoMode;
S32 gFB_Flags = rwVIDEOMODEEXCLUSIVE | rwVIDEOMODE_PS2_FSAAREADCIRCUIT;
U32 gVsyncCount;

static S32 vsyncCounterHandler(S32);
static RwTexture* TextureRead(const char* name, const char* maskName);

static U32 psSelectDevice()
{
    RwVideoMode videoMode;
    S32 i;
    S32 num;

    RpSkySelectDeepZBuffer(TRUE);

    num = RwEngineGetNumVideoModes();
    for (i = 0; i < num; i++)
    {
        RwEngineGetVideoModeInfo(&videoMode, i);
        if (videoMode.width == 640 && videoMode.height == 448 && videoMode.depth == 32 &&
            (gFB_Flags & videoMode.flags) == gFB_Flags)
        {
            RwEngineSetVideoMode(i);
            break;
        }
    }

    if (i == num)
    {
        return TRUE;
    }

    memcpy(&sVideoMode, &videoMode, sizeof(RwVideoMode));
    return FALSE;
}

void iSystemInit(U32 options)
{
    char* CDROM_IMAGE_FILE;
    char* HOSTIO_IMAGE_FILE;
    char* iopImageFile;
    S32 disk_type;

    if (options & 0x1)
    {
        printf("\n\niSystemInit - using host IO filesystem.\n\n");
    }
    else
    {
        printf("\n\niSystemInit - using CDROM/DVD filesystem\n\n");
    }

    sceSifInitRpc(0);

    if (options & 0x2)
    {
        CDROM_IMAGE_FILE = "cdrom0:\\MODULES\\IOPRP270.IMG;1";
        HOSTIO_IMAGE_FILE = "host0:/usr/local/sce/iop/modules/IOPRP270.IMG";
        iopImageFile = (options & 0x1) ? HOSTIO_IMAGE_FILE : CDROM_IMAGE_FILE;

        printf("Booting kernel image file '%s'\n", iopImageFile);
        while (!sceSifRebootIop(iopImageFile))
        {
        }
        while (!sceSifSyncIop())
        {
        }
        sceSifInitRpc(0);
    }

    while (sceSifInitIopHeap() < 0)
    {
        printf("IOP heap init failed\n");
    }

    if (options & 0x1)
    {
        DVD = 2;
    }
    else
    {
        sceCdInit(SCECdINIT);
        disk_type = sceCdGetDiskType();
        printf("%s media detected (0x%02X)\n", (DVD = disk_type == SCECdPS2DVD) ? "DVD" : "CD",
               disk_type);
        sceCdMmode(DVD ? SCECdDVD : SCECdCD);
    }

    sceFsReset();

    xDebugInit();
    xMemInit();
    iFileInit();
    iTimeInit();
    iVU0Reset();
    xPadInit();
    xSndInit();
    xMathInit();
    xMath3Init();
    xShadowInit();
    xFXInit();
}

void iSystemExit()
{
    xDebugExit();
    xMath3Exit();
    xMathExit();
    xSndExit();
    xPadKill();
    iFileExit();
    iTimeExit();
    xMemExit();
}

static U32 RWAttachPlugins()
{
    if (!RpPDSPluginAttach(43))
    {
        return TRUE;
    }

    rwPDS_G3x_Generic_AtmPipeRegister();
    rwPDS_G3x_ADL_MatPipeRegister();
    rwPDS_G3x_A4D_MatPipeRegister();
    rwPDS_G3_Generic_AtmPipeRegister();
    rwPDS_G3_Generic_MatPipeRegister();
    rwPDS_G3x_Skin_AtmPipeRegister();
    rwPDS_G3x_ADLSkin_MatPipeRegister();
    rwPDS_G3x_A4DSkin_MatPipeRegister();
    rwPDS_G3xd_ADL_MatPipeRegister();
    rwPDS_G3xd_A4D_MatPipeRegister();
    rwPDS_G3xd_ADLSkin_MatPipeRegister();
    rwPDS_G3xd_A4DSkin_MatPipeRegister();
    rwPDS_G3xd_ADLGem_MatPipeRegister();
    rwPDS_G3xd_A4DGem_MatPipeRegister();
    rwPDS_G3xd_ADLSkinGem_MatPipeRegister();
    rwPDS_G3xd_A4DSkinGem_MatPipeRegister();
    rwPDS_G3xd_ADLGem_MatPipeRegister();
    rwPDS_G3x_OPLClone_MatPipeRegister();
    rwPDS_G3x_OPLClone_AtmPipeRegister();
    rwPDS_G3_Generic_GrpMatPipeRegister();
    rwPDS_G3_Generic_GrpAtmPipeRegister();
    rwPDS_G3_Generic_GrpSctPipeRegister();
    rwPDS_G3_Im3D_TriPipeRegister();
    rwPDS_G3_Im3D_SegPipeRegister();
    rwPDS_G3_Im3D_TriObjPipeRegister();
    rwPDS_G3_Im3D_SegObjPipeRegister();
    rwPDS_G3_MatfxUV1_GrpMatPipeRegister();
    rwPDS_G3_MatfxUV2_GrpMatPipeRegister();
    rwPDS_G3_MatfxUV1_GrpAtmPipeRegister();
    rwPDS_G3_MatfxUV2_GrpAtmPipeRegister();
    rwPDS_G3_MatfxUV1_GrpSctPipeRegister();
    rwPDS_G3_MatfxUV2_GrpSctPipeRegister();
    rwPDS_G3_Skin_GrpMatPipeRegister();
    rwPDS_G3_Skin_GrpAtmPipeRegister();
    rwPDS_G3_SkinfxUV1_GrpMatPipeRegister();
    rwPDS_G3_SkinfxUV2_GrpMatPipeRegister();
    rwPDS_G3_SkinfxUV1_GrpAtmPipeRegister();
    rwPDS_G3_SkinfxUV2_GrpAtmPipeRegister();

    if (!RpWorldPluginAttach())
    {
        return TRUE;
    }
    if (!RpCollisionPluginAttach())
    {
        return TRUE;
    }
    if (!RpSkinPluginAttach())
    {
        return TRUE;
    }
    if (!RpHAnimPluginAttach())
    {
        return TRUE;
    }
    if (!RpMatFXPluginAttach())
    {
        return TRUE;
    }
    if (!RpUserDataPluginAttach())
    {
        return TRUE;
    }
    if (!RpPTankPluginAttach())
    {
        return TRUE;
    }

    if (!RpADCPluginAttach())
    {
        return TRUE;
    }

    return FALSE;
}

static S32 vsyncCounterHandler(S32)
{
    gVsyncCount++;
    return 0;
}

U32 iRenderWareInit()
{
    RwEngineOpenParams openParams;

    if (!RwEngineInit(NULL, rwENGINEINITNOFREELISTS, 0x3B0000))
    {
        return TRUE;
    }

    RwResourcesSetArenaSize(0x3B0000);

    if (!SkyInstallFileSystem(NULL))
    {
        return TRUE;
    }

    if (RWAttachPlugins())
    {
        return TRUE;
    }

    openParams.displayID = NULL;
    if (!RwEngineOpen(&openParams))
    {
        RwEngineTerm();
        return TRUE;
    }

    psSelectDevice();

    if (!RwEngineStart())
    {
        RwEngineClose();
        RwEngineTerm();
        return TRUE;
    }

    RwTextureSetReadCallBack(TextureRead);
    p2EnableFastIm3D();
    AddIntcHandler(INTC_VBLANK_S, vsyncCounterHandler, -1);

    return FALSE;
}

static RwTexture* TextureRead(const char* name, const char* maskName)
{
    char tmpname[256];
    RwTexture* result;
    U32 assetid;
    U32 tmpsize;

    sprintf(tmpname, "%s.rw3", name);
    assetid = xStrHash(tmpname);
    result = (RwTexture*)xSTFindAsset(assetid, &tmpsize);
    if (result != NULL)
    {
        if (RwRasterGetNumLevels(result->raster) > 1)
        {
            RwTextureSetFilterMode(result, rwFILTERMIPLINEAR);

            // This one texture in the Chum Bucket shows mip seams; keep it unfiltered.
            if (gTransitionSceneID == 'HB00' && !xStricmp(name, "surface_chum2"))
            {
                RwTextureSetFilterMode(result, rwFILTERLINEAR);
            }
        }

        strcpy(result->name, name);
        strcpy(result->mask, maskName);
        return result;
    }

    return NULL;
}

void iVSync()
{
    sceGsSyncV(0);
}

U8 iGetMinute()
{
    sceCdCLOCK clock;

    sceCdReadClock(&clock);
    sceScfGetLocalTimefromRTC(&clock);
    return BCDtoi(clock.minute);
}

U8 iGetHour()
{
    sceCdCLOCK clock;

    sceCdReadClock(&clock);
    sceScfGetLocalTimefromRTC(&clock);
    return BCDtoi(clock.hour);
}

U8 iGetDay()
{
    sceCdCLOCK clock;

    sceCdReadClock(&clock);
    sceScfGetLocalTimefromRTC(&clock);
    return BCDtoi(clock.day);
}

U8 iGetMonth()
{
    sceCdCLOCK clock;

    sceCdReadClock(&clock);
    sceScfGetLocalTimefromRTC(&clock);
    return BCDtoi(clock.month);
}

U32 iGetCurrFormattedDate(char* str)
{
    sceCdCLOCK clock;

    if (!sceCdReadClock(&clock))
    {
        sprintf(str, " ");
        return strlen(str);
    }

    sceScfGetLocalTimefromRTC(&clock);

    switch (sceScfGetDateNotation())
    {
    case SCE_DATE_YYYYMMDD:
        sprintf(str, "20%02x/%02x/%02x ", clock.year, clock.month, clock.day);
        break;
    case SCE_DATE_MMDDYYYY:
        sprintf(str, "%02x/%02x/20%02x ", clock.month, clock.day, clock.year);
        break;
    case SCE_DATE_DDMMYYYY:
        sprintf(str, "%02x/%02x/20%02x ", clock.day, clock.month, clock.year);
        break;
    }

    return strlen(str);
}

U32 iGetCurrFormattedTime(char* str)
{
    sceCdCLOCK clock;

    if (!sceCdReadClock(&clock))
    {
        sprintf(str, " ");
        return strlen(str);
    }

    sceScfGetLocalTimefromRTC(&clock);

    switch (sceScfGetTimeNotation())
    {
    case SCE_TIME_24HOUR:
        sprintf(str, "%02x:%02x:%02x", clock.hour, clock.minute, clock.second);
        break;
    case SCE_TIME_12HOUR:
        // The clock is BCD; 0x12 is noon.
        if (clock.hour < 0x12)
        {
            if (clock.hour == 0)
            {
                sprintf(str, "12:%02x:%02x AM", clock.minute, clock.second);
            }
            else
            {
                sprintf(str, "%02x:%02x:%02x AM", clock.hour, clock.minute, clock.second);
            }
        }
        else
        {
            sprintf(str, "%02d:%02x:%02x PM", (clock.hour / 16) * 10 + clock.hour % 16 - 12,
                    clock.minute, clock.second);
        }
        break;
    }

    return strlen(str);
}

void iLoadModule(const char* moduleName, const char* arguments)
{
    static char* PATHS_CDROM[] = { "cdrom0:/modules/", "cdrom0:/", NULL };
    static char* PATHS_HOST[] = { "host0:", "host0:/usr/local/sce/iop/modules/", NULL };

    char workingName[256];
    S32 j;
    S32 errorCode;

    for (char** path = (DVD == 2) ? PATHS_HOST : PATHS_CDROM; *path != NULL; path++)
    {
        strcpy(workingName, *path);
        strcat(workingName, moduleName);

        if (strncmp(workingName, "cdrom", 5) == 0)
        {
            // ISO 9660 names: version suffix, backslashes, upper case after "cdrom0:".
            strcat(workingName, ";1");
            for (j = 0; workingName[j] != '\0'; j++)
            {
                if (workingName[j] == '/')
                {
                    workingName[j] = '\\';
                }
                else if (workingName[j] >= 'a' && workingName[j] <= 'z' && j > 5)
                {
                    workingName[j] -= 'a' - 'A';
                }
            }
        }

        errorCode = sceSifLoadModule(workingName, arguments ? strlen(arguments) + 1 : 0, arguments);
        if (errorCode >= 0)
        {
            return;
        }
    }

    printf("unable to load module %s\n", moduleName);
}

void iSystem_GapTrackReport()
{
}
