#include "xHud.h"
#include "xDebug.h"
#include "xEvent.h"
#include "xMath.h"
#include "xMathInlines.h"
#include "xstransvc.h"
#include "zGlobals.h"
#include "xHudText.h"
#include "xHudModel.h"
#include "xHudUnitMeter.h"
#include "xHudFontMeter.h"

#include "zEnt.h"

#include <PowerPC_EABI_Support\MSL_C++\MSL_Common\Include\new.h>
#include <types.h>

#define lengthof(x) (sizeof(x) / sizeof((x)[0]))

// NOTE: retail's xHud.o carries these two file-scope statics in .rodata at
// offsets 0x30 and 0x40, both 16 bytes of { 0.0f, 0.0f, 1.0f, 1.0f }. They are
// unmangled, so they live outside `namespace xhud`, and nothing in this
// translation unit references them - their only consumers were deadstripped.
// The byte contents are certain; the exact type is inferred.
// These structs were used in deadstripped functions.
// This function is here to force the symbols to be linked.
void __deadstripped_xHud_head()
{
    const char _405[0x0C] = {};
    const char _406[0x0C] = {};
    const char _410[0x0C] = {};
    const char _441[0x0C] = {};
}

static const basic_rect<F32> screen_bounds = { 0.0f, 0.0f, 1.0f, 1.0f };
static const basic_rect<F32> default_adjust = { 0.0f, 0.0f, 1.0f, 1.0f };

void __deadstripped_xHud_rects()
{
    const char _594[0x28] = {};
    const char _595[0x28] = {};
    const char _596[0x28] = {};
    const char _597[0x28] = {};
    const char _598[0x28] = {};
    const char _599[0x28] = {};
    const char _600[0x28] = {};
    const char _713[0x10] = {};
}

namespace xhud
{

    block_allocator* block_allocator::_head_alloc;
    static bool inited;

    void block_allocator::flush_all()
    {
        for (block_allocator* allocator = _head_alloc; allocator != NULL;
             allocator = allocator->_next_alloc)
        {
            allocator->flush();
        }
    }

    block_allocator::block_allocator(U32 a0, U32 a1)
    {
        _block_size = ALIGN(a0, sizeof(holder)) + sizeof(holder);
        _top = NULL;
        _next_alloc = _head_alloc;
        _head_alloc = this;
        set_increment(a1);
    }

    void block_allocator::set_increment(U32 a0)
    {
        _alloc_size = _block_size * a0;
    }

    void block_allocator::size_reserve(U32 size)
    {
        holder* block = (holder*)xMemAllocSize(size);
        holder* end = (holder*)((U32)block + size);
        for (; block < end; block = (holder*)((U32)block + _block_size))
        {
            block->_next = _top;
            _top = block;
        }
    }

    void* block_allocator::alloc()
    {
        if (_top == NULL)
        {
            size_reserve(_alloc_size);
        }

        holder* block = _top;
        _top = block->_next;
        return block + 1;
    }

    void block_allocator::free(void* ptr)
    {
        holder* block = (holder*)ptr - 1;
        block->_next = _top;
        _top = block;
    }

    void block_allocator::flush()
    {
        _top = NULL;
    }

    block_allocator* widget::motive_allocator()
    {
        static block_allocator ba(40, 16);
        return &ba;
    }

    void init()
    {
        if (!inited)
        {
            inited = true;
        }
        else
        {
            widget::disable_all(true);
        }
    }

    void setup()
    {
        widget::setup_all();
    }

    void destroy()
    {
        xDebugRemoveTweak("HUD");
        widget::disable_all(true);
        block_allocator::flush_all();
    }

    void update(F32 dt)
    {
        widget::update_all(dt);
    }

    void render()
    {
        if (inited)
        {
            widget::render_all();
        }
    }

    widget::widget(const asset& asset)
    {
        a = &asset;
        _motive_top = NULL;
        _motive_temp = NULL;
        _motive_temp_tail = NULL;
        flag.visible = 1;
        flag.enabled = 0;
        rc.loc = asset.loc;
        rc.size = asset.size;
        rc.r = rc.g = rc.b = rc.a = 1.0f;
        start_rc = rc;
    }

    void widget::init_base(xBase& data, const xBaseAsset& asset, unsigned long chunk_size)
    {
        xBaseInit(&data, (xBaseAsset*)&asset);
        data.eventFunc = cb_dispatch;
        if (data.linkCount != 0)
        {
            data.link = (xLinkAsset*)((U32)&asset + chunk_size);
        }
    }

    void widget::destruct()
    {
        disable();
    }

    void widget::presetup()
    {
        activity = ACT_NONE;
        add_tweaks();
    }

    void widget::updater(F32 dt)
    {
        _motive_temp_tail = &_motive_temp;
        motive_node** itp = &_motive_top;
        motive_node* it = _motive_top;
        while (it != NULL)
        {
            bool unk = it->m.update(*this, dt);
            if (!unk)
            {
                *itp = it->next;
                motive_allocator()->free(it);
            }
            else
            {
                itp = &it->next;
            }
            it = *itp;
        }

        if (_motive_temp != NULL)
        {
            *_motive_temp_tail = _motive_top;
            _motive_top = _motive_temp;
            _motive_temp = NULL;
        }

        _motive_temp_tail = NULL;
        if (_motive_top == NULL)
        {
            activity = ACT_NONE;
        }
    }

    void widget::dispatcher(xBase*, U32 event, const F32* toParam, xBase*)
    {
        switch (event)
        {
        case eEventEnable:
            enable();
            break;
        case eEventDisable:
            disable();
            break;
        case eEventVisible:
        case eEventFastVisible:
            flag.visible = 1;
            break;
        case eEventInvisible:
        case eEventFastInvisible:
            flag.visible = 0;
            break;
        case eEventDispatcher_ShowHud:
            show();
            break;
        case eEventDispatcher_HideHud:
            hide();
            break;
        }
    }

    // Equivalent: scheduling
    U32 widget::type() const
    {
        static U32 myid = xStrHash(a->type_name());
        return myid;
    }

    bool widget::is(U32 id) const
    {
        return id == widget::type();
    }

    void widget::show()
    {
        clear_motives();

        activity = ACT_SHOW;

        F32 dx = start_rc.loc.x - rc.loc.x;
        F32 dy = start_rc.loc.y - rc.loc.y;
        F32 fVar1 = dx * dx + dy * dy;
        if (fVar1 <= 0.000000009999999f)
        {
            rc.loc = start_rc.loc;
            rc.a = start_rc.a;
        }
        else
        {
            F32 dVar4 = xsqrt(fVar1);
            F32 vy = 10.0f * dy;
            fVar1 = 10.0f * dx;
            F32 dVar5 = (-(fVar1 * fVar1 + (vy * vy)) / (2.0f * dVar4));

            add_motive(motive(&rc.loc.x, fVar1, dx, (dVar5 * dx) / dVar4,
                              accelerate_motive_update, NULL));

            add_motive(motive(&rc.loc.y, vy, dy, (dVar5 * dy) / dVar4,
                              accelerate_motive_update, NULL));

            fVar1 = start_rc.a - rc.a;
            add_motive(motive(&rc.a, 3.0f * fVar1, fVar1, 0.0f, linear_motive_update, NULL));
        }
    }

    // Pushes the widget off whichever screen edge its home position is nearest,
    // fading it out as it goes. Local names are DWARF's (dwarf/SB/Core/x/xHud.cpp
    // lists x y sx sy cx cy tx ty tcx tcy acx acy dist vx vy ex ey); which name
    // belongs to which quantity is inferred from the shape of the code.
    void widget::hide()
    {
        activity = ACT_HIDE;

        F32 x = start_rc.loc.x;
        F32 y = start_rc.loc.y;
        F32 sx = start_rc.size.x;
        F32 sy = start_rc.size.y;

        // The widget's centre, measured from the centre of the screen.
        F32 cx = (x - 0.5f) + 0.5f * sx;
        F32 cy = (y - 0.5f) + 0.5f * sy;
        F32 acx = iabs(cx);
        F32 acy = iabs(cy);
        if (iabs(acx + acy) <= 0.0001f)
        {
            // Dead centre: there is no edge to slide towards, so just blank it.
            rc.a = 0.0f;
        }
        else
        {
            // Push along the dominant axis until that axis clears the edge, and
            // scale the other axis by the same ratio so the widget leaves along
            // the line through the screen centre.
            F32 tcx;
            F32 tcy;
            if (acx > acy)
            {
                F32 ex;
                if (cx >= 0.0f)
                {
                    ex = 0.5f + sx;
                }
                else
                {
                    ex = -0.5f - sx;
                }
                tcx = ex;
                tcy = (ex * cy) / cx;
            }
            else
            {
                F32 ey;
                if (cy >= 0.0f)
                {
                    ey = 0.5f + sy;
                }
                else
                {
                    ey = -0.5f - sy;
                }
                tcy = ey;
                tcx = (ey * cx) / cy;
            }

            F32 tx = 0.5f + (tcx - 0.5f * sx);
            F32 ty = 0.5f + (tcy - 0.5f * sy);
            F32 vx = tx - rc.loc.x;
            F32 vy = ty - rc.loc.y;
            F32 dist = xsqrt(vx * vx + vy * vy);

            add_motive(motive(&rc.loc.x, 0.0f, vx, vx * dist, accelerate_motive_update, NULL));
            add_motive(motive(&rc.loc.y, 0.0f, vy, vy * dist, accelerate_motive_update, NULL));

            // The alpha motive runs the whole way down to zero, so its
            // max_offset is -rc.a - the mirror of show()'s `start_rc.a - rc.a`.
            add_motive(motive(&rc.a, 0.4f * -rc.a, -rc.a, 0.0f, linear_motive_update, NULL));
        }
    }

    namespace
    {

        void fp_setup(widget& w)
        {
            w.setup();
        }

        void fp_render(widget& w)
        {
            if (w.visible())
            {
                w.render();
            }
        }

        const struct
        {
            U8 widget_type;
            U32 widget_size;
        } known_types[] = {
            // The size is the stride of globals.sceneCur->baseList[type], i.e.
            // sizeof(xBase) plus the widget that follows it - see for_each()
            // below, which steps by it and offsets past the xBase.
            { eBaseTypeHUD_model, sizeof(xBase) + sizeof(model_widget) },
            { eBaseTypeHUD_unit_meter, sizeof(xBase) + sizeof(unit_meter_widget) },
            { eBaseTypeHUD_font_meter, sizeof(xBase) + sizeof(font_meter_widget) },
            { eBaseTypeHUD_text, sizeof(xBase) + sizeof(text_widget) },
        };

        struct functor_disable
        {
            functor_disable(bool b)
            {
                destroy_widgets = b;
            }

            void operator()(xhud::widget& widget)
            {
                widget.disable();
                if (destroy_widgets)
                {
                    widget.destroy();
                }
            }

            U8 destroy_widgets;
        };

        struct functor_update
        {
            functor_update(F32 dt)
            {
                delta_time = dt;
            }

            void operator()(xhud::widget& widget)
            {
                if (widget.enabled())
                    widget.update(delta_time);
            }

            F32 delta_time;
        };

        template <class F> void for_each(U8 widget_type, U32 type_size, F f)
        {
            U32 count = globals.sceneCur->baseCount[widget_type];
            U8* it = (U8*)globals.sceneCur->baseList[widget_type];
            U8* end = it + count * type_size;
            while (it != end)
            {
                f(*(widget*)(it + sizeof(xBase)));
                it += type_size;
            }
        }

        void render_one_model(xModelInstance& model, F32 alpha, const basic_rect<F32>& rect,
                              const xVec3& from, const xVec3& to, const xMat4x3& frame)
        {
            xModelSetMaterialAlpha(&model, 255.0f * alpha + 0.5f);
            xModelSetFrame(&model, &frame);
            xModelRender2D(model, rect, from, to);
        }

    } // namespace

    void widget::debug_render()
    {
    }

    void widget::setup_all()
    {
        for (U32 i = 0; i < lengthof(known_types); ++i)
        {
            for_each(known_types[i].widget_type, known_types[i].widget_size, fp_setup);
        }
    }

    S32 widget::cb_dispatch(xBase* from, xBase* to, U32 event, const F32* argf,
                            xBase* argw)
    {
        widget* w = (widget*)(to + 1);

        w->dispatch(from, event, argf, argw);

        return 1;
    }

    void widget::render_all()
    {
        debug_render();
        for (U32 i = 0; i < lengthof(known_types); ++i)
        {
            for_each(known_types[i].widget_type, known_types[i].widget_size, fp_render);
        }
    }

    void widget::update_all(F32 dt)
    {
        functor_update func(dt);
        for (U32 i = 0; i < lengthof(known_types); ++i)
        {
            for_each(known_types[i].widget_type, known_types[i].widget_size, func);
        }
    }

    void widget::disable_all(bool st)
    {
        functor_disable func(st);
        for (U32 i = 0; i < lengthof(known_types); ++i)
        {
            for_each(known_types[i].widget_type, known_types[i].widget_size, func);
        }
    }

    void widget::add_motive(const motive& m)
    {
        motive_node* n = (motive_node*)motive_allocator()->alloc();
        new (n) motive(m);

        if (_motive_temp_tail == NULL)
        {
            n->next = _motive_top;
            _motive_top = n;
        }
        else
        {
            if (_motive_temp == NULL)
            {
                _motive_temp_tail = &n->next;
            }
            n->next = _motive_temp;
            _motive_temp = n;
        }
    }

    void widget::clear_motives()
    {
        activity = ACT_NONE;
        motive_node* node = _motive_top;
        while (node != NULL)
        {
            node->m.finish();
            _motive_top = node->next;
            motive_allocator()->free(node);
            node = _motive_top;
        }
    }

    void widget::clear_motives(bool (*fp_update)(widget&, motive&, F32), void* context)
    {
        motive_node** itp = &_motive_top;
        motive_node* it = _motive_top;

        while (it != NULL)
        {
            if (it->m.fp_update == fp_update && it->m.context == context)
            {
                *itp = it->next;
                motive_allocator()->free(it);
            }
            else
            {
                itp = &it->next;
            }
            it = *itp;
        }

        if (_motive_top == NULL)
        {
            activity = ACT_NONE;
        }
    }

    bool linear_motive_update(widget& w, motive& m, F32 dt)
    {
        F32 diff = dt * m.delta;
        F32 remaining = m.max_offset - m.offset;
        if ((diff >= 0.0f && diff >= remaining) || (diff < 0.0f && diff <= remaining))
        {
            *m.value += remaining;
            m.offset = m.max_offset;
            return false;
        }
        else
        {
            *m.value += diff;
            m.offset += diff;
            return true;
        }
    }

    // Equivalent: regalloc
    bool accelerate_motive_update(widget& w, motive& m, F32 dt)
    {
        F32 remaining;
        F32 diff;
        F32 delta;

        diff = 0.5f * m.accel;
        delta = m.delta;
        m.delta = dt * m.accel + delta;
        delta *= dt;
        diff *= dt;
        diff = dt * diff + delta;

        remaining = m.max_offset - m.offset;

        if ((diff >= 0.0f && diff >= remaining) || (diff < 0.0f && diff <= remaining))
        {
            *m.value += remaining;
            m.offset = m.max_offset;
            return false;
        }
        else
        {
            *m.value += diff;
            m.offset += diff;
            return true;
        }
    }

    bool shake_motive_update(widget& w, motive& m, F32 dt)
    {
        static const float mult[4] = { -1.0f, -1.0f, 1.0f, 1.0f };

        *((U32*)&m.context) += 1;
        U32 i = *((U32*)&m.context);
        if (i > 0x32)
        {
            m.context = 0;
            *m.value -= m.offset;
            m.offset = 0.0f;
            return false;
        }

        F32 diff = m.delta * mult[i & 0x3];
        if ((i & 0x3) == 0)
        {
            m.delta = m.delta * m.accel;
        }

        *m.value += diff;
        m.offset += diff;

        return true;
    }

    bool delay_motive_update(widget& w, motive& m, F32 dt)
    {
        m.offset += dt;
        if (m.max_offset - m.offset < 0.0f)
        {
            ((motive_proc*)m.context)(w, m, dt);
            return false;
        }
        return true;
    }

    void render_model(xModelInstance& m, const xhud::render_context& rc)
    {
        basic_rect<F32> r = { 0 };
        r.x = rc.loc.x;
        r.y = rc.loc.y;
        r.w = rc.size.x;
        r.h = rc.size.y;

        xVec3 from = { 0, 0, 1 };
        xVec3 to = { 0, 0, -rc.loc.z };

        xMat4x3 frame;
        xMat3x3Euler(&frame, rc.rot.x, rc.rot.y, rc.rot.z);
        frame.right *= (1.0f + rc.loc.z);
        frame.up *= (1.0f + rc.loc.z);
        frame.at *= 0.0099999998f;
        frame.pos.z = 0.0f;
        frame.pos.y = 0.0f;
        frame.pos.x = 0.0f;
        frame.flags = 0;

        for (xModelInstance* model = &m; model; model = model->Next)
        {
            render_one_model(*model, rc.a, r, from, to, frame);
        }
    }

    void __deadstripped_xHud()
    {
        // "%d" was used in a deadstripped function. This function forces it to be used.
        xStrHash("%d");
    }

    xModelInstance* load_model(U32 id)
    {
        U32 bufsize;
        void* info = xSTFindAsset(xStrHashCat(id, ".minf"), &bufsize); // xModelAssetInfo*
        if (info != NULL)
        {
            return zEntRecurseModelInfo(info, NULL);
        }

        info = xSTFindAsset(id, &bufsize); // RpAtomic*
        if (info == NULL)
        {
            info = xSTFindAsset(xStrHashCat(id, ".dff"), &bufsize); // RpAtomic*
        }
        if (info == NULL)
        {
            return NULL;
        }

        return xModelInstanceAlloc((RpAtomic*)info, NULL, 0, 0, NULL);
    }

} // namespace xhud

// NOTE: this belongs in <new.h>. It is inline, so the compiler emits a weak
// out-of-line copy into every translation unit that placement-news.
inline void* operator new(size_t, void* ptr) throw()
{
    return ptr;
}

// NOTE: the original is a weak symbol, i.e. an inline in xColor.h, but it is
// emitted only here and nothing in this object references it. Defining it here
// reproduces the code exactly; it just makes the symbol strong.
iColor_tag xColorFromRGBA(U8 r, U8 g, U8 b, U8 a)
{
    iColor_tag color;

    color.r = r;
    color.g = g;
    color.b = b;
    color.a = a;

    return color;
}
