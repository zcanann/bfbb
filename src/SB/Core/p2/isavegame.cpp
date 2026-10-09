#include "isavegame.h"

#include "zGlobals.h"

#include "xPad.h"
#include "xSnd.h"
#include "xutil.h"

#include "iPad.h"
#include "iSystem.h"

#include <libcdvd.h>
#include <libmc.h>
#include <libscf.h>
#include <sifdev.h>
#include <stdio.h>
#include <string.h>
#include <types.h>

// Memory card directory names begin with the region's product code.
#if defined(VERSION_SLES_51968)
#define ISG_PRODUCT_CODE "BESLES-51968"
#elif defined(VERSION_SLES_51970)
#define ISG_PRODUCT_CODE "BESLES-51970"
#elif defined(VERSION_SLES_53623)
#define ISG_PRODUCT_CODE "BESLES-53623"
#else
#define ISG_PRODUCT_CODE "BASLUS-20680"
#endif

enum en_ISG_IOMODE
{
    ISG_IOMODE_READ = 1,
    ISG_IOMODE_WRITE = 2,
    ISG_IOMODE_APPEND = 3
};

enum en_ISGMC_ERRSTATUS
{
    ISGMC_ERR_NONE,
    ISGMC_ERR_NOMEMCARD,
    ISGMC_ERR_MKDIR,
    ISGMC_ERR_OPEN,
    ISGMC_ERR_CLOSE,
    ISGMC_ERR_READ,
    ISGMC_ERR_WRITE
};

enum en_ISGMCA_STATUS
{
    ISG_MCA_STAT_DONE_ERR = -1,
    ISG_MCA_STAT_INPROG = 0,
    ISG_MCA_STAT_DONE = 1
};

enum en_MEMCARD_SEEKPT
{
    ISG_MCSEEK_TOP,
    ISG_MCSEEK_CUR,
    ISG_MCSEEK_END
};

struct st_ISG_MEMCARD_DATA
{
    S32 mcport;
    S32 mcslot;
    S32 mcfp;
    en_ISG_IOMODE fmode;
    char gamepath[64];
    sceMcTblGetDir finfo;
    S32 cur_mcop;
    en_ISGMC_ERRSTATUS mcerr;
    S32 allow_cache;
};

struct st_ISGSESSION
{
    st_ISG_MEMCARD_DATA* mcdata;
    char gameroot[64];
    char gamedir[64];
    en_ASYNC_OPCODE as_curop;
    en_ASYNC_OPSTAT as_opstat;
    en_ASYNC_OPERR as_operr;
    void* cltdata;
    en_CHGCODE chgcode;
    void (*chgfunc)(void*, en_CHGCODE);
};

static char* g_scoobydoo_icon_list;
S32 gIconSize;
static char* g_scoobydoo_icon_copy;
static char* g_scoobydoo_icon_delete;
static volatile S32 g_isginit;

static st_ISG_MEMCARD_DATA g_mcdata_MAIN;
static st_ISGSESSION g_isgdata_MAIN;
static st_ISG_MEMCARD_DATA g_mcdata_MONITOR;
static st_ISGSESSION g_isgdata_MONITOR;

static char* g_isg_scemodule[] = { "mcman.irx", "mcserv.irx", NULL };

// Credits padding written into the unused space of the config placeholder file.
static char* g_strz_egotrip[] = { "Shiraz Akmal............................",
                                  "Tim Doyle...............................",
                                  "Jason Hoerner...........................",
                                  "Neil Ka'apuni...........................",
                                  "Dan Kollmorgen..........................",
                                  "Chris Masterton.........................",
                                  "Ryan Mapes..............................",
                                  NULL };

S32 iSG_start_your_engines();
S32 iSG_mcidx_portslot(S32 mcidx, S32* port, S32* slot, S32* concnt);
S32 iSG_mc_exists(st_ISG_MEMCARD_DATA* mcdata, S32 mcidx);
S32 iSG_mc_isformatted(st_ISG_MEMCARD_DATA* mcdata, S32 mcidx);
S32 iSG_mc_isPSIIcard(st_ISG_MEMCARD_DATA* mcdata, S32 mcidx);
S32 iSG_mc_availclust(st_ISG_MEMCARD_DATA* mcdata, S32 mcidx);
S32 iSG_mc_availDirEnt(st_ISG_MEMCARD_DATA* mcdata, S32 mcidx, const char* dpath);
S32 iSG_isSpaceForFile(st_ISG_MEMCARD_DATA* mcdata, S32 mcidx, S32 fsize, const char* dpath,
                              const char* fname, S32* bytesNeeded, S32* availOnDisk);
S32 iSG_get_finfo(st_ISG_MEMCARD_DATA* mcdata, const char* fname, const char* path);
S32 iSG_get_fmoddate(st_ISG_MEMCARD_DATA* mcdata, const char* fname, S32* sec, S32* min,
                            S32* hr, S32* mon, S32* day, S32* yr);
S32 iSG_mca_fmt(st_ISG_MEMCARD_DATA* mcdata, S32 force);
S32 iSG_mca_unfmt(st_ISG_MEMCARD_DATA* mcdata);
S32 iSG_mca_fopen(st_ISG_MEMCARD_DATA* mcdata, const char* fname, en_ISG_IOMODE mode);
S32 iSG_mca_fread(st_ISG_MEMCARD_DATA* mcdata, char* buf, S32 bufsize);
S32 iSG_mca_fwrite(st_ISG_MEMCARD_DATA* mcdata, char* data, S32 n);
en_ISGMCA_STATUS iSG_mcasync_chkop(st_ISG_MEMCARD_DATA* mcdata, S32 block,
                                          S32* sync_resval);
S32 iSG_is_MCOP_realerr(S32 mcop, S32 que_rc);
S32 iSG_add_sysicons(st_ISG_MEMCARD_DATA* mcdata);
S32 iSG_add_cfgholder(st_ISG_MEMCARD_DATA* mcdata);
S32 iSG_is_synccode_realerr(S32 mcop, S32 mcopret, st_ISG_MEMCARD_DATA* mcdata);
void SQUIB_init_st_iconsys(sceMcIconSys* icsys);

// Queue a close of the open file; completion is collected by iSG_mcasync_chkop.
static S32 iSG_mca_fclose(st_ISG_MEMCARD_DATA* mcdata)
{
    S32 result = 1;
    S32 rc;

    mcdata->cur_mcop = sceMcFuncNoClose;
    rc = sceMcClose(mcdata->mcfp);
    if (!iSG_is_MCOP_realerr(mcdata->cur_mcop, rc))
    {
        result = 0;
    }

    return result;
}

static S32 iSG_mca_chdir(st_ISG_MEMCARD_DATA* mcdata, const char* dpath)
{
    S32 result = 1;
    S32 rc;

    mcdata->allow_cache = 0;
    mcdata->cur_mcop = sceMcFuncNoChDir;
    rc = sceMcChdir(mcdata->mcport, mcdata->mcslot, dpath, NULL);
    if (!iSG_is_MCOP_realerr(mcdata->cur_mcop, rc))
    {
        result = 0;
    }

    return result;
}

static S32 iSG_mca_mkdir(st_ISG_MEMCARD_DATA* mcdata, const char* dpath)
{
    S32 result = 1;
    S32 rc;

    mcdata->allow_cache = 0;
    mcdata->cur_mcop = sceMcFuncNoMkdir;
    rc = sceMcMkdir(mcdata->mcport, mcdata->mcslot, dpath);
    if (!iSG_is_MCOP_realerr(mcdata->cur_mcop, rc))
    {
        result = 0;
    }

    return result;
}

// Synchronous forms of the queued operations above: queue, then block until
// the memory card server reports completion.
static S32 iSG_mc_chdir(st_ISG_MEMCARD_DATA* mcdata, const char* dpath)
{
    S32 result = 1;

    if (iSG_mca_chdir(mcdata, dpath) && iSG_mcasync_chkop(mcdata, 1, NULL) != ISG_MCA_STAT_DONE)
    {
        result = 0;
    }

    return result;
}

static S32 iSG_mc_mkdir(st_ISG_MEMCARD_DATA* mcdata, const char* dpath)
{
    S32 result = 1;

    if (iSG_mca_mkdir(mcdata, dpath) && iSG_mcasync_chkop(mcdata, 1, NULL) != ISG_MCA_STAT_DONE)
    {
        result = 0;
    }

    return result;
}

static S32 iSG_mc_fopen(st_ISG_MEMCARD_DATA* mcdata, const char* fname, en_ISG_IOMODE mode)
{
    S32 result = 1;

    if (iSG_mca_fopen(mcdata, fname, mode) &&
        iSG_mcasync_chkop(mcdata, 1, NULL) != ISG_MCA_STAT_DONE)
    {
        result = 0;
    }

    return result;
}

static S32 iSG_mc_fclose(st_ISG_MEMCARD_DATA* mcdata)
{
    S32 result = iSG_mca_fclose(mcdata);

    if (result)
    {
        iSG_mcasync_chkop(mcdata, 1, NULL);
    }

    return result;
}

static S32 iSG_mc_fread(st_ISG_MEMCARD_DATA* mcdata, char* buf, S32 bufsize)
{
    S32 result = 1;

    if (iSG_mca_fread(mcdata, buf, bufsize) &&
        iSG_mcasync_chkop(mcdata, 1, NULL) != ISG_MCA_STAT_DONE)
    {
        result = 0;
    }

    return result;
}

static S32 iSG_mc_fwrite(st_ISG_MEMCARD_DATA* mcdata, char* data, S32 n)
{
    S32 result = 1;

    if (iSG_mca_fwrite(mcdata, data, n) && iSG_mcasync_chkop(mcdata, 1, NULL) != ISG_MCA_STAT_DONE)
    {
        result = 0;
    }

    return result;
}

static S32 iSG_mc_fmt(st_ISG_MEMCARD_DATA* mcdata, S32 force)
{
    S32 result = 1;

    if (iSG_mca_fmt(mcdata, force) && iSG_mcasync_chkop(mcdata, 1, NULL) != ISG_MCA_STAT_DONE)
    {
        result = 0;
    }

    return result;
}

static S32 iSG_mc_settgt(st_ISG_MEMCARD_DATA* mcdata, S32 mcidx)
{
    S32 result = 1;
    S32 port = 0;
    S32 slot = 0;

    mcdata->allow_cache = 0;
    if (!iSG_mcidx_portslot(mcidx, &port, &slot, NULL))
    {
        result = 0;
    }
    else
    {
        mcdata->mcport = port;
        mcdata->mcslot = slot;
    }

    return result;
}

static S32 iSG_get_fsize(st_ISG_MEMCARD_DATA* mcdata, const char* fname)
{
    return iSG_get_finfo(mcdata, fname, mcdata->gamepath) ? mcdata->finfo.FileSizeByte : -1;
}

// A file written to completion has its closed attribute set.
static S32 iSG_fileKosher(st_ISG_MEMCARD_DATA* mcdata, const char* fname)
{
    S32 result = 1;

    if (!iSG_get_finfo(mcdata, fname, mcdata->gamepath))
    {
        result = 0;
    }
    else if (!(mcdata->finfo.AttrFile & sceMcFileAttrClosed))
    {
        result = 0;
    }

    return result;
}

S32 iSGStartup()
{
    if (g_isginit++ == 0)
    {
        iSG_start_your_engines();
    }

    return g_isginit;
}

S32 iSGShutdown()
{
    g_isginit--;
    return g_isginit;
}

char* iSGMakeName(en_NAMEGEN_TYPE type, const char* base, S32 idx)
{
    static S32 rotate;
    static char rotatebuf[8][32];

    char* use_buf;
    const char* fmt_sb = "%s%08s";
    const char* fmt_sd = "%s%02d";
    const char* fmt_sbd = "%s%06s%02d";

    use_buf = rotatebuf[rotate++];
    if (rotate == 8)
    {
        rotate = 0;
    }

    *use_buf = '\0';

    switch (type)
    {
    case ISG_NGTYP_GAMEFILE:
        if (base != NULL)
        {
            sprintf(use_buf, fmt_sd, base, idx);
        }
        else
        {
            sprintf(use_buf, fmt_sd, "SpongeBob", idx);
        }
        break;
    case ISG_NGTYP_GAMEDIR:
    case ISG_NGTYP_CONFIG:
        if (base != NULL)
        {
            if (idx > 0)
            {
                sprintf(use_buf, fmt_sbd, ISG_PRODUCT_CODE, base, idx);
            }
            else
            {
                sprintf(use_buf, fmt_sb, ISG_PRODUCT_CODE, base);
            }
        }
        else if (idx > 0)
        {
            sprintf(use_buf, fmt_sbd, ISG_PRODUCT_CODE, "HIBob   ", idx);
        }
        else
        {
            sprintf(use_buf, fmt_sb, ISG_PRODUCT_CODE, "HIBob   ");
        }
        break;
    case ISG_NGTYP_ICONTHUM:
        if (idx == 0)
        {
            strncpy(use_buf, "icon.sys", 32);
            use_buf[31] = '\0';
        }
        else
        {
            if (base != NULL)
            {
                sprintf(use_buf, fmt_sd, base, idx);
                strcat(use_buf, ".ico");
            }
            else
            {
                sprintf(use_buf, fmt_sd, "SpongeIcon", idx);
                strcat(use_buf, ".ico");
            }
        }
        break;
    }

    return use_buf;
}

st_ISGSESSION* iSGSessionBegin(void* cltdata, void (*chgfunc)(void*, en_CHGCODE), S32 monitor)
{
    st_ISGSESSION* isgdata;

    if (monitor)
    {
        isgdata = &g_isgdata_MONITOR;
    }
    else
    {
        isgdata = &g_isgdata_MAIN;
    }

    memset(isgdata, 0, sizeof(st_ISGSESSION));

    if (monitor)
    {
        isgdata->mcdata = &g_mcdata_MONITOR;
    }
    else
    {
        isgdata->mcdata = &g_mcdata_MAIN;
    }

    isgdata->as_curop = ISG_OPER_NOOP;
    isgdata->as_opstat = ISG_OPSTAT_SUCCESS;
    isgdata->as_operr = ISG_OPERR_NONE;
    isgdata->cltdata = cltdata;
    isgdata->chgcode = ISG_CHG_NONE;
    isgdata->chgfunc = chgfunc;
    strcpy(isgdata->gameroot, "/");

    isgdata->mcdata->mcfp = -1;
    isgdata->mcdata->cur_mcop = 0;
    isgdata->mcdata->mcport = -1;
    isgdata->mcdata->mcslot = -1;
    isgdata->mcdata->mcerr = ISGMC_ERR_NONE;
    isgdata->mcdata->allow_cache = 0;

    return isgdata;
}

void iSGSessionEnd(st_ISGSESSION* isgdata)
{
    memset(isgdata, 0, sizeof(st_ISGSESSION));
}

S32 iSGTgtCount(st_ISGSESSION* isgdata, S32* max)
{
    S32 rc;
    S32 tgtmax;
    S32 concnt[2] = {};
    S32 dp;
    S32 ds;

    iSG_mcidx_portslot(0, &dp, &ds, concnt);

    rc = sceMcGetSlotMax(0);
    tgtmax = sceMcGetSlotMax(1);
    if (rc >= 0)
    {
        tgtmax += rc;
    }

    if (max != NULL)
    {
        *max = tgtmax;
    }

    return concnt[0] + concnt[1];
}

S32 iSGTgtPhysSlotIdx(st_ISGSESSION* isgdata, S32 tidx)
{
    S32 concnt[2] = {};
    S32 dp;
    S32 ds;

    iSG_mcidx_portslot(0, &dp, &ds, concnt);
    return dp;
}

S32 iSGTgtFormat(st_ISGSESSION* isgdata, S32 tgtidx, S32 async, S32* canRecover)
{
    S32 result = 1;
    S32 rc;

    iSG_mc_exists(isgdata->mcdata, tgtidx);
    iSG_mc_isPSIIcard(isgdata->mcdata, tgtidx);
    rc = iSG_mc_isformatted(isgdata->mcdata, tgtidx);

    if (!rc && !async)
    {
        if (!iSG_mc_fmt(isgdata->mcdata, 0))
        {
            result = 0;
        }
    }
    else if (!rc)
    {
        isgdata->as_curop = ISG_OPER_INIT;
        if (!iSG_mca_fmt(isgdata->mcdata, 0))
        {
            isgdata->as_opstat = ISG_OPSTAT_FAILURE;
            isgdata->as_operr = ISG_OPERR_INITFAIL;
            isgdata->chgcode = ISG_CHG_TARGET;
            result = 0;
        }
        else
        {
            isgdata->as_opstat = ISG_OPSTAT_INPROG;
            isgdata->as_operr = ISG_OPERR_NONE;
            isgdata->chgcode = ISG_CHG_NONE;
        }
    }

    return result;
}

U32 iSGTgtState(st_ISGSESSION* isgdata, S32 tgtidx, const char* dpath)
{
    U32 state = 0;
    S32 rc;

    if (!iSG_mc_exists(isgdata->mcdata, tgtidx))
    {
        return 0;
    }

    if (!iSG_mc_isPSIIcard(isgdata->mcdata, tgtidx))
    {
        return 1;
    }

    if (iSG_mc_isformatted(isgdata->mcdata, tgtidx))
    {
        state |= 7;
    }
    else
    {
        return 5;
    }

    if (dpath != NULL && iSG_mc_chdir(isgdata->mcdata, dpath))
    {
        state |= 8;
    }

    return state;
}

S32 iSGTgtSetActive(st_ISGSESSION* isgdata, S32 tgtidx)
{
    return iSG_mc_settgt(isgdata->mcdata, tgtidx);
}

S32 iSGTgtHaveRoom(st_ISGSESSION* isgdata, S32 tidx, S32 fsize, const char* dpath,
                   const char* fname, S32* bytesNeeded, S32* availOnDisk, S32* needFile)
{
    S32 result;
    S32 i;
    char* gameName;

    for (i = 0; i < 8; i++)
    {
        gameName = iSGMakeName(ISG_NGTYP_GAMEFILE, NULL, i);
        if (!iSGGameExists(isgdata, gameName))
        {
            continue;
        }

        iSG_isSpaceForFile(isgdata->mcdata, tidx, fsize, dpath, fname, bytesNeeded, availOnDisk);
        return 1;
    }

    result =
        iSG_isSpaceForFile(isgdata->mcdata, tidx, fsize, dpath, fname, bytesNeeded, availOnDisk);
    return result;
}

S32 iSGTgtHaveRoomStartup(st_ISGSESSION* isgdata, S32 tidx, S32 fsize, const char* dpath,
                          const char* fname, S32* bytesNeeded, S32* availOnDisk, S32* needFile)
{
    S32 result =
        iSG_isSpaceForFile(isgdata->mcdata, tidx, fsize, dpath, fname, bytesNeeded, availOnDisk);
    return result;
}

U8 iSGGameExists(st_ISGSESSION* isgdata, const char* fname)
{
    S32 rc;
    char str_buf[64] = {};
    S32 len;
    S32 numfound = 0;
    st_ISG_MEMCARD_DATA* mcdata = isgdata->mcdata;
    char* path = mcdata->gamepath;

    mcdata->cur_mcop = sceMcFuncNoGetDir;

    if (path != NULL && *path != '\0' && fname != NULL && *fname != '\0')
    {
        len = strlen(path);
        if (path[len - 1] == '\\' || path[len - 1] == '/')
        {
            sprintf(str_buf, "%s%s", path, fname);
        }
        else
        {
            sprintf(str_buf, "%s/%s", path, fname);
        }
    }
    else if (path != NULL && *path != '\0')
    {
        strcpy(str_buf, path);
    }
    else
    {
        strcpy(str_buf, fname);
    }

    rc = sceMcGetDir(mcdata->mcport, mcdata->mcslot, str_buf, 0, 1, &mcdata->finfo);
    if (iSG_is_MCOP_realerr(mcdata->cur_mcop, rc) &&
        iSG_mcasync_chkop(mcdata, 1, &numfound) == ISG_MCA_STAT_DONE && numfound)
    {
        return 1;
    }

    return 0;
}

S32 iSGFileSize(st_ISGSESSION* isgdata, const char* fname)
{
    S32 size = iSG_get_fsize(isgdata->mcdata, fname);

    if (size < 0)
    {
        size = 0;
    }

    return size;
}

char* iSGFileModDate(st_ISGSESSION* isgdata, const char* fname)
{
    char* date_str = iSGFileModDate(isgdata, fname, NULL, NULL, NULL, NULL, NULL, NULL);
    return date_str;
}

char* iSGFileModDate(st_ISGSESSION* isgdata, const char* fname, S32* sec, S32* min, S32* hr,
                     S32* mon, S32* day, S32* yr)
{
    static char datestr[64];

    S32 rc;
    sceMcTblGetDir* finf = &isgdata->mcdata->finfo;
    sceCdCLOCK clock;

    datestr[0] = '\0';

    rc = iSG_get_fmoddate(isgdata->mcdata, fname, sec, min, hr, mon, day, yr);
    if (!rc)
    {
        return NULL;
    }

    // Memory card timestamps are kept in Japan time; show them in local time.
    clock.second = itoBCD(finf->_Modify.Sec);
    clock.minute = itoBCD(finf->_Modify.Min);
    clock.hour = itoBCD(finf->_Modify.Hour);
    clock.day = itoBCD(finf->_Modify.Day);
    clock.month = itoBCD(finf->_Modify.Month);
    clock.year = itoBCD(finf->_Modify.Year);

    sceScfGetLocalTimefromRTC(&clock);

    finf->_Modify.Sec = BCDtoi(clock.second);
    finf->_Modify.Min = BCDtoi(clock.minute);
    finf->_Modify.Hour = BCDtoi(clock.hour);
    finf->_Modify.Day = BCDtoi(clock.day);
    finf->_Modify.Month = BCDtoi(clock.month);
    finf->_Modify.Year = BCDtoi(clock.year);

#if defined(VERSION_SLES_51970)
    sprintf(datestr, "%02d/%02d/%04d %02d:%02d:%02d", finf->_Modify.Day, finf->_Modify.Month,
            finf->_Modify.Year, finf->_Modify.Hour, finf->_Modify.Min, finf->_Modify.Sec);
#else
    sprintf(datestr, "%02d/%02d/%04d %02d:%02d:%02d", finf->_Modify.Month, finf->_Modify.Day,
            finf->_Modify.Year, finf->_Modify.Hour, finf->_Modify.Min, finf->_Modify.Sec);
#endif

    return datestr;
}

S32 iSGSelectGameDir(st_ISGSESSION* isgdata, const char* dname)
{
    if (dname[0] == '/')
    {
        strncpy(isgdata->mcdata->gamepath, dname, sizeof(isgdata->mcdata->gamepath));
    }
    else
    {
        sprintf(isgdata->mcdata->gamepath, "/%s", dname);
    }

    isgdata->mcdata->gamepath[sizeof(isgdata->mcdata->gamepath) - 1] = '\0';

    return iSG_mc_chdir(isgdata->mcdata, isgdata->mcdata->gamepath);
}

S32 iSGSetupGameDir(st_ISGSESSION* isgdata, const char* dname, S32 force_iconfix)
{
    S32 result = 1;
    S32 rc;
    st_ISG_MEMCARD_DATA* mcdata = isgdata->mcdata;
    S32 dir_isnew = 0;
    char* strptr;

    strcpy(isgdata->gamedir, dname);
    sprintf(mcdata->gamepath, "%s%s", isgdata->gameroot, isgdata->gamedir);

    if (!iSG_get_finfo(mcdata, NULL, mcdata->gamepath))
    {
        dir_isnew = 1;
        if (!iSG_mc_chdir(mcdata, mcdata->gamepath))
        {
            dir_isnew = 1;
        }
    }

    if (dir_isnew)
    {
        if (!iSG_mc_mkdir(mcdata, mcdata->gamepath))
        {
            result = 0;
        }
    }

    if (result)
    {
        if (!iSG_mc_chdir(mcdata, mcdata->gamepath))
        {
            result = 0;
        }
    }

    if (result && !dir_isnew)
    {
        rc = iSG_get_finfo(mcdata, iSGMakeName(ISG_NGTYP_ICONTHUM, NULL, 0), mcdata->gamepath);
        rc += iSG_get_finfo(mcdata, iSGMakeName(ISG_NGTYP_ICONTHUM, NULL, 1), mcdata->gamepath);
        rc += iSG_get_finfo(mcdata, iSGMakeName(ISG_NGTYP_CONFIG, NULL, 0), mcdata->gamepath);
        if (rc < 3)
        {
            if (force_iconfix)
            {
                dir_isnew = 1;
            }
            else
            {
                result = 0;
            }
        }
    }

    if (force_iconfix)
    {
        dir_isnew = 1;
    }

    if (result && dir_isnew)
    {
        strptr = iSGMakeName(ISG_NGTYP_CONFIG, NULL, 0);
        if (!iSG_get_finfo(mcdata, strptr, mcdata->gamepath) && !iSG_add_cfgholder(mcdata))
        {
            result = 0;
        }

        if (!iSG_add_sysicons(mcdata))
        {
            result = 0;
        }
    }

    return result;
}

S32 iSGSaveFile(st_ISGSESSION* isgdata, const char* fname, char* data, S32 n, S32 async, char*)
{
    S32 result = 1;
    S32 rc;

    if (!iSG_mc_chdir(isgdata->mcdata, isgdata->mcdata->gamepath))
    {
        isgdata->as_curop = ISG_OPER_NOOP;
        isgdata->as_opstat = ISG_OPSTAT_FAILURE;
        isgdata->as_operr = ISG_OPERR_GAMEDIR;
        isgdata->chgcode = ISG_CHG_TARGET;
        result = 0;
    }

    if (!result)
    {
        return result;
    }

    isgdata->as_curop = ISG_OPER_SAVE;
    isgdata->as_opstat = ISG_OPSTAT_INPROG;
    isgdata->as_operr = ISG_OPERR_NONE;
    isgdata->chgcode = ISG_CHG_NONE;

    if (!iSG_mc_fopen(isgdata->mcdata, fname, ISG_IOMODE_WRITE))
    {
        isgdata->as_opstat = ISG_OPSTAT_FAILURE;
        isgdata->as_operr = ISG_OPERR_SVOPEN;
        isgdata->chgcode = ISG_CHG_TARGET;
        result = 0;
    }
    else if (async)
    {
        if (!iSG_mca_fwrite(isgdata->mcdata, data, n))
        {
            isgdata->as_opstat = ISG_OPSTAT_FAILURE;
            isgdata->as_operr = ISG_OPERR_SVINIT;
            isgdata->chgcode = ISG_CHG_GAMELIST;
            result = 0;
            iSG_mc_fclose(isgdata->mcdata);
        }
    }
    else if (iSG_mc_fwrite(isgdata->mcdata, data, n))
    {
        iSG_mc_fclose(isgdata->mcdata);
        if (!iSG_fileKosher(isgdata->mcdata, fname))
        {
            result = 0;
        }
    }
    else
    {
        isgdata->as_opstat = ISG_OPSTAT_FAILURE;
        isgdata->as_operr = ISG_OPERR_SVWRITE;
        isgdata->chgcode = ISG_CHG_GAMELIST;
        result = 0;
    }

    return result;
}

S32 iSGLoadFile(st_ISGSESSION* isgdata, const char* fname, char* databuf, S32 async)
{
    iSGReadLeader(isgdata, fname, databuf, iSGFileSize(isgdata, fname), async);
    return 1;
}

S32 iSGReadLeader(st_ISGSESSION* isgdata, const char* fname, char* databuf, S32 numbytes,
                  S32 async)
{
    S32 result = 1;
    S32 rc;

    if (!iSG_mc_chdir(isgdata->mcdata, isgdata->mcdata->gamepath))
    {
        isgdata->as_curop = ISG_OPER_NOOP;
        isgdata->as_opstat = ISG_OPSTAT_FAILURE;
        isgdata->as_operr = ISG_OPERR_GAMEDIR;
        isgdata->chgcode = ISG_CHG_TARGET;
        result = 0;
    }

    if (!result)
    {
        return result;
    }

    isgdata->as_curop = ISG_OPER_LOAD;
    isgdata->as_opstat = ISG_OPSTAT_INPROG;
    isgdata->as_operr = ISG_OPERR_NONE;
    isgdata->chgcode = ISG_CHG_NONE;

    if (!iSG_mc_fopen(isgdata->mcdata, fname, ISG_IOMODE_READ))
    {
        isgdata->as_opstat = ISG_OPSTAT_FAILURE;
        isgdata->as_operr = ISG_OPERR_LDOPEN;
        isgdata->chgcode = ISG_CHG_GAMELIST;
        result = 0;
    }
    else if (async)
    {
        if (!iSG_mca_fread(isgdata->mcdata, databuf, numbytes))
        {
            isgdata->as_opstat = ISG_OPSTAT_FAILURE;
            isgdata->as_operr = ISG_OPERR_LDINIT;
            isgdata->chgcode = ISG_CHG_GAMELIST;
            result = 0;
            iSG_mc_fclose(isgdata->mcdata);
        }
    }
    else
    {
        if (!iSG_mc_fread(isgdata->mcdata, databuf, numbytes))
        {
            isgdata->as_opstat = ISG_OPSTAT_FAILURE;
            isgdata->as_operr = ISG_OPERR_LDREAD;
            isgdata->chgcode = ISG_CHG_GAMELIST;
            result = 0;
        }

        iSG_mc_fclose(isgdata->mcdata);
    }

    return result;
}

en_ASYNC_OPSTAT iSGPollStatus(st_ISGSESSION* isgdata, en_ASYNC_OPCODE* curop, S32 block)
{
    S32 rc;
    S32 sceResultCode;

    if (curop != NULL)
    {
        *curop = isgdata->as_curop;
    }

    if (isgdata->as_curop == ISG_OPER_NOOP)
    {
        isgdata->as_opstat = ISG_OPSTAT_FAILURE;
        isgdata->as_operr = ISG_OPERR_NOOPER;
        isgdata->chgcode = ISG_CHG_TARGET;
        return isgdata->as_opstat;
    }

    rc = iSG_mcasync_chkop(isgdata->mcdata, block, &sceResultCode);
    if (rc == ISG_MCA_STAT_INPROG)
    {
        isgdata->as_opstat = ISG_OPSTAT_INPROG;
        isgdata->as_operr = ISG_OPERR_NONE;
        isgdata->chgcode = ISG_CHG_NONE;
    }
    else if (rc == ISG_MCA_STAT_DONE_ERR)
    {
        isgdata->as_opstat = ISG_OPSTAT_FAILURE;
        switch (isgdata->mcdata->mcerr)
        {
        case ISGMC_ERR_NOMEMCARD:
            isgdata->as_operr = ISG_OPERR_TGTERR;
            isgdata->chgcode = ISG_CHG_TARGET;
            break;
        case ISGMC_ERR_READ:
            isgdata->as_operr = ISG_OPERR_LDREAD;
            isgdata->chgcode = ISG_CHG_GAMELIST;
            break;
        case ISGMC_ERR_WRITE:
            isgdata->as_operr = ISG_OPERR_SVWRITE;
            isgdata->chgcode = ISG_CHG_GAMELIST;
            break;
        case ISGMC_ERR_NONE:
        case ISGMC_ERR_MKDIR:
        case ISGMC_ERR_OPEN:
        case ISGMC_ERR_CLOSE:
            if (sceResultCode == -3)
            {
                isgdata->as_operr = ISG_OPERR_NOROOM;
                isgdata->chgcode = ISG_CHG_GAMELIST;
            }
            break;
        default:
            isgdata->chgcode = ISG_CHG_TARGET;
            break;
        }
    }
    else
    {
        isgdata->as_opstat = ISG_OPSTAT_SUCCESS;
        isgdata->as_operr = ISG_OPERR_NONE;
        isgdata->chgcode = ISG_CHG_NONE;
        iSG_mc_fclose(isgdata->mcdata);
    }

    return isgdata->as_opstat;
}

en_ASYNC_OPERR iSGOpError(st_ISGSESSION* isgdata, char* errmsg)
{
    if (errmsg == NULL)
    {
        return isgdata->as_operr;
    }

    switch (isgdata->as_operr)
    {
    case ISG_OPERR_NONE:
        strncpy(errmsg, "No current error", 128);
        break;
    case ISG_OPERR_NOOPER:
        strncpy(errmsg, "No operation in async queue", 128);
        break;
    case ISG_OPERR_MULTIOPER:
        strncpy(errmsg, "Too many async ops queued simultaneously", 128);
        break;
    case ISG_OPERR_INITFAIL:
        strncpy(errmsg, "Init Failed", 128);
        break;
    case ISG_OPERR_GAMEDIR:
        strncpy(errmsg, "Unable to access Save Game Directory", 128);
        break;
    case ISG_OPERR_SVINIT:
        strncpy(errmsg, "Save Error - during initalization (async queue)", 128);
        break;
    case ISG_OPERR_SVWRITE:
        strncpy(errmsg, "Save Error - during write", 128);
        break;
    case ISG_OPERR_SVOPEN:
        strncpy(errmsg, "Save Error - opening file", 128);
        break;
    case ISG_OPERR_LDINIT:
        strncpy(errmsg, "Load Error - during initalization (async queue)", 128);
        break;
    case ISG_OPERR_LDREAD:
        strncpy(errmsg, "Load Error - during read", 128);
        break;
    case ISG_OPERR_LDOPEN:
        strncpy(errmsg, "Load Error - opening file", 128);
        break;
    case ISG_OPERR_TGTERR:
        strncpy(errmsg, "Target problem (general error)", 128);
        break;
    case ISG_OPERR_TGTREM:
        strncpy(errmsg, "Target Error - media removed or changed", 128);
        break;
    case ISG_OPERR_TGTPREP:
        strncpy(errmsg, "Target Error - Not ready for I/O (unformatted?)", 128);
        break;
    default:
        sprintf(errmsg, "%s (%d)", "Operation encountered unknown error", isgdata->as_operr);
        break;
    }

    errmsg[127] = '\0';
    return isgdata->as_operr;
}

void iSGAutoSave_Startup()
{
}

st_ISGSESSION* iSGAutoSave_Connect(S32 idx_target, void* cltdata, void (*chg)(void*, en_CHGCODE))
{
    st_ISGSESSION* isg = iSGSessionBegin(cltdata, chg, 1);

    if (isg == NULL)
    {
        return isg;
    }

    if (!iSGTgtSetActive(isg, idx_target))
    {
        iSGSessionEnd(isg);
        isg = NULL;
    }

    return isg;
}

void iSGAutoSave_Disconnect(st_ISGSESSION* isg)
{
    iSGSessionEnd(isg);
}

S32 iSGAutoSave_Monitor(st_ISGSESSION* isg, S32 idx_target)
{
    U32 stat;

    if (isg == NULL)
    {
        return 0;
    }

    stat = iSGTgtState(isg, idx_target, NULL);
    if (stat == 0 || (stat & 1) == 0)
    {
        if (isg->chgfunc != NULL)
        {
            globals.autoSaveFeature = 0;
            isg->chgfunc(isg->cltdata, ISG_CHG_TARGET);
        }
        return 0;
    }

    return 1;
}

S32 iSG_start_your_engines()
{
    S32 result = 1;
    S32 rc;
    S32 i;

    for (i = 0; g_isg_scemodule[i] != NULL; i++)
    {
        iLoadModule(g_isg_scemodule[i], NULL);
    }

    rc = sceMcInit();
    switch (rc)
    {
    case sceMcIniSucceed:
        break;
    case sceMcIniErrKernel:
        result = 0;
        break;
    case sceMcIniOldMcman:
        result = 0;
        break;
    default:
        result = 0;
        break;
    }

    return result;
}

S32 iSG_add_cfgholder(st_ISG_MEMCARD_DATA* mcdata)
{
    char cfgdata[992] = {};
    char* strptr;
    char* cfgname;
    S32 i;

    memset(cfgdata, 0xBF, sizeof(cfgdata));

    strptr = cfgdata;
    for (i = 0; g_strz_egotrip[i] != NULL; i++)
    {
        memcpy(strptr, g_strz_egotrip[i], 32);
        strptr += 32;
    }

    cfgname = iSGMakeName(ISG_NGTYP_CONFIG, NULL, 0);
    if (iSG_mc_fopen(mcdata, cfgname, ISG_IOMODE_WRITE))
    {
        iSG_mc_fwrite(mcdata, cfgdata, sizeof(cfgdata));
        iSG_mc_fclose(mcdata);
    }

    return 1;
}

S32 iSG_add_sysicons(st_ISG_MEMCARD_DATA* mcdata)
{
    sceMcIconSys icsysdata;
    char* iconname;

    SQUIB_init_st_iconsys(&icsysdata);

    iconname = iSGMakeName(ISG_NGTYP_ICONTHUM, NULL, 0);
    if (iSG_mc_fopen(mcdata, iconname, ISG_IOMODE_WRITE))
    {
        memcpy(icsysdata.FnameCopy, icsysdata.FnameView, sizeof(icsysdata.FnameCopy));
        memcpy(icsysdata.FnameDel, icsysdata.FnameView, sizeof(icsysdata.FnameDel));
        iSG_mc_fwrite(mcdata, (char*)&icsysdata, sizeof(sceMcIconSys));
        iSG_mc_fclose(mcdata);
    }

    iconname = iSGMakeName(ISG_NGTYP_ICONTHUM, NULL, 1);
    if (iSG_mc_fopen(mcdata, iconname, ISG_IOMODE_WRITE))
    {
        iSG_mc_fwrite(mcdata, g_scoobydoo_icon_list, gIconSize);
        iSG_mc_fclose(mcdata);
    }

    return 1;
}

void SQUIB_init_st_iconsys(sceMcIconSys* icsys)
{
    S32 bgcolor[4][4] = {
        { 65, 128, 115, 0 },
        { 65, 128, 115, 0 },
        { 10, 79, 110, 0 },
        { 10, 79, 110, 0 },
    };
    F32 lightdir[3][4] = {
        { 1.0f, 0.0f, 1.0f, 0.0f },
        { -1.0f, 0.0f, 1.0f, 0.0f },
        { 0.0f, 2.0f, 0.0f, 0.0f },
    };
    F32 lightcol[3][4] = {
        { 0.3f, 0.3f, 0.3f, 0.0f },
        { 0.3f, 0.3f, 0.3f, 0.0f },
        { 0.4f, 0.4f, 0.4f, 0.0f },
    };
    F32 ambient[4] = { 0.35f, 0.35f, 0.35f, 0.0f };
    char* iconname;
    U8 sjistitle[68] = {};

    memset(icsys, 0, sizeof(sceMcIconSys));

    icsys->Head[0] = 'P';
    icsys->Head[1] = 'S';
    icsys->Head[2] = '2';
    icsys->Head[3] = 'D';
    icsys->OffsLF = 30;

    memset(sjistitle, 0, sizeof(sjistitle));
    strtosjis((U8*)"SB - Battle For Bikini Bottom", sjistitle);
    strncpy((char*)icsys->TitleName, (char*)sjistitle, sizeof(icsys->TitleName));

    iconname = iSGMakeName(ISG_NGTYP_ICONTHUM, NULL, 1);
    strcpy((char*)icsys->FnameView, iconname);
    iconname = iSGMakeName(ISG_NGTYP_ICONTHUM, NULL, 2);
    strcpy((char*)icsys->FnameCopy, iconname);
    iconname = iSGMakeName(ISG_NGTYP_ICONTHUM, NULL, 3);
    strcpy((char*)icsys->FnameDel, iconname);

    icsys->TransRate = 0x70;
    memcpy(icsys->BgColor, bgcolor, sizeof(bgcolor));
    memcpy(icsys->LightDir, lightdir, sizeof(lightdir));
    memcpy(icsys->LightColor, lightcol, sizeof(lightcol));
    memcpy(icsys->Ambient, ambient, sizeof(ambient));
}

S32 iSG_mcidx_portslot(S32 mcidx, S32* port, S32* slot, S32* concnt)
{
    S32 result = 1;
    S32 rc;
    S32 ret = 0;
    S32 i;
    S32 type = 0;
    S32 tp;
    S32 con_p0 = 0;
    S32 con_p1 = 0;
    S32 use_port = -1;
    S32 cur_mcop = sceMcFuncNoCardInfo;

    sceMcGetSlotMax(0);
    sceMcGetSlotMax(1);

    for (i = 0; i < 2; i++)
    {
        tp = (i != 0);
        ret = 0;
        rc = sceMcGetInfo(tp, 0, &type, NULL, NULL);
        if (iSG_is_MCOP_realerr(cur_mcop, rc))
        {
            rc = sceMcSync(0, &cur_mcop, &ret);
            if (rc > 0)
            {
                if (type == sceMcTypePS2)
                {
                    switch (ret)
                    {
                    case 0:
                    case -1:
                    case -2:
                        if (mcidx == con_p0 + con_p1)
                        {
                            use_port = tp;
                        }

                        if (tp == 0)
                        {
                            con_p0++;
                        }
                        else
                        {
                            con_p1++;
                        }
                        break;
                    }
                }

                if (use_port > -1 && concnt == NULL)
                {
                    break;
                }
            }
        }
    }

    if (use_port > -1)
    {
        if (concnt != NULL)
        {
            concnt[0] = con_p0;
            concnt[1] = con_p1;
        }
        *port = use_port;
        *slot = 0;
    }
    else
    {
        result = 0;
    }

    return result;
}

S32 iSG_mc_exists(st_ISG_MEMCARD_DATA* mcdata, S32 mcidx)
{
    S32 result = 1;
    S32 rc;

    mcdata->cur_mcop = sceMcFuncNoCardInfo;
    iSG_mcidx_portslot(mcidx, &mcdata->mcport, &mcdata->mcslot, NULL);
    rc = sceMcGetInfo(mcdata->mcport, mcdata->mcslot, NULL, NULL, NULL);
    if (!iSG_is_MCOP_realerr(mcdata->cur_mcop, rc))
    {
        result = 0;
    }

    if (result && iSG_mcasync_chkop(mcdata, 1, NULL) != ISG_MCA_STAT_DONE)
    {
        result = 0;
    }

    return result;
}

S32 iSG_mc_isformatted(st_ISG_MEMCARD_DATA* mcdata, S32 mcidx)
{
    S32 result = 1;
    S32 rc;
    S32 is_fmtd = 0;

    mcdata->cur_mcop = sceMcFuncNoCardInfo;
    iSG_mcidx_portslot(mcidx, &mcdata->mcport, &mcdata->mcslot, NULL);
    rc = sceMcGetInfo(mcdata->mcport, mcdata->mcslot, NULL, NULL, &is_fmtd);
    if (!iSG_is_MCOP_realerr(mcdata->cur_mcop, rc))
    {
        result = 0;
    }

    if (result && !iSG_mcasync_chkop(mcdata, 1, NULL))
    {
        result = 0;
    }

    if (result == 0)
    {
        return 0;
    }

    return is_fmtd;
}

S32 iSG_mc_isPSIIcard(st_ISG_MEMCARD_DATA* mcdata, S32 mcidx)
{
    S32 result = 1;
    S32 rc;
    S32 type = 0;

    mcdata->cur_mcop = sceMcFuncNoCardInfo;
    iSG_mcidx_portslot(mcidx, &mcdata->mcport, &mcdata->mcslot, NULL);
    rc = sceMcGetInfo(mcdata->mcport, mcdata->mcslot, &type, NULL, NULL);
    if (!iSG_is_MCOP_realerr(mcdata->cur_mcop, rc))
    {
        result = 0;
    }

    if (result)
    {
        if (iSG_mcasync_chkop(mcdata, 1, NULL) != ISG_MCA_STAT_DONE)
        {
            result = 0;
        }
        else if (type != sceMcTypePS2)
        {
            result = 0;
        }
    }

    return result;
}

#pragma dont_inline on
S32 iSG_mc_availclust(st_ISG_MEMCARD_DATA* mcdata, S32 mcidx)
{
    S32 result = 1;
    S32 rc;
    S32 clust = 0;

    mcdata->cur_mcop = sceMcFuncNoCardInfo;
    iSG_mcidx_portslot(mcidx, &mcdata->mcport, &mcdata->mcslot, NULL);
    rc = sceMcGetInfo(mcdata->mcport, mcdata->mcslot, NULL, &clust, NULL);
    if (!iSG_is_MCOP_realerr(mcdata->cur_mcop, rc))
    {
        result = 0;
    }

    if (result > 0 && !iSG_mcasync_chkop(mcdata, 1, NULL))
    {
        result = 0;
    }

    return result ? clust : -1;
}

S32 iSG_mc_availDirEnt(st_ISG_MEMCARD_DATA* mcdata, S32 mcidx, const char* dpath)
{
    S32 result = 1;
    S32 rc;

    mcdata->cur_mcop = sceMcFuncNoEntSpace;
    iSG_mcidx_portslot(mcidx, &mcdata->mcport, &mcdata->mcslot, NULL);
    rc = sceMcGetEntSpace(mcdata->mcport, mcdata->mcslot, dpath);
    if (!iSG_is_MCOP_realerr(mcdata->cur_mcop, rc))
    {
        result = 0;
    }

    if (result > 0 && !iSG_mcasync_chkop(mcdata, 1, NULL))
    {
        result = 0;
    }

    return result ? 0 : -1;
}

#pragma dont_inline reset

S32 iSG_isSpaceForFile(st_ISG_MEMCARD_DATA* mcdata, S32 mcidx, S32 fsize, const char* dpath,
                              const char* fname, S32* bytesNeeded, S32* availOnDisk)
{
    S32 rc;
    S32 fc_need;
    S32 fEc_need;
    S32 xtra_fent;
    S32 estclust;
    S32 totclust;
    S32 reset_mcpath = 0;

    if (dpath != NULL && mcdata->gamepath[0] == '\0')
    {
        reset_mcpath = 1;
        strcpy(mcdata->gamepath, dpath);
    }

    iSG_mcidx_portslot(mcidx, &mcdata->mcport, &mcdata->mcslot, NULL);

    if (!iSG_get_finfo(mcdata, NULL, mcdata->gamepath))
    {
        // No game directory yet: the directory, icons and config need room too.
        estclust = 0x91;
    }
    else
    {
        fEc_need = !iSG_mc_availDirEnt(mcdata, mcidx, mcdata->gamepath);
        fc_need = (fsize + 1023) / 1024;

        xtra_fent = iSG_get_fsize(mcdata, fname);
        if (xtra_fent > 0)
        {
            if (xtra_fent >= fsize)
            {
                fc_need = 0;
            }
            else
            {
                fc_need -= (xtra_fent + 1023) / 1024;
            }
        }

        estclust = fc_need + fEc_need;
    }

    totclust = iSG_mc_availclust(mcdata, mcidx);

    if (reset_mcpath)
    {
        mcdata->gamepath[0] = '\0';
    }

    if (bytesNeeded != NULL)
    {
        *bytesNeeded = estclust;
    }

    if (availOnDisk != NULL)
    {
        *availOnDisk = totclust;
    }

    return totclust >= estclust;
}

S32 iSG_get_finfo(st_ISG_MEMCARD_DATA* mcdata, const char* fname, const char* path)
{
    S32 result = 1;
    S32 rc;
    char str_buf[64] = {};
    S32 len;
    S32 numfound = 0;

    if (mcdata->allow_cache && !(mcdata->finfo.AttrFile & 0x7FFFFFF) &&
        !strcmp((char*)mcdata->finfo.EntryName, fname))
    {
        return 1;
    }

    mcdata->cur_mcop = sceMcFuncNoGetDir;

    if (path != NULL && *path != '\0' && fname != NULL && *fname != '\0')
    {
        len = strlen(path);
        if (path[len - 1] == '\\' || path[len - 1] == '/')
        {
            sprintf(str_buf, "%s%s", path, fname);
        }
        else
        {
            sprintf(str_buf, "%s/%s", path, fname);
        }
    }
    else if (path != NULL && *path != '\0')
    {
        strcpy(str_buf, path);
    }
    else
    {
        strcpy(str_buf, fname);
    }

    rc = sceMcGetDir(mcdata->mcport, mcdata->mcslot, str_buf, 0, 1, &mcdata->finfo);
    if (!iSG_is_MCOP_realerr(mcdata->cur_mcop, rc))
    {
        result = 0;
    }
    else if (iSG_mcasync_chkop(mcdata, 1, &numfound) != ISG_MCA_STAT_DONE)
    {
        result = 0;
    }
    else if (!numfound)
    {
        result = 0;
    }

    if (result && (mcdata->finfo.AttrFile & sceMcFileAttrSubdir))
    {
        mcdata->allow_cache = 1;
    }
    else
    {
        mcdata->allow_cache = 0;
    }

    return result;
}

S32 iSG_get_fmoddate(st_ISG_MEMCARD_DATA* mcdata, const char* fname, S32* sec, S32* min,
                            S32* hr, S32* mon, S32* day, S32* yr)
{
    S32 result = 1;
    S32 rc;
    sceMcTblGetDir* finf = &mcdata->finfo;

    rc = iSG_get_finfo(mcdata, fname, mcdata->gamepath);
    if (!rc)
    {
        result = 0;
    }
    else
    {
        if (sec != NULL)
        {
            *sec = finf->_Modify.Sec;
        }
        if (min != NULL)
        {
            *min = finf->_Modify.Min;
        }
        if (hr != NULL)
        {
            *hr = finf->_Modify.Hour;
        }
        if (mon != NULL)
        {
            *mon = finf->_Modify.Month;
        }
        if (day != NULL)
        {
            *day = finf->_Modify.Day;
        }
        if (yr != NULL)
        {
            *yr = finf->_Modify.Year;
        }
    }

    return result;
}

S32 iSG_mca_fmt(st_ISG_MEMCARD_DATA* mcdata, S32 force)
{
    S32 result = 1;
    S32 rc;

    mcdata->allow_cache = 0;

    if (iSG_mc_isformatted(mcdata, mcdata->mcport) && force)
    {
        if (iSG_mca_unfmt(mcdata))
        {
            iSG_mcasync_chkop(mcdata, 1, NULL);
        }
    }

    if (iSG_mc_isformatted(mcdata, mcdata->mcport))
    {
        result = 0;
    }
    else
    {
        mcdata->cur_mcop = sceMcFuncNoFormat;
        rc = sceMcFormat(mcdata->mcport, mcdata->mcslot);
        if (!iSG_is_MCOP_realerr(mcdata->cur_mcop, rc))
        {
            result = 0;
        }
    }

    return result;
}

S32 iSG_mca_unfmt(st_ISG_MEMCARD_DATA* mcdata)
{
    S32 result = 1;
    S32 rc;

    mcdata->allow_cache = 0;

    if (!iSG_mc_isformatted(mcdata, mcdata->mcport))
    {
        result = 0;
    }
    else
    {
        mcdata->cur_mcop = sceMcFuncNoUnformat;
        rc = sceMcUnformat(mcdata->mcport, mcdata->mcslot);
        if (!iSG_is_MCOP_realerr(mcdata->cur_mcop, rc))
        {
            result = 0;
        }
    }

    return result;
}

S32 iSG_mca_fopen(st_ISG_MEMCARD_DATA* mcdata, const char* fname, en_ISG_IOMODE mode)
{
    S32 result = 1;
    S32 rc;
    S32 ps2mode;

    if (mode == ISG_IOMODE_READ)
    {
        ps2mode = SCE_RDONLY;
    }
    else
    {
        ps2mode = SCE_WRONLY | SCE_CREAT;
    }

    mcdata->cur_mcop = sceMcFuncNoOpen;
    rc = sceMcOpen(mcdata->mcport, mcdata->mcslot, fname, ps2mode);
    if (!iSG_is_MCOP_realerr(mcdata->cur_mcop, rc))
    {
        result = 0;
    }

    return result;
}

S32 iSG_mca_fread(st_ISG_MEMCARD_DATA* mcdata, char* buf, S32 bufsize)
{
    S32 result = 1;
    S32 rc;

    if (mcdata->mcfp < 0)
    {
        result = 0;
    }
    else
    {
        mcdata->cur_mcop = sceMcFuncNoRead;
        rc = sceMcRead(mcdata->mcfp, buf, bufsize);
        if (!iSG_is_MCOP_realerr(mcdata->cur_mcop, rc))
        {
            result = 0;
        }
    }

    return result;
}

S32 iSG_mca_fwrite(st_ISG_MEMCARD_DATA* mcdata, char* data, S32 n)
{
    S32 result = 1;
    S32 rc;

    mcdata->allow_cache = 0;

    if (mcdata->mcfp < 0)
    {
        result = 0;
    }
    else
    {
        mcdata->cur_mcop = sceMcFuncNoWrite;
        rc = sceMcWrite(mcdata->mcfp, data, n);
        if (!iSG_is_MCOP_realerr(mcdata->cur_mcop, rc))
        {
            result = 0;
        }
    }

    return result;
}

en_ISGMCA_STATUS iSG_mcasync_chkop(st_ISG_MEMCARD_DATA* mcdata, S32 block,
                                          S32* sync_resval)
{
    en_ISGMCA_STATUS result;
    S32 rc;
    S32 ret = 0;
    S32 mcf = mcdata->cur_mcop;
    U32 on;

    if (sync_resval != NULL)
    {
        *sync_resval = -1;
    }

    do
    {
        rc = sceMcSync(!block, &mcf, &ret);
        switch (rc)
        {
        case sceMcExecRun:
            result = ISG_MCA_STAT_INPROG;
            break;
        case sceMcExecFinish:
            result = ISG_MCA_STAT_DONE;
            break;
        case sceMcExecIdle:
            result = ISG_MCA_STAT_DONE_ERR;
            break;
        default:
            result = ISG_MCA_STAT_DONE_ERR;
            break;
        }

        if (block && result == ISG_MCA_STAT_INPROG)
        {
            iVSync();
        }

        // Keep sound and pads serviced while waiting on the card.
        xSndUpdate();
        iPadUpdate(&mPad[globals.currentActivePad], &on);
    } while (block && result == ISG_MCA_STAT_INPROG);

    if (result == ISG_MCA_STAT_DONE)
    {
        if (!iSG_is_synccode_realerr(mcf, ret, mcdata))
        {
            result = ISG_MCA_STAT_DONE_ERR;
        }

        if (sync_resval != NULL)
        {
            *sync_resval = ret;
        }

        if (result && mcf == mcdata->cur_mcop && mcdata->cur_mcop == sceMcFuncNoOpen)
        {
            mcdata->mcfp = ret;
        }
    }

    return result;
}

// Whether a completed operation's result code is acceptable for that operation.
S32 iSG_is_synccode_realerr(S32 mcop, S32 mcopret, st_ISG_MEMCARD_DATA* mcdata)
{
    S32 is_ok = 1;

    if (mcopret >= 0)
    {
    }
    else if (mcopret < -9)
    {
        is_ok = 0;
    }
    else if (mcop == sceMcFuncNoCardInfo)
    {
        switch (mcopret)
        {
        case 0:
        case -1:
        case -2:
            break;
        default:
            is_ok = 0;
            break;
        }
    }
    else if (mcop == sceMcFuncNoOpen)
    {
        switch (mcopret)
        {
        case -2:
            is_ok = 0;
            break;
        case -3:
            is_ok = 0;
            break;
        case -4:
            is_ok = 0;
            break;
        case -5:
        case -7:
            break;
        default:
            is_ok = 0;
            break;
        }
    }
    else if (mcop == sceMcFuncNoClose)
    {
        switch (mcopret)
        {
        case -2:
            is_ok = 0;
            break;
        case -4:
            is_ok = 0;
            break;
        case -5:
            is_ok = 0;
            break;
        default:
            is_ok = 0;
            break;
        }
    }
    else if (mcop == sceMcFuncNoSeek)
    {
        switch (mcopret)
        {
        case -2:
            is_ok = 0;
            break;
        case -4:
            is_ok = 0;
            break;
        default:
            is_ok = 0;
            break;
        }
    }
    else if (mcop == sceMcFuncNoRead)
    {
        switch (mcopret)
        {
        case -2:
            is_ok = 0;
            break;
        case -4:
            is_ok = 0;
            break;
        case -5:
            is_ok = 0;
            break;
        default:
            is_ok = 0;
            break;
        }
    }
    else if (mcop == sceMcFuncNoWrite)
    {
        switch (mcopret)
        {
        case -2:
            is_ok = 0;
            break;
        case -3:
            is_ok = 0;
            break;
        case -4:
            is_ok = 0;
            break;
        case -5:
            is_ok = 0;
            break;
        case -8:
            is_ok = 0;
            break;
        default:
            is_ok = 0;
            break;
        }
    }
    else if (mcop == sceMcFuncNoFlush)
    {
        switch (mcopret)
        {
        case -2:
            is_ok = 0;
            break;
        case -4:
            is_ok = 0;
            break;
        default:
            is_ok = 0;
            break;
        }
    }
    else if (mcop == sceMcFuncNoMkdir)
    {
        switch (mcopret)
        {
        case -2:
            is_ok = 0;
            break;
        case -3:
            is_ok = 0;
            break;
        case -4:
            is_ok = 0;
            break;
        default:
            is_ok = 0;
            break;
        }
    }
    else if (mcop == sceMcFuncNoChDir)
    {
        switch (mcopret)
        {
        case -2:
            is_ok = 0;
            break;
        case -4:
            is_ok = 0;
            break;
        case -6:
            is_ok = 0;
            break;
        default:
            is_ok = 0;
            break;
        }
    }
    else if (mcop == sceMcFuncNoGetDir)
    {
        switch (mcopret)
        {
        case -2:
            is_ok = 0;
            break;
        case -4:
            is_ok = 0;
            break;
        default:
            is_ok = 0;
            break;
        }
    }
    else if (mcop == sceMcFuncNoFileInfo)
    {
        switch (mcopret)
        {
        case -2:
            is_ok = 0;
            break;
        case -4:
            is_ok = 0;
            break;
        default:
            is_ok = 0;
            break;
        }
    }
    else if (mcop == sceMcFuncNoDelete)
    {
        switch (mcopret)
        {
        case -2:
            is_ok = 0;
            break;
        case -4:
            is_ok = 0;
            break;
        case -5:
            is_ok = 0;
            break;
        case -6:
            is_ok = 0;
            break;
        default:
            is_ok = 0;
            break;
        }
    }
    else if (mcop == sceMcFuncNoFormat)
    {
        is_ok = 0;
    }
    else if (mcop == sceMcFuncNoUnformat)
    {
        is_ok = 0;
    }
    else if (mcop == sceMcFuncNoEntSpace)
    {
        switch (mcopret)
        {
        case -2:
            is_ok = 0;
            break;
        case -4:
            break;
        default:
            is_ok = 0;
            break;
        }
    }
    else if (mcop == sceMcFuncNoRename)
    {
        switch (mcopret)
        {
        case -2:
            is_ok = 0;
            break;
        case -4:
            is_ok = 0;
            break;
        default:
            is_ok = 0;
            break;
        }
    }
    else if (mcop == sceMcFuncChgPrior)
    {
        is_ok = 0;
    }
    else if (mcop == 21)
    {
    }
    else
    {
        is_ok = 0;
    }

    return is_ok;
}

// Whether queueing an operation succeeded.
S32 iSG_is_MCOP_realerr(S32 mcop, S32 que_rc)
{
    S32 is_ok = 1;

    switch (mcop)
    {
    case sceMcFuncNoCardInfo:
    case sceMcFuncNoOpen:
    case sceMcFuncNoClose:
    case sceMcFuncNoSeek:
    case sceMcFuncNoRead:
    case sceMcFuncNoWrite:
    case sceMcFuncNoFlush:
    case sceMcFuncNoMkdir:
    case sceMcFuncNoChDir:
    case sceMcFuncNoGetDir:
    case sceMcFuncNoFileInfo:
    case sceMcFuncNoDelete:
    case sceMcFuncNoFormat:
    case sceMcFuncNoUnformat:
    case sceMcFuncNoEntSpace:
    case sceMcFuncNoRename:
    case sceMcFuncChgPrior:
        if (que_rc != 0)
        {
            is_ok = 0;
        }
        break;
    case 21:
        break;
    default:
        is_ok = 0;
        break;
    }

    return is_ok;
}

void iSGMakeTimeStamp(char* str)
{
    str[0] = '\0';
}

void iSGIconInit(void* iconData, U32 size)
{
    gIconSize = size;
    g_scoobydoo_icon_list = (char*)iconData;
}

U8 iSGIsGameCorrupt(st_ISGSESSION* sess, S32)
{
    char fileNames[3][25] = { ISG_PRODUCT_CODE "HIBob   ", "icon.sys", "SpongeIcon.ico" };
    char* gameDir;
    S32 resultCode;
    S32 i;

    gameDir = "/" ISG_PRODUCT_CODE "HIBob   ";
    sceMcChdir(sess->mcdata->mcport, sess->mcdata->mcslot, gameDir, NULL);
    sceMcSync(0, NULL, &resultCode);

    if (resultCode == 0)
    {
        for (i = 0; i < 3; i++)
        {
            sceMcOpen(sess->mcdata->mcport, sess->mcdata->mcslot, fileNames[i], SCE_RDONLY);
            sceMcSync(0, NULL, &resultCode);
            if (resultCode >= 0)
            {
                sceMcClose(resultCode);
                sceMcSync(0, NULL, &resultCode);
                return 1;
            }
        }
    }

    return 0;
}

U8 iSGCheckForGameFiles(S32 mcPort)
{
    char fileNames[3][25] = { "SpongeBob00", "SpongeBob01", "SpongeBob02" };
    char* gameDir;
    S32 resultCode;
    S32 i;

    gameDir = "/" ISG_PRODUCT_CODE "HIBob   ";
    sceMcChdir(mcPort, 0, gameDir, NULL);
    sceMcSync(0, NULL, &resultCode);

    if (resultCode == 0)
    {
        for (i = 0; i < 3; i++)
        {
            sceMcOpen(mcPort, 0, fileNames[i], SCE_RDONLY);
            sceMcSync(0, NULL, &resultCode);
            if (resultCode >= 0)
            {
                sceMcClose(resultCode);
                sceMcSync(0, NULL, &resultCode);
                return 1;
            }
        }
    }

    return 0;
}

U8 iSGCheckMemoryCard(st_ISGSESSION*, S32 index)
{
    S32 result;

    result = sceMcGetInfo(index, 0, NULL, NULL, NULL);
    sceMcSync(0, NULL, &result);

    if (result == 0 || result == -1 || result == -2)
    {
        return 1;
    }

    return 0;
}
