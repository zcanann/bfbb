#include "xHudUnitMeter.h"
#include "xString.h"
#include "xMathInlines.h"
#include "xDebug.h"

#include <PowerPC_EABI_Support/MSL_C++/MSL_Common/Include/new.h>
#include <math.h>
#include <types.h>

// NOTE: these two belong in headers (std::fmodf in <math.h>, xfmod in
// xMathInlines.h). They are inline, so the compiler emits a weak out-of-line
// copy into every translation unit that calls them.
namespace std
{
    extern inline float fmodf(float x, float y)
    {
        return ::fmod(x, y);
    }
}

inline F32 xfmod(F32 a, F32 b)
{
    return std::fmodf(a, b);
}

namespace xhud
{
    namespace {
        F32 tweak_anim_time_delta = 0.1;
    }
}

void xhud::unit_meter_widget::load(xBase& data, xDynAsset& asset, size_t arg2)
{
    init_base(data, asset, sizeof(xBase) + sizeof(unit_meter_widget));
    unit_meter_widget* widget = (unit_meter_widget*)(&data + 1);
    new (widget) unit_meter_widget((unit_meter_asset&)asset);
}

xhud::unit_meter_widget::unit_meter_widget(const xhud::unit_meter_asset& a)
    : meter_widget(a), res(a)
{
    S32 i, j;

    anim_time = 0.0f;

    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < 6; j++)
        {
            model[j][i] = load_model(res.model[i].id);
        }
    }

    static bool registered = false;
    if (!registered)
    {
        registered = true;
        xDebugAddTweak("Temp|HUD Unit Anim Delta", &tweak_anim_time_delta, 0.0f, 10.0f, NULL, NULL, 0);
    }
}

void xhud::unit_meter_widget::destruct()
{
    meter_widget::destruct();
}

void xhud::unit_meter_widget::destroy()
{
    this->destruct();
}

U32 xhud::unit_meter_widget::type() const
{
    static U32 myid = xStrHash(((unit_meter_asset*)a)->type_name());
    return myid;
}

bool xhud::unit_meter_widget::is(U32 id) const
{
    bool isType = false;
    if (unit_meter_widget::type() == id || meter_widget::is(id))
    {
       isType = true;
    }

    return isType;
}

void xhud::unit_meter_widget::setup()
{
    widget::presetup();
}

void xhud::unit_meter_widget::update(F32 dt)
{
    meter_widget::updater(dt);

    if (!widget::visible() || this->rc.a <= (0.5f / 255.0f))
    {
        return;
    }

    anim_time += dt;

    S32 units = 0.5f + max_value;
    if (units > 6)
    {
        units = 6;
    }

    for (S32 i = 0; i < units; i++) {
        S32 which = 0;
        if ((res.fill_forward && value >= i + 1) || (!res.fill_forward && value >= units - i))
        {
            which = 1;
        }
        
        xModelInstance* m = model[i][which];
        if (m != NULL && m->Anim != NULL && !(m->Anim->Single->State->Data->Duration <= 0.0f))
        {
            F32 duration = i * tweak_anim_time_delta + anim_time;
            if (duration > m->Anim->Single->State->Data->Duration) {
                duration = xfmod(duration, m->Anim->Single->State->Data->Duration);
            }

            m->Anim->Single->Time = duration;
            xModelEval(m);
        }
    }
}

void xhud::unit_meter_widget::render()
{
    render_context unitrc = this->rc;
    
    S32 units = 0.5f + max_value;
    if (units > 6)
    {
        units = 6;
    }

    for (S32 i = 0; i < units; i++)
    {
        S32 which = 0;
        if ((res.fill_forward && value >= i + 1) || (!res.fill_forward && value >= units - i))
        {
            which = 1;
        }

        if (model[i][which] == NULL)
        {
            continue;
        }

        unitrc.loc.x = rc.loc.x + res.model[which].loc.x + res.offset.x * i;
        unitrc.loc.y = rc.loc.y + res.model[which].loc.y + res.offset.y * i;
        unitrc.loc.z = rc.loc.z + res.model[which].loc.z + res.offset.z * i;
        unitrc.size.x = rc.size.x * res.model[which].size.x;
        unitrc.size.y = rc.size.y * res.model[which].size.y;
        unitrc.size.z = rc.size.z * res.model[which].size.z;

        render_model(*model[i][which], unitrc);
    }
}
