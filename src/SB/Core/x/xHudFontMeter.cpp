#include "xHudFontMeter.h"

#include <types.h>
#include <xMath2.h>
#include <stdio.h>
#include <PowerPC_EABI_Support\MSL_C++\MSL_Common\Include\new.h>

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
        myid = xStrHash(((xhud::font_meter_asset*)this)->type_name());
        init = 1;
    }
    return myid;
}

bool xhud::font_meter_widget::is(U32 id) const
{
    bool val = false;

    if (id == xhud::font_meter_widget::type() || xhud::meter_widget::is(id))
    {
        val = true;
    }
    return val;
}

// NOTE: this belongs in xHudFontMeter.h, defined inline in the class body --
// the target emits it weak, in its own section, and its string literal opens
// @stringBase0 ahead of update()'s format strings. Defined here it is strong
// and lands in .text between is() and update(); it has to sit above update()
// for the string pool to come out in the target's order.
char* xhud::font_meter_asset::type_name()
{
    return "hud:meter:font";
}

// Non-matching by six instructions, all register allocation: the target leaves
// r3 free until after format_text[mode] is loaded, so the `this->a` load and
// the format_text base both get r3, and `this->buffer` is materialised into r3
// only just before the call. Ours hoists `addi r3, r31, 0x118` to the top of
// the block and pushes the other two into r4. Identical instruction multiset
// modulo register numbering; ~20 source shapes measured, none below the r3/r4
// split, and the unpatched GC/2.0p1 emits the same code.
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
    this->font.c.a = (a <= 0.0f) ? 0 : ((a >= 255.0f) ? 255 : (U8)(S32)a);

    a = this->rc.a * (F32)this->start_font.drop_c.a + 0.5f;
    this->font.drop_c.a = (a <= 0.0f) ? 0 : ((a >= 255.0f) ? 255 : (U8)(S32)a);

    new_value = (S32)(this->value + 0.5f);
    if (this->last_value != new_value)
    {
        this->last_value = new_value;
        font_meter_asset& fma = *(font_meter_asset*)this->a;
        U8 mode;

        a = this->max_value;
        if (a < this->min_value)
        {
            mode = 0;
        }
        else
        {
            mode = fma.counter_mode;
        }
        sprintf(this->buffer, format_text[mode], new_value, (S32)(a + 0.5f));
        basic_rect<F32> bounds = this->xf.bounds(this->buffer);
        this->offset.x = -bounds.x;
        this->offset.y = -bounds.y;
    }
    return;
}

void xhud::font_meter_widget::render()

{
    F32 x;
    F32 temp_x;
    F32 y;

    temp_x = this->offset.x + this->rc.loc.x;
    y = this->offset.y + this->rc.loc.y;
    if (this->font.drop_c.a != 0)
    {
        this->xf.color = this->font.drop_c;
        x = temp_x + this->font.drop_x;
        this->xf.render(this->buffer, x, y + this->font.drop_y);
    }
    if (this->font.c.a != 0)
    {
        this->xf.color = this->font.c;
        this->xf.render(this->buffer, temp_x, y);
    }
    return;
}

// NOTE: this belongs in xFont.h. It is inline, so the compiler emits a weak
// out-of-line copy into every translation unit that calls it.
inline void xfont::render(const char* text, F32 x, F32 y) const
{
    start_render();
    irender(text, x, y);
    stop_render();
}
