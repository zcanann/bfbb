#include "xHudFontMeter.h"

#include <types.h>
#include <xMath2.h>
#include <stdio.h>
#if defined(PS2)
#include <new.h>
#else
#include <PowerPC_EABI_Support\MSL_C++\MSL_Common\Include\new.h>
#endif

void xhud::font_meter_widget::load(xBase& data, xDynAsset& asset, size_t)
{
    init_base(data, asset, sizeof(xBase) + sizeof(font_meter_widget));
    font_meter_widget* widget = (font_meter_widget*)(&data + 1);
    new (widget) font_meter_widget((font_meter_asset&)asset);
}

static const basic_rect<F32> screen_bounds = { 0.0f, 0.0f, 1.0f, 1.0f };

xhud::font_meter_widget::font_meter_widget(const xhud::font_meter_asset& init)
    : meter_widget(init), font(init.font), start_font(init.font)
{
    this->last_value = ((S32)(this->value)) - 20;
    this->xf.id = 0;
    this->xf.width = this->font.w;
    this->xf.height = this->font.h;
    this->xf.space = this->font.space;

    this->xf.color = *(iColor_tag*)&this->font.c;
    this->xf.clip = screen_bounds;
}

void xhud::font_meter_widget::destruct()
{
    xhud::meter_widget::destruct();
}

void xhud::font_meter_widget::destroy()
{
    this->destruct();
}

U32 xhud::font_meter_widget::type() const
{
    static U32 myid;
    static S8 init;

    if (init == 0)
    {
        myid = xStrHash(font_meter_asset::type_name());
        init = 1;
    }
    return myid;
}

bool xhud::font_meter_widget::is(U32 id) const
{
    return id == xhud::font_meter_widget::type() || xhud::meter_widget::is(id);
}

void xhud::font_meter_widget::update(F32 dt)

{
    static char* format_text[3] = { "%d", "%d/%d", "%d of %d" };

    F32 a;
    S32 new_value;

    this->updater(dt);
    this->xf.id = this->font.id;
    this->xf.space = this->font.space;
    a = this->rc.size.x;
    this->font.w = a;
    this->xf.width = a;
    a = this->rc.size.y;
    this->font.h = a;
    this->xf.height = a;

    a = this->rc.a * (F32)this->start_font.c.a + 0.5f;
    this->font.c.a = (a <= 0.0f) ? 0 : ((a >= 255.0f) ? 255 : (U8)a);

    a = this->rc.a * (F32)this->start_font.drop_c.a + 0.5f;
    this->font.drop_c.a = (a <= 0.0f) ? 0 : ((a >= 255.0f) ? 255 : (U8)a);

    new_value = (S32)(this->value + 0.5f);
    if (this->last_value != new_value)
    {
        this->last_value = new_value;
        font_meter_asset& fma = *(font_meter_asset*)this->a;

        // No `mode` local (the DWARF has none): the index is a ?: inline in the
        // call, U8-typed so the clrlslwi survives, and max_value is re-read.
        sprintf(this->buffer,
                format_text[(this->max_value < this->min_value) ? (U8)0 : fma.counter_mode],
                new_value, (S32)(this->max_value + 0.5f));
        basic_rect<F32> bounds = this->xf.bounds(this->buffer);
        this->offset.x = -bounds.x;
        this->offset.y = -bounds.y;
    }
    return;
}

void xhud::font_meter_widget::render()

{
    F32 x = this->offset.x + this->rc.loc.x;
    F32 y = this->offset.y + this->rc.loc.y;
    if (this->font.drop_c.a > 0)
    {
        this->xf.color = this->font.drop_c;
        this->xf.render(this->buffer, x + this->font.drop_x, y + this->font.drop_y);
    }
    if (this->font.c.a > 0)
    {
        this->xf.color = this->font.c;
        this->xf.render(this->buffer, x, y);
    }
}
