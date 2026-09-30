#include "xserializer.h"
#include "xMemMgr.h"
#include "xutil.h"
#include <string.h>

static S32 g_serinit;
static st_XSERIAL_DATA_PRIV g_xserdata = { 0, NULL, 0, NULL, NULL, NULL };

static S32 g_tbl_onbit[32] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                               0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

static S32 g_tbl_clear[32] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                               0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

static void xSER_init_tables();
static st_SERIAL_CLIENTINFO* XSER_get_client(U32 idtag);
static S32 xSER_xsgclt_svinfo_ver(void*, st_XSAVEGAME_DATA*, S32* cur_space, S32* max_fullgame);
static S32 xSER_xsgclt_svproc_ver(void*, st_XSAVEGAME_DATA* xsg, st_XSAVEGAME_WRITECONTEXT* wctxt);
static S32 xSER_xsgclt_ldproc_ver(void*, st_XSAVEGAME_DATA* xsg, st_XSAVEGAME_READCONTEXT* rctxt,
                                  U32, S32);
static S32 xSER_xsgclt_svinfo_clt(void* cltdata, st_XSAVEGAME_DATA*, S32* cur_space,
                                  S32* max_fullgame);
static S32 xSER_xsgclt_svproc_clt(void* cltdata, st_XSAVEGAME_DATA* xsg,
                                  st_XSAVEGAME_WRITECONTEXT* wctxt);
static S32 xSER_xsgclt_ldproc_clt(void*, st_XSAVEGAME_DATA* xsg, st_XSAVEGAME_READCONTEXT* rctxt,
                                  U32 idtag, S32);
static S32 xSER_xsgclt_svproc_fill(void*, st_XSAVEGAME_DATA* xsg, st_XSAVEGAME_WRITECONTEXT* wctxt);
static S32 xSER_xsgclt_svinfo_fill(void*, st_XSAVEGAME_DATA*, S32* cur_space, S32* max_fullgame);
static S32 xSER_xsgclt_ldproc_fill(void*, st_XSAVEGAME_DATA* xsg, st_XSAVEGAME_READCONTEXT* rctxt,
                                   U32, S32);

S32 xSerialStartup(S32 count, st_SERIAL_PERCID_SIZE* sizeinfo)
{
    if (g_serinit++ == 0)
    {
        memset(&g_xserdata, 0, sizeof(st_XSERIAL_DATA_PRIV));
        xSER_init_tables();
        xSER_init_buffers(count, sizeinfo);
    }

    return g_serinit;
}

S32 xSerialShutdown()
{
    g_serinit--;

    return g_serinit;
}

void xSerialTraverse(S32 (*func)(U32, xSerial*))
{
    st_XSERIAL_DATA_PRIV* xsd = &g_xserdata;
    xSerial xser;

    for (S32 i = 0; i < xsd->cltlist.cnt; i++)
    {
        st_SERIAL_CLIENTINFO* clt = (st_SERIAL_CLIENTINFO*)xsd->cltlist.list[i];
        xser.setClient(clt->idtag);
        S32 rc = func(clt->idtag, &xser);
        if (rc == 0)
            break;
    }
}

xSerial::xSerial()
{
}

xSerial::~xSerial()
{
    if (ctxtdata != NULL)
    {
        xUtil_idtag2string(idtag, 0);
    }
}

void xSerial::operator delete(void*)
{
}

void xSerial::setClient(U32 idtag)
{
    prepare(idtag);
}

// non-matching: register allocation
S32 xSerial::Write(char* data, S32 elesize, S32 n)
{
    S32 nbit;

    if (n == 0)
    {
        return 0;
    }

    nbit = n > 0 ? n * elesize * 8 : -n;

    if (n < 0)
    {
        S32 bidx = 0;
        S32 i;
        S32* iptr = (S32*)data;
        for (i = 0; i < nbit; i++)
        {
            wrbit(*iptr & g_tbl_onbit[bidx]);
            if (++bidx == 32)
            {
                bidx = 0;
                iptr++;
            }
        }
    }
    else
    {
        S32 bidx = 0;
        S32 i;
        char* cptr = data;
        for (i = 0; i < nbit; i++)
        {
            wrbit(*cptr & g_tbl_onbit[bidx]);
            if (++bidx == 8)
            {
                bidx = 0;
                cptr++;
            }
        }
    }

    return nbit;
}

S32 xSerial::Write_b1(S32 bits)
{
    return Write((char*)&bits, 4, -1);
}

S32 xSerial::Write_b7(U32 bits)
{
    return Write((char*)&bits, 4, -7);
}

S32 xSerial::Write(U8 data)
{
    return Write((char*)&data, 1, 1);
}

S32 xSerial::Write(S16 data)
{
    return Write((char*)&data, 2, 1);
}

S32 xSerial::Write(S32 data)
{
    return Write((char*)&data, 4, 1);
}

S32 xSerial::Write(U32 data)
{
    return Write((char*)&data, 4, 1);
}

S32 xSerial::Write(F32 data)
{
    return Write((char*)&data, 4, 1);
}

// non-matching: register allocation
S32 xSerial::Read(char* buf, S32 elesize, S32 n)
{
    S32 nbit = n > 0 ? n * elesize * 8 : -n;

    if (n < 0)
    {
        S32 bidx = 0;
        for (S32 i = 0; i < nbit; i++)
        {
            if (rdbit() != 0)
            {
                *(U32*)buf |= g_tbl_onbit[bidx];
            }
            else
            {
                *(U32*)buf &= g_tbl_clear[bidx];
            }

            if (++bidx == 32)
            {
                bidx = 0;
                buf = (char*)((U32*)buf + 1);
            }
        }
    }
    else
    {
        S32 bidx = 0;
        for (S32 i = 0; i < nbit; i++)
        {
            if (rdbit() != 0)
            {
                *buf |= (char)g_tbl_onbit[bidx];
            }
            else
            {
                *buf &= (char)g_tbl_clear[bidx];
            }

            if (++bidx == 8)
            {
                bidx = 0;
                buf++;
            }
        }
    }

    return nbit;
}

S32 xSerial::Read_b1(S32* bits)
{
    return Read((char*)bits, 4, -1);
}

S32 xSerial::Read_b7(U32* bits)
{
    return Read((char*)bits, 4, -7);
}

S32 xSerial::Read(U8* buf)
{
    return Read((char*)buf, 1, 1);
}

S32 xSerial::Read(S16* buf)
{
    return Read((char*)buf, 2, 1);
}

S32 xSerial::Read(S32* buf)
{
    return Read((char*)buf, 4, 1);
}

S32 xSerial::Read(U32* buf)
{
    return Read((char*)buf, 4, 1);
}

S32 xSerial::Read(F32* buf)
{
    return Read((char*)buf, 4, 1);
}

void xSerial::wrbit(S32 is_on)
{
    st_SERIAL_CLIENTINFO* clt = ctxtdata;

    if (bittally + 1 > clt->actsize * 8)
    {
        warned = TRUE;
        return;
    }

    clt->membuf[curele] &= g_tbl_clear[bitidx];
    if (is_on)
    {
        clt->membuf[curele] |= g_tbl_onbit[bitidx];
    }

    bitidx++;
    if (bitidx == 32)
    {
        curele++;
        bitidx = 0;
    }
    bittally++;
}

S32 xSerial::rdbit()
{
    st_SERIAL_CLIENTINFO* clt = ctxtdata;

    if (bittally + 1 > clt->actsize * 8)
    {
        warned = TRUE;
        return 0;
    }

    bool is_on = ctxtdata->membuf[curele] & g_tbl_onbit[bitidx];

    bitidx++;
    if (bitidx == 32)
    {
        curele++;
        bitidx = 0;
    }
    bittally++;

    return is_on;
}

void xSerial::prepare(U32 idtag)
{
    st_SERIAL_CLIENTINFO* clt = XSER_get_client(idtag);
    this->idtag = clt->idtag;
    baseoff = clt->trueoff;
    ctxtdata = clt;
    warned = FALSE;
    curele = 0;
    bitidx = 0;
    bittally = 0;
}

void xSerialWipeMainBuffer()
{
    memset(g_xserdata.bitbuf, 0, g_xserdata.buf_bytcnt);
}

static void xSER_init_tables()
{
    for (S32 i = 0; i < 32; i++)
    {
        g_tbl_onbit[i] = 1 << i;
        g_tbl_clear[i] = ~(1 << i);
    }
}

static void xSER_init_buffers(S32 count, st_SERIAL_PERCID_SIZE* sizeinfo)
{
    st_XSERIAL_DATA_PRIV* xsd = &g_xserdata;
    S32 i = 0;
    S32 tally = 0;
    S32 sicnt = 0;
    st_SERIAL_PERCID_SIZE* sitmp = NULL;
    st_SERIAL_CLIENTINFO* tmp_clt = NULL;

    XOrdInit(&g_xserdata.cltlist, count, 0);

    xsd->cltbuf =
        (st_SERIAL_CLIENTINFO*)xMemAlloc(gActiveHeap, count * sizeof(st_SERIAL_CLIENTINFO), 0);
    memset(xsd->cltbuf, 0, count * sizeof(st_SERIAL_CLIENTINFO));
    xsd->cltnext = xsd->cltbuf;

    sitmp = sizeinfo;
    while (sitmp->idtag != 0)
    {
        tally += (sitmp->needsize + 3) & ~3;
        sicnt++;
        sitmp++;
    }
    tally += (count - sicnt) * 400;

    xsd->bitbuf = (S32*)xMemAlloc(gActiveHeap, tally, 0);
    memset(xsd->bitbuf, 0, tally);

    xsd->buf_bytcnt = tally;

    sitmp = sizeinfo;
    tally = 0;
    tmp_clt = xsd->cltnext;
    while (sitmp->idtag != 0)
    {
        tmp_clt->idtag = sitmp->idtag;
        tmp_clt->trueoff = tally;
        tmp_clt->actsize = (sitmp->needsize + 3) & ~3;
        tmp_clt->membuf = xsd->bitbuf + tally / 4;
        XOrdAppend(&xsd->cltlist, tmp_clt);
        tally += tmp_clt->actsize;
        sitmp++;
        tmp_clt++;
    }
    XOrdSort(&xsd->cltlist, xSER_ord_compare);
    xsd->cltnext = tmp_clt;

    for (i = sicnt; i < count; i++)
    {
        tmp_clt->idtag = 0;
        tmp_clt->trueoff = tally;
        tmp_clt->actsize = 400;
        tmp_clt->membuf = xsd->bitbuf + tally / 4;
        tally += tmp_clt->actsize;
        tmp_clt++;
    }
}

static S32 xSER_ord_compare(void* e1, void* e2)
{
    S32 greater;

    if (*(U32*)e1 < *(U32*)e2)
    {
        return -1;
    }

    greater = (*(U32*)e1 > *(U32*)e2) ? 1 : 0;
    if (greater)
    {
        return 1;
    }
    return 0;
}

static S32 xSER_ord_test(const void* key, void* elt)
{
    S32 greater;

    if ((U32)key < *(U32*)elt)
    {
        return -1;
    }

    greater = ((U32)key > *(U32*)elt) ? 1 : 0;
    if (greater)
    {
        return 1;
    }
    return 0;
}

static st_SERIAL_CLIENTINFO* XSER_get_client(U32 idtag)
{
    st_XSERIAL_DATA_PRIV* xsd = &g_xserdata;
    st_SERIAL_CLIENTINFO* clt;
    S32 idx = XOrdLookup(&xsd->cltlist, (void*)idtag, xSER_ord_test);

    if (idx < 0)
    {
        clt = xsd->cltnext;
        xsd->cltnext = clt + 1;
        clt->idtag = idtag;
        XOrdInsert(&xsd->cltlist, clt, xSER_ord_compare);
    }
    else
    {
        clt = (st_SERIAL_CLIENTINFO*)xsd->cltlist.list[idx];
    }

    return clt;
}

S32 xSerial_svgame_register(st_XSAVEGAME_DATA* sgctxt, en_SAVEGAME_MODE mode)
{
    st_XSERIAL_DATA_PRIV* xsd = &g_xserdata;
    xsd->flg_info &= 0xfffffffe;

    if (mode == XSG_MODE_SAVE)
    {
        xSGAddSaveClient(sgctxt, 'SVID', xsd, xSER_xsgclt_svinfo_ver, xSER_xsgclt_svproc_ver);

        for (S32 i = 0; i < xsd->cltlist.cnt; i++)
        {
            xSGAddSaveClient(sgctxt, ((st_SERIAL_CLIENTINFO*)xsd->cltlist.list[i])->idtag,
                             xsd->cltlist.list[i], xSER_xsgclt_svinfo_clt,
                             xSER_xsgclt_svproc_clt);
        }
        xSGAddSaveClient(sgctxt, 'SFIL', &g_xserdata, xSER_xsgclt_svinfo_fill,
                         xSER_xsgclt_svproc_fill);
    }
    else if (mode == XSG_MODE_LOAD)
    {
        xSGAddLoadClient(sgctxt, 'SVID', xsd, xSER_xsgclt_ldproc_ver);
        xSGAddLoadClient(sgctxt, 0, &g_xserdata, xSER_xsgclt_ldproc_clt);
        xSGAddLoadClient(sgctxt, 'SFIL', &g_xserdata, xSER_xsgclt_ldproc_fill);
    }

    return 1;
}

static S32 xSER_xsgclt_svinfo_ver(void*, st_XSAVEGAME_DATA*, S32* cur_space, S32* max_fullgame)
{
    *cur_space = 4;
    *max_fullgame = 4;
    return 1;
}

static S32 xSER_xsgclt_svproc_ver(void*, st_XSAVEGAME_DATA* xsg, st_XSAVEGAME_WRITECONTEXT* wctxt)
{
    S32 ver = 1;
    xSGWriteData(xsg, wctxt, &ver, 1);
    return 1;
}

static S32 xSER_xsgclt_ldproc_ver(void*, st_XSAVEGAME_DATA* xsg, st_XSAVEGAME_READCONTEXT* rctxt,
                                  U32, S32)
{
    st_XSERIAL_DATA_PRIV* xsd = &g_xserdata;
    S32 ver = 0;
    xSGReadData(xsg, rctxt, &ver, 1);
    if (ver != 1)
    {
        xsd->flg_info |= 1;
    }
    return 1;
}

static S32 xSER_xsgclt_svinfo_clt(void* cltdata, st_XSAVEGAME_DATA*, S32* cur_space,
                                  S32* max_fullgame)
{
    *cur_space = ((st_SERIAL_CLIENTINFO*)cltdata)->actsize;
    *max_fullgame = ((st_SERIAL_CLIENTINFO*)cltdata)->actsize;
    return 1;
}

static S32 xSER_xsgclt_svproc_clt(void* cltdata, st_XSAVEGAME_DATA* xsg,
                                  st_XSAVEGAME_WRITECONTEXT* wctxt)
{
    st_SERIAL_CLIENTINFO* clt = XSER_get_client(((st_SERIAL_CLIENTINFO*)cltdata)->idtag);
    xSGWriteData(xsg, wctxt, (char*)clt->membuf, clt->actsize);
    return 1;
}

static S32 xSER_xsgclt_ldproc_clt(void*, st_XSAVEGAME_DATA* xsg, st_XSAVEGAME_READCONTEXT* rctxt,
                                  U32 idtag, S32)
{
    if (g_xserdata.flg_info & 1)
    {
        return 1;
    }

    st_SERIAL_CLIENTINFO* clt = XSER_get_client(idtag);
    xSGReadData(xsg, rctxt, (char*)clt->membuf, clt->actsize);

    return 1;
}

static S32 xSER_xsgclt_svinfo_fill(void*, st_XSAVEGAME_DATA*, S32* cur_space, S32* max_fullgame)
{
    st_XSERIAL_DATA_PRIV* xsd = &g_xserdata;
    S32 i;
    S32 tally = 0;
    S32 size;

    for (i = 0; i < xsd->cltlist.cnt; i++)
    {
        tally += ((st_SERIAL_CLIENTINFO*)xsd->cltlist.list[i])->actsize;
    }

    size = xsd->buf_bytcnt - tally + 8;
    *cur_space = size;
    *max_fullgame = size;

    return 1;
}

static S32 xSER_xsgclt_svproc_fill(void*, st_XSAVEGAME_DATA* xsg, st_XSAVEGAME_WRITECONTEXT* wctxt)
{
    char filbuf[9] = { 'R', 'y', 'a', 'n', 'N', 'e', 'i', 'l', '\x00' };
    xSGWriteData(xsg, wctxt, (char*)&filbuf, 8);
    return 1;
}

static S32 xSER_xsgclt_ldproc_fill(void*, st_XSAVEGAME_DATA* xsg, st_XSAVEGAME_READCONTEXT* rctxt,
                                   U32, S32)
{
    char filbuf[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    if (g_xserdata.flg_info & 1)
    {
        return 1;
    }

    xSGReadData(xsg, rctxt, (char*)&filbuf, 8);

    return 1;
}
