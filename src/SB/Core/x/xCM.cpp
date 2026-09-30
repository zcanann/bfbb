#include "xCM.h"
#include "xEvent.h"
#include "xFont.h"
#include "xstransvc.h"
#include "xDebug.h"
#include "xVec3.h"

#include <rwplcore.h>
#include <stdio.h>
#include <types.h>

// TODO: find and move to appropriate header
U32 Im2DRenderQuad(F32 x0, F32 y0, F32 x1, F32 y1, F32 z, F32 alpha, F32 uv_offset);

void __deadstripped_rodata()
{
    const xVec3 dead0 = {};
    const xVec3 dead1 = {};
    const xVec3 dead2 = {};
    const xVec3 dead3 = {};
    const U8 dead4[0x28] = {};
    const U8 dead5[0x28] = {};
    const U8 dead6[0x28] = {};
    const U8 dead7[0x28] = {};
    const U8 dead8[0x28] = {};
    const U8 dead9[0x28] = {};
    const U8 dead10[0x28] = {};
}

static const basic_rect<F32> screen_bounds = { 0.0f, 0.0f, 1.0f, 1.0f };
static const basic_rect<F32> default_adjust = { 0.0f, 0.0f, 1.0f, 1.0f };

F32 dtscale = 1.0f;

static F32 credits_time = 10000.0f;
static xCreditsData* credits_data;
static U32 credits_parentID;

static void xCMprep(xCreditsData* data)
{
    /*
        data seems to point to this general structure:
        0x00 - xCMheader
        0x18 - xCMcredits
        0x50 - xCMpreset* presets_array[xCMcredits.num_presets]
             - xCMhunk* hunk_array[?]
    */

    char* dp; // In the DWARF. Don't know what this is used for.
    xCMheader* hdr = (xCMheader*)data;

    if (hdr->magic != 0xBEEEEEEF)
    {
        return;
    }

    xCMcredits* cp = (xCMcredits*)(&hdr[1]);
    while ((S32)cp - (S32)data < hdr->total_size)
    {
        xCMpreset* pp = (xCMpreset*)(&cp[1]);
        xCMhunk* hp = (xCMhunk*)(&pp[cp->num_presets]);

        for (; (S32)hp - (S32)cp < cp->credits_size; hp = (xCMhunk*)((S32)hp + hp->hunk_size))
        {
            switch (pp[hp->preset].align)
            {
            case 0:
            case 1:
            case 2:
            case 3:

                if (hdr->state == 0)
                {
                    if (hp->text1 != NULL)
                    {
                        hp->text1 -= (S32)data;
                    }
                    if (hp->text2 != NULL)
                    {
                        hp->text2 -= (S32)data;
                    }
                }
                else
                {
                    if (hp->text1 != NULL)
                    {
                        hp->text1 += (S32)data;
                    }
                    if (hp->text2 != NULL)
                    {
                        hp->text2 += (S32)data;
                    }
                }
                break;
            }
        }
        cp = (xCMcredits*)hp;
    }

    hdr->state = hdr->state == 0 ? 1 : 0;
}

static iColor_tag xCMcolor_scale(iColor_tag color, F32 t)
{
    iColor_tag ret = color;
    F32 a = color.a * t;
    xColorInit(&ret, (F32)color.r, (F32)color.g, (F32)color.b, a);
    return ret;
}

void __deadstripped_strings(xCMpreset* preset)
{
    switch (preset->align)
    {
    // TODO: Assign correct cases to these and make an enum/defines out of it
    default:
    {
        xprintf("CM_ALIGN_CENTER");
        xprintf("CM_ALIGN_LEFT");
        xprintf("CM_ALIGN_RIGHT");
        xprintf("CM_ALIGN_INNER");
        xprintf("CM_ALIGN_TEXTURE");
    }
    }
}

static U32 xCMrender(F32 time, xCreditsData* data)
{
    if (data == NULL || ((xCMheader*)data)->magic != 0xBEEEEEEF)
        return 0;

    xCMheader* hdr = (xCMheader*)data;
    if (hdr->state == 1)
        xCMprep(data);

    char* dp = (char*)data;

    xprintf("credits time %6.2f\n", time);

    xCMhunk* hp;
    xCMcredits* cp = (xCMcredits*)(dp + sizeof(xCMheader));
    while ((U32)((char*)cp - dp) < hdr->total_size)
    {
        xCMpreset* pp = (xCMpreset*)((char*)cp + sizeof(xCMcredits));
        hp = (xCMhunk*)((char*)pp + cp->num_presets * sizeof(xCMpreset));

        for (; (U32)((char*)hp - (char*)cp) < cp->credits_size;
             hp = (xCMhunk*)((char*)hp + hp->hunk_size))
        {
            if (time >= hp->t0 && time <= hp->t1)
            {
                F32 t = (time - hp->t0) / (hp->t1 - hp->t0);
                xCMpreset* preset = &pp[hp->preset];

                F32 yScroll = t * (cp->out.y - cp->in.y) + cp->in.y;

                F32 a;
                if (t < cp->fin.start || t > cp->fout.end)
                    a = 0.0f;
                else if (t >= cp->fin.start && t <= cp->fin.end)
                    a = (t - cp->fin.start) / (cp->fin.end - cp->fin.start);
                else if (t >= cp->fout.start && t <= cp->fout.end)
                    a = (cp->fout.end - t) / (cp->fout.end - cp->fout.start);
                else
                    a = 1.0f;

                F32 x0, y0, x1, y1;
                switch (preset->align)
                {
                case 4:
                {
                    xCMtexture* tex = (xCMtexture*)&preset->box[0];
                    if (tex->texture == NULL)
                        tex->texture = (RwTexture*)xSTFindAsset(tex->assetID, NULL);
                    if (tex->texture != NULL)
                        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, tex->texture->raster);

                    x0 = 640.0f * tex->x;
                    y0 = 480.0f * tex->y;
                    x1 = 640.0f * (tex->x + tex->w);
                    y1 = 480.0f * (tex->y + tex->h);
                    Im2DRenderQuad(x0, y0, x1, y1, 0.0f, 1000000.0f, 0.5f);
                    xprintf("tex %6.2f,%6.2f - %6.2f,%6.2f\n", x0, y0, x1, y1);
                    break;
                }
                case 0:
                {
                    x0 = 0.5f * (1.0f - preset->box[0].box.x);
                    iColor_tag scaled = xCMcolor_scale(preset->box[0].color, a);

                    basic_rect<F32> bounds = { 0.0f, 0.0f, 0.0f, 0.0f };
                    bounds.x = x0;
                    bounds.y = yScroll;
                    bounds.w = preset->box[0].box.x;
                    bounds.h = preset->box[0].box.y;

                    xtextbox tb = xtextbox::create(
                        xfont::create(preset->box[0].font, NSCREENX(preset->box[0].char_size.x),
                                      NSCREENY(preset->box[0].char_size.y), 0.0f, scaled,
                                      screen_bounds),
                        bounds, 2, 0.0f, 0.0f, 0.0f, 0.0f);
                    tb.set_text(hp->text1);
                    tb.render(true);
                    break;
                }
                case 1:
                case 2:
                case 3:
                {
                    x0 = 0.5f * (1.0f - preset->box[0].box.x - preset->box[1].box.x -
                                 preset->innerspace);

                    U32 alignL, alignR;
                    if (preset->align == 1)
                    {
                        alignR = 0;
                        alignL = 0;
                    }
                    else if (preset->align == 2)
                    {
                        alignR = 1;
                        alignL = 1;
                    }
                    else
                    {
                        alignL = 1;
                        alignR = 0;
                    }

                    {
                        iColor_tag scaled = xCMcolor_scale(preset->box[0].color, a);
                        basic_rect<F32> bounds = { 0.0f, 0.0f, 0.0f, 0.0f };
                        bounds.x = x0;
                        bounds.y = yScroll;
                        bounds.w = preset->box[0].box.x;
                        bounds.h = preset->box[0].box.y;

                        xtextbox tb = xtextbox::create(
                            xfont::create(preset->box[0].font,
                                          NSCREENX(preset->box[0].char_size.x),
                                          NSCREENY(preset->box[0].char_size.y), 0.0f, scaled,
                                          screen_bounds),
                            bounds, alignL, 0.0f, 0.0f, 0.0f, 0.0f);
                        tb.set_text(hp->text1);
                        tb.render(true);
                    }

                    x1 = x0 + (preset->box[0].box.x + preset->innerspace);

                    {
                        iColor_tag scaled = xCMcolor_scale(preset->box[1].color, a);
                        basic_rect<F32> bounds = { 0.0f, 0.0f, 0.0f, 0.0f };
                        bounds.x = x1;
                        bounds.y = yScroll;
                        bounds.w = preset->box[1].box.x;
                        bounds.h = preset->box[1].box.y;

                        // The original uses the first box's font for both text boxes
                        xtextbox tb = xtextbox::create(
                            xfont::create(preset->box[0].font,
                                          NSCREENX(preset->box[1].char_size.x),
                                          NSCREENY(preset->box[1].char_size.y), 0.0f, scaled,
                                          screen_bounds),
                            bounds, alignR, 0.0f, 0.0f, 0.0f, 0.0f);
                        tb.set_text(hp->text2);
                        tb.render(true);
                    }
                    break;
                }
                default:
                    break;
                }
            }
        }
        cp = (xCMcredits*)hp;
    }

    return time >= 0.0f && time <= hdr->total_time;
}

void xCMupdate(F32 time)
{
    if (credits_data != 0)
    {
        credits_time += (time * dtscale);
        if (credits_time >= ((xCMheader*)credits_data)->total_time)
        {
            xCMstop();
        }
    }
}

void xCMrender()
{
    xCMrender(credits_time, credits_data);
}

void xCMstart(xCreditsData* data, F32 time, xBase* parent)
{
    if ((credits_data = data) != NULL)
    {
        credits_time = 0.0f;

        if (parent != NULL)
        {
            credits_parentID = parent->id;
        }
    }
}

// Equivalent: scheduling
void xCMstop()
{
    credits_time = 10000.0f;
    credits_data = NULL;
    if (credits_parentID != 0)
    {
        zEntEvent(credits_parentID, 0x212);
    }
}
