#include "xHudMeter.h"

#include <types.h>

#include "xString.h"
#include "xMathInlines.h"
#include <stdio.h>

#include <math.h>

// NOTE: these two belong in headers (std::powf in <math.h>, xpow in
// xMathInlines.h). They are inline, so the compiler emits a weak out-of-line
// copy into every translation unit that calls them.
namespace std
{
    extern inline float powf(float x, float y)
    {
        return ::pow(x, y);
    }
}

inline F32 xpow(F32 x, F32 y)
{
    return std::powf(x, y);
}

namespace xhud
{
    namespace
    {
        static void add_global_tweaks()
        {
        }
    } // namespace

    meter_widget::meter_widget(const meter_asset& asset)
        : widget((xhud::asset&)asset), res((xhud::meter_asset&)asset), value(asset.start_value),
          min_value(asset.min_value), max_value(asset.max_value), end_value(asset.start_value),
          value_vel(0.0f), ping_delay(10.0f)
    {
        add_global_tweaks();
    }
} // namespace xhud

void xhud::meter_widget::set_value(F32 v)
{
    F32 dvalue;
    F32 sign;

    dvalue = v - value;
    if (dvalue < -0.01f)
    {
        if (res.sound.start_decrement != 0)
        {
            xSndPlay(res.sound.start_decrement, 1.0f, 0.0f, 0x80, 0x0, NULL, SND_CAT_GAME, 0.0f);
        }

        if (res.decrement_time < 0.01f)
        {
            set_value_immediate(v);
            return;
        }

        sign = -1.0f;
    }
    else if (dvalue > 0.01f)
    {
        if (res.sound.start_increment != 0)
        {
            xSndPlay(res.sound.start_increment, 1.0f, 0.0f, 0x80, 0x0, NULL, SND_CAT_GAME, 0.0f);
        }

        if (res.increment_time < 0.01f)
        {
            set_value_immediate(v);
            return;
        }

        sign = 1.0f;
    }
    else
    {
        set_value_immediate(v);
        return;
    }

    end_value = v;

    if (res.decrement_time == 0.0f)
    {
        printf("decrement time = 0 -- ass saved!\n");
    }

    value_vel = sign / (res.decrement_time > 1e-5f ? res.decrement_time : 1e-5f);
    value_accel = 50.0f * sign;
    pitch = 0.0f;

    if (xsqrt(2.0f * dvalue / value_accel) > 2.0f)
    {
        value_accel = 2.0f * dvalue / 4.0f;
    }
}

void xhud::meter_widget::set_value_immediate(F32 v)
{
    value = v;
    end_value = v;
    value_vel = 0.0f;
}

void xhud::meter_widget::destruct()
{
    xhud::widget::destruct();
}

U32 xhud::meter_widget::type() const
{
    static U32 myid = xStrHash(res.type_name());

    return myid;
}

bool xhud::meter_widget::is(U32 id) const
{
    bool isTheWidget = false;

    if (id == xhud::meter_widget::type() || xhud::widget::is(id))
    {
        isTheWidget = true;
    }

    return isTheWidget;
}

void xhud::meter_widget::updater(F32 dt)
{
    F32 old_value;
    F32 pitch; // This was Heavy Iron, idk why they chose a name that causes name collisions :(
    F32 min_ping_time;
    F32 dvalue;

    xhud::widget::updater(dt);

    ping_delay += dt;
    this->pitch += dt;

    if (value_vel != 0.0f)
    {
        old_value = value;

        dvalue = value_vel * dt;
        value = value + (dvalue + dt * (0.5f * value_accel * dt));
        value_vel += value_accel * dt;

        if (value_vel < 0.0f)
        {
            if (value <= end_value)
            {
                value = end_value;
                value_vel = 0.0f;
            }

            pitch = -4.0f * this->pitch;
            pitch = range_limit<F32>(pitch, -10.0f, 6.5f);
            min_ping_time = 0.05f * xpow(0.5f, 0.083333336f * pitch);

            if ((S32)value != (S32)old_value && res.sound.decrement != 0 &&
                ping_delay > min_ping_time)
            {
                ping_delay = 0.0f;
                pings.play(res.sound.decrement, 1.0f, pitch, 0x80, 0, 0, SND_CAT_GAME);
            }
        }
        else
        {
            if (value >= end_value)
            {
                value = end_value;
                value_vel = 0.0f;
            }

            pitch = 2.0f * this->pitch;
            pitch = range_limit<F32>(pitch, -10.0f, 6.5f);
            min_ping_time = 0.05f * xpow(0.5f, 0.083333336f * pitch);

            if ((S32)value != (S32)old_value && res.sound.increment != 0 &&
                ping_delay > min_ping_time)
            {
                ping_delay = 0.0f;
                pings.play(res.sound.increment, 1.0f, pitch, 0x80, 0, 0, SND_CAT_GAME);
            }
        }
    }
}

// NOTE: these belong in xSnd.h. They are template members, so the compiler
// emits a weak out-of-line copy into every translation unit that instantiates
// them.
template <S32 N>
void sound_queue<N>::play(U32 id, F32 vol, F32 pitch, U32 priority, U32 flags, U32 parentID,
                          sound_category snd_category)
{
    U32 assetID = xSndPlay(id, vol, pitch, priority, flags, parentID, snd_category, 0.0f);

    push(assetID);
}

template <S32 N> void sound_queue<N>::push(U32 id)
{
    _playing[tail] = id;

    S32 h = head;
    S32 t = tail + 1;

    if (t <= h)
    {
        t += (N + 1);
    }

    if (t - h > N)
    {
        xSndStop(_playing[h]);
        head = (h + 1) % (N + 1);
    }

    tail = t % (N + 1);
}
